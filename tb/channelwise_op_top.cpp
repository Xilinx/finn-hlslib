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
 *  Authors: Tobias Alonso <tobiasa@xilinx.com>
 *
 *  \file channelwise_op_top.cpp
 *  
 *  HLS Top function with three ChannelWiseOperation activation layers for unit 
 *  testing
 *  
 *****************************************************************************/

#include "channelwise_op_top.h"

// Implements:
// in -> [width adapt] -> [bipolar mult] -> [add]  -> [mult] -> [width adapt] -> out
void Testbench_channelwise_op(stream<ap_uint<IFM_Channels*INPUT_BITS> > & in, 
                    stream<ap_uint<OFM_Channels*OUTPUT_BITS> > & out, unsigned int numReps){
    #pragma HLS DATAFLOW
    
    
    // [width adapt] 
    stream<ap_uint<PE*INPUT_BITS>>  wa_in;
    StreamingDataWidthConverter_Batch<IFM_Channels*INPUT_BITS, PE*INPUT_BITS, IFMDim*IFMDim>(
        in, wa_in, numReps);


    // [bipolar mult]
    ChannelWiseOperation<FOLD, PE,ap_uint<INPUT_BITS>, BIPO_PARAM_TYPE, BIPO_OUT_TYPE, 
            per_channel_neg<BIPO_OUT_TYPE> > bipolar_params= {.parameters = BIPOLAR_INIT};
    
    stream<ap_uint<PE*BIPO_OUT_BITS>>  bipolar_out;
    Thresholding_Batch< IFMDim, IFM_Channels, PE,
        Slice< ap_uint<INPUT_BITS> >, Slice<BIPO_OUT_TYPE> >
        (wa_in, bipolar_out, bipolar_params, numReps);


    // [add] 
    ChannelWiseOperation<FOLD, PE,BIPO_OUT_TYPE, ADD_PARAM_TYPE, ADD_OUT_TYPE, 
            std::plus<ADD_OUT_TYPE> > add_params = {.parameters = ADD_INIT};
    
    stream<ap_uint<PE*ADD_OUT_BITS>>  add_out;
    Thresholding_Batch< IFMDim, IFM_Channels, PE,
        Slice<BIPO_OUT_TYPE>, Slice<ADD_OUT_TYPE> >
        (bipolar_out, add_out, add_params, numReps);


    // [mult] 
    ChannelWiseOperation<FOLD, PE,ADD_OUT_TYPE, MULT_PARAM_TYPE, MULT_OUT_TYPE, 
            std::multiplies<MULT_OUT_TYPE> > mult_params= {.parameters = MULT_INIT};
    
    stream<ap_uint<PE*MULT_OUT_BITS>>  mul_out;
    Thresholding_Batch< IFMDim, IFM_Channels, PE,
        Slice<ADD_OUT_TYPE>, Slice<MULT_OUT_TYPE> >
        (add_out, mul_out, mult_params, numReps);

    // [width adapt]
    StreamingDataWidthConverter_Batch<PE*MULT_OUT_BITS, OFM_Channels*MULT_OUT_BITS, 
        IFMDim*IFMDim*FOLD >(mul_out, out, numReps);
}
