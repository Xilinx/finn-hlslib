#define SIMD 2
#define NUM_OUTPUTS 3

#define EL_TYPE ap_uint<3>

#define IN_TYPE hls::vector<EL_TYPE, SIMD>

#define NUM_FOLDS0 1
#define NUM_FOLDS1 1
#define NUM_FOLDS2 3
#define FOLDS_PER_OUTPUT {NUM_FOLDS0, NUM_FOLDS1, NUM_FOLDS2}

#define NUM_VECTORS 3