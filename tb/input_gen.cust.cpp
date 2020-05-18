
#include <hls_stream.h>
using namespace hls;
#include "ap_int.h"
#include "bnn-library.h"
#include "input_gen.h"

void Testbench(stream<ap_uint<IFM_Channels * INPUT_PRECISION>> &in, stream<ap_uint<IFM_Channels * INPUT_PRECISION>> &out, unsigned int numReps)
{
#pragma HLS DATAFLOW
    stream<ap_uint<SIMD * INPUT_PRECISION>> in_simd("in_simd");
    stream<ap_uint<SIMD * INPUT_PRECISION>> out_simd("out_simd");
    StreamingDataWidthConverter_Batch<IFM_Channels * INPUT_PRECISION, SIMD * INPUT_PRECISION, IFMDimH * IFMDimW>(in, in_simd, numReps);

    ConvolutionInputGenerator_rect<KERNEL_DIMH,
                                   KERNEL_DIMW,
                                   IFM_Channels,
                                   INPUT_PRECISION,
                                   IFMDimH,
                                   IFMDimW,
                                   OFMDimH,
                                   OFMDimW,
                                   SIMD,
                                   STRIDE>(in_simd, out_simd, numReps, ap_resource_dflt());

    StreamingDataWidthConverter_Batch<SIMD * INPUT_PRECISION, IFM_Channels * INPUT_PRECISION, KERNEL_DIMH * KERNEL_DIMW * OFMDimH * OFMDimW * IFM_Channels / SIMD>(out_simd, out, numReps);
}