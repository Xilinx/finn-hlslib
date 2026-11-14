#include "bipolar_top.hpp"

void bipolar_top(
	hls::stream<ap_uint<4>> &in,
	hls::stream<ap_uint<4>> &out
) {
#pragma HLS interface ap_ctrl_chain port=return
#pragma HLS interface axis port=in
#pragma HLS interface axis port=out

	// Read 4 Bipolar values packed as tensor
	tensor<Bipolar, 4> t;
	t.data() = in.read();

	// Extract elements
	Bipolar b0 = t[0], b1 = t[1], b2 = t[2], b3 = t[3];

	// XNOR popcount of adjacent pairs: (b0*b1) + (b2*b3)
	unsigned xnor_pop = (b0 * b1) + (b2 * b3);

	// Pack result: low 2 bits = xnor_pop, bit 2 = b0 raw, bit 3 = b3 raw
	ap_uint<4> res;
	res(1, 0) = xnor_pop;
	res[2] = ap_uint<1>(b0);
	res[3] = ap_uint<1>(b3);
	out.write(res);
}
