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
 *  \file concat_top.cpp
 *
 *  HLS Top function with channel concatenation operation for unit testing
 *
 *******************************************************************************/

#include <hls_stream.h>
#include <hls_vector.h>

#include "data/concat_config.h"
#include "concat.hpp"


void Testbench_concat(hls::stream<IN_TYPE0> &in0_V,
                      hls::stream<IN_TYPE1> &in1_V,
                      hls::stream<IN_TYPE2> &in2_V,
                      hls::stream<IN_TYPE3> &in3_V,
                      hls::stream<OUT_TYPE> &out_V)
{
#pragma HLS INTERFACE axis port=in0_V
#pragma HLS INTERFACE axis port=in1_V
#pragma HLS INTERFACE axis port=in2_V
#pragma HLS INTERFACE axis port=in3_V
#pragma HLS INTERFACE axis port=out_V
#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS aggregate variable=in0_V compact=bit
#pragma HLS aggregate variable=in1_V compact=bit
#pragma HLS aggregate variable=in2_V compact=bit
#pragma HLS aggregate variable=in3_V compact=bit
#pragma HLS aggregate variable=out_V compact=bit

StreamingConcat<NUM_FOLDS0, NUM_FOLDS1, NUM_FOLDS2, NUM_FOLDS3>(out_V, in0_V, in1_V, in2_V, in3_V);
}
