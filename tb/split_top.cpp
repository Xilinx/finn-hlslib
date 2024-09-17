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
 *
 *  \file split_top.cpp
 *
 *  HLS Top function with channel split operation for unit testing
 *
 *******************************************************************************/

#include <hls_stream.h>
#include <hls_vector.h>

#include "data/split_config.h"
#include "split.hpp"

void Testbench_split(hls::stream<IN_TYPE> &in0_V, hls::stream<IN_TYPE> (&out_arr)[NUM_OUTPUTS])
{
#pragma HLS INTERFACE axis port=in0_V
#pragma HLS INTERFACE axis port=out_arr[0]
#pragma HLS INTERFACE axis port=out_arr[1]
#pragma HLS INTERFACE axis port=out_arr[2]
#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS aggregate variable=in0_V compact=bit
#pragma HLS aggregate variable=out_arr[0] compact=bit
#pragma HLS aggregate variable=out_arr[1] compact=bit
#pragma HLS aggregate variable=out_arr[2] compact=bit

StreamingSplit<NUM_FOLDS0, NUM_FOLDS1, NUM_FOLDS2>(in0_V, out_arr);
}
