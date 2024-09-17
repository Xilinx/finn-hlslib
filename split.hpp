/******************************************************************************
 *  Copyright (c) 2024, Advanced Micro Devices, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1.  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2.  Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *  3.  Neither the name of the copyright holder nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/*******************************************************************************
 *
 *  Authors: Michal Danilowicz <danilowi@agh.edu.pl>     
 *
 *  \file split.hpp
 *
 *  This file defines template function
 *	of a channel split operation.
 *
 *******************************************************************************/

#include "utils.hpp"

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
