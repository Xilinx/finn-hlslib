#include "bnn-library.h"
#include "weights.hpp"
#include "config.h"
#include "memdata.h"

#define TILES TILE1
#define SIMD SIMD1
#define PE PE1
#define WP WIDTH

typedef hls::stream<ap_uint<SIMD * PE * WP>> paramS;

void GenWeightStream(paramS &paramStreamOut, int const numReps) {
#pragma HLS INTERFACE s_axilite port=return bundle=control
#pragma HLS INTERFACE s_axilite port=numReps bundle=control
#pragma HLS INTERFACE axis port=&paramStreamOut bundle=hostmem

  GenParamStream<TILE1, SIMD1, PE1, WIDTH>(PARAM::weights, paramStreamOut, numReps);
}
