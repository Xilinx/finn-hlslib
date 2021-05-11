//
// Created by erling on 5/3/21.
//

#ifndef UPSAMPLE_HPP
#define UPSAMPLE_HPP


// UpsampleNearest
//  Simple Upsamling layer implementing the most basic Nearest Neighbour upsampling assumes square IFM and OFM.
//  This can be solved without using a SWU (which is needed for more complicated algorithms e.g. bilinear)
//  Example: SIMD=1, IFMDim=2, OFMDim=5 => this gives scale_factor=2 padding=1. THis is consistent with PyTorch Upsample
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
template<unsigned int OFMDim, unsigned int IFMDim, unsigned int SIMD, unsigned int Input_precision>
void UpsampleNearestGenerateOutputRow(
        stream<ap_uint<Input_precision * SIMD>> & in,
        stream<ap_uint<Input_precision * SIMD>> & out,
        stream<ap_uint<Input_precision * SIMD>> & out_row_buf,
        bool write_row_buf
        ) {
  const unsigned int scale_factor = OFMDim/IFMDim;
  const unsigned int padding = OFMDim % IFMDim;

  for (unsigned int i = 0; i<IFMDim; i++) {
    ap_uint<Input_precision * SIMD> in_elem;

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



// Top level for the UpsampleNearest

template<unsigned int OFMDim, unsigned int IFMDim, unsigned int SIMD, unsigned int Input_precision>
void UpsampleNearest(
        stream<ap_uint<Input_precision * SIMD>> & in,
        stream<ap_uint<Input_precision * SIMD>> & out
) {
  CASSERT_DATAFLOW(OFMDim > IFMDim);

  const unsigned int scale_factor = OFMDim/IFMDim;
  const unsigned int padding = OFMDim % IFMDim;
  const unsigned int base_iter = IFMDim;

  // FIFO for temporary storing each row for duplication
  stream<ap_uint<Input_precision * SIMD>> row_buf;

  // Loop over the rows in the IFM
  for (unsigned int row_idx = 0; row_idx < base_iter; row_idx++) {

    if (row_idx == 0) {
      // Add possible padding
      for (int pad_idx = 0; pad_idx < padding; pad_idx++) {
        // If we are at the first iteration through the padding: Use input stream
        if (pad_idx == 0) {
          UpsampleNearestGenerateOutputRow<OFMDim, IFMDim, SIMD, Input_precision>(in, out, row_buf, true);
        } else {
          UpsampleNearestGenerateOutputRow<OFMDim, IFMDim, SIMD, Input_precision>(row_buf, out, row_buf, true);
        }
      }
    }
    // Then do the rows. They are done "scale" number of times

    for (unsigned int scale_idx = 0; scale_idx < scale_factor; scale_idx++) {
    	bool write_row_buf = scale_idx < (scale_idx - 1);
      if (scale_idx == 0 && !(row_idx == 0 && padding > 0)) {
        // First iteration of a row is fetched from top-level input. Except if its the very first and we have padding
        UpsampleNearestGenerateOutputRow<OFMDim, IFMDim, SIMD, Input_precision>(in, out, row_buf, write_row_buf);
      } else {
        UpsampleNearestGenerateOutputRow<OFMDim, IFMDim, SIMD, Input_precision>(row_buf, out, row_buf, write_row_buf);
      }
    }
  }
}

#endif //FINN_HLSLIB_UPSAMPLE_HPP
