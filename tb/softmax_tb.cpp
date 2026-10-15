#include <iostream>
#include <cmath>
#include <hls_stream.h>
#define AP_INT_MAX_W 8191
#include "ap_int.h"

#include "softmax_top.hpp"

#define MAX_IMAGES 2

int main() {
	// Test vectors: N_CLASSES=8 values
	int  inputs[MAX_IMAGES][N_CLASSES] = {
		{  10,  20,  30,  40,  50,  60,  70,  80 },
		{ -50, -20,   0,  10,  30,  60,  80, 100 },
	};

	hls::stream<sm_in_t>   input_stream("input_stream");
	hls::stream<sm_out_t>  output_stream("output_stream");

	constexpr unsigned  FOLDS = N_CLASSES / SIMD_SM;

	// Feed input
	for(unsigned  n = 0; n < MAX_IMAGES; n++) {
		for(unsigned  f = 0; f < FOLDS; f++) {
			ap_uint<SIMD_SM * 8>  word = 0;
			for(unsigned  s = 0; s < SIMD_SM; s++) {
				word((s+1)*8-1, s*8) = ap_uint<8>(ap_int<8>(inputs[n][f*SIMD_SM + s]));
			}
			input_stream.write(sm_in_t(word));
		}
	}

	// Call the 3-stage pipeline — need enough iterations
	// Each image processes FOLDS words through 3 stages; give generous headroom
	constexpr unsigned  ITERS = MAX_IMAGES * FOLDS * 20;
	for(unsigned  i = 0; i < ITERS; i++)  Testbench_softmax(input_stream, output_stream);

	// Verify
	int  err_cnt = 0;
	for(unsigned  n = 0; n < MAX_IMAGES; n++) {
		// Compute reference softmax
		float  ref[N_CLASSES];
		float  max_val = -1e30f;
		for(unsigned  i = 0; i < N_CLASSES; i++)
			if(inputs[n][i] > max_val)  max_val = inputs[n][i];
		float  sum = 0;
		for(unsigned  i = 0; i < N_CLASSES; i++) {
			ref[i] = std::exp(float(inputs[n][i]) - max_val);
			sum += ref[i];
		}
		for(unsigned  i = 0; i < N_CLASSES; i++)
			ref[i] /= sum;

		// Read and compare
		for(unsigned  f = 0; f < FOLDS; f++) {
			sm_out_t  outElem = output_stream.read();
			for(unsigned  s = 0; s < SIMD_SM; s++) {
				unsigned  idx = f * SIMD_SM + s;
				float  produced = outElem[s];
				float  expected = ref[idx];
				float  diff = std::fabs(produced - expected);
				if(diff > 1e-3f) {
					std::cout << "ERROR: Image " << n << " class " << idx
						<< " expected " << expected << " got " << produced
						<< " diff " << diff << std::endl;
					err_cnt++;
				}
			}
		}
		if(err_cnt == 0)
			std::cout << "Image #" << n << " passed." << std::endl;
	}

	if(err_cnt == 0)  std::cout << "Test passed." << std::endl;
	return  err_cnt ? 1 : 0;
}
