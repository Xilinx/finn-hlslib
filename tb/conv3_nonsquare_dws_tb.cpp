/******************************************************************************
 *  Copyright (c) 2019, Xilinx, Inc.
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
 *  Authors: Giulio Gambardella <giuliog@xilinx.com>
 *
 *  \file conv3_tb.cpp
 *
 *  Testbench for the convolution HLS block
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
#include "data/memdata_nonsquare_dws.h"
#include "data/config_nonsquare_dws.h"
#include "activations.hpp"
#include "weights.hpp"
#include "activations.hpp"
#include "interpret.hpp"
#include "mvau.hpp"
#include "conv.hpp"
using namespace hls;
using namespace std;


#define MAX_IMAGES 1
void Testbench_conv_nonsquare_dws(stream<ap_uint<FM_Channels1*INPUT_PRECISION> > & in, stream<ap_uint<FM_Channels1*ACTIVATION_PRECISION> > & out, unsigned int numReps);

int main()
{
	static	ap_uint<INPUT_PRECISION> IMAGE[MAX_IMAGES][IFMDim1_x][IFMDim1_y][FM_Channels1];
	static	ap_int<ACTIVATION_PRECISION> TEST[MAX_IMAGES][OFMDim1_x][OFMDim1_y][FM_Channels1];
	stream<ap_uint<FM_Channels1*INPUT_PRECISION> > input_stream("input_stream");
	stream<ap_uint<FM_Channels1*ACTIVATION_PRECISION> > output_stream("output_stream");
	unsigned int counter = 0;
	for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++) {
		for (unsigned int oy = 0; oy < IFMDim1_y; oy++) {
			for (unsigned int ox = 0; ox < IFMDim1_x; ox++) {
				ap_uint<INPUT_PRECISION*FM_Channels1> input_channel = 0;
				for(unsigned int channel = 0; channel < FM_Channels1; channel++)
				{
					ap_uint<INPUT_PRECISION> input = (ap_uint<INPUT_PRECISION>)(counter);
					IMAGE[n_image][ox][oy][channel]= input;
					input_channel = input_channel >> INPUT_PRECISION;
					input_channel(FM_Channels1*INPUT_PRECISION-1,(FM_Channels1-1)*INPUT_PRECISION)=input;

					counter++;
				}
				input_stream.write(input_channel);
			}
		}
	}
	static	ap_int<WIDTH> W1[FM_Channels1][KERNEL_DIM_X][KERNEL_DIM_Y];
	// initialize the weights
	constexpr int TX = KERNEL_DIM_X*KERNEL_DIM_Y;
	constexpr int TY = FM_Channels1 / PE1;
	unsigned int kx=0;
	unsigned int ky=0;
	unsigned int out_chan_count=0;
	for (unsigned int oy = 0; oy < TY; oy++) {
		for (unsigned int ox = 0; ox <TX; ox++) {
			for(int pe=0;pe <PE1;pe++){
				W1[out_chan_count][kx][ky] = PARAM::weights.weights(kx*KERNEL_DIM_Y + ky)[out_chan_count][0];
				kx++;
				if (kx==KERNEL_DIM_X){
					kx=0;
					ky++;
					if (ky==KERNEL_DIM_Y){
						ky=0;
						out_chan_count++;
						if (out_chan_count==FM_Channels1){
							out_chan_count=0;
						}
					}
				}
			}
		}
	}
	dwsconv_nonsquare<MAX_IMAGES,IFMDim1_x,IFMDim1_y,OFMDim1_x,OFMDim1_y,FM_Channels1, KERNEL_DIM_X, KERNEL_DIM_Y, STRIDE_x, STRIDE_y, ap_uint<INPUT_PRECISION>, ap_int<ACTIVATION_PRECISION>, ap_int<WIDTH> >(IMAGE, W1, TEST);
	Testbench_conv_nonsquare_dws(input_stream, output_stream, MAX_IMAGES);
	int err_counter = 0, err_perimage=0;
	ap_int<ACTIVATION_PRECISION> out_chan;
	for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++) {
		for (unsigned int oy = 0; oy < OFMDim1_y; oy++) {
			for (unsigned int ox = 0; ox < OFMDim1_x; ox++) {
				for(int e=0;e<1;e++){
					ap_uint<FM_Channels1*ACTIVATION_PRECISION> outElem = output_stream.read();
					for(unsigned int channel = 0; channel < FM_Channels1; channel++){
						ap_int<ACTIVATION_PRECISION> EXP = TEST[n_image][ox][oy][channel + e * FM_Channels1];
						out_chan(ACTIVATION_PRECISION-1,0) = outElem((channel + 1)*ACTIVATION_PRECISION-1,channel*ACTIVATION_PRECISION);

						if (EXP != out_chan){
							std::cout << "ERROR: Expected["<<oy <<"]["<<ox<<"]["<<channel<<"]=" << EXP << " actual " <<  out_chan << std::endl;
							err_counter ++;
							err_perimage++;
						}
					}
				}
			}
		}
		if(err_perimage == 0){
			std::cout << "Image # " << n_image << " passed the testing."<< std::endl;
		}
		else{
			err_perimage=0;
			std::cout << "Image # " << n_image << " failed the testing."<< std::endl;
		}
	}
	if(err_counter == 0){
		return 0;
	}
	else{
		return 1;
	}

}


