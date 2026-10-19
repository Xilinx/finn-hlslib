#include "maxpool_top.hpp"


void maxpool_top(
	hls::stream<T> &src,
	hls::stream<T> &dst
) {
#pragma HLS interface AXIS port=src
#pragma HLS interface AXIS port=dst
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS aggregate variable=src compact=bit
#pragma HLS aggregate variable=dst compact=bit

#pragma HLS dataflow disable_start_propagation
	MaxPool<T, K, CF>::process(src, dst);
}
