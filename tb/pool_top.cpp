/******************************************************************************
 *  Copyright (c) 2019, Xilinx, Inc.
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
 *
 *  Authors: Giulio Gambardella <giuliog@xilinx.com>
 *           Felix Jentzsch <felixj@xilinx.com>
 *           Jonas Kuehle <jonas.kuehle@cs.hs-fulda.de>
 *
 *  \file pool_top.cpp
 *
 *  HLS Top functions for HLS square/1d max pool block unit testing
 *
 *****************************************************************************/
#include <hls_stream.h>
#include "ap_int.h"
#include "bnn-library.h"


#include "data/pool_config.h"

void Testbench_pool_binary(hls::stream<hls::vector<T, FM_Channels1> > & in, hls::stream<hls::vector<T, FM_Channels1> > & out, unsigned int numReps){
#pragma HLS DATAFLOW
#pragma HLS interface AXIS port=in
#pragma HLS interface AXIS port=out
#pragma HLS aggregate variable=in compact=bit
#pragma HLS aggregate variable=out compact=bit
#pragma HLS dataflow disable_start_propagation
  StreamingMaxPool_Batch<IFMDim1, KERNEL_DIM, FM_Channels1, T, 0>(in, out, numReps);
}

void Testbench_pool(hls::stream<hls::vector<T, FM_Channels1> > & in, hls::stream<hls::vector<T, FM_Channels1> > & out, unsigned int numReps){
#pragma HLS DATAFLOW
#pragma HLS interface AXIS port=in
#pragma HLS interface AXIS port=out
#pragma HLS aggregate variable=in compact=bit
#pragma HLS aggregate variable=out compact=bit
#pragma HLS dataflow disable_start_propagation
	StreamingMaxPool_Batch<IFMDim1, KERNEL_DIM, FM_Channels1, T, 0>(in, out, numReps);
}

void Testbench_pool_1d(hls::stream<hls::vector<T, PE1> > & in, hls::stream<hls::vector<T, PE1> > & out){
#pragma HLS DATAFLOW
#pragma HLS interface AXIS port=in
#pragma HLS interface AXIS port=out
#pragma HLS aggregate variable=in compact=bit
#pragma HLS aggregate variable=out compact=bit
#pragma HLS dataflow disable_start_propagation
	StreamingMaxPool_1d<IFMDim1, KERNEL_DIM, FM_Channels1, PE1, OFMDim1, T, 0>(in, out);
}
