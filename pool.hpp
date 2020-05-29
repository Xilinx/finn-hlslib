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
