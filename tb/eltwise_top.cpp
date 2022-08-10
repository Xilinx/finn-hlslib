#include "eltwise.hpp"
#include "interpret.hpp"

#include <cassert>

#include "data/eltwise_config.h"


void Testbench_Eltwise(
	int  mode,
	hls::stream<ap_uint<NUM_CHANNELS * INPUT_1_WIDTH>> &in0,
	hls::stream<ap_uint<NUM_CHANNELS * INPUT_2_WIDTH>> &in1,
	hls::stream<ap_uint<NUM_CHANNELS * OUTPUT_WIDTH>>  &out
) {
	switch(mode) {
	case 0:
		StreamingEltwise<NUM_CHANNELS, NUM_CHANNELS, NUM_WORDS, Slice<ap_int<INPUT_1_WIDTH>>, Slice<ap_int<INPUT_2_WIDTH>>, Slice<ap_int<OUTPUT_WIDTH>>>(
			in0, in1, out, [](auto a, auto b) { return  a + b; }
		);
		break;
	case 1:
		StreamingEltwise<NUM_CHANNELS, NUM_CHANNELS, NUM_WORDS, Slice<ap_int<INPUT_1_WIDTH>>, Slice<ap_int<INPUT_2_WIDTH>>, Slice<ap_int<OUTPUT_WIDTH>>>(
			in0, in1, out, [](auto a, auto b) { return  a - b; }
		);
		break;
	case 2:
		StreamingEltwise<NUM_CHANNELS, NUM_CHANNELS, NUM_WORDS, Slice<ap_int<INPUT_1_WIDTH>>, Slice<ap_int<INPUT_2_WIDTH>>, Slice<ap_int<OUTPUT_WIDTH>>>(
			in0, in1, out, [](auto a, auto b) { return  a>b? a-b : b-a; }
		);
		break;
	default:
		assert(!"Mode out of range");
	}
}
