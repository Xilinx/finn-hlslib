/****************************************************************************
 * Copyright (C) 2024 - 2026, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#ifndef DUP_HPP
#define DUP_HPP

#include <hls_stream.h>
#include <algorithm>


//---------------------------------------------------------------------------
// Ultimate backing implementation with destination streams as parameter pack.

// Helper for parameter pack indexing.
namespace {
	/** Recursive writing to all streams in pack. */
	template<typename... TO>
	class PackWriter {};

	/** Terminating with last stream in stream pack. */
	template<typename  T0>
	class PackWriter<T0> {
	public:
		template<typename TI>
		void write(TI &x, hls::stream<T0>& dst0) {
#pragma HLS inline
			dst0.write(x);
		}
	};

	/** Write and pass recursively further into the pack. */
	template<typename  T0, typename... TO>
	class PackWriter<T0, TO...> {
		PackWriter<TO...>  inner;

	public:
		template<typename TI>
		void write(TI &x, hls::stream<T0>& dst0, hls::stream<TO>&... dst) {
#pragma HLS inline
			dst0.write(x);
			inner.write(x, dst...);
		}
	};
}

// Main dup implementation.
template<
	typename    TI,
	typename... TO
>
void StreamingDup(
	hls::stream<TI>    &src,
	hls::stream<TO>&... dst
){
#pragma HLS pipeline II=1 style=flp
	static PackWriter<TO...>  writer;

	if(!src.empty()) {
		TI const  x = src.read();
		writer.write(x, dst...);
	}
}


//---------------------------------------------------------------------------
// Adapter to facilitate the use of an array of destination streams.

// Indirection to unpack array into a list of arguments.
namespace {
	template<
		typename    TI,
		typename    TO,
		size_t...   Idxs
	>
	void StreamingDup0(
		hls::stream<TI>  &src,
		hls::stream<TO> (&dst)[sizeof...(Idxs)],
		std::index_sequence<Idxs...>
	) {
#pragma HLS inline
		StreamingDup(src, dst[Idxs]...);
	}
}

template<
	typename    TI,
	typename    TO,
	size_t      N
>
void StreamingDup(
	hls::stream<TI>  &src,
	hls::stream<TO> (&dst)[N]
) {
#pragma HLS inline
	StreamingDup0(src, dst, std::make_index_sequence<N>());
}

#endif
