/****************************************************************************
 * Copyright Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: MIT
 *
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#ifndef PAD_HPP
#define PAD_HPP

#include "utils.hpp"
#include <algorithm>
#include <hls_stream.h>
#include <ap_int.h>


template<
	unsigned  H,	// height of input feature map
	unsigned  W,	// width of input feature map
	unsigned  CF,	// channel fold (temporal, rather than spatial, factor of channel count)
	unsigned  PAD_N,	// padding north
	unsigned  PAD_E = PAD_N,	// padding east
	unsigned  PAD_S = PAD_N,	// padding south
	unsigned  PAD_W = PAD_E,	// padding west
	typename  T
>
void pad(
	hls::stream<T> &src,
	hls::stream<T> &dst,
	T const &pad_val = T{0,}
) {
#pragma HLS function_instantiate variable=pad_val
#pragma HLS pipeline II=1 style=flp
	constexpr unsigned  HO = PAD_N + H + PAD_S;
	constexpr unsigned  WO = PAD_W + W + PAD_E;

	static ap_uint<clog2(std::max(2u, HO))>  ho = 0;
	static ap_uint<clog2(std::max(2u, WO))>  wo = 0;
	static ModCounter<CF>  cf;
#pragma HLS reset variable=ho
#pragma HLS reset variable=wo
#pragma HLS reset variable=cf

	bool  wr = true;
	T  val = pad_val;
	if(((PAD_N <= ho) && (ho < PAD_N+H)) &&
	   ((PAD_W <= wo) && (wo < PAD_W+W))) {
		wr = src.read_nb(val);
	}
	if(wr) {
		dst.write(val);
		if(cf.tick()) {
			if(wo != WO-1)  wo++;
			else {
				wo = 0;
				if(ho != HO-1)  ho++;
				else  ho = 0;
			}
		}
	}

} // pad()

#endif
