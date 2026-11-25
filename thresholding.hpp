/******************************************************************************
 *  Copyright (C) 2026, Advanced Micro Devices, Inc.
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
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 * @brief	Pipelined thresholding by binary search over per-stage memories.
 * @author	Thomas B. Preußer <thomas.preusser@amd.com>
 *
 * @description
 *  Produces the count of those among N sorted thresholds that are not larger
 *  than the corresponding input, offset by BIAS:
 *     y = Σ(T_i <= x) + BIAS
 *  The count is determined by a binary search resolving one decision level
 *  per stage over M = clog2(N+1) stages, the HLS counterpart of
 *  finn-rtllib/thresholding/hdl/thresholding.sv.
 *
 *  Each stage S owns a memory per PE that holds only the separators reachable
 *  at this stage, i.e. the thresholds of indices (2k+1)*2^(M-1-S)-1 for
 *  k = 0, ..., 2^S-1 of all CF = C/PE channel folds. Such a memory has a depth
 *  of CF*2^S and is addressed by the channel fold concatenated with the S
 *  result bits resolved so far. Altogether, a PE stores CF*(2^M-1) thresholds
 *  without replication. Separators beyond N (if N+1 is not a power of two)
 *  are masked by a scope check rather than stored as saturated values.
 *
 *  The initial memory contents are supplied already split by stage through
 *  a class template INIT<S> specialized for every stage S < M with an inline
 *  accessor returning a function-local static. Unlike a static data member,
 *  such an accessor can be defined in a header shared by several translation
 *  units without C++17 inline variables (a static data member needs exactly
 *  one out-of-class definition, and constexpr is ruled out as the ap_* types
 *  are no literal types):
 *     template<> struct INIT<S> {
 *       static stage_rom<TI, PE, CF<<S> const &data() {
 *         static stage_rom<TI, PE, CF<<S> const  d = { ... };
 *         return  d;
 *       }
 *     };
 *  with data().data[pe][cf*2^S + k] holding threshold (2k+1)*2^(M-1-S)-1 of
 *  channel cf*PE + pe (arbitrary values for out-of-scope separators).
 *
 *  The storage of a stage memory is selected by its depth CF*2^S:
 *   - URAM   if DEPTH_TRIGGER_URAM != 0 and depth >= DEPTH_TRIGGER_URAM,
 *   - BRAM   if DEPTH_TRIGGER_BRAM != 0 and depth >= DEPTH_TRIGGER_BRAM,
 *   - LUTRAM if DEPTH_TRIGGER_BRAM != 0 below that,
 *   - left to the tool otherwise.
 *  All stage memories are read with a latency of one cycle (Vitis HLS rejects
 *  latency=0 for rom_1p on LUTRAM). URAM cannot be initialized on UltraScale+
 *  so that HLS rejects it as ROM (rom_1p + uram). The URAM trigger is reserved
 *  for the variant with runtime-written memories and must remain zero for now.
 *
 *  The static memories are owned by INIT<S>-specific instantiations so that
 *  two kernel instances must use distinct INIT templates.
 *
 *  Interfaces: The kernel runs as a free-running (flp) pipeline. On the
 *  AXI-Stream ports of a top function, the input register slice may be
 *  dropped (register_mode=off); the output register slice must be kept,
 *  since without it TVALID of dst depends combinationally on its TREADY,
 *  which violates AXI-Stream and deadlocks a sink waiting for TVALID.
 *******************************************************************************/
#ifndef THRESHOLDING_HPP
#define THRESHOLDING_HPP

#include <hls_stream.h>
#include <ap_int.h>
#include <cstddef>
#include <type_traits>

#include "utils.hpp"
#include "tensor.hpp"

/**
 * Contents of one stage memory: data[pe][cf*2^S + k].
 * Wrapped in a struct so that a stage memory can be copy-initialized.
 */
template<typename  T, unsigned  PE, unsigned  D>
struct stage_rom {
	T  data[PE][D];
};

namespace thresholding_detail {

	/** Storage implementation of a stage memory. */
	enum class mem_e { AUTO, LUTRAM, BRAM, URAM };

	constexpr mem_e select_mem(unsigned const  depth, unsigned const  trigger_bram, unsigned const  trigger_uram) {
		return
			(trigger_uram != 0) && (depth >= trigger_uram)? mem_e::URAM :
			(trigger_bram != 0) && (depth >= trigger_bram)? mem_e::BRAM :
			(trigger_bram != 0)? mem_e::LUTRAM : mem_e::AUTO;
	}

	/**
	 * Read port of a stage memory: one read per PE from its own memory.
	 *
	 * Specialized by storage implementation as the bind_storage pragma takes
	 * literal options only. The static memory is unique to the instantiating
	 * INIT class and is copy-initialized from INIT::data().
	 */
	template<mem_e  IMPL> struct stage_mem;

	template<> struct stage_mem<mem_e::AUTO> {
		template<typename INIT, unsigned  PE, unsigned  D, typename  T>
		static void read(T (&thr)[PE], unsigned const (&addr)[PE]) {
#pragma HLS inline
			static stage_rom<T, PE, D> const  mem = INIT::data();
#pragma HLS array_partition variable=mem.data complete dim=1
#pragma HLS bind_storage variable=mem.data type=rom_1p
			for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
				thr[pe] = mem.data[pe][addr[pe]];
			}
		}
	};
	template<> struct stage_mem<mem_e::LUTRAM> {
		template<typename INIT, unsigned  PE, unsigned  D, typename  T>
		static void read(T (&thr)[PE], unsigned const (&addr)[PE]) {
#pragma HLS inline
			static stage_rom<T, PE, D> const  mem = INIT::data();
#pragma HLS array_partition variable=mem.data complete dim=1
#pragma HLS bind_storage variable=mem.data type=rom_1p impl=lutram latency=1
			for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
				thr[pe] = mem.data[pe][addr[pe]];
			}
		}
	};
	template<> struct stage_mem<mem_e::BRAM> {
		template<typename INIT, unsigned  PE, unsigned  D, typename  T>
		static void read(T (&thr)[PE], unsigned const (&addr)[PE]) {
#pragma HLS inline
			static stage_rom<T, PE, D> const  mem = INIT::data();
#pragma HLS array_partition variable=mem.data complete dim=1
#pragma HLS bind_storage variable=mem.data type=rom_1p impl=bram latency=1
			for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
				thr[pe] = mem.data[pe][addr[pe]];
			}
		}
	};

	/**
	 * Search stage S of M resolving result bit M-1-S, recursing into the
	 * subsequent stages.
	 */
	template<
		template<unsigned> class  INIT,
		unsigned  N, unsigned  CF, unsigned  PE,
		unsigned  DEPTH_TRIGGER_BRAM, unsigned  DEPTH_TRIGGER_URAM,
		unsigned  S, bool = (S < clog2(N+1))
	>
	struct stage {
		static constexpr unsigned  M  = clog2(N+1);
		static constexpr unsigned  SN = M-1-S;     // Result bit resolved here
		static constexpr unsigned  D  = CF << S;   // Memory depth
		static constexpr unsigned  K  = ((N >> SN) + 1) / 2;	// In-scope separators per channel: k < K

		template<typename  TI, typename  R>
		static void run(R (&res)[PE], tensor<TI, PE> const &x, unsigned const  cnl) {
#pragma HLS inline
			using  init = INIT<S>;
			static_assert(std::is_same<typename std::decay<decltype(init::data())>::type, stage_rom<TI, PE, D>>::value,
				"INIT<S>::data() must return a stage_rom<TI, PE, CF<<S>.");

			unsigned  k[PE];
			unsigned  addr[PE];
#pragma HLS array_partition variable=k complete
#pragma HLS array_partition variable=addr complete
			for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
				k[pe] = unsigned(res[pe]) >> (SN+1);
				addr[pe] = (cnl << S) | k[pe];
			}

			TI  thr[PE];
#pragma HLS array_partition variable=thr complete
			stage_mem<select_mem(D, DEPTH_TRIGGER_BRAM, DEPTH_TRIGGER_URAM)>::template read<init, PE, D>(thr, addr);

			for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
				bool const  scope = (K == (1u << S)) || (k[pe] < K);
				res[pe][SN] = scope && (thr[pe] <= TI(x[pe]));
			}

			stage<INIT, N, CF, PE, DEPTH_TRIGGER_BRAM, DEPTH_TRIGGER_URAM, S+1>::run(res, x, cnl);
		}
	};
	template<
		template<unsigned> class  INIT,
		unsigned  N, unsigned  CF, unsigned  PE,
		unsigned  DEPTH_TRIGGER_BRAM, unsigned  DEPTH_TRIGGER_URAM,
		unsigned  S
	>
	struct stage<INIT, N, CF, PE, DEPTH_TRIGGER_BRAM, DEPTH_TRIGGER_URAM, S, false> {
		template<typename  TI, typename  R>
		static void run(R (&)[PE], tensor<TI, PE> const&, unsigned) {
#pragma HLS inline
		}
	};

} // namespace thresholding_detail

/**
 * Thresholding by binary search.
 *
 * @tparam N	number of thresholds per channel
 * @tparam C	number of channels
 * @tparam INIT	per-stage initial memory contents, see file description
 * @tparam BIAS	output offset: [0, N] -> [BIAS, N+BIAS]
 * @tparam DEPTH_TRIGGER_BRAM	if non-zero, stage memories of this depth or more go into BRAM, smaller ones into LUTRAM
 * @tparam DEPTH_TRIGGER_URAM	if non-zero, stage memories of this depth or more go into URAM (priority)
 * @tparam PE	[inferred] parallel processing elements, must divide C
 * @tparam TI	[inferred] input and threshold type, needs to be <= comparable
 * @tparam TO	[inferred] output type, default as O_BITS of the RTL
 */
template<
	unsigned  N,
	unsigned  C,
	template<unsigned> class  INIT,
	int       BIAS = 0,
	unsigned  DEPTH_TRIGGER_BRAM = 512,
	unsigned  DEPTH_TRIGGER_URAM = 0,
	unsigned  PE,	// [inferred]
	typename  TI,	// [inferred]
	unsigned  WO = BIAS >= 0?
		/* unsigned */ clog2(N+BIAS+1) :
		/* signed */ 1+clog2(-BIAS >= int(N)+BIAS+1? -BIAS : int(N)+BIAS+1),
	typename  TO = typename std::conditional<BIAS >= 0, ap_uint<WO>, ap_int<WO>>::type
>
void thresholding(
	hls::stream<tensor<TI,  PE>> &src,
	hls::stream<tensor<TO, PE>> &dst
) {
#pragma HLS pipeline II=1 style=flp
	static_assert(N > 0, "Need at least one threshold.");
	static_assert(C%PE == 0, "PE must divide channel count.");
	static_assert(DEPTH_TRIGGER_URAM == 0, "URAM cannot hold initialized ROM contents.");

	constexpr unsigned  CF = C/PE;
	constexpr unsigned  M  = clog2(N+1);

	// Channel Fold Rotation
	static ap_uint<CF < 2? 1 : clog2(CF)>  cnl = 0;
#pragma HLS reset variable=cnl

	auto const  x = src.read();

	ap_uint<M>  res[PE];
#pragma HLS array_partition variable=res complete
	for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
		res[pe] = 0;
	}
	thresholding_detail::stage<INIT, N, CF, PE, DEPTH_TRIGGER_BRAM, DEPTH_TRIGGER_URAM, 0>::run(res, x, unsigned(cnl));
	cnl = (cnl == CF-1)? decltype(cnl)(0) : decltype(cnl)(cnl+1);

	tensor<TO, PE>  y;
	for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
		y[pe] = TO(ap_int<M+1>(res[pe]) + BIAS);
	}
	dst.write(y);

} // thresholding()

#endif
