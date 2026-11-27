#include "dwc_top.hpp"
#include "dwc.hpp"

void Testbench_dwc(
	hls::stream<tensor<T, N1>> &in,
	hls::stream<tensor<T, N1>> &out
) {
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS dataflow disable_start_propagation
	static hls::stream<tensor<T, N2>>  s1("s1");
	static hls::stream<tensor<T, N2>>  s2("s2");
	StreamingDWC<T, N1, N2>(in, s1);   // widen
	StreamingDWC<T, N2, N2>(s1, s2);   // passthru
	StreamingDWC<T, N2, N1>(s2, out);  // narrow
}
