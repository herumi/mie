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
#define XBYAK_NO_OP_NAMES
#include <xbyak/xbyak.h>

namespace mie {

template<class Code>
struct StackFrame {
	Code *code_;
	int gtn_;
	const Xbyak::Reg64& a;
	const Xbyak::Reg64& d;
	const Xbyak::Reg64& gp0;
	const Xbyak::Reg64& gp1;
	const Xbyak::Reg64& gp2;
	const Xbyak::Reg64& gt0;
	const Xbyak::Reg64& gt1;
	const Xbyak::Reg64& gt2;
	const Xbyak::Reg64& gt3;
	const Xbyak::Reg64& gt4;
	const Xbyak::Reg64& gt5;
	const Xbyak::Reg64& gt6;
	const Xbyak::Reg64& gt7;
	const Xbyak::Reg64& gt8;
	const Xbyak::Reg64& gt9;
	const Xbyak::Reg64& rsp;
	const Xbyak::Reg64& r9;
	const Xbyak::AddressFrame& ptr;
	StackFrame(Code *code)
		: code_(code)
		, gtn_(0)
		, a(Xbyak::util::rax)
		, d(Xbyak::util::rdx)
#ifdef XBYAK64_WIN
		, gp0(Xbyak::util::rcx)
		, gp1(Xbyak::util::r9) // rdx => r9
		, gp2(Xbyak::util::r8)
		, gt0(Xbyak::util::r10)
		, gt1(Xbyak::util::r11)
		, gt2(Xbyak::util::rdi) // must be saved if used
		, gt3(Xbyak::util::rsi)
#else
		, gp0(Xbyak::util::rdi)
		, gp1(Xbyak::util::rsi)
		, gp2(Xbyak::util::r9) // rdx => r9
		, gt0(Xbyak::util::r8)
		, gt1(Xbyak::util::rcx)
		, gt2(Xbyak::util::r10)
		, gt3(Xbyak::util::r11)
#endif
		, gt4(Xbyak::util::r12) // must be saved if used
		, gt5(Xbyak::util::r13)
		, gt6(Xbyak::util::r14)
		, gt7(Xbyak::util::r15)
		, gt8(Xbyak::util::rbp)
		, gt9(Xbyak::util::rbx)
		, rsp(Xbyak::util::rsp)
		, r9(Xbyak::util::r9)
		, ptr(Xbyak::util::ptr)
	{
	}
	struct MakeStackFrame {
		StackFrame *sf_;
		int P_;
		MakeStackFrame(StackFrame *sf, int gtn, int numQword = 0)
			: sf_(sf)
			, P_(sf_->storeReg(gtn, numQword))
		{
		}
		~MakeStackFrame()
		{
			sf_->restoreReg(P_);
			sf_->code_->ret();
		}
	};
	/*
		utility function for many register
		you can use gt0, ..., gt(n-1) and rax, rdx
		gp0 : 1st parameter
		gp1 : 2nd parameter
		gp2 : 3rd parameter
		numQword : alloca stack if necessary
			rsp[0..8 * numQrod - 1] are available
	*/
	int storeReg(int gtn, int numQword = 0)
	{
		static const Xbyak::Reg64 tbl[] = {
			gt2, gt3, gt4, gt5, gt6, gt7, gt8, gt9
		};
		assert(0 <= gtn && gtn <= 10);
		gtn_ = gtn;
#ifdef XBYAK64_WIN
		const int P = 8 * (std::max(0, gtn - 6) + numQword);
		if (P > 0) code_->sub(rsp, P);
		for (int i = 3; i <= std::min(gtn, 6); i++) {
			code_->mov(ptr [rsp + P + (i - 2) * 8], tbl[i - 3]);
		}
		for (int i = 7; i <= gtn; i++) {
			code_->mov(ptr [rsp + P - 8 * (i - 6)], tbl[i - 3]);
		}
#else
		const int P = 8 * (std::max(0, gtn - 4) + numQword);
		if (P > 0) code_->sub(rsp, P);
		for (int i = 5; i <= gtn; i++) {
			code_->mov(ptr [rsp + P - 8 * (i - 4)], tbl[i - 3]);
		}
#endif
		code_->mov(r9, d);
		return P;
	}
	/*
		specify P as the return value of storeReg
	*/
	void restoreReg(int P)
	{
		static const Xbyak::Reg64 tbl[] = {
			gt2, gt3, gt4, gt5, gt6, gt7, gt8, gt9
		};
		assert(0 <= gtn_ && gtn_ <= 10);
#ifdef XBYAK64_WIN
		for (int i = 3; i <= std::min(gtn_, 6); i++) {
			code_->mov(tbl[i - 3], ptr [rsp + P + (i - 2) * 8]);
		}
		for (int i = 7; i <= gtn_; i++) {
			code_->mov(tbl[i - 3], ptr [rsp + P - 8 * (i - 6)]);
		}
#else
		for (int i = 5; i <= gtn_; i++) {
			code_->mov(tbl[i - 3], ptr [rsp + P - 8 * (i - 4)]);
		}
#endif
		if (P > 0) code_->add(rsp, P);
	}
private:
	StackFrame(const StackFrame&);
	void operator=(const StackFrame&);
};

struct FpGenerator : Xbyak::CodeGenerator, StackFrame<FpGenerator> {
	const uint64_t *p_;
	size_t pn_;
	bool isFullBit_;
	FpGenerator()
		: Xbyak::CodeGenerator()
		, StackFrame<FpGenerator>(this)
		, p_(0)
		, pn_(0)
		, isFullBit_(0)
	{
		MakeStackFrame sf(this, 2);
		mov(a, gp0);
		add(a, gp1);
		ret();
	}
	void init(const uint64_t *p, size_t pn)
	{
		if (pn < 2) throw cybozu::Exception("mie::FpGenerator:small pn") << pn;
		p_ = p;
		pn_ = pn;
		isFullBit_ = (p_[pn_ - 1] >> 63) != 0;
	}
private:
	FpGenerator(const FpGenerator&);
	void operator=(const FpGenerator&);
};

} // mie
