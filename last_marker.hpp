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
 ******************************************************************************/

/******************************************************************************
 * @brief	Function-pipelined TLastMarker kernels for AXI stream TLAST handling.
 * @author	Thomas B. Preusser <thomas.preusser@amd.com>
 ******************************************************************************/

#ifndef LAST_MARKER_HPP
#define LAST_MARKER_HPP

#include <ap_axi_sdata.h>
#include <hls_stream.h>

#include "utils.hpp"


//---------------------------------------------------------------------------
// Protocol adapter — abstracts qdma_axis vs. ap_axiu field access
//---------------------------------------------------------------------------
template<typename T> struct axi_adapter;

template<unsigned W>
struct axi_adapter<qdma_axis<W,0,0,0>> {
	using data_t = ap_uint<W>;
	static data_t get_data(qdma_axis<W,0,0,0> const &x) { return x.get_data(); }
	static qdma_axis<W,0,0,0> make(data_t const &d, bool last) {
		qdma_axis<W,0,0,0> t;
		t.set_data(d); t.set_keep(-1); t.set_last(last);
		return  t;
	}
};

template<unsigned W>
struct axi_adapter<ap_axiu<W,0,0,0>> {
	using data_t = ap_uint<W>;
	static data_t get_data(ap_axiu<W,0,0,0> const &x) { return x.data; }
	static ap_axiu<W,0,0,0> make(data_t const &d, bool last) {
		ap_axiu<W,0,0,0> t;
		t.data = d; t.keep = -1; t.last = last;
		return  t;
	}
};


//---------------------------------------------------------------------------
// TLAST Removal (Direction="in")
//
// Strips AXI sideband metadata, passes raw data through.
//---------------------------------------------------------------------------
template<
	typename  TAxi
>
void TLastMarker_In(
	hls::stream<TAxi>                               &in,
	hls::stream<typename axi_adapter<TAxi>::data_t> &out
) {
#pragma HLS pipeline II=1 style=flp
	if(!in.empty())  out.write(axi_adapter<TAxi>::get_data(in.read()));
}


//---------------------------------------------------------------------------
// TLAST Insertion (Direction="out")
//
// Uses ModCounter<NumIters> from utils.hpp: counts N-2 down to -1,
// sign bit drives TLAST, wraps back to N-2. Specialization for N=1
// is statically always true.
//---------------------------------------------------------------------------
template<
	unsigned  NumIters,
	typename  TAxi
>
void TLastMarker_Out(
	hls::stream<typename axi_adapter<TAxi>::data_t> &in,
	hls::stream<TAxi>                               &out
) {
#pragma HLS pipeline II=1 style=flp
	static ModCounter<NumIters>  cnt;
#pragma HLS reset variable=cnt
	if(!in.empty())  out.write(axi_adapter<TAxi>::make(in.read(), cnt.tick()));
}

#endif
