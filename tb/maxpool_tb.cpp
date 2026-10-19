#include "maxpool_top.hpp"


constexpr size_t  ROUNDS = 5;
constexpr size_t  N = 7;	// tensor dimension from T = tensor<short, 7>

int main() {
	hls::stream<T>  src;
	hls::stream<T>  dst;

	// Feed stimulus
	size_t  cnt = 0;
	for(size_t  i = 0; i < ROUNDS*K*CF; i++) {
		T  y;
		for(size_t  j = 0; j < N; j++) {
			y[j] = short(cnt++);
		}
		src.write(y);
	}

	unsigned  timeout = 0;
	while(timeout < CF*K+6) {
		maxpool_top(src, dst);
		if(dst.empty())  timeout++;
		else {
			T const  y = dst.read();
			std::cout << "> " << y << std::endl;
			timeout = 0;
		}
	}
}
