/******************************************************************************
 *  Copyright (c) 2021, Xilinx, Inc.
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
 *  Authors: Timoteo Garcia Bertoa <timoteog@xilinx.com>
 *
 *  \file tmrc_stmr_tb.cpp
 *
 *  Testbench for the HLS block which performs redundancy checks
 *
 *****************************************************************************/
#define AP_INT_MAX_W 16384
#define INJ false
#include <iostream>
#include <fstream>
#include <time.h>
#include <cmath>
#include <ctime>
#include <cstring>
#include <hls_stream.h>
#include <cstdlib>
#include "ap_int.h"
#include "weights.hpp"
#include "bnn-library.h"
#include "memdata_tmrc.h"
#include "config_tmrc.h"
#include "activations.hpp"
#include "weights.hpp"
#include "activations.hpp"
#include "interpret.hpp"
#include "mvau.hpp"
#include "conv.hpp"
using namespace hls;
using namespace std;

#define MAX_IMAGES 2

void Testbench_tmrc_stmr(stream<ap_uint<OFM_Channels1*ACTIVATION_PRECISION> > & in,
						 stream<ap_uint<(OFM_Channels1-NUM_RED*(REDF-1))*ACTIVATION_PRECISION> > & out,
						 unsigned int numReps,
						 ap_uint<2> &errortype);

int main()
{
	static	ap_uint<ACTIVATION_PRECISION> OFM_IN[MAX_IMAGES][OFMDim1*OFMDim1][OFM_Channels1];
	static	ap_uint<ACTIVATION_PRECISION> TEST[MAX_IMAGES][OFMDim1][OFMDim1][OFM_Channels1];
	//static	ap_uint<ACTIVATION_PRECISION> TEST[MAX_IMAGES][OFMDim1][OFMDim1][OFM_Channels1];
	stream<ap_uint<OFM_Channels1*ACTIVATION_PRECISION> > input_stream("input_stream");
	stream<ap_uint<(OFM_Channels1-NUM_RED*(REDF-1))*ACTIVATION_PRECISION> > output_stream("output_stream");
	int ch_pointer = 0;
	int numTriplets = sizeof(PARAM::red_ch_index)/sizeof(PARAM::red_ch_index[0]);

	// Generate input OFM
	unsigned int counter = 0;
	unsigned int counter_trip = 0;
	for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++) {
		for (unsigned int oy = 0; oy < OFMDim1; oy++) {
			for (unsigned int ox = 0; ox < OFMDim1; ox++) {
				ap_uint<ACTIVATION_PRECISION*OFM_Channels1> ofm_in_channel = 0;
				ch_pointer = 0;
				for(unsigned int channel = 0; channel < OFM_Channels1; channel++)
				{
					ap_uint<ACTIVATION_PRECISION> input = (ap_uint<ACTIVATION_PRECISION>)(counter);
					OFM_IN[n_image][oy*OFMDim1+ox][channel]= input;
					ofm_in_channel = ofm_in_channel >> ACTIVATION_PRECISION;
					ofm_in_channel(OFM_Channels1*ACTIVATION_PRECISION-1,(OFM_Channels1-1)*ACTIVATION_PRECISION)=input;
					TEST[n_image][ox][oy][channel]=input;
					//cout << "input: " << input << endl;
					// Check if the current channel needs to be triplicated
					if (PARAM::red_ch_index[ch_pointer] == channel) {
						if (ch_pointer < numTriplets) { ch_pointer++; }
						counter_trip++;
					}
					// If current channel is part of a triplet, track it and don't increase counter
					// If 3 channels have been counted already, increase counter to assign other value to next channel
					if(counter_trip != 0){
						if(counter_trip == 3){
							counter++;
							counter_trip = 0;
						} else {
							counter_trip++;
						}
					} else {
						counter++;
					}
				}
				input_stream.write(ofm_in_channel);
				//cout << "ofm_in_channel: " << ofm_in_channel << endl;
			}
		}
	}

	/* Error flag:
	 * 0: No errors
	 * 1: One PE is faulty
	 * 2: All PEs compute different values
	 */
	ap_uint<2> errortype = 0;

	// Perform test which checks redundant channels
	Testbench_tmrc_stmr(input_stream, output_stream, MAX_IMAGES, errortype);

	ap_uint<INPUT_PRECISION> TESTIMAGE[1][IFMDim1*IFMDim1][IFM_Channels1];
	int err_counter = 0, err_perimage=0;
	ap_uint<ACTIVATION_PRECISION> out_chan;
	int ch_conv = 0;
	// Loop of batch of images
	for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++) {

		cout << "Image " << n_image << endl;
		ap_uint<(OFM_Channels1-NUM_RED*(REDF-1))*ACTIVATION_PRECISION> outElem;
		cout << "TMRC outputs (applying redundancy check): " << endl;
		cout << "Format: <OFM[Row][Column][Channel]>" << endl;
		// Print output values
		for (unsigned int oy = 0; oy < OFMDim1; oy++) {
			for (unsigned int ox = 0; ox < OFMDim1; ox++) {
				outElem = output_stream.read();
				ch_pointer = 0;
				ch_conv = 0;
				for (unsigned int ch = 0; ch < (OFM_Channels1-NUM_RED*(REDF-1)); ch++) {
					ap_uint<ACTIVATION_PRECISION> EXP = TEST[n_image][ox][oy][ch_conv];// + e * OFM_Channels1];
					out_chan(ACTIVATION_PRECISION-1,0) = outElem((ch + 1)*ACTIVATION_PRECISION-1,ch*ACTIVATION_PRECISION);
					//std::cout << "OFM[" << ox << "][" << oy << "][" << ch << "]: " << outElem((ch+1)*ACTIVATION_PRECISION-1, ch*ACTIVATION_PRECISION) << std::endl;
				
					// red_ch_index points to the channel index of the OFM which includes triplications.
					// For example, for an original OFM with 3 channels, having triplicated the second and third, the indexes are 1 and 2. red_ch_index
					if ((PARAM::red_ch_index[ch_pointer]-2*ch_pointer) == ch) {

						ap_uint<ACTIVATION_PRECISION> EXP1 = TEST[n_image][ox][oy][ch_conv];// + e * OFM_Channels1];
						ap_uint<ACTIVATION_PRECISION> EXP2 = TEST[n_image][ox][oy][ch_conv+1];// + e * OFM_Channels1];
						ap_uint<ACTIVATION_PRECISION> EXP3 = TEST[n_image][ox][oy][ch_conv+2];// + e * OFM_Channels1];
						ap_uint<ACTIVATION_PRECISION> MAJ;

						if (INJ == false) {
							MAJ = EXP1;
							if (MAJ != out_chan){
								std::cout << "ERROR: Triplet case. No errors were injected. The computed values were " << EXP1 << ", " << EXP2 << ", " << EXP3 << ". " <<"Expected OFM["<<ox <<"]["<<oy<<"]["<<ch<<"]=" << MAJ << " actual " <<  out_chan << std::endl;
								err_counter ++;
								err_perimage++;
								cout << "Errors: " << err_perimage << endl;
							} else {
								std::cout << "Triplet case. No errors were injected. The computed values were " << EXP1 << ", " << EXP2 << ", " << EXP3 << ". " <<"Expected OFM["<<ox <<"]["<<oy<<"]["<<ch<<"]=" << MAJ << " actual " <<  out_chan << std::endl;
							}
						} else {
							if ((EXP1 == EXP2) || (EXP1 == EXP3)) { MAJ = EXP1; }
							else if (EXP2 == EXP3) { MAJ = EXP2; }
							else { MAJ = EXP1; }
							if (MAJ != out_chan){
								std::cout << "ERROR: Triplet case. Errors might have been injected. The computed values were " << EXP1 << ", " << EXP2 << ", " << EXP3 << ". " <<"Expected OFM["<<ox <<"]["<<oy<<"]["<<ch<<"]=" << MAJ << " actual " <<  out_chan << std::endl;
								err_counter ++;
								err_perimage++;
								cout << "Errors: " << err_perimage << endl;
							} else {
								std::cout << "Triplet case. Errors might have been injected. The computed values were " << EXP1 << ", " << EXP2 << ", " << EXP3 << ". " <<"Expected OFM["<<ox <<"]["<<oy<<"]["<<ch<<"]=" << MAJ << " actual " <<  out_chan << std::endl;
							}
						}

						if (ch_pointer < numTriplets) { ch_pointer++; }
						ch_conv += 3;
					} else {
						if (EXP != out_chan){

							std::cout << "ERROR: Expected OFM["<<ox <<"]["<<oy<<"]["<<ch<<"]=" << EXP << " actual " <<  out_chan << std::endl;
							//return 1;
							err_counter ++;
							err_perimage++;
							cout << "Errors: " << err_perimage << endl;
							//if(err_counter>10)
								//return 1;
						}else{
							std::cout << "Expected OFM[" << ox << "][" << oy << "][" << ch << "]: " << EXP << " actual " <<  out_chan << std::endl;
						}
						ch_conv += 1;
					}
				}
			}
		}
		cout << "---------------" << endl;

		// Check errors:
		if(errortype == 0 ){
			cout << "No faulty PEs!" << endl;
		} else if(errortype &(1 << 1)){
			cout << "The 3 PEs computed different values" << endl;
		} else if(errortype & 1){
			cout << "One PE faulty, results chosen from a valid PE" << endl;
		}

		if(err_perimage == 0){
			std::cout << "Image # " << n_image << " passed the testing.\n\n"<< std::endl;
		} else {
			err_perimage=0;
			std::cout << "Image # " << n_image << " failed the testing.\n\n"<< std::endl;
		}
	}

	if(err_counter == 0){
		return 0;
	} else {
		return 1;
	}
}


