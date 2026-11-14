##############################################################################
# Copyright Advanced Micro Devices, Inc.
# SPDX-License-Identifier: MIT
#
# @author	Thomas B. Preußer <thomas.preusser@amd.com>
###############################################################################
open_project hls-syn-pad
add_files pad_top.cpp -cflags "-std=c++14 -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb"
add_files -tb pad_tb.cpp -cflags "-std=c++14 -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb"
set_top pad_top
open_solution sol1
set_part xczu3eg-sbva484-1-i
create_clock -period 5 -name default
csim_design
csynth_design
cosim_design
exit
