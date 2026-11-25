#include "thresholding_top.hpp"
#include "thresholding_ref.h"

#include <iostream>
#include <random>

int main() {
	hls::stream<tensor<TI,  PE>>  src("src");
	hls::stream<tensor<TO, PE>>  dst("dst");

	// Stimuli: random values mixed with thresholds and their neighbors
	std::mt19937  gen(1);
	static TO  expected[NIN][PE];
	for(unsigned  i = 0; i < NIN; i++) {
		unsigned const  cf = i % CF;
		tensor<TI, PE>  x;
		for(unsigned  pe = 0; pe < PE; pe++) {
			unsigned const  ch = cf*PE + pe;
			TI  v;
			switch(gen() % 4) {
			case 0:
			case 1:
				v = rand_input(gen);
				break;
			default:
				v = perturb(THRESHS[ch][gen() % N], int(gen() % 3) - 1);
				break;
			}
			x[pe] = v;

			int  cnt = BIAS;
			for(unsigned  t = 0; t < N; t++)  cnt += THRESHS[ch][t] <= v;
			expected[i][pe] = TO(cnt);
		}
		src.write(x);
	}

	// One kernel invocation per input
	for(unsigned  i = 0; i < NIN; i++) {
		thresholding_top(src, dst);
	}

	// Verify
	unsigned  errors = 0;
	for(unsigned  i = 0; i < NIN; i++) {
		auto const  y = dst.read();
		for(unsigned  pe = 0; pe < PE; pe++) {
			TO const  got = y[pe];
			if(got != expected[i][pe]) {
				if(errors++ < 10) {
					std::cout << "ERROR input " << i << " pe " << pe
						<< " expected " << expected[i][pe] << " got " << got << std::endl;
				}
			}
		}
	}
	if(!dst.empty())  std::cout << "ERROR: surplus output" << std::endl;

	if(errors) {
		std::cout << errors << " errors detected." << std::endl;
		return  1;
	}
	std::cout << "Test passed. " << NIN << " inputs verified (N=" << N << ", C=" << C << ", PE=" << PE << ")." << std::endl;
	return  0;
}
