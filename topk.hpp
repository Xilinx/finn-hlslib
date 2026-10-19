/******************************************************************************
 * Copyright (c) 2019, Xilinx, Inc.
 * Copyright (c) 2025, AMD, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ******************************************************************************/
#ifndef TOPK_HPP
#define TOPK_HPP

#include "tensor.hpp"
#include "utils.hpp"

/**
 * \brief   LabelSelect_Batch (function-pipelined) — returns labels of
 *          top-NumTop values in stream, emitted in decreasing order.
 *
 * Two-phase operation: CAPTURE reads all NumClasses/PECount input words
 * and maintains an ordered top-NumTop buffer. EMIT drains the buffer.
 *
 * \tparam NumClasses   Number of classes of the dataset
 * \tparam PECount      Number of inputs to be processed in parallel
 * \tparam NumTop       Number of top classes to be selected in output
 * \tparam In_T         Datatype of the input
 * \tparam Out_T        Datatype of the output
 *
 * \param in            Input stream
 * \param out           Output stream
 */
template<
	unsigned int NumClasses,
	unsigned int PECount,
	unsigned int NumTop,
	typename In_T,
	typename Out_T>
void LabelSelect_Batch(
	hls::stream<tensor<In_T, PECount>> &in,
	hls::stream<tensor<Out_T, 1>> &out
) {
#pragma HLS pipeline II=1 style=frp
	static_assert(clog2(NumClasses) <= Out_T::width - Out_T::sign_flag, "");
	constexpr int  In_T_MIN_VAL = In_T::sign_flag? -(1<<(In_T::width-1)) : 0;
	constexpr unsigned  FOLDS = NumClasses / PECount;

	// State
	constexpr unsigned  STATE_CAPTURE = 0;
	constexpr unsigned  STATE_EMIT    = 1;

	static ap_uint<1>  state = STATE_CAPTURE;
	static unsigned  block = 0;
	static unsigned  emit_cnt = 0;
	static unsigned  idx = 0;
#pragma HLS reset variable=state
#pragma HLS reset variable=block
#pragma HLS reset variable=emit_cnt
#pragma HLS reset variable=idx

	static In_T   topval[NumTop];
	static Out_T  toplabels[NumTop];
#pragma HLS ARRAY_PARTITION variable=topval complete
#pragma HLS ARRAY_PARTITION variable=toplabels complete
#pragma HLS reset variable=topval off
#pragma HLS reset variable=toplabels off

	if(state == STATE_CAPTURE) {
		if(!in.empty()) {
			auto const  inElem = in.read();

			// Initialize on first block
			if(block == 0 && idx == 0) {
				for(unsigned  i = 0; i < NumTop; i++) {
#pragma HLS UNROLL
					topval[i]    = In_T_MIN_VAL;
					toplabels[i] = 0;
				}
			}

			// Process PECount elements in parallel
			for(unsigned  elem = 0; elem < PECount; elem++) {
#pragma HLS UNROLL
				In_T const  val = inElem[elem];

				// Parallel comparison — buffer is ordered, smallest at index 0
				bool  cmp[NumTop+1];
				for(unsigned  i = 0; i < NumTop; i++) {
#pragma HLS UNROLL
					cmp[i] = val > topval[i];
				}
				cmp[NumTop] = false;

				// Ordered insertion
				for(unsigned  i = 0; i < NumTop; i++) {
#pragma HLS UNROLL
					if(cmp[i]) {
						if(cmp[i+1]) {
							topval   [i] = topval   [i+1];
							toplabels[i] = toplabels[i+1];
						}
						else {
							topval   [i] = val;
							toplabels[i] = idx;
						}
					}
				}
				idx++;
			}

			if(++block == FOLDS) {
				block = 0;
				idx = 0;
				state = STATE_EMIT;
			}
		}
	}
	else {
		// Emit top labels in decreasing order (largest first)
		out.write(tensor<Out_T, 1>{toplabels[NumTop - 1 - emit_cnt]});
		if(++emit_cnt == NumTop) {
			emit_cnt = 0;
			state = STATE_CAPTURE;
		}
	}
}

#endif
