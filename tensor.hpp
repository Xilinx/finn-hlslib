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
#include "bipolar.hpp"


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


//===========================================================================
// Tensor
//===========================================================================

// Forward declarations
template<typename T, unsigned ...DIMS>
class tensor;

template<typename T, unsigned HOST_W, unsigned ...DIMS>
class tensor_view;

template<unsigned ...Is>
struct perm;

template<typename T, unsigned HOST_W, typename Perm, unsigned ...DIMS>
class tensor_transpose;

//-- Write-through proxy -----------------------------------------------------
//  References W bits within an arbitrary-width ap_uint at a computed offset.
//  Works for both scalar and tensor inner types via BitImage/to_bitimage.
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
	tensor_ref& operator=(tensor_ref const &src) {
#pragma HLS inline
		return  operator=(T(src));
	}

	// Chained indexing for tensor inner types (SFINAE: only when T has ref_at_static)
	template<typename U = T>
	auto operator[](unsigned i) -> decltype(U::template ref_at_static<HOST_W>(std::declval<ap_uint<HOST_W>&>(), 0u, 0u)) {
#pragma HLS inline
		return  T::template ref_at_static<HOST_W>(m_host, m_off, i);
	}
};

//-- Write-through range proxy -----------------------------------------------
//  References a contiguous run of INNER_W-wide elements within an ap_uint.
//  Runtime bounds, analogous to ap_range_ref.
template<unsigned INNER_W, unsigned HOST_W>
class tensor_range_ref {
	ap_uint<HOST_W>  &m_host;
	unsigned const    m_off;
	unsigned const    m_len;
public:
	tensor_range_ref(ap_uint<HOST_W> &host, unsigned off, unsigned len)
		: m_host(host), m_off(off), m_len(len) {}
	tensor_range_ref& operator=(tensor_range_ref const &src) {
#pragma HLS inline
		unsigned const  w = m_len * INNER_W;
		m_host(m_off + w - 1, m_off) = ap_uint<HOST_W>(src.m_host(src.m_off + w - 1, src.m_off));
		return  *this;
	}
};

//-- CRTP base: shared comparison, iteration, and size -----------------------
template<typename Derived, typename InnerT, unsigned Count>
class tensor_common {
	static constexpr unsigned  InnerW = width_v<InnerT>;
	static constexpr unsigned  width  = Count * InnerW;

public:
	auto&       data()       { return  static_cast<Derived*>(this)->m_data; }
	auto const& data() const { return  static_cast<Derived const*>(this)->m_data; }

	//-- Comparison (unsigned bit-image order) --------------------------------
	bool operator==(Derived const &o) const { return  data() == o.m_data; }
	bool operator!=(Derived const &o) const { return  data() != o.m_data; }
	bool operator< (Derived const &o) const { return  data() <  o.m_data; }
	bool operator<=(Derived const &o) const { return  data() <= o.m_data; }
	bool operator> (Derived const &o) const { return  data() >  o.m_data; }
	bool operator>=(Derived const &o) const { return  data() >= o.m_data; }

	//-- Size ----------------------------------------------------------------
	static constexpr unsigned size() { return  Count; }

	//-- Const iteration: yields InnerT by value -----------------------------
	class const_iterator {
		ap_uint<width> const  &m_data;
		unsigned               m_idx;
	public:
		const_iterator(ap_uint<width> const &data, unsigned idx) : m_data(data), m_idx(idx) {}
		InnerT operator*() const { return  BitImage<InnerT>::from(ap_uint<InnerW>(m_data((m_idx+1)*InnerW - 1, m_idx*InnerW))); }
		const_iterator& operator++() { ++m_idx; return  *this; }
		bool operator!=(const_iterator const &o) const { return  m_idx != o.m_idx; }
	};
	const_iterator begin() const { return  { data(), 0 }; }
	const_iterator end()   const { return  { data(), Count }; }

	//-- Mutable iteration: yields tensor_ref write-through proxies ----------
	class iterator {
		ap_uint<width>  &m_data;
		unsigned         m_idx;
	public:
		iterator(ap_uint<width> &data, unsigned idx) : m_data(data), m_idx(idx) {}
		tensor_ref<InnerT, InnerW, width> operator*() { return  { m_data, m_idx * InnerW }; }
		iterator& operator++() { ++m_idx; return  *this; }
		bool operator!=(iterator const &o) const { return  m_idx != o.m_idx; }
	};
	iterator begin() { return  { data(), 0 }; }
	iterator end()   { return  { data(), Count }; }

	//-- Runtime range slice: write-through proxy over [lo, hi] ---------------
	tensor_range_ref<InnerW, width> slice(unsigned hi, unsigned lo) {
#pragma HLS inline
		return  tensor_range_ref<InnerW, width>(data(), lo * InnerW, hi - lo + 1);
	}

	//-- Reshape: reinterpret bits under new shape (defined in tensor_view.hpp)
	template<typename T_new, unsigned ...NEW_DIMS>
	tensor_view<T_new, width, NEW_DIMS...> reshape();

	template<typename T_new, unsigned ...NEW_DIMS>
	tensor<T_new, NEW_DIMS...> reshape() const;

	//-- Balanced binary tree reduction over all Count elements ---------------
	template<typename TR = InnerT, typename F>
	TR reduce(F &&f) const {
#pragma HLS inline
		TR  tree[2*Count-1];
#pragma HLS array_partition complete dim=1 variable=tree
		for(unsigned  i = Count; i-- > 0;) {
#pragma HLS unroll
			tree[Count-1 + i] = static_cast<Derived const*>(this)->operator[](i);
		}
		for(unsigned  i = Count-1; i-- > 0;) {
#pragma HLS unroll
			tree[i] = f(tree[2*i+1], tree[2*i+2]);
		}
		return  tree[0];
	}
};


//-- 1-D base case -----------------------------------------------------------
template<typename T, unsigned N>
class tensor<T, N> : public tensor_common<tensor<T, N>, T, N> {
	friend class tensor_common<tensor<T, N>, T, N>;
	static constexpr unsigned  W = width_v<T>;

	ap_uint<N * W>  m_data;

public:
	static constexpr unsigned  width = N * W;

	using  inner_t = T;

	tensor() = default;
	explicit tensor(ap_uint<width> const &raw) : m_data(raw) {}

	// Broadcasting constructor: fill all N elements with the same value
	explicit tensor(T const &val) : m_data(0) {
#pragma HLS inline
		for(unsigned i = 0; i < N; i++) {
#pragma HLS unroll
			m_data((i+1)*W - 1, i*W) = to_bitimage(val);
		}
	}

	tensor(std::initializer_list<T> elems) : m_data(0) {
		auto  it = elems.begin();
		for(unsigned i = 0; i < N && it != elems.end(); i++, ++it) {
			m_data((i+1)*W - 1, i*W) = to_bitimage(*it);
		}
	}

	// Converting constructor: element-wise type conversion from tensor<U, N>
	template<typename U, typename = std::enable_if_t<!std::is_same<U, T>::value>>
	tensor(tensor<U, N> const &src) : m_data(0) {
#pragma HLS inline
		for(unsigned i = 0; i < N; i++) {
#pragma HLS unroll
			operator[](i) = T(src[i]);
		}
	}

	operator ap_uint<width>() const { return  m_data; }

	T operator[](unsigned i) const {
#pragma HLS inline
		return  BitImage<T>::from(ap_uint<W>(m_data((i+1)*W - 1, i*W)));
	}

	tensor_ref<T, W, width> operator[](unsigned i) {
#pragma HLS inline
		return  tensor_ref<T, W, width>(m_data, i * W);
	}

	// Static helper for chained ref_at from a parent's data store
	template<unsigned HOST_W>
	static tensor_ref<T, W, HOST_W> ref_at_static(ap_uint<HOST_W> &host, unsigned base, unsigned i) {
#pragma HLS inline
		return  tensor_ref<T, W, HOST_W>(host, base + i * W);
	}

	// View conversion (defined in tensor_view.hpp)
	operator tensor_view<T, width, N>();
	operator tensor_view<T, width, N>() const;

	// Pull in base-class begin/end (hidden by operator[] overloads)
	using tensor_common<tensor, T, N>::begin;
	using tensor_common<tensor, T, N>::end;
};

//-- N-D recursive case ------------------------------------------------------
template<typename T, unsigned DIM0, unsigned ...REST>
class tensor<T, DIM0, REST...> : public tensor_common<tensor<T, DIM0, REST...>, tensor<T, REST...>, DIM0> {
public:
	using inner_t = tensor<T, REST...>;

private:
	friend class tensor_common<tensor, inner_t, DIM0>;
	static constexpr unsigned  INNER_W = inner_t::width;

	ap_uint<DIM0 * INNER_W>  m_data;

public:
	static constexpr unsigned  width = DIM0 * INNER_W;

	tensor() = default;
	explicit tensor(ap_uint<width> const &raw) : m_data(raw) {}
	tensor(std::initializer_list<inner_t> elems) : m_data(0) {
		auto  it = elems.begin();
		for(unsigned i = 0; i < DIM0 && it != elems.end(); i++, ++it) {
			m_data((i+1)*INNER_W - 1, i*INNER_W) = ap_uint<INNER_W>(*it);
		}
	}
	operator ap_uint<width>() const { return  m_data; }

	inner_t const operator[](unsigned i) const {
#pragma HLS inline
		return  BitImage<inner_t>::from(ap_uint<INNER_W>(m_data((i+1)*INNER_W - 1, i*INNER_W)));
	}

	tensor_ref<inner_t, INNER_W, width> operator[](unsigned i) {
#pragma HLS inline
		return  tensor_ref<inner_t, INNER_W, width>(m_data, i * INNER_W);
	}

	// Static helper for chained ref_at from a parent's data store
	template<unsigned HOST_W>
	static tensor_ref<inner_t, INNER_W, HOST_W> ref_at_static(ap_uint<HOST_W> &host, unsigned base, unsigned i) {
#pragma HLS inline
		return  tensor_ref<inner_t, INNER_W, HOST_W>(host, base + i * INNER_W);
	}

	// Slice: fix leading index, return view of remainder (defined in tensor_view.hpp)
	tensor_view<T, width, REST...> slice(unsigned i);
	inner_t slice(unsigned i) const;

	// Range slice: select leading indices [LO, HI] (defined in tensor_view.hpp)
	template<unsigned HI, unsigned LO>
	tensor_view<T, width, HI-LO+1, REST...> slice();

	template<unsigned HI, unsigned LO>
	tensor<T, HI-LO+1, REST...> slice() const;

	// Transpose: permuted view (defined in tensor_view.hpp)
	template<unsigned ...PERM>
	tensor_transpose<T, width, perm<PERM...>, DIM0, REST...> transpose();

	// View conversion (defined in tensor_view.hpp)
	operator tensor_view<T, width, DIM0, REST...>();
	operator tensor_view<T, width, DIM0, REST...>() const;

	// Pull in base-class members hidden by derived overloads
	using tensor_common<tensor, inner_t, DIM0>::slice;
	using tensor_common<tensor, inner_t, DIM0>::begin;
	using tensor_common<tensor, inner_t, DIM0>::end;
};


//===========================================================================
// Bit-image support for tensor types (width_trait is above, before tensor_common)
//===========================================================================
template<typename T, unsigned ...DIMS>
ap_uint<tensor<T, DIMS...>::width> to_bitimage(tensor<T, DIMS...> const &val) {
#pragma HLS inline
	return  val;
}

template<typename T, unsigned ...DIMS>
struct BitImage<tensor<T, DIMS...>> {
	static tensor<T, DIMS...> from(ap_uint<tensor<T, DIMS...>::width> const &bits) {
#pragma HLS inline
		return  tensor<T, DIMS...>(bits);
	}
};


//===========================================================================
// Tensor Printing
//===========================================================================
template<typename  T, unsigned  N>
inline std::ostream& operator<<(std::ostream &o, tensor<T, N> const &v) {
	char  delim = '{';
	for(auto const &e : v) {
		o << delim << e;
		delim = ':';
	}
	return (o << '}');
}

#endif // TENSOR_HPP
