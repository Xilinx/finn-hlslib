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
#include "interpret.hpp"

// Forward declaration
template<typename T, unsigned ...DIMS>
struct tensor;

//-- 1-D base case -----------------------------------------------------------
template<typename T, unsigned N>
struct tensor<T, N> {
	static constexpr unsigned  W     = width_v<T>;
	static constexpr unsigned  TOTAL = N * W;
	ap_uint<TOTAL>  data;

	tensor() = default;
	tensor(ap_uint<TOTAL> const &raw) : data(raw) {}

	T operator[](unsigned i) const {
#pragma HLS inline
		return  T(ap_uint<W>(data((i+1)*W - 1, i*W)));
	}
};

//-- N-D recursive case ------------------------------------------------------
template<typename T, unsigned DIM0, unsigned ...REST>
struct tensor<T, DIM0, REST...> {
	using inner_t = tensor<T, REST...>;
	static constexpr unsigned  INNER_W = inner_t::TOTAL;
	static constexpr unsigned  TOTAL   = DIM0 * INNER_W;
	ap_uint<TOTAL>  data;

	tensor() = default;
	tensor(ap_uint<TOTAL> const &raw) : data(raw) {}

	inner_t operator[](unsigned i) const {
#pragma HLS inline
		inner_t  sub;
		sub.data = data((i+1)*INNER_W - 1, i*INNER_W);
		return  sub;
	}
};

#endif
