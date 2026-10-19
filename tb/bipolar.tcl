open_project hls-syn-bipolar
add_files bipolar_top.cpp -cflags "-std=c++14 -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb"
add_files -tb bipolar_tb.cpp -cflags "-std=c++14 -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb"
set_top bipolar_top
open_solution sol1
set_part xczu3eg-sbva484-1-i
create_clock -period 5 -name default
csim_design
csynth_design
cosim_design
exit
