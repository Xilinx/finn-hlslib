/******************************************************************************
 *  Copyright (c) 2022, Xilinx, Inc.
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
 *******************************************************************************
 * @brief	Testbench for weight_stream_splitter.
 * @author	Thomas B. Preusser <thomas.preusser@amd.com>
 * @note
 *	This test is not extremely strong as backpressure cannot be modelled and
 *	applied through hls::streams.
 *******************************************************************************/
#include "weight_stream_splitter_top.hpp"

#include <iostream>
#include <random>

int main() {
	unsigned  mismatches = 0;
	std::default_random_engine  rnd;
	std::uniform_int_distribution<>  dist(1, 10);

	hls::stream<T>  src;
	hls::stream<T>  dst[N];

	unsigned  frame = 0;
	unsigned  phase = 0;
	unsigned  exp[N] = { 0, };
	unsigned  cnt[N] = { 0, };
	unsigned  fin = 0;
	while(true) {
		// Feed data encoding (frame.phase)
		if((fin == N) && (phase == 0)) {
			bool  drained = src.empty();
			for(unsigned  i = 0; i < N; i++) {
				drained &= dst[i].empty();
			}
			if(drained) {
				for(unsigned  i = 0; i < N; i++) {
					unsigned const  offset = exp[i] & ((1u<<PHASE_BITS)-1);
					if(offset != 0) {
						std::cout << '#' << i << " terminated on non-zero phase in frame: " << offset << std::endl;
						mismatches++;
					}
				}
				std::cout << "Encountered " << mismatches << " errors." << std::endl;
				return  mismatches;
			}
		}
		else if(src.empty() && (dist(rnd) <= 8)) {
			src.write((frame << PHASE_BITS) | phase);
			if(++phase == PERIOD) {
				frame++;
				phase = 0;
			}
		}

		// Clock the DUT
		weight_stream_splitter_top(src, dst);

		// Drain output validating the preservation of the phase
		for(unsigned  i = 0; i < N; i++) {
			if(!dst[i].empty()) {
				auto const  y = dst[i].read();
				if((y & ((1u<<PHASE_BITS)-1)) != exp[i])  mismatches++;
				if(++exp[i] == PERIOD) {
					std::cout << "Wrap #" << i << '.' << cnt[i] << '<' << (y>>PHASE_BITS) << std::endl;
					exp[i] = 0;
					if(++cnt[i] == FRAMES)  fin++;
				}
			}
		}
	}

}
