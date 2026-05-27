##############################################################################
# Copyright (C) 2024, Advanced Micro Devices, Inc.
# All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
###############################################################################
# @author	Thomas B. Preußer <thomas.preusser@amd.com>
###############################################################################
open_project hls-syn-flatten
add_files flatten_top.cpp -cflags "-std=c++14 -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb"
add_files -tb flatten_tb.cpp -cflags "-std=c++14 -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb -DHLS_NO_XIL_FPO_LIB"
set_top flatten_top
open_solution sol1
set_part xczu3eg-sbva484-1-i
create_clock -period 5 -name default
csim_design
exit
