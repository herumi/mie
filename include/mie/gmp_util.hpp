#pragma once
/**
    @file
    @brief util function for gmp
    @author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://www.opensource.org/licenses/bsd-license.php
*/
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
	#pragma warning(push)
	#pragma warning(disable : 4616)
	#pragma warning(disable : 4800)
	#pragma warning(disable : 4244)
#endif
#include <gmpxx.h>
#ifdef _WIN32
	#pragma warning(pop)
#endif
#ifdef _WIN32
#pragma comment(lib, "mpir.lib")
#pragma comment(lib, "mpirxx.lib")
#endif

namespace mie { namespace gmp {

// z = [buf[n-1]:..:buf[1]:buf[0]]
// eg. buf[] = {0x12345678, 0xaabbccdd}; => z = 0xaabbccdd12345678;
template<class T>
void setRaw(mpz_class& z, const T *buf, size_t n)
{
	mpz_import(z.get_mpz_t(), n, -1, sizeof(*buf), 0, 0, buf);
}

inline bool fromStr(mpz_class &z, const std::string& str)
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
	if (mpz_init_set_str(z.get_mpz_t(), p, base) == 0) {
		return true;
	}
	mpz_clear(z.get_mpz_t());
	return false;
}

inline void toStr(std::string& str, const mpz_class& z, int base = 10)
{
	__gmp_alloc_cstring tmp(mpz_get_str(0, base, z.get_mpz_t()));
	if (base == 16) {
		str = "0x";
	} else if (base == 2) {
		str = "0b";
	} else if (base == 10) {
		str.clear();
	} else {
		fprintf(stderr, "not support base(%d) in toStr\n", base);
		::exit(1);
	}
	str += tmp.str;
}

inline void add(mpz_class& z, const mpz_class& x, const mpz_class& y)
{
	mpz_add(z.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
}

inline void sub(mpz_class& z, const mpz_class& x, const mpz_class& y)
{
	mpz_sub(z.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
}

inline void mul(mpz_class& z, const mpz_class& x, const mpz_class& y)
{
	mpz_mul(z.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
}

inline void divmod(mpz_class& q, mpz_class& r, const mpz_class& x, const mpz_class& y)
{
	mpz_divmod(q.get_mpz_t(), r.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
}

inline void div(mpz_class& q, const mpz_class& x, const mpz_class& y)
{
	mpz_div(q.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
}

inline void mod(mpz_class& r, const mpz_class& x, const mpz_class& m)
{
	mpz_mod(r.get_mpz_t(), x.get_mpz_t(), m.get_mpz_t());
}

inline void setZero(mpz_class& z)
{
	mpz_clear(z.get_mpz_t());
}

inline bool isNegative(const mpz_class& z)
{
	return mpz_sgn(z.get_mpz_t()) < 0;
}

inline void addMod(mpz_class& z, const mpz_class& x, const mpz_class& y, const mpz_class& m)
{
	add(z, x, y);
	mpz_class t;
	sub(t, z, m);
	if (!isNegative(t)) {
		z = t;
	}
}

inline void subMod(mpz_class& z, const mpz_class& x, const mpz_class& y, const mpz_class& m)
{
	sub(z, x, y);
	if (!isNegative(z)) return;
	add(z, z, m);
}

inline void mulMod(mpz_class& z, const mpz_class& x, const mpz_class& y, const mpz_class& m)
{
	mul(z, x, y);
	mod(z, z, m);
}

// z = x^y mod m
inline void powMod(mpz_class& z, const mpz_class& x, const mpz_class& y, const mpz_class& m)
{
	mpz_powm(z.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t(), m.get_mpz_t());
}

// z = 1/x mod m
inline void invMod(mpz_class& z, const mpz_class& x, const mpz_class& m)
{
	mpz_invert(z.get_mpz_t(), x.get_mpz_t(), m.get_mpz_t());
}

// z = lcm(x, y)
inline void lcm(mpz_class& z, const mpz_class& x, const mpz_class& y)
{
	mpz_lcm(z.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
}

inline bool isPrime(const mpz_class& x)
{
	return mpz_probab_prime_p(x.get_mpz_t(), 10) != 0;
}

inline size_t getBitLen(const mpz_class& x)
{
	return mpz_sizeinbase(x.get_mpz_t(), 2);
}

} } // mie::gmp

