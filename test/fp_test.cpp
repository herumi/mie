#include <cybozu/test.hpp>
#include <mie/fp.hpp>
#include <mie/gmp_util.hpp>

typedef mie::FpT<mie::Gmp> Fp;

CYBOZU_TEST_AUTO(cstr)
{
	const struct {
		const char *str;
		int val;
	} tbl[] = {
		{ "0", 0 },
		{ "1", 1 },
		{ "123", 123 },
		{ "-9999", -9999 },
		{ "0x123", 0x123 },
		{ "0b10101", 21 },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		// string cstr
		Fp x(tbl[i].str);
		CYBOZU_TEST_EQUAL(x, tbl[i].val);

		// int cstr
		Fp y(tbl[i].val);
		CYBOZU_TEST_EQUAL(y, x);

		// copy cstr
		Fp z(x);
		CYBOZU_TEST_EQUAL(z, x);

		// assign int
		Fp w;
		w = tbl[i].val;
		CYBOZU_TEST_EQUAL(w, x);

		// assign self
		Fp u;
		u = w;
		CYBOZU_TEST_EQUAL(u, x);

		// conv
		std::ostringstream os;
		os << tbl[i].val;

		std::string str;
		x.toStr(str);
		CYBOZU_TEST_EQUAL(str, os.str());
	}
}

CYBOZU_TEST_AUTO(conv)
{
	const char *bin = "0b100100011010001010110011110001001000000010010001101000101011001111000100100000001001000110100010101100111100010010000";
	const char *hex = "0x123456789012345678901234567890";
	const char *dec = "94522879687365475552814062743484560";
	Fp b(bin);
	Fp h(hex);
	Fp d(dec);
	CYBOZU_TEST_EQUAL(b, h);
	CYBOZU_TEST_EQUAL(b, d);

	std::string str;
	b.toStr(str, 2);
	CYBOZU_TEST_EQUAL(str, bin);
	b.toStr(str);
	CYBOZU_TEST_EQUAL(str, dec);
	b.toStr(str, 16);
	CYBOZU_TEST_EQUAL(str, hex);
}

CYBOZU_TEST_AUTO(compare)
{
	const struct {
		int lhs;
		int rhs;
		int cmp;
	} tbl[] = {
		{ 0, 0, 0 },
		{ 1, 0, 1 },
		{ 0, 1, -1 },
		{ -1, 0, -1 },
		{ 0, -1, 1 },
		{ 123, 456, -1 },
		{ 456, 123, 1 },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const Fp x(tbl[i].lhs);
		const Fp y(tbl[i].rhs);
		const int cmp = tbl[i].cmp;
		if (cmp == 0) {
			CYBOZU_TEST_EQUAL(x, y);
			CYBOZU_TEST_ASSERT(x >= y);
			CYBOZU_TEST_ASSERT(x <= y);
		} else if (cmp > 0) {
			CYBOZU_TEST_ASSERT(x > y);
			CYBOZU_TEST_ASSERT(x >= y);
		} else {
			CYBOZU_TEST_ASSERT(x < y);
			CYBOZU_TEST_ASSERT(x <= y);
		}
	}
}


const int m = 65537;

CYBOZU_TEST_AUTO(modulo)
{
	std::ostringstream ms;
	ms << m;
	Fp::setModulo(ms.str());

	std::string str;
	Fp::getModulo(str);
	CYBOZU_TEST_EQUAL(str, ms.str());
}

CYBOZU_TEST_AUTO(ope)
{
	const struct {
		int x;
		int y;
		int add; // x + y
		int sub; // x - y
		int mul; // x * y
	} tbl[] = {
		{ 0, 1, 1, m - 1, 0 },
		{ 9, 5, 14, 4, 45 },
		{ 10, 13, 23, m - 3, 130 },
		{ 2000, 1000, 3000, 1000, (2000 * 1000) % m },
		{ 12345, 9999, 12345 + 9999, 12345 - 9999, (12345 * 9999) % m },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const Fp x(tbl[i].x);
		const Fp y(tbl[i].y);
		Fp z;
		Fp::add(z, x, y);
		CYBOZU_TEST_EQUAL(z, tbl[i].add);
		Fp::sub(z, x, y);
		CYBOZU_TEST_EQUAL(z, tbl[i].sub);
		Fp::mul(z, x, y);
		CYBOZU_TEST_EQUAL(z, tbl[i].mul);

		Fp r;
		Fp::inv(r, y);
		Fp::mul(z, z, r);
		CYBOZU_TEST_EQUAL(z, tbl[i].x);

		z = x + y;
		CYBOZU_TEST_EQUAL(z, tbl[i].add);
		z = x - y;
		CYBOZU_TEST_EQUAL(z, tbl[i].sub);
		z = x * y;
		CYBOZU_TEST_EQUAL(z, tbl[i].mul);

		z = x / y;
		z *= y;
		CYBOZU_TEST_EQUAL(z, tbl[i].x);
	}
}

