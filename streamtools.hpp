/******************************************************************************
 *  Copyright (c) 2019, Xilinx, Inc.
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
 
/******************************************************************************
 *
 *  Authors: Giulio Gambardella <giuliog@xilinx.com>
 *           Thomas B. Preusser <thomas.preusser@utexas.edu>
 *             Marie-Curie Fellow, Xilinx Ireland, Grant Agreement No. 751339
 *           Christoph Doehring <cdoehrin@xilinx.com>
 *           Lukas Stasytis <lukas.stasytis@amd.com>
 *
 *  @file stream-tools.h
 *
 *  Library of templated HLS functions for BNN deployment. 
 *  This file lists a set of convenience funtions used to adapt stream size, 
 *  remove unnecessary streams (padding) and casting
 *
 ******************************************************************************/

#ifndef STREAMTOOLS_HPP
#define STREAMTOOLS_HPP

#include "ap_axi_sdata.h"
#include "tensor.hpp"

/**
 * \brief Feature map pixel padding - Pads each pixel in the input feature
 *        map with zeros. Used as a pre-processing step for the transposed
 * 		  convolution operation. Expects data in NHWC format, where N=1.
 *
 * \tparam OutputDim_x Padded width of the output feature map
 * \tparam OutputDim_y Padded height of the output feature map
 * \tparam Stride_x    Stride for each pixel along the width dimension 
 * \tparam Stride_y    Stride for each pixel along the height dimension
 * \tparam NumChannels Number of channels of the input feature map
 * \tparam SIMD		   Input parallelism 
 * \tparam In_t		   Input datatype
 *
 * @param src          Input stream
 * @param dst 		   Output stream
 */
template<
	unsigned OutputDim_x,
	unsigned OutputDim_y,
	unsigned Stride_x,
	unsigned Stride_y,
	unsigned NumChannels,
	unsigned SIMD,
	typename In_t
>
void FMPadding_Pixel_Nonsquare(
	hls::stream<tensor<In_t, SIMD>> &src,
	hls::stream<tensor<In_t, SIMD>> &dst
) {
	static_assert(NumChannels % SIMD == 0, "SIMD must divide channel count.");
	constexpr unsigned  Folding = NumChannels/SIMD;

	int unsigned  ytrig = 0;
	for(int unsigned  y = 0; y < OutputDim_y; y++) {
		int unsigned  xtrig = 0;
		for(int unsigned  x = 0; x < OutputDim_x; x++) {
			for(int unsigned  sf = 0; sf < Folding; sf++) {
#pragma HLS pipeline II=1 style=flp
				tensor<In_t, SIMD>  value(0);
				if((ytrig == 0) && (xtrig == 0))  value = src.read();
				dst.write(value);
			}
			if(++xtrig == Stride_x)  xtrig = 0;
		}
		if(++ytrig == Stride_y)  ytrig = 0;
	}
}

/**
 * \brief   Stream Data Width Converter - Converts the width of the input stream in the output stream
 *
 * Used to upscale or downscale a stream, without any loss of data in the procedure. 
 * For downscaling (InWidth > OutWidth), InWidth has to be a multiple of OutWidth.
 * For upscaling (InWidth < OutWidth), OutWidth has to be a multiple of InWidth.
 *
 * \tparam     InWidth      Width, in number of bits, of the input stream
 * \tparam     OutWidth     Width, in number of bits, of the output stream 
 * \tparam     NumInWords   Number of input words to process
 *
 * \param      in           Input stream
 * \param      out          Output stream
 * \param      numReps      Number of times the function has to be called
 *
 */
template<unsigned int InWidth,		
		unsigned int OutWidth,		
		unsigned int NumInWords		
>
void StreamingDataWidthConverter_Batch(hls::stream<ap_uint<InWidth>> & in,
		hls::stream<ap_uint<OutWidth>> & out, const unsigned int numReps) {
  static_assert((InWidth % OutWidth == 0) || (OutWidth % InWidth == 0), "");

  if (InWidth > OutWidth) {
    // emit multiple output words per input word read
    const unsigned int outPerIn = InWidth / OutWidth;
    const unsigned int totalIters = NumInWords * outPerIn * numReps;
    unsigned int o = 0;
    ap_uint<InWidth> ei = 0;
    for (unsigned int t = 0; t < totalIters; t++) {
#pragma HLS pipeline style=flp II=1
      // read new input word if current out count is zero
      if (o == 0) {
        ei = in.read();
	  }
      // pick output word from the rightmost position
      ap_uint<OutWidth> eo = ei(OutWidth - 1, 0);
      out.write(eo);
      // shift input to get new output word for next iteration
      ei = ei >> OutWidth;
      // increment written output count
      o++;
      // wraparound indices to recreate the nested loop structure
      if (o == outPerIn) {
        o = 0;
      }
    }
  } else if (InWidth == OutWidth) {
    // straight-through copy
    for (unsigned int i = 0; i < NumInWords * numReps; i++) {
#pragma HLS pipeline style=flp II=1
      ap_uint<InWidth> e = in.read();
      out.write(e);
    }
  } else { // InWidth < OutWidth
    // read multiple input words per output word emitted
    const unsigned int inPerOut = OutWidth / InWidth;
    const unsigned int totalIters = NumInWords * numReps;
    unsigned int i = 0;
    ap_uint<OutWidth> eo = 0;
    for (unsigned int t = 0; t < totalIters; t++) {
#pragma HLS pipeline style=flp II=1
      // read input and shift into output buffer
      ap_uint<InWidth> ei = in.read();
      eo = eo >> InWidth;
      eo(OutWidth - 1, OutWidth - InWidth) = ei;
      // increment read input count
      i++;
      // wraparound logic to recreate nested loop functionality
      if (i == inPerOut) {
        i = 0;
        out.write(eo);
      }
    }
  }
}

/**
 * \brief   QDMA stream to normal stream conversion - Reads in a QDMA stream and strips metadata (TLAST, TKEEP)
 *
 * Used as an adapter when connecting blocks through top-level Vitis streams (kernel to kernel or host to plaform streaming)
 *
 * \tparam     DataWidth    Width, in number of bits, of the data on streams
 * \tparam     NumTotal     Total number of words in the input stream
 *
 * \param      in           Input stream
 * \param      out          Output stream
 * \param      numReps      Number of frames / images
 *
 */
template<unsigned int DataWidth, unsigned int NumTotal>
void Qdma2Stream_Batch(hls::stream<qdma_axis<DataWidth,0,0,0>> & in, hls::stream<ap_uint<DataWidth>> & out, const unsigned int numReps){
	//TODO: static_assert to ensure DataWidth is power of 2 between 8 and 512
	for (unsigned int image = 0; image < numReps; image++) {
		for (unsigned int word = 0; word < NumTotal; word++) {
#pragma HLS pipeline style=flp II=1
			out.write(in.read().get_data());
		}
	}
}

/**
 * \brief   Normal stream to QDMA stream conversion - Reads in a stream and outputs a QDMA stream including metadata (TLAST, TKEEP)
 *
 * Used as an adapter when connecting blocks through top-level Vitis streams (kernel to kernel or host to plaform streaming)
 *
 * \tparam     DataWidth    Width, in number of bits, of the data on streams
 * \tparam     NumTotal     Total number of words in the input stream
 *
 * \param      in           Input stream
 * \param      out          Output stream
 * \param      numReps      Number of frames / images
 *
 */
template<unsigned int DataWidth, unsigned int NumTotal>
void Stream2Qdma_Batch(hls::stream<ap_uint<DataWidth>> & in, hls::stream<qdma_axis<DataWidth,0,0,0>> & out, const unsigned int numReps){
	for (unsigned int image = 0; image < numReps; image++) {
		for (unsigned int word = 0; word < NumTotal; word++) {
#pragma HLS pipeline style=flp II=1
			qdma_axis<DataWidth,0,0,0> temp;
			temp.set_data(in.read());
			temp.set_keep(-1);
			temp.set_last(word == NumTotal-1);
			out.write(temp);
		}
	}
}

#endif