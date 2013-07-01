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
#ifndef XBYAK_NO_OP_NAMES
	#define XBYAK_NO_OP_NAMES
#endif
#include <xbyak/xbyak.h>
#ifdef XBYAK32
	#error "not support 32-bit mode"
#endif
#include <xbyak/xbyak_util.h>

namespace mie {

namespace montgomery {

/*
	get pp such that p * pp = -1 mod M,
	where p is prime and M = 1 << 64(or 32).
	@param pLow [in] p mod M
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
	typedef Xbyak::RegExp RegExp;
	typedef Xbyak::Reg64 Reg64;
	typedef Xbyak::util::StackFrame StackFrame;
	typedef Xbyak::util::Pack Pack;
	static const int UseRDX = Xbyak::util::UseRDX;
	static const int UseRCX = Xbyak::util::UseRCX;
	Xbyak::util::Cpu cpu_;
	const uint64_t *p_;
	uint64_t pp_;
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
	void3op mul_;
	uint3opI mulI_;
	void2op neg_;
	void2op shr1_;
	FpGenerator()
		: p_(0)
		, pp_(0)
		, pn_(0)
		, isFullBit_(0)
		, addNc_(0)
		, subNc_(0)
		, add_(0)
		, sub_(0)
		, mul_(0)
		, mulI_(0)
		, neg_(0)
		, shr1_(0)
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
		pp_ = montgomery::getCoff(p[0]);
		pn_ = (int)pn;
		isFullBit_ = (p_[pn_ - 1] >> 63) != 0;
//		printf("p=%p, pn_=%d, isFullBit_=%d\n", p_, pn_, isFullBit_);

		setSize(0); // reset code
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
		align(16);
		mul_ = getCurr<void3op>();
		gen_mul();
		align(16);
		shr1_ = getCurr<void2op>();
		gen_shr1();
	}
	void gen_addSubNc(bool isAdd)
	{
		StackFrame sf(this, 3);
		if (isAdd) {
			gen_raw_add(sf.p[0], sf.p[1], sf.p[2], rax);
		} else {
			gen_raw_sub(sf.p[0], sf.p[1], sf.p[2], rax);
		}
		if (isFullBit_) {
			setc(al);
			movzx(eax, al);
		} else {
			xor_(eax, eax);
		}
	}
	/*
		pz[] = px[] + py[]
	*/
	void gen_raw_add(const RegExp& pz, const RegExp& px, const RegExp& py, const Reg64& t)
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
	void gen_raw_sub(const RegExp& pz, const RegExp& px, const RegExp& py, const Reg64& t)
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
	void gen_raw_neg(const RegExp& pz, const RegExp& px, const Reg64& t0, const Reg64& t1)
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
	void gen_raw_mulI(const RegExp& pz, const RegExp& px, const Reg64& y, const RegExp& pw, const Reg64& t, int n)
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
	/*
		(rdx:pz[]) = px[] * y
		use rax, rdx, pw[]
	*/
	void gen_raw_mulI_with_mulx(const RegExp& pz, const RegExp& px, const Reg64& y, const Reg64& t0, const Reg64& t1, int n)
	{
		mov(rdx, ptr [px]);
		mulx(rax, t0, y);
		mov(ptr [pz], t0);
		const Reg64 *pt0 = &rax;
		const Reg64 *pt1 = &t1;
		for (int i = 1; i < n; i++) {
			mov(rdx, ptr [px + 8 * i]);
			mulx(*pt1, t0, y);
			if (i == 1) {
				add(*pt0, t0);
			} else {
				adc(*pt0, t0);
			}
			mov(ptr [pz + 8 * i], *pt0);
			std::swap(pt0, pt1);
		}
		if (pt1 == &rax) mov(rax, *pt0);
		adc(rax, 0);
	}
	void gen_mulI()
	{
		const bool useMulx = cpu_.has(Xbyak::util::Cpu::tGPR2);
		if (useMulx) {
			printf("use mulx for mulI(%d)\n", pn_);
			// mulx H, L, x ; [H:L] = x * rdx
			StackFrame sf(this, 3, 2 | UseRDX);
			const Reg64& pz = sf.p[0];
			const Reg64& px = sf.p[1];
			const Reg64& y = sf.p[2];
			gen_raw_mulI_with_mulx(pz, px, y, sf.t[0], sf.t[1], pn_);
			return;
		}
		StackFrame sf(this, 3, 1 | UseRDX, pn_ * 8);
		const Reg64& pz = sf.p[0];
		const Reg64& px = sf.p[1];
		const Reg64& y = sf.p[2];
		gen_raw_mulI(pz, px, y, rsp, sf.t[0], pn_);
		mov(rax, rdx);
	}
	/*
		pz[] = px[]
	*/
	void gen_mov(const RegExp& pz, const RegExp& px, const Reg64& t, int n)
	{
		for (int i = 0; i < n; i++) {
			mov(t, ptr [px + i * 8]);
			mov(ptr [pz + i * 8], t);
		}
	}
	void gen_addMod3()
	{
		StackFrame sf(this, 3, 7);
		const Reg64& pz = sf.p[0];
		const Reg64& px = sf.p[1];
		const Reg64& py = sf.p[2];

		const Reg64& t0 = sf.t[0];
		const Reg64& t1 = sf.t[1];
		const Reg64& t2 = sf.t[2];
		const Reg64& t3 = sf.t[3];
		const Reg64& t4 = sf.t[4];
		const Reg64& t5 = sf.t[5];
		const Reg64& t6 = sf.t[6];

		xor_(t6, t6);
		load_rm(Pack(t2, t1, t0), px);
		add_rm(Pack(t2, t1, t0), py);
		mov_rr(Pack(t5, t4, t3), Pack(t2, t1, t0));
		adc(t6, 0);
		mov(rax, (size_t)p_);
		sub_rm(Pack(t5, t4, t3), rax);
		sbb(t6, 0);
		cmovc(t5, t2);
		cmovc(t4, t1);
		cmovc(t3, t0);
		store_mr(pz, Pack(t5, t4, t3));
	}
	void gen_subMod3()
	{
		StackFrame sf(this, 3, 4);
		const Reg64& pz = sf.p[0];
		const Reg64& px = sf.p[1];
		const Reg64& py = sf.p[2];

		Pack r1 = sf.t.sub(0, 2);
		r1.append(px); // r1 = [px, t1, t0]
		Pack r2 = sf.t.sub(2, 2);
		r2.append(rax); // r2 = [rax, t3, t2]

		load_rm(r1, px); // destroy px
		sub_rm(r1, py);
		sbb(py, py); // py = (x > y) ? 0 : -1
		mov(rax, (size_t)p_);
		load_rm(r2, rax); // destroy rax
		for (size_t i = 0; i < r2.size(); i++) {
			and_(r2[i], py);
		}
		add_rr(r1, r2);
		store_mr(pz, r1);
	}
	void gen_addMod()
	{
		if (pn_ == 3) {
			gen_addMod3();
			return;
		}
		StackFrame sf(this, 3, 0, pn_ * 8);
		const Reg64& pz = sf.p[0];
		const Reg64& px = sf.p[1];
		const Reg64& py = sf.p[2];

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
		if (pn_ == 3) {
			gen_subMod3();
			return;
		}
		StackFrame sf(this, 3);
		const Reg64& pz = sf.p[0];
		const Reg64& px = sf.p[1];
		const Reg64& py = sf.p[2];

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
		const Reg64& pz = sf.p[0];
		const Reg64& px = sf.p[1];
		gen_raw_neg(pz, px, sf.t[0], sf.t[1]);
	}
	void gen_shr1()
	{
		const int c = 1;
		StackFrame sf(this, 2, 1);
		const Reg64 *t0 = &rax;
		const Reg64 *t1 = &sf.t[0];
		const Reg64& pz = sf.p[0];
		const Reg64& px = sf.p[1];
		mov(*t0, ptr [px]);
		for (int i = 0; i < pn_ - 1; i++) {
			mov(*t1, ptr [px + 8 * (i + 1)]);
			shrd(*t0, *t1, c);
			mov(ptr [pz + i * 8], *t0);
			std::swap(t0, t1);
		}
		shr(*t0, c);
		mov(ptr [pz + (pn_ - 1) * 8], *t0);
	}
	void gen_mul()
	{
		switch (pn_) {
		case 3:
			gen_montMul3(p_, pp_);
			break;
		case 4:
			gen_montMul4(p_, pp_);
			break;
		default:
			printf("gen_mul is not implemented for n=%d\n", pn_);
			break;
		}
	}
	/*
		input (z, x, y) = (p0, p1, p2)
		z[0..3] <- montgomery(x[0..3], y[0..3])
		destroy gt0, ..., gt9, xm0, xm1, p2
	*/
	void gen_montMul4(const uint64_t *p, uint64_t pp)
	{
		StackFrame sf(this, 3, 10 | UseRDX);
		const Reg64& p0 = sf.p[0];
		const Reg64& p1 = sf.p[1];
		const Reg64& p2 = sf.p[2];

		const Reg64& t0 = sf.t[0];
		const Reg64& t1 = sf.t[1];
		const Reg64& t2 = sf.t[2];
		const Reg64& t3 = sf.t[3];
		const Reg64& t4 = sf.t[4];
		const Reg64& t5 = sf.t[5];
		const Reg64& t6 = sf.t[6];
		const Reg64& t7 = sf.t[7];
		const Reg64& t8 = sf.t[8];
		const Reg64& t9 = sf.t[9];

		movq(xm0, p0); // save p0
		mov(p0, (uint64_t)p);
		movq(xm1, p2);
		mov(p2, ptr [p2]);
		montgomery4_1(pp, t0, t7, t3, t2, t1, p1, p2, p0, t4, t5, t6, t8, t9, true);

		movq(p2, xm1);
		mov(p2, ptr [p2 + 8]);
		montgomery4_1(pp, t1, t0, t7, t3, t2, p1, p2, p0, t4, t5, t6, t8, t9, false);

		movq(p2, xm1);
		mov(p2, ptr [p2 + 16]);
		montgomery4_1(pp, t2, t1, t0, t7, t3, p1, p2, p0, t4, t5, t6, t8, t9, false);

		movq(p2, xm1);
		mov(p2, ptr [p2 + 24]);
		montgomery4_1(pp, t3, t2, t1, t0, t7, p1, p2, p0, t4, t5, t6, t8, t9, false);
		// [t7:t3:t2:t1:t0]

		mov(t4, t0);
		mov(t5, t1);
		mov(t6, t2);
		mov(rdx, t3);
		sub_rm(Pack(t3, t2, t1, t0), p0);
		if (isFullBit_) sbb(t7, 0);
		cmovc(t0, t4);
		cmovc(t1, t5);
		cmovc(t2, t6);
		cmovc(t3, rdx);

		movq(p0, xm0); // load p0
		store_mr(p0, Pack(t3, t2, t1, t0));
	}
	/*
		input (z, x, y) = (p0, p1, p2)
		z[0..2] <- montgomery(x[0..2], y[0..2])
		destroy gt0, ..., gt9, xm0, xm1, p2
	*/
	void gen_montMul3(const uint64_t *p, uint64_t pp)
	{
		StackFrame sf(this, 3, 10 | UseRDX);
		const Reg64& p0 = sf.p[0];
		const Reg64& p1 = sf.p[1];
		const Reg64& p2 = sf.p[2];

		const Reg64& t0 = sf.t[0];
		const Reg64& t1 = sf.t[1];
		const Reg64& t2 = sf.t[2];
		const Reg64& t3 = sf.t[3];
		const Reg64& t4 = sf.t[4];
		const Reg64& t5 = sf.t[5];
		const Reg64& t6 = sf.t[6];
		const Reg64& t7 = sf.t[7];
		const Reg64& t8 = sf.t[8];
		const Reg64& t9 = sf.t[9];

		movq(xm0, p0); // save p0
		mov(t7, (uint64_t)p);
		mov(t9, ptr [p2]);
		//                c3, c2, c1, c0, px, y,  p,
		montgomery3_1(pp, t0, t3, t2, t1, p1, t9, t7, t4, t5, t6, t8, p0, true);

		mov(t9, ptr [p2 + 8]);
		montgomery3_1(pp, t1, t0, t3, t2, p1, t9, t7, t4, t5, t6, t8, p0, false);

		mov(t9, ptr [p2 + 16]);
		montgomery3_1(pp, t2, t1, t0, t3, p1, t9, t7, t4, t5, t6, t8, p0, false);

		// [t3:t2:t1:t0]
		mov(t4, t0);
		mov(t5, t1);
		mov(t6, t2);
		sub_rm(Pack(t2, t1, t0), t7);
		if (isFullBit_) sbb(t3, 0);
		cmovc(t0, t4);
		cmovc(t1, t5);
		cmovc(t2, t6);
		movq(p0, xm0);
		store_mr(p0, Pack(t2, t1, t0));
	}
private:
	FpGenerator(const FpGenerator&);
	void operator=(const FpGenerator&);
	/*
		z[] = x[]
	*/
	void mov_rr(const Pack& z, const Pack& x)
	{
		assert(z.size() == x.size());
		for (int i = 0, n = (int)x.size(); i < n; i++) {
			mov(z[i], x[i]);
		}
	}
	/*
		m[] = x[]
	*/
	void store_mr(const RegExp& m, const Pack& x)
	{
		for (int i = 0, n = (int)x.size(); i < n; i++) {
			mov(ptr [m + 8 * i], x[i]);
		}
	}
	/*
		x[] = m[]
	*/
	void load_rm(const Pack& z, const RegExp& m)
	{
		for (int i = 0, n = (int)z.size(); i < n; i++) {
			mov(z[i], ptr [m + 8 * i]);
		}
	}
	/*
		z[] += x[]
	*/
	void add_rr(const Pack& z, const Pack& x)
	{
		add(z[0], x[0]);
		assert(z.size() == x.size());
		for (size_t i = 1, n = z.size(); i < n; i++) {
			adc(z[i], x[i]);
		}
	}
	/*
		z[] += m[]
	*/
	void add_rm(const Pack& z, const RegExp& m)
	{
		add(z[0], ptr [m + 8 * 0]);
		for (int i = 1, n = (int)z.size(); i < n; i++) {
			adc(z[i], ptr [m + 8 * i]);
		}
	}
	/*
		z[] -= m[]
	*/
	void sub_rm(const Pack& z, const RegExp& m)
	{
		sub(z[0], ptr [m + 8 * 0]);
		for (int i = 1, n = (int)z.size(); i < n; i++) {
			sbb(z[i], ptr [m + 8 * i]);
		}
	}
	/*
		[rdx:x:t1:t0] <- py[2:1:0] * x
		destroy x, t
	*/
	void mul3x1(const RegExp& py, const Reg64& x, const Reg64& t2, const Reg64& t1, const Reg64& t0, const Reg64& t)
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
		/*
			rdx:rax
			     t2:t
			        t1:t0
		*/
		add(t1, t);
		adc(rax, t2);
		adc(rdx, 0);
		mov(x, rax);
	}

	/*
		c = [c3:c2:c1:c0]
		c += x[2..0] * y
		q = uint64_t(c0 * pp)
		c = (c + q * p) >> 64
		input  [c3:c2:c1:c0], px, y, p
		output [c0:c3:c2:c1]

		@note use rax, rdx, destroy y
	*/
	void montgomery3_1(uint64_t pp, const Reg64& c3, const Reg64& c2, const Reg64& c1, const Reg64& c0,
		const Reg64& px, const Reg64& y, const Reg64& p,
		const Reg64& t0, const Reg64& t1, const Reg64& t2, const Reg64& t3, const Reg64& t4, bool isFirst)
	{
		if (isFirst) {
			mul3x1(px, y, c2, c1, c0, c3);
			mov(c3, rdx);
			// [c3:y:c1:c0] = px[2..0] * y
		} else {
			mul3x1(px, y, t2, t1, t0, t3);
			// [rdx:y:t1:t0] = px[2..0] * y
			if (isFullBit_) xor_(t4, t4);
			add_rr(Pack(c3, y, c1, c0), Pack(rdx, c2, t1, t0));
			if (isFullBit_) adc(t4, 0);
		}
		// [t4:c3:y:c1:c0]
		// t4 = 0 or 1 if isFullBit_, = 0 otherwise
		mov(rax, pp);
		mul(c0); // q = rax
		mov(c2, rax);
		mul3x1(p, c2, t2, t1, t0, t3);
		// [rdx:c2:t1:t0] = p * q
		add(c0, t0); // always c0 is zero
		adc(c1, t1);
		adc(c2, y);
		adc(c3, rdx);
		if (isFullBit_) {
			if (isFirst) {
				adc(c0, 0);
			} else {
				adc(c0, t4);
			}
		}
	}
	/*
		[rdx:x:t2:t1:t0] <- py[3:2:1:0] * x
		destroy x, t
	*/
	void mul4x1(const RegExp& py, const Reg64& x, const Reg64& t3, const Reg64& t2, const Reg64& t1, const Reg64& t0, const Reg64& t)
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
		c = [c4:c3:c2:c1:c0]
		c += x[3..0] * y
		q = uint64_t(c0 * pp)
		c = (c + q * p) >> 64
		input  [c4:c3:c2:c1:c0], px, y, p
		output [c0:c4:c3:c2:c1]

		@note use rax, rdx, destroy y
	*/
	void montgomery4_1(uint64_t pp, const Reg64& c4, const Reg64& c3, const Reg64& c2, const Reg64& c1, const Reg64& c0,
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
			if (isFullBit_) {
				push(px);
				xor_(px, px);
			}
			add_rr(Pack(c4, y, c2, c1, c0), Pack(rdx, c3, t2, t1, t0));
			if (isFullBit_) {
				adc(px, 0);
			}
		}
		// [px:c4:y:c2:c1:c0]
		// px = 0 or 1 if isFullBit_, = 0 otherwise
		mov(rax, pp);
		mul(c0); // q = rax
		mov(c3, rax);
		mul4x1(p, c3, t3, t2, t1, t0, t4);
		add(c0, t0); // always c0 is zero
		adc(c1, t1);
		adc(c2, t2);
		adc(c3, y);
		adc(c4, rdx);
		if (isFullBit_) {
			if (isFirst) {
				adc(c0, 0);
			} else {
				adc(c0, px);
				pop(px);
			}
		}
	}
};

} // mie
