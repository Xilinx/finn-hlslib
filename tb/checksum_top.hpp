/****************************************************************************
 * Copyright (C) Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @brief	Configurations of the checksum testbench.
 *
 * Testbench_Checksum is the synthesized top. Testbench_Checksum_Wide is
 * simulated only: its items are wider than the 23-bit slices the checksum
 * folds them into.
 ***************************************************************************/
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

// Items wider than a 23-bit slice, one per word
constexpr unsigned  WIDE_WORDS_PER_FRAME = 37;
constexpr unsigned  WIDE_ITEMS_PER_WORD  = 1;
constexpr unsigned  WIDE_ITEM_WIDTH      = 32;

using TW = ap_uint<WIDE_ITEMS_PER_WORD * WIDE_ITEM_WIDTH>;

void Testbench_Checksum_Wide(
	hls::stream<TW> &src,
	hls::stream<TW> &dst,
	ap_uint<32>     &chk,
	ap_uint<1>      drain
);

#endif
