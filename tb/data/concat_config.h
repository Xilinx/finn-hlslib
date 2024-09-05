#define SIMD 2
#define NUM_INPUTS 4

#define EL_TYPE0 ap_uint<3>
#define EL_TYPE1 ap_uint<3>
#define EL_TYPE2 ap_uint<3>
#define EL_TYPE3 ap_uint<3>
#define EL_TYPE_OUT ap_uint<3>

#define IN_TYPE0 hls::vector<EL_TYPE0, SIMD>
#define IN_TYPE1 hls::vector<EL_TYPE1, SIMD>
#define IN_TYPE2 hls::vector<EL_TYPE2, SIMD>
#define IN_TYPE3 hls::vector<EL_TYPE3, SIMD>
#define OUT_TYPE hls::vector<EL_TYPE_OUT, SIMD>

#define NUM_FOLDS0 1
#define NUM_FOLDS1 1
#define NUM_FOLDS2 3
#define NUM_FOLDS3 2
#define TOTAL_FOLDS 7

#define NUM_VECTORS 3