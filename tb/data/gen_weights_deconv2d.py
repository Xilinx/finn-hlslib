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

import random

outFileWeights = open("memdata_deconv2d.h" , "wt")
outFileConfig = open("config_deconv2d.h" , "wt")

num_images = 1 # num images
in_channels = 1 # input channels
out_channels = 1 # output channels
in_dim = 4 # assuming square inputs
stride = 3 # assuming square stride
kernel_size = 4 # assuming square kernels
padding = 3 # assuming square padidng
out_dim = stride * (in_dim - 1) + kernel_size - (2 * padding)

i_precision = 6
o_precision = 16
w_precision = 5
simd = in_channels # fully unrolling in channels
pe = 1

outFileConfig.write("#ifndef DECONV_CONFIG_H\n")
outFileConfig.write("#define DECONV_CONFIG_H\n")

# deconvolution hyperparameters
outFileConfig.write("constexpr unsigned  IFDim1 = %d;\n" % in_dim)
outFileConfig.write("constexpr unsigned  IFMCh1 = %d;\n" % in_channels)
outFileConfig.write("constexpr unsigned  OFDim1 = %d;\n" % out_dim)
outFileConfig.write("constexpr unsigned  OFMCh1 = %d;\n" % out_channels)
outFileConfig.write("constexpr unsigned  Kernel1 = %d;\n" % kernel_size)
outFileConfig.write("constexpr unsigned  Stride1 = %d;\n" % stride)
outFileConfig.write("constexpr unsigned  Padding1 = %d;\n" % padding)
outFileConfig.write("\n")

# feature map pixel padding hyperparameters
fm_out_x = in_dim + (in_dim - 1) * (stride - 1)
fm_pad_x = stride
outFileConfig.write("constexpr unsigned  FMPadODim1 = %d;\n" % fm_out_x)
outFileConfig.write("constexpr unsigned  FMPadStride1 = %d;\n" % fm_pad_x)
outFileConfig.write("constexpr unsigned  FMPadSIMD1 = %d;\n" % simd)
outFileConfig.write("\n")

# convolution hyperparameters
conv_stride = 1 # assuming square
conv_padding = kernel_size - padding - 1 # assuming square
assert conv_padding == 0, "not testing additional padding"
tile = in_channels * kernel_size * kernel_size * out_channels // (simd * pe)
outFileConfig.write("constexpr unsigned  ConvKernel1 = %d;\n" % kernel_size)
outFileConfig.write("constexpr unsigned  ConvIFMCh1 = %d;\n" % in_channels)
# input of direct convolution is the output of the pixel padding
outFileConfig.write("constexpr unsigned  ConvIFMDim1 = %d;\n" % fm_out_x)
outFileConfig.write("constexpr unsigned  ConvOFMCh1 = %d;\n" % out_channels)
outFileConfig.write("constexpr unsigned  ConvOFMDim1 = %d;\n" % out_dim)
# not testing addition padding node here
# outFileConfig.write("constexpr unsigned  ConvPadding1 = %d;\n" % conv_padding)
outFileConfig.write("constexpr unsigned  ConvStride1 = %d;\n" % conv_stride)
outFileConfig.write("constexpr unsigned  ConvSIMD1 = %d;\n" % simd)
outFileConfig.write("constexpr unsigned  ConvPE1 = %d;\n" % pe)
outFileConfig.write("\n")

# general hyperparameters
outFileConfig.write("constexpr unsigned  IPrecision = %d;\n" % i_precision)
outFileConfig.write("constexpr unsigned  OPrecision = %d;\n" % o_precision)
outFileConfig.write("constexpr unsigned  WPrecision = %d;\n" % w_precision)
outFileConfig.write("#endif\n")
outFileConfig.write("\n")
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
