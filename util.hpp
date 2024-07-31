#ifndef UTIL_HPP
#define UTIL_HPP

#include <ostream>
#include <ap_int.h>
#include <hls_vector.h>
#include <hls_stream.h>


//- Compile-Time Functions --------------------------------------------------

// ceil(log2(x))
template<typename T>
constexpr unsigned clog2(T  x) {
  return  x<2? 0 : 1+clog2((x+1)/2);
}

template<typename T>
constexpr unsigned gcd(T  a, T  b) {
	T const  r = a%b;
	return  (r == 0)? b : gcd(b, r);
}

//- Type Traits -------------------------------------------------------------

/**
 * Retrieving the return type from a function (member) pointer type.
 */
template<typename T>
struct return_value {};
template<typename  R, typename... Args>
struct return_value<R(Args...)> {
	using  type = R;
};
template<typename  R, typename... Args>
struct return_value<R(Args...) const> {
	using  type = R;
};
template<typename C, typename  R, typename... Args>
struct return_value<R (C::*)(Args...)> {
	using  type = R;
};
template<typename C, typename  R, typename... Args>
struct return_value<R (C::*)(Args...) const> {
	using  type = R;
};

//- Function Object Templates -----------------------------------------------

/**
 * Functor returning the Idx-th argument it's called with.
 */
template<unsigned  Idx>
struct Proj {
	template<typename  Arg0, typename... Args>
	auto operator()(Arg0 &&arg0, Args&& ...args) const noexcept {
		return  Proj<Idx-1>()(std::forward<Args>(args)...);
	}
};

template<>
struct Proj<0> {
	template<typename  Arg0, typename... Args>
	auto operator()(Arg0 &&arg0, Args&& ...args) const noexcept {
		return  std::forward<Arg0>(arg0);
	}
};


//- Streaming Flit with `last` Marking --------------------------------------
template<typename T>
struct flit_t {
	bool  last;
	T     data;

public:
	flit_t() {}
	flit_t(bool  last_, T const &data_) : last(last_), data(data_) {}
	~flit_t() {}
};

//- hls::vector<> Enablement ------------------------------------------------
template<typename  T, unsigned long  N>
inline std::ostream& operator<<(std::ostream &o, hls::vector<T, N> const &v) {
	char  delim = '{';
	for(auto const &x : v) {
		o << delim << x;
		delim = ':';
	}
	return (o << '}');
}

//- Streaming Copy ----------------------------------------------------------
template<typename T>
void move(hls::stream<T> &src, hls::stream<T> &dst) {
#pragma HLS pipeline II=1 style=flp
	if(!src.empty())  dst.write(src.read());
}

//- Tree Reduce -------------------------------------------------------------
template<
	unsigned long  N,
	typename  TA,
	typename  TR = TA,	// must be assignable from TA
	typename  F			// (TR, TR) -> TR
>
TR tree_reduce(hls::vector<TA, N> const &v, F &&f = F()) {
#pragma HLS inline
#pragma HLS function_instantiate variable=f
	TR  tree[2*N-1];
#pragma HLS array_partition complete dim=1 variable=tree
	for(unsigned  i = N; i-- > 0;) {
#pragma HLS unroll
		tree[N-1 + i] = v[i];
	}
	for(unsigned  i = N-1; i-- > 0;) {
#pragma HLS unroll
		tree[i] = f(tree[2*i+1], tree[2*i+2]);
	}
	return  tree[0];
}

//- Modulus Counter ---------------------------------------------------------

/**
 * Modulus counter returning true upon each N-th call of tick.
 * @description
 *	The implementation internally counts from N-2, ..., 0, -1 wrapping back to
 *	N-2 so that the sign bit of the counter value can directly serve as the
 *	wrap-around indicator without requiring a multi-bit comparator.
 */
template<unsigned  N> class ModCounter {
	ap_int<1+clog2(N-1)>  cnt = N-2;
public:
	bool tick() {
#pragma HLS inline
		bool const  ret = cnt < 0;
		cnt += ret? N-1 : -1;
		return  ret;
	}
};
template<> class ModCounter<1> {
public:
	bool tick() const {
#pragma HLS inline
		return  true;
	}
};
template<> class ModCounter<0> {};

#endif