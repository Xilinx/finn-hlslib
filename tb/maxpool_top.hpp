#include "maxpool.hpp"

using  T = tensor<short, 7>;
constexpr size_t  K  = 5;
constexpr size_t  CF = 3;


void maxpool_top(
	hls::stream<T> &src,
	hls::stream<T> &dst
);
