#pragma once
/**
    @file
    @brief operator
    @author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://www.opensource.org/licenses/bsd-license.php
*/
#ifdef _WIN32
	#ifndef MIE_FORCE_INLINE
		#define MIE_FORCE_INLINE __forceinline
	#endif
#else
	#ifndef MIE_FORCE_INLINE
		#define MIE_FORCE_INLINE __attribute__((always_inline))
	#endif
#endif

namespace mie {
/*
	T must have compare, add, sub, mul
*/
template<class T>
struct Empty {};

template<class T, class E = Empty<T> >
struct comparable : E {
	MIE_FORCE_INLINE friend bool operator<(const T& x, const T& y) { return T::compare(x, y) < 0; }
	MIE_FORCE_INLINE friend bool operator>=(const T& x, const T& y) { return !operator<(x, y); }

	MIE_FORCE_INLINE friend bool operator>(const T& x, const T& y) { return T::compare(x, y) > 0; }
	MIE_FORCE_INLINE friend bool operator<=(const T& x, const T& y) { return !operator>(x, y); }
	MIE_FORCE_INLINE friend bool operator==(const T& x, const T& y) { return T::compare(x, y) == 0; }
	MIE_FORCE_INLINE friend bool operator!=(const T& x, const T& y) { return !operator==(x, y); }
};

template<class T, class E = Empty<T> >
struct addsubmul : E {
	MIE_FORCE_INLINE T& operator+=(const T& rhs) { T::add(static_cast<T&>(*this), static_cast<const T&>(*this), rhs); return static_cast<T&>(*this); }
	MIE_FORCE_INLINE T& operator-=(const T& rhs) { T::sub(static_cast<T&>(*this), static_cast<const T&>(*this), rhs); return static_cast<T&>(*this); }
	MIE_FORCE_INLINE T& operator*=(const T& rhs) { T::mul(static_cast<T&>(*this), static_cast<const T&>(*this), rhs); return static_cast<T&>(*this); }
	MIE_FORCE_INLINE friend T operator+(const T& a, const T& b) { T c; T::add(c, a, b); return c; }
	MIE_FORCE_INLINE friend T operator-(const T& a, const T& b) { T c; T::sub(c, a, b); return c; }
	MIE_FORCE_INLINE friend T operator*(const T& a, const T& b) { T c; T::mul(c, a, b); return c; }
};

template<class T, class E = Empty<T> >
struct dividable : E {
	MIE_FORCE_INLINE T& operator/=(const T& rhs) { T rdummy; T::div(static_cast<T*>(this), rdummy, static_cast<const T&>(*this), rhs); return static_cast<T&>(*this); }
	MIE_FORCE_INLINE T& operator%=(const T& rhs) { T::div(0, static_cast<T&>(*this), static_cast<const T&>(*this), rhs); return static_cast<T&>(*this); }

	MIE_FORCE_INLINE friend T operator/(const T& a, const T& b) { T q, r; T::div(&q, r, a, b); return q; }
	MIE_FORCE_INLINE friend T operator%(const T& a, const T& b) { T r; T::div(0, r, a, b); return r; }
};

template<class T, class E = Empty<T> >
struct hasNegative : E {
	MIE_FORCE_INLINE T operator-() const { T c; T::neg(c, static_cast<const T&>(*this)); return c; }
};

template<class T, class E = Empty<T> >
struct shiftable : E {
	MIE_FORCE_INLINE T operator<<(size_t n) const { T out; T::shl(out, static_cast<const T&>(*this), n); return out; }
	MIE_FORCE_INLINE T operator>>(size_t n) const { T out; T::shr(out, static_cast<const T&>(*this), n); return out; }

//	T& operator<<=(size_t n) { *this = *this << n; return static_cast<T&>(*this); }
//	T& operator>>=(size_t n) { *this = *this >> n; return static_cast<T&>(*this); }
	MIE_FORCE_INLINE T& operator<<=(size_t n) { T::shl(static_cast<T&>(*this), static_cast<const T&>(*this), n); return static_cast<T&>(*this); }
	MIE_FORCE_INLINE T& operator>>=(size_t n) { T::shr(static_cast<T&>(*this), static_cast<const T&>(*this), n); return static_cast<T&>(*this); }
};

template<class T, class E = Empty<T> >
struct inversible : E {
	MIE_FORCE_INLINE void inverse() { T& self = static_cast<T&>(*this);T out; T::inv(out, self); self = out; }
	MIE_FORCE_INLINE friend T operator/(const T& x, const T& y) { T out; T::inv(out, y); out *= x; return out; }
	MIE_FORCE_INLINE T& operator/=(const T& x) { T rx; T::inv(rx, x); T& self = static_cast<T&>(*this); self *= rx; return self; }
};

} // mie

