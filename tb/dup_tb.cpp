/****************************************************************************
 * Copyright (C) 2024 - 2026 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#include "dup_top.hpp"

#include <iostream>


int main() {
	constexpr unsigned  ELEMS = 1001;

	hls::stream<T>  src;
	hls::stream<T>  dst[N];

	// Feed source
	for(unsigned  i = 0; i < ELEMS; i++)  src.write(i);

	// Consume and check outputs
	unsigned  i = 0;
	unsigned  timeout = 0;
	while(timeout < 100) {

		// DUT
		dup_top(src, dst);

		// Check for all outputs being ready
		bool  rdy = true;
		for(auto &d : dst)  rdy &= !d.empty();

		if(!rdy)  timeout++;
		else {
			for(auto &d : dst) {
				T const  y = d.read();
				if(y != i) {
					std::cerr << "Read " << y << " instead of " << i << std::endl;
					return  1;
				}
			}

			i++;
			timeout = 0;
		}
	}

	std::cout << "Verified " << i << " outputs." << std::endl;
	return (i != ELEMS);

} // main()
