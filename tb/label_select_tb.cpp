#include <iostream>
#include <hls_stream.h>
#define AP_INT_MAX_W 8191
#include "ap_int.h"

#include "label_select_top.hpp"

#define MAX_IMAGES 2

int main() {
	int  sig_in_vec[10] = {-12043, 30370, -23559, -30747, -11173, 30686, 19723,
		-8171, -16169, 809};
	int  sig_in_golden[10] = {5, 1, 6, 9, 7, 4, 0, 8, 2, 3};

	hls::stream<ls_in_t>  input_st("input_st");
	hls::stream<Out_T>    output_st("output_st");

	// Pack scalar inputs into PE1-wide tensors
	constexpr unsigned  FOLDS = FM_Channels1 / PE1;
	for(int  batch = 0; batch < MAX_IMAGES; batch++) {
		for(unsigned  f = 0; f < FOLDS; f++) {
			ap_uint<PE1 * INPUT_PRECISION>  word = 0;
			for(unsigned  pe = 0; pe < PE1; pe++) {
				unsigned  idx = f * PE1 + pe;
				word((pe+1)*INPUT_PRECISION-1, pe*INPUT_PRECISION) = ap_uint<INPUT_PRECISION>(In_T(sig_in_vec[idx]));
			}
			input_st.write(ls_in_t(word));
		}
	}

	// Call function-pipelined kernel (two-phase: FOLDS capture + NumTop emit per image)
	constexpr unsigned  ITERS_PER_IMAGE = FOLDS + NumTop;
	for(unsigned  i = 0; i < 2 * MAX_IMAGES * ITERS_PER_IMAGE; i++)
		Testbench_label_select(input_st, output_st);

	// Verify
	int  err_cnt = 0;
	for(int  batch = 0; batch < MAX_IMAGES; batch++) {
		for(int  i = 0; i < NumTop; i++) {
			Out_T  outElem = output_st.read();
			int  expected = sig_in_golden[i];
			if(outElem != expected) {
				std::cout << "ERROR: batch " << batch << " top " << i+1
					<< " expected " << expected << " got " << outElem << std::endl;
				err_cnt++;
			}
		}
	}

	if(err_cnt == 0)  std::cout << "Test passed." << std::endl;
	return  err_cnt ? 1 : 0;
}
