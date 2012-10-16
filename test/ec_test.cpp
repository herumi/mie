#include <cybozu/test.hpp>
#include <mie/fp.hpp>
#include <mie/gmp_util.hpp>
#include <mie/ec.hpp>

typedef mie::FpT<mie::Gmp> Fp;
typedef mie::ECA<Fp> EC;

const struct mie::ECparam secp192k1 = {
	"0xfffffffffffffffffffffffffffffffffffffffeffffee37",
	"0",
	"3",
	"0xdb4ff10ec057e9ae26b07d0280b7f4341da5d1b1eae06c7d",
	"0x9b2f2f6d9c5628a7844163d015be86344082aa88d95e2f9d",
	"0xfffffffffffffffffffffffe26f2fc170f69466a74defd8d"
};

struct Init {
	Init()
	{
		Fp::setModulo(secp192k1.p);
		EC::setParam(secp192k1.a, secp192k1.b);
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
	Fp x(secp192k1.gx);
	Fp y(secp192k1.gy);
	Fp n(secp192k1.n);
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
	Fp x(secp192k1.gx);
	Fp y(secp192k1.gy);
	Fp n(secp192k1.n);
	CYBOZU_TEST_ASSERT(EC::isValid(x, y));
	EC P(x, y);
	EC Q;
	EC::power(Q, P, 0);
	CYBOZU_TEST_ASSERT(Q.isZero());
	EC::power(Q, P, 1);
	CYBOZU_TEST_EQUAL(P, Q);
	EC R;
	EC::power(Q, P, 2);
	R = P + P;
	CYBOZU_TEST_EQUAL(Q, R);
	EC::power(Q, P, 3);
	R += P;
	CYBOZU_TEST_EQUAL(Q, R);
}

