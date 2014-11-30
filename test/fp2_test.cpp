#define PUT(x) std::cout << #x "=" << (x) << std::endl
#include <cybozu/test.hpp>
#include <mie/fp2.hpp>
#include <cybozu/benchmark.hpp>
#include <time.h>

#ifdef _MSC_VER
	#pragma warning(disable: 4127) // const condition
#endif

typedef mie::FpT<9> Fp;

CYBOZU_TEST_AUTO(cstr)
{
	const int m = 65535;
	Fp::setModulo("65535");
	const struct {
		const char *str;
		int val;
	} tbl[] = {
		{ "0", 0 },
		{ "1", 1 },
		{ "123", 123 },
		{ "0x123", 0x123 },
		{ "0b10101", 21 },
		{ "-123", m - 123 },
		{ "-0x123", m - 0x123 },
		{ "-0b10101", m - 21 },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		// string cstr
#if 0
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
#endif
	}
}

#if 0
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
		{ "0x100", 256, 0 },
		{ "0x100", 256, 16 },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		Fp x;
		x.fromStr(tbl[i].in, tbl[i].base);
		CYBOZU_TEST_EQUAL(x, tbl[i].out);
	}
	// conflict prefix with base
	Fp x;
	CYBOZU_TEST_EXCEPTION(x.fromStr("0b100", 16), cybozu::Exception);
	CYBOZU_TEST_EXCEPTION(x.fromStr("0x100", 2), cybozu::Exception);
}

CYBOZU_TEST_AUTO(stream)
{
	const struct {
		const char *in;
		int out10;
		int out16;
	} tbl[] = {
		{ "100", 100, 256 }, // set base = 10 if base = 0
		{ "0x100", 256, 256 },
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
	std::istringstream is("0b100");
	Fp x;
	CYBOZU_TEST_EXCEPTION(is >> std::hex >> x, cybozu::Exception);
}

CYBOZU_TEST_AUTO(conv)
{
	const char *bin = "0b1001000110100";
	const char *hex = "0x1234";
	const char *dec = "4660";
	Fp b(bin);
	Fp h(hex);
	Fp d(dec);
	CYBOZU_TEST_EQUAL(b, h);
	CYBOZU_TEST_EQUAL(b, d);

	std::string str;
	b.toStr(str, 2, true);
	CYBOZU_TEST_EQUAL(str, bin);
	b.toStr(str);
	CYBOZU_TEST_EQUAL(str, dec);
	b.toStr(str, 16, true);
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
		int sqr; // x^2
	} tbl[] = {
		{ 0, 1, 1, m - 1, 0, 0 },
		{ 9, 5, 14, 4, 45, 81 },
		{ 10, 13, 23, m - 3, 130, 100 },
		{ 2000, 1000, 3000, 1000, (2000 * 1000) % m, (2000 * 2000) % m },
		{ 12345, 9999, 12345 + 9999, 12345 - 9999, (12345 * 9999) % m, (12345 * 12345) % m },
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

		Fp::square(z, x);
		CYBOZU_TEST_EQUAL(z, tbl[i].sqr);

		z = x / y;
		z *= y;
		CYBOZU_TEST_EQUAL(z, tbl[i].x);
	}
}

struct tag2;

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
	typedef mie::FpT<mie::Gmp, tag2> Fp2;
	Fp2::setModulo("1000");
	x = 5;
	Fp2 n = 3;
	z = 3;
	Fp::power(x, x, z);
	CYBOZU_TEST_EQUAL(x, 125);
	x = 5;
	Fp::power(x, x, n);
	CYBOZU_TEST_EQUAL(x, 125);
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
	Fp::setModulo("1000000000000000000000");
	char b1[] = { 0x56, 0x34, 0x12 };
	Fp x;
	x.setRaw(b1, 3);
	CYBOZU_TEST_EQUAL(x, 0x123456);
	int b2[] = { 0x12, 0x34 };
	x.setRaw(b2, 2);
	CYBOZU_TEST_EQUAL(x, Fp("0x3400000012"));
	x.fromStr("0xffffffffffff");
	CYBOZU_TEST_EQUAL(x.getBitLen(), 48u);

	Fp::setModulo("0x1000000000000123456789");
	const struct {
		uint32_t buf[3];
		size_t bufN;
		const char *expected;
	} tbl[] = {
		{ { 0x23456788, 0x00000001, 0x00100000}, 1, "0x23456788" },
		{ { 0x23456788, 0x00000001, 0x00100000}, 2, "0x123456788" },
		{ { 0x23456788, 0x00000001, 0x00100000}, 3, "0x1000000000000123456788" },
		{ { 0x23456789, 0x00000001, 0x34100000}, 3, "0" },
		{ { 0x2345678a, 0x00000001, 0x99100000}, 3, "1" },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		x.setRaw(tbl[i].buf, tbl[i].bufN);
		CYBOZU_TEST_EQUAL(x, Fp(tbl[i].expected));
	}
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

CYBOZU_TEST_AUTO(getRaw)
{
	const struct {
		const char *s;
		uint32_t v[4];
		size_t vn;
	} tbl[] = {
		{ "0", { 0, 0, 0, 0 }, 1 },
		{ "1234", { 1234, 0, 0, 0 }, 1 },
		{ "0xaabbccdd12345678", { 0x12345678, 0xaabbccdd, 0, 0 }, 2 },
		{ "0x11112222333344445555666677778888", { 0x77778888, 0x55556666, 0x33334444, 0x11112222 }, 4 },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		mpz_class x(tbl[i].s);
		const size_t bufN = 8;
		uint32_t buf[bufN];
		size_t n = mie::Gmp::getRaw(buf, bufN, x);
		CYBOZU_TEST_EQUAL(n, tbl[i].vn);
		CYBOZU_TEST_EQUAL_ARRAY(buf, tbl[i].v, n);
	}
}

CYBOZU_TEST_AUTO(toStr)
{
	const char *tbl[] = {
		"0x0",
		"0x5",
		"0x123",
		"0x123456789012345679adbc",
		"0xffffffff26f2fc170f69466a74defd8d",
		"0x100000000000000000000000000000033",
		"0x11ee12312312940000000000000000000000000002342343"
	};
	Fp::setModulo("0xfffffffffffffffffffffffe26f2fc170f69466a74defd8d");
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		mpz_class x(tbl[i]);
		Fp y(tbl[i]);
		std::string xs, ys;
		mie::Gmp::toStr(xs, x, 16);
		y.toStr(ys, 16);
		CYBOZU_TEST_EQUAL(xs, ys);
	}
	{
		Fp x;
		x = 12345;
		uint64_t y = x.cvtInt();
		CYBOZU_TEST_EQUAL(y, 12345);
		x.fromStr("123456789012342342342342342");
		CYBOZU_TEST_EXCEPTION(x.cvtInt(), cybozu::Exception);
		bool err = false;
		CYBOZU_TEST_NO_EXCEPTION(x.cvtInt(&err));
		CYBOZU_TEST_ASSERT(err);
	}
}

CYBOZU_TEST_AUTO(binaryRepl)
{
	const struct {
		const char *s;
		size_t n;
		uint32_t v[6];
	} tbl[] = {
		{ "0", 0, { 0, 0, 0, 0, 0 } },
		{ "1234", 1, { 1234, 0, 0, 0, 0 } },
		{ "0xaabbccdd12345678", 2, { 0x12345678, 0xaabbccdd, 0, 0, 0 } },
		{ "0x11112222333344445555666677778888", 4, { 0x77778888, 0x55556666, 0x33334444, 0x11112222, 0 } },
		{ "0x9911112222333344445555666677778888", 5, { 0x77778888, 0x55556666, 0x33334444, 0x11112222, 0x99, 0 } },
	};
	Fp::setModulo("0xfffffffffffffffffffffffe26f2fc170f69466a74defd8d");
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		Fp x(tbl[i].s);
		cybozu::BitVector bv;
		x.appendToBitVec(bv);
		CYBOZU_TEST_EQUAL(bv.size(), Fp::getModBitLen());
		CYBOZU_TEST_EQUAL(bv.size(), Fp::getBitVecSize());
		const Fp::BlockType *block = bv.getBlock();
		if (sizeof(Fp::BlockType) == 4) {
			CYBOZU_TEST_EQUAL(bv.getBlockSize(), tbl[i].n);
			CYBOZU_TEST_EQUAL_ARRAY(block, tbl[i].v, tbl[i].n);
		} else {
			const size_t n = (tbl[i].n + 1) / 2;
			for (size_t j = 0; j < n; j++) {
				uint64_t v = (uint64_t(tbl[i].v[j * 2 + 1]) << 32) | tbl[i].v[j * 2];
				CYBOZU_TEST_EQUAL(block[j], v);
			}
		}
		Fp y;
		y.fromBitVec(bv);
		CYBOZU_TEST_EQUAL(x, y);
	}
}
#endif
