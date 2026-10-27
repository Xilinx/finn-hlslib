#include "channelwise_op_top.hpp"

void Testbench_channelwise_op(
	hls::stream<tensor<ap_uint<INPUT_BITS>, PE>> &in,
	hls::stream<tensor<MULT_OUT_TYPE, PE>>       &out
) {
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS pipeline II=1 style=flp

	// Activation parameters
	static ChannelWiseOperation<FOLD, PE, ap_uint<INPUT_BITS>, BIPO_PARAM_TYPE, BIPO_OUT_TYPE,
		per_channel_neg<BIPO_OUT_TYPE>> const  bipolar_params = {.parameters = BIPOLAR_INIT};
	static ChannelWiseOperation<FOLD, PE, BIPO_OUT_TYPE, ADD_PARAM_TYPE, ADD_OUT_TYPE,
		comp::add<ADD_PARAM_TYPE, BIPO_OUT_TYPE, ADD_OUT_TYPE>> const  add_params = {.parameters = ADD_INIT};
	static ChannelWiseOperation<FOLD, PE, ADD_OUT_TYPE, MULT_PARAM_TYPE, MULT_OUT_TYPE,
		comp::mul<MULT_PARAM_TYPE, ADD_OUT_TYPE, MULT_OUT_TYPE>> const  mult_params = {.parameters = MULT_INIT};

	constexpr unsigned  NF = IFM_Channels / PE;
	static unsigned  nf = 0;
#pragma HLS reset variable=nf

	if(!in.empty()) {
		auto const  inElem = in.read();
		tensor<MULT_OUT_TYPE, PE>  y;
		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
			BIPO_OUT_TYPE  bipo = bipolar_params.activate(nf, pe, inElem[pe]);
			ADD_OUT_TYPE   add  = add_params.activate(nf, pe, bipo);
			y[pe] = mult_params.activate(nf, pe, add);
		}
		out.write(y);
		if(++nf == NF)  nf = 0;
	}
}
