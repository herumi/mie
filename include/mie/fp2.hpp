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

} // fp

namespace fp_local {
struct TagDefault;
} // fp_local

template<size_t maxN, class tag = fp_local::TagDefault>
class FpT {
	void fillZeroo(uint64_t *p, size_t fromPos) const
	{
		for (size_t i = fromPos; i < N_; i++) p[i] = 0;
	}
public:
	FpT() {}
	FpT(int x) { operator=(x); }

	FpT& operator=(int x)
	{
		if (x >= 0) {
			v_[0] = x;
			fillZero(v_, 1);
		} else {
			assert(pOrg_ >= -x);
			v_[0] = -x;
			fillZero(v_, 1);
			neg();
		}
		return *this;
	}

	static inline void setModulo(const std::string& mstr, int base = 0)
	{
		bool isMinus;
		inFromStr(pOrg_, &isMinus, mstr, base);
		if (isMinus) throw cybozu::Exception("mie:FpT:setModulo:mstr is not minus") << mstr;
		pBitLen_ = Gmp::getBitLen(pOrg_);
		N_ = Gmp::getRaw(p_, maxN, pOrg_);
		if (N_ == 0) throw cybozu::Exception("mie:FpT:setModulo:bad mstr") << mstr;
	}
#if 0
	explicit FpT(const std::string& str, int base = 0)
	{
		fromStr(str, base);
	}
	void fromStr(const std::string& str, int base = 0)
	{
		bool isMinus;
		inFromStr(v_, &isMinus, str, base);
		if (v_ >= p_) throw cybozu::Exception("fp:FpT:fromStr:large str") << str;
		if (isMinus) {
			neg(*this, *this);
		}
	}
	void set(const std::string& str, int base = 0) { fromStr(str, base); }
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
	void clear()
	{
		T::clear(v_);
	}
	template<class RG>
	void setRand(RG& rg)
	{
		std::vector<uint64_t> buf(fp::getRoundNum<uint64_t>(pBitLen_));
		fp::getRandVal(buf.data(), rg, T::getBlock(p_), pBitLen_);
		T::setRaw(v_, buf.data(), buf.size());
	}
	/*
		ignore the value of inBuf over modulo
	*/
	template<class S>
	void setRaw(const S *inBuf, size_t n)
	{
		n = std::min(n, fp::getRoundNum<S>(pBitLen_));
		if (n == 0) {
			clear();
			return;
		}
		std::vector<S> buf(inBuf, inBuf + n);
		setMaskMod(buf);
	}
	static inline void getModulo(std::string& mstr)
	{
		T::toStr(mstr, p_);
	}
	static inline void add(FpT& z, const FpT& x, const FpT& y) { T::addMod(z.v_, x.v_, y.v_, p_); }
	static inline void sub(FpT& z, const FpT& x, const FpT& y) { T::subMod(z.v_, x.v_, y.v_, p_); }
	static inline void mul(FpT& z, const FpT& x, const FpT& y) { T::mulMod(z.v_, x.v_, y.v_, p_); }
	static inline void square(FpT& z, const FpT& x) { T::squareMod(z.v_, x.v_, p_); }

	static inline void add(FpT& z, const FpT& x, unsigned int y) { T::addMod(z.v_, x.v_, y, p_); }
	static inline void sub(FpT& z, const FpT& x, unsigned int y) { T::subMod(z.v_, x.v_, y, p_); }
	static inline void mul(FpT& z, const FpT& x, unsigned int y) { T::mulMod(z.v_, x.v_, y, p_); }
#endif

#if 0
	static inline void inv(FpT& z, const FpT& x) { T::invMod(z.v_, x.v_, p_); }
	static inline void div(FpT& z, const FpT& x, const FpT& y)
	{
		ImplType rev;
		T::invMod(rev, y.v_, p_);
		T::mulMod(z.v_, x.v_, rev, p_);
	}
	static inline void neg(FpT& z, const FpT& x)
	{
		if (x.isZero()) {
			z.clear();
		} else {
			T::sub(z.v_, p_, x.v_);
		}
	}
	static inline uint64_t getBlock(const FpT& x, size_t i)
	{
		return T::getBlock(x.v_, i);
	}
	static inline const uint64_t *getBlock(const FpT& x)
	{
		return T::getBlock(x.v_);
	}
	static inline size_t getBlockSize(const FpT& x)
	{
		return T::getBlockSize(x.v_);
	}
	static inline int compare(const FpT& x, const FpT& y)
	{
		return T::compare(x.v_, y.v_);
	}
	static inline bool isZero(const FpT& x)
	{
		return T::isZero(x.v_);
	}
	static inline size_t getBitLen(const FpT& x)
	{
		return T::getBitLen(x.v_);
	}
	static inline void shr(FpT& z, const FpT& x, size_t n)
	{
		z.v_ = x.v_ >> n;
	}
	bool isZero() const { return isZero(*this); }
	size_t getBitLen() const { return getBitLen(*this); }

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
	static inline size_t getModBitLen() { return pBitLen_; }
	const ImplType& getInnerValue() const { return v_; }
	static inline uint64_t cvtInt(const FpT& x, bool *err = 0)
	{
		if (x > uint64_t(0xffffffffffffffffull)) {
			if (err) {
				*err = true;
				return 0;
			} else {
				throw cybozu::Exception("fp:FpT:cvtInt:too large") << x;
			}
		}
		if (err) *err = false;
		if (sizeof(uint64_t) == 8) {
			return isZero(x) ? 0 : getBlock(x, 0);
		} else if (sizeof(uint64_t) == 4) {
			uint64_t ret = getBlock(x, 0);
			if (getBlockSize(x) == 2) {
				ret += uint64_t(getBlock(x, 1)) << 32;
			}
			return ret;
		} else {
			fprintf(stderr, "fp:FpT:cvtInt:not implemented\n");
			exit(1);
		}
	}
	uint64_t cvtInt(bool *err = 0) const { return cvtInt(*this, err); }
	/*
		append to bv(not clear bv)
	*/
	void appendToBitVec(cybozu::BitVector& bv) const
	{
		const size_t len = bv.size();
		bv.append(getBlock(*this), getBlockSize(*this) * sizeof(uint64_t) * 8);
		bv.resize(len + getModBitLen()); // zero extend if necessary
	}
	void fromBitVec(const cybozu::BitVector& bv)
	{
		if (bv.size() != pBitLen_) throw cybozu::Exception("FpT:fromBitVec:bad size") << bv.size() << pBitLen_;
		T::setRaw(v_, bv.getBlock(), bv.getBlockSize());
		if (v_ >= p_) throw cybozu::Exception("FpT:fromBitVec:large x") << v_ << p_;
	}
	static inline size_t getBitVecSize() { return pBitLen_; }
#endif
private:
	static mpz_class pOrg_;
	static uint64_t p_[maxN];
	static size_t pBitLen_;
	static size_t N_;
	uint64_t v_[maxN];
	static inline void inFromStr(mpz_class& x, bool *isMinus, const std::string& str, int base)
	{
		const char *p = fp::verifyStr(isMinus, &base, str);
		if (!Gmp::fromStr(x, p, base)) {
			throw cybozu::Exception("fp:FpT:inFromStr") << str;
		}
	}
#if 0
	template<class S>
	void setMaskMod(std::vector<S>& buf)
	{
		assert(buf.size() <= fp::getRoundNum<S>(pBitLen_));
		assert(!buf.empty());
		fp::maskBuffer(&buf[0], buf.size(), pBitLen_);
		T::setRaw(v_, &buf[0], buf.size());
		if (v_ >= p_) {
			T::sub(v_, v_, p_);
		}
		assert(v_ < p_);
	}
#endif
};

template<size_t maxN, class tag> mpz_class FpT<maxN, tag>::pOrg_;
template<size_t maxN, class tag> uint64_t FpT<maxN, tag>::p_[maxN];
template<size_t maxN, class tag> size_t FpT<maxN, tag>::pBitLen_;
template<size_t maxN, class tag> size_t FpT<maxN, tag>::N_;

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
