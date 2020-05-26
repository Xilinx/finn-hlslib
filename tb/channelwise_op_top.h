#include <hls_stream.h>
using namespace hls;
#include "ap_int.h"
#include "bnn-library.h"

#include "activations.hpp"
#include "weights.hpp"
#include "interpret.hpp"


#define PE 4
#define IFM_Channels 8
#define OFM_Channels IFM_Channels

#define IFMDim 3
#define OFMDim IFMDim

#define BIPO_PARAM_TYPE ap_uint<1>
#define ADD_PARAM_TYPE ap_int<3> 
#define MULT_PARAM_TYPE ap_int<3>
#define BIPOLAR_INIT {{1,1},{1,0},{0,1},{1,1}}
#define ADD_INIT {{ 2, 1},{ 0,-1},{-1,-3},{ 1, 1}} 
#define MULT_INIT {{ 3, 1}, { 2,-1}, {-1, 1}, { 1,-2}} 

#define INPUT_BITS 4
#define BIPO_OUT_BITS  (INPUT_BITS+1)
#define ADD_OUT_BITS  (BIPO_OUT_BITS+1)
#define MULT_OUT_BITS  (ADD_OUT_BITS+2)
#define OUTPUT_BITS MULT_OUT_BITS

#define IN_T ap_uint
#define BIPO_OUT_TYPE  ap_int<BIPO_OUT_BITS>
#define ADD_OUT_TYPE  ap_int<ADD_OUT_BITS>
#define MULT_OUT_TYPE  ap_int<MULT_OUT_BITS>
#define OUT_T ap_int

#define NF (IFM_Channels/PE)




const int bipolar_init[PE][NF] = BIPOLAR_INIT;
const int add_init[PE][NF] = ADD_INIT;
const int mult_init[PE][NF] = MULT_INIT;

template<typename T>
struct per_channel_neg
{
    constexpr T operator()(const ap_uint<1> &lhs, const T &rhs) const {
        return lhs? static_cast<decltype(-rhs)>(rhs):-rhs;
    }
    
};


void Testbench_channelwise_op(stream<ap_uint<IFM_Channels*INPUT_BITS> > & in, 
                    stream<ap_uint<OFM_Channels*OUTPUT_BITS> > & out, unsigned int numReps);
