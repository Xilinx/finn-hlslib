/******************************************************************************
 * Copyright (c) 2019, Xilinx, Inc.
 * Copyright (c) 2025, AMD, Inc.
 * All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  3. Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
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
 ****************************************************************************
 * @author	Giulio Gambardella <giuliog@xilinx.com>
 * @author	Felix Jentzsch <felixj@xilinx.com>
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 *****************************************************************************/

#include "pool_top.hpp"
#include "../pool.hpp"

#include "../interpret.hpp"
#include "../flatten.hpp"
#include "../streamtools.h"
#include "../slidingwindow.h"

template<size_t  CNT, typename  T, size_t  N>
void flatten_stream(
	hls::stream<hls::vector<T, N>>       &src,
	hls::stream<ap_uint<N * width_v<T>>> &dst
) {
	for(size_t  i = 0; i < CNT; i++) {
#pragma HLS pipeline II=1 style=frp
		dst.write(flatten(src.read()));
	}
}

template<size_t  CNT, typename  T, size_t  N>
void unflatten_stream(
	hls::stream<ap_uint<N * width_v<T>>> &src,
	hls::stream<hls::vector<T, N>>       &dst
) {
	for(size_t  i = 0; i < CNT; i++) {
#pragma HLS pipeline II=1 style=frp
		hls::vector<T, N>  vec;
		unflatten(vec, src.read());
		dst.write(vec);
	}
}


void pool_top(
	hls::stream<pix_t> &src,
	hls::stream<pix_t> &dst
) {
#pragma HLS interface AXIS port=src
#pragma HLS interface AXIS port=dst
#pragma HLS interface ap_ctrl_none port=return

#pragma HLS dataflow disable_start_propagation

	constexpr unsigned  VW = width_v<val_t>;
	constexpr unsigned  HO = 1+(H-K)/S;
	constexpr unsigned  WO = 1+(W-K)/S;
	hls::stream<ap_uint<C  * VW>> src_flat;
	hls::stream<ap_uint<PE * VW>> fold_flat;
	hls::stream<ap_uint<PE * VW>> swg_flat;
	hls::stream<vec_t> swg;
	hls::stream<vec_t> pooled;
	hls::stream<ap_uint<PE * VW>> pooled_flat;
	hls::stream<ap_uint<C  * VW>> dst_flat;
#pragma HLS stream depth=2 variable=src_flat
#pragma HLS stream depth=2 variable=fold_flat
#pragma HLS stream depth=2 variable=swg_flat
#pragma HLS stream depth=2 variable=swg
#pragma HLS stream depth=2 variable=pooled
#pragma HLS stream depth=2 variable=pooled_flat
#pragma HLS stream depth=2 variable=dst_flat

	flatten_stream<H*W>(src, src_flat);
	StreamingDataWidthConverter_Batch<C*VW, PE*VW, H*W>(src_flat, fold_flat, 1);
	ConvolutionInputGenerator_kernel_stride_dws<K, C, VW, H, HO, PE, S>(
		fold_flat, swg_flat, 1, ap_resource_dflt()
	);
	unflatten_stream<HO*WO*(C/PE)*K*K>(swg_flat, swg);

	MaxPoolFunction<vec_t> maxpool_fxn;
	Pool_batch<HO*WO*(C/PE)*K*K, K*K>(swg, pooled, maxpool_fxn);

	flatten_stream<HO*WO*(C/PE)>(pooled, pooled_flat);
	StreamingDataWidthConverter_Batch<PE*VW, C*VW, HO*WO*(C/PE)>(pooled_flat, dst_flat, 1);
	unflatten_stream<HO*WO>(dst_flat, dst);
}
