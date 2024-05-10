#include <hls_stream.h>
#include "bnn-library.h"
#include "data/dup_stream_config.h"

void Testbench_dup_stream(hls::stream<T> & in, hls::stream<T> & out1, hls::stream<T> & out2, unsigned int numReps){
#pragma HLS interface AXIS port=in
#pragma HLS interface AXIS port=out1
#pragma HLS interface AXIS port=out2
#pragma HLS aggregate variable=in compact=bit
#pragma HLS aggregate variable=out1 compact=bit
#pragma HLS aggregate variable=out2 compact=bit
#pragma HLS dataflow disable_start_propagation
	DuplicateStreams_Batch<T, NUM_REPEAT>(in, out1, out2, numReps);
}
