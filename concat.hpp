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

/*******************************************************************************
 *
 *  Authors: Thomas B. Preusser <thomas.preusser@amd.com>
 *           Michal Danilowicz <danilowi@agh.edu.pl>
 *
 *  \file concat.hpp
 *
 *  This file defines template functions
 *	of a channel concatenation operation.
 *
 *******************************************************************************/

#include <hls_stream.h>
#include <ap_int.h>
#include <algorithm>

#include "utils.hpp"


/** For inputs of the same datatype */
template<
	unsigned...  C,
	unsigned  N,
	typename  TO,
	typename  TI
>
void StreamingConcat(
	hls::stream<TO> &dst,
	hls::stream<TI> (&src)[N]
) {
	static_assert(sizeof...(C) == N, "Number of channel counts must match number of sources.");
	static_assert(std::min({C...}) > 0, "Cannot have a zero channel count.");
	static unsigned const  CNT_INIT[N] = { (C-2)... };
	static ap_uint<clog2(N)>                    sel = 0;
	static ap_int<1+clog2(std::max({C...})-1)>  cnt = CNT_INIT[0];
#pragma HLS reset variable=sel
#pragma HLS reset variable=cnt

#pragma HLS pipeline II=1 style=flp
	TO  y;
	if(src[sel].read_nb(y)) {
		dst.write(y);
		if(cnt >= 0)  cnt--;
		else {
			sel = (sel == N-1)? decltype(sel)(0) : decltype(sel)(sel+1);
			cnt = CNT_INIT[sel];
		}
	}

} // StreamingConcat()

namespace {
	/** Type conversion between vectors */
	template<typename TI, typename TO, size_t N>
	void convert_vector(hls::vector<TI, N>& src, hls::vector<TO, N>& dst){
#pragma HLS inline
		for(size_t i = 0; i < N; i++){
#pragma HLS UNROLL
			dst[i] = src[i];
		}
	}

	/** Recursive selector for reading from stream with represented index. */
	template<unsigned  IDX, typename... TI>
	class PackReader {};

	/** Terminating with last stream in stream pack. */
	template<unsigned  IDX, typename  T0>
	class PackReader<IDX, T0> {
	public:
		template<typename TO>
		bool read_nb(unsigned const  idx, TO &y, hls::stream<T0>& src0) {
			if(idx != IDX)  return  false;
			else {
				T0  y0;
				if(!src0.read_nb(y0))  return  false;
				convert_vector(y0, y);
				return  true;
			}
		}
	};

	/** Matching own index or recursively passing further into the pack. */
	template<unsigned  IDX, typename  T0, typename... TI>
	class PackReader<IDX, T0, TI...> {
		PackReader<IDX+1, TI...>  inner;

	public:
		template<typename TO>
		bool read_nb(unsigned const  idx, TO &y, hls::stream<T0>& src0, hls::stream<TI>&... src) {
			if(idx != IDX)  return  inner.read_nb(idx, y, src...);
			else {
				T0  y0;
				if(!src0.read_nb(y0))  return  false;
				convert_vector(y0, y);
				return  true;
			}
		}
	};
}

/** For inputs of different datatypes */
template<
	unsigned...  C,
	typename     TO,
	typename...  TI
>
void StreamingConcat(
	hls::stream<TO>    &dst,
	hls::stream<TI>&... src
) {
	constexpr unsigned  N = sizeof...(TI);
	static_assert(sizeof...(C) == N, "Number of channel counts must match number of sources.");
	static_assert(std::min({C...}) > 0, "Cannot have a zero channel count.");
	static unsigned const  CNT_INIT[N] = { (C-2)... };
	static ap_uint<clog2(N)>                    sel = 0;
	static ap_int<1+clog2(std::max({C...})-1)>  cnt = CNT_INIT[0];

#pragma HLS reset variable=sel
#pragma HLS reset variable=cnt
	static PackReader<0, TI...>  reader;
#pragma HLS pipeline II=1 style=flp

	TO  y;
	if(reader.read_nb(sel, y, src...)) {
		dst.write(y);
		if(cnt >= 0)  cnt--;
		else {
			sel = (sel == N-1)? decltype(sel)(0) : decltype(sel)(sel+1);
			cnt = CNT_INIT[sel];
		}
	}
} // StreamingConcat()
