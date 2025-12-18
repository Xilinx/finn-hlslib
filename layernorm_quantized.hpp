/****************************************************************************
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Leah Sieder <leah-louisa.sieder@amd.com>
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#ifndef LAYERNORM_QUANTIZED_HPP
#define LAYERNORM_QUANTIZED_HPP

#include "utils.hpp"
#include <hls_math.h>

template<size_t N, typename TI, typename TO, size_t SIMD>
void first_diamond(
	hls::stream<hls::vector<TI, SIMD>> &in_s,
	hls::stream<hls::vector<TI, SIMD>> &out_s,
	hls::stream<TO> &sum_s
){
#pragma HLS pipeline II=1 style=frp

    static_assert(N%SIMD == 0, "SIMD parallelism must divide vector length.");
    constexpr size_t  NN = N / SIMD;

	static ModCounter<NN>  count;
	static TO  sum = TO(0);
#pragma HLS reset variable=count
#pragma HLS reset variable=sum off

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
		sum += tree_reduce(in, [](TO const &a, TO const &b) -> TO { return  a+b; });
		if (count.tick()) {
			sum_s.write(sum); 
			sum = TO(0);
		}

	}

}

template<size_t N, typename TI, typename TO, size_t SIMD>
void sub(
	hls::stream<hls::vector<TI, SIMD>> &in_s,
	hls::stream<TO> &sum_s,
	hls::stream<hls::vector<TO, SIMD>> &out_s
){
#pragma HLS pipeline II=1 style=frp
	static_assert(N%SIMD == 0, "SIMD parallelism must divide vector length.");
    constexpr size_t  NN = N / SIMD;

	static ModCounter<NN>  count;
	static bool valid = false;
	static TO sum;
#pragma HLS reset variable=count
#pragma HLS reset variable=valid
#pragma HLS reset variable=sum

	if(!valid && !sum_s.empty()){
		sum = sum_s.read();
		valid = true;
	}

	if(valid && !in_s.empty()){
		hls::vector<TI, SIMD> in = in_s.read();
		hls::vector<TO, SIMD> out;
		for(unsigned i=0; i<SIMD; i++) {
#pragma HLS UNROLL
			out[i] = TO(in[i]) * N - sum;
		}
		out_s.write(out);
		if(count.tick()) {
			valid = false;
		}
	}
}

template<size_t N, typename TI, typename TO, size_t SIMD>
void second_diamond(
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
    size_t N,
    typename TI, // Input type must be an integer
    typename TO, // Output type must be a floating point
    size_t SIMD
>
void layernorm_quantized(
    hls::stream<hls::vector<TI, SIMD>> &src,
    hls::stream<hls::vector<TO, SIMD>> &dst,
    TO const eps = 1e-5f
){
	static_assert(is_floating_point_or_ap_float<TO>::value, "Output datatype must be a float or ap_float type");

	using TSUM = scale_width<TI, 1, clog2(N)>;

	constexpr size_t NN = N / SIMD;
	constexpr size_t STATISTICS_LATENCY_DIAMOND_1 = 8;
	constexpr size_t STATISTICS_LATENCY_DIAMOND_2 = 76;
	constexpr size_t STAT_STREAM_LEN = 2;
	constexpr size_t DIAMOND_1_BYPASS_STREAM_LEN = NN + STATISTICS_LATENCY_DIAMOND_1;
	constexpr size_t DIAMOND_2_BYPASS_STREAM_LEN = NN + STATISTICS_LATENCY_DIAMOND_2;

#pragma HLS DATAFLOW disable_start_propagation

	static hls::stream<hls::vector<TI, SIMD>> x0_s;
#pragma HLS stream variable=x0_s depth=DIAMOND_1_BYPASS_STREAM_LEN

	static hls::stream<TSUM> sum0_s;
#pragma HLS stream variable=sum0_s depth=STAT_STREAM_LEN

	static hls::stream<hls::vector<TSUM, SIMD>> y_s;
#pragma HLS stream variable=y_s depth=STAT_STREAM_LEN

	static hls::stream<hls::vector<TSUM, SIMD>> x1_s;
#pragma HLS stream variable=x1_s depth=DIAMOND_2_BYPASS_STREAM_LEN

	static hls::stream<TO> sum1_s;
#pragma HLS stream variable=sum1_s depth=STAT_STREAM_LEN

	first_diamond<N>(src, x0_s, sum0_s);
	sub<N>(x0_s, sum0_s, y_s);
	second_diamond<N>(y_s, x1_s, sum1_s, eps);
	mult<N>(x1_s, sum1_s, dst);


}

#endif