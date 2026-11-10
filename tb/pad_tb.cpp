/****************************************************************************
 * Copyright Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: MIT
 *
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#include "pad_top.hpp"
#include <iostream>
#include <iomanip>


static std::ostream& operator<<(std::ostream &o, vec_t const &v) {
	o << '{';
	for(unsigned  i = 0; i < PE; i++)  o << v[i] << ',';
	return  o << '}';
}

int main() {

	// Materialize input image
	val_t  ifm[H][W][C];
	{
		val_t  v = 1;
		for(unsigned  h = 0; h < H; h++)
			for(unsigned  w = 0; w < W; w++)
				for(unsigned  c = 0; c < C; c++)
					ifm[h][w][c] = v++;
	}

	// Feed source stream: channel-folded pixels in raster order
	hls::stream<vec_t>  src;
	for(unsigned  h = 0; h < H; h++) {
		for(unsigned  w = 0; w < W; w++) {
			for(unsigned  cf = 0; cf < CF; cf++) {
				vec_t  v;
				for(unsigned  pe = 0; pe < PE; pe++) {
					v[pe] = ifm[h][w][cf*PE + pe];
				}
				src.write(v);
			}
		}
	}

	// Reference: padded output in raster order (channel-folded)
	hls::stream<vec_t>  ref;
	for(signed  ho = -(signed)PAD_N; ho < (signed)(H + PAD_S); ho++) {
		for(signed  wo = -(signed)PAD_W; wo < (signed)(W + PAD_E); wo++) {
			for(unsigned  cf = 0; cf < CF; cf++) {
				vec_t  v;
				for(unsigned  pe = 0; pe < PE; pe++) {
					if(ho >= 0 && ho < (signed)H && wo >= 0 && wo < (signed)W) {
						v[pe] = ifm[ho][wo][cf*PE + pe];
					}
					else {
						v[pe] = 0;  // default pad value
					}
				}
				ref.write(v);
			}
		}
	}

	// DUT — call function-pipelined kernel repeatedly
	// Total input elements: H*W*CF
	// Total output elements: HO*WO*CF
	hls::stream<vec_t>  dst;
	unsigned const  calls = H*W*CF + HO*WO*CF;
	for(unsigned  i = 0; i < calls; i++)  pad_top(src, dst);

	// Output Validation
	unsigned  errors = 0;
	unsigned  idx = 0;
	while(true) {
		bool const  ref_empty = ref.empty();
		bool const  dst_empty = dst.empty();
		if(!ref_empty && !dst_empty) {
			auto const  exp = ref.read();
			auto const  act = dst.read();
			if(act != exp) {
				std::cerr
					<< "Output mismatch at " << idx << ":\n\t"
					<< "got " << act << "\n\t"
					<< "exp " << exp << std::endl;
				errors++;
			}
			idx++;
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

	if(!errors)  std::cout << "Test passed: " << idx << " outputs verified." << std::endl;
	return  errors;
}
