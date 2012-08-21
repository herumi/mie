#include <mie/paillier.hpp>
#include <iostream>

int main()
	try
{
	mie::paillier::PrivateKey prv;
	prv.init(1024);
	const mie::paillier::PublicKey& pub = prv.getPublicKey();
	mpz_class a = 9;
	mpz_class b = 8;
	std::cout << "a=" << a << std::endl;
	std::cout << "b=" << b << std::endl;
	mpz_class ea, eb;
	pub.enc(ea, a);
	pub.enc(eb, b);
	std::cout << "ea=" << ea << std::endl;
	std::cout << "eb=" << eb << std::endl;
	mpz_class c;
	mpz_class eab;
	pub.mul(eab, ea, eb);
	prv.dec(c, eab);
	std::cout << "c=" << c << std::endl;
	std::cout << "a+b=" << (a + b) << std::endl;
} catch (std::exception& e) {
	std::cerr << "err=" << e.what() << std::endl;
	return 1;
}


