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
 *  Authors: Giulio Gambardella <tobiasa@xilinx.com>
 *           Tobias Alonso <tobiasa@xilinx.com>
 *
 *  \file kernel_stride_maxpool_top.cpp
 *
 *  HLS Top function with a single MaxPool layer with kernel_size%stride !=0 
 *  for unit testing
 *
 *****************************************************************************/

#include "kernel_stride_maxpool_top.h"

void Testbench_kernel_stride_pool(stream<ap_uint<FM_Channels1*INPUT_PRECISION> > & in, 
                stream<ap_uint<FM_Channels1*INPUT_PRECISION> > & out, unsigned int numReps){

#pragma HLS DATAFLOW
    const int FOLD = FM_Channels1/PE1;

    hls::stream<ap_uint<FM_Channels1*INPUT_PRECISION> > padded_input("padded_input");
    hls::stream<ap_uint<PE1*INPUT_PRECISION> > aw_padded_input("aw_padded_input");
    hls::stream<ap_uint<PE1*INPUT_PRECISION> > swg_out("swg_out");
    hls::stream<ap_uint<PE1*INPUT_PRECISION> > pool_out("pool_out");


    FMPadding_Batch<IFMDim1,PoolInDim1,PADDING,FM_Channels1,FM_Channels1,ap_uint<INPUT_PRECISION> >
        (in,padded_input, numReps) ;

    StreamingDataWidthConverter_Batch<FM_Channels1*INPUT_PRECISION, 
                                        PE1*INPUT_PRECISION, (PoolInDim1)*(PoolInDim1)>
            (padded_input, aw_padded_input, numReps);
    
    ConvolutionInputGenerator_kernel_stride_dws<KERNEL_DIM, FM_Channels1,
                            INPUT_PRECISION, PoolInDim1, OFMDim1, PE1,2>
            (aw_padded_input, swg_out, numReps, ap_resource_dflt());

    MaxPoolFunction<ap_uint<INPUT_PRECISION>,KERNEL_DIM> maxpool_fxn;
    Pool_batch<FM_Channels1, PE1, KERNEL_DIM,
                Slice<ap_uint<INPUT_PRECISION> >, Slice< ap_uint<INPUT_PRECISION> > >
    (swg_out,pool_out, maxpool_fxn, OFMDim1*OFMDim1*numReps);

    StreamingDataWidthConverter_Batch<PE1*INPUT_PRECISION, 
                                    FM_Channels1*INPUT_PRECISION, OFMDim1*OFMDim1*FOLD>
            (pool_out, out, numReps);

}
