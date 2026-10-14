##############################################################################
# Copyright (C) 2024, Advanced Micro Devices, Inc.
# All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
###############################################################################
open_project hls-syn-mvau
add_files mvau_top.cpp -cflags "-std=c++14 -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb"
add_files -tb mvau_tb.cpp -cflags "-std=c++14 -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb"
set_top mvau_top
open_solution sol1
set_part xczu3eg-sbva484-1-i
create_clock -period 5 -name default
csim_design
csynth_design
cosim_design
exit
