#ifndef SOFTMAX_TOP_HPP
#define SOFTMAX_TOP_HPP

#include <hls_stream.h>
#include "ap_int.h"
#include "softmax.hpp"

#define N_CLASSES 8
#define SIMD_SM   4

using  sm_in_t  = tensor<ap_int<8>, SIMD_SM>;
using  sm_out_t = tensor<float, SIMD_SM>;

void Testbench_softmax(
	hls::stream<sm_in_t>  &in,
	hls::stream<sm_out_t> &out
);

#endif
