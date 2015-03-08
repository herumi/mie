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
#if defined(_WIN64) || defined(__x86_64__)
//	#define USE_MONT_FP
#endif
#include <cybozu/hash.hpp>
#include <cybozu/itoa.hpp>
#include <cybozu/atoi.hpp>
#include <cybozu/bitvector.hpp>
#include <mie/fp_base.hpp>
#include <mie/fp_util.hpp>
#include <mie/gmp_util.hpp>

#ifndef MIE_FP_BLOCK_MAX_BIT_N
	#define MIE_FP_BLOCK_MAX_BIT_N 640
#endif

namespace mie {

struct Block {
	typedef fp::Unit Unit;
	const Unit *p; // pointer to original FpT.v_
	size_t n;
	static const size_t UnitByteN = sizeof(Unit);
	static const size_t maxUnitN = (MIE_FP_BLOCK_MAX_BIT_N + UnitByteN * 8 - 1) / (UnitByteN * 8);
	Unit v_[maxUnitN];
};

template<size_t maxBitN, class tag = fp::TagDefault>
class FpT {
	typedef fp::Unit Unit;
	static const size_t UnitByteN = sizeof(Unit);
	static const size_t maxUnitN = (maxBitN + UnitByteN * 8 - 1) / (UnitByteN * 8);
	static fp::Op op_;
	static size_t pBitLen_;
	template<size_t maxBitN2, class tag2> friend class FpT;
public:
	typedef Unit BlockType;
	Unit v_[maxUnitN];
	void dump() const
	{
		const size_t N = op_.N;
		for (size_t i = 0; i < N; i++) {
			printf("%016llx ", (long long)v_[N - 1 - i]);
		}
		printf("\n");
	}
	static inline void setModulo(const std::string& mstr, int base = 0)
	{
		bool isMinus;
		mpz_class mp;
		inFromStr(mp, &isMinus, mstr, base);
		if (isMinus) throw cybozu::Exception("mie:FpT:setModulo:mstr is not minus") << mstr;
		pBitLen_ = Gmp::getBitLen(mp);
		if (pBitLen_ > maxBitN) throw cybozu::Exception("mie:FpT:setModulo:too large bitLen") << pBitLen_ << maxBitN;
		Unit p[maxUnitN] = {};
		const size_t n = Gmp::getRaw(p, maxUnitN, mp);
		if (n == 0) throw cybozu::Exception("mie:FpT:setModulo:bad mstr") << mstr;
#ifdef USE_MONT_FP
		if (pBitLen_ <= 128) {  op_ = fp::MontFp<128, tag>::init(p); }
		else if (pBitLen_ <= 192) { static fp::MontFp<192, tag> f; op_ = f.init(p); }
		else if (pBitLen_ <= 256) { static fp::MontFp<256, tag> f; op_ = f.init(p); }
		else if (pBitLen_ <= 384) { static fp::MontFp<384, tag> f; op_ = f.init(p); }
		else if (pBitLen_ <= 448) { static fp::MontFp<448, tag> f; op_ = f.init(p); }
		else if (pBitLen_ <= 576) { static fp::MontFp<576, tag> f; op_ = f.init(p); }
		else { static fp::MontFp<maxBitN, tag> f; op_ = f.init(p); }
#else
		if (pBitLen_ <= 128) {  op_ = fp::FixedFp<128, tag>::init(p); }
		else if (pBitLen_ <= 192) { static fp::FixedFp<192, tag> f; op_ = f.init(p); }
		else if (pBitLen_ <= 256) { static fp::FixedFp<256, tag> f; op_ = f.init(p); }
		else if (pBitLen_ <= 384) { static fp::FixedFp<384, tag> f; op_ = f.init(p); }
		else if (pBitLen_ <= 448) { static fp::FixedFp<448, tag> f; op_ = f.init(p); }
		else if (pBitLen_ <= 576) { static fp::FixedFp<576, tag> f; op_ = f.init(p); }
		else { static fp::FixedFp<maxBitN, tag> f; op_ = f.init(p); }
#endif
	}
	static inline void getModulo(std::string& pstr)
	{
		Gmp::toStr(pstr, op_.mp);
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
	FpT(int64_t x) { operator=(x); }
	explicit FpT(const std::string& str, int base = 0)
	{
		fromStr(str, base);
	}
	FpT& operator=(int64_t x)
	{
		clear();
		if (x) {
			int64_t y = x < 0 ? -x : x;
			if (sizeof(Unit) == 8) {
				v_[0] = y;
			} else {
				v_[0] = (uint32_t)y;
				v_[1] = (uint32_t)(y >> 32);
			}
			if (x < 0) neg(*this, *this);
			toMont(*this, *this);
		}
		return *this;
	}
	void toMont(FpT& y, const FpT& x)
	{
		if (op_.toMont) op_.toMont(y.v_, x.v_);
	}
	void fromMont(FpT& y, const FpT& x)
	{
		if (op_.fromMont) op_.fromMont(y.v_, x.v_);
	}
	void fromStr(const std::string& str, int base = 0)
	{
		bool isMinus;
		mpz_class x;
		inFromStr(x, &isMinus, str, base);
		if (x >= op_.mp) throw cybozu::Exception("fp:FpT:fromStr:large str") << str;
		fp::local::toArray(v_, op_.N, x.get_mpz_t());
		if (isMinus) {
			neg(*this, *this);
		}
		toMont(*this, *this);
	}
	// alias of fromStr
	void set(const std::string& str, int base = 0) { fromStr(str, base); }
	template<class S>
	void setRaw(const S *inBuf, size_t n)
	{
		const size_t byteN = sizeof(S) * n;
		const size_t fpByteN = sizeof(Unit) * op_.N;
		if (byteN > fpByteN) throw cybozu::Exception("setRaw:bad n") << n << fpByteN;
		assert(byteN <= fpByteN);
		memcpy(v_, inBuf, byteN);
		memset((char *)v_ + byteN, 0, fpByteN - byteN);
		if (!isValid()) throw cybozu::Exception("setRaw:large value");
		toMont(*this, *this);
	}
	template<class S>
	size_t getRaw(S *outBuf, size_t n) const
	{
		const size_t byteN = sizeof(S) * n;
		const size_t fpByteN = sizeof(Unit) * op_.N;
		if (byteN < fpByteN) throw cybozu::Exception("getRaw:bad n") << n << fpByteN;
		assert(byteN >= fpByteN);
		Block b;
		getBlock(b);
		memcpy(outBuf, b.p, fpByteN);
		const size_t writeN = (fpByteN + sizeof(S) - 1) / sizeof(S);
		memset((char *)outBuf + fpByteN, 0, writeN * sizeof(S) - fpByteN);
		return writeN;
	}
	void getBlock(Block& b) const
	{
		assert(maxUnitN >= Block::maxUnitN);
		b.n = op_.N;
		if (op_.fromMont) {
			op_.fromMont(b.v_, v_);
			b.p = &b.v_[0];
		} else {
			b.p = &v_[0];
		}
	}
	template<class RG>
	void setRand(RG& rg)
	{
		fp::getRandVal(v_, rg, op_.p, pBitLen_);
		fromMont(*this, *this);
	}
	static inline void toStr(std::string& str, const Unit *x, size_t n, int base = 10, bool withPrefix = false)
	{
		switch (base) {
		case 10:
			{
				mpz_class t;
				Gmp::setRaw(t, x, n);
				Gmp::toStr(str, t, 10);
			}
			return;
		case 16:
			mie::fp::toStr16(str, x, n, withPrefix);
			return;
		case 2:
			mie::fp::toStr2(str, x, n, withPrefix);
			return;
		default:
			throw cybozu::Exception("fp:FpT:toStr:bad base") << base;
		}
	}
	void toStr(std::string& str, int base = 10, bool withPrefix = false) const
	{
		Block b;
		getBlock(b);
		toStr(str, b.p, b.n, base, withPrefix);
	}
	std::string toStr(int base = 10, bool withPrefix = false) const
	{
		std::string str;
		toStr(str, base, withPrefix);
		return str;
	}
	static inline void add(FpT& z, const FpT& x, const FpT& y) { op_.add(z.v_, x.v_, y.v_); }
	static inline void sub(FpT& z, const FpT& x, const FpT& y) { op_.sub(z.v_, x.v_, y.v_); }
	static inline void mul(FpT& z, const FpT& x, const FpT& y) { op_.mul(z.v_, x.v_, y.v_); }
	static inline void inv(FpT& y, const FpT& x) { op_.inv(y.v_, x.v_); }
	static inline void neg(FpT& y, const FpT& x) { op_.neg(y.v_, x.v_); }
	static inline void square(FpT& y, const FpT& x) { op_.square(y.v_, x.v_); }
	static inline void div(FpT& z, const FpT& x, const FpT& y)
	{
		FpT rev;
		inv(rev, y);
		mul(z, x, rev);
	}
	static inline void powerArray(FpT& z, const FpT& x, const Unit *y, size_t yn)
	{
		FpT out(1);
		FpT t(x);
		for (size_t i = 0; i < yn; i++) {
			const Unit v = y[i];
			int m = (int)sizeof(Unit) * 8;
			if (i == yn - 1) {
				while (m > 0 && (v & (Unit(1) << (m - 1))) == 0) {
					m--;
				}
			}
			for (int j = 0; j < m; j++) {
				if (v & (Unit(1) << j)) {
					out *= t;
				}
				t *= t;
			}
		}
		z = out;
	}
	template<size_t maxBitN2, class tag2>
	static inline void power(FpT& z, const FpT& x, const FpT<maxBitN2, tag2>& y)
	{
		Block b;
		y.getBlock(b);
		powerArray(z, x, b.p, b.n);
	}
	static inline void power(FpT& z, const FpT& x, int y)
	{
		if (y < 0) throw cybozu::Exception("FpT:power with negative y is not support") << y;
		const Unit u = y;
		powerArray(z, x, &u, 1);
	}
	static inline void power(FpT& z, const FpT& x, const mpz_class& y)
	{
		if (y < 0) throw cybozu::Exception("FpT:power with negative y is not support") << y;
		powerArray(z, x, Gmp::getBlock(y), Gmp::getBlockSize(x));
	}
	bool isZero() const { return op_.isZero(v_); }
	/*
		append to bv(not clear bv)
	*/
	void appendToBitVec(cybozu::BitVector& bv) const
	{
		Block b;
		getBlock(b);
		bv.append(b.p, pBitLen_);
	}
	bool isValid() const
	{
		return fp::local::compareArray(v_, op_.p, op_.N) < 0;
	}
	void fromBitVec(const cybozu::BitVector& bv)
	{
		if (bv.size() != pBitLen_) throw cybozu::Exception("FpT:fromBitVec:bad size") << bv.size() << pBitLen_;
		setRaw(bv.getBlock(), bv.getBlockSize());
	}
	static inline size_t getModBitLen() { return pBitLen_; }
	static inline size_t getBitVecSize() { return pBitLen_; }
	bool operator==(const FpT& rhs) const { return fp::local::isEqualArray(v_, rhs.v_, op_.N); }
	bool operator!=(const FpT& rhs) const { return !operator==(rhs); }
	inline friend FpT operator+(const FpT& x, const FpT& y) { FpT z; add(z, x, y); return z; }
	inline friend FpT operator-(const FpT& x, const FpT& y) { FpT z; sub(z, x, y); return z; }
	inline friend FpT operator*(const FpT& x, const FpT& y) { FpT z; mul(z, x, y); return z; }
	inline friend FpT operator/(const FpT& x, const FpT& y) { FpT z; div(z, x, y); return z; }
	FpT& operator+=(const FpT& x) { add(*this, *this, x); return *this; }
	FpT& operator-=(const FpT& x) { sub(*this, *this, x); return *this; }
	FpT& operator*=(const FpT& x) { mul(*this, *this, x); return *this; }
	FpT& operator/=(const FpT& x) { div(*this, *this, x); return *this; }
	friend inline std::ostream& operator<<(std::ostream& os, const FpT& self)
	{
		const std::ios_base::fmtflags f = os.flags();
		if (f & std::ios_base::oct) throw cybozu::Exception("fpT:operator<<:oct is not supported");
		const int base = (f & std::ios_base::hex) ? 16 : 10;
		const bool showBase = (f & std::ios_base::showbase) != 0;
		std::string str;
		self.toStr(str, base, showBase);
		return os << str;
	}
	friend inline std::istream& operator>>(std::istream& is, FpT& self)
	{
		const std::ios_base::fmtflags f = is.flags();
		if (f & std::ios_base::oct) throw cybozu::Exception("fpT:operator>>:oct is not supported");
		const int base = (f & std::ios_base::hex) ? 16 : 0;
		std::string str;
		is >> str;
		self.fromStr(str, base);
		return is;
	}
	/*
		not support
		getBitLen, operator<, >
	*/
private:
	static inline void inFromStr(mpz_class& x, bool *isMinus, const std::string& str, int base)
	{
		const char *p = fp::verifyStr(isMinus, &base, str);
		if (!Gmp::fromStr(x, p, base)) {
			throw cybozu::Exception("fp:FpT:inFromStr") << str;
		}
	}
};

template<size_t maxBitN, class tag> fp::Op FpT<maxBitN, tag>::op_;
template<size_t maxBitN, class tag> size_t FpT<maxBitN, tag>::pBitLen_;

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
