#   Copyright (c) 2021, Xilinx, Inc.
#   All rights reserved.
# 
#   Redistribution and use in source and binary forms, with or without 
#   modification, are permitted provided that the following conditions are met:
#
#   1.  Redistributions of source code must retain the above copyright notice, 
#       this list of conditions and the following disclaimer.
#
#   2.  Redistributions in binary form must reproduce the above copyright 
#       notice, this list of conditions and the following disclaimer in the 
#       documentation and/or other materials provided with the distribution.
#
#   3.  Neither the name of the copyright holder nor the names of its 
#       contributors may be used to endorse or promote products derived from 
#       this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
#   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
#   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
#   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
#   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
#   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#   OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
#   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
#   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
#   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#  

################################################################################
#
# Authors: Timoteo Garcia Bertoa <timoteog@xilinx.com>     
#
# \file gen_weights_stmr.py
#
# This file checks PE/OFM channels mapping for TMR implementation, and 
# generates weights with/without error injection for triplicated channels as
# part of the OFM, generating config.h and memdata.h files to use in the TMR
# testbench.
#
################################################################################

import numpy as np
import os
import sys
import random 
import subprocess
import tmrmapping as TMR

#random.seed(1)
#np.random.seed(1)

# Define parameters
kernel_dim = 2 
stride = 1
input_precision = 8
ifm_channels = 2
ofm_channels = 8
ifm_dimension = 3
ofm_dimension = 2
activation_precision = 16
expand = 1
simd = 2
pe = 6
w_precision = 8
mmv = 1
redf = 3
num_red = 2
max_ch_width = 6

# Define how many errors to generate in the triplications (if injection is used)
num_trip_inj = 2 # Set this value to any number between 1 and number of triplications (num_red)
num_errors_pe = 2 # Set this value to any number between 1 and redundancy factor (redf)

# Check validity of the PE selection
TMR.isvalid(pe, ofm_channels, redf, num_red)

# Initialise arrays
ofm_ch_red = ofm_channels + (num_red * (redf - 1))
tile = ifm_channels *kernel_dim*kernel_dim * ofm_channels // (simd * ofm_channels) #assuming pe=ofm_channels to generate weights
tile_tri = tile * int(ofm_ch_red/pe) #tile when OFM contains triplications
w_t_values = np.zeros((ofm_ch_red, tile)) # weights including triplications
w_values = np.zeros((ofm_channels, tile)) # original weights
width = simd * w_precision

# Generate the set of weights
for p in range(ofm_channels):
    for t in range(tile): 
        w_values[p][t] = random.randint(0, 1 << width-1)  
        
# Channels to triplicate
ch_list = [1, 3]

# Retrieve triplicated weights, channel_mask and red_ch_index
w_t_values, channel_mask, red_ch_index = TMR.map(w_values, ofm_channels, pe, redf, num_red, ch_list)

# Transform channel mask to binary
tobin = '{0:0' + str(ofm_ch_red) + 'b}'
channel_mask = tobin.format(channel_mask)
   
# Check if user gave argument
if((len(sys.argv) == 1) or (len(sys.argv) > 2)):
    
    print("Error: Please, provide a valid, single argument, it should be either 'inj', 'no_inj' or 'tmrcheck'.")
    exit(1) 
   
# User wants to inject errors
elif(str(sys.argv[1]) == 'inj'):
    memdataname = "memdata_inj.h"
    configname = "config_inj.h"
    injecting = 'true'
    print("Injecting errors among the triplicated channels...")
    
    # Separate weights in case there is folding, to inject errors to individual blocks:
    w_t_values = np.concatenate(np.hsplit(w_t_values, ofm_ch_red/pe), axis=0)
    # With the current values, a single error is being injected to 2 triplets (there is chance to be the same triplet twice)
    for j in range (num_trip_inj):
        which_triplet = random.randint(0, num_red-1)
        temp_array = [red_ch_index[which_triplet][0], red_ch_index[which_triplet][0]+1, red_ch_index[which_triplet][0]+2]
        for i in range (num_errors_pe):
            which_triplication = int(np.random.choice(temp_array)) 
            w_t_values[which_triplication][random.randint(0, tile-1)] = random.randint(0, 1<<width-1)               
    # Put weights back to the previous position 
    w_t_values = np.concatenate(np.array_split(w_t_values, ofm_ch_red/pe), axis=1)      
    skip = 0
    
# User doesn't want to inject errors    
elif(str(sys.argv[1]) == 'no_inj'):     
    memdataname = "memdata_noinj.h"
    configname = "config_noinj.h"
    injecting = 'false'
    print("Error injection has been skipped.")    
    skip = 0
    
elif(str(sys.argv[1]) == 'tmrcheck'):     
    memdataname = "memdata_tmrc.h"
    configname = "config_tmrc.h"
    skip = 1
else:   
    print("Error: Please, provide a valid argument, it should be either 'inj', 'no_inj' or 'tmrcheck'.")
    exit(1) 
   
# Write config.h output file
outFileConfig = open(configname , "wt")
outFileConfig.write("#define KERNEL_DIM %d \n" % kernel_dim)
outFileConfig.write("#define SIMD1 %d \n" % simd)
outFileConfig.write("#define PE1 %d \n" % pe)
outFileConfig.write("#define MMV1 %d \n" % mmv)
outFileConfig.write("#define WIDTH %d \n" % w_precision)
outFileConfig.write("#define IFM_Channels1 %d \n" % ifm_channels)
outFileConfig.write("#define OFM_Channels1 %d // OFM_Channels %d + redundancies \n" % (ofm_ch_red, ofm_channels))
outFileConfig.write("#define IFMDim1 %d \n" % ifm_dimension)
outFileConfig.write("#define OFMDim1 %d \n" % ofm_dimension)
outFileConfig.write("#define STRIDE %d \n" % stride)
outFileConfig.write("#define INPUT_PRECISION %d \n" % input_precision)
outFileConfig.write("#define TILE1 %d \n" % tile_tri)
outFileConfig.write("#define ACTIVATION_PRECISION %d \n" % activation_precision)
outFileConfig.write("#define REDF %d \n" % redf)
outFileConfig.write("#define NUM_RED %d \n" % num_red)
outFileConfig.write("#define MAX_CH_WIDTH %d // 2^6 = 64 channel indexes \n" % max_ch_width)
if skip == 0:
    outFileConfig.write("#define INJ %s // Have errors been injected? \n" % injecting)
outFileConfig.close()

# Write memdata.h output file
outFileWeights = open(memdataname , "wt")
outFileWeights.write("#ifndef PARAMS_HPP\n")
outFileWeights.write("#define PARAMS_HPP\n")
outFileWeights.write("namespace PARAM{ \n")

if skip == 0:
    if (w_precision == 1):
        outFileWeights.write("static BinaryWeights<%d,%d,%d> weights= {\n{\n" %(simd,pe,tile * int(ofm_ch_red/pe)))
    else:
        outFileWeights.write("static FixedPointWeights<%d,ap_int<%d>,%d,%d> weights= {\n{\n" %(simd,w_precision,pe,tile_tri))           
                
    for p in range(pe):
        outFileWeights.write("{ \n")
        for t in range(tile * int(ofm_ch_red/pe)):
            outFileWeights.write("%s" % hex(int(w_t_values[p][t])))
            if t!=tile * int(ofm_ch_red/pe)-1:
                outFileWeights.write(",\n")
        outFileWeights.write("} \n")
        if p!=pe-1:
            outFileWeights.write(",")
    outFileWeights.write("}\n};\n\n")
    
outFileWeights.write("static const ap_uint<%d> channel_mask = 0b%s;\n\n" %(ofm_ch_red, channel_mask))
outFileWeights.write("static ap_uint<%d> red_ch_index[%d] = {" %(max_ch_width, num_red))

for t in range(num_red):
    outFileWeights.write("%s" % int(red_ch_index[t][0]))
    if t != num_red-1:
        outFileWeights.write(",")
outFileWeights.write("};")  
            
outFileWeights.write("\n\n}\n\n")   
outFileWeights.write("#endif \n")
outFileWeights.close()

print("Done! 'config.h' and 'memdata.h' generated.")
