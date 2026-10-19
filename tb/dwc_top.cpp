#include <hls_stream.h>
#include "ap_int.h"
#include "dwc.hpp"

#include "data/dwc_config.h"

void Testbench_dwc(hls::stream<ap_uint<INPUT_WIDTH>> &in, hls::stream<ap_uint<OUT_WIDTH>> &out) {
#pragma HLS interface ap_ctrl_none port=return
	StreamingDataWidthConverter<INPUT_WIDTH, OUT_WIDTH>(in, out);
}
