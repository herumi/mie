#pragma once
/**
	@file
	@brief subroutine for Block array
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/

#include <mie/fp_util.hpp>
#include <mie/gmp_util.hpp>

namespace mie { namespace fp {

template<size_t N>
struct GmpOp {
	static mpz_class p;
	void toArray(Block *y, const mpz_class& x)
	{
		const size_t n = Gmp::getBlockSize(x);
		assert(n <= N);
		Gmp::GetRaw(y, N, x);
		for (size_t i = n; i < N; i++) y[i] = 0;
	}
	void addArray(uint64_t *z, const uint64_t *x, const uint64_t *y)
	{
		mpz_class tx, ty, tz;
		Gmp::setRaw(&tx, x, N);
		Gmp::setRaw(&ty, y, N);
		tx += ty;
		tz = tx - p_;
		if (tz < 0) {
			Gmp::GetRaw(:
		}
		if (tx <
		if (tx >= pOrg_) {
			tx -= 
		}
	}
};

} } // mie::fp

