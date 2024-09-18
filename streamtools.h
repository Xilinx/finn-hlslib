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
 *  @file stream-tools.h
 *
 *  Library of templated HLS functions for BNN deployment. 
 *  This file lists a set of convenience funtions used to adapt stream size, 
 *  remove unnecessary streams (padding) and casting
 *
 ******************************************************************************/

#ifndef STREAMTOOLS_H
#define STREAMTOOLS_H

#include "ap_axi_sdata.h"
#include "utils.hpp"

/**
 * \brief   Stream limiter - limits the number of stream packets
 *
 * The block only let the first NumAllowed elements of a stream to pass through, the remainder
 * (NumTotal-NumAllowed) are consumed from input but not re-emitted from the output. 
 * Useful to remove padding 
 *
 * \tparam     DataWidth    Width, in number of bits, of the input and output stream
 * \tparam     NumAllowed   Number of words to pass through
 * \tparam     NumTotal     Total number of words (NumAllowed+NumDropped)
 *
 * \param      in           Input stream
 * \param      out          Output stream
 *
 */
template<unsigned int DataWidth,    
		unsigned int NumAllowed, 	
		unsigned int NumTotal       
>
void StreamLimiter(hls::stream<ap_uint<DataWidth> > & in,
		hls::stream<ap_uint<DataWidth> > & out) {
  static_assert(NumTotal >= NumAllowed, "");
  unsigned int numLeft = NumAllowed;
  for (unsigned int i = 0; i < NumTotal; i++) {
#pragma HLS pipeline style=flp II=1
    ap_uint<DataWidth> e = in.read();
    if (numLeft > 0) {
      out.write(e);
      numLeft--;
    }
  }
}

/**
 * \brief   Stream limiter batch - limits the number of stream packets multiple times
 *
 * The block only let the first NumAllowed elements of a stream to pass through, the remainder
 * (NumTotal-NumAllowed) are consumed from input but not re-emitted from the output. 
 * Useful to remove padding on multiple images (numReps)
 *
 * \tparam     DataWidth    Width, in number of bits, of the input and output stream
 * \tparam     NumAllowed   Number of words to pass through
 * \tparam     NumTotal     Total number of words (NumAllowed+NumDropped)
 *
 * \param      in           Input stream
 * \param      out          Output stream
 * \param      numReps      Number of times the StreamLimiter function has to be called
 *
 */
template<unsigned int DataWidth,	
		unsigned int NumAllowed, 	
		unsigned int NumTotal       
>
void StreamLimiter_Batch(hls::stream<ap_uint<DataWidth> > & in,
		hls::stream<ap_uint<DataWidth> > & out, unsigned int numReps) {
  for (unsigned int rep = 0; rep < numReps; rep++) {
    StreamLimiter<DataWidth, NumAllowed, NumTotal>(in, out);
  }
}

/**
 * \brief   Stream Padding - Padds the input with zeroes for when the sliding window is
 *          centered on border pixels
 *
 * Used to add padding to the input with zeroes in case the sliding window is
 * centered on border pixels 
 *
 * \tparam     ImgDim          Size of the input feature map
 * \tparam     KernelDim       Size of the sliding window
 * \tparam     Stride          Stride of the sliding window
 * \tparam     NumChannels     Amount of channels of the input feature map
 * \tparam     In_t            Input datatype
 * \tparam     PaddingStyle    Type of padding that will be applied
 * 
 * \param      in              Input stream
 * \param      out             Output stream
 *
 */
template<	unsigned int ImgDim, 
			unsigned int KernelDim, 
			unsigned int Stride, 
			unsigned int NumChannels,
			typename In_t,
      unsigned int PaddingStyle=2>
void SameResize(hls::stream<ap_uint<NumChannels* In_t::width> > &in,
		hls::stream<ap_uint<NumChannels* In_t::width> > &out){

	// Number of "same" windows over the input data
	constexpr unsigned int SameWindows = (ImgDim) / Stride + ((ImgDim % Stride) > 0);
	
	// Number of elements to generate as output per dimension
	constexpr unsigned int OutputDim = KernelDim + Stride * (SameWindows - 1);

	// Padding
	constexpr unsigned int Padding = OutputDim - ImgDim;

	// Padding Up and Left
  constexpr unsigned int PaddingUp = Padding/2 + ((PaddingStyle == 2) ? ((Padding % 2) > 0) : 0);
  constexpr unsigned int PaddingLeft = Padding/2 + ((PaddingStyle == 2) ? ((Padding % 2) > 0) : 0);

	// Padding Down and Right (might be 1 element more than up and left in case of odd padding)
	constexpr unsigned int PaddingDown = Padding - PaddingUp;
	constexpr unsigned int PaddingRight = Padding - PaddingLeft;
	ap_uint<NumChannels* In_t::width> outData, inData;

	for(unsigned int y = 0; y<OutputDim; y++){
		for(unsigned int x=0; x < OutputDim; x++){
#pragma HLS pipeline style=flp II=1

			// Padding Rows
			if(y < PaddingUp || y >= (OutputDim - PaddingDown)){
				outData = 0;
			}
			// Padding Cols
			else if(x < PaddingLeft || x >= (OutputDim - PaddingRight)){
				outData = 0;
			}
			// No Padding
			else{
				inData = in.read();
				outData = inData;
			}

			out.write(outData);
		}
	}
}

/**
 * \brief   Stream Padding - Padds the input of multiple frames with zeroes
 *          for when the sliding window is centered on border pixels
 *
 * Used to add padding with zeroes to multiple inputs in case the sliding window is
 * centered on border pixels 
 *
 * \tparam     ImgDim          Size of the input feature map
 * \tparam     KernelDim       Size of the sliding window
 * \tparam     Stride          Stride of the sliding window
 * \tparam     NumChannels     Amount of channels of the input feature map
 * \tparam     In_t            Input datatype
 * \tparam     PaddingStyle    Type of padding that will be applied
 * 
 * \param      in              Input stream
 * \param      out             Output stream
 * \param      numReps         Amount of frames / images
 *
 */
template<	unsigned int ImgDim, 
			unsigned int KernelDim, 
			unsigned int Stride, 
			unsigned int NumChannels,
			typename In_t,
      unsigned int PaddingStyle=2>
void SameResize_Batch(hls::stream<ap_uint<NumChannels* In_t::width> > &in,
		hls::stream<ap_uint<NumChannels* In_t::width> > &out,
		const unsigned int numReps) {
	for (unsigned int rep = 0; rep < numReps; rep++) {
		SameResize<ImgDim, KernelDim, Stride, NumChannels, In_t, PaddingStyle>(in, out);
	}

}



/**
 * \brief   Stream cast - Casts the input stream to a different datatype (OutT)
 *
 * Used to upscale or downscale a stream, enabling loss of information for downscaling or 
 * 0 padding for upscaling 
 *
 * \tparam     InT          Width, in number of bits, of the input and output stream
 * \tparam     OutT         Number of words to pass through
 *
 * \param      in           Input stream
 * \param      out          Output stream
 * \param      numReps      Number of times the StreamLimiter function has to be called
 *
 */
template<typename InT, typename OutT>
void StreamingCast(hls::stream<InT> & in, hls::stream<OutT> & out, unsigned int numReps) {
  for(unsigned int i = 0; i < numReps; i++) {
#pragma HLS pipeline style=flp II=1
    out.write((OutT) in.read());
  }
}

/**
 * \brief   FM Padding - Padds the input with zeroes for when the sliding window is
 *          centered on border pixels
 *
 * Used to add padding with zeroes to multiple inputs in case the sliding window is
 * centered on border pixels - working on non-square images and padding
 *
 * \tparam	OutputDim_x	Padded width of the output feature map
 * \tparam	OutputDim_y	Padded height of the output feature map
 * \tparam	PaddingLeft		Left image padding on x-axis
 * \tparam	PaddingRight	Right image padding on x-axis
 * \tparam	PaddingTop		Top image padding on y-axis
 * \tparam	PaddingBottom	Bottom image padding on y-axis
 * \tparam	NumChannels		Number of channels of the input feature map
 * \tparam	SIMD			Input parallelism 
 * \tparam	In_t			Input datatype
 *
 * \param	in		Input stream
 * \param	out		Output stream
 */
template<
	unsigned  OutputDim_x,
	unsigned  OutputDim_y,
	unsigned  PaddingLeft,
	unsigned  PaddingRight,
	unsigned  PaddingTop,
	unsigned  PaddingBottom,
	unsigned  NumChannels,
	unsigned  SIMD,
	typename  In_t
>
void FMPadding_nonsquare(
	hls::stream<ap_uint<SIMD*In_t::width>> &in,
	hls::stream<ap_uint<SIMD*In_t::width>> &out
){
	static_assert(NumChannels%SIMD == 0, "Channel count must be a SIMD multiple.");
	constexpr unsigned  Folding = NumChannels/SIMD;

	for(unsigned  y = 0; y < OutputDim_y; y++) {
		for(unsigned  x = 0; x < OutputDim_x; x++) {
			for(unsigned  sf = 0; sf < Folding; sf++) {
#pragma HLS pipeline style=flp II=1
				ap_uint<SIMD*In_t::width>  outData = 0;

				// Read & forward real data only for non-padding image kernel
				if(
					/* rows */ (PaddingTop  <= y) && (y < OutputDim_y - PaddingBottom) &&
					/* cols */ (PaddingLeft <= x) && (x < OutputDim_x - PaddingRight)
				) {
					outData = in.read();
				}
				out.write(outData);
			}
		}
	}
}

/**
 * \brief   FM Padding Non Square - Padds the input of multiple frames with zeroes
 *          for when the sliding window is centered on border pixels
 *
 * Used to add padding with zeroes to multiple inputs in case the sliding window is
 * centered on border pixels - working on non-square images and padding
 *
 * \tparam	OutputDim_x	Padded width of the output feature map
 * \tparam	OutputDim_y	Padded height of the output feature map
 * \tparam	PaddingLeft		Left image padding on x-axis
 * \tparam	PaddingRight	Right image padding on x-axis
 * \tparam	PaddingTop		Top image padding on y-axis
 * \tparam	PaddingBottom	Bottom image padding on y-axis
 * \tparam	NumChannels		Number of channels of the input feature map
 * \tparam	SIMD			Input parallelism 
 * \tparam	In_t			Input datatype
 *
 * \param	in		Input stream
 * \param	out		Output stream
 * \param	numReps	Number of frames / images
 */
template<
	unsigned  OutputDim_x,
	unsigned  OutputDim_y,
	unsigned  PaddingLeft,
	unsigned  PaddingRight,
	unsigned  PaddingTop,
	unsigned  PaddingBottom,
	unsigned  NumChannels,
	unsigned  SIMD,
	typename  In_t
>
void FMPadding_nonsquare_Batch(
	hls::stream<ap_uint<SIMD*In_t::width>> &in,
	hls::stream<ap_uint<SIMD*In_t::width>> &out,
	unsigned const  numReps
) {
	for (unsigned int rep = 0; rep < numReps; rep++) {
		FMPadding_nonsquare<
			OutputDim_x, OutputDim_y,
			PaddingLeft, PaddingRight, PaddingTop, PaddingBottom,
			NumChannels, SIMD, In_t
		>(in, out);
	}
}

/**
 * \brief   FM Padding - Padds the input with zeroes for when the sliding window is
 *          centered on border pixels
 *
 * Used to add padding to the input with zeroes in case the sliding window is
 * centered on border pixels
 *
 * \tparam	ImgDim			<ignored>
 * \tparam	OutputDim		Size of the output feature map
 * \tparam	PaddingBefore	Top / left padding
 * \tparam	PaddingBehind	Bottom / right padding
 * \tparam	NumChannels		Number of channels of the input feature map
 * \tparam	SIMD			Input parallelism 
 * \tparam	In_t			Input datatype
 *
 * \param	in	Input stream
 * \param	out	Output stream
 *
 */
template<
	unsigned  ImgDim,
	unsigned  OutputDim,
	unsigned  PaddingBefore,
	unsigned  PaddingBehind,
	unsigned  NumChannels,
	unsigned  SIMD,
	typename  In_t
>
void FMPadding(
	hls::stream<ap_uint<SIMD*In_t::width>> &in,
	hls::stream<ap_uint<SIMD*In_t::width>> &out
){
#pragma HLS inline
	FMPadding_nonsquare<
		OutputDim, OutputDim,
		PaddingBefore, PaddingBehind, PaddingBefore, PaddingBehind,
		NumChannels, SIMD, In_t
	>(in, out);
}

/**
 * \brief   FM Padding - Padds the input of multiple frames with zeroes
 *          for when the sliding window is centered on border pixels
 *
 * Used to add padding with zeroes to multiple inputs in case the sliding window is
 * centered on border pixels
 *
 * \tparam	ImgDim			<ignored>
 * \tparam	OutputDim		Size of the output feature map
 * \tparam	PaddingBefore	Top / left padding
 * \tparam	PaddingBehind	Bottom / right padding
 * \tparam	NumChannels		Number of channels of the input feature map
 * \tparam	SIMD			Input parallelism 
 * \tparam	In_t			Input datatype
 *
 * \param	in	Input stream
 * \param	out	Output stream
 * \param	numReps	Number of frames / images
 */
template<
	unsigned  ImgDim,
	unsigned  OutputDim,
	unsigned  PaddingBefore,
	unsigned  PaddingBehind,
	unsigned  NumChannels,
	unsigned  SIMD,
	typename  In_t
>
void FMPadding_Batch(
	hls::stream<ap_uint<SIMD*In_t::width>> &in,
	hls::stream<ap_uint<SIMD*In_t::width>> &out,
	unsigned const  numReps
) {
	for (unsigned int rep = 0; rep < numReps; rep++) {
		FMPadding<ImgDim, OutputDim, PaddingBefore, PaddingBehind, NumChannels, SIMD, In_t>(in, out);
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
void StreamingDataWidthConverter_Batch(hls::stream<ap_uint<InWidth> > & in,
		hls::stream<ap_uint<OutWidth> > & out, const unsigned int numReps) {
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
 * \brief   Generalized Stream Data Width Converter - 
 *			Converts the width of the input stream in the output stream
 *
 * Used to upscale or downscale a stream, without any loss of data in the procedure. 
 * Additionally performs padding or cropping of the streams if
 * InWidth*NumInWords != OutWidth*NumOutWords
 *
 * \tparam     InWidth      Width, in number of bits, of the input stream
 * \tparam     OutWidth     Width, in number of bits, of the output stream 
 * \tparam     NumInWords   Number of input words to process
 * \tparam     NumOutWords   Number of output words to process
 * \tparam     totalIters   Number of loop iterations in total to process a transaction

 *
 * \param      in           Input stream
 * \param      out          Output stream
 * \param      numReps      Number of times the function has to be called
 *
 */
template<unsigned int InWidth,		
		unsigned int OutWidth,		
		unsigned int NumInWords,
		unsigned int NumOutWords,
		unsigned int totalIters	
>
void StreamingDataWidthConverterGeneralized_Batch(hls::stream<ap_uint<InWidth> > & in,
		hls::stream<ap_uint<OutWidth> > & out, const unsigned int numReps) {

  const unsigned int  BufferLength = InWidth+OutWidth;
  constexpr unsigned  NumInWordsLog = clog2(NumInWords)+1;
  constexpr unsigned  NumOutWordsLog = clog2(NumOutWords)+1;
  constexpr unsigned  BufferWidthLog = clog2(BufferLength)+1;
  unsigned int totalItersReps = totalIters*numReps+numReps;
  ap_uint<NumOutWordsLog> words_written = 0;
  ap_uint<NumInWordsLog> words_read = 0;
  ap_uint<BufferWidthLog> els_in_buffer = 0;
  // we allocate OutWidth extra space for cases where we have leftover from
  // a previous word due to our els_in_buffer tracking scheme for when to 
  // read in ei (potentially introducing padding or cropping)
  ap_uint<OutWidth> eo_final = 0;
  ap_uint<InWidth+OutWidth> eo = 0;
  ap_uint<InWidth> ei;
  if (InWidth > OutWidth) {
    // emit multiple output words per input word read
    for (unsigned int t = 0; t < totalItersReps; t++) {
#pragma HLS pipeline style=flp II=1

	  // we reached the end of the transaction for this numReps superiteration
	  // reset all trackers to allow further stream IO and stop padding/cropping
	  eo_final(OutWidth-1,0) = eo(OutWidth-1,0);
	  // write each cycle and shift
	  if ((words_written < NumOutWords) && (els_in_buffer >= OutWidth)){
	    out.write(eo_final(OutWidth-1,0));
	    els_in_buffer -= OutWidth;
	    eo = eo >> OutWidth;
	    words_written+=1;
	  }
	  // conditionally read in
	  if (els_in_buffer < OutWidth) {
		if (words_read < NumInWords){
          ei = in.read();
		  words_read+=1;
		  eo(InWidth + els_in_buffer - 1, els_in_buffer) = ei(InWidth-1, 0);	
		}
		// always introducing elements to provide padding functionality
	    els_in_buffer += InWidth;
	  }
	  if ((words_written == NumOutWords) && (words_read == NumInWords)) {
		words_read = 0;
		words_written = 0;
		els_in_buffer = 0;
	  }
    }
  } else if (InWidth == OutWidth) {
    // straight-through copy
	// NumOutWords != NumInWords if padding or cropping happened
	// So we use one of two versions where we control how many times
	// the streams are read/written.
	if (NumOutWords >= NumInWords) {
		for (unsigned int i = 0; i < totalItersReps; i++) {
#pragma HLS pipeline style=flp II=1
			ap_uint<InWidth> e = 0;
			if (i < NumInWords) {
				e = in.read();
			}
			out.write(e);
		}	
	} else {
		for (unsigned int i = 0; i < totalItersReps; i++) {
#pragma HLS pipeline style=flp II=1
			ap_uint<InWidth> e = in.read();
			if (i < NumOutWords) {
				out.write(e);
			}	
		}	
	}
  } else { // InWidth < OutWidth
    // read multiple input words per output word emitted
    for (unsigned int t = 0; t < totalItersReps; t++) {
#pragma HLS pipeline style=flp II=1
	  // we reached the end of the transaction for this numReps superiteration
	  // reset all trackers to allow further stream IO and stop padding/cropping
      eo_final(OutWidth-1,0) = eo(OutWidth-1,0);
	  // conditionally write out
	  if (((els_in_buffer >= OutWidth) || (words_read >= NumInWords)) && (words_written < NumOutWords)) {
		out.write(eo_final(OutWidth-1,0));
		els_in_buffer -= OutWidth;
		eo = eo >> OutWidth;
		words_written+=1;
	  }
      // read input each cycle and shift into output buffer
	  // padding if we ran out of input words
	  if (words_read < NumInWords){
		ei = in.read();
		eo(InWidth + els_in_buffer - 1, els_in_buffer) = ei(InWidth-1, 0);
		words_read+=1;
		els_in_buffer += InWidth;
	  }
	  if ((words_written == NumOutWords) && (words_read == NumInWords)) {
		words_read = 0;
		words_written = 0;
		els_in_buffer = 0;
	  }

    }
  }
}

/**
 * \brief   Stream Data Width Converter No Multiple - 
 *          Converts the width of the input stream in the output stream for no multiple dimensions
 *
 * Used to downscale a stream, without any loss of data in the procedure. 
 * For downscaling (InWidth > OutWidth), InWidth has to be a multiple of OutWidth.
 *
 * \tparam     InWidth      Width, in number of bits, of the input stream
 * \tparam     OutWidth     Width, in number of bits, of the output stream 
 *
 * \param      in           Input stream
 * \param      out          Output stream
 *
 */
template<
    unsigned int InWidth,    
    unsigned int OutWidth
>
void StreamingDataWidthConverterNoMultiple(
    hls::stream<ap_uint<InWidth> > & in,
    hls::stream<ap_uint<OutWidth> > & out) {
    static_assert((InWidth % 2) == 0, "");
    static_assert((OutWidth % 2) == 0, "");
    static_assert(InWidth != OutWidth, "");
    static unsigned int      offset = 0; 

    if (InWidth > OutWidth){
     
      static ap_uint<OutWidth> remainder = 0;
      ap_uint<InWidth>  valueIn = in.read();
      
      if(offset !=0) {
        ap_uint<OutWidth>   valueOut = 0;
        valueOut = (valueIn(offset-1,0),remainder(OutWidth-offset-1,0));
        valueIn = valueIn(InWidth-1,offset); // leave the next part prepared 
        out.write(valueOut);
      }
      for (; offset <= (InWidth-OutWidth) ; offset+=OutWidth){
        ap_uint<OutWidth>   valueOut = valueIn(OutWidth-1,0);
        valueIn = valueIn(InWidth-1,OutWidth); // leave the next part prepared 
        out.write(valueOut);
      }
      remainder = valueIn;
      if (offset == InWidth)
        offset = 0;
      else
        offset = offset + OutWidth - InWidth;
    }
    else {
      /*OutWidth > InWidth*/
      static ap_uint<InWidth> remainder = 0;
      ap_uint<OutWidth> value = 0;
      if (offset !=0) {
        value(offset-1,0) = remainder(InWidth-1,InWidth-offset);
      }
      for (; offset <= (OutWidth-InWidth); offset+=InWidth){
        ap_uint<InWidth>   aux = in.read();
        value(offset+InWidth-1,offset) = aux;
      }
      if (offset != OutWidth){
        ap_uint<InWidth>   aux = in.read();
        value(OutWidth-1,offset) = aux(OutWidth-offset-1,0);
        remainder                   = aux;
        offset = offset + InWidth - OutWidth;
      }
      else
        offset = 0;
      out.write(value);
    }

}


/**
 * \brief   Stream Duplicator - Reads in a stream and writes the data into two identical streams
 *
 * Used to generate the inputs to the bypass and convolutional branches in Resnet-50
 *
 * \tparam     DataWidth    Width, in number of bits, of the streams
 * \tparam     NumTotal     Total number of words in the input stream
 *
 * \param      in           Input stream
 * \param      out1         Output stream I
 * \param      out2         Output stream II
 *
 */
template<unsigned int DataWidth,
		unsigned int NumTotal
>
void DuplicateStreams(hls::stream<ap_uint<DataWidth> > & in, hls::stream<ap_uint<DataWidth> > & out1,
		hls::stream<ap_uint<DataWidth> > & out2) {
	
	for (unsigned int i = 0; i < NumTotal; i++) {
#pragma HLS pipeline style=flp II=1
		ap_uint<DataWidth> e = in.read();
		
		out1.write(e);
		out2.write(e);
	}
}

/**
 * \brief   Batch Stream Duplicator - Reads in a stream multiple times and writes the data into two identical streams
 *
 * Used to generate the inputs to the bypass and convolutional branches in Resnet-50 when dealing with multiple 'frames'
 *
 * \tparam     DataWidth    Width, in number of bits, of the streams
 * \tparam     NumTotal     Total number of words in the input stream
 *
 * \param      in           Input stream
 * \param      out1         Output stream I
 * \param      out2         Output stream II
 * \param      numReps      Number of frames / images
 *
 */
template<unsigned int DataWidth,
		unsigned int NumTotal
>
void DuplicateStreams_Batch(hls::stream<ap_uint<DataWidth> > & in, hls::stream<ap_uint<DataWidth> > & out1,
		hls::stream<ap_uint<DataWidth> > & out2, const unsigned int numReps) {
	for (unsigned int image = 0; image < numReps; image++) {
		DuplicateStreams<DataWidth, NumTotal>(in, out1, out2);
	}
}

/**
 * \brief   Element-Wise Addition - Reads in data elements from two streams and writes the sum of these elements to an output
 *
 * \tparam     NumChannels  Amount of channels of the streams
 * \tparam     In1_t        First operand datatype
 * \tparam     In2_t        Second operand datatype 
 * \tparam     Out_t        Datatype of the accumulation output 
 * \tparam     NumTotal     Total number of words in the input streams
 * \tparam     offset       Offset value for the accumulation
 *
 * \param      in1          Input stream I
 * \param      in2          Input stream II
 * \param      out          Output stream
 *
 */

template <unsigned int NumChannels,
          typename In1_t,
          typename In2_t,
          typename Out_t,
          unsigned int NumTotal, 
          int offset = 0>
void AddStreams(hls::stream<ap_uint<NumChannels * In1_t::width>> &in1, hls::stream<ap_uint<NumChannels * In2_t::width>> &in2,
                hls::stream<ap_uint<NumChannels * Out_t::width>> &out) {

  for (unsigned int i = 0; i < NumTotal; i++) {
#pragma HLS pipeline style=flp II=1
    ap_uint<NumChannels * In1_t::width> e1 = in1.read();
    ap_uint<NumChannels * In2_t::width> e2 = in2.read();
    ap_uint<NumChannels * Out_t::width> e;
    for (unsigned int j = 0; j < NumChannels; j++) {
#pragma HLS UNROLL
      In1_t op1 = e1((j + 1) * In1_t::width - 1, j * In1_t::width);
      In2_t op2 = e2((j + 1) * In2_t::width - 1, j * In2_t::width);
      Out_t sum = op1 + op2 + offset;
      e((j + 1) * Out_t::width - 1, j * Out_t::width) = sum;
    }
    out.write(e);
  }
}


/**
 * \brief   
 *
 * Used to implement point-wise addition in Resnet-50 for multiple images
 *
 * \tparam     NumChannels  Amount of channels of the streams
 * \tparam     In1_t        First operand datatype
 * \tparam     In2_t        Second operand datatype 
 * \tparam     Out_t        Datatype of the accumulation output 
 * \tparam     NumTotal     Total number of words in the input streams
 * \tparam     offset       Offset value for the accumulation
 *
 * \param      in1          Input stream I
 * \param      in2          Input stream II
 * \param      out          Output stream
 * \param      numReps      Number of frames / images
 *
 */
template <unsigned int NumChannels,
          typename In1_t,
          typename In2_t,
          typename Out_t,
          unsigned int NumTotal,
          int offset = 0>
void AddStreams_Batch(hls::stream<ap_uint<NumChannels * In1_t::width>> &in1, hls::stream<ap_uint<NumChannels * In2_t::width>> &in2,
                hls::stream<ap_uint<NumChannels * Out_t::width>> &out, const unsigned int numReps) {
  for (unsigned int image = 0; image < numReps; image++) {
    AddStreams<NumChannels, In1_t, In2_t, Out_t, NumTotal, offset>(in1, in2, out);
  }
}

/**
 * \brief   Addition Layer - Reads in two streams and writes the sum of these streams to an output
 *
 * Used to merge the outputs of the bypass and convolutional branches in Resnet-50
 *
 * \tparam     NumChannels  Amount of channels of the streams
 * \tparam     In1_t        First operand datatype
 * \tparam     In2_t        Second operand datatype 
 * \tparam     Out_t        Datatype of the accumulation output  * \tparam     NumTotal     Total number of words in the input streams
 * \tparam     PECount      Amount of processing elements working in parallel 
 * \tparam     offset       Offset value for the accumulation 
 *
 * \param      in1          Input stream I
 * \param      in2          Input stream II
 * \param      out          Output stream
 * \param      numReps      Number of frames / images
 *
 */
template <unsigned int NumChannels,
          typename In1_t,
          typename In2_t,
          typename Out_t,
          unsigned int NumTotal,
          unsigned int PECount, 
          int offset = 0>
void AddStreamsLayer_Batch(hls::stream<ap_uint<NumChannels * In1_t::width>> &in1, hls::stream<ap_uint<NumChannels * In2_t::width>> &in2,
                           hls::stream<ap_uint<NumChannels * Out_t::width>> &out, const unsigned int numReps) {
#pragma HLS INLINE
  static_assert(NumChannels % PECount == 0, "");
  hls::stream<ap_uint<PECount * In1_t::width>> in_folded1;
  hls::stream<ap_uint<PECount * In2_t::width>> in_folded2;
  hls::stream<ap_uint<PECount * Out_t::width>> out_folded;
  StreamingDataWidthConverter_Batch<NumChannels * In1_t::width, PECount * In1_t::width, NumTotal>(in1, in_folded1, numReps);
  StreamingDataWidthConverter_Batch<NumChannels * In2_t::width, PECount * In2_t::width, NumTotal>(in2, in_folded2, numReps);
  AddStreams_Batch<PECount, In1_t, In2_t, Out_t, NumTotal *(NumChannels / PECount),offset>(in_folded1, in_folded2, out_folded, numReps);
  StreamingDataWidthConverter_Batch<PECount * Out_t::width, NumChannels * Out_t::width, NumTotal *(NumChannels / PECount)>(out_folded, out, numReps);
}


/**
 * \brief   Stream Multi Chan Data Width Converter - Converts the width of the input stream in the output stream, working on multiple parallel streams
 *
 * Used to upscale or downscale a stream, without any loss of data in the procedure. 
 * For downscaling (InWidth > OutWidth), InWidth has to be a multiple of OutWidth.
 * For upscaling (InWidth < OutWidth), OutWidth has to be a multiple of InWidth.
 * This version works on the MMV structure, with multiple parallel streams
 *
 * \tparam     InWidth      Width, in number of bits, of the input stream
 * \tparam     OutWidth     Width, in number of bits, of the output stream 
 * \tparam     NumInWords   Number of input words to process
 * \tparam     NumVecs      Number of parallel vectors MMV
 *
 * \param      in           Input stream
 * \param      out          Output stream
 * \param      numReps      Number of times the function has to be called
 *
 */
template<unsigned int InWidth,		// width of input stream
		unsigned int OutWidth,		// width of output stream
		unsigned int NumInWords,		// number of input words to process
		unsigned int NumVecs
>
void MultiChanDataWidthConverter_Batch(
	hls::stream<MultiChanData<NumVecs, InWidth> > & in,
	hls::stream<MultiChanData<NumVecs, OutWidth> > & out,
	const unsigned int numReps) {
	static_assert((InWidth % OutWidth == 0) || (OutWidth % InWidth == 0), "");
	if (InWidth > OutWidth) {
		// emit multiple output words per input word read
		const unsigned int outPerIn = InWidth / OutWidth;
		const unsigned int totalIters = NumInWords * outPerIn * numReps;
		unsigned int o = 0;
		MultiChanData<NumVecs, InWidth> ei;
		for (unsigned int t = 0; t < totalIters; t++) {
#pragma HLS pipeline style=flp II=1
			// read new input word if current out count is zero
			if (o == 0)
				ei = in.read();
			// pick output word from the rightmost position
			MultiChanData<NumVecs, OutWidth> eo;
			for(unsigned int v = 0; v < NumVecs; v++) {
#pragma HLS UNROLL
				eo.data[v] = (ei.data[v])(OutWidth - 1, 0);
				// shift input to get new output word for next iteration
				ei.data[v] = ei.data[v] >> OutWidth;
			}
			out.write(eo);
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
			MultiChanData<NumVecs, InWidth> e = in.read();
			MultiChanData<NumVecs, OutWidth> eo;
			// we don't support typecasting between templated types, so explicitly
			// transfer vector-by-vector here
			for(unsigned int v=0; v < NumVecs; v++) {
#pragma HLS UNROLL
				eo.data[v] = e.data[v];
			}
			out.write(eo);
		}
	} else { // InWidth < OutWidth
		// read multiple input words per output word emitted
		const unsigned int inPerOut = OutWidth / InWidth;
		const unsigned int totalIters = NumInWords * numReps;
		unsigned int i = 0;
		MultiChanData<NumVecs, OutWidth> eo;
		for (unsigned int t = 0; t < totalIters; t++) {
#pragma HLS pipeline style=flp II=1
			// read input and shift into output buffer
			MultiChanData<NumVecs, InWidth> ei = in.read();
			for(unsigned int v = 0; v < NumVecs; v++) {
#pragma HLS UNROLL
				eo.data[v] = eo.data[v] >> InWidth;
				(eo.data[v])(OutWidth - 1, OutWidth - InWidth) = ei.data[v];
			}
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
 * \brief   Flatten Multi Chan Data - Converts the parallel input stream in a flatten output stream
 *
 * Used to pach a flattened stream into a structure with multiple parallel streams
 *
 * \tparam     NumChannels  Number of channels flattened in the input stream
 * \tparam     DataWidth    Width, in number of bits, of each stream
 *
 * \param      in           Input parallel stream
 * \param      out          Output stream
 * \param      numReps      Number of times the function has to be called
 *
 */
template <unsigned int NumChannels, unsigned int DataWidth>
void FlattenMultiChanData(
	hls::stream<MultiChanData<NumChannels, DataWidth> > & in,
	hls::stream<ap_uint<NumChannels*DataWidth> > & out,
	const unsigned int numReps
) {
	for(unsigned int r = 0; r < numReps; r++) {
#pragma HLS pipeline style=flp II=1
		MultiChanData<NumChannels, DataWidth> e = in.read();
		ap_uint<NumChannels*DataWidth> o = 0;
		for(unsigned int v = 0; v < NumChannels; v++) {
#pragma HLS UNROLL
			o(DataWidth*(v+1)-1, DataWidth*v) = e.data[v];
		}
		out.write(o);
	}
}

/**
 * \brief   Pack Multi Chan Data - Converts the flatten input stream into a parallel output stream
 *
 * Used to pach a flattened stream into a structure with multiple parallel streams
 *
 * \tparam     NumChannels  Number of channels flattened in the input stream
 * \tparam     DataWidth    Width, in number of bits, of each stream
 *
 * \param      in           Input stream
 * \param      out          Output parallel stream
 * \param      numReps      Number of times the function has to be called
 *
 */
template <unsigned int NumChannels, unsigned int DataWidth>
void PackMultiChanData(
	hls::stream<ap_uint<NumChannels*DataWidth> > & in,
	hls::stream<MultiChanData<NumChannels, DataWidth> > & out,
	const unsigned int numReps
) {
	for(unsigned int r = 0; r < numReps; r++) {
#pragma HLS pipeline style=flp II=1
		ap_uint<NumChannels*DataWidth> e = in.read();
		MultiChanData<NumChannels, DataWidth> o;
		for(unsigned int v = 0; v < NumChannels; v++) {
#pragma HLS UNROLL
			o.data[v] = e(DataWidth*(v+1)-1, DataWidth*v);
		}
		out.write(o);
	}
}


template<unsigned IW, unsigned OW, unsigned N>
 class WidthAdjustedInputStream {
  hls::stream<ap_uint<OW>>  m_target;

 public:
  WidthAdjustedInputStream(hls::stream<ap_uint<IW> >&  source, unsigned const  reps) {
    StreamingDataWidthConverter_Batch<IW, OW, N>(source, m_target, reps);
  }
  ~WidthAdjustedInputStream() {}

 public:
  operator hls::stream<ap_uint<OW> >&() {
    return  m_target;
  }
};
template<unsigned W, unsigned N>
 class WidthAdjustedInputStream<W, W, N> {

  hls::stream<ap_uint<W>> &m_source;

 public:
  WidthAdjustedInputStream(hls::stream<ap_uint<W> >&  source, __attribute__((unused)) unsigned const  reps) : m_source(source) {}
  ~WidthAdjustedInputStream() {}

 public:
  operator hls::stream<ap_uint<W> >&() {
    return  m_source;
  }
};


template<unsigned IW, unsigned OW, unsigned N>
class WidthAdjustedOutputStream {
  hls::stream<ap_uint<IW>>  m_buffer;
  hls::stream<ap_uint<OW>> &m_target;
  unsigned const  m_reps;
  
 public:
  WidthAdjustedOutputStream(hls::stream<ap_uint<OW> >&  target, unsigned const  reps) : m_target(target), m_reps(reps) {}
  ~WidthAdjustedOutputStream() {
    StreamingDataWidthConverter_Batch<IW, OW, N>(m_buffer, m_target, m_reps);
  }

 public:
  operator hls::stream<ap_uint<IW> >&() {
    return  m_buffer;
  }
};
template<unsigned W, unsigned N>
 class WidthAdjustedOutputStream<W, W, N> {
  hls::stream<ap_uint<W>> &m_target;

 public:
  WidthAdjustedOutputStream(hls::stream<ap_uint<W> >&  target, __attribute__((unused)) unsigned const  reps)
    : m_target(target) {}
  ~WidthAdjustedOutputStream() {}

 public:
  operator hls::stream<ap_uint<W> >&() {
    return  m_target;
  }
};

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
void Qdma2Stream_Batch(hls::stream<qdma_axis<DataWidth,0,0,0> > & in, hls::stream<ap_uint<DataWidth> > & out, const unsigned int numReps){
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
void Stream2Qdma_Batch(hls::stream<ap_uint<DataWidth> > & in, hls::stream<qdma_axis<DataWidth,0,0,0> > & out, const unsigned int numReps){
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
