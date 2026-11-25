#include "thresholding_top.hpp"

void thresholding_top(
	hls::stream<tensor<TI, PE>> &src,
	hls::stream<tensor<TO, PE>> &dst
#if THRESHOLDING_CFG
	, hls::stream<cfg_t> &cfg,
	hls::stream<TT> &rsp
#endif
) {
#pragma HLS interface axis port=src register_mode=off
#pragma HLS interface axis port=dst
#if THRESHOLDING_CFG
# pragma HLS interface axis port=cfg register_mode=off
# pragma HLS aggregate variable=cfg compact=bit
# pragma HLS interface axis port=rsp
#endif
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS dataflow disable_start_propagation
#if THRESHOLDING_CFG
	thresholding<N, C, TT, thresh_init, BIAS, DEPTH_TRIGGER_BRAM, DEPTH_TRIGGER_URAM>(src, dst, cfg, rsp);
#else
	thresholding<N, C, TT, thresh_init, BIAS, DEPTH_TRIGGER_BRAM, DEPTH_TRIGGER_URAM>(src, dst);
#endif
}
