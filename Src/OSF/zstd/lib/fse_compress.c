/* ******************************************************************
* FSE : Finite State Entropy encoder
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
#include <compiler.h>
#include <debug.h>      /* assert, DEBUGLOG */
#include "hist.h"       /* HIST_count_wksp */
#include <bitstream.h>
#define FSE_STATIC_LINKING_ONLY
#include <fse.h>
#include <error_private.h>
#define ZSTD_DEPS_NEED_MALLOC
#define ZSTD_DEPS_NEED_MATH64
#include <zstd_deps.h>  /* ZSTD_malloc, ZSTD_free, memcpy, memset */
#include <bits.h> /* ZSTD_highbit32 */
// 
// Error Management
// 
#define FSE_isError ERR_isError
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

/* FSE_buildCTable_wksp() :
 * Same as FSE_buildCTable(), but using an externally allocated scratch buffer (`workSpace`).
 * wkspSize should be sized to handle worst case situation, which is `1<<max_tableLog * sizeof(FSE_FUNCTION_TYPE)`
 * workSpace must also be properly aligned with FSE_FUNCTION_TYPE requirements
 */
size_t FSE_buildCTable_wksp(FSE_CTable* ct, const short* normalizedCounter, uint maxSymbolValue, uint tableLog, void * workSpace, size_t wkspSize)
{
	const uint32 tableSize = 1 << tableLog;
	const uint32 tableMask = tableSize - 1;
	void * const ptr = ct;
	uint16* const tableU16 = ( (uint16*)ptr) + 2;
	void * const FSCT = ((uint32 *)ptr) + 1 /* header */ + (tableLog ? tableSize>>1 : 1);
	FSE_symbolCompressionTransform* const symbolTT = (FSE_symbolCompressionTransform*)(FSCT);
	const uint32 step = FSE_TABLESTEP(tableSize);
	const uint32 maxSV1 = maxSymbolValue+1;
	uint16* cumul = (uint16*)workSpace; /* size = maxSV1 */
	FSE_FUNCTION_TYPE* const tableSymbol = (FSE_FUNCTION_TYPE*)(cumul + (maxSV1+1)); /* size = tableSize */
	uint32 highThreshold = tableSize-1;
	assert(((size_t)workSpace & 1) == 0); /* Must be 2 bytes-aligned */
	if(FSE_BUILD_CTABLE_WORKSPACE_SIZE(maxSymbolValue, tableLog) > wkspSize) 
		return ERROR(tableLog_tooLarge);
	/* CTable header */
	tableU16[-2] = (uint16)tableLog;
	tableU16[-1] = (uint16)maxSymbolValue;
	assert(tableLog < 16); /* required for threshold strategy to work */
	/* For explanations on how to distribute symbol values over the table :
	 * http://fastcompression.blogspot.fr/2014/02/fse-distributing-symbol-values.html */
     #ifdef __clang_analyzer__
	memzero(tableSymbol, sizeof(*tableSymbol) * tableSize); // useless initialization, just to keep scan-build happy 
     #endif
	/* symbol start positions */
	{   
		uint32 u;
	    cumul[0] = 0;
	    for(u = 1; u <= maxSV1; u++) {
		    if(normalizedCounter[u-1]==-1) { /* Low proba symbol */
			    cumul[u] = cumul[u-1] + 1;
			    tableSymbol[highThreshold--] = (FSE_FUNCTION_TYPE)(u-1);
		    }
		    else {
			    assert(normalizedCounter[u-1] >= 0);
			    cumul[u] = cumul[u-1] + (uint16)normalizedCounter[u-1];
			    assert(cumul[u] >= cumul[u-1]); /* no overflow */
		    }
	    }
	    cumul[maxSV1] = (uint16)(tableSize+1);
	}
	/* Spread symbols */
	if(highThreshold == tableSize - 1) {
		/* Case for no low prob count symbols. Lay down 8 bytes at a time
		 * to reduce branch misses since we are operating on a small block
		 */
		BYTE * const spread = tableSymbol + tableSize; /* size = tableSize + 8 (may write beyond tableSize) */
		{   
			uint64 const add = 0x0101010101010101ull;
		    size_t pos = 0;
		    uint64 sv = 0;
		    uint32 s;
		    for(s = 0; s<maxSV1; ++s, sv += add) {
			    int i;
			    int const n = normalizedCounter[s];
			    MEM_write64(spread + pos, sv);
			    for(i = 8; i < n; i += 8) {
				    MEM_write64(spread + pos + i, sv);
			    }
			    assert(n>=0);
			    pos += (size_t)n;
		    }
		}
		/* Spread symbols across the table. Lack of lowprob symbols means that
		 * we don't need variable sized inner loop, so we can unroll the loop and
		 * reduce branch misses.
		 */
		{   
			size_t position = 0;
		    size_t s;
		    const size_t unroll = 2; /* Experimentally determined optimal unroll */
		    assert(tableSize % unroll == 0); /* FSE_MIN_TABLELOG is 5 */
		    for(s = 0; s < (size_t)tableSize; s += unroll) {
			    size_t u;
			    for(u = 0; u < unroll; ++u) {
				    const size_t uPosition = (position + (u * step)) & tableMask;
				    tableSymbol[uPosition] = spread[s + u];
			    }
			    position = (position + (unroll * step)) & tableMask;
		    }
		    assert(position == 0); /* Must have initialized all positions */
		}
	}
	else {
		uint32 position = 0;
		uint32 symbol;
		for(symbol = 0; symbol<maxSV1; symbol++) {
			int nbOccurrences;
			int const freq = normalizedCounter[symbol];
			for(nbOccurrences = 0; nbOccurrences<freq; nbOccurrences++) {
				tableSymbol[position] = (FSE_FUNCTION_TYPE)symbol;
				position = (position + step) & tableMask;
				while(position > highThreshold)
					position = (position + step) & tableMask; /* Low proba area */
			}
		}
		assert(position==0); /* Must have initialized all positions */
	}
	/* Build table */
	{   
		uint32 u; 
		for(u = 0; u<tableSize; u++) {
		    FSE_FUNCTION_TYPE s = tableSymbol[u]; /* note : static analyzer may not understand tableSymbol is properly initialized */
		    tableU16[cumul[s]++] = (uint16)(tableSize+u); /* TableU16 : sorted by symbol order; gives next state value */
	    }
	}
	/* Build Symbol Transformation Table */
	{   
		uint total = 0;
	    uint s;
	    for(s = 0; s<=maxSymbolValue; s++) {
		    switch(normalizedCounter[s]) {
			    case  0:
				/* filling nonetheless, for compatibility with FSE_getMaxNbBits() */
				symbolTT[s].deltaNbBits = ((tableLog+1) << 16) - (1<<tableLog);
				break;

			    case -1:
			    case  1:
				symbolTT[s].deltaNbBits = (tableLog << 16) - (1<<tableLog);
				assert(total <= INT_MAX);
				symbolTT[s].deltaFindState = (int)(total - 1);
				total++;
				break;
			    default:
				assert(normalizedCounter[s] > 1);
				{   
					const uint32 maxBitsOut = tableLog - ZSTD_highbit32((uint32)normalizedCounter[s]-1);
				    const uint32 minStatePlus = (uint32)normalizedCounter[s] << maxBitsOut;
				    symbolTT[s].deltaNbBits = (maxBitsOut << 16) - minStatePlus;
				    symbolTT[s].deltaFindState = (int)(total - (uint)normalizedCounter[s]);
				    total +=  (uint)normalizedCounter[s];
				}
		    }
	    }
	}
#if 0  /* debug : symbol costs */
	DEBUGLOG(5, "\n --- table statistics : ");
	{   
	    for(uint32 symbol = 0; symbol<=maxSymbolValue; symbol++) {
		    DEBUGLOG(5, "%3u: w=%3i,   maxBits=%u, fracBits=%.2f", symbol, normalizedCounter[symbol], FSE_getMaxNbBits(symbolTT, symbol),
				(double)FSE_bitCost(symbolTT, tableLog, symbol, 8) / 256);
	    }
	}
#endif
	return 0;
}

#ifndef FSE_COMMONDEFS_ONLY
// 
// FSE NCount encoding
// 
size_t FSE_NCountWriteBound(uint maxSymbolValue, uint tableLog)
{
	const size_t maxHeaderSize = (((maxSymbolValue+1) * tableLog
	    + 4                        /* bitCount initialized at 4 */
	    + 2 /* first two symbols may use one additional bit each */) / 8)
	    + 1                         /* round up to whole nb bytes */
	    + 2 /* additional two bytes for bitstream flush */;
	return maxSymbolValue ? maxHeaderSize : FSE_NCOUNTBOUND; /* maxSymbolValue==0 ? use default */
}

static size_t FSE_writeNCount_generic(void * header, size_t headerBufferSize, const short* normalizedCounter, uint maxSymbolValue, uint tableLog, uint writeIsSafe)
{
	BYTE * const ostart = (BYTE *)header;
	BYTE * out = ostart;
	BYTE * const oend = ostart + headerBufferSize;
	int nbBits;
	const int tableSize = 1 << tableLog;
	int remaining;
	int threshold;
	uint32 bitStream = 0;
	int bitCount = 0;
	uint symbol = 0;
	const uint alphabetSize = maxSymbolValue + 1;
	int previousIs0 = 0;
	/* Table Size */
	bitStream += (tableLog-FSE_MIN_TABLELOG) << bitCount;
	bitCount  += 4;
	/* Init */
	remaining = tableSize+1; /* +1 for extra accuracy */
	threshold = tableSize;
	nbBits = tableLog+1;
	while((symbol < alphabetSize) && (remaining>1)) { /* stops at 1 */
		if(previousIs0) {
			uint start = symbol;
			while((symbol < alphabetSize) && !normalizedCounter[symbol]) 
				symbol++;
			if(symbol == alphabetSize) 
				break; /* incorrect distribution */
			while(symbol >= start+24) {
				start += 24;
				bitStream += 0xFFFFU << bitCount;
				if((!writeIsSafe) && (out > oend-2))
					return ERROR(dstSize_tooSmall); /* Buffer overflow */
				out[0] = (BYTE)bitStream;
				out[1] = (BYTE)(bitStream>>8);
				out += 2;
				bitStream >>= 16;
			}
			while(symbol >= start+3) {
				start += 3;
				bitStream += 3 << bitCount;
				bitCount += 2;
			}
			bitStream += (symbol-start) << bitCount;
			bitCount += 2;
			if(bitCount>16) {
				if((!writeIsSafe) && (out > oend - 2))
					return ERROR(dstSize_tooSmall); /* Buffer overflow */
				out[0] = (BYTE)bitStream;
				out[1] = (BYTE)(bitStream>>8);
				out += 2;
				bitStream >>= 16;
				bitCount -= 16;
			}
		}
		{   
			int count = normalizedCounter[symbol++];
		    const int max = (2*threshold-1) - remaining;
		    remaining -= count < 0 ? -count : count;
		    count++; /* +1 for extra accuracy */
		    if(count>=threshold)
			    count += max; /* [0..max[ [max..threshold[ (...) [threshold+max 2*threshold[ */
		    bitStream += count << bitCount;
		    bitCount  += nbBits;
		    bitCount  -= (count<max);
		    previousIs0  = (count==1);
		    if(remaining<1) 
				return ERROR(GENERIC);
		    while(remaining<threshold) {
			    nbBits--; threshold >>= 1;
		    }
		}
		if(bitCount>16) {
			if((!writeIsSafe) && (out > oend - 2))
				return ERROR(dstSize_tooSmall); /* Buffer overflow */
			out[0] = (BYTE)bitStream;
			out[1] = (BYTE)(bitStream>>8);
			out += 2;
			bitStream >>= 16;
			bitCount -= 16;
		}
	}
	if(remaining != 1)
		return ERROR(GENERIC); /* incorrect normalized distribution */
	assert(symbol <= alphabetSize);
	/* flush remaining bitStream */
	if((!writeIsSafe) && (out > oend - 2))
		return ERROR(dstSize_tooSmall); /* Buffer overflow */
	out[0] = (BYTE)bitStream;
	out[1] = (BYTE)(bitStream>>8);
	out += (bitCount+7) /8;
	return (out-ostart);
}

size_t FSE_writeNCount(void * buffer, size_t bufferSize, const short* normalizedCounter, uint maxSymbolValue, uint tableLog)
{
	if(tableLog > FSE_MAX_TABLELOG) return ERROR(tableLog_tooLarge); /* Unsupported */
	if(tableLog < FSE_MIN_TABLELOG) return ERROR(GENERIC); /* Unsupported */
	if(bufferSize < FSE_NCountWriteBound(maxSymbolValue, tableLog))
		return FSE_writeNCount_generic(buffer, bufferSize, normalizedCounter, maxSymbolValue, tableLog, 0);
	return FSE_writeNCount_generic(buffer, bufferSize, normalizedCounter, maxSymbolValue, tableLog, 1 /* write in buffer is safe */);
}
// 
// FSE Compression Code
// 
FSE_CTable* FSE_createCTable(uint maxSymbolValue, uint tableLog)
{
	size_t size;
	if(tableLog > FSE_TABLELOG_ABSOLUTE_MAX) 
		tableLog = FSE_TABLELOG_ABSOLUTE_MAX;
	size = FSE_CTABLE_SIZE_U32(tableLog, maxSymbolValue) * sizeof(uint32);
	return (FSE_CTable*)SAlloc::M(size);
}

void FSE_freeCTable(FSE_CTable* ct) 
{
	SAlloc::F(ct);
}

/* provides the minimum logSize to safely represent a distribution */
static uint FSE_minTableLog(size_t srcSize, uint maxSymbolValue)
{
	uint32 minBitsSrc = ZSTD_highbit32((uint32)(srcSize)) + 1;
	uint32 minBitsSymbols = ZSTD_highbit32(maxSymbolValue) + 2;
	uint32 minBits = minBitsSrc < minBitsSymbols ? minBitsSrc : minBitsSymbols;
	assert(srcSize > 1); // Not supported RLE should be used instead
	return minBits;
}

uint FSE_optimalTableLog_internal(uint maxTableLog, size_t srcSize, uint maxSymbolValue, uint minus)
{
	uint32 maxBitsSrc = ZSTD_highbit32((uint32)(srcSize - 1)) - minus;
	uint32 tableLog = maxTableLog;
	uint32 minBits = FSE_minTableLog(srcSize, maxSymbolValue);
	assert(srcSize > 1); // Not supported RLE should be used instead
	if(tableLog==0) tableLog = FSE_DEFAULT_TABLELOG;
	if(maxBitsSrc < tableLog) tableLog = maxBitsSrc; /* Accuracy can be reduced */
	if(minBits > tableLog) tableLog = minBits; /* Need a minimum to safely represent all symbol values */
	if(tableLog < FSE_MIN_TABLELOG) tableLog = FSE_MIN_TABLELOG;
	if(tableLog > FSE_MAX_TABLELOG) tableLog = FSE_MAX_TABLELOG;
	return tableLog;
}

uint FSE_optimalTableLog(uint maxTableLog, size_t srcSize, uint maxSymbolValue)
{
	return FSE_optimalTableLog_internal(maxTableLog, srcSize, maxSymbolValue, 2);
}

/* Secondary normalization method.
   To be used when primary method fails. */

static size_t FSE_normalizeM2(short* norm, uint32 tableLog, const uint * count, size_t total, uint32 maxSymbolValue, short lowProbCount)
{
	short const NOT_YET_ASSIGNED = -2;
	uint32 s;
	uint32 distributed = 0;
	uint32 ToDistribute;
	/* Init */
	const uint32 lowThreshold = (uint32)(total >> tableLog);
	uint32 lowOne = (uint32)((total * 3) >> (tableLog + 1));
	for(s = 0; s<=maxSymbolValue; s++) {
		if(count[s] == 0) {
			norm[s] = 0;
			continue;
		}
		if(count[s] <= lowThreshold) {
			norm[s] = lowProbCount;
			distributed++;
			total -= count[s];
			continue;
		}
		if(count[s] <= lowOne) {
			norm[s] = 1;
			distributed++;
			total -= count[s];
			continue;
		}

		norm[s] = NOT_YET_ASSIGNED;
	}
	ToDistribute = (1 << tableLog) - distributed;

	if(ToDistribute == 0)
		return 0;

	if((total / ToDistribute) > lowOne) {
		/* risk of rounding to zero */
		lowOne = (uint32)((total * 3) / (ToDistribute * 2));
		for(s = 0; s<=maxSymbolValue; s++) {
			if((norm[s] == NOT_YET_ASSIGNED) && (count[s] <= lowOne)) {
				norm[s] = 1;
				distributed++;
				total -= count[s];
				continue;
			}
		}
		ToDistribute = (1 << tableLog) - distributed;
	}

	if(distributed == maxSymbolValue+1) {
		/* all values are pretty poor;
		   probably incompressible data (should have already been detected);
		   find max, then give all remaining points to max */
		uint32 maxV = 0, maxC = 0;
		for(s = 0; s<=maxSymbolValue; s++)
			if(count[s] > maxC) {
				maxV = s; maxC = count[s];
			}
		norm[maxV] += (short)ToDistribute;
		return 0;
	}

	if(total == 0) {
		/* all of the symbols were low enough for the lowOne or lowThreshold */
		for(s = 0; ToDistribute > 0; s = (s+1)%(maxSymbolValue+1))
			if(norm[s] > 0) {
				ToDistribute--; norm[s]++;
			}
		return 0;
	}

	{   uint64 const vStepLog = 62 - tableLog;
	    uint64 const mid = (1ULL << (vStepLog-1)) - 1;
	    uint64 const rStep = ZSTD_div64((((uint64)1<<vStepLog) * ToDistribute) + mid, (uint32)total); /* scale on remaining
		                                                                                    */
	    uint64 tmpTotal = mid;
	    for(s = 0; s<=maxSymbolValue; s++) {
		    if(norm[s]==NOT_YET_ASSIGNED) {
			    uint64 const end = tmpTotal + (count[s] * rStep);
			    const uint32 sStart = (uint32)(tmpTotal >> vStepLog);
			    const uint32 sEnd = (uint32)(end >> vStepLog);
			    const uint32 weight = sEnd - sStart;
			    if(weight < 1)
				    return ERROR(GENERIC);
			    norm[s] = (short)weight;
			    tmpTotal = end;
		    }
	    }
	}

	return 0;
}

size_t FSE_normalizeCount(short* normalizedCounter, uint tableLog, const uint * count, size_t total, uint maxSymbolValue, uint useLowProbCount)
{
	/* Sanity checks */
	if(tableLog==0) 
		tableLog = FSE_DEFAULT_TABLELOG;
	if(tableLog < FSE_MIN_TABLELOG) 
		return ERROR(GENERIC); /* Unsupported size */
	if(tableLog > FSE_MAX_TABLELOG) 
		return ERROR(tableLog_tooLarge); /* Unsupported size */
	if(tableLog < FSE_minTableLog(total, maxSymbolValue)) 
		return ERROR(GENERIC); /* Too small tableLog, compression potentially impossible */
	{   
		static const uint32 rtbTable[] = { 0, 473195, 504333, 520860, 550000, 700000, 750000, 830000 };
	    short const lowProbCount = useLowProbCount ? -1 : 1;
	    uint64 const scale = 62 - tableLog;
	    uint64 const step = ZSTD_div64((uint64)1<<62, (uint32)total); /* <== here, one division ! */
	    uint64 const vStep = 1ULL<<(scale-20);
	    int    stillToDistribute = 1<<tableLog;
	    uint   s;
	    uint   largest = 0;
	    short  largestP = 0;
	    uint32 lowThreshold = (uint32)(total >> tableLog);
	    for(s = 0; s<=maxSymbolValue; s++) {
		    if(count[s] == total) 
				return 0; /* rle special case */
		    if(count[s] == 0) {
			    normalizedCounter[s] = 0; 
				continue;
		    }
		    if(count[s] <= lowThreshold) {
			    normalizedCounter[s] = lowProbCount;
			    stillToDistribute--;
		    }
		    else {
			    short proba = (short)((count[s]*step) >> scale);
			    if(proba<8) {
				    uint64 restToBeat = vStep * rtbTable[proba];
				    proba += (count[s]*step) - ((uint64)proba<<scale) > restToBeat;
			    }
			    if(proba > largestP) {
				    largestP = proba; largest = s;
			    }
			    normalizedCounter[s] = proba;
			    stillToDistribute -= proba;
		    }
	    }
	    if(-stillToDistribute >= (normalizedCounter[largest] >> 1)) {
		    /* corner case, need another normalization method */
		    const size_t errorCode = FSE_normalizeM2(normalizedCounter, tableLog, count, total, maxSymbolValue, lowProbCount);
		    if(FSE_isError(errorCode)) return errorCode;
	    }
	    else normalizedCounter[largest] += (short)stillToDistribute; }

#if 0
	{ /* Print Table (debug) */
		uint32 s;
		uint32 nTotal = 0;
		for(s = 0; s<=maxSymbolValue; s++)
			RAWLOG(2, "%3i: %4i \n", s, normalizedCounter[s]);
		for(s = 0; s<=maxSymbolValue; s++)
			nTotal += abs(normalizedCounter[s]);
		if(nTotal != (1U<<tableLog))
			RAWLOG(2, "Warning !!! Total == %u != %u !!!", nTotal, 1U<<tableLog);
		getchar();
	}
#endif
	return tableLog;
}

/* fake FSE_CTable, for raw (uncompressed) input */
size_t FSE_buildCTable_raw(FSE_CTable* ct, uint nbBits)
{
	const uint tableSize = 1 << nbBits;
	const uint tableMask = tableSize - 1;
	const uint maxSymbolValue = tableMask;
	void * const ptr = ct;
	uint16* const tableU16 = ( (uint16*)ptr) + 2;
	void * const FSCT = ((uint32 *)ptr) + 1 /* header */ + (tableSize>>1); /* assumption : tableLog >= 1 */
	FSE_symbolCompressionTransform* const symbolTT = (FSE_symbolCompressionTransform*)(FSCT);
	uint s;
	/* Sanity checks */
	if(nbBits < 1) 
		return ERROR(GENERIC);          /* min size */
	/* header */
	tableU16[-2] = (uint16)nbBits;
	tableU16[-1] = (uint16)maxSymbolValue;
	/* Build table */
	for(s = 0; s<tableSize; s++)
		tableU16[s] = (uint16)(tableSize + s);
	/* Build Symbol Transformation Table */
	{   
		const uint32 deltaNbBits = (nbBits << 16) - (1 << nbBits);
	    for(s = 0; s<=maxSymbolValue; s++) {
		    symbolTT[s].deltaNbBits = deltaNbBits;
		    symbolTT[s].deltaFindState = s-1;
	    }
	}
	return 0;
}

/* fake FSE_CTable, for rle input (always same symbol) */
size_t FSE_buildCTable_rle(FSE_CTable* ct, BYTE symbolValue)
{
	void * ptr = ct;
	uint16* tableU16 = ( (uint16*)ptr) + 2;
	void * FSCTptr = (uint32 *)ptr + 2;
	FSE_symbolCompressionTransform* symbolTT = (FSE_symbolCompressionTransform*)FSCTptr;
	/* header */
	tableU16[-2] = (uint16)0;
	tableU16[-1] = (uint16)symbolValue;
	/* Build table */
	tableU16[0] = 0;
	tableU16[1] = 0; /* just in case */

	/* Build Symbol Transformation Table */
	symbolTT[symbolValue].deltaNbBits = 0;
	symbolTT[symbolValue].deltaFindState = 0;

	return 0;
}

static size_t FSE_compress_usingCTable_generic(void * dst, size_t dstSize, const void * src, size_t srcSize, const FSE_CTable* ct, const uint fast)
{
	const BYTE * const istart = (const BYTE *)src;
	const BYTE * const iend = istart + srcSize;
	const BYTE * ip = iend;
	BIT_CStream_t bitC;
	FSE_CState_t CState1, CState2;
	/* init */
	if(srcSize <= 2) 
		return 0;
	{ 
		const size_t initError = BIT_initCStream(&bitC, dst, dstSize);
		if(FSE_isError(initError)) 
			return 0; /* not enough space available to write a bitstream */ 
	}
#define FSE_FLUSHBITS(s)  (fast ? BIT_flushBitsFast(s) : BIT_flushBits(s))
	if(srcSize & 1) {
		FSE_initCState2(&CState1, ct, *--ip);
		FSE_initCState2(&CState2, ct, *--ip);
		FSE_encodeSymbol(&bitC, &CState1, *--ip);
		FSE_FLUSHBITS(&bitC);
	}
	else {
		FSE_initCState2(&CState2, ct, *--ip);
		FSE_initCState2(&CState1, ct, *--ip);
	}
	/* join to mod 4 */
	srcSize -= 2;
	if((sizeof(bitC.bitContainer)*8 > FSE_MAX_TABLELOG*4+7 ) && (srcSize & 2)) { /* test bit 2 */
		FSE_encodeSymbol(&bitC, &CState2, *--ip);
		FSE_encodeSymbol(&bitC, &CState1, *--ip);
		FSE_FLUSHBITS(&bitC);
	}
	/* 2 or 4 encoding per loop */
	while(ip>istart) {
		FSE_encodeSymbol(&bitC, &CState2, *--ip);
		if(sizeof(bitC.bitContainer)*8 < FSE_MAX_TABLELOG*2+7) /* this test must be static */
			FSE_FLUSHBITS(&bitC);
		FSE_encodeSymbol(&bitC, &CState1, *--ip);
		if(sizeof(bitC.bitContainer)*8 > FSE_MAX_TABLELOG*4+7) { /* this test must be static */
			FSE_encodeSymbol(&bitC, &CState2, *--ip);
			FSE_encodeSymbol(&bitC, &CState1, *--ip);
		}
		FSE_FLUSHBITS(&bitC);
	}
	FSE_flushCState(&bitC, &CState2);
	FSE_flushCState(&bitC, &CState1);
	return BIT_closeCStream(&bitC);
}

size_t FSE_compress_usingCTable(void * dst, size_t dstSize, const void * src, size_t srcSize, const FSE_CTable* ct)
{
	const uint fast = (dstSize >= FSE_BLOCKBOUND(srcSize));
	if(fast)
		return FSE_compress_usingCTable_generic(dst, dstSize, src, srcSize, ct, 1);
	else
		return FSE_compress_usingCTable_generic(dst, dstSize, src, srcSize, ct, 0);
}

size_t FSE_compressBound(size_t size) { return FSE_COMPRESSBOUND(size); }

#ifndef ZSTD_NO_UNUSED_FUNCTIONS
/* FSE_compress_wksp() :
 * Same as FSE_compress2(), but using an externally allocated scratch buffer (`workSpace`).
 * `wkspSize` size must be `(1<<tableLog)`.
 */
size_t FSE_compress_wksp(void * dst, size_t dstSize, const void * src, size_t srcSize, uint maxSymbolValue, uint tableLog, void * workSpace, size_t wkspSize)
{
	BYTE * const ostart = (BYTE *)dst;
	BYTE * op = ostart;
	BYTE * const oend = ostart + dstSize;
	uint count[FSE_MAX_SYMBOL_VALUE+1];
	int16 norm[FSE_MAX_SYMBOL_VALUE+1];
	FSE_CTable* CTable = (FSE_CTable*)workSpace;
	const size_t CTableSize = FSE_CTABLE_SIZE_U32(tableLog, maxSymbolValue);
	void * scratchBuffer = (void *)(CTable + CTableSize);
	const size_t scratchBufferSize = wkspSize - (CTableSize * sizeof(FSE_CTable));

	/* init conditions */
	if(wkspSize < FSE_COMPRESS_WKSP_SIZE_U32(tableLog, maxSymbolValue)) return ERROR(tableLog_tooLarge);
	if(srcSize <= 1) return 0; /* Not compressible */
	if(!maxSymbolValue) maxSymbolValue = FSE_MAX_SYMBOL_VALUE;
	if(!tableLog) tableLog = FSE_DEFAULT_TABLELOG;
	/* Scan input and build symbol stats */
	{   
		CHECK_V_F(maxCount, HIST_count_wksp(count, &maxSymbolValue, src, srcSize, scratchBuffer, scratchBufferSize) );
	    if(maxCount == srcSize) return 1; /* only a single symbol in src : rle */
	    if(maxCount == 1) return 0;      /* each symbol present maximum once => not compressible */
	    if(maxCount < (srcSize >> 7)) return 0; /* Heuristic : not compressible enough */
	}
	tableLog = FSE_optimalTableLog(tableLog, srcSize, maxSymbolValue);
	CHECK_F(FSE_normalizeCount(norm, tableLog, count, srcSize, maxSymbolValue, /* useLowProbCount */ srcSize >= 2048) );
	/* Write table description header */
	{   
		CHECK_V_F(nc_err, FSE_writeNCount(op, oend-op, norm, maxSymbolValue, tableLog) );
	    op += nc_err;
	}
	/* Compress */
	CHECK_F(FSE_buildCTable_wksp(CTable, norm, maxSymbolValue, tableLog, scratchBuffer, scratchBufferSize) );
	{   
		CHECK_V_F(cSize, FSE_compress_usingCTable(op, oend - op, src, srcSize, CTable) );
	    if(cSize == 0) 
			return 0; /* not enough space for compressed data */
	    op += cSize;
	}
	/* check compressibility */
	if( (size_t)(op-ostart) >= srcSize-1) 
		return 0;
	return op-ostart;
}

typedef struct {
	FSE_CTable CTable_max[FSE_CTABLE_SIZE_U32(FSE_MAX_TABLELOG, FSE_MAX_SYMBOL_VALUE)];
	union {
		uint32 hist_wksp[HIST_WKSP_SIZE_U32];
		BYTE scratchBuffer[1 << FSE_MAX_TABLELOG];
	} workspace;
} fseWkspMax_t;

size_t FSE_compress2(void * dst, size_t dstCapacity, const void * src, size_t srcSize, uint maxSymbolValue, uint tableLog)
{
	fseWkspMax_t scratchBuffer;
	DEBUG_STATIC_ASSERT(sizeof(scratchBuffer) >= FSE_COMPRESS_WKSP_SIZE_U32(FSE_MAX_TABLELOG, FSE_MAX_SYMBOL_VALUE)); // compilation failures here means scratchBuffer is not large enough
	if(tableLog > FSE_MAX_TABLELOG) return ERROR(tableLog_tooLarge);
	return FSE_compress_wksp(dst, dstCapacity, src, srcSize, maxSymbolValue, tableLog, &scratchBuffer, sizeof(scratchBuffer));
}

size_t FSE_compress(void * dst, size_t dstCapacity, const void * src, size_t srcSize)
{
	return FSE_compress2(dst, dstCapacity, src, srcSize, FSE_MAX_SYMBOL_VALUE, FSE_DEFAULT_TABLELOG);
}

#endif
#endif   /* FSE_COMMONDEFS_ONLY */
