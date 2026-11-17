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

	template<unsigned  S>
	class Phase {
		ap_uint<clog2(S)>  p = 0;
	public:
		bool hit() const {
#pragma HLS inline
			return  p == 0;
		}
		void restart() {
#pragma HLS inline
			p = 0;
		}
		void step() {
#pragma HLS inline
			if(p == S-1)  p = 0;
			else          p++;
		}
	};
	template<>
	class Phase<1> {
	public:
		bool hit() const {
#pragma HLS inline
			return  true;
		}
		void restart() const {
#pragma HLS inline
		}
		void step() const {
#pragma HLS inline
		}
	};

	/**
	 * One axis of the padded frame: LEAD leading positions, N data positions
	 * spaced by S (S-1 interior padding positions between neighbours) and
	 * TRAIL trailing positions.
	 */
	template<unsigned  N, unsigned  LEAD, unsigned  TRAIL, unsigned  S>
	class Axis {
		static_assert(N >= 1 && S >= 1, "Axis needs at least one data position and spacing of one.");
		static constexpr unsigned  SPAN = (N-1)*S + 1;
		static constexpr unsigned  LEN  = LEAD + SPAN + TRAIL;

		ap_uint<clog2((LEN < 2)? 2u : LEN)>  pos = 0;
		Phase<S>  ph;

	public:
		/** Does the current position hold data (instead of padding)? */
		bool data() const {
#pragma HLS inline
			return  (LEAD <= pos) && (pos < LEAD+SPAN) && ph.hit();
		}
		/** Is the current position the last one of the axis? */
		bool last() const {
#pragma HLS inline
			return  pos == LEN-1;
		}
		/** Advance to the next position, wrapping around after the last. */
		void next() {
#pragma HLS inline
			if(last()) {
				pos = 0;
				ph.restart();
			}
			else {
				if(pos < LEAD)  ph.restart();
				else            ph.step();
				pos++;
			}
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
