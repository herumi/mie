#pragma once
/**
	@file
	@brief power
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <assert.h>
#include <mie/tagmultigr.hpp>

namespace mie {

namespace power_impl {

template<class F>
struct TagInt {
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

template<>
struct TagInt<int> {
	typedef int block_type;
	static int getBlockSize(int)
	{
		return 1;
	}
	static block_type getBlock(int n, size_t i)
	{
		assert(i == 0);
		return n;
	}
};

template<>
struct TagInt<size_t> {
	typedef size_t block_type;
	static size_t getBlockSize(size_t)
	{
		return 1;
	}
	static block_type getBlock(size_t n, size_t i)
	{
		assert(i == 0);
		return n;
	}
};

template<class G, class F>
void power(G& z, const G& x, const F& y)
{
	typedef TagMultiGr<G> TagG;
	typedef power_impl::TagInt<F> TagI;
	typedef typename TagI::block_type block_type;
	if (y == 0) {
		TagG::init(z);
		return;
	}
	if (y == 1) {
		z = x;
		return;
	}
	G out;
	TagG::init(out);
	G t(x);
	for (size_t i = 0, n = TagI::getBlockSize(y); i < n; i++) {
		block_type v = TagI::getBlock(y, i);
		int m = (int)sizeof(block_type) * 8;
		if (i == n - 1) {
			// avoid unused multiplication
			while (m > 0 && (v & (block_type(1) << (m - 1))) == 0) {
				m--;
			}
		}
		for (int j = 0; j < m; j++) {
			if (v & (block_type(1) << j)) {
				TagG::mul(out, out, t);
			}
			TagG::square(t, t);
		}
	}
	z = out;
}

} } // mie::power_impl
