constexpr unsigned  SIMD1 = 1;
constexpr unsigned  INPUT_WIDTH = 8;
constexpr unsigned  INPUT_DIM_X = 30;
constexpr unsigned  INPUT_DIM_Y = 40;
constexpr unsigned  CHANNELS = 3;
constexpr unsigned  XSTRIDE = 5;
constexpr unsigned  YSTRIDE = 3;

constexpr unsigned  OUTPUT_DIM_X = INPUT_DIM_X + (INPUT_DIM_X - 1) * (XSTRIDE - 1);
constexpr unsigned  OUTPUT_DIM_Y = INPUT_DIM_Y + (INPUT_DIM_Y - 1) * (YSTRIDE - 1);