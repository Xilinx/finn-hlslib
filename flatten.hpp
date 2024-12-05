/******************************************************************************
 *  Copyright (c) 2024, Christoph Berganski
 *  Copyright (c) 2024, Advanced Micro Devices, Inc.
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
 *******************************************************************************/

#ifndef FLATTEN_HPP
#define FLATTEN_HPP

// HLS arbitrary precision types
#include <ap_int.h>

static ap_int<32> float2apint_cast(float const &arg) {
    union { int32_t  i; float  f; } const  conv = { .f = arg };
    return  (ap_int<32> )conv.i;
}

static ap_int<16> half2apint_cast(half const &arg) {
    union { int16_t  i; half  h; } const  conv = { .h = arg };
    return  (ap_int<16> )conv.i;
}

// Flattens an array of N elements of Type into a single bitvector
template<long unsigned N, class Type>
    ap_uint<N * Type::width> flatten(const Type buffer[N]) {
// Inline this small piece of bit merging logic
#pragma HLS INLINE
        // Fill a flat word of N times the bit-width of the element type
        ap_uint<N * Type::width> flat;
        // Merge all N chunks of the tile into the flat bitvector
        for(unsigned j = 0; j < N; ++j) {
// Do the merging of all chunks in parallel
#pragma HLS UNROLL
            // Insert the chunk into the right place of the
            // bitvector
            flat((j + 1) * Type::width - 1, j * Type::width) = buffer[j];
        }
        // Return the buffer flattened into a single bitvector
        return flat;
    }

// Flattens an array of N elements of float into a single bitvector
template<long unsigned N>
    ap_uint<N * 32> flatten(const float buffer[N]) {
// Inline this small piece of bit merging logic
#pragma HLS INLINE
        // Fill a flat word of N times the bit-width of the element type
        ap_uint<N * 32> flat;
        // Merge all N chunks of the tile into the flat bitvector
        for(unsigned j = 0; j < N; ++j) {
// Do the merging of all chunks in parallel
#pragma HLS UNROLL
            // Insert the chunk into the right place of the
            // bitvector
            flat((j + 1) * 32 - 1, j * 32) = float2apint_cast(buffer[j]);
        }
        // Return the buffer flattened into a single bitvector
        return flat;
    }

// Flattens an array of N elements of half into a single bitvector
template<long unsigned N>
    ap_uint<N * 16> flatten(const half buffer[N]) {
// Inline this small piece of bit merging logic
#pragma HLS INLINE
        // Fill a flat word of N times the bit-width of the element type
        ap_uint<N * 16> flat;
        // Merge all N chunks of the tile into the flat bitvector
        for(unsigned j = 0; j < N; ++j) {
// Do the merging of all chunks in parallel
#pragma HLS UNROLL
            // Insert the chunk into the right place of the
            // bitvector
            flat((j + 1) * 16 - 1, j * 16) = half2apint_cast(buffer[j]);
        }
        // Return the buffer flattened into a single bitvector
        return flat;
    }

#endif // FLATTEN_HPP
