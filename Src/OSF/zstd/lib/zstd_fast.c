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
#include "zstd_compress_internal.h"  /* ZSTD_hashPtr, ZSTD_count, ZSTD_storeSeq */
#include "zstd_fast.h"

void ZSTD_fillHashTable(ZSTD_matchState_t* ms, const void * const end, ZSTD_dictTableLoadMethod_e dtlm)
{
	const ZSTD_compressionParameters* const cParams = &ms->cParams;
	uint32 * const hashTable = ms->hashTable;
	const uint32 hBits = cParams->hashLog;
	const uint32 mls = cParams->minMatch;
	const BYTE * const base = ms->window.base;
	const BYTE * ip = base + ms->nextToUpdate;
	const BYTE * const iend = ((const BYTE *)end) - HASH_READ_SIZE;
	const uint32 fastHashFillStep = 3;

	/* Always insert every fastHashFillStep position into the hash table.
	 * Insert the other positions if their hash entry is empty.
	 */
	for(; ip + fastHashFillStep < iend + 2; ip += fastHashFillStep) {
		const uint32 curr = (uint32)(ip - base);
		const size_t hash0 = ZSTD_hashPtr(ip, hBits, mls);
		hashTable[hash0] = curr;
		if(dtlm == ZSTD_dtlm_fast) 
			continue;
		/* Only load extra positions for ZSTD_dtlm_full */
		{
		    for(uint32 p = 1; p < fastHashFillStep; ++p) {
			    const size_t hash = ZSTD_hashPtr(ip + p, hBits, mls);
			    if(hashTable[hash] == 0) { /* not yet filled */
				    hashTable[hash] = curr + p;
			    }
		    }
		}
	}
}

/**
 * If you squint hard enough (and ignore repcodes), the search operation at any
 * given position is broken into 4 stages:
 *
 * 1. Hash   (map position to hash value via input read)
 * 2. Lookup (map hash val to index via hashtable read)
 * 3. Load   (map index to value at that position via input read)
 * 4. Compare
 *
 * Each of these steps involves a memory read at an address which is computed
 * from the previous step. This means these steps must be sequenced and their
 * latencies are cumulative.
 *
 * Rather than do 1->2->3->4 sequentially for a single position before moving
 * onto the next, this implementation interleaves these operations across the
 * next few positions:
 *
 * R = Repcode Read & Compare
 * H = Hash
 * T = Table Lookup
 * M = Match Read & Compare
 *
 * Pos | Time -->
 * ----+-------------------
 * N   | ... M
 * N+1 | ...   TM
 * N+2 |    R H   T M
 * N+3 |         H    TM
 * N+4 |           R H   T M
 * N+5 |                H   ...
 * N+6 |                  R ...
 *
 * This is very much analogous to the pipelining of execution in a CPU. And just
 * like a CPU, we have to dump the pipeline when we find a match (i.e., take a
 * branch).
 *
 * When this happens, we throw away our current state, and do the following prep
 * to re-enter the loop:
 *
 * Pos | Time -->
 * ----+-------------------
 * N   | H T
 * N+1 |  H
 *
 * This is also the work we do at the beginning to enter the loop initially.
 */
FORCE_INLINE_TEMPLATE size_t ZSTD_compressBlock_fast_noDict_generic(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM],
    void const* src, size_t srcSize, const uint32 mls, const uint32 hasStep)
{
	const ZSTD_compressionParameters* const cParams = &ms->cParams;
	uint32 * const hashTable = ms->hashTable;
	const uint32 hlog = cParams->hashLog;
	/* support stepSize of 0 */
	const size_t stepSize = hasStep ? (cParams->targetLength + !(cParams->targetLength) + 1) : 2;
	const BYTE * const base = ms->window.base;
	const BYTE * const istart = PTR8C(src);
	const uint32 endIndex = (uint32)((size_t)(istart - base) + srcSize);
	const uint32 prefixStartIndex = ZSTD_getLowestPrefixIndex(ms, endIndex, cParams->windowLog);
	const BYTE * const prefixStart = base + prefixStartIndex;
	const BYTE * const iend = istart + srcSize;
	const BYTE * const ilimit = iend - HASH_READ_SIZE;

	const BYTE * anchor = istart;
	const BYTE * ip0 = istart;
	const BYTE * ip1;
	const BYTE * ip2;
	const BYTE * ip3;
	uint32 current0;

	uint32 rep_offset1 = rep[0];
	uint32 rep_offset2 = rep[1];
	uint32 offsetSaved = 0;

	size_t hash0; /* hash for ip0 */
	size_t hash1; /* hash for ip1 */
	uint32 idx; /* match idx for ip0 */
	uint32 mval; /* src value at match idx */

	uint32 offcode;
	const BYTE * match0;
	size_t mLength;

	/* ip0 and ip1 are always adjacent. The targetLength skipping and
	 * uncompressibility acceleration is applied to every other position,
	 * matching the behavior of #1562. step therefore represents the gap
	 * between pairs of positions, from ip0 to ip2 or ip1 to ip3. */
	size_t step;
	const BYTE * nextStep;
	const size_t kStepIncr = (1 << (kSearchStrength - 1));

	DEBUGLOG(5, "ZSTD_compressBlock_fast_generic");
	ip0 += (ip0 == prefixStart);
	{   
		const uint32 curr = (uint32)(ip0 - base);
	    const uint32 windowLow = ZSTD_getLowestPrefixIndex(ms, curr, cParams->windowLog);
	    const uint32 maxRep = curr - windowLow;
	    if(rep_offset2 > maxRep) 
			offsetSaved = rep_offset2, rep_offset2 = 0;
	    if(rep_offset1 > maxRep) 
			offsetSaved = rep_offset1, rep_offset1 = 0; 
	}
	/* start each op */
_start: /* Requires: ip0 */
	step = stepSize;
	nextStep = ip0 + kStepIncr;
	/* calculate positions, ip0 - anchor == 0, so we skip step calc */
	ip1 = ip0 + 1;
	ip2 = ip0 + step;
	ip3 = ip2 + 1;
	if(ip3 >= ilimit) {
		goto _cleanup;
	}
	hash0 = ZSTD_hashPtr(ip0, hlog, mls);
	hash1 = ZSTD_hashPtr(ip1, hlog, mls);
	idx = hashTable[hash0];
	do {
		/* load repcode match for ip[2]*/
		const uint32 rval = SMem::Get32(ip2 - rep_offset1);
		/* write back hash table entry */
		current0 = (uint32)(ip0 - base);
		hashTable[hash0] = current0;
		/* check repcode at ip[2] */
		if((SMem::Get32(ip2) == rval) & (rep_offset1 > 0)) {
			ip0 = ip2;
			match0 = ip0 - rep_offset1;
			mLength = ip0[-1] == match0[-1];
			ip0 -= mLength;
			match0 -= mLength;
			offcode = REPCODE1_TO_OFFBASE;
			mLength += 4;
			goto _match;
		}
		/* load match for ip[0] */
		if(idx >= prefixStartIndex) {
			mval = SMem::Get32(base + idx);
		}
		else {
			mval = SMem::Get32(ip0) ^ 1; /* guaranteed to not match. */
		}
		/* check match at ip[0] */
		if(SMem::Get32(ip0) == mval) {
			/* found a match! */
			goto _offset;
		}
		/* lookup ip[1] */
		idx = hashTable[hash1];
		/* hash ip[2] */
		hash0 = hash1;
		hash1 = ZSTD_hashPtr(ip2, hlog, mls);
		/* advance to next positions */
		ip0 = ip1;
		ip1 = ip2;
		ip2 = ip3;
		/* write back hash table entry */
		current0 = (uint32)(ip0 - base);
		hashTable[hash0] = current0;
		/* load match for ip[0] */
		if(idx >= prefixStartIndex) {
			mval = SMem::Get32(base + idx);
		}
		else {
			mval = SMem::Get32(ip0) ^ 1; /* guaranteed to not match. */
		}
		/* check match at ip[0] */
		if(SMem::Get32(ip0) == mval) {
			/* found a match! */
			goto _offset;
		}
		/* lookup ip[1] */
		idx = hashTable[hash1];
		/* hash ip[2] */
		hash0 = hash1;
		hash1 = ZSTD_hashPtr(ip2, hlog, mls);
		/* advance to next positions */
		ip0 = ip1;
		ip1 = ip2;
		ip2 = ip0 + step;
		ip3 = ip1 + step;
		/* calculate step */
		if(ip2 >= nextStep) {
			step++;
			PREFETCH_L1(ip1 + 64);
			PREFETCH_L1(ip1 + 128);
			nextStep += kStepIncr;
		}
	} while(ip3 < ilimit);
_cleanup:
	/* Note that there are probably still a couple positions we could search.
	 * However, it seems to be a meaningful performance hit to try to search
	 * them. So let's not. */
	/* save reps for next block */
	rep[0] = rep_offset1 ? rep_offset1 : offsetSaved;
	rep[1] = rep_offset2 ? rep_offset2 : offsetSaved;
	/* Return the last literals size */
	return (size_t)(iend - anchor);
_offset: /* Requires: ip0, idx */
	/* Compute the offset code. */
	match0 = base + idx;
	rep_offset2 = rep_offset1;
	rep_offset1 = (uint32)(ip0-match0);
	offcode = OFFSET_TO_OFFBASE(rep_offset1);
	mLength = 4;
	/* Count the backwards match length. */
	while(((ip0>anchor) & (match0>prefixStart)) && (ip0[-1] == match0[-1])) {
		ip0--;
		match0--;
		mLength++;
	}
_match: /* Requires: ip0, match0, offcode */
	/* Count the forward length. */
	mLength += ZSTD_count(ip0 + mLength, match0 + mLength, iend);
	ZSTD_storeSeq(seqStore, (size_t)(ip0 - anchor), anchor, iend, offcode, mLength);
	ip0 += mLength;
	anchor = ip0;
	/* write next hash table entry */
	if(ip1 < ip0) {
		hashTable[hash1] = (uint32)(ip1 - base);
	}
	/* Fill table and check for immediate repcode. */
	if(ip0 <= ilimit) {
		/* Fill Table */
		assert(base+current0+2 > istart); /* check base overflow */
		hashTable[ZSTD_hashPtr(base+current0+2, hlog, mls)] = current0+2; /* here because current+2 could be > iend-8 */
		hashTable[ZSTD_hashPtr(ip0-2, hlog, mls)] = (uint32)(ip0-2-base);
		if(rep_offset2 > 0) { /* rep_offset2==0 means rep_offset2 is invalidated */
			while( (ip0 <= ilimit) && (SMem::Get32(ip0) == SMem::Get32(ip0 - rep_offset2)) ) {
				/* store sequence */
				const size_t rLength = ZSTD_count(ip0+4, ip0+4-rep_offset2, iend) + 4;
				{ 
					const uint32 tmpOff = rep_offset2; 
					rep_offset2 = rep_offset1; 
					rep_offset1 = tmpOff; 
				} /* swap rep_offset2 <=> rep_offset1 */
				hashTable[ZSTD_hashPtr(ip0, hlog, mls)] = (uint32)(ip0-base);
				ip0 += rLength;
				ZSTD_storeSeq(seqStore, 0 /*litLen*/, anchor, iend, REPCODE1_TO_OFFBASE, rLength);
				anchor = ip0;
				continue; /* faster when present (confirmed on gcc-8) ... (?) */
			}
		}
	}
	goto _start;
}

#define ZSTD_GEN_FAST_FN(dictMode, mls, step)                                                            \
	static size_t ZSTD_compressBlock_fast_ ## dictMode ## _ ## mls ## _ ## step(                                      \
		ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM],                    \
		void const* src, size_t srcSize)                                                       \
	{                                                                                              \
		return ZSTD_compressBlock_fast_ ## dictMode ## _generic(ms, seqStore, rep, src, srcSize, mls, step); \
	}

ZSTD_GEN_FAST_FN(noDict, 4, 1)
ZSTD_GEN_FAST_FN(noDict, 5, 1)
ZSTD_GEN_FAST_FN(noDict, 6, 1)
ZSTD_GEN_FAST_FN(noDict, 7, 1)

ZSTD_GEN_FAST_FN(noDict, 4, 0)
ZSTD_GEN_FAST_FN(noDict, 5, 0)
ZSTD_GEN_FAST_FN(noDict, 6, 0)
ZSTD_GEN_FAST_FN(noDict, 7, 0)

size_t ZSTD_compressBlock_fast(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM],
    void const* src, size_t srcSize)
{
	const uint32 mls = ms->cParams.minMatch;
	assert(ms->dictMatchState == NULL);
	if(ms->cParams.targetLength > 1) {
		switch(mls)
		{
			default: /* includes case 3 */
			case 4:
			    return ZSTD_compressBlock_fast_noDict_4_1(ms, seqStore, rep, src, srcSize);
			case 5:
			    return ZSTD_compressBlock_fast_noDict_5_1(ms, seqStore, rep, src, srcSize);
			case 6:
			    return ZSTD_compressBlock_fast_noDict_6_1(ms, seqStore, rep, src, srcSize);
			case 7:
			    return ZSTD_compressBlock_fast_noDict_7_1(ms, seqStore, rep, src, srcSize);
		}
	}
	else {
		switch(mls)
		{
			default: /* includes case 3 */
			case 4:
			    return ZSTD_compressBlock_fast_noDict_4_0(ms, seqStore, rep, src, srcSize);
			case 5:
			    return ZSTD_compressBlock_fast_noDict_5_0(ms, seqStore, rep, src, srcSize);
			case 6:
			    return ZSTD_compressBlock_fast_noDict_6_0(ms, seqStore, rep, src, srcSize);
			case 7:
			    return ZSTD_compressBlock_fast_noDict_7_0(ms, seqStore, rep, src, srcSize);
		}
	}
}

FORCE_INLINE_TEMPLATE
size_t ZSTD_compressBlock_fast_dictMatchState_generic(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM],
    void const* src, size_t srcSize, const uint32 mls, const uint32 hasStep)
{
	const ZSTD_compressionParameters* const cParams = &ms->cParams;
	uint32 * const hashTable = ms->hashTable;
	const uint32 hlog = cParams->hashLog;
	/* support stepSize of 0 */
	const uint32 stepSize = cParams->targetLength + !(cParams->targetLength);
	const BYTE * const base = ms->window.base;
	const BYTE * const istart = PTR8C(src);
	const BYTE * ip0 = istart;
	const BYTE * ip1 = ip0 + stepSize; /* we assert below that stepSize >= 1 */
	const BYTE * anchor = istart;
	const uint32 prefixStartIndex = ms->window.dictLimit;
	const BYTE * const prefixStart = base + prefixStartIndex;
	const BYTE * const iend = istart + srcSize;
	const BYTE * const ilimit = iend - HASH_READ_SIZE;
	uint32 offset_1 = rep[0], offset_2 = rep[1];
	uint32 offsetSaved = 0;

	const ZSTD_matchState_t* const dms = ms->dictMatchState;
	const ZSTD_compressionParameters* const dictCParams = &dms->cParams;
	const uint32 * const dictHashTable = dms->hashTable;
	const uint32 dictStartIndex       = dms->window.dictLimit;
	const BYTE * const dictBase     = dms->window.base;
	const BYTE * const dictStart    = dictBase + dictStartIndex;
	const BYTE * const dictEnd      = dms->window.nextSrc;
	const uint32 dictIndexDelta       = prefixStartIndex - (uint32)(dictEnd - dictBase);
	const uint32 dictAndPrefixLength  = (uint32)(istart - prefixStart + dictEnd - dictStart);
	const uint32 dictHLog             = dictCParams->hashLog;

	/* if a dictionary is still attached, it necessarily means that
	 * it is within window size. So we just check it. */
	const uint32 maxDistance = 1U << cParams->windowLog;
	const uint32 endIndex = (uint32)((size_t)(istart - base) + srcSize);
	assert(endIndex - prefixStartIndex <= maxDistance);
	(void)maxDistance; (void)endIndex; /* these variables are not used when assert() is disabled */

	(void)hasStep; /* not currently specialized on whether it's accelerated */

	/* ensure there will be no underflow
	 * when translating a dict index into a local index */
	assert(prefixStartIndex >= (uint32)(dictEnd - dictBase));

	/* init */
	DEBUGLOG(5, "ZSTD_compressBlock_fast_dictMatchState_generic");
	ip0 += (dictAndPrefixLength == 0);
	/* dictMatchState repCode checks don't currently handle repCode == 0
	 * disabling. */
	assert(offset_1 <= dictAndPrefixLength);
	assert(offset_2 <= dictAndPrefixLength);

	/* Outer search loop */
	assert(stepSize >= 1);
	while(ip1 <= ilimit) { /* repcode check at (ip0 + 1) is safe because ip0 < ip1 */
		size_t mLength;
		size_t hash0 = ZSTD_hashPtr(ip0, hlog, mls);
		const size_t dictHash0 = ZSTD_hashPtr(ip0, dictHLog, mls);
		uint32 dictMatchIndex = dictHashTable[dictHash0];
		uint32 matchIndex = hashTable[hash0];
		uint32 curr = (uint32)(ip0 - base);
		size_t step = stepSize;
		const size_t kStepIncr = 1 << kSearchStrength;
		const BYTE * nextStep = ip0 + kStepIncr;

		/* Inner search loop */
		while(1) {
			const BYTE * match = base + matchIndex;
			const uint32 repIndex = curr + 1 - offset_1;
			const BYTE * repMatch = (repIndex < prefixStartIndex) ?
			    dictBase + (repIndex - dictIndexDelta) :
			    base + repIndex;
			const size_t hash1 = ZSTD_hashPtr(ip1, hlog, mls);
			const size_t dictHash1 = ZSTD_hashPtr(ip1, dictHLog, mls);
			hashTable[hash0] = curr; /* update hash table */

			if(((uint32)((prefixStartIndex - 1) - repIndex) >=
			    3) /* intentional underflow : ensure repIndex isn't overlapping dict + prefix */
			    && (SMem::Get32(repMatch) == SMem::Get32(ip0 + 1))) {
				const BYTE * const repMatchEnd = repIndex < prefixStartIndex ? dictEnd : iend;
				mLength = ZSTD_count_2segments(ip0 + 1 + 4, repMatch + 4, iend, repMatchEnd, prefixStart) + 4;
				ip0++;
				ZSTD_storeSeq(seqStore, (size_t)(ip0 - anchor), anchor, iend, REPCODE1_TO_OFFBASE, mLength);
				break;
			}
			else if(matchIndex <= prefixStartIndex) {
				/* We only look for a dict match if the normal matchIndex is invalid */
				const BYTE * dictMatch = dictBase + dictMatchIndex;
				if(dictMatchIndex > dictStartIndex &&
				    SMem::Get32(dictMatch) == SMem::Get32(ip0)) {
					/* found a dict match */
					const uint32 offset = (uint32)(curr - dictMatchIndex - dictIndexDelta);
					mLength = ZSTD_count_2segments(ip0 + 4, dictMatch + 4, iend, dictEnd, prefixStart) + 4;
					while(((ip0 > anchor) & (dictMatch > dictStart))
					    && (ip0[-1] == dictMatch[-1])) {
						ip0--;
						dictMatch--;
						mLength++;
					} /* catch up */
					offset_2 = offset_1;
					offset_1 = offset;
					ZSTD_storeSeq(seqStore, (size_t)(ip0 - anchor), anchor, iend, OFFSET_TO_OFFBASE(offset), mLength);
					break;
				}
			}
			else if(SMem::Get32(match) == SMem::Get32(ip0)) {
				/* found a regular match */
				const uint32 offset = (uint32)(ip0 - match);
				mLength = ZSTD_count(ip0 + 4, match + 4, iend) + 4;
				while(((ip0 > anchor) & (match > prefixStart))
				    && (ip0[-1] == match[-1])) {
					ip0--;
					match--;
					mLength++;
				} /* catch up */
				offset_2 = offset_1;
				offset_1 = offset;
				ZSTD_storeSeq(seqStore, (size_t)(ip0 - anchor), anchor, iend, OFFSET_TO_OFFBASE(offset), mLength);
				break;
			}
			/* Prepare for next iteration */
			dictMatchIndex = dictHashTable[dictHash1];
			matchIndex = hashTable[hash1];
			if(ip1 >= nextStep) {
				step++;
				nextStep += kStepIncr;
			}
			ip0 = ip1;
			ip1 = ip1 + step;
			if(ip1 > ilimit) goto _cleanup;

			curr = (uint32)(ip0 - base);
			hash0 = hash1;
		} /* end inner search loop */

		/* match found */
		assert(mLength);
		ip0 += mLength;
		anchor = ip0;

		if(ip0 <= ilimit) {
			/* Fill Table */
			assert(base+curr+2 > istart); /* check base overflow */
			hashTable[ZSTD_hashPtr(base+curr+2, hlog, mls)] = curr+2; /* here because curr+2 could be > iend-8 */
			hashTable[ZSTD_hashPtr(ip0-2, hlog, mls)] = (uint32)(ip0-2-base);
			/* check immediate repcode */
			while(ip0 <= ilimit) {
				const uint32 current2 = (uint32)(ip0-base);
				const uint32 repIndex2 = current2 - offset_2;
				const BYTE * repMatch2 = repIndex2 < prefixStartIndex ? dictBase - dictIndexDelta + repIndex2 : base + repIndex2;
				if( ((uint32)((prefixStartIndex-1) - (uint32)repIndex2) >= 3 /* intentional overflow */) && (SMem::Get32(repMatch2) == SMem::Get32(ip0))) {
					const BYTE * const repEnd2 = repIndex2 < prefixStartIndex ? dictEnd : iend;
					const size_t repLength2 = ZSTD_count_2segments(ip0+4, repMatch2+4, iend, repEnd2, prefixStart) + 4;
					uint32 tmpOffset = offset_2; offset_2 = offset_1; offset_1 = tmpOffset; /* swap offset_2 <=> offset_1 */
					ZSTD_storeSeq(seqStore, 0, anchor, iend, REPCODE1_TO_OFFBASE, repLength2);
					hashTable[ZSTD_hashPtr(ip0, hlog, mls)] = current2;
					ip0 += repLength2;
					anchor = ip0;
					continue;
				}
				break;
			}
		}
		/* Prepare for next iteration */
		assert(ip0 == anchor);
		ip1 = ip0 + stepSize;
	}
_cleanup:
	/* save reps for next block */
	rep[0] = offset_1 ? offset_1 : offsetSaved;
	rep[1] = offset_2 ? offset_2 : offsetSaved;
	/* Return the last literals size */
	return (size_t)(iend - anchor);
}

ZSTD_GEN_FAST_FN(dictMatchState, 4, 0)
ZSTD_GEN_FAST_FN(dictMatchState, 5, 0)
ZSTD_GEN_FAST_FN(dictMatchState, 6, 0)
ZSTD_GEN_FAST_FN(dictMatchState, 7, 0)

size_t ZSTD_compressBlock_fast_dictMatchState(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM],
    void const* src, size_t srcSize)
{
	const uint32 mls = ms->cParams.minMatch;
	assert(ms->dictMatchState != NULL);
	switch(mls) {
		default: /* includes case 3 */
		case 4: return ZSTD_compressBlock_fast_dictMatchState_4_0(ms, seqStore, rep, src, srcSize);
		case 5: return ZSTD_compressBlock_fast_dictMatchState_5_0(ms, seqStore, rep, src, srcSize);
		case 6: return ZSTD_compressBlock_fast_dictMatchState_6_0(ms, seqStore, rep, src, srcSize);
		case 7: return ZSTD_compressBlock_fast_dictMatchState_7_0(ms, seqStore, rep, src, srcSize);
	}
}

static size_t ZSTD_compressBlock_fast_extDict_generic(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM],
    void const* src, size_t srcSize, const uint32 mls, const uint32 hasStep)
{
	const ZSTD_compressionParameters* const cParams = &ms->cParams;
	uint32 * hashTable = ms->hashTable;
	const uint32 hlog = cParams->hashLog;
	/* support stepSize of 0 */
	const uint32 stepSize = cParams->targetLength + !(cParams->targetLength);
	const BYTE * const base = ms->window.base;
	const BYTE * const dictBase = ms->window.dictBase;
	const BYTE * const istart = PTR8C(src);
	const BYTE * ip = istart;
	const BYTE * anchor = istart;
	const uint32 endIndex = (uint32)((size_t)(istart - base) + srcSize);
	const uint32 lowLimit = ZSTD_getLowestMatchIndex(ms, endIndex, cParams->windowLog);
	const uint32 dictStartIndex = lowLimit;
	const BYTE * const dictStart = dictBase + dictStartIndex;
	const uint32 dictLimit = ms->window.dictLimit;
	const uint32 prefixStartIndex = dictLimit < lowLimit ? lowLimit : dictLimit;
	const BYTE * const prefixStart = base + prefixStartIndex;
	const BYTE * const dictEnd = dictBase + prefixStartIndex;
	const BYTE * const iend = istart + srcSize;
	const BYTE * const ilimit = iend - 8;
	uint32 offset_1 = rep[0], offset_2 = rep[1];

	(void)hasStep; /* not currently specialized on whether it's accelerated */

	DEBUGLOG(5, "ZSTD_compressBlock_fast_extDict_generic (offset_1=%u)", offset_1);

	/* switch to "regular" variant if extDict is invalidated due to maxDistance */
	if(prefixStartIndex == dictStartIndex)
		return ZSTD_compressBlock_fast(ms, seqStore, rep, src, srcSize);

	/* Search Loop */
	while(ip < ilimit) { /* < instead of <=, because (ip+1) */
		const size_t h = ZSTD_hashPtr(ip, hlog, mls);
		const uint32 matchIndex = hashTable[h];
		const BYTE * const matchBase = matchIndex < prefixStartIndex ? dictBase : base;
		const BYTE *  match = matchBase + matchIndex;
		const uint32 curr = (uint32)(ip-base);
		const uint32 repIndex = curr + 1 - offset_1;
		const BYTE * const repBase = repIndex < prefixStartIndex ? dictBase : base;
		const BYTE * const repMatch = repBase + repIndex;
		hashTable[h] = curr; /* update hash table */
		DEBUGLOG(7, "offset_1 = %u , curr = %u", offset_1, curr);

		if( ( ((uint32)((prefixStartIndex-1) - repIndex) >= 3) /* intentional underflow */
		    & (offset_1 <= curr+1 - dictStartIndex) ) /* note: we are searching at curr+1 */
		    && (SMem::Get32(repMatch) == SMem::Get32(ip+1)) ) {
			const BYTE * const repMatchEnd = repIndex < prefixStartIndex ? dictEnd : iend;
			const size_t rLength = ZSTD_count_2segments(ip+1 +4, repMatch +4, iend, repMatchEnd, prefixStart) + 4;
			ip++;
			ZSTD_storeSeq(seqStore, (size_t)(ip-anchor), anchor, iend, REPCODE1_TO_OFFBASE, rLength);
			ip += rLength;
			anchor = ip;
		}
		else {
			if((matchIndex < dictStartIndex) || (SMem::Get32(match) != SMem::Get32(ip)) ) {
				assert(stepSize >= 1);
				ip += ((ip-anchor) >> kSearchStrength) + stepSize;
				continue;
			}
			{   
				const BYTE * const matchEnd = matchIndex < prefixStartIndex ? dictEnd : iend;
			    const BYTE * const lowMatchPtr = matchIndex < prefixStartIndex ? dictStart : prefixStart;
			    const uint32 offset = curr - matchIndex;
			    size_t mLength = ZSTD_count_2segments(ip+4, match+4, iend, matchEnd, prefixStart) + 4;
			    while(((ip>anchor) & (match>lowMatchPtr)) && (ip[-1] == match[-1])) {
				    ip--; match--; mLength++;
			    } // catch up 
			    offset_2 = offset_1; 
				offset_1 = offset; /* update offset history */
			    ZSTD_storeSeq(seqStore, (size_t)(ip-anchor), anchor, iend, OFFSET_TO_OFFBASE(offset), mLength);
			    ip += mLength;
			    anchor = ip;
			}
		}

		if(ip <= ilimit) {
			/* Fill Table */
			hashTable[ZSTD_hashPtr(base+curr+2, hlog, mls)] = curr+2;
			hashTable[ZSTD_hashPtr(ip-2, hlog, mls)] = (uint32)(ip-2-base);
			/* check immediate repcode */
			while(ip <= ilimit) {
				const uint32 current2 = (uint32)(ip-base);
				const uint32 repIndex2 = current2 - offset_2;
				const BYTE * const repMatch2 = repIndex2 < prefixStartIndex ? dictBase + repIndex2 : base + repIndex2;
				if( (((uint32)((prefixStartIndex-1) - repIndex2) >= 3) & (offset_2 <= curr - dictStartIndex)) /* intentional overflow */
				    && (SMem::Get32(repMatch2) == SMem::Get32(ip)) ) {
					const BYTE * const repEnd2 = repIndex2 < prefixStartIndex ? dictEnd : iend;
					const size_t repLength2 = ZSTD_count_2segments(ip+4, repMatch2+4, iend, repEnd2, prefixStart) + 4;
					{ 
						const uint32 tmpOffset = offset_2; 
						offset_2 = offset_1; 
						offset_1 = tmpOffset; 
					} /* swap offset_2 <=> offset_1 */
					ZSTD_storeSeq(seqStore, 0 /*litlen*/, anchor, iend, REPCODE1_TO_OFFBASE, repLength2);
					hashTable[ZSTD_hashPtr(ip, hlog, mls)] = current2;
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

ZSTD_GEN_FAST_FN(extDict, 4, 0)
ZSTD_GEN_FAST_FN(extDict, 5, 0)
ZSTD_GEN_FAST_FN(extDict, 6, 0)
ZSTD_GEN_FAST_FN(extDict, 7, 0)

size_t ZSTD_compressBlock_fast_extDict(ZSTD_matchState_t* ms, seqStore_t* seqStore, uint32 rep[ZSTD_REP_NUM], void const* src, size_t srcSize)
{
	const uint32 mls = ms->cParams.minMatch;
	switch(mls) {
		default: /* includes case 3 */
		case 4: return ZSTD_compressBlock_fast_extDict_4_0(ms, seqStore, rep, src, srcSize);
		case 5: return ZSTD_compressBlock_fast_extDict_5_0(ms, seqStore, rep, src, srcSize);
		case 6: return ZSTD_compressBlock_fast_extDict_6_0(ms, seqStore, rep, src, srcSize);
		case 7: return ZSTD_compressBlock_fast_extDict_7_0(ms, seqStore, rep, src, srcSize);
	}
}
