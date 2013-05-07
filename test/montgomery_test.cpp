#include <mie/gmp_util.hpp>
#include <mie/fp.hpp>
#include <mie/fp_generator.hpp>
#include <iostream>
#include <time.h>
#include <sstream>
#include <cybozu/test.hpp>
#include <mie/mont_fp.hpp>

typedef mie::FpT<mie::Gmp> Fp;
typedef mie::MontFpT<4> MontFp4;
typedef mie::MontFpT<3> MontFp3;

template<class T>
void putRaw(const T& x)
{
	const uint64_t *p = x.getInnerValue();
	for (size_t i = 0, n = T::BlockSize; i < n; i++) {
		printf("%016llx", p[n - 1 - i]);
	}
	printf("\n");
}

#define PUT(x) std::cout << #x << '=' << x << std::endl

struct Montgomery {
	typedef mie::Gmp::BlockType BlockType;
	mpz_class p_;
	mpz_class R_;
	mpz_class RR_;
	BlockType pp_; // p * pp = -1 mod R
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

void test()
{
	const char *pStr = "0xfffffffffffffffffffffffffffffffffffffffeffffee37";
	const char *xStr = "6277101735386680763835789423207666416102355444459739541045";
	const char *yStr = "6277101735386680763835789423207666416102355444459739540047";
	Fp::setModulo(pStr);
	Fp s(xStr), t(yStr);
	s *= t;
	std::cout << s << std::endl;
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
		std::cout << z << std::endl;
	}

	puts("asm");
	MontFp3::setModulo(pStr);
	MontFp3 x(xStr), y(yStr);
	x *= y;
	std::cout << "x=" << x << std::endl;
}

void bench(const char *pStr)
{
	const int N = 500000;
	Fp::setModulo(pStr);
	const char *xStr = "2345678901234567900342423332197";

	std::cout << std::hex;
	std::string ret1;
	{
		Xbyak::util::Clock clk;
		Fp x(xStr);
		clk.begin();
		for (int i = 0; i < N; i++) {
			x *= x;
		}
		clk.end();
		printf("mul  %4.fclk ", clk.getClock() / double(N));
		std::ostringstream os;
		os << std::hex << x;
		ret1 = os.str();
		std::cout << ret1 << std::endl;
	}
	std::string ret2;
	{
		Xbyak::util::Clock clk;
		mpz_class p(pStr);
		Montgomery mont(p);
		mpz_class x(xStr);
		mont.toMont(x);
		clk.begin();
		for (int i = 0; i < N; i++) {
			mont.mul(x, x, x);
		}
		clk.end();
		mont.fromMont(x);
		printf("mont %4.fclk ", clk.getClock() / double(N));
		std::ostringstream os;
		os << std::hex << "0x" << x;
		ret2 = os.str();
		std::cout << ret2 << std::endl;
	}
	std::string ret3;
	{
		Xbyak::util::Clock clk;
		mpz_class p(pStr);
		Montgomery mont(p);
		mpz_class x(xStr);
		mont.toMont(x);
		const size_t aN = 4;
		uint64_t xa[aN];
		uint64_t pa[aN];
		mie::Gmp::getRaw(xa, aN, x);
		const size_t n = mie::Gmp::getRaw(pa, aN, p);
		mie::FpGenerator fg;
		fg.init(pa, n);
		mie::FpGenerator::void3op montMul = fg.mul_;

		clk.begin();
		for (int i = 0; i < N; i++) {
			montMul(xa, xa, xa);
		}
		clk.end();
		mie::Gmp::setRaw(x, xa, aN);
		mont.fromMont(x);
		printf("mont %4.fclk ", clk.getClock() / double(N));
		std::ostringstream os;
		os << std::hex << "0x" << x;
		ret3 = os.str();
		std::cout << ret3 << std::endl;
	}
	std::string ret4;
	{
		Xbyak::util::Clock clk;
		mpz_class p(pStr);
		uint64_t xa[4];
		size_t len = mie::Gmp::getRaw(xa, 4, p);

		std::ostringstream os;
		os << std::hex;
		switch (len) {
		case 3:
			{
				MontFp3::setModulo(pStr);
				MontFp3 x(xStr);
				clk.begin();
				for (int i = 0; i < N; i++) {
					x *= x;
				}
				clk.end();
				printf("mul  %4.fclk ", clk.getClock() / double(N));
				os << x;
			}
			break;
		case 4:
			{
				MontFp4::setModulo(pStr);
				MontFp4 x(xStr);
				clk.begin();
				for (int i = 0; i < N; i++) {
					x *= x;
				}
				clk.end();
				printf("mul  %4.fclk ", clk.getClock() / double(N));
				os << x;
			}
			break;
		default:
			printf("not support %d\n", (int)len);
			break;
		}
		ret1 = os.str();
		std::cout << ret1 << std::endl;
	}
	CYBOZU_TEST_EQUAL(ret1, ret2);
	CYBOZU_TEST_EQUAL(ret1, ret3);
}

CYBOZU_TEST_AUTO(main)
{
	test();
	const char *tbl[] = {
		"0x2523648240000001ba344d80000000086121000000000013a700000000000013",
		"0xf523648240000001ba344d80000000086121000000000013a700000000000013",
		"0xfffffffffffffffffffffffffffffffffffffffeffffee37",
		"0x7ffffffffffffffffffffffffffffffffffffffeffffee37",
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		printf("i=%d\n", (int)i);
		bench(tbl[i]);
	}
}
