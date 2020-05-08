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
#include "ap_int.h"
#include "bnn-library.h"

#include "activations.hpp"
#include "weights.hpp"
#include "activations.hpp"
#include "interpret.hpp"
#include "dma.h"
#include "mvau.hpp"
#include "conv.hpp"
#include "memdata.h"
#include "config.h"

void Testbench_conv(stream<ap_uint<IFM_Channels1*INPUT_PRECISION> > & in, stream<ap_uint<OFM_Channels1*ACTIVATION_PRECISION> > & out, unsigned int numReps){
#pragma HLS DATAFLOW

    unsigned const MatrixW = KERNEL_DIM * KERNEL_DIM * IFM_Channels1;
    unsigned const MatrixH = OFM_Channels1;
    unsigned const InpPerImage = IFMDim1*IFMDim1;

    hls::stream<ap_uint<SIMD1*INPUT_PRECISION> > convInp;
    hls::stream<ap_uint<SIMD1*PE1*WIDTH> > paramStreamOut;

    GenParamStream<TILE1, SIMD1, PE1, WIDTH>(PARAM::weights, paramStreamOut, numReps * OFMDim1 * OFMDim1);

    WidthAdjustedInputStream <IFM_Channels1*INPUT_PRECISION, SIMD1*INPUT_PRECISION, InpPerImage>  wa_in (in,  numReps);
    WidthAdjustedOutputStream <PE1*ACTIVATION_PRECISION, OFM_Channels1*ACTIVATION_PRECISION, OFMDim1 * OFMDim1 * (OFM_Channels1 / PE1)>  mvOut (out,  numReps);

    ConvolutionInputGenerator<KERNEL_DIM, IFM_Channels1, INPUT_PRECISION, IFMDim1, OFMDim1, SIMD1, 1>(wa_in, convInp, numReps, ap_resource_dflt());

    Matrix_Vector_Activate_Stream_Batch<MatrixW, MatrixH, SIMD1, PE1, Slice<ap_uint<INPUT_PRECISION> >, Slice<ap_int<ACTIVATION_PRECISION> >, Identity, ap_uint<WIDTH> >
    (    static_cast<hls::stream<ap_uint<SIMD1*INPUT_PRECISION>>&>(convInp),
         static_cast<hls::stream<ap_uint<PE1*ACTIVATION_PRECISION>>&>  (mvOut),
         paramStreamOut, PassThroughActivation<ap_uint<16>>(), numReps* OFMDim1 * OFMDim1, ap_resource_dsp()    );

}
