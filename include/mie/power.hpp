#pragma once
/**
    @file
    @brief Fp
    @author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://www.opensource.org/licenses/bsd-license.php
*/
namespace mie { namespace power_impl {

template<class G>
struct Tag1 {
	static void square(G& x)
	{
		x += x;
	}
	static void mul(G& y, const G& x)
	{
		y += x;
	}
	static void init(G& x)
	{
		x.clear();
	}
};

template<class F>
struct Tag2 {
	typedef typename F::block_type block_type;
	static size_t getBlockSize(const F& n)
	{
		return F::getBlockSize(n);
	}
	static block_type getBlock(const F& n, size_t i)
	{
		return F::getBlock(n, i);
	}
};

template<class G, class F>
void power(G& z, const G& x, const F& y)
{
	typedef power_impl::Tag1<G> TagG;
	typedef power_impl::Tag2<F> TagF;
	typedef typename TagF::block_type block_type;
	G t(x);
	G out;
	TagG::init(out);
	for (size_t i = 0, n = TagF::getBlockSize(y); i < n; i++) {
		block_type v = TagF::getBlock(y, i);
		int m = (int)sizeof(block_type) * 8;
		if (i == n - 1) {
			// avoid unused multiplication
			while (m > 0 && (v & (block_type(1) << (m - 1))) == 0) {
				m--;
			}
		}
		for (int j = 0; j < m; j++) {
			if (v & (block_type(1) << j)) {
				TagG::mul(out, t);
			}
			TagG::square(t);
		}
	}
	z = out;
}

} } // mie::power_impl

