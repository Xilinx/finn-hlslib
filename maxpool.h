/******************************************************************************
 * Copyright (c) 2019, Xilinx, Inc.
 * Copyright (c) 2025, AMD, Inc.
 * All rights reserved.
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
 * @author	Giulio Gambardella <giuliog@xilinx.com>
 * @author	Thomas B. Preusser <thomas.preusser@utexas.edu>
 *        	  Marie-Curie Fellow, Xilinx Ireland, Grant Agreement No. 751339
 * @author	Christoph Doehring <cdoehrin@xilinx.com>
 * @author	Felix Jentzsch <felixj@xilinx.com>
 * @author	Thomas B. Preusser <thomas.preusser@amd.com>
 *
 * @brief	Header requiring refactoring as it is now a mixed bag without maxpool.
 ******************************************************************************/

#ifndef MAXPOOL_H
#define MAXPOOL_H
 
#include "interpret.hpp"
#include "utils.hpp"

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

#endif
