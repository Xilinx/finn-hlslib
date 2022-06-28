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

#include <hls_stream.h>
using namespace hls;
#include "ap_int.h"
#include "bnn-library.h"

#include "activations.hpp"
#include "weights.hpp"
#include "interpret.hpp"


#define PE 4
#define IFM_Channels 8
#define OFM_Channels IFM_Channels

#define IFMDim 3
#define OFMDim IFMDim

#define BIPO_PARAM_TYPE ap_uint<1>
#define ADD_PARAM_TYPE ap_int<3> 
#define MULT_PARAM_TYPE ap_int<3>
#define BIPOLAR_INIT {{1,1},{1,0},{0,1},{1,1}}
#define ADD_INIT {{ 2, 1},{ 0,-1},{-1,-3},{ 1, 1}} 
#define MULT_INIT {{ 3, 1}, { 2,-1}, {-1, 1}, { 1,-2}} 

#define INPUT_BITS 4
#define BIPO_OUT_BITS  (INPUT_BITS+1)
#define ADD_OUT_BITS  (BIPO_OUT_BITS+1)
#define MULT_OUT_BITS  (ADD_OUT_BITS+2)
#define OUTPUT_BITS MULT_OUT_BITS

#define IN_T ap_uint
#define BIPO_OUT_TYPE  ap_int<BIPO_OUT_BITS>
#define ADD_OUT_TYPE  ap_int<ADD_OUT_BITS>
#define MULT_OUT_TYPE  ap_int<MULT_OUT_BITS>
#define OUT_T ap_int

#define FOLD (OFM_Channels/PE)




const int bipolar_init[PE][FOLD] = BIPOLAR_INIT;
const int add_init[PE][FOLD] = ADD_INIT;
const int mult_init[PE][FOLD] = MULT_INIT;

template<typename T>
struct per_channel_neg
{
    constexpr T operator()(const ap_uint<1> &lhs, const T &rhs) const {
        return lhs? static_cast<decltype(-rhs)>(rhs):-rhs;
    }
    
};


void Testbench_channelwise_op(stream<ap_uint<IFM_Channels*INPUT_BITS> > & in, 
                    stream<ap_uint<OFM_Channels*OUTPUT_BITS> > & out, unsigned int numReps);
