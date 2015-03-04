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
#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4127)
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
#endif
#include <cybozu/hash.hpp>
#include <cybozu/itoa.hpp>
#include <cybozu/atoi.hpp>
#include <cybozu/bitvector.hpp>
#include <mie/operator.hpp>
#include <mie/power.hpp>
#include <mie/fp_base.hpp>
#include <mie/fp_util.hpp>
#include <mie/gmp_util.hpp>

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

} // fp

template<size_t maxBitN, class tag = fp::TagDefault>
class FpT {
public:
	typedef fp::Unit Unit;
	static const size_t UnitByteN = sizeof(Unit);
	static const size_t maxUnitN = (maxBitN + UnitByteN * 8 - 1) / (UnitByteN * 8);
private:
	static fp::Op op_;
	static mpz_class mp_;
	static size_t pBitLen_;
	static size_t N_;

public:
	Unit v_[maxUnitN];
	static inline void setModulo(const std::string& mstr, int base = 0)
	{
		bool isMinus;
		inFromStr(mp_, &isMinus, mstr, base);
		if (isMinus) throw cybozu::Exception("mie:FpT:setModulo:mstr is not minus") << mstr;
		pBitLen_ = Gmp::getBitLen(mp_);
		if (pBitLen_ > maxBitN) throw cybozu::Exception("mie:FpT:setModulo:too large bitLen") << pBitLen_ << maxBitN;
		Unit p[maxUnitN];
		N_ = Gmp::getRaw(p, maxUnitN, mp_);
		if (N_ == 0) throw cybozu::Exception("mie:FpT:setModulo:bad mstr") << mstr;
		if (pBitLen_ <= 128) {  op_ = fp::FixedFp<128, tag>::init(p); }
		else if (pBitLen_ <= 192) { static fp::FixedFp<192, tag> fixed; op_ = fixed.init(p); }
		else if (pBitLen_ <= 256) { static fp::FixedFp<256, tag> fixed; op_ = fixed.init(p); }
		else if (pBitLen_ <= 384) { static fp::FixedFp<384, tag> fixed; op_ = fixed.init(p); }
		else if (pBitLen_ <= 448) { static fp::FixedFp<448, tag> fixed; op_ = fixed.init(p); }
		else if (pBitLen_ <= 576) { static fp::FixedFp<576, tag> fixed; op_ = fixed.init(p); }
		else { static fp::FixedFp<maxBitN, tag> fixed; op_ = fixed.init(p); }
	}
	FpT() {}
	FpT(const FpT& x)
	{
		op_.copy(v_, x.v_);
	}
	FpT& operator=(const FpT& x)
	{
		op_.copy(v_, x.v_);
		return *this;
	}
	void clear()
	{
		op_.clear(v_);
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
			op_.clear(v_);
			v_[0] = abs(x);
			if (x < 0) neg(*this, *this);
		}
		return *this;
	}
	void fromStr(const std::string& str, int base = 0)
	{
		bool isMinus;
		mpz_class x;
		inFromStr(x, &isMinus, str, base);
		if (x >= mp_) throw cybozu::Exception("fp:FpT:fromStr:large str") << str;
		toArray(v_, x);
		if (isMinus) {
			neg(*this, *this);
		}
	}
	// alias of fromStr
	void set(const std::string& str, int base = 0) { fromStr(str, base); }
	void toStr(std::string& str, int base = 10, bool withPrefix = false) const
	{
		switch (base) {
		case 10:
			{
				mpz_class x;
				Gmp::setRaw(x, v_, N_);
				Gmp::toStr(str, x, 10);
			}
			return;
		case 16:
			mie::fp::toStr16(str, v_, N_, withPrefix);
			return;
		case 2:
			mie::fp::toStr2(str, v_, N_, withPrefix);
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
		fp::getRandVal(v_, rg, op_.p, pBitLen_);
	}
	static inline void add(FpT& z, const FpT& x, const FpT& y) { op_.add(z.v_, x.v_, y.v_); }
	static inline void sub(FpT& z, const FpT& x, const FpT& y) { op_.sub(z.v_, x.v_, y.v_); }
	static inline void mul(FpT& z, const FpT& x, const FpT& y) { op_.mul(z.v_, x.v_, y.v_); }
	static inline void inv(FpT& y, const FpT& x) { op_.inv(y.v_, x.v_); }
	static inline void neg(FpT& y, const FpT& x) { op_.neg(y.v_, x.v_); }
	static inline void square(FpT& y, const FpT& x) { op_.mul(y.v_, x.v_, x.v_); }
	static inline void div(FpT& z, const FpT& x, const FpT& y)
	{
		FpT rev;
		inv(rev, y);
		mul(z, x, rev);
	}
	bool isZero() const { return op_.isZero(v_); }
	bool operator==(const FpT& rhs) const { return op_.isEqual(v_); }
	bool operator!=(const FpT& rhs) const { return !operator==(rhs); }
	static inline size_t getModBitLen() { return pBitLen_; }
	/*
		append to bv(not clear bv)
	*/
	void appendToBitVec(cybozu::BitVector& bv) const
	{
		bv.append(v_, pBitLen_);
	}
	void fromBitVec(const cybozu::BitVector& bv)
	{
		if (bv.size() != pBitLen_) throw cybozu::Exception("FpT:fromBitVec:bad size") << bv.size() << pBitLen_;
		memcpy(v_, bv.getBlock(), bv.getBlockSize() *  sizeof(Unit));
		if (op_.compare(v_, op_.p) >= 0) throw cybozu::Exception("FpT:fromBitVec:large x");
	}
	static inline size_t getBitVecSize() { return pBitLen_; }
	inline friend FpT operator+(const FpT& x, const FpT& y) { FpT z; FpT::add(z, x, y); return z; }
	FpT& operator+=(const FpT& x) { add(*this, *this, x); return *this; }
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
		const size_t n = Gmp::getBlockSize(x);
		assert(n <= N_);
		Gmp::getRaw(y, N_, x);
		for (size_t i = n; i < N_; i++) y[i] = 0;
	}
};

template<size_t maxBitN, class tag> fp::Op FpT<maxBitN, tag>::op_;
template<size_t maxBitN, class tag> mpz_class FpT<maxBitN, tag>::mp_;

template<size_t maxBitN, class tag> size_t FpT<maxBitN, tag>::pBitLen_;
template<size_t maxBitN, class tag> size_t FpT<maxBitN, tag>::N_;

} // mie

namespace std { CYBOZU_NAMESPACE_TR1_BEGIN
template<class T> struct hash;

#if 0
template<size_t maxUnitN, class tag>
struct hash<mie::FpT<maxUnitN, tag> > : public std::unary_function<mie::FpT<maxUnitN, tag>, size_t> {
	size_t operator()(const mie::FpT<maxUnitN, tag>& x, uint64_t v_ = 0) const
	{
		typedef mie::FpT<maxUnitN, tag> Fp;
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
