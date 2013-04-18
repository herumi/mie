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
	typedef Xbyak::Reg32e Reg32e;
	typedef Xbyak::Reg64 Reg64;
	typedef Xbyak::util::StackFrame StackFrame;
	const uint64_t *p_;
	int pn_;
	bool isFullBit_;
	typedef bool (*bool3op)(uint64_t*, const uint64_t*, const uint64_t*);
	typedef void (*void3op)(uint64_t*, const uint64_t*, const uint64_t*);
	bool3op addNC;
	bool3op subNC;
	void3op addMod;
	FpGenerator()
		: p_(0)
		, pn_(0)
		, isFullBit_(0)
		, addNC(0)
		, subNC(0)
		, addMod(0)
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

		align(16);
		addNC = getCurr<bool3op>();
		gen_addsubNC(true);
		align(16);
		subNC = getCurr<bool3op>();
		gen_addsubNC(false);
		align(16);
		addMod = getCurr<void3op>();
		gen_addMod();
	}
	void gen_addsubNC(bool isAdd)
	{
		StackFrame sf(this, 3);
		if (isAdd) {
			gen_raw_addNC(sf.p(0), sf.p(1), sf.p(2));
		} else {
			gen_raw_subNC(sf.p(0), sf.p(1), sf.p(2));
		}
		if (isFullBit_) {
			setc(al);
			movzx(eax, al);
		}
	}
	void gen_raw_addNC(const Reg32e& pz, const Reg32e& px, const Reg32e& py)
	{
		mov(rax, ptr [px]);
		add(rax, ptr [py]);
		mov(ptr [pz], rax);
		for (int i = 1; i < pn_; i++) {
			mov(rax, ptr [px + i * 8]);
			adc(rax, ptr [py + i * 8]);
			mov(ptr [pz + i * 8], rax);
		}
	}
	void gen_raw_subNC(const Reg32e& pz, const Reg32e& px, const Reg32e& py)
	{
		mov(rax, ptr [px]);
		sub(rax, ptr [py]);
		mov(ptr [pz], rax);
		for (int i = 1; i < pn_; i++) {
			mov(rax, ptr [px + i * 8]);
			sbb(rax, ptr [py + i * 8]);
			mov(ptr [pz + i * 8], rax);
		}
	}
	void gen_addMod()
	{
	}
private:
	FpGenerator(const FpGenerator&);
	void operator=(const FpGenerator&);
};

} // mie
