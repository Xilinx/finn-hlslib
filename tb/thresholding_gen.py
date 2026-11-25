#!/usr/bin/env python3
# Copyright (C) 2026, Advanced Micro Devices, Inc.
# All rights reserved.
#
# Generates the configuration headers of the thresholding test bench:
#   thresholding_top.hpp  parameters, types, per-stage memory initializers and
#                         the top function declaration
#   thresholding_ref.h    reference thresholds and stimuli (test bench only)
#
# The test runs in phases p, each with its own thresholds THRESHS[p] of all
# SETS threshold sets (selected randomly per input if SETS > 1): phase 0
# uses the initial memory contents. With --cfg (writable thresholds), phase 1
# writes a full threshold set, phase 2 overwrites the thresholds of every
# other channel, and the test closes with a readback of all addresses.
#
# @author	Thomas B. Preußer <thomas.preusser@amd.com>
import argparse
import math
import os
import zlib

import numpy as np


def clog2(x):
    return 0 if x < 2 else math.ceil(math.log2(x))


def out_bits(n, bias):
    if bias >= 0:
        return clog2(n + bias + 1)
    return 1 + clog2(-bias if -bias >= n + bias + 1 else n + bias + 1)


def raw_range(kind, w):
    """Range of the raw (integer) representation of a type."""
    return (0, 2**w - 1) if kind in ("uint", "ufixed") else (-(2 ** (w - 1)), 2 ** (w - 1) - 1)


def gen_thresholds(rng, ti, tt, c, n, dup=True):
    """Return sorted thresholds as raw TT integers (floats for float types).

    Thresholds are drawn from the intersection of the TI and the TT range. If
    TT can represent max(TI)+1, some channels get never-met thresholds of this
    value at their top end.
    """
    if tt[0] == "float":
        th = np.sort(rng.normal(0.0, 10.0, size=(c, n)).astype(np.float32), axis=-1)
        # Provoke duplicates
        if n > 1:
            th[0, n // 2] = th[0, n // 2 - 1]
        return th
    (ilo, ihi), (tlo, thi) = raw_range(ti[0], ti[1]), raw_range(tt[0], tt[1])
    th = np.sort(rng.integers(max(ilo, tlo), min(ihi, thi) + 1, size=(c, n)), axis=-1)
    if dup and n > 1:
        th[0, n // 2] = th[0, n // 2 - 1]  # duplicate
    if ihi + 1 <= thi:
        for ch in range(1, c, 3):
            th[ch, max(0, n - 1 - ch % 4) :] = ihi + 1  # never met
    return th


def c_list(a, fmt):
    if a.ndim == 1:
        return "{" + ",".join(fmt(x) for x in a) + "}"
    return "{" + ",\n".join(c_list(x, fmt) for x in a) + "}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--n", type=int, required=True)
    ap.add_argument("--c", type=int, required=True)
    ap.add_argument("--pe", type=int, required=True)
    kinds = ("int", "uint", "fixed", "ufixed", "float")
    ap.add_argument("--type", default="int", choices=kinds, help="input type TI")
    ap.add_argument("--w", type=int, default=8, help="input width")
    ap.add_argument("--i", type=int, default=4, help="integer bits of an ap_[u]fixed input")
    ap.add_argument("--tt", default="", choices=("",) + kinds, help="threshold type TT (default: TI)")
    ap.add_argument("--wt", type=int, default=0, help="threshold width (default: input width), same scale as TI")
    ap.add_argument("--bias", type=int, default=0)
    ap.add_argument("--to", default="", help="explicit output type (default: O_BITS formula)")
    ap.add_argument("--trigger-bram", type=int, default=512)
    ap.add_argument("--trigger-uram", type=int, default=0)
    ap.add_argument("--frames", type=int, default=0, help="input frames (default: >= 256 inputs)")
    ap.add_argument("--seed", default="", help="RNG seed string (default derived from parameters)")
    ap.add_argument("--no-dup", action="store_true", help="do not inject duplicate thresholds")
    ap.add_argument("--cfg", action="store_true", help="runtime-writable thresholds with config stream")
    ap.add_argument("--no-init", action="store_true", help="no initial memory contents (requires --cfg)")
    ap.add_argument("--sets", type=int, default=1, help="number of threshold sets")
    ap.add_argument("--out", default=os.path.dirname(os.path.abspath(__file__)))
    a = ap.parse_args()

    n, c, pe, bias = a.n, a.c, a.pe, a.bias
    assert c % pe == 0
    cf = c // pe
    m = clog2(n + 1)
    tt_kind, wt = a.tt or a.type, a.wt or a.w
    assert ("float" not in (a.type, tt_kind)) or (a.type == tt_kind and wt == a.w), "floats cannot be mixed"
    frac = a.w - a.i if a.type in ("fixed", "ufixed") else 0  # common scale of TI and TT
    seed = a.seed or "n%d_c%d_pe%d_%s%d" % (n, c, pe, a.type, a.w)
    if (tt_kind, wt) != (a.type, a.w):
        seed += "_%s%d" % (tt_kind, wt)
    rng = np.random.default_rng(zlib.crc32(seed.encode()))
    assert a.cfg or not a.no_init, "ROM thresholds need initial contents"
    sets = a.sets

    def gen_sets():
        return np.array([gen_thresholds(rng, (a.type, a.w), (tt_kind, wt), c, n, not a.no_dup) for _ in range(sets)])

    th = gen_sets()
    phases = [th]
    if a.cfg:
        phases.append(gen_sets())
        th2 = gen_sets()
        th2[:, 1::2] = phases[1][:, 1::2]
        phases.append(th2)
    if a.no_init:
        phases[0] = np.zeros_like(th)

    def c_type(kind, w):
        return {
            "int": "ap_int<%d>" % w,
            "uint": "ap_uint<%d>" % w,
            "fixed": "ap_fixed<%d, %d>" % (w, w - frac),
            "ufixed": "ap_ufixed<%d, %d>" % (w, w - frac),
            "float": "float",
        }[kind]

    def value(kind):
        """Format a raw value as C literal of the represented value."""
        if kind == "float":
            return lambda v: "%.9ef" % float(v)
        if kind in ("fixed", "ufixed"):
            return lambda v: "%.17g" % (float(v) / 2.0**frac)
        return lambda v: "%d" % int(v)

    ti_t, tt_t = c_type(a.type, a.w), c_type(tt_kind, wt)
    fmt = value(tt_kind)
    wo = out_bits(n, bias)
    to_t = a.to or ("ap_uint<%d>" % wo if bias >= 0 else "ap_int<%d>" % wo)
    frames = a.frames or max(3, -(-256 // cf))
    os.makedirs(a.out, exist_ok=True)

    # Per-stage initializers: stage s, PE p, entry row*2^s + k holds
    # threshold (2k+1)*2^(M-1-s)-1 of channel cf*PE + p of set t in row
    # t*2^clog2(CF) + cf (just cf for a single set), 0 if out of scope.
    rows = sets << clog2(cf) if sets > 1 else cf
    zero = np.zeros((), dtype=th.dtype)
    inits = []
    for s in range(0 if a.no_init else m):
        sn = m - 1 - s
        d = rows << s
        data = np.zeros((pe, d), dtype=th.dtype)
        for t in range(sets):
            for p in range(pe):
                for f_ in range(cf):
                    row = (t << clog2(cf)) + f_ if sets > 1 else f_
                    for k in range(1 << s):
                        idx = ((2 * k + 1) << sn) - 1
                        data[p, (row << s) + k] = th[t, f_ * pe + p, idx] if idx < n else zero
        inits.append(
            "template<> struct thresh_init<%d> {\n"
            "\tstatic stage_rom<TT, PE, %d> const &data() {\n"
            "\t\tstatic stage_rom<TT, PE, %d> const  d = {%s};\n"
            "\t\treturn  d;\n"
            "\t}\n"
            "};\n" % (s, d, d, c_list(data, fmt))
        )

    with open(os.path.join(a.out, "thresholding_top.hpp"), "w") as f:
        f.write(
            """// Generated by thresholding_gen.py
#ifndef THRESHOLDING_TOP_HPP
#define THRESHOLDING_TOP_HPP

#include <hls_stream.h>
#include <ap_int.h>
#include <ap_fixed.h>
#include "tensor.hpp"
#include "thresholding.hpp"

constexpr unsigned  N = %d, C = %d, PE = %d, CF = C/PE, M = %d;
constexpr int       BIAS = %d;
constexpr unsigned  SETS = %d;
constexpr unsigned  DEPTH_TRIGGER_BRAM = %d, DEPTH_TRIGGER_URAM = %d;
constexpr unsigned  NIN = %d;
using TI = %s;
using TT = %s;
using TO = %s;

// Per-stage memory initializers: inline accessors returning a function-local
// static, as this header is included by the kernel and the test bench alike
// and a static data member could only be defined once before C++17.
%s
%s
#define THRESHOLDING_SEL  %d
#define THRESHOLDING_CFG  %d
#if THRESHOLDING_CFG
using cfg_t = thresholding_cfg<N, C, PE, TT, SETS>;
constexpr unsigned  NB = %d, PB = %d, FB = %d, AW = thresholding_addr_bits(N, C, PE, SETS);
#endif

void thresholding_top(
	hls::stream<tensor<TI, PE>> &src,
#if THRESHOLDING_SEL
	hls::stream<thresholding_set_t<SETS>> &sel,
#endif
	hls::stream<tensor<TO, PE>> &dst
#if THRESHOLDING_CFG
	, hls::stream<cfg_t> &cfg,
	hls::stream<TT> &rsp
#endif
);

#endif
"""
            % (n, c, pe, m, bias, sets, a.trigger_bram, a.trigger_uram, frames * cf, ti_t, tt_t, to_t,
               "template<unsigned  S> struct thresh_init%s;" % (" {}" if a.no_init else ""),
               "".join(inits), sets > 1, a.cfg, clog2(n), clog2(pe), clog2(cf))
        )

    # Reference: thresholds and inputs compared as doubles (exact for the tested widths)
    with open(os.path.join(a.out, "thresholding_ref.h"), "w") as f:
        f.write("// Generated by thresholding_gen.py\n#include \"thresholding_top.hpp\"\n#include <cmath>\n#include <random>\n")
        f.write("constexpr unsigned  PHASES = %d;\n" % len(phases))
        f.write("constexpr bool  HAS_INIT = %s;\n" % ("false" if a.no_init else "true"))
        f.write(
            "static double const  THRESHS[PHASES][SETS][C][N] = %s;\n"
            % c_list(np.array(phases), value("int" if tt_kind != "float" else "float"))
        )
        if a.cfg:
            f.write("static TT const  THRESHS_TT[PHASES][SETS][C][N] = %s;\n" % c_list(np.array(phases), fmt))
        if tt_kind in ("fixed", "ufixed"):
            f.write("static double const  THRESH_SCALE = %.17g;\n" % 2.0**-frac)
        else:
            f.write("static double const  THRESH_SCALE = 1.0;\n")
        if a.type == "float":
            f.write(
                "inline double to_double(TI const &v) { return  v; }\n"
                "inline TI rand_input(std::mt19937 &g) { return  TI(std::uniform_real_distribution<float>(-40.f, 40.f)(g)); }\n"
                "inline TI perturb(double v, int d) { return  d == 0? TI(v) : std::nextafter(TI(v), d < 0? -1e30f : 1e30f); }\n"
                "static TI const  EDGES[] = { -3.4e38f, -0.0f, 0.0f, 3.4e38f };\n"
            )
        else:
            lo, hi = raw_range(a.type, a.w)
            tlo, thi = raw_range(tt_kind, wt)
            edges = sorted({max(lo, min(hi, e)) for e in (lo, hi, 0, tlo - 1, tlo, thi, thi + 1)})
            sgn = a.type in ("int", "fixed")
            f.write(
                "inline TI from_raw(long long r) { TI  v; v.range() = ap_uint<%d>(r); return  v; }\n"
                "inline double to_double(TI const &v) { return  double(ap_%sint<%d>(v.range()).%s()) * %.17g; }\n"
                "inline TI rand_input(std::mt19937 &g) { return  from_raw(uint64_t(g()) << 32 | g()); }\n"
                "inline TI perturb(double v, int d) {\n"
                "\tlong long const  r = std::llround(v) + d;\n"
                "\treturn  from_raw(r < %dLL? %dLL : r > %dLL? %dLL : r);\n"
                "}\n"
                "static TI const  EDGES[] = { %s };\n"
                % (
                    a.w,
                    "" if sgn else "u",
                    a.w,
                    "to_int64" if sgn else "to_uint64",
                    2.0**-frac,
                    lo,
                    lo,
                    hi,
                    hi,
                    ", ".join("from_raw(%dLL)" % e for e in edges),
                )
            )
    print("N=%d C=%d PE=%d CF=%d M=%d TI=%s TT=%s TO=%s NIN=%d" % (n, c, pe, cf, m, ti_t, tt_t, to_t, frames * cf))


if __name__ == "__main__":
    main()
