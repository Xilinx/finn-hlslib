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
 *           Felix Jentzsch <felixj@xilinx.com>
 *           Jonas Kuehle <jonas.kuehle@cs.hs-fulda.de>
 *
 *
 *  Library of templated HLS functions for QNN deployment. 
 *
 ******************************************************************************/

#ifndef MAXPOOL_H
#define MAXPOOL_H
 
#include <limits>

#include "interpret.hpp"
#include "utils.hpp"

/**
 * \brief   Max Pool implementation for non Binarized values
 *
 * This function performes the maxpool for non-binary inputs, and works with kernel and stride being equal
 *
 * \tparam ImgDim       Width and Heigth of the Input Feature Map (assumed square)
 * \tparam PoolDim      Dimension of the Max Pool kernel (assumed square)
 * \tparam NumChannels  Number of Input Feature Maps
 * \tparam ActType      DataType of the input activation (as used in the comparison)
 * \tparam min_value    Minimum value possible with the given ActType, used to initialize the value before the comparison
 * \tparam StreamW      Width of the input and output stream
 *
 * \param in            Input stream
 * \param out           Output stream
 *
 */
template<unsigned int ImgDim, unsigned int PoolDim, unsigned int NumChannels, typename ActType, int min_value>
void StreamingMaxPool(hls::stream<hls::vector<ActType, NumChannels> > & in,
        hls::stream<hls::vector<ActType, NumChannels> > & out) {
  static_assert(ImgDim % PoolDim == 0, "");
  // need buffer space for a single maxpooled row of the image
  ActType buf[ImgDim / PoolDim][NumChannels];
#pragma HLS ARRAY_PARTITION variable=buf complete dim=2
  for(unsigned int i = 0; i < ImgDim / PoolDim; i++) {
    for(unsigned int ch = 0; ch<NumChannels; ch++){
#pragma HLS UNROLL
      buf[i][ch] = min_value; // replace with std::numeric_limits<ActType>::min() as soon as implmented
    }
  }
  hls::vector<ActType, NumChannels> inputData,outputData;
  for (unsigned int yp = 0; yp < ImgDim / PoolDim; yp++) {
    for (unsigned int ky = 0; ky < PoolDim; ky++) {
      for (unsigned int xp = 0; xp < ImgDim / PoolDim; xp++) {
        // Change to comparator
        for (unsigned int kx = 0; kx < PoolDim; kx++) {
#pragma HLS pipeline style=flp II=1
          inputData = in.read();
          for(unsigned int ch = 0; ch<NumChannels; ch++){
#pragma HLS UNROLL
            ActType channeldata = inputData[ch];
            ActType oldMax = buf[xp][ch];
            if(channeldata > oldMax){
              buf[xp][ch] = channeldata;
            }
          }
        }
      }
    }
    for (unsigned int outpix = 0; outpix < ImgDim / PoolDim; outpix++) {
      for(unsigned int ch = 0; ch < NumChannels; ch++){
#pragma HLS UNROLL
        outputData[ch] = buf[outpix][ch];
        // get buffer ready for next use
        buf[outpix][ch] = min_value;
      }
      out.write(outputData);
    }
  }
}

/**
 * \brief   Max Pool implementation for non binarized values on multiple images
 *
 * This function performes the maxpool for non binary inputs, and works with kernel and stride being equal
 *
 * \tparam ImgDim       Width and Heigth of the Input Feature Map (assumed square)
 * \tparam PoolDim      Dimension of the Max Pool kernel (assumed square)
 * \tparam NumChannels  Number of Input Feature Maps
 * \tparam ActType      DataType of the input activation (as used in the comparison)
 * \tparam min_value    Minimum value possible with the given ActType, used to initialize the value before the comparison
 *
 * \param in            Input stream
 * \param out           Output stream
 * \param numReps       Number of time the function has to be repeatedly executed (e.g. number of images)
 *
 */
template<unsigned int ImgDim, unsigned int PoolDim, unsigned int NumChannels, typename ActType, int min_value>
void StreamingMaxPool_Batch(hls::stream<hls::vector<ActType, NumChannels> > & in,
        hls::stream<hls::vector<ActType, NumChannels> > & out, unsigned int numReps) {
#pragma HLS INLINE
  unsigned const  InpPerImage = ImgDim*ImgDim;
  unsigned const  OutPerImage = ImgDim*ImgDim / (PoolDim*PoolDim);
  hls::stream<hls::vector<ActType, NumChannels> > wa_in("StreamingMaxPool_Batch.wa_in");
  hls::stream<hls::vector<ActType, NumChannels> > mvOut("StreamingMaxPool_Batch.mvOut");
  StreamingDataWidthConverter_Batch<InpPerImage, ActType, ActType, NumChannels, NumChannels>(in, wa_in, numReps);
  for (unsigned int rep = 0; rep < numReps; rep++) {
    StreamingMaxPool<ImgDim, PoolDim, NumChannels, ActType, min_value>
      (wa_in, mvOut);
  }
  StreamingDataWidthConverter_Batch<OutPerImage, ActType, ActType, NumChannels, NumChannels>(mvOut, out, numReps);
}


/**
 * \brief   1D Max Pool implementation for non Binarized values
 *
 * This function performes the maxpool for non-binary inputs, and works with kernel and stride being equal
 *
 * \tparam ImgDim        Length of the Input Feature Map
 * \tparam PoolDim       Dimension of the Max Pool kernel
 * \tparam NumChannels   Number of Input Feature Maps
 * \tparam PE            Number of input rows (channels) computed in parallel
 * \tparam OutputSize    Length of the Output Feature Map
 * \tparam ActType       DataType of the input activation (as used in the comparison)
 * \tparam min_value     Minimum value possible with the given ActType, used to initialize the value before the comparison
 *
 * \param in             Input stream
 * \param out            Output stream
 *
 */

template<unsigned int ImgDim, unsigned int PoolDim, unsigned int NumChannels, unsigned int PE,
        unsigned int OutputSize, typename ActType, int min_value
>
void StreamingMaxPool_1d(hls::stream<hls::vector<ActType, PE> > & in,
        hls::stream<hls::vector<ActType, PE> > & out) {
  static_assert(NumChannels % PE == 0, "");
  constexpr unsigned NF = NumChannels / PE;
  constexpr unsigned REMAINDER_PIXELS = ImgDim > PoolDim * OutputSize ? ImgDim - OutputSize * PoolDim : 0;

  // need buffer space for a single maxpooled pixel of the image
  ActType buf[NF][PE];
#pragma HLS ARRAY_PARTITION variable=buf complete dim=2

  for(unsigned int ch = 0; ch < NF; ch++){
#pragma HLS pipeline style=flp II=1
    for(unsigned int p = 0; p < PE; p++){
#pragma HLS UNROLL
      buf[ch][p] = min_value;
    }
  }

  hls::vector<ActType, PE> inputData,outputData;
  unsigned input_count = 0;
  for (unsigned int xp = 0; xp < OutputSize; xp++) {
    // Change to comparator
    for (unsigned int kx = 0; kx < PoolDim; kx++) {
      if (input_count++ < ImgDim){
        for (unsigned int ch = 0; ch < NF; ch++){
#pragma HLS pipeline style=flp II=1
          inputData = in.read();
          for(unsigned int p = 0; p < PE; p++){
#pragma HLS UNROLL
            ActType const channeldata = inputData[p];
            ActType const oldMax = buf[ch][p];
            if(channeldata > oldMax){
              buf[ch][p] = channeldata;
            }
          }
        }
      }
    }
    for(unsigned int ch = 0; ch < NF; ch++){
#pragma HLS pipeline style=flp II=1
      for(unsigned int p = 0; p < PE; p++){
#pragma HLS UNROLL
        outputData[p] = buf[ch][p];
        // get buffer ready for next use
        buf[ch][p] = min_value;
      }
      out.write(outputData);
    }
  }

  for (unsigned int r = 0; r < REMAINDER_PIXELS*NF; r++){
#pragma HLS pipeline style=flp II=1
    inputData = in.read();
  }

}


/**
 * \brief   1D Max Pool implementation for non binarized values on multiple images
 *
 * This function performes the maxpool for non binary inputs, and works with kernel and stride being equal
 *
 * \tparam ImgDim       Length of the Input Feature Map
 * \tparam PoolDim      Dimension of the Max Pool kernel
 * \tparam NumChannels  Number of Input Feature Maps
 * \tparam PE           Number of input rows (channels) computed in parallel
 * \tparam ActType      DataType of the input activation (as used in the comparison)
 * \tparam min_value    Minimum value possible with the given ActType, used to initialize the value before the comparison
 *
 * \param in            Input stream
 * \param out           Output stream
 * \param numReps       Number of time the function has to be repeatedly executed (e.g. number of images)
 *
 */
template<unsigned int ImgDim, unsigned int PoolDim, unsigned int NumChannels, unsigned int PE,
        unsigned int OutputSize, typename ActType, int min_value
>
void StreamingMaxPool_Batch_1d(hls::stream<hls::vector<ActType, PE> > & in,
        hls::stream<hls::vector<ActType, PE> > & out, unsigned int numReps) {
#pragma HLS INLINE
  for (unsigned int rep = 0; rep < numReps; rep++) {
    StreamingMaxPool_1d<ImgDim, PoolDim, NumChannels, PE, OutputSize,
    ActType, min_value>
      (in, out);
  }
}


/**
 * \brief   ReLU for fixed-point or integer; can accept a bias at input, which it removes
 *
 * \tparam ImgDim       Width and Heigth of the Input Feature Map (assumed square)
 * \tparam NumChannels  Number of Input Feature Maps
 * \tparam ActType      DataType of the input activation (as used in the comparison)
 * \tparam PECount      PE parallelism to apply ReLU
 * \tparam offset       Offset to be subtracted before applying ReLU
 * 
 * \param in            Input stream
 * \param out           Output stream
 * \param numReps       Number of time the function has to be repeatedly executed (e.g. number of images)
 *
 */
template<
        unsigned int ImgDim,            
    unsigned int NumChannels,  
        typename ActType,           
        unsigned int PECount,
    int offset = 0>
void ReLU_Batch(hls::stream<ap_uint<PECount * ActType::width> > & in,
        hls::stream<ap_uint<PECount * ActType::width> > & out, const unsigned int numReps) {

    ap_uint<PECount * ActType::width> thin;
    ap_uint<PECount * ActType::width> thout;
    
    //call to thresholding library function
    for(unsigned int reps=0; reps<numReps; reps++){
        for(unsigned int pixel=0; pixel<ImgDim*ImgDim; pixel++){
      for(unsigned int fold=0; fold<NumChannels/PECount; fold++){
#pragma HLS pipeline style=flp II=1
        thin = in.read();
        for(unsigned int pe=0; pe<PECount; pe++){
#pragma HLS UNROLL
          // Threshold and assign to right bits of output buffers
          unsigned int lowBit = pe * ActType::width;
          unsigned int highBit = (pe+1) * ActType::width - 1;
          ActType val = thin(highBit,lowBit);
          ActType result;
          if(val < offset)
                  result = 0;
          else
                  result = val - offset;
          thout(highBit, lowBit) = result;
        }    
        out.write(thout);
      }
        }
    }
}

/**
 * \brief   Accumulate-pool - like average pooling over the whole frame, but without the division at end
 *
 * \tparam ImgDim       Width and Heigth of the Input Feature Map (assumed square)
 * \tparam NumChannels  Number of Input Feature Maps
 * \tparam ActType      DataType of the input activation (as used in the comparison)
 * \tparam PECount      PE parallelism to apply ReLU
 * \tparam AccType      Datatype of the accumulation (e.g. output)
 * 
 * \param in            Input stream
 * \param out           Output stream
 * \param numReps       Number of time the function has to be repeatedly executed (e.g. number of images)
 *
 */
template<
    unsigned int ImgDim,     
        unsigned int NumChannels,       
        typename ActType,           
        unsigned int PECount,      
        typename AccType>
void AccPool_Batch(hls::stream<ap_uint<PECount * ActType::width> > & in,
        hls::stream<ap_uint<PECount * AccType::width> > & out, const unsigned int numReps) {
    ap_uint<PECount * ActType::width> thin;
  ap_uint<PECount * AccType::width> accumulators[NumChannels/PECount];
#pragma HLS bind_storage variable=accumulators type=RAM_2P impl=LUTRAM

    //call to thresholding library function
    for(unsigned int reps=0; reps<numReps; reps++){
        for(unsigned int pixel=0; pixel<ImgDim*ImgDim; pixel++){
      for(unsigned int fold=0; fold<NumChannels/PECount; fold++){
#pragma HLS pipeline style=flp II=1
        thin = in.read();
        ap_uint<PECount * AccType::width> accbank = accumulators[fold];
        for(unsigned int pe=0; pe<PECount; pe++){
#pragma HLS UNROLL
          // Threshold and assign to right bits of output buffers
          ActType const  val = thin((pe+1) * ActType::width - 1,pe * ActType::width);
          AccType const  acc = accbank((pe+1) * AccType::width - 1,pe * AccType::width);
          AccType const  result = val + (pixel == 0? AccType(0) : acc);
          accbank((pe+1) * AccType::width - 1,pe * AccType::width) = result;
        }
        accumulators[fold] = accbank;     
      }
        }
    for (unsigned int fold = 0; fold < NumChannels / PECount; fold++)
    {
      out.write(accumulators[fold]);
    }
    }
}



/**
 * \brief   LabelSelect_Batch - returns labels of top-NumTop in stream
 *
 * \tparam NumClasses   Number of classes of the dataset
 * \tparam PECount      Number of inputs to be processed in parallel
 * \tparam NumTop       Number of top classes to be selected in output
 * \tparam In_T         Datatype of the input
 * \tparam Out_T        Datatype of the output
 * 
 * \param in            Input stream
 * \param out           Output stream
 * \param numReps       Number of times the function has to be repeatedly executed (e.g. number of images)
 *
 */

template<
    // tensor size parameters
    unsigned int NumClasses,
    unsigned int PECount,
    unsigned int NumTop,
    typename In_T,
    typename Out_T>
void LabelSelect_Batch(hls::stream<ap_uint<PECount * In_T::width> > & in,
        hls::stream<Out_T> & out, const unsigned int numReps) {

  // Check that classes, aka. labels / indeces, can be encoded as non-negative outputs
  static_assert(clog2(NumClasses) <= Out_T::width - Out_T::sign_flag, "");
  static In_T const  In_T_MIN_VAL = (In_T(-1)<0)? 1<<(In_T::width-1) : 0;

  // Array of encountered top values
  //  - maintains topval[i] <= topval[i+1]
  //  - keeps in alignment with toplabels
  In_T topval[NumTop];
#pragma HLS ARRAY_PARTITION variable=topval complete dim=1
  Out_T toplabels[NumTop];
#pragma HLS ARRAY_PARTITION variable=toplabels complete dim=1

  for(unsigned int reps=0; reps<numReps; reps++){
    unsigned int idx = 0;
    for(unsigned int topx=0; topx<NumTop; topx++){
#pragma HLS UNROLL
      topval   [topx] = In_T_MIN_VAL;
      toplabels[topx] = 0;
    }
    for(unsigned int block=0; block<(NumClasses/PECount); block++){
#pragma HLS pipeline style=flp II=1
      ap_uint<PECount * In_T::width> const  inval = in.read();
      for(unsigned int elem=0; elem<PECount; elem++){
#pragma HLS UNROLL
        // Extract individual input
        unsigned const  lowBit = elem * In_T::width;
        unsigned const  highBit = (elem+1) * In_T::width - 1;
        In_T const  val = inval(highBit,lowBit);

        // Compare input against all current tops
        bool  cmp[NumTop+1];
        for(unsigned  i = 0; i < NumTop; i++) {
#pragma HLS UNROLL
          cmp[i] = val > topval[i];
        }
        cmp[NumTop] = false;

        // Shift input into top array at the highest index where it is greater
        for(unsigned  i = 0; i < NumTop; i++) {
#pragma HLS UNROLL
          if(cmp[i]) {
            if(cmp[i+1]) {
              // Shift
              topval   [i] = topval   [i+1];
              toplabels[i] = toplabels[i+1];
            }
            else {
              // Insert
              topval   [i] = val;
              toplabels[i] = idx;
            }
          }
        }
        idx++;
      }
    }

    // Output - index of highest value first
    for(unsigned int topx = 0; topx < NumTop; topx++){
      out.write(toplabels[NumTop - topx - 1]);
    }
  }
}


/**
 * \brief Pool_batch function
 *
 * The function performs a generic pool function (defined in pool.hpp) and works in conjuction 
 * with a sliding window unit performing im2col on the input data, allowing 
 * generic kernel and stride values
 *
 * \tparam Channels   Number of channels in the pool layer
 * \tparam PE         Number of channels in the pool layer computed in parallel
 * \tparam TotalK     Total kernel size of pooling (e.g. 3x3=9)
 * \tparam TSrcI      DataType of the input value (Slice)
 * \tparam TDstI      DataType of the output value (Slice)
 * \tparam TI         DataType of the input stream - safely deducible from the paramaters
 * \tparam TO         DataType of the output stream - safely deducible from the paramaters
 * \tparam TA         DataType of the function class (e.g. Max, Avg, Sum) - safely deducible from the paramaters
 *
 * \param in          Input stream
 * \param out         Output stream
 * \param function    Function class in the pool (Max, Avg, Sum)
 * \param reps        Number of time the function has to be repeatedly executed (e.g. number of images)
 */
template<
  unsigned Channels, unsigned PE, unsigned TotalK,
  typename TSrcI = Identity,typename TDstI = Identity,
  typename TI, typename TO, typename TA
>
void Pool_batch(hls::stream<TI> &in,
                  hls::stream<TO> &out,
                  TA  const &function,
                  int const  reps) {

  constexpr unsigned  NF = Channels / PE;
  constexpr unsigned  SF = TotalK;
  constexpr unsigned  TOTAL_FOLD = NF * SF ;

  decltype(function.init())  accu[PE];
#pragma HLS ARRAY_PARTITION variable=accu complete dim=0

  unsigned  sf   = 0;
  // everything merged into a common iteration space (one "big" loop instead
  // of smaller nested loops) to get the pipelining the way we want
  for(unsigned  i = 0; i < reps * TOTAL_FOLD; i++) {
#pragma HLS pipeline style=flp II=1
    TI  pixel_slice;
    pixel_slice = in.read();

    // Threshold Initialisation
    if(sf == 0) {
      for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
        accu[pe] = function.init();
      }
    }

    auto const  slice_channels = TSrcI()(pixel_slice,0);
    for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
        accu[pe] = function.pool(slice_channels(pe,0), accu[pe]);
    }

    // keep track of which folded synapse/neuron we are processing
    if(++sf == SF) {
      // produce output and clear accumulators
      auto  outElem = TDstI().template operator()<TO>();
      for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
          outElem(pe,0,1) = function.activate(accu[pe]); //
      }
      out.write(outElem);
      // next folded neuron or image
      sf = 0;
    }
  }
}

#endif
