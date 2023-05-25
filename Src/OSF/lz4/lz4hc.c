/*
    LZ4 HC - High Compression Mode of LZ4
    Copyright (C) 2011-2017, Yann Collet.

    BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

 * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the
    distribution.

    You can contact the author at :
       - LZ4 source repository : https://github.com/lz4/lz4
       - LZ4 public forum : https://groups.google.com/forum/#!forum/lz4c
 */
// note : lz4hc is not an independent module, it requires lz4.h/lz4.c for proper compilation */
// 
#include <slib.h>
// 
// Tuning Parameter
// 
// HEAPMODE :
// Select how default compression function will allocate workplace memory,
// in stack (0:fastest), or in heap (1:requires malloc()).
// Since workplace is rather large, heap mode is recommended.
// 
#ifndef LZ4HC_HEAPMODE
	#define LZ4HC_HEAPMODE 1
#endif
/*===    Dependency    ===*/
#define LZ4_HC_STATIC_LINKING_ONLY
#include "lz4hc.h"
/*===   Common LZ4 definitions   ===*/
#if defined(__GNUC__)
	#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#if defined (__clang__)
	#pragma clang diagnostic ignored "-Wunused-function"
#endif
#define LZ4_COMMONDEFS_ONLY
#include "lz4.c"   /* LZ4_count, constants, mem */
/*===   Constants   ===*/
#define OPTIMAL_ML (int)((ML_MASK-1)+MINMATCH)
#define LZ4_OPT_NUM   (1<<12)
/*===   Macros   ===*/
//#define MIN(a, b)   ( (a) < (b) ? (a) : (b) )
//#define MAX(a, b)   ( (a) > (b) ? (a) : (b) )
#define HASH_FUNCTION(i)         (((i) * SlConst::MagicHashPrime32) >> ((MINMATCH*8)-LZ4HC_HASH_LOG))
#define DELTANEXTMAXD(p)         chainTable[(p) & LZ4HC_MAXD_MASK]    /* flexible, LZ4HC_MAXD dependent */
#define DELTANEXTU16(table, pos) table[static_cast<uint16>(pos)]   /* faster */

static uint32 LZ4HC_hashPtr(const void * ptr) 
{
	return HASH_FUNCTION(LZ4_read32(ptr));
}

typedef enum { 
	noDictCtx, 
	usingDictCtx 
} dictCtx_directive;
// 
// HC Compression
// 
static void LZ4HC_clearTables(LZ4HC_CCtx_internal* hc4)
{
	memzero(/*(void *)*/hc4->hashTable, sizeof(hc4->hashTable));
	memset(hc4->chainTable, 0xff, sizeof(hc4->chainTable));
}

static void LZ4HC_init(LZ4HC_CCtx_internal* hc4, const uint8 * start)
{
	uptrval startingOffset = hc4->end - hc4->base;
	if(startingOffset > SGIGABYTE(1)) {
		LZ4HC_clearTables(hc4);
		startingOffset = 0;
	}
	startingOffset += SKILOBYTE(64);
	hc4->nextToUpdate = (uint32)startingOffset;
	hc4->base = start - startingOffset;
	hc4->end = start;
	hc4->dictBase = start - startingOffset;
	hc4->dictLimit = (uint32)startingOffset;
	hc4->lowLimit = (uint32)startingOffset;
}
// 
// Update chains up to ip (excluded) 
// 
LZ4_FORCE_INLINE void LZ4HC_Insert(LZ4HC_CCtx_internal* hc4, const uint8 * ip)
{
	uint16 * const chainTable = hc4->chainTable;
	uint32 * const hashTable  = hc4->hashTable;
	const uint8 * const base = hc4->base;
	const uint32 target = (uint32)(ip - base);
	uint32 idx = hc4->nextToUpdate;
	while(idx < target) {
		const uint32 h = LZ4HC_hashPtr(base+idx);
		size_t delta = idx - hashTable[h];
		if(delta>MAX_DISTANCE) delta = MAX_DISTANCE;
		DELTANEXTU16(chainTable, idx) = (uint16)delta;
		hashTable[h] = idx;
		idx++;
	}
	hc4->nextToUpdate = target;
}
// 
// LZ4HC_countBack() :
// @return : negative value, nb of common bytes before ip/match */
// 
LZ4_FORCE_INLINE int LZ4HC_countBack(const uint8 * const ip, const uint8 * const match, const uint8 * const iMin, const uint8 * const mMin)
{
	int back = 0;
	int const min = (int)MAX(iMin - ip, mMin - match);
	assert(min <= 0);
	assert(ip >= iMin); assert((size_t)(ip-iMin) < (1U<<31));
	assert(match >= mMin); assert((size_t)(match - mMin) < (1U<<31));
	while((back > min) && (ip[back-1] == match[back-1]))
		back--;
	return back;
}
// 
// LZ4HC_countPattern() :
// pattern32 must be a sample of repetitive pattern of length 1, 2 or 4 (but not 3!) 
// 
static uint LZ4HC_countPattern(const uint8 * ip, const uint8 * const iEnd, const uint32 pattern32)
{
	const uint8 * const iStart = ip;
	reg_t const pattern = (sizeof(pattern)==8) ? (reg_t)pattern32 + (((reg_t)pattern32) << 32) : pattern32;
	while(LIKELY(ip < iEnd-(sizeof(pattern)-1))) {
		reg_t const diff = LZ4_read_ARCH(ip) ^ pattern;
		if(!diff) {
			ip += sizeof(pattern); 
			continue;
		}
		ip += LZ4_NbCommonBytes(diff);
		return (uint)(ip - iStart);
	}
	if(LZ4_isLittleEndian()) {
		reg_t patternByte = pattern;
		while((ip<iEnd) && (*ip == (uint8)patternByte)) {
			ip++; 
			patternByte >>= 8;
		}
	}
	else { /* big endian */
		uint32 bitOffset = (sizeof(pattern)*8) - 8;
		while(ip < iEnd) {
			uint8 const byte = (uint8)(pattern >> bitOffset);
			if(*ip != byte) 
				break;
			ip++; 
			bitOffset -= 8;
		}
	}
	return (uint)(ip - iStart);
}
// 
// LZ4HC_reverseCountPattern() :
// pattern must be a sample of repetitive pattern of length 1, 2 or 4 (but not 3!)
// read using natural platform endianess 
// 
static uint LZ4HC_reverseCountPattern(const uint8 * ip, const uint8 * const iLow, uint32 pattern)
{
	const uint8 * const iStart = ip;
	while(LIKELY(ip >= iLow+4)) {
		if(LZ4_read32(ip-4) != pattern) 
			break;
		ip -= 4;
	}
	{   
		const uint8 * bytePtr = (const uint8 *)(&pattern) + 3; /* works for any endianess */
	    while(LIKELY(ip>iLow)) {
		    if(ip[-1] != *bytePtr) break;
		    ip--; bytePtr--;
	    }
	}
	return (uint)(iStart - ip);
}

typedef enum { rep_untested, rep_not, rep_confirmed } repeat_state_e;
typedef enum { favorCompressionRatio = 0, favorDecompressionSpeed } HCfavor_e;

LZ4_FORCE_INLINE int LZ4HC_InsertAndGetWiderMatch(LZ4HC_CCtx_internal* hc4, const uint8 * const ip,
    const uint8 * const iLowLimit, const uint8 * const iHighLimit, int longest, const uint8 ** matchpos,
    const uint8 ** startpos, const int maxNbAttempts, const int patternAnalysis, const int chainSwap,
    const dictCtx_directive dict, const HCfavor_e favorDecSpeed)
{
	uint16* const chainTable = hc4->chainTable;
	uint32 * const HashTable = hc4->hashTable;
	const LZ4HC_CCtx_internal * const dictCtx = hc4->dictCtx;
	const uint8 * const base = hc4->base;
	const uint32 dictLimit = hc4->dictLimit;
	const uint8 * const lowPrefixPtr = base + dictLimit;
	const uint32 ipIndex = (uint32)(ip - base);
	const uint32 lowestMatchIndex = (hc4->lowLimit + SKILOBYTE(64) > ipIndex) ? hc4->lowLimit : ipIndex - MAX_DISTANCE;
	const uint8 * const dictBase = hc4->dictBase;
	int const lookBackLength = (int)(ip-iLowLimit);
	int nbAttempts = maxNbAttempts;
	int matchChainPos = 0;
	const uint32 pattern = LZ4_read32(ip);
	uint32 matchIndex;
	repeat_state_e repeat = rep_untested;
	size_t srcPatternLength = 0;

	//DEBUGLOG(7, "LZ4HC_InsertAndGetWiderMatch");
	/* First Match */
	LZ4HC_Insert(hc4, ip);
	matchIndex = HashTable[LZ4HC_hashPtr(ip)];
	//DEBUGLOG(7, "First match at index %u / %u (lowestMatchIndex)", matchIndex, lowestMatchIndex);
	while((matchIndex>=lowestMatchIndex) && (nbAttempts)) {
		int matchLength = 0;
		nbAttempts--;
		assert(matchIndex < ipIndex);
		if(favorDecSpeed && (ipIndex - matchIndex < 8)) {
			/* do nothing */
		}
		else if(matchIndex >= dictLimit) { /* within current Prefix */
			const uint8 * const matchPtr = base + matchIndex;
			assert(matchPtr >= lowPrefixPtr);
			assert(matchPtr < ip);
			assert(longest >= 1);
			if(LZ4_read16(iLowLimit + longest - 1) == LZ4_read16(matchPtr - lookBackLength + longest - 1)) {
				if(LZ4_read32(matchPtr) == pattern) {
					int const back = lookBackLength ? LZ4HC_countBack(ip, matchPtr, iLowLimit, lowPrefixPtr) : 0;
					matchLength = MINMATCH + LZ4_count(ip+MINMATCH, matchPtr+MINMATCH, iHighLimit);
					matchLength -= back;
					if(matchLength > longest) {
						longest = matchLength;
						*matchpos = matchPtr + back;
						*startpos = ip + back;
					}
				}
			}
		}
		else { /* lowestMatchIndex <= matchIndex < dictLimit */
			const uint8 * const matchPtr = dictBase + matchIndex;
			if(LZ4_read32(matchPtr) == pattern) {
				const uint8 * const dictStart = dictBase + hc4->lowLimit;
				int back = 0;
				const uint8 * vLimit = ip + (dictLimit - matchIndex);
				SETMIN(vLimit, iHighLimit);
				matchLength = LZ4_count(ip+MINMATCH, matchPtr+MINMATCH, vLimit) + MINMATCH;
				if((ip+matchLength == vLimit) && (vLimit < iHighLimit))
					matchLength += LZ4_count(ip+matchLength, lowPrefixPtr, iHighLimit);
				back = lookBackLength ? LZ4HC_countBack(ip, matchPtr, iLowLimit, dictStart) : 0;
				matchLength -= back;
				if(matchLength > longest) {
					longest = matchLength;
					*matchpos = base + matchIndex + back; /* virtual pos, relative to ip, to retrieve offset */
					*startpos = ip + back;
				}
			}
		}
		if(chainSwap && matchLength==longest) { /* better match => select a better chain */
			assert(lookBackLength==0); /* search forward only */
			if(matchIndex + longest <= ipIndex) {
				uint32 distanceToNextMatch = 1;
				int pos;
				for(pos = 0; pos <= longest - MINMATCH; pos++) {
					const uint32 candidateDist = DELTANEXTU16(chainTable, matchIndex + pos);
					if(candidateDist > distanceToNextMatch) {
						distanceToNextMatch = candidateDist;
						matchChainPos = pos;
					}
				}
				if(distanceToNextMatch > 1) {
					if(distanceToNextMatch > matchIndex) 
						break; /* avoid overflow */
					matchIndex -= distanceToNextMatch;
					continue;
				}
			}
		}

		{   
			const uint32 distNextMatch = DELTANEXTU16(chainTable, matchIndex);
		    if(patternAnalysis && distNextMatch==1 && matchChainPos==0) {
			    const uint32 matchCandidateIdx = matchIndex-1;
			    /* may be a repeated pattern */
			    if(repeat == rep_untested) {
				    if( ((pattern & 0xFFFF) == (pattern >> 16)) &  ((pattern & 0xFF)   == (pattern >> 24))) {
					    repeat = rep_confirmed;
					    srcPatternLength =
						LZ4HC_countPattern(ip+sizeof(pattern), iHighLimit, pattern) + sizeof(pattern);
				    }
				    else {
					    repeat = rep_not;
				    }
			    }
			    if((repeat == rep_confirmed) && (matchCandidateIdx >= dictLimit)) { // same segment only 
				    const uint8 * const matchPtr = base + matchCandidateIdx;
				    if(LZ4_read32(matchPtr) == pattern) { // good candidate 
					    const size_t forwardPatternLength = LZ4HC_countPattern(matchPtr+sizeof(pattern), iHighLimit, pattern) + sizeof(pattern);
					    const uint8 * const lowestMatchPtr = (lowPrefixPtr + MAX_DISTANCE >= ip) ? lowPrefixPtr : ip - MAX_DISTANCE;
					    const size_t backLength = LZ4HC_reverseCountPattern(matchPtr, lowestMatchPtr, pattern);
					    const size_t currentSegmentLength = backLength + forwardPatternLength;
					    if((currentSegmentLength >= srcPatternLength) // current pattern segment large enough to contain full srcPatternLength 
						&& (forwardPatternLength <= srcPatternLength)) { // haven't reached this position yet
						    matchIndex = matchCandidateIdx + (uint32)forwardPatternLength - (uint32)srcPatternLength; // best position, full pattern, might be followed by more match
					    }
					    else {
							// farthest position in current segment, will find a match of length currentSegmentLength + maybe some back 
						    matchIndex = matchCandidateIdx - (uint32)backLength; 
						    if(lookBackLength==0) { /* no back possible */
							    const size_t maxML = MIN(currentSegmentLength, srcPatternLength);
							    if((size_t)longest < maxML) {
								    assert(base + matchIndex < ip);
								    if(ip - (base+matchIndex) > MAX_DISTANCE) 
										break;
								    assert(maxML < SGIGABYTE(2));
								    longest = (int)maxML;
								    *matchpos = base + matchIndex; //virtual pos, relative to ip, to retrieve offset 
								    *startpos = ip;
							    }
							    {   
									const uint32 distToNextPattern = DELTANEXTU16(chainTable, matchIndex);
									if(distToNextPattern > matchIndex) 
										break; // avoid overflow 
									matchIndex -= distToNextPattern;
								}
						    }
					    }
					    continue;
				    }
			    }
		    }
		} /* PA optimization */

		/* follow current chain */
		matchIndex -= DELTANEXTU16(chainTable, matchIndex+matchChainPos);
	} /* while ((matchIndex>=lowestMatchIndex) && (nbAttempts)) */

	if(dict == usingDictCtx && nbAttempts && ipIndex - lowestMatchIndex < MAX_DISTANCE) {
		const size_t dictEndOffset = dictCtx->end - dictCtx->base;
		uint32 dictMatchIndex = dictCtx->hashTable[LZ4HC_hashPtr(ip)];
		assert(dictEndOffset <= SGIGABYTE(1));
		matchIndex = dictMatchIndex + lowestMatchIndex - (uint32)dictEndOffset;
		while(ipIndex - matchIndex <= MAX_DISTANCE && nbAttempts--) {
			const uint8 * const matchPtr = dictCtx->base + dictMatchIndex;
			if(LZ4_read32(matchPtr) == pattern) {
				int mlt;
				int back = 0;
				const uint8 * vLimit = ip + (dictEndOffset - dictMatchIndex);
				SETMIN(vLimit, iHighLimit);
				mlt = LZ4_count(ip+MINMATCH, matchPtr+MINMATCH, vLimit) + MINMATCH;
				back = lookBackLength ? LZ4HC_countBack(ip, matchPtr, iLowLimit, dictCtx->base + dictCtx->dictLimit) : 0;
				mlt -= back;
				if(mlt > longest) {
					longest = mlt;
					*matchpos = base + matchIndex + back;
					*startpos = ip + back;
				}
			}
			{   
				const uint32 nextOffset = DELTANEXTU16(dictCtx->chainTable, dictMatchIndex);
			    dictMatchIndex -= nextOffset;
			    matchIndex -= nextOffset;
			}
		}
	}
	return longest;
}

LZ4_FORCE_INLINE int LZ4HC_InsertAndFindBestMatch(LZ4HC_CCtx_internal* const hc4,   /* Index table will be updated */
    const uint8 * const ip, const uint8 * const iLimit, const uint8 ** matchpos, const int maxNbAttempts, const int patternAnalysis, const dictCtx_directive dict)
{
	const uint8 * uselessPtr = ip;
	/* note : LZ4HC_InsertAndGetWiderMatch() is able to modify the starting position of a match (*startpos),
	 * but this won't be the case here, as we define iLowLimit==ip,
	 * so LZ4HC_InsertAndGetWiderMatch() won't be allowed to search past ip */
	return LZ4HC_InsertAndGetWiderMatch(hc4, ip, ip, iLimit, MINMATCH-1, matchpos, &uselessPtr, maxNbAttempts, patternAnalysis, 0 /*chainSwap*/, dict, favorCompressionRatio);
}

typedef enum {
	noLimit = 0,
	limitedOutput = 1,
	limitedDestSize = 2
} limitedOutput_directive;

/* LZ4HC_encodeSequence() :
 * @return : 0 if ok,
 *      1 if buffer issue detected */
LZ4_FORCE_INLINE int LZ4HC_encodeSequence(const uint8 ** ip, uint8 ** op, const uint8 ** anchor, int matchLength, const uint8 * const match, limitedOutput_directive limit, uint8 * oend)
{
	size_t length;
	uint8 * const token = (*op)++;
#if defined(LZ4_DEBUG) && (LZ4_DEBUG >= 6)
	static const uint8 * start = NULL;
	static uint32 totalCost = 0;
	const uint32 pos = (start==NULL) ? 0 : (uint32)(*anchor - start);
	const uint32 ll = (uint32)(*ip - *anchor);
	const uint32 llAdd = (ll>=15) ? ((ll-15) / 255) + 1 : 0;
	const uint32 mlAdd = (matchLength>=19) ? ((matchLength-19) / 255) + 1 : 0;
	const uint32 cost = 1 + llAdd + ll + 2 + mlAdd;
	SETIFZQ(start, *anchor); // only works for single segment
	/* g_debuglog_enable = (pos >= 2228) & (pos <= 2262); */
	//DEBUGLOG(6, "pos:%7u -- literals:%3u, match:%4i, offset:%5u, cost:%3u + %u", pos, (uint32)(*ip - *anchor), matchLength, (uint32)(*ip-match), cost, totalCost);
	totalCost += cost;
#endif
	/* Encode Literal length */
	length = (size_t)(*ip - *anchor);
	if((limit) && ((*op + (length >> 8) + length + (2 + 1 + LASTLITERALS)) > oend)) 
		return 1; // Check output limit
	if(length >= RUN_MASK) {
		size_t len = length - RUN_MASK;
		*token = (RUN_MASK << ML_BITS);
		for(; len >= 255; len -= 255) 
			*(*op)++ = 255;
		*(*op)++ = (uint8)len;
	}
	else {
		*token = (uint8)(length << ML_BITS);
	}
	/* Copy Literals */
	LZ4_wildCopy(*op, *anchor, (*op) + length);
	*op += length;
	/* Encode Offset */
	assert( (*ip - match) <= MAX_DISTANCE); // note : consider providing offset as a value, rather than as a pointer difference 
	LZ4_writeLE16(*op, static_cast<uint16>(*ip-match)); 
	*op += 2;
	/* Encode MatchLength */
	assert(matchLength >= MINMATCH);
	length = (size_t)(matchLength - MINMATCH);
	if((limit) && (*op + (length >> 8) + (1 + LASTLITERALS) > oend)) 
		return 1; /* Check output limit */
	if(length >= ML_MASK) {
		*token += ML_MASK;
		length -= ML_MASK;
		for(; length >= 510; length -= 510) {
			*(*op)++ = 255; *(*op)++ = 255;
		}
		if(length >= 255) {
			length -= 255; *(*op)++ = 255;
		}
		*(*op)++ = (uint8)length;
	}
	else {
		*token += (uint8)(length);
	}
	/* Prepare next loop */
	*ip += matchLength;
	*anchor = *ip;
	return 0;
}

LZ4_FORCE_INLINE int LZ4HC_compress_hashChain(LZ4HC_CCtx_internal* const ctx, const char * const source, char * const dest,
    int* srcSizePtr, int const maxOutputSize, uint maxNbAttempts, const limitedOutput_directive limit, const dictCtx_directive dict)
{
	const int inputSize = *srcSizePtr;
	const int patternAnalysis = (maxNbAttempts > 128); /* levels 9+ */
	const uint8 * ip = (const uint8 *)source;
	const uint8 * anchor = ip;
	const uint8 * const iend = ip + inputSize;
	const uint8 * const mflimit = iend - MFLIMIT;
	const uint8 * const matchlimit = (iend - LASTLITERALS);
	uint8 * optr = (uint8 *)dest;
	uint8 * op = (uint8 *)dest;
	uint8 * oend = op + maxOutputSize;
	int ml0, ml, ml2, ml3;
	const uint8 * start0;
	const uint8 * ref0;
	const uint8 * ref = NULL;
	const uint8 * start2 = NULL;
	const uint8 * ref2 = NULL;
	const uint8 * start3 = NULL;
	const uint8 * ref3 = NULL;
	/* init */
	*srcSizePtr = 0;
	if(limit == limitedDestSize) 
		oend -= LASTLITERALS; // Hack for support LZ4 format restriction 
	if(inputSize < LZ4_minLength) 
		goto _last_literals; // Input too small, no compression (all literals) 
	/* Main Loop */
	while(ip <= mflimit) {
		ml = LZ4HC_InsertAndFindBestMatch(ctx, ip, matchlimit, &ref, maxNbAttempts, patternAnalysis, dict);
		if(ml<MINMATCH) {
			ip++; continue;
		}
		/* saved, in case we would skip too much */
		start0 = ip; 
		ref0 = ref; 
		ml0 = ml;
_Search2:
		if(ip+ml <= mflimit) {
			ml2 = LZ4HC_InsertAndGetWiderMatch(ctx, ip + ml - 2, ip + 0, matchlimit, ml, &ref2, &start2, maxNbAttempts, patternAnalysis, 0, dict, favorCompressionRatio);
		}
		else {
			ml2 = ml;
		}
		if(ml2 == ml) { /* No better match => encode ML1 */
			optr = op;
			if(LZ4HC_encodeSequence(&ip, &op, &anchor, ml, ref, limit, oend)) 
				goto _dest_overflow;
			continue;
		}
		if(start0 < ip) { /* first match was skipped at least once */
			if(start2 < ip + ml0) { /* squeezing ML1 between ML0(original ML1) and ML2 */
				ip = start0; ref = ref0; ml = ml0; /* restore initial ML1 */
			}
		}
		/* Here, start0==ip */
		if((start2 - ip) < 3) { /* First Match too small : removed */
			ml = ml2;
			ip = start2;
			ref = ref2;
			goto _Search2;
		}
_Search3:
		/* At this stage, we have :
		 *  ml2 > ml1, and
		 *  ip1+3 <= ip2 (usually < ip1+ml1) */
		if((start2 - ip) < OPTIMAL_ML) {
			int correction;
			int new_ml = ml;
			if(new_ml > OPTIMAL_ML) 
				new_ml = OPTIMAL_ML;
			if(ip+new_ml > start2 + ml2 - MINMATCH) 
				new_ml = (int)(start2 - ip) + ml2 - MINMATCH;
			correction = new_ml - (int)(start2 - ip);
			if(correction > 0) {
				start2 += correction;
				ref2 += correction;
				ml2 -= correction;
			}
		}
		/* Now, we have start2 = ip+new_ml, with new_ml = min(ml, OPTIMAL_ML=18) */
		if(start2 + ml2 <= mflimit) {
			ml3 = LZ4HC_InsertAndGetWiderMatch(ctx, start2 + ml2 - 3, start2, matchlimit, ml2, &ref3, &start3, maxNbAttempts, patternAnalysis, 0, dict, favorCompressionRatio);
		}
		else {
			ml3 = ml2;
		}
		if(ml3 == ml2) { /* No better match => encode ML1 and ML2 */
			/* ip & ref are known; Now for ml */
			if(start2 < ip+ml) 
				ml = (int)(start2 - ip);
			/* Now, encode 2 sequences */
			optr = op;
			if(LZ4HC_encodeSequence(&ip, &op, &anchor, ml, ref, limit, oend)) 
				goto _dest_overflow;
			ip = start2;
			optr = op;
			if(LZ4HC_encodeSequence(&ip, &op, &anchor, ml2, ref2, limit, oend)) 
				goto _dest_overflow;
			continue;
		}
		if(start3 < ip+ml+3) { // Not enough space for match 2 : remove it 
			if(start3 >= (ip+ml)) { // can write Seq1 immediately ==> Seq2 is removed, so Seq3 becomes Seq1
				if(start2 < ip+ml) {
					int correction = (int)(ip+ml - start2);
					start2 += correction;
					ref2 += correction;
					ml2 -= correction;
					if(ml2 < MINMATCH) {
						start2 = start3;
						ref2 = ref3;
						ml2 = ml3;
					}
				}
				optr = op;
				if(LZ4HC_encodeSequence(&ip, &op, &anchor, ml, ref, limit, oend)) 
					goto _dest_overflow;
				ip  = start3;
				ref = ref3;
				ml  = ml3;

				start0 = start2;
				ref0 = ref2;
				ml0 = ml2;
				goto _Search2;
			}
			start2 = start3;
			ref2 = ref3;
			ml2 = ml3;
			goto _Search3;
		}
		/*
		 * OK, now we have 3 ascending matches;
		 * let's write the first one ML1.
		 * ip & ref are known; Now decide ml.
		 */
		if(start2 < ip+ml) {
			if((start2 - ip) < OPTIMAL_ML) {
				int correction;
				if(ml > OPTIMAL_ML) 
					ml = OPTIMAL_ML;
				if(ip + ml > start2 + ml2 - MINMATCH) 
					ml = (int)(start2 - ip) + ml2 - MINMATCH;
				correction = ml - (int)(start2 - ip);
				if(correction > 0) {
					start2 += correction;
					ref2 += correction;
					ml2 -= correction;
				}
			}
			else {
				ml = (int)(start2 - ip);
			}
		}
		optr = op;
		if(LZ4HC_encodeSequence(&ip, &op, &anchor, ml, ref, limit, oend)) 
			goto _dest_overflow;
		/* ML2 becomes ML1 */
		ip = start2; 
		ref = ref2; 
		ml = ml2;
		/* ML3 becomes ML2 */
		start2 = start3; 
		ref2 = ref3; 
		ml2 = ml3;
		/* let's find a new ML3 */
		goto _Search3;
	}

_last_literals:
	/* Encode Last Literals */
	{   
		size_t lastRunSize = (size_t)(iend - anchor); /* literals */
	    size_t litLength = (lastRunSize + 255 - RUN_MASK) / 255;
	    const size_t totalSize = 1 + litLength + lastRunSize;
	    if(limit == limitedDestSize) 
			oend += LASTLITERALS; /* restore correct value */
	    if(limit && (op + totalSize > oend)) {
		    if(limit == limitedOutput) 
				return 0; /* Check output limit */
		    /* adapt lastRunSize to fill 'dest' */
		    lastRunSize  = (size_t)(oend - op) - 1;
		    litLength = (lastRunSize + 255 - RUN_MASK) / 255;
		    lastRunSize -= litLength;
	    }
	    ip = anchor + lastRunSize;
	    if(lastRunSize >= RUN_MASK) {
		    size_t accumulator = lastRunSize - RUN_MASK;
		    *op++ = (RUN_MASK << ML_BITS);
		    for(; accumulator >= 255; accumulator -= 255) 
				*op++ = 255;
		    *op++ = (uint8)accumulator;
	    }
	    else {
		    *op++ = (uint8)(lastRunSize << ML_BITS);
	    }
	    memcpy(op, anchor, lastRunSize);
	    op += lastRunSize;}

	/* End */
	*srcSizePtr = (int)(((const char *)ip) - source);
	return (int)(((char *)op)-dest);
_dest_overflow:
	if(limit == limitedDestSize) {
		op = optr; /* restore correct out pointer */
		goto _last_literals;
	}
	return 0;
}

static int LZ4HC_compress_optimal(LZ4HC_CCtx_internal* ctx, const char * const source, char * dst,
    int* srcSizePtr, int dstCapacity, int const nbSearches, size_t sufficient_len, const limitedOutput_directive limit, int const fullUpdate,
    const dictCtx_directive dict, HCfavor_e favorDecSpeed);

LZ4_FORCE_INLINE int LZ4HC_compress_generic_internal(LZ4HC_CCtx_internal* const ctx, const char * const src,
    char * const dst, int* const srcSizePtr, int const dstCapacity, int cLevel, const limitedOutput_directive limit, const dictCtx_directive dict)
{
	typedef enum { lz4hc, lz4opt } lz4hc_strat_e;
	typedef struct {
		lz4hc_strat_e strat;
		uint32 nbSearches;
		uint32 targetLength;
	} cParams_t;
	static const cParams_t clTable[LZ4HC_CLEVEL_MAX+1] = {
		{ lz4hc,     2, 16 },/* 0, unused */
		{ lz4hc,     2, 16 },/* 1, unused */
		{ lz4hc,     2, 16 },/* 2, unused */
		{ lz4hc,     4, 16 },/* 3 */
		{ lz4hc,     8, 16 },/* 4 */
		{ lz4hc,    16, 16 },/* 5 */
		{ lz4hc,    32, 16 },/* 6 */
		{ lz4hc,    64, 16 },/* 7 */
		{ lz4hc,   128, 16 },/* 8 */
		{ lz4hc,   256, 16 },/* 9 */
		{ lz4opt,   96, 64 },/*10==LZ4HC_CLEVEL_OPT_MIN*/
		{ lz4opt,  512, 128 },/*11 */
		{ lz4opt, 16384, LZ4_OPT_NUM }, /* 12==LZ4HC_CLEVEL_MAX */
	};
	//DEBUGLOG(4, "LZ4HC_compress_generic(%p, %p, %d)", ctx, src, *srcSizePtr);
	if(limit == limitedDestSize && dstCapacity < 1) 
		return 0;      // Impossible to store anything 
	if((uint32)*srcSizePtr > (uint32)LZ4_MAX_INPUT_SIZE) 
		return 0;       // Unsupported input size (too large or negative) 
	ctx->end += *srcSizePtr;
	if(cLevel < 1) cLevel = LZ4HC_CLEVEL_DEFAULT; // note : convention is different from lz4frame, maybe something to review 
	cLevel = MIN(LZ4HC_CLEVEL_MAX, cLevel);
	{   
		cParams_t const cParam = clTable[cLevel];
	    HCfavor_e const favor = ctx->favorDecSpeed ? favorDecompressionSpeed : favorCompressionRatio;
	    int result;
	    if(cParam.strat == lz4hc) {
		    result = LZ4HC_compress_hashChain(ctx, src, dst, srcSizePtr, dstCapacity, cParam.nbSearches, limit, dict);
	    }
	    else {
		    assert(cParam.strat == lz4opt);
		    result = LZ4HC_compress_optimal(ctx, src, dst, srcSizePtr, dstCapacity, cParam.nbSearches, cParam.targetLength, limit,
			    cLevel == LZ4HC_CLEVEL_MAX/* ultra mode */, dict, favor);
	    }
	    if(result <= 0) 
			ctx->dirty = 1;
	    return result;
	}
}

static void LZ4HC_setExternalDict(LZ4HC_CCtx_internal* ctxPtr, const uint8 * newBlock);

static int LZ4HC_compress_generic_noDictCtx(LZ4HC_CCtx_internal* const ctx, const char * const src, char * const dst,
    int* const srcSizePtr, int const dstCapacity, int cLevel, limitedOutput_directive limit)
{
	assert(ctx->dictCtx == NULL);
	return LZ4HC_compress_generic_internal(ctx, src, dst, srcSizePtr, dstCapacity, cLevel, limit, noDictCtx);
}

static int LZ4HC_compress_generic_dictCtx(LZ4HC_CCtx_internal* const ctx, const char * const src, char * const dst, 
	int* const srcSizePtr, int const dstCapacity, int cLevel, limitedOutput_directive limit)
{
	const size_t position = ctx->end - ctx->base - ctx->lowLimit;
	assert(ctx->dictCtx != NULL);
	if(position >= SKILOBYTE(64)) {
		ctx->dictCtx = NULL;
		return LZ4HC_compress_generic_noDictCtx(ctx, src, dst, srcSizePtr, dstCapacity, cLevel, limit);
	}
	else if(position == 0 && *srcSizePtr > SKILOBYTE(4)) {
		memcpy(ctx, ctx->dictCtx, sizeof(LZ4HC_CCtx_internal));
		LZ4HC_setExternalDict(ctx, (const uint8 *)src);
		ctx->compressionLevel = (short)cLevel;
		return LZ4HC_compress_generic_noDictCtx(ctx, src, dst, srcSizePtr, dstCapacity, cLevel, limit);
	}
	else {
		return LZ4HC_compress_generic_internal(ctx, src, dst, srcSizePtr, dstCapacity, cLevel, limit, usingDictCtx);
	}
}

static int LZ4HC_compress_generic(LZ4HC_CCtx_internal* const ctx, const char * const src, char * const dst, int* const srcSizePtr,
    int const dstCapacity, int cLevel, limitedOutput_directive limit)
{
	if(ctx->dictCtx == NULL) {
		return LZ4HC_compress_generic_noDictCtx(ctx, src, dst, srcSizePtr, dstCapacity, cLevel, limit);
	}
	else {
		return LZ4HC_compress_generic_dictCtx(ctx, src, dst, srcSizePtr, dstCapacity, cLevel, limit);
	}
}

int LZ4_sizeofStateHC(void) { return sizeof(LZ4_streamHC_t); }

int LZ4_compress_HC_extStateHC_fastReset(void * state, const char * src, char * dst, int srcSize, int dstCapacity, int compressionLevel)
{
	LZ4HC_CCtx_internal* const ctx = &((LZ4_streamHC_t*)state)->internal_donotuse;
	if(((size_t)(state)&(sizeof(void *)-1)) != 0) 
		return 0; // Error : state is not aligned for pointers (32 or 64 bits) 
	LZ4_resetStreamHC_fast((LZ4_streamHC_t*)state, compressionLevel);
	LZ4HC_init(ctx, (const uint8 *)src);
	if(dstCapacity < LZ4_compressBound(srcSize))
		return LZ4HC_compress_generic(ctx, src, dst, &srcSize, dstCapacity, compressionLevel, limitedOutput);
	else
		return LZ4HC_compress_generic(ctx, src, dst, &srcSize, dstCapacity, compressionLevel, noLimit);
}

int LZ4_compress_HC_extStateHC(void * state, const char * src, char * dst, int srcSize, int dstCapacity, int compressionLevel)
{
	if(((size_t)(state)&(sizeof(void *)-1)) != 0) 
		return 0; // Error : state is not aligned for pointers (32 or 64 bits) 
	LZ4_resetStreamHC((LZ4_streamHC_t*)state, compressionLevel);
	return LZ4_compress_HC_extStateHC_fastReset(state, src, dst, srcSize, dstCapacity, compressionLevel);
}

int LZ4_compress_HC(const char * src, char * dst, int srcSize, int dstCapacity, int compressionLevel)
{
#if defined(LZ4HC_HEAPMODE) && LZ4HC_HEAPMODE==1
	LZ4_streamHC_t* const statePtr = (LZ4_streamHC_t*)SAlloc::M(sizeof(LZ4_streamHC_t));
#else
	LZ4_streamHC_t state;
	LZ4_streamHC_t* const statePtr = &state;
#endif
	int const cSize = LZ4_compress_HC_extStateHC(statePtr, src, dst, srcSize, dstCapacity, compressionLevel);
#if defined(LZ4HC_HEAPMODE) && LZ4HC_HEAPMODE==1
	SAlloc::F(statePtr);
#endif
	return cSize;
}

/* LZ4_compress_HC_destSize() :
 * only compatible with regular HC parser */
int LZ4_compress_HC_destSize(void * LZ4HC_Data, const char * source, char * dest, int* sourceSizePtr, int targetDestSize, int cLevel)
{
	LZ4HC_CCtx_internal* const ctx = &((LZ4_streamHC_t*)LZ4HC_Data)->internal_donotuse;
	LZ4_resetStreamHC((LZ4_streamHC_t*)LZ4HC_Data, cLevel);
	LZ4HC_init(ctx, (const uint8 *)source);
	return LZ4HC_compress_generic(ctx, source, dest, sourceSizePtr, targetDestSize, cLevel, limitedDestSize);
}
// 
// Streaming Functions
// 
/* allocation */
LZ4_streamHC_t* LZ4_createStreamHC(void) 
{
	LZ4_streamHC_t * const LZ4_streamHCPtr = (LZ4_streamHC_t *)SAlloc::M(sizeof(LZ4_streamHC_t));
	if(LZ4_streamHCPtr) 
		LZ4_resetStreamHC(LZ4_streamHCPtr, LZ4HC_CLEVEL_DEFAULT);
	return LZ4_streamHCPtr;
}

int LZ4_freeStreamHC(LZ4_streamHC_t* LZ4_streamHCPtr) 
{
	//DEBUGLOG(4, "LZ4_freeStreamHC(%p)", LZ4_streamHCPtr);
	SAlloc::F(LZ4_streamHCPtr);
	return 0;
}

/* initialization */
void LZ4_resetStreamHC(LZ4_streamHC_t* LZ4_streamHCPtr, int compressionLevel)
{
	LZ4_STATIC_ASSERT(sizeof(LZ4HC_CCtx_internal) <= sizeof(size_t) * LZ4_STREAMHCSIZE_SIZET); // if compilation fails here, LZ4_STREAMHCSIZE must be increased 
	//DEBUGLOG(4, "LZ4_resetStreamHC(%p, %d)", LZ4_streamHCPtr, compressionLevel);
	LZ4_streamHCPtr->internal_donotuse.end = (const uint8 *)(ptrdiff_t)-1;
	LZ4_streamHCPtr->internal_donotuse.base = NULL;
	LZ4_streamHCPtr->internal_donotuse.dictCtx = NULL;
	LZ4_streamHCPtr->internal_donotuse.favorDecSpeed = 0;
	LZ4_streamHCPtr->internal_donotuse.dirty = 0;
	LZ4_setCompressionLevel(LZ4_streamHCPtr, compressionLevel);
}

void LZ4_resetStreamHC_fast(LZ4_streamHC_t* LZ4_streamHCPtr, int compressionLevel)
{
	//DEBUGLOG(4, "LZ4_resetStreamHC_fast(%p, %d)", LZ4_streamHCPtr, compressionLevel);
	if(LZ4_streamHCPtr->internal_donotuse.dirty) {
		LZ4_resetStreamHC(LZ4_streamHCPtr, compressionLevel);
	}
	else {
		LZ4_streamHCPtr->internal_donotuse.end -= (uptrval)LZ4_streamHCPtr->internal_donotuse.base;
		LZ4_streamHCPtr->internal_donotuse.base = NULL;
		LZ4_streamHCPtr->internal_donotuse.dictCtx = NULL;
		LZ4_setCompressionLevel(LZ4_streamHCPtr, compressionLevel);
	}
}

void LZ4_setCompressionLevel(LZ4_streamHC_t* LZ4_streamHCPtr, int compressionLevel)
{
	if(compressionLevel < 1) 
		compressionLevel = LZ4HC_CLEVEL_DEFAULT;
	if(compressionLevel > LZ4HC_CLEVEL_MAX) 
		compressionLevel = LZ4HC_CLEVEL_MAX;
	LZ4_streamHCPtr->internal_donotuse.compressionLevel = (short)compressionLevel;
}

void LZ4_favorDecompressionSpeed(LZ4_streamHC_t* LZ4_streamHCPtr, int favor)
{
	LZ4_streamHCPtr->internal_donotuse.favorDecSpeed = (favor!=0);
}

int LZ4_loadDictHC(LZ4_streamHC_t* LZ4_streamHCPtr, const char * dictionary, int dictSize)
{
	LZ4HC_CCtx_internal* const ctxPtr = &LZ4_streamHCPtr->internal_donotuse;
	//DEBUGLOG(4, "LZ4_loadDictHC(%p, %p, %d)", LZ4_streamHCPtr, dictionary, dictSize);
	if(dictSize > SKILOBYTE(64)) {
		dictionary += dictSize - SKILOBYTE(64);
		dictSize = SKILOBYTE(64);
	}
	LZ4_resetStreamHC(LZ4_streamHCPtr, ctxPtr->compressionLevel);
	LZ4HC_init(ctxPtr, (const uint8 *)dictionary);
	ctxPtr->end = (const uint8 *)dictionary + dictSize;
	if(dictSize >= 4) 
		LZ4HC_Insert(ctxPtr, ctxPtr->end-3);
	return dictSize;
}

void LZ4_attach_HC_dictionary(LZ4_streamHC_t * working_stream, const LZ4_streamHC_t * dictionary_stream) 
{
	working_stream->internal_donotuse.dictCtx = dictionary_stream != NULL ? &(dictionary_stream->internal_donotuse) : NULL;
}

/* compression */

static void LZ4HC_setExternalDict(LZ4HC_CCtx_internal* ctxPtr, const uint8 * newBlock)
{
	//DEBUGLOG(4, "LZ4HC_setExternalDict(%p, %p)", ctxPtr, newBlock);
	if(ctxPtr->end >= ctxPtr->base + ctxPtr->dictLimit + 4)
		LZ4HC_Insert(ctxPtr, ctxPtr->end-3); /* Referencing remaining dictionary content */
	/* Only one memory segment for extDict, so any previous extDict is lost at this stage */
	ctxPtr->lowLimit  = ctxPtr->dictLimit;
	ctxPtr->dictLimit = (uint32)(ctxPtr->end - ctxPtr->base);
	ctxPtr->dictBase  = ctxPtr->base;
	ctxPtr->base = newBlock - ctxPtr->dictLimit;
	ctxPtr->end  = newBlock;
	ctxPtr->nextToUpdate = ctxPtr->dictLimit; /* match referencing will resume from there */
}

static int LZ4_compressHC_continue_generic(LZ4_streamHC_t* LZ4_streamHCPtr, const char * src, char * dst, int* srcSizePtr, int dstCapacity, limitedOutput_directive limit)
{
	LZ4HC_CCtx_internal* const ctxPtr = &LZ4_streamHCPtr->internal_donotuse;
	//DEBUGLOG(4, "LZ4_compressHC_continue_generic(%p, %p, %d)", LZ4_streamHCPtr, src, *srcSizePtr);
	/* auto-init if forgotten */
	if(ctxPtr->base == NULL) 
		LZ4HC_init(ctxPtr, (const uint8 *)src);
	/* Check overflow */
	if((size_t)(ctxPtr->end - ctxPtr->base) > SGIGABYTE(2)) {
		size_t dictSize = (size_t)(ctxPtr->end - ctxPtr->base) - ctxPtr->dictLimit;
		SETMIN(dictSize, SKILOBYTE(64));
		LZ4_loadDictHC(LZ4_streamHCPtr, (const char *)(ctxPtr->end) - dictSize, (int)dictSize);
	}
	/* Check if blocks follow each other */
	if((const uint8 *)src != ctxPtr->end) 
		LZ4HC_setExternalDict(ctxPtr, (const uint8 *)src);
	/* Check overlapping input/dictionary space */
	{   
		const uint8 * sourceEnd = (const uint8 *)src + *srcSizePtr;
	    const uint8 * const dictBegin = ctxPtr->dictBase + ctxPtr->lowLimit;
	    const uint8 * const dictEnd   = ctxPtr->dictBase + ctxPtr->dictLimit;
	    if((sourceEnd > dictBegin) && ((const uint8 *)src < dictEnd)) {
			SETMIN(sourceEnd, dictEnd);
		    ctxPtr->lowLimit = (uint32)(sourceEnd - ctxPtr->dictBase);
		    if(ctxPtr->dictLimit - ctxPtr->lowLimit < 4) 
				ctxPtr->lowLimit = ctxPtr->dictLimit;
	    }
	}
	return LZ4HC_compress_generic(ctxPtr, src, dst, srcSizePtr, dstCapacity, ctxPtr->compressionLevel, limit);
}

int LZ4_compress_HC_continue(LZ4_streamHC_t* LZ4_streamHCPtr, const char * src, char * dst, int srcSize, int dstCapacity)
{
	if(dstCapacity < LZ4_compressBound(srcSize))
		return LZ4_compressHC_continue_generic(LZ4_streamHCPtr, src, dst, &srcSize, dstCapacity, limitedOutput);
	else
		return LZ4_compressHC_continue_generic(LZ4_streamHCPtr, src, dst, &srcSize, dstCapacity, noLimit);
}

int LZ4_compress_HC_continue_destSize(LZ4_streamHC_t* LZ4_streamHCPtr, const char * src, char * dst, int* srcSizePtr, int targetDestSize)
{
	return LZ4_compressHC_continue_generic(LZ4_streamHCPtr, src, dst, srcSizePtr, targetDestSize, limitedDestSize);
}

/* dictionary saving */

int LZ4_saveDictHC(LZ4_streamHC_t* LZ4_streamHCPtr, char * safeBuffer, int dictSize)
{
	LZ4HC_CCtx_internal* const streamPtr = &LZ4_streamHCPtr->internal_donotuse;
	int const prefixSize = (int)(streamPtr->end - (streamPtr->base + streamPtr->dictLimit));
	//DEBUGLOG(4, "LZ4_saveDictHC(%p, %p, %d)", LZ4_streamHCPtr, safeBuffer, dictSize);
	if(dictSize > SKILOBYTE(64)) 
		dictSize = SKILOBYTE(64);
	if(dictSize < 4) 
		dictSize = 0;
	if(dictSize > prefixSize) 
		dictSize = prefixSize;
	memmove(safeBuffer, streamPtr->end - dictSize, dictSize);
	{   
		const uint32 endIndex = (uint32)(streamPtr->end - streamPtr->base);
	    streamPtr->end = (const uint8 *)safeBuffer + dictSize;
	    streamPtr->base = streamPtr->end - endIndex;
	    streamPtr->dictLimit = endIndex - dictSize;
	    streamPtr->lowLimit = endIndex - dictSize;
	    if(streamPtr->nextToUpdate < streamPtr->dictLimit) 
			streamPtr->nextToUpdate = streamPtr->dictLimit; 
	}
	return dictSize;
}

/***********************************
*  Deprecated Functions
***********************************/
/* These functions currently generate deprecation warnings */
/* Deprecated compression functions */
int LZ4_compressHC(const char * src, char * dst, int srcSize) {
	return LZ4_compress_HC(src, dst, srcSize, LZ4_compressBound(srcSize), 0);
}

int LZ4_compressHC_limitedOutput(const char * src, char * dst, int srcSize, int maxDstSize) {
	return LZ4_compress_HC(src, dst, srcSize, maxDstSize, 0);
}

int LZ4_compressHC2(const char * src, char * dst, int srcSize, int cLevel) {
	return LZ4_compress_HC(src, dst, srcSize, LZ4_compressBound(srcSize), cLevel);
}

int LZ4_compressHC2_limitedOutput(const char * src, char * dst, int srcSize, int maxDstSize, int cLevel) {
	return LZ4_compress_HC(src, dst, srcSize, maxDstSize, cLevel);
}

int LZ4_compressHC_withStateHC(void * state, const char * src, char * dst, int srcSize) {
	return LZ4_compress_HC_extStateHC(state, src, dst, srcSize, LZ4_compressBound(srcSize), 0);
}

int LZ4_compressHC_limitedOutput_withStateHC(void * state, const char * src, char * dst, int srcSize, int maxDstSize) {
	return LZ4_compress_HC_extStateHC(state, src, dst, srcSize, maxDstSize, 0);
}

int LZ4_compressHC2_withStateHC(void * state, const char * src, char * dst, int srcSize, int cLevel) {
	return LZ4_compress_HC_extStateHC(state, src, dst, srcSize, LZ4_compressBound(srcSize), cLevel);
}

int LZ4_compressHC2_limitedOutput_withStateHC(void * state, const char * src, char * dst, int srcSize, int maxDstSize, int cLevel) {
	return LZ4_compress_HC_extStateHC(state, src, dst, srcSize, maxDstSize, cLevel);
}

int LZ4_compressHC_continue(LZ4_streamHC_t* ctx, const char * src, char * dst, int srcSize) {
	return LZ4_compress_HC_continue(ctx, src, dst, srcSize, LZ4_compressBound(srcSize));
}

int LZ4_compressHC_limitedOutput_continue(LZ4_streamHC_t* ctx, const char * src, char * dst, int srcSize, int maxDstSize) {
	return LZ4_compress_HC_continue(ctx, src, dst, srcSize, maxDstSize);
}

/* Deprecated streaming functions */
int LZ4_sizeofStreamStateHC(void) {
	return LZ4_STREAMHCSIZE;
}

int LZ4_resetStreamStateHC(void * state, char * inputBuffer)
{
	LZ4HC_CCtx_internal * ctx = &((LZ4_streamHC_t*)state)->internal_donotuse;
	if((((size_t)state) & (sizeof(void *)-1)) != 0) return 1; /* Error : pointer is not aligned for pointer (32 or 64
	                                                            bits) */
	LZ4_resetStreamHC((LZ4_streamHC_t*)state, ((LZ4_streamHC_t*)state)->internal_donotuse.compressionLevel);
	LZ4HC_init(ctx, (const uint8 *)inputBuffer);
	return 0;
}

void * LZ4_createHC(const char * inputBuffer)
{
	LZ4_streamHC_t * hc4 = (LZ4_streamHC_t *)SAlloc::M(sizeof(LZ4_streamHC_t));
	if(hc4 == NULL) 
		return NULL; /* not enough memory */
	LZ4_resetStreamHC(hc4, 0 /* compressionLevel */);
	LZ4HC_init(&hc4->internal_donotuse, (const uint8 *)inputBuffer);
	return hc4;
}

int LZ4_freeHC(void * LZ4HC_Data) 
{
	SAlloc::F(LZ4HC_Data);
	return 0;
}

int LZ4_compressHC2_continue(void * LZ4HC_Data, const char * src, char * dst, int srcSize, int cLevel)
{
	return LZ4HC_compress_generic(&((LZ4_streamHC_t*)LZ4HC_Data)->internal_donotuse, src, dst, &srcSize, 0, cLevel, noLimit);
}

int LZ4_compressHC2_limitedOutput_continue(void * LZ4HC_Data, const char * src, char * dst, int srcSize, int dstCapacity, int cLevel)
{
	return LZ4HC_compress_generic(&((LZ4_streamHC_t*)LZ4HC_Data)->internal_donotuse, src, dst, &srcSize, dstCapacity, cLevel, limitedOutput);
}

char * LZ4_slideInputBufferHC(void * LZ4HC_Data)
{
	LZ4_streamHC_t * ctx = (LZ4_streamHC_t*)LZ4HC_Data;
	const uint8 * bufferStart = ctx->internal_donotuse.base + ctx->internal_donotuse.lowLimit;
	LZ4_resetStreamHC_fast(ctx, ctx->internal_donotuse.compressionLevel);
	/* avoid const char * -> char * conversion warning :( */
	return (char *)(uptrval)bufferStart;
}
// 
// LZ4 Optimal parser (levels 10-12)
// 
typedef struct {
	int price;
	int off;
	int mlen;
	int litlen;
} LZ4HC_optimal_t;
// 
// price in bytes 
// 
LZ4_FORCE_INLINE int LZ4HC_literalsPrice(int const litlen)
{
	int price = litlen;
	if(litlen >= (int)RUN_MASK)
		price += 1 + (litlen-RUN_MASK)/255;
	return price;
}
// 
// requires mlen >= MINMATCH 
// 
LZ4_FORCE_INLINE int LZ4HC_sequencePrice(int litlen, int mlen)
{
	int price = 1 + 2; /* token + 16-bit offset */
	price += LZ4HC_literalsPrice(litlen);
	if(mlen >= (int)(ML_MASK+MINMATCH))
		price += 1 + (mlen-(ML_MASK+MINMATCH))/255;
	return price;
}

typedef struct {
	int off;
	int len;
} LZ4HC_match_t;

LZ4_FORCE_INLINE LZ4HC_match_t LZ4HC_FindLongerMatch(LZ4HC_CCtx_internal* const ctx, const uint8 * ip, const uint8 * const iHighLimit,
    int minLen, int nbSearches, const dictCtx_directive dict, const HCfavor_e favorDecSpeed)
{
	LZ4HC_match_t match = { 0, 0 };
	const uint8 * matchPtr = NULL;
	/* note : LZ4HC_InsertAndGetWiderMatch() is able to modify the starting position of a match (*startpos),
	 * but this won't be the case here, as we define iLowLimit==ip,
	 * so LZ4HC_InsertAndGetWiderMatch() won't be allowed to search past ip */
	int matchLength = LZ4HC_InsertAndGetWiderMatch(ctx, ip, ip, iHighLimit, minLen, &matchPtr, &ip, nbSearches, 1 /*patternAnalysis*/, 1 /*chainSwap*/, dict, favorDecSpeed);
	if(matchLength <= minLen) 
		return match;
	if(favorDecSpeed) {
		if((matchLength>18) & (matchLength<=36)) 
			matchLength = 18; /* favor shortcut */
	}
	match.len = matchLength;
	match.off = (int)(ip-matchPtr);
	return match;
}

static int LZ4HC_compress_optimal(LZ4HC_CCtx_internal* ctx, const char * const source, char * dst, int* srcSizePtr,
    int dstCapacity, int const nbSearches, size_t sufficient_len, const limitedOutput_directive limit, int const fullUpdate,
    const dictCtx_directive dict, const HCfavor_e favorDecSpeed)
{
#define TRAILING_LITERALS 3
	LZ4HC_optimal_t opt[LZ4_OPT_NUM + TRAILING_LITERALS]; /* ~64 KB, which is a bit large for stack... */

	const uint8 * ip = (const uint8 *)source;
	const uint8 * anchor = ip;
	const uint8 * const iend = ip + *srcSizePtr;
	const uint8 * const mflimit = iend - MFLIMIT;
	const uint8 * const matchlimit = iend - LASTLITERALS;
	uint8 * op = (uint8 *)dst;
	uint8 * opSaved = (uint8 *)dst;
	uint8 * oend = op + dstCapacity;
	/* init */
	//DEBUGLOG(5, "LZ4HC_compress_optimal");
	*srcSizePtr = 0;
	if(limit == limitedDestSize) oend -= LASTLITERALS; /* Hack for support LZ4 format restriction */
	if(sufficient_len >= LZ4_OPT_NUM) sufficient_len = LZ4_OPT_NUM-1;
	/* Main Loop */
	assert(ip - anchor < LZ4_MAX_INPUT_SIZE);
	while(ip <= mflimit) {
		int const llen = (int)(ip - anchor);
		int best_mlen, best_off;
		int cur, last_match_pos = 0;
		LZ4HC_match_t const firstMatch = LZ4HC_FindLongerMatch(ctx, ip, matchlimit, MINMATCH-1, nbSearches, dict, favorDecSpeed);
		if(firstMatch.len==0) {
			ip++; 
			continue;
		}
		if((size_t)firstMatch.len > sufficient_len) {
			/* good enough solution : immediate encoding */
			int const firstML = firstMatch.len;
			const uint8 * const matchPos = ip - firstMatch.off;
			opSaved = op;
			if(LZ4HC_encodeSequence(&ip, &op, &anchor, firstML, matchPos, limit, oend) ) // updates ip, op and anchor 
				goto _dest_overflow;
			continue;
		}
		// set prices for first positions (literals) 
		{   
		    for(int rPos = 0; rPos < MINMATCH; rPos++) {
			    int const cost = LZ4HC_literalsPrice(llen + rPos);
			    opt[rPos].mlen = 1;
			    opt[rPos].off = 0;
			    opt[rPos].litlen = llen + rPos;
			    opt[rPos].price = cost;
			    //DEBUGLOG(7, "rPos:%3i => price:%3i (litlen=%i) -- initial setup", rPos, cost, opt[rPos].litlen);
		    }
		}
		// set prices using initial match 
		{   
			int mlen = MINMATCH;
		    int const matchML = firstMatch.len; /* necessarily < sufficient_len < LZ4_OPT_NUM */
		    int const offset = firstMatch.off;
		    assert(matchML < LZ4_OPT_NUM);
		    for(; mlen <= matchML; mlen++) {
			    int const cost = LZ4HC_sequencePrice(llen, mlen);
			    opt[mlen].mlen = mlen;
			    opt[mlen].off = offset;
			    opt[mlen].litlen = llen;
			    opt[mlen].price = cost;
			    //DEBUGLOG(7, "rPos:%3i => price:%3i (matchlen=%i) -- initial setup", mlen, cost, mlen);
		    }
		}
		last_match_pos = firstMatch.len;
		{   
			int addLit;
		    for(addLit = 1; addLit <= TRAILING_LITERALS; addLit++) {
			    opt[last_match_pos+addLit].mlen = 1; /* literal */
			    opt[last_match_pos+addLit].off = 0;
			    opt[last_match_pos+addLit].litlen = addLit;
			    opt[last_match_pos+addLit].price = opt[last_match_pos].price + LZ4HC_literalsPrice(addLit);
			    //DEBUGLOG(7, "rPos:%3i => price:%3i (litlen=%i) -- initial setup", last_match_pos+addLit, opt[last_match_pos+addLit].price, addLit);
		    }
		}
		/* check further positions */
		for(cur = 1; cur < last_match_pos; cur++) {
			const uint8 * const curPtr = ip + cur;
			LZ4HC_match_t newMatch;
			if(curPtr > mflimit) 
				break;
			//DEBUGLOG(7, "rPos:%u[%u] vs [%u]%u", cur, opt[cur].price, opt[cur+1].price, cur+1);
			if(fullUpdate) {
				/* not useful to search here if next position has same (or lower) cost */
				if( (opt[cur+1].price <= opt[cur].price)
				    // in some cases, next position has same cost, but cost rises sharply after, so a small match would still be beneficial 
				    && (opt[cur+MINMATCH].price < opt[cur].price + 3 /*min seq price*/) )
					continue;
			}
			else {
				// not useful to search here if next position has same (or lower) cost 
				if(opt[cur+1].price <= opt[cur].price) 
					continue;
			}
			//DEBUGLOG(7, "search at rPos:%u", cur);
			if(fullUpdate)
				newMatch = LZ4HC_FindLongerMatch(ctx, curPtr, matchlimit, MINMATCH-1, nbSearches, dict, favorDecSpeed);
			else // only test matches of minimum length; slightly faster, but misses a few bytes 
				newMatch = LZ4HC_FindLongerMatch(ctx, curPtr, matchlimit, last_match_pos - cur, nbSearches, dict, favorDecSpeed);
			if(!newMatch.len) 
				continue;
			if(((size_t)newMatch.len > sufficient_len) || (newMatch.len + cur >= LZ4_OPT_NUM)) {
				/* immediate encoding */
				best_mlen = newMatch.len;
				best_off = newMatch.off;
				last_match_pos = cur + 1;
				goto encode;
			}
			/* before match : set price with literals at beginning */
			{   
				int const baseLitlen = opt[cur].litlen;
			    int litlen;
			    for(litlen = 1; litlen < MINMATCH; litlen++) {
				    int const price = opt[cur].price - LZ4HC_literalsPrice(baseLitlen) + LZ4HC_literalsPrice(baseLitlen+litlen);
				    int const pos = cur + litlen;
				    if(price < opt[pos].price) {
					    opt[pos].mlen = 1; /* literal */
					    opt[pos].off = 0;
					    opt[pos].litlen = baseLitlen+litlen;
					    opt[pos].price = price;
					    //DEBUGLOG(7, "rPos:%3i => price:%3i (litlen=%i)", pos, price, opt[pos].litlen);
				    }
			    }
			}
			/* set prices using match at position = cur */
			{   
				int const matchML = newMatch.len;
			    int ml = MINMATCH;
			    assert(cur + newMatch.len < LZ4_OPT_NUM);
			    for(; ml <= matchML; ml++) {
				    int const pos = cur + ml;
				    int const offset = newMatch.off;
				    int price;
				    int ll;
				    //DEBUGLOG(7, "testing price rPos %i (last_match_pos=%i)", pos, last_match_pos);
				    if(opt[cur].mlen == 1) {
					    ll = opt[cur].litlen;
					    price = ((cur > ll) ? opt[cur - ll].price : 0) + LZ4HC_sequencePrice(ll, ml);
				    }
				    else {
					    ll = 0;
					    price = opt[cur].price + LZ4HC_sequencePrice(0, ml);
				    }
				    assert((uint32)favorDecSpeed <= 1);
				    if(pos > last_match_pos+TRAILING_LITERALS || price <= opt[pos].price - (int)favorDecSpeed) {
					    //DEBUGLOG(7, "rPos:%3i => price:%3i (matchlen=%i)", pos, price, ml);
					    assert(pos < LZ4_OPT_NUM);
					    if((ml == matchML) /* last pos of last match */ && (last_match_pos < pos))
						    last_match_pos = pos;
					    opt[pos].mlen = ml;
					    opt[pos].off = offset;
					    opt[pos].litlen = ll;
					    opt[pos].price = price;
				    }
			    }
			}
			/* complete following positions with literals */
			{   
				int addLit;
			    for(addLit = 1; addLit <= TRAILING_LITERALS; addLit++) {
				    opt[last_match_pos+addLit].mlen = 1; /* literal */
				    opt[last_match_pos+addLit].off = 0;
				    opt[last_match_pos+addLit].litlen = addLit;
				    opt[last_match_pos+addLit].price = opt[last_match_pos].price + LZ4HC_literalsPrice(addLit);
				    //DEBUGLOG(7, "rPos:%3i => price:%3i (litlen=%i)", last_match_pos+addLit, opt[last_match_pos+addLit].price, addLit);
			    }
			}
		} /* for (cur = 1; cur <= last_match_pos; cur++) */
		best_mlen = opt[last_match_pos].mlen;
		best_off = opt[last_match_pos].off;
		cur = last_match_pos - best_mlen;
encode: // cur, last_match_pos, best_mlen, best_off must be set 
		assert(cur < LZ4_OPT_NUM);
		assert(last_match_pos >= 1); /* == 1 when only one candidate */
		//DEBUGLOG(6, "reverse traversal, looking for shortest path (last_match_pos=%i)", last_match_pos);
		{   
			int candidate_pos = cur;
		    int selected_matchLength = best_mlen;
		    int selected_offset = best_off;
		    while(1) { /* from end to beginning */
			    int const next_matchLength = opt[candidate_pos].mlen; /* can be 1, means literal */
			    int const next_offset = opt[candidate_pos].off;
			    //DEBUGLOG(7, "pos %i: sequence length %i", candidate_pos, selected_matchLength);
			    opt[candidate_pos].mlen = selected_matchLength;
			    opt[candidate_pos].off = selected_offset;
			    selected_matchLength = next_matchLength;
			    selected_offset = next_offset;
			    if(next_matchLength > candidate_pos) break; /* last match elected, first match to encode */
			    assert(next_matchLength > 0); /* can be 1, means literal */
			    candidate_pos -= next_matchLength;
		    }
		}
		// encode all recorded sequences in order 
		{   
			int rPos = 0; /* relative position (to ip) */
		    while(rPos < last_match_pos) {
			    int const ml = opt[rPos].mlen;
			    int const offset = opt[rPos].off;
			    if(ml == 1) {
				    ip++; 
					rPos++; 
					continue;
			    } // literal; note: can end up with several literals, in which case, skip them 
			    rPos += ml;
			    assert(ml >= MINMATCH);
			    assert((offset >= 1) && (offset <= MAX_DISTANCE));
			    opSaved = op;
			    if(LZ4HC_encodeSequence(&ip, &op, &anchor, ml, ip - offset, limit, oend) ) // updates ip, op and anchor 
				    goto _dest_overflow;
		    }
		}
	} /* while (ip <= mflimit) */
_last_literals:
	/* Encode Last Literals */
	{   
		size_t lastRunSize = (size_t)(iend - anchor); /* literals */
	    size_t litLength = (lastRunSize + 255 - RUN_MASK) / 255;
	    const size_t totalSize = 1 + litLength + lastRunSize;
	    if(limit == limitedDestSize) oend += LASTLITERALS; /* restore correct value */
	    if(limit && (op + totalSize > oend)) {
		    if(limit == limitedOutput) return 0; /* Check output limit */
		    /* adapt lastRunSize to fill 'dst' */
		    lastRunSize  = (size_t)(oend - op) - 1;
		    litLength = (lastRunSize + 255 - RUN_MASK) / 255;
		    lastRunSize -= litLength;
	    }
	    ip = anchor + lastRunSize;
	    if(lastRunSize >= RUN_MASK) {
		    size_t accumulator = lastRunSize - RUN_MASK;
		    *op++ = (RUN_MASK << ML_BITS);
		    for(; accumulator >= 255; accumulator -= 255) *op++ = 255;
		    *op++ = (uint8)accumulator;
	    }
	    else {
		    *op++ = (uint8)(lastRunSize << ML_BITS);
	    }
	    memcpy(op, anchor, lastRunSize);
	    op += lastRunSize;}
	/* End */
	*srcSizePtr = (int)(((const char *)ip) - source);
	return (int)((char *)op-dst);
_dest_overflow:
	if(limit == limitedDestSize) {
		op = opSaved; /* restore correct out pointer */
		goto _last_literals;
	}
	return 0;
}
