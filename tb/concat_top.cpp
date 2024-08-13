#include <hls_stream.h>
#include <hls_vector.h>
// #include "ap_int.h"
#include "bnn-library.h"

#include "data/concat_config.h"
#include "concat.hpp"

void Testbench_concat(hls::stream<IN_TYPE0> &in0_V,
                      hls::stream<IN_TYPE1> &in1_V,
                      hls::stream<IN_TYPE2> &in2_V,
                      hls::stream<IN_TYPE3> &in3_V,
                      hls::stream<OUT_TYPE> &out_V)
{
#pragma HLS INTERFACE axis port=in0_V
#pragma HLS INTERFACE axis port=in1_V
#pragma HLS INTERFACE axis port=in2_V
#pragma HLS INTERFACE axis port=in3_V
#pragma HLS INTERFACE axis port=out_V
#pragma HLS INTERFACE ap_ctrl_none port=return
StreamingConcat<NUM_FOLDS0, NUM_FOLDS1, NUM_FOLDS2, NUM_FOLDS3>(out_V, in0_V, in1_V, in2_V, in3_V);
}
