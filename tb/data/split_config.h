#include <ap_int.h>

#define NUM_OUTPUTS 3
#define NUM_FOLDS0 1
#define NUM_FOLDS1 1
#define NUM_FOLDS2 3
#define FOLDS_PER_OUTPUT {NUM_FOLDS0, NUM_FOLDS1, NUM_FOLDS2}
#define REP_COUNT 3

constexpr unsigned SIMD = 2;

using EL_TYPE = ap_uint<3>;
using IN_TYPE = hls::vector<EL_TYPE, SIMD>;
