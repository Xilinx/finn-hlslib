//
// Created by erling on 5/10/21.
//
#include <hls_stream.h>
using namespace hls;

#include "ap_int.h"
#include "../bnn-library.h"

#include "upsample_config.h"



void Testbench_upsample(stream<ap_uint<PRECISION * FM_CHANNELS>> &in, stream<ap_uint<PRECISION * FM_CHANNELS>> &out) {
  UpsampleNearest<OFMDIM, IFMDIM, FM_CHANNELS, PRECISION>(in,out);
}
