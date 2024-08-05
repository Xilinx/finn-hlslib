#include "util.hpp"

#include <hls_stream.h>
#include <ap_int.h>
#include <algorithm>


template<
    unsigned... C,
    typename TI,
    unsigned N
>
void StreamingSplit(
    hls::stream<TI>& src,
    hls::stream<TI> (&dst)[N]
){
    static ap_uint<clog2(N)> sel = 0;
    static unsigned const CNT_INIT[N] = {(C-2)...};
    static ap_int<1+clog2(std::max({C...})-1)>  cnt = CNT_INIT[0];

#pragma HLS reset variable=sel
#pragma HLS reset variable=cnt
#pragma HLS pipeline II=1 style=flp
 
    TI x;
    if(!src.empty()){
        x = src.read();
        dst[sel].write(x);
        if(cnt >= 0)
            cnt--;
        else{
            sel = (sel == N-1) ? decltype(sel)(0) : decltype(sel)(sel+1);
            cnt = CNT_INIT[sel]; 
        }
    }
}