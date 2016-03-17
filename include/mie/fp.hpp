#pragma once
/**
	@file
	@brief Fp
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
#include <mie/gmp_util.hpp>
#include <mie/fp_util.hpp>
#include <mie/power.hpp>
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

template<class T, class tag = fp_local::TagDefault>
class FpT : public ope::comparable<FpT<T, tag>,
	ope::addsub<FpT<T, tag>,
	ope::mulable<FpT<T, tag>,
	ope::invertible<FpT<T, tag>,
	ope::hasNegative<FpT<T, tag>,
	ope::hasIO<FpT<T, tag> > > > > > > {
	template<class T2, class tag2> friend class FpT;
public:
	typedef typename T::BlockType BlockType;
	typedef typename T::ImplType ImplType;
	FpT() {}
	FpT(int x) { operator=(x); }
	FpT(uint64_t x) { operator=(x); }
	explicit FpT(const std::string& str, int base = 0)
	{
		fromStr(str, base);
	}
	FpT& operator=(int x)
	{
		if (x >= 0) {
			v = x;
		} else {
			assert(m_ >= -x);
			T::sub(v, m_, -x);
		}
		return *this;
	}
	FpT& operator=(uint64_t x)
	{
		T::set(v, x);
		return *this;
	}
	void fromStr(const std::string& str, int base = 0)
	{
		bool isMinus;
		inFromStr(v, &isMinus, str, base);
		if (v >= m_) throw cybozu::Exception("fp:FpT:fromStr:large str") << str;
		if (isMinus) {
			neg(*this, *this);
		}
	}
	void set(const std::string& str, int base = 0) { fromStr(str, base); }
	void toStr(std::string& str, int base = 10, bool withPrefix = false) const
	{
		switch (base) {
		case 10:
			T::toStr(str, v, base);
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
		T::clear(v);
	}
	template<class RG>
	void setRand(RG& rg)
	{
		std::vector<BlockType> buf(fp::getRoundNum<BlockType>(modBitLen_));
		fp::getRandVal(buf.data(), rg, T::getBlock(m_), modBitLen_);
		T::setRaw(v, buf.data(), buf.size());
	}
	/*
		ignore the value of inBuf over modulo
	*/
	template<class S>
	void setRaw(const S *inBuf, size_t n)
	{
		n = std::min(n, fp::getRoundNum<S>(modBitLen_));
		if (n == 0) {
			clear();
			return;
		}
		std::vector<S> buf(inBuf, inBuf + n);
		setMaskMod(buf);
	}
	static inline void setModulo(const std::string& mstr, int base = 0)
	{
		bool isMinus;
		inFromStr(m_, &isMinus, mstr, base);
		if (isMinus) throw cybozu::Exception("fp:FpT:setModulo:mstr is not minus") << mstr;
		modBitLen_ = T::getBitLen(m_);
		opt_.init(m_);
		mpz_class p;
		mie::Gmp::fromStr(p, mstr, base);
		sq_.set(p);
	}
	static inline bool isYodd(const FpT& y)
	{
		if (y.isZero()) return false;
		return (getBlock(y)[0] & 1) == 1;
	}
	static inline bool squareRoot(FpT& y, const FpT& x)
	{
		return sq_.get(y.v, x.v);
	}
	static inline void getModulo(std::string& mstr)
	{
		T::toStr(mstr, m_);
	}
	static inline void add(FpT& z, const FpT& x, const FpT& y) { T::addMod(z.v, x.v, y.v, m_); }
	static inline void sub(FpT& z, const FpT& x, const FpT& y) { T::subMod(z.v, x.v, y.v, m_); }
	static inline void mul(FpT& z, const FpT& x, const FpT& y) { T::mulMod(z.v, x.v, y.v, m_); }
	static inline void square(FpT& z, const FpT& x) { T::squareMod(z.v, x.v, m_); }

	static inline void add(FpT& z, const FpT& x, unsigned int y) { T::addMod(z.v, x.v, y, m_); }
	static inline void sub(FpT& z, const FpT& x, unsigned int y) { T::subMod(z.v, x.v, y, m_); }
	static inline void mul(FpT& z, const FpT& x, unsigned int y) { T::mulMod(z.v, x.v, y, m_); }

	static inline void inv(FpT& z, const FpT& x) { T::invMod(z.v, x.v, m_); }
	static inline void div(FpT& z, const FpT& x, const FpT& y)
	{
		ImplType rev;
		T::invMod(rev, y.v, m_);
		T::mulMod(z.v, x.v, rev, m_);
	}
	static inline void neg(FpT& z, const FpT& x)
	{
		if (x.isZero()) {
			z.clear();
		} else {
			T::sub(z.v, m_, x.v);
		}
	}
	static inline BlockType getBlock(const FpT& x, size_t i)
	{
		return T::getBlock(x.v, i);
	}
	static inline const BlockType *getBlock(const FpT& x)
	{
		return T::getBlock(x.v);
	}
	static inline size_t getBlockSize(const FpT& x)
	{
		return T::getBlockSize(x.v);
	}
	static inline int compare(const FpT& x, const FpT& y)
	{
		return T::compare(x.v, y.v);
	}
	static inline bool isZero(const FpT& x)
	{
		return T::isZero(x.v);
	}
	static inline size_t getBitLen(const FpT& x)
	{
		return T::getBitLen(x.v);
	}
	static inline void shr(FpT& z, const FpT& x, size_t n)
	{
		z.v = x.v >> n;
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
			opt_.powMod(z.v, x.v, yy.v, m_);
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
	const ImplType& getInnerValue() const { return v; }
	static inline size_t getModBitLen() { return modBitLen_; }
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
		if (sizeof(BlockType) == 8) {
			return isZero(x) ? 0 : getBlock(x, 0);
		} else if (sizeof(BlockType) == 4) {
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
		bv.append(getBlock(*this), getBlockSize(*this) * sizeof(BlockType) * 8);
		bv.resize(len + getModBitLen()); // zero extend if necessary
	}
	void fromBitVec(const cybozu::BitVector& bv)
	{
		if (bv.size() != modBitLen_) throw cybozu::Exception("FpT:fromBitVec:bad size") << bv.size() << modBitLen_;
		T::setRaw(v, bv.getBlock(), bv.getBlockSize());
		if (v >= m_) throw cybozu::Exception("FpT:fromBitVec:large x") << v << m_;
	}
	static inline size_t getBitVecSize() { return modBitLen_; }
	static mie::ope::Optimized<ImplType> opt_;
private:
	static ImplType m_;
	static size_t modBitLen_;
	static mie::SquareRoot sq_;
	ImplType v;
	static inline void inFromStr(ImplType& t, bool *isMinus, const std::string& str, int base)
	{
		const char *p = fp::verifyStr(isMinus, &base, str);
		if (!T::fromStr(t, p, base)) {
			throw cybozu::Exception("fp:FpT:fromStr") << str;
		}
	}
	template<class S>
	void setMaskMod(std::vector<S>& buf)
	{
		assert(buf.size() <= fp::getRoundNum<S>(modBitLen_));
		assert(!buf.empty());
		fp::maskBuffer(&buf[0], buf.size(), modBitLen_);
		T::setRaw(v, &buf[0], buf.size());
		if (v >= m_) {
			T::sub(v, v, m_);
		}
		assert(v < m_);
	}
};

template<class T, class tag>
typename T::ImplType FpT<T, tag>::m_;
template<class T, class tag>
size_t FpT<T, tag>::modBitLen_;
template<class T, class tag>
mie::SquareRoot FpT<T, tag>::sq_;

template<class T, class tag>
mie::ope::Optimized<typename T::ImplType> FpT<T, tag>::opt_;

} // mie

#ifndef MIE_DONT_DEFINE_HASH
namespace std { CYBOZU_NAMESPACE_TR1_BEGIN
template<class T> struct hash;

template<class T, class tag>
struct hash<mie::FpT<T, tag> > {
	size_t operator()(const mie::FpT<T, tag>& x, uint64_t v = 0) const
	{
		typedef mie::FpT<T, tag> Fp;
		size_t n = Fp::getBlockSize(x);
		const typename Fp::BlockType *p = Fp::getBlock(x);
		return static_cast<size_t>(cybozu::hash64(p, n, v));
	}
};

CYBOZU_NAMESPACE_TR1_END } // std::tr1
#endif

#ifdef _MSC_VER
	#pragma warning(pop)
#endif
