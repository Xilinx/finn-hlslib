/******************************************************************************
 *
 *  Authors: Giulio Gambardella <giuliog@xilinx.com>
 *
 *  \file swg_tb.cpp
 *
 *  Testbench for the sliding window generator HLS block
 *
 *****************************************************************************/
#include <hls_stream.h>
#include "ap_int.h"
#include <iostream>
#include <string>
#include "math.h"
using namespace hls;
using namespace std;
#include "input_gen.h"

#define MAX_IMAGES 2

int main()
{
    static ap_uint<INPUT_PRECISION> INPUT_IMAGES[MAX_IMAGES][IFMDimH * IFMDimW][IFM_Channels];
    stream<ap_uint<IFM_Channels * INPUT_PRECISION>> input_stream("input_stream");
    stream<ap_uint<IFM_Channels * INPUT_PRECISION>> output_stream("output_stream");
    unsigned int counter = 0;
    for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++)
    {
        for (unsigned int oy = 0; oy < IFMDimH; oy++)
        {
            for (unsigned int ox = 0; ox < IFMDimW; ox++)
            {
                ap_uint<INPUT_PRECISION *IFM_Channels> input_channel = 0;
                for (unsigned int channel = 0; channel < IFM_Channels; channel++)
                {
                    ap_uint<INPUT_PRECISION> input = (ap_uint<INPUT_PRECISION>)(counter);
                    INPUT_IMAGES[n_image][oy * IFMDimW + ox][channel] = input;
                    input_channel = input_channel >> INPUT_PRECISION;
                    input_channel(IFM_Channels * INPUT_PRECISION - 1, (IFM_Channels - 1) * INPUT_PRECISION) = input;

                    counter++;
                }
                input_stream.write(input_channel);
            }
        }
    }
    Testbench(input_stream, output_stream, MAX_IMAGES);
    for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++)
    {
        for (unsigned int oy = 0; oy < OFMDimH; oy++)
        {
            for (unsigned int ox = 0; ox < OFMDimW; ox += MMV)
            {
                for (unsigned int ky = 0; ky < KERNEL_DIM; ky++)
                {
                    for (unsigned int kx = 0; kx < KERNEL_DIM; kx++)
                    {
                        unsigned int input_base = (oy * STRIDE) * IFMDimW + (ox * STRIDE);
                        unsigned int input_ind = input_base + ky * IFMDimW + kx;
                        ap_uint<IFM_Channels *INPUT_PRECISION> outElem = output_stream.read();
                        for (unsigned int channel = 0; channel < IFM_Channels; channel++)
                        {
                            ap_uint<INPUT_PRECISION> out_chan = 0;
                            out_chan = outElem(INPUT_PRECISION - 1, 0);
                            if (((INPUT_IMAGES[n_image][input_ind][channel])) != out_chan)
                            {
                                std::cout << "ERROR: "
                                          << " Expected " << INPUT_IMAGES[n_image][input_ind][channel] << " actual " << out_chan << std::endl;
                                std::cout << "oy= " << oy << " ox= " << ox << " ky= " << ky << " kx= " << kx << std::endl;
                                return 1;
                            }
                            outElem = outElem >> INPUT_PRECISION;
                        }
                    }
                }
            }
        }
        std::cout << "Image # " << n_image << std::endl;
    }
    return 0;
}