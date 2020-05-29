
#include "kernel_stride_maxpool_top.h"

void Testbench_kernel_stride_pool(stream<ap_uint<FM_Channels1*INPUT_PRECISION> > & in, 
                stream<ap_uint<FM_Channels1*INPUT_PRECISION> > & out, unsigned int numReps){

#pragma HLS DATAFLOW
    const int FOLD = FM_Channels1/PE1;

    hls::stream<ap_uint<FM_Channels1*INPUT_PRECISION> > padded_input("padded_input");
    hls::stream<ap_uint<PE1*INPUT_PRECISION> > aw_padded_input("aw_padded_input");
    hls::stream<ap_uint<PE1*INPUT_PRECISION> > swg_out("swg_out");
    hls::stream<ap_uint<PE1*INPUT_PRECISION> > pool_out("pool_out");


    FMPadding_Batch<IFMDim1,PoolInDim1,PADDING,FM_Channels1,ap_uint<INPUT_PRECISION> >
        (in,padded_input, numReps) ;

    StreamingDataWidthConverter_Batch<FM_Channels1*INPUT_PRECISION, 
                                        PE1*INPUT_PRECISION, (PoolInDim1)*(PoolInDim1)>
            (padded_input, aw_padded_input, numReps);
    
    ConvolutionInputGenerator_kernel_stride_dws<KERNEL_DIM, FM_Channels1,
                            INPUT_PRECISION, PoolInDim1, OFMDim1, PE1,2>
            (aw_padded_input, swg_out, numReps, ap_resource_dflt());

    MaxPoolFunction<ap_uint<INPUT_PRECISION>,KERNEL_DIM> maxpool_fxn;
    Pool_batch<FM_Channels1, PE1, KERNEL_DIM,
                Slice<ap_uint<INPUT_PRECISION> >, Slice< ap_uint<INPUT_PRECISION> > >
    (swg_out,pool_out, maxpool_fxn, OFMDim1*OFMDim1*numReps);

    StreamingDataWidthConverter_Batch<PE1*INPUT_PRECISION, 
                                    FM_Channels1*INPUT_PRECISION, OFMDim1*OFMDim1*FOLD>
            (pool_out, out, numReps);

}
