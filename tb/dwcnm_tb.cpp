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
 *           Mario Ruiz         <mruiznog@xilinx.com>
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
#define AP_INT_MAX_W 16384
#include "ap_int.h"
#include "bnn-library.h"

#include "dwcnm_config.h"

#include "activations.hpp"
#include "interpret.hpp"

using namespace hls;
using namespace std;


void Testbench_dwcnm(stream<ap_uint<INPUT_WIDTH> > & in, stream<ap_uint<OUT_WIDTH> > & out);

int main() {
	stream<ap_uint<INPUT_WIDTH> > input_stream("input_stream");
	stream<ap_uint<OUT_WIDTH> > output_stream("output_stream");
	ap_uint<OUT_WIDTH> expected[IMAGE_SIZE/OUT_WIDTH];
	unsigned int count_out  =0;
	unsigned int count_in   =0;
	unsigned int count_word =0;
	unsigned int offset     =0;
	ap_uint<OUT_WIDTH>   remaining = 0;
	
	if (INPUT_WIDTH > OUT_WIDTH) {
		for (unsigned int counter = 0; counter < IMAGE_SIZE/INPUT_WIDTH; counter++) {
			ap_uint<INPUT_WIDTH> value = 0;
			ap_uint<OUT_WIDTH>   aux = 0;
			if (offset !=0) {
				value(offset-1,0) = remaining(OUT_WIDTH-1,OUT_WIDTH-offset);
			}
			for (; offset <= (INPUT_WIDTH-OUT_WIDTH); offset+=OUT_WIDTH){
				aux = (ap_uint<OUT_WIDTH>) count_word++;
				value(offset+OUT_WIDTH-1,offset) = aux;
			}
			if (offset !=INPUT_WIDTH){
				aux = (ap_uint<OUT_WIDTH>) count_word++;
				value(INPUT_WIDTH-1,offset) = aux(INPUT_WIDTH-offset-1,0);
				remaining                   = aux;
				offset = offset + OUT_WIDTH - INPUT_WIDTH;
			}
			else
				offset = 0;
			input_stream.write(value);
		}
	}
	
	while (!input_stream.empty()) {
		Testbench_dwcnm(input_stream, output_stream);
	}
	
	for (unsigned int counter=0 ; counter <  IMAGE_SIZE/OUT_WIDTH; counter++){
		ap_uint<OUT_WIDTH> value = output_stream.read();
		if(value!= counter) {
			cout << "ERROR with counter " << std::dec << counter << std::dec << " expected " << counter << " value " << value << std::dec <<  endl;
			return(1);
		}
	}
	std::cout<< "Test passed successfully " << std::endl;
	return 0;

}


