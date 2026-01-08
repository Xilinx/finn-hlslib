/****************************************************************************
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Shane T. Fleming <shane.fleming@amd.com>
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#include "layernorm_top.hpp"
#include "layernorm.hpp"


void layernorm_top(
	hls::stream<hls::vector<TI, SIMD>> &src,
	hls::stream<hls::vector<TO, SIMD>> &dst
) {
#pragma HLS interface AXIS port=src
#pragma HLS interface AXIS port=dst
#pragma HLS interface ap_ctrl_none port=return

#pragma HLS dataflow disable_start_propagation
	layernorm<N>(src, dst);
}
