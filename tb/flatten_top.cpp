/****************************************************************************
 * Copyright (C) 2024, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/
#include <ap_fixed.h>
#include <ap_float.h>
#include "tensor.hpp"

// Minimal synthesizable wrapper exercising tensor construction/conversion
void flatten_top(ap_uint<32> const &in, ap_uint<32> &out) {
#pragma HLS inline off
	tensor<float, 1>  t(in);
	out = ap_uint<32>(t);
}
