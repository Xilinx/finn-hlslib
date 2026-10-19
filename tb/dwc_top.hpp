#ifndef DWC_TOP_HPP
#define DWC_TOP_HPP

#include <hls_stream.h>
#include <ap_int.h>

constexpr unsigned INPUT_WIDTH = 8;
constexpr unsigned OUT_WIDTH   = 4;
constexpr unsigned NumInWords  = 4;
constexpr unsigned NUM_REPEAT  = 1;

void Testbench_dwc(
	hls::stream<ap_uint<INPUT_WIDTH>> &in,
	hls::stream<ap_uint<OUT_WIDTH>>   &out
);

#endif
