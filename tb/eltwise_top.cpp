#include "eltwise.hpp"

#include <cassert>

#include "data/eltwise_config.h"


void Testbench_Eltwise(
	int  mode,
	hls::stream<tensor<ap_int<INPUT_1_WIDTH>, NUM_CHANNELS>> &in0,
	hls::stream<tensor<ap_int<INPUT_2_WIDTH>, NUM_CHANNELS>> &in1,
	hls::stream<tensor<ap_int<OUTPUT_WIDTH>,  NUM_CHANNELS>> &out
) {
	switch(mode) {
	case 0:
		StreamingEltwise<NUM_CHANNELS, NUM_CHANNELS, NUM_WORDS>(
			in0, in1, out, [](auto a, auto b) { return  a + b; }
		);
		break;
	case 1:
		StreamingEltwise<NUM_CHANNELS, NUM_CHANNELS, NUM_WORDS>(
			in0, in1, out, [](auto a, auto b) { return  a - b; }
		);
		break;
	case 2:
		StreamingEltwise<NUM_CHANNELS, NUM_CHANNELS, NUM_WORDS>(
			in0, in1, out, [](auto a, auto b) { return  a>b? a-b : b-a; }
		);
		break;
	default:
		assert(!"Mode out of range");
	}
}
