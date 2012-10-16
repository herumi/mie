#pragma once
/**
    @file
    @brief Fp
    @author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://www.opensource.org/licenses/bsd-license.php
*/
#include <sstream>
#include <vector>
#include <mie/exception.hpp>
#include <mie/operator.hpp>
#include <mie/power.hpp>

#define PUT(x) std::cout << #x "=" << (x) << std::endl
namespace mie {

template<class T>
class FpT : public ope::comparable<FpT<T>,
	ope::addsub<FpT<T>,
	ope::mulable<FpT<T>,
	ope::invertible<FpT<T>,
	ope::hasNegative<FpT<T> > > > > > {
public:
	typedef typename T::block_type block_type;
	typedef typename T::value_type value_type;
	FpT() {}
	FpT(int x) : v(x) {}
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
		v = x; return *this;
	}
	void fromStr(const std::string& str)
	{
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
		FpT::setRaw(v, &buf[0], n);
	}
	static inline void setModulo(const std::string& mstr)
	{
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
	bool isNegative() const { return T::isNegative(v); }
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

template<class T>
typename T::value_type FpT<T>::m_;

} // mie

