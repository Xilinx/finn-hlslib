#include "topk_top.hpp"

void Testbench_topk(
	hls::stream<src_t> &src,
	hls::stream<dst_t> &dst
) {
#pragma HLS interface ap_ctrl_none port=return
	topk<N, K>(src, dst);
}
