#pragma once
/**
	@file
	@brief elliptic curve
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <sstream>
#include <mie/exception.hpp>
#include <mie/operator.hpp>
#include <mie/power.hpp>

namespace mie {

/*
	elliptic curve with affine coordinate
	y^2 = x^3 + ax + b
*/
template<class _Fp>
class ECA : public ope::addsub<ECA<_Fp>,
	ope::hasNegative<ECA<_Fp> > > {
	typedef _Fp Fp;
public:
	Fp x, y;
	bool inf_;
	static Fp a_;
	static Fp b_;
	ECA() : inf_(true) {}
	ECA(const Fp& _x, const Fp& _y)
	{
		set(_x, _y);
	}

	static inline void setParam(const std::string& astr, const std::string& bstr)
	{
		a_.fromStr(astr);
		b_.fromStr(bstr);
	}
	static inline bool isValid(const Fp& _x, const Fp& _y)
	{
		return _y * _y == (_x * _x + a_) * _x + b_;
	}
	void set(const Fp& _x, const Fp& _y, bool verify = true)
	{
		if (verify && !isValid(_x, _y)) throw Exception("ECA:set");
		x = _x; y = _y;
		inf_ = false;
	}
	void clear()
	{
		inf_ = true;
		x.clear();
		y.clear();
	}

	static inline void dbl(ECA& R, const ECA& P, bool verifyInf = true)
	{
		if (verifyInf) {
			if (P.inf_) {
				R.clear(); return;
			}
		}
		Fp t = P.x * P.x * 3;
		t += a_;
		t /= (2 * P.y);
		Fp x3 = t * t - 2 * P.x;
		R.y = (P.x - x3) * t - P.y;
		R.x = x3;
		R.inf_ = false;
	}
	static inline void add(ECA& R, const ECA& P, const ECA& Q)
	{
		if (P.inf_) { R = Q; return; }
		if (Q.inf_) { R = P; return; }
		if (P.y == -Q.y) { R.clear(); return; }
		Fp t = Q.x - P.x;
		if (t.isZero()) {
			dbl(R, P, false);
			return;
		}
		t = (Q.y - P.y) / t;
		R.inf_ = false;
		Fp x3 = t * t - P.x - Q.x;
		R.y = t * (P.x - x3) - P.y;
		R.x = x3;
	}
	static inline void sub(ECA& R, const ECA& P, const ECA& Q)
	{
		ECA nQ;
		neg(nQ, Q);
		add(R, P, nQ);
	}
	static inline void neg(ECA& R, const ECA& P)
	{
		if (P.inf_) {
			R.inf_ = true;
			return;
		}
		R.inf_ = false;
		R.x = P.x;
		Fp::neg(R.y, P.y);
	}
	template<class N>
	static inline void power(ECA& z, const ECA& x, const N& y)
	{
		power_impl::power(z, x, y);
	}

	bool operator==(const ECA& rhs) const
	{
		if (inf_) return rhs.inf_;
		if (rhs.inf_) return false;
		return x == rhs.x && y == rhs.y;
	}
	bool operator!=(const ECA& rhs) const { return !operator==(rhs); }
	bool isZero() const { return inf_; }
	friend inline std::ostream& operator<<(std::ostream& os, const ECA& self)
	{
		if (self.inf_) {
			return os << 'O';
		} else {
			return os << self.x << ' ' << self.y;
		}
	}
	friend inline std::istream& operator>>(std::istream& is, ECA& self)
	{
		std::string str;
		is >> str;
		if (str == "O") {
			self.inf_ = true;
		} else {
			self.inf_ = false;
			self.x.fromStr(str);
			is >> self.y;
		}
		return is;
	}
};

template<class T>
struct TagMultiGr<ECA<T> > {
	static void square(ECA<T>& z, const ECA<T>& x)
	{
//		ECA<T>::dbl(z, x);
		ECA<T>::add(z, x, x);
	}
	static void mul(ECA<T>& z, const ECA<T>& x, const ECA<T>& y)
	{
		ECA<T>::add(z, x, y);
	}
	static void div(ECA<T>& z, const ECA<T>& x, const ECA<T>& y)
	{
		ECA<T>::sub(z, x, y);
	}
	static void init(ECA<T>& x)
	{
		x.clear();
	}
};

template<class _Fp>
_Fp ECA<_Fp>::a_;

template<class _Fp>
_Fp ECA<_Fp>::b_;

struct ECparam {
	const char *p;
	const char *a;
	const char *b;
	const char *gx;
	const char *gy;
	const char *n;
};

} // mie

