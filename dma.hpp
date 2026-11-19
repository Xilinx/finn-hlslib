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
 *
 *  \file dma.hpp
 *
 *  Library of templated HLS functions for DMA access.
 *  This file lists a set of functions to access memory mapped values into
 *  streams.
 *
 *****************************************************************************/
#ifndef DMA_HPP
#define DMA_HPP

#include <hls_stream.h>
#include "ap_axi_sdata.h"
#include "utils.hpp"


template<typename T, unsigned numBytes>
void Mem2Stream(T *in, hls::stream<T> &out) {
  constexpr unsigned  numWords = numBytes / (width_v<T> / 8);
  static_assert(width_v<T> % 8 == 0, "");
  static_assert(numWords != 0, "");
  for (unsigned i = 0; i < numWords; i++) {
#pragma HLS pipeline style=flp II=1
    out.write(in[i]);
  }
}

template<typename T, unsigned numBytes>
void Stream2Mem(hls::stream<T> &in, T *out) {
  constexpr unsigned  numWords = numBytes / (width_v<T> / 8);
  static_assert(width_v<T> % 8 == 0, "");
  static_assert(numWords != 0, "");
  for (unsigned i = 0; i < numWords; i++) {
#pragma HLS pipeline style=flp II=1
    out[i] = in.read();
  }
}

template<typename T, unsigned numBytes>
void Mem2Stream_Batch(T *in, hls::stream<T> &out, unsigned const numReps) {
  constexpr unsigned  indsPerRep = numBytes / (width_v<T> / 8);
  unsigned rep = 0;
  // make sure Mem2Stream does not get inlined here
  // we lose burst inference otherwise
  while (rep != numReps) {
    unsigned  repsLeft = numReps - rep;
    if ((repsLeft & 0xF) == 0) {
      Mem2Stream<T, numBytes * 16>(&in[rep * indsPerRep], out);
      rep += 16;
    } else {
      Mem2Stream<T, numBytes>(&in[rep * indsPerRep], out);
      rep += 1;
    }
  }
}

template<typename T, unsigned numBytes>
void Stream2Mem_Batch(hls::stream<T> &in, T *out, unsigned const numReps) {
  constexpr unsigned  indsPerRep = numBytes / (width_v<T> / 8);
  unsigned rep = 0;
  // make sure Stream2Mem does not get inlined here
  // we lose burst inference otherwise
  while (rep != numReps) {
    unsigned  repsLeft = numReps - rep;
    if ((repsLeft & 0xF) == 0) {
      Stream2Mem<T, numBytes * 16>(in, &out[rep * indsPerRep]);
      rep += 16;
    } else {
      Stream2Mem<T, numBytes>(in, &out[rep * indsPerRep]);
      rep += 1;
    }
  }
}

template<typename T, unsigned numBytes>
void Mem2Stream_Batch_external_wmem(T *in, hls::stream<T> &out, unsigned const numReps) {
  unsigned rep = 0;
  while (rep != numReps) {
    Mem2Stream<T, numBytes>(&in[0], out);
    rep += 1;
  }
}

template<typename T, unsigned NumTotal>
void Qdma2Stream_Batch(hls::stream<qdma_axis<width_v<T>,0,0,0>> &in, hls::stream<T> &out, unsigned const numReps) {
  for (unsigned image = 0; image < numReps; image++) {
    for (unsigned word = 0; word < NumTotal; word++) {
#pragma HLS pipeline style=flp II=1
      out.write(in.read().get_data());
    }
  }
}

template<typename T, unsigned NumTotal>
void Stream2Qdma_Batch(hls::stream<T> &in, hls::stream<qdma_axis<width_v<T>,0,0,0>> &out, unsigned const numReps) {
  for (unsigned image = 0; image < numReps; image++) {
    for (unsigned word = 0; word < NumTotal; word++) {
#pragma HLS pipeline style=flp II=1
      qdma_axis<width_v<T>,0,0,0>  temp;
      temp.set_data(in.read());
      temp.set_keep(-1);
      temp.set_last(word == NumTotal-1);
      out.write(temp);
    }
  }
}

#endif
