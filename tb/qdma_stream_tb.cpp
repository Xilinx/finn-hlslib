#include "ap_int.h"
#include <hls_stream.h>
#include "bnn-library.h"

void qdma_conv_top(hls::stream<qdma_axis<128,0,0,0> > & in, hls::stream<qdma_axis<128,0,0,0> > & out);

int main(){
	hls::stream<qdma_axis<128,0,0,0> > input_stream("in");
	hls::stream<qdma_axis<128,0,0,0> > output_stream("out");
	ap_uint<128> mem[1000];
	qdma_axis<128,0,0,0> tmp;
	
	for(int i=0; i<1000; i++){
		mem[i] = i;
		tmp.set_data(i);
		tmp.set_keep(-1);
		tmp.set_last(i==999);
		input_stream.write(tmp);
	}
	
	qdma_conv_top(input_stream, output_stream);
	
	for(int i=0; i<1000; i++){
		tmp = output_stream.read();
		if(tmp.get_data() != mem[i]){
			return 1;
		}
		if(tmp.get_last() != (i==999)){
			return 2;
		}
	}

	return 0;
}
