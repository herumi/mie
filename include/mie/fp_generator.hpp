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
	// add/sub without carry. return true if overflow
	typedef bool (*bool3op)(uint64_t*, const uint64_t*, const uint64_t*);

	// add/sub with mod
	typedef void (*void3op)(uint64_t*, const uint64_t*, const uint64_t*);

	// mul without carry. return top of z
	typedef uint64_t (*uint3opI)(uint64_t*, const uint64_t*, uint64_t);
	bool3op add_;
	bool3op sub_;
	void3op addMod_;
	void3op subMod_;
	uint3opI mulI_;
	FpGenerator()
		: p_(0)
		, pn_(0)
		, isFullBit_(0)
		, add_(0)
		, sub_(0)
		, addMod_(0)
		, subMod_(0)
		, mulI_(0)
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
		printf("p=%p, pn_=%d, isFullBit_=%d\n", p_, pn_, isFullBit_);

		align(16);
		add_ = getCurr<bool3op>();
		gen_addsub(true);
		align(16);
		sub_ = getCurr<bool3op>();
		gen_addsub(false);
		align(16);
		addMod_ = getCurr<void3op>();
		gen_addMod();
		align(16);
		subMod_ = getCurr<void3op>();
		gen_subMod();
	}
	void gen_addsub(bool isAdd)
	{
		StackFrame sf(this, 3);
		if (isAdd) {
			gen_raw_add(sf.p(0), sf.p(1), sf.p(2), rax);
		} else {
			gen_raw_sub(sf.p(0), sf.p(1), sf.p(2), rax);
		}
		if (isFullBit_) {
			setc(al);
			movzx(eax, al);
		}
	}
	/*
		pz[] = px[] + py[]
	*/
	void gen_raw_add(const Reg32e& pz, const Reg32e& px, const Reg32e& py, const Reg64& t)
	{
		mov(t, ptr [px]);
		add(t, ptr [py]);
		mov(ptr [pz], t);
		for (int i = 1; i < pn_; i++) {
			mov(t, ptr [px + i * 8]);
			adc(t, ptr [py + i * 8]);
			mov(ptr [pz + i * 8], t);
		}
	}
	/*
		pz[] = px[] - py[]
	*/
	void gen_raw_sub(const Reg32e& pz, const Reg32e& px, const Reg32e& py, const Reg64& t)
	{
		mov(t, ptr [px]);
		sub(t, ptr [py]);
		mov(ptr [pz], t);
		for (int i = 1; i < pn_; i++) {
			mov(t, ptr [px + i * 8]);
			sbb(t, ptr [py + i * 8]);
			mov(ptr [pz + i * 8], t);
		}
	}
	/*
		(rax:pz[]) = px[] * y
		use rax, rdx
	*/
#if 0
	void gen_raw_mulI2(const Reg32e& pz, const Reg32e& px, const Reg64& y, const Reg64& t0)
	{
	}
#endif
	/*
		pz[] = px[]
	*/
	void gen_mov(const Reg32e& pz, const Reg32e& px, const Reg64& t, int n)
	{
		for (int i = 0; i < n; i++) {
			mov(t, ptr [px + i * 8]);
			mov(ptr [pz + i * 8], t);
		}
	}
	void gen_addMod()
	{
		StackFrame sf(this, 3, 0, pn_ * 8);
		const Reg64& pz = sf.p(0);
		const Reg64& px = sf.p(1);
		const Reg64& py = sf.p(2);

		inLocalLabel();
		gen_raw_add(pz, px, py, rax);
		mov(px, (size_t)p_); // destroy px
		if (isFullBit_) {
			jc(".over");
		}
		gen_raw_sub(rsp, pz, px, rax);
		jc(".exit");
		gen_mov(pz, rsp, rax, pn_);
		if (isFullBit_) {
			jmp(".exit");
			L(".over");
			gen_raw_sub(pz, pz, px, rax);
		}
		L(".exit");
		outLocalLabel();
	}
	void gen_subMod()
	{
		StackFrame sf(this, 3);
		const Reg64& pz = sf.p(0);
		const Reg64& px = sf.p(1);
		const Reg64& py = sf.p(2);

		inLocalLabel();
		gen_raw_sub(pz, px, py, rax);
		jnc(".exit");
		mov(px, (size_t)p_);
		gen_raw_add(pz, pz, px, rax);
	L(".exit");
		outLocalLabel();
	}
private:
	FpGenerator(const FpGenerator&);
	void operator=(const FpGenerator&);
};

} // mie
