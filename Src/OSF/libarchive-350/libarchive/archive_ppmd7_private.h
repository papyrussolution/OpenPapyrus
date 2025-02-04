/* Ppmd7.h -- PPMdH compression codec
   2010-03-12 : Igor Pavlov : Public domain
   This code is based on PPMd var.H (2001): Dmitry Shkarin : Public domain */

/* This code supports virtual RangeDecoder and includes the implementation
   of RangeCoder from 7z, instead of RangeCoder from original PPMd var.H.
   If you need the compatibility with original PPMd var.H, you can use external RangeDecoder */

#ifndef ARCHIVE_PPMD7_PRIVATE_H_INCLUDED
#define ARCHIVE_PPMD7_PRIVATE_H_INCLUDED

#ifndef __LIBARCHIVE_BUILD
	#error This header is only to be used internally to libarchive.
#endif

#include "archive_ppmd_private.h"

#define PPMD7_MIN_ORDER 2
#define PPMD7_MAX_ORDER 64

#define PPMD7_MIN_MEM_SIZE (1 << 11)
#define PPMD7_MAX_MEM_SIZE (0xFFFFFFFFu - 12 * 3)

struct CPpmd7_Context_;

typedef
  #ifdef PPMD_32BIT
    struct CPpmd7_Context_ *
  #else
    UInt32
  #endif
    CPpmd7_Context_Ref;

typedef struct CPpmd7_Context_ {
	UInt16 NumStats;
	UInt16 SummFreq;
	CPpmd_State_Ref Stats;
	CPpmd7_Context_Ref Suffix;
} CPpmd7_Context;

#define Ppmd7Context_OneState(p) ((CPpmd_State*)&(p)->SummFreq)

typedef struct {
	CPpmd7_Context * MinContext;
	CPpmd7_Context * MaxContext;
	CPpmd_State * FoundState;
	uint   OrderFall;
	uint   InitEsc;
	uint   PrevSuccess;
	uint   MaxOrder;
	uint   HiBitsFlag;
	Int32  RunLength, InitRL; /* must be 32-bit at least */
	UInt32 Size;
	UInt32 GlueCount;
	Byte * Base;
	Byte * LoUnit;
	Byte * HiUnit;
	Byte * Text;
	Byte * UnitsStart;
	UInt32 AlignOffset;
	Byte Indx2Units[PPMD_NUM_INDEXES];
	Byte Units2Indx[128];
	CPpmd_Void_Ref FreeList[PPMD_NUM_INDEXES];
	Byte NS2Indx[256], NS2BSIndx[256], HB2Flag[256];
	CPpmd_See DummySee, See[25][16];
	UInt16 BinSumm[128][64];
} CPpmd7;

/* ---------- Decode ---------- */

typedef struct {
	UInt32 (* GetThreshold)(void * p, UInt32 total);
	void (* Decode)(void * p, UInt32 start, UInt32 size);
	UInt32 (* DecodeBit)(void * p, UInt32 size0);
} IPpmd7_RangeDec;

typedef struct {
	IPpmd7_RangeDec p;
	UInt32 Range;
	UInt32 Code;
	UInt32 Low;
	UInt32 Bottom;
	IByteIn * Stream;
} CPpmd7z_RangeDec;

/* ---------- Encode ---------- */

typedef struct {
	uint64 Low;
	UInt32 Range;
	Byte Cache;
	uint64 CacheSize;
	IByteOut * Stream;
} CPpmd7z_RangeEnc;

typedef struct {
	/* Base Functions */
	void (* Ppmd7_Construct)(CPpmd7 * p);
	boolint (* Ppmd7_Alloc)(CPpmd7 * p, UInt32 size);
	void (* Ppmd7_Free)(CPpmd7 * p);
	void (* Ppmd7_Init)(CPpmd7 * p, uint maxOrder);
  #define Ppmd7_WasAllocated(p) ((p)->Base != NULL)

	/* Decode Functions */
	void (* Ppmd7z_RangeDec_CreateVTable)(CPpmd7z_RangeDec * p);
	void (* PpmdRAR_RangeDec_CreateVTable)(CPpmd7z_RangeDec * p);
	boolint (* Ppmd7z_RangeDec_Init)(CPpmd7z_RangeDec * p);
	boolint (* PpmdRAR_RangeDec_Init)(CPpmd7z_RangeDec * p);
  #define Ppmd7z_RangeDec_IsFinishedOK(p) ((p)->Code == 0)
	int (* Ppmd7_DecodeSymbol)(CPpmd7 * p, IPpmd7_RangeDec * rc);
	/* Encode Functions */
	void (* Ppmd7z_RangeEnc_Init)(CPpmd7z_RangeEnc * p);
	void (* Ppmd7z_RangeEnc_FlushData)(CPpmd7z_RangeEnc * p);
	void (* Ppmd7_EncodeSymbol)(CPpmd7 * p, CPpmd7z_RangeEnc * rc, int symbol);
} IPpmd7;

extern const IPpmd7 __archive_ppmd7_functions;
#endif
