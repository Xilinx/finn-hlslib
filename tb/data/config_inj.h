#define KERNEL_DIM 2 
#define SIMD1 2 
#define PE1 6 
#define MMV1 1 
#define WIDTH 8 
#define IFM_Channels1 2 
#define OFM_Channels1 12 // OFM_Channels 8 + redundancies 
#define IFMDim1 3 
#define OFMDim1 2 
#define STRIDE 1 
#define INPUT_PRECISION 8 
#define TILE1 8 
#define ACTIVATION_PRECISION 16 
#define REDF 3 
#define NUM_RED 2 
#define MAX_CH_WIDTH 6 // 2^6 = 64 channel indexes 
#define INJ true // Have errors been injected? 
