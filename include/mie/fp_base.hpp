#pragma once
/**
	@file
	@brief basic operation
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4616)
	#pragma warning(disable : 4800)
	#pragma warning(disable : 4244)
	#pragma warning(disable : 4127)
	#pragma warning(disable : 4512)
	#pragma warning(disable : 4146)
#endif
#include <stdint.h>
#include <assert.h>
#include <mie/gmp_util.hpp>
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#include <cybozu/inttype.hpp>

namespace mie { namespace fp {

#if defined(CYBOZU_OS_BIT) && (CYBOZU_OS_BIT == 32)
typedef uint32_t Unit;
#else
typedef uint64_t Unit;
#endif

namespace local {

inline int compareArray(const Unit* x, const Unit* y, size_t n)
{
	for (size_t i = n - 1; i != size_t(-1); i--) {
		if (x[i] < y[i]) return -1;
		if (x[i] > y[i]) return 1;
	}
	return 0;
}

inline bool isEqualArray(const Unit* x, const Unit* y, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		if (x[i] != y[i]) return false;
	}
	return true;
}

inline void clearArray(Unit *x, size_t begin, size_t end)
{
	for (size_t i = begin; i < end; i++) x[i] = 0;
}

inline void copyArray(Unit *y, const Unit *x, size_t n)
{
	for (size_t i = 0; i < n; i++) y[i] = x[i];
}

inline void toArray(Unit *y, size_t yn, const mpz_srcptr x)
{
	const int xn = x->_mp_size;
	assert(xn >= 0);
	const Unit* xp = (const Unit*)x->_mp_d;
	assert(xn <= yn);
	copyArray(y, xp, xn);
	clearArray(y, xn, yn);
}

} // mie::fp
struct TagDefault;

// clear
typedef void (*void1op)(Unit*);

// neg, inv
typedef void (*void2op)(Unit*, const Unit*);

// add, sub, mul
typedef void (*void3op)(Unit*, const Unit*, const Unit*);

struct Op {
	size_t N;
	bool (*isZero)(const Unit*);
	void1op clear;
	void2op neg;
	void2op inv;
	void2op copy;
	void3op add;
	void3op sub;
	void3op mul;
	mpz_class mp;
	const Unit* p;
};

template<size_t bitN, class tag = TagDefault>
struct FixedFp {
	typedef fp::Unit Unit;
	static const size_t N = (bitN + sizeof(Unit) * 8 - 1) / (sizeof(Unit) * 8);
	static mpz_class mp_;
	static Unit p_[N];
	static inline void setModulo(const Unit* p)
	{
		assert(sizeof(mp_limb_t) == sizeof(Unit));
		copy(p_, p);
		Gmp::setRaw(mp_, p, N);
	}
	static inline void set_mpz_t(mpz_t& z, const Unit* p)
	{
		z->_mp_alloc = (int)N;
		int i = int(N);
		while (i > 0 && p[i - 1] == 0) {
			i--;
		}
		z->_mp_size = i;
		z->_mp_d = (mp_limb_t*)p;
	}
	static inline void set_zero(mpz_t& z, Unit *p, size_t n)
	{
		z->_mp_alloc = (int)n;
		z->_mp_size = 0;
		z->_mp_d = (mp_limb_t*)p;
	}
	static inline void clear(Unit *x)
	{
		local::clearArray(x, 0, N);
	}
	static inline void copy(Unit *y, const Unit *x)
	{
		local::copyArray(y, x, N);
	}
	static inline void add(Unit *z, const Unit *x, const Unit *y)
	{
		Unit ret[N + 2]; // not N + 1
		mpz_t mz, mx, my;
		set_zero(mz, ret, N + 2);
		set_mpz_t(mx, x);
		set_mpz_t(my, y);
		mpz_add(mz, mx, my);
		if (mpz_cmp(mz, mp_.get_mpz_t()) >= 0) {
			mpz_sub(mz, mz, mp_.get_mpz_t());
		}
		local::toArray(z, N, mz);
	}
	static inline void sub(Unit *z, const Unit *x, const Unit *y)
	{
		Unit ret[N + 1];
		mpz_t mz, mx, my;
		set_zero(mz, ret, N + 1);
		set_mpz_t(mx, x);
		set_mpz_t(my, y);
		mpz_sub(mz, mx, my);
		if (mpz_sgn(mz) < 0) {
			mpz_add(mz, mz, mp_.get_mpz_t());
		}
		local::toArray(z, N, mz);
	}
	static inline void mul(Unit *z, const Unit *x, const Unit *y)
	{
		Unit ret[N * 2];
		mpz_t mx, my, mz;
		set_zero(mz, ret, N * 2);
		set_mpz_t(mx, x);
		set_mpz_t(my, y);
		mpz_mul(mz, mx, my);
		mpz_mod(mz, mz, mp_.get_mpz_t());
		local::toArray(z, N, mz);
	}
	static inline void inv(Unit *y, const Unit *x)
	{
		mpz_class my;
		mpz_t mx;
		set_mpz_t(mx, x);
		mpz_invert(my.get_mpz_t(), mx, mp_.get_mpz_t());
		local::toArray(y, N, my.get_mpz_t());
	}
	static inline bool isZero(const Unit *x)
	{
		for (size_t i = 0; i < N; i++) {
			if (x[i]) return false;
		}
		return true;
	}
	static inline void neg(Unit *y, const Unit *x)
	{
		if (isZero(x)) {
			if (x != y) clear(y);
			return;
		}
		sub(y, p_, x);
	}
	static inline Op init(const Unit *p)
	{
		setModulo(p);
		Op op;
		op.N = N;
		op.isZero = &isZero;
		op.clear = &clear;
		op.neg = &neg;
		op.inv = &inv;
		op.copy = &copy;
		op.add = &add;
		op.sub = &sub;
		op.mul = &mul;
		op.mp = mp_;
		op.p = &p_[0];
		return op;
	}
};

template<size_t bitN, class tag> mpz_class FixedFp<bitN, tag>::mp_;
template<size_t bitN, class tag> fp::Unit FixedFp<bitN, tag>::p_[FixedFp<bitN, tag>::N];

} } // mie::fp
