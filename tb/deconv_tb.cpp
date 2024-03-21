#include "deconv_top.hpp"

#include <iostream>
#include <iomanip>


int main() {
	hls::stream<hls::vector<TI, SIMD>>  src;
	hls::stream<TO>  dst;

	for(unsigned  h = 0; h < H; h++) {
		for(unsigned  w = 0; w < W; w++) {
			src.write(TI(h*W + w));
		}
	}

	unsigned  cnt = 0;
	unsigned  timeout = 0;
	while(timeout < 200) {
		deconv_top(src, dst);
		if(dst.empty())  timeout++;
		else {
			auto const  y = dst.read();
			std::cout << (cnt%CO == 0? '\t' : ':') << y;
			if(++cnt == CO*S*(W-K/S+1)) {
				std::cout << std::endl;
				cnt = 0;
			}
			timeout = 0;
		}
	}
}
