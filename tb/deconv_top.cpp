/******************************************************************************
 *  Copyright (c) 2023, Advanced Micro Devices, Inc.
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
#include <hls_stream.h>
using namespace hls;
#include "ap_int.h"

#include "bnn-library.h"
#include "weights.hpp"
#include "activations.hpp"
#include "interpret.hpp"
#include "mvau.hpp"
#include "data/memdata_deconv2d.h"
#include "data/config_deconv2d.h"

constexpr unsigned  numReps = 1;

void test_deconv2d(
	stream<ap_uint<IFMCh1*IPrecision> > & src,
	stream<ap_uint<OFMCh1*OPrecision> > & dst
){
#pragma HLS DATAFLOW
	stream<ap_uint<IFMCh1*IPrecision> > conv_input("input_stream");
	FMPadding_Pixel<
		FMPadODim1, // dimension expected by direct conv
		FMPadStride1, // stride along pixel padding
		IFMCh1, // num channels expected by conv (input channels)
		IFMCh1, // packing along the channel dim
		ap_uint<IPrecision> // data type of values
	>(src, conv_input);
	// Note - would need to insert padding layer if padding is not 0, which is the
	// case in this testbench top-level function
	ConvLayer_Batch<
		ConvKernel1,
		ConvIFMCh1,
		ConvIFMDim1,
		ConvOFMCh1,
		ConvOFMDim1,
		ConvSIMD1,
		ConvPE1,
		Slice<ap_uint<IPrecision> >,
		Slice<ap_uint<OPrecision> >,
		Identity
	>(conv_input, dst, PARAM::weights, PassThroughActivation<ap_uint<16> >(), numReps, ap_resource_dsp());
}
