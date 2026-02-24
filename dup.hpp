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


template<
	unsigned  N,
	typename  T
>
void dup(
	hls::stream<T>  &src,
	hls::stream<T> (&dst)[N]
) {
#pragma HLS pipeline II=1 style=flp
	if(!src.empty()) {
		T const  x = src.read();
		for(unsigned  i = 0; i < N; i++) {
#pragma HLS unroll
			dst[i].write(x);
		}
	}

} // dup()

#endif
