#include "deconv_top.hpp"
#include "deconv.hpp"


static hls::vector<hls::vector<TW, SIMD>, PE> const  KERNEL[(CO/PE)*K*K*(CI/SIMD)] = {
	{{0x00,},}, {{0x01,},}, {{0x02,},}, {{0x03,},},
	{{0x10,},}, {{0x11,},}, {{0x12,},}, {{0x13,},},
	{{0x20,},}, {{0x21,},}, {{0x22,},}, {{0x23,},},
	{{0x30,},}, {{0x31,},}, {{0x32,},}, {{0x33,},},

	{{0x40,},}, {{0x41,},}, {{0x42,},}, {{0x43,},},
	{{0x50,},}, {{0x51,},}, {{0x52,},}, {{0x53,},},
	{{0x60,},}, {{0x61,},}, {{0x62,},}, {{0x63,},},
	{{0x70,},}, {{0x71,},}, {{0x72,},}, {{0x73,},},

//	{{0x80,},}, {{0x81,},}, {{0x82,},}, {{0x83,},},
//	{{0x90,},}, {{0x91,},}, {{0x92,},}, {{0x93,},},
//	{{0xA0,},}, {{0xA1,},}, {{0xA2,},}, {{0xA3,},},
//	{{0xB0,},}, {{0xB1,},}, {{0xB2,},}, {{0xB3,},},
};

void deconv_top(
	hls::stream<hls::vector<TI, SIMD>> &src,
	hls::stream<hls::vector<TO, PE>>   &dst
) {
#pragma HLS interface AXIS port=src
#pragma HLS interface AXIS port=dst
#pragma HLS interface ap_ctrl_none port=return

#pragma HLS dataflow disable_start_propagation
	deconv<K, S, H, W, CO, CI, PE, SIMD>(KERNEL, src, dst);

} // deconv_top()
