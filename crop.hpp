/****************************************************************************
 * Copyright (C) 2024, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#ifndef CROP_HPP
#define CROP_HPP

#include "util.hpp"
#include <hls_stream.h>
#include <ap_int.h>


template<
	unsigned  H,	// height of input feature map
	unsigned  W,	// width of input feature map
	unsigned  CF,	// channel fold (temporal, rather than spatial, factor of channel count)
	unsigned  CROP_N,	// cropping north
	unsigned  CROP_E = CROP_N,	// cropping east
	unsigned  CROP_S = CROP_N,	// cropping south
	unsigned  CROP_W = CROP_E,	// cropping west
	typename  T
>
void crop(
	hls::stream<T> &src,
	hls::stream<T> &dst
) {
#pragma HLS pipeline II=1 style=flp
	static_assert(CROP_N+CROP_S < H, "Image height too small for cropping.");
	static_assert(CROP_W+CROP_E < W, "Image width too small for cropping.");

	static ap_uzint<clog2(H)>  h = 0;
	static ap_uzint<clog2(W)>  w = 0;
	static ModCounter<CF>  cf;
#pragma HLS reset variable=h
#pragma HLS reset variable=w
#pragma HLS reset variable=cf

	if(!src.empty()) {
		auto const  val = src.read();
		if(
			((CROP_N <= h) && (h < H-CROP_S)) &&
			((CROP_W <= w) && (w < W-CROP_E))
		) {
			dst.write(val);
		}

		if(cf.tick()) {
			if(w != W-1)  w++;
			else {
				w = 0;
				if(h != H-1)  h++;
				else  h = 0;
			}
		}
	}

} // crop()

#endif
