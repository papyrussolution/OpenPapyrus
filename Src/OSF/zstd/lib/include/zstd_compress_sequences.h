/*
 * Copyright (c) Yann Collet, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */
#ifndef ZSTD_COMPRESS_SEQUENCES_H
#define ZSTD_COMPRESS_SEQUENCES_H

#include <fse.h> /* FSE_repeat, FSE_CTable */
#include <zstd_internal.h> /* symbolEncodingType_e, ZSTD_strategy */

typedef enum {
	ZSTD_defaultDisallowed = 0,
	ZSTD_defaultAllowed = 1
} ZSTD_defaultPolicy_e;

symbolEncodingType_e ZSTD_selectEncodingType(FSE_repeat* repeatMode, const uint * count, const uint max,
    const size_t mostFrequent, size_t nbSeq, const uint FSELog, const FSE_CTable * prevCTable,
    const short * defaultNorm, uint32 defaultNormLog, ZSTD_defaultPolicy_e const isDefaultAllowed, ZSTD_strategy const strategy);
size_t ZSTD_buildCTable(void* dst, size_t dstCapacity, FSE_CTable* nextCTable, uint32 FSELog, symbolEncodingType_e type,
    uint * count, uint32 max, const BYTE* codeTable, size_t nbSeq, const int16* defaultNorm, uint32 defaultNormLog, uint32 defaultMax,
    const FSE_CTable* prevCTable, size_t prevCTableSize, void* entropyWorkspace, size_t entropyWorkspaceSize);
size_t ZSTD_encodeSequences(void* dst, size_t dstCapacity, FSE_CTable const* CTable_MatchLength, BYTE const* mlCodeTable,
    FSE_CTable const* CTable_OffsetBits, BYTE const* ofCodeTable, FSE_CTable const* CTable_LitLength, BYTE const* llCodeTable,
    seqDef const* sequences, size_t nbSeq, int longOffsets, int bmi2);
size_t ZSTD_fseBitCost(const FSE_CTable * ctable, const uint * count, const uint max);
size_t ZSTD_crossEntropyCost(const short * norm, uint accuracyLog, const uint * count, const uint max);
#endif /* ZSTD_COMPRESS_SEQUENCES_H */
