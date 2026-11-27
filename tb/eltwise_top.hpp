#ifndef ELTWISE_TOP_HPP
#define ELTWISE_TOP_HPP

#include <hls_stream.h>
#include <ap_int.h>

#include "tensor.hpp"

constexpr unsigned  NUM_CHANNELS  = 1;
constexpr unsigned  INPUT_1_WIDTH = 4;
constexpr unsigned  INPUT_2_WIDTH = 4;
constexpr unsigned  OUTPUT_WIDTH  = 5;
constexpr unsigned  NUM_WORDS     = 4;
constexpr unsigned  NUM_REPEAT    = 1;

using in0_t = tensor<ap_int<INPUT_1_WIDTH>, NUM_CHANNELS>;
using in1_t = tensor<ap_int<INPUT_2_WIDTH>, NUM_CHANNELS>;
using out_t = tensor<ap_int<OUTPUT_WIDTH>,  NUM_CHANNELS>;

void Testbench_Eltwise(
	hls::stream<in0_t> &in0,
	hls::stream<in1_t> &in1,
	hls::stream<out_t> &out
);

#endif
