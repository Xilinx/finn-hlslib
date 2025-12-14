/****************************************************************************
 * Copyright (C) 2024-2025, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @brief	Floating-point pipeline for SoftMax implementation.
 * @author	Shane Fleming <shane.fleming@amd.com>
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 *
 * @description
 *	This design implements a 3-stage pipeline performing two normalization
 *	steps on the processed data:
 *	- Input normalization subtracting the maximum of the input vector from
 *	  all its elements bringing all exponentials into (0, 1]. Given the well-
 *	  defined value range of these exponentials, a fixed-point accumulation
 *	  can be performed. It guarantees that there would be, at least, one
 *	  accumulation order over the original floating-point exponentials that
 *	  does not achieve a better numeric accuracy than the performed fixed-point
 *	  accumulation. This is also not restricted to standard C++ floating
 *	  point types, but also supports VitisHLS ap_float<W,I> datatypes:
 *		- At least one exponential is 1 (corresponding to a maximum input).
 *		- Starting the accumulation with a 1 forces the unit of least precision
 *		  for the remainder of the accumulation process to, at best, 2^{W-I}
 *		  for ap_float<W,I>.
 *		- Adding an extra round bit with weight 2^{(W-I)+1)} accommodates potential
 +		  contributions that would result from floating-point normalization.
 *	- The actual SoftMax normalization dividing each individual exponential
 *	  by the total sum of exponentials. This stage computes the values with
 *	  the datatype exp_t and afterwards recasts it back to float:
 *
 *     ┌───────┐          ┌────────────────────┐          ┌────────┐
 *     │       │  ┌─────┐ │  ┌───┐  ┌───┐      │  ┌─────┐ │  ┌───┐ │
 *   ──┼───┬───┼─►│||N||├─┼─►│SUB├─►│EXP├──┬───┼─►│||N||├─┼─►│DIV├─┼─►
 *     │   │   │  └─────┘ │  └───┘  └───┘  │   │  └─────┘ │  └───┘ │
 *     │   ▼   │          │    ▲           ▼   │          │    ▲   │
 *     │ ┌───┐ │    ┌┐    │    │         ┌───┐ │    ┌┐    │    │   │
 *     │ │MAX├─┼───►│├────┼────┘         │SUM├─┼───►│├────┼────┘   │
 *     │ └───┘ │    └┘    │              └───┘ │    └┘    │        │
 *     └───────┘          └────────────────────┘          └────────┘
 *
 *	- The presence of infinite inputs is detected explicitly. If they exist,
 *	  they will force all other outputs to zero while distributing a total of
 *	  one evenly among themselves.
 *	- Any NaN in the input vector will result in an undefined output for
 *	  that vector.
 *
 * @todo	Optimize Normalization before Exponentiation
 *	Currently the input is converted to float before the maximum subtraction.
 *	This preempts range issues in performing the substraction as val-max
 *	may go below the original input range. This operation should be
 *	specialized for integral types widening the values into appropriate
 *	ap_int<> for performing the subtraction in fixed-point before the
 *	float conversion.
 * @todo	Optimize Computation of Exponentials for Lower-Precision Integers
 *	Instead of relying on a floating-point exponentiation, it's likely more
 *	efficient to rely on a table-based lookup for narrower ap_(u)int inputs
 *	of up to about 8 bits.
 ***************************************************************************/
#ifndef SOFTMAX_HPP
#define SOFTMAX_HPP

#include "utils.hpp"
#include <ap_fixed.h>
#include <hls_math.h>

// TI - The input datatype
// TO - The output datatype must be a floating point type (float / ap_float)
// N - The size of the vector that the SoftMax is being performed over
// SIMD - The amount of parallelism
template<typename TI,
 	 typename TO,
	 size_t N,
	 size_t SIMD>
class SoftMax {
	public:
		static_assert(is_floating_point_or_ap_float<TO>::value, "Internal datatype must be a float or ap_float type");

	public:
		// Public API for executing the softmax dataflow pipeline
		void execute(
			hls::stream<hls::vector<TI, SIMD>> &src,
			hls::stream<hls::vector<TO, SIMD>> &dst
		) {
#pragma HLS dataflow disable_start_propagation
#pragma HLS stream variable=max2exp_dat depth=N/SIMD
#pragma HLS stream variable=max2exp_max depth=2
#pragma HLS stream variable=exp2div_dat depth=N/SIMD
#pragma HLS stream variable=exp2div_sum depth=2
			static_assert(N%SIMD == 0, "N must be a multiple of SIMD");

			max_extract  (src);
			exponentiate ();
			div_stage    (dst);

		} // execute()


	private:

		static constexpr int SUM_PRECISION = std::numeric_limits<TO>::digits;

		// Internal Fixed-Point Datatype for Accumulation (ulp = 2^{-SUM_PRECISION})
		//	- exp_t - exponentials in [0:1]
		//	- red_t - reduction of SIMD exponentials in [0:SIMD]
		//	- sum_t - overall accumulated sum in [0:N*SIMD]
		using  exp_t = ap_ufixed<1+SUM_PRECISION, 1, AP_RND>;
		using  red_t = ap_ufixed<clog2(  SIMD+1)+SUM_PRECISION, clog2(  SIMD+1), AP_RND>;
		using  sum_t = ap_ufixed<clog2(N*SIMD+1)+SUM_PRECISION, clog2(N*SIMD+1), AP_RND>;

		// Helper function to detect infinities (for types with infinities)
		template <typename U = TI>
		constexpr typename std::enable_if<std::numeric_limits<U>::has_infinity, bool>::type
		check_infinity(U value) {
#pragma HLS inline
		    return (value == std::numeric_limits<U>::infinity());
		}

		// Helper function to detect infinities (for types without infinities)
		template <typename U = TI>
		constexpr typename std::enable_if<!std::numeric_limits<U>::has_infinity, bool>::type
		check_infinity(U) {
#pragma HLS inline
		    return false;
		}

		//-----------------------------------------------------------------------
		// Internal streams used to construct the pipeline
		hls::stream<hls::vector<TI, SIMD>>     max2exp_dat;
		hls::stream<TI>                        max2exp_max;
		hls::stream<hls::vector<exp_t, SIMD>>  exp2div_dat;
		hls::stream<sum_t>                     exp2div_sum;


		//-----------------------------------------------------------------------
		// Stage #1: Max Extraction & Infinity Detection
		ModCounter<N/SIMD>  max_cnt;
		TI                  max_val = std::numeric_limits<TI>::lowest();

		void max_extract(
			hls::stream<hls::vector<TI, SIMD>> &src
		) {
#pragma HLS pipeline II=1 style=flp
#pragma HLS reset variable=max_cnt
#pragma HLS reset variable=max_val

			if(!src.empty()) {
				auto const  x = src.read();
				max_val = std::max(max_val, tree_reduce(x, [](TI const &a, TI const &b) { return std::max(a,b); }));
				max2exp_dat.write(x);
				if(max_cnt.tick()) {
					max2exp_max.write(max_val);
					max_val = std::numeric_limits<TI>::lowest();
				}
			}

		} // max_extract()


	private:
		//-----------------------------------------------------------------------
		// Stage #2: Normalized Exponentiation
		// private instance members for the exponentiation pipeline stage
		bool                 exp_valid = false;
		ModCounter<N/SIMD>   exp_cnt;
		TO                   exp_max_value;
		bool           	     exp_has_infty;
		sum_t                exp_total;

		// normalised exponentiation
		void exponentiate() {
#pragma HLS pipeline II=1 style=flp
#pragma HLS reset variable=exp_valid
#pragma HLS reset variable=exp_cnt
#pragma HLS reset variable=exp_max_value off
#pragma HLS reset variable=exp_has_infty off
#pragma HLS reset variable=exp_total off

			if(!exp_valid && !max2exp_max.empty()) {
				exp_max_value = TO(max2exp_max.read());
				exp_has_infty = check_infinity(exp_max_value);
				exp_valid = true;
				exp_total = 0;
				return;
			}

			if(exp_valid && !max2exp_dat.empty()) {
				auto const  x = max2exp_dat.read();
				hls::vector<exp_t, SIMD>  y;

				for(size_t  i = 0; i < SIMD; i++) {
#pragma HLS unroll
					TI    const  xx = x[i];
					// In the presence of infinities, this switches to counting them.
					float const  yy = exp_has_infty? (check_infinity(xx)? 1.0f : 0.0f) : hls::exp(float(TO(xx) - exp_max_value));
					y[i] = exp_t(yy);
				}
				exp_total += tree_reduce<SIMD, exp_t, red_t>(y, [](red_t a, red_t b) { return  a+b; });
				exp2div_dat.write(y);

				if(exp_cnt.tick()) {
					exp2div_sum.write(exp_total);
					exp_valid = false;
				}
			}

		} // exponentiate()

	private:
		//-----------------------------------------------------------------------
		// Stage #3: SoftMax Normalisation
		// private instance members for the softmax normalisation pipeline stage
		bool div_valid = false;
		ModCounter<N/SIMD>   div_cnt;
		float                div_val;

		void div_stage(
			hls::stream<hls::vector<TO, SIMD>> &dst
		) {
#pragma HLS pipeline II=1 style=flp
#pragma	HLS reset variable=div_valid
#pragma	HLS reset variable=div_cnt
#pragma	HLS reset variable=div_val off

			if(!div_valid && !exp2div_sum.empty()) {
				div_val = float(exp2div_sum.read());
			      	div_valid = true;
			}

			if(div_valid && !exp2div_dat.empty()) {
				auto const  x = exp2div_dat.read();
				hls::vector<TO, SIMD>  y;

				for(unsigned  i = 0; i < SIMD; i++) {
#pragma	HLS unroll
					y[i] = TO(float(x[i])/div_val);
				}
				dst.write(y);

				if(div_cnt.tick())  div_valid = false;
			}

		} // div_stage()

};

#endif
