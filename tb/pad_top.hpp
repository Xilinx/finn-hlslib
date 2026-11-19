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

// Testbench Configuration Selection, set by pad.tcl
//	PAD_CFG		PAD Geometry, see below.
//	PAD_EAGER	1: Eager, 0: Lazy
#if !defined(PAD_CFG) || !defined(PAD_EAGER)
# error "PAD_CFG and PAD_EAGER must be defined, see pad.tcl."
#endif

constexpr unsigned  C  = 8;  // Channels
constexpr unsigned  PE = 4;  // Parallelism

#if PAD_CFG == 0
// 2D, asymmetric padding on all but the west edge
constexpr unsigned  H = 5;   // Input height
constexpr unsigned  W = 7;   // Input width
constexpr unsigned  PAD_N = 1;
constexpr unsigned  PAD_E = 2;
constexpr unsigned  PAD_S = 1;
constexpr unsigned  PAD_W = 0;
constexpr unsigned  SY = 1;
constexpr unsigned  SX = 1;
#elif PAD_CFG == 1
// 2D, no leading padding
constexpr unsigned  H = 5;
constexpr unsigned  W = 7;
constexpr unsigned  PAD_N = 0;
constexpr unsigned  PAD_E = 2;
constexpr unsigned  PAD_S = 1;
constexpr unsigned  PAD_W = 0;
constexpr unsigned  SY = 1;
constexpr unsigned  SX = 1;
#elif PAD_CFG == 2
// 2D, interior padding (pixel spacing of 2x3) in addition to edge padding
constexpr unsigned  H = 4;
constexpr unsigned  W = 3;
constexpr unsigned  PAD_N = 1;
constexpr unsigned  PAD_E = 1;
constexpr unsigned  PAD_S = 2;
constexpr unsigned  PAD_W = 2;
constexpr unsigned  SY = 2;
constexpr unsigned  SX = 3;
#elif PAD_CFG == 3
// 1D sequence
constexpr unsigned  H = 1;
constexpr unsigned  W = 6;
constexpr unsigned  PAD_N = 0;
constexpr unsigned  PAD_E = 1;
constexpr unsigned  PAD_S = 0;
constexpr unsigned  PAD_W = 2;
constexpr unsigned  SY = 1;
constexpr unsigned  SX = 1;
#elif PAD_CFG == 4
// Pure interior padding
constexpr unsigned  H = 3;
constexpr unsigned  W = 4;
constexpr unsigned  PAD_N = 0;
constexpr unsigned  PAD_E = 0;
constexpr unsigned  PAD_S = 0;
constexpr unsigned  PAD_W = 0;
constexpr unsigned  SY = 2;
constexpr unsigned  SX = 2;
#endif

constexpr bool  EAGER = PAD_EAGER != 0;

static_assert(C % PE == 0, "PE must divide channel count.");

using  val_t = ap_int<8>;
using  vec_t = tensor<val_t, PE>;

constexpr unsigned  CF = C / PE;
constexpr unsigned  HI = (H-1)*SY + 1;
constexpr unsigned  WI = (W-1)*SX + 1;
constexpr unsigned  HO = PAD_N + HI + PAD_S;
constexpr unsigned  WO = PAD_W + WI + PAD_E;

void pad_top(
	hls::stream<vec_t> &src,
	hls::stream<vec_t> &dst
);

#endif
