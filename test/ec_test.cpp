#include <cybozu/test.hpp>
#include <mie/fp.hpp>
#include <mie/gmp_util.hpp>
#include <mie/ec.hpp>
#include <mie/ecparam.hpp>
#include <time.h>

typedef mie::FpT<mie::Gmp> Fp;
typedef mie::EcT<Fp> Ec;

const mie::EcParam& para = mie::ecparam::secp192k1;

struct Init {
	Init()
	{
		Fp::setModulo(para.p);
		Ec::setParam(para.a, para.b);
	}
};

CYBOZU_TEST_SETUP_FIXTURE(Init);

CYBOZU_TEST_AUTO(cstr)
{
	Ec O;
	CYBOZU_TEST_ASSERT(O.isZero());
	Ec P;
	Ec::neg(P, O);
	CYBOZU_TEST_EQUAL(P, O);
}

CYBOZU_TEST_AUTO(ope)
{
	Fp x(para.gx);
	Fp y(para.gy);
	Fp n(para.n);
	CYBOZU_TEST_ASSERT(Ec::isValid(x, y));
	Ec P(x, y);
	Ec Q;
	Ec::neg(Q, P);
	CYBOZU_TEST_EQUAL(Q.x, P.x);
	CYBOZU_TEST_EQUAL(Q.y, -P.y);

	Ec R = P + Q;
	CYBOZU_TEST_ASSERT(R.isZero());
	Ec O;
	R = P + O;
	CYBOZU_TEST_EQUAL(R, P);
	R = O + P;
	CYBOZU_TEST_EQUAL(R, P);

	{
		Ec::dbl(R, P);
		Ec R2 = P + P;
		CYBOZU_TEST_EQUAL(R, R2);
		Ec R3L = R2 + P;
		Ec R3R = P + R2;
		CYBOZU_TEST_EQUAL(R3L, R3R);
		Ec::power(R, P, 2);
		CYBOZU_TEST_EQUAL(R, R2);
		Ec R4L = R3L + R2;
		Ec R4R = R2 + R3L;
		CYBOZU_TEST_EQUAL(R4L, R4R);
		Ec::power(R, P, 5);
		CYBOZU_TEST_EQUAL(R, R4L);
	}
	{
		R = P;
		for (int i = 0; i < 10; i++) {
			R += P;
		}
		Ec R2;
		Ec::power(R2, P, 11);
		CYBOZU_TEST_EQUAL(R, R2);
	}
	Ec::power(R, P, n - 1);
	CYBOZU_TEST_EQUAL(R, -P);
	Ec::power(R, P, n);
	CYBOZU_TEST_ASSERT(R.isZero());
}

CYBOZU_TEST_AUTO(power)
{
	Fp x(para.gx);
	Fp y(para.gy);
	Fp n(para.n);
	Ec P(x, y);
	Ec Q;
	Ec R;
	for (int i = 0; i < 100; i++) {
		Ec::power(Q, P, i);
		CYBOZU_TEST_EQUAL(Q, R);
		R += P;
	}
}

CYBOZU_TEST_AUTO(neg_power)
{
	Fp x(para.gx);
	Fp y(para.gy);
	Fp n(para.n);
	Ec P(x, y);
	Ec Q;
	Ec R;
	for (int i = 0; i < 100; i++) {
		Ec::power(Q, P, -i);
		CYBOZU_TEST_EQUAL(Q, R);
		R -= P;
	}
}

CYBOZU_TEST_AUTO(power_fp)
{
	Fp x(para.gx);
	Fp y(para.gy);
	Fp n(para.n);
	Ec P(x, y);
	Ec Q;
	Ec R;
	for (int i = 0; i < 100; i++) {
		Ec::power(Q, P, Fp(i));
		CYBOZU_TEST_EQUAL(Q, R);
		R += P;
	}
}

template<class F>
void test(F f, const char *msg)
{
	const int N = 1000000;
	Fp x(para.gx);
	Fp y(para.gy);
	Fp n(para.n);
	Ec P(x, y);
	Ec Q = P + P + P;
	clock_t begin = clock();
	for (int i = 0; i < N; i++) {
		f(Q, P, Q);
	}
	clock_t end = clock();
	printf("%s %.2fusec\n", msg, (end - begin) / double(CLOCKS_PER_SEC) / N * 1e6);
}
/*
	add 8.71usec -> 6.94
	sub 6.80usec -> 4.84
	dbl 9.59usec -> 7.75
	pos 2730usec -> 2153
*/
CYBOZU_TEST_AUTO(addsub_bench)
{
	test(Ec::add, "add");
	test(Ec::sub, "sub");
}

CYBOZU_TEST_AUTO(dbl_bench)
{
	const int N = 1000000;
	Fp x(para.gx);
	Fp y(para.gy);
	Fp n(para.n);
	Ec P(x, y);
	clock_t begin = clock();
	for (int i = 0; i < N; i++) {
		Ec::dbl(P, P);
	}
	clock_t end = clock();
	printf("dbl %.2fusec\n", (end - begin) / double(CLOCKS_PER_SEC) / N * 1e6);
}

CYBOZU_TEST_AUTO(power_bench)
{
	const int N = 1000;
	Fp x(para.gx);
	Fp y(para.gy);
	Fp n(para.n);
	Ec P(x, y);
	Fp z("0x9b2f2f6d9c5628a7844163d015be86344082aa88d95e2f9");
	clock_t begin = clock();
	for (int i = 0; i < N; i++) {
		Ec::power(P, P, z);
	}
	clock_t end = clock();
	printf("pow %.2fusec\n", (end - begin) / double(CLOCKS_PER_SEC) / N * 1e6);
}
