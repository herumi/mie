#include <map>
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

extern "C" void mie_fp_mont128(Unit* z, const Unit* x,  const Unit* y, const Unit* p, size_t r);

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

struct Montgomery {
	typedef mie::Gmp::BlockType BlockType;
	mpz_class p_;
	mpz_class R_; // (1 << (n_ * 64)) % p
	mpz_class RR_; // (R * R) % p
	BlockType r_; // p * r = -1 mod M = 1 << 64
	size_t n_;
	Montgomery() {}
	explicit Montgomery(const mpz_class& p)
	{
		p_ = p;
		r_ = mie::montgomery::getCoff(mie::Gmp::getBlock(p, 0));
		n_ = mie::Gmp::getBlockSize(p);
		R_ = 1;
		R_ = (R_ << (n_ * 64)) % p_;
		RR_ = (R_ * R_) % p_;
	}

	void toMont(mpz_class& x) const { mul(x, x, RR_); }
	void fromMont(mpz_class& x) const { mul(x, x, 1); }

	void mont(Unit *z, const Unit *x, const Unit *y) const
	{
		mpz_class mx, my;
		setMpz(mx, x, n_);
		setMpz(my, y, n_);
		mul(mx, mx, my);
		getMpz(z, n_, mx);
	}
	void mul(mpz_class& z, const mpz_class& x, const mpz_class& y) const
	{
#if 0
		const size_t ySize = mie::Gmp::getBlockSize(y);
		mpz_class c = x * mie::Gmp::getBlock(y, 0);
		BlockType q = mie::Gmp::getBlock(c, 0) * r_;
		c += p_ * q;
		c >>= sizeof(BlockType) * 8;
		for (size_t i = 1; i < n_; i++) {
			if (i < ySize) {
				c += x * mie::Gmp::getBlock(y, i);
			}
			BlockType q = mie::Gmp::getBlock(c, 0) * r_;
			c += p_ * q;
			c >>= sizeof(BlockType) * 8;
		}
		if (c >= p_) {
			c -= p_;
		}
		z = c;
#else
		z = x * y;
		for (size_t i = 0; i < n_; i++) {
			BlockType q = mie::Gmp::getBlock(z, 0) * r_;
			z += p_ * (mp_limb_t)q;
			z >>= sizeof(BlockType) * 8;
		}
		if (z >= p_) {
			z -= p_;
		}
#endif
	}
};

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
static inline void set_zero(mpz_t& z, Unit *p, size_t n)
{
	z->_mp_alloc = (int)n;
	z->_mp_size = 0;
	z->_mp_d = (mp_limb_t*)p;
}
static inline void set_mpz_t(mpz_t& z, const Unit* p, int n)
{
	z->_mp_alloc = n;
	int i = n;
	while (i > 0 && p[i - 1] == 0) {
		i--;
	}
	z->_mp_size = i;
	z->_mp_d = (mp_limb_t*)p;
}

// z[2n] <- x[n] * y[n]
void mulPreC(Unit *z, const Unit *x, const Unit *y, size_t n)
{
#if 1
	mpz_t mx, my, mz;
	set_zero(mz, z, n * 2);
	set_mpz_t(mx, x, n);
	set_mpz_t(my, y, n);
	mpz_mul(mz, mx, my);
	mie::fp::local::toArray(z, n * 2, mz);
#else
	mpz_class mx, my;
	setMpz(mx, x, n);
	setMpz(my, y, n);
	mx *= my;
	getMpz(z, n * 2, mx);
#endif
}

void modC(Unit *y, const Unit *x, const Unit *p, size_t n)
{
	mpz_t mx, my, mp;
	set_mpz_t(mx, x, n * 2);
	set_mpz_t(my, y, n);
	set_mpz_t(mp, p, n);
	mpz_mod(my, mx, mp);
	mie::fp::local::clearArray(y, my->_mp_size, n);
}

void mul(Unit *z, const Unit *x, const Unit *y, const Unit *p, size_t n)
{
	Unit ret[MAX_N * 2];
	mpz_t mx, my, mz, mp;
	set_zero(mz, ret, MAX_N * 2);
	set_mpz_t(mx, x, n);
	set_mpz_t(my, y, n);
	set_mpz_t(mp, p, n);
	mpz_mul(mz, mx, my);
	mpz_mod(mz, mz, mp);
	mie::fp::local::toArray(z, n, mz);
}

typedef void (*void4op)(Unit*, const Unit*, const Unit*, const Unit*);
typedef void (*void3op)(Unit*, const Unit*, const Unit*);

const struct FuncOp {
	size_t bitLen;
	void4op addS;
	void4op addL;
	void4op subS;
	void4op subL;
	void3op mulPre;
} gFuncOpTbl[] = {
	{ 128, mie_fp_add128S, mie_fp_add128L, mie_fp_sub128S, mie_fp_sub128L, mie_fp_mul128pre },
	{ 192, mie_fp_add192S, mie_fp_add192L, mie_fp_sub192S, mie_fp_sub192L, mie_fp_mul192pre },
	{ 256, mie_fp_add256S, mie_fp_add256L, mie_fp_sub256S, mie_fp_sub256L, mie_fp_mul256pre },
	{ 320, mie_fp_add320S, mie_fp_add320L, mie_fp_sub320S, mie_fp_sub320L, mie_fp_mul320pre },
	{ 384, mie_fp_add384S, mie_fp_add384L, mie_fp_sub384S, mie_fp_sub384L, mie_fp_mul384pre },
	{ 448, mie_fp_add448S, mie_fp_add448L, mie_fp_sub448S, mie_fp_sub448L, mie_fp_mul448pre },
	{ 512, mie_fp_add512S, mie_fp_add512L, mie_fp_sub512S, mie_fp_sub512L, mie_fp_mul512pre },
#if CYBOZU_OS_BIT == 32
	{ 160, mie_fp_add160S, mie_fp_add160L, mie_fp_sub160S, mie_fp_sub160L, mie_fp_mul160pre },
	{ 224, mie_fp_add224S, mie_fp_add224L, mie_fp_sub224S, mie_fp_sub224L, mie_fp_mul224pre },
	{ 288, mie_fp_add288S, mie_fp_add288L, mie_fp_sub288S, mie_fp_sub288L, mie_fp_mul288pre },
	{ 352, mie_fp_add352S, mie_fp_add352L, mie_fp_sub352S, mie_fp_sub352L, mie_fp_mul352pre },
	{ 416, mie_fp_add416S, mie_fp_add416L, mie_fp_sub416S, mie_fp_sub416L, mie_fp_mul416pre },
	{ 480, mie_fp_add480S, mie_fp_add480L, mie_fp_sub480S, mie_fp_sub480L, mie_fp_mul480pre },
	{ 544, mie_fp_add544S, mie_fp_add544L, mie_fp_sub544S, mie_fp_sub544L, mie_fp_mul544pre },
#else
	{ 576, mie_fp_add576S, mie_fp_add576L, mie_fp_sub576S, mie_fp_sub576L, mie_fp_mul576pre },
#endif
};

FuncOp getFuncOp(size_t bitLen)
{
	typedef std::map<size_t, FuncOp> Map;
	static Map map;
	static bool init = false;
	if (!init) {
		init = true;
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(gFuncOpTbl); i++) {
			map[gFuncOpTbl[i].bitLen] = gFuncOpTbl[i];
		}
	}
	for (Map::const_iterator i = map.begin(), ie = map.end(); i != ie; ++i) {
		if (bitLen <= i->second.bitLen) {
			return i->second;
		}
	}
	printf("ERR bitLen=%d\n", (int)bitLen);
	exit(1);
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
	const FuncOp funcOp = getFuncOp(bitLen);
	const void4op addS = funcOp.addS;
	const void4op addL = funcOp.addL;
	const void4op subS = funcOp.subS;
	const void4op subL = funcOp.subL;
	const void3op mulPre = funcOp.mulPre;

	mie::fp::Unit x[MAX_N], y[MAX_N];
	mie::fp::Unit z[MAX_N], w[MAX_N];
	mie::fp::Unit z2[MAX_N * 2];
	mie::fp::Unit w2[MAX_N * 2];
	cybozu::XorShift rg;
	mie::fp::getRandVal(x, rg, p, bitLen);
	mie::fp::getRandVal(y, rg, p, bitLen);
	const size_t C = 10;

	addC(z, x, y, p, n);
	addS(w, x, y, p);
	VERIFY_EQUAL(z, w, n);
	for (size_t i = 0; i < C; i++) {
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
		mulPreC(z2, x, z, n);
		mulPre(w2, x, z);
		VERIFY_EQUAL(z2, w2, n * 2);
	}
	if (doBench) {
//		CYBOZU_BENCH("addS", addS, x, y, x, p); // slow
//		CYBOZU_BENCH("subS", subS, x, y, x, p);
//		CYBOZU_BENCH("addL", addL, x, y, x, p);
//		CYBOZU_BENCH("subL", subL, x, y, x, p);
		CYBOZU_BENCH("mulPreA", mulPre, w2, y, x);
		CYBOZU_BENCH("mulPreC", mulPreC, w2, y, x, n);
//		CYBOZU_BENCH("modC", modC, x, w2, p, n);
	}
#ifdef USE_XBYAKxx
	if (bitLen <= 128) return;
	if (doBench) {
		fg.init(p, n);
		CYBOZU_BENCH("addA", fg.add_, x, y, x);
		CYBOZU_BENCH("subA", fg.sub_, x, y, x);
		CYBOZU_BENCH("mulA", fg.mul_, x, y, x);
	}
#endif
	if (bitLen == 128) {
		mpz_class mp;
		setMpz(mp, p, n);
		Montgomery m(mp);
		/*
			real mont
			   0    0
			   1    R^-1
			   R    1
			  -1    -R^-1
			  -R    -1
		*/
		mpz_class t = 1;
		const mpz_class R = (t << (n * 64)) % mp;
		const mpz_class tbl[] = {
			0, 1, R, mp - 1, mp - R
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			const mpz_class& mx = tbl[i];
			for (size_t j = i; j < CYBOZU_NUM_OF_ARRAY(tbl); j++) {
				const mpz_class& my = tbl[j];
				getMpz(x, n, mx);
				getMpz(y, n, my);
				m.mont(z, x, y);
				mie_fp_mont128(w, x, y, p, m.r_);
				VERIFY_EQUAL(z, w, n);
			}
		}

	}
}

CYBOZU_TEST_AUTO(all)
{
	const struct {
		size_t n;
		const uint64_t p[9];
	} tbl[] = {
		{ 2, { 0xf000000000000001, 1, } },
		{ 2, { 0x000000000000001d, 0x8000000000000000, } },
		{ 3, { 0x000000000000012b, 0x0000000000000000, 0x0000000080000000, } },
		{ 3, { 0x0f69466a74defd8d, 0xfffffffe26f2fc17, 0xffffffffffffffff, } },
		{ 3, { 0x7900342423332197, 0x1234567890123456, 0x1480948109481904, } },
		{ 3, { 0x0f69466a74defd8d, 0xfffffffe26f2fc17, 0x07ffffffffffffff, } },
		{ 4, { 0xa700000000000013, 0x6121000000000013, 0xba344d8000000008, 0x2523648240000001, } },
		{ 4, { 0x7900342423332197, 0x4242342420123456, 0x1234567892342342, 0x1480948109481904, } },
		{ 4, { 0x0f69466a74defd8d, 0xfffffffe26f2fc17, 0x17ffffffffffffff, 0x1513423423423415, } },
		{ 5, { 0x0000000000000009, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x8000000000000000, } },
		{ 6, { 0x00000000ffffffff, 0xffffffff00000000, 0xfffffffffffffffe, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, } },
		{ 6, { 0x4720422423332197, 0x0034230847204720, 0x3456789012345679, 0x4820984290482212, 0x9482094820948209, 0x0194810841094810, } },
		{ 6, { 0x7204224233321972, 0x0342308472047204, 0x4567890123456790, 0x0948204204243123, 0x2098420984209482, 0x2093482094810948, } },
		{ 7, { 0x0000000000000063, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x8000000000000000, } },
		{ 7, { 0x000000000fffcff1, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, } },
		{ 8, { 0xffffffffffffd0c9, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, } },
		{ 9, { 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0x00000000000001ff, } },
		{ 9, { 0x4720422423332197, 0x0034230847204720, 0x3456789012345679, 0x2498540975555312, 0x9482904924029424, 0x0948209842098402, 0x1098410948109482, 0x0820958209582094, 0x0000000000000029, } },
		{ 9, { 0x0f69466a74defd8d, 0xfffffffe26f2fc17, 0x7fffffffffffffff, 0x8572938572398583, 0x5732057823857293, 0x9820948205872380, 0x3409238420492034, 0x9483842098340298, 0x0000000000000003, } },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const size_t n = tbl[i].n;
		const size_t bitLen = (n - 1) * 64 + cybozu::bsr<uint64_t>(tbl[i].p[n - 1]) + 1;
		test((const Unit*)tbl[i].p, bitLen);
	}
}

