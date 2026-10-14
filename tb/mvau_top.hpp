/****************************************************************************
 * Copyright (C) 2024, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Thomas B. Preusser <thomas.preusser@amd.com>
 ****************************************************************************/
#ifndef MVAU_TOP_HPP
#define MVAU_TOP_HPP

#include <ap_int.h>
#include <hls_stream.h>

#include "tensor.hpp"

// Test configuration: MatrixW=8, MatrixH=4, SIMD=2, PE=2
// => NF=2, SF=4  (exercises folding in both dimensions)
constexpr unsigned  MW   = 8;
constexpr unsigned  MH   = 4;
constexpr unsigned  SIMD = 2;
constexpr unsigned  PE   = 2;
constexpr unsigned  NF   = MH / PE;   // 2
constexpr unsigned  SF   = MW / SIMD;  // 4
constexpr unsigned  MMV  = 1;

using TW = ap_int<4>;   // 4-bit signed weights
using TI = ap_uint<8>;  // 8-bit unsigned activations
using TO = ap_int<16>;  // 16-bit signed output (accumulator passthrough)

// Tensor-typed streams matching kernel interface
using act_t = tensor<TI, MMV, SIMD>;   // ap_uint<16>
using out_t = tensor<TO, MMV, PE>;     // ap_uint<32>
using wgt_t = tensor<TW, PE, SIMD>;    // ap_uint<16>

void mvau_top(
	hls::stream<act_t> &in,
	hls::stream<out_t> &out,
	hls::stream<wgt_t> &weights
);

#endif
