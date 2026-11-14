/******************************************************************************
 *  Copyright (c) 2024-2025, Advanced Micro Devices, Inc.
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
 * @author	Michal Danilowicz <danilowi@agh.edu.pl>
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 *******************************************************************************/

#include <hls_stream.h>
#include <ap_int.h>
#include <algorithm>

#include "utils.hpp"


//---------------------------------------------------------------------------
// Ultimate backing implementation with destination streams as parameter pack.

// Helper for parameter pack indexing.
namespace {
	/** Recursive selector for writing to stream with represented index. */
	template<unsigned  IDX, typename... TO>
	class PackWriter {};

	/** Terminating with last stream in stream pack. */
	template<unsigned  IDX, typename  T0>
	class PackWriter<IDX, T0> {
	public:
		template<typename TI>
		void write(unsigned const  idx, TI &x, hls::stream<T0>& dst0) {
#pragma HLS inline
			if(idx == IDX)  dst0.write(x);
		}
	};

	/** Matching own index or recursively passing further into the pack. */
	template<unsigned  IDX, typename  T0, typename... TO>
	class PackWriter<IDX, T0, TO...> {
		PackWriter<IDX+1, TO...>  inner;

	public:
		template<typename TI>
		void write(unsigned const  idx, TI &x, hls::stream<T0>& dst0, hls::stream<TO>&... dst) {
#pragma HLS inline
			if(idx != IDX)  return  inner.write(idx, x, dst...);
			else  dst0.write(x);
		}
	};
}

// Main split implementation.
template<
	unsigned... C,
	typename    TI,
	typename... TO
>
void StreamingSplit(
	hls::stream<TI>    &src,
	hls::stream<TO>&... dst
){
	constexpr unsigned  N = sizeof...(TO);
	constexpr unsigned  WRAP_INC = (1<<clog2(N))-(N-1);
	static_assert(sizeof...(C) == N, "Number of channel counts must match number of destinations.");
	static_assert(std::min({C...}) > 0, "Cannot have a zero channel count.");
	static unsigned const  CNT_INIT[N] = { (C-2)... };
	static ap_uint<clog2(N)>                    sel = 0;
	static ap_int<1+clog2(std::max({C...})-1)>  cnt = CNT_INIT[0];
#pragma HLS reset variable=sel
#pragma HLS reset variable=cnt
	static PackWriter<0, TO...>  writer;

#pragma HLS pipeline II=1 style=flp
 
	if(!src.empty()) {
		TI const  x = src.read();
		writer.write(sel, x, dst...);
		if(cnt >= 0)  cnt--;
		else {
			sel += (sel == N-1)? WRAP_INC : 1;
			cnt = CNT_INIT[sel];
		}
	}
}

//---------------------------------------------------------------------------
// Adapter to facilitate the use of an array of destination streams.

// Indirection to unpack array into a list of arguments.
namespace {
	template<
		unsigned... C,
		typename    TI,
		typename    TO,
		size_t...   Idxs
	>
	void StreamingSplit0(
		hls::stream<TI>  &src,
		hls::stream<TO> (&dst)[sizeof...(Idxs)],
		std::index_sequence<Idxs...>
	) {
#pragma HLS inline
		StreamingSplit<C...>(src, dst[Idxs]...);
	}
}

template<
	unsigned... C,
	typename    TI,
	typename    TO,
	size_t      N
>
void StreamingSplit(
	hls::stream<TI>  &src,
	hls::stream<TO> (&dst)[N]
) {
#pragma HLS inline
	StreamingSplit0<C...>(src, dst, std::make_index_sequence<N>());
}
