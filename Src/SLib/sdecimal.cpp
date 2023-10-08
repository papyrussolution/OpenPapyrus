// SDECIMAL.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
#include <cmath>
#include <..\OSF\abseil\absl\numeric\int128.h>

void SlI128_Add(uint64 a, uint64 b, _baseuint128_le * pResult)
{
}

void SlI128_Add(const _baseuint128_le * pA, const _baseuint128_le * pB, _baseuint128_le * pResult)
{
}

void SlI128_Sub(const _baseuint128_le * pA, const _baseuint128_le * pB, _baseint128_le * pResult)
{
}

void SlI128_Mul(uint64 a, uint64 b, _baseuint128_le * pResult)
{
}

void SlI128_Mul(const _baseuint128_le * pA, const _baseuint128_le * pB, _baseuint128_le * pResult)
{
}

void SlI128_Div(const _baseuint128_le * pA, uint64 b, uint64 * pResult)
{
}

void SlI128_Div(const _baseuint128_le * pA, uint64 b, _baseuint128_le * pResult)
{
}
//
// SUint128
// 
SUint128 & SUint128::Mul64(uint64 a, uint64 b)
{
	uint32 al = LoDWord(a);
	uint32 ah = HiDWord(a);
	uint32 bl = LoDWord(b);
	uint32 bh = HiDWord(b);
	uint64 r0 = static_cast<uint64>(al) * static_cast<uint64>(bl);
	uint64 r1 = static_cast<uint64>(al) * static_cast<uint64>(bh);
	uint64 r2 = static_cast<uint64>(ah) * static_cast<uint64>(bl);
	uint64 r3 = static_cast<uint64>(ah) * static_cast<uint64>(bh);
	r1 = r1 + HiDWord(r0); /* no carry possible */
	r1 = r1 + r2; /* but this can carry */
	if(r1 < r2) /* check */
		r3 = r3 + (1ULL << 32); // (1ULL << 32) - carry32
	hi = r3 + HiDWord(r1);
	lo = (r1 << 32) + LoDWord(r0);
	return *this;
}
//
// 
//
static uint64 GetUedMaxMantissa(uint numMantissaBits)
{
	uint64 result = 0ULL;
	assert(numMantissaBits >= 3 && numMantissaBits <= 64);
	return setlowbits64(numMantissaBits);
}

SDecimal::SDecimal() : Mant(0), Exp(0), Flags(0)
{
}

SDecimal::SDecimal(const char * pS) : Mant(0), Exp(0), Flags(0)
{
	if(!isempty(pS))
		FromStr(pS);
}

SDecimal::SDecimal(double v) : Mant(0), Exp(0), Flags(0)
{
	// @construction 
	uint64 mantissa = 0;
	int32  exp = 0;
	uint   f = SIEEE754::DoubleToDecimal(v, &mantissa, &exp);
	if(f & SIEEE754::fInternalError) {
		Flags = SIEEE754::fNAN;
	}
	else {
		// Знак exp, возвращаемый UnpackDouble противоположен знаку DenomDecPwr!
		assert((f & ~(SIEEE754::fINF|SIEEE754::fNAN|SIEEE754::fSIGN)) == 0);
		Flags = (static_cast<uint16>(f & ~SIEEE754::fSIGN)); // Флаг знака внутри класса не храним - знак числа определяется величиной Num
		if(!(Flags & (SIEEE754::fINF|SIEEE754::fNAN))) {
			if(mantissa != 0) { // В противном случае все число - 0 (мантисса, экспонента, знак)
				Mant = mantissa;
				Exp = static_cast<int16>(exp);
				if(f & SIEEE754::fSIGN)
					Mant = -Mant;
			}
		}
	}
}
	
SDecimal::SDecimal(int64 mantissa, int16 exp) : Mant(mantissa), Exp(exp), Flags(0)
{
	Normalize();
}

int SDecimal::FromStr(const char * pStr)
{
	Z();
	int    ok = 1;
	SIEEE754::ParseDecimalBlock blk;
	const char * p_end = 0;
	uint r = blk.Run(pStr, &p_end);
	if(r & SIEEE754::fInternalError) {
		ok = 0;
	}
	else if(r & SIEEE754::fNAN)
		Flags |= SIEEE754::fNAN;
	else if(r & (SIEEE754::fINF|SIEEE754::fSIGN))
		Flags |= (r & (SIEEE754::fINF|SIEEE754::fSIGN));
	else {
		Mant = satoi64(blk.DEC);
		if(blk.Flags & blk.fNegMant)
			Mant = -Mant;
		Exp = blk.DecExpOffs + ((blk.Flags & blk.fNegExp) ? -blk.Exp : blk.Exp);
		Normalize();
	}
	return ok;
}

SString & SDecimal::ToStr(long fmt, SString & rBuf) const
{
	rBuf.Z();
	if(IsZero())
		rBuf = "0";
	else {
		const bool sign = (Mant < 0);
		rBuf.Cat(abs(Mant));
		if(Exp > 0) {
			rBuf.CatCharN('0', Exp);
		}
		else if(Exp < 0) {
			if(-Exp < rBuf.Len())
				rBuf.Insert(rBuf.Len() + Exp, ".");
			else {
				rBuf.PadLeft(-Exp - rBuf.Len(), '0').Insert(0, "0.");
			}
		}
		if(sign)
			rBuf.Insert(0, "-");
	}
	if(fmt) {
		const int prc = SFMTPRC(fmt);
		//
	}
	return rBuf;
}

bool FASTCALL SDecimal::IsEq(const SDecimal & rS) const // @construction
{
	return (Mant == rS.Mant && Exp == rS.Exp && Flags == rS.Flags);
}
	
double SDecimal::GetReal() const
{
	//assert(Exp > -8 && Exp < 8);
	return static_cast<double>(Mant) * fpow10i(Exp);
}

bool SDecimal::Normalize(int64 mant, int16 exp)
{
	//
	// Под нормализацией (в контексте наших требований) предполагается
	// минимизация модуля экспоненты
	//
	bool ok = true;
	if(mant == 0) {
		Mant = 0;
		Exp = 0;
	}
	else {
		if(exp < 0) {
			while(exp < 0 && !(mant & 1) && (mant % 10) == 0) {
				mant /= 10;
				exp++;
			}
		}
		if(exp > 0) {
			const uint64 _max_mant_div_10 = GetUedMaxMantissa(64) / 10;
			while(exp > 0 && (uint64)abs(mant) <= _max_mant_div_10) {
				mant *= 10;
				exp--;
			}
		}
		Mant = mant;
		Exp = exp;
	}
	return ok;
}
	
bool SDecimal::Normalize()
{
	//
	// Под нормализацией (в контексте наших требований) предполагается
	// минимизация модуля экспоненты
	//
	bool ok = false;
	if(Mant == 0) {
		Exp = 0;
		ok = true;
	}
	else {
		if(Exp < 0) {
			while(Exp < 0 && !(Mant & 1) && (Mant % 10) == 0) {
				Mant /= 10;
				Exp++;
			}
			assert(Exp <= 0);
		}
		if(Exp > 0) {
			const uint64 _max_mant_div_10 = GetUedMaxMantissa(64) / 10;
			while(Exp > 0 && (uint64)abs(Mant) <= _max_mant_div_10) {
				Mant *= 10;
				Exp--;
			}
		}
		ok = true;
	}
	return ok;
}

SDecimal & SDecimal::Z()
{
	Mant = 0;
	Exp = 0;
	Flags = 0;
	return *this;
}
	
bool SDecimal::IsZero() const { return (Mant == 0); }

/*static*/SDecimal SDecimal::Neg(const SDecimal & rV)
{
	if(rV.IsZero())
		return rV;
	else
		return SDecimal(-rV.Mant, rV.Exp);
}

static uint64 RoundToNearestMul10(uint64 m) // @construction
{
	const uint64 m_div_10 = (m / 10ULL);
	const uint64 m_l = m_div_10 * 10ULL;
	uint64 result = m_l;
	if(m_l != m) {
		const uint64 dl = m - m_l;
		if(dl >= 5) {
			const uint64 m_u = m_l + 10ULL;
			if(dl > 5)
				result = m_u;
			else {
				assert(dl == 5);
				result = (m_div_10 & 1) ? m_u : m_l; // Ближайшее четное
			}
		}
	}
	return result;
}

static absl::uint128 RoundToNearestMul10(absl::uint128 m) // @construction
{
	const absl::uint128 m_div_10 = (m / 10ULL);
	const absl::uint128 m_l = m_div_10 * 10ULL;
	absl::uint128 result = m_l;
	if(m_l != m) {
		const absl::uint128 dl = m - m_l;
		if(dl >= 5) {
			const absl::uint128 m_u = m_l + 10ULL;
			if(dl > 5)
				result = m_u;
			else {
				assert(dl == 5);
				result = (m_div_10 & 1) ? m_u : m_l; // Ближайшее четное
			}
		}
	}
	return result;
}

static int TrimInt128(const absl::int128 & rVMant, int16 vexp, int64 & rMant, int16 & rExp)
{
	int   ok = 1;
	int16 exp = vexp;
	const bool sign = (rVMant < static_cast<absl::int128>(0ULL));
	absl::int128 mant(rVMant);
	if(sign) {
		while(ok && mant < INT64_MIN) {
			absl::uint128 t = RoundToNearestMul10(static_cast<absl::uint128>(-mant));
			mant = -static_cast<absl::int128>(t / 10);
			if(exp == INT16_MAX)
				ok = 0;
			else
				exp++;
		}
	}
	else {
		while(ok && mant > INT64_MAX) {
			absl::uint128 t = RoundToNearestMul10(static_cast<absl::uint128>(mant));
			mant = static_cast<absl::int128>(t / 10);
			if(exp == INT16_MAX)
				ok = 0;
			else
				exp++;
		}
	}
	if(ok) {
		assert(absl::Int128High64(mant) == 0);
		rMant = static_cast<int64>(mant);
		rExp = exp;
	}
	return ok;
}

enum UedDecSpecialValue {
	ueddecsvNormal = 0,
	ueddecsvZero,
	ueddecsvNan,
	ueddecsvInfPlus,
	ueddecsvInfMinus,
	ueddecsvUnderflow,
};

struct UedDecIndicator {
	UedDecIndicator() : Spc(ueddecsvNormal), AbsExp(0), AbsExp_Encoded(0), ExpBitCount(0), MaxMantissa(0), ExpSign(false), MantSign(false)
	{
	}
	UedDecSpecialValue Spc;
	uint   AbsExp;
	uint   AbsExp_Encoded;
	uint   ExpBitCount;
	uint64 MaxMantissa; // @!ToBits()
	bool   ExpSign;
	bool   MantSign;
	//
	// 5 bits // 3-->5
	//   000    exponent zero bits 
	//   001    exponent 3 bits [1..8]
	//   010    exponent 5 bits [9..32]
	//   011    exponent 5 bits [33..64]
	//   100    exponent 6 bits [65..128]
	//   101    reserve
	//   110    reserve
	//   111    INF
	//       00 exp+, mantissa+ // for INF: +INF
	//       01 exp+, mantissa- // for INF: -INF
	//       10 exp-, mantissa+ // for INF: UNDERFLOW
	//       11 exp-, mantissa- // for INF: NAN
	// E bits ([0..8] depends on special 3 bits above) bits  
	//   exponent (absolute)
	// (N-5-E) bits
	//   mantissa (absolute)
	//

	//
	// ARG(valueBitCount IN): 
	//   0 - биты индикатора сдвинуты в младшую часть аргумента ued
	//   32..64 - биты индикатора находятся в натуральном положении (старшая часть
	//     зоны шириной valueBitCount аргумента ued).
	//
	int    FromBits(uint valueBitCount, uint64 ued)
	{
		Spc = ueddecsvNormal;
		ExpBitCount = 0;
		AbsExp = 0;
		AbsExp_Encoded = 0;
		ExpSign = false;
		MantSign = false;
		int    ok = 1;
		assert(valueBitCount == 0 || (valueBitCount >= 24 && valueBitCount <= 64));
		THROW(valueBitCount == 0 || (valueBitCount >= 24 && valueBitCount <= 64));
		const uint64 _ind = valueBitCount ? (ued >> (valueBitCount - 5)) : ued;
		const uint sign_mask = static_cast<uint>((_ind >> 3) & 0x3ULL);
		switch(_ind & 0x7) {
			case 0:
				Spc = ueddecsvZero;
				break;
			case 1:
				ExpBitCount = 3;
				if(valueBitCount) {
					AbsExp = 1 + ((ued >> (valueBitCount - 5 - ExpBitCount)) & 0x7ULL);
				}
				switch(sign_mask) {
					case 1: MantSign = true; break;
					case 2: ExpSign = true; break;
					case 3: MantSign = true; ExpSign = true; break;
				}
				break;
			case 2:
				ExpBitCount = 5;
				if(valueBitCount) {
					AbsExp = 9 + ((ued >> (valueBitCount - 5 - ExpBitCount)) & 0x1fULL);
					THROW(AbsExp <= 32);
				}
				switch(sign_mask) {
					case 1: MantSign = true; break;
					case 2: ExpSign = true; break;
					case 3: MantSign = true; ExpSign = true; break;
				}
				break;
			case 3:
				ExpBitCount = 5;
				if(valueBitCount) {
					AbsExp = 33 + ((ued >> (valueBitCount - 5 - ExpBitCount)) & 0x1fULL);
					THROW(AbsExp <= 64);
				}
				switch(sign_mask) {
					case 1: MantSign = true; break;
					case 2: ExpSign = true; break;
					case 3: MantSign = true; ExpSign = true; break;
				}
				break;
			case 4:
				ExpBitCount = 6;
				if(valueBitCount) {
					AbsExp = 65 + ((ued >> (valueBitCount - 5 - ExpBitCount)) & 0x3fULL);
					THROW(AbsExp <= 128);
				}
				switch(sign_mask) {
					case 1: MantSign = true; break;
					case 2: ExpSign = true; break;
					case 3: MantSign = true; ExpSign = true; break;
				}
				break;
			case 5: // @reserve
				assert(0); 
				CALLEXCEPT();
				break;
			case 6: // @reserve
				assert(0);
				CALLEXCEPT();
				break;
			case 7:
				switch(sign_mask) {
					case 0: Spc = ueddecsvInfPlus; break;
					case 1: Spc = ueddecsvInfMinus; break;
					case 2: Spc = ueddecsvUnderflow; break;
					case 3: Spc = ueddecsvNan; break;
				}
				break;
		}
		CATCHZOK
		return ok;
	}
	//
	// ARG(valueBitCount IN): 
	//   0 - в возвращаемом значении биты индикатора находятся в младшей части
	//   32..64 - биты индикатора находятся в натуральном положении (старшая часть
	//     зоны шириной valueBitCount возвращаемого значения).
	//
	uint64 ToBits(uint valueBitCount)
	{
		assert(valueBitCount == 0 || (valueBitCount >= 24 && valueBitCount <= 64));
		uint32 indicator = 0;
		uint32 exp_mask = 0;
		AbsExp_Encoded = 0;
		switch(Spc) {
			case ueddecsvZero:      indicator  = 0x00U; break;
			case ueddecsvInfPlus:   indicator  = 0x07U; break;
			case ueddecsvInfMinus:  indicator  = 0x0fU; break;
			case ueddecsvUnderflow: indicator  = 0x17U; break;
			case ueddecsvNan:       indicator  = 0x3fU; break;
			case ueddecsvNormal: 
				{
					if(AbsExp == 0) {
						assert(!ExpSign);
						ExpBitCount = 0;
						indicator  = 0x00U;
					}
					else if(AbsExp <= 8) {
						ExpBitCount = 3;
						AbsExp_Encoded = AbsExp-1;
						indicator  = 0x01U;
					}
					else if(AbsExp <= 32) {
						ExpBitCount = 5;
						AbsExp_Encoded = AbsExp-9;
						indicator  = 0x02U;
					}
					else if(AbsExp <= 64) {
						ExpBitCount = 5;
						AbsExp_Encoded = AbsExp-33;
						indicator  = 0x03U;
					}
					else if(AbsExp <= 128) {
						ExpBitCount = 6;
						AbsExp_Encoded = AbsExp-65;
						indicator  = 0x04U;
					}
					else
						; // @err
					//
					if(valueBitCount)
						MaxMantissa = GetUedMaxMantissa(valueBitCount-5-ExpBitCount);
					if(ExpSign) {
						assert(AbsExp != 0);
						if(MantSign)
							indicator |= (0x3 << 3);
						else
							indicator |= (0x2 << 3);
					}
					else if(MantSign)
						indicator |= (0x1 << 3);
					else
						;
				}
				break;
			default:
				assert(oneof6(Spc, ueddecsvZero, ueddecsvNormal, ueddecsvInfPlus, ueddecsvInfMinus, ueddecsvUnderflow, ueddecsvNan));
				break;
		}
		return valueBitCount ? (((uint64)indicator) << (valueBitCount - 5)) : (uint64)indicator;
	}
};

uint64 SDecimal::ToUed_NonConst(uint numBits) // @construction
{
	uint64 ued = 0;
	assert(numBits >= 24 && numBits <= 64);
	if(numBits >= 24 && numBits <= 64) {
		for(bool done = false; !done;) {
			done = true;
			UedDecIndicator indicator;
			indicator.AbsExp = abs(Exp);
			indicator.ExpSign = (Exp < 0);
			indicator.MantSign = (Mant < 0);
			if(Flags & SIEEE754::fNAN)
				indicator.Spc = ueddecsvNan;
			else if(Flags & SIEEE754::fINF)
				indicator.Spc = (Mant < 0) ? ueddecsvInfMinus : ueddecsvInfPlus;
			else if(IsZero())
				indicator.Spc = ueddecsvZero;
			else 
				indicator.Spc = ueddecsvNormal;
			uint64 ib = indicator.ToBits(numBits);
			const uint mb = (numBits-5-indicator.ExpBitCount);
			if(indicator.Spc == ueddecsvNormal) {
				//uint  exp_bits = 0;
				//int16 abs_exp = abs(Exp);
				//uint  sign_exp = (Exp < 0) ? 1 : 0;
				//uint64 maxmant = 0;
				assert(indicator.MaxMantissa > 0);
				if(static_cast<uint64>(abs(Mant)) <= indicator.MaxMantissa) {
					//ued = ib | (setlowbits64(mb) & Mant);
					//assert(ued == Mant);
					const uint64 mant_bits = (setlowbits64(mb) & Mant);
					if(indicator.ExpBitCount) {
						const uint64 exp_bits = ((setlowbits64(indicator.ExpBitCount) & indicator.AbsExp_Encoded) << mb);
						ued = exp_bits | ib | mant_bits;
					}
					else {
						ued = ib | mant_bits;
					}
				}
				else {
					uint64 new_mantissa = RoundToNearestMul10(abs(Mant));
					new_mantissa /= 10;
					Mant = (Mant < 0) ? -new_mantissa : new_mantissa;
					Exp++;
					done = false;
				}
			}
			else {
				ued = ib | (setlowbits64(mb) & 0ULL);
			}
		}
	}
	CATCH
		ued = 0; // @error @todo Уточнить идентификацию ошибки
	ENDCATCH
	return ued;
}

int SDecimal::FromUed(uint64 ued, uint numBits) // @construction
{
	int    ok = 0;
	assert(numBits >= 24 && numBits <= 64);
	if(numBits >= 24 && numBits <= 64) {
		UedDecIndicator indicator;
		if(indicator.FromBits(numBits, ued)) {
			/*
					if(indicator.ExpBitCount) {
						ued = ((setlowbits64(indicator.ExpBitCount) & indicator.AbsExp) << mb) | ib | (setlowbits64(mb) & Mant);
					}
					else {
						ued = ib | (setlowbits64(mb) & Mant);
					} 
			*/
			uint64 abs_mantissa = 0;
			uint64 abs_exp = indicator.AbsExp;
			if(indicator.ExpBitCount) {
				abs_mantissa = setlowbits64(numBits-5-indicator.ExpBitCount) & ued;
			}
			else {
				abs_mantissa = setlowbits64(numBits-5) & ued;
				assert(abs_exp == 0);
			}
			Mant = indicator.MantSign ? -((int64)abs_mantissa) : (int64)abs_mantissa;
			Exp = (abs_exp == 0) ? 0 : (indicator.ExpSign ? -((int16)abs_exp) : (int16)abs_exp);
		}
	}
	return ok;
}

int SDecimal::Add(const SDecimal & rA, const SDecimal & rB)
{
	int ok = 1;
	if(rA.IsZero()) {
		Mant = rB.Mant;
		Exp = rB.Exp;
	}
	else if(rB.IsZero()) {
		Mant = rA.Mant;
		Exp = rA.Exp;
	}
	else if(rA.Exp == rB.Exp) {
		Mant = rA.Mant + rB.Mant;
		Exp = rA.Exp;
	}
	else if(rA.Exp > rB.Exp) {
		const uint _p = rA.Exp - rB.Exp;
		THROW(_p <= 18); // @todo @err (overflow)
		const uint64 _m = ui64pow10(_p);
		THROW(CheckOverflowMul(rA.Mant, _m));
		Mant = rA.Mant * _m + rB.Mant;
		Exp = rB.Exp;
	}
	else {
		assert(rB.Exp > rA.Exp);
		const uint _p = rB.Exp - rA.Exp;
		THROW(_p <= 18); // @todo @err (overflow)
		const uint64 _m = ui64pow10(_p);
		THROW(CheckOverflowMul(rB.Mant, _m));
		Mant = rB.Mant * _m + rA.Mant;
		Exp = rA.Exp;
	}
	Normalize();
	CATCHZOK
	return ok;
}

int SDecimal::Sub(const SDecimal & rA, const SDecimal & rB)
{
	return Add(rA, Neg(rB));
}

int SDecimal::Mul(const SDecimal & rA, int64 b)
{
	int    ok = 1;
	if(rA.IsZero() || b == 0)
		Z();
	else if(rA.Mant == 1) {
		Mant = b;
		Exp = rA.Exp;
	}
	else if(b == 1ULL) {
		Mant = rA.Mant;
		Exp = rA.Exp;
	}
	else {
		if(CheckOverflowMul(rA.Mant, b)) {
			// ok: переполнения нет
			Mant = rA.Mant * b;
			Exp = rA.Exp;
		}
		else { // переполнение: переходим к блану Б со 128-разрядными числами
			absl::int128 wide_mant = absl::int128(rA.Mant) * absl::int128(b);
			int64 new_mant = 0;
			int16 new_exp = 0;
			if(TrimInt128(wide_mant, rA.Exp, new_mant, new_exp)) {
				Mant = new_mant;
				Exp = new_exp;
				Normalize();
			}
			else
				ok = 0;
		}
	}
	return ok;
}

int SDecimal::Mul(const SDecimal & rA, const SDecimal & rB)
{
	int    ok = 1;
	if(rA.IsZero() || rB.IsZero())
		Z();
	else if(rA.Mant == 1) {
		Mant = rB.Mant;
		Exp = rA.Exp + rB.Exp;
	}
	else if(rB.Mant == 1) {
		Mant = rA.Mant;
		Exp = rA.Exp + rB.Exp;
	}
	else {
		if(CheckOverflowMul(rA.Mant, rB.Mant)) {
			// ok: переполнения нет
			Mant = rA.Mant * rB.Mant;
			Exp = rA.Exp + rB.Exp;
		}
		else { // переполнение: переходим к блану Б со 128-разрядными числами
			absl::int128 wide_mant = absl::int128(rA.Mant) * absl::int128(rB.Mant);
			int64 new_mant = 0;
			int16 new_exp = 0;
			if(TrimInt128(wide_mant, rA.Exp+rB.Exp, new_mant, new_exp)) {
				Mant = new_mant;
				Exp = new_exp;
				Normalize();
			}
			else
				ok = 0;
		}
	}
	return ok;
}

int SDecimal::Div(const SDecimal & rA, const SDecimal & rB)
{
	int ok = 1;
	if(rB.IsZero())
		ok = 0;
	else if(rA.IsZero())
		Z();
	else {
		const bool a_sign = (rA.Mant < 0);
		const bool b_sign = (rB.Mant < 0);
		uint64 a_mant = abs(rA.Mant);
		uint64 b_mant = abs(rB.Mant);
		int64 a_exp = rA.Exp;
		const uint lb_ = log10i_ceil(static_cast<uint64>(b_mant));
		absl::int128 r_mant = 0;
		int16 r_exp = rA.Exp - rB.Exp;

		uint64 rem = a_mant;
		do {
			uint64 t = rem / b_mant;
			rem = rem % b_mant;
			r_mant += t;
			if(rem != 0) {
				assert(rem < b_mant);
				uint la = log10i_floor(rem);
				assert(la < lb_); // la >= lb is impossible!
				uint64 p10 = ui64pow10(lb_ - la);
				rem *= p10;
				r_mant *= p10;
				r_exp -= (lb_ - la);
			}
		} while(rem != 0 && absl::Int128High64(r_mant) == 0);
		{
			int64 new_mant;
			int16 new_exp;
			if(TrimInt128(r_mant, r_exp, new_mant, new_exp)) {
				Mant = new_mant;
				Exp = new_exp;
				Normalize();
			}
			else
				ok = 0;
		}
		// !
		/*
		Mant = rA.Mant / rB.Mant;
		int denom_dec_pwr = static_cast<int>(rA.Exp) - static_cast<int>(rB.Exp);
		if(denom_dec_pwr < 0) {
			Mant *= ui64pow10(static_cast<uint>(-denom_dec_pwr));
			Exp = 0;
		}
		else {
			Exp = static_cast<uint16>(denom_dec_pwr);
			Normalize();
		}*/
	}
	return ok;
}

/*static*/int SDecimal::Test()
{
	int    ok = 1;
	SString temp_buf;

	double a = 1.0;
	double b = 2.0;
	double c = 4.0;
	double d = 1024.0;

	slprintf(temp_buf, "%f", 1.0);
	{
		struct TestEntry {
			int64  N;
			int16  Dp;
			double Rv;
		};
		const TestEntry entries[] = {
			{ 17171717171717LL, -7, 1717171.7171717 },
			{ 1LL, 0, 1.0 },
			{ 3LL, 0, 3.0 },
			{ 1LL, -1, 0.1 },
			{ 1LL, -2, 0.01 },
			{ 5LL, -1, 0.5 },
			{ 1LL, +1, 10.0 },
			{ 1LL, +2, 100.0 },
			{ 5LL, +1, 50.0 },
			{ 0LL, 0, 0.0 },
			{ 314159265359LL, -11, 3.14159265359 },
		};
		assert(SDecimal().IsZero());
		assert(!SDecimal(1, 1).IsZero());
		for(uint i = 0; i < SIZEOFARRAY(entries); i++) {
			const TestEntry & r_te = entries[i];
			const double tv = SDecimal(r_te.N, r_te.Dp).GetReal();
			assert(feqeps(tv, r_te.Rv, 1e-13));
			{
				SDecimal r(r_te.Rv);
				assert(r.GetReal() == tv);
				{
					uint64 ued = r.ToUed_NonConst(48);
					SDecimal r2;
					r2.FromUed(ued, 48);
					assert(r.IsEq(r2));
				}
			}
		}
		{
			SDecimal r;
			SDecimal r2;
			r.Add(SDecimal(0, 0), SDecimal(1, 0));
			assert(r.GetReal() == 1.0);
			r2.Sub(r, SDecimal(1, 0));
			assert(r2.GetReal() == 0.0);
			assert(r2.IsZero());
			//
			r.Add(SDecimal(0, 0), SDecimal(1, -5));
			assert(r.GetReal() == 0.00001);
			r2.Sub(r, SDecimal(1, -5));
			assert(r2.GetReal() == 0.0);
			assert(r2.IsZero());
			//
			r.Add(SDecimal(1, 0), SDecimal(1, -1));
			assert(r.GetReal() == 1.1);
			r2.Sub(r, SDecimal(1, -1));
			assert(r2.GetReal() == 1.0);
			//
			r.Add(SDecimal(703, 6), SDecimal(1, -1));
			assert(r.GetReal() == 703000000.1);
			r2.Sub(r, SDecimal(1, -1));
			assert(r2.GetReal() == 703000000.0);
			//
			r.Mul(SDecimal(1, 0), SDecimal(17, -3));
			assert(r.GetReal() == 0.017);
			//
			r.Mul(SDecimal(2, 0), SDecimal(17, -3));
			assert(r.GetReal() == 0.034);
			//
			r.Mul(SDecimal(1, 1), SDecimal(17, -3));
			assert(r.GetReal() == 0.17);
			//
			{
				r.Z();
				for(uint i = 0; i < 1000; i++) {
					r.Add(r, SDecimal(1, -6));
				}
				assert(r.GetReal() == 0.001);
			}
		}
	}
	return ok;
}