#include <stdint.h>
#include <string>
#include <cybozu/itoa.hpp>
#include <mie/fp_generator.hpp>
//#include <mie/fp.hpp>
//#include <mie/gmp_util.hpp>

//typedef mie::FpT<mie::Gmp> Fp;

const struct {
	int pn;
	uint64_t p[4];
} primeTable[] = {
	{ 4, { uint64_t(0xa700000000000013ull), uint64_t(0x6121000000000013ull), uint64_t(0xba344d8000000008ull), uint64_t(0x2523648240000001ull) } },
	{ 3, { uint64_t(0xfffffffeffffac73ull), uint64_t(0xffffffffffffffffull), uint64_t(0xffffffff) } },
	{ 3, { uint64_t(0xfffffffeffffee37ull), uint64_t(0xffffffffffffffffull), uint64_t(0xffffffffffffffff) } },
};

struct Int {
	int vn;
	uint64_t v[4];
	explicit Int(int vn)
	{
		if (vn > 4) {
			printf("vn(%d) is too large\n", vn);
			exit(1);
		}
		this->vn = vn;
	}
	void set(const uint64_t* x)
	{
		for (int i = 0; i < vn; i++) v[i] = x[i];
	}
	std::string toStr() const
	{
		std::string ret;
		for (int i = 0; i < vn; i++) {
			ret += cybozu::itohex(v[vn - 1 - i], false);
		}
		return ret;
	}
	void put(const char *msg = "") const
	{
		if (msg) printf("%s=", msg);
		printf("%s\n", toStr().c_str());
	}
};

void test(const uint64_t *p, int pn)
{
	// init
	printf("pn=%d\n", pn);
//	Fp::
	mie::FpGenerator fg;
	fg.init(p, pn);
	Int x(pn), z(pn);
	x.set(p);
	x.put("x");
	fg.add_(z.v, x.v, x.v);
	z.put("z");
}

int main()
{
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(primeTable); i++) {
		printf("test prime i=%d\n", (int)i);
		test(primeTable[i].p, primeTable[i].pn);
	}
}
