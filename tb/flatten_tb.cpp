/****************************************************************************
 * Copyright (C) 2024, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/

// g++ -Wall -Wno-unknown-pragmas -O2 -DHLS_NO_XIL_FPO_LIB
#include <iostream>
#include <cmath>
#include "flatten.hpp"


int main() {
	float const  a1[] = {  0.0f,  1.0f,  2.0f, 0.0f/0.0f };
	half  const  a2[] = { -0.0f, -1.0f, -2.0f, 1.0f/0.0f };
	char  const  a3[] = "abcdefg";
	hls::vector<ap_uint<4>, 6> const  v1 = { 2, 3, 5, 7, 11, 13 };

	std::cout << std::hex
		<< flatten(a1) << std::endl
		<< flatten(a2) << std::endl
		<< flatten(a3) << std::endl
		<< flatten(v1) << std::endl;

}
