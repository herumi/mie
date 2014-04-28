/*
	sample of Elliptic Curve Diffie-Hellman key sharing
*/
#include <iostream>
#include <fstream>
#include <cybozu/random_generator.hpp>
#include <mie/fp.hpp>
#include <mie/gmp_util.hpp>
#include <mie/ecparam.hpp>
#if defined(_WIN64) || defined(__x86_64__)
#define USE_MONT_FP
#endif
#ifdef USE_MONTFP
#include <mie/mont_fp.hpp>
typedef mie::MontFpT<3> Fp;
#else
typedef mie::FpT<mie::Gmp> Fp;
#endif

struct ZnTag;

typedef mie::EcT<Fp> Ec;
typedef mie::FpT<mie::Gmp, ZnTag> Zn;

int main()
{
	cybozu::RandomGenerator rg;
	/*
		system setup with a parameter secp192k1 recommended by SECG
		Ec is an elliptic curve over Fp
		the cyclic group of <P> is isomorphic to Zn
	*/
	const mie::EcParam& para = mie::ecparam::secp192k1;
	Zn::setModulo(para.n);
	Fp::setModulo(para.p);
	Ec::setParam(para.a, para.b);
	const Ec P(Fp(para.gx), Fp(para.gy));
	const size_t len = para.bitLen;

	/*
		Alice setups a private key a and public key aP
	*/
	Zn a;
	Ec aP;

	a.initRand(rg, len); // random
	Ec::power(aP, P, a); // aP = a * P;

	std::cout << "aP=" << aP << std::endl;

	/*
		Bob setups a private key b and public key bP
	*/
	Zn b;
	Ec bP;

	b.initRand(rg, len); // random
	Ec::power(bP, P, b); // bP = b * P;

	std::cout << "bP=" << bP << std::endl;

	Ec abP, baP;

	// Alice uses bP(B's public key) and a(A's priavte key)
	Ec::power(abP, bP, a); // abP = a * (bP)

	// Bob uses aP(A's public key) and b(B's private key)
	Ec::power(baP, aP, b); // baP = b * (aP)

	if (abP == baP) {
		std::cout << "key sharing succeed." << abP << std::endl;
	} else {
		std::cout << "ERR(not here)" << std::endl;
	}
}

