/****************************************************************************
 * Copyright (C) 2024, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#ifndef VVAU_TOP_HPP
#define VVAU_TOP_HPP

#include <ap_int.h>
#include <hls_stream.h>
#include <hls_vector.h>

// Test configuration: Channels=4, Kernel_2=6, SIMD=2, PE=2
// => NF=2, SF=3  (exercises folding in both dimensions)
constexpr unsigned  CHANNELS = 4;
constexpr unsigned  KERNEL2  = 6;
constexpr unsigned  SIMD     = 2;
constexpr unsigned  PE       = 2;
constexpr unsigned  NF       = CHANNELS / PE;   // 2
constexpr unsigned  SF       = KERNEL2 / SIMD;  // 3

using TW = ap_int<4>;   // 4-bit signed weights
using TI = ap_uint<8>;  // 8-bit unsigned activations
using TO = ap_int<16>;  // 16-bit signed output

using wgt_t = hls::vector<hls::vector<TW, SIMD>, PE>;
using act_t = hls::vector<hls::vector<TI, SIMD>, PE>;
using out_t = hls::vector<TO, PE>;

void vvau_top(
	hls::stream<act_t> &in,
	hls::stream<out_t> &out,
	hls::stream<wgt_t> &weights
);

#endif
