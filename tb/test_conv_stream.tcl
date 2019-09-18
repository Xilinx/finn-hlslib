open_project hls-syn-conv
add_files cnvlayer_unit.cpp -cflags "-std=c++0x -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb" 
add_files gen_unit.cpp -cflags "-std=c++0x -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb" 
add_files -tb conv_stream_tb.cpp -cflags "-std=c++0x -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb" 
#set_top Testbench_conv
set_top ConvLayer
open_solution sol1
set_part {xczu3eg-sbva484-1-i}
create_clock -period 5 -name default
csim_design
csynth_design
cosim_design
exit
