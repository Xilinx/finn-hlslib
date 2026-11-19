/****************************************************************************
 * Copyright Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: MIT
 *
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#ifndef PAD_HPP
#define PAD_HPP

#include <hls_stream.h>
#include <ap_int.h>

#include "utils.hpp"

namespace pad_detail {

	/**
	 * One axis of the padded frame: LEAD leading positions, N data positions
	 * spaced by S (S-1 interior padding positions between neighbours) and
	 * TRAIL trailing positions.
	 *
	 * The phase counter only advances between the first and the last data
	 * position, i.e., (N-1)*S times per pass. Being a multiple of S, it
	 * returns to its initial last phase without ever being restarted.
	 */
	template<unsigned  N, unsigned  LEAD, unsigned  TRAIL, unsigned  S>
	class Axis {
		static_assert(N >= 1 && S >= 1, "Axis needs at least one data position and spacing of one.");
		static constexpr unsigned  SPAN = (N-1)*S + 1;
		static constexpr unsigned  LEN  = LEAD + SPAN + TRAIL;

		ModCounter<LEN>     pos;
		ModCounter<S, S-1>  ph;	// on its last phase at every data position

	public:
		/** Does the current position hold data (instead of padding)? */
		bool data() const {
#pragma HLS inline
			return  pos.between(LEAD, LEAD+SPAN) && ph.last();
		}
		/** Is the current position the last one of the axis? */
		bool last() const {
#pragma HLS inline
			return  pos.last();
		}
		/** Advance to the next position, wrapping around after the last. */
		void next() {
#pragma HLS inline
			if(pos.between(LEAD, LEAD+SPAN-1))  ph.tick();
			pos.tick();
		}
	};
}

template<
	unsigned  H,	// height of input feature map
	unsigned  W,	// width of input feature map
	unsigned  CF,	// channel fold (temporal, rather than spatial, factor of channel count)
	unsigned  PAD_N,	// padding north
	unsigned  PAD_E = PAD_N,	// padding east
	unsigned  PAD_S = PAD_N,	// padding south
	unsigned  PAD_W = PAD_E,	// padding west
	unsigned  SY = 1,	// spacing of image rows (interior padding of SY-1 rows)
	unsigned  SX = 1,	// spacing of image columns (interior padding of SX-1 columns)
	bool      EAGER = true,	// emit leading padding without waiting for input
	typename  T
>
void pad(
	hls::stream<T> &src,
	hls::stream<T> &dst,
	T const &pad_val = T{0,}
) {
#pragma HLS function_instantiate variable=pad_val
#pragma HLS pipeline II=1 style=flp
	static pad_detail::Axis<H, PAD_N, PAD_S, SY>  ya;
	static pad_detail::Axis<W, PAD_W, PAD_E, SX>  xa;
	static ModCounter<CF>  cf;
	static bool  lead = true;	// leading padding of the frame pending, only used if !EAGER
#pragma HLS reset variable=ya
#pragma HLS reset variable=xa
#pragma HLS reset variable=cf
#pragma HLS reset variable=lead

	bool  wr = true;
	T  val = pad_val;
	if(!EAGER && lead) {
		// Leading padding waits for the first input of the frame.
		if(src.empty())  wr = false;
		else             lead = false;
	}
	if(wr && ya.data() && xa.data()) {
		wr = src.read_nb(val);
	}
	if(wr) {
		dst.write(val);
		if(cf.tick()) {
			if(!xa.last())  xa.next();
			else {
				xa.next();
				if(ya.last() && !EAGER)  lead = true;
				ya.next();
			}
		}
	}

} // pad()

#endif
