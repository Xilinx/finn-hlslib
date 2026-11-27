/****************************************************************************
 * Copyright (C) 2024 - 2026, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#ifndef DUP_TOP_HPP
#define DUP_TOP_HPP

#include <hls_stream.h>
#include <cstdint>


using  T = uint32_t;
constexpr unsigned  N = 5;

void dup_top(
	hls::stream<T>  &src,
	hls::stream<T> (&dst)[N]
);
#endif
