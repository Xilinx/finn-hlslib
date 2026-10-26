#include "dwc_top.hpp"
#include "dwc.hpp"

void Testbench_dwc(hls::stream<tensor<T, NI>> &in, hls::stream<tensor<T, NO>> &out) {
#pragma HLS interface ap_ctrl_none port=return
	StreamingDWC<T, NI, NO>(in, out);
}
