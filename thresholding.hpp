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
 *  Threshold sets: With SETS > 1 independent threshold sets (multi-layer
 *  offload, SETS and iset of the RTL), each input comes with the index of
 *  its set on a set selection stream. The set index extends the memory row
 *  above the channel fold, which is padded to a power of two as in the RTL:
 *  a stage memory then has ROWS = SETS*2^clog2(CF) rows, i.e. a depth of
 *  ROWS*2^S, addressed by set | cf | resolved result bits. For SETS = 1,
 *  ROWS = CF. A set index beyond SETS-1 is undefined.
 *
 *  Thresholds and inputs may have different types TT and TI. The comparison
 *  T_i <= x relies on the exact mixed comparison of C++ between ap_[u]int and
 *  ap_[u]fixed types, which holds in simulation and synthesis alike. The
 *  compiler is expected to choose TT such that every TT value is
 *  representable in TI (TT ⊆ TI), except that TT may be one bit wider than TI
 *  to represent never-met thresholds by the value max(TI)+1. The kernel is
 *  exact in both cases. Floating-point (float, half) types cannot be mixed:
 *  either type being one implies TT == TI.
 *
 *  Narrow thresholds: if TT is narrower than TI, the input is clamped once
 *  before the first stage to the integer bits of TT plus the sign bit of TI,
 *  the counterpart of the KI precision of the RTL. This preserves every
 *  comparison: an input above this range is clamped to its upper end, which
 *  is not below any threshold, and a signed input below it to its lower end,
 *  min(TT)-1 or below, which is below all thresholds. The search pipeline
 *  then compares and registers the narrower clamped value.
 *
 *  The initial memory contents are supplied already split by stage through
 *  a class template INIT<S> specialized for every stage S < M with an inline
 *  accessor returning a function-local static. Unlike a static data member,
 *  such an accessor can be defined in a header shared by several translation
 *  units without C++17 inline variables (a static data member needs exactly
 *  one out-of-class definition, and constexpr is ruled out as the ap_* types
 *  are no literal types):
 *     template<> struct INIT<S> {
 *       static stage_rom<TT, PE, ROWS<<S> const &data() {
 *         static stage_rom<TT, PE, ROWS<<S> const  d = { ... };
 *         return  d;
 *       }
 *     };
 *  with data().data[pe][row*2^S + k] holding threshold (2k+1)*2^(M-1-S)-1 of
 *  channel cf*PE + pe in row = cf, or row = t*2^clog2(CF) + cf for set t if
 *  SETS > 1 (arbitrary values for out-of-scope separators and padding rows).
 *
 *  Runtime-writable thresholds: A kernel variant with a configuration
 *  stream keeps the thresholds in writable stage memories (ram_1p) rather
 *  than ROMs. A configuration operation writes or reads back an individual
 *  threshold, takes precedence over data, as in the RTL, and is passed down
 *  the search pipeline in place of an input. It addresses threshold n of
 *  channel c in set t by the AXI-Lite word address layout of the RTL:
 *     addr = ((t*2^clog2(CF) + c/PE)*2^clog2(PE) + c%PE)*2^clog2(N) + n
 *  i.e. the fields t | cf | pe | n from the MSB, each clog2 of its count
 *  wide (the set field is absent for SETS = 1). The threshold n resides in
 *  stage S = M-1-ctz(n+1) at in-row index k = (n+1) >> (M-S), i.e. in the
 *  stage whose result bit is the lowest zero bit of n. An INIT<S> without a
 *  data() accessor leaves writable memories uninitialized (zero); it still
 *  names the kernel instance owning the memories.
 *
 *  The storage of a stage memory is selected by its depth ROWS*2^S:
 *   - URAM   if DEPTH_TRIGGER_URAM != 0 and depth >= DEPTH_TRIGGER_URAM,
 *   - BRAM   if DEPTH_TRIGGER_BRAM != 0 and depth >= DEPTH_TRIGGER_BRAM,
 *   - LUTRAM if DEPTH_TRIGGER_BRAM != 0 below that,
 *   - left to the tool otherwise.
 *  All stage memories are accessed with a latency of one cycle (Vitis HLS
 *  rejects latency=0 for rom_1p on LUTRAM). URAM cannot be initialized on
 *  UltraScale+: HLS rejects it as ROM (rom_1p + uram) and turns the
 *  initializer of a URAM RAM into runtime writes of every entry, which
 *  ruins the II. URAM is thus only available to writable memories without
 *  initial contents.
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

#include <cstddef>
#include <type_traits>

#include <hls_stream.h>
#include <ap_int.h>
#include <ap_fixed.h>

#include "utils.hpp"
#include "tensor.hpp"

/**
 * Contents of one stage memory: data[pe][row*2^S + k].
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

	/** Floating-point types, which only compare with their own type. */
	template<typename  T>
	struct is_float : std::integral_constant<bool, std::is_floating_point<T>::value || std::is_same<T, half>::value> {};

	/**
	 * Number format of the ap_[u]int and ap_[u]fixed types: W bits, of which
	 * I are integer bits (including the sign), signedness S.
	 */
	template<typename  T> struct num_fmt { static constexpr bool  KNOWN = false; };
	template<int  W_> struct num_fmt<ap_int<W_>> {
		static constexpr bool  KNOWN = true, FIXED = false, S = true;
		static constexpr int   W = W_, I = W_;
	};
	template<int  W_> struct num_fmt<ap_uint<W_>> {
		static constexpr bool  KNOWN = true, FIXED = false, S = false;
		static constexpr int   W = W_, I = W_;
	};
	template<int  W_, int  I_, ap_q_mode  Q, ap_o_mode  O, int  NS> struct num_fmt<ap_fixed<W_, I_, Q, O, NS>> {
		static constexpr bool  KNOWN = true, FIXED = true, S = true;
		static constexpr int   W = W_, I = I_;
	};
	template<int  W_, int  I_, ap_q_mode  Q, ap_o_mode  O, int  NS> struct num_fmt<ap_ufixed<W_, I_, Q, O, NS>> {
		static constexpr bool  KNOWN = true, FIXED = true, S = false;
		static constexpr int   W = W_, I = I_;
	};

	template<bool  FIXED, bool  S, int  W, int  I> struct make_num;
	template<int  W, int  I> struct make_num<false, true,  W, I> { using  type = ap_int<W>; };
	template<int  W, int  I> struct make_num<false, false, W, I> { using  type = ap_uint<W>; };
	template<int  W, int  I> struct make_num<true,  true,  W, I> { using  type = ap_fixed<W, I>; };
	template<int  W, int  I> struct make_num<true,  false, W, I> { using  type = ap_ufixed<W, I>; };

	/**
	 * Comparison type: TI narrowed to the integer bits of TT plus the sign
	 * bit of TI, keeping the fractional bits of TI. TI itself if this is not
	 * narrower or if either type is no ap_[u]int or ap_[u]fixed.
	 */
	template<typename  TI, typename  TT, bool = num_fmt<TI>::KNOWN && num_fmt<TT>::KNOWN>
	struct cmp_type {
		using  type = TI;
	};
	template<typename  TI, typename  TT>
	struct cmp_type<TI, TT, true> {
		using  fi = num_fmt<TI>;
		static constexpr int  I = num_fmt<TT>::I + fi::S;
		static constexpr int  W = I + (fi::W - fi::I);
		using  type = typename std::conditional<(W < fi::W), typename make_num<fi::FIXED, fi::S, W, I>::type, TI>::type;
	};

	/**
	 * Clamp an input to the comparison type TC (see file description), the
	 * counterpart of the input clamp of the RTL.
	 */
	template<typename  TC, typename  TI>
	TC clamp(TI const &x, std::true_type /* TC == TI */) {
#pragma HLS inline
		return  x;
	}
	template<typename  TC, typename  TI>
	TC clamp(TI const &x, std::false_type /* TC narrower than TI */) {
#pragma HLS inline
		constexpr int   WI = num_fmt<TI>::W;
		constexpr int   WC = num_fmt<TC>::W;
		constexpr bool  S  = num_fmt<TI>::S;

		ap_uint<WI> const  raw = x.range(WI-1, 0);
		ap_uint<WI-WC+S> const  top = raw(WI-1, WC-S);	// incl. the sign bit of TC if signed
		bool const  in_range = top.and_reduce()? S : !top.or_reduce();	// sign extension or all zeros
		ap_uint<WC> const  max = S? ap_uint<WC>(ap_uint<WC>(-1) >> 1) : ap_uint<WC>(-1);
		TC  y;
		y.range() = in_range? ap_uint<WC>(raw(WC-1, 0)) : S && raw[WI-1]? ap_uint<WC>(~max) : max;
		return  y;
	}

	/** Whether INIT provides initial contents through a data() accessor. */
	template<typename  INIT, typename = void>
	struct has_data : std::false_type {};
	template<typename  INIT>
	struct has_data<INIT, decltype(void(INIT::data()))> : std::true_type {};

	/**
	 * Initial contents of a stage memory: INIT::data() if provided, zero
	 * otherwise.
	 */
	template<typename  INIT, typename  TT, unsigned  PE, unsigned  D, bool = has_data<INIT>::value>
	struct stage_init {
		static constexpr bool  HAS_DATA = true;
		static_assert(std::is_same<typename std::decay<decltype(INIT::data())>::type, stage_rom<TT, PE, D>>::value,
			"INIT<S>::data() must return a stage_rom<TT, PE, ROWS<<S>.");
		static stage_rom<TT, PE, D> const &data() {
			return  INIT::data();
		}
	};
	template<typename  INIT, typename  TT, unsigned  PE, unsigned  D>
	struct stage_init<INIT, TT, PE, D, false> {
		static constexpr bool  HAS_DATA = false;
		static stage_rom<TT, PE, D> const &data() {
			static stage_rom<TT, PE, D> const  zero = {};
			return  zero;
		}
	};

	/**
	 * Access port of a stage memory: one access per PE to its own memory per
	 * invocation, either a read into thr or, if writable and we[pe], a write
	 * of val.
	 */
	template<typename  T, unsigned  PE, unsigned  D>
	void read(T (&thr)[PE], stage_rom<T, PE, D> const &mem, unsigned const (&addr)[PE]) {
#pragma HLS inline
		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
			thr[pe] = mem.data[pe][addr[pe]];
		}
	}
	template<typename  T, unsigned  PE, unsigned  D>
	void access(
		T (&thr)[PE], stage_rom<T, PE, D> &mem, unsigned const (&addr)[PE],
		bool const (&we)[PE], T const &val
	) {
#pragma HLS inline
		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
			if(we[pe])  mem.data[pe][addr[pe]] = val;
			else        thr[pe] = mem.data[pe][addr[pe]];
		}
	}

	/**
	 * Stage memory, ROM (WR = false) or writable RAM (WR = true).
	 *
	 * Specialized by storage implementation as the bind_storage pragma takes
	 * literal options only. The static memory is unique to the instantiating
	 * INIT class and is copy-initialized from INIT::data(), except for URAM,
	 * which cannot be initialized on UltraScale+.
	 */
	template<mem_e  IMPL, bool  WR> struct stage_mem;

	template<> struct stage_mem<mem_e::AUTO, false> {
		template<typename  INIT, unsigned  PE, unsigned  D, typename  T>
		static void access(T (&thr)[PE], unsigned const (&addr)[PE], bool const (&)[PE], T const&) {
#pragma HLS inline
			static stage_rom<T, PE, D> const  mem = INIT::data();
#pragma HLS array_partition variable=mem.data complete dim=1
#pragma HLS bind_storage variable=mem.data type=rom_1p
			read(thr, mem, addr);
		}
	};
	template<> struct stage_mem<mem_e::LUTRAM, false> {
		template<typename  INIT, unsigned  PE, unsigned  D, typename  T>
		static void access(T (&thr)[PE], unsigned const (&addr)[PE], bool const (&)[PE], T const&) {
#pragma HLS inline
			static stage_rom<T, PE, D> const  mem = INIT::data();
#pragma HLS array_partition variable=mem.data complete dim=1
#pragma HLS bind_storage variable=mem.data type=rom_1p impl=lutram latency=1
			read(thr, mem, addr);
		}
	};
	template<> struct stage_mem<mem_e::BRAM, false> {
		template<typename  INIT, unsigned  PE, unsigned  D, typename  T>
		static void access(T (&thr)[PE], unsigned const (&addr)[PE], bool const (&)[PE], T const&) {
#pragma HLS inline
			static stage_rom<T, PE, D> const  mem = INIT::data();
#pragma HLS array_partition variable=mem.data complete dim=1
#pragma HLS bind_storage variable=mem.data type=rom_1p impl=bram latency=1
			read(thr, mem, addr);
		}
	};
	template<> struct stage_mem<mem_e::AUTO, true> {
		template<typename  INIT, unsigned  PE, unsigned  D, typename  T>
		static void access(T (&thr)[PE], unsigned const (&addr)[PE], bool const (&we)[PE], T const &val) {
#pragma HLS inline
			static stage_rom<T, PE, D>  mem = INIT::data();
#pragma HLS array_partition variable=mem.data complete dim=1
#pragma HLS bind_storage variable=mem.data type=ram_1p
			thresholding_detail::access(thr, mem, addr, we, val);
		}
	};
	template<> struct stage_mem<mem_e::LUTRAM, true> {
		template<typename  INIT, unsigned  PE, unsigned  D, typename  T>
		static void access(T (&thr)[PE], unsigned const (&addr)[PE], bool const (&we)[PE], T const &val) {
#pragma HLS inline
			static stage_rom<T, PE, D>  mem = INIT::data();
#pragma HLS array_partition variable=mem.data complete dim=1
#pragma HLS bind_storage variable=mem.data type=ram_1p impl=lutram latency=1
			thresholding_detail::access(thr, mem, addr, we, val);
		}
	};
	template<> struct stage_mem<mem_e::BRAM, true> {
		template<typename  INIT, unsigned  PE, unsigned  D, typename  T>
		static void access(T (&thr)[PE], unsigned const (&addr)[PE], bool const (&we)[PE], T const &val) {
#pragma HLS inline
			static stage_rom<T, PE, D>  mem = INIT::data();
#pragma HLS array_partition variable=mem.data complete dim=1
#pragma HLS bind_storage variable=mem.data type=ram_1p impl=bram latency=1
			thresholding_detail::access(thr, mem, addr, we, val);
		}
	};
	template<> struct stage_mem<mem_e::URAM, true> {
		template<typename  INIT, unsigned  PE, unsigned  D, typename  T>
		static void access(T (&thr)[PE], unsigned const (&addr)[PE], bool const (&we)[PE], T const &val) {
#pragma HLS inline
			static_assert(!INIT::HAS_DATA, "URAM cannot be initialized: use an INIT without data() or raise DEPTH_TRIGGER_URAM.");
			static stage_rom<T, PE, D>  mem;
#pragma HLS array_partition variable=mem.data complete dim=1
#pragma HLS bind_storage variable=mem.data type=ram_1p impl=uram latency=1
			thresholding_detail::access(thr, mem, addr, we, val);
		}
	};

	/**
	 * Operation passing the search stages: thresholding of an input or, if
	 * cfg, a write or readback of threshold n in memory row row of PE pe.
	 */
	template<typename  TT>
	struct op_t {
		bool      cfg;	// configuration (true) or thresholding (false)
		bool      we;	// cfg: write (true) or readback (false)
		bool      ok;	// cfg: address within scope
		unsigned  row;	// memory row: set and channel fold
		unsigned  pe;	// cfg: PE
		unsigned  n;	// cfg: threshold index
		TT        val;	// cfg: write value
	};

	/**
	 * Search stage S of M resolving result bit M-1-S, recursing into the
	 * subsequent stages. A configuration operation accesses the threshold n
	 * in the stage S = M-1-ctz(n+1) at in-row index k = (n+1) >> (M-S),
	 * i.e. where the low M-S bits of n match 0 1...1.
	 */
	template<
		template<unsigned> class  INIT,
		unsigned  N, unsigned  ROWS, unsigned  PE, bool  WR,
		unsigned  DEPTH_TRIGGER_BRAM, unsigned  DEPTH_TRIGGER_URAM,
		unsigned  S, bool = (S < clog2(N+1))
	>
	struct stage {
		static constexpr unsigned  M  = clog2(N+1);
		static constexpr unsigned  SN = M-1-S;      // Result bit resolved here
		static constexpr unsigned  D  = ROWS << S;  // Memory depth
		static constexpr unsigned  K  = ((N >> SN) + 1) / 2;	// In-scope separators per channel: k < K

		template<typename  TT, typename  TC, typename  R>
		static void run(R (&res)[PE], TC const (&x)[PE], op_t<TT> const &op, TT &q) {
#pragma HLS inline
			bool const  hit = op.cfg && op.ok && (((op.n+1) & ((2u << SN) - 1)) == (1u << SN));

			unsigned  k[PE];
			unsigned  addr[PE];
			bool      we[PE];
#pragma HLS array_partition variable=k complete
#pragma HLS array_partition variable=addr complete
#pragma HLS array_partition variable=we complete
			for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
				k[pe] = unsigned(res[pe]) >> (SN+1);
				addr[pe] = (op.row << S) | k[pe];
				we[pe] = WR && hit && op.we && (op.pe == pe);
			}

			TT  thr[PE];
#pragma HLS array_partition variable=thr complete
			stage_mem<select_mem(D, DEPTH_TRIGGER_BRAM, DEPTH_TRIGGER_URAM), WR>::template access<stage_init<INIT<S>, TT, PE, D>, PE, D>(thr, addr, we, op.val);

			for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
				bool const  scope = (K == (1u << S)) || (k[pe] < K);
				res[pe][SN] = op.cfg? bool(res[pe][SN]) : scope && (thr[pe] <= x[pe]);
			}
			// Readback by indexed selection: a conditional assignment per PE
			// is scheduled into an extra pipeline stage after each memory
			// read (pipeline depth 11 rather than 7 for N=15, C=64, PE=4).
			if(WR && hit)  q = thr[op.pe];

			stage<INIT, N, ROWS, PE, WR, DEPTH_TRIGGER_BRAM, DEPTH_TRIGGER_URAM, S+1>::run(res, x, op, q);
		}
	};
	template<
		template<unsigned> class  INIT,
		unsigned  N, unsigned  ROWS, unsigned  PE, bool  WR,
		unsigned  DEPTH_TRIGGER_BRAM, unsigned  DEPTH_TRIGGER_URAM,
		unsigned  S
	>
	struct stage<INIT, N, ROWS, PE, WR, DEPTH_TRIGGER_BRAM, DEPTH_TRIGGER_URAM, S, false> {
		template<typename  TT, typename  TC, typename  R>
		static void run(R (&)[PE], TC const (&)[PE], op_t<TT> const&, TT&) {
#pragma HLS inline
		}
	};

	/**
	 * Search pipeline shared by the kernel variants: clamps the input,
	 * passes the operation through all stages and returns the biased count.
	 * A readback returns the addressed threshold through q, zero if out of
	 * scope.
	 */
	template<
		unsigned  N, unsigned  ROWS, typename  TT, template<unsigned> class  INIT, int  BIAS,
		bool  WR, unsigned  DEPTH_TRIGGER_BRAM, unsigned  DEPTH_TRIGGER_URAM,
		typename  TO, unsigned  PE, typename  TI
	>
	tensor<TO, PE> search(tensor<TI, PE> const &xi, op_t<TT> const &op, TT &q) {
#pragma HLS inline
		static_assert(
			!(is_float<TI>::value || is_float<TT>::value) || std::is_same<TI, TT>::value,
			"Floating-point inputs and thresholds must have the same type."
		);
		using  TC = typename cmp_type<TI, TT>::type;
		constexpr unsigned  M = clog2(N+1);

		TC  x[PE];
#pragma HLS array_partition variable=x complete
		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
			x[pe] = clamp<TC>(TI(xi[pe]), std::is_same<TC, TI>());
		}

		// Result bits, or the threshold index of a configuration operation,
		// which addresses the stage memories alike (the ptr of the RTL) so
		// that the address path needs no multiplexer.
		ap_uint<M>  res[PE];
#pragma HLS array_partition variable=res complete
		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
			res[pe] = op.cfg? op.n : 0;
		}
		q = TT(0);
		stage<INIT, N, ROWS, PE, WR, DEPTH_TRIGGER_BRAM, DEPTH_TRIGGER_URAM, 0>::run(res, x, op, q);

		tensor<TO, PE>  y;
		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS unroll
			y[pe] = TO(ap_int<M+1>(res[pe]) + BIAS);
		}
		return  y;
	}

} // namespace thresholding_detail

/**
 * Width of a threshold address: set, channel fold, PE and threshold index
 * fields, see file description.
 */
constexpr unsigned thresholding_addr_bits(unsigned const  n, unsigned const  c, unsigned const  pe, unsigned const  sets = 1) {
	return  clog2(sets) + clog2(c/pe) + clog2(pe) + clog2(n);
}

/**
 * Threshold configuration operation: write (we) of val to, or readback
 * (!we) of the threshold at addr. Aggregated with compact=bit on a
 * top-level AXI-Stream port, its TDATA holds from the LSB: val, addr, we.
 */
template<typename  TT, unsigned  AW>
struct thresholding_cfg_t {
	TT           val;
	ap_uint<AW>  addr;
	bool         we;
};
template<unsigned  N, unsigned  C, unsigned  PE, typename  TT, unsigned  SETS = 1>
using thresholding_cfg = thresholding_cfg_t<TT, thresholding_addr_bits(N, C, PE, SETS) < 1? 1 : thresholding_addr_bits(N, C, PE, SETS)>;

/** Set selection: an index < SETS, at least one bit wide (S_BITS of the RTL). */
template<unsigned  SETS>
using thresholding_set_t = ap_uint<(SETS > 2? clog2(SETS) : 1)>;

namespace thresholding_detail {

	/** Number of memory rows per stage and threshold: SETS*2^clog2(CF) or CF. */
	constexpr unsigned rows(unsigned const  cf, unsigned const  sets) {
		return  sets > 1? sets << clog2(cf) : cf;
	}

	/** Memory row of set and channel fold cf: set | cf. */
	template<unsigned  CF, unsigned  SETS>
	unsigned row(unsigned const  set, unsigned const  cf) {
#pragma HLS inline
		constexpr unsigned  FB = clog2(CF);
		return  SETS > 1? (set << FB) | cf : cf;
	}

	/**
	 * Thresholding step of the writable kernel variants: performs the
	 * configuration operation c if cfg_vld, otherwise thresholds x of set if
	 * dat_vld.
	 */
	template<
		unsigned  N, unsigned  CF, unsigned  SETS, typename  TT, template<unsigned> class  INIT, int  BIAS,
		unsigned  DEPTH_TRIGGER_BRAM, unsigned  DEPTH_TRIGGER_URAM,
		typename  TO, unsigned  PE, typename  TI, unsigned  AW, int  WC
	>
	void step(
		bool const  cfg_vld, thresholding_cfg_t<TT, AW> const &c,
		bool const  dat_vld, tensor<TI, PE> const &x, unsigned const  set,
		ap_uint<WC> &cnl,
		hls::stream<tensor<TO, PE>> &dst,
		hls::stream<TT> &rsp
	) {
#pragma HLS inline
		static_assert(thresholding_addr_bits(N, CF*PE, PE, SETS) <= 32, "Threshold address too wide.");
		constexpr unsigned  NB = clog2(N);
		constexpr unsigned  PB = clog2(PE);
		constexpr unsigned  FB = clog2(CF);

		op_t<TT>  op;
		{ // decode configuration address: set | cf | pe | n
			unsigned const  a = c.addr;
			unsigned const  n  = a & ((1u << NB) - 1);
			unsigned const  pe = (a >> NB) & ((1u << PB) - 1);
			unsigned const  cf = (a >> (NB + PB)) & ((1u << FB) - 1);
			unsigned const  s  = a >> (NB + PB + FB);
			op.cfg = cfg_vld;
			op.we  = c.we;
			op.ok  = (n < N) && (pe < PE) && (cf < CF) && (s < SETS);
			op.row = cfg_vld? row<CF, SETS>(s, cf) : row<CF, SETS>(set, cnl);
			op.pe  = pe;
			op.n   = n;
			op.val = c.val;
		}
		TT  q;
		auto const  y = search<N, rows(CF, SETS), TT, INIT, BIAS, true, DEPTH_TRIGGER_BRAM, DEPTH_TRIGGER_URAM, TO>(x, op, q);

		if(dat_vld) {
			dst.write(y);
			cnl = (cnl == CF-1)? ap_uint<WC>(0) : ap_uint<WC>(cnl+1);
		}
		if(cfg_vld && !c.we)  rsp.write(q);
	}

} // namespace thresholding_detail

/**
 * Thresholding by binary search with fixed thresholds in ROMs.
 *
 * Reads one input from src, if available, and writes its result to dst per
 * invocation.
 * The variant with a set selection stream sel additionally reads the index
 * of the threshold set to apply to the input.
 *
 * @tparam N	number of thresholds per channel
 * @tparam C	number of channels
 * @tparam TT	threshold type, see file description for its relation to TI
 * @tparam INIT	per-stage memory contents, see file description
 * @tparam BIAS	output offset: [0, N] -> [BIAS, N+BIAS]
 * @tparam SETS	number of threshold sets, more than one requires sel
 * @tparam DEPTH_TRIGGER_BRAM	if non-zero, stage memories of this depth or more go into BRAM, smaller ones into LUTRAM
 * @tparam DEPTH_TRIGGER_URAM	must be zero as URAM cannot be initialized
 * @tparam PE	[inferred] parallel processing elements, must divide C
 * @tparam TI	[inferred] input type
 * @tparam TO	[inferred] output type, default as O_BITS of the RTL
 */
template<
	unsigned  N,
	unsigned  C,
	typename  TT,
	template<unsigned> class  INIT,
	int       BIAS = 0,
	unsigned  SETS = 1,
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
	hls::stream<tensor<TI, PE>> &src,
	hls::stream<thresholding_set_t<SETS>> &sel,
	hls::stream<tensor<TO, PE>> &dst
) {
#pragma HLS pipeline II=1 style=flp
	static_assert(N > 0, "Need at least one threshold.");
	static_assert(C%PE == 0, "PE must divide channel count.");
	static_assert(SETS > 0, "Need at least one threshold set.");
	static_assert(DEPTH_TRIGGER_URAM == 0, "URAM cannot hold initialized ROM contents.");
	static_assert(thresholding_detail::has_data<INIT<0>>::value, "ROM thresholds need initial contents INIT<S>::data().");
	constexpr unsigned  CF = C/PE;

	// Channel Fold Rotation
	static ap_uint<CF < 2? 1 : clog2(CF)>  cnl = 0;
#pragma HLS reset variable=cnl

	// Non-blocking input, so that a free-running caller may poll
	if(!src.empty() && !sel.empty()) {
		auto const  x = src.read();
		thresholding_detail::op_t<TT>  op = {};
		op.row = thresholding_detail::row<CF, SETS>(sel.read(), cnl);
		TT  q;
		dst.write(thresholding_detail::search<
			N, thresholding_detail::rows(CF, SETS), TT, INIT, BIAS, false, DEPTH_TRIGGER_BRAM, DEPTH_TRIGGER_URAM, TO
		>(x, op, q));
		cnl = (cnl == CF-1)? decltype(cnl)(0) : decltype(cnl)(cnl+1);
	}

} // thresholding()

/** Thresholding with fixed thresholds of a single set, see above. */
template<
	unsigned  N,
	unsigned  C,
	typename  TT,
	template<unsigned> class  INIT,
	int       BIAS = 0,
	unsigned  SETS = 1,
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
	hls::stream<tensor<TI, PE>> &src,
	hls::stream<tensor<TO, PE>> &dst
) {
#pragma HLS pipeline II=1 style=flp
	static_assert(N > 0, "Need at least one threshold.");
	static_assert(C%PE == 0, "PE must divide channel count.");
	static_assert(SETS == 1, "Multiple threshold sets require a set selection stream.");
	static_assert(DEPTH_TRIGGER_URAM == 0, "URAM cannot hold initialized ROM contents.");
	static_assert(thresholding_detail::has_data<INIT<0>>::value, "ROM thresholds need initial contents INIT<S>::data().");
	constexpr unsigned  CF = C/PE;

	// Channel Fold Rotation
	static ap_uint<CF < 2? 1 : clog2(CF)>  cnl = 0;
#pragma HLS reset variable=cnl

	// Non-blocking input, so that a free-running caller may poll
	tensor<TI, PE>  x;
	if(src.read_nb(x)) {
		thresholding_detail::op_t<TT>  op = {};
		op.row = cnl;
		TT  q;
		dst.write(thresholding_detail::search<N, CF, TT, INIT, BIAS, false, DEPTH_TRIGGER_BRAM, DEPTH_TRIGGER_URAM, TO>(x, op, q));
		cnl = (cnl == CF-1)? decltype(cnl)(0) : decltype(cnl)(cnl+1);
	}

} // thresholding()

/**
 * Thresholding by binary search with runtime-writable thresholds.
 *
 * Per invocation, either performs one configuration operation from cfg if
 * available, which takes precedence, or reads one input from src (and its
 * set index from sel in the variant with set selection) and writes its
 * result to dst. A readback writes the addressed threshold to rsp, zero if
 * the address is out of scope (threshold index n >= N, PE, channel fold or
 * set beyond their counts). Writes out of scope are ignored. A threshold
 * write applies to all subsequent inputs.
 *
 * Template parameters as for the ROM variant, except:
 * @tparam INIT	per-stage initial memory contents, see file description;
 * 	an INIT<S> without data() leaves the memories uninitialized (zero)
 * @tparam DEPTH_TRIGGER_URAM	if non-zero, stage memories of this depth or
 * 	more go into URAM (priority), which requires uninitialized memories
 */
template<
	unsigned  N,
	unsigned  C,
	typename  TT,
	template<unsigned> class  INIT,
	int       BIAS = 0,
	unsigned  SETS = 1,
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
	hls::stream<tensor<TI, PE>> &src,
	hls::stream<thresholding_set_t<SETS>> &sel,
	hls::stream<tensor<TO, PE>> &dst,
	hls::stream<thresholding_cfg<N, C, PE, TT, SETS>> &cfg,
	hls::stream<TT> &rsp
) {
#pragma HLS pipeline II=1 style=flp
	static_assert(N > 0, "Need at least one threshold.");
	static_assert(C%PE == 0, "PE must divide channel count.");
	static_assert(SETS > 0, "Need at least one threshold set.");
	constexpr unsigned  CF = C/PE;

	// Channel Fold Rotation
	static ap_uint<CF < 2? 1 : clog2(CF)>  cnl = 0;
#pragma HLS reset variable=cnl

	// Configuration takes precedence over data
	thresholding_cfg<N, C, PE, TT, SETS>  c;
	tensor<TI, PE>  x;
	thresholding_set_t<SETS>  set = 0;
	bool const  cfg_vld = cfg.read_nb(c);
	bool const  dat_vld = !cfg_vld && !src.empty() && !sel.empty();
	if(dat_vld) {
		x = src.read();
		set = sel.read();
	}
	thresholding_detail::step<N, CF, SETS, TT, INIT, BIAS, DEPTH_TRIGGER_BRAM, DEPTH_TRIGGER_URAM>(
		cfg_vld, c, dat_vld, x, set, cnl, dst, rsp
	);

} // thresholding()

/** Thresholding with runtime-writable thresholds of a single set, see above. */
template<
	unsigned  N,
	unsigned  C,
	typename  TT,
	template<unsigned> class  INIT,
	int       BIAS = 0,
	unsigned  SETS = 1,
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
	hls::stream<tensor<TI, PE>> &src,
	hls::stream<tensor<TO, PE>> &dst,
	hls::stream<thresholding_cfg<N, C, PE, TT>> &cfg,
	hls::stream<TT> &rsp
) {
#pragma HLS pipeline II=1 style=flp
	static_assert(N > 0, "Need at least one threshold.");
	static_assert(C%PE == 0, "PE must divide channel count.");
	static_assert(SETS == 1, "Multiple threshold sets require a set selection stream.");
	constexpr unsigned  CF = C/PE;

	// Channel Fold Rotation
	static ap_uint<CF < 2? 1 : clog2(CF)>  cnl = 0;
#pragma HLS reset variable=cnl

	// Configuration takes precedence over data
	thresholding_cfg<N, C, PE, TT>  c;
	tensor<TI, PE>  x;
	bool const  cfg_vld = cfg.read_nb(c);
	bool const  dat_vld = !cfg_vld && src.read_nb(x);
	thresholding_detail::step<N, CF, 1, TT, INIT, BIAS, DEPTH_TRIGGER_BRAM, DEPTH_TRIGGER_URAM>(
		cfg_vld, c, dat_vld, x, 0, cnl, dst, rsp
	);

} // thresholding()

#endif
