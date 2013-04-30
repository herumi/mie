#include <mie/gmp_util.hpp>
#include <mie/fp.hpp>
#include <mie/fp_generator.hpp>
#include <iostream>
#include <time.h>

typedef mie::FpT<mie::Gmp> Fp;

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

void bench(const char *pStr)
{
	const int N = 500000;
	Fp::setModulo(pStr);
	const char *xStr = "2345678901234567900342423332197";

	std::cout << std::hex;
	{
		Xbyak::util::Clock clk;
		Fp x(xStr);
		clk.begin();
		for (int i = 0; i < N; i++) {
			x *= x;
		}
		clk.end();
		printf("mul  %4.fclk ", clk.getClock() / double(N));
		std::cout << x << std::endl;
	}
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
		std::cout << "0x" << x << std::endl;
	}
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
		if (mie::Gmp::getRaw(pa, aN, p) != 4) return; // now only for size = 4
		mie::FpGenerator fg;
		mie::FpGenerator::void3op montMul = fg.getCurr<mie::FpGenerator::void3op>();
		fg.gen_montMul4(pa, mont.pp_);
		clk.begin();
		for (int i = 0; i < N; i++) {
			montMul(xa, xa, xa);
		}
		clk.end();
		mie::Gmp::setRaw(x, xa, aN);
		mont.fromMont(x);
		printf("mont %4.fclk ", clk.getClock() / double(N));
		std::cout << "0x" << x << std::endl;
	}
}

int main()
{
	const char *tbl[] = {
		"0x2523648240000001ba344d80000000086121000000000013a700000000000013",
		"0xfffffffffffffffffffffffffffffffffffffffeffffee37",
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		printf("i=%d\n", (int)i);
		bench(tbl[i]);
	}
}
