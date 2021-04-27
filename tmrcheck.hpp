/******************************************************************************
 *  Copyright (c) 2021, Xilinx, Inc.
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
 *  Authors: Timoteo Garcia Bertoa <timoteog@xilinx.com>
 *
 *  \file tmrcheck.hpp
 *
 *  Library of templated HLS functions for BNN deployment.
 *  This file performs error checks to triplicated channels of an OFM
 *
 *****************************************************************************/

#ifndef TMR_HPP
#define TMR_HPP

#include "hls_stream.h"

/**
 * \brief Smart TMR block
 *
 * The function receives an OFM from the MVTU, with triplicated channels. It outputs a single channel for each triplication,
 * being this channel one which contains valid data. A flag is also available to watch the character of the error detected,
 * either all channels in a triplication are different, or just one.
 *
 * \tparam InW              Input data width, activation precision
 * \tparam OFMChannels      Number of Output Feature Map channels, including triplications
 * \tparam NUM_RED          Number of redundancies (or triplicated channels)
 * \tparam REDF             Redundancy factor (3 to triplicate)
 * \tparam OFMDim           Width and Height of the Output Feature Map (assumed square)
 * \tparam MAX_CH_WIDTH     Value to determine the precision of channel indexes
 *
 * \param in                Input stream
 * \param out               Output stream
 * \param errortype         Flag to inform redundancy check results. 0 if no faults, LSB set if one PE is faulty, MSB set if all differ
 * \param channel_mask      Value with binary channel masks (1 if channel is triplicated, 0 otherwise)
 * \param red_ch_index      Array of redundant triplets' indexes. Each position stores the first triplicated channel index of a triplet
 */

template<unsigned int InW,
         unsigned int OFMChannels,
         unsigned int NUM_RED,
         unsigned int REDF,
         unsigned int OFMDim,
         unsigned int MAX_CH_WIDTH>
void TMRCheck(hls::stream<ap_uint<InW*OFMChannels>> &in,
              hls::stream<ap_uint<InW*(OFMChannels-NUM_RED*(REDF-1))>> &out,
              ap_uint<2> &errortype,
              ap_uint<OFMChannels> channel_mask,
              ap_uint<MAX_CH_WIDTH> red_ch_index[NUM_RED]) {

    #pragma HLS ARRAY_PARTITION variable=red_ch_index complete dim=0
    ap_uint<InW*OFMChannels> input;

    // Number of channels without triplications
    constexpr unsigned int OFMChannelsTMR = (OFMChannels-NUM_RED*(REDF-1));
    errortype = 0;

    // CheckLoop: iterates over all OFM positions
    for(unsigned int pos = 0; pos < (OFMDim * OFMDim); pos++){
    #pragma HLS pipeline II=1

        // Read input stream
        input = in.read();

        ap_uint<InW*NUM_RED> tmr_out = {0};
        ap_uint<InW*OFMChannelsTMR> out_aux = {0};
        ap_uint<2> numerrors[NUM_RED];
        #pragma HLS ARRAY_PARTITION variable=numerrors complete dim=0

        // Check triplicated channel indexes and store the corresponding data to perform TMR check
        for(unsigned int i = 0; i < NUM_RED; i++){
        #pragma HLS UNROLL

            numerrors[i] = 0;
            // TMR CHECK: start
            // Store index of triplicated channel
            unsigned int idx = red_ch_index[i];
            // CompareLoop: performs comparisons between PE0, PE1, PE2
            for(unsigned int y = 0; y < REDF; y++){
                for(unsigned int x = y+1; x < REDF; x++){
                    if( (input((idx+y+1)*InW-1, (idx+y)*InW)) == (input((idx+x+1)*InW-1, (idx+x)*InW)) ){
                        tmr_out((i+1)*InW-1, i*InW) = input((idx+x+1)*InW-1, (idx+x)*InW);
                    } else {
                        numerrors[i]++;
                        if(numerrors[i] == REDF){
                            errortype |= (ap_uint<2>)0b10;
                        } else {
                            errortype |= (ap_uint<2>)0b1;
                        }
                        tmr_out((i+1)*InW-1, i*InW) = input((idx+1)*InW-1, idx*InW);
                    }
                }
            } // end CompareLoop
        } // end Check triplicated channel indexes

        ap_uint<OFMChannels> unitL = 1;
        ap_uint<1> compute[OFMChannels];
        #pragma HLS ARRAY_PARTITION variable=compute complete dim=0
        // ChannelLoop: iterates over all OFM channels (including triplications), and outputs either: TMR check output/input/nothing
        for(unsigned int k = 0; k < OFMChannels; k++){
        #pragma HLS UNROLL
            compute[k] = 0;
            // Check if current channel is any of the FIRST triplicated
            for(unsigned int i = 0; i < NUM_RED; i++){
                // Store index of triplicated channel
                unsigned int idx = red_ch_index[i];
                if(k == idx){
                    compute[k] = 1;
                }
            }
            
            // If it is one of the first triplicated, forward the TMR check output, which contains valid data
            if(compute[k]){
                out_aux = out_aux >> InW;
                out_aux(OFMChannelsTMR*InW-1, (OFMChannelsTMR-1)*InW) = tmr_out(InW-1, 0);
                tmr_out = tmr_out >> InW;
            // If it is not a first triplicated channel, check if it is a triplicated or not using mask
            } else if((channel_mask & (unitL << k)) != 0){
                ; // Nothing to do, skip triplicated channel
            // If it is a not triplicated channel, forward the input data
            } else {
                out_aux = out_aux >> InW;
                out_aux(OFMChannelsTMR*InW-1, (OFMChannelsTMR-1)*InW) = input((k+1)*InW-1, k*InW);
            }

        } // end ChannelLoop

        out.write(out_aux);
    } // end CheckLoop
} // end TMRCheck


/**
 * \brief Smart TMR block (batch)
 *
 * The function receives an OFM from the MVTU, with triplicated channels. It outputs a single channel for each triplication,
 * being this channel one which contains valid data. A flag is also available to watch the character of the error detected,
 * either all channels in a triplication are different, or just one.
 *
 * \tparam InW              Input data width, activation precision
 * \tparam OFMChannels      Number of Output Feature Map channels, including triplications
 * \tparam NUM_RED          Number of redundancies (or triplicated channels)
 * \tparam REDF             Redundancy factor (3 to triplicate)
 * \tparam OFMDim           Width and Height of the Output Feature Map (assumed square)
 * \tparam MAX_CH_WIDTH     Value to determine the precision of channel indexes
 *
 * \param in                Input stream
 * \param out               Output stream
 * \param errortype         Flag to inform redundancy check results. 0 if no faults, LSB set if one PE is faulty, MSB set if all differ
 * \param channel_mask      Value with binary channel masks (1 if channel is triplicated, 0 otherwise)
 * \param red_ch_index      Array of redundant triplets' indexes. Each position stores the first triplicated channel index of a triplet
 * \param numReps           Number of time the function has to be repeatedly executed (e.g. number of images)
 */

template<unsigned int InW,
         unsigned int OFMChannels,
         unsigned int NUM_RED,
         unsigned int REDF,
         unsigned int OFMDim,
         unsigned int MAX_CH_WIDTH>
void TMRCheck_Batch(hls::stream<ap_uint<InW*OFMChannels>> &in,
                    hls::stream<ap_uint<InW*(OFMChannels-NUM_RED*(REDF-1))>> &out,
                    ap_uint<2> &errortype,
                    ap_uint<OFMChannels> channel_mask,
                    ap_uint<MAX_CH_WIDTH> red_ch_index[NUM_RED],
                    unsigned int numReps) {

    for (unsigned int rep = 0; rep < numReps; rep++) {
        TMRCheck<InW, OFMChannels, NUM_RED, REDF, OFMDim, MAX_CH_WIDTH>(in, out, errortype, channel_mask, red_ch_index);
    }
}

#endif
