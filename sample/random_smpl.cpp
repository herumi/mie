#include <mie/fp.hpp>
#include <mie/gmp_util.hpp>
#include <mie/ecparam.hpp>
#include <cybozu/random_generator.hpp>
#include <map>

#ifdef USE_MONT_FP
#include <mie/mont_fp.hpp>
typedef mie::MontFpT<3> Fp;
#else
typedef mie::FpT<mie::Gmp> Fp;
#endif

typedef std::map<std::string, int> Map;

int main(int argc, char *argv[])
{
#ifdef USE_MONT_FP
	puts("use MontFp");
#else
	puts("use GMP");
#endif
	cybozu::RandomGenerator rg;
	const char *p = mie::ecparam::secp192k1.p;
	if (argc == 2) {
		p = argv[1];
	}
	Fp::setModulo(p);
	Fp x;
	printf("p=%s\n", p);
	Map m;
	for (int i = 0; i < 10000; i++) {
		x.setRand(rg);
		m[x.toStr(16)]++;
	}
	for (Map::const_iterator i = m.begin(), ie = m.end(); i != ie; ++i) {
		printf("%s %d\n", i->first.c_str(), i->second);
	}
}
