open_project hls-syn-checksum
add_files checksum_top.cpp -cflags "-std=c++14 -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb"
add_files -tb checksum_tb.cpp -cflags "-std=c++14 -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb"
set_top Testbench_Checksum
open_solution sol1
set_part {xczu3eg-sbva484-1-i}
create_clock -period 5 -name default
csim_design
csynth_design
# cosim_design is not applicable: chk uses ap_vld (AXI-Lite in production),
# which cosim does not support with ap_ctrl_none. Use checksum_tb.sv for
# RTL validation.
exit
