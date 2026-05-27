/****************************************************************************
 * Copyright (C) 2024, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/

// g++ -Wall -Wno-unknown-pragmas -O2 -DHLS_NO_XIL_FPO_LIB
#include <ap_fixed.h>
#include <ap_float.h>
#include "flatten.hpp"

#include <cstring>
#include <iostream>

static unsigned  errors = 0;

template<typename T, size_t N>
void check_roundtrip(T const (&orig)[N], char const *label) {
	auto const  flat = flatten(orig);
	T  restored[N];
	unflatten(restored, flat);
	for(size_t  i = 0; i < N; i++) {
		// Use memcmp for bitwise equality (handles NaN, -0)
		if(std::memcmp(&orig[i], &restored[i], sizeof(T)) != 0) {
			std::cerr << label << "[" << i << "]: round-trip mismatch" << std::endl;
			errors++;
		}
	}
}

template<typename T, size_t N>
void check_roundtrip(hls::vector<T, N> const &orig, char const *label) {
	auto const  flat = flatten(orig);
	hls::vector<T, N>  restored;
	unflatten(restored, flat);
	for(size_t  i = 0; i < N; i++) {
		T const  a = orig[i];
		T const  b = restored[i];
		if(std::memcmp(&a, &b, sizeof(T)) != 0) {
			std::cerr << label << "[" << i << "]: round-trip mismatch" << std::endl;
			errors++;
		}
	}
}

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

	// Round-trip checks for existing types
	check_roundtrip(a1, "float");
	check_roundtrip(a2, "half");
	check_roundtrip(a3, "char");
	check_roundtrip(v1, "vector<ap_uint<4>>");
	check_roundtrip(v2, "vector<ap_fixed<4,2>>");

	// ap_fixed round-trip
	{
		ap_fixed<8, 4> const  a[] = { 1.5, -2.25, 3.75, -0.5 };
		check_roundtrip(a, "ap_fixed<8,4>");
	}

	// ap_ufixed round-trip
	{
		ap_ufixed<8, 4> const  a[] = { 1.5, 2.25, 3.75, 0.5 };
		check_roundtrip(a, "ap_ufixed<8,4>");
	}

	// ap_float round-trip
	{
		ap_float<16, 5>  a[4];
		a[0] = 1.0;
		a[1] = -2.5;
		a[2] = 0.0;
		a[3] = 0.125;
		check_roundtrip(a, "ap_float<16,5>");
	}

	// double round-trip
	{
		double const  a[] = { 0.0, -0.0, 1.0, -1.0, 1.0/0.0, 0.0/0.0 };
		check_roundtrip(a, "double");
	}

	// ap_int round-trip
	{
		ap_int<7> const  a[] = { 0, 1, -1, 63, -64 };
		check_roundtrip(a, "ap_int<7>");
	}

	if(errors) {
		std::cerr << errors << " error(s) detected." << std::endl;
		return  1;
	}
	std::cout << "All round-trip checks passed." << std::endl;
	return  0;
}
