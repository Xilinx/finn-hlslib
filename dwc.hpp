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
#ifndef DWC_HPP
#define DWC_HPP

#include <ap_int.h>
#include <hls_stream.h>

/**
 * \brief   Stream Data Width Converter (function-pipelined)
 *
 * Used to upscale or downscale a stream, without any loss of data in the procedure.
 * For downscaling (InWidth > OutWidth), InWidth has to be a multiple of OutWidth.
 * For upscaling (InWidth < OutWidth), OutWidth has to be a multiple of InWidth.
 *
 * \tparam     InWidth      Width, in number of bits, of the input stream
 * \tparam     OutWidth     Width, in number of bits, of the output stream
 *
 * \param      in           Input stream
 * \param      out          Output stream
 */
template<unsigned int InWidth, unsigned int OutWidth>
void StreamingDataWidthConverter(hls::stream<ap_uint<InWidth>> &in,
		hls::stream<ap_uint<OutWidth>> &out) {
  static_assert((InWidth % OutWidth == 0) || (OutWidth % InWidth == 0), "");

#pragma HLS pipeline II=1 style=flp

  if constexpr(InWidth > OutWidth) {
    // Emit multiple output words per input word read
    constexpr unsigned  outPerIn = InWidth / OutWidth;
    static ap_uint<InWidth>  ei = 0;
    static unsigned  o = 0;
#pragma HLS reset variable=o

    if(o == 0) {
      if(in.empty())  return;
      ei = in.read();
    }
    out.write(ei(OutWidth - 1, 0));
    ei >>= OutWidth;
    if(++o == outPerIn)  o = 0;

  } else if constexpr(InWidth == OutWidth) {
    // Straight-through copy
    if(!in.empty())  out.write(in.read());

  } else {
    // Read multiple input words per output word emitted
    constexpr unsigned  inPerOut = OutWidth / InWidth;
    static ap_uint<OutWidth>  eo = 0;
    static unsigned  i = 0;
#pragma HLS reset variable=i

    if(!in.empty()) {
      eo >>= InWidth;
      eo(OutWidth - 1, OutWidth - InWidth) = in.read();
      if(++i == inPerOut) {
        i = 0;
        out.write(eo);
      }
    }
  }
}

#endif
