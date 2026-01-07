/******************************************************************************
 *  Copyright (c) 2023, Advanced Micro Devices, Inc.
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
#include <ap_int.h>
#include <hls_stream.h>

#include "bnn-library.h"
#include "data/config_fmpp.h"

#include <iostream>
#include <random>


void test_fm_pixel_padding(
	hls::stream<ap_uint<SIMD1*INPUT_WIDTH>> &src,
	hls::stream<ap_uint<SIMD1*INPUT_WIDTH>> &dst
);


int main() {
	std::cout << "Starting testbench for fm_pixel_padding" << std::endl;

	using T = ap_uint<SIMD1*INPUT_WIDTH>;
	hls::stream<T> input_stream("input_stream");
	hls::stream<T> output_stream("output_stream");
	T  expected[OUTPUT_DIM_Y][OUTPUT_DIM_X][CHANNELS];

	{ // Feed random input sequence
		std::minstd_rand  rd;
		std::uniform_int_distribution<int> dist(0, (1<<(SIMD1*INPUT_WIDTH))-1);

		unsigned  input_counter = 0;
		for(unsigned  y = 0; y < OUTPUT_DIM_Y; y++) {
			for(unsigned  x = 0; x < OUTPUT_DIM_X; x++) {
				for(unsigned  c = 0; c < CHANNELS; c++) {
					T  val = 0;
					if(((y % YSTRIDE) == 0) && ((x % XSTRIDE) == 0)) {
						val = dist(rd);
						input_stream.write(val);
						input_counter++;
					}
					expected[y][x][c] = val;
				}
			}
		}
		if(input_counter != (INPUT_DIM_X * INPUT_DIM_Y * CHANNELS)) {
			std::cout << "Input stream not fully populated." << std::endl;
			return 1;
		}
	}
	std::cout << "Finished writing to input stream" << std::endl;

	// Run top-level function
	test_fm_pixel_padding(input_stream, output_stream);
	std::cout << "Finished writing to output stream" << std::endl;

	// Verify correctness
	int  ret = 0;
	for(unsigned  y = 0; y < OUTPUT_DIM_Y; y++) {
		for(unsigned  x = 0; x < OUTPUT_DIM_X; x++) {
			for(unsigned  c = 0; c < CHANNELS; c++) {
				if(output_stream.empty()) {
					std::cerr << "Missing outputs." << std::endl;
					goto  err;
				}

				T const  val = output_stream.read();
				if(expected[y][x][c] != val) {
					std::cerr
						<< "Output mismatch [" << y << ':' << x << ':' << c << "]: "
						<< val << " instead of " << expected[y][x][c]
						<< std::endl;
					ret = 1;
				}
			}
		}
	}
	if(!output_stream.empty()) {
		std::cerr << "Output stream not empty." << std::endl;
err:
		ret = 1;
	}
	if(ret == 0)  std::cout << "Successfully passed csim testbench." << std::endl;
	return  ret;
}
