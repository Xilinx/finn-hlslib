/****************************************************************************
 * Copyright (C) Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: BSD-3-Clause
 ***************************************************************************/
#include "checksum_top.hpp"
#include "checksum.hpp"

void Testbench_Checksum(
	hls::stream<T> &src,
	hls::stream<T> &dst,
	ap_uint<32>    &chk,
	ap_uint<1>     drain
) {
#pragma HLS interface ap_ctrl_none port=return
	checksum<WORDS_PER_FRAME, ITEMS_PER_WORD>(src, dst, chk, drain);
}

void Testbench_Checksum_Wide(
	hls::stream<TW> &src,
	hls::stream<TW> &dst,
	ap_uint<32>     &chk,
	ap_uint<1>      drain
) {
#pragma HLS interface ap_ctrl_none port=return
	checksum<WIDE_WORDS_PER_FRAME, WIDE_ITEMS_PER_WORD>(src, dst, chk, drain);
}
