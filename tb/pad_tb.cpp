/****************************************************************************
 * Copyright Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: MIT
 *
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#include <iostream>
#include <iomanip>
#include <vector>

#include "pad_top.hpp"

static std::ostream& operator<<(std::ostream &o, vec_t const &v) {
	o << '{';
	for(unsigned  i = 0; i < PE; i++)  o << v[i] << ',';
	return  o << '}';
}

constexpr unsigned  NF = 2;	// frames
constexpr unsigned  LEAD = (PAD_N*WO + PAD_W)*CF;	// leading padding words of a frame
constexpr unsigned  NEXT = EAGER? LEAD : 0;	// emitted ahead of the input of the next frame

static tensor<val_t, C>  ifm[NF][H][W];

int main() {

	// Materialize input frames
	{
		val_t  v = 1;
		for(unsigned  f = 0; f < NF; f++)
			for(unsigned  h = 0; h < H; h++)
				for(unsigned  w = 0; w < W; w++)
					for(unsigned  c = 0; c < C; c++)
						ifm[f][h][w][c] = v++;
	}

	// Reference: padded frames in raster order (channel-folded).
	std::vector<vec_t>  ref;
	for(unsigned  f = 0; f < NF; f++) {
		for(signed  ho = -signed(PAD_N); ho < signed(HI + PAD_S); ho++) {
			for(signed  wo = -signed(PAD_W); wo < signed(WI + PAD_E); wo++) {
				bool const  img =
					(ho >= 0) && (ho < signed(HI)) && (ho % SY == 0) &&
					(wo >= 0) && (wo < signed(WI)) && (wo % SX == 0);
				for(unsigned  cf = 0; cf < CF; cf++) {
					ref.push_back(img? ifm[f][ho/SY][wo/SX].slice<PE>(cf*PE) : vec_t(val_t(0)));
				}
			}
		}
	}
	// Leading padding of the frame following the last, emitted ahead of its input if eager
	for(unsigned  i = 0; i < NEXT; i++)  ref.push_back(vec_t(val_t(0)));

	// Feed all frames
	hls::stream<vec_t>  src;
	for(unsigned  f = 0; f < NF; f++)
		for(unsigned  h = 0; h < H; h++)
			for(unsigned  w = 0; w < W; w++)
				for(unsigned  cf = 0; cf < CF; cf++)
					src.write(ifm[f][h][w].slice<PE>(cf*PE));

	// Run until the output stays quiet, validating on the fly
	hls::stream<vec_t>  dst;
	unsigned  idx = 0;
	unsigned  errors = 0;
	unsigned  timeout = 0;
	while(timeout < 100) {
		pad_top(src, dst);
		if(dst.empty())  timeout++;
		else {
			vec_t const  y = dst.read();
			if(idx < ref.size() && y != ref[idx]) {
				std::cerr
					<< "Output mismatch at " << idx << ":\n\t"
					<< "got " << y << "\n\t"
					<< "exp " << ref[idx] << std::endl;
				errors++;
			}
			idx++;
			timeout = 0;
		}
	}
	if(idx != ref.size()) {
		std::cerr << (idx < ref.size()? "Missing" : "Extra") << " output: "
			<< idx << " words instead of " << ref.size() << '.' << std::endl;
		errors++;
	}

	if(!errors) {
		std::cout
			<< "Test passed: " << idx << " outputs verified (cfg " << PAD_CFG
			<< (EAGER? ", eager" : ", lazy") << ")." << std::endl;
	}
	return  errors;
}
