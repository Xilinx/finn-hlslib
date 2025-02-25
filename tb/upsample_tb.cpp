/******************************************************************************
 *
 *  Authors:  
 *  			Michal Danilowicz <danilowi@agh.edu.pl>
 *
 *  \file upsample_top.cpp
 *  
 *  Testbench function for unit testing of the Upsample with Nearest Neighbour 
 *  
 *****************************************************************************/

#include <iostream>
#include <hls_stream.h>
#include <ap_int.h>
#include "data/upsample_config.h"


// using namespace hls;
// using namespace std;


void Testbench_upsample(hls::stream<VEC_TYPE> &in, hls::stream<VEC_TYPE> &out);
void Golden_upsample(EL_TYPE in[HI][WI][FM_CHANNELS], EL_TYPE out[HO][WO][FM_CHANNELS]);


int main(){
  static EL_TYPE golden_in[HI][WI][FM_CHANNELS];
  static EL_TYPE golden_out[HO][WO][FM_CHANNELS];

  hls::stream<VEC_TYPE> test_in("test_input");
  hls::stream<VEC_TYPE> test_out("test_ouput");

  for (int i = 0; i < HI; i++) {
    for (int j = 0; j < WI; j++) {

      VEC_TYPE simd_vector;
      // iterate over folds
      for (int fold = 0; fold < CF; fold++) {
        simd_vector = (EL_TYPE)0;

        // iterate over channels in fold
        for (int c = 0; c < SIMD; c++){
          EL_TYPE input = i*WI + j;
          golden_in[i][j][fold*SIMD + c] = input;
          simd_vector[c] = input;
        }
        
        test_in.write(simd_vector);
      }
    }
  }


  Golden_upsample(golden_in, golden_out);
  Testbench_upsample(test_in, test_out);


  int err_counter = 0;
  for (int i = 0; i < HO; i++) {
    for (int j = 0; j < WO; j++) {

      for (int fold = 0; fold < CF; fold++){
        VEC_TYPE out_vec = test_out.read();

        for (int c = 0; c < SIMD; c++){
          EL_TYPE expected = golden_out[i][j][fold*SIMD + c];
          EL_TYPE out_el = out_vec[c];
          if (expected != out_el){
            std::cerr << "ERROR: Expected["<<i<<"]["<<j<<"]["<<(fold*SIMD + c)<<"]=" << expected << " actual " << out_el << std::endl;
            err_counter++;
          }
        }
      }
    }
  }

  return err_counter;
}


void Golden_upsample(EL_TYPE in[HI][WI][FM_CHANNELS], EL_TYPE out[HO][WO][FM_CHANNELS]) {

	for(unsigned  i = 0; i < HO; i++) {
		unsigned const  ii = unsigned((0.5f + i) * HI/HO);
		for(unsigned  j = 0; j < WO; j++) {
			unsigned const  jj = unsigned((0.5f + j) * WI/WO);
			for (unsigned  k = 0; k < FM_CHANNELS; k++) {
				out[i][j][k] = in[ii][jj][k];
			}
		}
	}
}
