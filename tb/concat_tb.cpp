#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include <numeric>

#include <hls_stream.h>
#include <hls_vector.h>
#define AP_INT_MAX_W 8191
#include "ap_int.h"

#include "data/concat_config.h"
#include "concat.hpp"


void Testbench_concat(hls::stream<IN_TYPE0> &in0_V, hls::stream<IN_TYPE1> &in1_V, hls::stream<IN_TYPE2> &in2_V, hls::stream<IN_TYPE3> &in3_V, hls::stream<OUT_TYPE> &out_V);


int main()
{
	hls::stream<IN_TYPE0> in0_V ("in0_V");
	hls::stream<IN_TYPE1> in1_V ("in1_V");
	hls::stream<IN_TYPE2> in2_V ("in2_V");
	hls::stream<IN_TYPE3> in3_V ("in3_V");
	hls::stream<OUT_TYPE> out_V ("out_V");
	hls::stream<OUT_TYPE> expected ("expected");

	for (unsigned int counter = 0; counter < NUM_VECTORS; counter++){
		for(unsigned int f = 0; f < NUM_FOLDS0; f++){
			in0_V.write({1, 1});
			expected.write({1, 1});
		}
		for(unsigned int f = 0; f < NUM_FOLDS1; f++){
			in1_V.write({2, 2});
			expected.write({2, 2});
		}
		for(unsigned int f = 0; f < NUM_FOLDS2; f++){
			in2_V.write({3, 3});
			expected.write({3, 3});
		}
		for(unsigned int f = 0; f < NUM_FOLDS3; f++){
			in3_V.write({4, 4});
			expected.write({4, 4});
		}
	}

	unsigned timeout = 0;
	unsigned out_cnt = 0;
	while(timeout < 100){

		Testbench_concat(in0_V, in1_V, in2_V, in3_V, out_V);

		if(out_V.empty()){
			timeout++;
		}
		else{
			OUT_TYPE y = out_V.read();
			OUT_TYPE exp = expected.read();
			if(y != exp)
				std::cerr << "ERROR! Unexpected output nr " << out_cnt << ". Got: " << y << " Expected: " << exp << std::endl;
			std::cout << out_cnt << " output:  y: " << y << "exp: " << exp << std::endl;
			out_cnt++;
		}
	}
}


