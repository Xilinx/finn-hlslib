/******************************************************************************
 * Copyright (c) 2019, Xilinx, Inc.
 * Copyright (c) 2025, AMD, Inc.
 * All rights reserved.
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
 ****************************************************************************
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ******************************************************************************/
#ifndef POOL_TOP_HPP
#define POOL_TOP_HPP

#include <hls_stream.h>
#include <hls_vector.h>
#include <ap_int.h>


constexpr unsigned  H = 13; // Input height
constexpr unsigned  W = 13; // Input width (must currently equal H for input generator)
constexpr unsigned  C = 16; // Channels
constexpr unsigned  K = 3;  // Dimension of square kernel
constexpr unsigned  S = 2;  // Stride

constexpr unsigned  PE = 4; // Parallelism

static_assert((H-K)%S == 0, "Input height requires padding.");
static_assert((W-K)%S == 0, "Input width requires padding.");
static_assert(C%PE == 0, "PE must divide channel count.");

using  val_t = ap_uint<8>;
using  pix_t = hls::vector<val_t, C>;
using  vec_t = hls::vector<val_t, PE>;

void pool_top(
	hls::stream<pix_t> &src,
	hls::stream<pix_t> &dst
);

#endif
