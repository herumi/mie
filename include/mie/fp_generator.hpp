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
	typedef Xbyak::Operand Operand;
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

	// preInv
	typedef int (*int2op)(uint64_t*, const uint64_t*);
	bool3op addNc_;
	bool3op subNc_;
	void3op add_;
	void3op sub_;
	void3op mul_;
	uint3opI mulI_;
	void2op neg_;
	void2op shr1_;
	int2op preInv_;
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
		, preInv_(0)
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
		preInv_ = getCurr<int2op>();
		gen_preInv();
	}
	void gen_addSubNc(bool isAdd)
	{
		StackFrame sf(this, 3);
		if (isAdd) {
			gen_raw_add(sf.p[0], sf.p[1], sf.p[2], rax);
		} else {
			gen_raw_sub(sf.p[0], sf.p[1], sf.p[2], rax);
		}
		setc(al);
		movzx(eax, al);
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
	static inline void put_debug_inner(uint64_t k, const uint64_t *ptr, int n)
	{
		printf("A %3d  ", (int)k);
		for (int i = n - 1; i >= 0; i--) {
			printf("%016llx", (long long)ptr[i]);
		}
		printf("\n");
	}
#ifdef _MSC_VER
	void put_debug(const Reg64& k, const RegExp& m, int n)
	{
		static uint64_t save[7];
		// don't change rsp
		push(rax);
		mov(rax, (size_t)save);
		mov(ptr [rax + 8 * 0], rcx);
		mov(ptr [rax + 8 * 1], rdx);
		mov(ptr [rax + 8 * 2], r8);
		mov(ptr [rax + 8 * 3], r9);
		mov(ptr [rax + 8 * 4], r10);
		mov(ptr [rax + 8 * 5], r11);
		pop(rax);
		mov(rcx, (size_t)save);
		mov(ptr [rcx + 8 * 6], rax);

		mov(rcx, k);
		lea(rdx, ptr [m]);
		mov(r8, n);
		mov(rax, (size_t)put_debug_inner);
		sub(rsp, 32);
		call(rax);
		add(rsp, 32);

		mov(rcx, (size_t)save);
		mov(rax, ptr [rcx + 8 * 6]);
		push(rax);
		mov(rax, (size_t)save);
		mov(rcx, ptr [rax + 8 * 0]);
		mov(rdx, ptr [rax + 8 * 1]);
		mov(r8, ptr [rax + 8 * 2]);
		mov(r9, ptr [rax + 8 * 3]);
		mov(r10, ptr [rax + 8 * 4]);
		mov(r11, ptr [rax + 8 * 5]);
		pop(rax);
	}
#endif
	/*
		QQQ
		refactor this code. this is very complex
	*/
	struct MixPack {
		const Pack *p;
		int pn;
		const RegExp *m;
		int mn;
		MixPack(const Pack *p, const RegExp *m, int mn)
			: p(p), pn(p ? (int)p->size() : 0), m(m), mn(mn)
		{
		}
		int size() const { return pn + mn; }
		bool isReg(int n) const { return n < pn; }
		const Reg64& getReg(int n) const
		{
			assert(n < pn);
			return (*p)[n];
		}
		const RegExp& getMem() const { return *m; }
	};
	/*
		z >>= c
	*/
	void shr_mp(const MixPack& z, uint8_t c, const Reg64& t)
	{
		const int n = z.size();
		for (int i = 0; i < n - 1; i++) {
			if (z.isReg(i + 1)) {
				shrd(z.getReg(i), z.getReg(i + 1), c);
			} else if (z.isReg(i)) {
				mov(t, ptr [z.getMem() + (i + 1 - z.pn) * 8]);
				shrd(z.getReg(i), t, c);
			} else {
				mov(t, ptr [z.getMem() + (i + 1 - z.pn) * 8]);
				shrd(ptr [z.getMem() + (i + 1 - z.pn) * 8], t, c);
			}
		}
		if (z.isReg(n - 1)) {
			shr(z.getReg(n - 1), c);
		} else {
			shr(qword [z.getMem() + (n - 1 - z.pn) * 8], c);
		}
	}
	/*
		z *= 2
	*/
	void twice_mp(const MixPack& z, const Reg64& t)
	{
		if (z.isReg(0)) {
			add(z.getReg(0), z.getReg(0));
		} else {
			mov(t, ptr [z.getMem()]);
			add(t, t);
			mov(ptr [z.getMem()], t);
		}
		const int n = z.size();
		for (int i = 1; i < n; i++) {
			if (z.isReg(i)) {
				adc(z.getReg(i), z.getReg(i));
			} else {
				mov(t, ptr [z.getMem() + (i - z.pn) * 8]);
				adc(t, t);
				mov(ptr [z.getMem() + (i - z.pn) * 8], t);
			}
		}
	}
	/*
		z += x
	*/
	void add_mp(const MixPack& z, const MixPack& x, const Reg64& t)
	{
		assert(z.size() == x.size());
		if (z.isReg(0)) {
			if (x.isReg(0)) {
				add(z.getReg(0), x.getReg(0));
			} else {
				add(z.getReg(0), ptr [x.getMem()]);
			}
		} else {
			mov(t, ptr [z.getMem()]);
			if (x.isReg(0)) {
				add(t, x.getReg(0));
			} else {
				add(t, ptr [x.getMem()]);
			}
			mov(ptr [z.getMem()], t);
		}
		for (int i = 1, n = z.size(); i < n; i++) {
			if (z.isReg(i)) {
				if (x.isReg(i)) {
					adc(z.getReg(i), x.getReg(i));
				} else {
					adc(z.getReg(i), ptr [x.getMem() + (i - x.pn) * 8]);
				}
			} else {
				mov(t, ptr [z.getMem() + (i - z.pn) * 8]);
				if (x.isReg(i)) {
					adc(t, x.getReg(i));
				} else {
					adc(t, ptr [x.getMem() + (i - x.pn) * 8]);
				}
				mov(ptr [z.getMem() + (i - z.pn) * 8], t);
			}
		}
	}
	/*
		z -= x
	*/
	void sub_mp(const MixPack& z, const MixPack& x, const Reg64& t)
	{
		assert(z.size() == x.size());
		if (z.isReg(0)) {
			if (x.isReg(0)) {
				sub(z.getReg(0), x.getReg(0));
			} else {
				sub(z.getReg(0), ptr [x.getMem()]);
			}
		} else {
			mov(t, ptr [z.getMem()]);
			if (x.isReg(0)) {
				sub(t, x.getReg(0));
			} else {
				sub(t, ptr [x.getMem()]);
			}
			mov(ptr [z.getMem()], t);
		}
		for (int i = 1, n = z.size(); i < n; i++) {
			if (z.isReg(i)) {
				if (x.isReg(i)) {
					sbb(z.getReg(i), x.getReg(i));
				} else {
					sbb(z.getReg(i), ptr [x.getMem() + (i - x.pn) * 8]);
				}
			} else {
				mov(t, ptr [z.getMem() + (i - z.pn) * 8]);
				if (x.isReg(i)) {
					sbb(t, x.getReg(i));
				} else {
					sbb(t, ptr [x.getMem() + (i - x.pn) * 8]);
				}
				mov(ptr [z.getMem() + (i - z.pn) * 8], t);
			}
		}
	}
	void store_mp(const RegExp& m, const MixPack& z, const Reg64& t)
	{
		for (int i = 0, n = z.size(); i < n; i++) {
			if (z.isReg(i)) {
				mov(ptr [m + i * 8], z.getReg(i));
			} else {
				mov(t, ptr [z.getMem() + (i - z.pn) * 8]);
				mov(ptr [m + i * 8], t);
			}
		}
	}
	void load_mp(const MixPack& z, const RegExp& m, const Reg64& t)
	{
		for (int i = 0, n = z.size(); i < n; i++) {
			if (z.isReg(i)) {
				mov(z.getReg(i), ptr [m + i * 8]);
			} else {
				mov(t, ptr [m + i * 8]);
				mov(ptr [z.getMem() + (i - z.pn) * 8], t);
			}
		}
	}
	void set_mp(const MixPack& z, const Reg64& t)
	{
		for (int i = 0, n = z.size(); i < n; i++) {
			if (z.isReg(i)) {
				mov(z.getReg(i), t);
			} else {
				mov(ptr [z.getMem() + (i - z.pn) * 8], t);
			}
		}
	}
	void mov_mp(const MixPack& z, const MixPack& x, const Reg64& t)
	{
		for (int i = 0, n = z.size(); i < n; i++) {
			if (z.isReg(i)) {
				if (x.isReg(i)) {
					mov(z.getReg(i), x.getReg(i));
				} else {
					mov(z.getReg(i), ptr [x.getMem() + (i - x.pn) * 8]);
				}
			} else {
				if (x.isReg(i)) {
					mov(ptr [z.getMem() + (i - z.pn) * 8], x.getReg(i));
				} else {
					mov(t, ptr [x.getMem() + (i - x.pn) * 8]);
					mov(ptr [z.getMem() + (i - z.pn) * 8], t);
				}
			}
		}
	}
	/*
		input (r, x) = (p0, p1)
		int k = preInvC(r, x)
	*/
	void gen_preInv()
	{
		assert(pn_ >= 2);
		const int freeRegNum = 13;
		if (pn_ > 8) {
			fprintf(stderr, "pn_ = %d is not supported\n", pn_);
			exit(1);
		}
		StackFrame sf(this, 2, 10 | UseRDX | UseRCX, (std::max(0, pn_ * 5 - freeRegNum) + 1 + (isFullBit_ ? 1 : 0)) * 8);
		const Reg64& pr = sf.p[0];
		const Reg64& px = sf.p[1];
		const Reg64& t = rcx;
		/*
			rax, : k, rcx : temporary
			use rdx, pr, px in main loop, so we can use 13 registers
			v = t[0, pn_) : all registers
		*/
#if 1
		int rspPos = 0;

		assert((int)sf.t.size() >= pn_);
		const Pack v = sf.t.sub(0, pn_);
		const MixPack vv(&v, 0, 0);

		Pack remain = sf.t.sub(pn_);
		if (pn_ > 2) {
			remain.append(rdx).append(pr).append(px);
		}

		const int uRegNum = std::min((int)remain.size(), pn_);
		const int uMemNum = pn_ - uRegNum;
		const Pack u = remain.sub(0, uRegNum);
		const RegExp uMem = rsp + rspPos;
		const MixPack uu(&u, &uMem, uMemNum);
		rspPos += uMemNum * 8;

		remain = remain.sub(uRegNum);

		const int rRegNum = std::min((int)remain.size(), pn_);
		const int rMemNum = pn_ - rRegNum;
		const Pack r = remain.sub(0, rRegNum);
		const RegExp rMem = rsp + rspPos;
		const MixPack rr(&r, &rMem, rMemNum);
		rspPos += rMemNum * 8;
		remain = remain.sub(rRegNum);

		const int sRegNum = std::min((int)remain.size(), pn_);
		const int sMemNum = pn_ - sRegNum;
		const Pack s = remain.sub(0, sRegNum);
		const RegExp sMem = rsp + rspPos;
		const MixPack ss(&s, &sMem, sMemNum);
		rspPos += sMemNum * 8;
		remain = remain.sub(sRegNum);

		const int keepRegNum = std::min((int)remain.size(), pn_);
		const int keepMemNum = pn_ - keepRegNum;
		const Pack keep = remain.sub(0, keepRegNum);
		const RegExp keepMem = rsp + rspPos;
		const MixPack keep_v(&keep, &keepMem, keepMemNum);
		rspPos += keepMemNum * 8;
		remain = remain.sub(keepRegNum);

		const RegExp keep_pr = rsp + rspPos;
		rspPos += 8;
		const RegExp rTop = rsp + rspPos; // used if isFullBit_
#else
		const RegExp keep = rsp + 0;
		const MixPack keep_v(0, &keep, pn_);
		const RegExp r = keep + pn_ * 8;
		const RegExp s = r + pn_ * 8;
		const RegExp keep_pr = s + pn_ * 8;
		const RegExp rTop = keep_pr + 8; // used if isFullBit_
		const Pack v = sf.t.sub(0, pn_);
		const Pack u = sf.t.sub(pn_, pn_);
		const MixPack vv(&v, 0, 0);
		const MixPack uu(&u, 0, 0);
		const MixPack rr(0, &r, pn_);
		const MixPack ss(0, &s, pn_);
#endif
		inLocalLabel();
		mov(ptr [keep_pr], pr);
		load_rm(v, px); // v = x
		// px is free frome here
		mov(rax, (size_t)p_);
		load_rm(u, rax); // u = p_
		// k = 0
		xor_(rax, rax);
		// rTop = 0
		if (isFullBit_) {
			mov(ptr [rTop], rax);
		}
		// r = 0;
		set_mp(rr, rax);
		// s = 1
		set_mp(ss, rax);
		if (ss.isReg(0)) {
			mov(ss.getReg(0), 1);
		} else {
			mov(qword [ss.getMem()], 1);
		}
		jmp(".lp");
		align(16);
	L(".lp");
		or_r(t, v);
		jz(".exit", T_NEAR);

		test(u[0], 1);
		jz(".u_even", T_NEAR);
		test(v[0], 1);
		jz(".v_even", T_NEAR);
		mov_mp(keep_v, vv, t);
		sub_mp(vv, uu, t);
		jc(".v_lt_u", T_NEAR);
		add_mp(ss, rr, t);
	L(".v_even");
		shr_mp(vv, 1, t);
		twice_mp(rr, t);
		if (isFullBit_) {
			sbb(t, t);
			mov(ptr [rTop], t);
		}
		inc(rax);
		jmp(".lp", T_NEAR);
	L(".v_lt_u");
		mov_mp(vv, keep_v, t);
		sub_mp(uu, vv, t);
		add_mp(rr, ss, t);
		if (isFullBit_) {
			sbb(t, t);
			mov(ptr [rTop], t);
		}
	L(".u_even");
		shr_mp(uu, 1, t);
		twice_mp(ss, t);
		inc(rax);
		jmp(".lp", T_NEAR);
	L(".exit");
		mov(t, (size_t)p_);
		load_rm(v, t); // v = p
		if (isFullBit_) {
			mov(t, ptr [rTop]);
			test(t, t);
			jz("@f");
			sub_mp(rr, vv, t);
		L("@@");
		}
		sub_mp(vv, rr, t);
		jnc("@f");
		mov(t, (size_t)p_);
		add_rm(v, t);
	L("@@");
		mov(rcx, ptr [keep_pr]);
		store_mr(rcx, v);
		outLocalLabel();
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
		z[] -= x[]
	*/
	void sub_rr(const Pack& z, const Pack& x)
	{
		sub(z[0], x[0]);
		assert(z.size() == x.size());
		for (size_t i = 1, n = z.size(); i < n; i++) {
			sbb(z[i], x[i]);
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
		t = all or x[i]
		ZF = x is zero
	*/
	void or_r(const Reg64& t, const Pack& x)
	{
		const int n = (int)x.size();
		if (n == 1) {
			test(x[0], x[0]);
		} else {
			mov(t, x[0]);
			for (int i = 1; i < n; i++) {
				or_(t, x[i]);
			}
		}
	}
	/*
		m[] *= 2
	*/
	void twice_m(const RegExp& m, int n, const Reg64& t)
	{
		for (int i = 0; i < n; i++) {
			mov(t, ptr [m + i * 8]);
			if (i == 0) {
				add(t, t);
			} else {
				adc(t, t);
			}
			mov(ptr [m + i * 8], t);
		}
	}
	/*
		z[] *= 2
	*/
	void twice_r(const Pack& z)
	{
		const int n = (int)z.size();
		add(z[0], z[0]);
		for (int i = 1; i < n; i++) {
			adc(z[i], z[i]);
		}
	}
	/*
		z[] >>= c
	*/
	void shr_r(const Pack& z, uint8_t c)
	{
		const int n = (int)z.size();
		for (int i = 0; i < n - 1; i++) {
			shrd(z[i], z[i + 1], c);
		}
		shr(z[n - 1], c);
	}
	/*
		mz[] += mx[]
	*/
	void add_mm(const RegExp& mz, const RegExp& mx, int n, const Reg64& t)
	{
		for (int i = 0; i < n; i++) {
			mov(t, ptr [mx + i * 8]);
			if (i == 0) {
				add(ptr [mz + i * 8], t);
			} else {
				adc(ptr [mz + i * 8], t);
			}
		}
	}
	/*
		mz[] -= mx[]
	*/
	void sub_mm(const RegExp& mz, const RegExp& mx, int n, const Reg64& t)
	{
		for (int i = 0; i < n; i++) {
			mov(t, ptr [mx + i * 8]);
			if (i == 0) {
				sub(ptr [mz + i * 8], t);
			} else {
				sbb(ptr [mz + i * 8], t);
			}
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
