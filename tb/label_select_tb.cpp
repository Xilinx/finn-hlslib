#include <iostream>
#include <numeric>
#include <algorithm>
#include <hls_stream.h>
#include <ap_int.h>

#include "label_select_top.hpp"


constexpr unsigned  BATCH = 2;

int main() {
	tensor<TI, N> const  idat = tensor<TI, N>{
		-12043, 30370, -23559, -30747, -11173,
		 30686, 19723,  -8171, -16169,    809
	};
	// Reference: indices sorted by descending input value
	tensor<TO, N>  oref;
	std::iota(oref.begin(), oref.end(), TO(0));
	std::sort(oref.begin(), oref.end(), [&idat](TO a, TO b) { return  idat[a] > idat[b]; });

	hls::stream<src_t>  src("src");
	hls::stream<dst_t>  dst("dst");

	// Slice input tensor into PE-wide words
	for(unsigned  batch = 0; batch < BATCH; batch++) {
		for(unsigned  f = 0; f < N; f += PE) {
			src.write(idat.slice<PE>(f));
		}
	}

	// Run DUT and verify inline
	constexpr unsigned  EXPECTED_OUTPUTS = BATCH * K;
	unsigned  outputs_received = 0;
	unsigned  timeout = 0;
	unsigned  err_cnt = 0;
	while(timeout < 200) {
		Testbench_label_select(src, dst);

		if(dst.empty()) {
			timeout++;
			continue;
		}

		timeout = 0;
		TO const  actual = dst.read()[0];
		TO const  expected = oref[outputs_received % K];
		if(actual != expected) {
			std::cout << "ERROR: batch " << outputs_received / K << " top " << outputs_received % K + 1
				<< " expected " << expected << " got " << actual << std::endl;
			err_cnt++;
		}

		outputs_received++;
		if(outputs_received == EXPECTED_OUTPUTS)  break;
	}

	std::cout
		<< "Received " << outputs_received << " of " << EXPECTED_OUTPUTS << " outputs, "
		<< err_cnt << " errors."
		<< std::endl;

	if(outputs_received != EXPECTED_OUTPUTS) {
		std::cerr << "ERROR: Timed out waiting for outputs." << std::endl;
		return  1;
	}
	return  err_cnt? 1 : 0;
}
