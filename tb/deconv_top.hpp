#ifndef DECONV_TOP_HPP
#define DECONV_TOP_HPP

#include <ap_int.h>
#include <hls_stream.h>
#include <hls_vector.h>

constexpr unsigned  K = 4;		// kernel Size
constexpr unsigned  S = 2; 		// stride
constexpr unsigned  P = K-S;	// (de)padding
constexpr unsigned  H = 6;		// IFM height
constexpr unsigned  W = 6;		// IFM Width
constexpr unsigned  CI = 1;		// input channels
constexpr unsigned  CO = 2;		// output channels

using  TW = ap_uint< 8>;
using  TI = ap_uint< 4>;
using  TO = ap_uint<16>;

#if 1
constexpr unsigned  PE   = 1;
constexpr unsigned  SIMD = 1;

static TW const  KERNEL[(CO/PE)*K*K*(CI/SIMD)][PE][SIMD] = {
	{{0x00,},}, {{0x01,},}, {{0x02,},}, {{0x03,},},
	{{0x10,},}, {{0x11,},}, {{0x12,},}, {{0x13,},},
	{{0x20,},}, {{0x21,},}, {{0x22,},}, {{0x23,},},
	{{0x30,},}, {{0x31,},}, {{0x32,},}, {{0x33,},},

	{{0x40,},}, {{0x41,},}, {{0x42,},}, {{0x43,},},
	{{0x50,},}, {{0x51,},}, {{0x52,},}, {{0x53,},},
	{{0x60,},}, {{0x61,},}, {{0x62,},}, {{0x63,},},
	{{0x70,},}, {{0x71,},}, {{0x72,},}, {{0x73,},},
};

#else
constexpr unsigned  PE   = 2;
constexpr unsigned  SIMD = 1;

static TW const  KERNEL[(CO/PE)*K*K*(CI/SIMD)][PE][SIMD] = {
	{{0x00,},{0x40,},}, {{0x01,},{0x41,},}, {{0x02,},{0x42,},}, {{0x03,},{0x43,},},
	{{0x10,},{0x50,},}, {{0x11,},{0x51,},}, {{0x12,},{0x52,},}, {{0x13,},{0x53,},},
	{{0x20,},{0x60,},}, {{0x21,},{0x61,},}, {{0x22,},{0x62,},}, {{0x23,},{0x63,},},
	{{0x30,},{0x70,},}, {{0x31,},{0x71,},}, {{0x32,},{0x72,},}, {{0x33,},{0x73,},},
};
#endif

void deconv_top(
	hls::stream<hls::vector<TI, SIMD>> &src,
	hls::stream<hls::vector<TO, PE>>   &dst
);

#endif
