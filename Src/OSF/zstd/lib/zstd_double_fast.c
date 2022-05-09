/*
 * Copyright (c) Yann Collet, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */
#include <zstd-internal.h>
#pragma hdrstop
#include "zstd_compress_internal.h"
#include "zstd_double_fast.h"

void ZSTD_fillDoubleHashTable(ZSTD_matchState_t* ms, void const* end, ZSTD_dictTableLoadMethod_e dtlm)
{
	const ZSTD_compressionParameters* const cParams = &ms->cParams;
	uint32* const hashLarge = ms->hashTable;
	const uint32 hBitsL = cParams->hashLog;
	const uint32 mls = cParams->minMatch;
	uint32* const hashSmall = ms->chainTable;
	const uint32 hBitsS = cParams->chainLog;
	const BYTE* const base = ms->window.base;
	const BYTE* ip = base + ms->nextToUpdate;
	const BYTE* const iend = ((const BYTE*)end) - HASH_READ_SIZE;
	const uint32 fastHashFillStep = 3;

	/* Always insert every fastHashFillStep position into the hash tables.
	 * Insert the other positions into the large hash table if their entry
	 * is empty.
	 */
	for(; ip + fastHashFillStep - 1 <= iend; ip += fastHashFillStep) {
		const uint32 curr = (uint32)(ip - base);
		uint32 i;
		for(i = 0; i < fastHashFillStep; ++i) {
			const size_t smHash = ZSTD_hashPtr(ip + i, hBitsS, mls);
			const size_t lgHash = ZSTD_hashPtr(ip + i, hBitsL, 8);
			if(i == 0)
				hashSmall[smHash] = curr + i;
			if(i == 0 || hashLarge[lgHash] == 0)
				hashLarge[lgHash] = curr + i;
			/* Only load extra positions for ZSTD_dtlm_full */
			if(dtlm == ZSTD_dtlm_fast)
				break;
		}
	}
}

FORCE_INLINE_TEMPLATE
size_t ZSTD_compressBlock_doubleFast_noDict_generic(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM],
    void const* src, size_t srcSize, const uint32 mls /* template */)
{
	ZSTD_compressionParameters const* cParams = &ms->cParams;
	uint32* const hashLong = ms->hashTable;
	const uint32 hBitsL = cParams->hashLog;
	uint32* const hashSmall = ms->chainTable;
	const uint32 hBitsS = cParams->chainLog;
	const BYTE* const base = ms->window.base;
	const BYTE* const istart = (const BYTE*)src;
	const BYTE* anchor = istart;
	const uint32 endIndex = (uint32)((size_t)(istart - base) + srcSize);
	/* presumes that, if there is a dictionary, it must be using Attach mode */
	const uint32 prefixLowestIndex = ZSTD_getLowestPrefixIndex(ms, endIndex, cParams->windowLog);
	const BYTE* const prefixLowest = base + prefixLowestIndex;
	const BYTE* const iend = istart + srcSize;
	const BYTE* const ilimit = iend - HASH_READ_SIZE;
	uint32 offset_1 = rep[0], offset_2 = rep[1];
	uint32 offsetSaved = 0;
	size_t mLength;
	uint32 offset;
	uint32 curr;
	/* how many positions to search before increasing step size */
	const size_t kStepIncr = 1 << kSearchStrength;
	/* the position at which to increment the step size if no match is found */
	const BYTE* nextStep;
	size_t step; /* the current step size */
	size_t hl0; /* the long hash at ip */
	size_t hl1; /* the long hash at ip1 */
	uint32 idxl0; /* the long match index for ip */
	uint32 idxl1; /* the long match index for ip1 */
	const BYTE* matchl0; /* the long match for ip */
	const BYTE* matchs0; /* the short match for ip */
	const BYTE* matchl1; /* the long match for ip1 */
	const BYTE* ip = istart; /* the current position */
	const BYTE* ip1; /* the next position */
	DEBUGLOG(5, "ZSTD_compressBlock_doubleFast_noDict_generic");
	/* init */
	ip += ((ip - prefixLowest) == 0);
	{
		const uint32 current = (uint32)(ip - base);
		const uint32 windowLow = ZSTD_getLowestPrefixIndex(ms, current, cParams->windowLog);
		const uint32 maxRep = current - windowLow;
		if(offset_2 > maxRep) offsetSaved = offset_2, offset_2 = 0;
		if(offset_1 > maxRep) offsetSaved = offset_1, offset_1 = 0;
	}
	/* Outer Loop: one iteration per match found and stored */
	while(1) {
		step = 1;
		nextStep = ip + kStepIncr;
		ip1 = ip + step;
		if(ip1 > ilimit) {
			goto _cleanup;
		}
		hl0 = ZSTD_hashPtr(ip, hBitsL, 8);
		idxl0 = hashLong[hl0];
		matchl0 = base + idxl0;
		/* Inner Loop: one iteration per search / position */
		do {
			const size_t hs0 = ZSTD_hashPtr(ip, hBitsS, mls);
			const uint32 idxs0 = hashSmall[hs0];
			curr = (uint32)(ip-base);
			matchs0 = base + idxs0;
			hashLong[hl0] = hashSmall[hs0] = curr; /* update hash tables */
			/* check noDict repcode */
			if((offset_1 > 0) & (MEM_read32(ip+1-offset_1) == MEM_read32(ip+1))) {
				mLength = ZSTD_count(ip+1+4, ip+1+4-offset_1, iend) + 4;
				ip++;
				ZSTD_storeSeq(seqStore, (size_t)(ip-anchor), anchor, iend, REPCODE1_TO_OFFBASE, mLength);
				goto _match_stored;
			}
			hl1 = ZSTD_hashPtr(ip1, hBitsL, 8);
			if(idxl0 > prefixLowestIndex) {
				/* check prefix long match */
				if(MEM_read64(matchl0) == MEM_read64(ip)) {
					mLength = ZSTD_count(ip+8, matchl0+8, iend) + 8;
					offset = (uint32)(ip-matchl0);
					while(((ip>anchor) & (matchl0>prefixLowest)) && (ip[-1] == matchl0[-1])) {
						ip--; matchl0--; mLength++;
					} /* catch up */
					goto _match_found;
				}
			}
			idxl1 = hashLong[hl1];
			matchl1 = base + idxl1;
			if(idxs0 > prefixLowestIndex) {
				/* check prefix short match */
				if(MEM_read32(matchs0) == MEM_read32(ip)) {
					goto _search_next_long;
				}
			}
			if(ip1 >= nextStep) {
				PREFETCH_L1(ip1 + 64);
				PREFETCH_L1(ip1 + 128);
				step++;
				nextStep += kStepIncr;
			}
			ip = ip1;
			ip1 += step;

			hl0 = hl1;
			idxl0 = idxl1;
			matchl0 = matchl1;
    #if defined(__aarch64__)
			PREFETCH_L1(ip+256);
    #endif
		} while(ip1 <= ilimit);

_cleanup:
		/* save reps for next block */
		rep[0] = offset_1 ? offset_1 : offsetSaved;
		rep[1] = offset_2 ? offset_2 : offsetSaved;

		/* Return the last literals size */
		return (size_t)(iend - anchor);

_search_next_long:

		/* check prefix long +1 match */
		if(idxl1 > prefixLowestIndex) {
			if(MEM_read64(matchl1) == MEM_read64(ip1)) {
				ip = ip1;
				mLength = ZSTD_count(ip+8, matchl1+8, iend) + 8;
				offset = (uint32)(ip-matchl1);
				while(((ip>anchor) & (matchl1>prefixLowest)) && (ip[-1] == matchl1[-1])) {
					ip--; matchl1--; mLength++;
				} // catch up
				goto _match_found;
			}
		}

		/* if no long +1 match, explore the short match we found */
		mLength = ZSTD_count(ip+4, matchs0+4, iend) + 4;
		offset = (uint32)(ip - matchs0);
		while(((ip>anchor) & (matchs0>prefixLowest)) && (ip[-1] == matchs0[-1])) {
			ip--; matchs0--; mLength++;
		} // catch up
		/* fall-through */
_match_found:   /* requires ip, offset, mLength */
		offset_2 = offset_1;
		offset_1 = offset;
		if(step < 4) {
			/* It is unsafe to write this value back to the hashtable when ip1 is
			 * greater than or equal to the new ip we will have after we're done
			 * processing this match. Rather than perform that test directly
			 * (ip1 >= ip + mLength), which costs speed in practice, we do a simpler
			 * more predictable test. The minmatch even if we take a short match is
			 * 4 bytes, so as long as step, the distance between ip and ip1
			 * (initially) is less than 4, we know ip1 < new ip. */
			hashLong[hl1] = (uint32)(ip1 - base);
		}
		ZSTD_storeSeq(seqStore, (size_t)(ip-anchor), anchor, iend, OFFSET_TO_OFFBASE(offset), mLength);
_match_stored:
		/* match found */
		ip += mLength;
		anchor = ip;
		if(ip <= ilimit) {
			/* Complementary insertion */
			/* done after iLimit test, as candidates could be > iend-8 */
			{   
				const uint32 indexToInsert = curr+2;
			    hashLong[ZSTD_hashPtr(base+indexToInsert, hBitsL, 8)] = indexToInsert;
			    hashLong[ZSTD_hashPtr(ip-2, hBitsL, 8)] = (uint32)(ip-2-base);
			    hashSmall[ZSTD_hashPtr(base+indexToInsert, hBitsS, mls)] = indexToInsert;
			    hashSmall[ZSTD_hashPtr(ip-1, hBitsS, mls)] = (uint32)(ip-1-base);
			}
			/* check immediate repcode */
			while( (ip <= ilimit) && ( (offset_2>0) & (MEM_read32(ip) == MEM_read32(ip - offset_2)) )) {
				/* store sequence */
				const size_t rLength = ZSTD_count(ip+4, ip+4-offset_2, iend) + 4;
				const uint32 tmpOff = offset_2; offset_2 = offset_1; offset_1 = tmpOff; // swap offset_2 <=> offset_1 
				hashSmall[ZSTD_hashPtr(ip, hBitsS, mls)] = (uint32)(ip-base);
				hashLong[ZSTD_hashPtr(ip, hBitsL, 8)] = (uint32)(ip-base);
				ZSTD_storeSeq(seqStore, 0, anchor, iend, REPCODE1_TO_OFFBASE, rLength);
				ip += rLength;
				anchor = ip;
				continue; /* faster when present ... (?) */
			}
		}
	}
}

FORCE_INLINE_TEMPLATE size_t ZSTD_compressBlock_doubleFast_dictMatchState_generic(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM],
    void const* src, size_t srcSize, const uint32 mls /* template */)
{
	ZSTD_compressionParameters const* cParams = &ms->cParams;
	uint32 * hashLong = ms->hashTable;
	const uint32 hBitsL = cParams->hashLog;
	uint32 * hashSmall = ms->chainTable;
	const uint32 hBitsS = cParams->chainLog;
	const BYTE* const base = ms->window.base;
	const BYTE* const istart = (const BYTE*)src;
	const BYTE* ip = istart;
	const BYTE* anchor = istart;
	const uint32 endIndex = (uint32)((size_t)(istart - base) + srcSize);
	/* presumes that, if there is a dictionary, it must be using Attach mode */
	const uint32 prefixLowestIndex = ZSTD_getLowestPrefixIndex(ms, endIndex, cParams->windowLog);
	const BYTE* const prefixLowest = base + prefixLowestIndex;
	const BYTE* const iend = istart + srcSize;
	const BYTE* const ilimit = iend - HASH_READ_SIZE;
	uint32 offset_1 = rep[0], offset_2 = rep[1];
	uint32 offsetSaved = 0;

	const ZSTD_matchState_t* const dms = ms->dictMatchState;
	const ZSTD_compressionParameters* const dictCParams = &dms->cParams;
	const uint32* const dictHashLong  = dms->hashTable;
	const uint32* const dictHashSmall = dms->chainTable;
	const uint32 dictStartIndex       = dms->window.dictLimit;
	const BYTE* const dictBase     = dms->window.base;
	const BYTE* const dictStart    = dictBase + dictStartIndex;
	const BYTE* const dictEnd      = dms->window.nextSrc;
	const uint32 dictIndexDelta       = prefixLowestIndex - (uint32)(dictEnd - dictBase);
	const uint32 dictHBitsL           = dictCParams->hashLog;
	const uint32 dictHBitsS           = dictCParams->chainLog;
	const uint32 dictAndPrefixLength  = (uint32)((ip - prefixLowest) + (dictEnd - dictStart));
	DEBUGLOG(5, "ZSTD_compressBlock_doubleFast_dictMatchState_generic");
	/* if a dictionary is attached, it must be within window range */
	assert(ms->window.dictLimit + (1U << cParams->windowLog) >= endIndex);
	/* init */
	ip += (dictAndPrefixLength == 0);
	/* dictMatchState repCode checks don't currently handle repCode == 0
	 * disabling. */
	assert(offset_1 <= dictAndPrefixLength);
	assert(offset_2 <= dictAndPrefixLength);
	/* Main Search Loop */
	while(ip < ilimit) { /* < instead of <=, because repcode check at (ip+1) */
		size_t mLength;
		uint32 offset;
		const size_t h2 = ZSTD_hashPtr(ip, hBitsL, 8);
		const size_t h = ZSTD_hashPtr(ip, hBitsS, mls);
		const size_t dictHL = ZSTD_hashPtr(ip, dictHBitsL, 8);
		const size_t dictHS = ZSTD_hashPtr(ip, dictHBitsS, mls);
		const uint32 curr = (uint32)(ip-base);
		const uint32 matchIndexL = hashLong[h2];
		uint32 matchIndexS = hashSmall[h];
		const BYTE* matchLong = base + matchIndexL;
		const BYTE* match = base + matchIndexS;
		const uint32 repIndex = curr + 1 - offset_1;
		const BYTE* repMatch = (repIndex < prefixLowestIndex) ? dictBase + (repIndex - dictIndexDelta) : base + repIndex;
		hashLong[h2] = hashSmall[h] = curr; /* update hash tables */
		/* check repcode */
		if(((uint32)((prefixLowestIndex-1) - repIndex) >= 3 /* intentional underflow */) && (MEM_read32(repMatch) == MEM_read32(ip+1)) ) {
			const BYTE* repMatchEnd = repIndex < prefixLowestIndex ? dictEnd : iend;
			mLength = ZSTD_count_2segments(ip+1+4, repMatch+4, iend, repMatchEnd, prefixLowest) + 4;
			ip++;
			ZSTD_storeSeq(seqStore, (size_t)(ip-anchor), anchor, iend, REPCODE1_TO_OFFBASE, mLength);
			goto _match_stored;
		}
		if(matchIndexL > prefixLowestIndex) {
			/* check prefix long match */
			if(MEM_read64(matchLong) == MEM_read64(ip)) {
				mLength = ZSTD_count(ip+8, matchLong+8, iend) + 8;
				offset = (uint32)(ip-matchLong);
				while(((ip>anchor) & (matchLong>prefixLowest)) && (ip[-1] == matchLong[-1])) {
					ip--; matchLong--; mLength++;
				} // catch up
				goto _match_found;
			}
		}
		else {
			/* check dictMatchState long match */
			const uint32 dictMatchIndexL = dictHashLong[dictHL];
			const BYTE* dictMatchL = dictBase + dictMatchIndexL;
			assert(dictMatchL < dictEnd);
			if(dictMatchL > dictStart && MEM_read64(dictMatchL) == MEM_read64(ip)) {
				mLength = ZSTD_count_2segments(ip+8, dictMatchL+8, iend, dictEnd, prefixLowest) + 8;
				offset = (uint32)(curr - dictMatchIndexL - dictIndexDelta);
				while(((ip>anchor) & (dictMatchL>dictStart)) && (ip[-1] == dictMatchL[-1])) {
					ip--; dictMatchL--; mLength++;
				} // catch up
				goto _match_found;
			}
		}
		if(matchIndexS > prefixLowestIndex) {
			/* check prefix short match */
			if(MEM_read32(match) == MEM_read32(ip)) {
				goto _search_next_long;
			}
		}
		else {
			/* check dictMatchState short match */
			const uint32 dictMatchIndexS = dictHashSmall[dictHS];
			match = dictBase + dictMatchIndexS;
			matchIndexS = dictMatchIndexS + dictIndexDelta;
			if(match > dictStart && MEM_read32(match) == MEM_read32(ip)) {
				goto _search_next_long;
			}
		}
		ip += ((ip-anchor) >> kSearchStrength) + 1;
#if defined(__aarch64__)
		PREFETCH_L1(ip+256);
#endif
		continue;
_search_next_long:
		{	
			const size_t hl3 = ZSTD_hashPtr(ip+1, hBitsL, 8);
		    const size_t dictHLNext = ZSTD_hashPtr(ip+1, dictHBitsL, 8);
		    const uint32 matchIndexL3 = hashLong[hl3];
		    const BYTE* matchL3 = base + matchIndexL3;
		    hashLong[hl3] = curr + 1;
			/* check prefix long +1 match */
		    if(matchIndexL3 > prefixLowestIndex) {
			    if(MEM_read64(matchL3) == MEM_read64(ip+1)) {
				    mLength = ZSTD_count(ip+9, matchL3+8, iend) + 8;
				    ip++;
				    offset = (uint32)(ip-matchL3);
				    while(((ip>anchor) & (matchL3>prefixLowest)) && (ip[-1] == matchL3[-1])) {
					    ip--; matchL3--; mLength++;
				    } // catch up
				    goto _match_found;
			    }
		    }
		    else {
			    /* check dict long +1 match */
			    const uint32 dictMatchIndexL3 = dictHashLong[dictHLNext];
			    const BYTE* dictMatchL3 = dictBase + dictMatchIndexL3;
			    assert(dictMatchL3 < dictEnd);
			    if(dictMatchL3 > dictStart && MEM_read64(dictMatchL3) == MEM_read64(ip+1)) {
				    mLength = ZSTD_count_2segments(ip+1+8, dictMatchL3+8, iend, dictEnd, prefixLowest) + 8;
				    ip++;
				    offset = (uint32)(curr + 1 - dictMatchIndexL3 - dictIndexDelta);
				    while(((ip>anchor) & (dictMatchL3>dictStart)) && (ip[-1] == dictMatchL3[-1])) {
					    ip--; dictMatchL3--; mLength++;
				    } // catch up
				    goto _match_found;
			    }
		    }   
		}
		/* if no long +1 match, explore the short match we found */
		if(matchIndexS < prefixLowestIndex) {
			mLength = ZSTD_count_2segments(ip+4, match+4, iend, dictEnd, prefixLowest) + 4;
			offset = (uint32)(curr - matchIndexS);
			while(((ip>anchor) & (match>dictStart)) && (ip[-1] == match[-1])) {
				ip--; match--; mLength++;
			} // catch up
		}
		else {
			mLength = ZSTD_count(ip+4, match+4, iend) + 4;
			offset = (uint32)(ip - match);
			while(((ip>anchor) & (match>prefixLowest)) && (ip[-1] == match[-1])) {
				ip--; match--; mLength++;
			} // catch up 
		}
_match_found:
		offset_2 = offset_1;
		offset_1 = offset;
		ZSTD_storeSeq(seqStore, (size_t)(ip-anchor), anchor, iend, OFFSET_TO_OFFBASE(offset), mLength);
_match_stored:
		/* match found */
		ip += mLength;
		anchor = ip;
		if(ip <= ilimit) {
			/* Complementary insertion */
			/* done after iLimit test, as candidates could be > iend-8 */
			{   
				const uint32 indexToInsert = curr+2;
			    hashLong[ZSTD_hashPtr(base+indexToInsert, hBitsL, 8)] = indexToInsert;
			    hashLong[ZSTD_hashPtr(ip-2, hBitsL, 8)] = (uint32)(ip-2-base);
			    hashSmall[ZSTD_hashPtr(base+indexToInsert, hBitsS, mls)] = indexToInsert;
			    hashSmall[ZSTD_hashPtr(ip-1, hBitsS, mls)] = (uint32)(ip-1-base);
			}
			/* check immediate repcode */
			while(ip <= ilimit) {
				const uint32 current2 = (uint32)(ip-base);
				const uint32 repIndex2 = current2 - offset_2;
				const BYTE* repMatch2 = repIndex2 < prefixLowestIndex ? dictBase + repIndex2 - dictIndexDelta : base + repIndex2;
				if( ((uint32)((prefixLowestIndex-1) - (uint32)repIndex2) >= 3 /* intentional overflow */) && (MEM_read32(repMatch2) == MEM_read32(ip)) ) {
					const BYTE* const repEnd2 = repIndex2 < prefixLowestIndex ? dictEnd : iend;
					const size_t repLength2 = ZSTD_count_2segments(ip+4, repMatch2+4, iend, repEnd2, prefixLowest) + 4;
					uint32 tmpOffset = offset_2; offset_2 = offset_1; offset_1 = tmpOffset; // swap offset_2 <=> offset_1 
					ZSTD_storeSeq(seqStore, 0, anchor, iend, REPCODE1_TO_OFFBASE, repLength2);
					hashSmall[ZSTD_hashPtr(ip, hBitsS, mls)] = current2;
					hashLong[ZSTD_hashPtr(ip, hBitsL, 8)] = current2;
					ip += repLength2;
					anchor = ip;
					continue;
				}
				break;
			}
		}
	} /* while (ip < ilimit) */

	/* save reps for next block */
	rep[0] = offset_1 ? offset_1 : offsetSaved;
	rep[1] = offset_2 ? offset_2 : offsetSaved;

	/* Return the last literals size */
	return (size_t)(iend - anchor);
}

#define ZSTD_GEN_DFAST_FN(dictMode, mls)                                                                 \
	static size_t ZSTD_compressBlock_doubleFast_ ## dictMode ## _ ## mls(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize) \
	{                                                                                                    \
		return ZSTD_compressBlock_doubleFast_ ## dictMode ## _generic(ms, seqStore, rep, src, srcSize, mls); \
	}

ZSTD_GEN_DFAST_FN(noDict, 4)
ZSTD_GEN_DFAST_FN(noDict, 5)
ZSTD_GEN_DFAST_FN(noDict, 6)
ZSTD_GEN_DFAST_FN(noDict, 7)

ZSTD_GEN_DFAST_FN(dictMatchState, 4)
ZSTD_GEN_DFAST_FN(dictMatchState, 5)
ZSTD_GEN_DFAST_FN(dictMatchState, 6)
ZSTD_GEN_DFAST_FN(dictMatchState, 7)

size_t ZSTD_compressBlock_doubleFast(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM],
    void const* src, size_t srcSize)
{
	const uint32 mls = ms->cParams.minMatch;
	switch(mls)
	{
		default: /* includes case 3 */
		case 4:
		    return ZSTD_compressBlock_doubleFast_noDict_4(ms, seqStore, rep, src, srcSize);
		case 5:
		    return ZSTD_compressBlock_doubleFast_noDict_5(ms, seqStore, rep, src, srcSize);
		case 6:
		    return ZSTD_compressBlock_doubleFast_noDict_6(ms, seqStore, rep, src, srcSize);
		case 7:
		    return ZSTD_compressBlock_doubleFast_noDict_7(ms, seqStore, rep, src, srcSize);
	}
}

size_t ZSTD_compressBlock_doubleFast_dictMatchState(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	const uint32 mls = ms->cParams.minMatch;
	switch(mls) {
		default: /* includes case 3 */
		case 4: return ZSTD_compressBlock_doubleFast_dictMatchState_4(ms, seqStore, rep, src, srcSize);
		case 5: return ZSTD_compressBlock_doubleFast_dictMatchState_5(ms, seqStore, rep, src, srcSize);
		case 6: return ZSTD_compressBlock_doubleFast_dictMatchState_6(ms, seqStore, rep, src, srcSize);
		case 7: return ZSTD_compressBlock_doubleFast_dictMatchState_7(ms, seqStore, rep, src, srcSize);
	}
}

static size_t ZSTD_compressBlock_doubleFast_extDict_generic(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM],
    void const* src, size_t srcSize, const uint32 mls /* template */)
{
	ZSTD_compressionParameters const* cParams = &ms->cParams;
	uint32* const hashLong = ms->hashTable;
	const uint32 hBitsL = cParams->hashLog;
	uint32* const hashSmall = ms->chainTable;
	const uint32 hBitsS = cParams->chainLog;
	const BYTE* const istart = (const BYTE*)src;
	const BYTE* ip = istart;
	const BYTE* anchor = istart;
	const BYTE* const iend = istart + srcSize;
	const BYTE* const ilimit = iend - 8;
	const BYTE* const base = ms->window.base;
	const uint32 endIndex = (uint32)((size_t)(istart - base) + srcSize);
	const uint32 lowLimit = ZSTD_getLowestMatchIndex(ms, endIndex, cParams->windowLog);
	const uint32 dictStartIndex = lowLimit;
	const uint32 dictLimit = ms->window.dictLimit;
	const uint32 prefixStartIndex = (dictLimit > lowLimit) ? dictLimit : lowLimit;
	const BYTE* const prefixStart = base + prefixStartIndex;
	const BYTE* const dictBase = ms->window.dictBase;
	const BYTE* const dictStart = dictBase + dictStartIndex;
	const BYTE* const dictEnd = dictBase + prefixStartIndex;
	uint32 offset_1 = rep[0], offset_2 = rep[1];

	DEBUGLOG(5, "ZSTD_compressBlock_doubleFast_extDict_generic (srcSize=%zu)", srcSize);

	/* if extDict is invalidated due to maxDistance, switch to "regular" variant */
	if(prefixStartIndex == dictStartIndex)
		return ZSTD_compressBlock_doubleFast(ms, seqStore, rep, src, srcSize);

	/* Search Loop */
	while(ip < ilimit) { /* < instead of <=, because (ip+1) */
		const size_t hSmall = ZSTD_hashPtr(ip, hBitsS, mls);
		const uint32 matchIndex = hashSmall[hSmall];
		const BYTE* const matchBase = matchIndex < prefixStartIndex ? dictBase : base;
		const BYTE* match = matchBase + matchIndex;

		const size_t hLong = ZSTD_hashPtr(ip, hBitsL, 8);
		const uint32 matchLongIndex = hashLong[hLong];
		const BYTE* const matchLongBase = matchLongIndex < prefixStartIndex ? dictBase : base;
		const BYTE* matchLong = matchLongBase + matchLongIndex;

		const uint32 curr = (uint32)(ip-base);
		const uint32 repIndex = curr + 1 - offset_1; /* offset_1 expected <= curr +1 */
		const BYTE* const repBase = repIndex < prefixStartIndex ? dictBase : base;
		const BYTE* const repMatch = repBase + repIndex;
		size_t mLength;
		hashSmall[hSmall] = hashLong[hLong] = curr; /* update hash table */

		if((((uint32)((prefixStartIndex-1) - repIndex) >= 3) /* intentional underflow : ensure repIndex doesn't overlap dict + prefix */
		    & (offset_1 <= curr+1 - dictStartIndex)) /* note: we are searching at curr+1 */
		    && (MEM_read32(repMatch) == MEM_read32(ip+1)) ) {
			const BYTE* repMatchEnd = repIndex < prefixStartIndex ? dictEnd : iend;
			mLength = ZSTD_count_2segments(ip+1+4, repMatch+4, iend, repMatchEnd, prefixStart) + 4;
			ip++;
			ZSTD_storeSeq(seqStore, (size_t)(ip-anchor), anchor, iend, REPCODE1_TO_OFFBASE, mLength);
		}
		else {
			if((matchLongIndex > dictStartIndex) && (MEM_read64(matchLong) == MEM_read64(ip))) {
				const BYTE* const matchEnd = matchLongIndex < prefixStartIndex ? dictEnd : iend;
				const BYTE* const lowMatchPtr = matchLongIndex < prefixStartIndex ? dictStart : prefixStart;
				uint32 offset;
				mLength = ZSTD_count_2segments(ip+8, matchLong+8, iend, matchEnd, prefixStart) + 8;
				offset = curr - matchLongIndex;
				while(((ip>anchor) & (matchLong>lowMatchPtr)) && (ip[-1] == matchLong[-1])) {
					ip--; 
					matchLong--; 
					mLength++;
				} // catch up 
				offset_2 = offset_1;
				offset_1 = offset;
				ZSTD_storeSeq(seqStore, (size_t)(ip-anchor), anchor, iend, OFFSET_TO_OFFBASE(offset), mLength);
			}
			else if((matchIndex > dictStartIndex) && (MEM_read32(match) == MEM_read32(ip))) {
				const size_t h3 = ZSTD_hashPtr(ip+1, hBitsL, 8);
				const uint32 matchIndex3 = hashLong[h3];
				const BYTE* const match3Base = matchIndex3 < prefixStartIndex ? dictBase : base;
				const BYTE* match3 = match3Base + matchIndex3;
				uint32 offset;
				hashLong[h3] = curr + 1;
				if( (matchIndex3 > dictStartIndex) && (MEM_read64(match3) == MEM_read64(ip+1)) ) {
					const BYTE* const matchEnd = matchIndex3 < prefixStartIndex ? dictEnd : iend;
					const BYTE* const lowMatchPtr = matchIndex3 < prefixStartIndex ? dictStart : prefixStart;
					mLength = ZSTD_count_2segments(ip+9, match3+8, iend, matchEnd, prefixStart) + 8;
					ip++;
					offset = curr+1 - matchIndex3;
					while(((ip>anchor) & (match3>lowMatchPtr)) && (ip[-1] == match3[-1])) {
						ip--; match3--; mLength++;
					} // catch up 
				}
				else {
					const BYTE* const matchEnd = matchIndex < prefixStartIndex ? dictEnd : iend;
					const BYTE* const lowMatchPtr = matchIndex < prefixStartIndex ? dictStart : prefixStart;
					mLength = ZSTD_count_2segments(ip+4, match+4, iend, matchEnd, prefixStart) + 4;
					offset = curr - matchIndex;
					while(((ip>anchor) & (match>lowMatchPtr)) && (ip[-1] == match[-1])) {
						ip--; match--; mLength++;
					} // catch up
				}
				offset_2 = offset_1;
				offset_1 = offset;
				ZSTD_storeSeq(seqStore, (size_t)(ip-anchor), anchor, iend, OFFSET_TO_OFFBASE(offset), mLength);
			}
			else {
				ip += ((ip-anchor) >> kSearchStrength) + 1;
				continue;
			}
		}
		/* move to next sequence start */
		ip += mLength;
		anchor = ip;
		if(ip <= ilimit) {
			/* Complementary insertion */
			/* done after iLimit test, as candidates could be > iend-8 */
			{   
				const uint32 indexToInsert = curr+2;
			    hashLong[ZSTD_hashPtr(base+indexToInsert, hBitsL, 8)] = indexToInsert;
			    hashLong[ZSTD_hashPtr(ip-2, hBitsL, 8)] = (uint32)(ip-2-base);
			    hashSmall[ZSTD_hashPtr(base+indexToInsert, hBitsS, mls)] = indexToInsert;
			    hashSmall[ZSTD_hashPtr(ip-1, hBitsS, mls)] = (uint32)(ip-1-base);
			}
			/* check immediate repcode */
			while(ip <= ilimit) {
				const uint32 current2 = (uint32)(ip-base);
				const uint32 repIndex2 = current2 - offset_2;
				const BYTE* repMatch2 = repIndex2 < prefixStartIndex ? dictBase + repIndex2 : base + repIndex2;
				if( (((uint32)((prefixStartIndex-1) - repIndex2) >= 3) /* intentional overflow : ensure repIndex2 doesn't overlap dict + prefix */
				    & (offset_2 <= current2 - dictStartIndex)) && (MEM_read32(repMatch2) == MEM_read32(ip)) ) {
					const BYTE* const repEnd2 = repIndex2 < prefixStartIndex ? dictEnd : iend;
					const size_t repLength2 = ZSTD_count_2segments(ip+4, repMatch2+4, iend, repEnd2, prefixStart) + 4;
					const uint32 tmpOffset = offset_2; offset_2 = offset_1; offset_1 = tmpOffset; /* swap offset_2 <=> offset_1 */
					ZSTD_storeSeq(seqStore, 0, anchor, iend, REPCODE1_TO_OFFBASE, repLength2);
					hashSmall[ZSTD_hashPtr(ip, hBitsS, mls)] = current2;
					hashLong[ZSTD_hashPtr(ip, hBitsL, 8)] = current2;
					ip += repLength2;
					anchor = ip;
					continue;
				}
				break;
			}
		}
	}
	/* save reps for next block */
	rep[0] = offset_1;
	rep[1] = offset_2;
	/* Return the last literals size */
	return (size_t)(iend - anchor);
}

ZSTD_GEN_DFAST_FN(extDict, 4)
ZSTD_GEN_DFAST_FN(extDict, 5)
ZSTD_GEN_DFAST_FN(extDict, 6)
ZSTD_GEN_DFAST_FN(extDict, 7)

size_t ZSTD_compressBlock_doubleFast_extDict(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	const uint32 mls = ms->cParams.minMatch;
	switch(mls) {
		default: /* includes case 3 */
		case 4: return ZSTD_compressBlock_doubleFast_extDict_4(ms, seqStore, rep, src, srcSize);
		case 5: return ZSTD_compressBlock_doubleFast_extDict_5(ms, seqStore, rep, src, srcSize);
		case 6: return ZSTD_compressBlock_doubleFast_extDict_6(ms, seqStore, rep, src, srcSize);
		case 7: return ZSTD_compressBlock_doubleFast_extDict_7(ms, seqStore, rep, src, srcSize);
	}
}
