delete_project hls-syn-upsample
open_project hls-syn-upsample
add_files upsample_top.cpp -cflags "-std=c++14 -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb"
add_files -tb upsample_tb.cpp -cflags "-std=c++14 -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb"
set_top Testbench_upsample
open_solution sol1
set_part {xczu3eg-sbva484-1-i}
create_clock -period 5 -name default
csim_design
csynth_design
cosim_design
close_project
exit
