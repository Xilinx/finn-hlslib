#include "softmax_top.hpp"

static SoftMax<ap_int<8>, float, N_CLASSES, SIMD_SM>  sm;

void Testbench_softmax(
	hls::stream<sm_in_t>  &in,
	hls::stream<sm_out_t> &out
) {
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS dataflow disable_start_propagation
	sm.execute(in, out);
}
