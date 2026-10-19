#include "eltwise.hpp"

#include "data/eltwise_config.h"


using in0_t = tensor<ap_int<INPUT_1_WIDTH>, NUM_CHANNELS>;
using in1_t = tensor<ap_int<INPUT_2_WIDTH>, NUM_CHANNELS>;
using out_t = tensor<ap_int<OUTPUT_WIDTH>,  NUM_CHANNELS>;

void Testbench_Eltwise(
	hls::stream<in0_t> &in0,
	hls::stream<in1_t> &in1,
	hls::stream<out_t> &out
) {
#pragma HLS interface ap_ctrl_none port=return
	StreamingEltwise<NUM_CHANNELS>(
		in0, in1, out, [](auto a, auto b) { return  a + b; }
	);
}
