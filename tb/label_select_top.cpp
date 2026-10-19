#include "label_select_top.h"

void Testbench_label_select(
	hls::stream<ls_in_t> &in,
	hls::stream<Out_T>   &out
) {
#pragma HLS interface ap_ctrl_none port=return
	LabelSelect_Batch<NumClasses, PE1, NumTop, In_T, Out_T>(in, out);
}
