#pragma once
/**
	@file
	@brief operator
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#ifdef _WIN32
	#ifndef MIE_FORCE_INLINE
		#define MIE_FORCE_INLINE __forceinline
	#endif
	#pragma warning(push)
	#pragma warning(disable : 4714)
#else
	#ifndef MIE_FORCE_INLINE
		#define MIE_FORCE_INLINE __attribute__((always_inline))
	#endif
#endif

namespace mie { namespace ope {

template<class T>
struct Empty {};

/*
	T must have compare
*/
template<class T, class E = Empty<T> >
struct comparable : E {
	MIE_FORCE_INLINE friend bool operator<(const T& x, const T& y) { return T::compare(x, y) < 0; }
	MIE_FORCE_INLINE friend bool operator>=(const T& x, const T& y) { return !operator<(x, y); }

	MIE_FORCE_INLINE friend bool operator>(const T& x, const T& y) { return T::compare(x, y) > 0; }
	MIE_FORCE_INLINE friend bool operator<=(const T& x, const T& y) { return !operator>(x, y); }
	MIE_FORCE_INLINE friend bool operator==(const T& x, const T& y) { return T::compare(x, y) == 0; }
	MIE_FORCE_INLINE friend bool operator!=(const T& x, const T& y) { return !operator==(x, y); }
};

/*
	T must have add, sub
*/
template<class T, class E = Empty<T> >
struct addsub : E {
	template<class S> MIE_FORCE_INLINE T& operator+=(const S& rhs) { T::add(static_cast<T&>(*this), static_cast<const T&>(*this), rhs); return static_cast<T&>(*this); }
	template<class S> MIE_FORCE_INLINE T& operator-=(const S& rhs) { T::sub(static_cast<T&>(*this), static_cast<const T&>(*this), rhs); return static_cast<T&>(*this); }
	template<class S> MIE_FORCE_INLINE friend T operator+(const T& a, const S& b) { T c; T::add(c, a, b); return c; }
	template<class S> MIE_FORCE_INLINE friend T operator-(const T& a, const S& b) { T c; T::sub(c, a, b); return c; }
};

/*
	T must have mul
*/
template<class T, class E = Empty<T> >
struct mulable : E {
	template<class S> MIE_FORCE_INLINE T& operator*=(const S& rhs) { T::mul(static_cast<T&>(*this), static_cast<const T&>(*this), rhs); return static_cast<T&>(*this); }
	template<class S> MIE_FORCE_INLINE friend T operator*(const T& a, const S& b) { T c; T::mul(c, a, b); return c; }
};

/*
	T must have inv, mul
*/
template<class T, class E = Empty<T> >
struct invertible : E {
	MIE_FORCE_INLINE T& operator/=(const T& rhs) { T c; T::inv(c, rhs); T::mul(static_cast<T&>(*this), static_cast<const T&>(*this), c); return static_cast<T&>(*this); }
	MIE_FORCE_INLINE friend T operator/(const T& a, const T& b) { T c; T::inv(c, b); T::mul(c, c, a); return c; }
};

/*
	T must have neg
*/
template<class T, class E = Empty<T> >
struct hasNegative : E {
	MIE_FORCE_INLINE T operator-() const { T c; T::neg(c, static_cast<const T&>(*this)); return c; }
};

} } // mie::ope

#ifdef _WIN32
//	#pragma warning(pop)
#endif
