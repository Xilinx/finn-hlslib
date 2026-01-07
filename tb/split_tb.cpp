/******************************************************************************
 *  Copyright (c) 2024-2025, Advanced Micro Devices, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1.  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2.  Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *  3.  Neither the name of the copyright holder nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @author	Michal Danilowicz <danilowi@agh.edu.pl>
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 *
 * @brief Testbench for the channel split operation.
 *******************************************************************************/

#include "split_top.hpp"
#include <iostream>


template<
	typename  T,
	size_t    N
>
std::ostream& operator<<(std::ostream &o, hls::vector<T, N> const &v) {
	char  c = '{';
	for(auto const &e : v) {
		o << c << e;
		c = ',';
	}
	return  o << '}';
}

int main() {
	hls::stream<T> src;
	hls::stream<T> dst[NUM_OUTPUTS];
	hls::stream<T> exp[NUM_OUTPUTS];

	{ // prepare stimulus and expected output
		unsigned  c = 0;
		for(unsigned  r = 0; r < REPS; r++) {
			for(unsigned  d = 0; d < NUM_OUTPUTS; d++) {
				for(unsigned  i = 0; i < FOLDS_PER_OUTPUT[d]; i++) {
					T const  x = T(c++);
					src.write(x);
					exp[d].write(x);
				}
			}
		}
	}

	unsigned  timeout = 0;
	while(timeout < 100) {
		split_top(src, dst);

		bool  have = false;
		for(unsigned  d = 0; d < NUM_OUTPUTS; d++) {
			if(!dst[d].empty()) {
				auto const  y = dst[d].read();
				if(exp[d].empty()) {
					std::cerr << "Spurious output: " << y << std::endl;
					return  1;
				}
				auto const  ref = exp[d].read();
				if(y != ref) {
					std::cerr << "Output mismatch: " << y << " instead of " << ref << std::endl;
					return  1;
				}
				have = true;
			}
		}
		if(have)  timeout = 0;
		else  timeout++;
	}

	for(auto &s : exp) {
		if(!s.empty()) {
			std::cerr << "Missing output." << std::endl;
			return  1;
		}
	}

	return  0;
}
