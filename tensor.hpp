/******************************************************************************
 *  Copyright (C) Advanced Micro Devices, Inc.
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
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 * @brief	Little-endian tensor backed by ap_uint for unambiguous bit packing.
 * @author	Thomas B. Preusser <thomas.preusser@amd.com>
 *
 * @description
 *  Multi-dimensional tensor with element[0] at the LSB — matching the ap_uint
 *  and RTL memstream convention.  Backed by a single ap_uint<TOTAL_BITS> data
 *  member, so there is no packing ambiguity regardless of HLS aggregate pragmas.
 *  Element access via operator[] returns sub-tensors (by value) or typed
 *  elements, all resolved to wire selections in fully-unrolled HLS contexts.
 *******************************************************************************/

#ifndef TENSOR_HPP
#define TENSOR_HPP

#include <ap_int.h>
#include <ostream>
#include <initializer_list>
#include <utility>
#include "utils.hpp"


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

template<typename  T>
T from_bitimage(ap_uint<width_v<T>> const &bits) {
	return  BitImage<T>::from(bits);
}


//===========================================================================
// Tensor
//===========================================================================

// Forward declaration
template<typename T, unsigned ...DIMS>
class tensor;

//-- Leaf write-through proxy ------------------------------------------------
//  References W bits within an arbitrary-width ap_uint at a computed offset.
template<typename T, unsigned W, unsigned HOST_W>
class tensor_ref {
	ap_uint<HOST_W>  &m_host;
	unsigned const    m_off;
public:
	tensor_ref(ap_uint<HOST_W> &host, unsigned off) : m_host(host), m_off(off) {}
	operator T() const {
#pragma HLS inline
		return  BitImage<T>::from(ap_uint<W>(m_host(m_off + W - 1, m_off)));
	}
	tensor_ref& operator=(T const &val) {
#pragma HLS inline
		m_host(m_off + W - 1, m_off) = to_bitimage(val);
		return  *this;
	}
};

//-- N-D write-through proxy ------------------------------------------------
//  References INNER_W bits for a sub-tensor within an arbitrary-width ap_uint.
template<typename INNER_T, unsigned INNER_W, unsigned HOST_W>
class tensor_nd_ref {
	ap_uint<HOST_W>  &m_host;
	unsigned const    m_off;
public:
	tensor_nd_ref(ap_uint<HOST_W> &host, unsigned off) : m_host(host), m_off(off) {}
	operator INNER_T() const {
#pragma HLS inline
		return  INNER_T(ap_uint<INNER_W>(m_host(m_off + INNER_W - 1, m_off)));
	}
	tensor_nd_ref& operator=(INNER_T const &val) {
#pragma HLS inline
		m_host(m_off + INNER_W - 1, m_off) = ap_uint<INNER_W>(val);
		return  *this;
	}
	// Chained indexing: delegate to the inner tensor's ref_at_static
	// Template parameter defers instantiation until call site (after tensor is defined)
	template<typename IT = INNER_T>
	auto operator[](unsigned i) -> decltype(IT::template ref_at_static<HOST_W>(std::declval<ap_uint<HOST_W>&>(), 0u, 0u)) {
#pragma HLS inline
		return  IT::template ref_at_static<HOST_W>(m_host, m_off, i);
	}
};

//-- 1-D base case -----------------------------------------------------------
template<typename T, unsigned N>
class tensor<T, N> {
	static constexpr unsigned  W = width_v<T>;

	ap_uint<N * W>  m_data;

public:
	static constexpr unsigned  TOTAL = N * W;

	tensor() = default;
	explicit tensor(ap_uint<TOTAL> const &raw) : m_data(raw) {}
	tensor(std::initializer_list<T> elems) : m_data(0) {
		auto  it = elems.begin();
		for(unsigned i = 0; i < N && it != elems.end(); i++, ++it) {
			m_data((i+1)*W - 1, i*W) = to_bitimage(*it);
		}
	}
	operator ap_uint<TOTAL>() const { return  m_data; }

	T operator[](unsigned i) const {
#pragma HLS inline
		return  BitImage<T>::from(ap_uint<W>(m_data((i+1)*W - 1, i*W)));
	}

	tensor_ref<T, W, TOTAL> operator[](unsigned i) {
#pragma HLS inline
		return  tensor_ref<T, W, TOTAL>(m_data, i * W);
	}

	// Static helper for chained ref_at from a parent's data store
	template<unsigned HOST_W>
	static tensor_ref<T, W, HOST_W> ref_at_static(ap_uint<HOST_W> &host, unsigned base, unsigned i) {
#pragma HLS inline
		return  tensor_ref<T, W, HOST_W>(host, base + i * W);
	}

	// Balanced binary tree reduction over all N elements
	template<typename TR = T, typename F>
	TR reduce(F &&f) const {
#pragma HLS inline
		TR  tree[2*N-1];
#pragma HLS array_partition complete dim=1 variable=tree
		for(unsigned  i = N; i-- > 0;) {
#pragma HLS unroll
			tree[N-1 + i] = (*this)[i];
		}
		for(unsigned  i = N-1; i-- > 0;) {
#pragma HLS unroll
			tree[i] = f(tree[2*i+1], tree[2*i+2]);
		}
		return  tree[0];
	}
};

//-- N-D recursive case ------------------------------------------------------
template<typename T, unsigned DIM0, unsigned ...REST>
class tensor<T, DIM0, REST...> {
public:
	using inner_t = tensor<T, REST...>;

private:
	static constexpr unsigned  INNER_W = inner_t::TOTAL;

	ap_uint<DIM0 * INNER_W>  m_data;

public:
	static constexpr unsigned  TOTAL = DIM0 * INNER_W;

	tensor() = default;
	explicit tensor(ap_uint<TOTAL> const &raw) : m_data(raw) {}
	tensor(std::initializer_list<inner_t> elems) : m_data(0) {
		auto  it = elems.begin();
		for(unsigned i = 0; i < DIM0 && it != elems.end(); i++, ++it) {
			m_data((i+1)*INNER_W - 1, i*INNER_W) = ap_uint<INNER_W>(*it);
		}
	}
	operator ap_uint<TOTAL>() const { return  m_data; }

	inner_t const operator[](unsigned i) const {
#pragma HLS inline
		return  inner_t(ap_uint<INNER_W>(m_data((i+1)*INNER_W - 1, i*INNER_W)));
	}

	tensor_nd_ref<inner_t, INNER_W, TOTAL> operator[](unsigned i) {
#pragma HLS inline
		return  tensor_nd_ref<inner_t, INNER_W, TOTAL>(m_data, i * INNER_W);
	}

	// Static helper for chained ref_at from a parent's data store
	template<unsigned HOST_W>
	static tensor_nd_ref<inner_t, INNER_W, HOST_W> ref_at_static(ap_uint<HOST_W> &host, unsigned base, unsigned i) {
#pragma HLS inline
		return  tensor_nd_ref<inner_t, INNER_W, HOST_W>(host, base + i * INNER_W);
	}
};


//===========================================================================
// Tensor Printing
//===========================================================================
template<typename  T, unsigned  N>
inline std::ostream& operator<<(std::ostream &o, tensor<T, N> const &v) {
	char  delim = '{';
	for(unsigned  i = 0; i < N; i++) {
		o << delim << v[i];
		delim = ':';
	}
	return (o << '}');
}

#endif // TENSOR_HPP
