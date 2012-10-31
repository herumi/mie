#include <cybozu/test.hpp>
#include <mie/math.hpp>

CYBOZU_TEST_AUTO(fpclassify)
{
	union di {
		unsigned long long i;
		double d;
	};
	const struct {
		unsigned long long i;
		int c;
		bool isfinite;
		bool isnormal;
		bool isnan;
		bool isinf;
	} tbl[] = {
		{ 0x0000000000000000ULL, FP_ZERO, true, false, false, false }, // +0
		{ 0x8000000000000000ULL, FP_ZERO, true, false, false, false }, // -0
		{ 0x3ff0000000000000ULL, FP_NORMAL, true, true, false, false }, // +1
		{ 0xbff0000000000000ULL, FP_NORMAL, true, true, false, false }, // -1
		{ 0x0010000000000000ULL, FP_NORMAL, true, true, false, false }, //  DBL_MIN
		{ 0x8010000000000000ULL, FP_NORMAL, true, true, false, false }, // -DBL_MIN
		{ 0x7fefffffffffffffULL, FP_NORMAL, true, true, false, false }, //  DBL_MAX
		{ 0xffefffffffffffffULL, FP_NORMAL, true, true, false, false }, // -DBL_MAX
		{ 0x000fffffffffffffULL, FP_SUBNORMAL, true, false, false, false }, //  largest subnormal
		{ 0x800fffffffffffffULL, FP_SUBNORMAL, true, false, false, false }, // -largest subnormal
		{ 0x0000000000000001ULL, FP_SUBNORMAL, true, false, false, false }, //  small subnormal
		{ 0x8000000000000001ULL, FP_SUBNORMAL, true, false, false, false }, // -small subnormal

		{ 0x7ff0000000000000ULL, FP_INFINITE, false, false, false, true, }, // +inf
		{ 0xfff0000000000000ULL, FP_INFINITE, false, false, false, true, }, // -inf
		{ 0x7ff0000000000001ULL, FP_NAN, false, false, true, false }, //  signaling nan
		{ 0x7ff7ffffffffffffULL, FP_NAN, false, false, true, false }, //  signaling nan
		{ 0xfff7ffffffffffffULL, FP_NAN, false, false, true, false }, // -signaling nan
		{ 0x7ff8000000000000ULL, FP_NAN, false, false, true, false }, //  quiet nan
		{ 0xfff8000000000000ULL, FP_NAN, false, false, true, false }, //  quiet nan
		{ 0x7fffffffffffffffULL, FP_NAN, false, false, true, false }, //  quiet nan
		{ 0xffffffffffffffffULL, FP_NAN, false, false, true, false }, // -quiet nan
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		di di;
		di.i = tbl[i].i;
		double x = di.d;
		int c = fpclassify(x);
		CYBOZU_TEST_EQUAL(c, tbl[i].c);
		CYBOZU_TEST_EQUAL(!!isfinite(x), tbl[i].isfinite);
		CYBOZU_TEST_EQUAL(!!isnormal(x), tbl[i].isnormal);
		CYBOZU_TEST_EQUAL(!!isnan(x), tbl[i].isnan);
		CYBOZU_TEST_EQUAL(!!isinf(x), tbl[i].isinf);
	}
}
