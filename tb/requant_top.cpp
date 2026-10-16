#include "requant.hpp"
#include "data/requant_config.h"

// Test parameters: simple scale + bias per channel
// Channels are indexed as [pe][nf] with PE=2, NF=4
// Channel layout: ch0=(pe=0,nf=0) ch1=(pe=1,nf=0) ch2=(pe=0,nf=1) ...
static RequantParams<NF, PE> const rq_params = {
	.scales = {
		// pe=0: nf=0..3
		{ 1.0f, 0.5f, 2.0f, -1.0f },
		// pe=1: nf=0..3
		{ 0.25f, 1.0f, -0.5f, 3.0f }
	},
	.biases = {
		// pe=0: nf=0..3
		{ 0.0f, 10.0f, -5.0f, 0.0f },
		// pe=1: nf=0..3
		{ 1.0f, 0.0f, 0.0f, -100.0f }
	}
};

void Testbench_requant(
	hls::stream<tensor<TI, PE>> &in,
	hls::stream<tensor<TO, PE>> &out
) {
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS ARRAY_PARTITION variable=rq_params.scales complete dim=1
#pragma HLS ARRAY_PARTITION variable=rq_params.biases complete dim=1
	Requant_Batch<NumChannels, PE, MIN_VAL, MAX_VAL>(in, out, rq_params);
}
