#include <iostream>
#include <mie/paillier.hpp>
#include <cybozu/test.hpp>
#include <sstream>

CYBOZU_TEST_AUTO(getRand)
{
	cybozu::RandomGenerator rg;
	for (size_t bitLen = 62; bitLen < 66; bitLen++) {
		for (int i = 0; i < 100; i++) {
			mpz_class x;
			mie::Gmp::getRandPrime(x, bitLen, rg);
			CYBOZU_TEST_EQUAL(bitLen, mie::Gmp::getBitLen(x));
		}
	}
}

CYBOZU_TEST_AUTO(getRandPrime)
{
	cybozu::RandomGenerator rg;

	for (size_t bitLen = 126; bitLen < 130; bitLen++) {
		for (int i = 0; i < 100; i++) {
			mpz_class x, y, z;
			mie::Gmp::getRandPrime(x, bitLen, rg, true);
			mie::Gmp::getRandPrime(y, bitLen, rg, true);
			CYBOZU_TEST_EQUAL(bitLen, mie::Gmp::getBitLen(x));
			CYBOZU_TEST_EQUAL(bitLen, mie::Gmp::getBitLen(y));
			z = x * y;
			CYBOZU_TEST_EQUAL(mie::Gmp::getBitLen(z), bitLen * 2);
		}
	}
}

CYBOZU_TEST_AUTO(paillier)
{
	mpz_class m2("234567890123456789011223344554");
	mpz_class m1("123456789012345678901122334455");

	mie::paillier::PrivateKey prv;
	prv.init(1024);

	const mie::paillier::PublicKey& pub = prv.getPublicKey();

	{
		mpz_class enc;
		mpz_class dec;
		pub.enc(enc, m1);
		prv.dec(dec, enc);
		CYBOZU_TEST_EQUAL(m1, dec);
	}
	{
		mpz_class e1, e2;
		mpz_class d1, d2;
		pub.enc(e1, m1);
		pub.enc(e2, m2);
		mpz_class esum;
		pub.mul(esum, e1, e2);
		mpz_class c;
		prv.dec(c, esum);
		CYBOZU_TEST_EQUAL(c, m1 + m2);
	}
}

CYBOZU_TEST_AUTO(save_load)
{
	mie::paillier::PrivateKey prv;

	// setup
	{
		prv.init(1024);
		std::ostringstream os;
		os << prv;

		std::istringstream is(os.str());

		mie::paillier::PrivateKey prv2;
		is >> prv2;
		CYBOZU_TEST_EQUAL(prv, prv2);
	}

	mpz_class msg("123456789012345678901122334455");
	mpz_class enc;

	// enc
	{
		const mie::paillier::PublicKey& pub = prv.getPublicKey();

		std::ostringstream os;
		os << pub;
		std::istringstream is(os.str());

		mie::paillier::PublicKey pub2;
		is >> pub2;
		CYBOZU_TEST_EQUAL(pub, pub2);

		pub2.enc(enc, msg);
	}

	// dec
	mpz_class dec;
	prv.dec(dec, enc);
	CYBOZU_TEST_EQUAL(msg, dec);
}
