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
 *					 Jonas Kuehle <jonas.kuehle@cs.hs-fulda.de>
 *
 *  \file maxpool_tb.cpp
 *
 *  Testbench for the maxpool layer HLS block
 *
 *****************************************************************************/
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include <hls_stream.h>
#include <cstdlib>
#define AP_INT_MAX_W 8191
#include "ap_int.h"
#include "weights.hpp"
#include "bnn-library.h"

#include "data/pool_config.h"
#include "pool_tb.hpp"
#include "activations.hpp"
#include "interpret.hpp"

#define MAX_IMAGES 1
void Testbench_pool(hls::stream<hls::vector<T, FM_Channels1> > & in, hls::stream<hls::vector<T, FM_Channels1> > & out, unsigned int numReps);

int main()
{
	static	T IMAGE[MAX_IMAGES][IFMDim1][IFMDim1][FM_Channels1];
	static	T OUTPUT[MAX_IMAGES][OFMDim1][OFMDim1][FM_Channels1];
	hls::stream<hls::vector<T, FM_Channels1> > input_stream("input_stream");
	hls::stream<hls::vector<T, FM_Channels1> > output_stream("output_stream");
	unsigned int counter = 0;
	for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++) {
		for (unsigned int oy = 0; oy < IFMDim1; oy++) {
			for (unsigned int ox = 0; ox < IFMDim1; ox++) {
				hls::vector<T, FM_Channels1> input_channel;
				for(unsigned int channel = 0; channel < FM_Channels1; channel++)
				{
					T input = (T)(counter);
					IMAGE[n_image][oy][ox][channel]= input;
					input_channel[channel] = input;
					counter++;
				}
				input_stream.write(input_channel);
			}
		}
	}
	pool<MAX_IMAGES,IFMDim1,OFMDim1,FM_Channels1,KERNEL_DIM,KERNEL_DIM,T >(IMAGE,OUTPUT);
	Testbench_pool(input_stream, output_stream, MAX_IMAGES);
	int err_counter = 0, err_perimage=0;
	T out_chan;
	for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++) {
		for (unsigned int oy = 0; oy < OFMDim1; oy++) {
			for (unsigned int ox = 0; ox < OFMDim1; ox++) {
				hls::vector<T, FM_Channels1> outElem = output_stream.read();
				for(unsigned int channel = 0; channel < FM_Channels1; channel++){
					T EXP = OUTPUT[n_image][ox][oy][channel];
					out_chan = outElem[channel];
					if (EXP != out_chan){
						std::cout << "ERROR: Expected["<<oy <<"]["<<ox<<"]["<<channel<<"]=" << EXP << " actual " <<	out_chan << std::endl;
						err_counter ++;
						err_perimage++;
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
