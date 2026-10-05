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
 *
 *  Authors: Giulio Gambardella <giuliog@xilinx.com>
 *           Thomas B. Preusser <thomas.preusser@amd.com>
 *           Christoph Doehring <cdoehrin@xilinx.com>
 *
 *  \file mvau.hpp
 *
 *  Matrix-Vector-Activation Unit — function-pipelined implementation.
 *
 *  The function body is the pipeline stage: no loop, no reps parameter.
 *  Under ap_ctrl_none, it runs every cycle, processing one (sf, nf) fold
 *  beat per invocation. Data flow self-regulates via stream back-pressure.
 *
 *******************************************************************************/

#ifndef MVAU_HPP
#define MVAU_HPP

#include <hls_stream.h>
#include <hls_vector.h>

#include "activations.hpp"


//---------------------------------------------------------------------------
// MVU with embedded (internal) weights
//
// Weights are stored in a local array, accessed by fold index.
// The activation class must provide:
//   - TAcc init(unsigned nf, unsigned pe)
//   - TO   activate(unsigned nf, unsigned pe, TAcc const &accu)
//
template<
	unsigned  MatrixW,   // input features
	unsigned  MatrixH,   // output features
	unsigned  SIMD,      // input parallelism
	unsigned  PE,        // output parallelism
	typename  TW,        // weight element type
	typename  TI,        // input activation element type
	typename  TO,        // output element type
	typename  TW_arr,    // weight array type: operator[](tile) -> vector<vector<TW, SIMD>, PE>-like
	typename  TA         // activation class
>
void Matrix_Vector_Activate_Batch(
	hls::stream<hls::vector<TI, SIMD>>  &in,
	hls::stream<hls::vector<TO, PE>>    &out,
	TW_arr const                        &weights,
	TA const                            &activation
) {
#pragma HLS pipeline II=1 style=flp

	constexpr unsigned  NF = MatrixH / PE;
	constexpr unsigned  SF = MatrixW / SIMD;

	using TAcc = decltype(activation.init(0, 0));

	static TAcc  accu[PE];
	static hls::vector<TI, SIMD>  inputBuf[SF];
	static unsigned  sf = 0;
	static unsigned  nf = 0;
	static unsigned  nf_out = 0;
	static bool  push = false;
#pragma HLS array_partition variable=accu complete
#pragma HLS array_partition variable=inputBuf complete
#pragma HLS reset variable=accu off
#pragma HLS reset variable=inputBuf off
#pragma HLS reset variable=sf
#pragma HLS reset variable=nf
#pragma HLS reset variable=nf_out off
#pragma HLS reset variable=push

	// Output phase — one cycle after the last SF beat
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

	// Compute phase — non-blocking: only proceed when data is available
	// For nf==0: need fresh input from stream; for nf>0: reuse buffer (always ready)
	if(nf == 0? !in.empty() : true) {
		hls::vector<TI, SIMD>  inElem;
		if(nf == 0) {
			inElem = in.read();
			inputBuf[sf] = inElem;
		}
		else {
			inElem = inputBuf[sf];
		}

		// Initialize accumulators at the start of each neuron fold
		if(sf == 0) {
			for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
				accu[pe] = activation.init(nf, pe);
			}
		}

		// MAC — read weights from embedded array, broadcast input to all PE
		unsigned const  tile = nf * SF + sf;
		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
			auto const  w = weights[tile][pe];
			for(unsigned  i = 0; i < SIMD; i++) {
#pragma HLS unroll
				accu[pe] += w[i] * inElem[i];
			}
		}

		// Advance fold counters (modular wrap)
		if(++sf == SF) {
			sf = 0;
			nf_out = nf;
			push = true;
			if(++nf == NF) {
				nf = 0;
			}
		}
	}
}


//---------------------------------------------------------------------------
// MVU with streaming (decoupled/external) weights
//
// Weights arrive via a stream, one tile per beat.
//
template<
	unsigned  MatrixW,
	unsigned  MatrixH,
	unsigned  SIMD,
	unsigned  PE,
	typename  TW,
	typename  TI,
	typename  TO,
	typename  TA
>
void Matrix_Vector_Activate_Stream_Batch(
	hls::stream<hls::vector<TI, SIMD>>                  &in,
	hls::stream<hls::vector<TO, PE>>                     &out,
	hls::stream<hls::vector<hls::vector<TW, SIMD>, PE>> &weights,
	TA const                                             &activation
) {
#pragma HLS pipeline II=1 style=flp

	constexpr unsigned  NF = MatrixH / PE;
	constexpr unsigned  SF = MatrixW / SIMD;

	using TAcc = decltype(activation.init(0, 0));

	static TAcc  accu[PE];
	static hls::vector<TI, SIMD>  inputBuf[SF];
	static unsigned  sf = 0;
	static unsigned  nf = 0;
	static unsigned  nf_out = 0;
	static bool  push = false;
#pragma HLS array_partition variable=accu complete
#pragma HLS array_partition variable=inputBuf complete
#pragma HLS reset variable=accu off
#pragma HLS reset variable=inputBuf off
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

	// Compute phase — non-blocking: need weights AND (fresh input or reuse)
	if(!weights.empty() && (nf == 0? !in.empty() : true)) {
		hls::vector<TI, SIMD>  inElem;
		if(nf == 0) {
			inElem = in.read();
			inputBuf[sf] = inElem;
		}
		else {
			inElem = inputBuf[sf];
		}

		// Initialize accumulators
		if(sf == 0) {
			for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
				accu[pe] = activation.init(nf, pe);
			}
		}

		// MAC — read weights from stream, broadcast input to all PE
		auto const  ww = weights.read();
		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
			auto const  w = ww[pe];
			for(unsigned  i = 0; i < SIMD; i++) {
#pragma HLS unroll
				accu[pe] += w[i] * inElem[i];
			}
		}

		// Advance fold counters
		if(++sf == SF) {
			sf = 0;
			nf_out = nf;
			push = true;
			if(++nf == NF) {
				nf = 0;
			}
		}
	}
}

#endif
