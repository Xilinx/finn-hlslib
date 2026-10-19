/****************************************************************************
 * Copyright (C) 2024, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Thomas B. Preusser <thomas.preusser@amd.com>
 ****************************************************************************/
#include "mvau_top.hpp"
#include "mvau.hpp"
#include "activations.hpp"

void mvau_top(
	hls::stream<act_t> &in,
	hls::stream<out_t> &out,
	hls::stream<wgt_t> &weights
) {
#pragma HLS interface axis port=in
#pragma HLS interface axis port=out
#pragma HLS interface axis port=weights
#pragma HLS aggregate compact=bit variable=in
#pragma HLS aggregate compact=bit variable=out
#pragma HLS aggregate compact=bit variable=weights
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS dataflow disable_start_propagation

	Matrix_Vector_Activate_Stream_Batch<MW, MH, SIMD, PE, MMV, TW, TI, TO>(
		in, out, weights, PassThroughActivation<TO>(), ap_resource_dflt()
	);
}
