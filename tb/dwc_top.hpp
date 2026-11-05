#ifndef DWC_TOP_HPP
#define DWC_TOP_HPP

#include <hls_stream.h>
#include "tensor.hpp"

using T = ap_uint<4>;
constexpr unsigned  N1 = 1;  // narrow width
constexpr unsigned  N2 = 2;  // wide width
constexpr unsigned  NumInWords = 8;

void Testbench_dwc(
	hls::stream<tensor<T, N1>> &in,
	hls::stream<tensor<T, N1>> &out
);

#endif
