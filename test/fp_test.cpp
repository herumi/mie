#include <cybozu/test.hpp>
#include <mie/fp.hpp>
#include <mie/gmp_util.hpp>
#include <time.h>

typedef mie::FpT<mie::Gmp> Fp;

const int m = 65537;
struct Init {
	Init()
	{
		std::ostringstream ms;
		ms << m;
		Fp::setModulo(ms.str());
	}
};

CYBOZU_TEST_SETUP_FIXTURE(Init);

CYBOZU_TEST_AUTO(cstr)
{
	const struct {
		const char *str;
		int val;
	} tbl[] = {
		{ "0", 0 },
		{ "1", 1 },
		{ "123", 123 },
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
	CYBOZU_TEST_EXCEPTION_MESSAGE(Fp("-123"), cybozu::Exception, "fromStr");
}

CYBOZU_TEST_AUTO(bitLen)
{
	const struct {
		const char *str;
		size_t len;
	} tbl[] = {
		{ "0", 1 },
		{ "1", 1 },
		{ "2", 2 },
		{ "3", 2 },
		{ "0xffff", 16 },
		{ "0xffffffffffff", 48 },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		Fp x(tbl[i].str);
		CYBOZU_TEST_EQUAL(x.getBitLen(), tbl[i].len);
	}
}

CYBOZU_TEST_AUTO(fromStr)
{
	const struct {
		const char *in;
		int out;
		int base;
	} tbl[] = {
		{ "100", 100, 0 }, // set base = 10 if base = 0
		{ "100", 4, 2 },
		{ "100", 256, 16 },
		{ "0b100", 4, 0 },
		{ "0b100", 4, 2 },
		{ "0b100", 4, 16 }, // ignore base
		{ "0x100", 256, 0 },
		{ "0x100", 256, 2 }, // ignore base
		{ "0x100", 256, 16 },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		Fp x;
		x.fromStr(tbl[i].in, tbl[i].base);
		CYBOZU_TEST_EQUAL(x, tbl[i].out);
	}
}

CYBOZU_TEST_AUTO(stream)
{
	const struct {
		const char *in;
		int out10;
		int out16;
	} tbl[] = {
		{ "100", 100, 256 }, // set base = 10 if base = 0
		{ "0b100", 4, 4 },
		{ "0x100", 256, 256 }, // ignore base
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		{
			std::istringstream is(tbl[i].in);
			Fp x;
			is >> x;
			CYBOZU_TEST_EQUAL(x, tbl[i].out10);
		}
		{
			std::istringstream is(tbl[i].in);
			Fp x;
			is >> std::hex >> x;
			CYBOZU_TEST_EQUAL(x, tbl[i].out16);
		}
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
		{ -1, 0, 1 }, // m-1, 0
		{ 0, -1, -1 }, // 0, m-1
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
	{
		Fp x(5);
		CYBOZU_TEST_ASSERT(x < 10);
		CYBOZU_TEST_ASSERT(x == 5);
		CYBOZU_TEST_ASSERT(x > 2);
	}
}

CYBOZU_TEST_AUTO(modulo)
{
	std::ostringstream ms;
	ms << m;

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

CYBOZU_TEST_AUTO(power)
{
	Fp x, y, z;
	x = 12345;
	z = 1;
	for (int i = 0; i < 100; i++) {
		Fp::power(y, x, i);
		CYBOZU_TEST_EQUAL(y, z);
		z *= x;
	}
}

CYBOZU_TEST_AUTO(neg_power)
{
	Fp x, y, z;
	x = 12345;
	z = 1;
	Fp rx = 1 / x;
	for (int i = 0; i < 100; i++) {
		Fp::power(y, x, -i);
		CYBOZU_TEST_EQUAL(y, z);
		z *= rx;
	}
}

CYBOZU_TEST_AUTO(power_fp)
{
	Fp x, y, z;
	x = 12345;
	z = 1;
	for (int i = 0; i < 100; i++) {
		Fp::power(y, x, Fp(i));
		CYBOZU_TEST_EQUAL(y, z);
		z *= x;
	}
}

struct TagAnother;

CYBOZU_TEST_AUTO(another)
{
	typedef mie::FpT<mie::Gmp, TagAnother> G;
	G::setModulo("13");
	G a = 3;
	G b = 9;
	a *= b;
	CYBOZU_TEST_EQUAL(a, 1);
}

CYBOZU_TEST_AUTO(setRaw)
{
	Fp::setModulo("100000000000000");
	char b1[] = { 0x56, 0x34, 0x12 };
	Fp x;
	x.setRaw(b1, 3);
	CYBOZU_TEST_EQUAL(x, 0x123456);
	int b2[] = { 0x12, 0x34 };
	x.setRaw(b2, 2);
	CYBOZU_TEST_EQUAL(x, Fp("0x3400000012"));
	Fp::setModulo("65537");
	int b3 = 65540;
	x.setRaw(&b3, 1);
	CYBOZU_TEST_EQUAL(x, 3);
}

CYBOZU_TEST_AUTO(set64bit)
{
	Fp::setModulo("0x10000000000000000000");
	const struct {
		const char *p;
		uint64_t i;
	} tbl[] = {
		{ "0x1234567812345678", uint64_t(0x1234567812345678ull) },
		{ "0xaaaaaaaaaaaaaaaa", uint64_t(0xaaaaaaaaaaaaaaaaull) },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		Fp x(tbl[i].p);
		Fp y(tbl[i].i);
		CYBOZU_TEST_EQUAL(x, y);
	}
}

#ifdef NDEBUG
CYBOZU_TEST_AUTO(bench)
{
	Fp::setModulo("0xfffffffffffffffffffffffe26f2fc170f69466a74defd8d");
	const int N = 5000000;
	Fp x("12345678901234567900342423332197");
	double base = 0;
	{
		clock_t begin = clock();
		for (int i = 0; i < N; i++) {
			x += x;
		}
		clock_t end = clock();
		base = (end - begin) / double(CLOCKS_PER_SEC) / N * 1e9;
		printf("add %7.2fnsec\n", base);
	}
	{
		clock_t begin = clock();
		for (int i = 0; i < N; i++) {
			x += x;
		}
		clock_t end = clock();
		double t = (end - begin) / double(CLOCKS_PER_SEC) / N * 1e9;
		printf("add %7.2fnsec(x%5.2f)\n", t, t / base);
	}
	{
		Fp y("0x7ffffffffffffffffffffffe26f2fc170f69466a74defd8d");
		clock_t begin = clock();
		for (int i = 0; i < N; i++) {
			x -= y;
		}
		clock_t end = clock();
		double t = (end - begin) / double(CLOCKS_PER_SEC) / N * 1e9;
		printf("sub %7.2fnsec(x%5.2f)\n", t, t / base);
	}
	{
		clock_t begin = clock();
		for (int i = 0; i < N; i++) {
			x *= x;
		}
		clock_t end = clock();
		double t = (end - begin) / double(CLOCKS_PER_SEC) / N * 1e9;
		printf("mul %7.2fnsec(x%5.2f)\n", t, t / base);
	}
	{
		Fp y("0xfffffffffffffffffffffe26f2fc170f69466a74defd8d");
		clock_t begin = clock();
		for (int i = 0; i < N; i++) {
			x /= y;
		}
		clock_t end = clock();
		double t = (end - begin) / double(CLOCKS_PER_SEC) / N * 1e9;
		printf("div %7.2fnsec(x%5.2f)\n", t, t / base);
	}
}
#endif

