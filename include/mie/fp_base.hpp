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

struct TagDefault;

#if defined(CYBOZU_OS_BIT) && (CYBOZU_OS_BIT == 32)
typedef uint32_t Unit;
#else
typedef uint64_t Unit;
#endif

// clear
typedef void (*void1op)(Unit*);

// neg, inv
typedef void (*void2op)(Unit*, const Unit*);

// add, sub, mul
typedef void (*void3op)(Unit*, const Unit*, const Unit*);

struct Op {
	size_t N;
	bool (*isZero)(const Unit*);
	bool (*isEqual)(const Unit*, const Unit*);
	int (*compare)(const Unit*, const Unit*);
	void1op clear;
	void2op neg;
	void2op inv;
	void2op copy;
	void3op add;
	void3op sub;
	void3op mul;
	const Unit* p;
};

template<size_t bitN, class tag = TagDefault>
struct FixedFp {
	typedef fp::Unit Unit;
	static const size_t N = (bitN + sizeof(Unit) * 8 - 1) / (sizeof(Unit) * 8);
	static mpz_t mp_;
	static Unit p_[N];
	static inline void setModulo(const Unit* p)
	{
		for (size_t i = 0; i < N; i++) p_[i] = p[i];
		set_mpz_t(mp_, p_);
		assert(sizeof(mp_limb_t) == sizeof(Unit));
	}
	static inline void set_mpz_t(mpz_t& z, const Unit* p)
	{
		z->_mp_alloc = (int)N;
		z->_mp_size = (int)N;
		z->_mp_d = (mp_limb_t*)p;
	}
	static inline void set_zero(mpz_t& z, Unit *p, size_t n)
	{
		z->_mp_alloc = (int)n;
		z->_mp_size = 0;
		z->_mp_d = (mp_limb_t*)p;
	}
	static inline void clear(Unit *x, size_t begin, size_t end)
	{
		for (size_t i = begin; i < end; i++) x[i] = 0;
	}
	static inline void clear(Unit *x)
	{
		for (size_t i = 0; i < N; i++) x[i] = 0;
	}
	static inline void copy(Unit *y, const Unit *x, size_t n)
	{
		for (size_t i = 0; i < n; i++) y[i] = x[i];
	}
	static inline void copy(Unit *y, const Unit *x)
	{
		for (size_t i = 0; i < N; i++) y[i] = x[i];
	}
	static inline void copy(Unit *p, const mpz_t& x)
	{
		const int n = x->_mp_size;
		assert(n >= 0);
		const Unit* xp = (const Unit*)x->_mp_d;
		assert(n <= N);
		copy(p, xp, n);
		clear(p, n, N);
	}
	static inline void add(Unit *z, const Unit *x, const Unit *y)
	{
		Unit ret[N + 2];
		mpz_t mz, mx, my;
		set_zero(mz, ret, N + 2);
		set_mpz_t(mx, x);
		set_mpz_t(my, y);
		mpz_add(mz, mx, my);
		if (mpz_cmp(mz, mp_) >= 0) {
			mpz_sub(mz, mz, mp_);
		}
		copy(z, mz);
	}
	static inline void sub(Unit *z, const Unit *x, const Unit *y)
	{
		Unit ret[N + 2];
		mpz_t mz, mx, my;
		set_zero(mz, ret, N + 2);
		set_mpz_t(mx, x);
		set_mpz_t(my, y);
		mpz_sub(mz, mx, my);
		if (mpz_sgn(mz) < 0) {
			mpz_add(mz, mz, mp_);
		}
		copy(z, mz);
	}
	static inline void mul(Unit *z, const Unit *x, const Unit *y)
	{
		Unit ret[N * 2];
		mpz_t mz, mx, my;
		set_zero(mz, ret, N * 2);
		set_mpz_t(mx, x);
		set_mpz_t(my, y);
		mpz_mul(mz, mx, my);
		mpz_mod(mz, mz, mp_);
		copy(z, mz);
	}
	static inline bool isZero(const Unit *x)
	{
		for (size_t i = 0; i < N; i++) {
			if (x[i]) return false;
		}
		return true;
	}
	static inline bool isEqual(const Unit *x, const Unit *y)
	{
		for (size_t i = 0; i < N; i++) {
			if (x[i] != y[i]) return false;
		}
		return true;
	}
	static inline int compare(const Unit *x, const Unit *y)
	{
		for (size_t i = N - 1; i != size_t(-1); i--) {
			if (x[i] < y[i]) return -1;
			if (x[i] > y[i]) return 1;
		}
		return 0;
	}
	static inline void neg(Unit *y, const Unit *x)
	{
		if (isZero(x)) {
			if (x != y) clear(y, 0, N);
			return;
		}
		sub(y, p_, x);
	}
	static inline void inv(Unit *y, const Unit *x)
	{
		mpz_class my, mx;
		Gmp::setRaw(mx, x, N);
		mpz_invert(my.get_mpz_t(), mx.get_mpz_t(), mp_);
		Gmp::getRaw(y, N, my);
	}
	static inline Op init(const Unit *p)
	{
		setModulo(p);
		Op op;
		op.N = N;
		op.isZero = &isZero;
		op.isEqual = &isEqual;
		op.compare = &compare;
		op.clear = &clear;
		op.neg = &neg;
		op.inv = &inv;
		op.copy = &copy;
		op.add = &add;
		op.sub = &sub;
		op.mul = &mul;
		op.p = &p_[0];
		return op;
	}
};

template<size_t bitN, class tag> mpz_t FixedFp<bitN, tag>::mp_;
template<size_t bitN, class tag> fp::Unit FixedFp<bitN, tag>::p_[FixedFp<bitN, tag>::N];

} } // mie::fp
