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
#ifdef XBYAK32
	#error "not support 32-bit mode"
#endif
#include <xbyak/xbyak_util.h>

namespace mie {

namespace montgomery {

/*
	get p' such that p * p' = -1 mod R,
	where p is prime and R = 1 << 64(or 32).
	@param pLow [in] p mod R
	T is uint32_t or uint64_t
*/
template<class T>
T getCoff(T pLow)
{
	T ret = 0;
	T t = 0;
	T x = 1;

	for (size_t i = 0; i < sizeof(T) * 8; i++) {
		if ((t & 1) == 0) {
			t += pLow;
			ret += x;
		}
		t >>= 1;
		x <<= 1;
	}
	return ret;
}

} // montgomery

struct FpGenerator : Xbyak::CodeGenerator {
	typedef Xbyak::Reg32e Reg32e;
	typedef Xbyak::Reg64 Reg64;
	typedef Xbyak::util::StackFrame StackFrame;
	static const int UseRDX = Xbyak::util::UseRDX;
	static const int UseRCX = Xbyak::util::UseRCX;
	const uint64_t *p_;
	int pn_;
	bool isFullBit_;
	// add/sub without carry. return true if overflow
	typedef bool (*bool3op)(uint64_t*, const uint64_t*, const uint64_t*);

	// add/sub with mod
	typedef void (*void3op)(uint64_t*, const uint64_t*, const uint64_t*);

	// mul without carry. return top of z
	typedef uint64_t (*uint3opI)(uint64_t*, const uint64_t*, uint64_t);

	// neg
	typedef void (*void2op)(uint64_t*, const uint64_t*);
	bool3op addNc_;
	bool3op subNc_;
	void3op add_;
	void3op sub_;
	uint3opI mulI_;
	void2op neg_;
	FpGenerator()
		: p_(0)
		, pn_(0)
		, isFullBit_(0)
		, addNc_(0)
		, subNc_(0)
		, add_(0)
		, sub_(0)
		, mulI_(0)
		, neg_(0)
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
		addNc_ = getCurr<bool3op>();
		gen_addSubNc(true);
		align(16);
		subNc_ = getCurr<bool3op>();
		gen_addSubNc(false);
		align(16);
		add_ = getCurr<void3op>();
		gen_addMod();
		align(16);
		sub_ = getCurr<void3op>();
		gen_sub();
		align(16);
		neg_ = getCurr<void2op>();
		gen_neg();
		align(16);
		mulI_ = getCurr<uint3opI>();
		gen_mulI();
	}
	void gen_addSubNc(bool isAdd)
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
		pz[] = -px[]
	*/
	void gen_raw_neg(const Reg32e& pz, const Reg32e& px, const Reg64& t0, const Reg64& t1)
	{
		inLocalLabel();
		mov(t0, ptr [px]);
		test(t0, t0);
		jnz(".neg");
		if (pn_ > 1) {
			for (int i = 1; i < pn_; i++) {
				or_(t0, ptr [px + i * 8]);
			}
			jnz(".neg");
		}
		// zero
		for (int i = 0; i < pn_; i++) {
			mov(ptr [pz + i * 8], t0);
		}
		jmp(".exit");
	L(".neg");
		mov(t1, (size_t)p_);
		gen_raw_sub(pz, t1, px, t0);
	L(".exit");
		outLocalLabel();
	}
	/*
		(rdx:pz[]) = px[] * y
		use rax, rdx, pw[]
		@note this is general version(maybe not so fast)
	*/
	void gen_raw_mulI(const Reg32e& pz, const Reg32e& px, const Reg64& y, const Reg32e& pw, const Reg64& t, int n)
	{
		assert(n >= 2);
		if (n == 2) {
			mov(rax, ptr [px]);
			mul(y);
			mov(ptr [pz], rax);
			mov(t, rdx);
			mov(rax, ptr [px + 8]);
			mul(y);
			add(rax, t);
			adc(rdx, 0);
			mov(ptr [pz + 8], rax);
			return;
		}
		for (int i = 0; i < n; i++) {
			mov(rax, ptr [px + i * 8]);
			mul(y);
			mov(ptr [pz + i * 8], rax);
			if (i < n - 1) {
				mov(ptr [pw + i * 8], rdx);
			}
		}
		for (int i = 1; i < n; i++) {
			mov(t, ptr [pz + i * 8]);
			if (i == 1) {
				add(t, ptr [pw + (i - 1) * 8]);
			} else {
				adc(t, ptr [pw + (i - 1) * 8]);
			}
			mov(ptr [pz + i * 8], t);
		}
		adc(rdx, 0);
	}
	void gen_mulI()
	{
		StackFrame sf(this, 3, 1 | UseRDX, pn_ * 8);
		const Reg64& pz = sf.p(0);
		const Reg64& px = sf.p(1);
		const Reg64& y = sf.p(2);
		gen_raw_mulI(pz, px, y, rsp, sf.t(0), pn_);
		mov(rax, rdx);
	}
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
	void gen_sub()
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
	void gen_neg()
	{
		StackFrame sf(this, 2, 2);
		const Reg64& pz = sf.p(0);
		const Reg64& px = sf.p(1);
		gen_raw_neg(pz, px, sf.t(0), sf.t(1));
	}
	/*
		input (z, x, y) = (gp1, gp2, gp3)
		z[0..3] <- montgomery(x[0..3], y[0..3])
		destroy gt1, ..., gt10, xm0, xm1, gp3
	*/
	void gen_montMul4(const uint64_t *p, uint64_t pp)
	{
		StackFrame sf(this, 3, 10 | UseRDX);
		const Reg64& gp1 = sf.p(0);
		const Reg64& gp2 = sf.p(1);
		const Reg64& gp3 = sf.p(2);

		const Reg64& gt1 = sf.t(0);
		const Reg64& gt2 = sf.t(1);
		const Reg64& gt3 = sf.t(2);
		const Reg64& gt4 = sf.t(3);
		const Reg64& gt5 = sf.t(4);
		const Reg64& gt6 = sf.t(5);
		const Reg64& gt7 = sf.t(6);
		const Reg64& gt8 = sf.t(7);
		const Reg64& gt9 = sf.t(8);
		const Reg64& gt10 = sf.t(9);

		movq(xm0, gp1); // save gp1
		mov(gp1, (uint64_t)p);
		movq(xm1, gp3);
		mov(gp3, ptr [gp3]);
		montgomery1(pp, gt1, gt8, gt4, gt3, gt2, gp2, gp3, gp1, gt5, gt6, gt7, gt9, gt10, true);

		movq(gp3, xm1);
		mov(gp3, ptr [gp3 + 8]);
		montgomery1(pp, gt2, gt1, gt8, gt4, gt3, gp2, gp3, gp1, gt5, gt6, gt7, gt9, gt10, false);

		movq(gp3, xm1);
		mov(gp3, ptr [gp3 + 16]);
		montgomery1(pp, gt3, gt2, gt1, gt8, gt4, gp2, gp3, gp1, gt5, gt6, gt7, gt9, gt10, false);

		movq(gp3, xm1);
		mov(gp3, ptr [gp3 + 24]);
		montgomery1(pp, gt4, gt3, gt2, gt1, gt8, gp2, gp3, gp1, gt5, gt6, gt7, gt9, gt10, false);
		// [gt8:gt4:gt3:gt2:gt1]

		mov(gt5, gt1);
		mov(gt6, gt2);
		mov(gt7, gt3);
		mov(rdx, gt4);
		sub_rm(gt4, gt3, gt2, gt1, gp1);
		cmovc(gt1, gt5);
		cmovc(gt2, gt6);
		cmovc(gt3, gt7);
		cmovc(gt4, rdx);

		movq(gp1, xm0); // load gp1
		store_mr(gp1, gt4, gt3, gt2, gt1);
	}
private:
	/*
		[z3:z2:z1:z0] = [m3:m2:m1:m0]
	*/
	void store_mr(const Reg32e& m, const Reg64& x3, const Reg64& x2, const Reg64& x1, const Reg64& x0)
	{
		mov(ptr [m + 8 * 0], x0);
		mov(ptr [m + 8 * 1], x1);
		mov(ptr [m + 8 * 2], x2);
		mov(ptr [m + 8 * 3], x3);
	}
	/*
		[rdx:x:t2:t1:t0] <- py[3:2:1:0] * x
		destroy x, t
	*/
	void mul4x1(const Reg32e& py, const Reg64& x, const Reg64& t3, const Reg64& t2, const Reg64& t1, const Reg64& t0, const Reg64& t)
	{
		mov(rax, ptr [py]);
		mul(x);
		mov(t0, rax);
		mov(t1, rdx);
		mov(rax, ptr [py + 8]);
		mul(x);
		mov(t, rax);
		mov(t2, rdx);
		mov(rax, ptr [py + 8 * 2]);
		mul(x);
		mov(t3, rax);
		mov(rax, x);
		mov(x, rdx);
		mul(qword [py + 8 * 3]);
		add(t1, t);
		adc(t2, t3);
		adc(x, rax);
		adc(rdx, 0);
	}

	/*
		[z3:z2:z1:z0] += [x3:x2:x1:x0]
	*/
	void add_rr(const Reg64& z3, const Reg64& z2, const Reg64& z1, const Reg64& z0,
		const Reg64& x3, const Reg64& x2, const Reg64& x1, const Reg64& x0)
	{
		add(z0, x0);
		adc(z1, x1);
		adc(z2, x2);
		adc(z3, x3);
	}
	/*
		[z3:z2:z1:z0] -= [m3:m2:m1:m0]
	*/
	void sub_rm(const Reg64& z3, const Reg64& z2, const Reg64& z1, const Reg64& z0,
		const Reg32e& m)
	{
		sub(z0, ptr [m + 8 * 0]);
		sbb(z1, ptr [m + 8 * 1]);
		sbb(z2, ptr [m + 8 * 2]);
		sbb(z3, ptr [m + 8 * 3]);
	}
	/*
		c = [c4:c3:c2:c1:c0]
		c += x[3..0] * y
		q = uint64_t(c0 * pp)
		c = (c + q * p) >> 64
		input  [c4:c3:c2:c1:c0], px, y, p
		output [c0:c4:c3:c2:c1]

		@note use rax, rdx, destroy y
		@note max([c4:c3:c2:c1:c0]) = 2p - 1, ie. c4 = 0 or 1
	*/
	void montgomery1(uint64_t pp, const Reg64& c4, const Reg64& c3, const Reg64& c2, const Reg64& c1, const Reg64& c0,
		const Reg64& px, const Reg64& y, const Reg64& p,
		const Reg64& t0, const Reg64& t1, const Reg64& t2, const Reg64& t3, const Reg64& t4, bool isFirst)
	{
		if (isFirst) {
			mul4x1(px, y, c3, c2, c1, c0, c4);
			mov(c4, rdx);
			// [c4:y:c2:c1:c0] = px[3..0] * y
		} else {
			mul4x1(px, y, t3, t2, t1, t0, t4);
			// [rdx:y:t2:t1:t0] = px[3..0] * y
			add_rr(y, c2, c1, c0, c3, t2, t1, t0);
			adc(c4, rdx);
		}
		mov(rax, pp);
		mul(c0); // q = rax
		mov(c3, rax);
		mul4x1(p, c3, t3, t2, t1, t0, t4);
		add(c0, t0);
		mov(c0, 0);
		adc(c1, t1);
		adc(c2, t2);
		adc(c3, y);
		adc(c4, rdx);
		adc(c0, 0);
	}
	FpGenerator(const FpGenerator&);
	void operator=(const FpGenerator&);
};

} // mie
