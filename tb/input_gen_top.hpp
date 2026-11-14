/****************************************************************************
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @brief	Unit test top for input_gen<> driven by a pooling loop nest.
 *
 * The geometry deliberately skips input: every other row (stride 2 over a
 * single-row kernel) and every third column (stride 3 over a two-column
 * kernel). It further has OH*SH > H, i.e. the last stride period extends
 * past the input although no read does.
 ****************************************************************************/
#ifndef INPUT_GEN_TOP_HPP
#define INPUT_GEN_TOP_HPP

#include <hls_stream.h>
#include <ap_int.h>
#include "utils.hpp"

namespace top {
	constexpr unsigned  H  = 9;
	constexpr unsigned  W  = 9;
	constexpr unsigned  CF = 2;
	constexpr unsigned  KH = 1;
	constexpr unsigned  KW = 2;
	constexpr unsigned  SH = 2;
	constexpr unsigned  SW = 3;
}

using  val_t = ap_uint<16>;

void input_gen_top(
	hls::stream<val_t> &src,
	hls::stream<flit_t<val_t>> &dst
);

#endif
