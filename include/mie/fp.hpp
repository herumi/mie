#pragma once
/**
    @file
    @brief Fp
    @author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://www.opensource.org/licenses/bsd-license.php
*/
#include <sstream>
#include <mie/exception.hpp>

namespace mie {

template<class T>
class FpT {
	typedef typename T::value_type value_type;
	static T m_;
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
public:
	FpT() {}
	FpT(int x) : v(x) {}
	explicit FpT(const std::string& str)
	{
		fromStr(str);
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
	void set(int x)
	{
		v = x;
	}
	static inline void setModulo(const std::string& str)
	{
		fromStr(m_, str);
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
		T::invMod(rev.v, y.v, m_);
		T::mulMod(z, x, rev.v);
	}
	bool isZero() const
	{
		return T::v.isZero();
	}
	friend inline bool operator==(const FpT& x, const FpT& y)
	{
		return T::isEqual(x.v, y.v);
	}
	friend inline bool operator==(const FpT& x, int y)
	{
		return T::isEqual(x.v, y);
	}
	friend inline bool operator!=(const FpT& x, const FpT& y)
	{
		return !operator==(x.v, y.v);
	}
	friend inline bool operator!=(const FpT& x, int y)
	{
		return !operator==(x.v, y);
	}
	friend inline int cmp(const FpT& x, const FpT& y)
	{
		return T::cmp(x.v, y.v);
	}
	friend inline std::ostream& operator<<(std::ostream& os, const FpT& self)
	{
		return os << self.v;
	}
	friend inline std::istream& operator>>(std::istream& is, const FpT& self)
	{
		std::string str;
		is >> str;
		self.fromStr(str);
		return is;
	}
};

template<class T>
T FpT<T>::m_;

} // mie

