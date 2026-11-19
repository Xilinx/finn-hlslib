/******************************************************************************
 * Copyright (c) 2019, Xilinx, Inc.
 * Copyright (c) 2025, AMD, Inc.
 * All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  3. Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
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
 ****************************************************************************
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 *****************************************************************************/

#include "pool_top.hpp"
#include "utils.hpp"
#include "input_gen.hpp"
#include "pool.hpp"

// Sliding-window max pooling as composed by FINN's Pool_hls:
// input_gen<> streams each window (flit_t marks its last element) into pool<F>.
void pool_top(
	hls::stream<vec_t> &src,
	hls::stream<vec_t> &dst
) {
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS dataflow disable_start_propagation
	static hls::stream<flit_t<vec_t>>  feed("feed");
#pragma HLS stream depth=2 variable=feed
	// loop nest: ho, wo, cf, kh, kw
	input_gen<2, H*W*CF, HO, S*W*CF, WO, S*CF, CF, 1, K, W*CF, K, CF>(src, feed);
	pool<MaxPool<vec_t>>(feed, dst);
}
