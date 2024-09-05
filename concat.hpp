#include "utils.hpp"

#include <hls_stream.h>
#include <ap_int.h>
#include <algorithm>


template<typename Tsrc, typename Tdst>
void convert_vector(Tsrc& src, Tdst& dst){
    for(std::size_t i = 0; i < src.size(); i++){
#pragma HLS UNROLL
        dst[i] = src[i];
    }
}

template<
	unsigned...  C,
	unsigned  N,
	typename  TO,
	typename  TI
>
void StreamingConcat(
	hls::stream<TO> &dst,
	hls::stream<TI> (&src)[N]
) {
	static_assert(sizeof...(C) == N, "Number of channel counts must match number of sources.");
	static_assert(std::min({C...}) > 0, "Cannot have a zero channel count.");
	static unsigned const  CNT_INIT[N] = { (C-2)... };
	static ap_uint<clog2(N)>                    sel = 0;
	static ap_int<1+clog2(std::max({C...})-1)>  cnt = CNT_INIT[0];
#pragma HLS reset variable=sel
#pragma HLS reset variable=cnt

#pragma HLS pipeline II=1 style=flp
	TO  y;
	if(src[sel].read_nb(y)) {
		dst.write(y);
		if(cnt >= 0)  cnt--;
		else {
			sel = (sel == N-1)? decltype(sel)(0) : decltype(sel)(sel+1);
			cnt = CNT_INIT[sel];
		}
	}

} // concat()

namespace {
	/** Recursive selector for reading from stream with represented index. */
	template<unsigned  IDX, typename... TI>
	class PackReader {};

	/** Terminating with last stream in stream pack. */
	template<unsigned  IDX, typename  T0>
	class PackReader<IDX, T0> {
	public:
		template<typename TO>
		bool read_nb(unsigned const  idx, TO &y, hls::stream<T0>& src0) {
			if(idx != IDX)  return  false;
			else {
				T0  y0;
				if(!src0.read_nb(y0))  return  false;
				convert_vector(y0, y);
				return  true;
			}
		}
	};

	/** Matching own index or recursively passing further into the pack. */
	template<unsigned  IDX, typename  T0, typename... TI>
	class PackReader<IDX, T0, TI...> {
		PackReader<IDX+1, TI...>  inner;

	public:
		template<typename TO>
		bool read_nb(unsigned const  idx, TO &y, hls::stream<T0>& src0, hls::stream<TI>&... src) {
			if(idx != IDX)  return  inner.read_nb(idx, y, src...);
			else {
				T0  y0;
				if(!src0.read_nb(y0))  return  false;
				convert_vector(y0, y);
				return  true;
			}
		}
	};
}

template<
	unsigned...  C,
	typename     TO,
	typename...  TI
>
void StreamingConcat(
	hls::stream<TO>    &dst,
	hls::stream<TI>&... src
) {
	constexpr unsigned  N = sizeof...(TI);
	static_assert(sizeof...(C) == N, "Number of channel counts must match number of sources.");
	static_assert(std::min({C...}) > 0, "Cannot have a zero channel count.");
	static unsigned const  CNT_INIT[N] = { (C-2)... };
	static ap_uint<clog2(N)>                    sel = 0;
	static ap_int<1+clog2(std::max({C...})-1)>  cnt = CNT_INIT[0];

#pragma HLS reset variable=sel
#pragma HLS reset variable=cnt
	static PackReader<0, TI...>  reader;
#pragma HLS pipeline II=1 style=flp

	TO  y;
	if(reader.read_nb(sel, y, src...)) {
		dst.write(y);
		if(cnt >= 0)  cnt--;
		else {
			sel = (sel == N-1)? decltype(sel)(0) : decltype(sel)(sel+1);
			cnt = CNT_INIT[sel];
		}
	}

} // concat()

