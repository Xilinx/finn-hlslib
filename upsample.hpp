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
 *  		    erling on 5/10/21.
 *
 *
 *  Library of templated HLS functions for QNN deployment. 
 *  Targeting upsampling layers
 *
 ******************************************************************************/

#ifndef UPSAMPLE_HPP
#define UPSAMPLE_HPP

#include <ap_int.h>
#include <hls_stream.h>


/**
 * \brief Upsampling with the Nearest Neighbour algorithm. Works with square feature maps
 *
 * \tparam 	OFMDim 		Size of the output feature map
 * \tparam 	IFMDim 		Size of the input feature map
 * \tparam 	NumChannels 	Amount of channels of the input feature map
 * \tparam 	In_t		 	Input datatype
 *
 * \param 	in 				Input stream
 * \param 	out 			Output stream
 */
template<unsigned int OFMDim,
	unsigned int IFMDim,
	unsigned int NumChannels,
	typename In_t>
void UpsampleNearestNeighbour(
        hls::stream<ap_uint<NumChannels * In_t::width>> & in,
        hls::stream<ap_uint<NumChannels * In_t::width>> & out
) {
  static_assert(OFMDim > IFMDim, "");

  constexpr unsigned int scale_factor = OFMDim/IFMDim;
  constexpr unsigned int Padding = OFMDim % IFMDim;
  // Padding might be asymmetrical
  constexpr unsigned int PaddingDown = Padding/2;
  constexpr unsigned int PaddingUp = Padding - PaddingDown;
  // Padding might be asymmetrical
  constexpr unsigned int PaddingRight = Padding/2;
  constexpr unsigned int PaddingLeft = Padding - PaddingRight;

  ap_uint<NumChannels * In_t::width> outData, inData;
  ap_uint<NumChannels * In_t::width> RowBuf[IFMDim];
  int count_row = -PaddingUp; // Counter used to understand whether reading (and buffering) a row or not - Made in order to avoid modulo operations
  for (unsigned int y = 0; y < OFMDim; y++) {
	  for (unsigned int x = 0; x < OFMDim; x++) {
#pragma HLS pipeline style=flp II=1
		bool read_row = (y ==0) || count_row==scale_factor;
		if ((x < IFMDim) && read_row)
		{
			inData = in.read();
			RowBuf[x] = inData;
		}
		// Padding Cols
		if(x < PaddingLeft){
			outData = RowBuf[0];
		}
		else if (x >= (OFMDim - PaddingRight)){
			outData = RowBuf[IFMDim-1];

		}
		// Padding Rows
		else if(y < PaddingUp || y >= (OFMDim - PaddingDown)){
			outData = RowBuf[(x-PaddingLeft)/scale_factor];
		}
		// No Padding
		else{

			outData = RowBuf[(x-PaddingLeft)/scale_factor];
		}
		//std::cout << outData << " " ;
		out.write(outData);
	  }// end for y
	  //std::cout << std::endl;
	  count_row++;
	  if (count_row > scale_factor)
		  count_row =1;
  } // end for x

}


/**
 * \brief Upsampling with the Nearest Neighbour algorithm. Works with square feature maps on multiple images
 *
 * \tparam 	OFMDim 		Size of the output feature map
 * \tparam 	IFMDim 		Size of the input feature map
 * \tparam 	NumChannels 	Amount of channels of the input feature map
 * \tparam 	In_t		 	Input datatype
 *
 * \param 	in 			Input stream
 * \param 	out 			Output stream
 * \param     numReps      Number of time the function has to be repeatedly executed (e.g. number of images)
 */
template<unsigned int OFMDim,
	unsigned int IFMDim,
	unsigned int NumChannels,
	typename In_t>
void UpsampleNearestNeighbour_Batch(
        hls::stream<ap_uint<NumChannels * In_t::width>> & in,
        hls::stream<ap_uint<NumChannels * In_t::width>> & out,
		unsigned int numReps) {
  for (unsigned int rep = 0; rep < numReps; rep++) {
	UpsampleNearestNeighbour<OFMDim, IFMDim, NumChannels, In_t>(in, out);
  }
}

/**
 * \brief Upsampling a vector with the Nearest Neighbour algorithm.
 *
 * \tparam	OFMDim		Output vector length - must be a whole multiple of IFMDim
 * \tparam	IFMDim		Input vector length
 * \tparam	NumChannels Channels per element
 * \tparam	In_t		Per-channel input type
 *
 * \param	src	Input stream
 * \param	dst	Output stream
 */
template<
	unsigned  OFMDim,		// Output vector length - must be a whole multiple of IFMDim
	unsigned  IFMDim,		// Input vector length
	unsigned  NumChannels,	// Channels per element
	typename  In_t			// Per-channel input type
>
void UpsampleNearestNeighbour_1D(
        hls::stream<ap_uint<NumChannels * In_t::width>> &src,
        hls::stream<ap_uint<NumChannels * In_t::width>> &dst
) {
	static_assert(OFMDim % IFMDim == 0, "OFMDim must be a whole multiple of IFMDim.");
	constexpr unsigned  REPS = OFMDim / IFMDim;

	using  buf_t = ap_uint<NumChannels * In_t::width>;
	buf_t     buf;
	unsigned  rep = 0;
	for(unsigned  i = 0; i < OFMDim; i++) {
#pragma HLS pipeline II=1 style=flp
		if(rep == 0)  buf = src.read();
		dst.write(buf);
		if(++rep == REPS)  rep = 0;
	}
}

#endif
