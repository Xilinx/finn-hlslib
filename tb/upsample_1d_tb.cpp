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
#include "upsample_1d_top.hpp"

#include <iostream>
#include <random>


int main() {
	using  TT = ap_uint<NumChannels * T::width>;

	unsigned  errors = 0; {
		std::default_random_engine  rnd;
		std::uniform_int_distribution<>  dist(1, (1<<TT::width)-1);

		for(unsigned  k = 0; k < 7; k++) {
			hls::stream<TT>  src("src");
			hls::stream<TT>  dst("dst");
			TT  ref[IFMDim];

			// Feed an input vector
			for(unsigned  i = 0; i < IFMDim; i++) {
				TT const  x = dist(rnd);
				src.write(x);
				ref[i] = x;
			}

			// Execute upscale
			upsample_1d_top(src, dst);
			if(!src.empty())  errors++;

			// Verify the result
			for(unsigned  i = 0; i < OFMDim; i++) {
				TT  y;
				if(!dst.read_nb(y) || (y != ref[i*IFMDim/OFMDim]))  errors++;
			}
			if(!dst.empty())  errors++;
		}
	}

	if(errors == 0)  return  0;
	else {
		std::cout << errors << " errors." << std::endl;
		return  1;
	}
}
