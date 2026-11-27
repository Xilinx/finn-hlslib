#ifndef BIPOLAR_TOP_HPP
#define BIPOLAR_TOP_HPP

#include "tensor.hpp"
#include <hls_stream.h>

void bipolar_top(
	hls::stream<ap_uint<4>> &in,
	hls::stream<ap_uint<4>> &out
);

#endif
