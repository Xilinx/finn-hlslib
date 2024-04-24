#include <iostream>
#include <cstdlib>
#include <cstring>
#include <hls_stream.h>
#define AP_INT_MAX_W 8191
#include "ap_int.h"
#include "data/dup_stream_config.h"

using namespace hls;
using namespace std;

#define MAX_IMAGES 1
void Testbench_dup_stream(stream<T> & in, stream<T> & out1, stream<T> & out2, unsigned int numReps);

int main()
{
  stream<T> input_stream("input_stream");
  stream<T> output_stream1("output_stream");
  stream<T> output_stream2("output_stream");
  static T expected[NUM_REPEAT*MAX_IMAGES];
  unsigned int count_out = 0;
  unsigned int count_in = 0;
  for (unsigned int counter = 0; counter < NUM_REPEAT*MAX_IMAGES; counter++) {
    T value =  T (counter);
    input_stream.write(value);
    expected[counter] = value;
  }
  Testbench_dup_stream(input_stream, output_stream1, output_stream2, MAX_IMAGES);
  for (unsigned int counter=0 ; counter <  NUM_REPEAT*MAX_IMAGES; counter++)
  {
    T value1 = output_stream1.read();
    T value2 = output_stream2.read();
    if((value1!= expected[counter]) || (value1!= expected[counter]))
    {
      cout << "ERROR with counter " << counter << std::hex << " expected " << expected[counter] << " value1 " << value1 << " value2 " << value2 << std::dec <<  endl;
      return(1);
    }
  }
}

