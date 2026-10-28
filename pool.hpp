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
// Stateful pool functor: accumulator as instance state

/*!
 * \brief  Pool functor lifting an element-level Op to scalar types.
 *
 * Owns its accumulator and first-element flag. Pool_batch drives this
 * via pool() and result(), holding a single static instance.
 *
 * \tparam Op   Element-level operation (MaxOp, SumOp, AvgOp<N>, QuantAvgOp<SHIFT>)
 * \tparam TO   Output type
 * \tparam TA   Accumulator type (defaults to TO)
 */
template<typename Op, typename TO, typename TA = TO>
class PoolFunction {
	TA    accu;
	bool  first = true;

public:
	using  output_t = TO;

	template<typename TI>
	void operator()(TI const &x, bool last) {
#pragma HLS inline
#pragma HLS reset variable=accu off
#pragma HLS reset variable=first
		accu = Op::template pool<TA>(first? Op::template init<TA>() : TA(accu), TA(x));
		first = last;
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
	bool            first = true;

public:
	using  output_t = tensor<EO, PE>;

	template<typename TI>
	void operator()(TI const &x, bool last) {
#pragma HLS inline
#pragma HLS array_partition variable=accu complete dim=0
#pragma HLS reset variable=accu off
#pragma HLS reset variable=first
		for(unsigned i = 0; i < PE; i++) {
#pragma HLS unroll
			accu[i] = Op::template pool<EA>(first? Op::template init<EA>() : EA(accu[i]), EA(x[i]));
		}
		first = last;
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

template<typename TO, typename TA = TO>
using MaxPool = PoolFunction<MaxOp, TO, TA>;

template<typename TO, typename TA = TO>
using SumPool = PoolFunction<SumOp, TO, TA>;

template<typename TO, typename TA, size_t N>
using AvgPool = PoolFunction<AvgOp<N>, TO, TA>;

template<typename TO, typename TA, size_t SHIFT>
using QuantAvgPool = PoolFunction<QuantAvgOp<SHIFT>, TO, TA>;


//===========================================================================
// Operator Top-Level Function

/**
 * Generic pooling: reduce-until-last using flit_t markers from input_gen.
 * The stateful functor F owns the accumulator. Pool_batch is a pure
 * scheduling wrapper with no state of its own.
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
	static F  pool;

	if(!src.empty()) {
		auto const  x = src.read();
		pool(x.data, x.last);
		if(x.last)  dst.write(*pool);
	}
}



//===========================================================================
// Legacy AccPool_Batch

/**
 * \brief   Accumulate-pool (function-pipelined) — global accumulation across
 *          ImgDim×ImgDim spatial positions, emitting one result per channel fold.
 *
 * \tparam ImgDim       Width and Height of the Input Feature Map (assumed square)
 * \tparam NumChannels  Number of Input Feature Maps
 * \tparam ActType      DataType of the input activation
 * \tparam PECount      PE parallelism
 * \tparam AccType      Datatype of the accumulation (e.g. output)
 *
 * \param in            Input stream
 * \param out           Output stream
 */
template<
    unsigned int ImgDim,
	unsigned int NumChannels,
	typename ActType,
	unsigned int PECount,
	typename AccType>
void AccPool_Batch(
	hls::stream<tensor<ActType, PECount>> &in,
	hls::stream<tensor<AccType, PECount>> &out
) {
#pragma HLS pipeline II=1 style=flp
	constexpr unsigned  FOLDS = NumChannels / PECount;
	constexpr unsigned  PIXELS = ImgDim * ImgDim;
	static tensor<AccType, PECount>  accumulators[FOLDS];
	static unsigned  fold = 0;
	static unsigned  pixel = 0;
#pragma HLS reset variable=fold
#pragma HLS reset variable=pixel
#pragma HLS reset variable=accumulators off
#pragma HLS bind_storage variable=accumulators type=RAM_2P impl=LUTRAM

	if(!in.empty()) {
		auto const  x = in.read();
		bool const  first_pixel = (pixel == 0);

		tensor<AccType, PECount>  acc;
		for(unsigned  pe = 0; pe < PECount; pe++) {
#pragma HLS UNROLL
			acc[pe] = AccType(x[pe]) + (first_pixel ? AccType(0) : AccType(accumulators[fold][pe]));
		}
		accumulators[fold] = acc;

		bool const  last_pixel = (pixel == PIXELS - 1);
		if(last_pixel)  out.write(acc);

		if(++fold == FOLDS) {
			fold = 0;
			if(++pixel == PIXELS)  pixel = 0;
		}
	}
}

#endif
