#pragma once
/**
	@file
	@brief subroutine for BlockType array
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/

#include <mie/fp_util.hpp>
#include <mie/gmp_util.hpp>

namespace mie { namespace fp {

template<size_t N>
struct Operation {
	mpz_class p;
	void clear(BlockType *x)
	{
		for (size_t i = 0; i < N; i++) x[i] = 0;
	}
	static inline void toArray(BlockType *y, const mpz_class& x)
	{
		const size_t n = Gmp::getBlockTypeSize(x);
		assert(n <= N);
		Gmp::GetRaw(y, N, x);
		for (size_t i = n; i < N; i++) y[i] = 0;
	}
	static inline void shrC(BlockType *z, const BlockTYpe *x, size_t n)
	{
		assert(n < 32);
		mpz_class t;
		Gmp::setRaw(&t, x, N);
		t >>= n;
		toArray(z, t);
	}
	static inline void addC(BlockType *z, const BlockType *x, const BlockType *y)
	{
		mpz_class tx, ty;
		Gmp::setRaw(&tx, x, N);
		Gmp::setRaw(&ty, y, N);
		tx += ty;
		mpz_class tz = tx - p_;
		if (tz < 0) {
			toArray(z, tx);
		} else {
			toArray(z, tz);
		}
	}
	static inline void subC(BlockType *z, const BlockType *x, const BlockType *y)
	{
		mpz_class tx, ty;
		Gmp::setRaw(&tx, x, N);
		Gmp::setRaw(&ty, y, N);
		tx -= ty;
		if (tx < 0) {
			tx += p_;
		}
		toArray(z, tx);
	}
	static inline void mulC(BlockType *z, const BlockType *x, const BlockType *y)
	{
		mpz_class tx, ty;
		Gmp::setRaw(&tx, x, N);
		Gmp::setRaw(&ty, y, N);
		tx *= ty;
		tx %= p_;
		toArray(z, tx);
	}
	static inline void invC(BlockType *y, const BlockType *x)
	{
		mpz_class t;
		Gmp::setRaw(&t, x, N);
		Gmp::invMod(t, t, p_);
		toArray(y, t);
	}
	static inline void negC(BlockType *y, const BlockType *x)
	{
		mpz_class t;
		Gmp::setRaw(&t, x, N);
		if (t == 0) {
			clear(y);
			return;
		}
		t = p_ - t;
		toArray(y, t);
	}
	static inline void sqrC(BlockType *y, const BlockType *x)
	{
		mpz_class t;
		Gmp::setRaw(&t, x, N);
		t *= t;
		t %= p_;
		toArray(y, t);
	}
	/*
		x[2 * N] mod p -> y[N]
	*/
	static inline void modC(BlockType *y, const BlockType *x)
	{
		mpz_class t;
		Gmp::setRaw(&t, x, N * 2);
		t %= p_;
		toArray(y, t);
	}
};

} } // mie::fp

