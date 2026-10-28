/******************************************************************************
 * Copyright (c) 2019, Xilinx, Inc.
 * Copyright (c) 2025, AMD, Inc.
 * All rights reserved.
 *
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions are met:
 *
 *	1.	Redistributions of source code must retain the above copyright notice,
 *		 this list of conditions and the following disclaimer.
 *
 *	2.	Redistributions in binary form must reproduce the above copyright
 *			notice, this list of conditions and the following disclaimer in the
 *			documentation and/or other materials provided with the distribution.
 *
 *	3.	Neither the name of the copyright holder nor the names of its
 *			contributors may be used to endorse or promote products derived from
 *			this software without specific prior written permission.
 *
 *	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *	THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *	PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *	CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *	EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *	OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *	WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *	OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *	ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 * @author	Giulio Gambardella <giuliog@xilinx.com>
 * @author	Tobias Alonso <tobiasa@xilinx.com>
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ******************************************************************************/
#ifndef POOL_HPP
#define POOL_HPP

#include "tensor.hpp"
#include "utils.hpp"

#include <type_traits>
#include <limits>
#include <algorithm>


//===========================================================================
// Element-level operations

/*!
 * \brief  Element-level operation for max pooling.
 */
struct MaxOp {
	template<typename E>
	static E init() {
#pragma HLS inline
		return  std::numeric_limits<E>::min();
	}
	template<typename E>
	static E pool(E a, E b) {
#pragma HLS inline
		return  std::max(a, b);
	}
	template<typename EO, typename EA>
	static EO activate(EA a) {
#pragma HLS inline
		return  a;
	}
};

/*!
 * \brief  Element-level operation for sum pooling (activate = identity).
 */
struct SumOp {
	template<typename E>
	static E init() {
#pragma HLS inline
		return  E(0);
	}
	template<typename E>
	static E pool(E a, E b) {
#pragma HLS inline
		return  a + b;
	}
	template<typename EO, typename EA>
	static EO activate(EA a) {
#pragma HLS inline
		return  a;
	}
};

/*!
 * \brief  Element-level operation for average pooling (activate = divide by N).
 */
template<size_t N>
struct AvgOp : SumOp {
	template<typename EO, typename EA>
	static EO activate(EA a) {
#pragma HLS inline
		return  a / N;
	}
};

/*!
 * \brief  Element-level operation for quantized average pooling (activate = right shift).
 */
template<size_t SHIFT>
struct QuantAvgOp : SumOp {
	template<typename EO, typename EA>
	static EO activate(EA a) {
#pragma HLS inline
		return  EO(a >> SHIFT);
	}
};


//===========================================================================
// Pool functor: accumulator as instance state

/*!
 * \brief  Pool functor lifting an element-level Op to scalar types.
 *
 * Pure accumulator — no control state.  The caller (pool(), pool_global())
 * owns the first flag and passes it to operator().
 *
 * \tparam Op   Element-level operation (MaxOp, SumOp, AvgOp<N>, QuantAvgOp<SHIFT>)
 * \tparam TO   Output type
 * \tparam TA   Accumulator type (defaults to TO)
 */
template<typename Op, typename TO, typename TA = TO>
class PoolFunction {
	TA  accu;

public:
	using  output_t = TO;

	template<typename TI>
	void operator()(bool first, TI const &x) {
#pragma HLS inline
#pragma HLS reset variable=accu off
		accu = Op::template pool<TA>(first? Op::template init<TA>() : TA(accu), TA(x));
	}
	TO operator*() const {
#pragma HLS inline
		return  Op::template activate<TO, TA>(accu);
	}
};

/*!
 * \brief  Pool functor lifting an element-level Op to tensor types.
 */
template<typename Op, typename EO, typename EA, unsigned PE>
class PoolFunction<Op, tensor<EO, PE>, tensor<EA, PE>> {
	tensor<EA, PE>  accu;

public:
	using  output_t = tensor<EO, PE>;

	template<typename TI>
	void operator()(bool first, TI const &x) {
#pragma HLS inline
#pragma HLS reset variable=accu off
		for(unsigned i = 0; i < PE; i++) {
#pragma HLS unroll
			accu[i] = Op::template pool<EA>(first? Op::template init<EA>() : EA(accu[i]), EA(x[i]));
		}
	}
	output_t operator*() const {
#pragma HLS inline
		output_t  res;
		for(unsigned i = 0; i < PE; i++) {
#pragma HLS unroll
			res[i] = Op::template activate<EO, EA>(accu[i]);
		}
		return  res;
	}
};


//===========================================================================
// Concrete pool functors

template<typename T>
using MaxPool = PoolFunction<MaxOp, T, T>;

template<typename T>
using SumPool = PoolFunction<SumOp, T, T>;

template<typename TO, typename TA, size_t N>
using AvgPool = PoolFunction<AvgOp<N>, TO, TA>;

template<typename TO, typename TA, size_t SHIFT>
using QuantAvgPool = PoolFunction<QuantAvgOp<SHIFT>, TO, TA>;


//===========================================================================
// Operator Top-Level Function

/**
 * Generic pooling: reduce-until-last using flit_t markers from input_gen.
 * Owns a single functor instance and the first flag.
 *
 * @tparam F   Pool functor type (MaxPool<TO>, QuantAvgPool<TO,TA,SHIFT>, ...)
 * @param  src Input stream of flit_t<TI> (data + last marker)
 * @param  dst Output stream
 */
template<
	typename F,
	typename TI    // [inferred] Input data type
>
void pool(
	hls::stream<flit_t<TI>>            &src,
	hls::stream<typename F::output_t>  &dst
) {
#pragma HLS pipeline II=1 style=flp
	static F     pool;
	static bool  first = true;
#pragma HLS reset variable=first

	if(!src.empty()) {
		auto const  x = src.read();
		pool(first, x.data);
		if(x.last)  dst.write(*pool);
		first = x.last;
	}
}



//===========================================================================
// Global pooling: array of accumulators, round-robin over channel folds

/**
 * Global pooling: reduce PIXELS spatial positions per channel fold, emitting
 * one activated result per fold after the final pixel.  Customizable through
 * the same PoolFunction aliases (SumPool, MaxPool, QuantAvgPool, ...).
 *
 * Architectural difference to pool<F>(): maintains an array of CF functor
 * instances in LUTRAM, stepped through round-robin.  A ModCounter<PIXELS>
 * fires after all spatial positions have been absorbed, implying output
 * and the assertion of first — just as in pool().
 *
 * @tparam F       Pool functor type (SumPool<TO,TA>, MaxPool<TO>, ...)
 * @tparam CF      Channel folds = NumChannels / PE
 * @tparam PIXELS  Spatial positions to reduce (H * W)
 * @param  src     Input stream (plain tensor, no flit_t)
 * @param  dst     Output stream
 */
template<
	typename F,
	unsigned CF,
	unsigned PIXELS,
	typename TI    // [inferred] Input stream element type
>
void pool_global(
	hls::stream<TI>                    &src,
	hls::stream<typename F::output_t>  &dst
) {
#pragma HLS pipeline II=1 style=flp
	static F                   accumulators[CF];
	static unsigned            fold = 0;
	static bool                first = true;
	static ModCounter<PIXELS>  px;
#pragma HLS reset variable=fold
#pragma HLS reset variable=first
#pragma HLS reset variable=px

	if(!src.empty()) {
		auto const  x = src.read();

		accumulators[fold](first, x);
		if(px.last())  dst.write(*accumulators[fold]);

		if(fold++ == CF-1) {
			fold = 0;
			first = px.tick();
		}
	}
}

#endif
