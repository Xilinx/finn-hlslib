#include <hls_stream.h>
#include <ap_int.h>

#include <iostream>
#include <iomanip>

#include "data/eltwise_config.h"


using namespace hls;


void Testbench_Eltwise(int mode, stream<ap_uint<NUM_CHANNELS * INPUT_1_WIDTH> > & in0, stream<ap_uint<NUM_CHANNELS * INPUT_2_WIDTH> > & in1, stream<ap_uint<NUM_CHANNELS * OUTPUT_WIDTH> > & out);

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

	stream<ap_uint<NUM_CHANNELS * INPUT_1_WIDTH>> input_stream1("input_stream1");
	stream<ap_uint<NUM_CHANNELS * INPUT_2_WIDTH>> input_stream2("input_stream2");
	stream<ap_uint<NUM_CHANNELS * OUTPUT_WIDTH>>  output_stream("output_stream");
	ap_uint<NUM_CHANNELS * OUTPUT_WIDTH>  expected[NUM_REPEAT*NUM_WORDS];

	for(int mode = 0; mode < 3; mode++) {
		unsigned int count_out = 0;
		unsigned int count_in = 0;
		for (unsigned int counter = 0; counter < NUM_REPEAT*NUM_WORDS; counter++) {
			ap_uint<NUM_CHANNELS * INPUT_1_WIDTH> value1 = (ap_uint<NUM_CHANNELS * INPUT_1_WIDTH>) (counter);
			ap_uint<NUM_CHANNELS * INPUT_2_WIDTH> value2 = (ap_uint<NUM_CHANNELS * INPUT_2_WIDTH>) (counter + 1);
			sw_golden(mode, value1, value2, expected[counter]);
			input_stream1.write(value1);
			input_stream2.write(value2);		
		}
		Testbench_Eltwise(mode, input_stream1, input_stream2, output_stream);
		for (unsigned int counter = 0; counter < NUM_REPEAT*NUM_WORDS; counter++) {
			ap_uint<NUM_CHANNELS * OUTPUT_WIDTH> value = output_stream.read();
			if(value!= expected[counter]) {
				std::cout << "ERROR with mode " << mode << " counter " << counter << std::hex << " expected " << expected[counter] << " value " << value << std::dec << std::endl;
				return  1;
			}
			else {
				std::cout << "mode = " << mode << " counter = " << counter << " " << std::hex << value << std::dec << std::endl;
			}
		}
	}
}
