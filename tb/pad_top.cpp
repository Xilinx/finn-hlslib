/****************************************************************************
 * Copyright Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: MIT
 *
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#include "pad_top.hpp"
#include "pad.hpp"

void pad_top(
	hls::stream<vec_t> &src,
	hls::stream<vec_t> &dst
) {
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS dataflow disable_start_propagation
	pad<H, W, CF, PAD_N, PAD_E, PAD_S, PAD_W>(src, dst);
}
