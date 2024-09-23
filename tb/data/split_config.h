#include <ap_int.h>

constexpr unsigned NUM_OUTPUTS = 3;
constexpr unsigned NUM_FOLDS0 = 1;
constexpr unsigned NUM_FOLDS1 = 1;
constexpr unsigned NUM_FOLDS2 = 3;
constexpr unsigned REP_COUNT = 3;
constexpr unsigned SIMD = 2;
constexpr unsigned FOLDS_PER_OUTPUT[NUM_OUTPUTS] = {NUM_FOLDS0, NUM_FOLDS1, NUM_FOLDS2};

using EL_TYPE = ap_uint<3>;
using IN_TYPE = hls::vector<EL_TYPE, SIMD>;
