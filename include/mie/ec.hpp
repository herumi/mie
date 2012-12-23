#pragma once
/**
	@file
	@brief elliptic curve
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <sstream>
#include <cybozu/exception.hpp>
#include <mie/operator.hpp>
#include <mie/power.hpp>

namespace mie {

/*
	elliptic curve with affine coordinate
	y^2 = x^3 + ax + b
*/
template<class _Fp>
class EcT : public ope::addsub<EcT<_Fp>,
	ope::comparable<EcT<_Fp>,
	ope::hasNegative<EcT<_Fp> > > > {
	typedef _Fp Fp;
public:
	Fp x, y;
	bool inf_;
	static Fp a_;
	static Fp b_;
	EcT() : inf_(true) {}
	EcT(const Fp& _x, const Fp& _y)
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
		if (verify && !isValid(_x, _y)) throw cybozu::Exception("ec:EcT:set") << _x << _y;
		x = _x; y = _y;
		inf_ = false;
	}
	void clear()
	{
		inf_ = true;
		x.clear();
		y.clear();
	}

	static inline void dbl(EcT& R, const EcT& P, bool verifyInf = true)
	{
		if (verifyInf) {
			if (P.inf_) {
				R.clear(); return;
			}
		}
		Fp t, s;
		Fp::mul(t, P.x, P.x);
		Fp::add(s, t, t);
		t += s;
		t += a_;
		Fp::add(s, P.y, P.y);
		t /= s;
		Fp::mul(s, t, t);
		s -= P.x;
		Fp x3;
		Fp::sub(x3, s, P.x);
		Fp::sub(s, P.x, x3);
		s *= t;
		Fp::sub(R.y, s, P.y);
		R.x = x3;
		R.inf_ = false;
	}
	static inline void add(EcT& R, const EcT& P, const EcT& Q)
	{
		if (P.inf_) { R = Q; return; }
		if (Q.inf_) { R = P; return; }
		Fp t;
		Fp::neg(t, Q.y);
		if (P.y == t) { R.clear(); return; }
		Fp::sub(t, Q.x, P.x);
		if (t.isZero()) {
			dbl(R, P, false);
			return;
		}
		Fp s;
		Fp::sub(s, Q.y, P.y);
		Fp::div(t, s, t);
		R.inf_ = false;
		Fp x3;
		Fp::mul(x3, t, t);
		x3 -= P.x;
		x3 -= Q.x;
		Fp::sub(s, P.x, x3);
		s *= t;
		Fp::sub(R.y, s, P.y);
		R.x = x3;
	}
	static inline void sub(EcT& R, const EcT& P, const EcT& Q)
	{
#if 1
		if (P.inf_) { neg(R, Q); return; }
		if (Q.inf_) { R = P; return; }
		if (P.y == Q.y) { R.clear(); return; }
		Fp t;
		Fp::sub(t, Q.x, P.x);
		if (t.isZero()) {
			dbl(R, P, false);
			return;
		}
		Fp s;
		Fp::add(s, Q.y, P.y);
		Fp::neg(s, s);
		Fp::div(t, s, t);
		R.inf_ = false;
		Fp x3;
		Fp::mul(x3, t, t);
		x3 -= P.x;
		x3 -= Q.x;
		Fp::sub(s, P.x, x3);
		s *= t;
		Fp::sub(R.y, s, P.y);
		R.x = x3;
#else
		EcT nQ;
		neg(nQ, Q);
		add(R, P, nQ);
#endif
	}
	static inline void neg(EcT& R, const EcT& P)
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
	static inline void power(EcT& z, const EcT& x, const N& y)
	{
		power_impl::power(z, x, y);
	}
	/*
		O <= P for any P
		(Px, Py) <= (P'x, P'y) iff Px < P'x or Px == P'x and Py <= P'y
	*/
	static inline int compare(const EcT& P, const EcT& Q)
	{
		if (P.inf_) {
			if (Q.inf_) return 0;
			return -1;
		}
		if (Q.inf_) return 1;
		int c = _Fp::compare(P.x, Q.x);
		if (c > 0) return 1;
		if (c < 0) return -1;
		return _Fp::compare(P.y, Q.y);
	}
	bool isZero() const { return inf_; }
	friend inline std::ostream& operator<<(std::ostream& os, const EcT& self)
	{
		if (self.inf_) {
			return os << 'O';
		} else {
			return os << self.x << ' ' << self.y;
		}
	}
	friend inline std::istream& operator>>(std::istream& is, EcT& self)
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
struct TagMultiGr<EcT<T> > {
	static void square(EcT<T>& z, const EcT<T>& x)
	{
		EcT<T>::dbl(z, x);
	}
	static void mul(EcT<T>& z, const EcT<T>& x, const EcT<T>& y)
	{
		EcT<T>::add(z, x, y);
	}
	static void inv(EcT<T>& z, const EcT<T>& x)
	{
		EcT<T>::neg(z, x);
	}
	static void div(EcT<T>& z, const EcT<T>& x, const EcT<T>& y)
	{
		EcT<T>::sub(z, x, y);
	}
	static void init(EcT<T>& x)
	{
		x.clear();
	}
};

template<class _Fp>
_Fp EcT<_Fp>::a_;

template<class _Fp>
_Fp EcT<_Fp>::b_;

struct EcParam {
	const char *name;
	const char *p;
	const char *a;
	const char *b;
	const char *gx;
	const char *gy;
	const char *n;
};

} // mie
