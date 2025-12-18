/****************************************************************************
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Leah Sieder <leah-louisa.sieder@amd.com>
 ****************************************************************************/
#include "rmsnorm_top.hpp"
#include "rmsnorm.hpp"


void rmsnorm_top(
	hls::stream<hls::vector<TI, SIMD>> &src,
	hls::stream<hls::vector<TO, SIMD>> &dst
) {
#pragma HLS interface AXIS port=src
#pragma HLS interface AXIS port=dst
#pragma HLS interface ap_ctrl_none port=return

#pragma HLS dataflow disable_start_propagation
	rmsnorm<N>(src, dst);
}