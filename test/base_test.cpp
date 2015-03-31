#include <map>
#define MIE_USE_LLVM
#include <mie/fp_base.hpp>
#include <cybozu/test.hpp>
#include <cybozu/benchmark.hpp>
#include <cybozu/xorshift.hpp>
#include <cybozu/bit_operation.hpp>
#include <mie/fp_util.hpp>
#include <mie/fp2.hpp>

#include <mie/fp_generator.hpp>
#if (CYBOZU_HOST == CYBOZU_HOST_INTEL) && (CYBOZU_OS_BIT == 64)
	#define USE_XBYAK
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


struct Montgomery {
	static const size_t UnitBit = sizeof(Unit) * 8;
	mpz_class p_;
	mpz_class R_; // (1 << (n_ * UnitBit)) % p
	mpz_class RR_; // (R * R) % p
	Unit r_; // p * r = -1 mod M = 1 << UnitBit
	size_t n_;
	Montgomery() {}
	explicit Montgomery(const mpz_class& p)
	{
		if (!mie::Gmp::isPrime(p)) throw cybozu::Exception("not prime") << p;
		p_ = p;
		r_ = mie::montgomery::getCoff(mie::Gmp::getBlock(p, 0));
		n_ = mie::Gmp::getBlockSize(p);
		R_ = (mpz_class(1) << (n_ * UnitBit)) % p_;
		RR_ = (R_ * R_) % p_;
	}

	void toMont(mpz_class& x) const { mul(x, x, RR_); }
	void fromMont(mpz_class& x) const { mul(x, x, 1); }

	void mul(Unit *z, const Unit *x, const Unit *y) const
	{
		mpz_class mx, my, mz;
		setMpz(mx, x, n_);
		setMpz(my, y, n_);
#if 0
		mul(mz, mx, my);
#else
		mod(mz, mx * my);
#endif
		getMpz(z, n_, mz);
	}
	void mul(mpz_class& z, const mpz_class& x, const mpz_class& y) const
	{
		const size_t ySize = mie::Gmp::getBlockSize(y);
		mpz_class c = y == 0 ? mpz_class(0) : x * mie::Gmp::getBlock(y, 0);
		Unit q = c == 0 ? 0 : mie::Gmp::getBlock(c, 0) * r_;
		c += p_ * q;
		c >>= sizeof(Unit) * 8;
		for (size_t i = 1; i < n_; i++) {
			if (i < ySize) {
				c += x * mie::Gmp::getBlock(y, i);
			}
			Unit q = c == 0 ? 0 : mie::Gmp::getBlock(c, 0) * r_;
			c += p_ * q;
			c >>= sizeof(Unit) * 8;
		}
		if (c >= p_) {
			c -= p_;
		}
		z = c;
	}
	// x = x * y
	void mod(Unit *y, const Unit *x) const
	{
		mpz_class mx;
		setMpz(mx, x, n_ * 2);
		mod(mx, mx);
		getMpz(y, n_, mx);
	}
	void mod(mpz_class& y, const mpz_class& x) const
	{
		y = x;
		for (size_t i = 0; i < n_; i++) {
			if (y != 0) {
				Unit q = mie::Gmp::getBlock(y, 0) * r_;
				y += p_ * (mp_limb_t)q;
			}
			y >>= sizeof(Unit) * 8;
		}
		if (y >= p_) {
			y -= p_;
		}
	}
};

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

void mulC(Unit *z, const Unit *x, const Unit *y, const Unit *p, size_t n)
{
	Unit ret[MAX_N * 2];
	assert(n <= MAX_N);
	mpz_t mx, my, mz, mp;
	set_zero(mz, ret, n * 2);
	set_mpz_t(mx, x, n);
	set_mpz_t(my, y, n);
	set_mpz_t(mp, p, n);
	mpz_mul(mz, mx, my);
	mpz_mod(mz, mz, mp);
	mie::fp::local::toArray(z, n, mz);
}

typedef mie::fp::void3op void3op;
typedef mie::fp::void4op void4op;
typedef mie::fp::void4Iop void4Iop;
typedef mie::fp::void3Iop void3Iop;

const struct FuncOp {
	size_t bitLen;
	void4op addS;
	void4op addL;
	void4op subS;
	void4op subL;
	void3op mulPre;
	void4Iop mont;
	void3Iop mod;
} gFuncOpTbl[] = {
	{ 128, mie_fp_add128S, mie_fp_add128L, mie_fp_sub128S, mie_fp_sub128L, mie_fp_mul128pre, mie_fp_mont128, mie_fp_mod128 },
	{ 192, mie_fp_add192S, mie_fp_add192L, mie_fp_sub192S, mie_fp_sub192L, mie_fp_mul192pre, mie_fp_mont192, mie_fp_mod192 },
	{ 256, mie_fp_add256S, mie_fp_add256L, mie_fp_sub256S, mie_fp_sub256L, mie_fp_mul256pre, mie_fp_mont256, mie_fp_mod256 },
	{ 320, mie_fp_add320S, mie_fp_add320L, mie_fp_sub320S, mie_fp_sub320L, mie_fp_mul320pre, mie_fp_mont320, mie_fp_mod320 },
	{ 384, mie_fp_add384S, mie_fp_add384L, mie_fp_sub384S, mie_fp_sub384L, mie_fp_mul384pre, mie_fp_mont384, mie_fp_mod384 },
	{ 448, mie_fp_add448S, mie_fp_add448L, mie_fp_sub448S, mie_fp_sub448L, mie_fp_mul448pre, mie_fp_mont448, mie_fp_mod448 },
	{ 512, mie_fp_add512S, mie_fp_add512L, mie_fp_sub512S, mie_fp_sub512L, mie_fp_mul512pre, mie_fp_mont512, mie_fp_mod512 },
#if CYBOZU_OS_BIT == 32
	{ 160, mie_fp_add160S, mie_fp_add160L, mie_fp_sub160S, mie_fp_sub160L, mie_fp_mul160pre, mie_fp_mont160, mie_fp_mod160 },
	{ 224, mie_fp_add224S, mie_fp_add224L, mie_fp_sub224S, mie_fp_sub224L, mie_fp_mul224pre, mie_fp_mont224, mie_fp_mod224 },
	{ 288, mie_fp_add288S, mie_fp_add288L, mie_fp_sub288S, mie_fp_sub288L, mie_fp_mul288pre, mie_fp_mont288, mie_fp_mod288 },
	{ 352, mie_fp_add352S, mie_fp_add352L, mie_fp_sub352S, mie_fp_sub352L, mie_fp_mul352pre, mie_fp_mont352, mie_fp_mod352 },
	{ 416, mie_fp_add416S, mie_fp_add416L, mie_fp_sub416S, mie_fp_sub416L, mie_fp_mul416pre, mie_fp_mont416, mie_fp_mod416 },
	{ 480, mie_fp_add480S, mie_fp_add480L, mie_fp_sub480S, mie_fp_sub480L, mie_fp_mul480pre, mie_fp_mont480, mie_fp_mod480 },
	{ 544, mie_fp_add544S, mie_fp_add544L, mie_fp_sub544S, mie_fp_sub544L, mie_fp_mul544pre, mie_fp_mont544, mie_fp_mod544 },
#else
	{ 576, mie_fp_add576S, mie_fp_add576L, mie_fp_sub576S, mie_fp_sub576L, mie_fp_mul576pre, mie_fp_mont576, mie_fp_mod576 },
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
	printf("bitLen %d\n", (int)bitLen);
	const size_t n = getUnitN(bitLen);
#ifdef NDEBUG
	bool doBench = true;
#else
	bool doBench = false;
#endif
	mpz_class mp;
	setMpz(mp, p, n);
	const FuncOp funcOp = getFuncOp(bitLen);
	const void4op addS = funcOp.addS;
	const void4op addL = funcOp.addL;
	const void4op subS = funcOp.subS;
	const void4op subL = funcOp.subL;
	const void3op mulPre = funcOp.mulPre;
	const void4Iop mont = funcOp.mont;
	const void3Iop mod = funcOp.mod;

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
	{
		Montgomery m(mp);
#ifdef USE_XBYAK
		if (bitLen > 128) fg.init(p, n);
#endif
		/*
			real mont
			   0    0
			   1    R^-1
			   R    1
			  -1    -R^-1
			  -R    -1
		*/
		const mpz_class R = (mpz_class(1) << bitLen) % mp;
		const mpz_class tbl[] = {
			0, 1, R, mp - 1, mp - R
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			const mpz_class& mx = tbl[i];
			for (size_t j = i; j < CYBOZU_NUM_OF_ARRAY(tbl); j++) {
				const mpz_class& my = tbl[j];
				getMpz(x, n, mx);
				getMpz(y, n, my);
				m.mul(z, x, y);
				mont(w, x, y, p, m.r_);
				VERIFY_EQUAL(z, w, n);
				mpz_class xy = mx * my;
				getMpz(w2, n * 2, xy);
				m.mod(w, w2);
				VERIFY_EQUAL(z, w, n);
				mod(w, w2, p, m.r_);
				VERIFY_EQUAL(z, w, n);
#ifdef USE_XBYAK
				if (bitLen > 128) {
					fg.mul_(w, x, y);
					VERIFY_EQUAL(z, w, n);
				}
#endif
			}
		}
		if (doBench) {
			memset(x, -1, sizeof(x));
			memset(y, -1, sizeof(y));
			CYBOZU_BENCH("mulPre_ll", mulPre, w2, y, x);
			CYBOZU_BENCH("mod_ll   ", mod, x, w2, p, m.r_);
			CYBOZU_BENCH("mont_ll  ", mont, x, y, x, p, m.r_);
			if (mp == mpz_class("0xfffffffffffffffffffffffffffffffeffffffffffffffff")) {
				CYBOZU_BENCH("NSIT_P192mod", mie_fp_mul_NIST_P192, x, y, x);
			}
		}
	}
	if (doBench) {
		memset(x, -1, sizeof(x));
		memset(y, -1, sizeof(y));
//		CYBOZU_BENCH("addS", addS, x, y, x, p); // slow
//		CYBOZU_BENCH("subS", subS, x, y, x, p);
//		CYBOZU_BENCH("addL", addL, x, y, x, p);
//		CYBOZU_BENCH("subL", subL, x, y, x, p);
		CYBOZU_BENCH("mulPreC  ", mulPreC, w2, y, x, n);
		CYBOZU_BENCH("modC     ", modC, x, w2, p, n);
//		CYBOZU_BENCH("mulC     ", mulC, x, y, x, p, n);
	}
#ifdef USE_XBYAK
	if (bitLen <= 128) return;
	if (doBench) {
		fg.init(p, n);
//		CYBOZU_BENCH("add_X    ", fg.add_, x, y, x);
//		CYBOZU_BENCH("sub_X    ", fg.sub_, x, y, x);
		CYBOZU_BENCH("mont_X   ", fg.mul_, x, y, x);
	}
#endif
}

CYBOZU_TEST_AUTO(all)
{
	const struct {
		size_t n;
		const uint64_t p[9];
	} tbl[] = {
//		{ 2, { 0xf000000000000001, 1, } },
//		{ 2, { 0x000000000000001d, 0x8000000000000000, } },
//		{ 3, { 0xfffffffffffffcb5, 0xffffffffffffffff, 0x00000000ffffffff, } },
//		{ 3, { 0x0f69466a74defd8d, 0xfffffffe26f2fc17, 0x07ffffffffffffff, } },
		{ 3, { 0xffffffffffffffff, 0xfffffffffffffffe, 0xffffffffffffffff, } },
		{ 3, { 0xfffffffeffffee37, 0xffffffffffffffff, 0xffffffffffffffff, } },
//		{ 4, { 0x7900342423332197, 0x4242342420123456, 0x1234567892342342, 0x1480948109481904, } },
//		{ 4, { 0x0f69466a74defd8d, 0xfffffffe26f2fc17, 0x17ffffffffffffff, 0x1513423423423415, } },
		{ 4, { 0xa700000000000013, 0x6121000000000013, 0xba344d8000000008, 0x2523648240000001, } },
		{ 4, { 0xfffffffefffffc2f, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, } },
//		{ 5, { 0x0000000000000009, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x8000000000000000, } },
		{ 5, { 0xfffffffffffffc97, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, } },
//		{ 6, { 0x4720422423332197, 0x0034230847204720, 0x3456789012345679, 0x4820984290482212, 0x9482094820948209, 0x0194810841094810, } },
		{ 6, { 0x00000000ffffffff, 0xffffffff00000000, 0xfffffffffffffffe, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, } },
//		{ 7, { 0x0000000000000063, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x8000000000000000, } },
		{ 7, { 0xfffffffffffffcc7, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, } },
		{ 8, { 0xffffffffffffd0c9, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, } },
		{ 9, { 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0x00000000000001ff, } },
//		{ 9, { 0x4720422423332197, 0x0034230847204720, 0x3456789012345679, 0x2498540975555312, 0x9482904924029424, 0x0948209842098402, 0x1098410948109482, 0x0820958209582094, 0x0000000000000029, } },
//		{ 9, { 0x0f69466a74defd8d, 0xfffffffe26f2fc17, 0x7fffffffffffffff, 0x8572938572398583, 0x5732057823857293, 0x9820948205872380, 0x3409238420492034, 0x9483842098340298, 0x0000000000000003, } },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const size_t n = tbl[i].n;
		const size_t bitLen = (n - 1) * 64 + cybozu::bsr<uint64_t>(tbl[i].p[n - 1]) + 1;
		test((const Unit*)tbl[i].p, bitLen);
	}
}

