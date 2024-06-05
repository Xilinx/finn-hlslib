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
 *           Felix Jentzsch <felixj@xilinx.com>
 *           Jonas Kuehle <jonas.kuehle@cs.hs-fulda.de>
 *
 *  \file maxpool_1d_tb.cpp
 *
 *  Testbench for the 1d maxpool layer HLS block
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
void Testbench_pool_1d(hls::stream<hls::vector<T, PE1> > & in, hls::stream<hls::vector<T, PE1> > & out);

int main()
{
	static	T IMAGE[MAX_IMAGES][IFMDim1][FM_Channels1];
	static	T OUTPUT[MAX_IMAGES][OFMDim1][FM_Channels1];
	hls::stream<hls::vector<T, PE1> > input_stream("input_stream");
	hls::stream<hls::vector<T, PE1> > output_stream("output_stream");
	unsigned int counter = 0;
	for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++) {
		for (unsigned int ox = 0; ox < IFMDim1; ox++) {
			hls::vector<T, PE1> input_channel;
			for(unsigned int channel = 0; channel < FM_Channels1/PE1; channel++){
				for (unsigned int p = 0; p < PE1; p++){
					T input = (T)(counter);
					IMAGE[n_image][ox][channel*PE1+p]= input;
					input_channel[p] = input;
					counter++;
				}
				input_stream.write(input_channel);
			}
		}
	}
	pool_1d<MAX_IMAGES,IFMDim1,OFMDim1,FM_Channels1,KERNEL_DIM,KERNEL_DIM,T >(IMAGE,OUTPUT);
	Testbench_pool_1d(input_stream, output_stream);
	int err_counter = 0, err_perimage=0;
	T out_chan;
	for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++) {
		for (unsigned int ox = 0; ox < OFMDim1; ox++) {
			for (unsigned int channel = 0; channel < FM_Channels1/PE1; channel++){
				hls::vector<T, PE1> outElem = output_stream.read();
				for (unsigned int p = 0; p < PE1; p++){
					T EXP = OUTPUT[n_image][ox][channel*PE1+p];
					out_chan = outElem[p];
					if (EXP != out_chan){
						std::cout << "ERROR: Expected["<<ox<<"]["<<channel*PE1+p<<"]=" << EXP << " actual " <<  out_chan << std::endl;
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
