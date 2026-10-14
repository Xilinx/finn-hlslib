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
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 *****************************************************************************/

#include "pool_top.hpp"
#include <iostream>
#include <limits>


static std::ostream& operator<<(std::ostream &o, vec_t const &v) {
	o << '{';
	for(unsigned  i = 0; i < PE; i++)  o << v[i] << ',';
	return  o << '}';
}

int main() {

	// Materialize input image
	val_t  ifm[H][W][C];
	{
		val_t  v = 0;
		for(unsigned  h = 0; h < H; h++)
			for(unsigned  w = 0; w < W; w++)
				for(unsigned  c = 0; c < C; c++)
					ifm[h][w][c] = v++;
	}

	// Generate pre-windowed, pre-folded input for MaxPool<vec_t, K*K, CF>
	// Stream order: for each output pixel (ho, wo):
	//   for each kernel position (kh*K + kw) in 0..K*K-1:
	//     for each channel fold cf in 0..CF-1:
	//       one vec_t with PE channels
	hls::stream<vec_t>  src;
	for(unsigned  ho = 0; ho < HO; ho++) {
		for(unsigned  wo = 0; wo < WO; wo++) {
			for(unsigned  kh = 0; kh < K; kh++) {
				for(unsigned  kw = 0; kw < K; kw++) {
					for(unsigned  cf = 0; cf < CF; cf++) {
						vec_t  v;
						for(unsigned  pe = 0; pe < PE; pe++) {
							v[pe] = ifm[ho*S + kh][wo*S + kw][cf*PE + pe];
						}
						src.write(v);
					}
				}
			}
		}
	}

	// Reference: element-wise max pool per channel fold
	hls::stream<vec_t>  ref;
	for(unsigned  ho = 0; ho < HO; ho++) {
		for(unsigned  wo = 0; wo < WO; wo++) {
			for(unsigned  cf = 0; cf < CF; cf++) {
				vec_t  mx;
				for(unsigned  pe = 0; pe < PE; pe++)
					mx[pe] = std::numeric_limits<val_t>::min();
				for(unsigned  kh = 0; kh < K; kh++) {
					for(unsigned  kw = 0; kw < K; kw++) {
						for(unsigned  pe = 0; pe < PE; pe++) {
							val_t const  x = ifm[ho*S + kh][wo*S + kw][cf*PE + pe];
							mx[pe] = std::max(val_t(mx[pe]), x);
						}
					}
				}
				ref.write(mx);
			}
		}
	}

	// DUT — call function-pipelined kernel repeatedly
	hls::stream<vec_t>  dst;
	for(unsigned  i = 0; i < HO*WO*K*K*CF + HO*WO*CF; i++)  pool_top(src, dst);

	// Output Validation
	unsigned  errors = 0;
	while(true) {
		bool const  ref_empty = ref.empty();
		bool const  dst_empty = dst.empty();
		if(!ref_empty && !dst_empty) {
			auto const  exp = ref.read();
			auto const  act = dst.read();
			if(ap_uint<vec_t::TOTAL>(act) != ap_uint<vec_t::TOTAL>(exp)) {
				std::cerr
					<< "Output mismatch:\n\t"
					<< act << "\n\t"
					<< exp << std::endl;
				errors++;
			}
			continue;
		}
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
