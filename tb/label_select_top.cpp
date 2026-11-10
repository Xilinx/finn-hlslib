#include "label_select_top.hpp"

void Testbench_label_select(
	hls::stream<src_t> &src,
	hls::stream<dst_t> &dst
) {
#pragma HLS interface ap_ctrl_none port=return
	LabelSelect_Batch<N, K>(src, dst);
}
