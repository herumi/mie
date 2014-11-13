#pragma once
/**
	@file
	@brief new Fp(under construction)
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <sstream>
#include <vector>
#include <cybozu/hash.hpp>
#include <cybozu/itoa.hpp>
#include <cybozu/atoi.hpp>
#include <cybozu/bitvector.hpp>
#include <mie/operator.hpp>
#include <mie/power.hpp>
#include <mie/fp_util.hpp>
#include <mie/gmp_util.hpp>
#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4127)
#endif

namespace mie {

namespace fp {

template<class S>
void maskBuffer(S* buf, size_t bufN, size_t bitLen)
{
	size_t unitSize = sizeof(S) * 8;
	if (bitLen >= unitSize * bufN) return;
	const size_t n = getRoundNum<S>(bitLen);
	const size_t rem = bitLen & (unitSize - 1);
	if (rem > 0) buf[n - 1] &= (S(1) << rem) - 1;
}

#if defined(CYBOZU_OS_BIT) && (CYBOZU_OS_BIT == 32)
typedef uint32_t Unit;
#else
typedef uint64_t Unit;
#endif

// add/sub without carry. return true if overflow
typedef bool (*bool3op)(Unit*, const Unit*, const Unit*);

// add/sub with mod
typedef void (*void3op)(Unit*, const Unit*, const Unit*);

// mul without carry. return top of z
typedef Unit (*uint3opI)(Unit*, const Unit*, Unit);

// neg
typedef void (*void2op)(Unit*, const Unit*);

// preInv
typedef int (*int2op)(Unit*, const Unit*);

} // fp

namespace fp_local {

struct TagDefault;

} // fp_local

template<size_t maxBitLen, class tag = fp_local::TagDefault>
class FpT {
public:
	typedef fp::Unit Unit;
	static const size_t UnitSize = sizeof(Unit);
	static const size_t maxN = (maxBitLen + UnitSize * 8 - 1) / (UnitSize * 8);
private:
	static fp::void3op add_;
	static fp::void3op sub_;
	static fp::void3op mul_;
	static fp::void3op div_;
	static fp::void2op inv_;
	static fp::void2op neg_;
	static mpz_class pOrg_;
	static size_t pBitLen_;
	static size_t N_;
	static Unit p_[maxN];

	Unit v_[maxN];

	void fillZero(Unit *x, size_t fromPos) const
	{
		for (size_t i = fromPos; i < N_; i++) x[i] = 0;
	}
	void copy(Unit *y, const Unit *x) const
	{
		for (size_t i = 0; i < N_; i++) y[i] = x[i];
	}
public:
	FpT() {}
	FpT(const FpT& x)
	{
		copy(v_, x.v_);
	}
	FpT& operator=(const FpT& x)
	{
		copy(v_, x.v_);
		return *this;
	}
	void clear()
	{
		fillZero(v_, 0);
	}
	FpT(int x) { operator=(x); }
	explicit FpT(const std::string& str, int base = 0)
	{
		fromStr(str, base);
	}
	FpT& operator=(int x)
	{
		if (x == 0) {
			clear();
		} else {
			v_[0] = abs(x);
			fillZero(v_, 1);
			if (x < 0) sub_(v_, p_, v_);
		}
		return *this;
	}
	void fromStr(const std::string& str, int base = 0)
	{
		bool isMinus;
		mpz_class x;
		inFromStr(x, &isMinus, str, base);
		if (x >= pOrg_) throw cybozu::Exception("fp:FpT:fromStr:large str") << str;
		toArray(v_, x);
		if (isMinus) {
			neg(*this, *this);
		}
	}
	// alias of fromStr
	void set(const std::string& str, int base = 0) { fromStr(str, base); }
	static inline void setModulo(const std::string& mstr, int base = 0)
	{
		bool isMinus;
		inFromStr(pOrg_, &isMinus, mstr, base);
		if (isMinus) throw cybozu::Exception("mie:FpT:setModulo:mstr is not minus") << mstr;
		pBitLen_ = Gmp::getBitLen(pOrg_);
		N_ = Gmp::getRaw(p_, maxN, pOrg_);
		if (N_ == 0) throw cybozu::Exception("mie:FpT:setModulo:bad mstr") << mstr;
	}
	void toStr(std::string& str, int base = 10, bool withPrefix = false) const
	{
		switch (base) {
		case 10:
			T::toStr(str, v_, base);
			return;
		case 16:
			mie::fp::toStr16(str, getBlock(*this), getBlockSize(*this), withPrefix);
			return;
		case 2:
			mie::fp::toStr2(str, getBlock(*this), getBlockSize(*this), withPrefix);
			return;
		default:
			throw cybozu::Exception("fp:FpT:toStr:bad base") << base;
		}
	}
	std::string toStr(int base = 10, bool withPrefix = false) const
	{
		std::string str;
		toStr(str, base, withPrefix);
		return str;
	}
	template<class RG>
	void setRand(RG& rg)
	{
		fp::getRandVal(v_, rg, p_, pBitLen_);
	}
	static inline void add(FpT& z, const FpT& x, const FpT& y) { add_(z.v_, x.v_, y.v_); }
	static inline void sub(FpT& z, const FpT& x, const FpT& y) { sub_(z.v_, x.v_, y.v_); }
	static inline void mul(FpT& z, const FpT& x, const FpT& y) { mul_(z.v_, x.v_, y.v_); }
	static inline void div(FpT& z, const FpT& x, const FpT& y)
	{
		FpT rev;
		inv(rev, y);
		mul(z, x, rev);
	}
	static inline void neg(FpT& z, const FpT& x)
	{
		if (x.isZero()) {
			z.clear();
		} else {
			sub_(z.v_, p_, x.v_);
		}
	}
	static inline void inv(FpT& z, const FpT& x) { inv_(z.v_, x.v_); }

	bool isZero() const
	{
		for (size_t i = 0; i < N_; i++) {
			if (v_[i]) return false;
		}
		return true;
	}
	bool operator==(const FpT& rhs) const
	{
		for (size_t i = 0; i < N_; i++) {
			if (v_[i] != rhs.v_[i]) return false;
		}
		return true;
	}
	bool operator!=(const FpT& rhs) const { return !operator==(rhs); }

	static inline size_t getModBitLen() { return pBitLen_; }
	/*
		append to bv(not clear bv)
	*/
	void appendToBitVec(cybozu::BitVector& bv) const
	{
		const size_t len = bv.size();
		bv.append(v_, N * UnitSize * 8);
		bv.resize(len + pBitLen_); // zero extend if necessary
	}
	void fromBitVec(const cybozu::BitVector& bv)
	{
		if (bv.size() != pBitLen_) throw cybozu::Exception("FpT:fromBitVec:bad size") << bv.size() << pBitLen_;
		T::setRaw(v_, bv.getBlock(), bv.getBlockSize());
		if (v_ >= p_) throw cybozu::Exception("FpT:fromBitVec:large x") << v_ << p_;
	}
	static inline size_t getBitVecSize() { return pBitLen_; }
private:
	static inline void inFromStr(mpz_class& x, bool *isMinus, const std::string& str, int base)
	{
		const char *p = fp::verifyStr(isMinus, &base, str);
		if (!Gmp::fromStr(x, p, base)) {
			throw cybozu::Exception("fp:FpT:inFromStr") << str;
		}
	}
	static inline void toArray(Unit *y, const mpz_class& x)
	{
		const size_t n = Gmp::getBlockTypeSize(x);
		assert(n <= N_);
		Gmp::GetRaw(y, N_, x);
		fillZero(y, n);
	}

#if 0

	template<class T2, class tag2>
	static void power(FpT& z, const FpT& x, const FpT<T2, tag2>& y)
	{
		// QQQ : fix bad detection of class
		if (opt_.hasPowMod()) {
			const bool negative = y < 0;
			const FpT<T2, tag2> yy(negative ? -y : y);
			opt_.powMod(z.v_, x.v_, yy.v_, p_);
			if (negative) {
				inv(z, z);
			}
		} else {
			power_impl::power(z, x, y);
		}
	}
	template<class Int>
	static void power(FpT& z, const FpT& x, const Int& y)
	{
		power_impl::power(z, x, y);
	}
#endif
};

template<size_t maxBiLen, class tag> void fp::void3op FpT<MaxBitLen, tag>::add_;
template<size_t maxBiLen, class tag> void fp::void3op FpT<MaxBitLen, tag>::sub_;
template<size_t maxBiLen, class tag> void fp::void3op FpT<MaxBitLen, tag>::mul_;
template<size_t maxBiLen, class tag> void fp::void3op FpT<MaxBitLen, tag>::div_;

template<size_t maxBiLen, class tag> void fp::void2op FpT<MaxBitLen, tag>::neg_;
template<size_t maxBiLen, class tag> void fp::void2op FpT<MaxBitLen, tag>::inv_;

template<size_t maxBitLen, class tag> mpz_class FpT<maxBitLen, tag>::pOrg_;
template<size_t maxBitLen, class tag> size_t FpT<maxBitLen, tag>::pBitLen_;
template<size_t maxBitLen, class tag> size_t FpT<maxBitLen, tag>::N_;
template<size_t maxBitLen, class tag> uint64_t FpT<maxBitLen, tag>::p_[FpT<maxBitLen, tag>::maxN];

} // mie

namespace std { CYBOZU_NAMESPACE_TR1_BEGIN
template<class T> struct hash;

#if 0
template<size_t maxN, class tag>
struct hash<mie::FpT<maxN, tag> > : public std::unary_function<mie::FpT<maxN, tag>, size_t> {
	size_t operator()(const mie::FpT<maxN, tag>& x, uint64_t v_ = 0) const
	{
		typedef mie::FpT<maxN, tag> Fp;
		size_t n = Fp::getBlockSize(x);
		const typename Fp::uint64_t *p = Fp::getBlock(x);
		return static_cast<size_t>(cybozu::hash64(p, n, v_));
	}
};
#endif

CYBOZU_NAMESPACE_TR1_END } // std::tr1

#ifdef _WIN32
	#pragma warning(pop)
#endif
