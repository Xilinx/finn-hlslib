/******************************************************************************
 *  Copyright (C) Advanced Micro Devices, Inc.
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
 *******************************************************************************
 * @brief	Requantization: scale + bias with rounding and saturation.
 * @author	Jakoba Petri-Koenig <jakoba.petri-koenig@amd.com>
 * @author	Thomas B. Preusser <thomas.preusser@amd.com>
 ******************************************************************************/

#ifndef REQUANT_HPP
#define REQUANT_HPP

#include "tensor.hpp"
#include <hls_stream.h>
#include <hls_math.h>

/**
 * Per-channel requantization parameters.
 *
 * Layout [PE][NF] matches ThresholdsActivation convention: PE is
 * the first index so that ARRAY_PARTITION on dim=1 yields full
 * parallelism across the PE lanes.
 *
 * @tparam NF   Number of channel folds (NumChannels / PE)
 * @tparam PE   Number of channels processed in parallel
 */
template<unsigned NF, unsigned PE>
struct RequantParams {
	float  scales[PE][NF];
	float  biases[PE][NF];
};

/**
 * Requantization function (function-pipelined).
 *
 * Computes  out = clip(round(in * scale + bias), MIN, MAX)
 * for each PE lane independently.
 *
 * @tparam NumChannels  Total number of channels
 * @tparam PE           Number of channels processed in parallel
 * @tparam MIN          Saturation minimum (inclusive)
 * @tparam MAX          Saturation maximum (inclusive)
 * @tparam TI           Input element type
 * @tparam TO           Output element type
 *
 * @param in            Input stream
 * @param out           Output stream
 * @param params        Per-channel scale and bias parameters
 */
template<
	unsigned NumChannels, unsigned PE,
	int MIN, int MAX,
	typename TI, typename TO>
void Requant_Batch(
	hls::stream<tensor<TI, PE>> &in,
	hls::stream<tensor<TO, PE>> &out,
	RequantParams<NumChannels/PE, PE> const &params
) {
#pragma HLS pipeline II=1 style=flp
	constexpr unsigned  NF = NumChannels / PE;
	static unsigned  nf = 0;
#pragma HLS reset variable=nf

	if(!in.empty()) {
		auto const  x = in.read();
		tensor<TO, PE>  y;
		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
			float  scaled = float(x[pe]) * params.scales[pe][nf]
			              + params.biases[pe][nf];
			int  rounded = int(hls::lrintf(scaled));
			if(rounded < MIN)  rounded = MIN;
			if(rounded > MAX)  rounded = MAX;
			y[pe] = TO(rounded);
		}
		out.write(y);
		if(++nf == NF)  nf = 0;
	}
}

#endif // REQUANT_HPP
