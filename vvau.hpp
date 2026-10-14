/******************************************************************************
 *  Copyright (c) 2019, Xilinx, Inc.
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
 * @brief	Vector-Vector-Activation Unit (function-pipelined).
 * @author	Giulio Gambardella <giuliog@xilinx.com>
 * @author	Thomas B. Preusser <thomas.preusser@amd.com>
 *
 * @description
 *  Function-pipelined VVU for depthwise separable convolutions.
 *  Each call processes one clock cycle's worth of work.
 *  Key difference from MVU: each PE processes its own dedicated input
 *  activation slice (no input broadcasting/reuse across neuron folds).
 *******************************************************************************/

#ifndef VVAU_HPP
#define VVAU_HPP

#include "hls_stream.h"

#include "mac.hpp"
#include "interpret.hpp"


/**
 * \brief Vector-vector-activate with embedded (internal) weights.
 *
 * Function-pipelined variant: no reps loop, static state, non-blocking reads.
 *
 * \tparam Channels   Number of channels
 * \tparam Kernel_2   Kernel * Kernel dimension (Kernel ^ 2 if square)
 * \tparam SIMD       Number of input columns computed in parallel
 * \tparam PE         Number of output rows computed in parallel
 * \tparam MMV        Number of output pixels computed in parallel
 * \tparam TSrcI      DataType of the input activation
 * \tparam TDstI      DataType of the output activation
 * \tparam TWeightI   DataType of the weights
 * \tparam TI         DataType of the input stream
 * \tparam TO         DataType of the output stream
 * \tparam TW         DataType of the weights matrix
 * \tparam TA         DataType of the activation class
 * \tparam R          Resource type for the MAC block
 */
template<
  unsigned Channels, unsigned Kernel_2, unsigned SIMD, unsigned PE, unsigned MMV,
  typename TSrcI = Identity, typename TDstI = Identity, typename TWeightI = Identity,
  typename TI, typename TO, typename TW, typename TA, typename R
>
void Vector_Vector_Activate_Batch(hls::stream<TI> &in,
				  hls::stream<TO> &out,
				  TW  const &weights,
				  TA  const &activation,
				  R const &r) {
#pragma HLS pipeline II=1 style=flp

  constexpr unsigned  NF = Channels / PE;
  constexpr unsigned  SF = Kernel_2 / SIMD;

  using TAcc = decltype(activation.init(0, 0));

  static TAcc  accu[MMV][PE];
  static unsigned  sf = 0;
  static unsigned  nf = 0;
  static unsigned  nf_out = 0;
  static bool  push = false;
#pragma HLS ARRAY_PARTITION variable=accu complete dim=0
#pragma HLS reset variable=accu off
#pragma HLS reset variable=sf
#pragma HLS reset variable=nf
#pragma HLS reset variable=nf_out off
#pragma HLS reset variable=push

  // Output phase — one cycle after the last SF beat
  if(push) {
    auto  outElem = TDstI().template operator()<TO>();
    for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
      for(unsigned mmv = 0; mmv < MMV; mmv++) {
#pragma HLS UNROLL
        outElem(pe, mmv, 1) = activation.activate(nf_out, pe, accu[mmv][pe]);
      }
    }
    out.write(outElem);
    push = false;
  }

  // Compute phase — non-blocking
  if(!in.empty()) {
    TI  inElem;
    inElem = in.read();

    // Threshold Initialisation
    if(sf == 0) {
      for(unsigned  pe = 0; pe < PE; pe++) {
        for(unsigned mmv = 0; mmv < MMV; mmv++) {
#pragma HLS UNROLL
          accu[mmv][pe] = activation.init(nf, pe);
        }
      }
    }

    // compute vector-vector product for each processing element
    auto const &w = weights.weights(nf * SF + sf);
    for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
      auto const  wgt = TWeightI()(w[pe]);

      ap_uint<SIMD*TSrcI::width> act_simd = 0;
      for(unsigned int simd = 0; simd < SIMD; simd++) {
        act_simd(TSrcI::width*(simd+1)-1, TSrcI::width*simd) = (Slice<ap_uint<PE*TSrcI::width>>()(inElem)(SIMD - simd - 1, 0))
                                                               (TSrcI::width*(pe+1)-1, TSrcI::width*pe);
      }

      for(unsigned mmv = 0; mmv < MMV; mmv++) {
        auto const  act = TSrcI()(act_simd, mmv);
        accu[mmv][pe] = mac<SIMD>(accu[mmv][pe], wgt, act, r, mmv);
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


/**
 * \brief Vector-vector-activate with streaming (decoupled/external) weights.
 *
 * Function-pipelined variant: no reps loop, static state, non-blocking reads.
 *
 * \tparam Channels   Number of channels
 * \tparam Kernel_2   Kernel * Kernel dimension
 * \tparam SIMD       Number of input columns computed in parallel
 * \tparam PE         Number of output rows computed in parallel
 * \tparam MMV        Number of output pixels computed in parallel
 * \tparam TSrcI      DataType of the input activation
 * \tparam TDstI      DataType of the output activation
 * \tparam TWeightI   DataType of the weights
 * \tparam TW         Weight element type (not deducible)
 * \tparam TI         DataType of the input stream
 * \tparam TO         DataType of the output stream
 * \tparam TA         DataType of the activation class
 * \tparam R          Resource type for the MAC block
 */
template<
	unsigned Channels, unsigned Kernel_2, unsigned SIMD, unsigned PE, unsigned MMV,
	typename TSrcI = Identity, typename TDstI = Identity, typename TWeightI = Identity, typename TW,
	typename TI, typename TO, typename TA, typename R
>
void Vector_Vector_Activate_Stream_Batch(
	hls::stream<TI> &in,
	hls::stream<TO> &out,
	hls::stream<ap_uint<PE*SIMD*TW::width>> &weights,
	TA  const &activation,
	R const &r
) {
#pragma HLS pipeline II=1 style=flp

	constexpr unsigned  NF = Channels / PE;
	constexpr unsigned  SF = Kernel_2 / SIMD;

	using TAcc = decltype(activation.init(0, 0));

	static TAcc  accu[MMV][PE];
	static unsigned  sf = 0;
	static unsigned  nf = 0;
	static unsigned  nf_out = 0;
	static bool  push = false;
#pragma HLS ARRAY_PARTITION variable=accu complete dim=0
#pragma HLS reset variable=accu off
#pragma HLS reset variable=sf
#pragma HLS reset variable=nf
#pragma HLS reset variable=nf_out off
#pragma HLS reset variable=push

	// unpacked and packed buffers for weight stream
	Weights_Tile<SIMD, TW, PE>  w;
#pragma HLS ARRAY_PARTITION variable=w.m_weights complete dim=0

	// Output phase
	if(push) {
		auto  outElem = TDstI().template operator()<TO>();
		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
			for(unsigned mmv = 0; mmv < MMV; mmv++) {
#pragma HLS UNROLL
				outElem(pe, mmv, 1) = activation.activate(nf_out, pe, accu[mmv][pe]);
			}
		}
		out.write(outElem);
		push = false;
	}

	// Compute phase — non-blocking: need both input and weights
	if(!in.empty() && !weights.empty()) {
		TI  inElem;
		inElem = in.read();

		// Threshold Initialisation
		if(sf == 0) {
			for(unsigned  pe = 0; pe < PE; pe++) {
				for(unsigned mmv = 0; mmv < MMV; mmv++) {
#pragma HLS UNROLL
					accu[mmv][pe] = activation.init(nf, pe);
				}
			}
		}

		// Packed and unpacked weight representations
		ap_uint<PE * SIMD * TW::width> const  W_packed = weights.read();
		for(unsigned pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
			w.m_weights[pe] = W_packed((pe+1)*SIMD*TW::width-1, pe*SIMD*TW::width);
		}

		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
			auto const  wgt = TWeightI()(w[pe]);

			ap_uint<SIMD*TSrcI::width> act_simd = 0;
			for(unsigned int simd = 0; simd < SIMD; simd++) {
				act_simd(TSrcI::width*(simd+1)-1, TSrcI::width*simd) = (Slice<ap_uint<PE*TSrcI::width>>()(inElem)(SIMD - simd - 1, 0))
																	   (TSrcI::width*(pe+1)-1, TSrcI::width*pe);
			}

			for(unsigned mmv = 0; mmv < MMV; mmv++) {
				auto const  act = TSrcI()(act_simd, mmv);
				accu[mmv][pe] = mac<SIMD>(accu[mmv][pe], wgt, act, r, mmv);
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
