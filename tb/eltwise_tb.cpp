#include "eltwise_top.hpp"

#include <iostream>

int main() {
	constexpr unsigned  N = NUM_REPEAT * NUM_WORDS;

	hls::stream<in0_t>  input_stream1("input_stream1");
	hls::stream<in1_t>  input_stream2("input_stream2");
	hls::stream<out_t>  output_stream("output_stream");

	// Feed inputs
	ap_uint<NUM_CHANNELS * OUTPUT_WIDTH>  expected[N];
	for(unsigned  i = 0; i < N; i++) {
		ap_uint<NUM_CHANNELS * INPUT_1_WIDTH>  value1 = i;
		ap_uint<NUM_CHANNELS * INPUT_2_WIDTH>  value2 = i + 1;
		input_stream1.write(in0_t(value1));
		input_stream2.write(in1_t(value2));
		// Reference: add
		for(int c = 0; c < NUM_CHANNELS; c++) {
			ap_int<INPUT_1_WIDTH>  op1 = value1((c+1)*INPUT_1_WIDTH-1, c*INPUT_1_WIDTH);
			ap_int<INPUT_2_WIDTH>  op2 = value2((c+1)*INPUT_2_WIDTH-1, c*INPUT_2_WIDTH);
			expected[i]((c+1)*OUTPUT_WIDTH-1, c*OUTPUT_WIDTH) = op1 + op2;
		}
	}

	// Call function-pipelined kernel
	for(unsigned  i = 0; i < 2*N; i++)  Testbench_Eltwise(input_stream1, input_stream2, output_stream);

	// Verify
	for(unsigned  i = 0; i < N; i++) {
		ap_uint<NUM_CHANNELS * OUTPUT_WIDTH>  value = output_stream.read();
		if(value != expected[i]) {
			std::cout << "ERROR counter " << i << std::hex
				<< " expected " << expected[i] << " value " << value
				<< std::dec << std::endl;
			return  1;
		}
	}
	std::cout << "Test passed." << std::endl;
	return  0;
}
