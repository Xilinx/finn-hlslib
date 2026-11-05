#include "checksum_top.hpp"

#include <iostream>
#include <cstdlib>

int main() {
	constexpr unsigned  N = NUM_FRAMES * WORDS_PER_FRAME;

	hls::stream<T>  src("src"), dst("dst");
	ap_uint<32>  chk;
	ap_uint<1>   drain = 0;

	// Feed all frames and compute expected checksums
	ap_uint<32>  expected_chk[NUM_FRAMES];
	T            expected_data[N];
	for(unsigned  r = 0; r < NUM_FRAMES; r++) {
		ap_uint<24>  sum = 0;
		for(unsigned  i = 0; i < WORDS_PER_FRAME; i++) {
			T  word = 0;
			for(unsigned  k = 0; k < ITEMS_PER_WORD; k++) {
				ap_uint<ITEM_WIDTH>  v = (rand() >> 7) & ((1 << ITEM_WIDTH) - 1);
				word((k+1)*ITEM_WIDTH-1, k*ITEM_WIDTH) = v;
				sum += ((ITEMS_PER_WORD*i+k)%3 + 1) * v;
			}
			unsigned const  idx = r * WORDS_PER_FRAME + i;
			expected_data[idx] = word;
			src.write(word);
		}
		expected_chk[r] = (ap_uint<8>(r), sum);
	}

	// Call function-pipelined kernel: N input words + extra idle calls
	for(unsigned  c = 0; c < 2*N; c++) {
		Testbench_Checksum(src, dst, chk, drain);
	}

	// Verify pass-thru data
	if(dst.size() != N) {
		std::cout << "ERROR: expected " << N << " output words, got " << dst.size() << std::endl;
		return  1;
	}
	for(unsigned  i = 0; i < N; i++) {
		T  out = dst.read();
		if(out != expected_data[i]) {
			std::cout << "ERROR data[" << i << "] expected " << std::hex << expected_data[i]
				<< " got " << out << std::dec << std::endl;
			return  1;
		}
	}

	// The final chk holds the last frame's checksum
	if(chk != expected_chk[NUM_FRAMES - 1]) {
		std::cout << "ERROR final checksum expected " << std::hex << expected_chk[NUM_FRAMES - 1]
			<< " got " << chk << std::dec << std::endl;
		return  1;
	}

	std::cout << "Test passed." << std::endl;
	return  0;
}
