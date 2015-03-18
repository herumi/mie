#define MIE_USE_LLVM
#include <mie/fp_base.hpp>
#include <cybozu/test.hpp>
#include <cybozu/benchmark.hpp>
#include <cybozu/xorshift.hpp>
#include <cybozu/bit_operation.hpp>
#include <mie/fp_util.hpp>
#include <mie/fp2.hpp>

#if (CYBOZU_HOST == CYBOZU_HOST_INTEL) && (CYBOZU_OS_BIT == 64)
	#define USE_XBYAK
	#include <mie/fp_generator.hpp>
	static mie::FpGenerator fg;
#endif
#define PUT(x) std::cout << #x "=" << (x) << std::endl

const size_t MAX_N = 32;
typedef mie::fp::Unit Unit;

size_t getUnitN(size_t bitLen)
{
	return (bitLen + sizeof(Unit) * 8 - 1) / (sizeof(Unit) * 8);
}

void setMpz(mpz_class& mx, const Unit *x, size_t n)
{
	mie::Gmp::setRaw(mx, x, n);
}
void getMpz(Unit *x, size_t n, const mpz_class& mx)
{
	mie::fp::local::toArray(x,  n, mx.get_mpz_t());
}

void put(const char *msg, const Unit *x, size_t n)
{
	printf("%s ", msg);
	for (size_t i = 0; i < n; i++) printf("%016llx ", (long long)x[n - 1 - i]);
	printf("\n");
}
void verifyEqual(const Unit *x, const Unit *y, size_t n, const char *file, int line)
{
	bool ok = mie::fp::local::isEqualArray(x, y, n);
	CYBOZU_TEST_ASSERT(ok);
	if (ok) return;
	printf("%s:%d\n", file, line);
	put("L", x, n);
	put("R", y, n);
	exit(1);
}
#define VERIFY_EQUAL(x, y, n) verifyEqual(x, y, n, __FILE__, __LINE__)

void addC(Unit *z, const Unit *x, const Unit *y, const Unit *p, size_t n)
{
	mpz_class mx, my, mp;
	setMpz(mx, x, n);
	setMpz(my, y, n);
	setMpz(mp, p, n);
	mx += my;
	if (mx >= mp) mx -= mp;
	getMpz(z, n, mx);
}
void subC(Unit *z, const Unit *x, const Unit *y, const Unit *p, size_t n)
{
	mpz_class mx, my, mp;
	setMpz(mx, x, n);
	setMpz(my, y, n);
	setMpz(mp, p, n);
	mx -= my;
	if (mx < 0) mx += mp;
	getMpz(z, n, mx);
}

void test(const Unit *p, size_t bitLen)
{
	printf("test bitLen=%d\n", (int)bitLen);
	const size_t n = getUnitN(bitLen);
#ifdef NDEBUG
	bool doBench = true;
#else
	bool doBench = false;
#endif
	typedef void (*void4op)(Unit*, const Unit*, const Unit*, const Unit*);
	void4op addS, addL, subS, subL;

	if (bitLen <= 128) {
		addS = mie_fp_add128S;
		addL = mie_fp_add128L;
		subS = mie_fp_sub128S;
		subL = mie_fp_sub128L;
	} else
	if (bitLen <= 192) {
		addS = mie_fp_add192S;
		addL = mie_fp_add192L;
		subS = mie_fp_sub192S;
		subL = mie_fp_sub192L;
	} else
	if (bitLen <= 256) {
		addS = mie_fp_add256S;
		addL = mie_fp_add256L;
		subS = mie_fp_sub256S;
		subL = mie_fp_sub256L;
	} else
	if (bitLen <= 384) {
		addS = mie_fp_add384S;
		addL = mie_fp_add384L;
		subS = mie_fp_sub384S;
		subL = mie_fp_sub384L;
	} else
	if (bitLen <= 576) {
		addS = mie_fp_add576S;
		addL = mie_fp_add576L;
		subS = mie_fp_sub576S;
		subL = mie_fp_sub576L;
	} else {
		puts("ERR"); exit(1);
	}

	mie::fp::Unit x[MAX_N], y[MAX_N];
	mie::fp::Unit z[MAX_N], w[MAX_N];
	cybozu::XorShift rg;
	mie::fp::getRandVal(x, rg, p, bitLen);
	mie::fp::getRandVal(y, rg, p, bitLen);
	const size_t C = 10;

	addC(z, x, y, p, n);
	addS(w, x, y, p);
	VERIFY_EQUAL(z, w, n);
	for (size_t i = 0; i < C; i++) {
printf("i=%d\n", (int)i);
		addC(z, y, z, p, n);
		addS(w, y, w, p);
		VERIFY_EQUAL(z, w, n);
		addC(z, y, z, p, n);
		addL(w, y, w, p);
		VERIFY_EQUAL(z, w, n);
		subC(z, x, z, p, n);
		subS(w, x, w, p);
		VERIFY_EQUAL(z, w, n);
		subC(z, x, z, p, n);
		subL(w, x, w, p);
		VERIFY_EQUAL(z, w, n);
	}
	if (doBench) {
		CYBOZU_BENCH("addS", addS, x, y, x, p);
		CYBOZU_BENCH("addL", addL, x, y, x, p);
		CYBOZU_BENCH("subS", subS, x, y, x, p);
		CYBOZU_BENCH("subL", subL, x, y, x, p);
	}
#ifdef USE_XBYAK
	if (bitLen <= 128) return;
	if (doBench) {
		fg.init(p, n);
		CYBOZU_BENCH("add-asm", fg.add_, x, y, x);
		CYBOZU_BENCH("sub-asm", fg.sub_, x, y, x);
	}
#endif
}

CYBOZU_TEST_AUTO(all)
{
	const struct {
		size_t n;
		const uint64_t p[9];
	} tbl[] = {
		{ 2, { 0xf000000000000001, 1 } },
		{ 3, { 0x0f69466a74defd8d,  0xfffffffe26f2fc17,  0xffffffffffffffff,  } },
		{ 3, { 0x7900342423332197,  0x1234567890123456,  0x1480948109481904,  } },
		{ 3, { 0x0f69466a74defd8d,  0xfffffffe26f2fc17,  0x07ffffffffffffff,  } },
		{ 4, { 0xa700000000000013,  0x6121000000000013,  0xba344d8000000008,  0x2523648240000001,  } },
		{ 4, { 0x7900342423332197,  0x4242342420123456,  0x1234567892342342,  0x1480948109481904,  } },
		{ 4, { 0x0f69466a74defd8d,  0xfffffffe26f2fc17,  0x17ffffffffffffff,  0x1513423423423415,  } },
		{ 6, { 0x00000000ffffffff,  0xffffffff00000000,  0xfffffffffffffffe,  0xffffffffffffffff,  0xffffffffffffffff,  0xffffffffffffffff,  } },
		{ 6, { 0x4720422423332197,  0x0034230847204720,  0x3456789012345679,  0x4820984290482212,  0x9482094820948209,  0x0194810841094810,  } },
		{ 6, { 0x7204224233321972,  0x0342308472047204,  0x4567890123456790,  0x0948204204243123,  0x2098420984209482,  0x2093482094810948,  } },
		{ 9, { 0xffffffffffffffff,  0xffffffffffffffff,  0xffffffffffffffff,  0xffffffffffffffff,  0xffffffffffffffff,  0xffffffffffffffff,  0xffffffffffffffff,  0xffffffffffffffff,  0x00000000000001ff,  } },
		{ 9, { 0x4720422423332197,  0x0034230847204720,  0x3456789012345679,  0x2498540975555312,  0x9482904924029424,  0x0948209842098402,  0x1098410948109482,  0x0820958209582094,  0x0000000000000029,  } },
		{ 9, { 0x0f69466a74defd8d,  0xfffffffe26f2fc17,  0x7fffffffffffffff,  0x8572938572398583,  0x5732057823857293,  0x9820948205872380,  0x3409238420492034,  0x9483842098340298,  0x0000000000000003,  } },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const size_t n = tbl[i].n;
		const size_t bitLen = (n - 1) * 64 + cybozu::bsr<uint64_t>(tbl[i].p[n - 1]);
		test((const Unit*)tbl[i].p, bitLen);
	}
}

