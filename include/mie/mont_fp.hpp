#pragma once
/**
	@file
	@brief Fp with montgomery(EXPERIMENTAL IMPLEMENTAION)
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause

	@note this class should be merged to FpT
*/
#include <sstream>
#include <vector>
#include <mie/gmp_util.hpp>
#include <mie/fp.hpp>
#include <mie/fp_generator.hpp>

namespace mie {

template<size_t N, class tag = fp_local::TagDefault>
class MontFpT : public ope::addsub<MontFpT<N, tag>,
	ope::mulable<MontFpT<N, tag>,
	ope::invertible<MontFpT<N, tag>,
	ope::hasNegative<MontFpT<N, tag> > > > > {

	static mpz_class pOrg_;
	static MontFpT p_;
	static MontFpT one_;
	static MontFpT R_; // (1 << (N * 64)) % p
	static MontFpT RR_; // (R * R) % p
	static MontFpT invTbl_[N * 64 * 2];
	static FpGenerator fg_;
	uint64_t v_[N];
	/*
		QQQ:move to Gmp
	*/
	static inline void fromStr(mpz_class& z, const std::string& str, int base = 0)
	{
		const char *p = str.c_str();
		if (str.size() > 2 && str[0] == '0') {
			if (str[1] == 'x') {
				base = 16;
				p += 2;
			} else if (str[1] == 'b') {
				base = 2;
				p += 2;
			}
		}
		if (base == 0) base = 10;
		if (!Gmp::fromStr(z, p, base)) {
			throw cybozu::Exception("fp:MontFpT:fromStr") << str;
		}
	}
	void fromRawGmp(const mpz_class& x)
	{
		if (Gmp::getRaw(v_, N, x) == 0) {
			throw cybozu::Exception("MontFpT:fromRawGmp") << x;
		}
	}
	static void initInvTbl(MontFpT *invTbl)
	{
		MontFpT t(2);
		const int n = N * 64 * 2;
		for (int i = 0; i < n; i++) {
			invTbl[n - 1 - i] = t;
			t += t;
		}
	}
	typedef void (*void3op)(MontFpT&, const MontFpT&, const MontFpT&);
	typedef void (*void2op)(MontFpT&, const MontFpT&);
public:
	static const size_t BlockSize = N;
	typedef uint64_t BlockType;
	MontFpT() {}
	MontFpT(int x) { operator=(x); }
	MontFpT(uint64_t x) { operator=(x); }
	explicit MontFpT(const std::string& str)
	{
		fromStr(str);
	}
	MontFpT(const MontFpT& x)
	{
		*this = x;
	}
	MontFpT& operator=(const MontFpT& x)
	{
		for (size_t i = 0; i < N; i++) v_[i] = x.v_[i];
		return *this;
	}
	MontFpT& operator=(int x)
	{
		if (x == 0) {
			clear();
		} else {
			v_[0] = abs(x);
			for (size_t i = 1; i < N; i++) v_[i] = 0;
			mul(*this, *this, RR_);
			if (x < 0) {
				neg(*this, *this);
			}
		}
		return *this;
	}
	MontFpT& operator=(uint64_t x)
	{
		v_[0] = x;
		for (size_t i = 1; i < N; i++) v_[i] = 0;
		mul(*this, *this, RR_);
		return *this;
	}
	// QQQ:refactor with fromStr(mpz_class)
	void fromStr(const std::string& str, int base = 0)
	{
		if (str.empty() || str[0] == '-') {
			throw cybozu::Exception("fp:MontFpT:fromStr") << str;
		}
		{
			const char *p = str.c_str();
			size_t size = str.size();
			if (p[0] == '0') {
				if (p[1] == 'x') {
					base = 16;
					p += 2;
					size -= 2;
				} else if (p[1] == 'b') {
					base = 2;
					p += 2;
					size -= 2;
				}
			}
			if (base == 16) {
				MontFpT t;
				mie::fp::fromStr16(t.v_, N, p, size);
				mul(*this, t, RR_);
				return;
			}
		}
		mpz_class t;
		fromStr(t, str, base);
		toMont(*this, t);
	}
	void set(const std::string& str, int base = 0) { fromStr(str, base); }
	void toStr(std::string& str, int base = 10) const
	{
		if (base == 16) {
			MontFpT t;
			mul(t, *this, one_);
			mie::fp::toStr16(str, t.v_, N);
			return;
		}
		mpz_class t;
		fromMont(t, *this);
		Gmp::toStr(str, t, base);
		if (base == 16) {
			str.insert(0, "0x");
		} else if (base == 2) {
			str.insert(0, "0b");
		} else if (base != 10) {
			throw cybozu::Exception("fp:MontFpT:toStr:bad base") << base;
		}
	}
	std::string toStr(int base = 10) const
	{
		std::string str;
		toStr(str, base);
		return str;
	}
	void clear()
	{
		for (size_t i = 0; i < N; i++) v_[i] = 0;
	}
	template<class RG>
	void initRand(RG& rg, size_t bitLen)
	{
		const size_t rem = bitLen & 31;
		const size_t n = (bitLen + 31) / 32;
		std::vector<unsigned int> buf(n);
		rg.read(&buf[0], n);
		if (rem > 0) buf[n - 1] &= (1U << rem) - 1;
		buf[n - 1] |= 1U << rem;
		setRaw(&buf[0], n);
	}
	template<class S>
	void setRaw(const S *buf, size_t n)
	{
		if (n * sizeof(S) > N * 8) {
			throw cybozu::Exception("MontFp:setRaw:too large") << n;
		}
		clear();
		memcpy(v_, buf, n * sizeof(S));
		// QQQ : mod
	}
	static inline void setModulo(const std::string& pstr, int base = 0)
	{
		if (pstr.empty() || pstr[0] == '-') {
			throw cybozu::Exception("MontFpT:setModulo") << pstr;
		}
		fromStr(pOrg_, pstr, base);
		if ((Gmp::getBitLen(pOrg_) + 63) / 64 != N) {
			throw cybozu::Exception("MontFp:setModulo:bad prime length") << pstr;
		}
		p_.fromRawGmp(pOrg_);
		mpz_class t = 1;
		one_.fromRawGmp(t);
		t = (t << (N * 64)) % pOrg_;
		R_.fromRawGmp(t);
		t = (t * t) % pOrg_;
		RR_.fromRawGmp(t);
		fg_.init(p_.v_, N);
		add = Xbyak::CastTo<void3op>(fg_.add_);
		sub = Xbyak::CastTo<void3op>(fg_.sub_);
		mul = Xbyak::CastTo<void3op>(fg_.mul_);
		neg = Xbyak::CastTo<void2op>(fg_.neg_);
		shr1 = Xbyak::CastTo<void2op>(fg_.shr1_);
		addNc = Xbyak::CastTo<void3op>(fg_.addNc_);
		subNc = Xbyak::CastTo<void3op>(fg_.subNc_);
		initInvTbl(invTbl_);
	}
	static inline void getModulo(std::string& pstr)
	{
		Gmp::toStr(pstr, pOrg_);
	}
	static inline void fromMont(mpz_class& z, const MontFpT& x)
	{
		MontFpT t;
		mul(t, x, one_);
		Gmp::setRaw(z, t.v_, N);
	}
	static inline void toMont(MontFpT& z, const mpz_class& x)
	{
		MontFpT t;
		t.fromRawGmp(x);
		mul(z, t, RR_);
	}
	static void3op add;
	static void3op sub;
	static void3op mul;
	static void2op neg;
	static void2op shr1;
	static void3op addNc;
	static void3op subNc;
	static inline void square(MontFpT& z, const MontFpT& x)
	{
		mul(z, x, x);
	}
	static inline void inv(MontFpT& z, const MontFpT& x)
	{
#if 0
		MontFpT u, v, r, s;
		u = p_;
		v = x;
		r.clear();
		s.clear(); s.v_[0] = 1; // s is real 1
		int k = 0;
		while (!v.isZero()) {
			if ((u.v_[0] & 1) == 0) {
				shr1(u, u);
				addNc(s, s, s);
			} else if ((v.v_[0] & 1) == 0) {
				shr1(v, v);
				addNc(r, r, r);
			} else if (compare(v, u) >= 0) {
				subNc(v, v, u);
				addNc(s, s, r);
				shr1(v, v);
				addNc(r, r, r);
			} else {
				subNc(u, u, v);
				addNc(r, r, s);
				shr1(u, u);
				addNc(s, s, s);
			}
			k++;
		}
		if (compare(r, p_) >= 0) {
			subNc(r, r, p_);
		}
		assert(!r.isZero());
		subNc(r, p_, r);
		/*
			xr = 2^k
			R = 2^256
			get r2^(-k)R^2 = r 2^(512 - k)
		*/
		mul(z, r, invTbl_[k]);
#else
		mpz_class t;
		fromMont(t, x);
		Gmp::invMod(t, t, pOrg_);
		toMont(z, t);
#endif
	}
	static inline void div(MontFpT& z, const MontFpT& x, const MontFpT& y)
	{
		MontFpT ry;
		inv(ry, y);
		mul(z, x, ry);
	}
#if 0
	static inline BlockType getBlock(const MontFpT& x, size_t i)
	{
		return Gmp::getBlock(x.v, i);
	}
	static inline const BlockType *getBlock(const MontFpT& x)
	{
		return Gmp::getBlock(x.v);
	}
	static inline size_t getBlockSize(const MontFpT& x)
	{
		return Gmp::getBlockSize(x.v);
	}
	static inline void shr(MontFpT& z, const MontFpT& x, size_t n)
	{
		z.v = x.v >> n;
	}
#endif
	static inline int compare(const MontFpT& x, const MontFpT& y)
	{
		for (size_t i = 0; i < N; i++) {
			const uint64_t a = x.v_[N - 1 - i];
			const uint64_t b = y.v_[N - 1 - i];
			if (a > b) return 1;
			if (a < b) return -1;
		}
		return 0;
	}
	static inline bool isZero(const MontFpT& x)
	{
		if (x.v_[0]) return false;
		uint64_t r = 0;
		for (size_t i = 1; i < N; i++) {
			r |= x.v_[i];
		}
		return r == 0;
	}
	bool isZero() const { return isZero(*this); }
	friend inline std::ostream& operator<<(std::ostream& os, const MontFpT& self)
	{
		const int base = (os.flags() & std::ios_base::hex) ? 16 : 10;
		std::string str;
		self.toStr(str, base);
		return os << str;
	}
	friend inline std::istream& operator>>(std::istream& is, MontFpT& self)
	{
		const int base = (is.flags() & std::ios_base::hex) ? 16 : 0;
		std::string str;
		is >> str;
		self.fromStr(str, base);
		return is;
	}
	template<class Z>
	static void power(MontFpT& z, const MontFpT& x, const Z& y)
	{
		power_impl::power(z, x, y);
	}
	const uint64_t* getInnerValue() const { return v_; }
	bool operator==(const MontFpT& rhs) const { return compare(*this, rhs) == 0; }
	bool operator!=(const MontFpT& rhs) const { return compare(*this, rhs) != 0; }
};

template<size_t N, class tag>mpz_class MontFpT<N, tag>::pOrg_;
template<size_t N, class tag>MontFpT<N, tag> MontFpT<N, tag>::p_;
template<size_t N, class tag>MontFpT<N, tag> MontFpT<N, tag>::one_;
template<size_t N, class tag>MontFpT<N, tag> MontFpT<N, tag>::R_;
template<size_t N, class tag>MontFpT<N, tag> MontFpT<N, tag>::RR_;
template<size_t N, class tag>MontFpT<N, tag> MontFpT<N, tag>::invTbl_[N * 64 * 2];
template<size_t N, class tag>FpGenerator MontFpT<N, tag>::fg_;

template<size_t N, class tag>typename MontFpT<N, tag>::void3op MontFpT<N, tag>::add;
template<size_t N, class tag>typename MontFpT<N, tag>::void3op MontFpT<N, tag>::sub;
template<size_t N, class tag>typename MontFpT<N, tag>::void3op MontFpT<N, tag>::mul;
template<size_t N, class tag>typename MontFpT<N, tag>::void2op MontFpT<N, tag>::neg;
template<size_t N, class tag>typename MontFpT<N, tag>::void2op MontFpT<N, tag>::shr1;
template<size_t N, class tag>typename MontFpT<N, tag>::void3op MontFpT<N, tag>::addNc;
template<size_t N, class tag>typename MontFpT<N, tag>::void3op MontFpT<N, tag>::subNc;

} // mie

namespace std { CYBOZU_NAMESPACE_TR1_BEGIN
template<class T> struct hash;

template<size_t N, class tag>
struct hash<mie::MontFpT<N, tag> > : public std::unary_function<mie::MontFpT<N, tag>, size_t> {
	size_t operator()(const mie::MontFpT<N, tag>& x, uint64_t v = 0) const
	{
		return static_cast<size_t>(cybozu::hash64(x.getInnerValue(), N, v));
	}
};

CYBOZU_NAMESPACE_TR1_END } // std::tr1
