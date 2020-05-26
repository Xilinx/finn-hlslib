
#include "channelwise_op_top.h"

// Implements:
// in -> [width adapt] -> [bipolar mult] -> [add]  -> [mult] -> [width adapt] -> out
void Testbench_channelwise_op(stream<ap_uint<IFM_Channels*INPUT_BITS> > & in, 
                    stream<ap_uint<OFM_Channels*OUTPUT_BITS> > & out, unsigned int numReps){
    #pragma HLS DATAFLOW
    
    
    // [width adapt] 
    stream<ap_uint<PE*INPUT_BITS>>  wa_in;
    StreamingDataWidthConverter_Batch<IFM_Channels*INPUT_BITS, PE*INPUT_BITS, IFMDim*IFMDim>(
        in, wa_in, numReps);


    // [bipolar mult]
    ChannelWiseOperation<NF, PE,ap_uint<INPUT_BITS>, BIPO_OUT_TYPE, 
                BIPO_PARAM_TYPE, per_channel_neg<BIPO_OUT_TYPE> > bipolar_params= 
                                        {.parameter = BIPOLAR_INIT};
    
    stream<ap_uint<PE*BIPO_OUT_BITS>>  bipolar_out;
    Thresholding_Batch< IFMDim, IFM_Channels, PE,
        Slice< ap_uint<INPUT_BITS> >, Slice<BIPO_OUT_TYPE> >
        (wa_in, bipolar_out, bipolar_params, numReps);


    // [add] 
    ChannelWiseOperation<NF, PE,BIPO_OUT_TYPE, ADD_OUT_TYPE, 
                ADD_PARAM_TYPE, std::plus<ADD_OUT_TYPE> > add_params = 
                                                    {.parameter = ADD_INIT};
    
    stream<ap_uint<PE*ADD_OUT_BITS>>  add_out;
    Thresholding_Batch< IFMDim, IFM_Channels, PE,
        Slice<BIPO_OUT_TYPE>, Slice<ADD_OUT_TYPE> >
        (bipolar_out, add_out, add_params, numReps);


    // [mult] 
    ChannelWiseOperation<NF, PE,ADD_OUT_TYPE, MULT_OUT_TYPE, 
                MULT_PARAM_TYPE, std::multiplies<MULT_OUT_TYPE> > mult_params= 
                                                        {.parameter = MULT_INIT};
    
    stream<ap_uint<PE*MULT_OUT_BITS>>  mul_out;
    Thresholding_Batch< IFMDim, IFM_Channels, PE,
        Slice<ADD_OUT_TYPE>, Slice<MULT_OUT_TYPE> >
        (add_out, mul_out, mult_params, numReps);

    // [width adapt]
    StreamingDataWidthConverter_Batch<PE*MULT_OUT_BITS, OFM_Channels*MULT_OUT_BITS, 
        IFMDim*IFMDim*NF >(mul_out, out, numReps);
}
