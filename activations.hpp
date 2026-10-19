/******************************************************************************
 *  Copyright (c) 2019, Xilinx, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1.  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2.  Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *  3.  Neither the name of the copyright holder nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************/

/*******************************************************************************
 *
 *  Authors: Giulio Gambardella <giuliog@xilinx.com>
 *           Thomas B. Preusser <thomas.preusser@utexas.edu>
 *             Marie-Curie Fellow, Xilinx Ireland, Grant Agreement No. 751339
 *           Christoph Doehring <cdoehrin@xilinx.com>
 *
 *  @file activations.hpp
 *
 *  Library of templated HLS classes for BNN deployment. 
 *  This file lists a set of classes used to implement  
 *  threshold memory in neural network. 
 *
 *  This project has received funding from the European Union's Framework
 *  Programme for Research and Innovation Horizon 2020 (2014-2020) under
 *  the Marie Skłodowska-Curie Grant Agreement No. 751339.
 *
 *******************************************************************************/

#ifndef ACTIVATIONS_HPP
#define ACTIVATIONS_HPP

#include "tensor.hpp"
#include <hls_stream.h>
#include <functional>

namespace comp{
  using std::binary_function;

  template<typename input_type_1 = void, typename input_type_2 = void>
    struct greater;

  template<typename input_type_1 = void, typename input_type_2 = void>
    struct less;

  template<typename input_type_1 = void, typename input_type_2 = void>
    struct greater_equal;

  template<typename input_type_1 = void, typename input_type_2 = void>
    struct less_equal;

  template<typename input_type_1 = void, typename input_type_2 = void, typename result_type = void>
    struct add;	

  template<typename input_type_1 = void, typename input_type_2 = void, typename result_type = void>
    struct mul;	

  template<typename input_type_1 = void, typename input_type_2 = void, typename result_type = void>
    struct max;

  template<typename input_type_1, typename input_type_2>
    struct greater : public binary_function<input_type_1, input_type_2, ap_uint<1>> {
      ap_uint<1>
      operator()(const input_type_1& a, const input_type_2& b) const
      { return a > b; }
    };

  template<typename input_type_1, typename input_type_2>
    struct less : public binary_function<input_type_1, input_type_2, ap_uint<1>> {
      ap_uint<1>
      operator()(const input_type_1& a, const input_type_2& b) const
      { return a < b; }
    };

  template<typename input_type_1, typename input_type_2>
    struct greater_equal : public binary_function<input_type_1, input_type_2, ap_uint<1>> {
      ap_uint<1>
      operator()(const input_type_1& a, const input_type_2& b) const
      { return a >= b; }
    };

  template<typename input_type_1, typename input_type_2>
    struct less_equal : public binary_function<input_type_1, input_type_2, ap_uint<1>> {
      ap_uint<1>
      operator()(const input_type_1& a, const input_type_2& b) const
      { return a <= b; }
    };
	
  template<typename input_type_1, typename input_type_2, typename result_type>
    struct add : public binary_function<input_type_1, input_type_2, result_type> {
      result_type
      operator()(const input_type_1& a, const input_type_2& b) const
      { return a + b; }
    };

  template<typename input_type_1, typename input_type_2, typename result_type>
    struct mul : public binary_function<input_type_1, input_type_2, result_type> {
      result_type
      operator()(const input_type_1& a, const input_type_2& b) const
      { return a * b; }
    };

  template<typename input_type_1, typename input_type_2, typename result_type>
    struct max : public binary_function<input_type_1, input_type_2, result_type> {
      result_type
      operator()(const input_type_1& a, const input_type_2& b) const
      { return a > b ? a : b; }
    };
}

/**
 * General contract for activation functions.
 *
 * This class itself has no formal significance for the implementation
 * of the MVAU. Implementations of activation functions are encouraged
 * to implement it nonetheless to guarantee appropriate function
 * signatures.
 */
template<typename TA, typename TO>
class Activation {
public:
  TA init(__attribute__((unused)) unsigned const  nf, __attribute__((unused)) unsigned const  pe) const {
#pragma HLS inline
    return  TA(0);
  }

  /**
   * Compute the activation of the passed accumulator value accu in row idx.
   */
  TO activate(unsigned const  nf, unsigned const  pe, TA const &accu) const;
};

/**
 * A no-op activation that simply outputs the computed accumulator
 * output as the final result.
 */
template<typename T>
class PassThroughActivation : public Activation<T, T> {
public:
  T activate(__attribute__((unused)) unsigned const  nf, __attribute__((unused)) unsigned const  pe, T const &accu) const {
#pragma HLS inline
    return  accu;
  }
};

/**
 * Use a simple global threshold comparison as activation function.
 *
 * The constant threshold is initialized at construction.
 * The default comparison returns true if the threshold value is
 * smaller than the passed accumulator value.
 */
template<typename TA, typename Compare = comp::less<TA, TA>>
class ThresholdActivation : public Activation<TA, bool> {
  TA const  m_threshold;
public:
  ThresholdActivation(TA const &threshold) : m_threshold(threshold) {
#pragma HLS inline
  }

public:
  bool activate(__attribute__((unused)) unsigned const  nf, __attribute__((unused)) unsigned const  pe, TA const &accu) const {
#pragma HLS inline
    return  Compare()(m_threshold, accu);
  }
};

/*!
 * Use a simple per-row threshold comparison as activation function.
 *
 * The thresholds are taken from an array indexed by output row.
 * It is currently public to allow direct initialization and
 * to make its name accessible for top-level HLS pragmas.
 *
 * The default comparison returns true if the threshold value defined for
 * the indexed row is smaller than the passed accumulator value.
 */
template<unsigned NF, unsigned PE, unsigned NumTH, 
	 typename TA, typename TR, int ActVal = 0, typename Compare = comp::less<TA, TA>>
class ThresholdsActivation {
public:
  TA m_thresholds[PE][NF][NumTH];
  
public:
  TA init(__attribute__((unused)) unsigned const  nf, __attribute__((unused)) unsigned const  pe) const {
#pragma HLS inline
    return  TA(0);
  }

public:
  TR activate(unsigned const  nf, unsigned const  pe,  TA const &accu) const {
#pragma HLS inline
    TR result=ActVal;
	for(unsigned int i=0; i< NumTH; i++){
#pragma HLS unroll
      result+=Compare()(m_thresholds[pe][nf][i], accu);
    }
    return result;
  }
};


/*!
 * \brief Use a simple activation function with per-row parameters.
 *
 * The parameters are taken from an array indexed by output row.
 * It is currently public to allow direct initialization and
 * to make its name accessible for top-level HLS pragmas.
 * 
 * \tparam NF    First dimension of the parameter matrix
 * \tparam PE    Second dimension of the parameter matrix
 * \tparam TI    DataType of input layer values
 * \tparam TP    DataType of parameters
 * \tparam TR    DataType of return values
 * \tparam Fxn   Function to be applied on the channel input value
 */

template<unsigned NF, unsigned PE,
   typename TI, typename TP, typename TR, typename Fxn = comp::mul<TI, TP, TR>>
class ChannelWiseOperation {
public:
  TP parameters[PE][NF];
public:
  TI init(__attribute__((unused)) unsigned const  nf, __attribute__((unused)) unsigned const  pe) const {
#pragma HLS inline
    return  TI(0);
  }
public:
  TR activate(unsigned const  nf, unsigned const  pe,  TI const &in) const {
#pragma HLS inline
    TR result = Fxn()(parameters[pe][nf], in);
    return result;
  }
};

/*!
 * \brief Thresholding function (function-pipelined)
 *
 * \tparam NumChannels    Number of channels in input feature map
 * \tparam PE             Number of output rows computed in parallel
 * \tparam TI             Element type of the input activations
 * \tparam TO             Element type of the output activations
 * \tparam TA             Activation class (e.g. thresholds)
 *
 * \param in              Input stream
 * \param out             Output stream
 * \param activation      Activation class
 */
template <
    unsigned NumChannels, unsigned PE,
    typename TI, typename TO, typename TA>
void Thresholding_Batch(hls::stream<tensor<TI, PE>> &in,
                        hls::stream<tensor<TO, PE>> &out,
                        TA const &activation)
{
#pragma HLS pipeline II=1 style=flp
  constexpr unsigned  NF = NumChannels / PE;
  static unsigned  nf = 0;
#pragma HLS reset variable=nf

  if(!in.empty()) {
    auto const  inElem = in.read();
    tensor<TO, PE>  y;
    for (unsigned pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
      y[pe] = activation.activate(nf, pe, inElem[pe]);
    }
    out.write(y);
    if(++nf == NF)  nf = 0;
  }
}

/*!
 * \brief Thresholding function with streaming thresholds (function-pipelined)
 *
 * \tparam NumChannels    Number of channels in input feature map
 * \tparam PE             Number of output rows computed in parallel
 * \tparam ActVal         Initial value of activation at start of thresholding procedure
 * \tparam TT             Element type of the thresholds
 * \tparam NumSteps       Number of thresholds per activation
 * \tparam TI             Element type of the input activations
 * \tparam TO             Element type of the output activations
 *
 * \param in              Input stream
 * \param out             Output stream
 * \param weight          Threshold stream
 */
template <
    unsigned NumChannels, unsigned PE,
    int ActVal=0, typename TT, unsigned int NumSteps,
    typename TI, typename TO>
void Thresholding_Stream_Batch(hls::stream<tensor<TI, PE>> &in,
                        hls::stream<tensor<TO, PE>> &out,
                        hls::stream<tensor<TT, PE, NumSteps>> &weight)
{
#pragma HLS pipeline II=1 style=flp

  if(!in.empty() && !weight.empty()) {
    ThresholdsActivation<1, PE, NumSteps, TT, TO, ActVal, comp::less_equal<TT, TT>> internal_thr;
#pragma HLS ARRAY_PARTITION variable=internal_thr.m_thresholds complete dim=0

    auto const  thr = weight.read();
    auto const  inElem = in.read();
    tensor<TO, PE>  y;

    for (unsigned pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
      auto const  pe_thr = thr[pe];
      for (unsigned nt = 0; nt < NumSteps; nt++) {
#pragma HLS UNROLL
        internal_thr.m_thresholds[pe][0][nt] = pe_thr[nt];
      }
      y[pe] = internal_thr.activate(0, pe, inElem[pe]);
    }
    out.write(y);
  }
}

#endif
