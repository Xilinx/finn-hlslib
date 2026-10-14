/****************************************************************************
 * Copyright (C) 2024, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#include "mvau_top.hpp"

#include <iostream>
#include <cstdlib>


int main() {
	constexpr unsigned  NUM_VECTORS = 7;  // number of input vectors to test

	hls::stream<act_t>  src("src");
	hls::stream<out_t>  dst("dst");
	hls::stream<wgt_t>  wgt("wgt");

	// Weight matrix W[MH][MW] — small enough to enumerate
	// W[oh][iw] = (oh * MW + iw) % 7 - 3  (range: -3..3, fits in 4-bit signed)
	int  W[MH][MW];
	for(unsigned oh = 0; oh < MH; oh++)
		for(unsigned iw = 0; iw < MW; iw++)
			W[oh][iw] = int((oh * MW + iw) % 7) - 3;

	// Compute reference outputs: out[v][oh] = sum_iw( W[oh][iw] * act[v][iw] )
	int  act[NUM_VECTORS][MW];
	int  ref[NUM_VECTORS][MH];
	for(unsigned v = 0; v < NUM_VECTORS; v++) {
		for(unsigned iw = 0; iw < MW; iw++)
			act[v][iw] = (v * MW + iw) % 256;  // 8-bit unsigned range
		for(unsigned oh = 0; oh < MH; oh++) {
			int acc = 0;
			for(unsigned iw = 0; iw < MW; iw++)
				acc += W[oh][iw] * act[v][iw];
			ref[v][oh] = acc;
		}
	}

	// Feed weight stream: for each input vector, provide NF*SF weight tiles
	// Tile order: nf=0..NF-1, sf=0..SF-1
	// Each tile packs PE groups of SIMD weights:
	//   bits[(pe+1)*SIMD*TW::width-1 : pe*SIMD*TW::width] = W[nf*PE+pe][sf*SIMD+0..SIMD-1]
	for(unsigned v = 0; v < NUM_VECTORS; v++) {
		for(unsigned nf = 0; nf < NF; nf++) {
			for(unsigned sf = 0; sf < SF; sf++) {
				wgt_t  tile = 0;
				for(unsigned pe = 0; pe < PE; pe++) {
					for(unsigned s = 0; s < SIMD; s++) {
						unsigned  bit_lo = pe * SIMD * TW::width + s * TW::width;
						tile(bit_lo + TW::width - 1, bit_lo) = TW(W[nf * PE + pe][sf * SIMD + s]);
					}
				}
				wgt.write(tile);
			}
		}
	}

	// Feed activation stream: SF beats per vector (only nf==0 reads from stream)
	// Each beat packs SIMD activations: bits[(s+1)*TI::width-1 : s*TI::width] = act[v][sf*SIMD+s]
	for(unsigned v = 0; v < NUM_VECTORS; v++) {
		for(unsigned sf = 0; sf < SF; sf++) {
			act_t  a = 0;
			for(unsigned s = 0; s < SIMD; s++) {
				unsigned  bit_lo = s * TI::width;
				a(bit_lo + TI::width - 1, bit_lo) = TI(act[v][sf * SIMD + s]);
			}
			src.write(a);
		}
	}

	// Run DUT — call repeatedly
	unsigned  outputs_received = 0;
	unsigned  expected_outputs = NUM_VECTORS * NF;  // NF output beats per vector
	unsigned  timeout = 0;
	int  errors = 0;

	while(timeout < 200) {
		mvau_top(src, dst, wgt);

		if(dst.empty()) {
			timeout++;
			continue;
		}

		timeout = 0;
		out_t  y = dst.read();

		// Determine which vector and neuron fold this output belongs to
		unsigned  vec = outputs_received / NF;
		unsigned  nf  = outputs_received % NF;

		for(unsigned pe = 0; pe < PE; pe++) {
			unsigned  bit_lo = pe * TO::width;
			int  expected = ref[vec][nf * PE + pe];
			int  actual   = int(TO(y(bit_lo + TO::width - 1, bit_lo)));
			if(expected != actual) {
				std::cerr << "ERROR: vec=" << vec << " oh=" << (nf*PE+pe)
				          << " expected=" << expected << " actual=" << actual << std::endl;
				errors++;
			}
		}

		outputs_received++;
		if(outputs_received == expected_outputs) break;
	}

	std::cout << "Received " << outputs_received << " of " << expected_outputs
	          << " output beats, " << errors << " errors." << std::endl;

	if(outputs_received != expected_outputs) {
		std::cerr << "ERROR: Timed out waiting for outputs." << std::endl;
		return 1;
	}
	return (errors != 0);
}
