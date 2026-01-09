/****************************************************************************
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Leah Sieder <leah-louisa.sieder@amd.com>
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#ifndef LAYERNORM_HPP
#define LAYERNORM_HPP

#include "utils.hpp"
#include <hls_math.h>


template<
	size_t    N,     // size of feature vector
	typename  TI,    // [inferred] feature data type
	typename  TS,    // [inferred] sum data type
	size_t    SIMD,  // [inferred] feature parallelism
	typename  F_PRE  // [inferred] functor for feature refinement prior to accumulation: (TI) -> TS
>
static void sum_statistics(
	hls::stream<hls::vector<TI, SIMD>> &src,
	hls::stream<hls::vector<TI, SIMD>> &dst,
	hls::stream<TS>                    &sum,
	F_PRE && fpre
){
#pragma HLS function_instantiate variable=fpre
#pragma HLS pipeline II=1 style=frp
	static_assert(N%SIMD == 0, "SIMD parallelism must divide vector length.");
	constexpr size_t  NN = N / SIMD;

	static ModCounter<NN>  count;
	static TS  accu = TS(0);
#pragma HLS reset variable=count
#pragma HLS reset variable=accu

	if(!src.empty()){
		hls::vector<TI, SIMD> const  x = src.read();
		hls::vector<TS, SIMD>  t;

		// Bypass
		dst.write(x);

		// Statistics Accumulation
		for(size_t  i = 0; i < SIMD; i++) {
#pragma HLS unroll
			t[i] = fpre(x[i]);
		}
		accu += tree_reduce(t, [](TS const &a, TS const &b) -> TS { return  a+b; });
		if(count.tick()) {
			sum.write(accu);
			accu = TS(0);
		}
	}

} // sum_statistics()

template<
	size_t    N,     // size of feature vector
	typename  TI,    // [inferred] input feature data type
	typename  TN,    // [inferred] norm data type
	typename  TO,    // [inferred] normalized output feature data type
	size_t    SIMD,  // [inferred] feature parallelism
	typename  F_NORM // [inferred] functor for feature normalization: (TI, TN) -> TO
>
static void normalize(
	hls::stream<hls::vector<TI, SIMD>> &src,
	hls::stream<TN>                    &norm,
	hls::stream<hls::vector<TO, SIMD>> &dst,
	F_NORM &&fnorm
) {
#pragma HLS function_instantiate variable=fnorm
#pragma HLS pipeline II=1 style=frp
	static_assert(N%SIMD == 0, "SIMD parallelism must divide vector length.");
	constexpr size_t  NN = N / SIMD;

	static ModCounter<NN>  count;
	static bool  valid = false;
	static TN  nrm;
#pragma HLS reset variable=count
#pragma HLS reset variable=valid
#pragma HLS reset variable=nrm off

	if(!valid)  valid = norm.read_nb(nrm);
	if(valid && !src.empty()){
		hls::vector<TI, SIMD> const  x = src.read();
		hls::vector<TO, SIMD>  y;
		for(unsigned i=0; i<SIMD; i++) {
#pragma HLS UNROLL
			y[i] = fnorm(x[i], nrm);
		}
		dst.write(y);
		if(count.tick())  valid = false;
	}

} // normalize()


template<
	size_t    N,
	typename  TI,
	typename  TO,
	size_t    SIMD
>
void layernorm(
	hls::stream<hls::vector<TI, SIMD>> &src,
	hls::stream<hls::vector<TO, SIMD>> &dst,
	TO const eps = 1e-5f
){
#pragma HLS function_instantiate variable=eps
#pragma HLS DATAFLOW disable_start_propagation

	//-----------------------------------------------------------------------
	// Derive Types for Intermediates:
	//  - integer inputs: widened safely,
	//  - otherwise: TO is used.
	using  TI_LIMITS = typename std::numeric_limits<TI>;
	// Sum over N inputs
	using  TS = typename std::conditional<
		TI_LIMITS::is_integer,
		typename std::conditional<
			TI_LIMITS::is_signed,
			ap_int<clog2(N) + TI_LIMITS::digits + 1>,
			ap_uint<clog2(N) + TI_LIMITS::digits>
		>::type,
		TO
	>::type;
	// Scaled, mean-normalized data: N*x[i] - sum
	using  TN = typename std::conditional<
		TI_LIMITS::is_integer,
		ap_int<1 + clog2(N) + TI_LIMITS::digits + TI_LIMITS::is_signed>,
		TO
	>::type;
	// Sum of Squares of mean-normalized data
	using  TQS = typename std::conditional<
		TI_LIMITS::is_integer,
		ap_uint<clog2(N) + 2*(clog2(N) + TI_LIMITS::digits + TI_LIMITS::is_signed)>,
		TO
	>::type;

	//-----------------------------------------------------------------------
	// Derive Needed Stream Depths
	constexpr size_t  NN = N / SIMD;
	constexpr size_t  STATISTICS_LATENCY_DIAMOND_0 = 30; // worst seen so far after synthesis: 26;
	constexpr size_t  STATISTICS_LATENCY_DIAMOND_1 = 90; // worst seen so far after synthesis: 82;
	constexpr size_t  BYPASS_DEPTH_0 = NN + STATISTICS_LATENCY_DIAMOND_0;
	constexpr size_t  BYPASS_DEPTH_1 = NN + STATISTICS_LATENCY_DIAMOND_1;

	//-----------------------------------------------------------------------
	// First Diamond for Means Normalization
	static hls::stream<hls::vector<TI, SIMD>>  bypass0;
#pragma HLS stream variable=bypass0 depth=BYPASS_DEPTH_0
	static hls::stream<TS>  sum0;
#pragma HLS stream variable=sum0 depth=2
	static hls::stream<hls::vector<TN, SIMD>> norm0;
#pragma HLS stream variable=norm0 depth=2

	sum_statistics<N>(
		src, bypass0, sum0,
		[](TI const &x) -> TS { return  TS(x); }
	);
	normalize<N>(
		bypass0, sum0, norm0,
		[](TI const &x, TS const &m) -> TN { return  TN(N*x) - TN(m); }
	);

	//-----------------------------------------------------------------------
	// Second Diamond for Variance Normalization
	static hls::stream<hls::vector<TN, SIMD>>  bypass1;
#pragma HLS stream variable=bypass1 depth=BYPASS_DEPTH_1
	static hls::stream<TQS>  sum1;
#pragma HLS stream variable=sum1 depth=2
	static hls::stream<TO>  coeff1;
#pragma HLS stream variable=coeff1 depth=2
	sum_statistics<N>(
		norm0, bypass1, sum1,
		[](TN const &x) -> TQS { return  TQS(x*x); }
	);
	[](hls::stream<TQS> &src, hls::stream<TO> &dst, TO const  eps) {
#pragma HLS function_instantiate variable=eps
#pragma HLS pipeline II=NN
		constexpr TO  INVN = TO(1) / N;
		if(!src.empty())  dst.write(hls::rsqrt(INVN * TO(src.read()) + eps));
	}(sum1, coeff1, eps);
	normalize<N>(
		bypass1, coeff1, dst,
		[](TN const &x, TO const &m) -> TO { return  m * TO(x); }
	);

} // layernorm()

#endif
