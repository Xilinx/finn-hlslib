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
 * @brief	Splits a repetitive weight stream among multiple consumers ensuring
 *			the integrity of a frame period upon unbalanced backpressure.
 * @author	Thomas B. Preusser <thomas.preusser@amd.com>
 *******************************************************************************/
#ifndef WEIGHT_STREAM_SPLITTER_HPP
#define WEIGHT_STREAM_SPLITTER_HPP

#include <ap_int.h>
#include <hls_stream.h>

#include "utils.hpp"


template<
	unsigned  PERIOD,
	unsigned  N,
	typename  T
>
void weight_stream_splitter(
	hls::stream<T>  &src,
	hls::stream<T> (&dst)[N]
) {
#pragma HLS pipeline II=1 style=flp

	using  count_t = ap_uint<clog2(PERIOD)>;
	static count_t  cnt_src    = 0;
	static count_t  cnt_dst[N] = { 0, };
#pragma HLS reset variable=cnt_src
#pragma HLS reset variable=cnt_dst

	ap_uint<N>  ready;
	for(unsigned  i = 0; i < N; i++) {
#pragma HLS unroll
		bool const  synced = (cnt_dst[i] == cnt_src);
		ready[i] = synced && !dst[i].full();
	}
	if(!src.empty() && ready.or_reduce()) {
		T const  x = src.read();
		if(++cnt_src == PERIOD)  cnt_src = 0;

		for(unsigned  i = 0; i < N; i++) {
#pragma HLS unroll
			if(ready[i]) {
				dst[i].write(x);
				if(++cnt_dst[i] == PERIOD)  cnt_dst[i] = 0;
			}
		}
	}
}

#endif
