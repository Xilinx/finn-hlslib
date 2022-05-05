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
 *  Authors: Mirza Mrahorovic <mirzam@xilinx.com>
 *
 *  \file swg_1D_dws_tb.cpp
 *
 *  Testbench for the sliding window generator HLS block for 1D convolutions
 *
 *****************************************************************************/
#define AP_INT_MAX_W 4096
#include <hls_stream.h>
#include "ap_int.h"
#include <iostream>
#include <string>
#include "bnn-library.h"

#include "data/input_gen_1d_dws.h"

#include "math.h"
using namespace hls;
using namespace std;

#define MAX_IMAGES 1

void Testbench(stream<ap_uint<SIMD1*INPUT_PRECISION1> > & in, stream<ap_uint<SIMD1*INPUT_PRECISION1> > & out);

int main()
{
	stream<ap_uint<IFM_Channels1*INPUT_PRECISION1> > input_stream("input_stream");
	stream<ap_uint<IFM_Channels1*INPUT_PRECISION1> > output_stream("output_stream");
	stream<ap_uint<SIMD1*INPUT_PRECISION1> > in_simd("in_simd");
	stream<ap_uint<SIMD1*INPUT_PRECISION1> > out_simd("out_simd");

	static	ap_int<INPUT_PRECISION1> IMAGE[MAX_IMAGES][IFMDim_x][IFM_Channels1];
	int counter = 0;
	ap_uint<SIMD1*INPUT_PRECISION1> input_channel = 0;
	for(unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++) {
		for(unsigned int x = 0; x < IFMDim_x; x++) {
            for(unsigned int c = 0; c < IFM_Channels1/SIMD1; c++) {
                input_channel = 0;
                for(unsigned int s = 0; s < SIMD1; s++){
                    ap_int<INPUT_PRECISION1> input = (ap_int<INPUT_PRECISION1>)(counter);
                    IMAGE[n_image][x][c*SIMD1+s]= input;
                    input_channel = input_channel >> INPUT_PRECISION1;
                    input_channel(SIMD1*INPUT_PRECISION1-1,(SIMD1-1)*INPUT_PRECISION1)=input;
                    counter++;
                }
                in_simd.write(input_channel);
            }
		}
	}
	Testbench(in_simd, out_simd);

	ap_int<INPUT_PRECISION1> out_chan;
	int expected_value;
	for(unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++) {
		for(unsigned int ox = 0; ox < OFMDim_x; ox++) {
            for(unsigned int chan = 0; chan < IFM_Channels1/SIMD1; chan++) {
                for(unsigned int kx = 0; kx < KERNEL_DIM_x; kx++) {
                    ap_uint<SIMD1*INPUT_PRECISION1> outElem = out_simd.read();
                    for(unsigned int s = 0; s < SIMD1; s++){
                        out_chan(INPUT_PRECISION1-1,0) = outElem((s + 1)*INPUT_PRECISION1-1,s*INPUT_PRECISION1);
                        expected_value = (ap_int<INPUT_PRECISION1>) IMAGE[n_image][ox*STRIDE_x+kx*DILATION_x][SIMD1*chan+s];
                        if (out_chan != expected_value){
                            std::cout << "ERROR: Expected " << expected_value << " actual " <<  out_chan << std::endl;
                            std::cout << "Position: OFMDim_x " << ox <<  " KERNEL_DIM_x " <<  kx << " IFM_Channels1/SIMD1 " <<  chan << " SIMD " << s << std::endl;
                            return 1;
                            }
                        }
                }
            }
        }
	}

	return 0;
}
