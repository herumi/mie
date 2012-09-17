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
template<size_t tag = 0>
class FpT {
	static mpz_class m_;
	mpz_class x_;
public:
	FpT() {}
	FpT(uint32_t x) : x_(x) {}
	explicit FpT(const std::string& str)
	{
		set(str);
	}
	void set(uint32_t x)
	{
		x_ = x;
	}
	void set(const std::string& str)
	{
		const char *p = str.c_str();
		int base = 10;
		if (str.size() > 2 && str[0] == '0') {
			if (str[1] == 'x') {
				base = 16;
				p += 2;
			} else if (str[1] == 'b') {
				base = 2;
				p += 2;
			}
		}
		if (mpz_init_set_str(x_.get_mpz_t(), p, base) != 0) {
			mpz_clear(x_.get_mpz_t());
			throw std::invalid_argument("mpz_init_set_str");
		}
	}
};

} // mie::gmp

} // mie

