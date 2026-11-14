##############################################################################
# Copyright (c) 2019, Xilinx, Inc.
# Copyright (c) 2025 - 2026, AMD, Inc.
# All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
###############################################################################
# @author	Giulio Gambardella <giuliog@xilinx.com>
# @author	Tobias Alonso <tobiasa@xilinx.com>
# @author	Thomas B. Preußer <thomas.preusser@amd.com>
###############################################################################
open_project hls-syn-dup
add_files dup_top.cpp -cflags "-std=c++14 -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb"
add_files -tb dup_tb.cpp -cflags "-std=c++14 -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb"
set_top dup_top
open_solution sol1
set_part xczu3eg-sbva484-1-i
create_clock -period 5 -name default
csim_design
csynth_design
cosim_design
exit
