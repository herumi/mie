#include <mie/fp_generator.hpp>
#include <stdint.h>
#include <string>
#include <cybozu/itoa.hpp>

const uint64_t g_p[] = {
	uint64_t(0xa700000000000013ull),
	uint64_t(0x6121000000000013ull),
	uint64_t(0xba344d8000000008ull),
	uint64_t(0x2523648240000001ull)
};

template<int N>
struct IntT {
	uint64_t v_[N];
	void set(const uint64_t* x)
	{
		for (int i = 0; i < N; i++) v_[i] = x[i];
	}
	std::string toStr() const
	{
		std::string ret;
		for (int i = 0; i < N; i++) {
			ret += cybozu::itohex(v_[N - 1 - i], false);
		}
		return ret;
	}
	void put() const
	{
		printf("%s\n", toStr().c_str());
	}
};

typedef IntT<4> Int;

int main()
{
	mie::FpGenerator fg;
	fg.init(g_p, CYBOZU_NUM_OF_ARRAY(g_p));
	void (*addNC)(Int&, const Int&, const Int&) = fg.getCode<void (*)(Int&, const Int&, const Int&)>();
	fg.genAddNC();
	Int x, z;
	x.set(g_p);
	x.put();
	addNC(z, x, x);
	z.put();
}
