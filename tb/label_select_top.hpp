#ifndef LABEL_SELECT_TOP_H
#define LABEL_SELECT_TOP_H

#include <hls_stream.h>
#include "ap_int.h"
#include "tensor.hpp"
#include "topk.hpp"

#define SIGNED_INPUT 1
#define NumClasses 10
#define OUT_WIDTH 4
#define NumTop 3

#define FM_Channels1 NumClasses
#define PE1 5
#define INPUT_PRECISION 16

#if SIGNED_INPUT
    #define In_T ap_int<INPUT_PRECISION>
#else
    #define In_T ap_uint<INPUT_PRECISION>
#endif

#define Out_T ap_uint<OUT_WIDTH>

using ls_in_t = tensor<In_T, PE1>;

void Testbench_label_select(
	hls::stream<ls_in_t> &in,
	hls::stream<Out_T>   &out
);

#endif
