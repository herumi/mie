#define PUT(x) std::cout << #x "=" << (x) << std::endl
#include <cybozu/test.hpp>
#include <cybozu/benchmark.hpp>
#include <mie/fp.hpp>
#include <mie/gmp_util.hpp>
#include <time.h>
#include <mie/mont_fp.hpp>

typedef mie::FpT<mie::Gmp> Zn;
typedef mie::MontFpT<4> MontFp4;
typedef mie::MontFpT<3> MontFp3;

struct Montgomery {
	typedef mie::Gmp::BlockType BlockType;
	mpz_class p_;
	mpz_class R_; // (1 << (pn_ * 64)) % p
	mpz_class RR_; // (R * R) % p
	BlockType pp_; // p * pp = -1 mod M = 1 << 64
	size_t pn_;
	Montgomery() {}
	explicit Montgomery(const mpz_class& p)
	{
		p_ = p;
		pp_ = mie::montgomery::getCoff(mie::Gmp::getBlock(p, 0));
		pn_ = mie::Gmp::getBlockSize(p);
		R_ = 1;
		R_ = (R_ << (pn_ * 64)) % p_;
		RR_ = (R_ * R_) % p_;
	}

	void toMont(mpz_class& x) const { mul(x, x, RR_); }
	void fromMont(mpz_class& x) const { mul(x, x, 1); }

	void mul(mpz_class& z, const mpz_class& x, const mpz_class& y) const
	{
#if 0
		const size_t ySize = mie::Gmp::getBlockSize(y);
		mpz_class c = x * mie::Gmp::getBlock(y, 0);
		BlockType q = mie::Gmp::getBlock(c, 0) * pp_;
		c += p_ * q;
		c >>= sizeof(BlockType) * 8;
		for (size_t i = 1; i < pn_; i++) {
			if (i < ySize) {
				c += x * mie::Gmp::getBlock(y, i);
			}
			BlockType q = mie::Gmp::getBlock(c, 0) * pp_;
			c += p_ * q;
			c >>= sizeof(BlockType) * 8;
		}
		if (c >= p_) {
			c -= p_;
		}
		z = c;
#else
		z = x * y;
		for (size_t i = 0; i < pn_; i++) {
			BlockType q = mie::Gmp::getBlock(z, 0) * pp_;
			z += p_ * q;
			z >>= sizeof(BlockType) * 8;
		}
		if (z >= p_) {
			z -= p_;
		}
#endif
	}
};

template<class T>
mpz_class toGmp(const T& x)
{
	std::string str = x.toStr();
	mpz_class t;
	mie::Gmp::fromStr(t, str);
	return t;
}

template<class T>
std::string toStr(const T& x)
{
	std::ostringstream os;
	os << x;
	return os.str();
}

template<class T, class U>
T castTo(const U& x)
{
	T t;
	t.fromStr(toStr(x));
	return t;
}

template<class T>
void putRaw(const T& x)
{
	const uint64_t *p = x.getInnerValue();
	for (size_t i = 0, n = T::BlockSize; i < n; i++) {
		printf("%016llx", p[n - 1 - i]);
	}
	printf("\n");
}

template<size_t N>
struct Test {
	typedef mie::MontFpT<N> Fp;
	mpz_class m;
	void run(const char *p)
	{
		Fp::setModulo(p);
		m = p;
		Zn::setModulo(p);
		edge();
		cstr();
		fromStr();
		stream();
		conv();
		compare();
		modulo();
		ope();
		power();
		neg_power();
		power_Zn();
		setRaw();
		set64bit();
		getRaw();
		bench();
	}
	void cstr()
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
	void toStr()
	{
		Fp x(0);
		std::string str;
		str = x.toStr();
		CYBOZU_TEST_EQUAL(str, "0");
		str = x.toStr(2, true);
		CYBOZU_TEST_EQUAL(str, "0");
		str = x.toStr(2, false);
		CYBOZU_TEST_EQUAL(str, "0");
		str = x.toStr(16, true);
		CYBOZU_TEST_EQUAL(str, "0");
		str = x.toStr(16, false);
		CYBOZU_TEST_EQUAL(str, "0");

		x = 123;
		str = x.toStr();
		CYBOZU_TEST_EQUAL(str, "123");
		str = x.toStr(2, true);
		CYBOZU_TEST_EQUAL(str, "0b1111011");
		str = x.toStr(2, false);
		CYBOZU_TEST_EQUAL(str, "1111011");
		str = x.toStr(16, true);
		CYBOZU_TEST_EQUAL(str, "0x7b");
		str = x.toStr(16, false);
		CYBOZU_TEST_EQUAL(str, "7b");

		{
			std::ostringstream os;
			os << x;
			CYBOZU_TEST_EQUAL(os.str(), "123");
		}
		{
			std::ostringstream os;
			os << std::hex << std::showbase << x;
			CYBOZU_TEST_EQUAL(os.str(), "0x7b");
		}
		{
			std::ostringstream os;
			os << std::hex << x;
			CYBOZU_TEST_EQUAL(os.str(), "7b");
		}
	}

	void fromStr()
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

	void stream()
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
	void edge()
	{
		std::cout << std::hex;
		/*
			real mont
			   0    0
			   1    R^-1
			   R    1
			  -1    -R^-1
			  -R    -1
		*/
		mpz_class t = 1;
		const mpz_class R = (t << (Fp::BlockSize * 64)) % m;
		const mpz_class tbl[] = {
			0, 1, R, m - 1, m - R
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			const mpz_class& x = tbl[i];
			for (size_t j = i; j < CYBOZU_NUM_OF_ARRAY(tbl); j++) {
				const mpz_class& y = tbl[j];
				mpz_class z = (x * y) % m;
				Fp xx, yy;
				Fp::toMont(xx, x);
				Fp::toMont(yy, y);
				Fp zz = xx * yy;
				mpz_class t;
				Fp::fromMont(t, zz);
				CYBOZU_TEST_EQUAL(z, t);
			}
		}
		std::cout << std::dec;
	}

	void conv()
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
		b.toStr(str, 2, true);
		CYBOZU_TEST_EQUAL(str, bin);
		b.toStr(str);
		CYBOZU_TEST_EQUAL(str, dec);
		b.toStr(str, 16, true);
		CYBOZU_TEST_EQUAL(str, hex);
	}

	void compare()
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
			{ 5, 5, 0 },
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			const Fp x(tbl[i].lhs);
			const Fp y(tbl[i].rhs);
			const int cmp = tbl[i].cmp;
			if (cmp == 0) {
				CYBOZU_TEST_EQUAL(x, y);
			} else {
				CYBOZU_TEST_ASSERT(x != y);
			}
		}
	}

	void modulo()
	{
		std::ostringstream ms;
		ms << m;

		std::string str;
		Fp::getModulo(str);
		CYBOZU_TEST_EQUAL(str, ms.str());
	}

	void ope()
	{
		const struct {
			Zn x;
			Zn y;
			Zn add; // x + y
			Zn sub; // x - y
			Zn mul; // x * y
		} tbl[] = {
			{ 0, 1, 1, -1, 0 },
			{ 9, 7, 16, 2, 63 },
			{ 10, 13, 23, -3, 130 },
			{ 2000, -1000, 1000, 3000, -2000000 },
			{ -12345, -9999, -(12345 + 9999), - 12345 + 9999, 12345 * 9999 },
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			const Fp x(castTo<Fp>(tbl[i].x));
			const Fp y(castTo<Fp>(tbl[i].y));
			Fp z;
			Fp::add(z, x, y);
			CYBOZU_TEST_EQUAL(z, castTo<Fp>(tbl[i].add));
			Fp::sub(z, x, y);
			CYBOZU_TEST_EQUAL(z, castTo<Fp>(tbl[i].sub));
			Fp::mul(z, x, y);
			CYBOZU_TEST_EQUAL(z, castTo<Fp>(tbl[i].mul));

			Fp r;
			Fp::inv(r, y);
			Zn rr = 1 / tbl[i].y;
			CYBOZU_TEST_EQUAL(r, castTo<Fp>(rr));
			Fp::mul(z, z, r);
			CYBOZU_TEST_EQUAL(z, castTo<Fp>(tbl[i].x));

			z = x + y;
			CYBOZU_TEST_EQUAL(z, castTo<Fp>(tbl[i].add));
			z = x - y;
			CYBOZU_TEST_EQUAL(z, castTo<Fp>(tbl[i].sub));
			z = x * y;
			CYBOZU_TEST_EQUAL(z, castTo<Fp>(tbl[i].mul));

			z = x / y;
			z *= y;
			CYBOZU_TEST_EQUAL(z, castTo<Fp>(tbl[i].x));
		}
	}

	void power()
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

	void neg_power()
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

	void power_Zn()
	{
		Fp x, y, z;
		x = 12345;
		z = 1;
		for (int i = 0; i < 100; i++) {
			Fp::power(y, x, Zn(i));
			CYBOZU_TEST_EQUAL(y, z);
			z *= x;
		}
	}

	void setRaw()
	{
		// QQQ
#if 0
		char b1[] = { 0x56, 0x34, 0x12 };
		Fp x;
		x.setRaw(b1, 3);
		CYBOZU_TEST_EQUAL(x, 0x123456);
		int b2[] = { 0x12, 0x34 };
		x.setRaw(b2, 2);
		CYBOZU_TEST_EQUAL(x, Fp("0x3400000012"));
#endif
	}

	void set64bit()
	{
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

	void getRaw()
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
			for (size_t j = 0; j < n; j++) {
				CYBOZU_TEST_EQUAL(buf[j], tbl[i].v[j]);
			}
		}
	}
	void bench()
	{
		Fp x("12345678901234567900342423332197");
		Fp y("0x7ffffffffffffffffffffffe26f2fc170f69466a74defd8d");
		CYBOZU_BENCH("add", operator+, x, x);
		CYBOZU_BENCH("sub", operator-, x, y);
		CYBOZU_BENCH("mul", operator*, x, x);
		CYBOZU_BENCH("div", operator/, x, y);
	}
};

void customTest(const char *pStr, const char *xStr, const char *yStr)
{
	std::string rOrg, rC, rAsm;
	Zn::setModulo(pStr);
	Zn s(xStr), t(yStr);
	s *= t;
	rOrg = toStr(s);
	{
		puts("C");
		mpz_class p(pStr);
		Montgomery mont(p);
		mpz_class x(xStr), y(yStr);
		mont.toMont(x);
		mont.toMont(y);
		mpz_class z;
		mont.mul(z, x, y);
		mont.fromMont(z);
		rC = toStr(z);
	}

	puts("asm");
	MontFp3::setModulo(pStr);
	MontFp3 x(xStr), y(yStr);
	x *= y;
	rAsm = toStr(x);
	CYBOZU_TEST_EQUAL(rOrg, rC);
	CYBOZU_TEST_EQUAL(rOrg, rAsm);
}

CYBOZU_TEST_AUTO(customTest)
{
	const struct {
		const char *p;
		const char *x;
		const char *y;
	} tbl[] = {
		{
			"0xfffffffffffffffffffffffffffffffffffffffeffffee37",
			"6277101735386680763835789423207666416102355444459739541045",
			"6277101735386680763835789423207666416102355444459739540047"
		},
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		customTest(tbl[i].p, tbl[i].x, tbl[i].y);
	}
}

CYBOZU_TEST_AUTO(test3)
{
	Test<3> test;
	const char *tbl[] = {
		"0x000000000000000100000000000000000000000000000033", // min prime
		"0x00000000fffffffffffffffffffffffffffffffeffffac73",
		"0x0000000100000000000000000001b8fa16dfab9aca16b6b3",
		"0x000000010000000000000000000000000000000000000007",
		"0x30000000000000000000000000000000000000000000002b",
		"0x70000000000000000000000000000000000000000000001f",
		"0x800000000000000000000000000000000000000000000005",
		"0xfffffffffffffffffffffffffffffffffffffffeffffee37",
		"0xfffffffffffffffffffffffe26f2fc170f69466a74defd8d",
		"0xffffffffffffffffffffffffffffffffffffffffffffff13", // max prime
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		printf("prime=%s\n", tbl[i]);
		test.run(tbl[i]);
	}
}

CYBOZU_TEST_AUTO(test4)
{
	Test<4> test;
	const char *tbl[] = {
		"0x0000000000000001000000000000000000000000000000000000000000000085", // min prime
		"0x2523648240000001ba344d80000000086121000000000013a700000000000013",
		"0x7523648240000001ba344d80000000086121000000000013a700000000000017",
		"0x800000000000000000000000000000000000000000000000000000000000005f",
		"0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff43", // max prime
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		printf("prime=%s\n", tbl[i]);
		test.run(tbl[i]);
	}
}

CYBOZU_TEST_AUTO(toStr16)
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
	MontFp3::setModulo("0xffffffffffffffffffffffffffffffffffffffffffffff13");
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		std::string str, str2;
		MontFp3 x(tbl[i]);
		x.toStr(str, 16);
		mpz_class y(tbl[i]);
		mie::Gmp::toStr(str2, y, 16);
		CYBOZU_TEST_EQUAL(str, str2);
	}
}

CYBOZU_TEST_AUTO(fromStr16)
{
	const struct {
		const char *str;
		uint64_t x[4];
	} tbl[] = {
		{ "0", { 0, 0, 0, 0 } },
		{ "5", { 5, 0, 0, 0 } },
		{ "123", { 0x123, 0, 0, 0 } },
		{ "123456789012345679adbc", { uint64_t(0x789012345679adbcull), 0x123456, 0, 0 } },
		{ "ffffffff26f2fc170f69466a74defd8d", { uint64_t(0x0f69466a74defd8dull), uint64_t(0xffffffff26f2fc17ull), 0, 0 } },
		{ "100000000000000000000000000000033", { uint64_t(0x0000000000000033ull), 0, 1, 0 } },
		{ "11ee12312312940000000000000000000000000002342343", { uint64_t(0x0000000002342343ull), uint64_t(0x0000000000000000ull), uint64_t(0x11ee123123129400ull), 0 } },
		{ "1234567890abcdefABCDEF123456789aba32134723424242424", { uint64_t(0x2134723424242424ull), uint64_t(0xDEF123456789aba3ull), uint64_t(0x4567890abcdefABCull), 0x123 } },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const size_t xN = 4;
		uint64_t x[xN];
		mie::fp::fromStr16(x, xN, tbl[i].str, strlen(tbl[i].str));
		for (size_t j = 0; j < xN; j++) {
			CYBOZU_TEST_EQUAL(x[j], tbl[i].x[j]);
		}
	}
}

#if 0
CYBOZU_TEST_AUTO(toStr16bench)
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
	const int C = 500000;
	MontFp3::setModulo("0xffffffffffffffffffffffffffffffffffffffffffffff13");
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		std::string str, str2;
		MontFp3 x(tbl[i]);
		CYBOZU_BENCH_C("Mont:toStr", C, x.toStr, str, 16);
		mpz_class y(tbl[i]);
		CYBOZU_BENCH_C("Gmp:toStr ", C, mie::Gmp::toStr, str2, y, 16);
		str2.insert(0, "0x");
		CYBOZU_TEST_EQUAL(str, str2);
	}
}

CYBOZU_TEST_AUTO(fromStr16bench)
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
	const int C = 500000;
	MontFp3::setModulo("0xffffffffffffffffffffffffffffffffffffffffffffff13");
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		std::string str = tbl[i];
		MontFp3 x;
		CYBOZU_BENCH_C("Mont:fromStr", C, x.fromStr, str);

		mpz_class y;
		str.erase(0, 2);
		CYBOZU_BENCH_C("Gmp:fromStr ", C, mie::Gmp::fromStr, y, str, 16);
		x.toStr(str, 16);
		std::string str2;
		mie::Gmp::toStr(str2, y, 16);
		str2.insert(0, "0x");
		CYBOZU_TEST_EQUAL(str, str2);
	}
}
#endif
