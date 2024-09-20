#include <ap_int.h>

constexpr unsigned NUM_INPUTS = 4;
constexpr unsigned NUM_FOLDS0 = 1;
constexpr unsigned NUM_FOLDS1 = 1;
constexpr unsigned NUM_FOLDS2 = 3;
constexpr unsigned NUM_FOLDS3 = 2;
constexpr unsigned TOTAL_FOLDS = 7;
constexpr unsigned REP_COUNT = 3;
constexpr unsigned SIMD = 2;

using EL_TYPE0 = ap_uint<3>;
using EL_TYPE1 = ap_uint<3>;
using EL_TYPE2 = ap_uint<3>;
using EL_TYPE3 = ap_uint<3>;
using EL_TYPE_OUT = ap_uint<3>;

using IN_TYPE0 = hls::vector<EL_TYPE0, SIMD>;
using IN_TYPE1 = hls::vector<EL_TYPE1, SIMD>;
using IN_TYPE2 = hls::vector<EL_TYPE2, SIMD>;
using IN_TYPE3 = hls::vector<EL_TYPE3, SIMD>;
using OUT_TYPE = hls::vector<EL_TYPE_OUT, SIMD>;
