#   Copyright (c) 2023, Advanced Micro Devices, Inc.
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

import os
import sys
import random 
import subprocess
import numpy as np

outFileWeights = open("memdata_deconv2d.h" , "wt")
outFileConfig = open("config_deconv2d.h" , "wt")

num_images = 1 # num images
in_channels = 3 # input channels
out_channels = 4 # output channels
in_x = in_y = 4 # height/width of inp (assuming square)
padding = 1 # padding (assuming square)
stride_x = stride_y = 2 # stride (assuming square)
kernel_x = kernel_y = 4 # kernel size (assuming square)
out_x = stride_x * (in_x - 1) - (2 * padding) + kernel_x
out_y = stride_y * (in_y - 1) - (2 * padding) + kernel_y

assert out_x % in_x == 0, "Need even upsampling factor."
assert out_y % in_y == 0, "Need even upsampling factor."

i_precision = 4
o_precision = 16
w_precision = 4
simd = in_channels # fully unrolling in channels
pe = 2
# mmv = 1 # todo - figure out what this is

conv_stride = 1 # assuming square
conv_padding = kernel_x - padding - 1 # assuming square

tile = in_channels * kernel_x * kernel_y * out_channels // (simd * pe)

assert in_y == in_x, "Testing square inputs."
assert out_y == out_x, "Testing square outputs."
assert kernel_x == kernel_y, "Testing square kernels."
assert stride_x == stride_y, "Testing square strides."
outFileConfig.write("constexpr unsigned  DeconvIFDim = %d;\n" % in_x)
outFileConfig.write("constexpr unsigned  DeconvIFMCh = %d;\n" % in_channels)
outFileConfig.write("constexpr unsigned  DeconvOFDim = %d;\n" % out_x)
outFileConfig.write("constexpr unsigned  DeconvOFMCh = %d;\n" % out_channels)
outFileConfig.write("constexpr unsigned  DeconvKernel = %d;\n" % kernel_x)
outFileConfig.write("constexpr unsigned  DeconvStride = %d;\n" % stride_x)
outFileConfig.write("constexpr unsigned  DeconvPadding = %d;\n" % padding)
outFileConfig.write("constexpr unsigned  IPrecision = %d;\n" % i_precision)
outFileConfig.write("constexpr unsigned  OPrecision = %d;\n" % o_precision)
outFileConfig.write("constexpr unsigned  WPrecision = %d;\n" % w_precision)
outFileConfig.write("constexpr unsigned  ConvSIMD1 = %d;\n" % simd)
outFileConfig.write("constexpr unsigned  ConvPE1 = %d;\n" % pe)

fm_out_x = in_x + (in_x - 1) * (stride_x - 1)
fm_pad_x = out_x // in_x
outFileConfig.write("constexpr unsigned  FMPadODim = %d;\n" % fm_out_x)
outFileConfig.write("constexpr unsigned  FMPadStride = %d;\n" % fm_pad_x)

outFileConfig.close()


outFileWeights.write("#ifndef PARAMS_HPP\n")
outFileWeights.write("#define PARAMS_HPP\n")

outFileWeights.write("namespace PARAM{ \n")
if (w_precision == 1):
	outFileWeights.write("static BinaryWeights<%d,%d,%d> weights= {\n{\n" %(simd, pe, tile))
else:
	outFileWeights.write(
		"static FixedPointWeights<%d,ap_int<%d>,%d,%d> weights= {\n{\n" % (simd, w_precision, pe, tile)
	)
for p in range(pe):
	outFileWeights.write("{ \n")
	for t in range(tile):
		width = simd*w_precision
		val = random.randint(0, 1<<width-1)
		outFileWeights.write("%s" % hex(val))
		if t!=tile-1:
			outFileWeights.write(",\n")
	outFileWeights.write("} \n")
	if p!=pe-1:
		outFileWeights.write(",")
outFileWeights.write("}\n};\n } \n")
outFileWeights.write("#endif \n")
outFileWeights.close()
