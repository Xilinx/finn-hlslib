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
 *
 *  \file conv_top.cpp
 *
 *  HLS Top function with a single convolutional layer for unit testing
 *
 *****************************************************************************/
#include <hls_stream.h>
using namespace hls;
#define AP_INT_MAX_W 4096
#include "ap_int.h"
#include "bnn-library.h"

#include "activations.hpp"
#include "weights.hpp"
#include "activations.hpp"
#include "interpret.hpp"
#include "mvau.hpp"
#include "conv.hpp"
#include "memdata-conv-dws.h"
#include "config-conv-dws.h"

void Testbench_conv_dws(stream<ap_uint<FM_Channels1*INPUT_PRECISION> > & in, stream<ap_uint<FM_Channels1*ACTIVATION_PRECISION> > & out, unsigned int numReps){
#pragma HLS DATAFLOW
	hls::stream<ap_uint<FM_Channels1*ap_uint<INPUT_PRECISION>::width> > resized_stream("resized_stream");
	hls::stream<ap_uint<PE1*ap_uint<INPUT_PRECISION>::width> > resized_stream_pe("resized_stream_pe");
	hls::stream<ap_uint<PE1*ap_uint<INPUT_PRECISION>::width> > swg_out("swg_out");
	SameResize_Batch<IFMDim1, KERNEL_DIM, STRIDE, FM_Channels1, ap_uint<INPUT_PRECISION> >(in, resized_stream, numReps);
	StreamingDataWidthConverter_Batch<FM_Channels1*INPUT_PRECISION, PE1*INPUT_PRECISION, (IFMDim1+2)*(IFMDim1+2)>(resized_stream, resized_stream_pe, numReps);
	ConvolutionInputGenerator_dws<KERNEL_DIM, FM_Channels1, ap_uint<INPUT_PRECISION>::width, IFMDim1+2, OFMDim1, PE1,1>(resized_stream_pe, swg_out, numReps, ap_resource_dflt());
	Vector_Vector_Activate_Batch<FM_Channels1, KERNEL_DIM, PE1, PE1, MMV1, Slice<ap_uint<INPUT_PRECISION> >, Slice<ap_int<16> >, Identity>(swg_out, out, PARAM::weights, PassThroughActivation<ap_int<16>>(), numReps*OFMDim1*OFMDim1, ap_resource_dsp());

}
