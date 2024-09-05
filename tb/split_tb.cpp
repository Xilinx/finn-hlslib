#include <iostream>

#include <hls_stream.h>
#include <hls_vector.h>

#include "data/split_config.h"
#include "split.hpp"


void Testbench_split(hls::stream<IN_TYPE> &in0_V, hls::stream<IN_TYPE> (&out_arr)[NUM_OUTPUTS]);
unsigned int folds_per_output[NUM_OUTPUTS] = FOLDS_PER_OUTPUT;

int main()
{
	hls::stream<IN_TYPE> in0_V ("in0_V");
	hls::stream<IN_TYPE> out_arr[NUM_OUTPUTS];
	hls::stream<IN_TYPE> expected[NUM_OUTPUTS];

	// prepare stimulus and expected output
	for (unsigned int counter = 0; counter < NUM_VECTORS; counter++){
		for(unsigned int output = 0; output < NUM_OUTPUTS; output++){
			for(unsigned int f = 0; f < folds_per_output[output]; f++){
				IN_TYPE el = (EL_TYPE)(output + 1);
				in0_V.write(el);
				expected[output].write(el);
			}
		}
	}

	unsigned timeout = 0;
	unsigned out_cnt = 0;
	while(timeout < 100){

		Testbench_split(in0_V, out_arr);

		bool all_empty = true;
		for(unsigned int output = 0; output < NUM_OUTPUTS; output++){
			if(!out_arr[output].empty()){
				all_empty = false;
				break;
			}
		}
		if(all_empty){
			timeout++;
		}
		else{
			for(int i = 0; i < NUM_OUTPUTS; i++){
				if(!out_arr[i].empty()){
					IN_TYPE y = out_arr[i].read();
					IN_TYPE exp = expected[i].read();
					if(y != exp)
						std::cerr << "ERROR! Unexpected output nr " << out_cnt << ". Got: " << y << " Expected: " << exp << std::endl;
					out_cnt++;
				}
			}
		}
	}
}


