#include <iostream>

#include <hls_stream.h>
#include <hls_vector.h>

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
			IN_TYPE0 el0 = (EL_TYPE0)1;
			in0_V.write(el0);
			expected.write(el0);
		}
		for(unsigned int f = 0; f < NUM_FOLDS1; f++){
			IN_TYPE0 el1 = (EL_TYPE1)2;
			in1_V.write(el1);
			expected.write(el1);
		}
		for(unsigned int f = 0; f < NUM_FOLDS2; f++){
			IN_TYPE0 el2 = (EL_TYPE2)3;
			in2_V.write(el2);
			expected.write(el2);
		}
		for(unsigned int f = 0; f < NUM_FOLDS3; f++){
			IN_TYPE0 el3 = (EL_TYPE3)4;
			in3_V.write(el3);
			expected.write(el3);
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
			out_cnt++;
		}
	}
}


