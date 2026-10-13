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
 *******************************************************************************/

/*******************************************************************************
 *
 *  Authors: Thomas B. Preusser <thomas.preusser@amd.com>
 *
 *  \file bipolar.hpp
 *
 *  Bipolar element type for XNOR-popcount networks.
 *
 *  Stored as a single bit (binary: 0 -> -1, 1 -> +1).
 *  Multiplication of two Bipolar values produces the XNOR result:
 *    (+1)*(+1) = +1  ->  1 XNOR 1 = 1
 *    (+1)*(-1) = -1  ->  1 XNOR 0 = 0
 *    (-1)*(+1) = -1  ->  0 XNOR 1 = 0
 *    (-1)*(-1) = +1  ->  0 XNOR 0 = 1
 *
 *  The product type is unsigned int (popcount-compatible), so accumulating
 *  Bipolar products gives the XNOR popcount directly.
 *
 *******************************************************************************/

#ifndef BIPOLAR_HPP
#define BIPOLAR_HPP

#include <ap_int.h>

class Bipolar {
	ap_uint<1>  v;

public:
	static const int  width = 1;
	Bipolar() : v(0) {}
	Bipolar(float f) : v(f != 0? 1 : 0) {}
	Bipolar(ap_uint<1> b) : v(b) {}
	Bipolar(int i) : v(i >= 0? 1 : 0) {}

	// XNOR multiply: returns 1 when bits match, 0 otherwise
	friend unsigned operator*(Bipolar const &a, Bipolar const &b) {
		return  a.v == b.v;
	}

	// Mixed multiply: Bipolar * T  ->  b.v? +x : -x  =  (2*b.v - 1) * x
	template<int W, bool S>
	friend ap_int<W+1> operator*(Bipolar const &b, ap_int_base<W, S> const &x) {
		return  b.v? ap_int<W+1>(x) : ap_int<W+1>(-x);
	}
	template<int W, bool S>
	friend ap_int<W+1> operator*(ap_int_base<W, S> const &x, Bipolar const &b) {
		return  b.v? ap_int<W+1>(x) : ap_int<W+1>(-x);
	}
};

#endif
