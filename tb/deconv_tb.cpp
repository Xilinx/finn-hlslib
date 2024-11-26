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

#include "bnn-library.h"
#include "deconv.hpp"
#include "data/config_deconv2d.h"
#include "data/memdata_deconv2d.h"

#include <iostream>
#include <random>


void test_deconv2d(
	hls::stream<ap_uint<IFMCh1*IPrecision> > & src,
	hls::stream<ap_uint<OFMCh1*OPrecision> > & dst
);

int main() {
	std::cout << "Starting testbench for deconvolution" << std::endl;

	ap_uint<IPrecision>  inp_image[IFDim1][IFDim1][IFMCh1];
	ap_uint<OPrecision>  ref_image[OFDim1][OFDim1][OFMCh1];
	hls::stream<ap_uint<IFMCh1*IPrecision> > input_stream("input_stream");
	hls::stream<ap_uint<OFMCh1*OPrecision> > output_stream("output_stream");
	
	{ // Feed random input sequence
		std::minstd_rand  rd;
		std::uniform_int_distribution<int> dist(0, (1<<(IFMCh1*IPrecision))-1);

		unsigned  input_counter = 0;
		for(unsigned  y = 0; y < IFDim1; y++) {
			for(unsigned  x = 0; x < IFDim1; x++) {
				ap_uint<IFMCh1*IPrecision>  input_channel = 0;
				for(unsigned  c = 0; c < IFMCh1; c++) {
					ap_uint<IPrecision>  val = dist(rd);
					inp_image[y][x][c] = val;
					input_channel = input_channel >> IPrecision;
					input_channel(IFMCh1 * IPrecision - 1, (IFMCh1 - 1) * IPrecision) = val;
					input_counter++;
				}
				input_stream.write(input_channel);
			}
		}
		if(input_counter != (IFDim1 * IFDim1 * IFMCh1)) {
			std::cout << "Input stream not fully populated." << std::endl;
			return 1;
		}
	}
	std::cout << "Finished writing to input stream" << std::endl;

	// Create weights
	static ap_uint<WPrecision>  weights[IFMCh1][OFMCh1][Kernel1][Kernel1];
	{
		unsigned  oc = 0; // output channel counter
		unsigned  ic = 0; // input channel counter
		unsigned  kx = 0; // kernel_x counter
		unsigned  ky = 0; // kernel_y counter
		constexpr int  xTile = (IFMCh1 * Kernel1 * Kernel1) / ConvSIMD1;
		constexpr int  yTile = OFMCh1 / ConvPE1;
		for (unsigned  oy = 0; oy < yTile; oy++) {
			for (unsigned ox = 0; ox < xTile; ox++) {
				for (unsigned pe = 0; pe < ConvPE1; pe++) {
					for (unsigned simd = 0; simd < ConvSIMD1; simd++) {
						// need to transpose the weights since weights are for conv2d
						unsigned  dkx = Kernel1 - kx - 1;
						unsigned  dky = Kernel1 - ky - 1;
						weights[ic][oc][dky][dkx] = PARAM::weights.weights(oy*xTile + ox)[pe][simd];
						ic++;
						if (ic == IFMCh1){
							ic=0;
							kx++;
							if (kx == Kernel1){
								kx=0;
								ky++;
								if (ky == Kernel1){
									ky=0;
									oc++;
									if (oc == OFMCh1){
										oc=0;
									}
								}
							}
						}
					}
				}

			}
		}
	}
	std::cout << "Finished writing the weights" << std::endl;

	// initialize the output buffer to 0
	for (unsigned y = 0; y < OFDim1; y++) {
		for (unsigned x = 0; x < OFDim1; x++) {
			for (unsigned c = 0; c < OFMCh1; c++) {
				ref_image[y][x][c] = 0;
			}
		}
	}

	// calculate expected outputs from deconvolution
	std::cout << "Calculating expected output" << std::endl;
	deconv2d<
		IFDim1,
		IFMCh1,
		OFDim1,
		OFMCh1,
		Kernel1,
		Stride1,
		Padding1,
		ap_uint<IPrecision>,
		ap_uint<OPrecision>,
		ap_uint<WPrecision>
	>(inp_image, weights, ref_image);

	// Run top-level function
	test_deconv2d(input_stream, output_stream);
	std::cout << "Finished writing to output stream" << std::endl;

	{// Verify correctness
		ap_uint<OPrecision>  val, exp;
		unsigned int  num_errors = 0;
		for(unsigned  y = 0; y < OFDim1; y++) {
			for(unsigned  x = 0; x < OFDim1; x++) {

				if(output_stream.empty()) {
					std::cerr << "Missing outputs." << std::endl;
					return  1;
				}
				ap_uint<OFMCh1 * OPrecision>  out = output_stream.read();

				for(unsigned  c = 0; c < OFMCh1; c++) {
					exp = ref_image[y][x][c];
					val(OPrecision - 1, 0) = out((c + 1)*OPrecision - 1, c * OPrecision);
					if(exp != val) {
						std::cout << "Error: Expected["<<y<<"]["<<x<<"]["<<c<<"]="<<exp<<", got "<<out<< std::endl;
						num_errors++;
					}
				}
			}
		}
		if(!output_stream.empty()) {
			std::cerr << "Output stream not empty." << std::endl;
			return 1;
		}
		else if(num_errors == 0) {
			std::cout << "Outputs successfully aligns." << std::endl;
		}
		else {
			std::cerr << "Error: " << num_errors << " total errors." << std::endl;
			return 1;
		}
	}
}
