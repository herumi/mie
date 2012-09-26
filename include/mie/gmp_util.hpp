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

namespace mie {

struct Gmp {
	typedef mpz_class value_type;
	// z = [buf[n-1]:..:buf[1]:buf[0]]
	// eg. buf[] = {0x12345678, 0xaabbccdd}; => z = 0xaabbccdd12345678;
	template<class T>
	static void setRaw(mpz_class& z, const T *buf, size_t n)
	{
		mpz_import(z.get_mpz_t(), n, -1, sizeof(*buf), 0, 0, buf);
	}

	static inline bool fromStr(mpz_class &z, const std::string& str, int base = 10)
	{
		return z.set_str(str, base) == 0;
	}

	static inline void toStr(std::string& str, const mpz_class& z, int base = 10)
	{
		str = z.get_str(base);
	}

	static inline void add(mpz_class& z, const mpz_class& x, const mpz_class& y)
	{
		mpz_add(z.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
	}

	static inline void sub(mpz_class& z, const mpz_class& x, const mpz_class& y)
	{
		mpz_sub(z.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
	}

	static inline void mul(mpz_class& z, const mpz_class& x, const mpz_class& y)
	{
		mpz_mul(z.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
	}

	static inline void divmod(mpz_class& q, mpz_class& r, const mpz_class& x, const mpz_class& y)
	{
		mpz_divmod(q.get_mpz_t(), r.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
	}

	static inline void div(mpz_class& q, const mpz_class& x, const mpz_class& y)
	{
		mpz_div(q.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
	}

	static inline void mod(mpz_class& r, const mpz_class& x, const mpz_class& m)
	{
		mpz_mod(r.get_mpz_t(), x.get_mpz_t(), m.get_mpz_t());
	}

	static inline void clear(mpz_class& z)
	{
		mpz_set_ui(z.get_mpz_t(), 0);
	}
	static inline bool isZero(const mpz_class& z)
	{
		return mpz_sgn(z.get_mpz_t()) == 0;
	}

	static inline bool isNegative(const mpz_class& z)
	{
		return mpz_sgn(z.get_mpz_t()) < 0;
	}
	static inline void neg(mpz_class& z, const mpz_class& x)
	{
		mpz_neg(z.get_mpz_t(), x.get_mpz_t());
	}

	static inline int compare(const mpz_class& x, const mpz_class & y)
	{
		return mpz_cmp(x.get_mpz_t(), y.get_mpz_t());
	}

	static inline void addMod(mpz_class& z, const mpz_class& x, const mpz_class& y, const mpz_class& m)
	{
		add(z, x, y);
		mpz_class t;
		sub(t, z, m);
		if (!isNegative(t)) {
			z = t;
		}
	}

	static inline void subMod(mpz_class& z, const mpz_class& x, const mpz_class& y, const mpz_class& m)
	{
		sub(z, x, y);
		if (!isNegative(z)) return;
		add(z, z, m);
	}

	static inline void mulMod(mpz_class& z, const mpz_class& x, const mpz_class& y, const mpz_class& m)
	{
		mul(z, x, y);
		mod(z, z, m);
	}

	// z = x^y mod m
	static inline void powMod(mpz_class& z, const mpz_class& x, const mpz_class& y, const mpz_class& m)
	{
		mpz_powm(z.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t(), m.get_mpz_t());
	}

	// z = 1/x mod m
	static inline void invMod(mpz_class& z, const mpz_class& x, const mpz_class& m)
	{
		mpz_invert(z.get_mpz_t(), x.get_mpz_t(), m.get_mpz_t());
	}

	// z = lcm(x, y)
	static inline void lcm(mpz_class& z, const mpz_class& x, const mpz_class& y)
	{
		mpz_lcm(z.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
	}

	static inline bool isPrime(const mpz_class& x)
	{
		return mpz_probab_prime_p(x.get_mpz_t(), 10) != 0;
	}

	static inline size_t getBitLen(const mpz_class& x)
	{
		return mpz_sizeinbase(x.get_mpz_t(), 2);
	}
};

} // mie::gmp

