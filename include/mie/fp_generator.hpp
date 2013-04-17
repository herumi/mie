#pragma once
/**
	@file
	@brief Fp generator
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <stdio.h>
#include <assert.h>
#include <cybozu/exception.hpp>
#include <xbyak/xbyak.h>
#include <xbyak/xbyak_util.h>

namespace mie {

struct FpGenerator : Xbyak::CodeGenerator {
	const uint64_t *p_;
	int pn_;
	bool isFullBit_;
	FpGenerator()
		: p_(0)
		, pn_(0)
		, isFullBit_(0)
	{
	}
	/*
		@param p [in] pointer to prime
		@param pn [in] length of prime
	*/
	void init(const uint64_t *p, size_t pn)
	{
		if (pn < 2) throw cybozu::Exception("mie::FpGenerator:small pn") << pn;
		p_ = p;
		pn_ = (int)pn;
		isFullBit_ = (p_[pn_ - 1] >> 63) != 0;
	}
	void genAddNC()
	{
		using namespace Xbyak;
		util::StackFrame sf(this, 3);
		const Reg64& pz = sf.p(0);
		const Reg64& px = sf.p(1);
		const Reg64& py = sf.p(2);
		mov(rax, ptr [px]);
		add(rax, ptr [py]);
		mov(ptr [pz], rax);
		for (int i = 1; i < pn_; i++) {
			mov(rax, ptr [px + i * 8]);
			adc(rax, ptr [py + i * 8]);
			mov(ptr [pz + i * 8], rax);
		}
	}
private:
	FpGenerator(const FpGenerator&);
	void operator=(const FpGenerator&);
};

} // mie
