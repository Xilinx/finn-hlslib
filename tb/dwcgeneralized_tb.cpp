/******************************************************************************
 *  Copyright (c) 2024, Xilinx, Inc.
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
 *  Authors: Lukas Stasytis <lukas.stasytis@amd.com>
 *
 *  \file dwcgeneralized_tb.cpp
 *
 *  Testbench for the generalized data-width converter HLS block
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

#include "data/dwcgeneralized_config.h"

#include "activations.hpp"
#include "interpret.hpp"

using namespace hls;
using namespace std;

void Testbench_dwcgeneralized(stream<ap_uint<INPUT_WIDTH> > & in, stream<ap_uint<OUT_WIDTH> > & out, unsigned int numReps);

int main()
{
	stream<ap_uint<INPUT_WIDTH> > input_stream("input_stream");
	stream<ap_uint<OUT_WIDTH> > output_stream("output_stream");
	static ap_uint<OUT_WIDTH> expected[NumOutWords*NUM_REPEAT];
	static ap_uint<INPUT_WIDTH+OUT_WIDTH> expected_buffer[NumOutWords*NUM_REPEAT];
	// extremely over-provisioned vector for accumulating transaction words for InWidth > OutWidth case
	static ap_uint<(INPUT_WIDTH+OUT_WIDTH)*(NumInWords*NumOutWords)> read_buffer[NUM_REPEAT];
	unsigned int count_out = 0;
	unsigned int width_processed = 0;
	unsigned int repeats_processed = 0;
	unsigned int writes_to_process = 0;
	unsigned int in_reads = 0;
	unsigned int out_writes = 0;
	constexpr unsigned totalIters = (NumInWords > NumOutWords ? NumInWords : NumOutWords);
	for (unsigned int counter = 0; counter < totalIters*NUM_REPEAT; counter++) {
		ap_uint<INPUT_WIDTH> value = (ap_uint<INPUT_WIDTH>) 0;
		if (in_reads < NumInWords){
			value = (ap_uint<INPUT_WIDTH>) counter;
			input_stream.write(value);
			in_reads++;
		}
		if(INPUT_WIDTH < OUT_WIDTH){
			ap_uint<INPUT_WIDTH+OUT_WIDTH> val_out = expected_buffer[count_out];
			val_out(INPUT_WIDTH+width_processed-1,width_processed)=value;
			expected_buffer[count_out]=val_out;
			width_processed += INPUT_WIDTH;

			if (in_reads == NumInWords){ // force padding if necessary
				width_processed = OUT_WIDTH;
			}
			if (width_processed >= (OUT_WIDTH)){
				out_writes++;
				expected_buffer[count_out]=val_out;
				expected[count_out] = val_out(OUT_WIDTH-1,0);
				val_out = val_out >> OUT_WIDTH;
				count_out++;
				expected_buffer[count_out] = val_out;
				width_processed-= OUT_WIDTH;
			}
		}
		else if(INPUT_WIDTH == OUT_WIDTH)
		{
			expected[counter] = value;
			out_writes++;
		} else //INPUT_WIDTH > OUT_WIDTH
		{
			if (width_processed < INPUT_WIDTH*NumInWords){  // input becomes conditional
				read_buffer[repeats_processed](INPUT_WIDTH+width_processed-1,width_processed) = value;
				width_processed += INPUT_WIDTH;
			}
			ap_uint<OUT_WIDTH> val_out = read_buffer[repeats_processed](OUT_WIDTH+writes_to_process-1,writes_to_process);
			expected_buffer[count_out]=val_out;
			writes_to_process += OUT_WIDTH;

			if (out_writes < NumOutWords){
				expected[count_out]=val_out;
				out_writes++;
				count_out++;
				width_processed-= OUT_WIDTH;
			}
		}
		if (out_writes == NumOutWords && in_reads == NumInWords) {
			in_reads = 0;
			out_writes = 0;
			repeats_processed++;
			writes_to_process = 0;
			width_processed = 0;
		}
	}
	Testbench_dwcgeneralized(input_stream, output_stream, NUM_REPEAT);
	for (unsigned int counter=0 ; counter <  NumOutWords*NUM_REPEAT; counter++)
	{
		ap_uint<OUT_WIDTH> value = output_stream.read();
		if(value!= expected[counter])
		{
			cout << "ERROR with counter " << counter << std::hex << " expected " << expected[counter] << " value " << value << std::dec <<  endl;
			return(1);
		}
	}

}