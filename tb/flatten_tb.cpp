/****************************************************************************
 * Copyright (C) 2024, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/

// g++ -Wall -Wno-unknown-pragmas -O2 -DHLS_NO_XIL_FPO_LIB
#include "flatten.hpp"

#include <iostream>
#include "ap_float.h"


int main() {
	static_assert(width_v<short> == 16, "Wrong bitwidth for short.");
	static_assert(width_v<ap_int<3>> == 3, "Wrong bitwidth for ap_int.");
	static_assert(width_v<ap_fixed< 6, 3>> == 6, "Wrong bitwidth for ap_fixed.");
	static_assert(width_v<ap_float<12, 5>> == 12, "Wrong bitwidth for ap_float.");

	float const  a1[] = {  0.0f,  1.0f,  2.0f, 0.0f/0.0f };
	half  const  a2[] = { -0.0f, -1.0f, -2.0f, 1.0f/0.0f };
	char  const  a3[] = "abcdefg";
	hls::vector<ap_uint<4>,     6> const  v1 = { 2, 3, 5, 7, 11, 13 };
	hls::vector<ap_fixed<4, 2>, 6> const  v2 = { 0.5, .75, 1.25, 1.75, 2.75, 3.25 };

	std::cout << std::hex
		<< flatten(a1) << std::endl
		<< flatten(a2) << std::endl
		<< flatten(a3) << std::endl
		<< flatten(v1) << std::endl
		<< flatten(v2) << std::endl;

}
