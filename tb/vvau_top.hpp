/****************************************************************************
 * Copyright (C) 2024, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Thomas B. Preusser <thomas.preusser@amd.com>
 ****************************************************************************/
#ifndef VVAU_TOP_HPP
#define VVAU_TOP_HPP

#include <ap_int.h>
#include <hls_stream.h>

#include "tensor.hpp"

// Test configuration: Channels=4, Kernel_2=6, SIMD=2, PE=2
// => NF=2, SF=3  (exercises folding in both dimensions)
constexpr unsigned  CHANNELS = 4;
constexpr unsigned  KERNEL2  = 6;
constexpr unsigned  SIMD     = 2;
constexpr unsigned  PE       = 2;
constexpr unsigned  NF       = CHANNELS / PE;   // 2
constexpr unsigned  SF       = KERNEL2 / SIMD;  // 3
constexpr unsigned  MMV      = 1;

using TW = ap_int<4>;   // 4-bit signed weights
using TI = ap_uint<8>;  // 8-bit unsigned activations
using TO = ap_int<16>;  // 16-bit signed output

// Tensor-typed streams matching kernel interface
// VVAU input: each PE has its own MMV*SIMD activation slice
using act_t = tensor<TI, PE, MMV, SIMD>;   // ap_uint<32>
using out_t = tensor<TO, MMV, PE>;          // ap_uint<32>
using wgt_t = tensor<TW, PE, SIMD>;         // ap_uint<16>

void vvau_top(
	hls::stream<act_t> &in,
	hls::stream<out_t> &out,
	hls::stream<wgt_t> &weights
);

#endif
