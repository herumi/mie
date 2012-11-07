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
#include <mie/exception.hpp>
#include <mie/operator.hpp>
#include <mie/power.hpp>

#define PUT(x) std::cout << #x "=" << (x) << std::endl
namespace mie {

template<class T, int tag = 0>
class FpT : public ope::comparable<FpT<T, tag>,
	ope::addsub<FpT<T, tag>,
	ope::mulable<FpT<T, tag>,
	ope::invertible<FpT<T, tag>,
	ope::hasNegative<FpT<T, tag> > > > > > {
public:
	typedef typename T::block_type block_type;
	typedef typename T::value_type value_type;
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
			throw Exception("FpT::fromStr ") << str;
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
			throw Exception("FpT::toStr ", "bad base") << base;
		}
	}
	void clear()
	{
		T::clear(v);
	}
	template<class RG>
	void initRand(RG& r, size_t bitLen)
	{
		const size_t rem = bitLen & 31;
		const size_t n = (bitLen + 31) / 32;
		std::vector<unsigned int> buf(n);
		for (size_t i = 0; i < n; i++) {
			buf[i] = r();
		}
		if (rem > 0) buf[n - 1] &= (1U << rem) - 1;
		buf[n - 1] |= 1U << rem;
		T::setRaw(v, &buf[0], n);
	}
	static inline void setModulo(const std::string& mstr)
	{
		if (mstr.empty() || mstr[0] == '-') {
			throw Exception("FpT::setModulo ", mstr);
		}
		FpT::fromStr(m_, mstr);
	}
	static inline void getModulo(std::string& mstr)
	{
		T::toStr(mstr, m_);
	}
	static inline void add(FpT& z, const FpT& x, const FpT& y)
	{
		T::addMod(z.v, x.v, y.v, m_);
	}
	static inline void sub(FpT& z, const FpT& x, const FpT& y)
	{
		T::subMod(z.v, x.v, y.v, m_);
	}
	static inline void mul(FpT& z, const FpT& x, const FpT& y)
	{
		T::mulMod(z.v, x.v, y.v, m_);
	}
	static inline void inv(FpT& z, const FpT& x)
	{
		T::invMod(z.v, x.v, m_);
	}
	static inline void div(FpT& z, const FpT& x, const FpT& y)
	{
		value_type rev;
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
	static inline block_type getBlock(const FpT& x, size_t i)
	{
		return T::getBlock(x.v, i);
	}
	static inline size_t getBlockSize(const FpT& x)
	{
		return T::getBlockSize(x.v);
	}
	static inline int compare(const FpT& x, const FpT& y)
	{
		return T::compare(x.v, y.v);
	}
	bool isZero() const { return T::isZero(v); }
	size_t getBitLen() const
	{
		return T::getBitLen(v);
	}
	friend inline std::ostream& operator<<(std::ostream& os, const FpT& self)
	{
		return os << self.v;
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
	static value_type m_;
	value_type v;
	static inline void fromStr(value_type& t, const std::string& str)
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
			throw Exception("FpT::FpT ", str);
		}
	}
};

template<class T, int tag>
typename T::value_type FpT<T, tag>::m_;

} // mie

