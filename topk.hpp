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
 *          top-K values in stream, emitted in decreasing order.
 *
 * Two-phase operation: CAPTURE reads all N/PE input words
 * and maintains an ordered top-K buffer. EMIT drains the buffer.
 *
 * \tparam N   Number of classes of the dataset
 * \tparam K   Number of top classes to be selected in output
 * \tparam TI  [inferred] Datatype of the input
 * \tparam PE  [inferred] Number of inputs to be processed in parallel
 * \tparam TO  [inferred] Datatype of the output (must represent class indices [0, N))
 *
 * \param src  Input stream
 * \param dst  Output stream
 */
template<
	unsigned  N,
	unsigned  K,
	typename  TI,
	unsigned  PE,
	typename  TO
>
void LabelSelect_Batch(
	hls::stream<tensor<TI, PE>> &src,
	hls::stream<tensor<TO,  1>> &dst
) {
#pragma HLS pipeline II=1 style=frp
	static_assert(clog2(N) <= std::numeric_limits<TO>::digits, "TO must represent class indices [0, N)");

	// State
	constexpr unsigned  STATE_CAPTURE = 0;
	constexpr unsigned  STATE_EMIT    = 1;
	static ap_uint<1>  state = STATE_CAPTURE;
#pragma HLS reset variable=state

	static ap_uzint<clog2(N)>  idx = 0;
	static ModCounter<K>  emit_cnt;
#pragma HLS reset variable=idx
#pragma HLS reset variable=emit_cnt

	static TI  topval[K];
	static TO  toplabels[K];
#pragma HLS ARRAY_PARTITION variable=topval complete
#pragma HLS ARRAY_PARTITION variable=toplabels complete
#pragma HLS reset variable=topval off
#pragma HLS reset variable=toplabels off

	if(state == STATE_CAPTURE) {
		if(!src.empty()) {

			// Initialize on first block
			if(idx == 0) {
				for(unsigned  i = 0; i < K; i++) {
#pragma HLS UNROLL
					topval[i]    = std::numeric_limits<TI>::lowest();
					toplabels[i] = 0;
				}
			}

			// Process PE elements in parallel
			auto const  inElem = src.read();
			for(unsigned  i = 0; i < PE; i++) {
#pragma HLS UNROLL
				TI const  val = inElem[i];

				// Parallel comparison — buffer is ordered, smallest at index 0
				bool  cmp[K+1];
				for(unsigned  j = 0; j < K; j++) {
#pragma HLS UNROLL
					cmp[j] = val > topval[j];
				}
				cmp[K] = false;

				// Ordered insertion
				for(unsigned  j = 0; j < K; j++) {
#pragma HLS UNROLL
					if(cmp[j]) {
						if(cmp[j+1]) {
							topval   [j] = topval   [j+1];
							toplabels[j] = toplabels[j+1];
						}
						else {
							topval   [j] = val;
							toplabels[j] = idx + i;
						}
					}
				}
			}

			if(idx != N-PE)  idx += PE;
			else {
				idx = 0;
				state = STATE_EMIT;
			}
		}
	}
	else {
		// Emit top labels in decreasing order (largest first)
		dst.write(tensor<TO, 1>{toplabels[K-1]});
		for(unsigned  i = K-1; i > 0; i--) {
#pragma HLS UNROLL
			toplabels[i] = toplabels[i-1];
		}
		if(emit_cnt.tick())  state = STATE_CAPTURE;
	}
}

#endif
