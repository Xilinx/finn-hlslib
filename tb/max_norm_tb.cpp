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
 * @brief	Testbench for MaxNorm layer.
 * @author	Thomas B. Preusser <thomas.preusser@amd.com>
 *******************************************************************************/
#include "max_norm_top.hpp"

#include <iostream>
#include <iomanip>
#include <random>

int main() {
	using  TI = ap_uint<WI>;
	using  TO = ap_uint<WO>;

	unsigned  mismatches = 0; {
		std::default_random_engine  rnd;
		std::uniform_int_distribution<>  dist(1, (1<<WI)-1);

		hls::stream<TI>  src("src");
		hls::stream<TI>  bypass("bypass");
		hls::stream<TO>  dst[2];
		for(unsigned  k = 0; k < 12; k++) {
			TI const  max = dist(rnd);
			src.write(max);
			bypass.write(max);

			std::uniform_int_distribution<>  d(0, max);
			for(unsigned  i = 1; i < FM_SIZE; i++) {
				TI const  x = d(rnd);
				src.write(x);
				bypass.write(x);
			}
			std::cout << "MAX=" << max << std::endl;

			max_norm_top(src, dst);

			float const  ref_scale[2] = {
				SCALE0 * float((1uL<<WO)-1) / max,
				SCALE1 * float((1uL<<WO)-1) / max
			};
			for(unsigned  i = 0; i < FM_SIZE; i++) {
				TI const  x = bypass.read();
				std::cout << std::setw(3) << x << " ->";
				for(unsigned  j = 0; j < 2; j++) {
					TO const     y   = dst[j].read();
					float const  ref = ref_scale[j] * x;
					bool  const  ok  = std::abs(y-ref) <= 0.5f;
					if(!ok)  mismatches++;
					std::cout <<'\t' << std::setw(3) << y << " / " << std::setw(7) << ref << '\t' << (ok? '.' : 'X');
				}
				std::cout << std::endl;
			}
			std::cout << "--------------\n" << std::endl;
		}
	}

	if(mismatches == 0)  return  0;
	else {
		std::cout << mismatches << " output mismatches." << std::endl;
		return  1;
	}
}
