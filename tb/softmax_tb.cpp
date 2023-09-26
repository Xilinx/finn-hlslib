/******************************************************************************
 *  Copyright (c) 2023, Xilinx, Inc.
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
 * @brief	Testbench for SoftMax layer.
 * @author	Thomas B. Preusser <thomas.preusser@amd.com>
 *******************************************************************************/
#include "softmax_top.hpp"

#include <iostream>
#include <iomanip>
#include <random>

int main() {
	hls::stream<TI>     src("src");
	hls::stream<TI>     bypass("bypass");
	hls::stream<float>  dst("dst");

	{
		std::default_random_engine  rnd;
		std::uniform_int_distribution<>  dist(0, (1<<TI::width)-1);
		for(unsigned  i = 0; i < FM_SIZE; i++) {
			TI const  x = dist(rnd);
			src   .write(x);
			bypass.write(x);
		}
	}

	softmax_top(src, dst);
	for(unsigned  i = 0; i < FM_SIZE; i++) {
		TI    const  x = bypass.read();
		float const  y = dst.read();
		std::cout << std::setw(3) << x << " -> " << y << std::endl;
	}
	std::cout << "--------------\n" << std::endl;

}
