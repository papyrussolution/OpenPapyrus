// SMEM.CPP
// Copyright (c) Sobolev A. 1993-2001, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2016, 2017
// @codepage windows-1251
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
//
//
//
#define REP8(op)       op;op;op;op;op;op;op;op
#define REP_CASE3(op)  case 3:op;case 2:op;case 1:op;
#define REP_CASE7(op)  case 7:op;case 6:op;case 5:op;case 4:op;case 3:op;case 2:op;case 1:op;

//#define ALIGNSIZE(s,bits)     ((((s) + ((1 << (bits))-1)) >> (bits)) << (bits))
#define ALIGNDOWN(s,bits)     (((s) >> (bits)) << (bits))

void FASTCALL memmovo(void * pDest, const void * pSrc, size_t sz)
{
#define INCPTR32(p) (p) = (PTR32(p)+1)
#define INCPTR8(p)  (p) = (PTR8(p)+1)
#define DECPTR32(p) (p) = (PTR32(p)-1)
#define DECPTR8(p)  (p) = (PTR8(p)-1)
	// Алгоритм заточен на выравнивание адреса источника по границе 32 байт (2^5)
	if(pDest != pSrc) {
		switch(sz) {
			case 1: *PTR8(pDest)  = *PTR8(pSrc);  return;
			case 2: *PTR16(pDest) = *PTR16(pSrc); return;
			case 4: *PTR32(pDest) = *PTR32(pSrc); return;
			case 8:
				{
					uint32 t1, t2;
					t1 = PTR32(pSrc)[0];
					t2 = PTR32(pSrc)[1];
					PTR32(pDest)[0] = t1;
					PTR32(pDest)[1] = t2;
				}
				return;
		}
		if(pDest < pSrc || (PTR8(pDest) - PTR8(pSrc)) >= (int)sz) {
			size_t delta = MIN(PTR8(ALIGNSIZE((ulong)pSrc, 5)) - PTR8(pSrc), (int)sz);
			if(delta) {
				switch(delta / 4) { REP_CASE7(*PTR32(pDest) = *PTR32(pSrc); INCPTR32(pDest); INCPTR32(pSrc); ) }
				switch(delta % 4) { REP_CASE3(*PTR8(pDest)  = *PTR8(pSrc);  INCPTR8(pDest);  INCPTR8(pSrc);)  }
				sz -= delta;
			}
			if(sz) {
				while(sz / 32) {
					REP8(*PTR32(pDest) = *PTR32(pSrc); INCPTR32(pDest); INCPTR32(pSrc));
					sz -= 32;
				}
				switch(sz / 4) { REP_CASE7(*PTR32(pDest) = *PTR32(pSrc); INCPTR32(pDest); INCPTR32(pSrc)) }
				switch(sz % 4) { REP_CASE3(*PTR8(pDest)  = *PTR8(pSrc);  INCPTR8(pDest);  INCPTR8(pSrc))  }
			}
		}
		else { // (pDest - pSrc) < sz
			pDest = PTR8(pDest) + sz;
			pSrc  = PTR8(pSrc)  + sz;
			size_t delta = MIN(((ulong)pSrc) - ALIGNDOWN((ulong)pSrc, 5), sz);
			if(delta) {
				switch(delta % 4) { REP_CASE3(DECPTR8(pDest); DECPTR8(pSrc); *PTR8(pDest) = *PTR8(pSrc))  }
				switch(delta / 4) { REP_CASE7(DECPTR32(pDest); DECPTR32(pSrc); *PTR32(pDest) = *PTR32(pSrc)) }
				sz -= delta;
			}
			if(sz) {
				while(sz / 32) {
					REP8(DECPTR32(pDest); DECPTR32(pSrc); *PTR32(pDest) = *PTR32(pSrc));
					sz -= 32;
				}
				switch(sz % 4) { REP_CASE3(DECPTR8(pDest);  DECPTR8(pSrc); *PTR8(pDest)  = *PTR8(pSrc)) }
				switch(sz / 4) { REP_CASE7(DECPTR32(pDest); DECPTR32(pSrc); *PTR32(pDest) = *PTR32(pSrc)) }
			}
		}
	}
}

int FASTCALL ismemzero(const void * p, size_t s)
{
	switch(s) {
		case  1: return BIN(PTR8(p)[0] == 0);
		case  2: return BIN(PTR16(p)[0] == 0);
		case  4: return BIN(PTR32(p)[0] == 0);
		case  8: return BIN(PTR32(p)[0] == 0 && PTR32(p)[1] == 0);
		case 12: return BIN(PTR32(p)[0] == 0 && PTR32(p)[1] == 0 && PTR32(p)[2] == 0);
		case 16: return BIN(PTR32(p)[0] == 0 && PTR32(p)[1] == 0 && PTR32(p)[2] == 0 && PTR32(p)[3] == 0);
	}
	size_t v = s / 4;
	size_t m = s % 4;
	size_t i;
	for(i = 0; i < v; i++) {
		if(PTR32(p)[i] != 0)
			return 0;
	}
	for(i = 0; i < m; i++)
		if(PTR8(p)[(v*4)+i] != 0)
			return 0;
	return 1;
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

int SLAPI memdword(void * p, size_t size, uint32 key, size_t * pOffs)
{
	const size_t s4 = size / 4;
	uint32 * p32 = (uint32 *)p;
	for(size_t i = 0; i < s4; i++)
		if(*p32 == key) {
			ASSIGN_PTR(pOffs, i * 4);
			return 1;
		}
		else
			p32++;
	return 0;
}

#ifndef _WIN32_WCE // {
uint16 FASTCALL swapw(uint16 w)
{
	/* @v9.6.4
	__asm {
		mov ax, w
		xchg al, ah
	}
	*/
	return ((w>>8) | (w<<8)); // @v9.6.4
}

uint32 FASTCALL swapdw(uint32 dw)
{
	/* @v9.6.4
	__asm {
		mov eax, dw
		mov edx, eax
		and eax, 0xffff0000U
		shr eax, 16
		and edx, 0x0000ffffU
		shl edx, 16
		or  eax, edx
	}
	*/
	return (((dw & 0xffff0000U) >> 16) | ((dw & 0x0000ffffU) << 16));
}
#endif // } _WIN32_WCE

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
					uint32 * p321 = (uint32 *)p1;
					uint32 * p322 = (uint32 *)p2;
					for(size_t i = 0; i < s4; i++) {
						const uint32 t32 = *p321;
						*p321++ = *p322;
						*p322++ = t32;
					}
				}
				if(m4) {
					uint8 * p81 = (uint8 *)(((uint32 *)p1)+s4);
					uint8 * p82 = (uint8 *)(((uint32 *)p2)+s4);
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

void * SLAPI catmem(void * pDest, size_t destSize, const void * pSrc, size_t srcSize)
{
	size_t new_size = destSize + srcSize;
	void * p_tmp = pDest;
	if(new_size >= destSize && (p_tmp = SAlloc::R(pDest, new_size)) != 0)
		memmove((char *)p_tmp + destSize, pSrc, srcSize);
	return p_tmp;
}

void FASTCALL Exchange(int * pA, int * pB)
{
	int    temp = *pA;
	*pA = *pB;
	*pB = temp;
}

void FASTCALL Exchange(uint * pA, uint * pB)
{
	uint   temp = *pA;
	*pA = *pB;
	*pB = temp;
}

void FASTCALL ExchangeToOrder(int * pA, int * pB)
{
	if(*pA > *pB) {
		int    temp = *pA;
		*pA = *pB;
		*pB = temp;
	}
}

void FASTCALL Exchange(long * pA, long * pB)
{
	long   temp = *pA;
	*pA = *pB;
	*pB = temp;
}

void FASTCALL ExchangeToOrder(long * pA, long * pB)
{
	if(*pA > *pB) {
		long   temp = *pA;
		*pA = *pB;
		*pB = temp;
	}
}

void FASTCALL Exchange(ulong * pA, ulong * pB)
{
	ulong  temp = *pA;
	*pA = *pB;
	*pB = temp;
}

void FASTCALL Exchange(int16 * pA, int16 * pB)
{
	int16  temp = *pA;
	*pA = *pB;
	*pB = temp;
}

void FASTCALL ExchangeToOrder(int16 * pA, int16 * pB)
{
	if(*pA > *pB) {
		int16  temp = *pA;
		*pA = *pB;
		*pB = temp;
	}
}

void FASTCALL Exchange(float * pA, float * pB)
{
	memswap(pA, pB, sizeof(*pA));
}

void FASTCALL Exchange(double * pA, double * pB)
{
	memswap(pA, pB, sizeof(*pA));
}

void FASTCALL ExchangeToOrder(double * pA, double * pB)
{
	if(*pA > *pB)
		memswap(pA, pB, sizeof(*pA));
}

//static
void * FASTCALL SAlloc::M(size_t sz)
{
	void * p_result = malloc(sz);
	if(!p_result)
		SLS.SetError(SLERR_NOMEM);
	return p_result;
}

//static
void * FASTCALL SAlloc::C(size_t n, size_t sz)
{
	void * p_result = calloc(n, sz);
	if(!p_result)
		SLS.SetError(SLERR_NOMEM);
	return p_result;
}

//static
void * FASTCALL SAlloc::R(void * ptr, size_t sz)
{
	void * p_result = realloc(ptr, sz);
	if(!p_result)
		SLS.SetError(SLERR_NOMEM);
	return p_result;
}

//static
void FASTCALL SAlloc::F(void * ptr)
{
	free(ptr);
}

void * operator new(size_t sz)
{
	void * p_result = malloc(sz);
	if(!p_result)
		SLS.SetError(SLERR_NOMEM);
	return p_result;
}

void operator delete(void * ptr)
{
	free(ptr);
}
//
// @TEST {
//
#if SLTEST_RUNNING // {

#define MEMPAIR_COUNT 1000
#define MEMBLK_SIZE   (36*1024)
#define MEMBLK_EXT_SIZE 32
#define BYTES_SIZE    (5 * 1024) // Величина должна превышать размер страницы памяти

#include <asmlib.h> // @v7.0.12

struct SlTestFixtureMEMMOVO {
public:
	SlTestFixtureMEMMOVO()
	{
		assert(BYTES_SIZE % 4 == 0);
		P_Bytes = (char *)SAlloc::M(BYTES_SIZE);
		assert(P_Bytes);
		P_Pattern = (char *)SAlloc::M(BYTES_SIZE);
		assert(P_Pattern);
		{
			SRng * p_rng = SRng::CreateInstance(SRng::algMT, 0);
			p_rng->Set(7619);
			for(size_t i = 0; i < BYTES_SIZE/sizeof(uint32); i++) {
				((uint32*)P_Bytes)[i] = ((uint32*)P_Pattern)[i] = p_rng->Get();
			}
			delete p_rng;
		}
	}
	~SlTestFixtureMEMMOVO()
	{
		SAlloc::F(P_Bytes);
		SAlloc::F(P_Pattern);
	}
	char * P_Bytes;
	char * P_Pattern;
};

SLTEST_FIXTURE(MEMMOVO, SlTestFixtureMEMMOVO)
{
	int    ok = 1;
	char * b = (char *)F.P_Bytes;
	const  size_t bs = BYTES_SIZE;
	int    bm = -1;
	if(pBenchmark == 0)
		bm = 0;
	else if(sstreqi_ascii(pBenchmark, "memmovo"))
		bm = 1;
	else if(sstreqi_ascii(pBenchmark, "memmove"))
		bm = 2;
	else if(sstreqi_ascii(pBenchmark, "A_memmove"))
		bm = 3;
	else
		SetInfo("invalid benchmark");
	if(bm >= 0) {
		size_t s;
		for(s = 1; s <= bs/4; s++) {
			const size_t start = bs/4;
			const size_t zone  = bs/2;
			for(size_t offs = start; offs < (start+zone-s); offs++) {
				const size_t src  = offs;
				const size_t dest = bs-offs-s;
				assert(src >= start && src < start+zone);
				assert(dest >= start && dest < start+zone);
				if(bm == 0) {
					//
					// Тестируем A_memmove
					// Она должна скопировать блок длиной s из одной части F.P_Bytes в другую не задев
					// сопредельные участки памяти. Результат копирования сравниваем с F.P_Pattern
					//
					//memmovo(F.P_Bytes+dest, F.P_Bytes+src, s);
					A_memmove(F.P_Bytes+dest, F.P_Bytes+src, s);
					THROW(SLTEST_CHECK_Z(memcmp(F.P_Bytes+dest, F.P_Pattern+src, s)));
					THROW(SLTEST_CHECK_Z(memcmp(F.P_Bytes, F.P_Pattern, dest)));
					THROW(SLTEST_CHECK_Z(memcmp(F.P_Bytes+dest+s, F.P_Pattern+dest+s, BYTES_SIZE-dest-s)));
					//
					// Стандартная процедура копирования для восстановления эквивалентности P_Bytes и P_Pattern
					// Закладываемся на то, что memmove работает правильно.
					// В случае бенчмарка этот вызов "разбавляет" кэш за счет обращения к отличному от F.P_Bytes
					// блоку памяти.
					//
					memmove(F.P_Pattern+dest, F.P_Pattern+src, s);
				}
				else if(bm == 1) {
					memmovo(F.P_Bytes+dest, F.P_Bytes+src, s);
					memmovo(F.P_Pattern+dest, F.P_Pattern+src, s);
				}
				else if(bm == 2) {
					memmove(F.P_Bytes+dest, F.P_Bytes+src, s);
					memmove(F.P_Pattern+dest, F.P_Pattern+src, s);
				}
				else if(bm == 3) {
					A_memmove(F.P_Bytes+dest, F.P_Bytes+src, s);
					A_memmove(F.P_Pattern+dest, F.P_Pattern+src, s);
				}
			}
		}
		//
		// Тестирование A_memset и ismemzero
		//
		if(bm == 0) {
			for(s = 1; s <= bs/4; s++) {
				const size_t start = bs/4;
				const size_t zone  = bs/2;
				for(size_t offs = start; offs < (start+zone-s); offs++) {
					const size_t src  = offs;
					const size_t dest = bs-offs-s;
					assert(src >= start && src < start+zone);
					assert(dest >= start && dest < start+zone);
					//
					// Тестируем A_memset
					// Она должна обнулить заданный участок памяти не задев сопредельные участки.
					//
					A_memset(F.P_Bytes+dest, 0, s);
					THROW(SLTEST_CHECK_NZ(ismemzero(F.P_Bytes+dest, s)));
					THROW(SLTEST_CHECK_Z(memcmp(F.P_Bytes, F.P_Pattern, dest)));
					THROW(SLTEST_CHECK_Z(memcmp(F.P_Bytes+dest+s, F.P_Pattern+dest+s, BYTES_SIZE-dest-s)));
					//
					// Возвращаем назад содержимое F.P_Bytes[dest..s-1]
					//
					memmove(F.P_Bytes+dest, F.P_Pattern+dest, s);
				}
			}
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

#endif // } SLTEST_RUNNING
//
// } @TEST
//
