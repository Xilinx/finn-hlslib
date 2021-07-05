//
// Created by erling on 5/3/21.
//

#ifndef UPSAMPLE_HPP
#define UPSAMPLE_HPP


// UpsampleNearest
//  Simple Upsamling layer implementing the most basic Nearest Neighbour upsampling assumes square IFM and OFM.
//  This can be solved without using a SWU (which is needed for more complicated algorithms e.g. bilinear)
//  Example: NumChannels=1, IFMDim=2, OFMDim=5 => this gives scale_factor=2 padding=1. THis is consistent with PyTorch Upsample
//            1 1 1 2 2
//  1 2  ->   1 1 1 2 2
//  3 4       1 1 1 2 2
//            3 3 3 4 4
//            3 3 3 4 4
// This can be achieved by reading and buffering row 1 ([1 2]) and creating the output pattern:
//  [1 1 1 2 2
//   1 1 1 2 2
//   1 1 1 2 2 ]
// Notice that since we have "asymmetric" upsamling we pad the an extra "1" and also and extra "[1 1 1 2 2]" row.


// This functions take a stream as input and produces one row as the output stream
//  The idea is that you can either pass the top-level input stream or the row_buf fifo
// to this function


/**
 * @brief Utility function for UpsampleNearest. Based on IFMDim and OFMDim it duplicates an input row to an output row. It also writes the input back to out_row_buf for buffering
 * 
 * @tparam OFMDim height/width of output feature map 
 * @tparam IFMDim height/width of input feature map
 * @tparam NumChannels depth of input feature map
 * @tparam Input_precision bitwidth of the pixel representation
 * @param in Input stream NHWC
 * @param out Output stream NHWC
 * @param out_row_buf Directly connected to the input, used so that the rows can be buffered and re-fed into this function according to the scale factor
 * @param write_row_buf Boolean input guarding the out_row_buf.
 */
template<unsigned int OFMDim, unsigned int IFMDim, unsigned int NumChannels, unsigned int Input_precision>
void UpsampleNearestGenerateOutputRow(
        stream<ap_uint<Input_precision * NumChannels>> & in,
        stream<ap_uint<Input_precision * NumChannels>> & out,
        stream<ap_uint<Input_precision * NumChannels>> & out_row_buf,
        bool write_row_buf
        ) {
  const unsigned int scale_factor = OFMDim/IFMDim;
  const unsigned int padding = OFMDim % IFMDim;

  for (unsigned int i = 0; i<IFMDim; i++) {
    ap_uint<Input_precision * NumChannels> in_elem;

    in_elem = in.read();

    // A bit hacky. But if we set the write_row_buf flag we will pass the input to the out_row_buf
    //  output. So that it can also be stored in the row_buf
    if (write_row_buf) {
      out_row_buf.write(in_elem);
    }

    // Add padding
    if (i == 0) {
      for (unsigned int pad_idx = 0; pad_idx<padding; pad_idx++) {
        out.write(in_elem);
      }
    }

    // Duplicate input to the output stream
    for (unsigned int scale_idx = 0; scale_idx < scale_factor; scale_idx++) {
    	out.write(in_elem);
    }
  }
}



/**
 * @brief Upsampling with the Nearest Neighbour algorithm. Only accept square inputs (trivial to extend). It assumes a NHWC data layout and reads in row-by-row of C pixels. 
 *  each pixel is duplicated according to the scale factor = OFMDim/IFMDim. Each row is buffered internally and "replayed" to duplicate in both dimensions.
 *  Does not have any configuration parameters which the FINN compiler can tune. 
 * 
 * @tparam OFMDim height/width of output feature map 
 * @tparam IFMDim height/width of input feature map
 * @tparam NumChannels depth of input feature map
 * @tparam Input_precision bitwidth of the pixel representation
 * @param in Input stream NHWC
 * @param out Output stream NHWC
 */
template<unsigned int OFMDim, unsigned int IFMDim, unsigned int NumChannels, unsigned int Input_precision>
void UpsampleNearest(
        stream<ap_uint<Input_precision * NumChannels>> & in,
        stream<ap_uint<Input_precision * NumChannels>> & out
) {
  CASSERT_DATAFLOW(OFMDim > IFMDim);

  const unsigned int scale_factor = OFMDim/IFMDim;
  const unsigned int padding = OFMDim % IFMDim;
  const unsigned int base_iter = IFMDim;

  // FIFO for temporary storing each row for duplication
  stream<ap_uint<Input_precision * NumChannels>, IFMDim> row_buf;

  // Loop over the rows in the IFM
  for (unsigned int row_idx = 0; row_idx < base_iter; row_idx++) {

    if (row_idx == 0) {
      // Add possible padding
      for (int pad_idx = 0; pad_idx < padding; pad_idx++) {
        // If we are at the first iteration through the padding: Use input stream
        if (pad_idx == 0) {
          UpsampleNearestGenerateOutputRow<OFMDim, IFMDim, NumChannels, Input_precision>(in, out, row_buf, true);
        } else {
          UpsampleNearestGenerateOutputRow<OFMDim, IFMDim, NumChannels, Input_precision>(row_buf, out, row_buf, true);
        }
      }
    }
    // Then do the rows. They are done "scale" number of times

    for (unsigned int scale_idx = 0; scale_idx < scale_factor; scale_idx++) {
    	bool write_row_buf = scale_idx < (scale_idx - 1);
      if (scale_idx == 0 && !(row_idx == 0 && padding > 0)) {
        // First iteration of a row is fetched from top-level input. Except if its the very first and we have padding
        UpsampleNearestGenerateOutputRow<OFMDim, IFMDim, NumChannels, Input_precision>(in, out, row_buf, write_row_buf);
      } else {
        UpsampleNearestGenerateOutputRow<OFMDim, IFMDim, NumChannels, Input_precision>(row_buf, out, row_buf, write_row_buf);
      }
    }
  }
}

#endif //FINN_HLSLIB_UPSAMPLE_HPP
