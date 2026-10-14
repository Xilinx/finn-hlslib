/******************************************************************************
 *  Copyright (c) 2022, Xilinx, Inc.
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
 ******************************************************************************/

/******************************************************************************
 *	@brief
 *		Templatized streaming elementwise operator, consuming two streams and
 *		producing a third. The operator and datatypes can be customized.
 *	@author Yaman Umuroglu <yamanu@amd.com>
 ******************************************************************************/

#ifndef ELTWISE_HPP
#define ELTWISE_HPP

#include <hls_stream.h>
#include "tensor.hpp"


/**
 * \brief StreamingEltwise function
 *
 * \tparam Channels   Number of channels for eltwise operation
 * \tparam PE         Number of channels computed in parallel
 * \tparam N          Total number of elements to process
 * \tparam TE0        Element type of input 0
 * \tparam TE1        Element type of input 1
 * \tparam TEO        Element type of output
 * \tparam Fxn        Function class (e.g. Max, Avg, Sum) - safely deducible
 *
 * \param in0         Input stream 0
 * \param in1         Input stream 1
 * \param out         Output stream
 * \param f           Function to apply elementwise
 */
template<
	unsigned Channels, unsigned PE, unsigned N,
	typename TE0, typename TE1, typename TEO,
	typename Fxn
>
void StreamingEltwise(
	hls::stream<tensor<TE0, PE>> &in0,
	hls::stream<tensor<TE1, PE>> &in1,
	hls::stream<tensor<TEO, PE>> &out,
	Fxn &&f
) {
	constexpr unsigned  TOTAL_FOLD = (Channels / PE) * N;
	for(unsigned  i = 0; i < TOTAL_FOLD; i++) {
#pragma HLS pipeline style=flp II=1
		auto const  a = in0.read();
		auto const  b = in1.read();
		tensor<TEO, PE>  y;
		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
			y[pe] = f(a[pe], b[pe]);
		}
		out.write(y);
	}
}

#endif
