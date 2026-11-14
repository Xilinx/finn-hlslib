/******************************************************************************
 *  Copyright (c) 2024, Advanced Micro Devices, Inc.
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
 *
 ******************************************************************************/
#ifndef DWC_HPP
#define DWC_HPP

#include "tensor_view.hpp"

/**
 * @brief	Stream Data Width Converter
 *
 * Converts between tensor streams of different element counts but the same
 * element type. One dimension must divide the other.
 *
 * @tparam	 T   Element type (e.g. ap_int<4>)
 * @tparam	 NI  Number of elements per input word
 * @tparam	 NO  Number of elements per output word
 */
template<typename T, unsigned NI, unsigned NO>
void StreamingDWC(
	hls::stream<tensor<T, NI>> &in,
	hls::stream<tensor<T, NO>> &out
) {
#pragma HLS pipeline II=1 style=flp
	static_assert((NI % NO == 0) || (NO % NI == 0), "One dimension must divide the other.");

	if constexpr(NI > NO) {
		static tensor<T, NI/NO, NO>  ei;
		static unsigned  o = 0;
#pragma HLS reset variable=ei off
#pragma HLS reset variable=o

		if(o == 0) {
			if(in.empty())  return;
			ei = in.read().template reshape<T, NI/NO, NO>();
		}
		out.write(ei.slice(0));
		ei.slice(NI/NO-2, 0) = ei.slice(NI/NO-1, 1);
		if(o++ == NI/NO-1)  o = 0;

	} else if constexpr(NI == NO) {
		if(!in.empty())  out.write(in.read());

	} else {
		static tensor<T, NO/NI, NI>  eo;
		static unsigned  i = 0;
#pragma HLS reset variable=eo off
#pragma HLS reset variable=i

		if(!in.empty()) {
			eo.slice(NO/NI-2, 0) = eo.slice(NO/NI-1, 1);
			eo.slice(NO/NI-1) = in.read();
			if(i++ == NO/NI-1) {
				i = 0;
				out.write(eo.template reshape<T, NO>());
			}
		}
	}
}

#endif
