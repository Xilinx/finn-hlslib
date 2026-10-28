/****************************************************************************
 * Copyright Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: MIT
 *
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#ifndef PAD_TOP_HPP
#define PAD_TOP_HPP

#include <hls_stream.h>
#include <ap_int.h>

#include "tensor.hpp"

constexpr unsigned  H = 5;   // Input height
constexpr unsigned  W = 7;   // Input width
constexpr unsigned  C = 8;   // Channels
constexpr unsigned  PE = 4;  // Parallelism

constexpr unsigned  PAD_N = 1;
constexpr unsigned  PAD_E = 2;
constexpr unsigned  PAD_S = 1;
constexpr unsigned  PAD_W = 0;

static_assert(C % PE == 0, "PE must divide channel count.");

using  val_t = ap_int<8>;
using  vec_t = tensor<val_t, PE>;

constexpr unsigned  CF = C / PE;
constexpr unsigned  HO = PAD_N + H + PAD_S;
constexpr unsigned  WO = PAD_W + W + PAD_E;

void pad_top(
	hls::stream<vec_t> &src,
	hls::stream<vec_t> &dst
);

#endif
