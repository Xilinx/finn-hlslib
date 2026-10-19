#include <ap_int.h>
#include "tensor.hpp"

constexpr unsigned  NumChannels = 8;
constexpr unsigned  PE          = 2;
constexpr unsigned  NF          = NumChannels / PE;
constexpr int       MIN_VAL     = -128;
constexpr int       MAX_VAL     =  127;

using TI = ap_int<16>;
using TO = ap_int<8>;

constexpr unsigned  NUM_WORDS  = NF * 3;  // 3 full channel sweeps
