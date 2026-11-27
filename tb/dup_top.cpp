/****************************************************************************
 * Copyright (C) 2024 - 2026 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#include "dup_top.hpp"
#include "dup.hpp"


void dup_top(
	hls::stream<T>  &src,
	hls::stream<T> (&dst)[N]
) {
#pragma HLS interface AXIS port=src
#pragma HLS interface AXIS port=dst
#pragma HLS aggregate compact=bit variable=src
#pragma HLS aggregate compact=bit variable=dst

#pragma HLS interface ap_ctrl_none port=return
#pragma HLS dataflow disable_start_propagation

	StreamingDup(src, dst);

} // dup()
