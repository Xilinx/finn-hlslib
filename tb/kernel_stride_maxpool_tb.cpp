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
 *           Tobias Alonso <tobiasa@xilinx.com>
 *
 *  \file kernel_stride_maxpool_tb.cpp
 *
 *  Testbench for the MaxPool layer with kernel_size%stride !=0
 *
 *****************************************************************************/

#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include <hls_stream.h>
#include <cstdlib>
#define AP_INT_MAX_W 16384
#include "ap_int.h"
#include "weights.hpp"
#include "bnn-library.h"

#include "kernel_stride_maxpool_top.h"
#include "pool_tb.hpp"

using namespace hls;
using namespace std;

#define ROUNDS 2 // at least 2 for cosim to measure II
#define MAX_IMAGES 1
int main()
{   
    unsigned int value = 0;
    for (unsigned int rnd_idx = 0; rnd_idx < ROUNDS; rnd_idx++){
        static ap_uint<INPUT_PRECISION> IMAGE[MAX_IMAGES][IFMDim1*IFMDim1][FM_Channels1];
        static ap_uint<INPUT_PRECISION> IMAGE_PADDED[MAX_IMAGES][PoolInDim1][PoolInDim1][FM_Channels1];
        ap_uint<INPUT_PRECISION> TEST[MAX_IMAGES][OFMDim1][OFMDim1][FM_Channels1];
        stream<ap_uint<FM_Channels1*INPUT_PRECISION> > input_stream("input_stream");
        stream<ap_uint<FM_Channels1*INPUT_PRECISION> > output_stream("output_stream");
        
        for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++)
            for (unsigned int oy = 0; oy < PoolInDim1; oy++)
                for (unsigned int ox = 0; ox < PoolInDim1; ox++)
                    for(unsigned int channel = 0; channel < FM_Channels1; channel++)
                        IMAGE_PADDED[n_image][oy][ox][channel]=0;



        for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++) {
            for (unsigned int oy = 0; oy < IFMDim1; oy++) {
                for (unsigned int ox = 0; ox < IFMDim1; ox++) {
                    ap_uint<INPUT_PRECISION*FM_Channels1> input_channel = 0;
                    for(unsigned int channel = 0; channel < FM_Channels1; channel++)
                    {
                        ap_uint<INPUT_PRECISION> input = (ap_uint<INPUT_PRECISION>)(value);
                        IMAGE[n_image][oy*IFMDim1+ox][channel]= input;
                        IMAGE_PADDED[n_image][oy+1][ox+1][channel]=input;
                        input_channel = input_channel >> INPUT_PRECISION;
                        input_channel(FM_Channels1*INPUT_PRECISION-1,(FM_Channels1-1)*INPUT_PRECISION)=input;
                        value++;
                    }
                    input_stream.write(input_channel);

                }
            }
        }

        // Maxpool function
        pool<MAX_IMAGES,PoolInDim1,OFMDim1,FM_Channels1,KERNEL_DIM,STRIDE,ap_uint<INPUT_PRECISION>>(IMAGE_PADDED,TEST);

        Testbench_kernel_stride_pool(input_stream, output_stream, MAX_IMAGES);

        ap_uint<INPUT_PRECISION> out_chan;
        int output_value;

        for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++) {
            for (unsigned int oy = 0; oy < OFMDim1; oy++) {
                for (unsigned int ox = 0; ox < OFMDim1; ox++) {
                    for(int e=0;e<1;e++){
                        ap_uint<FM_Channels1*INPUT_PRECISION> outElem = output_stream.read();
                        for(unsigned int channel = 0; channel < FM_Channels1; channel++){
                            ap_uint<INPUT_PRECISION> EXP = TEST[n_image][ox][oy][channel + e * FM_Channels1];
                            out_chan = outElem((channel + 1)*INPUT_PRECISION-1,channel*INPUT_PRECISION);
                            if (EXP != out_chan){
                                std::cout << "ERROR:Round("<<rnd_idx<<") Expected["<<oy <<"]["<<ox
                                    <<"]["<<channel<<"]=" << EXP << " actual " <<  out_chan << std::endl;

                                for (int ky=0;ky<KERNEL_DIM;ky++){
    								for (int kx=0;kx<KERNEL_DIM;kx++)
    									std::cout <<IMAGE_PADDED[n_image][(oy*STRIDE+ky)][ox*STRIDE+kx][channel]<<", ";
    								std::cout << std::endl;
                                }

                                std::cout << std::endl;
                                return 1;
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}
