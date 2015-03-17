#define MIE_USE_LLVM
#include <mie/fp_base.hpp>
#include <cybozu/test.hpp>
#include <cybozu/benchmark.hpp>
#include <cybozu/xorshift.hpp>
#include <cybozu/bit_operation.hpp>
#include <mie/fp_util.hpp>

#ifdef NDEBUG
void benchSub(const mie::fp::Unit *p, size_t bitLen)
{
	mie::fp::Unit x[32], y[32];
	cybozu::XorShift rg;
	mie::fp::getRandVal(x, rg, p, bitLen);
	mie::fp::getRandVal(y, rg, p, bitLen);
	CYBOZU_BENCH("add128S", mie_fp_add128S, x, y, x, p);
	CYBOZU_BENCH("add128L", mie_fp_add128L, x, y, x, p);
}
CYBOZU_TEST_AUTO(bench)
{
	const struct {
		size_t n;
		const uint64_t p[9];
	} tbl[] = {
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
		benchSub(tbl[i].p, bitLen);
	}
}
#endif

