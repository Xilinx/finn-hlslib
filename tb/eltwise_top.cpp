#include <hls_stream.h>
using namespace hls;
#include "ap_int.h"
#include "eltwise.hpp"
#include "interpret.hpp"


#include "data/eltwise_config.h"

void Testbench_Eltwise(int mode, stream<ap_uint<NUM_CHANNELS * INPUT_1_WIDTH> > & in0, stream<ap_uint<NUM_CHANNELS * INPUT_2_WIDTH> > & in1, stream<ap_uint<NUM_CHANNELS * OUTPUT_WIDTH> > & out) 
{
    switch(mode) {
		case 0:
			AddEltwiseFunction<ap_int<INPUT_1_WIDTH>, ap_int<INPUT_2_WIDTH>, ap_int<OUTPUT_WIDTH>> add_eltwise_fxn;
            StreamingEltwise<NUM_CHANNELS, NUM_CHANNELS, NUM_WORDS, Slice<ap_int<INPUT_1_WIDTH>>, Slice<ap_int<INPUT_2_WIDTH>>, Slice<ap_int<OUTPUT_WIDTH>> >(in0, in1, out, add_eltwise_fxn);
			break;
		case 1:
			SubEltwiseFunction<ap_int<INPUT_1_WIDTH>, ap_int<INPUT_2_WIDTH>, ap_int<OUTPUT_WIDTH>> sub_eltwise_fxn;
            StreamingEltwise<NUM_CHANNELS, NUM_CHANNELS, NUM_WORDS, Slice<ap_int<INPUT_1_WIDTH>>, Slice<ap_int<INPUT_2_WIDTH>>, Slice<ap_int<OUTPUT_WIDTH>> >(in0, in1, out, sub_eltwise_fxn);
			break;
		case 2:
			AbsDiffEltwiseFunction<ap_int<INPUT_1_WIDTH>, ap_int<INPUT_2_WIDTH>, ap_int<OUTPUT_WIDTH>> absdiff_eltwise_fxn;
            StreamingEltwise<NUM_CHANNELS, NUM_CHANNELS, NUM_WORDS, Slice<ap_int<INPUT_1_WIDTH>>, Slice<ap_int<INPUT_2_WIDTH>>, Slice<ap_int<OUTPUT_WIDTH>> >(in0, in1, out, absdiff_eltwise_fxn);
		default:
            break;	
	}
}
