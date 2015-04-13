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
#include <mie/fp_generator.hpp>

#ifndef MIE_FP_BLOCK_MAX_BIT_N
	#define MIE_FP_BLOCK_MAX_BIT_N 521
#endif

namespace mie { namespace fp {

#if defined(CYBOZU_OS_BIT) && (CYBOZU_OS_BIT == 32)
typedef uint32_t Unit;
#else
typedef uint64_t Unit;
#endif

template<class T, size_t bitLen>
struct ElementNumT {
	static const size_t value = (bitLen + sizeof(T) * 8 - 1) / (sizeof(T) * 8);
};
static const size_t maxUnitN = fp::ElementNumT<Unit, MIE_FP_BLOCK_MAX_BIT_N>::value;

typedef void (*void1op)(Unit*);
typedef void (*void2op)(Unit*, const Unit*);
typedef void (*void3op)(Unit*, const Unit*, const Unit*);
typedef void (*void4op)(Unit*, const Unit*, const Unit*, const Unit*);
typedef int (*int2op)(Unit*, const Unit*);
typedef void (*void4Iop)(Unit*, const Unit*, const Unit*, const Unit*, Unit);
typedef void (*void3Iop)(Unit*, const Unit*, const Unit*, Unit);

} } // mie::fp

#ifdef MIE_USE_LLVM

extern "C" {

#define MIE_FP_DEF_FUNC(len) \
void mie_fp_add ## len ## S(mie::fp::Unit*, const mie::fp::Unit*, const mie::fp::Unit*, const mie::fp::Unit*); \
void mie_fp_add ## len ## L(mie::fp::Unit*, const mie::fp::Unit*, const mie::fp::Unit*, const mie::fp::Unit*); \
void mie_fp_sub ## len ## S(mie::fp::Unit*, const mie::fp::Unit*, const mie::fp::Unit*, const mie::fp::Unit*); \
void mie_fp_sub ## len ## L(mie::fp::Unit*, const mie::fp::Unit*, const mie::fp::Unit*, const mie::fp::Unit*); \
void mie_fp_mul ## len ## pre(mie::fp::Unit*, const mie::fp::Unit*, const mie::fp::Unit*); \
void mie_fp_mod ## len(mie::fp::Unit*, const mie::fp::Unit*, const mie::fp::Unit*, mie::fp::Unit); \
void mie_fp_mont ## len(mie::fp::Unit*, const mie::fp::Unit*, const mie::fp::Unit*, const mie::fp::Unit*, mie::fp::Unit);

MIE_FP_DEF_FUNC(128)
MIE_FP_DEF_FUNC(192)
MIE_FP_DEF_FUNC(256)
MIE_FP_DEF_FUNC(320)
MIE_FP_DEF_FUNC(384)
MIE_FP_DEF_FUNC(448)
MIE_FP_DEF_FUNC(512)
#if CYBOZU_OS_BIT == 64
MIE_FP_DEF_FUNC(576)
#else
MIE_FP_DEF_FUNC(160)
MIE_FP_DEF_FUNC(224)
MIE_FP_DEF_FUNC(288)
MIE_FP_DEF_FUNC(352)
MIE_FP_DEF_FUNC(416)
MIE_FP_DEF_FUNC(480)
MIE_FP_DEF_FUNC(544)
#endif
#undef MIE_FP_DEF_FUNC

void mie_fp_mul_NIST_P192(mie::fp::Unit*, const mie::fp::Unit*, const mie::fp::Unit*);

}

#endif

namespace mie { namespace fp {

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

inline bool isZeroArray(const Unit *x, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		if (x[i]) return false;
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
	assert(xn <= (int)yn);
	copyArray(y, xp, xn);
	clearArray(y, xn, yn);
}

} // mie::fp
struct TagDefault;

struct Op {
	mpz_class mp;
	const Unit *p;
	size_t N;
	bool (*isZero)(const Unit*);
	void1op clear;
	void2op neg;
	void2op inv;
	void2op square;
	void2op copy;
	void3op add;
	void3op sub;
	void3op mul;
	// for Montgomery
	void2op toMont;
	void2op fromMont;
	Op()
		: p(0), N(0), isZero(0), clear(0), neg(0), inv(0)
		, square(0), copy(0),add(0), sub(0), mul(0), toMont(0), fromMont(0)
	{
	}
};

template<class tag, size_t bitN>
struct FpBase {
	typedef fp::Unit Unit;
	static const size_t N = fp::ElementNumT<Unit, bitN>::value;
	static const size_t invTblN = N * sizeof(Unit) * 8 * 2;
	static mpz_class mp_;
	static Unit p_[N];
#ifdef MIE_FP_GENERATOR_USE_XBYAK
	// montgomery
	static Unit one_[N];
	static Unit R_[N]; // (1 << (N * 64)) % p
	static Unit RR_[N]; // (R * R) % p
	static Unit invTbl_[invTblN][N];
	static FpGenerator fg_;
#endif
	static void3op mul;

	//////////////////////////////////////////////////////////////////
	// for FixedFp
	static inline void set_mpz_t(mpz_t& z, const Unit* p, int n = (int)N)
	{
		z->_mp_alloc = n;
		int i = n;
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
	static inline void neg(Unit *y, const Unit *x)
	{
		if (isZero(x)) {
			if (x != y) clear(y);
			return;
		}
		sub(y, p_, x);
	}
	// z[N * 2] = x[N] * y[N]
	static inline void mulPre(Unit *z, const Unit *x, const Unit *y)
	{
		mpz_t mx, my, mz;
		set_zero(mz, z, N * 2);
		set_mpz_t(mx, x);
		set_mpz_t(my, y);
		mpz_mul(mz, mx, my);
		local::toArray(z, N * 2, mz);
	}
	// y[N] = x[N * 2] mod p[N]
	static inline void modF(Unit *y, const Unit *x)
	{
		mpz_t mx, my;
		set_mpz_t(mx, x, N * 2);
		set_mpz_t(my, y, N);
		mpz_mod(my, mx, mp_.get_mpz_t());
		local::clearArray(y, my->_mp_size, N);
	}
	// z[N] = x[N] * y[N] mod p[N]
	static inline void mulF(Unit *z, const Unit *x, const Unit *y)
	{
		Unit ret[N * 2];
#if 0
		mulPre(ret, x, y);
		modF(z, ret);
#else
		mpz_t mx, my, mz;
		set_zero(mz, ret, N * 2);
		set_mpz_t(mx, x);
		set_mpz_t(my, y);
		mpz_mul(mz, mx, my);
		mpz_mod(mz, mz, mp_.get_mpz_t());
		local::toArray(z, N, mz);
#endif
	}
#ifdef MIE_USE_LLVM
#define MIE_FP_DEF_METHOD(len, suffix) \
static inline void add ## len(Unit* z, const Unit* x, const Unit* y) { mie_fp_add ## len ## suffix(z, x, y, p_); } \
static inline void sub ## len(Unit* z, const Unit* x, const Unit* y) { mie_fp_sub ## len ## suffix(z, x, y, p_); } \
static inline void mul ## len(Unit* z, const Unit* x, const Unit* y) { Unit ret[N * 2]; mie_fp_mul ## len ## pre(ret, x, y); modF(z, ret); }

#if CYBOZU_OS_BIT == 64
MIE_FP_DEF_METHOD(128, S)
MIE_FP_DEF_METHOD(192, S)
MIE_FP_DEF_METHOD(256, S)
MIE_FP_DEF_METHOD(320, S)
MIE_FP_DEF_METHOD(384, L)
MIE_FP_DEF_METHOD(448, L)
MIE_FP_DEF_METHOD(512, L)
MIE_FP_DEF_METHOD(576, L)
#else
MIE_FP_DEF_METHOD(128, S)
MIE_FP_DEF_METHOD(160, L)
MIE_FP_DEF_METHOD(192, L)
MIE_FP_DEF_METHOD(224, L)
MIE_FP_DEF_METHOD(256, L)
MIE_FP_DEF_METHOD(288, L)
MIE_FP_DEF_METHOD(320, L)
MIE_FP_DEF_METHOD(352, L)
MIE_FP_DEF_METHOD(384, L)
MIE_FP_DEF_METHOD(416, L)
MIE_FP_DEF_METHOD(448, L)
MIE_FP_DEF_METHOD(480, L)
MIE_FP_DEF_METHOD(512, L)
MIE_FP_DEF_METHOD(544, L)
#endif
#undef MIE_FP_DEF_METHOD
#endif
	// y[N] = 1 / x[N] mod p[N]
	static inline void invF(Unit *y, const Unit *x)
	{
		mpz_class my;
		mpz_t mx;
		set_mpz_t(mx, x);
		mpz_invert(my.get_mpz_t(), mx, mp_.get_mpz_t());
		local::toArray(y, N, my.get_mpz_t());
	}
	//////////////////////////////////////////////////////////////////
	// for Montgomery
#ifdef MIE_FP_GENERATOR_USE_XBYAK
	static inline void fromRawGmp(Unit *y, const mpz_class& x)
	{
		local::toArray(y, N, x.get_mpz_t());
	}
	static void initInvTbl(Unit invTbl[invTblN][N])
	{
		void3op _add = Xbyak::CastTo<void3op>(fg_.add_);
		Unit t[N];
		clear(t);
		t[0] = 2;
		toMont(t, t);
		for (int i = 0; i < invTblN; i++) {
			copy(invTbl[invTblN - 1 - i], t);
			_add(t, t, t);
		}
	}
	static inline void invM(Unit *y, const Unit *x)
	{
		const int2op preInv = Xbyak::CastTo<int2op>(fg_.preInv_);
		Unit r[N];
		int k = preInv(r, x);
		/*
			xr = 2^k
			R = 2^(N * 64)
			get r2^(-k)R^2 = r 2^(N * 64 * 2 - k)
		*/
		mul(y, r, invTbl_[k]);
	}
	static inline void toMont(Unit *y, const Unit *x)
	{
		mul(y, x, RR_);
	}
	static inline void fromMont(Unit *y, const Unit *x)
	{
		mul(y, x, one_);
	}
#endif
	// common
	static inline void square(Unit *y, const Unit *x)
	{
		mul(y, x, x);
	}
	static inline void clear(Unit *x)
	{
		local::clearArray(x, 0, N);
	}
	static inline void copy(Unit *y, const Unit *x)
	{
		local::copyArray(y, x, N);
	}
	static inline bool isZero(const Unit *x)
	{
		return local::isZeroArray(x, N);
	}
	static inline void init(Op& op, const Unit *p, bool useMont = true)
	{
		assert(N >= 2);
		assert(sizeof(mp_limb_t) == sizeof(Unit));
		copy(p_, p);
		Gmp::setRaw(mp_, p, N);

		op.N = N;
		op.mp = mp_;
		op.p = &p_[0];

		op.isZero = &isZero;
		op.clear = &clear;
		op.copy = &copy;

#ifdef MIE_FP_GENERATOR_USE_XBYAK
		if (useMont) {
			mpz_class t = 1;
			fromRawGmp(one_, t);
			t = (t << (N * 64)) % mp_;
			fromRawGmp(R_, t);
			t = (t * t) % mp_;
			fromRawGmp(RR_, t);
			fg_.init(p_, N);
			// used by initInvTbl
			mul = Xbyak::CastTo<void3op>(fg_.mul_);

			op.neg = Xbyak::CastTo<void2op>(fg_.neg_);
			op.inv = &invM;
			op.square = Xbyak::CastTo<void2op>(fg_.sqr_);
			if (op.square == 0) op.square = &square;
			op.add = Xbyak::CastTo<void3op>(fg_.add_);
			op.sub = Xbyak::CastTo<void3op>(fg_.sub_);
			op.mul = mul;
			op.toMont = &toMont;
			op.fromMont = &fromMont;

	//		shr1 = Xbyak::CastTo<void2op>(fg_.shr1_);
	//		addNc = Xbyak::CastTo<bool3op>(fg_.addNc_);
	//		subNc = Xbyak::CastTo<bool3op>(fg_.subNc_);
			initInvTbl(invTbl_);
		} else
#endif
		{
			op.neg = &neg;
			op.inv = &invF;
			op.square = &square;
			op.add = &add;
			op.sub = &sub;
			op.mul = &mulF;

#ifdef MIE_USE_LLVM
			const size_t roundN = N * sizeof(Unit) * 8;
			switch (roundN) {
			case 128: op.add = &add128; op.sub = &sub128; op.mul = &mul128; break;
			case 192: op.add = &add192; op.sub = &sub192; op.mul = &mul192; break;
			case 256: op.add = &add256; op.sub = &sub256; op.mul = &mul256; break;
			case 320: op.add = &add320; op.sub = &sub320; op.mul = &mul320; break;
			case 384: op.add = &add384; op.sub = &sub384; op.mul = &mul384; break;
			case 448: op.add = &add448; op.sub = &sub448; op.mul = &mul448; break;
			case 512: op.add = &add512; op.sub = &sub512; op.mul = &mul512; break;
#if CYBOZU_OS_BIT == 64
			case 576: op.add = &add576; op.sub = &sub576; op.mul = &mul576; break;
#else
			case 160: op.add = &add160; op.sub = &sub160; op.mul = &mul160; break;
			case 224: op.add = &add224; op.sub = &sub224; op.mul = &mul224; break;
			case 288: op.add = &add288; op.sub = &sub288; op.mul = &mul288; break;
			case 352: op.add = &add352; op.sub = &sub352; op.mul = &mul352; break;
			case 416: op.add = &add416; op.sub = &sub416; op.mul = &mul416; break;
			case 480: op.add = &add480; op.sub = &sub480; op.mul = &mul480; break;
			case 544: op.add = &add544; op.sub = &sub544; op.mul = &mul544; break;
#endif
			}
			if (mp_ == mpz_class("0xfffffffffffffffffffffffffffffffeffffffffffffffff")) {
				op.mul = &mie_fp_mul_NIST_P192; // slower than MontFp192
			}
#endif
			mul = op.mul;
		}
	}
};
template<class tag, size_t bitN> mpz_class FpBase<tag, bitN>::mp_;
template<class tag, size_t bitN> fp::Unit FpBase<tag, bitN>::p_[FpBase<tag, bitN>::N];
#ifdef MIE_FP_GENERATOR_USE_XBYAK
template<class tag, size_t bitN> fp::Unit FpBase<tag, bitN>::one_[FpBase<tag, bitN>::N];
template<class tag, size_t bitN> fp::Unit FpBase<tag, bitN>::R_[FpBase<tag, bitN>::N];
template<class tag, size_t bitN> fp::Unit FpBase<tag, bitN>::RR_[FpBase<tag, bitN>::N];
template<class tag, size_t bitN> fp::Unit FpBase<tag, bitN>::invTbl_[FpBase<tag, bitN>::invTblN][FpBase<tag, bitN>::N];
template<class tag, size_t bitN> FpGenerator FpBase<tag, bitN>::fg_;
#endif
template<class tag, size_t bitN> void3op FpBase<tag, bitN>::mul;

} } // mie::fp
