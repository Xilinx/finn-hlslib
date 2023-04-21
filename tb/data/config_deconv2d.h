/******************************************************************************
 *  Copyright (c) 2023, Advanced Micro Devices, Inc.
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

#ifndef DECONV_CONF_H
#define DECONV_CONF_H

constexpr unsigned  IFDim1 = 4;
constexpr unsigned  IFMCh1 = 1;
constexpr unsigned  OFDim1 = 7;
constexpr unsigned  OFMCh1 = 1;
constexpr unsigned  Kernel1 = 4;
constexpr unsigned  Stride1 = 3;
constexpr unsigned  Padding1 = 3;

constexpr unsigned  FMPadODim1 = 10;
constexpr unsigned  FMPadStride1 = 3;
constexpr unsigned  FMPadSIMD1 = 1;

constexpr unsigned  ConvKernel1 = 4;
constexpr unsigned  ConvIFMCh1 = 1;
constexpr unsigned  ConvIFMDim1 = 10;
constexpr unsigned  ConvOFMCh1 = 1;
constexpr unsigned  ConvOFMDim1 = 7;
constexpr unsigned  ConvStride1 = 1;
constexpr unsigned  ConvSIMD1 = 1;
constexpr unsigned  ConvPE1 = 1;

constexpr unsigned  IPrecision = 6;
constexpr unsigned  OPrecision = 16;
constexpr unsigned  WPrecision = 5;

#endif
