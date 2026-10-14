/****************************************************************************
 * Copyright (C) 2024, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ****************************************************************************/
#include "vvau_top.hpp"

#include <iostream>
#include <cstdlib>


int main() {
	constexpr unsigned  NUM_PIXELS = 5;  // number of input pixels to test

	hls::stream<act_t>  src("src");
	hls::stream<out_t>  dst("dst");
	hls::stream<wgt_t>  wgt("wgt");

	// Depthwise weight kernel: W[ch][k2]
	// Each channel has its own kernel (no cross-channel mixing)
	// W[ch][k] = (ch * KERNEL2 + k) % 7 - 3
	int  W[CHANNELS][KERNEL2];
	for(unsigned ch = 0; ch < CHANNELS; ch++)
		for(unsigned k = 0; k < KERNEL2; k++)
			W[ch][k] = int((ch * KERNEL2 + k) % 7) - 3;

	// Input activations: act[pixel][ch][k] (depthwise — per-channel, per-kernel-position)
	int  act_vals[NUM_PIXELS][CHANNELS][KERNEL2];
	int  ref[NUM_PIXELS][CHANNELS];
	for(unsigned p = 0; p < NUM_PIXELS; p++) {
		for(unsigned ch = 0; ch < CHANNELS; ch++) {
			int acc = 0;
			for(unsigned k = 0; k < KERNEL2; k++) {
				act_vals[p][ch][k] = (p * CHANNELS * KERNEL2 + ch * KERNEL2 + k) % 256;
				acc += W[ch][k] * act_vals[p][ch][k];
			}
			ref[p][ch] = acc;
		}
	}

	// Feed streams: for each pixel, NF*SF beats
	// Beat order: nf=0..NF-1, sf=0..SF-1
	// Weight tile: pe0 SIMD weights in LSBs, pe1 SIMD weights above, etc.
	// Act tile: pe0 SIMD activations in LSBs, pe1 SIMD activations above, etc.
	for(unsigned p = 0; p < NUM_PIXELS; p++) {
		for(unsigned nf = 0; nf < NF; nf++) {
			for(unsigned sf = 0; sf < SF; sf++) {
				wgt_t  wtile = 0;
				act_t  atile = 0;
				for(unsigned pe = 0; pe < PE; pe++) {
					unsigned  ch = nf * PE + pe;
					for(unsigned s = 0; s < SIMD; s++) {
						unsigned  k = sf * SIMD + s;

						unsigned  wbit_lo = pe * SIMD * TW::width + s * TW::width;
						wtile(wbit_lo + TW::width - 1, wbit_lo) = TW(W[ch][k]);

						unsigned  abit_lo = pe * SIMD * TI::width + s * TI::width;
						atile(abit_lo + TI::width - 1, abit_lo) = TI(act_vals[p][ch][k]);
					}
				}
				wgt.write(wtile);
				src.write(atile);
			}
		}
	}

	// Run DUT
	unsigned  outputs_received = 0;
	unsigned  expected_outputs = NUM_PIXELS * NF;
	unsigned  timeout = 0;
	int  errors = 0;

	while(timeout < 200) {
		vvau_top(src, dst, wgt);

		if(dst.empty()) {
			timeout++;
			continue;
		}

		timeout = 0;
		out_t  y = dst.read();

		unsigned  pix = outputs_received / NF;
		unsigned  nf  = outputs_received % NF;

		for(unsigned pe = 0; pe < PE; pe++) {
			unsigned  bit_lo = pe * TO::width;
			int  expected = ref[pix][nf * PE + pe];
			int  actual   = int(TO(y(bit_lo + TO::width - 1, bit_lo)));
			if(expected != actual) {
				std::cerr << "ERROR: pix=" << pix << " ch=" << (nf*PE+pe)
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
