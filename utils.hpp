/******************************************************************************
 *  Copyright (c) 2019, Xilinx, Inc.
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
 *******************************************************************************/
 
/*******************************************************************************
 *
 *  Authors: Giulio Gambardella <giuliog@xilinx.com>
 *           Thomas B. Preusser <thomas.preusser@utexas.edu>
 *             Marie-Curie Fellow, Xilinx Ireland, Grant Agreement No. 751339
 *           Christoph Doehring <cdoehrin@xilinx.com>
 *
 *  @file utils.hpp
 *
 *  This project has received funding from the European Union's Framework
 *  Programme for Research and Innovation Horizon 2020 (2014-2020) under
 *  the Marie Skłodowska-Curie Grant Agreement No. 751339.
 *
 *******************************************************************************/

#ifndef UTILS_HPP
#define UTILS_HPP

#include <ap_int.h>

#include <iostream>
#include <fstream>
#include <cstddef>

//- Static Evaluation of ceil(log2(x)) ---------------------------------------
template<typename T>
constexpr unsigned clog2(T  x) {
  return  x<2? 0 : 1+clog2((x+1)/2);
}

//- Helpers to get hold of types ---------------------------------------------
template<typename T> struct first_param {};
template<typename R, typename A, typename... Args>
struct first_param<R (*)(A, Args...)> { typedef A  type; };
template<typename C, typename R, typename A, typename... Args>
struct first_param<R (C::*)(A, Args...)> { typedef A  type; };

//- Resource Representatives -------------------------------------------------
class ap_resource_dflt {};
class ap_resource_lut {};
class ap_resource_dsp {};
//- Resource Representatives for sliding window-------------------------------
class ap_resource_lutram {};
class ap_resource_bram {};
class ap_resource_uram {};

/**
 * \brief   Stream logger - Logging call to dump on file - not synthezisable
 *
 *
 * \tparam     BitWidth    Width, in number of bits, of the input (and output) stream
 *
 * \param      layer_name   File name of the dump
 * \param      log          Input (and output) stream
 *
 */
template < unsigned int BitWidth >
void logStringStream(const char *layer_name, hls::stream<ap_uint<BitWidth> > &log){
    std::ofstream ofs(layer_name);
    hls::stream<ap_uint<BitWidth> > tmp_stream;
	
  while(!log.empty()){
    ap_uint<BitWidth> tmp = (ap_uint<BitWidth>) log.read();
    ofs << std::hex << tmp << std::endl;
    tmp_stream.write(tmp);
  }

  while(!tmp_stream.empty()){
    ap_uint<BitWidth> tmp = tmp_stream.read();
    log.write((ap_uint<BitWidth>) tmp);
  }

  ofs.close();
}

//- Type Traits -------------------------------------------------------------

/**
 * Retrieving the return type from a function (member) pointer type.
 */
template<typename T>
struct return_value {};
template<typename  R, typename... Args>
struct return_value<R(Args...)> {
	using  type = R;
};
template<typename  R, typename... Args>
struct return_value<R(Args...) const> {
	using  type = R;
};
template<typename C, typename  R, typename... Args>
struct return_value<R (C::*)(Args...)> {
	using  type = R;
};
template<typename C, typename  R, typename... Args>
struct return_value<R (C::*)(Args...) const> {
	using  type = R;
};

/**
 * Scaling type bitwidths
 */
template<typename TI, int S, int O>
struct scale_width_trait;
template<int S, int O>
struct scale_width_trait<float, S, O> {
    using type = float;
};
template<int S, int O>
struct scale_width_trait<int, S, O> {
    using type = ap_int<sizeof(int) * S + O>;
};
template<int S, int O>
struct scale_width_trait<unsigned int, S, O> {
    using type = ap_uint<sizeof(unsigned int) * S + O>;
};
template<int W, int S, int O>
struct scale_width_trait<ap_int<W>, S, O> {
    using type = ap_int<W * S + O>;
};
template<int W, int S, int O>
struct scale_width_trait<ap_uint<W>, S, O> {
    using type = ap_uint<W * S + O>;
};
template<typename TI, int S = 1, int O = 0>
using scale_width = typename scale_width_trait<TI, S, O>::type; // Comfort trait for easy usage

template<typename T>
struct is_ap_float : std::false_type {};

template<int W, int I>
struct is_ap_float<ap_float<W,I>> : std::true_type {};

template<typename T>
struct is_floating_point_or_ap_float
    : std::integral_constant<bool, std::is_floating_point<T>::value || is_ap_float<T>::value> {};

template<typename T>
struct is_ap_int : std::false_type {};

template <int W>
struct is_ap_int<ap_int<W>> : std::true_type {};

template <int W>
struct is_ap_int<ap_uint<W>> : std::true_type {};

template<typename T>
struct is_integer_or_ap_int
    : std::integral_constant<bool, std::is_integral<T>::value || is_ap_int<T>::value> {};

//- Tree Reduce -------------------------------------------------------------
template<
	size_t    N,
	typename  TA,
	typename  F			// (TR, TR) -> TR
>
auto tree_reduce(hls::vector<TA, N> const &v, F &&f = F()) {
#pragma HLS inline
	using TR = decltype(f(v[0], v[1]));
	TR  tree[2*N-1];
#pragma HLS array_partition complete dim=1 variable=tree
	for(unsigned  i = N; i-- > 0;) {
#pragma HLS unroll
		tree[N-1 + i] = v[i];
	}
	for(unsigned  i = N-1; i-- > 0;) {
#pragma HLS unroll
		tree[i] = f(tree[2*i+1], tree[2*i+2]);
	}
	return  tree[0];
}

//- Modulus Counter ---------------------------------------------------------

/**
 * Modulus counter returning true upon each N-th call of tick.
 * @description
 *	The implementation internally counts from N-2, ..., 0, -1 wrapping back to
 *	N-2 so that the sign bit of the counter value can directly serve as the
 *	wrap-around indicator without requiring a multi-bit comparator.
 */
template<unsigned  N> class ModCounter {
	ap_int<1+clog2(N-1)>  cnt = N-2;
public:
	bool last() const {
#pragma HLS inline
		return  cnt < 0;
	}
	bool tick() {
#pragma HLS inline
		bool const  ret = last();
		cnt += ret? N-1 : -1;
		return  ret;
	}
};
template<> class ModCounter<1> {
public:
	bool last() const {
#pragma HLS inline
		return  true;
	}
	bool tick() const {
#pragma HLS inline
		return  true;
	}
};
template<> class ModCounter<0> {};

#endif
