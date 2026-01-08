/****************************************************************************
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Shane T. Fleming <shane.fleming@amd.com>
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#ifndef LAYERNORM_TOP_HPP
#define LAYERNORM_TOP_HPP

#include <hls_stream.h>
#include <hls_vector.h>
#include <ap_int.h>


constexpr unsigned  N    = 384;
constexpr unsigned  SIMD =   4;
using  TI = ap_int<5>;
using  TO = float;

void layernorm_top(
	hls::stream<hls::vector<TI, SIMD>> &src,
	hls::stream<hls::vector<TO, SIMD>> &dst
);

#endif
