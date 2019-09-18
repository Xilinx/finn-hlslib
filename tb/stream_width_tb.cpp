#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include <hls_stream.h>
#include <cstdlib>
#define AP_INT_MAX_W 16384
#include "ap_int.h"
#include "weights.hpp"
#include "bnn-library.h"
#include "memdata.h"
#include "config.h"
#include "activations.hpp"
#include "weights.hpp"
#include "activations.hpp"
#include "interpret.hpp"
#include "mvau.hpp"
#include "conv.hpp"
using namespace hls;
using namespace std;

#define MEM_INTERFACE 64
#define MEM_ENTRIES 16 // amount of MEM_INTERFACE sized words
#define T_SIZE 8 // Word size / Tile size
#define NBITS (MEM_INTERFACE * MEM_ENTRIES)

void StreamWidthConverter(ap_uint<MEM_INTERFACE> * din, stream<ap_uint<T_SIZE>> &dout, unsigned int numReps);

int main() {
    // Size of the final stream (tile size)
    const unsigned int t_size = T_SIZE; // PE1 * SIMD1 * WIDTH;
    unsigned int nWords = (T_SIZE > MEM_INTERFACE) ?  T_SIZE / MEM_INTERFACE : MEM_INTERFACE / T_SIZE;
    bool result = false; // test pass = false, fail = true
    
    // External memory dummy data container
    ap_uint<MEM_INTERFACE> *extmem_data = new ap_uint<MEM_INTERFACE>[MEM_ENTRIES];

    // initialize data (loads numbers in sequence into the external memory words)
    // IF: Tile Size > Interface: concatenate the tiles in sequence in each external memory word
    if (T_SIZE < MEM_INTERFACE) {     
        unsigned int seq = 0;
        for (unsigned int entry = 0; entry < MEM_ENTRIES; entry++) {
            for (unsigned int word = 0; word < nWords; word++) {
                // read from back to front to counter the reversal of the stream width converter
                unsigned int hi = MEM_INTERFACE - (nWords - word - 1) * T_SIZE - 1;
                unsigned int lo = MEM_INTERFACE - (nWords - word) * T_SIZE;            
                extmem_data[entry](hi, lo) = seq; // note: max seq value depends on tile size
                seq++;
            }        
        }
    }
    // ELSE: Spread the sequence over the individual external memory words (to be concatenated into tiles)
    else {
        unsigned int seq = 0;
        for (unsigned int entry = 0; entry < MEM_ENTRIES; entry += nWords) {
            // nested loop for each N-fold of words to counter the order reversal of the stream width converter
            for (unsigned int word = 0; word < nWords; word++) {
                extmem_data[entry + nWords - word - 1] = seq;
                seq++;
            }            
        }
    }

    // Output MVAU stream
    stream<ap_uint<t_size>> outStrm("InTest.OutputStream");
#pragma HLS STREAM variable=outStrm depth=2

    // Stream the array and adjust the size of the stream
    StreamWidthConverter(extmem_data, outStrm, 1);
    unsigned int counter = 0; // current sequence value
    unsigned int mem_counter = 0; // current memory word
    unsigned int tile_counter = 0; // current tile

    cout << "\nReading out the MVAU stream...\n" << endl;

    while(!outStrm.empty()) {
        cout << "[Tile " << tile_counter << "]" << endl;         
        ap_uint<T_SIZE> retValue = outStrm.read();
        tile_counter++;

        // IF: T_SIZE > MEM_INTERFACE read out the sequence from partitions of the MVAU stream
        if (T_SIZE > MEM_INTERFACE) {
            for (unsigned int word = 0; word < nWords; word++) {
                unsigned int hi = T_SIZE - word * MEM_INTERFACE - 1;
                unsigned int lo = T_SIZE - (word + 1) * MEM_INTERFACE;

                cout << "Expected: " << counter << ", Read: " << retValue(hi, lo) << endl;
                if (counter != retValue(hi, lo)) {                    
                    result = true; // fail if a partition of the streamed word
                    // did not adhere to the sequence
                }
                counter++;     
                cout << "External Memory word " << counter << " checked\n" << endl;

            }
        }
        else {
            cout << "Expected: " << counter << ", Read: " << retValue << endl;
            if (counter != retValue) {                
                result = true; // fail if the streamed words are not in sequence
            }
            counter++;

            if (!(counter % nWords)) {
                cout << "External Memory word " << mem_counter << " checked\n" << endl;
                mem_counter++;
            }  
        }        
    }
    return result;    
}
