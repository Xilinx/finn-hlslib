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
 *  \file concat_tb.cpp
 *
 *  Testbench for the channel concatenation operation.
 *
 *******************************************************************************/

#include <iostream>

#include <hls_stream.h>
#include <hls_vector.h>

#include "data/concat_config.h"
#include "concat.hpp"


void Testbench_concat(hls::stream<IN_TYPE0> &in0_V, hls::stream<IN_TYPE1> &in1_V, hls::stream<IN_TYPE2> &in2_V, hls::stream<IN_TYPE3> &in3_V, hls::stream<OUT_TYPE> &out_V);


int main()
{
	hls::stream<IN_TYPE0> in0_V ("in0_V");
	hls::stream<IN_TYPE1> in1_V ("in1_V");
	hls::stream<IN_TYPE2> in2_V ("in2_V");
	hls::stream<IN_TYPE3> in3_V ("in3_V");
	hls::stream<OUT_TYPE> out_V ("out_V");
	hls::stream<OUT_TYPE> expected ("expected");

	for (unsigned int counter = 0; counter < REP_COUNT; counter++){
		for(unsigned int f = 0; f < NUM_FOLDS0; f++){
			IN_TYPE0 el0 = (EL_TYPE0)1;
			in0_V.write(el0);
			expected.write(el0);
		}
		for(unsigned int f = 0; f < NUM_FOLDS1; f++){
			IN_TYPE1 el1 = (EL_TYPE1)2;
			in1_V.write(el1);
			expected.write(el1);
		}
		for(unsigned int f = 0; f < NUM_FOLDS2; f++){
			IN_TYPE2 el2 = (EL_TYPE2)3;
			in2_V.write(el2);
			expected.write(el2);
		}
		for(unsigned int f = 0; f < NUM_FOLDS3; f++){
			IN_TYPE3 el3 = (EL_TYPE3)4;
			in3_V.write(el3);
			expected.write(el3);
		}
	}

	unsigned timeout = 0;
	unsigned out_cnt = 0;
	while(timeout < 100){

		Testbench_concat(in0_V, in1_V, in2_V, in3_V, out_V);

		if(out_V.empty()){
			timeout++;
		}
		else{
			OUT_TYPE y = out_V.read();
			OUT_TYPE exp = expected.read();
			if(y != exp)
				std::cerr << "ERROR! Unexpected output nr " << out_cnt << ". Got: " << y << " Expected: " << exp << std::endl;
			out_cnt++;
		}
	}
}
