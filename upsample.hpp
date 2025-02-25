/******************************************************************************
 *  Copyright (c) 2024, Advanced Micro Devices, Inc.
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
/*******************************************************************************
 *
 *  Authors: Michal Danilowicz <danilowi@agh.edu.pl>
 *           Thomas B. Preusser <thomas.preusser@amd.com>       
 *
 *  \file upsample.hpp
 *
 *  This file defines template functions
 *	of a nearest neighbour upsampling operation.
 *
 *******************************************************************************/


#ifndef UPSAMPLE_HPP
#define UPSAMPLE_HPP

#include <ap_int.h>
#include <hls_stream.h>
#include <functional>

#include "utils.hpp"


/**
 * @brief  Two-dimensional nearest-neighbor upsampling.
 * @description
 *	This implementation leverages the Bresenham approximation approach,
 *	originally devised for approximated line drawing on pixelated output
 *	devices, for the selection of the desired closest input positions.
 *	This selection process is performed independently along each feature map
 *	dimension.
 *	In each dimension, output coordinates [0, O) are mapped back to input
 *	coordinates [0, I). With respect to Bresenham, the output assumes the role
 *	of the x- and the input the role of the y-axis in drawing a line of a slope
 *	smaller than 1. The mappings of the first and last pixels, i.e. (0, 0) and
 *	(O-1, I-1) are used as anchors and are assumed to have no associated
 *	approximation errors. For intermediate mappings (o, i), we seek the nearest
 *	input index as:
 *		i = round( (I-1)/(O-1) * o )
 *	We choose to use rounding up for ties and substitute:
 *		X := O-1
 *		Y := I-1
 *	to yield:
 *		i = round( Y/X * o )
 *
 *	For a sequential stepping through the output indices [0, O-1), the
 *	corresponding sequence of input indices can be tracked along just by
 *	maintaining an approximation error. Initially, e_0 = 0. An iterative step
 *	taking o to o+1 produces the tentative error ê_{j+1} = e_j + Y/X assuming
 *	no increment of the input index. If this tentative error remains below
 *	a half, it is carried over to the next step and the input index is
 *	not changed. Otherwise, the input index is incremented and the error is
 *	discounted accordingly. This yields an iteration step as:
 *
 *		ê_{j+1} = e_j + Y/X
 *		e_{j+1} = ê_{j+1} - (ê_{j+1} < 0.5? 0 : 1)
 *		i_{j+1} = i_j     + (ê_{j+1} < 0.5? 0 : 1)
 *
 *	The fractional representation of the error can be made integer by using:
 *
 *		E_j = 2X*e_j + 2Y - X
 *
 *	yielding:
 *
 *		E_0     = 2Y - X
 *		Ê_{j+1} = E_j     + 2Y
 *		E_{j+1} = Ê_{j+1} - (Ê_{j+1} < 2Y? 0 : 2X)
 *		i_{j+1} = i_j     + (Ê_{j+1} < 2Y? 0 :  1)
 *
 *	The detour through the tentative error can be removed from the computation:
 *
 *		E_0     = 2Y - X
 *		E_{j+1} = E_j + 2Y - (E_j < 0? 0 : 2X)
 *		i_{j+1} = i_j      + (E_j < 0? 0 :  1)
 *
 *	As all E_j are sums of Y and X and only their signedness is relevant for
 *	algorithmic decisions, Y and X can initially be reduced by their greatest
 *	common divisor. Finally, observing that the only possible odd contribution
 *	to E_j comes through the initialization of E_0, it can be truncated away:
 *
 *		E_0     = floor(Y - X/2)
 *		E_{j+1} = E_j + Y - (E_j < 0? 0 : X)
 *		i_{j+1} = i_j     + (E_j < 0? 0 : 1)
 */
template<
	unsigned  HI,	// Height of input feature map
	unsigned  WI,	// Width of input feature map
	unsigned  HO,	// Height of output feature map
	unsigned  WO,	// Width of output feature map
	unsigned  CF,	// Channel Fold
	typename  T
>
void upsample_nn(
	hls::stream<T> &src,
	hls::stream<T> &dst
) {
	static_assert(HI <= HO, "Output height cannot be smaller than input dimension.");
	static_assert(WI <= WO, "Output width cannot be smaller than input dimension.");
	static_assert(0 < HI, "Input height must be positive.");
	static_assert(0 < WI, "Input width must be positive.");

	//- Error Tracking along each Dimension ---------------------------------

	// Translate dimensions into endpoint distances and remove common factors
	constexpr unsigned  HX_ = HO-1;
	constexpr unsigned  HY_ = HI-1;
	constexpr unsigned  HD_ = gcd(HX_, HY_);
	constexpr unsigned  HX = HX_/HD_;
	constexpr unsigned  HY = HY_/HD_;

	constexpr unsigned  WX_ = WO-1;
	constexpr unsigned  WY_ = WI-1;
	constexpr unsigned  WD_ = gcd(WX_, WY_);
	constexpr unsigned  WX = WX_/WD_;
	constexpr unsigned  WY = WY_/WD_;

	// Error Initialization Values
	constexpr int  HE0 = HY - (HX+1)/2;
	constexpr int  WE0 = WY - (WX+1)/2;

	//- Ring Buffer for Upsampling Replay -----------------------------------
	ap_int<1+clog2(std::max(HX-HY, HY))>  he = HE0;	// range: Y-X <= e < Y
	T buf[WI*CF];
	bool  fill = true;
	for(unsigned ho = 0; ho < HO; ho++){

		if(fill) {
			for(unsigned  wp = 0; wp < WI*CF; wp++) {
				buf[wp] = src.read();
			}
		}

		unsigned  rp = 0;
		ap_int<1+clog2(std::max(WX-WY, WY))>  we = WE0;
		for(unsigned wo = 0; wo < WO; wo++){
			for(unsigned cf = 0; cf < CF; cf++){
				dst.write(buf[rp++]);
			}
			bool const repw = we < 0;
			we += repw? WY : WY-WX;
			if(repw) rp -= CF;
		}

		bool const reph = he < 0;
		he += reph? HY : HY-HX;
		fill = !reph;
	}

} // upsample_nn()

#endif