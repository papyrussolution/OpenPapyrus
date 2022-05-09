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
#include "zstd_compress_sequences.h"
/**
 * -log2(x / 256) lookup table for x in [0, 256).
 * If x == 0: Return 0
 * Else: Return floor(-log2(x / 256) * 256)
 */
static const uint kInverseProbabilityLog256[256] = {
	0,    2048, 1792, 1642, 1536, 1453, 1386, 1329, 1280, 1236, 1197, 1162,
	1130, 1100, 1073, 1047, 1024, 1001, 980,  960,  941,  923,  906,  889,
	874,  859,  844,  830,  817,  804,  791,  779,  768,  756,  745,  734,
	724,  714,  704,  694,  685,  676,  667,  658,  650,  642,  633,  626,
	618,  610,  603,  595,  588,  581,  574,  567,  561,  554,  548,  542,
	535,  529,  523,  517,  512,  506,  500,  495,  489,  484,  478,  473,
	468,  463,  458,  453,  448,  443,  438,  434,  429,  424,  420,  415,
	411,  407,  402,  398,  394,  390,  386,  382,  377,  373,  370,  366,
	362,  358,  354,  350,  347,  343,  339,  336,  332,  329,  325,  322,
	318,  315,  311,  308,  305,  302,  298,  295,  292,  289,  286,  282,
	279,  276,  273,  270,  267,  264,  261,  258,  256,  253,  250,  247,
	244,  241,  239,  236,  233,  230,  228,  225,  222,  220,  217,  215,
	212,  209,  207,  204,  202,  199,  197,  194,  192,  190,  187,  185,
	182,  180,  178,  175,  173,  171,  168,  166,  164,  162,  159,  157,
	155,  153,  151,  149,  146,  144,  142,  140,  138,  136,  134,  132,
	130,  128,  126,  123,  121,  119,  117,  115,  114,  112,  110,  108,
	106,  104,  102,  100,  98,   96,   94,   93,   91,   89,   87,   85,
	83,   82,   80,   78,   76,   74,   73,   71,   69,   67,   66,   64,
	62,   61,   59,   57,   55,   54,   52,   50,   49,   47,   46,   44,
	42,   41,   39,   37,   36,   34,   33,   31,   30,   28,   26,   25,
	23,   22,   20,   19,   17,   16,   14,   13,   11,   10,   8,    7,
	5,    4,    2,    1,
};

static uint FASTCALL ZSTD_getFSEMaxSymbolValue(const FSE_CTable * ctable) 
{
	const void * ptr = ctable;
	const uint16 * u16ptr = (const uint16 *)ptr;
	const uint32 maxSymbolValue = MEM_read16(u16ptr + 1);
	return maxSymbolValue;
}
/**
 * Returns true if we should use ncount=-1 else we should
 * use ncount=1 for low probability symbols instead.
 */
static uint FASTCALL ZSTD_useLowProbCount(const size_t nbSeq)
{
	/* Heuristic: This should cover most blocks <= 16K and
	 * start to fade out after 16K to about 32K depending on compressibility.
	 */
	return (nbSeq >= 2048);
}
/**
 * Returns the cost in bytes of encoding the normalized count header.
 * Returns an error if any of the helper functions return an error.
 */
static size_t ZSTD_NCountCost(const uint * count, const uint max, const size_t nbSeq, const uint FSELog)
{
	BYTE wksp[FSE_NCOUNTBOUND];
	int16 norm[MaxSeq + 1];
	const uint32 tableLog = FSE_optimalTableLog(FSELog, nbSeq, max);
	FORWARD_IF_ERROR(FSE_normalizeCount(norm, tableLog, count, nbSeq, max, ZSTD_useLowProbCount(nbSeq)), "");
	return FSE_writeNCount(wksp, sizeof(wksp), norm, max, tableLog);
}
/**
 * Returns the cost in bits of encoding the distribution described by count
 * using the entropy bound.
 */
static size_t ZSTD_entropyCost(const uint * count, const uint max, const size_t total)
{
	uint cost = 0;
	assert(total > 0);
	for(uint s = 0; s <= max; ++s) {
		uint norm = (uint)((256 * count[s]) / total);
		if(count[s] != 0 && norm == 0)
			norm = 1;
		assert(count[s] < total);
		cost += count[s] * kInverseProbabilityLog256[norm];
	}
	return cost >> 8;
}
/**
 * Returns the cost in bits of encoding the distribution in count using ctable.
 * Returns an error if ctable cannot represent all the symbols in count.
 */
size_t ZSTD_fseBitCost(const FSE_CTable * ctable, const uint * count, const uint max)
{
	const uint kAccuracyLog = 8;
	size_t cost = 0;
	uint   s;
	FSE_CState_t cstate;
	FSE_initCState(&cstate, ctable);
	if(ZSTD_getFSEMaxSymbolValue(ctable) < max) {
		DEBUGLOG(5, "Repeat FSE_CTable has maxSymbolValue %u < %u", ZSTD_getFSEMaxSymbolValue(ctable), max);
		return ERROR(GENERIC);
	}
	for(s = 0; s <= max; ++s) {
		const uint tableLog = cstate.stateLog;
		const uint badCost = (tableLog + 1) << kAccuracyLog;
		const uint bitCost = FSE_bitCost(cstate.symbolTT, tableLog, s, kAccuracyLog);
		if(count[s] == 0)
			continue;
		if(bitCost >= badCost) {
			DEBUGLOG(5, "Repeat FSE_CTable has Prob[%u] == 0", s);
			return ERROR(GENERIC);
		}
		cost += (size_t)count[s] * bitCost;
	}
	return cost >> kAccuracyLog;
}

/**
 * Returns the cost in bits of encoding the distribution in count using the
 * table described by norm. The max symbol support by norm is assumed >= max.
 * norm must be valid for every symbol with non-zero probability in count.
 */
size_t ZSTD_crossEntropyCost(const short * norm, uint accuracyLog, const uint * count, const uint max)
{
	const uint shift = 8 - accuracyLog;
	size_t cost = 0;
	assert(accuracyLog <= 8);
	for(uint s = 0; s <= max; ++s) {
		const uint normAcc = (norm[s] != -1) ? (uint)norm[s] : 1;
		const uint norm256 = normAcc << shift;
		assert(norm256 > 0);
		assert(norm256 < 256);
		cost += count[s] * kInverseProbabilityLog256[norm256];
	}
	return cost >> 8;
}

symbolEncodingType_e ZSTD_selectEncodingType(FSE_repeat* repeatMode, const uint * count, const uint max,
    const size_t mostFrequent, size_t nbSeq, const uint FSELog, const FSE_CTable * prevCTable,
    const short * defaultNorm, uint32 defaultNormLog, ZSTD_defaultPolicy_e const isDefaultAllowed, ZSTD_strategy const strategy)
{
	ZSTD_STATIC_ASSERT(ZSTD_defaultDisallowed == 0 && ZSTD_defaultAllowed != 0);
	if(mostFrequent == nbSeq) {
		*repeatMode = FSE_repeat_none;
		if(isDefaultAllowed && nbSeq <= 2) {
			/* Prefer set_basic over set_rle when there are 2 or fewer symbols,
			 * since RLE uses 1 byte, but set_basic uses 5-6 bits per symbol.
			 * If basic encoding isn't possible, always choose RLE.
			 */
			DEBUGLOG(5, "Selected set_basic");
			return set_basic;
		}
		DEBUGLOG(5, "Selected set_rle");
		return set_rle;
	}
	if(strategy < ZSTD_lazy) {
		if(isDefaultAllowed) {
			const size_t staticFse_nbSeq_max = 1000;
			const size_t mult = 10 - strategy;
			const size_t baseLog = 3;
			const size_t dynamicFse_nbSeq_min = (((size_t)1 << defaultNormLog) * mult) >> baseLog; // 28-36 for offset, 56-72 for lengths 
			assert(defaultNormLog >= 5 && defaultNormLog <= 6); /* xx_DEFAULTNORMLOG */
			assert(mult <= 9 && mult >= 7);
			if((*repeatMode == FSE_repeat_valid) && (nbSeq < staticFse_nbSeq_max)) {
				DEBUGLOG(5, "Selected set_repeat");
				return set_repeat;
			}
			if((nbSeq < dynamicFse_nbSeq_min) || (mostFrequent < (nbSeq >> (defaultNormLog-1)))) {
				DEBUGLOG(5, "Selected set_basic");
				/* The format allows default tables to be repeated, but it isn't useful.
				 * When using simple heuristics to select encoding type, we don't want
				 * to confuse these tables with dictionaries. When running more careful
				 * analysis, we don't need to waste time checking both repeating tables
				 * and default tables.
				 */
				*repeatMode = FSE_repeat_none;
				return set_basic;
			}
		}
	}
	else {
		const size_t basicCost = isDefaultAllowed ? ZSTD_crossEntropyCost(defaultNorm, defaultNormLog, count, max) : ERROR(GENERIC);
		const size_t repeatCost = *repeatMode != FSE_repeat_none ? ZSTD_fseBitCost(prevCTable, count, max) : ERROR(GENERIC);
		const size_t NCountCost = ZSTD_NCountCost(count, max, nbSeq, FSELog);
		const size_t compressedCost = (NCountCost << 3) + ZSTD_entropyCost(count, max, nbSeq);
		if(isDefaultAllowed) {
			assert(!ZSTD_isError(basicCost));
			assert(!(*repeatMode == FSE_repeat_valid && ZSTD_isError(repeatCost)));
		}
		assert(!ZSTD_isError(NCountCost));
		assert(compressedCost < ERROR(maxCode));
		DEBUGLOG(5, "Estimated bit costs: basic=%u\trepeat=%u\tcompressed=%u", (uint)basicCost, (uint)repeatCost, (uint)compressedCost);
		if(basicCost <= repeatCost && basicCost <= compressedCost) {
			DEBUGLOG(5, "Selected set_basic");
			assert(isDefaultAllowed);
			*repeatMode = FSE_repeat_none;
			return set_basic;
		}
		if(repeatCost <= compressedCost) {
			DEBUGLOG(5, "Selected set_repeat");
			assert(!ZSTD_isError(repeatCost));
			return set_repeat;
		}
		assert(compressedCost < basicCost && compressedCost < repeatCost);
	}
	DEBUGLOG(5, "Selected set_compressed");
	*repeatMode = FSE_repeat_check;
	return set_compressed;
}

typedef struct {
	int16 norm[MaxSeq + 1];
	uint32 wksp[FSE_BUILD_CTABLE_WORKSPACE_SIZE_U32(MaxSeq, MaxFSELog)];
} ZSTD_BuildCTableWksp;

size_t ZSTD_buildCTable(void * dst, size_t dstCapacity, FSE_CTable* nextCTable, uint32 FSELog, symbolEncodingType_e type,
    uint * count, uint32 max, const BYTE* codeTable, size_t nbSeq, const int16 * defaultNorm, uint32 defaultNormLog, uint32 defaultMax,
    const FSE_CTable* prevCTable, size_t prevCTableSize, void * entropyWorkspace, size_t entropyWorkspaceSize)
{
	BYTE* op = (BYTE*)dst;
	const BYTE* const oend = op + dstCapacity;
	DEBUGLOG(6, "ZSTD_buildCTable (dstCapacity=%u)", (uint)dstCapacity);
	switch(type) {
		case set_rle:
		    FORWARD_IF_ERROR(FSE_buildCTable_rle(nextCTable, (BYTE)max), "");
		    RETURN_ERROR_IF(dstCapacity==0, dstSize_tooSmall, "not enough space");
		    *op = codeTable[0];
		    return 1;
		case set_repeat:
		    memcpy(nextCTable, prevCTable, prevCTableSize);
		    return 0;
		case set_basic:
		    FORWARD_IF_ERROR(FSE_buildCTable_wksp(nextCTable, defaultNorm, defaultMax, defaultNormLog, entropyWorkspace,
			entropyWorkspaceSize), ""); // note : could be pre-calculated
		    return 0;
		case set_compressed: {
		    ZSTD_BuildCTableWksp* wksp = (ZSTD_BuildCTableWksp*)entropyWorkspace;
		    size_t nbSeq_1 = nbSeq;
		    const uint32 tableLog = FSE_optimalTableLog(FSELog, nbSeq, max);
		    if(count[codeTable[nbSeq-1]] > 1) {
			    count[codeTable[nbSeq-1]]--;
			    nbSeq_1--;
		    }
		    assert(nbSeq_1 > 1);
		    assert(entropyWorkspaceSize >= sizeof(ZSTD_BuildCTableWksp));
		    (void)entropyWorkspaceSize;
		    FORWARD_IF_ERROR(FSE_normalizeCount(wksp->norm, tableLog, count, nbSeq_1, max, ZSTD_useLowProbCount(nbSeq_1)),
			"FSE_normalizeCount failed");
		    assert(oend >= op);
		    {   
				const size_t NCountSize = FSE_writeNCount(op, (size_t)(oend - op), wksp->norm, max, tableLog); // overflow protected 
				FORWARD_IF_ERROR(NCountSize, "FSE_writeNCount failed");
				FORWARD_IF_ERROR(FSE_buildCTable_wksp(nextCTable, wksp->norm, max, tableLog, wksp->wksp, sizeof(wksp->wksp)), "FSE_buildCTable_wksp failed");
				return NCountSize;
			}
	    }
		default: assert(0); RETURN_ERROR(GENERIC, "impossible to reach");
	}
}

FORCE_INLINE_TEMPLATE size_t ZSTD_encodeSequences_body(void * dst, size_t dstCapacity,
    FSE_CTable const* CTable_MatchLength, BYTE const* mlCodeTable,
    FSE_CTable const* CTable_OffsetBits, BYTE const* ofCodeTable,
    FSE_CTable const* CTable_LitLength, BYTE const* llCodeTable,
    seqDef const* sequences, size_t nbSeq, int longOffsets)
{
	BIT_CStream_t blockStream;
	FSE_CState_t stateMatchLength;
	FSE_CState_t stateOffsetBits;
	FSE_CState_t stateLitLength;
	RETURN_ERROR_IF(ERR_isError(BIT_initCStream(&blockStream, dst, dstCapacity)), dstSize_tooSmall, "not enough space remaining");
	DEBUGLOG(6, "available space for bitstream : %i  (dstCapacity=%u)", (int)(blockStream.endPtr - blockStream.startPtr), (uint)dstCapacity);
	/* first symbols */
	FSE_initCState2(&stateMatchLength, CTable_MatchLength, mlCodeTable[nbSeq-1]);
	FSE_initCState2(&stateOffsetBits,  CTable_OffsetBits,  ofCodeTable[nbSeq-1]);
	FSE_initCState2(&stateLitLength,   CTable_LitLength,   llCodeTable[nbSeq-1]);
	BIT_addBits(&blockStream, sequences[nbSeq-1].litLength, LL_bits[llCodeTable[nbSeq-1]]);
	if(MEM_32bits()) BIT_flushBits(&blockStream);
	BIT_addBits(&blockStream, sequences[nbSeq-1].mlBase, ML_bits[mlCodeTable[nbSeq-1]]);
	if(MEM_32bits()) BIT_flushBits(&blockStream);
	if(longOffsets) {
		const uint32 ofBits = ofCodeTable[nbSeq-1];
		const uint extraBits = ofBits - MIN(ofBits, STREAM_ACCUMULATOR_MIN-1);
		if(extraBits) {
			BIT_addBits(&blockStream, sequences[nbSeq-1].offBase, extraBits);
			BIT_flushBits(&blockStream);
		}
		BIT_addBits(&blockStream, sequences[nbSeq-1].offBase >> extraBits,
		    ofBits - extraBits);
	}
	else {
		BIT_addBits(&blockStream, sequences[nbSeq-1].offBase, ofCodeTable[nbSeq-1]);
	}
	BIT_flushBits(&blockStream);
	{   
		size_t n;
	    for(n = nbSeq-2; n<nbSeq; n--) {   /* intentional underflow */
		    BYTE const llCode = llCodeTable[n];
		    BYTE const ofCode = ofCodeTable[n];
		    BYTE const mlCode = mlCodeTable[n];
		    const uint32 llBits = LL_bits[llCode];
		    const uint32 ofBits = ofCode;
		    const uint32 mlBits = ML_bits[mlCode];
		    DEBUGLOG(6, "encoding: litlen:%2u - matchlen:%2u - offCode:%7u",
			(uint)sequences[n].litLength,
			(uint)sequences[n].mlBase + MINMATCH,
			(uint)sequences[n].offBase);
		    /* 32b*/  /* 64b*/
		    /* (7)*/  /* (7)*/
		    FSE_encodeSymbol(&blockStream, &stateOffsetBits, ofCode); /* 15 */  /* 15 */
		    FSE_encodeSymbol(&blockStream, &stateMatchLength, mlCode); /* 24 */  /* 24 */
		    if(MEM_32bits()) BIT_flushBits(&blockStream);           /* (7)*/
		    FSE_encodeSymbol(&blockStream, &stateLitLength, llCode); /* 16 */  /* 33 */
		    if(MEM_32bits() || (ofBits+mlBits+llBits >= 64-7-(LLFSELog+MLFSELog+OffFSELog)))
			    BIT_flushBits(&blockStream);                    /* (7)*/
		    BIT_addBits(&blockStream, sequences[n].litLength, llBits);
		    if(MEM_32bits() && ((llBits+mlBits)>24)) BIT_flushBits(&blockStream);
		    BIT_addBits(&blockStream, sequences[n].mlBase, mlBits);
		    if(MEM_32bits() || (ofBits+mlBits+llBits > 56)) BIT_flushBits(&blockStream);
		    if(longOffsets) {
			    const uint extraBits = ofBits - MIN(ofBits, STREAM_ACCUMULATOR_MIN-1);
			    if(extraBits) {
				    BIT_addBits(&blockStream, sequences[n].offBase, extraBits);
				    BIT_flushBits(&blockStream);            /* (7)*/
			    }
			    BIT_addBits(&blockStream, sequences[n].offBase >> extraBits,
				ofBits - extraBits);                        /* 31 */
		    }
		    else {
			    BIT_addBits(&blockStream, sequences[n].offBase, ofBits); /* 31 */
		    }
		    BIT_flushBits(&blockStream);                            /* (7)*/
		    DEBUGLOG(7, "remaining space : %i", (int)(blockStream.endPtr - blockStream.ptr));
	    }
	}

	DEBUGLOG(6, "ZSTD_encodeSequences: flushing ML state with %u bits", stateMatchLength.stateLog);
	FSE_flushCState(&blockStream, &stateMatchLength);
	DEBUGLOG(6, "ZSTD_encodeSequences: flushing Off state with %u bits", stateOffsetBits.stateLog);
	FSE_flushCState(&blockStream, &stateOffsetBits);
	DEBUGLOG(6, "ZSTD_encodeSequences: flushing LL state with %u bits", stateLitLength.stateLog);
	FSE_flushCState(&blockStream, &stateLitLength);
	{   
		const size_t streamSize = BIT_closeCStream(&blockStream);
	    RETURN_ERROR_IF(streamSize==0, dstSize_tooSmall, "not enough space");
	    return streamSize;
	}
}

static size_t ZSTD_encodeSequences_default(void * dst, size_t dstCapacity,
    FSE_CTable const* CTable_MatchLength, BYTE const* mlCodeTable,
    FSE_CTable const* CTable_OffsetBits, BYTE const* ofCodeTable,
    FSE_CTable const* CTable_LitLength, BYTE const* llCodeTable,
    seqDef const* sequences, size_t nbSeq, int longOffsets)
{
	return ZSTD_encodeSequences_body(dst, dstCapacity,
		   CTable_MatchLength, mlCodeTable,
		   CTable_OffsetBits, ofCodeTable,
		   CTable_LitLength, llCodeTable,
		   sequences, nbSeq, longOffsets);
}

#if DYNAMIC_BMI2

static BMI2_TARGET_ATTRIBUTE size_t ZSTD_encodeSequences_bmi2(void * dst, size_t dstCapacity,
    FSE_CTable const* CTable_MatchLength, BYTE const* mlCodeTable,
    FSE_CTable const* CTable_OffsetBits, BYTE const* ofCodeTable,
    FSE_CTable const* CTable_LitLength, BYTE const* llCodeTable,
    seqDef const* sequences, size_t nbSeq, int longOffsets)
{
	return ZSTD_encodeSequences_body(dst, dstCapacity,
		   CTable_MatchLength, mlCodeTable,
		   CTable_OffsetBits, ofCodeTable,
		   CTable_LitLength, llCodeTable,
		   sequences, nbSeq, longOffsets);
}

#endif

size_t ZSTD_encodeSequences(void * dst, size_t dstCapacity,
    FSE_CTable const* CTable_MatchLength, BYTE const* mlCodeTable,
    FSE_CTable const* CTable_OffsetBits, BYTE const* ofCodeTable,
    FSE_CTable const* CTable_LitLength, BYTE const* llCodeTable,
    seqDef const* sequences, size_t nbSeq, int longOffsets, int bmi2)
{
	DEBUGLOG(5, "ZSTD_encodeSequences: dstCapacity = %u", (uint)dstCapacity);
#if DYNAMIC_BMI2
	if(bmi2) {
		return ZSTD_encodeSequences_bmi2(dst, dstCapacity, CTable_MatchLength, mlCodeTable, CTable_OffsetBits, 
			ofCodeTable, CTable_LitLength, llCodeTable, sequences, nbSeq, longOffsets);
	}
#endif
	(void)bmi2;
	return ZSTD_encodeSequences_default(dst, dstCapacity, CTable_MatchLength, mlCodeTable, CTable_OffsetBits, 
		ofCodeTable, CTable_LitLength, llCodeTable, sequences, nbSeq, longOffsets);
}
