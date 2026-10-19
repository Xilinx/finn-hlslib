/****************************************************************************
 * Copyright (C) 2024, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Thomas B. Preusser <thomas.preusser@amd.com>
 ****************************************************************************/
#include "mvau_top.hpp"

#include <iostream>
#include <cstdlib>


int main() {
	constexpr unsigned  NUM_VECTORS = 7;  // number of input vectors to test

	hls::stream<act_t>  src("src");
	hls::stream<out_t>  dst("dst");
	hls::stream<wgt_t>  wgt("wgt");

	// Weight matrix W[MH][MW] -- small enough to enumerate
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
	// Each tile: tensor<TW, PE, SIMD> — tile[pe][s] = W[nf*PE+pe][sf*SIMD+s]
	for(unsigned v = 0; v < NUM_VECTORS; v++) {
		for(unsigned nf = 0; nf < NF; nf++) {
			for(unsigned sf = 0; sf < SF; sf++) {
				wgt_t  tile;
				for(unsigned pe = 0; pe < PE; pe++) {
					for(unsigned s = 0; s < SIMD; s++) {
						tile[pe][s] = TW(W[nf * PE + pe][sf * SIMD + s]);
					}
				}
				wgt.write(tile);
			}
		}
	}

	// Feed activation stream: SF beats per vector
	// Each beat: tensor<TI, MMV, SIMD> — a[0][s] = act[v][sf*SIMD+s]
	for(unsigned v = 0; v < NUM_VECTORS; v++) {
		for(unsigned sf = 0; sf < SF; sf++) {
			act_t  a;
			for(unsigned s = 0; s < SIMD; s++) {
				a[0][s] = TI(act[v][sf * SIMD + s]);
			}
			src.write(a);
		}
	}

	// Run DUT -- call repeatedly (function-pipelined, ap_ctrl_none)
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
			int  expected = ref[vec][nf * PE + pe];
			TO   elem     = y[0][pe];
			int  actual   = elem;
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
