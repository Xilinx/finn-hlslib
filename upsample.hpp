/******************************************************************************
 *  Copyright (c) 2019-2022, Xilinx, Inc.
 *  Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
 * @author	Giulio Gambardella <giuliog@xilinx.com>
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 * @brief	Nearest-neighbor upsampling for arbitrary dimensionality.
 ******************************************************************************/

#ifndef UPSAMPLE_HPP
#define UPSAMPLE_HPP

#include <type_traits>
#include <hls_task.h>
#include "utils.hpp"


// Namespace to encapsulate auxiliary data structures.
namespace upsample {

	//-----------------------------------------------------------------------
	// Portfolio of Possible Steppers in Recursive Dimension Unfolding

	// Common Interface Definition (never actually dispatched virtually)
	template<size_t  X>
	class Stepper {
	protected:
		ModCounter<X>  cnt;

	public:
		// Return: will replay
		virtual bool rep() = 0;
		// Return: dimension completed
		virtual bool step() {
#pragma HLS inline
			return  cnt.tick();
		}
	};

	// Unchanged Dimension: X == Y
	template<size_t  X>
	class StepperCopy : public Stepper<X> {
	public:
		bool rep() override {
#pragma HLS inline
			return  false;
		}
	};

	// Broadcast Dimension: 1 == Y < X
	template<size_t  X>
	class StepperBroadcast : public Stepper<X> {
	public:
		bool rep() override {
#pragma HLS inline
			return !this->cnt.last();
		}
	};

	// General Upsampled Dimension: 1 < Y < X
	// --------------------------------------
	// Upsampling is modelled after the Bresenham algorithm for drawing the
	// diagonal in a rectangle of width X and height Y onto a pixelated output
	// device. Placing the origin of cartesian coordinates into its lower left
	// corner, the perfect line runs from (0,0) to (X,Y). Pixels of width and
	// height 1 are drawn consecutively stepping along the x-axis, each
	// directly above the integer part of the ideal y-coordinate of its
	// horizontal midpoint. The Bresenham implementation tracks the fractional
	// part of the y-coordinate explicitly as an approximation error and
	// otherwise only identfies when to step its integer part up.
	// The initial fractional error at the first pixel's midpoint of x=1/2 is:
	//
	//	e_0 = 1/2 * Y/X
	//
	// Stepping from one pixel to its immediate successor with an unchanged
	// y-coordinate produces a tentative error of:
	//
	//	ê_{i+1} = e_i + Y/X
	//
	// If ê_{i+1} ≥ 1, the integer part of the y-coordinate is incremented.
	// The actual next error is corrected to be:
	//
	//	e_{i+1} = ê_{i+1} - (ê_{i+1} ≥ 1? 1 : 0)
	//
	// For an simpler compute, substitute E_0 = (X * e_0) - X + Y:
	//
	//	E_0     = (3Y - 2X)/2
	//	Ê_{i+1} = E_i + Y
	//	E_{i+1} = Ê_{i+1} - (Ê_{i+1} ≥ Y? X : 0)
	//
	// Substituting the tentative error directly into the last equation yields:
	//
	//	E_{i+1} = E_i + Y - (E_i ≥ 0? X : 0)
	//
	// Finally, note that the potential fractional part of 1/2 in E_0 can be
	// ignored and truncated as the algorithmic progression of the error does
	// not depend on it. Also, common factors between X and Y can be eliminated
	// as they produce the same sequence of decisions only with a scaled
	// representation of the error.
	template<size_t  Y, size_t  X>
	class StepperBresenham : public Stepper<X> {
		static_assert(Y < X, "Nothing to upsample.");

		// Eliminate common factors
		static constexpr size_t  GCD = gcd(Y, X);
		static constexpr size_t  YY = Y / GCD;
		static constexpr size_t  XX = X / GCD;

		// Approximation error in range [YY-XX, YY)
		using err_t = ap_int<1+clog2(std::max(XX-YY, YY))>;
		static constexpr int  E0 = int(3*YY - 2*XX) >> 1;

	private:
		err_t  err = E0;

	public:
		bool rep() override {
#pragma HLS inline
			return  err < 0;
		}

		bool step() override {
#pragma HLS inline
			err += err < 0? YY : YY-XX;
			return  Stepper<X>::step();
		}
	};

	//-----------------------------------------------------------------------
	// Result of a tick of the loop nest
	// propagated and manipuated from the innermost to the outermost loop.
	struct tickres_t {
		using  inc_t = typename std::make_signed<size_t>::type;

		bool   tick;	// inner loop nest completed
		inc_t  inc; 	// read pointer increment
		bool   free;	// have free pointer follow read pointer
	};

	// Abstract interface of nesting level (never actually called virtually)
	class NestBase {
	public:
		virtual tickres_t tick() = 0;
	};

	// Empty template prototype of nesting level
	template<size_t...  V>
	class Nest {};

	// Termination level of recursive unfolding of dimensions
	template<>
	class Nest<> : public NestBase {
	public:
		static constexpr size_t  volume   = 1;
		static constexpr size_t  buf_size = 1;
	public:
		tickres_t tick() override {
#pragma HLS inline
			return  { true, 1, true };
		}
	};

	// Recursive unfolding of one dimension level
	template<size_t  Y, size_t  X, size_t...  V>
	class Nest<Y, X, V...> : public NestBase {
		// Select the input index stepper for this level
		using  step_t =
			typename std::conditional<Y == X, StepperCopy<X>,
			typename std::conditional<Y == 1, StepperBroadcast<X>, StepperBresenham<Y, X>>::type>::type;
		// Unfolding of remaining enclosed loop nest
		using  inner_t = Nest<V...>;

	public:
		// Represented Data Volume
		static constexpr size_t  volume = Y * inner_t::volume;
		// Needed Buffer Size
		static constexpr size_t  buf_size = Y == X? inner_t::buf_size : volume;

	private:
		step_t  step;
		inner_t  inner;

	public:
		tickres_t tick() override {
#pragma HLS inline
			bool const  rep = step.rep();
			tickres_t   res = inner.tick();
			if(res.tick) {
				res.tick = step.step();
				if(rep)  res.inc = 1 - inner_t::volume;
			}
			if(rep)  res.free = false;
			return  res;
		}
	};

} // namespace upsample

//===========================================================================
// Designated External Function Interface (incl. buffer management)

/**
 * Nearest-neighbor upsampling of a hypercubical feature map of an arbitrary
 * dimensionality. From slowest- to fastest-changing dimension, each one is
 * specified as a pair comprising first its input and second its output size.
 * The output size of a dimension may not be smaller than its corresponding
 * input size.
 * The upsampling is generally performed using the Bresenham algorithm. The
 * extreme degenerated cases of upsampling a dimension by X:X or 1:X are
 * subjected to specialized optimizations.
 */
template<
	size_t... V,	// (Input Dim, Output Dim)*
	typename  T 	// [inferred] Stream Data Type
>
void upsample_nn(
	hls::stream<T> &src,
	hls::stream<T> &dst
) {
#pragma HLS pipeline II=1 style=frp
	constexpr size_t  WP_DELAY = 4;
	using  MyNest = upsample::Nest<V...>;
	constexpr size_t  ADDR_BITS = clog2(MyNest::buf_size);
	using  idx_t = ap_uint<ADDR_BITS>;
	using  ptr_t = ap_int<1 + ADDR_BITS>;

	static MyNest  nest;
	static T      buf[1 << ADDR_BITS];
	static ptr_t  wp[WP_DELAY] = { 0, };	// write pointer: delay for rp comparison
	static ptr_t  rp = 0;	// read pointer: bounded by wp
	static ptr_t  fp = 0;	// free pointer: bounds wp for next buffer generation
#pragma HLS reset variable=nest
#pragma HLS reset variable=buf off
#pragma HLS reset variable=wp
#pragma HLS reset variable=rp
#pragma HLS reset variable=fp
#pragma HLS dependence variable=buf inter false
#pragma HLS dependence variable=buf intra false
#pragma HLS array_partition variable=wp complete

	//- Output Buffer Register ----------------------------------------------
	static bool  ovld = false;
	static T     obuf;
#pragma HLS reset variable=ovld
#pragma HLS reset variable=obuf off

	// Update delay pipeline for wp
	for(unsigned  i = WP_DELAY-1; i > 0; i--)  wp[i] = wp[i-1];

	// Read into buffer memory if capacity is available
	if(/* wp <= fp' */ ptr_t(wp[0]-fp) >= 0) {
		T  x;
		if(src.read_nb(x))  buf[idx_t(wp[0]++)] = x;
	}

	// Try to clear output buffer
	if(ovld)  ovld = !dst.write_nb(obuf);

	// Try to refill output buffer
	if(!ovld) {
		obuf = buf[idx_t(rp)];

		if(/* rp < wp */ ptr_t(rp-wp[WP_DELAY-1]) < 0) {
			auto const  res = nest.tick();
			rp += res.inc;
			if(res.free)  fp = rp;

			ovld = true;
		}
	}
} // upsample_nn()

//===========================================================================
// DEPRECATED LEGACY INTERFACE

/**
 * \brief	Nearest-neigbor upsampling for square feature maps.
 *
 * \tparam 	OFMDim 		Size of the output feature map
 * \tparam 	IFMDim 		Size of the input feature map
 * \tparam 	NumChannels	Number of channels of the feature map
 * \tparam 	In_t		 Input datatype
 *
 * \param 	src	Input stream
 * \param 	dst	Output stream
 */
template<
	size_t  OFMDim,
	size_t  IFMDim,
	size_t  NumChannels,
	typename In_t
>
void UpsampleNearestNeighbour(
	hls::stream<ap_uint<NumChannels * In_t::width>> &src,
	hls::stream<ap_uint<NumChannels * In_t::width>> &dst
) {
	// Simulation loop, HW will loop infinitely
	for(size_t  i = 0; i < OFMDim*OFMDim + 5; i++) {
#pragma HLS dataflow
		upsample_nn<IFMDim, OFMDim, IFMDim, OFMDim>(src, dst);
	}

} // UpsampleNearestNeighbour()

/**
 * \brief	Nearest-neigbor upsampling for square feature maps and multiple inputs.
 *
 * \tparam 	OFMDim 		Size of the output feature map
 * \tparam 	IFMDim 		Size of the input feature map
 * \tparam 	NumChannels	Number of channels of the feature map
 * \tparam 	In_t		 Input datatype
 *
 * \param 	src	Input stream
 * \param 	dst	Output stream
 */
template<
	size_t  OFMDim,
	size_t  IFMDim,
	size_t  NumChannels,
	typename In_t
>
void UpsampleNearestNeighbour(
	hls::stream<ap_uint<NumChannels * In_t::width>> &src,
	hls::stream<ap_uint<NumChannels * In_t::width>> &dst,
	unsigned const  numReps
) {
	for(unsigned  rep = 0; rep < numReps; rep++) {
#pragma HLS dataflow
		UpsampleNearestNeighbour<OFMDim, IFMDim, NumChannels, In_t>(src, dst);
	}

} // UpsampleNearestNeighbour()

/**
 * \brief	Nearest-neigbor upsampling for a 1D vector.
 *
 * \tparam	OFMDim		Output vector length
 * \tparam	IFMDim		Input vector length
 * \tparam	NumChannels Channels per element
 * \tparam	In_t		Per-channel input type
 *
 * \param	src	Input stream
 * \param	dst	Output stream
 */
template<
	size_t  OFMDim,		// Output vector length - must be a whole multiple of IFMDim
	size_t  IFMDim,		// Input vector length
	size_t  NumChannels,	// Channels per element
	typename  In_t			// Per-channel input type
>
void UpsampleNearestNeighbour_1D(
	hls::stream<ap_uint<NumChannels * In_t::width>> &src,
	hls::stream<ap_uint<NumChannels * In_t::width>> &dst
) {
#pragma HLS dataflow disable_start_propagation
	// Simulation loop, HW will loop infinitely
	for(size_t  i = 0; i < OFMDim + 5; i++) {
#pragma HLS dataflow
		upsample_nn<IFMDim, OFMDim>(src, dst);
	}

} // UpsampleNearestNeighbour_1D()

#endif
