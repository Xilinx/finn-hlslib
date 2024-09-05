#include <hls_stream.h>
#include <hls_vector.h>

#include "data/split_config.h"
#include "split.hpp"

void Testbench_split(hls::stream<IN_TYPE> &in0_V, hls::stream<IN_TYPE> (&out_arr)[NUM_OUTPUTS])
{
#pragma HLS INTERFACE axis port=in0_V
#pragma HLS INTERFACE axis port=out_arr[0]
#pragma HLS INTERFACE axis port=out_arr[1]
#pragma HLS INTERFACE axis port=out_arr[2]
#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS aggregate variable=in0_V compact=bit
#pragma HLS aggregate variable=out_arr[0] compact=bit
#pragma HLS aggregate variable=out_arr[1] compact=bit
#pragma HLS aggregate variable=out_arr[2] compact=bit

StreamingSplit<NUM_FOLDS0, NUM_FOLDS1, NUM_FOLDS2>(in0_V, out_arr);
}
