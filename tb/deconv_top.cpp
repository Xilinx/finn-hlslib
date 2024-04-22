#include "deconv_top.hpp"
#include "deconv.hpp"


void deconv_top(
	hls::stream<hls::vector<TI, SIMD>> &src,
	hls::stream<hls::vector<TO, PE>>   &dst
) {
#pragma HLS interface AXIS port=src
#pragma HLS interface AXIS port=dst
#pragma HLS interface ap_ctrl_none port=return

#pragma HLS dataflow disable_start_propagation

	deconv<K, S, P, H, W, CO, CI, PE, SIMD>(KERNEL, src, dst);

} // deconv_top()
