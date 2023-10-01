// SMEM.CPP
// Copyright (c) Sobolev A. 1993-2001, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2016, 2017, 2019, 2020, 2021, 2022, 2023
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
//
// @v11.2.3 Поступила информация, что JobServer в последних релизах "падает". Отключаем для сервера mimalloc и проверям.
// @v11.2.4 Похоже, возникают спонтанные аварии и на клиентских сессиях
//
#if _MSC_VER >= 1900 /*&& !defined(NDEBUG)*/ && !defined(_PPSERVER)
	// @v11.2.4 #define USE_MIMALLOC
#endif
#ifdef USE_MIMALLOC
	#include "mimalloc\mimalloc.h"
#endif
//
//
//
#define REP8(op)       op;op;op;op;op;op;op;op
#define REP_CASE3(op)  case 3:op;case 2:op;case 1:op;
#define REP_CASE7(op)  case 7:op;case 6:op;case 5:op;case 4:op;case 3:op;case 2:op;case 1:op;

//#define ALIGNSIZE(s,bits)     ((((s) + ((1 << (bits))-1)) >> (bits)) << (bits))
#define ALIGNDOWN(s,bits)     (((s) >> (bits)) << (bits))

bool FASTCALL ismemzero(const void * p, size_t s)
{
	switch(s) {
		case  1: return BIN(PTR8C(p)[0] == 0);
		case  2: return BIN(PTR16C(p)[0] == 0);
		case  4: return BIN(PTR32C(p)[0] == 0);
		case  8: return BIN(PTR32C(p)[0] == 0 && PTR32C(p)[1] == 0);
		case 12: return BIN(PTR32C(p)[0] == 0 && PTR32C(p)[1] == 0 && PTR32C(p)[2] == 0);
		case 16: return BIN(PTR32C(p)[0] == 0 && PTR32C(p)[1] == 0 && PTR32C(p)[2] == 0 && PTR32C(p)[3] == 0);
	}
	size_t v = s / 4;
	size_t m = s % 4;
	size_t i;
	for(i = 0; i < v; i++) {
		if(PTR32C(p)[i] != 0)
			return false;
	}
	for(i = 0; i < m; i++)
		if(PTR8C(p)[(v*4)+i] != 0)
			return false;
	return true;
}

void * FASTCALL memzero(void * p, size_t s)
{
	if(p) {
		switch(s) {
			case 1: PTR8(p)[0] = 0; break;
			case 2: PTR16(p)[0] = 0; break;
			case 4:
				PTR32(p)[0] = 0;
				break;
			case 8:
				PTR32(p)[0] = 0;
				PTR32(p)[1] = 0;
				break;
			case 12:
				PTR32(p)[0] = 0;
				PTR32(p)[1] = 0;
				PTR32(p)[2] = 0;
				break;
			case 16:
				PTR32(p)[0] = 0;
				PTR32(p)[1] = 0;
				PTR32(p)[2] = 0;
				PTR32(p)[3] = 0;
				break;
			case 20:
				PTR32(p)[0] = 0;
				PTR32(p)[1] = 0;
				PTR32(p)[2] = 0;
				PTR32(p)[3] = 0;
				PTR32(p)[4] = 0;
				break;
			default:
				memset(p, 0, s);
				break;
		}
	}
	return p;
}

int memdword(void * p, size_t size, uint32 key, size_t * pOffs)
{
	const size_t s4 = size / 4;
	uint32 * p32 = static_cast<uint32 *>(p);
	for(size_t i = 0; i < s4; i++)
		if(*p32 == key) {
			ASSIGN_PTR(pOffs, i * 4);
			return 1;
		}
		else
			p32++;
	return 0;
}

uint32 FASTCALL msb32(uint32 x)
{
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return (x & ~(x >> 1));
}

void FASTCALL memswap(void * p1, void * p2, size_t size)
{
	switch(size) {
		case 1:
			{
				const uint8 t8 = PTR8(p1)[0];
				PTR8(p1)[0] = PTR8(p2)[0];
				PTR8(p2)[0] = t8;
			}
			break;
		case 2:
			{
				const uint16 t16 = PTR16(p1)[0];
				PTR16(p1)[0] = PTR16(p2)[0];
				PTR16(p2)[0] = t16;
			}
			break;
		case 4:
			{
				const uint32 t32 = PTR32(p1)[0];
				PTR32(p1)[0] = PTR32(p2)[0];
				PTR32(p2)[0] = t32;
			}
			break;
		case 8:
			{
				uint32 t32 = PTR32(p1)[0];
				PTR32(p1)[0] = PTR32(p2)[0];
				PTR32(p2)[0] = t32;
				t32 = PTR32(p1)[1];
				PTR32(p1)[1] = PTR32(p2)[1];
				PTR32(p2)[1] = t32;
			}
			break;
		default:
			{
				const size_t s4 = size / 4; // size >> 2
				const size_t m4 = size % 4; // size &= 3
				if(s4) {
					uint32 * p321 = static_cast<uint32 *>(p1);
					uint32 * p322 = static_cast<uint32 *>(p2);
					for(size_t i = 0; i < s4; i++) {
						const uint32 t32 = *p321;
						*p321++ = *p322;
						*p322++ = t32;
					}
				}
				if(m4) {
					uint8 * p81 = reinterpret_cast<uint8 *>(PTR32(p1)+s4);
					uint8 * p82 = reinterpret_cast<uint8 *>(PTR32(p2)+s4);
					for(size_t i = 0; i < m4; i++) {
						const uint8 t8 = *p81;
						*p81++ = *p82;
						*p82++ = t8;
					}
				}
			}
			break;
	}
}

void * catmem(void * pDest, size_t destSize, const void * pSrc, size_t srcSize)
{
	size_t new_size = destSize + srcSize;
	void * p_tmp = pDest;
	if(new_size >= destSize && (p_tmp = SAlloc::R(pDest, new_size)) != 0)
		memmove((char *)p_tmp + destSize, pSrc, srcSize);
	return p_tmp;
}

//void FASTCALL Exchange(int * pA, int * pB)
//{
//	int    temp = *pA;
//	*pA = *pB;
//	*pB = temp;
//}
//
//void FASTCALL Exchange(uint * pA, uint * pB)
//{
//	uint   temp = *pA;
//	*pA = *pB;
//	*pB = temp;
//}
//
//void FASTCALL ExchangeForOrder(int * pA, int * pB)
//{
//	if(*pA > *pB) {
//		int    temp = *pA;
//		*pA = *pB;
//		*pB = temp;
//	}
//}
//
//void FASTCALL Exchange(long * pA, long * pB)
//{
//	long   temp = *pA;
//	*pA = *pB;
//	*pB = temp;
//}
//
//void FASTCALL ExchangeForOrder(long * pA, long * pB)
//{
//	if(*pA > *pB) {
//		long   temp = *pA;
//		*pA = *pB;
//		*pB = temp;
//	}
//}
//
//void FASTCALL Exchange(ulong * pA, ulong * pB)
//{
//	ulong  temp = *pA;
//	*pA = *pB;
//	*pB = temp;
//}
//
//void FASTCALL Exchange(int16 * pA, int16 * pB)
//{
//	int16  temp = *pA;
//	*pA = *pB;
//	*pB = temp;
//}
//
//void FASTCALL ExchangeForOrder(int16 * pA, int16 * pB)
//{
//	if(*pA > *pB) {
//		int16  temp = *pA;
//		*pA = *pB;
//		*pB = temp;
//	}
//}
//
//void FASTCALL Exchange(int64 * pA, int64 * pB)
//{
//	int64  temp = *pA;
//	*pA = *pB;
//	*pB = temp;
//}
//
//void FASTCALL Exchange(float * pA, float * pB)
//{
//	memswap(pA, pB, sizeof(*pA));
//}
//
//void FASTCALL Exchange(double * pA, double * pB)
//{
//	memswap(pA, pB, sizeof(*pA));
//}
//
//void FASTCALL ExchangeForOrder(double * pA, double * pB)
//{
//	if(*pA > *pB)
//		memswap(pA, pB, sizeof(*pA));
//}
//
//
// @costruction {
SAlloc::Stat::Stat()
{
}

void FASTCALL SAlloc::Stat::RegisterAlloc(uint size)
{
	int do_shrink = 0;
	Lck_A.Lock();
	AllocEntry entry;
	entry.Size = size;
	entry.Count = 1;
	AL.insert(&entry);
	if((AL.getCount() & 0xfff) == 0)
		do_shrink = 1;
	Lck_A.Unlock();
	if(do_shrink)
		Shrink();
}

void SAlloc::Stat::RegisterRealloc(uint fromSize, uint toSize)
{
	int do_shrink = 0;
	Lck_R.Lock();
	ReallocEntry entry;
	entry.FromSize = fromSize;
	entry.ToSize = toSize;
	entry.Count = 1;
	RL.insert(&entry);
	if((RL.getCount() & 0xfff) == 0)
		do_shrink = 1;
	Lck_R.Unlock();
	if(do_shrink)
		Shrink();
}

void SAlloc::Stat::Shrink()
{
	{
		Lck_A.Lock();
		uint i = AL.getCount();
		if(i) do {
			const AllocEntry & r_entry = AL.at(--i);
			for(uint j = 0; j < i; j++) {
				if(AL.at(j).Size == r_entry.Size) {
					AL.at(j).Count += r_entry.Count;
					AL.atFree(i);
					break;
				}
			}
		} while(i);
		Lck_A.Unlock();
	}
	{
		Lck_R.Lock();
		uint i = RL.getCount();
		if(i) do {
			const ReallocEntry & r_entry = RL.at(--i);
			for(uint j = 0; j < i; j++) {
				const ReallocEntry & r_entry2 = RL.at(j);
				if(r_entry2.FromSize == r_entry.FromSize && r_entry2.ToSize == r_entry.ToSize) {
					RL.at(j).Count += r_entry.Count;
					RL.atFree(i);
					break;
				}
			}
		} while(i);
		Lck_R.Unlock();
	}
}

void FASTCALL SAlloc::Stat::Merge(const Stat & rS)
{
	{
		Lck_A.Lock();
		for(uint i = 0; i < rS.AL.getCount(); i++) {
			const AllocEntry & r_entry = rS.AL.at(i);
			uint pos = 0;
			if(AL.lsearch(&r_entry.Size, &pos, CMPF_LONG)) {
				AL.at(pos).Count += r_entry.Count;
			}
		}
		Lck_A.Unlock();
	}
	{
		Lck_R.Lock();
		for(uint i = 0; i < rS.RL.getCount(); i++) {
			const ReallocEntry & r_entry = rS.RL.at(i);
			uint pos = 0;
			if(RL.lsearch(&r_entry.FromSize, &pos, PTR_CMPFUNC(_2long))) {
				RL.at(pos).Count += r_entry.Count;
			}
		}
		Lck_R.Unlock();
	}
}

int SAlloc::Stat::Output(SString & rBuf)
{
	int    ok = -1;
	rBuf.Z();
	if(AL.getCount() || RL.getCount()) {
		Shrink();
		{
			Lck_A.Lock();
			if(AL.getCount()) {
				AL.sort(CMPF_LONG);
				rBuf.Cat("alloc-stat").CR();
				for(uint i = 0; i < AL.getCount(); i++) {
					const AllocEntry & r_entry = AL.at(i);
					rBuf.Cat(r_entry.Size).Tab().Cat(r_entry.Count).CR();
				}
			}
			Lck_A.Unlock();
		}
		{
			Lck_R.Lock();
			if(RL.getCount()) {
				RL.sort(PTR_CMPFUNC(_2long));
				rBuf.Cat("realloc-stat").CR();
				for(uint i = 0; i < RL.getCount(); i++) {
					const ReallocEntry & r_entry = RL.at(i);
					rBuf.Cat(r_entry.FromSize).Tab().Cat(r_entry.ToSize).Tab().Cat(r_entry.Count).CR();
				}
			}
			Lck_R.Unlock();
		}
	}
	return ok;
}
// } @costruction 

/*static*/void * FASTCALL SAlloc::M(size_t sz)
{
#if SLGATHERALLOCSTATISTICS
	SLS.GetAllocStat().RegisterAlloc(sz);
#endif
#ifdef USE_MIMALLOC
	void * p_result = mi_malloc(sz);
#else
	void * p_result = malloc(sz);
#endif
	if(!p_result)
		SLS.SetError(SLERR_NOMEM);
	return p_result;
}

/*static*/void * FASTCALL SAlloc::M_zon0(size_t sz) { return sz ? M(sz) : 0; }

/*static*/void * FASTCALL SAlloc::C(size_t n, size_t sz)
{
#if SLGATHERALLOCSTATISTICS
	SLS.GetAllocStat().RegisterAlloc(n * sz);
#endif
#ifdef USE_MIMALLOC
	void * p_result = mi_calloc(n, sz);
#else
	void * p_result = calloc(n, sz);
#endif
	if(!p_result)
		SLS.SetError(SLERR_NOMEM);
	return p_result;
}

/*static*/void * FASTCALL SAlloc::C_zon0(size_t n, size_t sz) { return (n && sz) ? C(n, sz) : 0; }

/*static*/void * FASTCALL SAlloc::R(void * ptr, size_t sz)
{
#if SLGATHERALLOCSTATISTICS
	// На предварительном этапе будем realloc регистрировать как Alloc поскольку я пока не разобрался как получить размер
	// перераспределяемого блока
	SLS.GetAllocStat().RegisterAlloc(sz);
#endif
#ifdef USE_MIMALLOC
	void * p_result = mi_realloc(ptr, sz);
#else
	void * p_result = realloc(ptr, sz);
#endif
	if(!p_result)
		SLS.SetError(SLERR_NOMEM);
	return p_result;
}

/*static*/void FASTCALL SAlloc::F(void * ptr)
{
#ifdef USE_MIMALLOC
	mi_free(ptr);
#else
	free(ptr);
#endif
}

void * operator new(size_t sz)
{
#if SLGATHERALLOCSTATISTICS
	SLS.GetAllocStat().RegisterAlloc(sz);
#endif
#ifdef USE_MIMALLOC
	void * p_result = mi_new(sz);
#else
	void * p_result = malloc(sz);
#endif
	if(!p_result)
		SLS.SetError(SLERR_NOMEM);
	return p_result;
}

void operator delete(void * ptr)
{
#ifdef USE_MIMALLOC
	mi_free(ptr);
#else
	free(ptr);
#endif
}

void operator delete [] (void * ptr)
{
#ifdef USE_MIMALLOC
	mi_free(ptr);
#else
	free(ptr);
#endif
}

void FASTCALL SObfuscateBuffer(void * pBuf, size_t bufSize)
{
	SLS.GetTLA().Rg.ObfuscateBuffer(pBuf, bufSize);
}
