// BITARRAY.CPP
// Copyright (c) A.Sobolev 2000, 2001, 2004, 2006, 2007, 2008, 2010, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2022, 2023
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

BitArray::BitArray() : Count(0) 
{
	Size = 64;
	P_Buf = static_cast<char *>(SAlloc::M(Size));
	resetbitstring(P_Buf, Size);
}

BitArray::BitArray(const BitArray & s) : Count(0)
{
	SBaseBuffer::Init();
	Copy(s);
}

BitArray::~BitArray()
{
	SBaseBuffer::Destroy();
}

BitArray & FASTCALL BitArray::operator = (const BitArray & s)
{
	Copy(s);
	return *this;
}

int FASTCALL BitArray::Copy(const BitArray & s)
{
	size_t new_size = s.Size;
	P_Buf = static_cast<char *>(SAlloc::R(P_Buf, new_size));
	if(P_Buf) {
		Size = new_size;
		memcpy(P_Buf, s.P_Buf, Size);
		Count = s.Count;
		return 1;
	}
	else
		return 0;
}

int BitArray::Init(const void * pBits, size_t count)
{
	int    ok = -1;
	if(count && pBits) {
		size_t new_size = ((count + 31) / 32) * 4;
		if(Size < new_size) {
			P_Buf = static_cast<char *>(SAlloc::R(P_Buf, new_size));
			Size = new_size;
			resetbitstring(P_Buf, Size);
		}
		Count = count;
		memcpy(P_Buf, pBits, new_size);
		ok = 1;
	}
	return ok;
}

void BitArray::Clear()
{
	Zero();
	Count = 0;
}

size_t BitArray::getCount() const { return Count; }
int    FASTCALL BitArray::get(size_t pos) const { return (pos < Count) ? getbit32(P_Buf, Size, pos) : -1; }
uint32 FASTCALL BitArray::getN(size_t pos, uint count) const { return getbits(P_Buf, Size, pos, count); }
int    FASTCALL BitArray::operator [] (size_t pos) const { return get(pos); }
int    FASTCALL BitArray::insert(int val) { return atInsert(Count, val); }
size_t BitArray::getBufSize() const { return ((Count + 31) / 32) * 4; }

bool FASTCALL BitArray::IsEq(const BitArray & rS) const
{
	bool   eq = true;
	const  size_t c = getCount();
	if(c == rS.getCount()) {
		for(uint i = 0; eq && i < c; i++) {
			if(getbit32(P_Buf, Size, i) != getbit32(rS.P_Buf, rS.Size, i))
				eq = false;
		}
	}
	else
		eq = false;
	return eq;
}

size_t FASTCALL BitArray::getCountVal(int val) const
{
	size_t i;
	size_t c = (Count / 32);
	uint   r = 0;
	for(i = 0; i < c; i++) {
		//
		// Трюк с подсчетом ненулевых битов посредством обнуления последнего единичного бита X & (X-1) {
		//
		/* @v7.8.3
		uint   w = PTR32(P_Buf)[i];
		uint   popc = 0;
		for(; w != 0; w &= (w-1))
			popc++;
		*/
		//
		// @v7.8.3
		// Вариант с использованием функции из ASMLIB. Корректность протестирована, но
		// остаются вопросы насчет зависаний системы при использовании каких-то функций из
		// этой библиотеки.
		// uint   popc = A_popcount(PTR32(P_Buf)[i]);
		//
		uint   popc = popcount32(PTR32(P_Buf)[i]); // @v7.8.3 @proof
		// }
		if(val)
			r += popc;
		else
			r += (32-popc);
	}
	{
		i = (c * 32);
		c = Count;
		for(; i < c; i++) {
			if(get(i)) {
				if(val)
					r++;
			}
			else {
				if(!val)
					r++;
			}
		}
	}
	return r;
}

size_t FASTCALL BitArray::findFirst(int val, size_t start) const
{
	size_t p = start;
	while(p < Count) {
		int    v = getbit32(P_Buf, Size, p++);
		if((v && val) || (!v && !val))
			return p;
	}
	return 0;
}

int FASTCALL BitArray::set(size_t pos, int val)
{
	if(pos < Count) {
		if(val)
			setbit32(P_Buf, Size, pos);
		else
			resetbit32(P_Buf, Size, pos);
		return 1;
	}
	else
		return 0;
}

int FASTCALL BitArray::atInsert(size_t pos, int val)
{
	if(pos <= Count) {
		size_t new_size = ((Count + 32) / 32) * 4;
		if(new_size > Size) {
			P_Buf = static_cast<char *>(SAlloc::R(P_Buf, new_size));
			Size = new_size;
		}
		if(pos < Count)
			insbit(P_Buf, Size, pos);
		Count++;
		set(pos, val);
		return 1;
	}
	else
		return 0;
}

int FASTCALL BitArray::insertN(int val, size_t N)
{
	if(N == 0)
		return 0;
	else {
		size_t s = (((Count + N - 1) + 32) / 32) * 4;
		if(s > Size) {
			P_Buf = static_cast<char *>(SAlloc::R(P_Buf, s));
			Size = s;
		}
		s = Count; // prev value of BitArray::Count
		Count += N;
		if(val) {
			for(size_t i = s; i < Count; i++)
				setbit32(P_Buf, Size, i);
		}
		else {
			for(size_t i = s; i < Count; i++)
				resetbit32(P_Buf, Size, i);
		}
		return 1;
	}
}

int FASTCALL BitArray::atFree(size_t pos)
{
	if(pos < Count) {
		delbit(P_Buf, Size, pos);
		Count--;
		return 1;
	}
	else
		return 0;
}

int BitArray::getBuf(void * pBits, size_t maxLen) const
{
	int    ok = -1;
	if(pBits && maxLen) {
		size_t  buflen = getBufSize();
		if(maxLen > buflen) {
			memcpy(pBits, P_Buf, buflen);
			memzero(((char *)pBits) + buflen, maxLen - buflen);
		}
		else
			memcpy(pBits, P_Buf, maxLen);
		ok = 1;
	}
	return ok;
}
//
//
//
static FORCEINLINE uint secp256k1_ctz32_var_debruijn(uint32 v) 
{
	static const uint8 debruijn[32] = {
		0x00, 0x01, 0x02, 0x18, 0x03, 0x13, 0x06, 0x19, 0x16, 0x04, 0x14, 0x0A,
		0x10, 0x07, 0x0C, 0x1A, 0x1F, 0x17, 0x12, 0x05, 0x15, 0x09, 0x0F, 0x0B,
		0x1E, 0x11, 0x08, 0x0E, 0x1D, 0x0D, 0x1C, 0x1B
	};
	return debruijn[(uint32)((v & -v) * 0x04D7651FU) >> 27];
}

static FORCEINLINE uint secp256k1_ctz64_var_debruijn(uint64 x) 
{
	static const uint8 debruijn[64] = {
		0, 1, 2, 53, 3, 7, 54, 27, 4, 38, 41, 8, 34, 55, 48, 28,
		62, 5, 39, 46, 44, 42, 22, 9, 24, 35, 59, 56, 49, 18, 29, 11,
		63, 52, 6, 26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
		51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12
	};
	return debruijn[(uint64)((x & -x) * 0x022FDD63CC95386DU) >> 58];
}

/*static*/uint FASTCALL SBits::Ctz_fallback(uint32 v)
{
	if(v == 0)
		return (sizeof(v) << 3);
	else {
		// de Bruijn multiplication, see <http://supertech.csail.mit.edu/papers/debruijn.pdf>
		return secp256k1_ctz32_var_debruijn(v);
	}
}

/*static*/uint FASTCALL SBits::Ctz_fallback(uint64 v)
{
	if(v == 0)
		return (sizeof(v) << 3);
	else {
		// de Bruijn multiplication, see <http://supertech.csail.mit.edu/papers/debruijn.pdf>
		return secp256k1_ctz64_var_debruijn(v);
	}
}

/*static*/uint FASTCALL SBits::Clz_fallback(uint32 v)
{
	if(v == 0)
		return (sizeof(v) << 3);
	else {
		static const uint32 DeBruijnClz[32] = {0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31};
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		return (31 - DeBruijnClz[(v * 0x07C4ACDDU) >> 27]);
	}
}

/*static*/uint FASTCALL SBits::Clz_fallback(uint64 v)
{
	if(v == 0)
		return (sizeof(v) << 3);
	else {
		const uint32 most_significant_word = (uint32)(v >> 32);
		const uint32 least_significant_word = (uint32)v;
		// Ниже мы в обязательном порядке используем _fallback-вариант функции
		// с целью обеспечить адекватное тестирование полной fallback-реализации.
		return (most_significant_word == 0) ? (32 + Clz_fallback(least_significant_word)) : Clz_fallback(most_significant_word);
	}
}

/*static*/uint FASTCALL SBits::Cpop_fallback(uint32 v)
{
	constexpr uint32 m1 = 0x55555555;
	constexpr uint32 m2 = 0x33333333;
	constexpr uint32 m4 = 0x0f0f0f0f;
	v -= (v >> 1) & m1;
	v = (v & m2) + ((v >> 2) & m2);
	v = (v + (v >> 4)) & m4;
	v += v >>  8;
	return (v + (v >> 16)) & 0x3f;
}

/*static*/uint FASTCALL SBits::Cpop_fallback(uint64 v)
{
	constexpr uint64 m1 = 0x5555555555555555ULL;
	constexpr uint64 m2 = 0x3333333333333333ULL;
	constexpr uint64 m4 = 0x0F0F0F0F0F0F0F0FULL;
	constexpr uint64 h01 = 0x0101010101010101ULL;
	v -= (v >> 1) & m1;
	v = (v & m2) + ((v >> 2) & m2);
	v = (v + (v >> 4)) & m4;
	return (v * h01) >> 56;
}

#if (defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__clang__)
	/*static*/uint FASTCALL SBits::Clz(uint32 v) { return (uint)__builtin_clz(v); }
	/*static*/uint FASTCALL SBits::Clz(uint64 v) { return (uint)(__builtin_clzll(v)); }
	/*static*/uint FASTCALL SBits::Ctz(uint32 v) { return __builtin_ctz(v); }
	/*static*/uint FASTCALL SBits::Ctz(uint64 v) { return __builtin_ctzll(v); }
	/*static*/uint FASTCALL SBits::Cpop(uint32 v) { return __builtin_popcount(v); }
	/*static*/uint FASTCALL SBits::Cpop(uint64 v) { return __builtin_popcountll(v); }
#elif defined(_MSC_VER) 
	#include <nmmintrin.h>

	/*static*/uint FASTCALL SBits::Clz(uint32 v)
	{
		if(v == 0)
			return (sizeof(v) << 3);
		else {
			ulong where;
			_BitScanReverse(&where, v);
			return (uint)(31U - where);
		}
	}
	/*static*/uint FASTCALL SBits::Ctz(uint32 v)
	{
		if(v == 0)
			return (sizeof(v) << 3);
		else {
			ulong where;
			_BitScanForward(&where, v);
			return where;	
		}
	}
	/*static*/uint FASTCALL SBits::Cpop(uint32 v) { return _mm_popcnt_u32(v); }
	#if defined(_M_X64)
		/*static*/uint FASTCALL SBits::Clz(uint64 v)
		{
			if(v == 0)
				return (sizeof(v) << 3);
			else {
			#if STATIC_BMI2 == 1
				return _lzcnt_u64(v);
			#else
				ulong r;
				_BitScanReverse64(&r, v);
				return (uint)(63 - r);
			#endif
			}
		}
		/*static*/uint FASTCALL SBits::Ctz(uint64 v)
		{
			if(v == 0)
				return (sizeof(v) << 3);
			else {
			#if STATIC_BMI2 == 1
				return _tzcnt_u64(val);
			#else
				ulong where;
				_BitScanForward64(&where, v);
				return where;
			#endif
			}
		}
		/*static*/uint FASTCALL SBits::Cpop(uint64 v) { return _mm_popcnt_u64(v); }
	#else 
		/*static*/uint FASTCALL SBits::Clz(uint64 v)
		{
			const uint32 most_significant_word = (uint32)(v >> 32);
			const uint32 least_significant_word = (uint32)v;
			return (most_significant_word == 0) ? (32 + SBits::Clz(least_significant_word)) : SBits::Clz(most_significant_word);
		}
		/*static*/uint FASTCALL SBits::Ctz(uint64 v)
		{
			const uint32 most_significant_word = (uint32)(v >> 32);
			const uint32 least_significant_word = (uint32)v;
			return (least_significant_word == 0) ? (32 + SBits::Ctz(most_significant_word)) : SBits::Ctz(least_significant_word);
		}
		#if defined(_M_IX86)
			/*static*/uint FASTCALL SBits::Cpop(uint64 v) { return _mm_popcnt_u32((uint32)v) + _mm_popcnt_u32((uint32)(v >> 32)); }
		#endif
	#endif
#else
	/*static*/uint FASTCALL SBits::Clz(uint32 v) { return Clz_fallback(v); }
	/*static*/uint FASTCALL SBits::Clz(uint64 v) { return Clz_fallback(v); }
	/*static*/uint FASTCALL SBits::Ctz(uint32 v) { return Ctz_fallback(v); }
	/*static*/uint FASTCALL SBits::Ctz(uint64 v) { return Ctz_fallback(v); }
	/*static*/uint FASTCALL SBits::Cpop(uint32 v) { return Cpop_fallback(v); }
	/*static*/uint FASTCALL SBits::Cpop(uint64 v) { return Cpop_fallback(v); }
#endif
