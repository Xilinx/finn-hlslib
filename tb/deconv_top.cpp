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

#include "../bnn-library.h"
#include "../weights.hpp"
// #include "activations.hpp"
// #include "interpret.hpp"
// #include "mvau.hpp"
// #include "conv.hpp"
#include "data/memdata_deconv2d.h"
#include "data/config_deconv2d.h"

void test_deconv2d(
	stream<ap_uint<DeconvIFMCh*IPrecision> > &src,
	stream<ap_uint<DeconvOFMCh*OPrecision> > &dst
){
#pragma HLS DATAFLOW

	ap_uint<DeconvIFMCh*IPrecision>  val = 0;
	val = src.read();

	// stream<ap_uint<IFM_Channels1*INPUT_PRECISION> > conv_input("input_stream");
	// FMPadding_Pixel_Nonsquare<
	// 	OUTPUT_DIM_X, // dimension expected by conv
	// 	OUTPUT_DIM_Y, // dimension expected by conv
	// 	XSTRIDE, // stride along pixel padding
	// 	YSTRIDE, // stride along pixel padding
	// 	CHANNELS, // num channels expected by conv (input channels)
	// 	SIMD1, // packing along the channel dim
	// 	ap_uint<INPUT_WIDTH> // data type of values
	// >(src, conv_input);

	// // TODO - replace weights and pass-through activation
	// // TODO - figure out why pass-through activation is 16 bits
	// // TODO - figure out why we are using DSP
	// ConvLayer_Batch<
	// 	KERNEL_DIM,
	// 	IFM_Channels1,
	// 	IFMDim1,
	// 	OFM_Channels1,
	// 	OFMDim1,
	// 	SIMD1,
	// 	PE1,
	// 	Slice<ap_uint<INPUT_PRECISION> >,
	// 	Slice<ap_uint<ACTIVATION_PRECISION> >,
	// 	Identity
	// >(conv_inputs, dst, PARAM::weights, PassThroughActivation<ap_uint<16>>(), 1, ap_resource_dsp());
}
