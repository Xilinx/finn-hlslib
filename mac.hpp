/******************************************************************************
 *  Copyright (C) Advanced Micro Devices, Inc.
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
 *
 *******************************************************************************
 * @brief	Resource-aware multiply-accumulate helpers.
 * @author	Thomas B. Preusser <thomas.preusser@amd.com>
 *******************************************************************************/

#ifndef MAC_HPP
#define MAC_HPP

#include "utils.hpp"

// Default — no pragma, let Vitis HLS decide
template<typename TAcc, typename TW, typename TI>
void mac(TAcc &acc, TW w, TI a, ap_resource_dflt const&) {
#pragma HLS inline
	acc += w * a;
}

// LUT (fabric) implementation
template<typename TAcc, typename TW, typename TI>
void mac(TAcc &acc, TW w, TI a, ap_resource_lut const&) {
#pragma HLS inline
#pragma HLS BIND_OP variable=acc op=mul impl=fabric
	acc += w * a;
}

// DSP implementation
template<typename TAcc, typename TW, typename TI>
void mac(TAcc &acc, TW w, TI a, ap_resource_dsp const&) {
#pragma HLS inline
#pragma HLS BIND_OP variable=acc op=mul impl=dsp
	acc += w * a;
}

#endif
