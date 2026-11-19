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

#ifdef __RTL_SIMULATION__
constexpr unsigned  NF_STAGED = 0;	// staged feeding depends on the call sequence
#else
constexpr unsigned  NF_STAGED = 2;
#endif
constexpr unsigned  NF_BATCH = 2;
constexpr unsigned  NF = NF_STAGED + NF_BATCH;

constexpr unsigned  FW = HO*WO*CF;	// output words per frame
constexpr unsigned  LEAD = (PAD_N*WO + PAD_W)*CF;	// leading padding words of a frame
constexpr unsigned  NEXT = EAGER? LEAD : 0;	// emitted ahead of the input of the next frame
constexpr unsigned  SPARE = 16;	// calls beyond the expected output

static val_t  ifm[NF][H][W][C];

static void feed(hls::stream<vec_t> &src, unsigned const  f) {
	for(unsigned  h = 0; h < H; h++) {
		for(unsigned  w = 0; w < W; w++) {
			for(unsigned  cf = 0; cf < CF; cf++) {
				vec_t  v;
				for(unsigned  pe = 0; pe < PE; pe++) {
					v[pe] = ifm[f][h][w][cf*PE + pe];
				}
				src.write(v);
			}
		}
	}
}

static void run(hls::stream<vec_t> &src, hls::stream<vec_t> &dst, unsigned const  calls) {
	for(unsigned  i = 0; i < calls; i++)  pad_top(src, dst);
}

static void drain(hls::stream<vec_t> &dst, std::vector<vec_t> &got) {
	while(!dst.empty())  got.push_back(dst.read());
}

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
					vec_t  v;
					for(unsigned  pe = 0; pe < PE; pe++) {
						v[pe] = img? ifm[f][ho/SY][wo/SX][cf*PE + pe] : val_t(0);
					}
					ref.push_back(v);
				}
			}
		}
	}
	// Leading padding of the frame following the last, emitted ahead of its input if eager
	for(unsigned  i = 0; i < NEXT; i++)  ref.push_back(vec_t(val_t(0)));

	hls::stream<vec_t>  src;
	hls::stream<vec_t>  dst;
	std::vector<vec_t>  got;
	unsigned  errors = 0;

	// Staged frames probe the behavior without input before feeding each frame
	for(unsigned  f = 0; f < NF_STAGED; f++) {
		run(src, dst, FW + LEAD + SPARE);
		drain(dst, got);
		if(got.size() != f*FW + NEXT) {
			std::cerr
				<< "Frame " << f << " without input: "
				<< "emitted " << got.size() << " words in total, expected " << f*FW + NEXT << '.'
				<< std::endl;
			errors++;
		}

		feed(src, f);
		run(src, dst, FW + LEAD + SPARE);
		drain(dst, got);
		if(got.size() != (f+1)*FW + NEXT) {
			std::cerr
				<< "Frame " << f << " with input: "
				<< "emitted " << got.size() << " words in total, expected " << (f+1)*FW + NEXT << '.'
				<< std::endl;
			errors++;
		}
	}

	// Frames fed back-to-back
	for(unsigned  f = NF_STAGED; f < NF; f++)  feed(src, f);
	run(src, dst, NF_BATCH*FW + LEAD + SPARE);
	drain(dst, got);

	// Output Validation
	unsigned  idx = 0;
	for(; idx < ref.size() && idx < got.size(); idx++) {
		if(got[idx] != ref[idx]) {
			std::cerr
				<< "Output mismatch at " << idx << ":\n\t"
				<< "got " << got[idx] << "\n\t"
				<< "exp " << ref[idx] << std::endl;
			errors++;
		}
	}
	if(idx < ref.size()) {
		std::cerr << "Missing output." << std::endl;
		errors++;
	}
	else if(idx < got.size()) {
		std::cerr << "Extra output." << std::endl;
		errors++;
	}

	if(!errors) {
		std::cout
			<< "Test passed: " << idx << " outputs verified (cfg " << PAD_CFG
			<< (EAGER? ", eager" : ", lazy") << ")." << std::endl;
	}
	return  errors;
}
