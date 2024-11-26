/******************************************************************************
 *  Copyright (c) 2024, Advanced Micro Devices, Inc.
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

/*******************************************************************************
 *
 *  Authors: Michal Danilowicz <danilowi@agh.edu.pl>     
 *
 *  \file split_tb.cpp
 *
 *  Testbench for the channel split operation.
 *
 *******************************************************************************/

#include <iostream>

#include <hls_stream.h>
#include <hls_vector.h>

#include "data/split_config.h"
#include "split.hpp"


void Testbench_split(hls::stream<IN_TYPE> &in0_V, hls::stream<IN_TYPE> (&out_arr)[NUM_OUTPUTS]);

int main()
{
	hls::stream<IN_TYPE> in0_V ("in0_V");
	hls::stream<IN_TYPE> out_arr[NUM_OUTPUTS];
	hls::stream<IN_TYPE> expected[NUM_OUTPUTS];

	// prepare stimulus and expected output
	for (unsigned int counter = 0; counter < REP_COUNT; counter++){
		for(unsigned int output = 0; output < REP_COUNT; output++){
			for(unsigned int f = 0; f < FOLDS_PER_OUTPUT[output]; f++){
				IN_TYPE el = (EL_TYPE)(output + 1);
				in0_V.write(el);
				expected[output].write(el);
			}
		}
	}

	unsigned timeout = 0;
	unsigned out_cnt = 0;
	while(timeout < 100){

		Testbench_split(in0_V, out_arr);

		bool all_empty = true;
		for(unsigned int output = 0; output < NUM_OUTPUTS; output++){
			if(!out_arr[output].empty()){
				all_empty = false;
				break;
			}
		}
		if(all_empty){
			timeout++;
		}
		else{
			for(int i = 0; i < NUM_OUTPUTS; i++){
				if(!out_arr[i].empty()){
					IN_TYPE y = out_arr[i].read();
					IN_TYPE exp = expected[i].read();
					if(y != exp)
						std::cerr << "ERROR! Unexpected output nr " << out_cnt << ". Got: " << y << " Expected: " << exp << std::endl;
					out_cnt++;
				}
			}
		}
	}
}
