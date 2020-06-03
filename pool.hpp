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
 ******************************************************************************/
 
/******************************************************************************
 *
 *  Authors: Giulio Gambardella <giuliog@xilinx.com>
 *           Tobias Alonso <tobiasa@xilinx.com>
 *  \file pool.hpp
 *
 *  This file defines the pool activations
 *
 ******************************************************************************/

#ifndef POOL_HPP
#define POOL_HPP

#include "interpret.hpp"

/**
 * General contract for pool functions.
 *
 * This class itself has no formal significance for the implementation
 * of the MVAU. Implementations of activation functions are encouraged
 * to implement it nonetheless to guarantee appropriate function
 * signatures.
 */
template<typename TA, typename TO, unsigned size>
class PoolFunction {
public:
  TA init(unsigned const  nf, unsigned const  pe) const {
#pragma HLS inline
    return  TA(0);
  }

  /**
   * Compute the activation of the passed accumulator value accu in row idx.
   */
  TA pool(TA const &input, TA const &accu) const;
  TO activate(TA const &accu) const;
};


template<typename T, unsigned size>
class MaxPoolFunction : public PoolFunction<T, T, size> {
public:
  T pool(T const &input, T const &accu) const{
#pragma HLS inline
    return std::max(input,accu);
  }
  
  T activate(T const &accu) const {
#pragma HLS inline
    return  accu;
  }
};


template<typename TA, typename TO, unsigned size>
class AvgPoolFunction : public PoolFunction<TA, TO, size> {
public:
  TA pool(TA const &input, TA const &accu) const{
#pragma HLS inline
    return std::plus<TA>(input,accu);
  }
  
  TO activate(TA const &accu) const {
#pragma HLS inline
    return  (accu/size);
  }
};


template<typename TA, unsigned size>
class AccPoolFunction : public PoolFunction<TA, TA, size> {
public:
  TA pool(TA const &input, TA const &accu) const{
#pragma HLS inline
    return std::plus<TA>(input,accu);
  }
  
  TA activate(TA const &accu) const {
#pragma HLS inline
    return  accu;
  }
};

#endif
