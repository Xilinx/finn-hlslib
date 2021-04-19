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
 *  Authors: Giulio Gambardella <giuliog@xilinx.com>
 *
 *  \file conv.hpp
 *
 *  C++ Implementation of a convolution, used for testbench
 *
 *****************************************************************************/
#ifndef CONV_TB_H
#define CONV_TB_H

template<int MAX_IMAGE,
	int IFMDim, 
	int OFMDim, 
	int IFMCh, 
	int OFMCh, 
	typename TI, 
	typename TO, 
	typename TW>
	void conv_1x1(TI const img[MAX_IMAGE][IFMDim][IFMDim][IFMCh], TW const weights[OFMCh][IFMCh], TO out[MAX_IMAGE][OFMDim][OFMDim][OFMCh]){
		constexpr int stride= (OFMDim==1)? IFMDim:(IFMDim - 1)/(OFMDim - 1);
		for(int n=0;n<MAX_IMAGE;n++)
			for(int x=0;x<OFMDim;x++)
				for(int y=0;y<OFMDim;y++)
					for(int h=0;h<OFMCh;h++){
						TO tmp = 0;
						for(int w=0;w<IFMCh;w++)
							tmp+=img[n][x*stride][y*stride][w] * weights[h][w];
						out[n][x][y][h] = tmp;

					}
	}

template<int MAX_IMAGE,
	int IFMDim,
	int OFMDim,
	int IFMCh,
	int OFMCh,
	int kernel,
	int stride,
	typename TI,
	typename TO,
	typename TW>
	void conv(TI const img[MAX_IMAGE][IFMDim*IFMDim][IFMCh], TW const weights[OFMCh][kernel][kernel][IFMCh], TO out[MAX_IMAGE][OFMDim][OFMDim][OFMCh]){
		for(int n=0;n<MAX_IMAGE;n++)
			for(int x=0;x<OFMDim;x++)
				for(int y=0;y<OFMDim;y++)
					for(int h=0;h<OFMCh;h++){
						TO tmp = 0;
						for (int ky=0;ky<kernel;ky++)
							for (int kx=0;kx<kernel;kx++)
								for(int w=0;w<IFMCh;w++){
									tmp+=img[n][(y*stride+ky)*IFMDim+x*stride+kx][w] * weights[h][kx][ky][w];
								}
						out[n][x][y][h] = tmp;
					}
	}

template<int MAX_IMAGE,
	int IFMDim,
	int OFMDim,
	int IFMCh,
	int OFMCh,
	int kernel,
	int stride,
	typename TI,
	typename TO,
	typename TW>
	void conv_stmr(TI const img[MAX_IMAGE][IFMDim*IFMDim][IFMCh], TW const weights[OFMCh][kernel][kernel][IFMCh], TO out[MAX_IMAGE][OFMDim][OFMDim][OFMCh]){
		for(int n=0;n<MAX_IMAGE;n++){
			cout << "Image " << n << endl;
			cout << "Conv function computes: " << endl;
			cout << "Format: <OFM[Row][Column][Channel]>" << endl;
			for(int x=0;x<OFMDim;x++)
				for(int y=0;y<OFMDim;y++)
					for(int h=0;h<OFMCh;h++){
						TO tmp = 0;
						for (int ky=0;ky<kernel;ky++)
							for (int kx=0;kx<kernel;kx++)
								for(int w=0;w<IFMCh;w++){
									tmp+=img[n][(y*stride+ky)*IFMDim+x*stride+kx][w] * weights[h][kx][ky][w];
									//cout << tmp << " = " << weights[h][kx][ky][w] << " * " << img[n][(y*stride+ky)*IFMDim+x*stride+kx][w] << endl;
								}
						out[n][x][y][h] = tmp;
						cout << "OFM[" << x << "][" << y << "][" << h << "]: " << out[n][x][y][h] << endl;
					}
			cout << "---------------\n\n" << endl;
		}
	}

template<int MAX_IMAGE,
	int IFMDim_x,
	int IFMDim_y,
	int OFMDim_x,
	int OFMDim_y,
	int IFMCh,
	int OFMCh,
	int kernel_x,
	int kernel_y,
	int stride_x,
	int stride_y,
	typename TI,
	typename TO,
	typename TW>
	void conv_nonsquare(TI const img[MAX_IMAGE][IFMDim_x][IFMDim_y][IFMCh], TW const weights[OFMCh][kernel_x][kernel_y][IFMCh], TO out[MAX_IMAGE][OFMDim_x][OFMDim_y][OFMCh]){
		for(int n=0;n<MAX_IMAGE;n++)
			for(int y=0;y<OFMDim_y;y++)
				for(int x=0;x<OFMDim_x;x++)
					for(int h=0;h<OFMCh;h++){
						TO tmp = 0;
						for (int ky=0;ky<kernel_y;ky++)
							for (int kx=0;kx<kernel_x;kx++)
								for(int w=0;w<IFMCh;w++){
									tmp+=img[n][x*stride_x+kx][y*stride_y+ky][w] * weights[h][kx][ky][w];
								}
						out[n][x][y][h] = tmp;
					}
	}



template<int MAX_IMAGE,
	int IFMDim,
	int OFMDim,
	int FMCh,
	int kernel,
	int stride,
	typename TI,
	typename TO,
	typename TW>
	void dwsconv(TI const img[MAX_IMAGE][IFMDim][IFMDim][FMCh], TW const weights[FMCh][kernel][kernel], TO out[MAX_IMAGE][OFMDim][OFMDim][FMCh]){
		for(int n=0;n<MAX_IMAGE;n++)
			for(int y=0;y<OFMDim;y++)
				for(int x=0;x<OFMDim;x++)
					for(int h=0;h<FMCh;h++){
						TO tmp = 0;
						for (int ky=0;ky<kernel;ky++)
							for (int kx=0;kx<kernel;kx++){
								tmp+=img[n][y+kx][x+ky][h] * weights[h][kx][ky];
							}
						out[n][x][y][h] = tmp;
					}
	}


template<int MAX_IMAGE,
	int IFMDim_x,
	int IFMDim_y,
	int OFMDim_x,
	int OFMDim_y,
	int FMCh,
	int kernel_x,
	int kernel_y,
	int stride_x,
	int stride_y,
	typename TI,
	typename TO,
	typename TW>
	void dwsconv_nonsquare(TI const img[MAX_IMAGE][IFMDim_x][IFMDim_y][FMCh], TW const weights[FMCh][kernel_x][kernel_y], TO out[MAX_IMAGE][OFMDim_x][OFMDim_y][FMCh]){
		for(int n=0;n<MAX_IMAGE;n++)
			for(int y=0;y<OFMDim_y;y++)
				for(int x=0;x<OFMDim_x;x++)
					for(int h=0;h<FMCh;h++){
						TO tmp = 0;
						for (int ky=0;ky<kernel_y;ky++)
							for (int kx=0;kx<kernel_x;kx++){
								tmp+=img[n][x*stride_x+kx][y*stride_y+ky][h] * weights[h][kx][ky];
							}
						out[n][x][y][h] = tmp;
					}
	}

#endif
