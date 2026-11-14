/****************************************************************************
 * Copyright (C) 2024, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @brief	Deconvolution adopted from
 *			https://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=9592768
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 ***************************************************************************/
#ifndef DECONV_HPP
#define DECONV_HPP

#include <ap_int.h>
#include <hls_stream.h>

#include "tensor.hpp"
#include "utils.hpp"

//===========================================================================
// Utility

//- Feature Map Pixel Padding -----------------------------------------------
/**
 * \brief Feature map pixel padding - Pads each pixel in the input feature
 *        map with zeros. Used as a pre-processing step for the transposed
 * 		  convolution operation. Expects data in NHWC format, where N=1.
 *
 * \tparam OutputDim_x Padded width of the output feature map
 * \tparam OutputDim_y Padded height of the output feature map
 * \tparam Stride_x    Stride for each pixel along the width dimension
 * \tparam Stride_y    Stride for each pixel along the height dimension
 * \tparam NumChannels Number of channels of the input feature map
 * \tparam SIMD		   Input parallelism
 * \tparam In_t		   Input datatype
 *
 * @param src          Input stream
 * @param dst 		   Output stream
 */
template<
	unsigned OutputDim_x,
	unsigned OutputDim_y,
	unsigned Stride_x,
	unsigned Stride_y,
	unsigned NumChannels,
	unsigned SIMD,
	typename In_t
>
void FMPadding_Pixel_Nonsquare(
	hls::stream<tensor<In_t, SIMD>> &src,
	hls::stream<tensor<In_t, SIMD>> &dst
) {
	static_assert(NumChannels % SIMD == 0, "SIMD must divide channel count.");
	constexpr unsigned  Folding = NumChannels/SIMD;

#pragma HLS pipeline II=1 style=flp
	static ModCounter<Folding>      sf;
	static ModCounter<OutputDim_x>  xcnt;
	static ModCounter<OutputDim_y>  ycnt;
	static unsigned  xtrig = 0;
	static unsigned  ytrig = 0;
#pragma HLS reset variable=sf
#pragma HLS reset variable=xcnt
#pragma HLS reset variable=ycnt
#pragma HLS reset variable=xtrig
#pragma HLS reset variable=ytrig

	bool const  is_data = (ytrig == 0) && (xtrig == 0);
	if(is_data ? !src.empty() : true) {
		tensor<In_t, SIMD>  value(0);
		if(is_data)  value = src.read();
		dst.write(value);

		if(sf.tick()) {
			if(xcnt.tick()) {
				xtrig = 0;
				if(ycnt.tick())  ytrig = 0;
				else if(++ytrig == Stride_y)  ytrig = 0;
			}
			else {
				if(++xtrig == Stride_x)  xtrig = 0;
			}
		}
	}
}

//- Feature Map Cropping ----------------------------------------------------
template<
	unsigned  P,	// Cropping to remove from all individual edges
	unsigned  H,	// IFM Height
	unsigned  W,	// IFM Width
	unsigned  C,	// IFM Channel Count
	unsigned  SIMD,
	typename  T
>
void crop(
	hls::stream<tensor<T, SIMD>> &src,
	hls::stream<tensor<T, SIMD>> &dst
) {
#pragma HLS interface ap_ctrl_none port=return
	static_assert(C%SIMD == 0, "SIMD parallelism must divide channel count.");

#pragma HLS pipeline II=1 style=flp

	// Positioning within the Uncropped Input Feature Map
	static unsigned  h = 0;
	static unsigned  w = 0;
	static unsigned  d = 0;
#pragma HLS reset variable=h
#pragma HLS reset variable=w
#pragma HLS reset variable=d

	if(!src.empty()) {
		auto const  x = src.read();
		if((P <= w) && (w < W-P) && (P <= h) && (h < H-P))  dst.write(x);
		if(++d == C/SIMD) {
			d = 0;
			if(++w == W) {
				w = 0;
				if(++h == H)  h = 0;
			}
		}
	}

} // crop()

template<
	unsigned  P,	// Padding to all individual edges
	unsigned  H,	// IFM Height
	unsigned  W,	// IFM Width
	unsigned  C,	// IFM Channel Count
	unsigned  SIMD,
	typename  T,
	typename  TV
>
void pad(
	hls::stream<tensor<T, SIMD>> &src,
	hls::stream<tensor<T, SIMD>> &dst,
	TV const  val
) {
#pragma HLS interface ap_ctrl_none port=return
	static_assert(C%SIMD == 0, "SIMD parallelism must divide channel count.");

#pragma HLS function_instantiate variable=val
#pragma HLS pipeline II=1 style=flp

	// Positioning within Padded Output Feature Map
	static unsigned  h = 0;
	static unsigned  w = 0;
	static unsigned  d = 0;
#pragma HLS reset variable=h
#pragma HLS reset variable=w
#pragma HLS reset variable=d

	bool  wr = false;
	tensor<T, SIMD>  y;
	if((h < P) || (P+H <= h) || (w < P) || (P+W <= w)) {
		wr = true;
		for(unsigned  i = 0; i < SIMD; i++) {
#pragma HLS unroll
			y[i] = val;
		}
	}
	else {
		wr = src.read_nb(y);
	}

	if(wr) {
		dst.write(y);
		if(++d == C/SIMD) {
			d = 0;
			if(++w == P+W+P) {
				w = 0;
				if(++h == P+H+P)  h = 0;
			}
		}
	}

} // pad()

//===========================================================================
// Deconv Building Blocks

template<
	unsigned  K,	// kernel Size
	unsigned  S, 	// stride
	unsigned  H,	// IFM height
	unsigned  W,	// IFM Width
	unsigned  CF,	// channel fold (CO/PE)
	unsigned  SF,	// SIMD fold (CI/SIMD)
	unsigned  PE,
	unsigned  SIMD,
	typename  TW
>
void deconv_weights(
	TW const (&kernel)[CF*K*K*SF][PE][SIMD],
	hls::stream<tensor<TW, PE, SIMD>> &dst
) {
#pragma HLS interface ap_ctrl_none port=return

#pragma HLS pipeline II=1 style=flp
	static_assert(K%S == 0, "Stride must divide kernel size.");
	constexpr unsigned  KK = K/S;

	static unsigned  idx = SF*(K+1)*(K-S);
	static unsigned  y = 0;
	static unsigned  sy = 0;
	static unsigned  x = 0;
	static unsigned  sx = 0;
	static unsigned  c = 0;
	static unsigned  ksy = 0;
	static unsigned  ksx = 0;
	static unsigned  d = 0;
#pragma HLS reset variable=idx
#pragma HLS reset variable=y
#pragma HLS reset variable=sy
#pragma HLS reset variable=x
#pragma HLS reset variable=sx
#pragma HLS reset variable=c
#pragma HLS reset variable=ksy
#pragma HLS reset variable=ksx
#pragma HLS reset variable=d

#pragma HLS array_partition variable=kernel dim=1
	tensor<TW, PE, SIMD>  v;
	for(unsigned  i = 0; i < PE; i++) {
#pragma HLS unroll
		for(unsigned  j = 0; j < SIMD; j++) {
#pragma HLS unroll
			v[i][j] = kernel[idx][i][j];
		}
	}

	if(dst.write_nb(v)) {

		signed  delta = 1;
		if(d != SF-1)  d++;	// [c,ky,kx,d] += [0,0,0,1]
		else {
			d = 0;

			delta -= SF*(S+1);	// [c,ky,kx,d] += [0,0,-S,-SF]
			if(ksx != KK-1)  ksx++;
			else {
				ksx = 0;

				delta -= SF*K*(S-1);	// [c,ky,kx,d] += [0,-S,K,0]
				if(ksy != KK-1)  ksy++;
				else {
					ksy = 0;

					delta += 2*SF*K*K;	// [c,ky,kx,d] += [1,K,0,0]
					if(c != CF-1)  c++;
					else {
						c = 0;

						delta -= SF*(CF*K*K-1);	// [c,ky,kx,d] += [-CF,0,1,0]
						if(sx != S-1)  sx++;
						else {
							sx = 0;

							delta -= SF*S;	// [c,ky,kx,d] += [0,0,-S,0]
							if(x != W-KK)  x++;
							else {
								x = 0;

								delta += SF*K;	// [c,ky,kx,d] += [0,1,0,0]
								if(sy != S-1)  sy++;
								else {
									sy = 0;

									delta -= SF*K*S;	// [c,ky,kx,d] += [0,-S,0,0]
									if(y != H-KK)  y++;
									else {
										y = 0;
									}
								}
							}
						}
					}
				}
			}
		}
		idx += delta;
	}

} // deconv_weights()

template<
	unsigned  K,	// kernel Size
	unsigned  S, 	// stride
	unsigned  H,	// IFM height
	unsigned  W,	// IFM Width
	unsigned  CF,	// channel fold (CO/PE)
	unsigned  SF,	// SIMD fold (CI/SIMD)
	typename  T		// e.g. tensor<TI, SIMD>
>
void deconv_swg(
	hls::stream<T> &src,
	hls::stream<T> &dst
) {
#pragma HLS interface ap_ctrl_none port=return

#pragma HLS pipeline II=1 style=flp
	static_assert(K%S == 0, "Stride must divide kernel size.");
	constexpr unsigned  KK = K/S;
	constexpr unsigned  ADDR_BITS = clog2(KK*W*SF);

	// Cyclic buffer with wrapping pointers, pointers have an extra MSB beyond the memory address space to capture buffer generations
	//	- wp & cp increment monotonously, rp wiggles between them
	//	- read can proceed if rp < wp
	//	- cp <= rp < cp', rp cannot rewind below cp
	//	- wp <= cp', write can proceed if wp < cp' (cp of next buffer generation)
	static T  buf[1<<ADDR_BITS];
#pragma HLS dependence variable=buf inter direction=WAR false
#pragma HLS dependence variable=buf inter direction=RAW distance=1 true
	using  ptr_t = ap_int<1+ADDR_BITS>;
	constexpr unsigned  WP_DEPTH = 4;	// TODO: Why do we need 4? How does this relate to `distance` above?
	static ptr_t  wp[WP_DEPTH] = { 0, };	// incl. delayed pointers for guarding read progession
	static ptr_t  rp = 0;
	static ptr_t  cp = 0;
#pragma HLS array_partition variable=wp complete
#pragma HLS reset variable=wp
#pragma HLS reset variable=rp
#pragma HLS reset variable=cp

	static unsigned  h  = 0;
	static unsigned  sh = 0;
	static unsigned  w  = 0;
	static unsigned  sw = 0;
	static unsigned  kh = 0;
	static unsigned  kw = 0;
	static unsigned  d  = 0;
#pragma HLS reset variable=h
#pragma HLS reset variable=sh
#pragma HLS reset variable=w
#pragma HLS reset variable=sw
#pragma HLS reset variable=kh
#pragma HLS reset variable=kw
#pragma HLS reset variable=d

	for(unsigned  i = WP_DEPTH-1; i > 0; i--)  wp[i] = wp[i-1];
	if(/* rp < wp */ ptr_t(rp-wp[WP_DEPTH-1]) < 0) {
		T const  y = buf[ap_uint<ADDR_BITS>(rp)];
		if(dst.write_nb(y)) {
			signed    rinc = 1;	// default: one step forward
			unsigned  cinc = 0;

			if(d != SF-1)  d++;
			else {
				d = 0;
				if(kw != KK-1)  kw++;
				else {
					kw = 0;
					rinc += (W-KK)*SF;	// skip to next row of kernel volume
					if(kh != KK-1)  kh++;
					else {
						kh = 0;
						rinc -= KK*W*SF;	// unskip back to beginning of kernel volume
						if(sw != CF*S-1)  sw++;
						else {
							sw = 0;
							rinc += SF;	// advance kernel position horizontally
							if(w != W-KK)  w++;
							else {
								w = 0;
								rinc -= (W-KK+1)*SF;	// unskip row for repetition
								if(sh != S-1)  sh++;
								else {
									sh = 0;
									rinc += W*SF;	// advance kernel position vertically
									cinc = W*SF;
									if(h != H-KK)  h++;
									else {
										h = 0;
										rinc += (KK-1)*W*SF;	// skip shallow leftover rows
										cinc = KK*W*SF;
									}
								}
							}
						}
					}
				}
			}
			rp += rinc;
			cp += cinc;
		}
	}

	if(/* wp <= cp' */ ptr_t(wp[0]-cp) >= 0) {
		T  x;
		if(src.read_nb(x))  buf[ap_uint<ADDR_BITS>(wp[0]++)] = x;
	}

} // deconv_swg()

template<
	unsigned  N,	// dot product depth
	unsigned  PE,
	unsigned  SIMD,
	typename  TW,
	typename  TI,
	typename  TO
>
void deconv_mvu(
	hls::stream<tensor<TW, PE, SIMD>> &wgt,
	hls::stream<tensor<TI, SIMD>>     &src,
	hls::stream<tensor<TO, PE>>       &dst
) {
#pragma HLS interface ap_ctrl_none port=return

#pragma HLS pipeline II=1 style=flp
	static TO  accu[PE] = { 0, };
	static ap_uint<clog2(N)>  cnt = 0;
	static bool  push = false;
#pragma HLS array_partition variable=accu complete
#pragma HLS reset variable=accu
#pragma HLS reset variable=cnt
#pragma HLS reset variable=push

	// Complete marked Output
	if(push) {
		tensor<TO, PE>  y;
		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
			y[pe] = accu[pe];
			accu[pe] = 0;
		}
		dst.write(y);
		push = false;
	}

	if(!wgt.empty() && !src.empty()) {

		// Broadcast activation to all PEs in parallel
		auto const  ww = wgt.read();
		auto const  a  = src.read();
		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
			auto const  w = ww[pe];
			TO  p = 0;
			for(unsigned  i = 0; i < SIMD; i++) {
#pragma HLS unroll
				p += w[i] * a[i];
			}
			accu[pe] += p;
		}

		// Mark for Output
		if(cnt < N-1)  cnt++;
		else {
			cnt = 0;
			push = true;
		}

	}

} // deconv_mvu()


template<
	unsigned  K,	// kernel Size
	unsigned  S, 	// stride
	unsigned  P,	// (de)padding
	unsigned  H,	// IFM height
	unsigned  W,	// IFM Width
	unsigned  CO,	// output channels
	unsigned  CI,	// input channels
	unsigned  PE,
	unsigned  SIMD,
	typename  TW,
	typename  TI,
	typename  TO
>
void deconv(
	TW const (&kernel)[(CO/PE)*K*K*(CI/SIMD)][PE][SIMD],
	hls::stream<tensor<TI, SIMD>> &src,
	hls::stream<tensor<TO, PE>>   &dst
) {
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS dataflow disable_start_propagation

	// Parameter Validation & Fold Derivation
	static_assert(CO%PE   == 0, "PE parallelism must divide output channel count.");
	static_assert(CI%SIMD == 0, "SIMD parallelism must divide input channel count.");
	constexpr unsigned  CF = CO/PE;
	constexpr unsigned  SF = CI/SIMD;

	// Padding and cropping values to accommodate P != K-S
	constexpr unsigned  PADUP = (P >= K-S)? 0 : (K-P-1)/S;
	constexpr unsigned  CROP  = S*PADUP - ((K-S)-P);
	constexpr unsigned  H_EFF = PADUP + H + PADUP;
	constexpr unsigned  W_EFF = PADUP + W + PADUP;
	constexpr unsigned  HO_EFF = (H_EFF+1)*S - K;
	constexpr unsigned  WO_EFF = (W_EFF+1)*S - K;

	// Continuous Weight Feed
	static hls::stream<tensor<TW, PE, SIMD>>  wgt("wgt");
#pragma HLS stream depth=2 variable=wgt
	deconv_weights<K, S, H_EFF, W_EFF, CF, SF>(kernel, wgt);

	// Activation Processing Pipeline: pad -> swg -> mvu -> crop
	static hls::stream<tensor<TI, SIMD>>  src_eff("src_eff");
	static hls::stream<tensor<TI, SIMD>>  swg    ("swg");
	static hls::stream<tensor<TO, PE>>  dst_eff("dst_eff");
#pragma HLS stream depth=2 variable=src_eff
#pragma HLS stream depth=2 variable=swg
#pragma HLS stream depth=2 variable=dst_eff

	pad<PADUP, H, W, CI>(src, src_eff, TI(0));

	deconv_swg<K, S, H_EFF, W_EFF, CF, SF>(src_eff, swg);
	deconv_mvu<K/S*K/S*SF>(wgt, swg, dst_eff);

	crop<CROP, HO_EFF, WO_EFF, CO>(dst_eff, dst);

} // deconv()

#endif
