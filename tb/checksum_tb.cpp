/****************************************************************************
 * Copyright (C) Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: BSD-3-Clause
 ***************************************************************************/
#include "checksum_top.hpp"

#include <iostream>
#include <cstdlib>

// An item folded to 23 bits: the XOR of its 23-bit slices from the LSB
template<unsigned IW>
static ap_uint<23> fold(ap_uint<IW> const &v) {
	ap_uint<23>  w = 0;
	for(unsigned  k = 0; 23*k < IW; k++)  w ^= ap_uint<23>(v >> (23*k));
	return  w;
}

template<unsigned IW>
static ap_uint<IW> random_item() {
	ap_uint<IW>  v = 0;
	for(unsigned  k = 0; k < IW; k += 16)  v = (v << 16) | ap_uint<IW>(rand() & 0xFFFF);
	return  v;
}

// Streams NUM_FRAMES frames through the kernel and checks the forwarded data
// and the last frame's checksum.
template<unsigned WPF, unsigned IPW, unsigned IW, typename TT, typename Kernel>
static bool test(char const *name, Kernel &&kernel) {
	constexpr unsigned  N = NUM_FRAMES * WPF;

	hls::stream<TT>  src("src"), dst("dst");
	ap_uint<32>  chk;
	ap_uint<1>   drain = 0;

	// Feed all frames and compute expected checksums
	ap_uint<32>  expected_chk[NUM_FRAMES];
	static TT    expected_data[N];
	for(unsigned  r = 0; r < NUM_FRAMES; r++) {
		ap_uint<24>  sum = 0;
		for(unsigned  i = 0; i < WPF; i++) {
			TT  word = 0;
			for(unsigned  k = 0; k < IPW; k++) {
				ap_uint<IW> const  v = random_item<IW>();
				word((k+1)*IW-1, k*IW) = v;
				sum += ((IPW*i+k)%3 + 1) * fold<IW>(v);
			}
			unsigned const  idx = r * WPF + i;
			expected_data[idx] = word;
			src.write(word);
		}
		expected_chk[r] = (ap_uint<8>(r), sum);
	}

	// Call function-pipelined kernel: N input words + extra idle calls
	for(unsigned  c = 0; c < 2*N; c++)  kernel(src, dst, chk, drain);

	// Verify pass-thru data
	if(dst.size() != N) {
		std::cout << name << ": ERROR expected " << N << " output words, got " << dst.size() << std::endl;
		return  false;
	}
	for(unsigned  i = 0; i < N; i++) {
		TT  out = dst.read();
		if(out != expected_data[i]) {
			std::cout << name << ": ERROR data[" << i << "] expected " << std::hex << expected_data[i]
				<< " got " << out << std::dec << std::endl;
			return  false;
		}
	}

	// The final chk holds the last frame's checksum
	if(chk != expected_chk[NUM_FRAMES - 1]) {
		std::cout << name << ": ERROR final checksum expected " << std::hex << expected_chk[NUM_FRAMES - 1]
			<< " got " << chk << std::dec << std::endl;
		return  false;
	}
	std::cout << name << ": passed." << std::endl;
	return  true;
}

int main() {
	bool  ok = test<WORDS_PER_FRAME, ITEMS_PER_WORD, ITEM_WIDTH, T>("Checksum", Testbench_Checksum);
	ok &= test<WIDE_WORDS_PER_FRAME, WIDE_ITEMS_PER_WORD, WIDE_ITEM_WIDTH, TW>("Checksum_Wide", Testbench_Checksum_Wide);
	if(!ok)  return  1;
	std::cout << "Test passed." << std::endl;
	return  0;
}
