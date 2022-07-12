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
 *******************************************************************************
 * @brief	Top-level for MaxNorm layer test.
 * @author	Thomas B. Preusser <thomas.preusser@amd.com>
 *******************************************************************************/
#include "normalize.hpp"
#include "max_norm_top.hpp"


void max_norm_top(
	hls::stream<ap_uint<WI>>  &src,
	hls::stream<ap_uint<WO>> (&dst)[2]
) {
#pragma HLS interface AXIS port=src
#pragma HLS interface AXIS port=dst[0]
#pragma HLS interface AXIS port=dst[1]
#pragma HLS dataflow disable_start_propagation
	hls::stream<ap_uint<WI>>  split[2];
	for(unsigned  i = 0; i < FM_SIZE; i++) {
		auto const  x = src.read();
		split[0].write(x);
		split[1].write(x);
	}
	max_norm<FM_SIZE, NORMAX0>(split[0], dst[0]);
	max_norm<FM_SIZE, NORMAX1>(split[1], dst[1]);
}
