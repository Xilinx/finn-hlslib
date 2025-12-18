/****************************************************************************
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Leah Sieder <leah-louisa.sieder@amd.com>
 ****************************************************************************/
#ifndef RMSNORM_HPP
#define RMSNORM_HPP

#include "util.hpp"
#include <hls_math.h>

template<size_t N, typename TI, typename TO, size_t SIMD>
void stat_stage(
	hls::stream<hls::vector<TI, SIMD>> &in_s,
	hls::stream<hls::vector<TI, SIMD>> &out_s,
	hls::stream<TO> &sqrsumrsqrt_s,
	const TO eps
){
#pragma HLS pipeline II=1 style=frp
	static_assert(N%SIMD == 0, "SIMD parallelism must divide vector length.");
    constexpr size_t  NN = N / SIMD;
	constexpr float INVN = 1.0f / N; 

	using TSQR = scale_width<TI, 2>;
	using TSQRSUM = scale_width<TI, 2, clog2(N)>;

	static ModCounter<NN>  count;
	static TSQRSUM  sqrsum = TO(0);
#pragma HLS reset variable=count
#pragma HLS reset variable=sqrsum

	if(!in_s.empty()){
		hls::vector<TI, SIMD> in = in_s.read();

		// bypass
		hls::vector<TI, SIMD> out;
		for(unsigned i=0; i<SIMD; i++) {
#pragma HLS UNROLL
			out[i] = in[i];	
		}
		out_s.write(out);

		// statistics
		hls::vector<TSQR, SIMD> sqr;
		for(unsigned i=0; i<SIMD; i++) {
#pragma HLS UNROLL
			sqr[i] = in[i] * in[i];	
		}
		sqrsum += tree_reduce(sqr, [](TSQR const &a, TSQR const &b) -> TSQRSUM { return  a+b; });
		if (count.tick()) {
			TO normsum = TO(sqrsum) * INVN;
			TO sqrsumrsqrt = hls::rsqrt(normsum + eps);
			sqrsumrsqrt_s.write(sqrsumrsqrt); 
			sqrsum = TO(0);
		}
	}
}

template<size_t N, typename TI, typename TO, size_t SIMD>
void mult(
	hls::stream<hls::vector<TI, SIMD>> &in_s,
	hls::stream<TO> &sqrsumrsqrt_s,
	hls::stream<hls::vector<TO, SIMD>> &out_s
){
#pragma HLS pipeline II=1 style=frp
	static_assert(N%SIMD == 0, "SIMD parallelism must divide vector length.");
    constexpr size_t  NN = N / SIMD;

	static ModCounter<NN>  count;
	static bool valid = false;
	static TO sqrsumrsqrt;
#pragma HLS reset variable=count
#pragma HLS reset variable=valid
#pragma HLS reset variable=sqrsumrsqrt off

	if(!valid && !sqrsumrsqrt_s.empty()){
		sqrsumrsqrt = sqrsumrsqrt_s.read();
		valid = true;
	}

	if(valid && !in_s.empty()){
		hls::vector<TI, SIMD> in = in_s.read();
		hls::vector<TO, SIMD> out;
		for(unsigned i=0; i<SIMD; i++) {
#pragma HLS UNROLL
			out[i] = TO(in[i]) * sqrsumrsqrt;
		}
		out_s.write(out);
		if(count.tick()) {
			valid = false;
		}
	}

}

template<
	size_t    N,
	typename  TI, // Input type
	typename  TO, // Output type
	size_t    SIMD
>
void rmsnorm(
	hls::stream<hls::vector<TI, SIMD>> &src,
	hls::stream<hls::vector<TO, SIMD>> &dst,
	TO const  eps = 1e-5f
) {
	static_assert(is_floating_point_or_ap_float<TO>::value, "Output datatype must be a float or ap_float type");

	constexpr size_t NN = N / SIMD;
	constexpr size_t STATISTICS_LATENCY = 6;
	constexpr size_t STAT_STREAM_LEN = 2;
	constexpr size_t BYPASS_STREAM_LEN = NN + STATISTICS_LATENCY;

#pragma HLS DATAFLOW disable_start_propagation

	static hls::stream<hls::vector<TI, SIMD>> x_s;
#pragma HLS stream variable=x_s depth=N
	static hls::stream<TO> sum_s;
#pragma HLS stream variable=sum_s depth=2

	stat_stage<N>(src, x_s, sum_s, eps);
	mult<N>(x_s, sum_s, dst);
}

#endif