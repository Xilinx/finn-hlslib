#ifndef CHECKSUM_TOP_HPP
#define CHECKSUM_TOP_HPP

#include <hls_stream.h>
#include <ap_int.h>

constexpr unsigned  WORDS_PER_FRAME = 60;
constexpr unsigned  ITEMS_PER_WORD  = 4;
constexpr unsigned  ITEM_WIDTH      = 8;
constexpr unsigned  NUM_FRAMES      = 311;

using T = ap_uint<ITEMS_PER_WORD * ITEM_WIDTH>;

void Testbench_Checksum(
	hls::stream<T> &src,
	hls::stream<T> &dst,
	ap_uint<32>    &chk,
	ap_uint<1>     drain
);

#endif
