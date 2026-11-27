/****************************************************************************
 * Copyright (C) Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @brief	Bit images: bit widths and raw bit representations of element types.
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 *
 * @description
 *  width_v<T> gives the width of the bit image of T. to_bitimage() and
 *  from_bitimage() convert between T and its bit image ap_uint<width_v<T>>.
 *  The ap_[u]fixed and ap_float conversions depend on the guards of their
 *  headers: ap_int.h pulls in ap_fixed.h, and ap_float.h is included here
 *  from Vitis HLS 2024.2 on, as in utils.hpp.
 ***************************************************************************/
#ifndef BITIMAGE_HPP
#define BITIMAGE_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <ap_int.h>
#include <utils/hls_version.h>
#if XILINX_HLS_VERSION_RELEASE >= 20242
# include <ap_float.h>
#endif

#include "bipolar.hpp"


// Determine bit width of types
//  Primary template dispatches via SFINAE: prefers T::width if available,
//  else falls back to 8*sizeof(T).  Specialize width_trait<T> directly
//  for types that need a custom width (e.g. Bipolar).
template<typename  T, typename = void>
struct width_trait : std::integral_constant<size_t, 8*sizeof(T)> {};

template<typename  T>
struct width_trait<T, typename std::enable_if<(T::width, true)>::type>
	: std::integral_constant<size_t, T::width> {};

template<typename  T> constexpr size_t  width_v = width_trait<T>::value;

// Bipolar — width is 1 bit
template<> struct width_trait<Bipolar> : std::integral_constant<size_t, 1> {};


//===========================================================================
// Bit-image conversion: type-safe reinterpretation between T and ap_uint<W>.
//  - to_bitimage(val)         : T -> ap_uint<width_v<T>>
//  - BitImage<T>::from(bits)  : ap_uint<width_v<T>> -> T
//===========================================================================

//-- to_bitimage: obtain raw bit representation -----------------------------
template<typename  T>
ap_uint<width_v<T>> to_bitimage(T const &val) {
#pragma HLS inline
	static_assert(std::is_integral<T>::value, "Non-integral types require bit image specialization.");
	return  val;
}

inline
ap_uint<1> to_bitimage(Bipolar const &val) {
#pragma HLS inline
	return  ap_uint<1>(val);
}

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

#ifdef __AP_FIXED_H__
template<int  W, int  I, ap_q_mode  Q, ap_o_mode  O, int  N>
ap_uint<W> to_bitimage(ap_fixed<W, I, Q, O, N> const &val) {
#pragma HLS inline
	return  val(W-1, 0);
}
template<int  W, int  I, ap_q_mode  Q, ap_o_mode  O, int  N>
ap_uint<W> to_bitimage(ap_ufixed<W, I, Q, O, N> const &val) {
#pragma HLS inline
	return  val(W-1, 0);
}
#endif

#ifdef __AP_FLOAT_H__
template<int  W, int  E>
ap_uint<W> to_bitimage(ap_float<W, E> const &val) {
	return (ap_uint<1>(val.sign_ref()), val.exponent_ref(), val.mantissa_ref());
}
#endif

inline
ap_uint<16> to_bitimage(half const &val) {
#pragma HLS inline
	union { uint16_t  i; half  f; } const  conv = { .f = val };
	return  to_bitimage(conv.i);
}
inline
ap_uint<32> to_bitimage(float const &val) {
#pragma HLS inline
	union { uint32_t  i; float  f; } const  conv = { .f = val };
	return  to_bitimage(conv.i);
}
inline
ap_uint<64> to_bitimage(double const &val) {
#pragma HLS inline
	union { uint64_t  i; double  f; } const  conv = { .f = val };
	return  to_bitimage(conv.i);
}

//-- BitImage<T>::from: reconstruct typed value from raw bits ---------------
template<typename  T>
struct BitImage {
	static_assert(std::is_integral<T>::value, "Non-integral types require specialization.");
	static T from(ap_uint<width_v<T>> const &bits) {
#pragma HLS inline
		return  bits;
	}
};

template<int  W>
struct BitImage<ap_uint<W>> {
	static ap_uint<W> from(ap_uint<W> const &bits) {
#pragma HLS inline
		return  bits;
	}
};
template<int  W>
struct BitImage<ap_int<W>> {
	static ap_int<W> from(ap_uint<W> const &bits) {
#pragma HLS inline
		return  bits;
	}
};

#ifdef __AP_FIXED_H__
template<int  W, int  I, ap_q_mode  Q, ap_o_mode  O, int  N>
struct BitImage<ap_fixed<W, I, Q, O, N>> {
	static ap_fixed<W, I, Q, O, N> from(ap_uint<W> const &bits) {
#pragma HLS inline
		ap_fixed<W, I, Q, O, N>  result;
		result(W-1, 0) = bits;
		return  result;
	}
};
template<int  W, int  I, ap_q_mode  Q, ap_o_mode  O, int  N>
struct BitImage<ap_ufixed<W, I, Q, O, N>> {
	static ap_ufixed<W, I, Q, O, N> from(ap_uint<W> const &bits) {
#pragma HLS inline
		ap_ufixed<W, I, Q, O, N>  result;
		result(W-1, 0) = bits;
		return  result;
	}
};
#endif

#ifdef __AP_FLOAT_H__
template<int  W, int  E>
struct BitImage<ap_float<W, E>> {
	static ap_float<W, E> from(ap_uint<W> const &bits) {
		constexpr int  M = W - E;
		return  ap_float<W, E>(bits[W-1], bits(W-2, M-1), bits(M-2, 0));
	}
};
#endif

template<>
struct BitImage<half> {
	static half from(ap_uint<16> const &bits) {
#pragma HLS inline
		union { uint16_t  i; half  f; } const  conv = { .i = uint16_t(bits) };
		return  conv.f;
	}
};
template<>
struct BitImage<float> {
	static float from(ap_uint<32> const &bits) {
#pragma HLS inline
		union { uint32_t  i; float  f; } const  conv = { .i = uint32_t(bits) };
		return  conv.f;
	}
};
template<>
struct BitImage<double> {
	static double from(ap_uint<64> const &bits) {
#pragma HLS inline
		union { uint64_t  i; double  f; } const  conv = { .i = uint64_t(bits) };
		return  conv.f;
	}
};
template<>
struct BitImage<Bipolar> {
	static Bipolar from(ap_uint<1> const &bits) {
#pragma HLS inline
		return  Bipolar(bits);
	}
};

template<typename  T>
T from_bitimage(ap_uint<width_v<T>> const &bits) {
	return  BitImage<T>::from(bits);
}

#endif
