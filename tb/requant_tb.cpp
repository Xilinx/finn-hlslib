#include <hls_stream.h>
#include <ap_int.h>

#include <iostream>
#include <cmath>
#include <algorithm>

#include "data/requant_config.h"

void Testbench_requant(hls::stream<tensor<TI, PE>> &in, hls::stream<tensor<TO, PE>> &out);

// Reference parameters (must match requant_top.cpp)
static float const ref_scales[PE][NF] = {
	{ 1.0f, 0.5f, 2.0f, -1.0f },
	{ 0.25f, 1.0f, -0.5f, 3.0f }
};
static float const ref_biases[PE][NF] = {
	{ 0.0f, 10.0f, -5.0f, 0.0f },
	{ 1.0f, 0.0f, 0.0f, -100.0f }
};

// Reference requant
static int ref_requant(int x, unsigned pe, unsigned nf) {
	float  scaled = float(x) * ref_scales[pe][nf] + ref_biases[pe][nf];
	int  rounded = int(std::lrintf(scaled));
	return  std::max(MIN_VAL, std::min(MAX_VAL, rounded));
}

int main() {
	hls::stream<tensor<TI, PE>>  input_stream("input_stream");
	hls::stream<tensor<TO, PE>>  output_stream("output_stream");

	// Expected outputs
	TO  expected[NUM_WORDS][PE];

	// Generate test inputs: sweep values to exercise saturation
	int const  test_vals[] = { 0, 1, -1, 50, -50, 100, -100, 127, -128, 200, -200, 500 };
	constexpr unsigned  num_test = sizeof(test_vals) / sizeof(test_vals[0]);

	for(unsigned  w = 0; w < NUM_WORDS; w++) {
		unsigned  nf = w % NF;
		tensor<TI, PE>  x;
		for(unsigned  pe = 0; pe < PE; pe++) {
			int  val = test_vals[(w * PE + pe) % num_test];
			x[pe] = TI(val);
			expected[w][pe] = TO(ref_requant(val, pe, nf));
		}
		input_stream.write(x);
	}

	// Call function-pipelined kernel (2× for latency margin)
	for(unsigned  i = 0; i < 2 * NUM_WORDS; i++) {
		Testbench_requant(input_stream, output_stream);
	}

	// Verify
	int  errors = 0;
	for(unsigned  w = 0; w < NUM_WORDS; w++) {
		auto const  y = output_stream.read();
		for(unsigned  pe = 0; pe < PE; pe++) {
			TO  got = y[pe];
			TO  exp = expected[w][pe];
			if(got != exp) {
				std::cout << "ERROR word " << w << " pe " << pe
					<< " expected " << int(exp) << " got " << int(got) << std::endl;
				errors++;
			}
		}
	}

	if(errors) {
		std::cout << errors << " errors detected." << std::endl;
		return  1;
	}
	std::cout << "Test passed. " << NUM_WORDS << " words verified." << std::endl;
	return  0;
}
