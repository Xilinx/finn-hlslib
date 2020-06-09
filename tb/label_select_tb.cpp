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
 *  Authors: Tobias Alonso <tobiasa@xilinx.com>
 *
 *  \file label_select_tb.cpp
 *
 *  Testbench for the LabelSelect_Batch layer 
 *
 *****************************************************************************/

#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include <hls_stream.h>
#include <cstdlib>
#define AP_INT_MAX_W 16384
#include "ap_int.h"
#include "weights.hpp"
#include "bnn-library.h"

#include "label_select_top.h"

using namespace hls;
using namespace std;

#define MAX_IMAGES 2

int main(int argc, char const *argv[])
{   
    
    int sig_in_vec[10] = {-12043, 30370, -23559, -30747, -11173, 30686, 19723, 
                            -8171, -16169, 809};
    int sig_in_golden[10] = {5, 1, 6, 9, 7, 4, 0, 8, 2, 3};

    uint unsig_in_vec[10] = {12043, 30370, 23559, 30747, 11173, 30686, 19723, 
                            8171, 16169, 809};
    uint unsig_in_golden[10] = {3, 5, 1, 2, 6, 8, 0, 4, 7, 9};

    hls::stream<ap_uint<INPUT_PRECISION>> input_st;
    hls::stream<Out_T> output_st;
    for (int batch = 0; batch < MAX_IMAGES; ++batch){
        for (int i = 0; i < FM_Channels1; ++i)
        {   
            #if SIGNED_INPUT
                input_st.write(sig_in_vec[i]);
            #else
                input_st.write(unsig_in_vec[i]);
            #endif
        }
    }
    

    Testbench_label_select(input_st, output_st, MAX_IMAGES);

    int err_cnt = 0;

    for (int batch = 0; batch < MAX_IMAGES; ++batch){
        for (int i = 0; i < NumTop; ++i)
        {
            Out_T outElem = output_st.read();

            #if SIGNED_INPUT
                int expected = sig_in_golden[i];
            #else
                uint expected = unsig_in_golden[i];
            #endif

            if (outElem != expected){
                cout<<"ERROR: top "<<i+1<<" was expected to be "<< expected <<
                    " but got "<<outElem <<endl;
                err_cnt++;
            }

        }
    }

    return err_cnt;
}
