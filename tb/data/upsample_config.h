#ifndef UPSAMPLE_CONFIG_H
#define UPSAMPLE_CONFIG_H

#include <ap_int.h>
#include <hls_vector.h>

constexpr unsigned PRECISION = 8;
constexpr unsigned HI = 12;
constexpr unsigned WI = 20;
constexpr unsigned HO = 24;
constexpr unsigned WO = 40;

constexpr unsigned FM_CHANNELS = 3;
constexpr unsigned SIMD = 1;
constexpr unsigned CF = FM_CHANNELS / SIMD;

using EL_TYPE = ap_uint<PRECISION>;
using VEC_TYPE = hls::vector<EL_TYPE, SIMD>;

#endif //FINN_HLSLIB_UPSAMPLE_CONFIG_H
