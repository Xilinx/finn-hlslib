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
 *  \file channelwise_op_tb.cpp
 *
 *  Testbench for the ChannelWiseOperation activation
 *
 *****************************************************************************/

#include <iostream>
#include <cmath>
#include <cstring>
#include <hls_stream.h>
#include <cstdlib>
#define AP_INT_MAX_W 16384
#include "ap_int.h"
#include "weights.hpp"
#include "bnn-library.h"
#include "activations.hpp"
#include "interpret.hpp"
using namespace hls;
using namespace std;
  
#include "channelwise_op_top.h"  

#define MAX_IMAGES 2
#define ROUNDS 2 // ROUNDS >1 to get cosim to measure II

int main()
{   

    // input and output of verification function
    int IMAGE[ROUNDS][MAX_IMAGES][IFMDim][IFMDim][IFM_Channels];

    // input and output of DUT
    stream<ap_uint<IFM_Channels*INPUT_BITS> > input_stream[ROUNDS];
    stream<ap_uint<IFM_Channels*OUTPUT_BITS> > output_stream[ROUNDS];

    // generate image and load input stream 
    unsigned int counter = 0;
    for(unsigned int round_idx = 0; round_idx < ROUNDS; round_idx++) {
        for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++) {
            for (unsigned int oy = 0; oy < IFMDim; oy++) {
                for (unsigned int ox = 0; ox < IFMDim; ox++) {
                    IN_T<INPUT_BITS*IFM_Channels> input_channel = 0;
                    for(unsigned int channel = 0; channel < IFM_Channels; channel++)
                    {   
                        // generate new value casted to the input type
                        IN_T<INPUT_BITS> input = (IN_T<INPUT_BITS>)(counter);

                        // store value in image for verification function
                        IMAGE[round_idx][n_image][ox][oy][channel]= input;

                        // insert on most significant bits of pixel buffer
                        input_channel = input_channel >> INPUT_BITS;
                        input_channel(IFM_Channels*INPUT_BITS-1,(IFM_Channels-1)*INPUT_BITS)=input;

                        counter++;
                    }
                    input_stream[round_idx].write(input_channel);
                }
            }
        }
    }

    // call DUT
    for (int i = 0; i < ROUNDS; ++i)
    {
        Testbench_channelwise_op(input_stream[i], output_stream[i], MAX_IMAGES);
    }
    

    // verification function
    int err_counter = 0, err_perimage=0;
    OUT_T<OUTPUT_BITS> produced_out;
    for(unsigned int round_idx = 0; round_idx < ROUNDS; round_idx++) {
        for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++) {
            for (unsigned int oy = 0; oy < OFMDim; oy++) {
                for (unsigned int ox = 0; ox < OFMDim; ox++) {
                    OUT_T<OFM_Channels*OUTPUT_BITS> outElem = output_stream[round_idx].read();

                    for(unsigned int channel = 0; channel < OFM_Channels; channel++){
                        int f_idx = int(channel/PE);
                        int pe_idx = channel%PE;

                        int expected_out  = IMAGE[round_idx][n_image][ox][oy][channel];
                        expected_out  = bipolar_init[pe_idx][f_idx]? expected_out:-expected_out;
                        expected_out += add_init[pe_idx][f_idx];
                        expected_out *= mult_init[pe_idx][f_idx];

                        produced_out = outElem((channel + 1)*OUTPUT_BITS-1,channel*OUTPUT_BITS);

                        if (expected_out != produced_out){
                            std::cout << "ERROR: In round"<<round_idx<<", Expected["<<oy <<"]["<<ox<<"]["<<channel<<"]=" 
                            << expected_out << " actual " <<  produced_out <<
                            " | input value:"<<IMAGE[round_idx][n_image][ox][oy][channel] <<
                            " | Params - >  bip: "<<bipolar_init[pe_idx][f_idx]<< 
                            " | add: "<<add_init[pe_idx][f_idx]<< 
                            " | mul: "<<mult_init[pe_idx][f_idx]<< 
                            std::endl;
                            err_counter ++;
                            err_perimage++;
                        }
                    }
                    
                }
            }

            if(err_perimage == 0){
                std::cout<<"Round "<<round_idx << ", Image # " << n_image << " passed the testing."<< std::endl;
            }
            else{
                std::cout << "Round "<<round_idx << ", Image # " << n_image << " failed the testing."<<
                " Errors:"<<err_perimage<<"/"<<OFMDim*OFMDim*OFM_Channels << std::endl;
                err_perimage=0;
            }
        }
    }

    if(err_counter == 0){
        return 0;
    }
    else{
        return 1;
    }

}


