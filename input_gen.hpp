/****************************************************************************
 * Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#ifndef INPUT_GEN_HPP
#define INPUT_GEN_HPP

#include <ap_int.h>
#include <hls_stream.h>
#include "utils.hpp"

#include <algorithm>
#include <tuple>
#include <type_traits>

/**
 * Computes the updates of the read and free pointers for a buffer read out by
 * the specified loop nest.
 *
 * @param  R	true if this loop updates the free pointer (see below for determination)
 * @param  V	loop nest specifiction, odd length, see specializations
 *
 * A given perfect loop nest:
 *
 *	for(unsigned  i0 = 0; i0 < N0; i0++) {
 *		for(unsigned  i1 = 0; i1 < N1; i1++) {
 *			...
 *			for(unsigned  in = 0; in < Nn; in++) {
 *				emit(ifm[C0*i0 + C1*i1 + ... + Cn*in]);
 *			}
 *			...
 *		}
 *	}
 *
 * encodes as:
 *
 *	Nest<true, IFM_SIZE, N0, C0, N1, C1, ..., Nn, Cn>
 *
 * As this class computes relative updates by each invocation of `tick()`,
 * an absolute offset must be reflected in the original pointer initialization.
 * The contract for a directly enclosed loop is:
 *	- For the total of an entire period of increments, the cumulative read pointer
 *	  updates amount to the number immediately preceding its execution count.
 *	- The free pointer is incremented in lockstep if R is true and if this loops
 *	  own increments are positive and would fit entirely into a period of the
 *	  enclosing loop.
 * Currently, all coefficients Ci must be positive. The implication is that
 * every completed loop induces a net non-negative read-pointer increment.
 * Negative read pointer updates only happen upon loop termination - still leaving
 * a net positive update for the enclosing loop but possibly retracting the read
 * pointer back to the expected enclosing increment after overshooting
 * internally (if Ci < Ni * C(i+1)).
 * As each completed loop guarantees a net positive increment, negative pointer
 * retractions never add up. Thus, the biggest retraction can be used to
 * dimension provided buffer storage.
 */
template<bool R, unsigned... V>
class Nest {};

/**
 * Terminal innermost loop.
 *
 * @param  R	also responsible for the update of the free pointer
 * @param  W	represented increment of read pointer
 */
template<
	bool      R,
	unsigned  W
>
class Nest<R, W> {
public:
	static constexpr unsigned  rp_rewind = 0;
	static constexpr unsigned  fp_rewind = 0;

	static constexpr int  max_rp_retract = 0;

public:
	std::tuple<int, unsigned, ap_int<1>> tick() {
#pragma HLS inline
		return  { W, R? W : 0, -1 };
	}
};

/**
 * Non-terminal loop.
 *
 * @param  R	also responsible for the update of the free pointer
 * @param  W	increment of read pointer after full loop period (N iterations)
 * @param  N	iteration count of directly enclosed loop
 * @param  C	iterational increment of read pointer by directly enclosed loop
 * @param  V	further nested loops
 *
 *	- Each non-terminal loop will slice off two values, W & N, from the
 *	  specification vector V.
 *	- The directly enclosed loop will inherit responsibility for the
 *	  free pointer update only if it represents a strictly monotonic increase
 *	  contained entirely within the pointer update of this loop.
 */
template<
	bool      R,
	unsigned  W,
	unsigned  N,
	unsigned  C,
	unsigned... V
>
class Nest<R, W, N, C, V...> {

	static constexpr bool  R_INNER = R && (0 < C) && (C*N <= W);
	using  Inner = Nest<R_INNER, C, V...>;

public:
	static constexpr unsigned  rp_rewind = (N-1)*C + Inner::rp_rewind;
	static constexpr unsigned  fp_rewind = R_INNER? (N-1)*C + Inner::fp_rewind : 0;

private:
	static constexpr int  terminal_rp_inc = W - rp_rewind;
public:
	static constexpr int  max_rp_retract = std::max(-terminal_rp_inc, Inner::max_rp_retract);

private:
	static_assert(N > 0, "Must have positive iteration count.");
	ap_int<1+clog2(std::max(1u, N-1))>  cnt = N-2;	// N-2, N-1, ..., 1, 0, -1
	Inner  inner;

public:
	std::tuple<int, unsigned, ap_int<2+sizeof...(V)/2>> tick() {
#pragma HLS inline
		auto const  t = inner.tick();
		int       rp_inc = std::get<0>(t);
		unsigned  fp_inc = std::get<1>(t);
		ap_int<2+sizeof...(V)/2>  term = std::get<2>(t);

		if(term < 0) {
			if(cnt < 0) {
				rp_inc = terminal_rp_inc;
				if(R)  fp_inc = W - fp_rewind;
				cnt = N-2;
			}
			else {
				term[decltype(term)::width-1] = 0;
				cnt--;
			}
		}
		return { rp_inc, fp_inc, term };
	}
};

/**
 * Input generator:
 *	- over a feature map of pixels of type T
 *	- iterated over by the loop nest specified by V
 *	- optionally identifying the completion of a kernel produced by the M innermost loops.
 *
 * @param	M	innermost loop count constituting a kernel
 *		M <  0 - no `last` indicator on destination stream
 *		M >= 0 - `last` indicator on destination stream:
 *			0 - always asserted
 *			1 - upon completion of innermost loop
 *			M - upon completion of M innermost loops
 * @param	V	loop nest descriptor, see above for Nest<>
 * @param	T	(inferred) pixel type
 */
template<int  M, unsigned... V, typename  T>
void input_gen(
	hls::stream<T> &src,
	hls::stream<typename std::conditional<M < 0, T, flit_t<T>>::type> &dst
) {
#pragma HLS pipeline II=1 style=flp

	// Write Pointer update delay needed to accommodate memory read-out latency.
	constexpr unsigned  WP_DELAY = 4;

	using  MyNest = Nest<true, V...>;
	constexpr unsigned  ADDR_BITS = clog2(MyNest::max_rp_retract + WP_DELAY + 2);
	constexpr unsigned  BUF_SIZE  = 1 << ADDR_BITS;
	using  ptr_t = ap_int<1 + ADDR_BITS>;

	static MyNest  nest;
	static T  buf[BUF_SIZE];
	static ptr_t  wp[WP_DELAY] = { 0, };
	static ptr_t  rp = 0;
	static ptr_t  fp = 0;
#pragma HLS reset variable=nest
#pragma HLS reset variable=buf off
#pragma HLS reset variable=wp
#pragma HLS reset variable=rp
#pragma HLS reset variable=fp
#pragma HLS dependence variable=buf inter false
#pragma HLS dependence variable=buf intra false
#pragma HLS array_partition variable=wp complete

	static bool  ovld = false;
	static struct OBuf {
		bool  lst;
		T     dat;

	public:
		operator T const&()  const { return  dat; }
		operator flit_t<T>() const { return { lst, dat }; }
	} obuf;
#pragma HLS reset variable=ovld
#pragma HLS reset variable=obuf off

	// Update delay pipeline for wp
	for(unsigned  i = WP_DELAY-1; i > 0; i--)  wp[i] = wp[i-1];

	// Read into buffer memory if capacity is available
	if(/* wp <= fp' */ ptr_t(wp[0]-fp) >= 0) {
		T  x;
		if(src.read_nb(x))  buf[ap_uint<ADDR_BITS>(wp[0]++)] = x;
	}

	// Try to clear output buffer
	if(ovld)  ovld = !dst.write_nb(obuf);

	// Try to refill output buffer
	if(!ovld) {
		obuf.dat = buf[ap_uint<ADDR_BITS>(rp)];

		if(/* rp < wp */ ptr_t(rp-wp[WP_DELAY-1]) < 0) {
			auto const  t = nest.tick();
			rp += std::get<0>(t);
			fp += std::get<1>(t);

			if(M >= 0)  obuf.lst = std::get<2>(t)[M];
			ovld = true;
		}
	}

} // input_gen()

#endif
