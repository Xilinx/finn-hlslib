/******************************************************************************
 *  Copyright (c) 2022, Xilinx, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1.  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2.  Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *  3.  Neither the name of the copyright holder nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 * @brief	Normalization layers.
 * @author	Thomas B. Preusser <thomas.preusser@amd.com>
 *******************************************************************************/
#ifndef NORMALIZE_HPP
#define NORMALIZE_HPP

#include <ap_int.h>
#include <hls_stream.h>
#include <hls_math.h>
#include <functional>

#include "utils.hpp"

/**
 * Subjects a feature map stream [FM_SIZE x CHANNELS] to a channelwise normalization
 * using the coefficients obtained by calling f() CHANNELS times for each input
 * feature map.
 *
 * Type Requirements:
 *	f: void -> TC
 *	g: TC x TI -> TO
 */
template<
	unsigned  FM_SIZE,					// Feature Map Size
	unsigned  CHANNELS,					// Channels per Feature Map Pixel
	typename  G = std::multiplies<>,	// Scaling Function
	typename  F,						// Coefficient Adjustment Function
	typename  TI,						// Input Feature Type
	typename  TO						// Output Feature Type
>
void normalize(
	hls::stream<TI> &src,
	hls::stream<TO> &dst,
	F &&f,
	G &&g = G()
) {
#pragma HLS dataflow disable_start_propagation
	decltype(f())  coeff_buf[CHANNELS];
	for(unsigned  c = 0; c < CHANNELS; c++) {
#pragma HLS pipeline II=1 style=flp
		coeff_buf[c] = f();
	}

	for(unsigned  i = 0; i < FM_SIZE; i++) {
		for(unsigned  c = 0; c < CHANNELS; c++) {
#pragma HLS pipeline II=1 style=flp
			TI const  x = src.read();
			TO const  y = g(coeff_buf[c], x);
			dst.write(y);
		}
	}

} // normalize()

//---------------------------------------------------------------------------
// MaxNorm

/**
 * MaxNorm helper extracting the maximum from every FM_SIZE forwarded values.
 */
template<
	unsigned  FM_SIZE,
	int  W
>
static void max_norm_extract(
	hls::stream<ap_uint<W>> &src_val,
	hls::stream<ap_uint<W>> &dst_val,
	hls::stream<ap_uint<W>> &dst_max
) {
	ap_uint<W>  max;
	for(unsigned  i = 0; i < FM_SIZE; i++) {
#pragma HLS pipeline II=1 style=flp
		auto const  x = src_val.read();
		max = std::max(i? max : ap_uint<W>(1) /* prevent division by zero */, x);
		dst_val.write(x);
		if(i == FM_SIZE-1)  dst_max.write(max);
	}
}

/**
 * MaxNorm helper normalizing every FM_SIZE forwarded values.
 */
template<
	unsigned  FM_SIZE,
	int  WI,
	int  WO
>
static void max_norm_adjust(
	hls::stream<ap_uint<WI>> &src_val,
	hls::stream<ap_uint<WI>> &src_max,
	hls::stream<ap_uint<WO>> &dst_val,
	float  max
) {
#pragma HLS function_instantiate variable=max
	float  coeff;
	ap_uint<WI>  x;
	for(unsigned  i = 0; i < FM_SIZE; i++) {
#pragma HLS pipeline II=1 style=flp
		if(i == 0)  coeff = max / src_max.read();
		src_val.read_nb(x);
		dst_val.write(ap_uint<WO>(int(rintf(coeff * x))));
	}
}

/**
 * Quantized maximum normalization over input vectors of length FM_SIZE
 * into the numeric range of the output type `ap_uint<WO>`:
 *
 *	x_i -> round( (2^WO-1) * SCALE * x_i / max{x_j | j=0:FM_SIZE} )
 */
template<
	unsigned  FM_SIZE,	// Vector length
	int  WI,			// Input Precision
	int  WO				// Output Precision
>
void max_norm(
	hls::stream<ap_uint<WI>> &src,
	hls::stream<ap_uint<WO>> &dst,
	float const  scale = 1.0f
) {
#pragma HLS function_instantiate variable=scale
#pragma HLS dataflow disable_start_propagation
//	assert((0.0f < scale) && (scale <= 1.0f));
	float const  max = ((1<<WO)-1) * scale;

	hls::stream<ap_uint<WI>>  buf_val;
	hls::stream<ap_uint<WI>>  buf_max;
#pragma HLS stream variable=buf_val depth=FM_SIZE
#pragma HLS stream variable=buf_max depth=2
	max_norm_extract<FM_SIZE>(src, buf_val, buf_max);
	max_norm_adjust<FM_SIZE>(buf_val, buf_max, dst, max);

} // max_norm()


template<
	unsigned  FM_SIZE,
	typename  TI	// input type must be comparable and convertible to float
>
void softmax(
	hls::stream<TI>    &src,
	hls::stream<float> &dst
) {
	TI  max_val;
	ap_uint<clog2(FM_SIZE+1)>  max_cnt = 0;

	struct buf_s {
		TI     xi;
		float  xx;
	};
	hls::stream<buf_s>  buf;
#pragma HLS stream variable=buf depth=FM_SIZE
	float  total = 0.0f;

	for(unsigned  i = 0; i < FM_SIZE; i++) {
#pragma HLS pipeline II=1 style=flp
		TI const  x = src.read();
		if((max_cnt == 0) || (max_val < x)) {
			max_val = x;
			max_cnt = 1;
		}
		else if(max_val == x) {
			max_cnt++;
		}

		float const xx = hls::exp(float(x));
		buf.write(buf_s{ x, xx });
		total += xx;
	}

	bool const  ovf = hls::isinf(total);
	for(unsigned  i = 0; i < FM_SIZE; i++) {
#pragma HLS pipeline II=1 style=flp
		buf_s const  x = buf.read();

		float  a;
		float  d;
		if(ovf) {
			a = x.xi == max_val? 1.0f : 0.0f;
			d = max_cnt;
		}
		else {
			a = x.xx;
			d = total;
		}
		dst.write(a/d);
	}

} // softmax()

#endif
