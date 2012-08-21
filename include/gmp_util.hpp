#pragma once
/**
    @file
    @brief util function for gmp
    @author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://www.opensource.org/licenses/bsd-license.php
*/
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

// z = x^y mod n
inline void powMod(mpz_class& z, const mpz_class& x, const mpz_class& y, const mpz_class& n)
{
	mpz_powm(z.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t(), n.get_mpz_t());
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

// z = 1/x mod n
inline void invMod(mpz_class& z, const mpz_class& x, const mpz_class& n)
{
	mpz_invert(z.get_mpz_t(), x.get_mpz_t(), n.get_mpz_t());
}

// z = [buf[n-1]:..:buf[1]:buf[0]]
// eg. buf[] = {0x12345678, 0xaabbccdd}; => z = 0xaabbccdd12345678;
template<class T>
void setRaw(mpz_class& z, const T *buf, size_t n)
{
	mpz_import(z.get_mpz_t(), n, -1, sizeof(*buf), 0, 0, buf);
}

inline size_t getBitLen(const mpz_class& x)
{
	return mpz_sizeinbase(x.get_mpz_t(), 2);
}

} // mie

