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
 *
 *  \file dwc_tb.cpp
 *
 *  Testbench for the data-width converter HLS block
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

#include "data/dwc_parallelwindow_config.h"

#include "activations.hpp"
#include "interpret.hpp"

using namespace hls;
using namespace std;

#define MAX_IMAGES 1
void Testbench_dwc_parallelwindow(stream<ap_uint<INPUT_WIDTH> > & in, stream<ap_uint<OUT_WIDTH> > & out, unsigned int numReps);

int main()
{
	stream<ap_uint<INPUT_WIDTH> > input_stream("input_stream");
	stream<ap_uint<OUT_WIDTH> > output_stream("output_stream");
	ap_uint<INPUT_WIDTH> golden_input [NUM_REPEAT];
	for (unsigned int counter = 0; counter < NUM_REPEAT*MAX_IMAGES; counter++) {
		// Populate input stream
		ap_uint<INPUT_WIDTH> value;
		for (unsigned int i = 0; i < CHANNELS1 * KERNEL1; i++){
			value(INPUT_WIDTH-1, INPUT_WIDTH-ACTWIDTH1) = (ap_uint<ACTWIDTH1>) (i + counter*CHANNELS1*KERNEL1);
			value = value >> ACTWIDTH1;
		}
		cout << "Input stream: ";
		for (unsigned int i = 0; i < CHANNELS1 * KERNEL1; i++){
			cout << value((i+1)*ACTWIDTH1-1, i*ACTWIDTH1) << ", ";
		}
		cout << endl;
		input_stream.write(value);
		golden_input[counter] = value;
	}
	Testbench_dwc_parallelwindow(input_stream, output_stream, MAX_IMAGES);
	for (unsigned int counter=0 ; counter < NUM_REPEAT*MAX_IMAGES; counter++)
	{
		for (unsigned int i = 0; i < NF1; i++){
			for (unsigned int j = 0; j < SF1; j++){
				cout << "Stream read" << endl;
				ap_uint<OUT_WIDTH> value = output_stream.read();
				for (unsigned int k = 0; k < SIMD1; k++){
					for (unsigned int l = 0; l < PE1; l++){
						unsigned int lower_bound = (i*PE1 + j*SIMD1*CHANNELS1 + k*CHANNELS1 + l)*ACTWIDTH1;
						unsigned int upper_bound = (i*PE1 + j*SIMD1*CHANNELS1 + k*CHANNELS1 + l + 1)*ACTWIDTH1;
						ap_uint<ACTWIDTH1> expected = golden_input[counter](upper_bound-1, lower_bound);
						ap_uint<ACTWIDTH1> computed = value(ACTWIDTH1-1, 0);
						value = value >> ACTWIDTH1;
						if (expected != computed){
							cout << "ERROR at {NF1, SF1, SIMD1, PE1} = " << "{" << i << ", " << j << ", " << k << ", " << l << "}, Expected/Computed = " << expected << ", " << computed << endl; 
						} else {
							cout << "CORRECT at {NF1, SF1, SIMD1, PE1} = " << "{" << i << ", " << j << ", " << k << ", " << l << "}, Expected/Computed = " << expected << ", " << computed << endl; 
						}
					}
				}
			}
		}	
	}
}


