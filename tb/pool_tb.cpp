/******************************************************************************
 * Copyright (c) 2019, Xilinx, Inc.
 * Copyright (c) 2025, AMD, Inc.
 * All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1.  Redistributions of source code must retain the above copyright notice,
 *	 this list of conditions and the following disclaimer.
 *
 *  2.  Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 *
 *  3.  Neither the name of the copyright holder nor the names of its
 *	  contributors may be used to endorse or promote products derived from
 *	  this software without specific prior written permission.
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
 ****************************************************************************
 * @author	Giulio Gambardella <giuliog@xilinx.com>
 * @author	Tobias Alonso <tobiasa@xilinx.com>
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 *****************************************************************************/

#include "pool_top.hpp"
#include <iostream>


static std::ostream& operator<<(std::ostream &o, pix_t const &pix) {
	o << '{';
	for(auto const &ele : pix)  o << ele << ',';
	return  o << '}';
}

int main() {
	constexpr unsigned  HO = 1+(H-K)/S;
	constexpr unsigned  WO = 1+(W-K)/S;

	// Input and Reference Output
	hls::stream<pix_t>  src;
	hls::stream<pix_t>  ref; {
		pix_t  ifm[H][W]; // materialized image for reference computation

		{ // Input Feed
			val_t  v = 0;
			for(size_t  h = 0; h < H; h++) {
				for(size_t  w = 0; w < W; w++) {
					pix_t  pix;
					for(size_t  c = 0; c < C; c++)  pix[c] = v++;
					src.write(pix);
					ifm[h][w] = pix;
				}
			}
		}

		// Reference Computation
		for(size_t  ho = 0; ho < HO; ho++) {
			for(size_t  wo = 0; wo < WO; wo++) {
				pix_t  pix = pix_t(std::numeric_limits<val_t>::min());
				for(size_t  kh = 0; kh < K; kh++) {
					for(size_t  kw = 0; kw < K; kw++) {
						pix_t const  x = ifm[ho*S + kh][wo*S + kw];
						for(size_t  c = 0; c < C; c++)  pix[c] = std::max(pix[c], x[c]);
					}
				}
				ref.write(pix);
			}
		}
	}

	// DUT
	hls::stream<pix_t>  dst;
	pool_top(src, dst);

	// Output Validation
	unsigned  errors = 0;
	while(true) {
		bool const  ref_empty = ref.empty();
		bool const  dst_empty = dst.empty();
		if(!ref_empty && !dst_empty) {
			// Compare and continue
			auto const  exp = ref.read();
			auto const  act = dst.read();

			if(act != exp) {
				std::cerr
					<< "Output mismatch:\n\t"
					<< act << "\n\t"
					<< exp << std::endl;
				errors++;
			}
			continue;
		}

		// At least one stream is empty -> terminate
		if(!ref_empty) {
			std::cerr << "Missing output." << std::endl;
			errors++;
		}
		else if(!dst_empty) {
			std::cerr << "Extra output." << std::endl;
			errors++;
		}
		break;
	}
	return  errors;
}
