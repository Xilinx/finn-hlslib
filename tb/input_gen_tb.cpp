/****************************************************************************
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @brief	Testbench for input_gen<> with pooling loop nests.
 *
 * Streams several frames back-to-back and checks the emitted window
 * sequence (data and window-completion marker) against the loop nest, that
 * all input is consumed, and that no extra output appears. Geometries cover
 * exact tiling, overlapping windows, floor remainders and kernel < stride,
 * i.e. readout patterns that skip input.
 ****************************************************************************/
#include <iostream>
#include <vector>
#include "input_gen_top.hpp"
#include "input_gen.hpp"

constexpr unsigned  FRAMES = 3;

template<unsigned H, unsigned W, unsigned CF, unsigned KH, unsigned KW, unsigned SH, unsigned SW>
struct Geometry {
	static constexpr unsigned  OH = (H-KH)/SH + 1;
	static constexpr unsigned  OW = (W-KW)/SW + 1;
	static constexpr unsigned  IN_PER_FRAME  = H*W*CF;
	static constexpr unsigned  OUT_PER_FRAME = OH*OW*CF*KH*KW;

	static void dut(hls::stream<val_t> &src, hls::stream<flit_t<val_t>> &dst) {
		input_gen<2, H*W*CF, OH, SH*W*CF, OW, SW*CF, CF, 1, KH, W*CF, KW, CF>(src, dst);
	}

	template<typename F>
	static unsigned check(char const *name, F &&step) {
		hls::stream<val_t>  src;
		hls::stream<flit_t<val_t>>  dst;
		std::vector<flit_t<val_t>>  exp;
		for(unsigned  f = 0; f < FRAMES; f++) {
			for(unsigned  i = 0; i < IN_PER_FRAME; i++)  src.write(val_t(f*IN_PER_FRAME + i));
			for(unsigned  oh = 0; oh < OH; oh++)
				for(unsigned  ow = 0; ow < OW; ow++)
					for(unsigned  cf = 0; cf < CF; cf++)
						for(unsigned  kh = 0; kh < KH; kh++)
							for(unsigned  kw = 0; kw < KW; kw++)
								exp.emplace_back(
									(kh == KH-1) && (kw == KW-1),
									val_t(f*IN_PER_FRAME + ((oh*SH + kh)*W + (ow*SW + kw))*CF + cf)
								);
		}

		// Each call consumes and emits at most one element; leave ample slack
		// for latency and draining.
		unsigned const  calls = FRAMES*(IN_PER_FRAME + OUT_PER_FRAME) + 1000;
		for(unsigned  i = 0; i < calls; i++)  step(src, dst);

		unsigned  errors = 0;
		unsigned  n = 0;
		while(!dst.empty()) {
			auto const  act = dst.read();
			if(n >= exp.size()) {
				errors++;
			}
			else if((act.data != exp[n].data) || (act.last != exp[n].last)) {
				if(errors < 5) {
					std::cerr << name << ": output " << n << " is " << act.data << '/' << act.last
					          << ", expected " << exp[n].data << '/' << exp[n].last << std::endl;
				}
				errors++;
			}
			n++;
		}
		if(n != exp.size())  std::cerr << name << ": " << n << " outputs, expected " << exp.size() << std::endl;
		if(!src.empty())     std::cerr << name << ": " << src.size() << " inputs not consumed" << std::endl;
		errors += (n != exp.size()) + !src.empty();
		std::cout << (errors? "FAIL " : "PASS ") << name << std::endl;
		return  errors;
	}

	static unsigned check(char const *name) { return  check(name, dut); }
};

int main() {
	unsigned  errors = 0;

	// DUT top-level (subject of synthesis and co-simulation)
	{
		using namespace top;
		errors += Geometry<H, W, CF, KH, KW, SH, SW>::check("top 9x9 CF2 k1x2 s2x3", input_gen_top);
	}

	// Pure C sweep across readout patterns
	// exact tiling
	errors += Geometry< 6, 10, 2, 2, 2, 2, 2>::check(" 6x10 CF2 k2   s2");
	errors += Geometry< 8,  9, 2, 2, 3, 2, 3>::check(" 8x9  CF2 k2x3 s2x3");
	errors += Geometry<28, 28, 4, 2, 2, 2, 2>::check("28x28 CF4 k2   s2");
	errors += Geometry< 7,  7,16, 7, 7, 7, 7>::check(" 7x7  CF16 k7  s7 (global)");
	errors += Geometry< 9,  9, 2, 1, 1, 1, 1>::check(" 9x9  CF2 k1   s1 (identity)");
	// overlapping windows
	errors += Geometry< 9,  9, 2, 3, 3, 1, 1>::check(" 9x9  CF2 k3   s1");
	errors += Geometry< 9,  9, 2, 3, 3, 2, 2>::check(" 9x9  CF2 k3   s2");
	errors += Geometry< 6,  6, 1, 4, 4, 1, 1>::check(" 6x6  CF1 k4   s1");
	// floor remainder
	errors += Geometry< 7,  7, 1, 2, 2, 2, 2>::check(" 7x7  CF1 k2   s2 (remainder)");
	errors += Geometry< 8,  8, 1, 3, 3, 2, 2>::check(" 8x8  CF1 k3   s2 (remainder)");
	errors += Geometry< 9, 10, 2, 2, 3, 2, 2>::check(" 9x10 CF2 k2x3 s2 (remainder)");
	errors += Geometry<12,  9, 3, 5, 3, 2, 1>::check("12x9  CF3 k5x3 s2x1 (overlap+remainder)");
	errors += Geometry<28, 28, 2, 3, 3, 2, 2>::check("28x28 CF2 k3   s2 (overlap+remainder)");
	errors += Geometry< 5,  9, 1, 3, 3, 3, 3>::check(" 5x9  CF1 k3   s3 (single output row)");
	// kernel < stride
	errors += Geometry<10,  6, 2, 1, 2, 2, 3>::check("10x6  CF2 k1x2 s2x3");
	errors += Geometry<10,  6, 4, 1, 2, 2, 3>::check("10x6  CF4 k1x2 s2x3");
	errors += Geometry< 9,  9, 1, 1, 1, 2, 2>::check(" 9x9  CF1 k1   s2");
	errors += Geometry< 5, 13, 3, 2, 4, 3, 5>::check(" 5x13 CF3 k2x4 s3x5");
	errors += Geometry<11,  7, 2, 1, 1, 3, 2>::check("11x7  CF2 k1   s3x2");
	errors += Geometry<12, 12, 2, 2, 2, 3, 3>::check("12x12 CF2 k2   s3");
	errors += Geometry<13, 11, 1, 2, 3, 4, 5>::check("13x11 CF1 k2x3 s4x5");

	std::cout << (errors? "FAILED" : "PASSED") << " with " << errors << " error(s)" << std::endl;
	return  errors != 0;
}
