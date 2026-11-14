#include "eltwise_top.hpp"
#include "eltwise.hpp"

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
