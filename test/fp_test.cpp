#include <cybozu/test.hpp>
#include <mie/fp.hpp>
#include <mie/gmp_util.hpp>

typedef mie::FpT<mpz_class> Fp;

CYBOZU_TEST_AUTO(conv_str)
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
	for (size_t i = 0; CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		Fp x(tbl[i].str);
		CYBOZU_TEST_EQUAL(x, tbl[i].val);
		std::ostringstream os;
		os << tbl[i].val;

		std::string str;
		x.toStr(str);
		CYBOZU_TEST_EQUAL(str, os.str());
	}
}

