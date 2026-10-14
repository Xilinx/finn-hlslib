#include <ap_int.h>
#include "tensor.hpp"

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

using IN_TYPE0 = tensor<EL_TYPE0, SIMD>;
using IN_TYPE1 = tensor<EL_TYPE1, SIMD>;
using IN_TYPE2 = tensor<EL_TYPE2, SIMD>;
using IN_TYPE3 = tensor<EL_TYPE3, SIMD>;
using OUT_TYPE = tensor<EL_TYPE_OUT, SIMD>;
