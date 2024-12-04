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
#include <ostream>
#include <fstream>
#include <cstddef>
#include <hls_vector.h>
#include <hls_stream.h>


//- Compile-Time Functions --------------------------------------------------

// ceil(log2(x))
template<typename T>
constexpr unsigned clog2(T  x) {
  return  x<2? 0 : 1+clog2((x+1)/2);
}

template<typename T>
constexpr unsigned gcd(T  a, T  b) {
	if(b == 0)  return  a;
	else {
		T const  r = a%b;
		return  (r == 0)? b : gcd(b, r);
	}
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

//- Zero-Width-Enabled Arbitrary-Precision Numbers ..........................

// Non-zero-width instances as thinnest possible wrapper around ap_uint<N>.
template<unsigned N> class ap_uzint : public ap_uint<N> {
public:
	template<typename... T> ap_uzint(T&&... val) : ap_uint<N>(std::forward<T>(val)...) {}
};

// Zero-width specialization avoiding the illegal ap_uint<0> parametrization.
template<> class ap_uzint<0> : public std::false_type {
	// Enable initialization and assignment from any source type with complete truncation.
public:
	template<typename... T> ap_uzint(T&&...) {}
	template<typename T> ap_uzint& operator=(T&&) { return *this; }

	// Allow but neutralize increment operators
	ap_uzint& operator++()    { return *this; }
	ap_uzint& operator++(int) { return *this; }
	ap_uzint& operator--()    { return *this; }
	ap_uzint& operator--(int) { return *this; }
	template<typename T> ap_uzint& operator+=(T&&) { return *this; }
	template<typename T> ap_uzint& operator-=(T&&) { return *this; }
};

// Non-zero-width instances as thinnest possible wrapper around ap_uint<N>.
template<unsigned N> class ap_zint : public ap_int<N> {
public:
	template<typename... T> ap_zint(T&&... val) : ap_int<N>(std::forward<T>(val)...) {}
};

// Zero-width specialization avoiding the illegal ap_uint<0> parametrization.
template<> class ap_zint<0> : public std::false_type {
	// Enable initialization and assignment from any source type with complete truncation.
public:
	template<typename... T> ap_zint(T&&...) {}
	template<typename T> ap_zint& operator=(T&&) { return *this; }

	// Allow but neutralize increment operators
	ap_zint& operator++()    { return *this; }
	ap_zint& operator++(int) { return *this; }
	ap_zint& operator--()    { return *this; }
	ap_zint& operator--(int) { return *this; }
	template<typename T> ap_zint& operator+=(T&&) { return *this; }
	template<typename T> ap_zint& operator-=(T&&) { return *this; }
};

//- hls::vector<> Enablement ------------------------------------------------
template<typename  T, size_t  N>
inline std::ostream& operator<<(std::ostream &o, hls::vector<T, N> const &v) {
	char  delim = '{';
	for(auto const &x : v) {
		o << delim << x;
		delim = ':';
	}
	return (o << '}');
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
