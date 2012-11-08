#include <cybozu/test.hpp>
#include <mie/fp.hpp>
#include <mie/gmp_util.hpp>
#include <mie/ec.hpp>
#include <mie/ecparam.hpp>

typedef mie::FpT<mie::Gmp> Fp;
typedef mie::ECA<Fp> EC;

const mie::ECparam& para = mie::ecparam::secp192k1;

struct Init {
	Init()
	{
		Fp::setModulo(para.p);
		EC::setParam(para.a, para.b);
	}
};

CYBOZU_TEST_SETUP_FIXTURE(Init);

CYBOZU_TEST_AUTO(cstr)
{
	EC O;
	CYBOZU_TEST_ASSERT(O.isZero());
	EC P;
	EC::neg(P, O);
	CYBOZU_TEST_EQUAL(P, O);
}

CYBOZU_TEST_AUTO(ope)
{
	Fp x(para.gx);
	Fp y(para.gy);
	Fp n(para.n);
	CYBOZU_TEST_ASSERT(EC::isValid(x, y));
	EC P(x, y);
	EC Q;
	EC::neg(Q, P);
	CYBOZU_TEST_EQUAL(Q.x, P.x);
	CYBOZU_TEST_EQUAL(Q.y, -P.y);

	EC R = P + Q;
	CYBOZU_TEST_ASSERT(R.isZero());
	EC O;
	R = P + O;
	CYBOZU_TEST_EQUAL(R, P);
	R = O + P;
	CYBOZU_TEST_EQUAL(R, P);

	{
		EC::dbl(R, P);
		EC R2 = P + P;
		CYBOZU_TEST_EQUAL(R, R2);
		EC R3L = R2 + P;
		EC R3R = P + R2;
		CYBOZU_TEST_EQUAL(R3L, R3R);
		EC::power(R, P, 2);
		CYBOZU_TEST_EQUAL(R, R2);
		EC R4L = R3L + R2;
		EC R4R = R2 + R3L;
		CYBOZU_TEST_EQUAL(R4L, R4R);
		EC::power(R, P, 5);
		CYBOZU_TEST_EQUAL(R, R4L);
	}
	{
		R = P;
		for (int i = 0; i < 10; i++) {
			R += P;
		}
		EC R2;
		EC::power(R2, P, 11);
		CYBOZU_TEST_EQUAL(R, R2);
	}
	EC::power(R, P, n - 1);
	CYBOZU_TEST_EQUAL(R, -P);
	EC::power(R, P, n);
	CYBOZU_TEST_ASSERT(R.isZero());
}

CYBOZU_TEST_AUTO(power)
{
	Fp x(para.gx);
	Fp y(para.gy);
	Fp n(para.n);
	EC P(x, y);
	EC Q;
	EC R;
	for (int i = 0; i < 100; i++) {
		EC::power(Q, P, i);
		CYBOZU_TEST_EQUAL(Q, R);
		R += P;
	}
}

CYBOZU_TEST_AUTO(neg_power)
{
	Fp x(para.gx);
	Fp y(para.gy);
	Fp n(para.n);
	EC P(x, y);
	EC Q;
	EC R;
	for (int i = 0; i < 100; i++) {
		EC::power(Q, P, -i);
		CYBOZU_TEST_EQUAL(Q, R);
		R -= P;
	}
}

CYBOZU_TEST_AUTO(power_fp)
{
	Fp x(para.gx);
	Fp y(para.gy);
	Fp n(para.n);
	EC P(x, y);
	EC Q;
	EC R;
	for (int i = 0; i < 100; i++) {
		EC::power(Q, P, Fp(i));
		CYBOZU_TEST_EQUAL(Q, R);
		R += P;
	}
}
