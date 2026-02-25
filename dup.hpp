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
#include <utility>


//---------------------------------------------------------------------------
// Variadic dup() implementation with destination streams as parameter pack.

// Main dup implementation.
template<
	typename    TI, // [inferred] Type carried on input stream
	typename... TO  // [inferred] Types carried on output streams, must be assignable from TI
>
void StreamingDup(
	hls::stream<TI>    &src,
	hls::stream<TO>&... dst
){
#pragma HLS pipeline II=1 style=flp

	if(!src.empty()) {
		TI const  x = src.read();
		(void)std::initializer_list<int>{(dst.write(x), 0)...};
	}
}

//---------------------------------------------------------------------------
// Adapter to facilitate the use of an array of destination streams.

// Indirection to unpack array into a list of arguments.
namespace {
	template<
		typename       TI,
		typename       TO,
		std::size_t... Idxs
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
	typename     TI, // [inferred] Type carried on input stream
	typename     TO, // [inferred] Type carried on output streams, must be assignable from TI
	std::size_t  N   // [inferred] Number of output streams
>
void StreamingDup(
	hls::stream<TI>  &src,
	hls::stream<TO> (&dst)[N]
) {
#pragma HLS inline
	StreamingDup0(src, dst, std::make_index_sequence<N>());
}

#endif
