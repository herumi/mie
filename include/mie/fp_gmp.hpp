#pragma once
/**
    @file
    @brief Fp by gmp
    @author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://www.opensource.org/licenses/bsd-license.php
*/
#include <mie/gmp_util.hpp>

namespace mie {

namespace gmp {
template<int tag = 0>
class FpT {
	static mpz_class m_;
	mpz_class v;
public:
	FpT() {}
	FpT(uint32_t x) : v(x) {}
	explicit FpT(const std::string& str)
	{
		set(str);
	}
	void set(uint32_t x)
	{
		v = x;
	}
	static inline void setModulo(const std::string& str)
	{
		:x

	}
	static inline void add(FpT& z, const FpT& x, const FpT& y)
	{
		z.v = x.v + y.v;
		z.v -= m_;
	}
};


template<int tag = 0>
mpz_class FpT<tag>::m_;

} // mie::gmp

} // mie

