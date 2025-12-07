/* ******************************************************************
* FSE : Finite State Entropy decoder
* Copyright (c) Yann Collet, Facebook, Inc.
*
*  You can contact the author at :
*  - FSE source repository : https://github.com/Cyan4973/FiniteStateEntropy
*  - Public forum : https://groups.google.com/forum/#!forum/lz4c
*
* This source code is licensed under both the BSD-style license (found in the
* LICENSE file in the root directory of this source tree) and the GPLv2 (found
* in the COPYING file in the root directory of this source tree).
* You may select, at your option, one of the above-listed licenses.
****************************************************************** */

#include <zstd-internal.h>
#pragma hdrstop
#include "debug.h"      /* assert */
#include "bitstream.h"
#include "compiler.h"
#define FSE_STATIC_LINKING_ONLY
#include "fse.h"
#include "error_private.h"
#define ZSTD_DEPS_NEED_MALLOC
#include "zstd_deps.h"
#include "bits.h"       /* ZSTD_highbit32 */
//
// Error Management
//
#define FSE_isError ERR_isError
#define FSE_STATIC_ASSERT(c) DEBUG_STATIC_ASSERT(c)   /* use only *after* variable declarations */
//
// Templates
//
/*
   designed to be included
   for type-specific functions (template emulation in C)
   Objective is to write these functions only once, for improved maintenance
 */
/* safety checks */
#ifndef FSE_FUNCTION_EXTENSION
	#error "FSE_FUNCTION_EXTENSION must be defined"
#endif
#ifndef FSE_FUNCTION_TYPE
	#error "FSE_FUNCTION_TYPE must be defined"
#endif
/* Function names */
#define FSE_CAT(X, Y) X ## Y
#define FSE_FUNCTION_NAME(X, Y) FSE_CAT(X, Y)
#define FSE_TYPE_NAME(X, Y) FSE_CAT(X, Y)

/* Function templates */
FSE_DTable* FSE_createDTable(uint tableLog)
{
	SETMIN(tableLog, FSE_TABLELOG_ABSOLUTE_MAX);
	return (FSE_DTable*)SAlloc::M(FSE_DTABLE_SIZE_U32(tableLog) * sizeof(uint32) );
}

void FSE_freeDTable(FSE_DTable* dt) { SAlloc::F(dt); }

static size_t FSE_buildDTable_internal(FSE_DTable* dt, const short* normalizedCounter, uint maxSymbolValue, uint tableLog, void * workSpace, size_t wkspSize)
{
	void * const tdPtr = dt+1; /* because *dt is unsigned, 32-bits aligned on 32-bits */
	FSE_DECODE_TYPE* const tableDecode = (FSE_DECODE_TYPE*)(tdPtr);
	uint16* symbolNext = (uint16*)workSpace;
	BYTE * spread = (BYTE *)(symbolNext + maxSymbolValue + 1);
	const uint32 maxSV1 = maxSymbolValue + 1;
	const uint32 tableSize = 1 << tableLog;
	uint32 highThreshold = tableSize-1;
	/* Sanity Checks */
	if(FSE_BUILD_DTABLE_WKSP_SIZE(tableLog, maxSymbolValue) > wkspSize) 
		return ERROR(maxSymbolValue_tooLarge);
	if(maxSymbolValue > FSE_MAX_SYMBOL_VALUE) 
		return ERROR(maxSymbolValue_tooLarge);
	if(tableLog > FSE_MAX_TABLELOG) 
		return ERROR(tableLog_tooLarge);
	/* Init, lay down lowprob symbols */
	{   
		FSE_DTableHeader DTableH;
	    DTableH.tableLog = (uint16)tableLog;
	    DTableH.fastMode = 1;
	    {   
			const int16 largeLimit = static_cast<int16>(1 << (tableLog-1));
			for(uint32 s = 0; s<maxSV1; s++) {
				if(normalizedCounter[s] == -1) {
					tableDecode[highThreshold--].symbol = (FSE_FUNCTION_TYPE)s;
					symbolNext[s] = 1;
				}
				else {
					if(normalizedCounter[s] >= largeLimit) DTableH.fastMode = 0;
					symbolNext[s] = normalizedCounter[s];
				}
			}
	    }
	    memcpy(dt, &DTableH, sizeof(DTableH));
	}

	/* Spread symbols */
	if(highThreshold == tableSize - 1) {
		const size_t tableMask = tableSize-1;
		const size_t step = FSE_TABLESTEP(tableSize);
		/* First lay down the symbols in order.
		 * We use a uint64_t to lay down 8 bytes at a time. This reduces branch
		 * misses since small blocks generally have small table logs, so nearly
		 * all symbols have counts <= 8. We ensure we have 8 bytes at the end of
		 * our buffer to handle the over-write.
		 */
		{
			uint64 const add = 0x0101010101010101ull;
			size_t pos = 0;
			uint64 sv = 0;
			uint32 s;
			for(s = 0; s<maxSV1; ++s, sv += add) {
				int i;
				int const n = normalizedCounter[s];
				SMem::Put(spread + pos, sv);
				for(i = 8; i < n; i += 8) {
					SMem::Put(spread + pos + i, sv);
				}
				pos += n;
			}
		}
		/* Now we spread those positions across the table.
		 * The benefit of doing it in two stages is that we avoid the
		 * variable size inner loop, which caused lots of branch misses.
		 * Now we can run through all the positions without any branch misses.
		 * We unroll the loop twice, since that is what empirically worked best.
		 */
		{
			size_t position = 0;
			size_t s;
			const size_t unroll = 2;
			assert(tableSize % unroll == 0); /* FSE_MIN_TABLELOG is 5 */
			for(s = 0; s < (size_t)tableSize; s += unroll) {
				size_t u;
				for(u = 0; u < unroll; ++u) {
					const size_t uPosition = (position + (u * step)) & tableMask;
					tableDecode[uPosition].symbol = spread[s + u];
				}
				position = (position + (unroll * step)) & tableMask;
			}
			assert(position == 0);
		}
	}
	else {
		const uint32 tableMask = tableSize-1;
		const uint32 step = FSE_TABLESTEP(tableSize);
		uint32 s, position = 0;
		for(s = 0; s<maxSV1; s++) {
			int i;
			for(i = 0; i<normalizedCounter[s]; i++) {
				tableDecode[position].symbol = (FSE_FUNCTION_TYPE)s;
				position = (position + step) & tableMask;
				while(position > highThreshold) position = (position + step) & tableMask; /* lowprob
				                                                                             area */
			}
		}
		if(position!=0) return ERROR(GENERIC); /* position must reach all cells once, otherwise
		                                          normalizedCounter is incorrect */
	}

	/* Build Decoding table */
	{   uint32 u;
	    for(u = 0; u<tableSize; u++) {
		    FSE_FUNCTION_TYPE const symbol = (FSE_FUNCTION_TYPE)(tableDecode[u].symbol);
		    const uint32 nextState = symbolNext[symbol]++;
		    tableDecode[u].nbBits = (BYTE)(tableLog - ZSTD_highbit32(nextState) );
		    tableDecode[u].newState = (uint16)( (nextState << tableDecode[u].nbBits) - tableSize);
	    }
	}

	return 0;
}

size_t FSE_buildDTable_wksp(FSE_DTable* dt, const short* normalizedCounter, uint maxSymbolValue, uint tableLog, void * workSpace, size_t wkspSize)
{
	return FSE_buildDTable_internal(dt, normalizedCounter, maxSymbolValue, tableLog, workSpace, wkspSize);
}

#ifndef FSE_COMMONDEFS_ONLY

/*-*******************************************************
*  Decompression (Byte symbols)
*********************************************************/
size_t FSE_buildDTable_rle(FSE_DTable* dt, BYTE symbolValue)
{
	void * ptr = dt;
	FSE_DTableHeader* const DTableH = (FSE_DTableHeader*)ptr;
	void * dPtr = dt + 1;
	FSE_decode_t* const cell = (FSE_decode_t*)dPtr;

	DTableH->tableLog = 0;
	DTableH->fastMode = 0;

	cell->newState = 0;
	cell->symbol = symbolValue;
	cell->nbBits = 0;

	return 0;
}

size_t FSE_buildDTable_raw(FSE_DTable* dt, uint nbBits)
{
	void * ptr = dt;
	FSE_DTableHeader* const DTableH = (FSE_DTableHeader*)ptr;
	void * dPtr = dt + 1;
	FSE_decode_t* const dinfo = (FSE_decode_t*)dPtr;
	const uint tableSize = 1 << nbBits;
	const uint tableMask = tableSize - 1;
	const uint maxSV1 = tableMask+1;
	uint s;
	/* Sanity checks */
	if(nbBits < 1) 
		return ERROR(GENERIC);      /* min size */
	/* Build Decoding Table */
	DTableH->tableLog = (uint16)nbBits;
	DTableH->fastMode = 1;
	for(s = 0; s<maxSV1; s++) {
		dinfo[s].newState = 0;
		dinfo[s].symbol = (BYTE)s;
		dinfo[s].nbBits = (BYTE)nbBits;
	}

	return 0;
}

FORCE_INLINE_TEMPLATE size_t FSE_decompress_usingDTable_generic(void * dst, size_t maxDstSize,
    const void * cSrc, size_t cSrcSize, const FSE_DTable* dt, const uint fast)
{
	BYTE * const ostart = (BYTE *)dst;
	BYTE * op = ostart;
	BYTE * const omax = op + maxDstSize;
	BYTE * const olimit = omax-3;
	BIT_DStream_t bitD;
	FSE_DState_t state1;
	FSE_DState_t state2;
	/* Init */
	CHECK_F(BIT_initDStream(&bitD, cSrc, cSrcSize));
	FSE_initDState(&state1, &bitD, dt);
	FSE_initDState(&state2, &bitD, dt);
#define FSE_GETSYMBOL(statePtr) fast ? FSE_decodeSymbolFast(statePtr, &bitD) : FSE_decodeSymbol(statePtr, &bitD)
	/* 4 symbols per loop */
	for(; (BIT_reloadDStream(&bitD)==BIT_DStream_unfinished) & (op<olimit); op += 4) {
		op[0] = FSE_GETSYMBOL(&state1);

		if(FSE_MAX_TABLELOG*2+7 > sizeof(bitD.bitContainer)*8) /* This test must be static */
			BIT_reloadDStream(&bitD);

		op[1] = FSE_GETSYMBOL(&state2);

		if(FSE_MAX_TABLELOG*4+7 > sizeof(bitD.bitContainer)*8) { /* This test must be static */
			if(BIT_reloadDStream(&bitD) > BIT_DStream_unfinished) {
				op += 2; break;
			}
		}

		op[2] = FSE_GETSYMBOL(&state1);

		if(FSE_MAX_TABLELOG*2+7 > sizeof(bitD.bitContainer)*8) /* This test must be static */
			BIT_reloadDStream(&bitD);

		op[3] = FSE_GETSYMBOL(&state2);
	}

	/* tail */
	/* note : BIT_reloadDStream(&bitD) >= FSE_DStream_partiallyFilled; Ends at exactly BIT_DStream_completed */
	while(1) {
		if(op>(omax-2)) return ERROR(dstSize_tooSmall);
		*op++ = FSE_GETSYMBOL(&state1);
		if(BIT_reloadDStream(&bitD)==BIT_DStream_overflow) {
			*op++ = FSE_GETSYMBOL(&state2);
			break;
		}

		if(op>(omax-2)) return ERROR(dstSize_tooSmall);
		*op++ = FSE_GETSYMBOL(&state2);
		if(BIT_reloadDStream(&bitD)==BIT_DStream_overflow) {
			*op++ = FSE_GETSYMBOL(&state1);
			break;
		}
	}

	return op-ostart;
}

size_t FSE_decompress_usingDTable(void * dst, size_t originalSize,
    const void * cSrc, size_t cSrcSize,
    const FSE_DTable* dt)
{
	const void * ptr = dt;
	const FSE_DTableHeader* DTableH = (const FSE_DTableHeader*)ptr;
	const uint32 fastMode = DTableH->fastMode;

	/* select fast mode (static) */
	if(fastMode) return FSE_decompress_usingDTable_generic(dst, originalSize, cSrc, cSrcSize, dt, 1);
	return FSE_decompress_usingDTable_generic(dst, originalSize, cSrc, cSrcSize, dt, 0);
}

size_t FSE_decompress_wksp(void * dst, size_t dstCapacity, const void * cSrc, size_t cSrcSize, uint maxLog, void * workSpace, size_t wkspSize)
{
	return FSE_decompress_wksp_bmi2(dst, dstCapacity, cSrc, cSrcSize, maxLog, workSpace, wkspSize, /* bmi2 */ 0);
}

typedef struct {
	short ncount[FSE_MAX_SYMBOL_VALUE + 1];
	FSE_DTable dtable[1]; /* Dynamically sized */
} FSE_DecompressWksp;

FORCE_INLINE_TEMPLATE size_t FSE_decompress_wksp_body(void * dst, size_t dstCapacity, const void * cSrc, size_t cSrcSize,
    uint maxLog, void * workSpace, size_t wkspSize, int bmi2)
{
	const BYTE * const istart = PTR8C(cSrc);
	const BYTE * ip = istart;
	uint tableLog;
	uint maxSymbolValue = FSE_MAX_SYMBOL_VALUE;
	FSE_DecompressWksp* const wksp = (FSE_DecompressWksp*)workSpace;
	DEBUG_STATIC_ASSERT((FSE_MAX_SYMBOL_VALUE + 1) % 2 == 0);
	if(wkspSize < sizeof(*wksp)) 
		return ERROR(GENERIC);
	/* normal FSE decoding mode */
	{
		const size_t NCountLength = FSE_readNCount_bmi2(wksp->ncount, &maxSymbolValue, &tableLog, istart, cSrcSize, bmi2);
		if(FSE_isError(NCountLength)) return NCountLength;
		if(tableLog > maxLog) return ERROR(tableLog_tooLarge);
		assert(NCountLength <= cSrcSize);
		ip += NCountLength;
		cSrcSize -= NCountLength;
	}

	if(FSE_DECOMPRESS_WKSP_SIZE(tableLog, maxSymbolValue) > wkspSize) return ERROR(tableLog_tooLarge);
	assert(sizeof(*wksp) + FSE_DTABLE_SIZE(tableLog) <= wkspSize);
	workSpace = (BYTE *)workSpace + sizeof(*wksp) + FSE_DTABLE_SIZE(tableLog);
	wkspSize -= sizeof(*wksp) + FSE_DTABLE_SIZE(tableLog);

	CHECK_F(FSE_buildDTable_internal(wksp->dtable, wksp->ncount, maxSymbolValue, tableLog, workSpace, wkspSize) );

	{
		const void * ptr = wksp->dtable;
		const FSE_DTableHeader* DTableH = (const FSE_DTableHeader*)ptr;
		const uint32 fastMode = DTableH->fastMode;

		/* select fast mode (static) */
		if(fastMode) return FSE_decompress_usingDTable_generic(dst, dstCapacity, ip, cSrcSize, wksp->dtable, 1);
		return FSE_decompress_usingDTable_generic(dst, dstCapacity, ip, cSrcSize, wksp->dtable, 0);
	}
}

/* Avoids the FORCE_INLINE of the _body() function. */
static size_t FSE_decompress_wksp_body_default(void * dst, size_t dstCapacity, const void * cSrc, size_t cSrcSize, uint maxLog, void * workSpace, size_t wkspSize)
{
	return FSE_decompress_wksp_body(dst, dstCapacity, cSrc, cSrcSize, maxLog, workSpace, wkspSize, 0);
}

#if DYNAMIC_BMI2
BMI2_TARGET_ATTRIBUTE static size_t FSE_decompress_wksp_body_bmi2(void * dst, size_t dstCapacity, const void * cSrc, size_t cSrcSize, uint maxLog, void * workSpace, size_t wkspSize)
{
	return FSE_decompress_wksp_body(dst, dstCapacity, cSrc, cSrcSize, maxLog, workSpace, wkspSize, 1);
}

#endif

size_t FSE_decompress_wksp_bmi2(void * dst, size_t dstCapacity, const void * cSrc, size_t cSrcSize, uint maxLog, void * workSpace, size_t wkspSize, int bmi2)
{
#if DYNAMIC_BMI2
	if(bmi2) {
		return FSE_decompress_wksp_body_bmi2(dst, dstCapacity, cSrc, cSrcSize, maxLog, workSpace, wkspSize);
	}
#endif
	(void)bmi2;
	return FSE_decompress_wksp_body_default(dst, dstCapacity, cSrc, cSrcSize, maxLog, workSpace, wkspSize);
}

typedef FSE_DTable DTable_max_t[FSE_DTABLE_SIZE_U32(FSE_MAX_TABLELOG)];

#ifndef ZSTD_NO_UNUSED_FUNCTIONS
size_t FSE_buildDTable(FSE_DTable* dt, const short* normalizedCounter, uint maxSymbolValue, uint tableLog) 
{
	uint32 wksp[FSE_BUILD_DTABLE_WKSP_SIZE_U32(FSE_TABLELOG_ABSOLUTE_MAX, FSE_MAX_SYMBOL_VALUE)];
	return FSE_buildDTable_wksp(dt, normalizedCounter, maxSymbolValue, tableLog, wksp, sizeof(wksp));
}

size_t FSE_decompress(void * dst, size_t dstCapacity, const void * cSrc, size_t cSrcSize)
{
	/* Static analyzer seems unable to understand this table will be properly initialized later */
	uint32 wksp[FSE_DECOMPRESS_WKSP_SIZE_U32(FSE_MAX_TABLELOG, FSE_MAX_SYMBOL_VALUE)];
	return FSE_decompress_wksp(dst, dstCapacity, cSrc, cSrcSize, FSE_MAX_TABLELOG, wksp, sizeof(wksp));
}

#endif
#endif   /* FSE_COMMONDEFS_ONLY */
