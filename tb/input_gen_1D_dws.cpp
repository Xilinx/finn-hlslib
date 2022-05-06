/******************************************************************************
 *  Copyright (c) 2021, Xilinx, Inc.
 *  Copyright (c) 2022, Advanced Micro Devices, Inc.
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
 *           Felix Jentzsch <felix.jentzsch@upb.de>
 *
 *  \file input_gen_1D.cpp
 *
 *  HLS Top function with a single HLS sliding-window generator block unit testing (for 1D convolution)
 *
 *****************************************************************************/
#define AP_INT_MAX_W 4096

#include <hls_stream.h>
using namespace hls;
#include "ap_int.h"
#include "bnn-library.h"
#include "data/input_gen_1d_dws.h"

void Testbench(stream<ap_uint<SIMD1*INPUT_PRECISION1> > & in, stream<ap_uint<SIMD1*INPUT_PRECISION1> > & out)//, unsigned int numReps)
{
#pragma HLS DATAFLOW

	ConvolutionInputGenerator_1D_dws<KERNEL_DIM_x,
	IFM_Channels1,
	INPUT_PRECISION1,
	IFMDim_x,
	OFMDim_x,
	SIMD1>(in, out, 1, ap_resource_dflt());
}
