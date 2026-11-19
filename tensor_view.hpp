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
 * @brief	Non-owning tensor views: reshape, slice, and transpose.
 * @author	Thomas B. Preusser <thomas.preusser@amd.com>
 *******************************************************************************/

#ifndef TENSOR_VIEW_HPP
#define TENSOR_VIEW_HPP

#include "tensor.hpp"


//===========================================================================
// tensor_view: offset-aware non-owning view into an ap_uint<HOST_W>
//===========================================================================

//-- 1-D base case -----------------------------------------------------------
template<typename T, unsigned HOST_W, unsigned N>
class tensor_view<T, HOST_W, N> {
	static constexpr unsigned  W = width_v<T>;

	ap_uint<HOST_W>  &m_data;
	unsigned const    m_off;

public:
	static constexpr unsigned  width = N * W;

	tensor_view(ap_uint<HOST_W> &data, unsigned off = 0) : m_data(data), m_off(off) {}

	//-- Data access ---------------------------------------------------------
	ap_uint<HOST_W>&       data()       { return  m_data; }
	ap_uint<HOST_W> const& data() const { return  m_data; }
	unsigned offset() const { return  m_off; }

	//-- Size ----------------------------------------------------------------
	static constexpr unsigned size() { return  N; }

	//-- Element access ------------------------------------------------------
	T operator[](unsigned i) const {
#pragma HLS inline
		unsigned const  off = m_off + i * W;
		return  BitImage<T>::from(ap_uint<W>(m_data(off + W - 1, off)));
	}

	tensor_ref<T, W, HOST_W> operator[](unsigned i) {
#pragma HLS inline
		return  tensor_ref<T, W, HOST_W>(m_data, m_off + i * W);
	}

	//-- Snapshot: owning copy -----------------------------------------------
	operator tensor<T, N>() const {
#pragma HLS inline
		return  tensor<T, N>(ap_uint<width>(m_data(m_off + width - 1, m_off)));
	}

	//-- Write-through assignment --------------------------------------------
	tensor_view& operator=(tensor<T, N> const &t) {
#pragma HLS inline
		m_data(m_off + width - 1, m_off) = ap_uint<width>(t);
		return  *this;
	}

	//-- Reshape (reinterpret under new shape) -------------------------------
	template<typename T_new, unsigned ...NEW_DIMS>
	tensor_view<T_new, HOST_W, NEW_DIMS...> reshape() {
#pragma HLS inline
		static_assert(width == tensor<T_new, NEW_DIMS...>::width,
			"reshape: total bit-width must match");
		return  tensor_view<T_new, HOST_W, NEW_DIMS...>(m_data, m_off);
	}

	//-- Const iteration: yields T by value ----------------------------------
	class const_iterator {
		ap_uint<HOST_W> const  &m_data;
		unsigned const          m_base;
		unsigned                m_idx;
	public:
		const_iterator(ap_uint<HOST_W> const &data, unsigned base, unsigned idx)
			: m_data(data), m_base(base), m_idx(idx) {}
		T operator*() const {
			unsigned const  off = m_base + m_idx * W;
			return  BitImage<T>::from(ap_uint<W>(m_data(off + W - 1, off)));
		}
		const_iterator& operator++() { ++m_idx; return  *this; }
		bool operator!=(const_iterator const &o) const { return  m_idx != o.m_idx; }
	};
	const_iterator begin() const { return  { m_data, m_off, 0 }; }
	const_iterator end()   const { return  { m_data, m_off, N }; }

	//-- Mutable iteration: yields tensor_ref write-through proxies ----------
	class iterator {
		ap_uint<HOST_W>  &m_data;
		unsigned const    m_base;
		unsigned          m_idx;
	public:
		iterator(ap_uint<HOST_W> &data, unsigned base, unsigned idx)
			: m_data(data), m_base(base), m_idx(idx) {}
		tensor_ref<T, W, HOST_W> operator*() { return  { m_data, m_base + m_idx * W }; }
		iterator& operator++() { ++m_idx; return  *this; }
		bool operator!=(iterator const &o) const { return  m_idx != o.m_idx; }
	};
	iterator begin() { return  { m_data, m_off, 0 }; }
	iterator end()   { return  { m_data, m_off, N }; }

	//-- Balanced binary tree reduction --------------------------------------
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
template<typename T, unsigned HOST_W, unsigned DIM0, unsigned ...REST>
class tensor_view<T, HOST_W, DIM0, REST...> {
public:
	using inner_t = tensor<T, REST...>;

private:
	static constexpr unsigned  INNER_W = inner_t::width;

	ap_uint<HOST_W>  &m_data;
	unsigned const    m_off;

public:
	static constexpr unsigned  width = DIM0 * INNER_W;

	tensor_view(ap_uint<HOST_W> &data, unsigned off = 0) : m_data(data), m_off(off) {}

	//-- Data access ---------------------------------------------------------
	ap_uint<HOST_W>&       data()       { return  m_data; }
	ap_uint<HOST_W> const& data() const { return  m_data; }
	unsigned offset() const { return  m_off; }

	//-- Size ----------------------------------------------------------------
	static constexpr unsigned size() { return  DIM0; }

	//-- Element access ------------------------------------------------------
	inner_t operator[](unsigned i) const {
#pragma HLS inline
		unsigned const  off = m_off + i * INNER_W;
		return  BitImage<inner_t>::from(ap_uint<INNER_W>(m_data(off + INNER_W - 1, off)));
	}

	tensor_ref<inner_t, INNER_W, HOST_W> operator[](unsigned i) {
#pragma HLS inline
		return  tensor_ref<inner_t, INNER_W, HOST_W>(m_data, m_off + i * INNER_W);
	}

	//-- Snapshot: owning copy -----------------------------------------------
	operator tensor<T, DIM0, REST...>() const {
#pragma HLS inline
		return  tensor<T, DIM0, REST...>(ap_uint<width>(m_data(m_off + width - 1, m_off)));
	}

	//-- Write-through assignment --------------------------------------------
	tensor_view& operator=(tensor<T, DIM0, REST...> const &t) {
#pragma HLS inline
		m_data(m_off + width - 1, m_off) = ap_uint<width>(t);
		return  *this;
	}

	//-- Reshape (reinterpret under new shape) -------------------------------
	template<typename T_new, unsigned ...NEW_DIMS>
	tensor_view<T_new, HOST_W, NEW_DIMS...> reshape() {
#pragma HLS inline
		static_assert(width == tensor<T_new, NEW_DIMS...>::width,
			"reshape: total bit-width must match");
		return  tensor_view<T_new, HOST_W, NEW_DIMS...>(m_data, m_off);
	}

	//-- Slice: fix leading index, return view of remainder ------------------
	tensor_view<T, HOST_W, REST...> slice(unsigned i) {
#pragma HLS inline
		return  tensor_view<T, HOST_W, REST...>(m_data, m_off + i * INNER_W);
	}

	inner_t slice(unsigned i) const {
#pragma HLS inline
		return  (*this)[i];
	}

	//-- Range slice: select leading indices [LO, HI] -----------------------
	template<unsigned HI, unsigned LO>
	tensor_view<T, HOST_W, HI-LO+1, REST...> slice() {
#pragma HLS inline
		static_assert(LO <= HI && HI < DIM0, "slice: invalid range");
		return  tensor_view<T, HOST_W, HI-LO+1, REST...>(m_data, m_off + LO * INNER_W);
	}

	template<unsigned HI, unsigned LO>
	tensor<T, HI-LO+1, REST...> slice() const {
#pragma HLS inline
		static_assert(LO <= HI && HI < DIM0, "slice: invalid range");
		constexpr unsigned  SLICE_W = (HI-LO+1) * INNER_W;
		unsigned const  off = m_off + LO * INNER_W;
		return  tensor<T, HI-LO+1, REST...>(ap_uint<SLICE_W>(m_data(off + SLICE_W - 1, off)));
	}

	//-- Runtime range slice: write-through proxy over [lo, hi] -------------
	tensor_range_ref<INNER_W, HOST_W> slice(unsigned hi, unsigned lo) {
#pragma HLS inline
		return  tensor_range_ref<INNER_W, HOST_W>(m_data, m_off + lo * INNER_W, hi - lo + 1);
	}

	//-- Const iteration: yields inner_t by value ----------------------------
	class const_iterator {
		ap_uint<HOST_W> const  &m_data;
		unsigned const          m_base;
		unsigned                m_idx;
	public:
		const_iterator(ap_uint<HOST_W> const &data, unsigned base, unsigned idx)
			: m_data(data), m_base(base), m_idx(idx) {}
		inner_t operator*() const {
			unsigned const  off = m_base + m_idx * INNER_W;
			return  BitImage<inner_t>::from(ap_uint<INNER_W>(m_data(off + INNER_W - 1, off)));
		}
		const_iterator& operator++() { ++m_idx; return  *this; }
		bool operator!=(const_iterator const &o) const { return  m_idx != o.m_idx; }
	};
	const_iterator begin() const { return  { m_data, m_off, 0 }; }
	const_iterator end()   const { return  { m_data, m_off, DIM0 }; }

	//-- Mutable iteration: yields tensor_ref write-through proxies ----------
	class iterator {
		ap_uint<HOST_W>  &m_data;
		unsigned const    m_base;
		unsigned          m_idx;
	public:
		iterator(ap_uint<HOST_W> &data, unsigned base, unsigned idx)
			: m_data(data), m_base(base), m_idx(idx) {}
		tensor_ref<inner_t, INNER_W, HOST_W> operator*() { return  { m_data, m_base + m_idx * INNER_W }; }
		iterator& operator++() { ++m_idx; return  *this; }
		bool operator!=(iterator const &o) const { return  m_idx != o.m_idx; }
	};
	iterator begin() { return  { m_data, m_off, 0 }; }
	iterator end()   { return  { m_data, m_off, DIM0 }; }

	//-- Balanced binary tree reduction --------------------------------------
	template<typename TR = inner_t, typename F>
	TR reduce(F &&f) const {
#pragma HLS inline
		TR  tree[2*DIM0-1];
#pragma HLS array_partition complete dim=1 variable=tree
		for(unsigned  i = DIM0; i-- > 0;) {
#pragma HLS unroll
			tree[DIM0-1 + i] = (*this)[i];
		}
		for(unsigned  i = DIM0-1; i-- > 0;) {
#pragma HLS unroll
			tree[i] = f(tree[2*i+1], tree[2*i+2]);
		}
		return  tree[0];
	}
};


//===========================================================================
// Deferred member bodies for tensor (declared in tensor.hpp)
//===========================================================================

//-- reshape() on tensor_common --------------------------------------------
template<typename Derived, typename InnerT, unsigned Count>
template<typename T_new, unsigned ...NEW_DIMS>
auto tensor_common<Derived, InnerT, Count>::reshape()
	-> tensor_view<T_new, width, NEW_DIMS...>
{
#pragma HLS inline
	static_assert(width == tensor<T_new, NEW_DIMS...>::width,
		"reshape: total bit-width must match");
	return  tensor_view<T_new, width, NEW_DIMS...>(data(), 0);
}

template<typename Derived, typename InnerT, unsigned Count>
template<typename T_new, unsigned ...NEW_DIMS>
auto tensor_common<Derived, InnerT, Count>::reshape() const
	-> tensor<T_new, NEW_DIMS...>
{
#pragma HLS inline
	static_assert(width == tensor<T_new, NEW_DIMS...>::width,
		"reshape: total bit-width must match");
	return  tensor<T_new, NEW_DIMS...>(ap_uint<width>(data()));
}

//-- slice() on tensor<T, DIM0, REST...> ------------------------------------
template<typename T, unsigned DIM0, unsigned ...REST>
auto tensor<T, DIM0, REST...>::slice(unsigned i)
	-> tensor_view<T, width, REST...>
{
#pragma HLS inline
	return  tensor_view<T, width, REST...>(m_data, i * INNER_W);
}

template<typename T, unsigned DIM0, unsigned ...REST>
auto tensor<T, DIM0, REST...>::slice(unsigned i) const
	-> inner_t
{
#pragma HLS inline
	return  (*this)[i];
}

template<typename T, unsigned DIM0, unsigned ...REST>
template<unsigned HI, unsigned LO>
auto tensor<T, DIM0, REST...>::slice()
	-> tensor_view<T, width, HI-LO+1, REST...>
{
#pragma HLS inline
	static_assert(LO <= HI && HI < DIM0, "slice: invalid range");
	return  tensor_view<T, width, HI-LO+1, REST...>(m_data, LO * INNER_W);
}

template<typename T, unsigned DIM0, unsigned ...REST>
template<unsigned HI, unsigned LO>
auto tensor<T, DIM0, REST...>::slice() const
	-> tensor<T, HI-LO+1, REST...>
{
#pragma HLS inline
	static_assert(LO <= HI && HI < DIM0, "slice: invalid range");
	constexpr unsigned  SLICE_W = (HI-LO+1) * INNER_W;
	return  tensor<T, HI-LO+1, REST...>(ap_uint<SLICE_W>(m_data(LO*INNER_W + SLICE_W - 1, LO*INNER_W)));
}


//-- tensor -> tensor_view conversion operators -----------------------------
template<typename T, unsigned N>
tensor<T, N>::operator tensor_view<T, tensor<T,N>::width, N>() {
	return  tensor_view<T, width, N>(m_data, 0);
}

template<typename T, unsigned N>
tensor<T, N>::operator tensor_view<T, tensor<T,N>::width, N>() const {
	return  tensor_view<T, width, N>(const_cast<ap_uint<width>&>(m_data), 0);
}

template<typename T, unsigned DIM0, unsigned ...REST>
tensor<T, DIM0, REST...>::operator tensor_view<T, tensor<T,DIM0,REST...>::width, DIM0, REST...>() {
	return  tensor_view<T, width, DIM0, REST...>(m_data, 0);
}

template<typename T, unsigned DIM0, unsigned ...REST>
tensor<T, DIM0, REST...>::operator tensor_view<T, tensor<T,DIM0,REST...>::width, DIM0, REST...>() const {
	return  tensor_view<T, width, DIM0, REST...>(const_cast<ap_uint<width>&>(m_data), 0);
}


//===========================================================================
// Printing for tensor_view
//===========================================================================
template<typename T, unsigned HOST_W, unsigned N>
inline std::ostream& operator<<(std::ostream &o, tensor_view<T, HOST_W, N> const &v) {
	char  delim = '{';
	for(auto const &e : v) {
		o << delim << e;
		delim = ':';
	}
	return (o << '}');
}


//===========================================================================
// Compile-time permutation helpers
//===========================================================================

template<unsigned ...Is>
struct perm {};

namespace detail {

//-- Get N-th element from a parameter pack ----------------------------------
template<unsigned Idx, unsigned Head, unsigned ...Tail>
struct nth : nth<Idx-1, Tail...> {};

template<unsigned Head, unsigned ...Tail>
struct nth<0, Head, Tail...> { static constexpr unsigned value = Head; };

//-- Compute stride for dimension D in shape <DIMS...> ----------------------
//   stride(D) = product of DIMS[D+1] * ... * DIMS[N-1] * width_v<T>
template<typename T, unsigned D, unsigned ...DIMS>
struct stride_at;

// Recursive: skip dimensions until we reach position D
template<typename T, unsigned D, unsigned Head, unsigned ...Tail>
struct stride_at<T, D, Head, Tail...> : stride_at<T, D-1, Tail...> {};

// Base: D==0 means this is the target dimension; stride = product of remaining * W
template<typename T, unsigned Head, unsigned ...Tail>
struct stride_at<T, 0, Head, Tail...> {
	static constexpr unsigned value = tensor<T, Tail...>::width;
};

// Edge case: D==0 and no Tail means innermost dimension
template<typename T, unsigned Head>
struct stride_at<T, 0, Head> {
	static constexpr unsigned value = width_v<T>;
};

//-- Validate permutation: check all values in [0, N) are present exactly once
template<unsigned N, unsigned ...Is>
struct is_valid_perm {
	static constexpr bool check() {
		unsigned  seen[N] = {};
		unsigned  vals[] = { Is... };
		for(unsigned i = 0; i < sizeof...(Is); i++) {
			if(vals[i] >= N)  return  false;
			if(seen[vals[i]])  return  false;
			seen[vals[i]] = 1;
		}
		return  sizeof...(Is) == N;
	}
	static constexpr bool value = check();
};

//-- Reorder dimensions according to permutation -----------------------------
template<typename T, typename Perm, unsigned ...DIMS>
struct permuted_dim;

template<typename T, unsigned ...Is, unsigned ...DIMS>
struct permuted_dim<T, perm<Is...>, DIMS...> {
	// The I-th permuted dimension is DIMS[PERM[I]]
	template<unsigned I>
	static constexpr unsigned dim() { return  nth<nth<I, Is...>::value, DIMS...>::value; }
};

} // namespace detail


//===========================================================================
// transpose_cursor: chained operator[] for transposed element access
//===========================================================================

// Forward declaration
template<typename T, unsigned HOST_W, typename StridePack, typename DimPack>
class transpose_cursor;

namespace detail {
	template<unsigned ...Vs> struct uint_pack {};
}

//-- Terminal cursor: all dimensions consumed --------------------------------
template<typename T, unsigned HOST_W>
class transpose_cursor<T, HOST_W, detail::uint_pack<>, detail::uint_pack<>> {
	ap_uint<HOST_W>  &m_host;
	unsigned const    m_off;

public:
	transpose_cursor(ap_uint<HOST_W> &host, unsigned off) : m_host(host), m_off(off) {}

	operator T() const {
#pragma HLS inline
		constexpr unsigned  W = width_v<T>;
		return  BitImage<T>::from(ap_uint<W>(m_host(m_off + W - 1, m_off)));
	}

	transpose_cursor& operator=(T const &val) {
#pragma HLS inline
		constexpr unsigned  W = width_v<T>;
		m_host(m_off + W - 1, m_off) = to_bitimage(val);
		return  *this;
	}
};

//-- Recursive cursor: more dimensions to index ------------------------------
template<typename T, unsigned HOST_W,
	unsigned STRIDE_HEAD, unsigned ...STRIDE_TAIL,
	unsigned DIM_HEAD, unsigned ...DIM_TAIL>
class transpose_cursor<T, HOST_W,
	detail::uint_pack<STRIDE_HEAD, STRIDE_TAIL...>,
	detail::uint_pack<DIM_HEAD, DIM_TAIL...>>
{
	ap_uint<HOST_W>  &m_host;
	unsigned const    m_off;

public:
	transpose_cursor(ap_uint<HOST_W> &host, unsigned off) : m_host(host), m_off(off) {}

	static constexpr unsigned size() { return  DIM_HEAD; }

	auto operator[](unsigned i) const
		-> transpose_cursor<T, HOST_W,
			detail::uint_pack<STRIDE_TAIL...>,
			detail::uint_pack<DIM_TAIL...>> const
	{
#pragma HLS inline
		return  { const_cast<ap_uint<HOST_W>&>(m_host), m_off + i * STRIDE_HEAD };
	}

	auto operator[](unsigned i)
		-> transpose_cursor<T, HOST_W,
			detail::uint_pack<STRIDE_TAIL...>,
			detail::uint_pack<DIM_TAIL...>>
	{
#pragma HLS inline
		return  { m_host, m_off + i * STRIDE_HEAD };
	}
};


//===========================================================================
// tensor_transpose: generic N-D permuted view
//===========================================================================

// Helper: build stride and dim packs for the permuted view
namespace detail {

template<typename T, typename Perm, typename DimTuple>
struct transpose_traits;

template<typename T, unsigned ...Is, unsigned ...DIMS>
struct transpose_traits<T, perm<Is...>, uint_pack<DIMS...>> {
	// Original strides reordered by permutation
	using stride_pack = uint_pack<stride_at<T, Is, DIMS...>::value...>;
	// Dimensions reordered by permutation
	using dim_pack = uint_pack<nth<Is, DIMS...>::value...>;
};

} // namespace detail

template<typename T, unsigned HOST_W, unsigned ...Is, unsigned ...DIMS>
class tensor_transpose<T, HOST_W, perm<Is...>, DIMS...> {
	static_assert(detail::is_valid_perm<sizeof...(DIMS), Is...>::value,
		"transpose: invalid permutation");
	static_assert(sizeof...(Is) == sizeof...(DIMS),
		"transpose: permutation size must match number of dimensions");

	using traits = detail::transpose_traits<T, perm<Is...>, detail::uint_pack<DIMS...>>;
	using stride_pack = typename traits::stride_pack;
	using dim_pack    = typename traits::dim_pack;

	ap_uint<HOST_W>  &m_data;
	unsigned const    m_off;

public:
	static constexpr unsigned  width = tensor<T, DIMS...>::width;

	tensor_transpose(ap_uint<HOST_W> &data, unsigned off = 0) : m_data(data), m_off(off) {}

	using top_cursor = transpose_cursor<T, HOST_W, stride_pack, dim_pack>;

	//-- Element access via cursor chain -------------------------------------
	auto operator[](unsigned i) const
		-> decltype(std::declval<top_cursor const>()[i])
	{
#pragma HLS inline
		top_cursor const  cur(const_cast<ap_uint<HOST_W>&>(m_data), m_off);
		return  cur[i];
	}

	auto operator[](unsigned i)
		-> decltype(std::declval<top_cursor>()[i])
	{
#pragma HLS inline
		return  top_cursor(m_data, m_off)[i];
	}

	//-- Size of outermost permuted dimension --------------------------------
	static constexpr unsigned size() {
		return  detail::nth<0, detail::nth<Is, DIMS...>::value...>::value;
	}
};

//-- transpose() deferred body -----------------------------------------------
template<typename T, unsigned DIM0, unsigned ...REST>
template<unsigned ...PERM>
auto tensor<T, DIM0, REST...>::transpose()
	-> tensor_transpose<T, width, perm<PERM...>, DIM0, REST...>
{
#pragma HLS inline
	return  tensor_transpose<T, width, perm<PERM...>, DIM0, REST...>(m_data, 0);
}

#endif // TENSOR_VIEW_HPP
