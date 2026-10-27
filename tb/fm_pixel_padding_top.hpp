#ifndef FM_PIXEL_PADDING_TOP_HPP
#define FM_PIXEL_PADDING_TOP_HPP

#include <hls_stream.h>
#include <ap_int.h>

#include "tensor.hpp"

constexpr unsigned  SIMD1 = 1;
constexpr unsigned  INPUT_WIDTH = 8;
constexpr unsigned  INPUT_DIM_X = 6;
constexpr unsigned  INPUT_DIM_Y = 5;
constexpr unsigned  CHANNELS = 3;
constexpr unsigned  XSTRIDE = 5;
constexpr unsigned  YSTRIDE = 3;

constexpr unsigned  OUTPUT_DIM_X = INPUT_DIM_X + (INPUT_DIM_X - 1) * (XSTRIDE - 1);
constexpr unsigned  OUTPUT_DIM_Y = INPUT_DIM_Y + (INPUT_DIM_Y - 1) * (YSTRIDE - 1);

using  vec_t = tensor<ap_uint<INPUT_WIDTH>, SIMD1>;

void test_fm_pixel_padding(
	hls::stream<vec_t> &src,
	hls::stream<vec_t> &dst
);

#endif
