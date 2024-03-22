#ifndef DECONV_HPP
#define DECONV_HPP

#include <ap_int.h>
#include <hls_stream.h>
#include <hls_vector.h>

#include <iostream>


//===========================================================================
// Utility

//- Static Evaluation of ceil(log2(x)) --------------------------------------
constexpr unsigned clog2(unsigned  x) {
  return  x<2? 0 : 1+clog2((x+1)/2);
}


//===========================================================================
// Deconv Building Blocks

template<
	unsigned  K,	// kernel Size
	unsigned  S, 	// stride
	unsigned  H,	// IFM height
	unsigned  W,	// IFM Width
	unsigned  CF,	// channel fold (CO/PE)
	unsigned  SF,	// SIMD fold (CI/SIMD)
	typename  TV	// e.g. hls::vector<hls::vector<TW, SIMD>, PE>
>
void deconv_weights(
	TV const (&kernel)[CF*K*K*SF],
	hls::stream<TV> &dst
) {
#pragma HLS pipeline II=1 style=flp
	static_assert(K%S == 0, "Kernel size must be multiple of stride.");
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

	if(dst.write_nb(kernel[idx])) {

//std::cout
//	<< '|' << y << ':' << sy << ':' << x << ':' << sx << ':' << c << ':' << ksy << ':' << ksx << ':' << d
//	<< std::endl;
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
	typename  T		// e.g. hls::vector<TI, SIMD>
>
void deconv_swg(
	hls::stream<T> &src,
	hls::stream<T> &dst
) {
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
	constexpr unsigned  WP_DEPTH = 4;
	static ptr_t  wp[WP_DEPTH] = { 0, };	// incl. delayed pointers for guarding read progession
	static ptr_t  rp = 0;
	static ptr_t  cp = 0;
#pragma HLS partition variable=wp complete
#pragma HLS reset variable=wp
#pragma HLS reset variable=rp
#pragma HLS reset variable=cp

	/*
	// Produce output in this scheme:
	for(unsigned  h = 0; h < H-KK+1; h++) {
		for(unsigned  sh = 0; sh < S; sh++) {
			for(unsigned  w = 0; w < W-KK+1; w++) {
				for(unsigned  sw = 0; sw < S*CO; sw++) {
					for(unsigned  kh = 0; kh < KK; kh++) {
						for(unsigned  kw = 0; kw < KK; kw++) {
							for(unsigned  d = 0; d < SF; d++) {
								emit(img[h+kh, w+kw, d]);
							}
						}
					}
				}
			}
		}
	}
	*/
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
						if(sw != CO*S-1)  sw++;
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
	hls::stream<hls::vector<hls::vector<TW, SIMD>, PE>> &wgt,
	hls::stream<hls::vector<TI, SIMD>>                  &src,
	hls::stream<hls::vector<TO, PE>>                    &dst
) {
#pragma HLS pipeline II=1 style=flp
	static hls::vector<TO, PE>  accu = { 0, };
	static ap_uint<clog2(N)>  cnt = 0;
	static bool  push = false;
#pragma HLS reset variable=accu
#pragma HLS reset variable=cnt
#pragma HLS reset variable=push

	// Complete marked Output
	if(push) {
//std::cout << "-> " << accu << std::endl;
		dst.write(accu);
		for(unsigned  pe = 0; pe < PE; pe++)  accu[pe] = 0;
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
//std::cout << "+= " << w[i] << " * " << a[i] << std::endl;
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
	hls::vector<hls::vector<TW, SIMD>, PE> const (&kernel)[(CO/PE)*K*K*(CI/SIMD)],
	hls::stream<hls::vector<TI, SIMD>> &src,
	hls::stream<hls::vector<TO, PE>>   &dst
) {
#pragma HLS dataflow disable_start_propagation

	static hls::stream<hls::vector<hls::vector<TW, SIMD>, PE>>  wgt("wgt");
	static hls::stream<hls::vector<TI, SIMD>>  swg("swg");
#pragma HLS stream depth=2 variable=wgt
#pragma HLS stream depth=2 variable=swg

	constexpr unsigned  CF = CO/PE;
	constexpr unsigned  SF = CI/SIMD;
	deconv_weights<K, S, H, W, CF, SF>(kernel, wgt);
	deconv_swg    <K, S, H, W, CF, SF>(src, swg);
	deconv_mvu  <K/S*K/S*SF, PE, SIMD>(wgt, swg, dst);

} // deconv()

#endif
