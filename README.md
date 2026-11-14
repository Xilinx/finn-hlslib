# FINN
FINN is an ML framework by the Integrated Communications and AI Lab of AMD Research & Advanced Development.
It provides an end-to-end flow for the exploration and implementation of quantized neural network inference solutions on FPGAs.
FINN generates dataflow architectures as a physical representation of the implemented custom network in space.
It is not a generic DNN acceleration solution but relies on co-design and design space exploration for quantization and parallelization tuning so as to optimize a solutions with respect to resource and performance requirements.

# FINN-hlslib
This repository is the collection of the templated C++ HLS models of various neural network operators that the FINN compiler instantiates into its generated neural network inference solution.
The library is built to synthesize using the Vitis HLS tool.
