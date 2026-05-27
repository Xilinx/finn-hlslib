/****************************************************************************
 * Copyright (C) 2024, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/
#include <ap_fixed.h>
#include <ap_float.h>
#include "flatten.hpp"

// Minimal synthesizable wrapper exercising flatten/unflatten
void flatten_top(ap_uint<32> const &in, ap_uint<32> &out) {
#pragma HLS inline off
	float  buf[1];
	unflatten(buf, in);
	out = flatten(buf);
}
