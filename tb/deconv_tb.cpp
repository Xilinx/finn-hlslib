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
 ******************************************************************************/
#include <ap_int.h>
#include <hls_stream.h>

#include "../bnn-library.h"
#include "deconv.hpp"
#include "data/config_deconv2d.h"

#include <iostream>
#include <random>


void test_deconv2d(
	hls::stream<ap_uint<DeconvIFMCh*IPrecision> > & src,
	hls::stream<ap_uint<DeconvOFMCh*OPrecision> > & dst
);

int main() {
	std::cout << "Starting testbench for deconvolution" << std::endl;

	ap_uint<IPrecision>  inp_image[DeconvIFDim][DeconvIFDim][DeconvIFMCh];
	ap_uint<OPrecision>  out_image[DeconvOFDim][DeconvOFDim][DeconvOFMCh];
	hls::stream<ap_uint<DeconvIFMCh*IPrecision> > input_stream("input_stream");
	hls::stream<ap_uint<DeconvOFMCh*OPrecision> > output_stream("output_stream");
	ap_uint<WPrecision>  weights[DeconvIFMCh][DeconvOFMCh][DeconvKernel][DeconvKernel];
	
	{ // Feed random input sequence
		std::random_device rd;
		std::uniform_int_distribution<int> dist(0, (1<<(DeconvIFMCh*IPrecision))-1);
		unsigned  input_counter = 0;

		for(unsigned  y = 0; y < DeconvIFDim; y++) {
			for(unsigned  x = 0; x < DeconvIFDim; x++) {
				for(unsigned  c = 0; c < DeconvIFMCh; c++) {
					ap_uint<IPrecision>  val = dist(rd);
					input_stream.write(val);
					inp_image[y][x][c] = val;
					input_counter++;
				}
			}
		}
		if(input_counter != (DeconvIFDim * DeconvIFDim * DeconvIFMCh)) {
			std::cout << "Input stream not fully populated." << std::endl;
			return 1;
		}
	}
	std::cout << "Finished writing to input stream" << std::endl;

	// TODO - create weights
	// { // Feed random weight sequence
	// 	std::random_device rd;
	// 	std::uniform_int_distribution<int> dist(0, (1<<(WEIGHT_PRECISION))-1);
	// 	unsigned  input_counter = 0;

	// 	for(unsigned  y = 0; y < DECONV_INPUT_DIM_Y; y++) {
	// 		for(unsigned  x = 0; x < DECONV_INPUT_DIM_X; x++) {
	// 			for(unsigned  c = 0; c < INPUT_CHANNELS; c++) {
	// 				TI  val = dist(rd);
	// 				input_stream.write(val);
	// 				image[y][x][c] = val;
	// 				input_counter++;
	// 			}
	// 		}
	// 	}
	// 	if(input_counter != (DECONV_INPUT_DIM_X * DECONV_INPUT_DIM_Y * INPUT_CHANNELS)) {
	// 		std::cout << "Input stream not fully populated." << std::endl;
	// 		return 1;
	// 	}
	// }
	std::cout << "Finished writing to input stream" << std::endl;


	// TODO - calculate expected outputs from deconvolution
	std::cout << "Calculating expected output" << std::endl;
	deconv2d<
		DeconvIFDim,
		DeconvIFMCh,
		DeconvOFDim,
		DeconvOFMCh,
		DeconvKernel,
		DeconvStride,
		DeconvPadding,
		ap_uint<IPrecision>,
		ap_uint<OPrecision>,
		ap_uint<WPrecision>
	>(inp_image, weights, out_image);

	// Run top-level function
	test_deconv2d(input_stream, output_stream);
	std::cout << "Finished writing to output stream" << std::endl;

	// Verify correctness
	// for(unsigned  y = 0; y < OUTPUT_DIM_Y; y++) {
	// 	for(unsigned  x = 0; x < OUTPUT_DIM_X; x++) {
	// 		for(unsigned  c = 0; c < CHANNELS; c++) {
	// 			if(output_stream.empty()) {
	// 				std::cerr << "Missing outputs." << std::endl;
	// 				return  1;
	// 			}

	// 			T const  val = output_stream.read();
	// 			if(expected[y][x][c] != val) {
	// 				std::cerr << "Output mismatch." << std::endl;
	// 				return  1;
	// 			}
	// 		}
	// 	}
	// }
	// if(!output_stream.empty()) {
	// 	std::cerr << "Output stream not empty." << std::endl;
	// 	return 1;
	// }
}