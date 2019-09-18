#include <hls_stream.h>
using namespace hls;
#include "ap_int.h"
#include "bnn-library.h"

#define MEM_INTERFACE 64
#define MEM_ENTRIES 16
#define T_SIZE 8
#define NBITS (MEM_INTERFACE * MEM_ENTRIES)

void StreamWidthConverter(ap_uint<MEM_INTERFACE> *din, stream<ap_uint<T_SIZE>> &dout, const unsigned int numReps) {
#pragma HLS INTERFACE m_axi offset=slave port=din bundle=hostmem depth=512
#pragma HLS INTERFACE axis port=&dout bundle=hostmem

// Stream from memory input
    stream<ap_uint<MEM_INTERFACE>> memStrm("test.memStrm");
#pragma HLS STREAM variable=memStrm depth=2
// Generate the stream from the memory array
    Mem2Stream_Batch<MEM_INTERFACE, (NBITS / 8)>(din, memStrm, numReps);
// Adjust the memory stream width to what the MVAU expects (Tile Width)   
    StreamingDataWidthConverter_Batch<MEM_INTERFACE, T_SIZE, MEM_ENTRIES>(memStrm, dout, numReps);
}
