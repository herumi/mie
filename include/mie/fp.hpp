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
#include <cybozu/exception.hpp>
#include <cybozu/hash.hpp>
#include <mie/operator.hpp>
#include <mie/power.hpp>

#define PUT(x) std::cout << #x "=" << (x) << std::endl
namespace mie {

namespace fp_local {
struct TagDefault;
}

template<class T, class tag = fp_local::TagDefault>
class FpT : public ope::comparable<FpT<T, tag>,
	ope::addsub<FpT<T, tag>,
	ope::mulable<FpT<T, tag>,
	ope::invertible<FpT<T, tag>,
	ope::hasNegative<FpT<T, tag> > > > > > {
public:
	typedef typename T::BlockType BlockType;
	typedef typename T::ImplType ImplType;
	FpT() {}
	FpT(int x) { operator=(x); }
	explicit FpT(const std::string& str)
	{
		fromStr(str);
	}
	FpT(const FpT& x)
		: v(x.v)
	{
	}
	FpT& operator=(const FpT& x)
	{
		v = x.v; return *this;
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
	void fromStr(const std::string& str)
	{
		if (str.empty() || str[0] == '-') {
			throw cybozu::Exception("fp:FpT:fromStr") << str;
		}
		fromStr(v, str);
	}
	void toStr(std::string& str, int base = 10) const
	{
		T::toStr(str, v, base);
		if (base == 16) {
			str.insert(0, "0x");
		} else if (base == 2) {
			str.insert(0, "0b");
		} else if (base != 10) {
			throw cybozu::Exception("fp:FpT:toStr:bad base") << base;
		}
	}
	std::string toStr(int base = 10) const
	{
		std::string str;
		toStr(str, base);
		return str;
	}
	void clear()
	{
		T::clear(v);
	}
	template<class RG>
	void initRand(RG& rg, size_t bitLen)
	{
		const size_t rem = bitLen & 31;
		const size_t n = (bitLen + 31) / 32;
		std::vector<unsigned int> buf(n);
		for (size_t i = 0; i < n; i++) {
			buf[i] = rg();
		}
		if (rem > 0) buf[n - 1] &= (1U << rem) - 1;
		buf[n - 1] |= 1U << rem;
		T::setRaw(v, &buf[0], n);
	}
	template<class S>
	void setRaw(const S *buf, size_t n)
	{
		T::setRaw(v, buf, n);
		T::mod(v, v, m_);
	}
	static inline void setModulo(const std::string& mstr)
	{
		if (mstr.empty() || mstr[0] == '-') {
			throw cybozu::Exception("fp:FpT:setModulo") << mstr;
		}
		FpT::fromStr(m_, mstr);
	}
	static inline void getModulo(std::string& mstr)
	{
		T::toStr(mstr, m_);
	}
	static inline void add(FpT& z, const FpT& x, const FpT& y) { T::addMod(z.v, x.v, y.v, m_); }
	static inline void sub(FpT& z, const FpT& x, const FpT& y) { T::subMod(z.v, x.v, y.v, m_); }
	static inline void mul(FpT& z, const FpT& x, const FpT& y) { T::mulMod(z.v, x.v, y.v, m_); }

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
	bool isZero() const { return isZero(*this); }
	size_t getBitLen() const { return getBitLen(*this); }
	friend inline std::ostream& operator<<(std::ostream& os, const FpT& self)
	{
		int base = (os.flags() & std::ios_base::hex) ? 16 : 10;
		std::string str;
		self.toStr(str, base);
		return os << str;
	}
	friend inline std::istream& operator>>(std::istream& is, FpT& self)
	{
		std::string str;
		is >> str;
		self.fromStr(str);
		return is;
	}
	template<class N>
	static void power(FpT& z, const FpT& x, const N& y)
	{
		power_impl::power(z, x, y);
	}
private:
	static ImplType m_;
	ImplType v;
	static inline void fromStr(ImplType& t, const std::string& str)
	{
		const char *p = str.c_str();
		int base = 10;
		if (str.size() > 2 && str[0] == '0') {
			if (str[1] == 'x') {
				base = 16;
				p += 2;
			} else if (str[1] == 'b') {
				base = 2;
				p += 2;
			}
		}
		if (!T::fromStr(t, p, base)) {
			throw cybozu::Exception("fp:FpT:FpT") << str;
		}
	}
};

template<class T, class tag>
typename T::ImplType FpT<T, tag>::m_;

} // mie

namespace std {
template<class T> struct hash;

template<class T, class tag>
struct hash<mie::FpT<T, tag> > : public std::unary_function<mie::FpT<T, tag>, size_t> {
	size_t operator()(const mie::FpT<T, tag>& x, uint64_t v = 0) const
	{
		typedef mie::FpT<T, tag> Fp;
		size_t n = Fp::getBlockSize(x);
		const typename Fp::BlockType *p = Fp::getBlock(x);
		return static_cast<size_t>(cybozu::hash64(p, n, v));
	}
};

} // std

