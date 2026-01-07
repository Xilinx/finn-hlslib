/******************************************************************************
 *
 *  Authors:  erling on 5/10/21.
 *  			Giulio Gambardella <giuliog@xilinx.com>
 *
 *  \file upsample_top.cpp
 *  
 *  Testbench function for unit testing of the Upsample with Nearest Neighbour 
 *  
 *****************************************************************************/

#include <iostream>
#include <hls_stream.h>
#include "ap_int.h"
#include "data/upsample_config.h"

using namespace std;


void Testbench_upsample(
	hls::stream<ap_uint<PRECISION * FM_CHANNELS>> &in,
	hls::stream<ap_uint<PRECISION * FM_CHANNELS>> &out
);

void Golden_upsample(
	ap_uint<PRECISION> const (&src)[IFMDIM][IFMDIM][FM_CHANNELS],
	ap_uint<PRECISION>       (&dst)[OFMDIM][OFMDIM][FM_CHANNELS]
);

int main(){
  ap_uint<PRECISION> golden_in[IFMDIM][IFMDIM][FM_CHANNELS];
  ap_uint<PRECISION> golden_out[OFMDIM][OFMDIM][FM_CHANNELS];

  hls::stream<ap_uint<PRECISION*FM_CHANNELS>> test_in("test_input");
  hls::stream<ap_uint<PRECISION*FM_CHANNELS>> test_out("test_ouput");

  for (int i = 0; i<IFMDIM; i++) {
    for (int j = 0; j<IFMDIM; j++) {
      ap_uint<PRECISION*FM_CHANNELS> input_channel = 0;
      for (int k = 0; k<FM_CHANNELS; k++) {
        ap_uint<PRECISION> input = i*IFMDIM + j;
        input_channel = input_channel << PRECISION;
        input_channel(PRECISION-1,0) = input;
        golden_in[i][j][k] = input;
      }
      test_in.write(input_channel);
    }
  }

  Golden_upsample(golden_in, golden_out);
  Testbench_upsample(test_in, test_out);

  ap_uint<PRECISION> out_channel;
  int err_counter = 0;
  for (int i = 0; i<OFMDIM; i++) {
    for (int j = 0; j<OFMDIM; j++) {
      ap_uint<PRECISION * FM_CHANNELS> out_elem = test_out.read();
      for (int k = 0; k<FM_CHANNELS; k++) {
        ap_uint<PRECISION> expect = golden_out[i][j][k];
        out_channel(PRECISION-1,0) = out_elem((k+1)*PRECISION-1, k*PRECISION);
        if (expect != out_channel) {
          cout << "ERROR: Expected["<<i<<"]["<<j<<"]["<<k<<"]=" <<expect <<" actual " <<out_channel <<endl;
          err_counter++;
        }
      }
    }
  }

  return err_counter;
}

void Golden_upsample(
	ap_uint<PRECISION> const (&src)[IFMDIM][IFMDIM][FM_CHANNELS],
	ap_uint<PRECISION>       (&dst)[OFMDIM][OFMDIM][FM_CHANNELS]
) {
	for(unsigned  i = 0; i < OFMDIM; i++) {
		unsigned const  ii = unsigned((0.5f + i) * IFMDIM/OFMDIM);
		for(unsigned  j = 0; j < OFMDIM; j++) {
			unsigned const  jj = unsigned((0.5f + j) * IFMDIM/OFMDIM);
			for (unsigned  k = 0; k < FM_CHANNELS; k++) {
				dst[i][j][k] = src[ii][jj][k];
			}
		}
	}
}
