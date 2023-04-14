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

#ifndef DECONV_TB_H
#define DECONV_TB_H

template<
    unsigned IFMDim,
    unsigned IFMCh,
    unsigned OFMDim,
    unsigned OFMCh,
    unsigned Kernel,
    unsigned Stride,
    unsigned Padding,
    typename TI,
    typename TO,
    typename TW
>
void deconv2d(
    TI const image[IFMDim][IFMDim][IFMCh],
    TW const weights [IFMCh][OFMCh][Kernel][Kernel],
    TO outputs [OFMDim][OFMDim][OFMCh]
) {
    for (unsigned oc=0; oc < OFMCh; oc++) {
        for (unsigned ic=0; ic < IFMCh; ic++) {
            for (unsigned kh=0; kh < Kernel; kh++) {
                for (unsigned kw=0; kw < Kernel; kw++) {
                    TW  w = weights[ic][oc][kh][kw];
                    for (unsigned ih=0; ih < IFMDim; ih++) {
                        for (unsigned iw=0; iw < IFMDim; iw++) {
                            TI  x = image[ih][iw][ic];
                            unsigned  oh = (Stride * ih) + kh - Padding;
                            unsigned  ow = (Stride * iw) + kw - Padding;
                            if ((oh < OFMDim) && (oh >= 0) && (ow < OFMDim) && (ow >= 0)) {
                                TO  y = x * w;
                                outputs[oh][ow][oc] += y;
                            }
                        }
                    }
                }
            }
        }
    }
}

#endif
