#include <hls_stream.h>
#include <ap_int.h>

#include <iostream>
#include <iomanip>

#include "data/eltwise_config.h"
#include "tensor.hpp"


using in0_t = tensor<ap_int<INPUT_1_WIDTH>, NUM_CHANNELS>;
using in1_t = tensor<ap_int<INPUT_2_WIDTH>, NUM_CHANNELS>;
using out_t = tensor<ap_int<OUTPUT_WIDTH>,  NUM_CHANNELS>;

void Testbench_Eltwise(int mode, hls::stream<in0_t> &in0, hls::stream<in1_t> &in1, hls::stream<out_t> &out);

void sw_golden(int mode, ap_uint<NUM_CHANNELS * INPUT_1_WIDTH> val1, ap_uint<NUM_CHANNELS * INPUT_2_WIDTH> val2, ap_uint<NUM_CHANNELS * OUTPUT_WIDTH> & out) {
	for (int i = 0; i < NUM_CHANNELS; i++) {
		ap_int<INPUT_1_WIDTH> op1 = val1((i+1)*INPUT_1_WIDTH-1, i*INPUT_1_WIDTH);
		ap_int<INPUT_2_WIDTH> op2 = val2((i+1)*INPUT_2_WIDTH-1, i*INPUT_2_WIDTH);
		ap_int<OUTPUT_WIDTH> sum = 0;
		switch(mode) {
			case 0:
				sum = op1 + op2;
				break;
			case 1:
				sum = op1 - op2;
				break;
			case 2:
				sum = op1 > op2 ? op1-op2 : op2-op1;
				break;
			default:
				break;
		}
		out((i+1)*OUTPUT_WIDTH-1,i*OUTPUT_WIDTH)  = sum;
	}
}

int main() {

	hls::stream<in0_t> input_stream1("input_stream1");
	hls::stream<in1_t> input_stream2("input_stream2");
	hls::stream<out_t> output_stream("output_stream");
	ap_uint<NUM_CHANNELS * OUTPUT_WIDTH>  expected[NUM_REPEAT*NUM_WORDS];

	for(int mode = 0; mode < 3; mode++) {
		for (unsigned int counter = 0; counter < NUM_REPEAT*NUM_WORDS; counter++) {
			ap_uint<NUM_CHANNELS * INPUT_1_WIDTH> value1 = counter;
			ap_uint<NUM_CHANNELS * INPUT_2_WIDTH> value2 = counter + 1;
			sw_golden(mode, value1, value2, expected[counter]);
			input_stream1.write(in0_t(value1));
			input_stream2.write(in1_t(value2));
		}
		Testbench_Eltwise(mode, input_stream1, input_stream2, output_stream);
		for (unsigned int counter = 0; counter < NUM_REPEAT*NUM_WORDS; counter++) {
			ap_uint<NUM_CHANNELS * OUTPUT_WIDTH> value = output_stream.read();
			if(value != expected[counter]) {
				std::cout << "ERROR with mode " << mode << " counter " << counter << std::hex << " expected " << expected[counter] << " value " << value << std::dec << std::endl;
				return  1;
			}
			else {
				std::cout << "mode = " << mode << " counter = " << counter << " " << std::hex << value << std::dec << std::endl;
			}
		}
	}
}
