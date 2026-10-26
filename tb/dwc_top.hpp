#ifndef DWC_TOP_HPP
#define DWC_TOP_HPP

#include <hls_stream.h>
#include "tensor.hpp"

using T = ap_uint<4>;
constexpr unsigned NI = 2;
constexpr unsigned NO = 1;
constexpr unsigned NumInWords = 4;

void Testbench_dwc(
	hls::stream<tensor<T, NI>> &in,
	hls::stream<tensor<T, NO>> &out
);

#endif
