open_project hls-syn-kernel_stride_pool
add_files kernel_stride_maxpool_top.cpp -cflags "-fdiagnostics-color -std=c++0x -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb" 
add_files -tb kernel_stride_maxpool_tb.cpp -cflags "-std=c++0x -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb" 
set_top Testbench_kernel_stride_pool
open_solution sol1
set_part {xczu3eg-sbva484-1-i}
create_clock -period 5 -name default
csim_design -clean -compiler clang 
csynth_design
cosim_design
exit
