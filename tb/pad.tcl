##############################################################################
# Copyright Advanced Micro Devices, Inc.
# SPDX-License-Identifier: MIT
#
# @author	Thomas B. Preußer <thomas.preusser@amd.com>
#
# Runs csim, csynth and cosim of the pad<> testbench for all configurations
# (PAD_CFG, see pad_top.hpp) and both handshake semantics (PAD_EAGER).
# The sets may be restricted via the environment variables PAD_CFGS and
# PAD_EAGERS, e.g., PAD_CFGS="0 2" PAD_EAGERS="0".
###############################################################################
set tbdir [file dirname [file normalize [info script]]]

set cfgs   {0 1 2 3 4}
set eagers {1 0}
if {[info exists ::env(PAD_CFGS)]}   { set cfgs   $::env(PAD_CFGS) }
if {[info exists ::env(PAD_EAGERS)]} { set eagers $::env(PAD_EAGERS) }

foreach cfg $cfgs {
	foreach eager $eagers {
		set flags "-std=c++14 -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb -DPAD_CFG=$cfg -DPAD_EAGER=$eager"
		open_project -reset hls-syn-pad-cfg${cfg}-eager${eager}
		add_files $tbdir/pad_top.cpp -cflags $flags
		add_files -tb $tbdir/pad_tb.cpp -cflags $flags
		set_top pad_top
		open_solution sol1
		set_part xczu3eg-sbva484-1-i
		create_clock -period 5 -name default
		csim_design
		csynth_design
		cosim_design
		close_project
	}
}
exit
