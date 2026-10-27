#include "dwc_top.hpp"

#include <iostream>

int main() {
	hls::stream<tensor<T, N1>>  in("in"), out("out");

	// Feed input
	for(unsigned  i = 0; i < NumInWords; i++) {
		tensor<T, N1>  v;
		for(unsigned  e = 0; e < N1; e++)  v[e] = T(i * N1 + e);
		in.write(v);
	}

	// Call kernel enough times to drain pipeline
	for(unsigned  i = 0; i < 4 * NumInWords; i++)
		Testbench_dwc(in, out);

	// Verify identity
	for(unsigned  i = 0; i < NumInWords; i++) {
		tensor<T, N1>  expected;
		for(unsigned  e = 0; e < N1; e++)  expected[e] = T(i * N1 + e);
		tensor<T, N1>  got = out.read();
		if(got != expected) {
			std::cout << "ERROR at " << i
				<< " expected " << expected << " got " << got << std::endl;
			return  1;
		}
	}
	std::cout << "Test passed." << std::endl;
	return  0;
}
