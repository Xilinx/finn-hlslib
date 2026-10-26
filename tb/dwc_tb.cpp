#include "dwc_top.hpp"

#include <iostream>

int main() {
	hls::stream<tensor<T, NI>>  input_stream("input_stream");
	hls::stream<tensor<T, NO>>  output_stream("output_stream");

	constexpr unsigned  NUM_OUT = NumInWords * NI / NO;
	tensor<T, NO>  expected[NUM_OUT];

	// Generate input and expected output
	unsigned  count_out = 0;
	unsigned  count_elem = 0;
	tensor<T, NO>  accum;
	for(unsigned  i = 0; i < NumInWords; i++) {
		tensor<T, NI>  value;
		for(unsigned  e = 0; e < NI; e++)  value[e] = T(i * NI + e);
		input_stream.write(value);

		if constexpr(NI >= NO) {
			// Narrowing or equal: each input yields NI/NO outputs
			for(unsigned  k = 0; k < NI/NO; k++) {
				tensor<T, NO>  chunk;
				for(unsigned  e = 0; e < NO; e++)  chunk[e] = value[k * NO + e];
				expected[count_out++] = chunk;
			}
		}
		else {
			// Widening: accumulate NO/NI inputs into one output
			for(unsigned  e = 0; e < NI; e++)  accum[count_elem++] = value[e];
			if(count_elem == NO) {
				expected[count_out++] = accum;
				count_elem = 0;
			}
		}
	}

	// Call function-pipelined kernel enough times to drain all outputs
	constexpr unsigned  ITERS = 2 * (NumInWords > NUM_OUT ? NumInWords : NUM_OUT);
	for(unsigned  i = 0; i < ITERS; i++)  Testbench_dwc(input_stream, output_stream);

	// Verify
	for(unsigned  i = 0; i < NUM_OUT; i++) {
		tensor<T, NO>  value = output_stream.read();
		if(value != expected[i]) {
			std::cout << "ERROR at output " << i
				<< " expected " << expected[i] << " got " << value << std::endl;
			return  1;
		}
	}
	std::cout << "Test passed." << std::endl;
	return  0;
}
