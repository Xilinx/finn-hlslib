#############################################################################
#  Copyright (c) 2025, Xilinx, Inc.
#  All rights reserved.
#
#  SPDX-License-Identifier: BSD-3-Clause
#############################################################################
set dut layernorm
set part xcv80-lsva4737-2MHP-e-s

open_project hls-syn-${dut}
add_files ${dut}_top.cpp -cflags "-Wall -O2 -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb"
add_files -tb ${dut}_tb.cpp -cflags "-Wall -O2 -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb"
set_top ${dut}_top
open_solution sol1
set_part $part
create_clock -period 2 -name default

csim_design 
csynth_design
cosim_design
exit
