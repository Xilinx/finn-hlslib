##
# Copyright (C) 2022, Advanced Micro Devices, Inc. All rights reserved.
# CONFIDENTIAL - for internal use or use under binding NDA only
#
# @brief	Testbench for weight_stream_splitter.
# @author	Thomas B. Preußer <thomas.preusser@amd.com>
##

set dut weight_stream_splitter

# Setup Project
open_project $dut.vitis
add_files -cflags "-std=c++14 -Wall -O2 -I$::env(FINN_HLS_ROOT)" [file join [file dirname [info script]] ${dut}_top.cpp]
add_files -tb -cflags "-std=c++14 -Wall -O2 -I$::env(FINN_HLS_ROOT)" [file join [file dirname [info script]] ${dut}_tb.cpp]

# RTL Synthesis
open_solution sol1
set_part zynquplus
set_top ${dut}_top
create_clock -period 3 -name default
csim_design
csynth_design

quit
