
#include <hls_stream.h>
using namespace hls;
#include "ap_int.h"
#include "bnn-library.h"
#include "pool.hpp"

#define KERNEL_DIM 3 
#define FM_Channels1 16
#define IFMDim1 16
#define PADDING 2
#define PoolInDim1 (IFMDim1+PADDING)
#define STRIDE 2
#define OFMDim1  (IFMDim1/STRIDE) //Using padding
#define INPUT_PRECISION 4
#define PE1 4

void Testbench_kernel_stride_pool(stream<ap_uint<FM_Channels1*INPUT_PRECISION> > & in, 
                stream<ap_uint<FM_Channels1*INPUT_PRECISION> > & out, unsigned int numReps);
