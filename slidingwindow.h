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
 *******************************************************************************/

 /******************************************************************************
 *
 *  Authors: Giulio Gambardella <giuliog@xilinx.com>
 *           Thomas B. Preusser <thomas.preusser@utexas.edu>
 *             Marie-Curie Fellow, Xilinx Ireland, Grant Agreement No. 751339
 *           Christoph Doehring <cdoehrin@xilinx.com>
 *           Felix Jentzsch <felix.jentzsch@upb.de>
 *
 *  \file slidingwindow.h
 *
 *  Library of templated HLS functions for BNN deployment. 
 *  This file lists a set of convenience funtions used to implement  
 *  Sliding window generator for convolutions
 *
 *****************************************************************************/

#ifndef SLIDINGWINDOW_H
#define SLIDINGWINDOW_H
 
#include "utils.hpp"

#define MAX(x, y) (((x) > (y)) ? (x) : (y)) /* \brief Maximum value between x and y*/
#define MIN(x, y) (((x) > (y)) ? (y) : (x)) /* !< \brief Minimum value between x and y*/
/**
 * \brief     Memory resource pragma instantiation for the sliding window generator, default resource
 * 
 * The buffer in the sliding window generator can be implemented in multiple hardware resources. 
 * 
 * ap_resource_dflt will let HLS choose the best one
 * ap_resource_bram will force HLS to implement the buffer in BRAMs
 * ap_resource_uram will force HLS to implement the buffer in URAMs
 * ap_resource_lutram will force HLS to implement the buffer in LUTRAMs
 *
 * \tparam     T		Datatype of the buffer instantiated in the sliding window generator
 * 
 * \param      inputBuf	Buffer used in the SWG
 * \param      r     	Resource type for the hardware implementation
 *
 * \return     Result of the multiply operation
 */
template <typename T>
void memory_resource(T inputBuf, ap_resource_dflt const&){
#pragma HLS inline
#pragma HLS RESOURCE variable=inputBuf core=RAM_2P
}
/**
 * \brief     Memory resource pragma instantiation for the sliding window generator, BRAM resource
 * 
 * The buffer in the sliding window generator can be implemented in multiple hardware resources. 
 * 
 * ap_resource_dflt will let HLS choose the best one
 * ap_resource_bram will force HLS to implement the buffer in BRAMs
 * ap_resource_uram will force HLS to implement the buffer in URAMs
 * ap_resource_lutram will force HLS to implement the buffer in LUTRAMs
 *
 * \tparam     T		Datatype of the buffer instantiated in the sliding window generator
 * 
 * \param      inputBuf	Buffer used in the SWG
 * \param      r     	Resource type for the hardware implementation
 *
 * \return     Result of the multiply operation
 */
template <typename T>
void memory_resource(T inputBuf, ap_resource_bram const&){
#pragma HLS inline
#pragma HLS RESOURCE variable=inputBuf core=RAM_S2P_BRAM
}
/**
 * \brief     Memory resource pragma instantiation for the sliding window generator, URAM resource
 * 
 * The buffer in the sliding window generator can be implemented in multiple hardware resources. 
 * 
 * ap_resource_dflt will let HLS choose the best one
 * ap_resource_bram will force HLS to implement the buffer in BRAMs
 * ap_resource_uram will force HLS to implement the buffer in URAMs
 * ap_resource_lutram will force HLS to implement the buffer in LUTRAMs
 *
 * \tparam     T		Datatype of the buffer instantiated in the sliding window generator
 * 
 * \param      inputBuf	Buffer used in the SWG
 * \param      r     	Resource type for the hardware implementation
 *
 * \return     Result of the multiply operation
 */
template <typename T>
void memory_resource(T inputBuf, ap_resource_uram const&){
#pragma HLS inline
#pragma HLS RESOURCE variable=inputBuf core=RAM_S2P_URAM
}
/**
 * \brief     Memory resource pragma instantiation for the sliding window generator, LUTRAM resource
 * 
 * The buffer in the sliding window generator can be implemented in multiple hardware resources. 
 * 
 * ap_resource_dflt will let HLS choose the best one
 * ap_resource_bram will force HLS to implement the buffer in BRAMs
 * ap_resource_uram will force HLS to implement the buffer in URAMs
 * ap_resource_lutram will force HLS to implement the buffer in LUTRAMs
 *
 * \tparam     T		Datatype of the buffer instantiated in the sliding window generator
 * 
 * \param      inputBuf	Buffer used in the SWG
 * \param      r     	Resource type for the hardware implementation
 *
 * \return     Result of the multiply operation
 */
template <typename T>
void memory_resource(T inputBuf, ap_resource_lutram const&){
#pragma HLS inline
#pragma HLS RESOURCE variable=inputBuf core=RAM_S2P_LUTRAM
}

/**
 * \brief Sliding Window unit that produces output vectors for feeding
 * a Matrix_Vector_Activate_Batch, implementing the im2col algorithm. To be used only if 
 * ConvKernelDim%Stride = 0
 *
 * \tparam ConvKernelDim    Dimension of the convolutional kernel (assumed square)
 * \tparam IFMChannels      Number of Input Feature Maps
 * \tparam Input_precision  Number bits per pixel
 * \tparam IFMDim           Width and Heigth of the Input Feature Map (assumed square)
 * \tparam OFMDim           Width and Heigth of the Output Feature Map (assumed square)
 * \tparam SIMD             Number of input columns computed in parallel
 * \tparam Stride           Stride of the convolutional kernel
 * \tparam R          	  Datatype for the resource used for FPGA implementation of the SWG  - safely deducible from the paramaters
 *
 * \param in                Input stream
 * \param out               Output stream
 * \param numReps           Number of time the function has to be repeatedly executed (e.g. number of images)
 * \param r			  Resource type for the hardware implementation of the memory block
 */
template<unsigned int ConvKernelDim, 
		 unsigned int IFMChannels,
		 unsigned int Input_precision,		
		 unsigned int IFMDim, 
		 unsigned int OFMDim,
		 unsigned int SIMD,
		 unsigned int Stride, 
		 typename R>  
void ConvolutionInputGenerator(
		stream<ap_uint<SIMD*Input_precision> > & in,
		stream<ap_uint<SIMD*Input_precision> > & out,
		const unsigned int numReps,
		R const &r) {
  CASSERT_DATAFLOW(IFMChannels % SIMD == 0);
  CASSERT_DATAFLOW(ConvKernelDim % Stride == 0);
  const unsigned int multiplying_factor = IFMChannels/SIMD;
  const unsigned int number_blocks = ConvKernelDim/Stride + 1 ;
  ap_uint<SIMD*Input_precision> inputBuf[number_blocks][Stride * IFMDim * multiplying_factor];
#pragma HLS ARRAY_PARTITION variable=inputBuf complete dim=1
  memory_resource(inputBuf, r);
  const unsigned int cycles_write_block = (OFMDim * ConvKernelDim * ConvKernelDim * multiplying_factor);
  const unsigned int cycles_read_block = Stride * IFMDim * multiplying_factor;
  const unsigned int max_cycles = MAX(cycles_write_block,cycles_read_block);
  const unsigned int baseIter = IFMDim * ConvKernelDim * multiplying_factor// Initial buffer
			                  + OFMDim * MAX(cycles_write_block,cycles_read_block);
  unsigned int counter_internal_block = 0;
  unsigned int current_block_write = 0;
  unsigned int next_block_write = 0;	
  unsigned int current_line = 0;
  unsigned int read_block = 0; 
  unsigned int inp = 0, ofm_y = 0, ofm_x = 0, k_y = 0, k_x = 0, count_simd =0;
#pragma HLS reset variable=inp
  for (unsigned int count_image = 0; count_image < numReps; count_image++) {
    for (unsigned int i = 0; i < baseIter; i++) {
#pragma HLS PIPELINE II=1
      if (inp < IFMDim * ConvKernelDim*multiplying_factor) {// Initial buffer of ConvKernelDim lines	
        ap_uint<SIMD*Input_precision> inElem;
        inElem = in.read();
        inputBuf[current_block_write][current_line] = inElem;
        current_line++;
        inp++;
        if (current_line == Stride * IFMDim * multiplying_factor ) {
          current_line = 0;
          current_block_write++;
          if (current_block_write == number_blocks) {
            current_block_write=0;
          }
          read_block++;
          counter_internal_block = 0;
        }
      } else {
        if (counter_internal_block < cycles_write_block-1) { // We are writing output, MMV IFMChan per cycle
          unsigned int current_block_read = (current_block_write + 1 + k_y / Stride);
          if (current_block_read >= number_blocks) {
            current_block_read-= number_blocks;
		  }
          unsigned int current_line_in_block = ((k_y%Stride) * IFMDim + ofm_x*Stride + k_x)*multiplying_factor + count_simd;
          ap_uint<SIMD*Input_precision> outElem = inputBuf[current_block_read][(current_line_in_block)];
          out.write(outElem);
          count_simd++;
          if (count_simd == multiplying_factor) {
            count_simd=0;					
            k_x++;
            if (k_x == ConvKernelDim) {
              k_x = 0;
              k_y++;
              if (k_y == ConvKernelDim) {
                k_y = 0;
                ofm_x ++;
                if (ofm_x == OFMDim) {
                  ofm_x = 0;
                  ofm_y++;
                  if (ofm_y == OFMDim) {
                    ofm_y = 0;
                    inp = 0;
                  }
                }
              }
            }
          }
        }
        if ((counter_internal_block < cycles_read_block-1) && (read_block<IFMDim/Stride)) { // In parallel we write in the buffer, in the current block write if we still need to
          ap_uint<SIMD*Input_precision> inElem;
          inElem = in.read();
          inputBuf[current_block_write][current_line] = inElem;
#pragma AP dependence variable=inputBuf intra false
#pragma AP dependence variable=inputBuf inter false
          current_line++;
          if (current_line == Stride * IFMDim * multiplying_factor) {// We read the whole block, we change the next block in which we want to we
            // We filled up a block, let's not read until
            current_line = 0;
            read_block++;
            current_block_write++;
            if (current_block_write == number_blocks) {
              current_block_write=0;
			}
#pragma AP dependence variable=current_block_write intra false	
          }
        }
        counter_internal_block++; // = (counter_internal_block +1) % max_cycles;
        if (counter_internal_block == (max_cycles-1)) {
          counter_internal_block = 0;
        }
      }
    } // End base_iter
	read_block = 0;
  } // End count_image
} // End generator

/**
 * \brief Sliding Window unit that produces output vectors for feeding
 * a Matrix_Vector_Activate_Batch, implementing the im2col algorithm with support to multiple output pixels
 *
 *
 * \tparam ConvKernelDim    Dimension of the convolutional kernel (assumed square)
 * \tparam IFMChannels      Number of Input Feature Maps
 * \tparam Input_precision  Number bits per pixel
 * \tparam IFMDim           Width and Heigth of the Input Feature Map (assumed square)
 * \tparam OFMDim           Width and Heigth of the Output Feature Map (assumed square)
 * \tparam SIMD             Number of input columns computed in parallel
 * \tparam Stride           Stride of the convolutional kernel
 * \tparam MMV              Number of pixels that have to be produced in parallel
 * \tparam R          	  Datatype for the resource used for FPGA implementation of the SWG  - safely deducible from the paramaters
 *
 * \param in                Input stream
 * \param out               Output stream
 * \param numReps           Number of time the function has to be repeatedly executed (e.g. number of images)
 * \param r			  Resource type for the hardware implementation of the memory block
 */
template<unsigned int ConvKernelDim, 
		unsigned int IFMChannels,
		unsigned int Input_precision,
		unsigned int IFMDim, 
		unsigned int OFMDim,
		unsigned int SIMD,
		unsigned int Stride, 
		unsigned int MMV, 
		typename R>   
void ConvolutionInputGenerator_MMV(
		stream<ap_uint<SIMD*Input_precision> > & in,
		stream<MultiChanData<MMV, SIMD*Input_precision> > & out,
		const unsigned int numReps,
		R const &r) {
  	CASSERT_DATAFLOW(IFMChannels % SIMD == 0);
  	CASSERT_DATAFLOW(OFMDim % MMV == 0);
	CASSERT_DATAFLOW(ConvKernelDim % Stride == 0);
	CASSERT_DATAFLOW(MMV <= OFMDim);
	constexpr unsigned int multiplying_factor = IFMChannels/SIMD;
	constexpr unsigned int number_blocks = ConvKernelDim/Stride + 1 ;
  ap_uint<SIMD*Input_precision> inputBuf[MMV][number_blocks][Stride * IFMDim * multiplying_factor];
#pragma HLS DEPENDENCE variable=inputBuf inter false
#pragma HLS DEPENDENCE variable=inputBuf intra false
#pragma HLS ARRAY_PARTITION variable=inputBuf complete dim=1
#pragma HLS ARRAY_PARTITION variable=inputBuf complete dim=2
	memory_resource(inputBuf, r);
	constexpr unsigned int cycles_write_block = (OFMDim * ConvKernelDim * ConvKernelDim * multiplying_factor)/MMV;
	constexpr unsigned int cycles_read_block = Stride * IFMDim * multiplying_factor;
	constexpr unsigned int max_cycles = MAX(cycles_write_block,cycles_read_block);
	const unsigned int baseIter = IFMDim * ConvKernelDim * multiplying_factor// Initial buffer
			+ OFMDim * MAX(cycles_write_block,cycles_read_block);
	unsigned int counter_internal_block = 0;
	unsigned int current_block_write = 0;
	unsigned int next_block_write = 0;	
	unsigned int current_line = 0;
	unsigned int read_block = 0; 
	unsigned int inp = 0, ofm_y = 0, ofm_x = 0, k_y = 0, k_x = 0, count_simd =0;
#pragma HLS reset variable=inp
	for (unsigned int count_image = 0; count_image < numReps; count_image++) {
		for (unsigned int i = 0; i < baseIter; i++) {
	#pragma HLS PIPELINE II=1
			if (inp < IFMDim * ConvKernelDim*multiplying_factor) // Initial buffer of ConvKernelDim lines
				{
				ap_uint<SIMD*Input_precision> inElem;
				inElem = in.read();
				for(unsigned int v = 0; v < MMV; v++)
					{
#pragma HLS UNROLL
					inputBuf[v][current_block_write][current_line] = inElem;
					}
				current_line++;
				inp++;
				if (current_line == Stride * IFMDim * multiplying_factor )
					{
					current_line = 0;
					current_block_write++;
					if (current_block_write == number_blocks)
						current_block_write=0;
					read_block++;
					counter_internal_block = 0;
					}
				}
			else
				{
				if (counter_internal_block < cycles_write_block-1) // We are writing output, MMV IFMChan per cycle
				{
					unsigned int current_block_read = (current_block_write + 1 + k_y / Stride);
					if (current_block_read >= number_blocks)
						current_block_read-= number_blocks;
					unsigned int current_line_in_block = ((k_y%Stride) * IFMDim + ofm_x*Stride + k_x)*multiplying_factor + count_simd;
					MultiChanData<MMV, SIMD*Input_precision> outElem;
					// parallel read from all input buffers
					for(unsigned int v = 0; v < MMV; v++) {
#pragma HLS UNROLL
						// each buffer's read addr is offset by its buffer index
						ap_uint<SIMD*Input_precision> temp_value = inputBuf[v][current_block_read][(current_line_in_block + v*Stride*multiplying_factor)];
						outElem.data[v] = temp_value;
					}
					out.write(outElem);
					count_simd++;
					if (count_simd == multiplying_factor) {
						count_simd=0;					
						k_x++;
						if (k_x == ConvKernelDim) {
							k_x = 0;
							k_y++;
							if (k_y == ConvKernelDim) {
								k_y = 0;
								ofm_x += MMV;
								if (ofm_x == OFMDim) {
									ofm_x = 0;
									ofm_y++;
									if (ofm_y == OFMDim) {
										ofm_y = 0;
										inp = 0;
									}
								}
							}
						}
					}
				}
				if ((counter_internal_block < cycles_read_block-1) && (read_block<IFMDim/Stride)) // In parallel we write in the buffer, in the current block write if we still need to
				{
					ap_uint<SIMD*Input_precision> inElem;
					inElem = in.read();
					for(unsigned int v = 0; v < MMV; v++) {
#pragma HLS UNROLL
						inputBuf[v][current_block_write][current_line] = inElem;
#pragma AP dependence variable=inputBuf intra false
#pragma AP dependence variable=inputBuf inter false
						}

					current_line++;
					if (current_line == Stride * IFMDim * multiplying_factor) // We read the whole block, we change the next block in which we want to we
					{ // We filled up a block, let's not read until
						current_line = 0;
						read_block++;
						current_block_write++;
						if (current_block_write == number_blocks)
							current_block_write=0;
#pragma AP dependence variable=current_block_write intra false	
					}
				}
				counter_internal_block++; // = (counter_internal_block +1) % max_cycles;
				if (counter_internal_block == (max_cycles-1))
				{
					counter_internal_block = 0;
				}
			}
		} // End base_iter
	read_block = 0;
	} // End count_image
} // End generator


/**
 * \brief Sliding Window unit that produces output vectors for feeding
 * a Matrix_Vector_Activate_Batch, implementing the im2col algorithm. To be used when 
 * ConvKernelDim%Stride != 0 (e.g., Kernel=3, Stride=2)
 *
 * \tparam ConvKernelDim    Dimension of the convolutional kernel (assumed square)
 * \tparam IFMChannels      Number of Input Feature Maps
 * \tparam Input_precision  Number bits per pixel
 * \tparam IFMDim           Width and Heigth of the Input Feature Map (assumed square)
 * \tparam OFMDim           Width and Heigth of the Output Feature Map (assumed square)
 * \tparam SIMD             Number of input columns computed in parallel
 * \tparam Stride           Stride of the convolutional kernel
 * \tparam R          	  Datatype for the resource used for FPGA implementation of the SWG  - safely deducible from the paramaters
 *
 * \param in                Input stream
 * \param out               Output stream
 * \param numReps           Number of time the function has to be repeatedly executed (e.g. number of images)
 * \param r			  Resource type for the hardware implementation of the memory block
 */

template<unsigned int ConvKernelDim, 
		 unsigned int IFMChannels,
		 unsigned int Input_precision,		
		 unsigned int IFMDim, 
		 unsigned int OFMDim,
		 unsigned int SIMD,
		 unsigned int Stride, 
		 typename R>  
void ConvolutionInputGenerator_kernel_stride(  
		stream<ap_uint<SIMD*Input_precision> > & in,
		stream<ap_uint<SIMD*Input_precision> > & out,
		const unsigned int numReps,
		R const &r) {
	CASSERT_DATAFLOW(IFMChannels % SIMD == 0);
    CASSERT_DATAFLOW(ConvKernelDim % Stride != 0);
	const unsigned int multiplying_factor = IFMChannels/SIMD;
	const unsigned int number_blocks = ConvKernelDim + Stride ;
	ap_uint<SIMD*Input_precision> inputBuf[number_blocks][IFMDim * multiplying_factor];
#pragma HLS ARRAY_PARTITION variable=inputBuf complete dim=1
    memory_resource(inputBuf, r);
	const unsigned int cycles_write_block = OFMDim * ConvKernelDim * ConvKernelDim * multiplying_factor;
	const unsigned int cycles_read_block = IFMDim * Stride * multiplying_factor;
	const unsigned int max_cycles = MAX(cycles_write_block, cycles_read_block);
	const unsigned int baseIter = (IFMDim * ConvKernelDim * multiplying_factor) + (OFMDim-1) * max_cycles+MAX(cycles_write_block,OFMDim);
	const unsigned int initial_buffer_cycles = (IFMDim * ConvKernelDim * multiplying_factor) ;
	unsigned int counter_internal_block = 0;
	unsigned int next_block_write = 0;
	unsigned int current_line = 0;

	unsigned int inp = 0, ofm_y = 0, ofm_x = 0, k_y = 0, k_x = 0, current_k_y = 0, count_simd =0;
#pragma HLS RESET variable=inp

#pragma HLS DEPENDENCE variable=inputBuf inter false
#pragma HLS DEPENDENCE variable=inputBuf intra false

// #pragma HLS RESOURCE variable inputBuf core=RAM_2P_LUTRAM
for (unsigned int count_image = 0; count_image < numReps; count_image++) {
  unsigned int floor_block_read = 0, ceil_block_read = number_blocks;
  unsigned int current_block_write = 0;
  #pragma HLS DEPENDENCE variable=current_block_write intra false
  unsigned int read_block = 0;
		for (unsigned int i = 0; i < baseIter; i++) {
	#pragma HLS PIPELINE II=1
			if (inp < initial_buffer_cycles) // Initial buffer of PoolDim lines
			{
				ap_uint<SIMD*Input_precision> inElem;
				inElem = in.read();
				inputBuf[current_block_write][current_line] = inElem;
				current_line++;
				inp++;
				if (current_line == IFMDim * multiplying_factor)
				{
					current_line = 0;
					current_block_write++;
					if (current_block_write == number_blocks)
						current_block_write = 0;
					read_block++;
					counter_internal_block = 0;
				}
			}
			else
			{
				if (counter_internal_block < cycles_write_block-1 || read_block==IFMDim) // We are writing output, MMV IFMChan per cycle
				{
					//following code implements: current_block_read = (ofm_y*Stride + k_y)%number_blocks;
          unsigned int current_block_read = (ofm_y*Stride + k_y);
            //reminder computation
            if (current_block_read >= ceil_block_read)
            {
              floor_block_read += number_blocks;
              ceil_block_read += number_blocks;
            }else if(current_block_read < floor_block_read){
              ceil_block_read -= number_blocks;
              floor_block_read -= number_blocks;
            }
            current_block_read -= floor_block_read;

					unsigned int current_line_in_block = (ofm_x * Stride + k_x)*multiplying_factor + count_simd;
					ap_uint<SIMD*Input_precision> outElem = inputBuf[current_block_read][(current_line_in_block)];
					out.write(outElem);
					count_simd++;
					if (count_simd == multiplying_factor) {
						count_simd=0;	
						k_x++;
						if (k_x == ConvKernelDim) {
							k_x = 0;
							k_y++;
							if (k_y == ConvKernelDim) {
								k_y = 0;
								ofm_x++;
								if (ofm_x == OFMDim) {
									ofm_x = 0;
									ofm_y++;
									if (ofm_y == OFMDim) {
										ofm_y = 0;
										inp = 0;
									}
								}
							}
						}
					}
				}
				if ((counter_internal_block < cycles_read_block - 1) && (read_block<IFMDim)) // In parallel we write in the buffer, in the current block write if we still need to
				{
					ap_uint<SIMD*Input_precision> inElem;
					inElem = in.read();
					inputBuf[current_block_write][current_line] = inElem;
#pragma HLS DEPENDENCE variable=inputBuf inter false
#pragma HLS DEPENDENCE variable=inputBuf intra false
					current_line++;
					if (current_line == IFMDim * multiplying_factor) // We read the whole block, we change the next block in which we want to we
					{ // We filled up a block, let's not read until
						current_line = 0;
						read_block++;
						current_block_write++;
						if (current_block_write == number_blocks)
							current_block_write = 0;
#pragma HLS DEPENDENCE variable=current_block_write intra false
					}
				}
				counter_internal_block++; // = (counter_internal_block +1) % max_cycles;
                if (counter_internal_block == (max_cycles-1))
				{
				   counter_internal_block = 0;
				}
			}
		} // End base_iter
  }
}

/**
 * \brief Sliding Window unit that produces output vectors for feeding
 * a Matrix_Vector_Activate_Batch, implementing the im2col algorithm. To be used when 
 * ConvKernelDim%Stride != 0 (e.g., Kernel=3, Stride=2)
 *
 * \tparam ConvKernelDim    Dimension of the convolutional kernel (assumed square)
 * \tparam IFMChannels      Number of Input Feature Maps
 * \tparam Input_precision  Number bits per pixel
 * \tparam IFMDim           Width and Heigth of the Input Feature Map (assumed square)
 * \tparam OFMDim           Width and Heigth of the Output Feature Map (assumed square)
 * \tparam SIMD             Number of input columns computed in parallel
 * \tparam Stride           Stride of the convolutional kernel
 * \tparam MMV              Number of pixels that have to be produced in parallel
 * \tparam R          	  Datatype for the resource used for FPGA implementation of the SWG  - safely deducible from the paramaters
 *
 * \param in                Input stream
 * \param out               Output stream
 * \param numReps           Number of time the function has to be repeatedly executed (e.g. number of images)
 * \param r			  Resource type for the hardware implementation of the memory block
 */

template<unsigned int ConvKernelDim, 
		 unsigned int IFMChannels,
		 unsigned int Input_precision,		
		 unsigned int IFMDim, 
		 unsigned int OFMDim,
		 unsigned int SIMD,
		 unsigned int Stride, 
		 unsigned int MMV, 
		 typename R>  
void ConvolutionInputGenerator_kernel_stride_MMV(  
		stream<ap_uint<SIMD*Input_precision> > & in,
		stream<MultiChanData<MMV, SIMD*Input_precision> > & out,
		const unsigned int numReps,
		R const &r) {
	CASSERT_DATAFLOW(IFMChannels % SIMD == 0);
    CASSERT_DATAFLOW(ConvKernelDim % Stride != 0);
	CASSERT_DATAFLOW(OFMDim % MMV == 0);
	CASSERT_DATAFLOW(MMV <= OFMDim);

	const unsigned int multiplying_factor = IFMChannels/SIMD;
	const unsigned int number_blocks = ConvKernelDim + Stride ;
	ap_uint<SIMD*Input_precision> inputBuf[MMV][number_blocks][IFMDim * multiplying_factor];
#pragma HLS DEPENDENCE variable=inputBuf inter false
#pragma HLS DEPENDENCE variable=inputBuf intra false
#pragma HLS ARRAY_PARTITION variable=inputBuf complete dim=1
#pragma HLS ARRAY_PARTITION variable=inputBuf complete dim=2
    memory_resource(inputBuf, r);
	const unsigned int cycles_write_block = (OFMDim * ConvKernelDim * ConvKernelDim * multiplying_factor)/MMV;
	const unsigned int cycles_read_block = IFMDim * Stride * multiplying_factor;
	const unsigned int max_cycles = MAX(cycles_write_block, cycles_read_block);
	const unsigned int baseIter = (IFMDim * ConvKernelDim * multiplying_factor) + (OFMDim-1) * max_cycles+MAX(cycles_write_block,OFMDim);
	const unsigned int initial_buffer_cycles = (IFMDim * ConvKernelDim * multiplying_factor) ;
	unsigned int counter_internal_block = 0;
	unsigned int next_block_write = 0;
	unsigned int current_line = 0;

	unsigned int inp = 0, ofm_y = 0, ofm_x = 0, k_y = 0, k_x = 0, current_k_y = 0, count_simd =0;
#pragma HLS RESET variable=inp

for (unsigned int count_image = 0; count_image < numReps; count_image++) {
  unsigned int floor_block_read = 0, ceil_block_read = number_blocks;
  unsigned int current_block_write = 0;
  #pragma HLS DEPENDENCE variable=current_block_write intra false
  unsigned int read_block = 0;
		for (unsigned int i = 0; i < baseIter; i++) {
	#pragma HLS PIPELINE II=1
			if (inp < initial_buffer_cycles) // Initial buffer of PoolDim lines
			{
				ap_uint<SIMD*Input_precision> inElem;
				inElem = in.read();
				for(unsigned int v = 0; v < MMV; v++)
					{
#pragma HLS UNROLL
					inputBuf[v][current_block_write][current_line] = inElem;
					}
				current_line++;
				inp++;
				if (current_line == IFMDim * multiplying_factor)
				{
					current_line = 0;
					current_block_write++;
					if (current_block_write == number_blocks)
						current_block_write = 0;
					read_block++;
					counter_internal_block = 0;
				}
			}
			else
			{
				if (counter_internal_block < cycles_write_block-1 || read_block==IFMDim) // We are writing output, MMV IFMChan per cycle
				{
					//following code implements: current_block_read = (ofm_y*Stride + k_y)%number_blocks;
            unsigned int current_block_read = (ofm_y*Stride + k_y);
            //reminder computation
            if (current_block_read >= ceil_block_read)
            {
              floor_block_read += number_blocks;
              ceil_block_read += number_blocks;
            }else if(current_block_read < floor_block_read){
              ceil_block_read -= number_blocks;
              floor_block_read -= number_blocks;
            }
            current_block_read -= floor_block_read;
			unsigned int current_line_in_block = (ofm_x * Stride + k_x)*multiplying_factor + count_simd;
			MultiChanData<MMV, SIMD*Input_precision> outElem;
			for(unsigned int v = 0; v < MMV; v++) {
#pragma HLS UNROLL
				// each buffer's read addr is offset by its buffer index
				ap_uint<SIMD*Input_precision> temp_value = inputBuf[v][current_block_read][(current_line_in_block + v*Stride*multiplying_factor)];
				outElem.data[v] = temp_value;
			}
			out.write(outElem);
			count_simd++;
			if (count_simd == multiplying_factor) {
				count_simd=0;	
				k_x++;
				if (k_x == ConvKernelDim) {
					k_x = 0;
					k_y++;
					if (k_y == ConvKernelDim) {
						k_y = 0;
						ofm_x += MMV;
						if (ofm_x == OFMDim) {
							ofm_x = 0;
							ofm_y++;
							if (ofm_y == OFMDim) {
								ofm_y = 0;
								inp = 0;
								}
							}
						}
					}
				}
			}
			if ((counter_internal_block < cycles_read_block - 1) && (read_block<IFMDim)) // In parallel we write in the buffer, in the current block write if we still need to
			{
				ap_uint<SIMD*Input_precision> inElem;
				inElem = in.read();
				for(unsigned int v = 0; v < MMV; v++) {
#pragma HLS UNROLL
					inputBuf[v][current_block_write][current_line] = inElem;
					}
				current_line++;
				if (current_line == IFMDim * multiplying_factor) // We read the whole block, we change the next block in which we want to we
				{ // We filled up a block, let's not read until
					current_line = 0;
					read_block++;
					current_block_write++;
					if (current_block_write == number_blocks)
						current_block_write = 0;
#pragma HLS DEPENDENCE variable=current_block_write intra false
				}
			}
			counter_internal_block++; // = (counter_internal_block +1) % max_cycles;
			if (counter_internal_block == (max_cycles-1))
			{
			   counter_internal_block = 0;
			}
		}
	} // End base_iter
  }
}


/**
 * \brief Sliding Window unit that produces output vectors for feeding
 * a Vector_Vector_Activate_Batch, implementing the im2col algorithm for depthwise separable convolutions. To be used only if 
 * ConvKernelDim%Stride = 0 and square kernel
 *
 * \tparam ConvKernelDim    Dimension of the convolutional kernel (assumed square)
 * \tparam IFMChannels      Number of Input Feature Maps
 * \tparam Input_precision  Number bits per pixel
 * \tparam IFMDim           Width and Heigth of the Input Feature Map (assumed square)
 * \tparam OFMDim           Width and Heigth of the Output Feature Map (assumed square)
 * \tparam SIMD             Number of input columns computed in parallel
 * \tparam Stride           Stride of the convolutional kernel
 * \tparam R          	  Datatype for the resource used for FPGA implementation of the SWG  - safely deducible from the paramaters
 *
 * \param in                Input stream
 * \param out               Output stream
 * \param numReps           Number of time the function has to be repeatedly executed (e.g. number of images)
 * \param r			  Resource type for the hardware implementation of the memory block
 */
template<unsigned int ConvKernelDim, 
		 unsigned int IFMChannels,
		 unsigned int Input_precision,		
		 unsigned int IFMDim, 
		 unsigned int OFMDim,
		 unsigned int SIMD,
		 unsigned int Stride, 
		 typename R>  
void ConvolutionInputGenerator_dws(
		stream<ap_uint<SIMD*Input_precision> > & in,
		stream<ap_uint<SIMD*Input_precision> > & out,
		const unsigned int numReps,
		R const &r) {
  CASSERT_DATAFLOW(IFMChannels % SIMD == 0);
  CASSERT_DATAFLOW(ConvKernelDim % Stride == 0);
  const unsigned int multiplying_factor = IFMChannels/SIMD;
  const unsigned int number_blocks = ConvKernelDim/Stride + 1 ;
  ap_uint<SIMD*Input_precision> inputBuf[number_blocks][Stride * IFMDim * multiplying_factor];
#pragma HLS ARRAY_PARTITION variable=inputBuf complete dim=1
  memory_resource(inputBuf, r);
  const unsigned int cycles_write_block = (OFMDim * ConvKernelDim * ConvKernelDim * multiplying_factor);
  const unsigned int cycles_read_block = Stride * IFMDim * multiplying_factor;
  const unsigned int max_cycles = MAX(cycles_write_block,cycles_read_block);
  const unsigned int baseIter = IFMDim * ConvKernelDim * multiplying_factor// Initial buffer
			                  + OFMDim * MAX(cycles_write_block,cycles_read_block);
  unsigned int counter_internal_block = 0;
  unsigned int current_block_write = 0;
  unsigned int next_block_write = 0;	
  unsigned int current_line = 0;
  unsigned int read_block = 0; 
  unsigned int inp = 0, ofm_y = 0, ofm_x = 0, k_y = 0, k_x = 0, count_simd =0;
#pragma HLS reset variable=inp
  for (unsigned int count_image = 0; count_image < numReps; count_image++) {
    for (unsigned int i = 0; i < baseIter; i++) {
#pragma HLS PIPELINE II=1
      if (inp < IFMDim * ConvKernelDim*multiplying_factor) {// Initial buffer of ConvKernelDim lines	
        ap_uint<SIMD*Input_precision> inElem;
        inElem = in.read();
        inputBuf[current_block_write][current_line] = inElem;
        current_line++;
        inp++;
        if (current_line == Stride * IFMDim * multiplying_factor ) {
          current_line = 0;
          current_block_write++;
          if (current_block_write == number_blocks) {
            current_block_write=0;
          }
          read_block++;
          counter_internal_block = 0;
        }
      } else {
        if (counter_internal_block < cycles_write_block-1) { // We are writing output, MMV IFMChan per cycle
          unsigned int current_block_read = (current_block_write + 1 + k_y / Stride);
          if (current_block_read >= number_blocks) {
            current_block_read-= number_blocks;
		  }
          unsigned int current_line_in_block = ((k_y%Stride) * IFMDim + ofm_x*Stride + k_x)*multiplying_factor + count_simd;
          ap_uint<SIMD*Input_precision> outElem = inputBuf[current_block_read][(current_line_in_block)];
          out.write(outElem);		
		  k_x++;
		  if (k_x == ConvKernelDim) {
		    k_x = 0;
		    k_y++;
		    if (k_y == ConvKernelDim) {
			  k_y = 0;
			  count_simd++;
			  if (count_simd == multiplying_factor) {
			    count_simd=0;	
                ofm_x ++;
                if (ofm_x == OFMDim) {
                  ofm_x = 0;
                  ofm_y++;
                  if (ofm_y == OFMDim) {
                    ofm_y = 0;
                    inp = 0;
                  }
                }
              }
            }
          }
        }
        if ((counter_internal_block < cycles_read_block-1) && (read_block<IFMDim/Stride)) { // In parallel we write in the buffer, in the current block write if we still need to
          ap_uint<SIMD*Input_precision> inElem;
          inElem = in.read();
          inputBuf[current_block_write][current_line] = inElem;
#pragma AP dependence variable=inputBuf intra false
#pragma AP dependence variable=inputBuf inter false
          current_line++;
          if (current_line == Stride * IFMDim * multiplying_factor) {// We read the whole block, we change the next block in which we want to we
            // We filled up a block, let's not read until
            current_line = 0;
            read_block++;
            current_block_write++;
            if (current_block_write == number_blocks) {
              current_block_write=0;
			}
#pragma AP dependence variable=current_block_write intra false	
          }
        }
        counter_internal_block++; // = (counter_internal_block +1) % max_cycles;
        if (counter_internal_block == (max_cycles-1)) {
          counter_internal_block = 0;
        }
      }
    } // End base_iter
	read_block = 0;
  } // End count_image
} // End generator


/**
 * \brief Sliding Window unit that produces output vectors for feeding
 * a Vector_Vector_Activate_Batch, implementing the im2col algorithm for depthwise separable convolutions. To be used when 
 * ConvKernelDim%Stride != 0 (e.g., Kernel=3, Stride=2)
 *
 * \tparam ConvKernelDim    Dimension of the convolutional kernel (assumed square)
 * \tparam IFMChannels      Number of Input Feature Maps
 * \tparam Input_precision  Number bits per pixel
 * \tparam IFMDim           Width and Heigth of the Input Feature Map (assumed square)
 * \tparam OFMDim           Width and Heigth of the Output Feature Map (assumed square)
 * \tparam SIMD             Number of input columns computed in parallel
 * \tparam Stride           Stride of the convolutional kernel
 * \tparam R          	  Datatype for the resource used for FPGA implementation of the SWG  - safely deducible from the paramaters
 * 
 * \param in                Input stream
 * \param out               Output stream
 * \param numReps           Number of time the function has to be repeatedly executed (e.g. number of images)
 * \param r			  Resource type for the hardware implementation of the memory block
 */

template<unsigned int ConvKernelDim, 
         unsigned int IFMChannels,
         unsigned int Input_precision,      
         unsigned int IFMDim, 
         unsigned int OFMDim,
         unsigned int SIMD,
         unsigned int Stride, 
         typename R>  
void ConvolutionInputGenerator_kernel_stride_dws(  
    stream<ap_uint<SIMD*Input_precision> > & in,
    stream<ap_uint<SIMD*Input_precision> > & out,
    const unsigned int numReps,
    R const &r) {
    CASSERT_DATAFLOW(IFMChannels % SIMD == 0);
    CASSERT_DATAFLOW(ConvKernelDim % Stride != 0);
    const unsigned int multiplying_factor = IFMChannels/SIMD;
    const unsigned int number_blocks = ConvKernelDim + Stride ;
    ap_uint<SIMD*Input_precision> inputBuf[number_blocks][IFMDim * multiplying_factor];
#pragma HLS ARRAY_PARTITION variable=inputBuf complete dim=1
    memory_resource(inputBuf, r);
    const unsigned int cycles_write_block = OFMDim * ConvKernelDim * ConvKernelDim * multiplying_factor;
    const unsigned int cycles_read_block = IFMDim * Stride * multiplying_factor;
    const unsigned int max_cycles = MAX(cycles_write_block, cycles_read_block);
    const unsigned int baseIter = (IFMDim * ConvKernelDim * multiplying_factor) + (OFMDim-1) * max_cycles+MAX(cycles_write_block,OFMDim);
    const unsigned int initial_buffer_cycles = (IFMDim * ConvKernelDim * multiplying_factor) ;
    unsigned int counter_internal_block = 0;
    unsigned int next_block_write = 0;
    unsigned int current_line = 0;
    unsigned int inp = 0, ofm_y = 0, ofm_x = 0, k_y = 0, k_x = 0, current_k_y = 0, count_simd =0;
#pragma HLS RESET variable=inp
  
#pragma HLS DEPENDENCE variable=inputBuf inter false
#pragma HLS DEPENDENCE variable=inputBuf intra false

// #pragma HLS RESOURCE variable inputBuf core=RAM_2P_LUTRAM
  for (unsigned int count_image = 0; count_image < numReps; count_image++) {
      unsigned int floor_block_read = 0, ceil_block_read = number_blocks;
      unsigned int read_block = 0;
      unsigned int current_block_write = 0;
      for (unsigned int i = 0; i < baseIter; i++) {
      #pragma HLS PIPELINE II=1

      #pragma HLS DEPENDENCE variable=current_block_write intra false

            if (inp < initial_buffer_cycles) // Initial buffer of PoolDim lines
            {
                ap_uint<SIMD*Input_precision> inElem;
                inElem = in.read();
                inputBuf[current_block_write][current_line] = inElem;
                current_line++;
                inp++;
                if (current_line == IFMDim * multiplying_factor)
                {
                    current_line = 0;
                    current_block_write++;
                    if (current_block_write == number_blocks)
                        current_block_write = 0;
                    read_block++;
                    counter_internal_block = 0;
                }
            }
            else
            {
                if (counter_internal_block < cycles_write_block-1 || read_block==IFMDim) // We are writing output, MMV IFMChan per cycle
                {
          //following code implements: current_block_read = (ofm_y*Stride + k_y)%number_blocks;
            unsigned int current_block_read = (ofm_y*Stride + k_y);
            //reminder computation
            if (current_block_read >= ceil_block_read)
            {
              floor_block_read += number_blocks;
              ceil_block_read += number_blocks;
            }else if(current_block_read < floor_block_read){
              ceil_block_read -= number_blocks;
              floor_block_read -= number_blocks;
            }
            current_block_read -= floor_block_read;

                    unsigned int current_line_in_block = (ofm_x * Stride + k_x)*multiplying_factor + count_simd;
                    ap_uint<SIMD*Input_precision> outElem = inputBuf[current_block_read][(current_line_in_block)];
                    out.write(outElem);
                    k_x++;
                    if (k_x == ConvKernelDim) {
                        k_x = 0;
                        k_y++;
                        if (k_y == ConvKernelDim) {
                            k_y = 0;
                            count_simd++;
                            if (count_simd == multiplying_factor) {
                                count_simd=0;   
                                ofm_x++;
                                if (ofm_x == OFMDim) {
                                    ofm_x = 0;
                                    ofm_y++;
                                    if (ofm_y == OFMDim) {
                                        ofm_y = 0;
                                        inp = 0;
                                    }
                                }
                            }
                        }
                    }
                }
                if ((counter_internal_block < cycles_read_block - 1) && (read_block<IFMDim)) // In parallel we write in the buffer, in the current block write if we still need to
                {
                    ap_uint<SIMD*Input_precision> inElem;
                    inElem = in.read();
                    inputBuf[current_block_write][current_line] = inElem;
#pragma HLS DEPENDENCE variable=inputBuf inter false
#pragma HLS DEPENDENCE variable=inputBuf intra false
                    current_line++;
                    if (current_line == IFMDim * multiplying_factor) // We read the whole block, we change the next block in which we want to we
                    { // We filled up a block, let's not read until
                        current_line = 0;
                        read_block++;
                        current_block_write++;
                        if (current_block_write == number_blocks)
                            current_block_write = 0;
    #pragma HLS DEPENDENCE variable=current_block_write intra false
                    }
                }
                counter_internal_block++; // = (counter_internal_block +1) % max_cycles;
                if (counter_internal_block == (max_cycles-1))
                {
                   counter_internal_block = 0;
                }
            }
        } // End base_iter
  }
}


/**
 * \brief Sliding Window unit that produces output vectors for feeding
 * a Vector_Vector_Activate_Batch, implementing the im2col algorithm  for depthwise separable convolutions with support to multiple output pixels
 *
 *
 * \tparam ConvKernelDim    Dimension of the convolutional kernel (assumed square)
 * \tparam IFMChannels      Number of Input Feature Maps
 * \tparam Input_precision  Number bits per pixel
 * \tparam IFMDim           Width and Heigth of the Input Feature Map (assumed square)
 * \tparam OFMDim           Width and Heigth of the Output Feature Map (assumed square)
 * \tparam SIMD             Number of input columns computed in parallel
 * \tparam Stride           Stride of the convolutional kernel
 * \tparam MMV              Number of pixels that have to be produced in parallel
 * \tparam R          	  Datatype for the resource used for FPGA implementation of the SWG  - safely deducible from the paramaters
 *
 * \param in                Input stream
 * \param out               Output stream
 * \param numReps           Number of time the function has to be repeatedly executed (e.g. number of images)
 * \param r			  Resource type for the hardware implementation of the memory block
 */
template<unsigned int ConvKernelDim, 
		unsigned int IFMChannels,
		unsigned int Input_precision,
		unsigned int IFMDim, 
		unsigned int OFMDim,
		unsigned int SIMD,
		unsigned int Stride, 
		unsigned int MMV, 
		typename R>   
void ConvolutionInputGenerator_dws_MMV(
		stream<ap_uint<SIMD*Input_precision> > & in,
		stream<MultiChanData<MMV, SIMD*Input_precision> > & out,
		const unsigned int numReps,
		R const &r) {
  	CASSERT_DATAFLOW(IFMChannels % SIMD == 0);
  	CASSERT_DATAFLOW(OFMDim % MMV == 0);
	CASSERT_DATAFLOW(ConvKernelDim % Stride == 0);
	CASSERT_DATAFLOW(MMV <= OFMDim);
	constexpr unsigned int multiplying_factor = IFMChannels/SIMD;
	constexpr unsigned int number_blocks = ConvKernelDim/Stride + 1 ;
  ap_uint<SIMD*Input_precision> inputBuf[MMV][number_blocks][Stride * IFMDim * multiplying_factor];
#pragma HLS DEPENDENCE variable=inputBuf inter false
#pragma HLS DEPENDENCE variable=inputBuf intra false
#pragma HLS ARRAY_PARTITION variable=inputBuf complete dim=1
#pragma HLS ARRAY_PARTITION variable=inputBuf complete dim=2
	memory_resource(inputBuf, r);
	constexpr unsigned int cycles_write_block = (OFMDim * ConvKernelDim * ConvKernelDim * multiplying_factor)/MMV;
	constexpr unsigned int cycles_read_block = Stride * IFMDim * multiplying_factor;
	constexpr unsigned int max_cycles = MAX(cycles_write_block,cycles_read_block);
	const unsigned int baseIter = IFMDim * ConvKernelDim * multiplying_factor// Initial buffer
			+ OFMDim * MAX(cycles_write_block,cycles_read_block);
	unsigned int counter_internal_block = 0;
	unsigned int current_block_write = 0;
	unsigned int next_block_write = 0;	
	unsigned int current_line = 0;
	unsigned int read_block = 0; 
	unsigned int inp = 0, ofm_y = 0, ofm_x = 0, k_y = 0, k_x = 0, count_simd =0;
#pragma HLS reset variable=inp
	for (unsigned int count_image = 0; count_image < numReps; count_image++) {
		for (unsigned int i = 0; i < baseIter; i++) {
	#pragma HLS PIPELINE II=1
			if (inp < IFMDim * ConvKernelDim*multiplying_factor) // Initial buffer of ConvKernelDim lines
				{
				ap_uint<SIMD*Input_precision> inElem;
				inElem = in.read();
				for(unsigned int v = 0; v < MMV; v++)
					{
#pragma HLS UNROLL
					inputBuf[v][current_block_write][current_line] = inElem;
					}
				current_line++;
				inp++;
				if (current_line == Stride * IFMDim * multiplying_factor )
					{
					current_line = 0;
					current_block_write++;
					if (current_block_write == number_blocks)
						current_block_write=0;
					read_block++;
					counter_internal_block = 0;
					}
				}
			else
				{
				if (counter_internal_block < cycles_write_block-1) // We are writing output, MMV IFMChan per cycle
				{
					unsigned int current_block_read = (current_block_write + 1 + k_y / Stride);
					if (current_block_read >= number_blocks)
						current_block_read-= number_blocks;
					unsigned int current_line_in_block = ((k_y%Stride) * IFMDim + ofm_x*Stride + k_x)*multiplying_factor + count_simd;
					MultiChanData<MMV, SIMD*Input_precision> outElem;
					// parallel read from all input buffers
					for(unsigned int v = 0; v < MMV; v++) {
#pragma HLS UNROLL
						// each buffer's read addr is offset by its buffer index
						ap_uint<SIMD*Input_precision> temp_value = inputBuf[v][current_block_read][(current_line_in_block + v*Stride*multiplying_factor)];
						outElem.data[v] = temp_value;
					}
					out.write(outElem);			
					k_x++;
					if (k_x == ConvKernelDim) {
						k_x = 0;
						k_y++;
						if (k_y == ConvKernelDim) {
							k_y = 0;
							count_simd++;
							if (count_simd == multiplying_factor) {
								count_simd=0;	
								ofm_x += MMV;
								if (ofm_x == OFMDim) {
									ofm_x = 0;
									ofm_y++;
									if (ofm_y == OFMDim) {
										ofm_y = 0;
										inp = 0;
									}
								}
							}
						}
					}
				}
				if ((counter_internal_block < cycles_read_block-1) && (read_block<IFMDim/Stride)) // In parallel we write in the buffer, in the current block write if we still need to
				{
					ap_uint<SIMD*Input_precision> inElem;
					inElem = in.read();
					for(unsigned int v = 0; v < MMV; v++) {
#pragma HLS UNROLL
						inputBuf[v][current_block_write][current_line] = inElem;
#pragma AP dependence variable=inputBuf intra false
#pragma AP dependence variable=inputBuf inter false
						}

					current_line++;
					if (current_line == Stride * IFMDim * multiplying_factor) // We read the whole block, we change the next block in which we want to we
					{ // We filled up a block, let's not read until
						current_line = 0;
						read_block++;
						current_block_write++;
						if (current_block_write == number_blocks)
							current_block_write=0;
#pragma AP dependence variable=current_block_write intra false	
					}
				}
				counter_internal_block++; // = (counter_internal_block +1) % max_cycles;
				if (counter_internal_block == (max_cycles-1))
				{
					counter_internal_block = 0;
				}
			}
		} // End base_iter
	read_block = 0;
	} // End count_image
} // End generator



/**
 * \brief Sliding Window for 1x1 kernel with stride!=1
 * 
 * Basically performs a downsampling of the image removing rows and columns
 *
 * \tparam IFMChannels      Number of Input Feature Maps
 * \tparam Input_precision  Number bits per pixel
 * \tparam IFMDim           Width and Heigth of the Input Feature Map (assumed square)
 * \tparam SIMD             Number of input columns computed in parallel
 * \tparam Stride           Stride of the convolutional kernel
 *
 * \param in                Input stream
 * \param out               Output stream
 * \param numReps           Number of time the function has to be repeatedly executed (e.g. number of images)
 */
template<	unsigned int IFMChannels,
		unsigned int Input_precision,		
		unsigned int IFMDim, 	
		unsigned int SIMD,  
		unsigned int Stride>
void ConvolutionInputGenerator_kernel1(
		stream<ap_uint<SIMD*Input_precision> > & in,
		stream<ap_uint<SIMD*Input_precision> > & out, 
		const unsigned int numReps) {
CASSERT_DATAFLOW(IFMChannels % SIMD == 0);
	for (unsigned int im=0; im<numReps; im++) {
		for (unsigned int y = 0; y < IFMDim; y++) {
			for (unsigned int x = 0; x < IFMDim; x++) {
				for (unsigned int count_simd =0; count_simd < IFMChannels/SIMD; count_simd++) {
				#pragma HLS PIPELINE II=1
					ap_uint<SIMD*Input_precision> inElem = in.read();
					if ((x%Stride == 0)&&(y%Stride == 0)) {
						out.write(inElem);
					}
				}
			}
		}
	}		
}


/**
 * \brief Sliding Window unit that produces output vectors for feeding
 * a Matrix_Vector_Activate_Batch, implementing the im2col algorithm. To be used when kernel is not square
 *
 * \tparam ConvKernelDim_x    	Dimension of the convolutional kernel - x axis
 * \tparam ConvKernelDim_y    	Dimension of the convolutional kernel - y axis
 * \tparam IFMChannels      	Number of Input Feature Maps
 * \tparam Input_precision  	Number bits per pixel
 * \tparam IFMDim_x          	Width of the Input Feature Map
 * \tparam IFMDim_y           	Height of the Input Feature Map
 * \tparam OFMDim_x           	Width of the Output Feature Map
 * \tparam OFMDim_y           	Height of the Output Feature Map
 * \tparam SIMD             	Number of input columns computed in parallel
 * \tparam Stride_x           	Stride of the convolutional kernel - x axis
 * \tparam Stride_y          	Stride of the convolutional kernel - y axis
 * \tparam R          	  		Datatype for the resource used for FPGA implementation of the SWG  - safely deducible from the parameters
 *
 * \param in                	Input stream
 * \param out               	Output stream
 * \param numReps           	Number of time the function has to be repeatedly executed (e.g. number of images)
 * \param r			  			Resource type for the hardware implementation of the memory block
 */
template<unsigned int ConvKernelDim_x,
		 unsigned int ConvKernelDim_y,
		 unsigned int IFMChannels,
		 unsigned int Input_precision,
		 unsigned int IFMDim_x,
		 unsigned int IFMDim_y,
		 unsigned int OFMDim_x,
		 unsigned int OFMDim_y,
		 unsigned int SIMD,
		 unsigned int Stride_x,
		 unsigned int Stride_y,
		 typename R>
void ConvolutionInputGenerator_NonSquare(
		stream<ap_uint<SIMD*Input_precision> > & in,
		stream<ap_uint<SIMD*Input_precision> > & out,
		const unsigned int numReps,
		R const &r) {
  CASSERT_DATAFLOW(IFMChannels % SIMD == 0);
  const unsigned int multiplying_factor = IFMChannels/SIMD;
  const unsigned int number_blocks = ConvKernelDim_y/Stride_y + 1 ;
  ap_uint<SIMD*Input_precision> inputBuf[number_blocks][Stride_x * IFMDim_x * multiplying_factor];

#pragma HLS ARRAY_PARTITION variable=inputBuf complete dim=1
  memory_resource(inputBuf, r);
  const unsigned int cycles_write_block = (OFMDim_x * ConvKernelDim_x * ConvKernelDim_y * multiplying_factor);
  const unsigned int cycles_read_block = Stride_x * IFMDim_x * multiplying_factor;
  const unsigned int max_cycles = MAX(cycles_write_block,cycles_read_block);
  const unsigned int baseIter = IFMDim_x * ConvKernelDim_y * multiplying_factor// Initial buffer
			                  + OFMDim_y * MAX(cycles_write_block,cycles_read_block);
  unsigned int counter_internal_block = 0;
  unsigned int current_block_write = 0;
  unsigned int next_block_write = 0;
  unsigned int current_line = 0;
  unsigned int read_block = 0;
  unsigned int inp = 0, ofm_y = 0, ofm_x = 0, k_y = 0, k_x = 0, count_simd =0;
#pragma HLS reset variable=inp
  for (unsigned int count_image = 0; count_image < numReps; count_image++) {
    for (unsigned int i = 0; i < baseIter; i++) {
#pragma HLS PIPELINE II=1
      if (inp < IFMDim_x * ConvKernelDim_y *multiplying_factor) {// Initial buffer of ConvKernelDim lines
        ap_uint<SIMD*Input_precision> inElem;
        inElem = in.read();
        inputBuf[current_block_write][current_line] = inElem;
        current_line++;
        inp++;
        if (current_line == Stride_x * IFMDim_x * multiplying_factor ) {
          current_line = 0;
          current_block_write++;
          if (current_block_write == number_blocks) {
            current_block_write=0;
          }
          read_block++;
          counter_internal_block = 0;
        }
      } else {
        if (counter_internal_block < cycles_write_block-1) { // We are writing output, MMV IFMChan per cycle
          unsigned int current_block_read = (current_block_write + 1 + k_y / Stride_y);
          if (current_block_read >= number_blocks) {
            current_block_read-= number_blocks;
		  }
          unsigned int current_line_in_block = ((k_y%Stride_y) * IFMDim_y + ofm_x*Stride_x + k_x)*multiplying_factor + count_simd;
          ap_uint<SIMD*Input_precision> outElem = inputBuf[current_block_read][(current_line_in_block)];
          out.write(outElem);
          count_simd++;
          if (count_simd == multiplying_factor) {
            count_simd=0;
            k_x++;
            if (k_x == ConvKernelDim_x) {
              k_x = 0;
              k_y++;
              if (k_y == ConvKernelDim_y) {
                k_y = 0;
                ofm_x ++;
                if (ofm_x == OFMDim_x) {
                  ofm_x = 0;
                  ofm_y++;
                  if (ofm_y == OFMDim_y) {
                    ofm_y = 0;
                    inp = 0;
                  }
                }
              }
            }
          }
        }
        if ((counter_internal_block < cycles_read_block-1) && (read_block<IFMDim_y/Stride_y)) { // In parallel we write in the buffer, in the current block write if we still need to
          ap_uint<SIMD*Input_precision> inElem;
          inElem = in.read();
          inputBuf[current_block_write][current_line] = inElem;
#pragma AP dependence variable=inputBuf intra false
#pragma AP dependence variable=inputBuf inter false
          current_line++;
          if (current_line == Stride_x * IFMDim_x * multiplying_factor) {// We read the whole block, we change the next block in which we want to we
            // We filled up a block, let's not read until
            current_line = 0;
            read_block++;
            current_block_write++;
            if (current_block_write == number_blocks) {
              current_block_write=0;
			}
#pragma AP dependence variable=current_block_write intra false
          }
        }
        counter_internal_block++; // = (counter_internal_block +1) % max_cycles;
        if (counter_internal_block == (max_cycles-1)) {
          counter_internal_block = 0;
        }
      }
    } // End base_iter
	read_block = 0;
  } // End count_image
} // End generator

/**
 * \brief Sliding Window unit that produces output vectors for feeding
 * a Vector_Vector_Activate_Batch, implementing the im2col algorithm for depthwise separable convolutions. To be used when kernel is not square
 *
 * \tparam ConvKernelDim_x    	Dimension of the convolutional kernel - x axis
 * \tparam ConvKernelDim_y    	Dimension of the convolutional kernel - y axis
 * \tparam IFMChannels      	Number of Input Feature Maps
 * \tparam Input_precision  	Number bits per pixel
 * \tparam IFMDim_x          	Width of the Input Feature Map
 * \tparam IFMDim_y           	Height of the Input Feature Map
 * \tparam OFMDim_x           	Width of the Output Feature Map
 * \tparam OFMDim_y           	Height of the Output Feature Map
 * \tparam SIMD             	Number of input columns computed in parallel
 * \tparam Stride_x           	Stride of the convolutional kernel - x axis
 * \tparam Stride_y          	Stride of the convolutional kernel - y axis
 * \tparam R          	  		Datatype for the resource used for FPGA implementation of the SWG  - safely deducible from the parameters
 *
 * \param in                	Input stream
 * \param out               	Output stream
 * \param numReps           	Number of time the function has to be repeatedly executed (e.g. number of images)
 * \param r			  			Resource type for the hardware implementation of the memory block
 */
template<unsigned int ConvKernelDim_x,
		 unsigned int ConvKernelDim_y,
		 unsigned int IFMChannels,
		 unsigned int Input_precision,
		 unsigned int IFMDim_x,
		 unsigned int IFMDim_y,
		 unsigned int OFMDim_x,
		 unsigned int OFMDim_y,
		 unsigned int SIMD,
		 unsigned int Stride_x,
		 unsigned int Stride_y,
		 typename R>
void ConvolutionInputGenerator_NonSquare_dws(
		stream<ap_uint<SIMD*Input_precision> > & in,
		stream<ap_uint<SIMD*Input_precision> > & out,
		const unsigned int numReps,
		R const &r) {
  CASSERT_DATAFLOW(IFMChannels % SIMD == 0);
  const unsigned int multiplying_factor = IFMChannels/SIMD;
  const unsigned int number_blocks = ConvKernelDim_y/Stride_y + 1 ;
  ap_uint<SIMD*Input_precision> inputBuf[number_blocks][Stride_x * IFMDim_x * multiplying_factor];

#pragma HLS ARRAY_PARTITION variable=inputBuf complete dim=1
  memory_resource(inputBuf, r);
  const unsigned int cycles_write_block = (OFMDim_x * ConvKernelDim_x * ConvKernelDim_y * multiplying_factor);
  const unsigned int cycles_read_block = Stride_x * IFMDim_x * multiplying_factor;
  const unsigned int max_cycles = MAX(cycles_write_block,cycles_read_block);
  const unsigned int baseIter = IFMDim_x * ConvKernelDim_y * multiplying_factor// Initial buffer
			                  + OFMDim_y * MAX(cycles_write_block,cycles_read_block);
  unsigned int counter_internal_block = 0;
  unsigned int current_block_write = 0;
  unsigned int next_block_write = 0;
  unsigned int current_line = 0;
  unsigned int read_block = 0;
  unsigned int inp = 0, ofm_y = 0, ofm_x = 0, k_y = 0, k_x = 0, count_simd =0;
#pragma HLS reset variable=inp
  for (unsigned int count_image = 0; count_image < numReps; count_image++) {
    for (unsigned int i = 0; i < baseIter; i++) {
#pragma HLS PIPELINE II=1
      if (inp < IFMDim_x * ConvKernelDim_y *multiplying_factor) {// Initial buffer of ConvKernelDim lines
        ap_uint<SIMD*Input_precision> inElem;
        inElem = in.read();
        inputBuf[current_block_write][current_line] = inElem;
        current_line++;
        inp++;
        if (current_line == Stride_x * IFMDim_x * multiplying_factor ) {
          current_line = 0;
          current_block_write++;
          if (current_block_write == number_blocks) {
            current_block_write=0;
          }
          read_block++;
          counter_internal_block = 0;
        }
      } else {
        if (counter_internal_block < cycles_write_block-1) { // We are writing output, MMV IFMChan per cycle
          unsigned int current_block_read = (current_block_write + 1 + k_y / Stride_y);
          if (current_block_read >= number_blocks) {
            current_block_read-= number_blocks;
		  }
          unsigned int current_line_in_block = ((k_y%Stride_y) * IFMDim_y + ofm_x*Stride_x + k_x)*multiplying_factor + count_simd;
          ap_uint<SIMD*Input_precision> outElem = inputBuf[current_block_read][(current_line_in_block)];
          out.write(outElem);
		  k_x++;
		  if (k_x == ConvKernelDim_x) {
		    k_x = 0;
		    k_y++;
		    if (k_y == ConvKernelDim_y) {
			  k_y = 0;
			  count_simd++;
			  if (count_simd == multiplying_factor) {
			    count_simd=0;
			    ofm_x ++;
			    if (ofm_x == OFMDim_x) {
			      ofm_x = 0;
			      ofm_y++;
			      if (ofm_y == OFMDim_y) {
				    ofm_y = 0;
				    inp = 0;
                  }
                }
              }
            }
          }
        }
        if ((counter_internal_block < cycles_read_block-1) && (read_block<IFMDim_y/Stride_y)) { // In parallel we write in the buffer, in the current block write if we still need to
          ap_uint<SIMD*Input_precision> inElem;
          inElem = in.read();
          inputBuf[current_block_write][current_line] = inElem;
#pragma AP dependence variable=inputBuf intra false
#pragma AP dependence variable=inputBuf inter false
          current_line++;
          if (current_line == Stride_x * IFMDim_x * multiplying_factor) {// We read the whole block, we change the next block in which we want to we
            // We filled up a block, let's not read until
            current_line = 0;
            read_block++;
            current_block_write++;
            if (current_block_write == number_blocks) {
              current_block_write=0;
			}
#pragma AP dependence variable=current_block_write intra false
          }
        }
        counter_internal_block++; // = (counter_internal_block +1) % max_cycles;
        if (counter_internal_block == (max_cycles-1)) {
          counter_internal_block = 0;
        }
      }
    } // End base_iter
	read_block = 0;
  } // End count_image
} // End generator

/**
 * \brief Sliding Window unit that produces output vectors for feeding
 * a Matrix_Vector_Activate_Batch, implementing the im2col algorithm. To be used when kernel is not square and with dilation
 * NOTE: Dilation over the Y axis not yet supported
 *
 * \tparam ConvKernelDim_x    	Dimension of the convolutional kernel - x axis
 * \tparam ConvKernelDim_y    	Dimension of the convolutional kernel - y axis
 * \tparam IFMChannels      	Number of Input Feature Maps
 * \tparam Input_precision  	Number bits per pixel
 * \tparam IFMDim_x          	Width of the Input Feature Map
 * \tparam IFMDim_y           	Height of the Input Feature Map
 * \tparam OFMDim_x           	Width of the Output Feature Map
 * \tparam OFMDim_y           	Height of the Output Feature Map
 * \tparam SIMD             	Number of input columns computed in parallel
 * \tparam Stride_x           	Stride of the convolutional kernel - x axis
 * \tparam Stride_y          	Stride of the convolutional kernel - y axis
 * \tparam Dilation_x          	Dilation the convolutional kernel - x axis
 * \tparam Dilation_y          	Dilation the convolutional kernel - y axis
 * \tparam R          	  		Datatype for the resource used for FPGA implementation of the SWG  - safely deducible from the parameters
 *
 * \param in                	Input stream
 * \param out               	Output stream
 * \param numReps           	Number of time the function has to be repeatedly executed (e.g. number of images)
 * \param r			  			Resource type for the hardware implementation of the memory block
 */
template<unsigned int ConvKernelDim_x,
		 unsigned int ConvKernelDim_y,
		 unsigned int IFMChannels,
		 unsigned int Input_precision,
		 unsigned int IFMDim_x,
		 unsigned int IFMDim_y,
		 unsigned int OFMDim_x,
		 unsigned int OFMDim_y,
		 unsigned int SIMD,
		 unsigned int Stride_x,
		 unsigned int Stride_y,
		 unsigned int Dilation_x,
		 unsigned int Dilation_y,
		 typename R>
void ConvolutionInputGenerator_NonSquare_Dilated(
		stream<ap_uint<SIMD*Input_precision> > & in,
		stream<ap_uint<SIMD*Input_precision> > & out,
		const unsigned int numReps,
		R const &r) {
  CASSERT_DATAFLOW(IFMChannels % SIMD == 0);
  CASSERT_DATAFLOW(Dilation_y == 1); // Dilation on the Y axes not yet supported, available only for API definition

  const unsigned int multiplying_factor = IFMChannels/SIMD;
  const unsigned int number_blocks = (ConvKernelDim_y*Dilation_y)/Stride_y + 1 ;
  ap_uint<SIMD*Input_precision> inputBuf[number_blocks][Stride_x * IFMDim_x * multiplying_factor];

#pragma HLS ARRAY_PARTITION variable=inputBuf complete dim=1
  memory_resource(inputBuf, r);
  const unsigned int cycles_write_block = (OFMDim_x * ConvKernelDim_x * ConvKernelDim_y * multiplying_factor);
  const unsigned int cycles_read_block = Stride_x * IFMDim_x * multiplying_factor;
  const unsigned int max_cycles = MAX(cycles_write_block,cycles_read_block);
  const unsigned int baseIter = IFMDim_x * ConvKernelDim_y * Dilation_y  * multiplying_factor// Initial buffer
			                  + OFMDim_y * MAX(cycles_write_block,cycles_read_block);
  unsigned int counter_internal_block = 0;
  unsigned int current_block_write = 0;
  unsigned int next_block_write = 0;
  unsigned int current_line = 0;
  unsigned int read_block = 0;
  unsigned int inp = 0, ofm_y = 0, ofm_x = 0, k_y = 0, k_x = 0, count_simd =0;
#pragma HLS reset variable=inp
  for (unsigned int count_image = 0; count_image < numReps; count_image++) {
    for (unsigned int i = 0; i < baseIter; i++) {
#pragma HLS PIPELINE II=1
      if (inp < IFMDim_x * ConvKernelDim_y * Dilation_y *multiplying_factor) {// Initial buffer of ConvKernelDim lines
        ap_uint<SIMD*Input_precision> inElem;
        inElem = in.read();
        inputBuf[current_block_write][current_line] = inElem;
        current_line++;
        inp++;
        if (current_line == Stride_x * IFMDim_x * multiplying_factor ) {
          current_line = 0;
          current_block_write++;
          if (current_block_write == number_blocks) {
            current_block_write=0;
          }
          read_block++;
          counter_internal_block = 0;
        }
      } else {
        if (counter_internal_block < cycles_write_block-1) { // We are writing output, IFMChan per cycle
          unsigned int current_block_read = (current_block_write + 1 + (k_y*Dilation_y) / Stride_y);
          if (current_block_read >= number_blocks) {
            current_block_read-= number_blocks;
		  }
          unsigned int current_line_in_block = ((ofm_x*Stride_x + k_x*Dilation_x)*multiplying_factor + count_simd);
          ap_uint<SIMD*Input_precision> outElem;
          outElem = inputBuf[current_block_read][(current_line_in_block)];
          out.write(outElem);
          count_simd++;
          if (count_simd == multiplying_factor) {
            count_simd=0;
            k_x++;
            if (k_x == ConvKernelDim_x) {
              k_x = 0;
              k_y++;
              if (k_y == ConvKernelDim_y) {
                k_y = 0;
                ofm_x ++;
                if (ofm_x == OFMDim_x) {
                  ofm_x = 0;
                  ofm_y++;
                  if (ofm_y == OFMDim_y) {
                    ofm_y = 0;
                    inp = 0;
                  }
                }
              }
            }
          }
        }
        if ((counter_internal_block < cycles_read_block-1) && (read_block<IFMDim_y/Stride_y)) { // In parallel we write in the buffer, in the current block write if we still need to
          ap_uint<SIMD*Input_precision> inElem;
          inElem = in.read();
          inputBuf[current_block_write][current_line] = inElem;
#pragma AP dependence variable=inputBuf intra false
#pragma AP dependence variable=inputBuf inter false
          current_line++;
          if (current_line == Stride_x * IFMDim_x * multiplying_factor) {// We read the whole block, we change the next block in which we want to we
            // We filled up a block, let's not read until
            current_line = 0;
            read_block++;
            current_block_write++;
            if (current_block_write == number_blocks) {
              current_block_write=0;
			}
#pragma AP dependence variable=current_block_write intra false
          }
        }
        counter_internal_block++; // = (counter_internal_block +1) % max_cycles;
        if (counter_internal_block == (max_cycles-1)) {
          counter_internal_block = 0;
        }
      }
    } // End base_iter
	read_block = 0;
  } // End count_image
} // End generator

/**
 * \brief Sliding Window unit that produces output vectors for feeding
 * a Matrix_Vector_Activate_Batch, implementing the im2col algorithm.
 * To be used only for 1D feature maps.
 * Feeds all pixels of the window in parallel for full SIMD unfolding of following layer.
 * NOTE: Currently restricted to: SIMD = IFMChannels
 *
 * \tparam ConvKernelDim    	Dimension of the convolutional kernel
 * \tparam IFMChannels      	Number of Input Feature Maps
 * \tparam Input_precision  	Number bits per pixel
 * \tparam IFMDim           	Dimension of the Input Feature Map
 * \tparam OFMDim           	Dimension of the Output Feature Map
 * \tparam SIMD             	Number of input columns computed in parallel
 * \tparam Stride          	  Stride of the convolutional kernel
 * \tparam R          	  		Datatype for the resource used for FPGA implementation of the SWG  - safely deducible from the parameters
 *
 * \param in                	Input stream
 * \param out               	Output stream
 * \param numReps           	Number of time the function has to be repeatedly executed (e.g. number of images)
 * \param r			  			      Resource type for the hardware implementation of the memory block
*/
template<unsigned int ConvKernelDim,
		 unsigned int IFMChannels,
		 unsigned int Input_precision,
		 unsigned int IFMDim,
		 unsigned int OFMDim,
		 unsigned int SIMD,
		 unsigned int Stride,
		 typename R>
void ConvolutionInputGenerator_1D_parallel(
		stream<ap_uint<SIMD*Input_precision> > & in,
		stream<ap_uint<ConvKernelDim*SIMD*Input_precision> > & out,
		const unsigned int numReps,
		R const &r) {

  CASSERT_DATAFLOW(IFMChannels % SIMD == 0);
  const unsigned int number_blocks = ConvKernelDim + 1 ;
  ap_uint<SIMD*Input_precision> inputBuf[number_blocks];

#pragma HLS ARRAY_PARTITION variable=inputBuf complete
  //memory_resource(inputBuf, r); use reg regardless of setting
  const unsigned int baseIter = IFMDim + 1;
  unsigned int current_block_write = 0;
  unsigned int next_block_write = 0;
  unsigned int read_block = 0;
  unsigned int stride_counter = 0;
  for (unsigned int count_image = 0; count_image < numReps; count_image++) {
    for (unsigned int i = 0; i < baseIter; i++) {
#pragma HLS PIPELINE II=1
      if (read_block < ConvKernelDim) { // Initial buffer of ConvKernelDim lines
        ap_uint<SIMD*Input_precision> inElem;
        inElem = in.read();
        inputBuf[current_block_write] = inElem;

        current_block_write++;
        if (current_block_write == number_blocks) {
          current_block_write=0;
        }
        read_block++;
        
      } else {
        
        if(stride_counter == 0) { // READING from buffer
          ap_uint<ConvKernelDim*SIMD*Input_precision> outElem;

          for(int k_y=0; k_y<ConvKernelDim; k_y++)
          {
  #pragma HLS UNROLL
            unsigned int current_block_read = (current_block_write + 1 + k_y);
            if (current_block_read >= number_blocks) {
              current_block_read-= number_blocks;
            }
            outElem((k_y+1)*SIMD*Input_precision-1, k_y*SIMD*Input_precision) = inputBuf[current_block_read];
          }

          out.write(outElem);
        }
        
        if (read_block<IFMDim) { // WRITING to buffer
          ap_uint<SIMD*Input_precision> inElem;
          inElem = in.read();
          inputBuf[current_block_write] = inElem;
#pragma AP dependence variable=inputBuf intra false
#pragma AP dependence variable=inputBuf inter false
            
          read_block++;
          current_block_write++;
          if (current_block_write == number_blocks) {
            current_block_write=0;
			    }
          stride_counter++;
          if (stride_counter == Stride) {
            stride_counter=0;
          }
#pragma AP dependence variable=current_block_write intra false
        }
      }
    } // End base_iter
	read_block = 0;
  stride_counter = 0;
  } // End count_image
} // End generator


#endif
