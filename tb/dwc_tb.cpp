#include <iostream>
#include <hls_stream.h>
#define AP_INT_MAX_W 8191
#include "ap_int.h"

#include "data/dwc_config.h"

void Testbench_dwc(hls::stream<ap_uint<INPUT_WIDTH>> &in, hls::stream<ap_uint<OUT_WIDTH>> &out);

int main() {
	hls::stream<ap_uint<INPUT_WIDTH>>  input_stream("input_stream");
	hls::stream<ap_uint<OUT_WIDTH>>    output_stream("output_stream");

	constexpr unsigned  NUM_OUT = NumInWords * INPUT_WIDTH / OUT_WIDTH;
	ap_uint<OUT_WIDTH>  expected[NUM_OUT];

	// Generate input and expected output
	unsigned  count_out = 0;
	unsigned  count_in = 0;
	for(unsigned  i = 0; i < NumInWords; i++) {
		ap_uint<INPUT_WIDTH>  value = (ap_uint<INPUT_WIDTH>)i;
		input_stream.write(value);

		if constexpr(INPUT_WIDTH < OUT_WIDTH) {
			ap_uint<OUT_WIDTH>  val_out = expected[count_out];
			val_out >>= INPUT_WIDTH;
			val_out(OUT_WIDTH-1, OUT_WIDTH-INPUT_WIDTH) = value;
			expected[count_out] = val_out;
			if(++count_in == OUT_WIDTH/INPUT_WIDTH) {
				count_out++;
				count_in = 0;
			}
		}
		else if constexpr(INPUT_WIDTH == OUT_WIDTH) {
			expected[i] = value;
		}
		else {
			ap_uint<INPUT_WIDTH>  v = value;
			for(unsigned  w = 0; w < INPUT_WIDTH/OUT_WIDTH; w++) {
				expected[count_out++] = v(OUT_WIDTH-1, 0);
				v >>= OUT_WIDTH;
			}
		}
	}

	// Call function-pipelined kernel
	// For downscaling: NumInWords input reads + NumInWords*(InWidth/OutWidth-1) output-only iterations
	// For upscaling: NumInWords input reads (some produce output)
	// Overestimate with 2*max(in, out) calls
	constexpr unsigned  ITERS = 2 * (NumInWords > NUM_OUT ? NumInWords : NUM_OUT);
	for(unsigned  i = 0; i < ITERS; i++)  Testbench_dwc(input_stream, output_stream);

	// Verify
	for(unsigned  i = 0; i < NUM_OUT; i++) {
		ap_uint<OUT_WIDTH>  value = output_stream.read();
		if(value != expected[i]) {
			std::cout << "ERROR counter " << i << std::hex
				<< " expected " << expected[i] << " value " << value
				<< std::dec << std::endl;
			return  1;
		}
	}
	std::cout << "Test passed." << std::endl;
	return  0;
}
