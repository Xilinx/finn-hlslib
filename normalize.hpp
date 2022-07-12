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

/**
 * Quantized maximum normalization over input vectors of length FM_SIZE
 * into the numeric range of the output type `ap_uint<WO>`:
 *
 *	x_i -> round( (2^WO-1) * x_i / max{x_j | j=0:FM_SIZE} )
 */
template<
	unsigned  FM_SIZE,		// Vector length
	unsigned  NORMAX = 0,	// Value of normalized maximum: 0 -> 2^WO-1
	int  WI,				// Input Precision
	int  WO					// Output Precision
>
void max_norm(
	hls::stream<ap_uint<WI>> &src,
	hls::stream<ap_uint<WO>> &dst
) {
	static_assert(clog2(1+NORMAX) <= WO, "Specified normalized maximum exceeds output range");
	static ap_uint<WO> const  MAX { NORMAX? NORMAX : -1u };

#pragma HLS dataflow disable_start_propagation
	hls::stream<ap_uint<WI>>  buffer;
#pragma HLS stream variable=buffer depth=FM_SIZE

	ap_uint<WI>  max = 1;	// Prevent division by zero
	for(unsigned  i = 0; i < FM_SIZE; i++) {
#pragma HLS pipeline II=1 style=flp
		auto const  x = src.read();
		max = std::max(max, x);
		buffer.write(x);
	}
	normalize<FM_SIZE, 1>(
		buffer, dst,
		[max]() -> ap_uint<WI+WO+1> {
#pragma HLS inline
// @todo Force a LUT implementation for low precisions (8 bits and fewer) instead of true division.
			ap_uint<WI+WO+2> const  d = ap_uint<WI+WO+2>((MAX, ap_uint<WI+2>(0))) / max;
			return  d(WI+WO+1, 1) + d[0];
		},
		[](ap_uint<WI+WO+1> const &scale, ap_uint<WI> const &x) -> ap_uint<WO> {
#pragma HLS inline
			ap_uint<WO+WI+1> const  p = scale*x;
			return  p(WO+WI, WI+1) + p[WI];
		}
	);

} // max_norm()

#endif
