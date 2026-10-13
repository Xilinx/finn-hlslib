/******************************************************************************
 *  Copyright (c) Xilinx, Inc.
 *  Copyright (C) Advanced Micro Devices, Inc.
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
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************/

/*******************************************************************************
 * @brief	Vector-Vector-Activation Unit (depthwise separable convolutions).
 * @author	Giulio Gambardella <giuliog@xilinx.com>
 * @author	Thomas B. Preusser <thomas.preusser@amd.com>
 *
 * @description
 *  Key difference from MVU: each PE processes its own dedicated input
 *  activation slice (no input broadcasting/reuse across neuron folds).
 *******************************************************************************/

#ifndef VVAU_HPP
#define VVAU_HPP

#include <hls_stream.h>
#include <hls_vector.h>

#include "activations.hpp"
#include "mac.hpp"
#include "tensor.hpp"


//---------------------------------------------------------------------------
// VVU with embedded (internal) weights
//
template<
	unsigned  Channels,  // total output channels
	unsigned  Kernel_2,  // kernel elements (k_h * k_w)
	unsigned  SIMD,      // input parallelism (kernel elements per cycle)
	unsigned  PE,        // output parallelism (channels per cycle)
	typename  TW,        // weight element type
	typename  TI,        // input activation element type
	typename  TO,        // output element type
	typename  TW_arr,    // weight array type: operator[](tile) -> vector<vector<TW, SIMD>, PE>-like
	typename  TA,        // activation class
	typename  TR = ap_resource_dflt  // compute resource
>
void Vector_Vector_Activate_Batch(
	hls::stream<hls::vector<hls::vector<TI, SIMD>, PE>> &in,
	hls::stream<hls::vector<TO, PE>>                    &out,
	TW_arr const                                        &weights,
	TA const                                            &activation,
	TR const                                            &res = {}
) {
#pragma HLS pipeline II=1 style=flp

	constexpr unsigned  NF = Channels / PE;
	constexpr unsigned  SF = Kernel_2 / SIMD;

	using TAcc = decltype(activation.init(0, 0));

	static TAcc  accu[PE];
	static unsigned  sf = 0;
	static unsigned  nf = 0;
	static unsigned  nf_out = 0;
	static bool  push = false;
#pragma HLS array_partition variable=accu complete
#pragma HLS reset variable=accu off
#pragma HLS reset variable=sf
#pragma HLS reset variable=nf
#pragma HLS reset variable=nf_out off
#pragma HLS reset variable=push

	// Output phase
	if(push) {
		hls::vector<TO, PE>  y;
		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
			y[pe] = activation.activate(nf_out, pe, accu[pe]);
			accu[pe] = 0;
		}
		out.write(y);
		push = false;
	}

	// Compute phase — non-blocking: only proceed when input is available
	if(!in.empty()) {
		auto const  inElem = in.read();

		// Initialize accumulators
		if(sf == 0) {
			for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
				accu[pe] = activation.init(nf, pe);
			}
		}

		// MAC — each PE gets its own input activation slice
		unsigned const  tile = nf * SF + sf;
		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
			auto const  w = weights[tile][pe];
			auto const  a = inElem[pe];
			for(unsigned  i = 0; i < SIMD; i++) {
#pragma HLS unroll
				mac(accu[pe], w[i], a[i], res);
			}
		}

		// Advance fold counters
		if(++sf == SF) {
			sf = 0;
			nf_out = nf;
			push = true;
			if(++nf == NF)  nf = 0;
		}
	}
}


//---------------------------------------------------------------------------
// VVU with streaming (decoupled/external) weights
//
template<
	unsigned  Channels,
	unsigned  Kernel_2,
	unsigned  SIMD,
	unsigned  PE,
	typename  TW,
	typename  TI,
	typename  TO,
	typename  TA,
	typename  TR = ap_resource_dflt
>
void Vector_Vector_Activate_Stream_Batch(
	hls::stream<hls::vector<hls::vector<TI, SIMD>, PE>> &in,
	hls::stream<hls::vector<TO, PE>>                    &out,
	hls::stream<tensor<TW, PE, SIMD>>                   &weights,
	TA const                                            &activation,
	TR const                                            &res = {}
) {
#pragma HLS pipeline II=1 style=flp

	constexpr unsigned  NF = Channels / PE;
	constexpr unsigned  SF = Kernel_2 / SIMD;

	using TAcc = decltype(activation.init(0, 0));

	static TAcc  accu[PE];
	static unsigned  sf = 0;
	static unsigned  nf = 0;
	static unsigned  nf_out = 0;
	static bool  push = false;
#pragma HLS array_partition variable=accu complete
#pragma HLS reset variable=accu off
#pragma HLS reset variable=sf
#pragma HLS reset variable=nf
#pragma HLS reset variable=nf_out off
#pragma HLS reset variable=push

	// Output phase
	if(push) {
		hls::vector<TO, PE>  y;
		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
			y[pe] = activation.activate(nf_out, pe, accu[pe]);
			accu[pe] = 0;
		}
		out.write(y);
		push = false;
	}

	// Compute phase — non-blocking: need both input and weights
	if(!in.empty() && !weights.empty()) {
		auto const  inElem = in.read();
		auto const  ww = weights.read();

		// Initialize accumulators
		if(sf == 0) {
			for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
				accu[pe] = activation.init(nf, pe);
			}
		}

		// MAC
		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
			auto const  w = ww[pe];
			auto const  a = inElem[pe];
			for(unsigned  i = 0; i < SIMD; i++) {
#pragma HLS unroll
				mac(accu[pe], w[i], a[i], res);
			}
		}

		// Advance fold counters
		if(++sf == SF) {
			sf = 0;
			nf_out = nf;
			push = true;
			if(++nf == NF)  nf = 0;
		}
	}
}

#endif
