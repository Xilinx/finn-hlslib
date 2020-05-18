###############################################################################
 #
 #  Authors: Giulio Gambardella <giuliog@xilinx.com>
 #
 # \file test_swg.tcl
 #
 # Tcl script for HLS csim, synthesis and cosim of the sliding window generator block
 #
###############################################################################
open_project hls-syn-swg
add_files input_gen_cust.cpp -cflags "-std=c++0x -I$::env(FINN_HLS_ROOT)" 
add_files -tb swg_cust_tb.cpp -cflags "-std=c++0x -I$::env(FINN_HLS_ROOT)" 
set_top Testbench
open_solution sol1
set_part {xczu3eg-sbva484-1-i}
create_clock -period 5 -name default
csim_design
csynth_design
cosim_design
exit
