#include <hls_stream.h>
#include "ap_int.h"
#include "streamtools.hpp"


#include "data/add_config.h"

void Testbench_add(hls::stream<ap_uint<NUM_CHANNELS * INPUT_WIDTH>> &in1, hls::stream<ap_uint<NUM_CHANNELS * INPUT_WIDTH>> &in2, hls::stream<ap_uint<NUM_CHANNELS * OUTPUT_WIDTH>> &out, const unsigned int numReps){
	
	if (SAT == 0)
		AddStreams_Batch<NUM_CHANNELS, ap_uint<INPUT_WIDTH>, ap_uint<INPUT_WIDTH>, ap_ufixed<OUTPUT_WIDTH, OUTPUT_WIDTH, AP_TRN>, NUM_WORDS, OFFSET>(in1, in2, out, numReps);
	else
		AddStreams_Batch<NUM_CHANNELS, ap_uint<INPUT_WIDTH>, ap_uint<INPUT_WIDTH>, ap_ufixed<OUTPUT_WIDTH, OUTPUT_WIDTH, AP_RND, AP_SAT>, NUM_WORDS, OFFSET>(in1, in2, out, numReps);
}
