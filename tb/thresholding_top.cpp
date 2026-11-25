#include "thresholding_top.hpp"

void thresholding_top(
	hls::stream<tensor<TI,  PE>> &src,
	hls::stream<tensor<TO, PE>> &dst
) {
#pragma HLS interface axis port=src register_mode=off
#pragma HLS interface axis port=dst
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS dataflow disable_start_propagation
	thresholding<N, C, thresh_init, BIAS, DEPTH_TRIGGER_BRAM, DEPTH_TRIGGER_URAM>(src, dst);
}
