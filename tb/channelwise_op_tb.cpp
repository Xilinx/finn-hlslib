#include <iostream>
#include <hls_stream.h>
#define AP_INT_MAX_W 8191
#include "ap_int.h"

#include "channelwise_op_top.h"

#define MAX_IMAGES 2

using  in_t  = tensor<ap_uint<INPUT_BITS>, PE>;
using  out_t = tensor<MULT_OUT_TYPE, PE>;

int main() {
	constexpr unsigned  TOTAL = MAX_IMAGES * IFMDim * IFMDim * FOLD;

	int  IMAGE[MAX_IMAGES][IFMDim][IFMDim][IFM_Channels];

	hls::stream<in_t>   input_stream("input_stream");
	hls::stream<out_t>  output_stream("output_stream");

	// Generate input
	unsigned  counter = 0;
	for(unsigned  n = 0; n < MAX_IMAGES; n++) {
		for(unsigned  oy = 0; oy < IFMDim; oy++) {
			for(unsigned  ox = 0; ox < IFMDim; ox++) {
				for(unsigned  f = 0; f < FOLD; f++) {
					ap_uint<PE * INPUT_BITS>  word = 0;
					for(unsigned  p = 0; p < PE; p++) {
						unsigned  ch = f*PE + p;
						ap_uint<INPUT_BITS>  val = (ap_uint<INPUT_BITS>)(counter);
						IMAGE[n][ox][oy][ch] = (int)(unsigned)val;
						word((p+1)*INPUT_BITS-1, p*INPUT_BITS) = val;
						counter++;
					}
					input_stream.write(in_t(word));
				}
			}
		}
	}

	// Call function-pipelined kernel
	for(unsigned  i = 0; i < 2*TOTAL; i++)  Testbench_channelwise_op(input_stream, output_stream);

	// Verify
	int  err_counter = 0;
	for(unsigned  n = 0; n < MAX_IMAGES; n++) {
		int  err_perimage = 0;
		for(unsigned  oy = 0; oy < IFMDim; oy++) {
			for(unsigned  ox = 0; ox < IFMDim; ox++) {
				for(unsigned  f = 0; f < FOLD; f++) {
					out_t  outElem = output_stream.read();
					for(unsigned  p = 0; p < PE; p++) {
						unsigned  ch = f*PE + p;
						int  expected = IMAGE[n][ox][oy][ch];
						expected = bipolar_init[p][f] ? expected : -expected;
						expected += add_init[p][f];
						expected *= mult_init[p][f];

						MULT_OUT_TYPE  produced = outElem[p];
						if(expected != produced) {
							std::cout << "ERROR: Expected[" << oy << "][" << ox << "][" << ch << "]="
								<< expected << " actual " << produced << std::endl;
							err_counter++;
							err_perimage++;
						}
					}
				}
			}
		}
		if(err_perimage == 0)
			std::cout << "Image #" << n << " passed." << std::endl;
		else
			std::cout << "Image #" << n << " failed. Errors: " << err_perimage << std::endl;
	}
	return  err_counter ? 1 : 0;
}
