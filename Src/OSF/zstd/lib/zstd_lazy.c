/*
 * Copyright (c) Yann Collet, Facebook, Inc. All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */
#include <zstd-internal.h>
#pragma hdrstop
#include "zstd_compress_internal.h"
#include "zstd_lazy.h"
#include <bits.h> /* ZSTD_countTrailingZeros64 */

/*-*************************************
*  Binary Tree search
***************************************/

static void ZSTD_updateDUBT(ZSTD_matchState_t* ms, const BYTE * ip, const BYTE * iend, uint32 mls)
{
	const ZSTD_compressionParameters* const cParams = &ms->cParams;
	uint32 * const hashTable = ms->hashTable;
	const uint32 hashLog = cParams->hashLog;
	uint32 * const bt = ms->chainTable;
	const uint32 btLog  = cParams->chainLog - 1;
	const uint32 btMask = (1 << btLog) - 1;
	const BYTE * const base = ms->window.base;
	const uint32 target = (uint32)(ip - base);
	uint32 idx = ms->nextToUpdate;
	if(idx != target)
		DEBUGLOG(7, "ZSTD_updateDUBT, from %u to %u (dictLimit:%u)", idx, target, ms->window.dictLimit);
	assert(ip + 8 <= iend); /* condition for ZSTD_hashPtr */
	(void)iend;

	assert(idx >= ms->window.dictLimit); /* condition for valid base+idx */
	for(; idx < target; idx++) {
		const size_t h  = ZSTD_hashPtr(base + idx, hashLog, mls);/* assumption : ip + 8 <= iend */
		const uint32 matchIndex = hashTable[h];

		uint32 *   const nextCandidatePtr = bt + 2*(idx&btMask);
		uint32 *   const sortMarkPtr  = nextCandidatePtr + 1;

		DEBUGLOG(8, "ZSTD_updateDUBT: insert %u", idx);
		hashTable[h] = idx; /* Update Hash Table */
		*nextCandidatePtr = matchIndex; /* update BT like a chain */
		*sortMarkPtr = ZSTD_DUBT_UNSORTED_MARK;
	}
	ms->nextToUpdate = target;
}

/** ZSTD_insertDUBT1() :
 *  sort one already inserted but unsorted position
 *  assumption : curr >= btlow == (curr - btmask)
 *  doesn't fail */
static void ZSTD_insertDUBT1(const ZSTD_matchState_t* ms,
    uint32 curr, const BYTE * inputEnd,
    uint32 nbCompares, uint32 btLow,
    const ZSTD_dictMode_e dictMode)
{
	const ZSTD_compressionParameters* const cParams = &ms->cParams;
	uint32 * const bt = ms->chainTable;
	const uint32 btLog  = cParams->chainLog - 1;
	const uint32 btMask = (1 << btLog) - 1;
	size_t commonLengthSmaller = 0, commonLengthLarger = 0;
	const BYTE * const base = ms->window.base;
	const BYTE * const dictBase = ms->window.dictBase;
	const uint32 dictLimit = ms->window.dictLimit;
	const BYTE * const ip = (curr>=dictLimit) ? base + curr : dictBase + curr;
	const BYTE * const iend = (curr>=dictLimit) ? inputEnd : dictBase + dictLimit;
	const BYTE * const dictEnd = dictBase + dictLimit;
	const BYTE * const prefixStart = base + dictLimit;
	const BYTE * match;
	uint32 * smallerPtr = bt + 2*(curr&btMask);
	uint32 * largerPtr  = smallerPtr + 1;
	uint32 matchIndex = *smallerPtr; /* this candidate is unsorted : next sorted candidate is reached through
	                                 *smallerPtr, while *largerPtr contains previous unsorted candidate (which is
	                                 already saved and can be overwritten) */
	uint32 dummy32; /* to be nullified at the end */
	const uint32 windowValid = ms->window.lowLimit;
	const uint32 maxDistance = 1U << cParams->windowLog;
	const uint32 windowLow = (curr - windowValid > maxDistance) ? curr - maxDistance : windowValid;

	DEBUGLOG(8, "ZSTD_insertDUBT1(%u) (dictLimit=%u, lowLimit=%u)",
	    curr, dictLimit, windowLow);
	assert(curr >= btLow);
	assert(ip < iend); /* condition for ZSTD_count */

	for(; nbCompares && (matchIndex > windowLow); --nbCompares) {
		uint32 * const nextPtr = bt + 2*(matchIndex & btMask);
		size_t matchLength = MIN(commonLengthSmaller, commonLengthLarger); /* guaranteed minimum nb of common
		                                                                      bytes */
		assert(matchIndex < curr);
		/* note : all candidates are now supposed sorted,
		 * but it's still possible to have nextPtr[1] == ZSTD_DUBT_UNSORTED_MARK
		 * when a real index has the same value as ZSTD_DUBT_UNSORTED_MARK */

		if( (dictMode != ZSTD_extDict)
		    || (matchIndex+matchLength >= dictLimit) /* both in current segment*/
		    || (curr < dictLimit) /* both in extDict */) {
			const BYTE * const mBase = ( (dictMode != ZSTD_extDict)
			    || (matchIndex+matchLength >= dictLimit)) ?
			    base : dictBase;
			assert( (matchIndex+matchLength >= dictLimit) /* might be wrong if extDict is incorrectly set to
			                                                 0 */
			    || (curr < dictLimit) );
			match = mBase + matchIndex;
			matchLength += ZSTD_count(ip+matchLength, match+matchLength, iend);
		}
		else {
			match = dictBase + matchIndex;
			matchLength += ZSTD_count_2segments(ip+matchLength, match+matchLength, iend, dictEnd, prefixStart);
			if(matchIndex+matchLength >= dictLimit)
				match = base + matchIndex; /* preparation for next read of match[matchLength] */
		}

		DEBUGLOG(8, "ZSTD_insertDUBT1: comparing %u with %u : found %u common bytes ",
		    curr, matchIndex, (uint32)matchLength);

		if(ip+matchLength == iend) { /* equal : no way to know if inf or sup */
			break; /* drop , to guarantee consistency ; miss a bit of compression, but other solutions can corrupt tree */
		}
		if(match[matchLength] < ip[matchLength]) { /* necessarily within buffer */
			/* match is smaller than current */
			*smallerPtr = matchIndex; /* update smaller idx */
			commonLengthSmaller = matchLength; /* all smaller will now have at least this guaranteed common length */
			if(matchIndex <= btLow) {
				smallerPtr = &dummy32; break;
			}                                              /* beyond tree size, stop searching */
			DEBUGLOG(8, "ZSTD_insertDUBT1: %u (>btLow=%u) is smaller : next => %u",
			    matchIndex, btLow, nextPtr[1]);
			smallerPtr = nextPtr+1;   /* new "candidate" => larger than match, which was smaller than target */
			matchIndex = nextPtr[1];  /* new matchIndex, larger than previous and closer to current */
		}
		else {
			/* match is larger than current */
			*largerPtr = matchIndex;
			commonLengthLarger = matchLength;
			if(matchIndex <= btLow) {
				largerPtr = &dummy32; break;
			} /* beyond tree size, stop searching */
			DEBUGLOG(8, "ZSTD_insertDUBT1: %u (>btLow=%u) is larger => %u", matchIndex, btLow, nextPtr[0]);
			largerPtr = nextPtr;
			matchIndex = nextPtr[0];
		}
	}

	*smallerPtr = *largerPtr = 0;
}

static size_t ZSTD_DUBT_findBetterDictMatch(const ZSTD_matchState_t* ms,
    const BYTE * const ip, const BYTE * const iend,
    size_t* offsetPtr,
    size_t bestLength,
    uint32 nbCompares,
    const uint32 mls,
    const ZSTD_dictMode_e dictMode)
{
	const ZSTD_matchState_t * const dms = ms->dictMatchState;
	const ZSTD_compressionParameters* const dmsCParams = &dms->cParams;
	const uint32 * const dictHashTable = dms->hashTable;
	const uint32 hashLog = dmsCParams->hashLog;
	const size_t h  = ZSTD_hashPtr(ip, hashLog, mls);
	uint32 dictMatchIndex = dictHashTable[h];

	const BYTE * const base = ms->window.base;
	const BYTE * const prefixStart = base + ms->window.dictLimit;
	const uint32 curr = (uint32)(ip-base);
	const BYTE * const dictBase = dms->window.base;
	const BYTE * const dictEnd = dms->window.nextSrc;
	const uint32 dictHighLimit = (uint32)(dms->window.nextSrc - dms->window.base);
	const uint32 dictLowLimit = dms->window.lowLimit;
	const uint32 dictIndexDelta = ms->window.lowLimit - dictHighLimit;

	uint32 *        const dictBt = dms->chainTable;
	const uint32 btLog  = dmsCParams->chainLog - 1;
	const uint32 btMask = (1 << btLog) - 1;
	const uint32 btLow = (btMask >= dictHighLimit - dictLowLimit) ? dictLowLimit : dictHighLimit - btMask;

	size_t commonLengthSmaller = 0, commonLengthLarger = 0;

	(void)dictMode;
	assert(dictMode == ZSTD_dictMatchState);

	for(; nbCompares && (dictMatchIndex > dictLowLimit); --nbCompares) {
		uint32 * const nextPtr = dictBt + 2*(dictMatchIndex & btMask);
		size_t matchLength = MIN(commonLengthSmaller, commonLengthLarger); /* guaranteed minimum nb of common
		                                                                      bytes */
		const BYTE * match = dictBase + dictMatchIndex;
		matchLength += ZSTD_count_2segments(ip+matchLength, match+matchLength, iend, dictEnd, prefixStart);
		if(dictMatchIndex+matchLength >= dictHighLimit)
			match = base + dictMatchIndex + dictIndexDelta; /* to prepare for next usage of
		                                                           match[matchLength] */

		if(matchLength > bestLength) {
			uint32 matchIndex = dictMatchIndex + dictIndexDelta;
			if( (4*(int)(matchLength-bestLength)) >
			    (int)(ZSTD_highbit32(curr-matchIndex+1) - ZSTD_highbit32((uint32)offsetPtr[0]+1)) ) {
				DEBUGLOG(9,
				    "ZSTD_DUBT_findBetterDictMatch(%u) : found better match length %u -> %u and offsetCode %u -> %u (dictMatchIndex %u, matchIndex %u)",
				    curr,
				    (uint32)bestLength,
				    (uint32)matchLength,
				    (uint32)*offsetPtr,
				    OFFSET_TO_OFFBASE(curr - matchIndex),
				    dictMatchIndex,
				    matchIndex);
				bestLength = matchLength, *offsetPtr = OFFSET_TO_OFFBASE(curr - matchIndex);
			}
			if(ip+matchLength == iend) { /* reached end of input : ip[matchLength] is not valid, no way to
				                        know if it's larger or smaller than match */
				break; /* drop, to guarantee consistency (miss a little bit of compression) */
			}
		}

		if(match[matchLength] < ip[matchLength]) {
			if(dictMatchIndex <= btLow) {
				break;
			}                             /* beyond tree size, stop the search */
			commonLengthSmaller = matchLength; /* all smaller will now have at least this guaranteed common
			                                      length */
			dictMatchIndex = nextPtr[1];  /* new matchIndex larger than previous (closer to current) */
		}
		else {
			/* match is larger than current */
			if(dictMatchIndex <= btLow) {
				break;
			}                             /* beyond tree size, stop the search */
			commonLengthLarger = matchLength;
			dictMatchIndex = nextPtr[0];
		}
	}
	if(bestLength >= MINMATCH) {
		const uint32 mIndex = curr - (uint32)OFFBASE_TO_OFFSET(*offsetPtr); (void)mIndex;
		DEBUGLOG(8, "ZSTD_DUBT_findBetterDictMatch(%u) : found match of length %u and offsetCode %u (pos %u)", curr, (uint32)bestLength, (uint32)*offsetPtr, mIndex);
	}
	return bestLength;
}

static size_t ZSTD_DUBT_findBestMatch(ZSTD_matchState_t* ms, const BYTE * const ip, const BYTE * const iend,
    size_t* offBasePtr, const uint32 mls, const ZSTD_dictMode_e dictMode)
{
	const ZSTD_compressionParameters* const cParams = &ms->cParams;
	uint32 *   const hashTable = ms->hashTable;
	const uint32 hashLog = cParams->hashLog;
	const size_t h  = ZSTD_hashPtr(ip, hashLog, mls);
	uint32 matchIndex  = hashTable[h];
	const BYTE * const base = ms->window.base;
	const uint32 curr = (uint32)(ip-base);
	const uint32 windowLow = ZSTD_getLowestMatchIndex(ms, curr, cParams->windowLog);
	uint32 *   const bt = ms->chainTable;
	const uint32 btLog  = cParams->chainLog - 1;
	const uint32 btMask = (1 << btLog) - 1;
	const uint32 btLow = (btMask >= curr) ? 0 : curr - btMask;
	const uint32 unsortLimit = MAX(btLow, windowLow);
	uint32 *         nextCandidate = bt + 2*(matchIndex&btMask);
	uint32 *         unsortedMark = bt + 2*(matchIndex&btMask) + 1;
	uint32 nbCompares = 1U << cParams->searchLog;
	uint32 nbCandidates = nbCompares;
	uint32 previousCandidate = 0;
	DEBUGLOG(7, "ZSTD_DUBT_findBestMatch (%u) ", curr);
	assert(ip <= iend-8); /* required for h calculation */
	assert(dictMode != ZSTD_dedicatedDictSearch);
	/* reach end of unsorted candidates list */
	while( (matchIndex > unsortLimit) && (*unsortedMark == ZSTD_DUBT_UNSORTED_MARK) && (nbCandidates > 1) ) {
		DEBUGLOG(8, "ZSTD_DUBT_findBestMatch: candidate %u is unsorted", matchIndex);
		*unsortedMark = previousCandidate; /* the unsortedMark becomes a reversed chain, to move up back to original position */
		previousCandidate = matchIndex;
		matchIndex = *nextCandidate;
		nextCandidate = bt + 2*(matchIndex&btMask);
		unsortedMark = bt + 2*(matchIndex&btMask) + 1;
		nbCandidates--;
	}
	/* nullify last candidate if it's still unsorted
	 * simplification, detrimental to compression ratio, beneficial for speed */
	if( (matchIndex > unsortLimit) && (*unsortedMark==ZSTD_DUBT_UNSORTED_MARK) ) {
		DEBUGLOG(7, "ZSTD_DUBT_findBestMatch: nullify last unsorted candidate %u", matchIndex);
		*nextCandidate = *unsortedMark = 0;
	}
	/* batch sort stacked candidates */
	matchIndex = previousCandidate;
	while(matchIndex) { /* will end on matchIndex == 0 */
		uint32 * const nextCandidateIdxPtr = bt + 2*(matchIndex&btMask) + 1;
		const uint32 nextCandidateIdx = *nextCandidateIdxPtr;
		ZSTD_insertDUBT1(ms, matchIndex, iend, nbCandidates, unsortLimit, dictMode);
		matchIndex = nextCandidateIdx;
		nbCandidates++;
	}
	/* find longest match */
	{   
		size_t commonLengthSmaller = 0, commonLengthLarger = 0;
	    const BYTE * const dictBase = ms->window.dictBase;
	    const uint32 dictLimit = ms->window.dictLimit;
	    const BYTE * const dictEnd = dictBase + dictLimit;
	    const BYTE * const prefixStart = base + dictLimit;
	    uint32 * smallerPtr = bt + 2*(curr&btMask);
	    uint32 * largerPtr  = bt + 2*(curr&btMask) + 1;
	    uint32 matchEndIdx = curr + 8 + 1;
	    uint32 dummy32; /* to be nullified at the end */
	    size_t bestLength = 0;
	    matchIndex  = hashTable[h];
	    hashTable[h] = curr; /* Update Hash Table */
	    for(; nbCompares && (matchIndex > windowLow); --nbCompares) {
		    uint32 * const nextPtr = bt + 2*(matchIndex & btMask);
		    size_t matchLength = MIN(commonLengthSmaller, commonLengthLarger); /* guaranteed minimum nb of common bytes */
		    const BYTE * match;
		    if((dictMode != ZSTD_extDict) || (matchIndex+matchLength >= dictLimit)) {
			    match = base + matchIndex;
			    matchLength += ZSTD_count(ip+matchLength, match+matchLength, iend);
		    }
		    else {
			    match = dictBase + matchIndex;
			    matchLength += ZSTD_count_2segments(ip+matchLength, match+matchLength, iend, dictEnd, prefixStart);
			    if(matchIndex+matchLength >= dictLimit)
				    match = base + matchIndex; /* to prepare for next usage of match[matchLength] */
		    }
		    if(matchLength > bestLength) {
			    if(matchLength > matchEndIdx - matchIndex)
				    matchEndIdx = matchIndex + (uint32)matchLength;
			    if( (4*(int)(matchLength-bestLength)) > (int)(ZSTD_highbit32(curr - matchIndex + 1) - ZSTD_highbit32((uint32)*offBasePtr)) )
				    bestLength = matchLength, *offBasePtr = OFFSET_TO_OFFBASE(curr - matchIndex);
			    if(ip+matchLength == iend) { /* equal : no way to know if inf or sup */
				    if(dictMode == ZSTD_dictMatchState) {
					    nbCompares = 0; /* in addition to avoiding checking any further in this loop, make sure we skip checking in the dictionary. */
				    }
				    break; /* drop, to guarantee consistency (miss a little bit of compression) */
			    }
		    }
		    if(match[matchLength] < ip[matchLength]) {
			    /* match is smaller than current */
			    *smallerPtr = matchIndex; /* update smaller idx */
			    commonLengthSmaller = matchLength; /* all smaller will now have at least this guaranteed common length */
			    if(matchIndex <= btLow) {
				    smallerPtr = &dummy32; break;
			    }                                              /* beyond tree size, stop the search */
			    smallerPtr = nextPtr+1;   /* new "smaller" => larger of match */
			    matchIndex = nextPtr[1];  /* new matchIndex larger than previous (closer to current) */
		    }
		    else {
			    /* match is larger than current */
			    *largerPtr = matchIndex;
			    commonLengthLarger = matchLength;
			    if(matchIndex <= btLow) {
				    largerPtr = &dummy32; break;
			    } /* beyond tree size, stop the search */
			    largerPtr = nextPtr;
			    matchIndex = nextPtr[0];
		    }
	    }
	    *smallerPtr = *largerPtr = 0;
	    assert(nbCompares <= (1U << ZSTD_SEARCHLOG_MAX)); /* Check we haven't underflowed. */
	    if(dictMode == ZSTD_dictMatchState && nbCompares) {
		    bestLength = ZSTD_DUBT_findBetterDictMatch(ms, ip, iend, offBasePtr, bestLength, nbCompares, mls, dictMode);
	    }
	    assert(matchEndIdx > curr+8); /* ensure nextToUpdate is increased */
	    ms->nextToUpdate = matchEndIdx - 8; /* skip repetitive patterns */
	    if(bestLength >= MINMATCH) {
		    const uint32 mIndex = curr - (uint32)OFFBASE_TO_OFFSET(*offBasePtr); (void)mIndex;
		    DEBUGLOG(8, "ZSTD_DUBT_findBestMatch(%u) : found match of length %u and offsetCode %u (pos %u)",
			curr, (uint32)bestLength, (uint32)*offBasePtr, mIndex);
	    }
	    return bestLength;
	}
}

/** ZSTD_BtFindBestMatch() : Tree updater, providing best match */
FORCE_INLINE_TEMPLATE size_t ZSTD_BtFindBestMatch(ZSTD_matchState_t* ms, const BYTE * const ip, const BYTE * const iLimit,
    size_t* offBasePtr, const uint32 mls /* template */, const ZSTD_dictMode_e dictMode)
{
	DEBUGLOG(7, "ZSTD_BtFindBestMatch");
	if(ip < ms->window.base + ms->nextToUpdate) 
		return 0; /* skipped area */
	ZSTD_updateDUBT(ms, ip, iLimit, mls);
	return ZSTD_DUBT_findBestMatch(ms, ip, iLimit, offBasePtr, mls, dictMode);
}

/***********************************
* Dedicated dict search
***********************************/

void ZSTD_dedicatedDictSearch_lazy_loadDictionary(ZSTD_matchState_t* ms, const BYTE * const ip)
{
	const BYTE * const base = ms->window.base;
	const uint32 target = (uint32)(ip - base);
	uint32 * const hashTable = ms->hashTable;
	uint32 * const chainTable = ms->chainTable;
	const uint32 chainSize = 1 << ms->cParams.chainLog;
	uint32 idx = ms->nextToUpdate;
	const uint32 minChain = chainSize < target - idx ? target - chainSize : idx;
	const uint32 bucketSize = 1 << ZSTD_LAZY_DDSS_BUCKET_LOG;
	const uint32 cacheSize = bucketSize - 1;
	const uint32 chainAttempts = (1 << ms->cParams.searchLog) - cacheSize;
	const uint32 chainLimit = chainAttempts > 255 ? 255 : chainAttempts;

	/* We know the hashtable is oversized by a factor of `bucketSize`.
	 * We are going to temporarily pretend `bucketSize == 1`, keeping only a
	 * single entry. We will use the rest of the space to construct a temporary
	 * chaintable.
	 */
	const uint32 hashLog = ms->cParams.hashLog - ZSTD_LAZY_DDSS_BUCKET_LOG;
	uint32 * const tmpHashTable = hashTable;
	uint32 * const tmpChainTable = hashTable + ((size_t)1 << hashLog);
	const uint32 tmpChainSize = (uint32)((1 << ZSTD_LAZY_DDSS_BUCKET_LOG) - 1) << hashLog;
	const uint32 tmpMinChain = tmpChainSize < target ? target - tmpChainSize : idx;
	uint32 hashIdx;
	assert(ms->cParams.chainLog <= 24);
	assert(ms->cParams.hashLog > ms->cParams.chainLog);
	assert(idx != 0);
	assert(tmpMinChain <= minChain);
	/* fill conventional hash table and conventional chain table */
	for(; idx < target; idx++) {
		const uint32 h = (uint32)ZSTD_hashPtr(base + idx, hashLog, ms->cParams.minMatch);
		if(idx >= tmpMinChain) {
			tmpChainTable[idx - tmpMinChain] = hashTable[h];
		}
		tmpHashTable[h] = idx;
	}

	/* sort chains into ddss chain table */
	{
		uint32 chainPos = 0;
		for(hashIdx = 0; hashIdx < (1U << hashLog); hashIdx++) {
			uint32 count;
			uint32 countBeyondMinChain = 0;
			uint32 i = tmpHashTable[hashIdx];
			for(count = 0; i >= tmpMinChain && count < cacheSize; count++) {
				/* skip through the chain to the first position that won't be
				 * in the hash cache bucket */
				if(i < minChain) {
					countBeyondMinChain++;
				}
				i = tmpChainTable[i - tmpMinChain];
			}
			if(count == cacheSize) {
				for(count = 0; count < chainLimit;) {
					if(i < minChain) {
						if(!i || ++countBeyondMinChain > cacheSize) {
							/* only allow pulling `cacheSize` number of entries
							 * into the cache or chainTable beyond `minChain`,
							 * to replace the entries pulled out of the
							 * chainTable into the cache. This lets us reach
							 * back further without increasing the total number
							 * of entries in the chainTable, guaranteeing the
							 * DDSS chain table will fit into the space
							 * allocated for the regular one. */
							break;
						}
					}
					chainTable[chainPos++] = i;
					count++;
					if(i < tmpMinChain) {
						break;
					}
					i = tmpChainTable[i - tmpMinChain];
				}
			}
			else {
				count = 0;
			}
			if(count) {
				tmpHashTable[hashIdx] = ((chainPos - count) << 8) + count;
			}
			else {
				tmpHashTable[hashIdx] = 0;
			}
		}
		assert(chainPos <= chainSize); /* I believe this is guaranteed... */
	}
	/* move chain pointers into the last entry of each hash bucket */
	for(hashIdx = (1 << hashLog); hashIdx;) {
		const uint32 bucketIdx = --hashIdx << ZSTD_LAZY_DDSS_BUCKET_LOG;
		const uint32 chainPackedPointer = tmpHashTable[hashIdx];
		for(uint32 i = 0; i < cacheSize; i++) {
			hashTable[bucketIdx + i] = 0;
		}
		hashTable[bucketIdx + bucketSize - 1] = chainPackedPointer;
	}
	/* fill the buckets of the hash table */
	for(idx = ms->nextToUpdate; idx < target; idx++) {
		const uint32 h = (uint32)ZSTD_hashPtr(base + idx, hashLog, ms->cParams.minMatch) << ZSTD_LAZY_DDSS_BUCKET_LOG;
		/* Shift hash cache down 1. */
		for(uint32 i = cacheSize - 1; i; i--)
			hashTable[h + i] = hashTable[h + i - 1];
		hashTable[h] = idx;
	}
	ms->nextToUpdate = target;
}

/* Returns the longest match length found in the dedicated dict search structure.
 * If none are longer than the argument ml, then ml will be returned.
 */
FORCE_INLINE_TEMPLATE
size_t ZSTD_dedicatedDictSearch_lazy_search(size_t* offsetPtr, size_t ml, uint32 nbAttempts,
    const ZSTD_matchState_t* const dms,
    const BYTE * const ip, const BYTE * const iLimit,
    const BYTE * const prefixStart, const uint32 curr,
    const uint32 dictLimit, const size_t ddsIdx) {
	const uint32 ddsLowestIndex  = dms->window.dictLimit;
	const BYTE * const ddsBase = dms->window.base;
	const BYTE * const ddsEnd  = dms->window.nextSrc;
	const uint32 ddsSize         = (uint32)(ddsEnd - ddsBase);
	const uint32 ddsIndexDelta   = dictLimit - ddsSize;
	const uint32 bucketSize      = (1 << ZSTD_LAZY_DDSS_BUCKET_LOG);
	const uint32 bucketLimit     = nbAttempts < bucketSize - 1 ? nbAttempts : bucketSize - 1;
	uint32 ddsAttempt;
	uint32 matchIndex;

	for(ddsAttempt = 0; ddsAttempt < bucketSize - 1; ddsAttempt++) {
		PREFETCH_L1(ddsBase + dms->hashTable[ddsIdx + ddsAttempt]);
	}
	{
		const uint32 chainPackedPointer = dms->hashTable[ddsIdx + bucketSize - 1];
		const uint32 chainIndex = chainPackedPointer >> 8;
		PREFETCH_L1(&dms->chainTable[chainIndex]);
	}

	for(ddsAttempt = 0; ddsAttempt < bucketLimit; ddsAttempt++) {
		size_t currentMl = 0;
		const BYTE * match;
		matchIndex = dms->hashTable[ddsIdx + ddsAttempt];
		match = ddsBase + matchIndex;
		if(!matchIndex) {
			return ml;
		}
		/* guaranteed by table construction */
		(void)ddsLowestIndex;
		assert(matchIndex >= ddsLowestIndex);
		assert(match+4 <= ddsEnd);
		if(SMem::Get32(match) == SMem::Get32(ip)) {
			/* assumption : matchIndex <= dictLimit-4 (by table construction) */
			currentMl = ZSTD_count_2segments(ip+4, match+4, iLimit, ddsEnd, prefixStart) + 4;
		}

		/* save best solution */
		if(currentMl > ml) {
			ml = currentMl;
			*offsetPtr = OFFSET_TO_OFFBASE(curr - (matchIndex + ddsIndexDelta));
			if(ip+currentMl == iLimit) {
				/* best possible, avoids read overflow on next attempt */
				return ml;
			}
		}
	}

	{
		const uint32 chainPackedPointer = dms->hashTable[ddsIdx + bucketSize - 1];
		uint32 chainIndex = chainPackedPointer >> 8;
		const uint32 chainLength = chainPackedPointer & 0xFF;
		const uint32 chainAttempts = nbAttempts - ddsAttempt;
		const uint32 chainLimit = chainAttempts > chainLength ? chainLength : chainAttempts;
		uint32 chainAttempt;
		for(chainAttempt = 0; chainAttempt < chainLimit; chainAttempt++) {
			PREFETCH_L1(ddsBase + dms->chainTable[chainIndex + chainAttempt]);
		}
		for(chainAttempt = 0; chainAttempt < chainLimit; chainAttempt++, chainIndex++) {
			size_t currentMl = 0;
			const BYTE * match;
			matchIndex = dms->chainTable[chainIndex];
			match = ddsBase + matchIndex;
			/* guaranteed by table construction */
			assert(matchIndex >= ddsLowestIndex);
			assert(match+4 <= ddsEnd);
			if(SMem::Get32(match) == SMem::Get32(ip)) {
				/* assumption : matchIndex <= dictLimit-4 (by table construction) */
				currentMl = ZSTD_count_2segments(ip+4, match+4, iLimit, ddsEnd, prefixStart) + 4;
			}

			/* save best solution */
			if(currentMl > ml) {
				ml = currentMl;
				*offsetPtr = OFFSET_TO_OFFBASE(curr - (matchIndex + ddsIndexDelta));
				if(ip+currentMl == iLimit) 
					break; // best possible, avoids read overflow on next attempt
			}
		}
	}
	return ml;
}

/* *********************************
*  Hash Chain
***********************************/
#define NEXT_IN_CHAIN(d, mask)   chainTable[(d) & (mask)]

/* Update chains up to ip (excluded)
   Assumption : always within prefix (i.e. not within extDict) */
FORCE_INLINE_TEMPLATE uint32 ZSTD_insertAndFindFirstIndex_internal(ZSTD_matchState_t* ms,
    const ZSTD_compressionParameters* const cParams,
    const BYTE * ip, const uint32 mls)
{
	uint32 * const hashTable  = ms->hashTable;
	const uint32 hashLog = cParams->hashLog;
	uint32 * const chainTable = ms->chainTable;
	const uint32 chainMask = (1 << cParams->chainLog) - 1;
	const BYTE * const base = ms->window.base;
	const uint32 target = (uint32)(ip - base);
	uint32 idx = ms->nextToUpdate;

	while(idx < target) { /* catch up */
		const size_t h = ZSTD_hashPtr(base+idx, hashLog, mls);
		NEXT_IN_CHAIN(idx, chainMask) = hashTable[h];
		hashTable[h] = idx;
		idx++;
	}

	ms->nextToUpdate = target;
	return hashTable[ZSTD_hashPtr(ip, hashLog, mls)];
}

uint32 ZSTD_insertAndFindFirstIndex(ZSTD_matchState_t* ms, const BYTE * ip) {
	const ZSTD_compressionParameters* const cParams = &ms->cParams;
	return ZSTD_insertAndFindFirstIndex_internal(ms, cParams, ip, ms->cParams.minMatch);
}

/* inlining is important to hardwire a hot branch (template emulation) */
FORCE_INLINE_TEMPLATE
size_t ZSTD_HcFindBestMatch(ZSTD_matchState_t* ms,
    const BYTE * const ip, const BYTE * const iLimit,
    size_t* offsetPtr,
    const uint32 mls, const ZSTD_dictMode_e dictMode)
{
	const ZSTD_compressionParameters* const cParams = &ms->cParams;
	uint32 * const chainTable = ms->chainTable;
	const uint32 chainSize = (1 << cParams->chainLog);
	const uint32 chainMask = chainSize-1;
	const BYTE * const base = ms->window.base;
	const BYTE * const dictBase = ms->window.dictBase;
	const uint32 dictLimit = ms->window.dictLimit;
	const BYTE * const prefixStart = base + dictLimit;
	const BYTE * const dictEnd = dictBase + dictLimit;
	const uint32 curr = (uint32)(ip-base);
	const uint32 maxDistance = 1U << cParams->windowLog;
	const uint32 lowestValid = ms->window.lowLimit;
	const uint32 withinMaxDistance = (curr - lowestValid > maxDistance) ? curr - maxDistance : lowestValid;
	const uint32 isDictionary = (ms->loadedDictEnd != 0);
	const uint32 lowLimit = isDictionary ? lowestValid : withinMaxDistance;
	const uint32 minChain = curr > chainSize ? curr - chainSize : 0;
	uint32 nbAttempts = 1U << cParams->searchLog;
	size_t ml = 4-1;

	const ZSTD_matchState_t* const dms = ms->dictMatchState;
	const uint32 ddsHashLog = dictMode == ZSTD_dedicatedDictSearch
	    ? dms->cParams.hashLog - ZSTD_LAZY_DDSS_BUCKET_LOG : 0;
	const size_t ddsIdx = dictMode == ZSTD_dedicatedDictSearch
	    ? ZSTD_hashPtr(ip, ddsHashLog, mls) << ZSTD_LAZY_DDSS_BUCKET_LOG : 0;

	uint32 matchIndex;

	if(dictMode == ZSTD_dedicatedDictSearch) {
		const uint32 * entry = &dms->hashTable[ddsIdx];
		PREFETCH_L1(entry);
	}

	/* HC4 match finder */
	matchIndex = ZSTD_insertAndFindFirstIndex_internal(ms, cParams, ip, mls);

	for(; (matchIndex>=lowLimit) & (nbAttempts>0); nbAttempts--) {
		size_t currentMl = 0;
		if((dictMode != ZSTD_extDict) || matchIndex >= dictLimit) {
			const BYTE * const match = base + matchIndex;
			assert(matchIndex >= dictLimit); /* ensures this is true if dictMode != ZSTD_extDict */
			if(match[ml] == ip[ml]) /* potentially better */
				currentMl = ZSTD_count(ip, match, iLimit);
		}
		else {
			const BYTE * const match = dictBase + matchIndex;
			assert(match+4 <= dictEnd);
			if(SMem::Get32(match) == SMem::Get32(ip)) /* assumption : matchIndex <= dictLimit-4 (by table
				                                   construction) */
				currentMl = ZSTD_count_2segments(ip+4, match+4, iLimit, dictEnd, prefixStart) + 4;
		}

		/* save best solution */
		if(currentMl > ml) {
			ml = currentMl;
			*offsetPtr = OFFSET_TO_OFFBASE(curr - matchIndex);
			if(ip+currentMl == iLimit) break; /* best possible, avoids read overflow on next attempt */
		}

		if(matchIndex <= minChain) break;
		matchIndex = NEXT_IN_CHAIN(matchIndex, chainMask);
	}

	assert(nbAttempts <= (1U << ZSTD_SEARCHLOG_MAX)); /* Check we haven't underflowed. */
	if(dictMode == ZSTD_dedicatedDictSearch) {
		ml = ZSTD_dedicatedDictSearch_lazy_search(offsetPtr, ml, nbAttempts, dms,
			ip, iLimit, prefixStart, curr, dictLimit, ddsIdx);
	}
	else if(dictMode == ZSTD_dictMatchState) {
		const uint32 * const dmsChainTable = dms->chainTable;
		const uint32 dmsChainSize         = (1 << dms->cParams.chainLog);
		const uint32 dmsChainMask         = dmsChainSize - 1;
		const uint32 dmsLowestIndex       = dms->window.dictLimit;
		const BYTE * const dmsBase      = dms->window.base;
		const BYTE * const dmsEnd       = dms->window.nextSrc;
		const uint32 dmsSize              = (uint32)(dmsEnd - dmsBase);
		const uint32 dmsIndexDelta        = dictLimit - dmsSize;
		const uint32 dmsMinChain = dmsSize > dmsChainSize ? dmsSize - dmsChainSize : 0;

		matchIndex = dms->hashTable[ZSTD_hashPtr(ip, dms->cParams.hashLog, mls)];

		for(; (matchIndex>=dmsLowestIndex) & (nbAttempts>0); nbAttempts--) {
			size_t currentMl = 0;
			const BYTE * const match = dmsBase + matchIndex;
			assert(match+4 <= dmsEnd);
			if(SMem::Get32(match) == SMem::Get32(ip)) /* assumption : matchIndex <= dictLimit-4 (by table
				                                   construction) */
				currentMl = ZSTD_count_2segments(ip+4, match+4, iLimit, dmsEnd, prefixStart) + 4;

			/* save best solution */
			if(currentMl > ml) {
				ml = currentMl;
				assert(curr > matchIndex + dmsIndexDelta);
				*offsetPtr = OFFSET_TO_OFFBASE(curr - (matchIndex + dmsIndexDelta));
				if(ip+currentMl == iLimit) break; /* best possible, avoids read overflow on next attempt */
			}

			if(matchIndex <= dmsMinChain) break;

			matchIndex = dmsChainTable[matchIndex & dmsChainMask];
		}
	}

	return ml;
}

/* *********************************
* (SIMD) Row-based matchfinder
***********************************/
/* Constants for row-based hash */
#define ZSTD_ROW_HASH_TAG_OFFSET 16     /* byte offset of hashes in the match state's tagTable from the beginning of a
	                                   row */
#define ZSTD_ROW_HASH_TAG_BITS 8        /* nb bits to use for the tag */
#define ZSTD_ROW_HASH_TAG_MASK ((1u << ZSTD_ROW_HASH_TAG_BITS) - 1)
#define ZSTD_ROW_HASH_MAX_ENTRIES 64    /* absolute maximum number of entries per row, for all configurations */

#define ZSTD_ROW_HASH_CACHE_MASK (ZSTD_ROW_HASH_CACHE_SIZE - 1)

typedef uint64 ZSTD_VecMask;   /* Clarifies when we are interacting with a uint64 representing a mask of matches */

/* ZSTD_VecMask_next():
 * Starting from the LSB, returns the idx of the next non-zero bit.
 * Basically counting the nb of trailing zeroes.
 */
MEM_STATIC uint32 ZSTD_VecMask_next(ZSTD_VecMask val) { return /*ZSTD_countTrailingZeros64*/SBits::Ctz(val); }

/* ZSTD_rotateRight_*():
 * Rotates a bitfield to the right by "count" bits.
 * https://en.wikipedia.org/w/index.php?title=Circular_shift&oldid=991635599#Implementing_circular_shifts
 */
FORCE_INLINE_TEMPLATE uint64 ZSTD_rotateRight_U64(uint64 const value, uint32 count) 
{
	assert(count < 64);
	count &= 0x3F; /* for fickle pattern recognition */
	return (value >> count) | (uint64)(value << ((0U - count) & 0x3F));
}

FORCE_INLINE_TEMPLATE uint32 ZSTD_rotateRight_U32(const uint32 value, uint32 count) 
{
	assert(count < 32);
	count &= 0x1F; /* for fickle pattern recognition */
	return (value >> count) | (uint32)(value << ((0U - count) & 0x1F));
}

FORCE_INLINE_TEMPLATE uint16 ZSTD_rotateRight_U16(uint16 const value, uint32 count) 
{
	assert(count < 16);
	count &= 0x0F; /* for fickle pattern recognition */
	return (value >> count) | (uint16)(value << ((0U - count) & 0x0F));
}

/* ZSTD_row_nextIndex():
 * Returns the next index to insert at within a tagTable row, and updates the "head"
 * value to reflect the update. Essentially cycles backwards from [0, {entries per row})
 */
FORCE_INLINE_TEMPLATE uint32 ZSTD_row_nextIndex(BYTE * const tagRow, const uint32 rowMask) 
{
	const uint32 next = (*tagRow - 1) & rowMask;
	*tagRow = (BYTE)next;
	return next;
}

/* ZSTD_isAligned():
 * Checks that a pointer is aligned to "align" bytes which must be a power of 2.
 */
MEM_STATIC int ZSTD_isAligned(void const* ptr, size_t align) {
	assert((align & (align - 1)) == 0);
	return (((size_t)ptr) & (align - 1)) == 0;
}

/* ZSTD_row_prefetch():
 * Performs prefetching for the hashTable and tagTable at a given row.
 */
FORCE_INLINE_TEMPLATE void ZSTD_row_prefetch(const uint32 * hashTable, uint16 const* tagTable, const uint32 relRow, const uint32 rowLog) {
	PREFETCH_L1(hashTable + relRow);
	if(rowLog >= 5) {
		PREFETCH_L1(hashTable + relRow + 16);
		/* Note: prefetching more of the hash table does not appear to be beneficial for 128-entry rows */
	}
	PREFETCH_L1(tagTable + relRow);
	if(rowLog == 6) {
		PREFETCH_L1(tagTable + relRow + 32);
	}
	assert(rowLog == 4 || rowLog == 5 || rowLog == 6);
	assert(ZSTD_isAligned(hashTable + relRow, 64));             /* prefetched hash row always 64-byte aligned */
	assert(ZSTD_isAligned(tagTable + relRow, (size_t)1 << rowLog)); /* prefetched tagRow sits on correct multiple of
	                                                                   bytes (32,64,128) */
}

/* ZSTD_row_fillHashCache():
 * Fill up the hash cache starting at idx, prefetching up to ZSTD_ROW_HASH_CACHE_SIZE entries,
 * but not beyond iLimit.
 */
FORCE_INLINE_TEMPLATE void ZSTD_row_fillHashCache(ZSTD_matchState_t* ms, const BYTE * base,
    const uint32 rowLog, const uint32 mls,
    uint32 idx, const BYTE * const iLimit)
{
	const uint32 * const hashTable = ms->hashTable;
	uint16 const* const tagTable = ms->tagTable;
	const uint32 hashLog = ms->rowHashLog;
	const uint32 maxElemsToPrefetch = (base + idx) > iLimit ? 0 : (uint32)(iLimit - (base + idx) + 1);
	const uint32 lim = idx + MIN(ZSTD_ROW_HASH_CACHE_SIZE, maxElemsToPrefetch);

	for(; idx < lim; ++idx) {
		const uint32 hash = (uint32)ZSTD_hashPtr(base + idx, hashLog + ZSTD_ROW_HASH_TAG_BITS, mls);
		const uint32 row = (hash >> ZSTD_ROW_HASH_TAG_BITS) << rowLog;
		ZSTD_row_prefetch(hashTable, tagTable, row, rowLog);
		ms->hashCache[idx & ZSTD_ROW_HASH_CACHE_MASK] = hash;
	}

	DEBUGLOG(6, "ZSTD_row_fillHashCache(): [%u %u %u %u %u %u %u %u]", ms->hashCache[0], ms->hashCache[1],
	    ms->hashCache[2], ms->hashCache[3], ms->hashCache[4],
	    ms->hashCache[5], ms->hashCache[6], ms->hashCache[7]);
}

/* ZSTD_row_nextCachedHash():
 * Returns the hash of base + idx, and replaces the hash in the hash cache with the byte at
 * base + idx + ZSTD_ROW_HASH_CACHE_SIZE. Also prefetches the appropriate rows from hashTable and tagTable.
 */
FORCE_INLINE_TEMPLATE uint32 ZSTD_row_nextCachedHash(uint32 * cache, const uint32 * hashTable, uint16 const* tagTable, BYTE const* base,
    uint32 idx, const uint32 hashLog, const uint32 rowLog, const uint32 mls)
{
	const uint32 newHash = (uint32)ZSTD_hashPtr(base+idx+ZSTD_ROW_HASH_CACHE_SIZE, hashLog + ZSTD_ROW_HASH_TAG_BITS, mls);
	const uint32 row = (newHash >> ZSTD_ROW_HASH_TAG_BITS) << rowLog;
	ZSTD_row_prefetch(hashTable, tagTable, row, rowLog);
	{   
		const uint32 hash = cache[idx & ZSTD_ROW_HASH_CACHE_MASK];
	    cache[idx & ZSTD_ROW_HASH_CACHE_MASK] = newHash;
	    return hash;
	}
}

/* ZSTD_row_update_internalImpl():
 * Updates the hash table with positions starting from updateStartIdx until updateEndIdx.
 */
FORCE_INLINE_TEMPLATE void ZSTD_row_update_internalImpl(ZSTD_matchState_t* ms, uint32 updateStartIdx, const uint32 updateEndIdx,
    const uint32 mls, const uint32 rowLog, const uint32 rowMask, const uint32 useCache)
{
	uint32 * const hashTable = ms->hashTable;
	uint16* const tagTable = ms->tagTable;
	const uint32 hashLog = ms->rowHashLog;
	const BYTE * const base = ms->window.base;
	DEBUGLOG(6, "ZSTD_row_update_internalImpl(): updateStartIdx=%u, updateEndIdx=%u", updateStartIdx, updateEndIdx);
	for(; updateStartIdx < updateEndIdx; ++updateStartIdx) {
		const uint32 hash = useCache ? ZSTD_row_nextCachedHash(ms->hashCache, hashTable, tagTable,
			base, updateStartIdx, hashLog, rowLog, mls) : (uint32)ZSTD_hashPtr(base + updateStartIdx, hashLog + ZSTD_ROW_HASH_TAG_BITS, mls);
		const uint32 relRow = (hash >> ZSTD_ROW_HASH_TAG_BITS) << rowLog;
		uint32 * const row = hashTable + relRow;
		BYTE * tagRow = (BYTE *)(tagTable + relRow); // Though tagTable is laid out as a table of uint16, each tag is only 1 byte.
			// Explicit cast allows us to get exact desired position within each row 
		const uint32 pos = ZSTD_row_nextIndex(tagRow, rowMask);
		assert(hash == ZSTD_hashPtr(base + updateStartIdx, hashLog + ZSTD_ROW_HASH_TAG_BITS, mls));
		((BYTE *)tagRow)[pos + ZSTD_ROW_HASH_TAG_OFFSET] = hash & ZSTD_ROW_HASH_TAG_MASK;
		row[pos] = updateStartIdx;
	}
}

/* ZSTD_row_update_internal():
 * Inserts the byte at ip into the appropriate position in the hash table, and updates ms->nextToUpdate.
 * Skips sections of long matches as is necessary.
 */
FORCE_INLINE_TEMPLATE void ZSTD_row_update_internal(ZSTD_matchState_t* ms, const BYTE * ip,
    const uint32 mls, const uint32 rowLog, const uint32 rowMask, const uint32 useCache)
{
	uint32 idx = ms->nextToUpdate;
	const BYTE * const base = ms->window.base;
	const uint32 target = (uint32)(ip - base);
	const uint32 kSkipThreshold = 384;
	const uint32 kMaxMatchStartPositionsToUpdate = 96;
	const uint32 kMaxMatchEndPositionsToUpdate = 32;
	if(useCache) {
		/* Only skip positions when using hash cache, i.e.
		 * if we are loading a dict, don't skip anything.
		 * If we decide to skip, then we only update a set number
		 * of positions at the beginning and end of the match.
		 */
		if(UNLIKELY(target - idx > kSkipThreshold)) {
			const uint32 bound = idx + kMaxMatchStartPositionsToUpdate;
			ZSTD_row_update_internalImpl(ms, idx, bound, mls, rowLog, rowMask, useCache);
			idx = target - kMaxMatchEndPositionsToUpdate;
			ZSTD_row_fillHashCache(ms, base, rowLog, mls, idx, ip+1);
		}
	}
	assert(target >= idx);
	ZSTD_row_update_internalImpl(ms, idx, target, mls, rowLog, rowMask, useCache);
	ms->nextToUpdate = target;
}

/* ZSTD_row_update():
 * External wrapper for ZSTD_row_update_internal(). Used for filling the hashtable during dictionary
 * processing.
 */
void ZSTD_row_update(ZSTD_matchState_t* const ms, const BYTE * ip) 
{
	const uint32 rowLog = BOUNDED(4, ms->cParams.searchLog, 6);
	const uint32 rowMask = (1u << rowLog) - 1;
	const uint32 mls = MIN(ms->cParams.minMatch, 6 /* mls caps out at 6 */);
	DEBUGLOG(5, "ZSTD_row_update(), rowLog=%u", rowLog);
	ZSTD_row_update_internal(ms, ip, mls, rowLog, rowMask, 0 /* don't use cache */);
}

#if defined(ZSTD_ARCH_X86_SSE2)
FORCE_INLINE_TEMPLATE ZSTD_VecMask ZSTD_row_getSSEMask(int nbChunks, const BYTE * const src, const BYTE tag, const uint32 head)
{
	const __m128i comparisonMask = _mm_set1_epi8((char)tag);
	int matches[4] = {0};
	int i;
	assert(nbChunks == 1 || nbChunks == 2 || nbChunks == 4);
	for(i = 0; i<nbChunks; i++) {
		const __m128i chunk = _mm_loadu_si128((const __m128i*)(const void *)(src + 16*i));
		const __m128i equalMask = _mm_cmpeq_epi8(chunk, comparisonMask);
		matches[i] = _mm_movemask_epi8(equalMask);
	}
	if(nbChunks == 1) return ZSTD_rotateRight_U16((uint16)matches[0], head);
	if(nbChunks == 2) return ZSTD_rotateRight_U32((uint32)matches[1] << 16 | (uint32)matches[0], head);
	assert(nbChunks == 4);
	return ZSTD_rotateRight_U64((uint64)matches[3] << 48 | (uint64)matches[2] << 32 | (uint64)matches[1] << 16 | (uint64)matches[0], head);
}

#endif

/* Returns a ZSTD_VecMask (uint32) that has the nth bit set to 1 if the newly-computed "tag" matches
 * the hash at the nth position in a row of the tagTable.
 * Each row is a circular buffer beginning at the value of "head". So we must rotate the "matches" bitfield
 * to match up with the actual layout of the entries within the hashTable */
FORCE_INLINE_TEMPLATE ZSTD_VecMask ZSTD_row_getMatchMask(const BYTE * const tagRow, const BYTE tag, const uint32 head, const uint32 rowEntries)
{
	const BYTE * const src = tagRow + ZSTD_ROW_HASH_TAG_OFFSET;
	assert((rowEntries == 16) || (rowEntries == 32) || rowEntries == 64);
	assert(rowEntries <= ZSTD_ROW_HASH_MAX_ENTRIES);
#if defined(ZSTD_ARCH_X86_SSE2)
	return ZSTD_row_getSSEMask(rowEntries / 16, src, tag, head);
#else /* SW or NEON-LE */

#if defined(ZSTD_ARCH_ARM_NEON)
	/* This NEON path only works for little endian - otherwise use SWAR below */
	if(MEM_isLittleEndian()) {
		if(rowEntries == 16) {
			const uint8x16_t chunk = vld1q_u8(src);
			const uint16x8_t equalMask = vreinterpretq_u16_u8(vceqq_u8(chunk, vdupq_n_u8(tag)));
			const uint16x8_t t0 = vshlq_n_u16(equalMask, 7);
			const uint32x4_t t1 = vreinterpretq_u32_u16(vsriq_n_u16(t0, t0, 14));
			const uint64x2_t t2 = vreinterpretq_u64_u32(vshrq_n_u32(t1, 14));
			const uint8x16_t t3 = vreinterpretq_u8_u64(vsraq_n_u64(t2, t2, 28));
			const uint16 hi = (uint16)vgetq_lane_u8(t3, 8);
			const uint16 lo = (uint16)vgetq_lane_u8(t3, 0);
			return ZSTD_rotateRight_U16((hi << 8) | lo, head);
		}
		else if(rowEntries == 32) {
			const uint16x8x2_t chunk = vld2q_u16((const uint16*)(const void *)src);
			const uint8x16_t chunk0 = vreinterpretq_u8_u16(chunk.val[0]);
			const uint8x16_t chunk1 = vreinterpretq_u8_u16(chunk.val[1]);
			const uint8x16_t equalMask0 = vceqq_u8(chunk0, vdupq_n_u8(tag));
			const uint8x16_t equalMask1 = vceqq_u8(chunk1, vdupq_n_u8(tag));
			const int8x8_t pack0 = vqmovn_s16(vreinterpretq_s16_u8(equalMask0));
			const int8x8_t pack1 = vqmovn_s16(vreinterpretq_s16_u8(equalMask1));
			const uint8x8_t t0 = vreinterpret_u8_s8(pack0);
			const uint8x8_t t1 = vreinterpret_u8_s8(pack1);
			const uint8x8_t t2 = vsri_n_u8(t1, t0, 2);
			const uint8x8x2_t t3 = vuzp_u8(t2, t0);
			const uint8x8_t t4 = vsri_n_u8(t3.val[1], t3.val[0], 4);
			const uint32 matches = vget_lane_u32(vreinterpret_u32_u8(t4), 0);
			return ZSTD_rotateRight_U32(matches, head);
		}
		else { /* rowEntries == 64 */
			const uint8x16x4_t chunk = vld4q_u8(src);
			const uint8x16_t dup = vdupq_n_u8(tag);
			const uint8x16_t cmp0 = vceqq_u8(chunk.val[0], dup);
			const uint8x16_t cmp1 = vceqq_u8(chunk.val[1], dup);
			const uint8x16_t cmp2 = vceqq_u8(chunk.val[2], dup);
			const uint8x16_t cmp3 = vceqq_u8(chunk.val[3], dup);

			const uint8x16_t t0 = vsriq_n_u8(cmp1, cmp0, 1);
			const uint8x16_t t1 = vsriq_n_u8(cmp3, cmp2, 1);
			const uint8x16_t t2 = vsriq_n_u8(t1, t0, 2);
			const uint8x16_t t3 = vsriq_n_u8(t2, t2, 4);
			const uint8x8_t t4 = vshrn_n_u16(vreinterpretq_u16_u8(t3), 4);
			const uint64 matches = vget_lane_u64(vreinterpret_u64_u8(t4), 0);
			return ZSTD_rotateRight_U64(matches, head);
		}
	}
#endif /* ZSTD_ARCH_ARM_NEON */
	/* SWAR */
	{   const size_t chunkSize = sizeof(size_t);
	    const size_t shiftAmount = ((chunkSize * 8) - chunkSize);
	    const size_t xFF = ~((size_t)0);
	    const size_t x01 = xFF / 0xFF;
	    const size_t x80 = x01 << 7;
	    const size_t splatChar = tag * x01;
	    ZSTD_VecMask matches = 0;
	    int i = rowEntries - chunkSize;
	    assert((sizeof(size_t) == 4) || (sizeof(size_t) == 8));
	    if(MEM_isLittleEndian()) { /* runtime check so have two loops */
		    const size_t extractMagic = (xFF / 0x7F) >> chunkSize;
		    do {
			    size_t chunk = SMem::GetSizeT(&src[i]);
			    chunk ^= splatChar;
			    chunk = (((chunk | x80) - x01) | chunk) & x80;
			    matches <<= chunkSize;
			    matches |= (chunk * extractMagic) >> shiftAmount;
			    i -= chunkSize;
		    } while(i >= 0);
	    }
	    else { /* big endian: reverse bits during extraction */
		    const size_t msb = xFF ^ (xFF >> 1);
		    const size_t extractMagic = (msb / 0x1FF) | msb;
		    do {
			    size_t chunk = SMem::GetSizeT(&src[i]);
			    chunk ^= splatChar;
			    chunk = (((chunk | x80) - x01) | chunk) & x80;
			    matches <<= chunkSize;
			    matches |= ((chunk >> 7) * extractMagic) >> shiftAmount;
			    i -= chunkSize;
		    } while(i >= 0);
	    }
	    matches = ~matches;
	    if(rowEntries == 16) {
		    return ZSTD_rotateRight_U16((uint16)matches, head);
	    }
	    else if(rowEntries == 32) {
		    return ZSTD_rotateRight_U32((uint32)matches, head);
	    }
	    else {
		    return ZSTD_rotateRight_U64((uint64)matches, head);
	    }
	}
#endif
}

/* The high-level approach of the SIMD row based match finder is as follows:
 * - Figure out where to insert the new entry:
 *      - Generate a hash from a byte along with an additional 1-byte "short hash". The additional byte is our "tag"
 *      - The hashTable is effectively split into groups or "rows" of 16 or 32 entries of uint32, and the hash determines
 *        which row to insert into.
 *      - Determine the correct position within the row to insert the entry into. Each row of 16 or 32 can
 *        be considered as a circular buffer with a "head" index that resides in the tagTable.
 *      - Also insert the "tag" into the equivalent row and position in the tagTable.
 *          - Note: The tagTable has 17 or 33 1-byte entries per row, due to 16 or 32 tags, and 1 "head" entry.
 *                  The 17 or 33 entry rows are spaced out to occur every 32 or 64 bytes, respectively,
 *                  for alignment/performance reasons, leaving some bytes unused.
 * - Use SIMD to efficiently compare the tags in the tagTable to the 1-byte "short hash" and
 *   generate a bitfield that we can cycle through to check the collisions in the hash table.
 * - Pick the longest match.
 */
FORCE_INLINE_TEMPLATE size_t ZSTD_RowFindBestMatch(ZSTD_matchState_t* ms, const BYTE * const ip, const BYTE * const iLimit,
    size_t* offsetPtr, const uint32 mls, const ZSTD_dictMode_e dictMode, const uint32 rowLog)
{
	uint32 * const hashTable = ms->hashTable;
	uint16* const tagTable = ms->tagTable;
	uint32 * const hashCache = ms->hashCache;
	const uint32 hashLog = ms->rowHashLog;
	const ZSTD_compressionParameters* const cParams = &ms->cParams;
	const BYTE * const base = ms->window.base;
	const BYTE * const dictBase = ms->window.dictBase;
	const uint32 dictLimit = ms->window.dictLimit;
	const BYTE * const prefixStart = base + dictLimit;
	const BYTE * const dictEnd = dictBase + dictLimit;
	const uint32 curr = (uint32)(ip-base);
	const uint32 maxDistance = 1U << cParams->windowLog;
	const uint32 lowestValid = ms->window.lowLimit;
	const uint32 withinMaxDistance = (curr - lowestValid > maxDistance) ? curr - maxDistance : lowestValid;
	const uint32 isDictionary = (ms->loadedDictEnd != 0);
	const uint32 lowLimit = isDictionary ? lowestValid : withinMaxDistance;
	const uint32 rowEntries = (1U << rowLog);
	const uint32 rowMask = rowEntries - 1;
	const uint32 cappedSearchLog = MIN(cParams->searchLog, rowLog); // nb of searches is capped at nb entries per row 
	uint32 nbAttempts = 1U << cappedSearchLog;
	size_t ml = 4-1;
	/* DMS/DDS variables that may be referenced laster */
	const ZSTD_matchState_t* const dms = ms->dictMatchState;
	/* Initialize the following variables to satisfy static analyzer */
	size_t ddsIdx = 0;
	uint32 ddsExtraAttempts = 0; /* cctx hash tables are limited in searches, but allow extra searches into DDS */
	uint32 dmsTag = 0;
	uint32 * dmsRow = NULL;
	BYTE * dmsTagRow = NULL;
	if(dictMode == ZSTD_dedicatedDictSearch) {
		const uint32 ddsHashLog = dms->cParams.hashLog - ZSTD_LAZY_DDSS_BUCKET_LOG;
		{ /* Prefetch DDS hashtable entry */
			ddsIdx = ZSTD_hashPtr(ip, ddsHashLog, mls) << ZSTD_LAZY_DDSS_BUCKET_LOG;
			PREFETCH_L1(&dms->hashTable[ddsIdx]);
		}
		ddsExtraAttempts = cParams->searchLog > rowLog ? 1U << (cParams->searchLog - rowLog) : 0;
	}
	if(dictMode == ZSTD_dictMatchState) {
		/* Prefetch DMS rows */
		uint32 * const dmsHashTable = dms->hashTable;
		uint16* const dmsTagTable = dms->tagTable;
		const uint32 dmsHash = (uint32)ZSTD_hashPtr(ip, dms->rowHashLog + ZSTD_ROW_HASH_TAG_BITS, mls);
		const uint32 dmsRelRow = (dmsHash >> ZSTD_ROW_HASH_TAG_BITS) << rowLog;
		dmsTag = dmsHash & ZSTD_ROW_HASH_TAG_MASK;
		dmsTagRow = (BYTE *)(dmsTagTable + dmsRelRow);
		dmsRow = dmsHashTable + dmsRelRow;
		ZSTD_row_prefetch(dmsHashTable, dmsTagTable, dmsRelRow, rowLog);
	}
	/* Update the hashTable and tagTable up to (but not including) ip */
	ZSTD_row_update_internal(ms, ip, mls, rowLog, rowMask, 1 /* useCache */);
	{ /* Get the hash for ip, compute the appropriate row */
		const uint32 hash = ZSTD_row_nextCachedHash(hashCache, hashTable, tagTable, base, curr, hashLog, rowLog, mls);
		const uint32 relRow = (hash >> ZSTD_ROW_HASH_TAG_BITS) << rowLog;
		const uint32 tag = hash & ZSTD_ROW_HASH_TAG_MASK;
		uint32 * const row = hashTable + relRow;
		BYTE * tagRow = (BYTE *)(tagTable + relRow);
		const uint32 head = *tagRow & rowMask;
		uint32 matchBuffer[ZSTD_ROW_HASH_MAX_ENTRIES];
		size_t numMatches = 0;
		size_t currMatch = 0;
		ZSTD_VecMask matches = ZSTD_row_getMatchMask(tagRow, (BYTE)tag, head, rowEntries);
		/* Cycle through the matches and prefetch */
		for(; (matches > 0) && (nbAttempts > 0); --nbAttempts, matches &= (matches - 1)) {
			const uint32 matchPos = (head + ZSTD_VecMask_next(matches)) & rowMask;
			const uint32 matchIndex = row[matchPos];
			assert(numMatches < rowEntries);
			if(matchIndex < lowLimit)
				break;
			if((dictMode != ZSTD_extDict) || matchIndex >= dictLimit) {
				PREFETCH_L1(base + matchIndex);
			}
			else {
				PREFETCH_L1(dictBase + matchIndex);
			}
			matchBuffer[numMatches++] = matchIndex;
		}
		/* Speed opt: insert current byte into hashtable too. This allows us to avoid one iteration of the loop
		   in ZSTD_row_update_internal() at the next search. */
		{
			const uint32 pos = ZSTD_row_nextIndex(tagRow, rowMask);
			tagRow[pos + ZSTD_ROW_HASH_TAG_OFFSET] = (BYTE)tag;
			row[pos] = ms->nextToUpdate++;
		}
		/* Return the longest match */
		for(; currMatch < numMatches; ++currMatch) {
			const uint32 matchIndex = matchBuffer[currMatch];
			size_t currentMl = 0;
			assert(matchIndex < curr);
			assert(matchIndex >= lowLimit);
			if((dictMode != ZSTD_extDict) || matchIndex >= dictLimit) {
				const BYTE * const match = base + matchIndex;
				assert(matchIndex >= dictLimit); /* ensures this is true if dictMode != ZSTD_extDict */
				if(match[ml] == ip[ml]) /* potentially better */
					currentMl = ZSTD_count(ip, match, iLimit);
			}
			else {
				const BYTE * const match = dictBase + matchIndex;
				assert(match+4 <= dictEnd);
				if(SMem::Get32(match) == SMem::Get32(ip)) // assumption : matchIndex <= dictLimit-4 (by table construction) 
					currentMl = ZSTD_count_2segments(ip+4, match+4, iLimit, dictEnd, prefixStart) + 4;
			}

			/* Save best solution */
			if(currentMl > ml) {
				ml = currentMl;
				*offsetPtr = OFFSET_TO_OFFBASE(curr - matchIndex);
				if(ip+currentMl == iLimit) 
					break; // best possible, avoids read overflow on next attempt
			}
		}
	}
	assert(nbAttempts <= (1U << ZSTD_SEARCHLOG_MAX)); /* Check we haven't underflowed. */
	if(dictMode == ZSTD_dedicatedDictSearch) {
		ml = ZSTD_dedicatedDictSearch_lazy_search(offsetPtr, ml, nbAttempts + ddsExtraAttempts, dms, ip, iLimit, prefixStart, curr, dictLimit, ddsIdx);
	}
	else if(dictMode == ZSTD_dictMatchState) {
		/* TODO: Measure and potentially add prefetching to DMS */
		const uint32 dmsLowestIndex       = dms->window.dictLimit;
		const BYTE * const dmsBase      = dms->window.base;
		const BYTE * const dmsEnd       = dms->window.nextSrc;
		const uint32 dmsSize              = (uint32)(dmsEnd - dmsBase);
		const uint32 dmsIndexDelta        = dictLimit - dmsSize;
		{   
			const uint32 head = *dmsTagRow & rowMask;
		    uint32 matchBuffer[ZSTD_ROW_HASH_MAX_ENTRIES];
		    size_t numMatches = 0;
		    size_t currMatch = 0;
		    ZSTD_VecMask matches = ZSTD_row_getMatchMask(dmsTagRow, (BYTE)dmsTag, head, rowEntries);
		    for(; (matches > 0) && (nbAttempts > 0); --nbAttempts, matches &= (matches - 1)) {
			    const uint32 matchPos = (head + ZSTD_VecMask_next(matches)) & rowMask;
			    const uint32 matchIndex = dmsRow[matchPos];
			    if(matchIndex < dmsLowestIndex)
				    break;
			    PREFETCH_L1(dmsBase + matchIndex);
			    matchBuffer[numMatches++] = matchIndex;
		    }
			/* Return the longest match */
		    for(; currMatch < numMatches; ++currMatch) {
			    const uint32 matchIndex = matchBuffer[currMatch];
			    size_t currentMl = 0;
			    assert(matchIndex >= dmsLowestIndex);
			    assert(matchIndex < curr);
			    {   
					const BYTE * const match = dmsBase + matchIndex;
					assert(match+4 <= dmsEnd);
					if(SMem::Get32(match) == SMem::Get32(ip))
						currentMl = ZSTD_count_2segments(ip+4, match+4, iLimit, dmsEnd, prefixStart) + 4; 
				}
			    if(currentMl > ml) {
				    ml = currentMl;
				    assert(curr > matchIndex + dmsIndexDelta);
				    *offsetPtr = OFFSET_TO_OFFBASE(curr - (matchIndex + dmsIndexDelta));
				    if(ip+currentMl == iLimit) 
						break;
			    }
		    }
		}
	}
	return ml;
}

typedef size_t (* searchMax_f)(ZSTD_matchState_t* ms, const BYTE * ip, const BYTE * iLimit, size_t* offsetPtr);

/**
 * This struct contains the functions necessary for lazy to search.
 * Currently, that is only searchMax. However, it is still valuable to have the
 * VTable because this makes it easier to add more functions to the VTable later.
 *
 * TODO: The start of the search function involves loading and calculating a
 * bunch of constants from the ZSTD_matchState_t. These computations could be
 * done in an initialization function, and saved somewhere in the match state.
 * Then we could pass a pointer to the saved state instead of the match state,
 * and avoid duplicate computations.
 *
 * TODO: Move the match re-winding into searchMax. This improves compression
 * ratio, and unlocks further simplifications with the next TODO.
 *
 * TODO: Try moving the repcode search into searchMax. After the re-winding
 * and repcode search are in searchMax, there is no more logic in the match
 * finder loop that requires knowledge about the dictMode. So we should be
 * able to avoid force inlining it, and we can join the extDict loop with
 * the single segment loop. It should go in searchMax instead of its own
 * function to avoid having multiple virtual function calls per search.
 */
typedef struct {
	searchMax_f searchMax;
} ZSTD_LazyVTable;

#define GEN_ZSTD_BT_VTABLE(dictMode, mls)                                             \
	static size_t ZSTD_BtFindBestMatch_ ## dictMode ## _ ## mls(                            \
		ZSTD_matchState_t* ms,                                                    \
		const BYTE * ip, const BYTE * const iLimit,                                 \
		size_t* offBasePtr)                                                       \
	{                                                                                 \
		assert(MAX(4, MIN(6, ms->cParams.minMatch)) == mls);                          \
		return ZSTD_BtFindBestMatch(ms, ip, iLimit, offBasePtr, mls, ZSTD_ ## dictMode); \
	}                                                                                 \
	static const ZSTD_LazyVTable ZSTD_BtVTable_ ## dictMode ## _ ## mls = {                 \
		ZSTD_BtFindBestMatch_ ## dictMode ## _ ## mls                                       \
	};

#define GEN_ZSTD_HC_VTABLE(dictMode, mls)                                             \
	static size_t ZSTD_HcFindBestMatch_ ## dictMode ## _ ## mls(                            \
		ZSTD_matchState_t* ms,                                                    \
		const BYTE * ip, const BYTE * const iLimit,                                 \
		size_t* offsetPtr)                                                        \
	{                                                                                 \
		assert(MAX(4, MIN(6, ms->cParams.minMatch)) == mls);                          \
		return ZSTD_HcFindBestMatch(ms, ip, iLimit, offsetPtr, mls, ZSTD_ ## dictMode); \
	}                                                                                 \
	static const ZSTD_LazyVTable ZSTD_HcVTable_ ## dictMode ## _ ## mls = {                 \
		ZSTD_HcFindBestMatch_ ## dictMode ## _ ## mls                                       \
	};

#define GEN_ZSTD_ROW_VTABLE(dictMode, mls, rowLog)                                             \
	static size_t ZSTD_RowFindBestMatch_ ## dictMode ## _ ## mls ## _ ## rowLog(                         \
		ZSTD_matchState_t* ms,                                                             \
		const BYTE * ip, const BYTE * const iLimit,                                          \
		size_t* offsetPtr)                                                                 \
	{                                                                                          \
		assert(MAX(4, MIN(6, ms->cParams.minMatch)) == mls);                                   \
		assert(MAX(4, MIN(6, ms->cParams.searchLog)) == rowLog);                               \
		return ZSTD_RowFindBestMatch(ms, ip, iLimit, offsetPtr, mls, ZSTD_ ## dictMode, rowLog); \
	}                                                                                          \
	static const ZSTD_LazyVTable ZSTD_RowVTable_ ## dictMode ## _ ## mls ## _ ## rowLog = {              \
		ZSTD_RowFindBestMatch_ ## dictMode ## _ ## mls ## _ ## rowLog                                    \
	};

#define ZSTD_FOR_EACH_ROWLOG(X, dictMode, mls) \
	X(dictMode, mls, 4)                        \
	X(dictMode, mls, 5)                        \
	X(dictMode, mls, 6)

#define ZSTD_FOR_EACH_MLS_ROWLOG(X, dictMode) \
	ZSTD_FOR_EACH_ROWLOG(X, dictMode, 4)      \
	ZSTD_FOR_EACH_ROWLOG(X, dictMode, 5)      \
	ZSTD_FOR_EACH_ROWLOG(X, dictMode, 6)

#define ZSTD_FOR_EACH_MLS(X, dictMode) \
	X(dictMode, 4)                     \
	X(dictMode, 5)                     \
	X(dictMode, 6)

#define ZSTD_FOR_EACH_DICT_MODE(X, ...) \
	X(__VA_ARGS__, noDict)              \
	X(__VA_ARGS__, extDict)             \
	X(__VA_ARGS__, dictMatchState)      \
	X(__VA_ARGS__, dedicatedDictSearch)

/* Generate Row VTables for each combination of (dictMode, mls, rowLog) */
ZSTD_FOR_EACH_DICT_MODE(ZSTD_FOR_EACH_MLS_ROWLOG, GEN_ZSTD_ROW_VTABLE)
/* Generate Binary Tree VTables for each combination of (dictMode, mls) */
ZSTD_FOR_EACH_DICT_MODE(ZSTD_FOR_EACH_MLS, GEN_ZSTD_BT_VTABLE)
/* Generate Hash Chain VTables for each combination of (dictMode, mls) */
ZSTD_FOR_EACH_DICT_MODE(ZSTD_FOR_EACH_MLS, GEN_ZSTD_HC_VTABLE)

#define GEN_ZSTD_BT_VTABLE_ARRAY(dictMode) \
	{                                      \
		&ZSTD_BtVTable_ ## dictMode ## _4,     \
		&ZSTD_BtVTable_ ## dictMode ## _5,     \
		&ZSTD_BtVTable_ ## dictMode ## _6      \
	}

#define GEN_ZSTD_HC_VTABLE_ARRAY(dictMode) \
	{                                      \
		&ZSTD_HcVTable_ ## dictMode ## _4,     \
		&ZSTD_HcVTable_ ## dictMode ## _5,     \
		&ZSTD_HcVTable_ ## dictMode ## _6      \
	}

#define GEN_ZSTD_ROW_VTABLE_ARRAY_(dictMode, mls) \
	{                                             \
		&ZSTD_RowVTable_ ## dictMode ## _ ## mls ## _4,   \
		&ZSTD_RowVTable_ ## dictMode ## _ ## mls ## _5,   \
		&ZSTD_RowVTable_ ## dictMode ## _ ## mls ## _6    \
	}

#define GEN_ZSTD_ROW_VTABLE_ARRAY(dictMode)      \
	{                                            \
		GEN_ZSTD_ROW_VTABLE_ARRAY_(dictMode, 4), \
		GEN_ZSTD_ROW_VTABLE_ARRAY_(dictMode, 5), \
		GEN_ZSTD_ROW_VTABLE_ARRAY_(dictMode, 6)  \
	}

#define GEN_ZSTD_VTABLE_ARRAY(X) \
	{                            \
		X(noDict),               \
		X(extDict),              \
		X(dictMatchState),       \
		X(dedicatedDictSearch)   \
	}

/* *******************************
*  Common parser - lazy strategy
*********************************/
typedef enum { search_hashChain = 0, search_binaryTree = 1, search_rowHash = 2 } searchMethod_e;

/**
 * This table is indexed first by the four ZSTD_dictMode_e values, and then
 * by the two searchMethod_e values. NULLs are placed for configurations
 * that should never occur (extDict modes go to the other implementation
 * below and there is no DDSS for binary tree search yet).
 */

static ZSTD_LazyVTable const* ZSTD_selectLazyVTable(ZSTD_matchState_t const* ms, searchMethod_e searchMethod, ZSTD_dictMode_e dictMode)
{
	/* Fill the Hc/Bt VTable arrays with the right functions for the (dictMode, mls) combination. */
	ZSTD_LazyVTable const* const hcVTables[4][3] = GEN_ZSTD_VTABLE_ARRAY(GEN_ZSTD_HC_VTABLE_ARRAY);
	ZSTD_LazyVTable const* const btVTables[4][3] = GEN_ZSTD_VTABLE_ARRAY(GEN_ZSTD_BT_VTABLE_ARRAY);
	/* Fill the Row VTable array with the right functions for the (dictMode, mls, rowLog) combination. */
	ZSTD_LazyVTable const* const rowVTables[4][3][3] = GEN_ZSTD_VTABLE_ARRAY(GEN_ZSTD_ROW_VTABLE_ARRAY);

	const uint32 mls = MAX(4, MIN(6, ms->cParams.minMatch));
	const uint32 rowLog = MAX(4, MIN(6, ms->cParams.searchLog));
	switch(searchMethod) {
		case search_hashChain:
		    return hcVTables[dictMode][mls - 4];
		case search_binaryTree:
		    return btVTables[dictMode][mls - 4];
		case search_rowHash:
		    return rowVTables[dictMode][mls - 4][rowLog - 4];
		default:
		    return NULL;
	}
}

FORCE_INLINE_TEMPLATE size_t ZSTD_compressBlock_lazy_generic(ZSTD_matchState_t* ms, seqStore_t* seqStore,
    uint32 rep[ZSTD_REP_NUM],
    const void * src, size_t srcSize,
    const searchMethod_e searchMethod, const uint32 depth,
    ZSTD_dictMode_e const dictMode)
{
	const BYTE * const istart = PTR8C(src);
	const BYTE * ip = istart;
	const BYTE * anchor = istart;
	const BYTE * const iend = istart + srcSize;
	const BYTE * const ilimit = (searchMethod == search_rowHash) ? iend - 8 - ZSTD_ROW_HASH_CACHE_SIZE : iend - 8;
	const BYTE * const base = ms->window.base;
	const uint32 prefixLowestIndex = ms->window.dictLimit;
	const BYTE * const prefixLowest = base + prefixLowestIndex;

	searchMax_f const searchMax = ZSTD_selectLazyVTable(ms, searchMethod, dictMode)->searchMax;
	uint32 offset_1 = rep[0], offset_2 = rep[1], savedOffset = 0;

	const int isDMS = dictMode == ZSTD_dictMatchState;
	const int isDDS = dictMode == ZSTD_dedicatedDictSearch;
	const int isDxS = isDMS || isDDS;
	const ZSTD_matchState_t* const dms = ms->dictMatchState;
	const uint32 dictLowestIndex      = isDxS ? dms->window.dictLimit : 0;
	const BYTE * const dictBase     = isDxS ? dms->window.base : NULL;
	const BYTE * const dictLowest   = isDxS ? dictBase + dictLowestIndex : NULL;
	const BYTE * const dictEnd      = isDxS ? dms->window.nextSrc : NULL;
	const uint32 dictIndexDelta       = isDxS ?
	    prefixLowestIndex - (uint32)(dictEnd - dictBase) :
	    0;
	const uint32 dictAndPrefixLength = (uint32)((ip - prefixLowest) + (dictEnd - dictLowest));

	assert(searchMax != NULL);

	DEBUGLOG(5, "ZSTD_compressBlock_lazy_generic (dictMode=%u) (searchFunc=%u)", (uint32)dictMode, (uint32)searchMethod);
	ip += (dictAndPrefixLength == 0);
	if(dictMode == ZSTD_noDict) {
		const uint32 curr = (uint32)(ip - base);
		const uint32 windowLow = ZSTD_getLowestPrefixIndex(ms, curr, ms->cParams.windowLog);
		const uint32 maxRep = curr - windowLow;
		if(offset_2 > maxRep) savedOffset = offset_2, offset_2 = 0;
		if(offset_1 > maxRep) savedOffset = offset_1, offset_1 = 0;
	}
	if(isDxS) {
		/* dictMatchState repCode checks don't currently handle repCode == 0
		 * disabling. */
		assert(offset_1 <= dictAndPrefixLength);
		assert(offset_2 <= dictAndPrefixLength);
	}

	if(searchMethod == search_rowHash) {
		const uint32 rowLog = MAX(4, MIN(6, ms->cParams.searchLog));
		ZSTD_row_fillHashCache(ms, base, rowLog,
		    MIN(ms->cParams.minMatch, 6 /* mls caps out at 6 */),
		    ms->nextToUpdate, ilimit);
	}

	/* Match Loop */
#if defined(__GNUC__) && defined(__x86_64__)
	/* I've measured random a 5% speed loss on levels 5 & 6 (greedy) when the
	 * code alignment is perturbed. To fix the instability align the loop on 32-bytes.
	 */
	__asm__ (".p2align 5");
#endif
	while(ip < ilimit) {
		size_t matchLength = 0;
		size_t offBase = REPCODE1_TO_OFFBASE;
		const BYTE * start = ip+1;
		DEBUGLOG(7, "search baseline (depth 0)");

		/* check repCode */
		if(isDxS) {
			const uint32 repIndex = (uint32)(ip - base) + 1 - offset_1;
			const BYTE * repMatch = ((dictMode == ZSTD_dictMatchState || dictMode == ZSTD_dedicatedDictSearch)
			    && repIndex < prefixLowestIndex) ?
			    dictBase + (repIndex - dictIndexDelta) :
			    base + repIndex;
			if(((uint32)((prefixLowestIndex-1) - repIndex) >= 3 /* intentional underflow */)
			    && (SMem::Get32(repMatch) == SMem::Get32(ip+1)) ) {
				const BYTE * repMatchEnd = repIndex < prefixLowestIndex ? dictEnd : iend;
				matchLength = ZSTD_count_2segments(ip+1+4, repMatch+4, iend, repMatchEnd, prefixLowest) + 4;
				if(depth==0) goto _storeSequence;
			}
		}
		if(dictMode == ZSTD_noDict
		    && ((offset_1 > 0) & (SMem::Get32(ip+1-offset_1) == SMem::Get32(ip+1)))) {
			matchLength = ZSTD_count(ip+1+4, ip+1+4-offset_1, iend) + 4;
			if(depth==0) goto _storeSequence;
		}

		/* first search (depth 0) */
		{   size_t offbaseFound = 999999999;
		    const size_t ml2 = searchMax(ms, ip, iend, &offbaseFound);
		    if(ml2 > matchLength)
			    matchLength = ml2, start = ip, offBase = offbaseFound; }

		if(matchLength < 4) {
			ip += ((ip-anchor) >> kSearchStrength) + 1; /* jump faster over incompressible sections */
			continue;
		}

		/* let's try to find a better solution */
		if(depth>=1)
			while(ip<ilimit) {
				DEBUGLOG(7, "search depth 1");
				ip++;
				if((dictMode == ZSTD_noDict) && (offBase) && ((offset_1>0) & (SMem::Get32(ip) == SMem::Get32(ip - offset_1)))) {
					const size_t mlRep = ZSTD_count(ip+4, ip+4-offset_1, iend) + 4;
					int const gain2 = (int)(mlRep * 3);
					int const gain1 = (int)(matchLength*3 - ZSTD_highbit32((uint32)offBase) + 1);
					if((mlRep >= 4) && (gain2 > gain1))
						matchLength = mlRep, offBase = REPCODE1_TO_OFFBASE, start = ip;
				}
				if(isDxS) {
					const uint32 repIndex = (uint32)(ip - base) - offset_1;
					const BYTE * repMatch = repIndex < prefixLowestIndex ? dictBase + (repIndex - dictIndexDelta) : base + repIndex;
					if(((uint32)((prefixLowestIndex-1) - repIndex) >= 3 /* intentional underflow */) && (SMem::Get32(repMatch) == SMem::Get32(ip)) ) {
						const BYTE * repMatchEnd = repIndex < prefixLowestIndex ? dictEnd : iend;
						const size_t mlRep = ZSTD_count_2segments(ip+4, repMatch+4, iend, repMatchEnd, prefixLowest) + 4;
						int const gain2 = (int)(mlRep * 3);
						int const gain1 = (int)(matchLength*3 - ZSTD_highbit32((uint32)offBase) + 1);
						if((mlRep >= 4) && (gain2 > gain1))
							matchLength = mlRep, offBase = REPCODE1_TO_OFFBASE, start = ip;
					}
				}
				{   size_t ofbCandidate = 999999999;
				    const size_t ml2 = searchMax(ms, ip, iend, &ofbCandidate);
				    int const gain2 = (int)(ml2*4 - ZSTD_highbit32((uint32)ofbCandidate)); /* raw approx */
				    int const gain1 = (int)(matchLength*4 - ZSTD_highbit32((uint32)offBase) + 4);
				    if((ml2 >= 4) && (gain2 > gain1)) {
					    matchLength = ml2, offBase = ofbCandidate, start = ip;
					    continue; /* search a better one */
				    }
				}

				/* let's find an even better one */
				if((depth==2) && (ip<ilimit)) {
					DEBUGLOG(7, "search depth 2");
					ip++;
					if( (dictMode == ZSTD_noDict)
					    && (offBase) && ((offset_1>0) & (SMem::Get32(ip) == SMem::Get32(ip - offset_1)))) {
						const size_t mlRep = ZSTD_count(ip+4, ip+4-offset_1, iend) + 4;
						int const gain2 = (int)(mlRep * 4);
						int const gain1 = (int)(matchLength*4 - ZSTD_highbit32((uint32)offBase) + 1);
						if((mlRep >= 4) && (gain2 > gain1))
							matchLength = mlRep, offBase = REPCODE1_TO_OFFBASE, start = ip;
					}
					if(isDxS) {
						const uint32 repIndex = (uint32)(ip - base) - offset_1;
						const BYTE * repMatch = repIndex < prefixLowestIndex ?
						    dictBase + (repIndex - dictIndexDelta) :
						    base + repIndex;
						if(((uint32)((prefixLowestIndex-1) - repIndex) >= 3 /* intentional
						                                                    underflow */            )
						    && (SMem::Get32(repMatch) == SMem::Get32(ip)) ) {
							const BYTE * repMatchEnd = repIndex < prefixLowestIndex ? dictEnd : iend;
							const size_t mlRep = ZSTD_count_2segments(ip+4,
								repMatch+4,
								iend,
								repMatchEnd,
								prefixLowest) + 4;
							int const gain2 = (int)(mlRep * 4);
							int const gain1 = (int)(matchLength*4 - ZSTD_highbit32((uint32)offBase) + 1);
							if((mlRep >= 4) && (gain2 > gain1))
								matchLength = mlRep, offBase = REPCODE1_TO_OFFBASE, start = ip;
						}
					}
					{   size_t ofbCandidate = 999999999;
					    const size_t ml2 = searchMax(ms, ip, iend, &ofbCandidate);
					    int const gain2 = (int)(ml2*4 - ZSTD_highbit32((uint32)ofbCandidate)); /* raw approx */
					    int const gain1 = (int)(matchLength*4 - ZSTD_highbit32((uint32)offBase) + 7);
					    if((ml2 >= 4) && (gain2 > gain1)) {
						    matchLength = ml2, offBase = ofbCandidate, start = ip;
						    continue;
					    }
					}
				}
				break; /* nothing found : store previous solution */
			}

		/* NOTE:
		 * Pay attention that `start[-value]` can lead to strange undefined behavior
		 * notably if `value` is unsigned, resulting in a large positive `-value`.
		 */
		/* catch up */
		if(OFFBASE_IS_OFFSET(offBase)) {
			if(dictMode == ZSTD_noDict) {
				while( ((start > anchor) & (start - OFFBASE_TO_OFFSET(offBase) > prefixLowest))
				    && (start[-1] == (start-OFFBASE_TO_OFFSET(offBase))[-1]) ) { /* only search for offset within prefix */
					start--; matchLength++;
				}
			}
			if(isDxS) {
				const uint32 matchIndex = (uint32)((size_t)(start-base) - OFFBASE_TO_OFFSET(offBase));
				const BYTE * match = (matchIndex < prefixLowestIndex) ? dictBase + matchIndex - dictIndexDelta : base + matchIndex;
				const BYTE * const mStart = (matchIndex < prefixLowestIndex) ? dictLowest : prefixLowest;
				while((start>anchor) && (match>mStart) && (start[-1] == match[-1])) {
					start--; match--; matchLength++;
				} // catch up
			}
			offset_2 = offset_1; offset_1 = (uint32)OFFBASE_TO_OFFSET(offBase);
		}
		/* store sequence */
_storeSequence:
		{   
			const size_t litLength = (size_t)(start - anchor);
		    ZSTD_storeSeq(seqStore, litLength, anchor, iend, (uint32)offBase, matchLength);
		    anchor = ip = start + matchLength;
		}
		/* check immediate repcode */
		if(isDxS) {
			while(ip <= ilimit) {
				const uint32 current2 = (uint32)(ip-base);
				const uint32 repIndex = current2 - offset_2;
				const BYTE * repMatch = repIndex < prefixLowestIndex ? dictBase - dictIndexDelta + repIndex : base + repIndex;
				if(((uint32)((prefixLowestIndex-1) - (uint32)repIndex) >= 3 /* intentional overflow */) && (SMem::Get32(repMatch) == SMem::Get32(ip))) {
					const BYTE * const repEnd2 = repIndex < prefixLowestIndex ? dictEnd : iend;
					matchLength = ZSTD_count_2segments(ip+4, repMatch+4, iend, repEnd2, prefixLowest) + 4;
					// swap offset_2 <=> offset_1 
					offBase = offset_2; 
					offset_2 = offset_1; 
					offset_1 = (uint32)offBase; 
					ZSTD_storeSeq(seqStore, 0, anchor, iend, REPCODE1_TO_OFFBASE, matchLength);
					ip += matchLength;
					anchor = ip;
					continue;
				}
				break;
			}
		}
		if(dictMode == ZSTD_noDict) {
			while( ((ip <= ilimit) & (offset_2>0)) && (SMem::Get32(ip) == SMem::Get32(ip - offset_2)) ) {
				/* store sequence */
				matchLength = ZSTD_count(ip+4, ip+4-offset_2, iend) + 4;
				offBase = offset_2; offset_2 = offset_1; offset_1 = (uint32)offBase; /* swap repcodes */
				ZSTD_storeSeq(seqStore, 0, anchor, iend, REPCODE1_TO_OFFBASE, matchLength);
				ip += matchLength;
				anchor = ip;
				continue; /* faster when present ... (?) */
			}
		}
	}
	/* Save reps for next block */
	rep[0] = offset_1 ? offset_1 : savedOffset;
	rep[1] = offset_2 ? offset_2 : savedOffset;
	/* Return the last literals size */
	return (size_t)(iend - anchor);
}

size_t ZSTD_compressBlock_btlazy2(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_binaryTree, 2, ZSTD_noDict);
}

size_t ZSTD_compressBlock_lazy2(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_hashChain, 2, ZSTD_noDict);
}

size_t ZSTD_compressBlock_lazy(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_hashChain, 1, ZSTD_noDict);
}

size_t ZSTD_compressBlock_greedy(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_hashChain, 0, ZSTD_noDict);
}

size_t ZSTD_compressBlock_btlazy2_dictMatchState(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_binaryTree, 2, ZSTD_dictMatchState);
}

size_t ZSTD_compressBlock_lazy2_dictMatchState(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_hashChain, 2, ZSTD_dictMatchState);
}

size_t ZSTD_compressBlock_lazy_dictMatchState(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_hashChain, 1, ZSTD_dictMatchState);
}

size_t ZSTD_compressBlock_greedy_dictMatchState(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_hashChain, 0, ZSTD_dictMatchState);
}

size_t ZSTD_compressBlock_lazy2_dedicatedDictSearch(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_hashChain, 2, ZSTD_dedicatedDictSearch);
}

size_t ZSTD_compressBlock_lazy_dedicatedDictSearch(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_hashChain, 1, ZSTD_dedicatedDictSearch);
}

size_t ZSTD_compressBlock_greedy_dedicatedDictSearch(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_hashChain, 0, ZSTD_dedicatedDictSearch);
}

/* Row-based matchfinder */
size_t ZSTD_compressBlock_lazy2_row(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_rowHash, 2, ZSTD_noDict);
}

size_t ZSTD_compressBlock_lazy_row(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM],
    void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_rowHash, 1, ZSTD_noDict);
}

size_t ZSTD_compressBlock_greedy_row(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM],
    void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_rowHash, 0, ZSTD_noDict);
}

size_t ZSTD_compressBlock_lazy2_dictMatchState_row(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM],
    void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_rowHash, 2, ZSTD_dictMatchState);
}

size_t ZSTD_compressBlock_lazy_dictMatchState_row(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM],
    void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_rowHash, 1, ZSTD_dictMatchState);
}

size_t ZSTD_compressBlock_greedy_dictMatchState_row(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM],
    void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_rowHash, 0, ZSTD_dictMatchState);
}

size_t ZSTD_compressBlock_lazy2_dedicatedDictSearch_row(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM],
    void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_rowHash, 2, ZSTD_dedicatedDictSearch);
}

size_t ZSTD_compressBlock_lazy_dedicatedDictSearch_row(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM],
    void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_rowHash, 1, ZSTD_dedicatedDictSearch);
}

size_t ZSTD_compressBlock_greedy_dedicatedDictSearch_row(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM],
    void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_generic(ms, seqStore, rep, src, srcSize, search_rowHash, 0, ZSTD_dedicatedDictSearch);
}

FORCE_INLINE_TEMPLATE size_t ZSTD_compressBlock_lazy_extDict_generic(ZSTD_matchState_t* ms, seqStore_t* seqStore,
    uint32 rep[ZSTD_REP_NUM], const void * src, size_t srcSize, const searchMethod_e searchMethod, const uint32 depth)
{
	const BYTE * const istart = PTR8C(src);
	const BYTE * ip = istart;
	const BYTE * anchor = istart;
	const BYTE * const iend = istart + srcSize;
	const BYTE * const ilimit = searchMethod == search_rowHash ? iend - 8 - ZSTD_ROW_HASH_CACHE_SIZE : iend - 8;
	const BYTE * const base = ms->window.base;
	const uint32 dictLimit = ms->window.dictLimit;
	const BYTE * const prefixStart = base + dictLimit;
	const BYTE * const dictBase = ms->window.dictBase;
	const BYTE * const dictEnd  = dictBase + dictLimit;
	const BYTE * const dictStart  = dictBase + ms->window.lowLimit;
	const uint32 windowLog = ms->cParams.windowLog;
	const uint32 rowLog = ms->cParams.searchLog < 5 ? 4 : 5;

	searchMax_f const searchMax = ZSTD_selectLazyVTable(ms, searchMethod, ZSTD_extDict)->searchMax;
	uint32 offset_1 = rep[0], offset_2 = rep[1];
	DEBUGLOG(5, "ZSTD_compressBlock_lazy_extDict_generic (searchFunc=%u)", (uint32)searchMethod);
	/* init */
	ip += (ip == prefixStart);
	if(searchMethod == search_rowHash) {
		ZSTD_row_fillHashCache(ms, base, rowLog, MIN(ms->cParams.minMatch, 6 /* mls caps out at 6 */), ms->nextToUpdate, ilimit);
	}

	/* Match Loop */
#if defined(__GNUC__) && defined(__x86_64__)
	/* I've measured random a 5% speed loss on levels 5 & 6 (greedy) when the
	 * code alignment is perturbed. To fix the instability align the loop on 32-bytes.
	 */
	__asm__ (".p2align 5");
#endif
	while(ip < ilimit) {
		size_t matchLength = 0;
		size_t offBase = REPCODE1_TO_OFFBASE;
		const BYTE * start = ip+1;
		uint32 curr = (uint32)(ip-base);
		/* check repCode */
		{   
			const uint32 windowLow = ZSTD_getLowestMatchIndex(ms, curr+1, windowLog);
		    const uint32 repIndex = (uint32)(curr+1 - offset_1);
		    const BYTE * const repBase = repIndex < dictLimit ? dictBase : base;
		    const BYTE * const repMatch = repBase + repIndex;
		    if(((uint32)((dictLimit-1) - repIndex) >= 3) /* intentional overflow */ & (offset_1 <= curr+1 - windowLow) ) /* note: we are searching at curr+1 */
			    if(SMem::Get32(ip+1) == SMem::Get32(repMatch)) {
				    /* repcode detected we should take it */
				    const BYTE * const repEnd = repIndex < dictLimit ? dictEnd : iend;
				    matchLength = ZSTD_count_2segments(ip+1+4, repMatch+4, iend, repEnd, prefixStart) + 4;
				    if(depth==0) goto _storeSequence;
			    }
		}
		/* first search (depth 0) */
		{   
			size_t ofbCandidate = 999999999;
		    const size_t ml2 = searchMax(ms, ip, iend, &ofbCandidate);
		    if(ml2 > matchLength)
			    matchLength = ml2, start = ip, offBase = ofbCandidate; 
		}
		if(matchLength < 4) {
			ip += ((ip-anchor) >> kSearchStrength) + 1; /* jump faster over incompressible sections */
			continue;
		}
		/* let's try to find a better solution */
		if(depth>=1)
			while(ip<ilimit) {
				ip++;
				curr++;
				/* check repCode */
				if(offBase) {
					const uint32 windowLow = ZSTD_getLowestMatchIndex(ms, curr, windowLog);
					const uint32 repIndex = (uint32)(curr - offset_1);
					const BYTE * const repBase = repIndex < dictLimit ? dictBase : base;
					const BYTE * const repMatch = repBase + repIndex;
					if( ((uint32)((dictLimit-1) - repIndex) >= 3) /* intentional overflow : do not test positions overlapping 2 memory segments  */
					    & (offset_1 <= curr - windowLow) ) /* equivalent to `curr > repIndex >= windowLow` */
						if(SMem::Get32(ip) == SMem::Get32(repMatch)) {
							/* repcode detected */
							const BYTE * const repEnd = repIndex < dictLimit ? dictEnd : iend;
							const size_t repLength = ZSTD_count_2segments(ip+4, repMatch+4, iend, repEnd, prefixStart) + 4;
							int const gain2 = (int)(repLength * 3);
							int const gain1 = (int)(matchLength*3 - ZSTD_highbit32((uint32)offBase) + 1);
							if((repLength >= 4) && (gain2 > gain1))
								matchLength = repLength, offBase = REPCODE1_TO_OFFBASE, start = ip;
						}
				}
				/* search match, depth 1 */
				{   
					size_t ofbCandidate = 999999999;
				    const size_t ml2 = searchMax(ms, ip, iend, &ofbCandidate);
				    int const gain2 = (int)(ml2*4 - ZSTD_highbit32((uint32)ofbCandidate)); /* raw approx */
				    int const gain1 = (int)(matchLength*4 - ZSTD_highbit32((uint32)offBase) + 4);
				    if((ml2 >= 4) && (gain2 > gain1)) {
					    matchLength = ml2, offBase = ofbCandidate, start = ip;
					    continue; /* search a better one */
				    }
				}
				/* let's find an even better one */
				if((depth==2) && (ip<ilimit)) {
					ip++;
					curr++;
					/* check repCode */
					if(offBase) {
						const uint32 windowLow = ZSTD_getLowestMatchIndex(ms, curr, windowLog);
						const uint32 repIndex = (uint32)(curr - offset_1);
						const BYTE * const repBase = repIndex < dictLimit ? dictBase : base;
						const BYTE * const repMatch = repBase + repIndex;
						if( ((uint32)((dictLimit-1) - repIndex) >= 3) // intentional overflow : do not test positions overlapping 2 memory segments 
						    & (offset_1 <= curr - windowLow) ) // equivalent to `curr > repIndex >= windowLow` 
							if(SMem::Get32(ip) == SMem::Get32(repMatch)) {
								/* repcode detected */
								const BYTE * const repEnd = repIndex < dictLimit ? dictEnd : iend;
								const size_t repLength = ZSTD_count_2segments(ip+4, repMatch+4, iend, repEnd, prefixStart) + 4;
								int const gain2 = (int)(repLength * 4);
								int const gain1 = (int)(matchLength*4 - ZSTD_highbit32((uint32)offBase) + 1);
								if((repLength >= 4) && (gain2 > gain1))
									matchLength = repLength, offBase = REPCODE1_TO_OFFBASE, start = ip;
							}
					}
					/* search match, depth 2 */
					{   
						size_t ofbCandidate = 999999999;
					    const size_t ml2 = searchMax(ms, ip, iend, &ofbCandidate);
					    int const gain2 = (int)(ml2*4 - ZSTD_highbit32((uint32)ofbCandidate)); // raw approx
					    int const gain1 = (int)(matchLength*4 - ZSTD_highbit32((uint32)offBase) + 7);
					    if((ml2 >= 4) && (gain2 > gain1)) {
						    matchLength = ml2, offBase = ofbCandidate, start = ip;
						    continue;
					    }
					}
				}
				break; /* nothing found : store previous solution */
			}

		/* catch up */
		if(OFFBASE_IS_OFFSET(offBase)) {
			const uint32 matchIndex = (uint32)((size_t)(start-base) - OFFBASE_TO_OFFSET(offBase));
			const BYTE * match = (matchIndex < dictLimit) ? dictBase + matchIndex : base + matchIndex;
			const BYTE * const mStart = (matchIndex < dictLimit) ? dictStart : prefixStart;
			while((start>anchor) && (match>mStart) && (start[-1] == match[-1])) {
				start--; match--; matchLength++;
			} // catch up
			offset_2 = offset_1; offset_1 = (uint32)OFFBASE_TO_OFFSET(offBase);
		}
		/* store sequence */
_storeSequence:
		{   
			const size_t litLength = (size_t)(start - anchor);
		    ZSTD_storeSeq(seqStore, litLength, anchor, iend, (uint32)offBase, matchLength);
		    anchor = ip = start + matchLength;
		}
		/* check immediate repcode */
		while(ip <= ilimit) {
			const uint32 repCurrent = (uint32)(ip-base);
			const uint32 windowLow = ZSTD_getLowestMatchIndex(ms, repCurrent, windowLog);
			const uint32 repIndex = repCurrent - offset_2;
			const BYTE * const repBase = repIndex < dictLimit ? dictBase : base;
			const BYTE * const repMatch = repBase + repIndex;
			if( ((uint32)((dictLimit-1) - repIndex) >= 3) /* intentional overflow : do not test positions overlapping 2 memory segments  */
			    & (offset_2 <= repCurrent - windowLow) ) /* equivalent to `curr > repIndex >= windowLow` */
				if(SMem::Get32(ip) == SMem::Get32(repMatch)) {
					/* repcode detected we should take it */
					const BYTE * const repEnd = repIndex < dictLimit ? dictEnd : iend;
					matchLength = ZSTD_count_2segments(ip+4, repMatch+4, iend, repEnd, prefixStart) + 4;
					offBase = offset_2; offset_2 = offset_1; offset_1 = (uint32)offBase; /* swap offset
					                                                                     history */
					ZSTD_storeSeq(seqStore, 0, anchor, iend, REPCODE1_TO_OFFBASE, matchLength);
					ip += matchLength;
					anchor = ip;
					continue; /* faster when present ... (?) */
				}
			break;
		}
	}
	/* Save reps for next block */
	rep[0] = offset_1;
	rep[1] = offset_2;
	/* Return the last literals size */
	return (size_t)(iend - anchor);
}

size_t ZSTD_compressBlock_greedy_extDict(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_extDict_generic(ms, seqStore, rep, src, srcSize, search_hashChain, 0);
}

size_t ZSTD_compressBlock_lazy_extDict(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_extDict_generic(ms, seqStore, rep, src, srcSize, search_hashChain, 1);
}

size_t ZSTD_compressBlock_lazy2_extDict(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_extDict_generic(ms, seqStore, rep, src, srcSize, search_hashChain, 2);
}

size_t ZSTD_compressBlock_btlazy2_extDict(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_extDict_generic(ms, seqStore, rep, src, srcSize, search_binaryTree, 2);
}

size_t ZSTD_compressBlock_greedy_extDict_row(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_extDict_generic(ms, seqStore, rep, src, srcSize, search_rowHash, 0);
}

size_t ZSTD_compressBlock_lazy_extDict_row(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_extDict_generic(ms, seqStore, rep, src, srcSize, search_rowHash, 1);
}

size_t ZSTD_compressBlock_lazy2_extDict_row(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	return ZSTD_compressBlock_lazy_extDict_generic(ms, seqStore, rep, src, srcSize, search_rowHash, 2);
}
