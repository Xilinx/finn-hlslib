/******************************************************************************
 * Copyright (c) 2019, Xilinx, Inc.
 * Copyright (c) 2025, AMD, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************
 * @author	Giulio Gambardella <giuliog@xilinx.com>
 * @author	Tobias Alonso <tobiasa@xilinx.com>
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ******************************************************************************/

#ifndef MAXPOOL_HPP
#define MAXPOOL_HPP

#include <cstddef>
#include <algorithm>
#include <limits>

#include "tensor.hpp"

//===========================================================================

template<typename  T, size_t...>
class MaxPool {};

template<typename  T>
class MaxPool<T> {
protected:
	static T init() {
#pragma HLS inline
		return  std::numeric_limits<T>::min();
	}
	static T max(T const &a, T const &b) {
#pragma HLS inline
		return  std::max(a, b);
	}
};

template<typename  E, unsigned  N>
class MaxPool<tensor<E, N>> {
	using  T = tensor<E, N>;

protected:
	// Broadcast initialization
	static T init() {
#pragma HLS inline
		T  y;
		for(auto e : y)  e = std::numeric_limits<E>::min();
		return  y;
	}

	// Elementwise maximum rather than lexicographic input selection
	static T max(T const &a, T const &b) {
#pragma HLS inline
		T  y;
		for(unsigned  i = 0; i < N; i++) {
#pragma HLS unroll
			y[i] = std::max(a[i], b[i]);
		}
		return  y;
	}
};

template<
	typename  T,
	size_t    K,
	size_t    CF
>
class MaxPool<T, K, CF> : private MaxPool<T> {

	//-----------------------------------------------------------------------
	// Public HW Interface
public:
	static
	void process(hls::stream<T> &src, hls::stream<T> &dst) {
#pragma HLS pipeline II=1 style=frp
		static ModCounter<K>   cnt;
		static ModCounter<CF>  cf;
		static T     accu[CF];
		static bool  empty = true;
#pragma HLS reset variable=cnt
#pragma HLS reset variable=cf
#pragma HLS reset variable=accu off
#pragma HLS reset variable=empty

		if(!src.empty()) {
			T const  prev = accu[CF-1];
			for(size_t  i = CF-1; i > 0; i--) {
#pragma HLS unroll
				accu[i] = accu[i-1];
			}
			accu[0] = MaxPool::max(empty? MaxPool::init() : prev, src.read());
			if(cnt.last())  dst.write(accu[0]);
			if(cf.tick()) {
				empty = cnt.tick();
			}
		}
	}

}; // MaxPool<T, K, CF>

#endif
