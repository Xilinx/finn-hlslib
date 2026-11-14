#ifndef LABEL_SELECT_TOP_H
#define LABEL_SELECT_TOP_H

#include <hls_stream.h>
#include "ap_int.h"
#include "tensor.hpp"
#include "topk.hpp"

constexpr unsigned  N  = 10;
constexpr unsigned  K  = 3;
constexpr unsigned  PE = 5;

using TI = ap_int<16>;
using TO = ap_uint<4>;

using src_t = tensor<TI, PE>;
using dst_t = tensor<TO,  1>;

void Testbench_label_select(
	hls::stream<src_t> &src,
	hls::stream<dst_t> &dst
);

#endif
