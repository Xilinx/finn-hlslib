/******************************************************************************
 *  Copyright (c) 2024, Christoph Berganski
 *  Copyright (c) 2024, Advanced Micro Devices, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1.  Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
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

#ifndef FLATTEN_HPP
#define FLATTEN_HPP

#include <ap_int.h>
#include <ap_fixed.h>
#include <ap_float.h>

#include <hls_vector.h>
#include <cstddef>

#include "interpret.hpp"	// width_v(T)


//---------------------------------------------------------------------------
// Utility: Get the raw bit image of a value
template<typename  T>
ap_uint<width_v<T>> to_bitimage(T const &val) {
#pragma HLS inline
	static_assert(std::is_integral<T>::value, "Non-integral types require bit image specialization.");
	return  val;
}

// ap_*-Type Specializations
template<int  W>
ap_uint<W> to_bitimage(ap_int<W> const &val) {
#pragma HLS inline
	return  val;
}
template<int  W>
ap_uint<W> to_bitimage(ap_uint<W> const &val) {
#pragma HLS inline
	return  val;
}

template <int  W, int  I, ap_q_mode  Q, ap_o_mode  O, int  N>
ap_uint<W> to_bitimage(ap_fixed<W, I, Q, O, N> const &val) {
#pragma HLS inline
	return  val(W-1, 0);
}

template<int W, int E>
ap_uint<W> to_bitimage(ap_float<W, E> const &val) {
	return (ap_uint<1>(val.sign_ref()), val.exponent_ref(), val.mantissa_ref());
}

// Floating-point Specializations
ap_uint<16> to_bitimage(half const &val) {
#pragma HLS inline
	union { uint16_t  i; half  f; } const  conv = { .f = val };
	return  to_bitimage(conv.i);
}
ap_uint<32> to_bitimage(float const &val) {
#pragma HLS inline
	union { uint32_t  i; float  f; } const  conv = { .f = val };
	return  to_bitimage(conv.i);
}
ap_uint<64> to_bitimage(double const &val) {
#pragma HLS inline
	union { uint64_t  i; double  f; } const  conv = { .f = val };
	return  to_bitimage(conv.i);
}


//---------------------------------------------------------------------------
// Actual Flattening: all inlined, just wiring in HW
//	Ultimately (C++20), there should probably be a single copy leveraging a
//	concept that covers T[N], std::array<T, N> and hls::vector<T, N>.

// Flatten an array of N elements of type T into an ap_uint<>
template<
	typename  T,	// [inferred] Element type
	size_t    N 	// [inferred] Array size
>
ap_uint<N * width_v<T>> flatten(T const (&buffer)[N]) {
#pragma HLS INLINE
	constexpr size_t  W = width_v<T>;
	ap_uint<N * W>  flat;
	for(size_t  j = 0; j < N; j++) {
#pragma HLS UNROLL
		flat((j+1)*W - 1, j*W) = to_bitimage(buffer[j]);
	}
	return flat;
}

// Flatten an hls::vector<> of N elements of type T into an ap_uint<>
template<
	typename  T,	// [inferred] Element type
	size_t    N 	// [inferred] Vector length
>
ap_uint<N * width_v<T>> flatten(hls::vector<T, N> const &vec) {
#pragma HLS INLINE
	constexpr size_t  W = width_v<T>;
	ap_uint<N * W>  flat;
	for(size_t  j = 0; j < N; j++) {
#pragma HLS UNROLL
		flat((j+1)*W - 1, j*W) = to_bitimage(vec[j]);
	}
	return flat;
}

#endif // FLATTEN_HPP