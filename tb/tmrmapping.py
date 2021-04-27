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
# \file tmrmapping.py
#
# This library contains function definitios for TMR implementation.
#
################################################################################

import numpy as np

def isvalid(pe, ofm_ch, redf, num_red):
	
	""" 
    =======================================================================================
    
    This function checks if, given a number of PEs and number of redundancies for an OFM, 
	it is possible to map the complete channel stack so that at least each repeated channel
	is computed by separate PEs.
	
	Errors are flagged if the number of PEs suggested is not valid.
        
	Parameters:

	 - pe      : number of PEs
	 - ofm_ch  : number of OFM channels, not counting the triplications
	 - redf    : redundancy factor, (3 for triplication)
	 - num_red : number of triplicated channels
         
    =======================================================================================
    """
	
	if pe < 3:
		print("Error: Number of PEs must be at least 3, to be able to map 1 PE per triplicated channel")
		exit(1)
	else:
		
		# Number of output channels including triplications
		ofm_ch_red = ofm_ch + (num_red * (redf - 1))
		
		if pe > ofm_ch_red:
			print("Error: Number of PEs can be reduced to {}".format(ofm_ch_red))
			exit(1)
		elif pe == ofm_ch_red:
			print("Number of PE = {} is valid!".format(pe))
		elif pe < ofm_ch_red:
			if ofm_ch_red % pe == 0:
				fold = ofm_ch_red / pe
				print("Number of PE = {} is valid, neuron fold = {}".format(pe, fold))
			else:
				print("Error: Number of PEs = {} is not valid. Number of total channels should be divisible by number of PE.".format(pe))
				exit(1)		 
				 
def map(param_in, ofm_ch, pe, redf, num_red, ch_list = []):
	
	""" 
    =======================================================================================
    
    This function maps the triplications and returns the new set of weights, providing also
	a channel mask value and a vector of indexes.
	
	The user can provide weights, number of redundancies and redundancy factor, and the 
	function will return a new set of weights with the specified channels repeated as many times
	as selected with the redundancy factor.
	
	The user should specify which channels are desired to be repeated.
        
	Parameters:

	 - param_in     : matrix of weights, NxM, where N = OFM channels (not inc. trip.), M = tile
	 - ofm_ch       : number of OFM channels, not counting the triplications
	 - pe	        : number of PEs used to compute convolution
	 - redf         : redundancy factor, (3 for triplication)
	 - num_red      : number of triplicated channels
	 - ch_list      : list of channel indexes desired to be repeated
	 
	Returns:
	
	 - param_out    : matrix of weights, NxM, where N = OFM channels (inc. trip.), M = tile
	 - channel_mask : value to mask repeated (1) and not repeated channels (0)
	 - red_ch_index : vector with the indexes of the repeated channels
         
    =======================================================================================
    """
    
    # Flag if the number of triplications don't match
	if len(ch_list) != num_red:
		print("Error: You have specified {} triplications, but {} channel/s to triplicate".format(num_red, len(ch_list)))
		exit(1)	
		
    # Flag if user specifies a wrong channel number
	for x in ch_list:
		if x >= ofm_ch:
			print("Error: You have specified an invalid channel, it should be betwen 0 and {}!".format(ofm_ch - 1))
			exit(1)	
	
	# Number of output channels including triplications
	ofm_ch_red = ofm_ch + (num_red * (redf - 1))
	
	# Generate channel_mask
	channel_mask = ''
	for k in range(ofm_ch):
		added = 0
		for ch in ch_list:
			if k == ch:
				channel_mask = "111" + channel_mask
				added = 1
		if added == 0:
			channel_mask = "0" + channel_mask	
	channel_mask = "0b" + channel_mask
	channel_mask = int(channel_mask, 2)
	
	red_ch_index = np.zeros((num_red, 1))
	# Generate red_ch_index, and order rows to show LSB index first
	aux = 0
	ch_count = 0
	for ch in range(ofm_ch_red):
		if ch in ch_list:
			red_ch_index[aux] = ch_count
			aux += 1
			ch_count += 2
		ch_count += 1	
	red_ch_index = red_ch_index[np.lexsort(np.fliplr(red_ch_index).T)]
	
	ch_repeats = []
	# Create list with number of repeats per channel, and triplicate weights
	for ch in range(ofm_ch):
		added = 0
		for x in ch_list:
			if ch == x:
				ch_repeats.append(redf)
				added = 1
		if added == 0:
			ch_repeats.append(1)
	
	# Triplicate channels	
	param_out = np.repeat(param_in, ch_repeats, axis = 0)
	
	# Consider folding case, split in chunks and concatenate the rows a neuron folding number of times
	param_out = np.concatenate(np.array_split(param_out, ofm_ch_red/pe), axis=1)
			
	return param_out, channel_mask, red_ch_index
