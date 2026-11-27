/****************************************************************************
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Shane T. Fleming <shane.fleming@amd.com>
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#include "layernorm_top.hpp"
#include <cstdlib>
#include <cmath>


// how much to test
constexpr unsigned  ROUNDS = 7;


// Function to calculate mean
static float mean(float const *const  array, unsigned const  size) {
	float  sum = 0.f;
	for(unsigned  i = 0; i < size; i++) {
		sum += array[i];
	}
	return  sum / size;
}

// Function to calculate variance
static float variance(float const *const  array, unsigned const  size, float const  mean) {
	float  sum = 0.f;
	for(unsigned  i = 0; i < size; i++) {
		float const  d = array[i] - mean;
		sum += d*d;
	}
	return  sum / size;
}

static void ref_layernorm(float const *const  input, float *const  output, unsigned const  length) {
	float const  mean_val = mean(input, length);
	float const  variance_val = variance(input, length, mean_val);
	for(unsigned  i = 0; i < length; i++) {
		output[i] = (input[i] - mean_val) / sqrt(variance_val + 1e-5);
	}
}

static bool closeEnough(float const  num1, float const  num2, float const  tolerance) {
	return  std::abs(num1 - num2) <= tolerance;
}


int main() {
	hls::stream<tensor<TI, SIMD>>  src;
	hls::stream<tensor<TO, SIMD>>  dst;

	// Reference input and output
	float  ref_in [ROUNDS][N];
	float  ref_out[ROUNDS][N];

	// Create the input stream (and test stream)
	for(unsigned r=0; r<ROUNDS; r++){
		for (unsigned i=0; i<N; i+=SIMD) {
			tensor<TI, SIMD>  x;
			for(unsigned j=0; j<SIMD; j++) {
				TI const  ref_val = (r+1) * (i + j);
				x[j] = ref_val;
				ref_in[r][i+j] = float(ref_val);
			}
			src.write(x);
		}
		ref_layernorm(ref_in[r], ref_out[r], N);
	}

	unsigned  r = 0;
	unsigned  i = 0;
	unsigned  errors = 0;

	unsigned  timeout = 0;
	while(timeout < 200) {
		layernorm_top(src, dst);
		if(dst.empty())  timeout++;
		else if(r < ROUNDS) {
			auto const  y = dst.read();
			for(unsigned j=0; j<SIMD; j++) {
				TO  val = y[j];
				if(!closeEnough(val, ref_out[r][i+j], 1e-5)) {
					std::cerr << "Error: "  << val << " !=  " << ref_out[r][i+j] << std::endl;
					errors++;
				}
			}
			i += SIMD;
			if(i == N) {
				i = 0;
				r++;
			}
			timeout = 0;
		}
	}
	if(!dst.empty()) {
		std::cerr << "Extraneous output." << std::endl;
		errors++;
	}
	return  errors;
}
