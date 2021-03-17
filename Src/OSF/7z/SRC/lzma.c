// LZMA.C
// Igor Pavlov : Public domain 
//
#include <7z-internal.h>
#pragma hdrstop
//
// LzmaEnc.c -- LZMA Encoder
//
// #define SHOW_STAT 
// #define SHOW_STAT2 

#ifdef SHOW_STAT
	static unsigned g_STAT_OFFSET = 0;
#endif
#define kMaxHistorySize ((uint32)3 << 29)
//#define kMaxHistorySize ((uint32)7 << 29) 
#define kBlockSizeMax ((1 << LZMA_NUM_BLOCK_SIZE_BITS) - 1)
#define kBlockSize (9 << 10)
#define kUnpackBlockSize (1 << 18)
#define kMatchArraySize (1 << 21)
#define kMatchRecordMaxSize ((LZMA_MATCH_LEN_MAX * 2 + 3) * LZMA_MATCH_LEN_MAX)
#define kNumMaxDirectBits (31)
#define kNumTopBits 24
#define kTopValue ((uint32)1 << kNumTopBits)
#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)
#define kNumMoveBits 5
#define kProbInitValue (kBitModelTotal >> 1)
#define kNumMoveReducingBits 4
#define kNumBitPriceShiftBits 4
#define kBitPrice (1 << kNumBitPriceShiftBits)

void FASTCALL LzmaEncProps_Init(CLzmaEncProps * p)
{
	p->level = 5;
	p->dictSize = p->mc = 0;
	p->reduceSize = static_cast<uint64>(-1LL);
	p->lc = p->lp = p->pb = p->algo = p->fb = p->btMode = p->numHashBytes = p->numThreads = -1;
	p->writeEndMark = 0;
}

void FASTCALL LzmaEncProps_Normalize(CLzmaEncProps * p)
{
	int level = p->level;
	if(level < 0) 
		level = 5;
	p->level = level;
	if(p->dictSize == 0) 
		p->dictSize = (level <= 5 ? (1 << (level * 2 + 14)) : (level == 6 ? (1 << 25) : (1 << 26)));
	if(p->dictSize > p->reduceSize) {
		for(uint i = 11; i <= 30; i++) {
			if((uint32)p->reduceSize <= ((uint32)2 << i)) {
				p->dictSize = ((uint32)2 << i); break;
			}
			if((uint32)p->reduceSize <= ((uint32)3 << i)) {
				p->dictSize = ((uint32)3 << i); break;
			}
		}
	}
	if(p->lc < 0) p->lc = 3;
	if(p->lp < 0) p->lp = 0;
	if(p->pb < 0) p->pb = 2;

	if(p->algo < 0) p->algo = (level < 5 ? 0 : 1);
	if(p->fb < 0) p->fb = (level < 7 ? 32 : 64);
	if(p->btMode < 0) p->btMode = (p->algo == 0 ? 0 : 1);
	if(p->numHashBytes < 0) p->numHashBytes = 4;
	if(p->mc == 0) p->mc = (16 + (p->fb >> 1)) >> (p->btMode ? 0 : 1);

	if(p->numThreads < 0)
		p->numThreads =
      #ifndef _7ZIP_ST
		    ((p->btMode && p->algo) ? 2 : 1);
      #else
		    1;
      #endif
}

uint32 LzmaEncProps_GetDictSize(const CLzmaEncProps * props2)
{
	CLzmaEncProps props = *props2;
	LzmaEncProps_Normalize(&props);
	return props.dictSize;
}

#if(_MSC_VER >= 1400)
/* BSR code is fast for some new CPUs */
/* #define LZMA_LOG_BSR */
#endif

#ifdef LZMA_LOG_BSR

#define kDicLogSizeMaxCompress 32

#define BSR2_RET(pos, res) { unsigned long zz; _BitScanReverse(&zz, (pos)); res = (zz + zz) + ((pos >> (zz - 1)) & 1); }

static uint32 GetPosSlot1(uint32 pos)
{
	uint32 res;
	BSR2_RET(pos, res);
	return res;
}

#define GetPosSlot2(pos, res) { BSR2_RET(pos, res); }
#define GetPosSlot(pos, res) { if(pos < 2) res = pos; else BSR2_RET(pos, res); }

#else

#define kNumLogBits (9 + sizeof(size_t) / 2)
/* #define kNumLogBits (11 + sizeof(size_t) / 8 * 3) */

#define kDicLogSizeMaxCompress ((kNumLogBits - 1) * 2 + 7)

static void LzmaEnc_FastPosInit(Byte * g_FastPos)
{
	unsigned slot;
	g_FastPos[0] = 0;
	g_FastPos[1] = 1;
	g_FastPos += 2;
	for(slot = 2; slot < kNumLogBits * 2; slot++) {
		size_t k = ((size_t)1 << ((slot >> 1) - 1));
		size_t j;
		for(j = 0; j < k; j++)
			g_FastPos[j] = (Byte)slot;
		g_FastPos += k;
	}
}

/* we can use ((limit - pos) >> 31) only if(pos < ((uint32)1 << 31)) */
/*
   #define BSR2_RET(pos, res) { uint32 zz = 6 + ((kNumLogBits - 1) & \
   (0 - (((((uint32)1 << (kNumLogBits + 6)) - 1) - pos) >> 31))); \
   res = p->g_FastPos[pos >> zz] + (zz * 2); }
 */

/*
   #define BSR2_RET(pos, res) { uint32 zz = 6 + ((kNumLogBits - 1) & \
   (0 - (((((uint32)1 << (kNumLogBits)) - 1) - (pos >> 6)) >> 31))); \
   res = p->g_FastPos[pos >> zz] + (zz * 2); }
 */

#define BSR2_RET(pos, res) { uint32 zz = (pos < (1 << (kNumLogBits + 6))) ? 6 : 6 + kNumLogBits - 1; \
			     res = p->g_FastPos[pos >> zz] + (zz * 2); }

/*
   #define BSR2_RET(pos, res) { res = (pos < (1 << (kNumLogBits + 6))) ? \
   p->g_FastPos[pos >> 6] + 12 : \
   p->g_FastPos[pos >> (6 + kNumLogBits - 1)] + (6 + (kNumLogBits - 1)) * 2; }
 */

#define GetPosSlot1(pos) p->g_FastPos[pos]
#define GetPosSlot2(pos, res) { BSR2_RET(pos, res); }
#define GetPosSlot(pos, res) { if(pos < kNumFullDistances) res = p->g_FastPos[pos]; else BSR2_RET(pos, res); }

#endif

#define LZMA_NUM_REPS 4

typedef unsigned CState;

typedef struct {
	uint32 price;
	CState state;
	int    prev1IsChar;
	int    prev2;
	uint32 posPrev2;
	uint32 backPrev2;
	uint32 posPrev;
	uint32 backPrev;
	uint32 backs[LZMA_NUM_REPS];
} COptimal;

#define kNumOpts (1 << 12)
#define kNumLenToPosStates 4
#define kNumPosSlotBits 6
#define kDicLogSizeMin 0
#define kDicLogSizeMax 32
#define kDistTableSizeMax (kDicLogSizeMax * 2)
#define kNumAlignBits 4
#define kAlignTableSize (1 << kNumAlignBits)
#define kAlignMask (kAlignTableSize - 1)
#define kStartPosModelIndex 4
#define kEndPosModelIndex 14
#define kNumPosModels (kEndPosModelIndex - kStartPosModelIndex)
#define kNumFullDistances (1 << (kEndPosModelIndex >> 1))
#ifdef _LZMA_PROB32
	#define CLzmaProb uint32
#else
	#define CLzmaProb uint16
#endif
#define LZMA_PB_MAX 4
#define LZMA_LC_MAX 8
#define LZMA_LP_MAX 4
#define LZMA_NUM_PB_STATES_MAX (1 << LZMA_PB_MAX)
#define kLenNumLowBits 3
#define kLenNumLowSymbols (1 << kLenNumLowBits)
#define kLenNumMidBits 3
#define kLenNumMidSymbols (1 << kLenNumMidBits)
#define kLenNumHighBits 8
#define kLenNumHighSymbols (1 << kLenNumHighBits)
#define kLenNumSymbolsTotal (kLenNumLowSymbols + kLenNumMidSymbols + kLenNumHighSymbols)
#define LZMA_MATCH_LEN_MIN 2
#define LZMA_MATCH_LEN_MAX (LZMA_MATCH_LEN_MIN + kLenNumSymbolsTotal - 1)
#define kNumStates 12

typedef struct {
	CLzmaProb choice;
	CLzmaProb choice2;
	CLzmaProb low[LZMA_NUM_PB_STATES_MAX << kLenNumLowBits];
	CLzmaProb mid[LZMA_NUM_PB_STATES_MAX << kLenNumMidBits];
	CLzmaProb high[kLenNumHighSymbols];
} CLenEnc;

typedef struct {
	CLenEnc p;
	uint32 tableSize;
	uint32 prices[LZMA_NUM_PB_STATES_MAX][kLenNumSymbolsTotal];
	uint32 counters[LZMA_NUM_PB_STATES_MAX];
} CLenPriceEnc;

typedef struct {
	uint32 range;
	Byte cache;
	uint64 low;
	uint64 cacheSize;
	Byte * buf;
	Byte * bufLim;
	Byte * bufBase;
	ISeqOutStream * outStream;
	uint64 processed;
	SRes res;
} CRangeEnc;

typedef struct {
	CLzmaProb * litProbs;

	uint32 state;
	uint32 reps[LZMA_NUM_REPS];

	CLzmaProb isMatch[kNumStates][LZMA_NUM_PB_STATES_MAX];
	CLzmaProb isRep[kNumStates];
	CLzmaProb isRepG0[kNumStates];
	CLzmaProb isRepG1[kNumStates];
	CLzmaProb isRepG2[kNumStates];
	CLzmaProb isRep0Long[kNumStates][LZMA_NUM_PB_STATES_MAX];

	CLzmaProb posSlotEncoder[kNumLenToPosStates][1 << kNumPosSlotBits];
	CLzmaProb posEncoders[kNumFullDistances - kEndPosModelIndex];
	CLzmaProb posAlignEncoder[1 << kNumAlignBits];

	CLenPriceEnc lenEnc;
	CLenPriceEnc repLenEnc;
} CSaveState;

typedef struct {
	void * matchFinderObj;
	IMatchFinder matchFinder;
	uint32 optimumEndIndex;
	uint32 optimumCurrentIndex;
	uint32 longestMatchLength;
	uint32 numPairs;
	uint32 numAvail;
	uint32 numFastBytes;
	uint32 additionalOffset;
	uint32 reps[LZMA_NUM_REPS];
	uint32 state;
	uint   lc;
	uint   lp;
	uint   pb;
	uint   lpMask;
	uint   pbMask;
	uint   lclp;
	CLzmaProb * litProbs;
	Bool   fastMode;
	Bool   writeEndMark;
	Bool   finished;
	Bool   multiThread;
	Bool   needInit;
	uint64 nowPos64;
	uint32 matchPriceCount;
	uint32 alignPriceCount;
	uint32 distTableSize;
	uint32 dictSize;
	SRes result;
	CRangeEnc rc;
  #ifndef _7ZIP_ST
	Bool mtMode;
	CMatchFinderMt matchFinderMt;
  #endif
	CMatchFinder matchFinderBase;
  #ifndef _7ZIP_ST
	Byte pad[128];
  #endif
	COptimal opt[kNumOpts];
  #ifndef LZMA_LOG_BSR
	Byte g_FastPos[1 << kNumLogBits];
  #endif
	uint32 ProbPrices[kBitModelTotal >> kNumMoveReducingBits];
	uint32 matches[LZMA_MATCH_LEN_MAX * 2 + 2 + 1];
	uint32 posSlotPrices[kNumLenToPosStates][kDistTableSizeMax];
	uint32 distancesPrices[kNumLenToPosStates][kNumFullDistances];
	uint32 alignPrices[kAlignTableSize];
	CLzmaProb isMatch[kNumStates][LZMA_NUM_PB_STATES_MAX];
	CLzmaProb isRep[kNumStates];
	CLzmaProb isRepG0[kNumStates];
	CLzmaProb isRepG1[kNumStates];
	CLzmaProb isRepG2[kNumStates];
	CLzmaProb isRep0Long[kNumStates][LZMA_NUM_PB_STATES_MAX];
	CLzmaProb posSlotEncoder[kNumLenToPosStates][1 << kNumPosSlotBits];
	CLzmaProb posEncoders[kNumFullDistances - kEndPosModelIndex];
	CLzmaProb posAlignEncoder[1 << kNumAlignBits];
	CLenPriceEnc lenEnc;
	CLenPriceEnc repLenEnc;
	CSaveState saveState;
  #ifndef _7ZIP_ST
	Byte pad2[128];
  #endif
} CLzmaEnc;

void LzmaEnc_SaveState(CLzmaEncHandle pp)
{
	CLzmaEnc * p = (CLzmaEnc*)pp;
	CSaveState * dest = &p->saveState;
	int i;
	dest->lenEnc = p->lenEnc;
	dest->repLenEnc = p->repLenEnc;
	dest->state = p->state;

	for(i = 0; i < kNumStates; i++) {
		memcpy(dest->isMatch[i], p->isMatch[i], sizeof(p->isMatch[i]));
		memcpy(dest->isRep0Long[i], p->isRep0Long[i], sizeof(p->isRep0Long[i]));
	}
	for(i = 0; i < kNumLenToPosStates; i++)
		memcpy(dest->posSlotEncoder[i], p->posSlotEncoder[i], sizeof(p->posSlotEncoder[i]));
	memcpy(dest->isRep, p->isRep, sizeof(p->isRep));
	memcpy(dest->isRepG0, p->isRepG0, sizeof(p->isRepG0));
	memcpy(dest->isRepG1, p->isRepG1, sizeof(p->isRepG1));
	memcpy(dest->isRepG2, p->isRepG2, sizeof(p->isRepG2));
	memcpy(dest->posEncoders, p->posEncoders, sizeof(p->posEncoders));
	memcpy(dest->posAlignEncoder, p->posAlignEncoder, sizeof(p->posAlignEncoder));
	memcpy(dest->reps, p->reps, sizeof(p->reps));
	memcpy(dest->litProbs, p->litProbs, ((uint32)0x300 << p->lclp) * sizeof(CLzmaProb));
}

void LzmaEnc_RestoreState(CLzmaEncHandle pp)
{
	CLzmaEnc * dest = (CLzmaEnc*)pp;
	const CSaveState * p = &dest->saveState;
	int i;
	dest->lenEnc = p->lenEnc;
	dest->repLenEnc = p->repLenEnc;
	dest->state = p->state;
	for(i = 0; i < kNumStates; i++) {
		memcpy(dest->isMatch[i], p->isMatch[i], sizeof(p->isMatch[i]));
		memcpy(dest->isRep0Long[i], p->isRep0Long[i], sizeof(p->isRep0Long[i]));
	}
	for(i = 0; i < kNumLenToPosStates; i++)
		memcpy(dest->posSlotEncoder[i], p->posSlotEncoder[i], sizeof(p->posSlotEncoder[i]));
	memcpy(dest->isRep, p->isRep, sizeof(p->isRep));
	memcpy(dest->isRepG0, p->isRepG0, sizeof(p->isRepG0));
	memcpy(dest->isRepG1, p->isRepG1, sizeof(p->isRepG1));
	memcpy(dest->isRepG2, p->isRepG2, sizeof(p->isRepG2));
	memcpy(dest->posEncoders, p->posEncoders, sizeof(p->posEncoders));
	memcpy(dest->posAlignEncoder, p->posAlignEncoder, sizeof(p->posAlignEncoder));
	memcpy(dest->reps, p->reps, sizeof(p->reps));
	memcpy(dest->litProbs, p->litProbs, ((uint32)0x300 << dest->lclp) * sizeof(CLzmaProb));
}

SRes LzmaEnc_SetProps(CLzmaEncHandle pp, const CLzmaEncProps * props2)
{
	CLzmaEnc * p = (CLzmaEnc*)pp;
	CLzmaEncProps props = *props2;
	LzmaEncProps_Normalize(&props);

	if(props.lc > LZMA_LC_MAX || props.lp > LZMA_LP_MAX || props.pb > LZMA_PB_MAX || props.dictSize > ((uint64)1 << kDicLogSizeMaxCompress) || props.dictSize > kMaxHistorySize)
		return SZ_ERROR_PARAM;
	p->dictSize = props.dictSize;
	{
		uint   fb = props.fb;
		if(fb < 5)
			fb = 5;
		if(fb > LZMA_MATCH_LEN_MAX)
			fb = LZMA_MATCH_LEN_MAX;
		p->numFastBytes = fb;
	}
	p->lc = props.lc;
	p->lp = props.lp;
	p->pb = props.pb;
	p->fastMode = (props.algo == 0);
	p->matchFinderBase.btMode = (Byte)(props.btMode ? 1 : 0);
	{
		uint32 numHashBytes = 4;
		if(props.btMode) {
			if(props.numHashBytes < 2)
				numHashBytes = 2;
			else if(props.numHashBytes < 4)
				numHashBytes = props.numHashBytes;
		}
		p->matchFinderBase.numHashBytes = numHashBytes;
	}
	p->matchFinderBase.cutValue = props.mc;
	p->writeEndMark = props.writeEndMark;
  #ifndef _7ZIP_ST
	/*
	   if(newMultiThread != _multiThread)
	   {
	   ReleaseMatchFinder();
	   _multiThread = newMultiThread;
	   }
	 */
	p->multiThread = (props.numThreads > 1);
  #endif

	return SZ_OK;
}

static const int kLiteralNextStates[kNumStates] = {0, 0, 0, 0, 1, 2, 3, 4,  5,  6,   4, 5};
static const int kMatchNextStates[kNumStates]   = {7, 7, 7, 7, 7, 7, 7, 10, 10, 10, 10, 10};
static const int kRepNextStates[kNumStates]     = {8, 8, 8, 8, 8, 8, 8, 11, 11, 11, 11, 11};
static const int kShortRepNextStates[kNumStates] = {9, 9, 9, 9, 9, 9, 9, 11, 11, 11, 11, 11};

#define IsCharState(s) ((s) < 7)

#define GetLenToPosState(len) (((len) < kNumLenToPosStates + 1) ? (len) - 2 : kNumLenToPosStates - 1)

#define kInfinityPrice (1 << 30)

static void RangeEnc_Construct(CRangeEnc * p)
{
	p->outStream = NULL;
	p->bufBase = NULL;
}

#define RangeEnc_GetProcessed(p) ((p)->processed + ((p)->buf - (p)->bufBase) + (p)->cacheSize)

#define RC_BUF_SIZE (1 << 16)
static int RangeEnc_Alloc(CRangeEnc * p, ISzAllocPtr alloc)
{
	if(!p->bufBase) {
		p->bufBase = (Byte *)ISzAlloc_Alloc(alloc, RC_BUF_SIZE);
		if(!p->bufBase)
			return 0;
		p->bufLim = p->bufBase + RC_BUF_SIZE;
	}
	return 1;
}

static void RangeEnc_Free(CRangeEnc * p, ISzAllocPtr alloc)
{
	ISzAlloc_Free(alloc, p->bufBase);
	p->bufBase = 0;
}

static void RangeEnc_Init(CRangeEnc * p)
{
	/* Stream.Init(); */
	p->low = 0;
	p->range = 0xFFFFFFFF;
	p->cacheSize = 1;
	p->cache = 0;
	p->buf = p->bufBase;
	p->processed = 0;
	p->res = SZ_OK;
}

static void FASTCALL RangeEnc_FlushStream(CRangeEnc * p)
{
	if(p->res == SZ_OK) {
		size_t num = p->buf - p->bufBase;
		if(num != ISeqOutStream_Write(p->outStream, p->bufBase, num))
			p->res = SZ_ERROR_WRITE;
		p->processed += num;
		p->buf = p->bufBase;
	}
}

static void FASTCALL RangeEnc_ShiftLow(CRangeEnc * p)
{
	if((uint32)p->low < (uint32)0xFF000000 || (uint)(p->low >> 32) != 0) {
		Byte temp = p->cache;
		do {
			Byte * buf = p->buf;
			*buf++ = (Byte)(temp + (Byte)(p->low >> 32));
			p->buf = buf;
			if(buf == p->bufLim)
				RangeEnc_FlushStream(p);
			temp = 0xFF;
		} while(--p->cacheSize != 0);
		p->cache = (Byte)((uint32)p->low >> 24);
	}
	p->cacheSize++;
	p->low = (uint32)p->low << 8;
}

static void RangeEnc_FlushData(CRangeEnc * p)
{
	for(int i = 0; i < 5; i++)
		RangeEnc_ShiftLow(p);
}

static void FASTCALL RangeEnc_EncodeDirectBits(CRangeEnc * p, uint32 value, unsigned numBits)
{
	do {
		p->range >>= 1;
		p->low += p->range & (0 - ((value >> --numBits) & 1));
		if(p->range < kTopValue) {
			p->range <<= 8;
			RangeEnc_ShiftLow(p);
		}
	} while(numBits != 0);
}

static void FASTCALL RangeEnc_EncodeBit(CRangeEnc * p, CLzmaProb * prob, uint32 symbol)
{
	uint32 ttt = *prob;
	uint32 newBound = (p->range >> kNumBitModelTotalBits) * ttt;
	if(symbol == 0) {
		p->range = newBound;
		ttt += (kBitModelTotal - ttt) >> kNumMoveBits;
	}
	else {
		p->low += newBound;
		p->range -= newBound;
		ttt -= ttt >> kNumMoveBits;
	}
	*prob = (CLzmaProb)ttt;
	if(p->range < kTopValue) {
		p->range <<= 8;
		RangeEnc_ShiftLow(p);
	}
}

static void FASTCALL LitEnc_Encode(CRangeEnc * p, CLzmaProb * probs, uint32 symbol)
{
	symbol |= 0x100;
	do {
		RangeEnc_EncodeBit(p, probs + (symbol >> 8), (symbol >> 7) & 1);
		symbol <<= 1;
	} while(symbol < 0x10000);
}

static void FASTCALL LitEnc_EncodeMatched(CRangeEnc * p, CLzmaProb * probs, uint32 symbol, uint32 matchByte)
{
	uint32 offs = 0x100;
	symbol |= 0x100;
	do {
		matchByte <<= 1;
		RangeEnc_EncodeBit(p, probs + (offs + (matchByte & offs) + (symbol >> 8)), (symbol >> 7) & 1);
		symbol <<= 1;
		offs &= ~(matchByte ^ symbol);
	} while(symbol < 0x10000);
}

static void LzmaEnc_InitPriceTables(uint32 * ProbPrices)
{
	for(uint32 i = (1 << kNumMoveReducingBits) / 2; i < kBitModelTotal; i += (1 << kNumMoveReducingBits)) {
		const int kCyclesBits = kNumBitPriceShiftBits;
		uint32 w = i;
		uint32 bitCount = 0;
		int j;
		for(j = 0; j < kCyclesBits; j++) {
			w = w * w;
			bitCount <<= 1;
			while(w >= ((uint32)1 << 16)) {
				w >>= 1;
				bitCount++;
			}
		}
		ProbPrices[i >> kNumMoveReducingBits] = ((kNumBitModelTotalBits << kCyclesBits) - 15 - bitCount);
	}
}

#define GET_PRICE(prob, symbol)	p->ProbPrices[((prob) ^ (((-(int)(symbol))) & (kBitModelTotal - 1))) >> kNumMoveReducingBits];
#define GET_PRICEa(prob, symbol) ProbPrices[((prob) ^ ((-((int)(symbol))) & (kBitModelTotal - 1))) >> kNumMoveReducingBits];
#define GET_PRICE_0(prob) p->ProbPrices[(prob) >> kNumMoveReducingBits]
#define GET_PRICE_1(prob) p->ProbPrices[((prob) ^ (kBitModelTotal - 1)) >> kNumMoveReducingBits]
#define GET_PRICE_0a(prob) ProbPrices[(prob) >> kNumMoveReducingBits]
#define GET_PRICE_1a(prob) ProbPrices[((prob) ^ (kBitModelTotal - 1)) >> kNumMoveReducingBits]

static uint32 LitEnc_GetPrice(const CLzmaProb * probs, uint32 symbol, const uint32 * ProbPrices)
{
	uint32 price = 0;
	symbol |= 0x100;
	do {
		price += GET_PRICEa(probs[symbol >> 8], (symbol >> 7) & 1);
		symbol <<= 1;
	} while(symbol < 0x10000);
	return price;
}

static uint32 LitEnc_GetPriceMatched(const CLzmaProb * probs, uint32 symbol, uint32 matchByte, const uint32 * ProbPrices)
{
	uint32 price = 0;
	uint32 offs = 0x100;
	symbol |= 0x100;
	do {
		matchByte <<= 1;
		price += GET_PRICEa(probs[offs + (matchByte & offs) + (symbol >> 8)], (symbol >> 7) & 1);
		symbol <<= 1;
		offs &= ~(matchByte ^ symbol);
	} while(symbol < 0x10000);
	return price;
}

static void FASTCALL RcTree_Encode(CRangeEnc * rc, CLzmaProb * probs, int numBitLevels, uint32 symbol)
{
	uint32 m = 1;
	for(int i = numBitLevels; i != 0; ) {
		uint32 bit;
		i--;
		bit = (symbol >> i) & 1;
		RangeEnc_EncodeBit(rc, probs + m, bit);
		m = (m << 1) | bit;
	}
}

static void RcTree_ReverseEncode(CRangeEnc * rc, CLzmaProb * probs, int numBitLevels, uint32 symbol)
{
	uint32 m = 1;
	for(int i = 0; i < numBitLevels; i++) {
		uint32 bit = symbol & 1;
		RangeEnc_EncodeBit(rc, probs + m, bit);
		m = (m << 1) | bit;
		symbol >>= 1;
	}
}

static uint32 FASTCALL RcTree_GetPrice(const CLzmaProb * probs, int numBitLevels, uint32 symbol, const uint32 * ProbPrices)
{
	uint32 price = 0;
	symbol |= (1 << numBitLevels);
	while(symbol != 1) {
		price += GET_PRICEa(probs[symbol >> 1], symbol & 1);
		symbol >>= 1;
	}
	return price;
}

static uint32 FASTCALL RcTree_ReverseGetPrice(const CLzmaProb * probs, int numBitLevels, uint32 symbol, const uint32 * ProbPrices)
{
	uint32 price = 0;
	uint32 m = 1;
	for(int i = numBitLevels; i != 0; i--) {
		uint32 bit = symbol & 1;
		symbol >>= 1;
		price += GET_PRICEa(probs[m], bit);
		m = (m << 1) | bit;
	}
	return price;
}

static void FASTCALL LenEnc_Init(CLenEnc * p)
{
	uint i;
	p->choice = p->choice2 = kProbInitValue;
	for(i = 0; i < (LZMA_NUM_PB_STATES_MAX << kLenNumLowBits); i++)
		p->low[i] = kProbInitValue;
	for(i = 0; i < (LZMA_NUM_PB_STATES_MAX << kLenNumMidBits); i++)
		p->mid[i] = kProbInitValue;
	for(i = 0; i < kLenNumHighSymbols; i++)
		p->high[i] = kProbInitValue;
}

static void FASTCALL LenEnc_Encode(CLenEnc * p, CRangeEnc * rc, uint32 symbol, uint32 posState)
{
	if(symbol < kLenNumLowSymbols) {
		RangeEnc_EncodeBit(rc, &p->choice, 0);
		RcTree_Encode(rc, p->low + (posState << kLenNumLowBits), kLenNumLowBits, symbol);
	}
	else {
		RangeEnc_EncodeBit(rc, &p->choice, 1);
		if(symbol < kLenNumLowSymbols + kLenNumMidSymbols) {
			RangeEnc_EncodeBit(rc, &p->choice2, 0);
			RcTree_Encode(rc, p->mid + (posState << kLenNumMidBits), kLenNumMidBits, symbol - kLenNumLowSymbols);
		}
		else {
			RangeEnc_EncodeBit(rc, &p->choice2, 1);
			RcTree_Encode(rc, p->high, kLenNumHighBits, symbol - kLenNumLowSymbols - kLenNumMidSymbols);
		}
	}
}

static void LenEnc_SetPrices(const CLenEnc * p, uint32 posState, uint32 numSymbols, uint32 * prices, const uint32 * ProbPrices)
{
	uint32 a0 = GET_PRICE_0a(p->choice);
	uint32 a1 = GET_PRICE_1a(p->choice);
	uint32 b0 = a1 + GET_PRICE_0a(p->choice2);
	uint32 b1 = a1 + GET_PRICE_1a(p->choice2);
	uint32 i = 0;
	for(i = 0; i < kLenNumLowSymbols; i++) {
		if(i >= numSymbols)
			return;
		prices[i] = a0 + RcTree_GetPrice(p->low + (posState << kLenNumLowBits), kLenNumLowBits, i, ProbPrices);
	}
	for(; i < kLenNumLowSymbols + kLenNumMidSymbols; i++) {
		if(i >= numSymbols)
			return;
		prices[i] = b0 + RcTree_GetPrice(p->mid + (posState << kLenNumMidBits), kLenNumMidBits, i - kLenNumLowSymbols, ProbPrices);
	}
	for(; i < numSymbols; i++)
		prices[i] = b1 + RcTree_GetPrice(p->high, kLenNumHighBits, i - kLenNumLowSymbols - kLenNumMidSymbols, ProbPrices);
}

static void FASTCALL LenPriceEnc_UpdateTable(CLenPriceEnc * p, uint32 posState, const uint32 * ProbPrices)
{
	LenEnc_SetPrices(&p->p, posState, p->tableSize, p->prices[posState], ProbPrices);
	p->counters[posState] = p->tableSize;
}

static void LenPriceEnc_UpdateTables(CLenPriceEnc * p, uint32 numPosStates, const uint32 * ProbPrices)
{
	for(uint32 posState = 0; posState < numPosStates; posState++)
		LenPriceEnc_UpdateTable(p, posState, ProbPrices);
}

static void FASTCALL LenEnc_Encode2(CLenPriceEnc * p, CRangeEnc * rc, uint32 symbol, uint32 posState, Bool updatePrice, const uint32 * ProbPrices)
{
	LenEnc_Encode(&p->p, rc, symbol, posState);
	if(updatePrice)
		if(--p->counters[posState] == 0)
			LenPriceEnc_UpdateTable(p, posState, ProbPrices);
}

static void FASTCALL MovePos(CLzmaEnc * p, uint32 num)
{
  #ifdef SHOW_STAT
	g_STAT_OFFSET += num;
	printf("\n MovePos %u", num);
  #endif
	if(num != 0) {
		p->additionalOffset += num;
		p->matchFinder.Skip(p->matchFinderObj, num);
	}
}

static uint32 FASTCALL ReadMatchDistances(CLzmaEnc * p, uint32 * numDistancePairsRes)
{
	uint32 lenRes = 0, numPairs;
	p->numAvail = p->matchFinder.GetNumAvailableBytes(p->matchFinderObj);
	numPairs = p->matchFinder.GetMatches(p->matchFinderObj, p->matches);
  #ifdef SHOW_STAT
	printf("\n i = %u numPairs = %u    ", g_STAT_OFFSET, numPairs / 2);
	g_STAT_OFFSET++;
	{
		uint32 i;
		for(i = 0; i < numPairs; i += 2)
			printf("%2u %6u   | ", p->matches[i], p->matches[i+1]);
	}
  #endif
	if(numPairs > 0) {
		lenRes = p->matches[(size_t)numPairs - 2];
		if(lenRes == p->numFastBytes) {
			uint32 numAvail = p->numAvail;
			if(numAvail > LZMA_MATCH_LEN_MAX)
				numAvail = LZMA_MATCH_LEN_MAX;
			{
				const Byte * pbyCur = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
				const Byte * pby = pbyCur + lenRes;
				ptrdiff_t dif = (ptrdiff_t)-1 - p->matches[(size_t)numPairs - 1];
				const Byte * pbyLim = pbyCur + numAvail;
				for(; pby != pbyLim && *pby == pby[dif]; pby++) ;
				lenRes = (uint32)(pby - pbyCur);
			}
		}
	}
	p->additionalOffset++;
	*numDistancePairsRes = numPairs;
	return lenRes;
}

#define MakeAsChar(p) (p)->backPrev = (uint32)(-1); (p)->prev1IsChar = False;
#define MakeAsShortRep(p) (p)->backPrev = 0; (p)->prev1IsChar = False;
#define IsShortRep(p) ((p)->backPrev == 0)

static uint32 FASTCALL GetRepLen1Price(const CLzmaEnc * p, uint32 state, uint32 posState)
{
	return GET_PRICE_0(p->isRepG0[state]) + GET_PRICE_0(p->isRep0Long[state][posState]);
}

static uint32 FASTCALL GetPureRepPrice(const CLzmaEnc * p, uint32 repIndex, uint32 state, uint32 posState)
{
	uint32 price;
	if(repIndex == 0) {
		price = GET_PRICE_0(p->isRepG0[state]);
		price += GET_PRICE_1(p->isRep0Long[state][posState]);
	}
	else {
		price = GET_PRICE_1(p->isRepG0[state]);
		if(repIndex == 1)
			price += GET_PRICE_0(p->isRepG1[state]);
		else {
			price += GET_PRICE_1(p->isRepG1[state]);
			price += GET_PRICE(p->isRepG2[state], repIndex - 2);
		}
	}
	return price;
}

static uint32 FASTCALL GetRepPrice(CLzmaEnc * p, uint32 repIndex, uint32 len, uint32 state, uint32 posState)
{
	return p->repLenEnc.prices[posState][(size_t)len - LZMA_MATCH_LEN_MIN] + GetPureRepPrice(p, repIndex, state, posState);
}

static uint32 FASTCALL Backward(CLzmaEnc * p, uint32 * backRes, uint32 cur)
{
	uint32 posMem = p->opt[cur].posPrev;
	uint32 backMem = p->opt[cur].backPrev;
	p->optimumEndIndex = cur;
	do {
		if(p->opt[cur].prev1IsChar) {
			MakeAsChar(&p->opt[posMem])
			p->opt[posMem].posPrev = posMem - 1;
			if(p->opt[cur].prev2) {
				p->opt[(size_t)posMem - 1].prev1IsChar = False;
				p->opt[(size_t)posMem - 1].posPrev = p->opt[cur].posPrev2;
				p->opt[(size_t)posMem - 1].backPrev = p->opt[cur].backPrev2;
			}
		}
		{
			uint32 posPrev = posMem;
			uint32 backCur = backMem;
			backMem = p->opt[posPrev].backPrev;
			posMem = p->opt[posPrev].posPrev;
			p->opt[posPrev].backPrev = backCur;
			p->opt[posPrev].posPrev = cur;
			cur = posPrev;
		}
	} while(cur != 0);
	*backRes = p->opt[0].backPrev;
	p->optimumCurrentIndex  = p->opt[0].posPrev;
	return p->optimumCurrentIndex;
}

#define LIT_PROBS(pos, prevByte) (p->litProbs + ((((pos) & p->lpMask) << p->lc) + ((prevByte) >> (8 - p->lc))) * (uint32)0x300)

static uint32 GetOptimum(CLzmaEnc * p, uint32 position, uint32 * backRes)
{
	uint32 lenEnd, cur;
	uint32 reps[LZMA_NUM_REPS], repLens[LZMA_NUM_REPS];
	uint32 * matches;
	{
		uint32 numAvail, mainLen, numPairs, repMaxIndex, i, posState, len;
		uint32 matchPrice, repMatchPrice, normalMatchPrice;
		const Byte * data;
		Byte curByte, matchByte;
		if(p->optimumEndIndex != p->optimumCurrentIndex) {
			const COptimal * opt = &p->opt[p->optimumCurrentIndex];
			uint32 lenRes = opt->posPrev - p->optimumCurrentIndex;
			*backRes = opt->backPrev;
			p->optimumCurrentIndex = opt->posPrev;
			return lenRes;
		}
		p->optimumCurrentIndex = p->optimumEndIndex = 0;

		if(p->additionalOffset == 0)
			mainLen = ReadMatchDistances(p, &numPairs);
		else {
			mainLen = p->longestMatchLength;
			numPairs = p->numPairs;
		}

		numAvail = p->numAvail;
		if(numAvail < 2) {
			*backRes = (uint32)(-1);
			return 1;
		}
		if(numAvail > LZMA_MATCH_LEN_MAX)
			numAvail = LZMA_MATCH_LEN_MAX;

		data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
		repMaxIndex = 0;
		for(i = 0; i < LZMA_NUM_REPS; i++) {
			uint32 lenTest;
			const Byte * data2;
			reps[i] = p->reps[i];
			data2 = data - reps[i] - 1;
			if(data[0] != data2[0] || data[1] != data2[1]) {
				repLens[i] = 0;
				continue;
			}
			for(lenTest = 2; lenTest < numAvail && data[lenTest] == data2[lenTest]; lenTest++) ;
			repLens[i] = lenTest;
			if(lenTest > repLens[repMaxIndex])
				repMaxIndex = i;
		}
		if(repLens[repMaxIndex] >= p->numFastBytes) {
			uint32 lenRes;
			*backRes = repMaxIndex;
			lenRes = repLens[repMaxIndex];
			MovePos(p, lenRes - 1);
			return lenRes;
		}
		matches = p->matches;
		if(mainLen >= p->numFastBytes) {
			*backRes = matches[(size_t)numPairs - 1] + LZMA_NUM_REPS;
			MovePos(p, mainLen - 1);
			return mainLen;
		}
		curByte = *data;
		matchByte = *(data - (reps[0] + 1));
		if(mainLen < 2 && curByte != matchByte && repLens[repMaxIndex] < 2) {
			*backRes = static_cast<uint32>(-1);
			return 1;
		}
		p->opt[0].state = (CState)p->state;
		posState = (position & p->pbMask);
		{
			const CLzmaProb * probs = LIT_PROBS(position, *(data - 1));
			p->opt[1].price = GET_PRICE_0(p->isMatch[p->state][posState]) + (!IsCharState(p->state) ?
			    LitEnc_GetPriceMatched(probs, curByte, matchByte, p->ProbPrices) : LitEnc_GetPrice(probs, curByte, p->ProbPrices));
		}
		MakeAsChar(&p->opt[1]);
		matchPrice = GET_PRICE_1(p->isMatch[p->state][posState]);
		repMatchPrice = matchPrice + GET_PRICE_1(p->isRep[p->state]);
		if(matchByte == curByte) {
			uint32 shortRepPrice = repMatchPrice + GetRepLen1Price(p, p->state, posState);
			if(shortRepPrice < p->opt[1].price) {
				p->opt[1].price = shortRepPrice;
				MakeAsShortRep(&p->opt[1]);
			}
		}
		lenEnd = ((mainLen >= repLens[repMaxIndex]) ? mainLen : repLens[repMaxIndex]);
		if(lenEnd < 2) {
			*backRes = p->opt[1].backPrev;
			return 1;
		}
		p->opt[1].posPrev = 0;
		for(i = 0; i < LZMA_NUM_REPS; i++)
			p->opt[0].backs[i] = reps[i];
		len = lenEnd;
		do {
			p->opt[len--].price = kInfinityPrice;
		} while(len >= 2);
		for(i = 0; i < LZMA_NUM_REPS; i++) {
			uint32 repLen = repLens[i];
			uint32 price;
			if(repLen < 2)
				continue;
			price = repMatchPrice + GetPureRepPrice(p, i, p->state, posState);
			do {
				uint32 curAndLenPrice = price + p->repLenEnc.prices[posState][(size_t)repLen - 2];
				COptimal * opt = &p->opt[repLen];
				if(curAndLenPrice < opt->price) {
					opt->price = curAndLenPrice;
					opt->posPrev = 0;
					opt->backPrev = i;
					opt->prev1IsChar = False;
				}
			} while(--repLen >= 2);
		}
		normalMatchPrice = matchPrice + GET_PRICE_0(p->isRep[p->state]);
		len = ((repLens[0] >= 2) ? repLens[0] + 1 : 2);
		if(len <= mainLen) {
			uint32 offs = 0;
			while(len > matches[offs])
				offs += 2;
			for(;; len++) {
				COptimal * opt;
				uint32 distance = matches[(size_t)offs + 1];

				uint32 curAndLenPrice = normalMatchPrice + p->lenEnc.prices[posState][(size_t)len - LZMA_MATCH_LEN_MIN];
				uint32 lenToPosState = GetLenToPosState(len);
				if(distance < kNumFullDistances)
					curAndLenPrice += p->distancesPrices[lenToPosState][distance];
				else {
					uint32 slot;
					GetPosSlot2(distance, slot);
					curAndLenPrice += p->alignPrices[distance & kAlignMask] + p->posSlotPrices[lenToPosState][slot];
				}
				opt = &p->opt[len];
				if(curAndLenPrice < opt->price) {
					opt->price = curAndLenPrice;
					opt->posPrev = 0;
					opt->backPrev = distance + LZMA_NUM_REPS;
					opt->prev1IsChar = False;
				}
				if(len == matches[offs]) {
					offs += 2;
					if(offs == numPairs)
						break;
				}
			}
		}
		cur = 0;
    #ifdef SHOW_STAT2
		/* if(position >= 0) */
		{
			uint i;
			printf("\n pos = %4X", position);
			for(i = cur; i <= lenEnd; i++)
				printf("\nprice[%4X] = %u", position - cur + i, p->opt[i].price);
		}
    #endif
	}
	for(;; ) {
		uint32 numAvail;
		uint32 numAvailFull, newLen, numPairs, posPrev, state, posState, startLen;
		uint32 curPrice, curAnd1Price, matchPrice, repMatchPrice;
		Bool nextIsChar;
		Byte curByte, matchByte;
		const Byte * data;
		COptimal * curOpt;
		COptimal * nextOpt;
		cur++;
		if(cur == lenEnd)
			return Backward(p, backRes, cur);
		newLen = ReadMatchDistances(p, &numPairs);
		if(newLen >= p->numFastBytes) {
			p->numPairs = numPairs;
			p->longestMatchLength = newLen;
			return Backward(p, backRes, cur);
		}
		position++;
		curOpt = &p->opt[cur];
		posPrev = curOpt->posPrev;
		if(curOpt->prev1IsChar) {
			posPrev--;
			if(curOpt->prev2) {
				state = p->opt[curOpt->posPrev2].state;
				if(curOpt->backPrev2 < LZMA_NUM_REPS)
					state = kRepNextStates[state];
				else
					state = kMatchNextStates[state];
			}
			else
				state = p->opt[posPrev].state;
			state = kLiteralNextStates[state];
		}
		else
			state = p->opt[posPrev].state;
		if(posPrev == cur - 1) {
			if(IsShortRep(curOpt))
				state = kShortRepNextStates[state];
			else
				state = kLiteralNextStates[state];
		}
		else {
			uint32 pos;
			const COptimal * prevOpt;
			if(curOpt->prev1IsChar && curOpt->prev2) {
				posPrev = curOpt->posPrev2;
				pos = curOpt->backPrev2;
				state = kRepNextStates[state];
			}
			else {
				pos = curOpt->backPrev;
				if(pos < LZMA_NUM_REPS)
					state = kRepNextStates[state];
				else
					state = kMatchNextStates[state];
			}
			prevOpt = &p->opt[posPrev];
			if(pos < LZMA_NUM_REPS) {
				uint32 i;
				reps[0] = prevOpt->backs[pos];
				for(i = 1; i <= pos; i++)
					reps[i] = prevOpt->backs[(size_t)i - 1];
				for(; i < LZMA_NUM_REPS; i++)
					reps[i] = prevOpt->backs[i];
			}
			else {
				uint32 i;
				reps[0] = (pos - LZMA_NUM_REPS);
				for(i = 1; i < LZMA_NUM_REPS; i++)
					reps[i] = prevOpt->backs[(size_t)i - 1];
			}
		}
		curOpt->state = (CState)state;
		curOpt->backs[0] = reps[0];
		curOpt->backs[1] = reps[1];
		curOpt->backs[2] = reps[2];
		curOpt->backs[3] = reps[3];
		curPrice = curOpt->price;
		nextIsChar = False;
		data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
		curByte = *data;
		matchByte = *(data - (reps[0] + 1));
		posState = (position & p->pbMask);
		curAnd1Price = curPrice + GET_PRICE_0(p->isMatch[state][posState]);
		{
			const CLzmaProb * probs = LIT_PROBS(position, *(data - 1));
			curAnd1Price += (!IsCharState(state) ? LitEnc_GetPriceMatched(probs, curByte, matchByte, p->ProbPrices) : LitEnc_GetPrice(probs, curByte, p->ProbPrices));
		}
		nextOpt = &p->opt[(size_t)cur + 1];
		if(curAnd1Price < nextOpt->price) {
			nextOpt->price = curAnd1Price;
			nextOpt->posPrev = cur;
			MakeAsChar(nextOpt);
			nextIsChar = True;
		}
		matchPrice = curPrice + GET_PRICE_1(p->isMatch[state][posState]);
		repMatchPrice = matchPrice + GET_PRICE_1(p->isRep[state]);
		if(matchByte == curByte && !(nextOpt->posPrev < cur && nextOpt->backPrev == 0)) {
			uint32 shortRepPrice = repMatchPrice + GetRepLen1Price(p, state, posState);
			if(shortRepPrice <= nextOpt->price) {
				nextOpt->price = shortRepPrice;
				nextOpt->posPrev = cur;
				MakeAsShortRep(nextOpt);
				nextIsChar = True;
			}
		}
		numAvailFull = p->numAvail;
		{
			uint32 temp = kNumOpts - 1 - cur;
			if(temp < numAvailFull)
				numAvailFull = temp;
		}
		if(numAvailFull < 2)
			continue;
		numAvail = (numAvailFull <= p->numFastBytes ? numAvailFull : p->numFastBytes);
		if(!nextIsChar && matchByte != curByte) { /* speed optimization */
			/* try Literal + rep0 */
			uint32 temp;
			uint32 lenTest2;
			const Byte * data2 = data - reps[0] - 1;
			uint32 limit = p->numFastBytes + 1;
			SETMIN(limit, numAvailFull);
			for(temp = 1; temp < limit && data[temp] == data2[temp]; temp++) 
				;
			lenTest2 = temp - 1;
			if(lenTest2 >= 2) {
				uint32 state2 = kLiteralNextStates[state];
				uint32 posStateNext = (position + 1) & p->pbMask;
				uint32 nextRepMatchPrice = curAnd1Price + GET_PRICE_1(p->isMatch[state2][posStateNext]) + GET_PRICE_1(p->isRep[state2]);
				/* for(; lenTest2 >= 2; lenTest2--) */
				{
					uint32 curAndLenPrice;
					COptimal * opt;
					uint32 offset = cur + 1 + lenTest2;
					while(lenEnd < offset)
						p->opt[++lenEnd].price = kInfinityPrice;
					curAndLenPrice = nextRepMatchPrice + GetRepPrice(p, 0, lenTest2, state2, posStateNext);
					opt = &p->opt[offset];
					if(curAndLenPrice < opt->price) {
						opt->price = curAndLenPrice;
						opt->posPrev = cur + 1;
						opt->backPrev = 0;
						opt->prev1IsChar = True;
						opt->prev2 = False;
					}
				}
			}
		}
		startLen = 2; /* speed optimization */
		{
			uint32 repIndex;
			for(repIndex = 0; repIndex < LZMA_NUM_REPS; repIndex++) {
				uint32 lenTest;
				uint32 lenTestTemp;
				uint32 price;
				const Byte * data2 = data - reps[repIndex] - 1;
				if(data[0] != data2[0] || data[1] != data2[1])
					continue;
				for(lenTest = 2; lenTest < numAvail && data[lenTest] == data2[lenTest]; lenTest++) ;
				while(lenEnd < cur + lenTest)
					p->opt[++lenEnd].price = kInfinityPrice;
				lenTestTemp = lenTest;
				price = repMatchPrice + GetPureRepPrice(p, repIndex, state, posState);
				do {
					uint32 curAndLenPrice = price + p->repLenEnc.prices[posState][(size_t)lenTest - 2];
					COptimal * opt = &p->opt[cur + lenTest];
					if(curAndLenPrice < opt->price) {
						opt->price = curAndLenPrice;
						opt->posPrev = cur;
						opt->backPrev = repIndex;
						opt->prev1IsChar = False;
					}
				} while(--lenTest >= 2);
				lenTest = lenTestTemp;
				if(repIndex == 0)
					startLen = lenTest + 1;
				/* if(_maxMode) */
				{
					uint32 lenTest2 = lenTest + 1;
					uint32 limit = lenTest2 + p->numFastBytes;
					if(limit > numAvailFull)
						limit = numAvailFull;
					for(; lenTest2 < limit && data[lenTest2] == data2[lenTest2]; lenTest2++) ;
					lenTest2 -= lenTest + 1;
					if(lenTest2 >= 2) {
						uint32 nextRepMatchPrice;
						uint32 state2 = kRepNextStates[state];
						uint32 posStateNext = (position + lenTest) & p->pbMask;
						uint32 curAndLenCharPrice =
						    price + p->repLenEnc.prices[posState][(size_t)lenTest - 2] +
						    GET_PRICE_0(p->isMatch[state2][posStateNext]) +
						    LitEnc_GetPriceMatched(LIT_PROBS(position + lenTest, data[(size_t)lenTest - 1]),
						    data[lenTest], data2[lenTest], p->ProbPrices);
						state2 = kLiteralNextStates[state2];
						posStateNext = (position + lenTest + 1) & p->pbMask;
						nextRepMatchPrice = curAndLenCharPrice + GET_PRICE_1(p->isMatch[state2][posStateNext]) + GET_PRICE_1(p->isRep[state2]);
						/* for(; lenTest2 >= 2; lenTest2--) */
						{
							uint32 curAndLenPrice;
							COptimal * opt;
							uint32 offset = cur + lenTest + 1 + lenTest2;
							while(lenEnd < offset)
								p->opt[++lenEnd].price = kInfinityPrice;
							curAndLenPrice = nextRepMatchPrice + GetRepPrice(p, 0, lenTest2, state2, posStateNext);
							opt = &p->opt[offset];
							if(curAndLenPrice < opt->price) {
								opt->price = curAndLenPrice;
								opt->posPrev = cur + lenTest + 1;
								opt->backPrev = 0;
								opt->prev1IsChar = True;
								opt->prev2 = True;
								opt->posPrev2 = cur;
								opt->backPrev2 = repIndex;
							}
						}
					}
				}
			}
		}
		/* for(uint32 lenTest = 2; lenTest <= newLen; lenTest++) */
		if(newLen > numAvail) {
			newLen = numAvail;
			for(numPairs = 0; newLen > matches[numPairs]; numPairs += 2) 
				;
			matches[numPairs] = newLen;
			numPairs += 2;
		}
		if(newLen >= startLen) {
			uint32 normalMatchPrice = matchPrice + GET_PRICE_0(p->isRep[state]);
			uint32 offs, curBack, posSlot;
			uint32 lenTest;
			while(lenEnd < cur + newLen)
				p->opt[++lenEnd].price = kInfinityPrice;
			offs = 0;
			while(startLen > matches[offs])
				offs += 2;
			curBack = matches[(size_t)offs + 1];
			GetPosSlot2(curBack, posSlot);
			for(lenTest = /*2*/ startLen;; lenTest++) {
				uint32 curAndLenPrice = normalMatchPrice + p->lenEnc.prices[posState][(size_t)lenTest - LZMA_MATCH_LEN_MIN];
				{
					uint32 lenToPosState = GetLenToPosState(lenTest);
					COptimal * opt;
					if(curBack < kNumFullDistances)
						curAndLenPrice += p->distancesPrices[lenToPosState][curBack];
					else
						curAndLenPrice += p->posSlotPrices[lenToPosState][posSlot] + p->alignPrices[curBack & kAlignMask];
					opt = &p->opt[cur + lenTest];
					if(curAndLenPrice < opt->price) {
						opt->price = curAndLenPrice;
						opt->posPrev = cur;
						opt->backPrev = curBack + LZMA_NUM_REPS;
						opt->prev1IsChar = False;
					}
				}
				if(/*_maxMode && */ lenTest == matches[offs]) {
					/* Try Match + Literal + Rep0 */
					const Byte * data2 = data - curBack - 1;
					uint32 lenTest2 = lenTest + 1;
					uint32 limit = lenTest2 + p->numFastBytes;
					SETMIN(limit, numAvailFull);
					for(; lenTest2 < limit && data[lenTest2] == data2[lenTest2]; lenTest2++) 
						;
					lenTest2 -= lenTest + 1;
					if(lenTest2 >= 2) {
						uint32 nextRepMatchPrice;
						uint32 state2 = kMatchNextStates[state];
						uint32 posStateNext = (position + lenTest) & p->pbMask;
						uint32 curAndLenCharPrice = curAndLenPrice + GET_PRICE_0(p->isMatch[state2][posStateNext]) + 
							LitEnc_GetPriceMatched(LIT_PROBS(position + lenTest, data[(size_t)lenTest - 1]), data[lenTest], data2[lenTest], p->ProbPrices);
						state2 = kLiteralNextStates[state2];
						posStateNext = (posStateNext + 1) & p->pbMask;
						nextRepMatchPrice = curAndLenCharPrice + GET_PRICE_1(p->isMatch[state2][posStateNext]) + GET_PRICE_1(p->isRep[state2]);
						/* for(; lenTest2 >= 2; lenTest2--) */
						{
							uint32 offset = cur + lenTest + 1 + lenTest2;
							uint32 curAndLenPrice2;
							COptimal * opt;
							while(lenEnd < offset)
								p->opt[++lenEnd].price = kInfinityPrice;
							curAndLenPrice2 = nextRepMatchPrice + GetRepPrice(p, 0, lenTest2, state2, posStateNext);
							opt = &p->opt[offset];
							if(curAndLenPrice2 < opt->price) {
								opt->price = curAndLenPrice2;
								opt->posPrev = cur + lenTest + 1;
								opt->backPrev = 0;
								opt->prev1IsChar = True;
								opt->prev2 = True;
								opt->posPrev2 = cur;
								opt->backPrev2 = curBack + LZMA_NUM_REPS;
							}
						}
					}
					offs += 2;
					if(offs == numPairs)
						break;
					curBack = matches[(size_t)offs + 1];
					if(curBack >= kNumFullDistances)
						GetPosSlot2(curBack, posSlot);
				}
			}
		}
	}
}

#define ChangePair(smallDist, bigDist) (((bigDist) >> 7) > (smallDist))

static uint32 GetOptimumFast(CLzmaEnc * p, uint32 * backRes)
{
	uint32 numAvail, mainLen, mainDist, numPairs, repIndex, repLen, i;
	const Byte * data;
	const uint32 * matches;
	if(p->additionalOffset == 0)
		mainLen = ReadMatchDistances(p, &numPairs);
	else {
		mainLen = p->longestMatchLength;
		numPairs = p->numPairs;
	}
	numAvail = p->numAvail;
	*backRes = static_cast<uint32>(-1);
	if(numAvail < 2)
		return 1;
	SETMIN(numAvail, LZMA_MATCH_LEN_MAX);
	data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
	repLen = repIndex = 0;
	for(i = 0; i < LZMA_NUM_REPS; i++) {
		uint32 len;
		const Byte * data2 = data - p->reps[i] - 1;
		if(data[0] != data2[0] || data[1] != data2[1])
			continue;
		for(len = 2; len < numAvail && data[len] == data2[len]; len++) ;
		if(len >= p->numFastBytes) {
			*backRes = i;
			MovePos(p, len - 1);
			return len;
		}
		if(len > repLen) {
			repIndex = i;
			repLen = len;
		}
	}
	matches = p->matches;
	if(mainLen >= p->numFastBytes) {
		*backRes = matches[(size_t)numPairs - 1] + LZMA_NUM_REPS;
		MovePos(p, mainLen - 1);
		return mainLen;
	}
	mainDist = 0; /* for GCC */
	if(mainLen >= 2) {
		mainDist = matches[(size_t)numPairs - 1];
		while(numPairs > 2 && mainLen == matches[(size_t)numPairs - 4] + 1) {
			if(!ChangePair(matches[(size_t)numPairs - 3], mainDist))
				break;
			numPairs -= 2;
			mainLen = matches[(size_t)numPairs - 2];
			mainDist = matches[(size_t)numPairs - 1];
		}
		if(mainLen == 2 && mainDist >= 0x80)
			mainLen = 1;
	}
	if(repLen >= 2 && ((repLen + 1 >= mainLen) || (repLen + 2 >= mainLen && mainDist >= (1 << 9)) || (repLen + 3 >= mainLen && mainDist >= (1 << 15)))) {
		*backRes = repIndex;
		MovePos(p, repLen - 1);
		return repLen;
	}
	if(mainLen < 2 || numAvail <= 2)
		return 1;
	p->longestMatchLength = ReadMatchDistances(p, &p->numPairs);
	if(p->longestMatchLength >= 2) {
		uint32 newDistance = matches[(size_t)p->numPairs - 1];
		if((p->longestMatchLength >= mainLen && newDistance < mainDist) || (p->longestMatchLength == mainLen + 1 && !ChangePair(mainDist, newDistance)) ||
		    (p->longestMatchLength > mainLen + 1) || (p->longestMatchLength + 1 >= mainLen && mainLen >= 3 && ChangePair(newDistance, mainDist)))
			return 1;
	}
	data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
	for(i = 0; i < LZMA_NUM_REPS; i++) {
		uint32 len, limit;
		const Byte * data2 = data - p->reps[i] - 1;
		if(data[0] != data2[0] || data[1] != data2[1])
			continue;
		limit = mainLen - 1;
		for(len = 2; len < limit && data[len] == data2[len]; len++) 
			;
		if(len >= limit)
			return 1;
	}
	*backRes = mainDist + LZMA_NUM_REPS;
	MovePos(p, mainLen - 2);
	return mainLen;
}

static void FASTCALL WriteEndMarker(CLzmaEnc * p, uint32 posState)
{
	uint32 len;
	RangeEnc_EncodeBit(&p->rc, &p->isMatch[p->state][posState], 1);
	RangeEnc_EncodeBit(&p->rc, &p->isRep[p->state], 0);
	p->state = kMatchNextStates[p->state];
	len = LZMA_MATCH_LEN_MIN;
	LenEnc_Encode2(&p->lenEnc, &p->rc, len - LZMA_MATCH_LEN_MIN, posState, !p->fastMode, p->ProbPrices);
	RcTree_Encode(&p->rc, p->posSlotEncoder[GetLenToPosState(len)], kNumPosSlotBits, (1 << kNumPosSlotBits) - 1);
	RangeEnc_EncodeDirectBits(&p->rc, (((uint32)1 << 30) - 1) >> kNumAlignBits, 30 - kNumAlignBits);
	RcTree_ReverseEncode(&p->rc, p->posAlignEncoder, kNumAlignBits, kAlignMask);
}

static SRes FASTCALL CheckErrors(CLzmaEnc * p)
{
	if(p->result == SZ_OK) {
		if(p->rc.res != SZ_OK)
			p->result = SZ_ERROR_WRITE;
		if(p->matchFinderBase.result != SZ_OK)
			p->result = SZ_ERROR_READ;
		if(p->result != SZ_OK)
			p->finished = True;
	}
	return p->result;
}

static SRes FASTCALL Flush(CLzmaEnc * p, uint32 nowPos)
{
	// ReleaseMFStream(); 
	p->finished = True;
	if(p->writeEndMark)
		WriteEndMarker(p, nowPos & p->pbMask);
	RangeEnc_FlushData(&p->rc);
	RangeEnc_FlushStream(&p->rc);
	return CheckErrors(p);
}

static void FASTCALL FillAlignPrices(CLzmaEnc * p)
{
	for(uint32 i = 0; i < kAlignTableSize; i++)
		p->alignPrices[i] = RcTree_ReverseGetPrice(p->posAlignEncoder, kNumAlignBits, i, p->ProbPrices);
	p->alignPriceCount = 0;
}

static void FASTCALL FillDistancesPrices(CLzmaEnc * p)
{
	uint32 tempPrices[kNumFullDistances];
	uint32 i, lenToPosState;
	for(i = kStartPosModelIndex; i < kNumFullDistances; i++) {
		uint32 posSlot = GetPosSlot1(i);
		uint32 footerBits = ((posSlot >> 1) - 1);
		uint32 base = ((2 | (posSlot & 1)) << footerBits);
		tempPrices[i] = RcTree_ReverseGetPrice(p->posEncoders + base - posSlot - 1, footerBits, i - base, p->ProbPrices);
	}
	for(lenToPosState = 0; lenToPosState < kNumLenToPosStates; lenToPosState++) {
		uint32 posSlot;
		const CLzmaProb * encoder = p->posSlotEncoder[lenToPosState];
		uint32 * posSlotPrices = p->posSlotPrices[lenToPosState];
		for(posSlot = 0; posSlot < p->distTableSize; posSlot++)
			posSlotPrices[posSlot] = RcTree_GetPrice(encoder, kNumPosSlotBits, posSlot, p->ProbPrices);
		for(posSlot = kEndPosModelIndex; posSlot < p->distTableSize; posSlot++)
			posSlotPrices[posSlot] += ((((posSlot >> 1) - 1) - kNumAlignBits) << kNumBitPriceShiftBits);
		{
			uint32 * distancesPrices = p->distancesPrices[lenToPosState];
			for(i = 0; i < kStartPosModelIndex; i++)
				distancesPrices[i] = posSlotPrices[i];
			for(; i < kNumFullDistances; i++)
				distancesPrices[i] = posSlotPrices[GetPosSlot1(i)] + tempPrices[i];
		}
	}
	p->matchPriceCount = 0;
}

void LzmaEnc_Construct(CLzmaEnc * p)
{
	RangeEnc_Construct(&p->rc);
	MatchFinder_Construct(&p->matchFinderBase);
  #ifndef _7ZIP_ST
	MatchFinderMt_Construct(&p->matchFinderMt);
	p->matchFinderMt.MatchFinder = &p->matchFinderBase;
  #endif
	{
		CLzmaEncProps props;
		LzmaEncProps_Init(&props);
		LzmaEnc_SetProps(p, &props);
	}
  #ifndef LZMA_LOG_BSR
	LzmaEnc_FastPosInit(p->g_FastPos);
  #endif
	LzmaEnc_InitPriceTables(p->ProbPrices);
	p->litProbs = NULL;
	p->saveState.litProbs = NULL;
}

CLzmaEncHandle LzmaEnc_Create(ISzAllocPtr alloc)
{
	void * p = ISzAlloc_Alloc(alloc, sizeof(CLzmaEnc));
	if(p)
		LzmaEnc_Construct((CLzmaEnc*)p);
	return p;
}

static void FASTCALL LzmaEnc_FreeLits(CLzmaEnc * p, ISzAllocPtr alloc)
{
	ISzAlloc_Free(alloc, p->litProbs);
	ISzAlloc_Free(alloc, p->saveState.litProbs);
	p->litProbs = NULL;
	p->saveState.litProbs = NULL;
}

void LzmaEnc_Destruct(CLzmaEnc * p, ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
#ifndef _7ZIP_ST
	MatchFinderMt_Destruct(&p->matchFinderMt, allocBig);
#endif
	MatchFinder_Free(&p->matchFinderBase, allocBig);
	LzmaEnc_FreeLits(p, alloc);
	RangeEnc_Free(&p->rc, alloc);
}

void LzmaEnc_Destroy(CLzmaEncHandle p, ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
	LzmaEnc_Destruct((CLzmaEnc*)p, alloc, allocBig);
	ISzAlloc_Free(alloc, p);
}

static SRes LzmaEnc_CodeOneBlock(CLzmaEnc * p, Bool useLimits, uint32 maxPackSize, uint32 maxUnpackSize)
{
	uint32 nowPos32, startPos32;
	if(p->needInit) {
		p->matchFinder.Init(p->matchFinderObj);
		p->needInit = 0;
	}
	if(p->finished)
		return p->result;
	RINOK(CheckErrors(p));
	nowPos32 = (uint32)p->nowPos64;
	startPos32 = nowPos32;
	if(p->nowPos64 == 0) {
		uint32 numPairs;
		Byte curByte;
		if(p->matchFinder.GetNumAvailableBytes(p->matchFinderObj) == 0)
			return Flush(p, nowPos32);
		ReadMatchDistances(p, &numPairs);
		RangeEnc_EncodeBit(&p->rc, &p->isMatch[p->state][0], 0);
		p->state = kLiteralNextStates[p->state];
		curByte = *(p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - p->additionalOffset);
		LitEnc_Encode(&p->rc, p->litProbs, curByte);
		p->additionalOffset--;
		nowPos32++;
	}
	if(p->matchFinder.GetNumAvailableBytes(p->matchFinderObj) != 0)
		for(;; ) {
			uint32 pos, len, posState;
			len = p->fastMode ? GetOptimumFast(p, &pos) : GetOptimum(p, nowPos32, &pos);
    #ifdef SHOW_STAT2
			printf("\n pos = %4X,   len = %u   pos = %u", nowPos32, len, pos);
    #endif
			posState = nowPos32 & p->pbMask;
			if(len == 1 && pos == static_cast<uint32>(-1)) {
				Byte curByte;
				CLzmaProb * probs;
				const Byte * data;
				RangeEnc_EncodeBit(&p->rc, &p->isMatch[p->state][posState], 0);
				data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - p->additionalOffset;
				curByte = *data;
				probs = LIT_PROBS(nowPos32, *(data - 1));
				if(IsCharState(p->state))
					LitEnc_Encode(&p->rc, probs, curByte);
				else
					LitEnc_EncodeMatched(&p->rc, probs, curByte, *(data - p->reps[0] - 1));
				p->state = kLiteralNextStates[p->state];
			}
			else {
				RangeEnc_EncodeBit(&p->rc, &p->isMatch[p->state][posState], 1);
				if(pos < LZMA_NUM_REPS) {
					RangeEnc_EncodeBit(&p->rc, &p->isRep[p->state], 1);
					if(pos == 0) {
						RangeEnc_EncodeBit(&p->rc, &p->isRepG0[p->state], 0);
						RangeEnc_EncodeBit(&p->rc, &p->isRep0Long[p->state][posState], ((len == 1) ? 0 : 1));
					}
					else {
						uint32 distance = p->reps[pos];
						RangeEnc_EncodeBit(&p->rc, &p->isRepG0[p->state], 1);
						if(pos == 1)
							RangeEnc_EncodeBit(&p->rc, &p->isRepG1[p->state], 0);
						else {
							RangeEnc_EncodeBit(&p->rc, &p->isRepG1[p->state], 1);
							RangeEnc_EncodeBit(&p->rc, &p->isRepG2[p->state], pos - 2);
							if(pos == 3)
								p->reps[3] = p->reps[2];
							p->reps[2] = p->reps[1];
						}
						p->reps[1] = p->reps[0];
						p->reps[0] = distance;
					}
					if(len == 1)
						p->state = kShortRepNextStates[p->state];
					else {
						LenEnc_Encode2(&p->repLenEnc, &p->rc, (len - LZMA_MATCH_LEN_MIN), posState, !p->fastMode, p->ProbPrices);
						p->state = kRepNextStates[p->state];
					}
				}
				else {
					uint32 posSlot;
					RangeEnc_EncodeBit(&p->rc, &p->isRep[p->state], 0);
					p->state = kMatchNextStates[p->state];
					LenEnc_Encode2(&p->lenEnc, &p->rc, len - LZMA_MATCH_LEN_MIN, posState, !p->fastMode, p->ProbPrices);
					pos -= LZMA_NUM_REPS;
					GetPosSlot(pos, posSlot);
					RcTree_Encode(&p->rc, p->posSlotEncoder[GetLenToPosState(len)], kNumPosSlotBits, posSlot);
					if(posSlot >= kStartPosModelIndex) {
						uint32 footerBits = ((posSlot >> 1) - 1);
						uint32 base = ((2 | (posSlot & 1)) << footerBits);
						uint32 posReduced = pos - base;
						if(posSlot < kEndPosModelIndex)
							RcTree_ReverseEncode(&p->rc, p->posEncoders + base - posSlot - 1, footerBits, posReduced);
						else {
							RangeEnc_EncodeDirectBits(&p->rc, posReduced >> kNumAlignBits, footerBits - kNumAlignBits);
							RcTree_ReverseEncode(&p->rc, p->posAlignEncoder, kNumAlignBits, posReduced & kAlignMask);
							p->alignPriceCount++;
						}
					}
					p->reps[3] = p->reps[2];
					p->reps[2] = p->reps[1];
					p->reps[1] = p->reps[0];
					p->reps[0] = pos;
					p->matchPriceCount++;
				}
			}
			p->additionalOffset -= len;
			nowPos32 += len;
			if(p->additionalOffset == 0) {
				uint32 processed;
				if(!p->fastMode) {
					if(p->matchPriceCount >= (1 << 7))
						FillDistancesPrices(p);
					if(p->alignPriceCount >= kAlignTableSize)
						FillAlignPrices(p);
				}
				if(p->matchFinder.GetNumAvailableBytes(p->matchFinderObj) == 0)
					break;
				processed = nowPos32 - startPos32;
				if(useLimits) {
					if(processed + kNumOpts + 300 >= maxUnpackSize || RangeEnc_GetProcessed(&p->rc) + kNumOpts * 2 >= maxPackSize)
						break;
				}
				else if(processed >= (1 << 17)) {
					p->nowPos64 += nowPos32 - startPos32;
					return CheckErrors(p);
				}
			}
		}
	p->nowPos64 += nowPos32 - startPos32;
	return Flush(p, nowPos32);
}

#define kBigHashDicLimit ((uint32)1 << 24)

static SRes LzmaEnc_Alloc(CLzmaEnc * p, uint32 keepWindowSize, ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
	uint32 beforeSize = kNumOpts;
	if(!RangeEnc_Alloc(&p->rc, alloc))
		return SZ_ERROR_MEM;
  #ifndef _7ZIP_ST
	p->mtMode = (p->multiThread && !p->fastMode && (p->matchFinderBase.btMode != 0));
  #endif
	{
		uint   lclp = p->lc + p->lp;
		if(!p->litProbs || !p->saveState.litProbs || p->lclp != lclp) {
			LzmaEnc_FreeLits(p, alloc);
			p->litProbs = (CLzmaProb*)ISzAlloc_Alloc(alloc, ((uint32)0x300 << lclp) * sizeof(CLzmaProb));
			p->saveState.litProbs = (CLzmaProb*)ISzAlloc_Alloc(alloc, ((uint32)0x300 << lclp) * sizeof(CLzmaProb));
			if(!p->litProbs || !p->saveState.litProbs) {
				LzmaEnc_FreeLits(p, alloc);
				return SZ_ERROR_MEM;
			}
			p->lclp = lclp;
		}
	}
	p->matchFinderBase.bigHash = (Byte)(p->dictSize > kBigHashDicLimit ? 1 : 0);
	if(beforeSize + p->dictSize < keepWindowSize)
		beforeSize = keepWindowSize - p->dictSize;
  #ifndef _7ZIP_ST
	if(p->mtMode) {
		RINOK(MatchFinderMt_Create(&p->matchFinderMt, p->dictSize, beforeSize, p->numFastBytes, LZMA_MATCH_LEN_MAX, allocBig));
		p->matchFinderObj = &p->matchFinderMt;
		MatchFinderMt_CreateVTable(&p->matchFinderMt, &p->matchFinder);
	}
	else
  #endif
	{
		if(!MatchFinder_Create(&p->matchFinderBase, p->dictSize, beforeSize, p->numFastBytes, LZMA_MATCH_LEN_MAX, allocBig))
			return SZ_ERROR_MEM;
		p->matchFinderObj = &p->matchFinderBase;
		MatchFinder_CreateVTable(&p->matchFinderBase, &p->matchFinder);
	}
	return SZ_OK;
}

void LzmaEnc_Init(CLzmaEnc * p)
{
	uint32 i;
	p->state = 0;
	for(i = 0; i < LZMA_NUM_REPS; i++)
		p->reps[i] = 0;
	RangeEnc_Init(&p->rc);
	for(i = 0; i < kNumStates; i++) {
		for(uint32 j = 0; j < LZMA_NUM_PB_STATES_MAX; j++) {
			p->isMatch[i][j] = kProbInitValue;
			p->isRep0Long[i][j] = kProbInitValue;
		}
		p->isRep[i] = kProbInitValue;
		p->isRepG0[i] = kProbInitValue;
		p->isRepG1[i] = kProbInitValue;
		p->isRepG2[i] = kProbInitValue;
	}
	{
		uint32 num = (uint32)0x300 << (p->lp + p->lc);
		CLzmaProb * probs = p->litProbs;
		for(i = 0; i < num; i++)
			probs[i] = kProbInitValue;
	}
	{
		for(i = 0; i < kNumLenToPosStates; i++) {
			CLzmaProb * probs = p->posSlotEncoder[i];
			for(uint32 j = 0; j < (1 << kNumPosSlotBits); j++)
				probs[j] = kProbInitValue;
		}
	}
	{
		for(i = 0; i < kNumFullDistances - kEndPosModelIndex; i++)
			p->posEncoders[i] = kProbInitValue;
	}
	LenEnc_Init(&p->lenEnc.p);
	LenEnc_Init(&p->repLenEnc.p);
	for(i = 0; i < (1 << kNumAlignBits); i++)
		p->posAlignEncoder[i] = kProbInitValue;
	p->optimumEndIndex = 0;
	p->optimumCurrentIndex = 0;
	p->additionalOffset = 0;
	p->pbMask = (1 << p->pb) - 1;
	p->lpMask = (1 << p->lp) - 1;
}

void LzmaEnc_InitPrices(CLzmaEnc * p)
{
	if(!p->fastMode) {
		FillDistancesPrices(p);
		FillAlignPrices(p);
	}
	p->lenEnc.tableSize = p->repLenEnc.tableSize = p->numFastBytes + 1 - LZMA_MATCH_LEN_MIN;
	LenPriceEnc_UpdateTables(&p->lenEnc, 1 << p->pb, p->ProbPrices);
	LenPriceEnc_UpdateTables(&p->repLenEnc, 1 << p->pb, p->ProbPrices);
}

static SRes LzmaEnc_AllocAndInit(CLzmaEnc * p, uint32 keepWindowSize, ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
	uint32 i;
	for(i = 0; i < (uint32)kDicLogSizeMaxCompress; i++)
		if(p->dictSize <= ((uint32)1 << i))
			break;
	p->distTableSize = i * 2;
	p->finished = False;
	p->result = SZ_OK;
	RINOK(LzmaEnc_Alloc(p, keepWindowSize, alloc, allocBig));
	LzmaEnc_Init(p);
	LzmaEnc_InitPrices(p);
	p->nowPos64 = 0;
	return SZ_OK;
}

static SRes LzmaEnc_Prepare(CLzmaEncHandle pp, ISeqOutStream * outStream, ISeqInStream * inStream, ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
	CLzmaEnc * p = (CLzmaEnc*)pp;
	p->matchFinderBase.stream = inStream;
	p->needInit = 1;
	p->rc.outStream = outStream;
	return LzmaEnc_AllocAndInit(p, 0, alloc, allocBig);
}

SRes LzmaEnc_PrepareForLzma2(CLzmaEncHandle pp, ISeqInStream * inStream, uint32 keepWindowSize, ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
	CLzmaEnc * p = (CLzmaEnc*)pp;
	p->matchFinderBase.stream = inStream;
	p->needInit = 1;
	return LzmaEnc_AllocAndInit(p, keepWindowSize, alloc, allocBig);
}

static void LzmaEnc_SetInputBuf(CLzmaEnc * p, const Byte * src, SizeT srcLen)
{
	p->matchFinderBase.directInput = 1;
	p->matchFinderBase.bufferBase = (Byte *)src;
	p->matchFinderBase.directInputRem = srcLen;
}

SRes LzmaEnc_MemPrepare(CLzmaEncHandle pp, const Byte * src, SizeT srcLen, uint32 keepWindowSize, ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
	CLzmaEnc * p = (CLzmaEnc*)pp;
	LzmaEnc_SetInputBuf(p, src, srcLen);
	p->needInit = 1;
	return LzmaEnc_AllocAndInit(p, keepWindowSize, alloc, allocBig);
}

void LzmaEnc_Finish(CLzmaEncHandle pp)
{
  #ifndef _7ZIP_ST
	CLzmaEnc * p = (CLzmaEnc*)pp;
	if(p->mtMode)
		MatchFinderMt_ReleaseStream(&p->matchFinderMt);
  #else
	UNUSED_VAR(pp);
  #endif
}

typedef struct {
	ISeqOutStream funcTable;
	Byte * data;
	SizeT rem;
	Bool overflow;
} CSeqOutStreamBuf;

static size_t MyWrite(const ISeqOutStream * pp, const void * data, size_t size)
{
	CSeqOutStreamBuf * p = CONTAINER_FROM_VTBL(pp, CSeqOutStreamBuf, funcTable);
	if(p->rem < size) {
		size = p->rem;
		p->overflow = True;
	}
	memcpy(p->data, data, size);
	p->rem -= size;
	p->data += size;
	return size;
}

uint32 LzmaEnc_GetNumAvailableBytes(CLzmaEncHandle pp)
{
	const CLzmaEnc * p = (CLzmaEnc*)pp;
	return p->matchFinder.GetNumAvailableBytes(p->matchFinderObj);
}

const Byte * LzmaEnc_GetCurBuf(CLzmaEncHandle pp)
{
	const CLzmaEnc * p = (CLzmaEnc*)pp;
	return p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - p->additionalOffset;
}

SRes LzmaEnc_CodeOneMemBlock(CLzmaEncHandle pp, Bool reInit, Byte * dest, size_t * destLen, uint32 desiredPackSize, uint32 * unpackSize)
{
	CLzmaEnc * p = (CLzmaEnc*)pp;
	uint64 nowPos64;
	SRes res;
	CSeqOutStreamBuf outStream;
	outStream.funcTable.Write = MyWrite;
	outStream.data = dest;
	outStream.rem = *destLen;
	outStream.overflow = False;
	p->writeEndMark = False;
	p->finished = False;
	p->result = SZ_OK;
	if(reInit)
		LzmaEnc_Init(p);
	LzmaEnc_InitPrices(p);
	nowPos64 = p->nowPos64;
	RangeEnc_Init(&p->rc);
	p->rc.outStream = &outStream.funcTable;
	res = LzmaEnc_CodeOneBlock(p, True, desiredPackSize, *unpackSize);
	*unpackSize = (uint32)(p->nowPos64 - nowPos64);
	*destLen -= outStream.rem;
	return outStream.overflow ? SZ_ERROR_OUTPUT_EOF : res;
}

static SRes LzmaEnc_Encode2(CLzmaEnc * p, ICompressProgress * progress)
{
	SRes res = SZ_OK;
  #ifndef _7ZIP_ST
	Byte allocaDummy[0x300];
	allocaDummy[0] = 0;
	allocaDummy[1] = allocaDummy[0];
  #endif
	for(;; ) {
		res = LzmaEnc_CodeOneBlock(p, False, 0, 0);
		if(res != SZ_OK || p->finished)
			break;
		if(progress) {
			res = ICompressProgress_Progress(progress, p->nowPos64, RangeEnc_GetProcessed(&p->rc));
			if(res != SZ_OK) {
				res = SZ_ERROR_PROGRESS;
				break;
			}
		}
	}
	LzmaEnc_Finish(p);
	/*
	   if(res == SZ_OK && !Inline_MatchFinder_IsFinishedOK(&p->matchFinderBase))
	   res = SZ_ERROR_FAIL;
	   }
	 */
	return res;
}

SRes LzmaEnc_Encode(CLzmaEncHandle pp, ISeqOutStream * outStream, ISeqInStream * inStream, ICompressProgress * progress,
    ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
	RINOK(LzmaEnc_Prepare(pp, outStream, inStream, alloc, allocBig));
	return LzmaEnc_Encode2((CLzmaEnc*)pp, progress);
}

SRes LzmaEnc_WriteProperties(CLzmaEncHandle pp, Byte * props, SizeT * size)
{
	CLzmaEnc * p = (CLzmaEnc*)pp;
	uint i;
	uint32 dictSize = p->dictSize;
	if(*size < LZMA_PROPS_SIZE)
		return SZ_ERROR_PARAM;
	else {
		*size = LZMA_PROPS_SIZE;
		props[0] = (Byte)((p->pb * 5 + p->lp) * 9 + p->lc);
		if(dictSize >= ((uint32)1 << 22)) {
			uint32 kDictMask = ((uint32)1 << 20) - 1;
			if(dictSize < (uint32)0xFFFFFFFF - kDictMask)
				dictSize = (dictSize + kDictMask) & ~kDictMask;
		}
		else {
			for(i = 11; i <= 30; i++) {
				if(dictSize <= ((uint32)2 << i)) {
					dictSize = (2 << i); 
					break;
				}
				if(dictSize <= ((uint32)3 << i)) {
					dictSize = (3 << i); 
					break;
				}
			}
		}
		for(i = 0; i < 4; i++)
			props[1 + i] = (Byte)(dictSize >> (8 * i));
		return SZ_OK;
	}
}

uint LzmaEnc_IsWriteEndMark(CLzmaEncHandle pp)
{
	return ((CLzmaEnc*)pp)->writeEndMark;
}

SRes LzmaEnc_MemEncode(CLzmaEncHandle pp, Byte * dest, SizeT * destLen, const Byte * src, SizeT srcLen,
    int writeEndMark, ICompressProgress * progress, ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
	SRes res;
	CLzmaEnc * p = (CLzmaEnc*)pp;
	CSeqOutStreamBuf outStream;
	outStream.funcTable.Write = MyWrite;
	outStream.data = dest;
	outStream.rem = *destLen;
	outStream.overflow = False;
	p->writeEndMark = writeEndMark;
	p->rc.outStream = &outStream.funcTable;
	res = LzmaEnc_MemPrepare(pp, src, srcLen, 0, alloc, allocBig);
	if(res == SZ_OK) {
		res = LzmaEnc_Encode2(p, progress);
		if(res == SZ_OK && p->nowPos64 != srcLen)
			res = SZ_ERROR_FAIL;
	}
	*destLen -= outStream.rem;
	return outStream.overflow ? SZ_ERROR_OUTPUT_EOF : res;
}

SRes LzmaEncode(Byte * dest, SizeT * destLen, const Byte * src, SizeT srcLen,
    const CLzmaEncProps * props, Byte * propsEncoded, SizeT * propsSize, int writeEndMark,
    ICompressProgress * progress, ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
	CLzmaEnc * p = (CLzmaEnc*)LzmaEnc_Create(alloc);
	SRes res;
	if(!p)
		return SZ_ERROR_MEM;
	res = LzmaEnc_SetProps(p, props);
	if(res == SZ_OK) {
		res = LzmaEnc_WriteProperties(p, propsEncoded, propsSize);
		if(res == SZ_OK)
			res = LzmaEnc_MemEncode(p, dest, destLen, src, srcLen, writeEndMark, progress, alloc, allocBig);
	}
	LzmaEnc_Destroy(p, alloc, allocBig);
	return res;
}
//
// LzmaDec.c -- LZMA Decoder
#define kNumTopBits 24
#define kTopValue ((uint32)1 << kNumTopBits)
#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)
#define kNumMoveBits 5
#define RC_INIT_SIZE 5
#define NORMALIZE if(range < kTopValue) { range <<= 8; code = (code << 8) | (*buf++); }
#define IF_BIT_0(p) ttt = *(p); NORMALIZE; bound = (range >> kNumBitModelTotalBits) * ttt; if(code < bound)
#define UPDATE_0(p) range = bound; *(p) = (CLzmaProb)(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits));
#define UPDATE_1(p) range -= bound; code -= bound; *(p) = (CLzmaProb)(ttt - (ttt >> kNumMoveBits));
#define GET_BIT2(p, i, A0, A1) IF_BIT_0(p) { UPDATE_0(p); i = (i + i); A0; } else { UPDATE_1(p); i = (i + i) + 1; A1; }
#define GET_BIT(p, i) GET_BIT2(p, i,; ,; )
#define TREE_GET_BIT(probs, i) { GET_BIT((probs + i), i); }
#define TREE_DECODE(probs, limit, i) { i = 1; do { TREE_GET_BIT(probs, i); } while(i < limit); i -= limit; }
//#define _LZMA_SIZE_OPT 

#ifdef _LZMA_SIZE_OPT
	#define TREE_6_DECODE(probs, i) TREE_DECODE(probs, (1 << 6), i)
#else
	#define TREE_6_DECODE(probs, i)	{ i = 1; TREE_GET_BIT(probs, i); TREE_GET_BIT(probs, i); TREE_GET_BIT(probs, i); TREE_GET_BIT(probs, i); TREE_GET_BIT(probs, i); TREE_GET_BIT(probs, i); i -= 0x40; }
#endif

#define NORMAL_LITER_DEC GET_BIT(prob + symbol, symbol)
#define MATCHED_LITER_DEC \
	matchByte <<= 1; \
	bit = (matchByte & offs); \
	probLit = prob + offs + bit + symbol; \
	GET_BIT2(probLit, symbol, offs &= ~bit, offs &= bit)

#define NORMALIZE_CHECK if(range < kTopValue) { if(buf >= bufLimit) return DUMMY_ERROR; range <<= 8; code = (code << 8) | (*buf++); }

#define IF_BIT_0_CHECK(p) ttt = *(p); NORMALIZE_CHECK; bound = (range >> kNumBitModelTotalBits) * ttt; if(code < bound)
#define UPDATE_0_CHECK range = bound;
#define UPDATE_1_CHECK range -= bound; code -= bound;
#define GET_BIT2_CHECK(p, i, A0, A1) IF_BIT_0_CHECK(p) { UPDATE_0_CHECK; i = (i + i); A0; } else { UPDATE_1_CHECK; i = (i + i) + 1; A1; }
#define GET_BIT_CHECK(p, i) GET_BIT2_CHECK(p, i,; ,; )
#define TREE_DECODE_CHECK(probs, limit, i) { i = 1; do { GET_BIT_CHECK(probs + i, i) } while(i < limit); i -= limit; }

#define kNumPosBitsMax 4
#define kNumPosStatesMax (1 << kNumPosBitsMax)

#define kLenNumLowBits 3
#define kLenNumLowSymbols (1 << kLenNumLowBits)
#define kLenNumMidBits 3
#define kLenNumMidSymbols (1 << kLenNumMidBits)
#define kLenNumHighBits 8
#define kLenNumHighSymbols (1 << kLenNumHighBits)

#define LenChoice 0
#define LenChoice2 (LenChoice + 1)
#define LenLow (LenChoice2 + 1)
#define LenMid (LenLow + (kNumPosStatesMax << kLenNumLowBits))
#define LenHigh (LenMid + (kNumPosStatesMax << kLenNumMidBits))
#define kNumLenProbs (LenHigh + kLenNumHighSymbols)

#define kNumStates 12
#define kNumLitStates 7

#define kStartPosModelIndex 4
#define kEndPosModelIndex 14
#define kNumFullDistances (1 << (kEndPosModelIndex >> 1))

#define kNumPosSlotBits 6
#define kNumLenToPosStates 4

#define kNumAlignBits 4
#define kAlignTableSize (1 << kNumAlignBits)

#define kMatchMinLen 2
#define kMatchSpecLenStart (kMatchMinLen + kLenNumLowSymbols + kLenNumMidSymbols + kLenNumHighSymbols)

#define IsMatch 0
#define IsRep (IsMatch + (kNumStates << kNumPosBitsMax))
#define IsRepG0 (IsRep + kNumStates)
#define IsRepG1 (IsRepG0 + kNumStates)
#define IsRepG2 (IsRepG1 + kNumStates)
#define IsRep0Long (IsRepG2 + kNumStates)
#define PosSlot (IsRep0Long + (kNumStates << kNumPosBitsMax))
#define SpecPos (PosSlot + (kNumLenToPosStates << kNumPosSlotBits))
#define Align (SpecPos + kNumFullDistances - kEndPosModelIndex)
#define LenCoder (Align + kAlignTableSize)
#define RepLenCoder (LenCoder + kNumLenProbs)
#define Literal (RepLenCoder + kNumLenProbs)

#define LZMA_BASE_SIZE 1846
#define LZMA_LIT_SIZE 0x300

#if Literal != LZMA_BASE_SIZE
StopCompilingDueBUG
#endif

#define LzmaProps_GetNumProbs(p) (Literal + ((uint32)LZMA_LIT_SIZE << ((p)->lc + (p)->lp)))

#define LZMA_DIC_MIN (1 << 12)

/* First LZMA-symbol is always decoded.
   And it decodes new LZMA-symbols while (buf < bufLimit), but "buf" is without last normalization
   Out:
   Result:
    SZ_OK - OK
    SZ_ERROR_DATA - Error
   p->remainLen:
    < kMatchSpecLenStart : normal remain
    = kMatchSpecLenStart : finished
    = kMatchSpecLenStart + 1 : Flush marker (unused now)
    = kMatchSpecLenStart + 2 : State Init Marker (unused now)
 */

static int FASTCALL LzmaDec_DecodeReal(CLzmaDec * p, SizeT limit, const Byte * bufLimit)
{
	CLzmaProb * probs = p->probs;
	unsigned state = p->state;
	uint32 rep0 = p->reps[0], rep1 = p->reps[1], rep2 = p->reps[2], rep3 = p->reps[3];
	unsigned pbMask = ((uint)1 << (p->prop.pb)) - 1;
	unsigned lpMask = ((uint)1 << (p->prop.lp)) - 1;
	unsigned lc = p->prop.lc;
	Byte * dic = p->dic;
	SizeT dicBufSize = p->dicBufSize;
	SizeT dicPos = p->dicPos;
	uint32 processedPos = p->processedPos;
	uint32 checkDicSize = p->checkDicSize;
	uint len = 0;
	const Byte * buf = p->buf;
	uint32 range = p->range;
	uint32 code = p->code;
	do {
		uint32 bound;
		unsigned ttt;
		unsigned posState = processedPos & pbMask;
		CLzmaProb * prob = probs + IsMatch + (state << kNumPosBitsMax) + posState;
		IF_BIT_0(prob) {
			unsigned symbol;
			UPDATE_0(prob);
			prob = probs + Literal;
			if(processedPos != 0 || checkDicSize != 0)
				prob += ((uint32)LZMA_LIT_SIZE * (((processedPos & lpMask) << lc) + (dic[(dicPos == 0 ? dicBufSize : dicPos) - 1] >> (8 - lc))));
			processedPos++;
			if(state < kNumLitStates) {
				state -= (state < 4) ? state : 3;
				symbol = 1;
	#ifdef _LZMA_SIZE_OPT
				do { NORMAL_LITER_DEC } while(symbol < 0x100);
	#else
				NORMAL_LITER_DEC
				NORMAL_LITER_DEC
				NORMAL_LITER_DEC
				NORMAL_LITER_DEC
				NORMAL_LITER_DEC
				NORMAL_LITER_DEC
				NORMAL_LITER_DEC
				    NORMAL_LITER_DEC
	#endif
			}
			else {
				unsigned matchByte = dic[dicPos - rep0 + (dicPos < rep0 ? dicBufSize : 0)];
				unsigned offs = 0x100;
				state -= (state < 10) ? 3 : 6;
				symbol = 1;
	#ifdef _LZMA_SIZE_OPT
				do {
					unsigned bit;
					CLzmaProb * probLit;
					MATCHED_LITER_DEC
				} while(symbol < 0x100);
	#else
				{
					unsigned bit;
					CLzmaProb * probLit;
					MATCHED_LITER_DEC
					MATCHED_LITER_DEC
					MATCHED_LITER_DEC
					MATCHED_LITER_DEC
					MATCHED_LITER_DEC
					MATCHED_LITER_DEC
					MATCHED_LITER_DEC
					    MATCHED_LITER_DEC
				}
	#endif
			}
			dic[dicPos++] = (Byte)symbol;
			continue;
		}
		{
			UPDATE_1(prob);
			prob = probs + IsRep + state;
			IF_BIT_0(prob) {
				UPDATE_0(prob);
				state += kNumStates;
				prob = probs + LenCoder;
			}
			else {
				UPDATE_1(prob);
				if(checkDicSize == 0 && processedPos == 0)
					return SZ_ERROR_DATA;
				prob = probs + IsRepG0 + state;
				IF_BIT_0(prob) {
					UPDATE_0(prob);
					prob = probs + IsRep0Long + (state << kNumPosBitsMax) + posState;
					IF_BIT_0(prob) {
						UPDATE_0(prob);
						dic[dicPos] = dic[dicPos - rep0 + (dicPos < rep0 ? dicBufSize : 0)];
						dicPos++;
						processedPos++;
						state = state < kNumLitStates ? 9 : 11;
						continue;
					}
					UPDATE_1(prob);
				}
				else {
					uint32 distance;
					UPDATE_1(prob);
					prob = probs + IsRepG1 + state;
					IF_BIT_0(prob) {
						UPDATE_0(prob);
						distance = rep1;
					}
					else {
						UPDATE_1(prob);
						prob = probs + IsRepG2 + state;
						IF_BIT_0(prob) {
							UPDATE_0(prob);
							distance = rep2;
						}
						else {
							UPDATE_1(prob);
							distance = rep3;
							rep3 = rep2;
						}
						rep2 = rep1;
					}
					rep1 = rep0;
					rep0 = distance;
				}
				state = state < kNumLitStates ? 8 : 11;
				prob = probs + RepLenCoder;
			}
      #ifdef _LZMA_SIZE_OPT
			{
				unsigned lim, offset;
				CLzmaProb * probLen = prob + LenChoice;
				IF_BIT_0(probLen) {
					UPDATE_0(probLen);
					probLen = prob + LenLow + (posState << kLenNumLowBits);
					offset = 0;
					lim = (1 << kLenNumLowBits);
				}
				else {
					UPDATE_1(probLen);
					probLen = prob + LenChoice2;
					IF_BIT_0(probLen) {
						UPDATE_0(probLen);
						probLen = prob + LenMid + (posState << kLenNumMidBits);
						offset = kLenNumLowSymbols;
						lim = (1 << kLenNumMidBits);
					}
					else {
						UPDATE_1(probLen);
						probLen = prob + LenHigh;
						offset = kLenNumLowSymbols + kLenNumMidSymbols;
						lim = (1 << kLenNumHighBits);
					}
				}
				TREE_DECODE(probLen, lim, len);
				len += offset;
			}
      #else
			{
				CLzmaProb * probLen = prob + LenChoice;
				IF_BIT_0(probLen) {
					UPDATE_0(probLen);
					probLen = prob + LenLow + (posState << kLenNumLowBits);
					len = 1;
					TREE_GET_BIT(probLen, len);
					TREE_GET_BIT(probLen, len);
					TREE_GET_BIT(probLen, len);
					len -= 8;
				}
				else {
					UPDATE_1(probLen);
					probLen = prob + LenChoice2;
					IF_BIT_0(probLen) {
						UPDATE_0(probLen);
						probLen = prob + LenMid + (posState << kLenNumMidBits);
						len = 1;
						TREE_GET_BIT(probLen, len);
						TREE_GET_BIT(probLen, len);
						TREE_GET_BIT(probLen, len);
					}
					else {
						UPDATE_1(probLen);
						probLen = prob + LenHigh;
						TREE_DECODE(probLen, (1 << kLenNumHighBits), len);
						len += kLenNumLowSymbols + kLenNumMidSymbols;
					}
				}
			}
      #endif
			if(state >= kNumStates) {
				uint32 distance;
				prob = probs + PosSlot + ((len < kNumLenToPosStates ? len : kNumLenToPosStates - 1) << kNumPosSlotBits);
				TREE_6_DECODE(prob, distance);
				if(distance >= kStartPosModelIndex) {
					unsigned posSlot = (uint)distance;
					unsigned numDirectBits = (uint)(((distance >> 1) - 1));
					distance = (2 | (distance & 1));
					if(posSlot < kEndPosModelIndex) {
						distance <<= numDirectBits;
						prob = probs + SpecPos + distance - posSlot - 1;
						{
							uint32 mask = 1;
							unsigned i = 1;
							do {
								GET_BIT2(prob + i, i,; , distance |= mask);
								mask <<= 1;
							} while(--numDirectBits != 0);
						}
					}
					else {
						numDirectBits -= kNumAlignBits;
						do {
							NORMALIZE
							    range >>= 1;
							{
								uint32 t;
								code -= range;
								t = (0 - ((uint32)code >> 31)); // (uint32)((int32)code >> 31) 
								distance = (distance << 1) + (t + 1);
								code += range & t;
							}
							/*
								distance <<= 1;
								if(code >= range) {
									code -= range;
									distance |= 1;
								}
							 */
						} while(--numDirectBits != 0);
						prob = probs + Align;
						distance <<= kNumAlignBits;
						{
							unsigned i = 1;
							GET_BIT2(prob + i, i,; , distance |= 1);
							GET_BIT2(prob + i, i,; , distance |= 2);
							GET_BIT2(prob + i, i,; , distance |= 4);
							GET_BIT2(prob + i, i,; , distance |= 8);
						}
						if(distance == (uint32)0xFFFFFFFF) {
							len += kMatchSpecLenStart;
							state -= kNumStates;
							break;
						}
					}
				}
				rep3 = rep2;
				rep2 = rep1;
				rep1 = rep0;
				rep0 = distance + 1;
				if(checkDicSize == 0) {
					if(distance >= processedPos) {
						p->dicPos = dicPos;
						return SZ_ERROR_DATA;
					}
				}
				else if(distance >= checkDicSize) {
					p->dicPos = dicPos;
					return SZ_ERROR_DATA;
				}
				state = (state < kNumStates + kNumLitStates) ? kNumLitStates : kNumLitStates + 3;
			}
			len += kMatchMinLen;
			{
				SizeT rem;
				unsigned curLen;
				SizeT pos;
				if((rem = limit - dicPos) == 0) {
					p->dicPos = dicPos;
					return SZ_ERROR_DATA;
				}
				curLen = ((rem < len) ? (uint)rem : len);
				pos = dicPos - rep0 + (dicPos < rep0 ? dicBufSize : 0);
				processedPos += curLen;
				len -= curLen;
				if(curLen <= dicBufSize - pos) {
					Byte * dest = dic + dicPos;
					ptrdiff_t src = (ptrdiff_t)pos - (ptrdiff_t)dicPos;
					const Byte * lim = dest + curLen;
					dicPos += curLen;
					do {
						*(dest) = (Byte)*(dest + src);
					} while(++dest != lim);
				}
				else {
					do {
						dic[dicPos++] = dic[pos];
						if(++pos == dicBufSize)
							pos = 0;
					} while(--curLen != 0);
				}
			}
		}
	} while(dicPos < limit && buf < bufLimit);
	NORMALIZE;
	p->buf = buf;
	p->range = range;
	p->code = code;
	p->remainLen = len;
	p->dicPos = dicPos;
	p->processedPos = processedPos;
	p->reps[0] = rep0;
	p->reps[1] = rep1;
	p->reps[2] = rep2;
	p->reps[3] = rep3;
	p->state = state;
	return SZ_OK;
}

static void FASTCALL LzmaDec_WriteRem(CLzmaDec * p, SizeT limit)
{
	if(p->remainLen != 0 && p->remainLen < kMatchSpecLenStart) {
		Byte * dic = p->dic;
		SizeT dicPos = p->dicPos;
		SizeT dicBufSize = p->dicBufSize;
		uint len = p->remainLen;
		SizeT rep0 = p->reps[0]; /* we use SizeT to avoid the BUG of VC14 for AMD64 */
		SizeT rem = limit - dicPos;
		if(rem < len)
			len = (uint)(rem);
		if(p->checkDicSize == 0 && p->prop.dicSize - p->processedPos <= len)
			p->checkDicSize = p->prop.dicSize;
		p->processedPos += len;
		p->remainLen -= len;
		while(len != 0) {
			len--;
			dic[dicPos] = dic[dicPos - rep0 + (dicPos < rep0 ? dicBufSize : 0)];
			dicPos++;
		}
		p->dicPos = dicPos;
	}
}

static int FASTCALL LzmaDec_DecodeReal2(CLzmaDec * p, SizeT limit, const Byte * bufLimit)
{
	do {
		SizeT limit2 = limit;
		if(p->checkDicSize == 0) {
			uint32 rem = p->prop.dicSize - p->processedPos;
			if(limit - p->dicPos > rem)
				limit2 = p->dicPos + rem;
		}
		RINOK(LzmaDec_DecodeReal(p, limit2, bufLimit));
		if(p->checkDicSize == 0 && p->processedPos >= p->prop.dicSize)
			p->checkDicSize = p->prop.dicSize;
		LzmaDec_WriteRem(p, limit);
	} while(p->dicPos < limit && p->buf < bufLimit && p->remainLen < kMatchSpecLenStart);
	SETMIN(p->remainLen, kMatchSpecLenStart);
	return 0;
}

typedef enum {
	DUMMY_ERROR, /* unexpected end of input stream */
	DUMMY_LIT,
	DUMMY_MATCH,
	DUMMY_REP
} ELzmaDummy;

static ELzmaDummy LzmaDec_TryDummy(const CLzmaDec * p, const Byte * buf, SizeT inSize)
{
	uint32 range = p->range;
	uint32 code = p->code;
	const Byte * bufLimit = buf + inSize;
	const CLzmaProb * probs = p->probs;
	unsigned state = p->state;
	ELzmaDummy res;
	{
		uint32 bound;
		unsigned ttt;
		unsigned posState = (p->processedPos) & ((1 << p->prop.pb) - 1);
		const CLzmaProb * prob = probs + IsMatch + (state << kNumPosBitsMax) + posState;
		IF_BIT_0_CHECK(prob) {
			UPDATE_0_CHECK
			/* if(bufLimit - buf >= 7) return DUMMY_LIT; */
			    prob = probs + Literal;
			if(p->checkDicSize != 0 || p->processedPos != 0)
				prob += ((uint32)LZMA_LIT_SIZE * ((((p->processedPos) & ((1 << (p->prop.lp)) - 1)) << p->prop.lc) +
					(p->dic[(p->dicPos == 0 ? p->dicBufSize : p->dicPos) - 1] >> (8 - p->prop.lc))));
			if(state < kNumLitStates) {
				unsigned symbol = 1;
				do { GET_BIT_CHECK(prob + symbol, symbol) } while(symbol < 0x100);
			}
			else {
				unsigned matchByte = p->dic[p->dicPos - p->reps[0] + (p->dicPos < p->reps[0] ? p->dicBufSize : 0)];
				unsigned offs = 0x100;
				unsigned symbol = 1;
				do {
					unsigned bit;
					const CLzmaProb * probLit;
					matchByte <<= 1;
					bit = (matchByte & offs);
					probLit = prob + offs + bit + symbol;
					GET_BIT2_CHECK(probLit, symbol, offs &= ~bit, offs &= bit)
				} while(symbol < 0x100);
			}
			res = DUMMY_LIT;
		}
		else {
			unsigned len;
			UPDATE_1_CHECK;
			prob = probs + IsRep + state;
			IF_BIT_0_CHECK(prob) {
				UPDATE_0_CHECK;
				state = 0;
				prob = probs + LenCoder;
				res = DUMMY_MATCH;
			}
			else {
				UPDATE_1_CHECK;
				res = DUMMY_REP;
				prob = probs + IsRepG0 + state;
				IF_BIT_0_CHECK(prob) {
					UPDATE_0_CHECK;
					prob = probs + IsRep0Long + (state << kNumPosBitsMax) + posState;
					IF_BIT_0_CHECK(prob) {
						UPDATE_0_CHECK;
						NORMALIZE_CHECK;
						return DUMMY_REP;
					}
					else {
						UPDATE_1_CHECK;
					}
				}
				else {
					UPDATE_1_CHECK;
					prob = probs + IsRepG1 + state;
					IF_BIT_0_CHECK(prob) {
						UPDATE_0_CHECK;
					}
					else {
						UPDATE_1_CHECK;
						prob = probs + IsRepG2 + state;
						IF_BIT_0_CHECK(prob) {
							UPDATE_0_CHECK;
						}
						else {
							UPDATE_1_CHECK;
						}
					}
				}
				state = kNumStates;
				prob = probs + RepLenCoder;
			}
			{
				unsigned limit, offset;
				const CLzmaProb * probLen = prob + LenChoice;
				IF_BIT_0_CHECK(probLen) {
					UPDATE_0_CHECK;
					probLen = prob + LenLow + (posState << kLenNumLowBits);
					offset = 0;
					limit = 1 << kLenNumLowBits;
				}
				else {
					UPDATE_1_CHECK;
					probLen = prob + LenChoice2;
					IF_BIT_0_CHECK(probLen) {
						UPDATE_0_CHECK;
						probLen = prob + LenMid + (posState << kLenNumMidBits);
						offset = kLenNumLowSymbols;
						limit = 1 << kLenNumMidBits;
					}
					else {
						UPDATE_1_CHECK;
						probLen = prob + LenHigh;
						offset = kLenNumLowSymbols + kLenNumMidSymbols;
						limit = 1 << kLenNumHighBits;
					}
				}
				TREE_DECODE_CHECK(probLen, limit, len);
				len += offset;
			}
			if(state < 4) {
				unsigned posSlot;
				prob = probs + PosSlot + ((len < kNumLenToPosStates ? len : kNumLenToPosStates - 1) << kNumPosSlotBits);
				TREE_DECODE_CHECK(prob, 1 << kNumPosSlotBits, posSlot);
				if(posSlot >= kStartPosModelIndex) {
					unsigned numDirectBits = ((posSlot >> 1) - 1);
					/* if(bufLimit - buf >= 8) return DUMMY_MATCH; */
					if(posSlot < kEndPosModelIndex) {
						prob = probs + SpecPos + ((2 | (posSlot & 1)) << numDirectBits) - posSlot - 1;
					}
					else {
						numDirectBits -= kNumAlignBits;
						do {
							NORMALIZE_CHECK
							    range >>= 1;
							code -= range & (((code - range) >> 31) - 1);
							/* if(code >= range) code -= range; */
						} while(--numDirectBits != 0);
						prob = probs + Align;
						numDirectBits = kNumAlignBits;
					}
					{
						unsigned i = 1;
						do {
							GET_BIT_CHECK(prob + i, i);
						} while(--numDirectBits != 0);
					}
				}
			}
		}
	}
	NORMALIZE_CHECK;
	return res;
}

void LzmaDec_InitDicAndState(CLzmaDec * p, Bool initDic, Bool initState)
{
	p->needFlush = 1;
	p->remainLen = 0;
	p->tempBufSize = 0;
	if(initDic) {
		p->processedPos = 0;
		p->checkDicSize = 0;
		p->needInitState = 1;
	}
	if(initState)
		p->needInitState = 1;
}

void LzmaDec_Init(CLzmaDec * p)
{
	p->dicPos = 0;
	LzmaDec_InitDicAndState(p, True, True);
}

static void LzmaDec_InitStateReal(CLzmaDec * p)
{
	SizeT numProbs = LzmaProps_GetNumProbs(&p->prop);
	SizeT i;
	CLzmaProb * probs = p->probs;
	for(i = 0; i < numProbs; i++)
		probs[i] = kBitModelTotal >> 1;
	p->reps[0] = p->reps[1] = p->reps[2] = p->reps[3] = 1;
	p->state = 0;
	p->needInitState = 0;
}

SRes LzmaDec_DecodeToDic(CLzmaDec * p, SizeT dicLimit, const Byte * src, SizeT * srcLen, ELzmaFinishMode finishMode, ELzmaStatus * status)
{
	SizeT inSize = *srcLen;
	(*srcLen) = 0;
	LzmaDec_WriteRem(p, dicLimit);
	*status = LZMA_STATUS_NOT_SPECIFIED;
	while(p->remainLen != kMatchSpecLenStart) {
		int checkEndMarkNow;
		if(p->needFlush) {
			for(; inSize > 0 && p->tempBufSize < RC_INIT_SIZE; (*srcLen)++, inSize--)
				p->tempBuf[p->tempBufSize++] = *src++;
			if(p->tempBufSize < RC_INIT_SIZE) {
				*status = LZMA_STATUS_NEEDS_MORE_INPUT;
				return SZ_OK;
			}
			if(p->tempBuf[0] != 0)
				return SZ_ERROR_DATA;
			p->code = ((uint32)p->tempBuf[1] << 24) | ((uint32)p->tempBuf[2] << 16) | ((uint32)p->tempBuf[3] << 8) | ((uint32)p->tempBuf[4]);
			p->range = 0xFFFFFFFF;
			p->needFlush = 0;
			p->tempBufSize = 0;
		}

		checkEndMarkNow = 0;
		if(p->dicPos >= dicLimit) {
			if(p->remainLen == 0 && p->code == 0) {
				*status = LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK;
				return SZ_OK;
			}
			if(finishMode == LZMA_FINISH_ANY) {
				*status = LZMA_STATUS_NOT_FINISHED;
				return SZ_OK;
			}
			if(p->remainLen != 0) {
				*status = LZMA_STATUS_NOT_FINISHED;
				return SZ_ERROR_DATA;
			}
			checkEndMarkNow = 1;
		}
		if(p->needInitState)
			LzmaDec_InitStateReal(p);
		if(p->tempBufSize == 0) {
			SizeT processed;
			const Byte * bufLimit;
			if(inSize < LZMA_REQUIRED_INPUT_MAX || checkEndMarkNow) {
				int dummyRes = LzmaDec_TryDummy(p, src, inSize);
				if(dummyRes == DUMMY_ERROR) {
					memcpy(p->tempBuf, src, inSize);
					p->tempBufSize = (uint)inSize;
					(*srcLen) += inSize;
					*status = LZMA_STATUS_NEEDS_MORE_INPUT;
					return SZ_OK;
				}
				if(checkEndMarkNow && dummyRes != DUMMY_MATCH) {
					*status = LZMA_STATUS_NOT_FINISHED;
					return SZ_ERROR_DATA;
				}
				bufLimit = src;
			}
			else
				bufLimit = src + inSize - LZMA_REQUIRED_INPUT_MAX;
			p->buf = src;
			if(LzmaDec_DecodeReal2(p, dicLimit, bufLimit) != 0)
				return SZ_ERROR_DATA;
			processed = (SizeT)(p->buf - src);
			(*srcLen) += processed;
			src += processed;
			inSize -= processed;
		}
		else {
			unsigned rem = p->tempBufSize, lookAhead = 0;
			while(rem < LZMA_REQUIRED_INPUT_MAX && lookAhead < inSize)
				p->tempBuf[rem++] = src[lookAhead++];
			p->tempBufSize = rem;
			if(rem < LZMA_REQUIRED_INPUT_MAX || checkEndMarkNow) {
				int dummyRes = LzmaDec_TryDummy(p, p->tempBuf, rem);
				if(dummyRes == DUMMY_ERROR) {
					(*srcLen) += lookAhead;
					*status = LZMA_STATUS_NEEDS_MORE_INPUT;
					return SZ_OK;
				}
				if(checkEndMarkNow && dummyRes != DUMMY_MATCH) {
					*status = LZMA_STATUS_NOT_FINISHED;
					return SZ_ERROR_DATA;
				}
			}
			p->buf = p->tempBuf;
			if(LzmaDec_DecodeReal2(p, dicLimit, p->buf) != 0)
				return SZ_ERROR_DATA;
			{
				unsigned kkk = (uint)(p->buf - p->tempBuf);
				if(rem < kkk)
					return SZ_ERROR_FAIL;  /* some internal error */
				rem -= kkk;
				if(lookAhead < rem)
					return SZ_ERROR_FAIL;  /* some internal error */
				lookAhead -= rem;
			}
			(*srcLen) += lookAhead;
			src += lookAhead;
			inSize -= lookAhead;
			p->tempBufSize = 0;
		}
	}
	if(p->code == 0)
		*status = LZMA_STATUS_FINISHED_WITH_MARK;
	return (p->code == 0) ? SZ_OK : SZ_ERROR_DATA;
}

SRes LzmaDec_DecodeToBuf(CLzmaDec * p, Byte * dest, SizeT * destLen, const Byte * src, SizeT * srcLen, ELzmaFinishMode finishMode, ELzmaStatus * status)
{
	SizeT outSize = *destLen;
	SizeT inSize = *srcLen;
	*srcLen = *destLen = 0;
	for(;; ) {
		SizeT inSizeCur = inSize, outSizeCur, dicPos;
		ELzmaFinishMode curFinishMode;
		SRes res;
		if(p->dicPos == p->dicBufSize)
			p->dicPos = 0;
		dicPos = p->dicPos;
		if(outSize > p->dicBufSize - dicPos) {
			outSizeCur = p->dicBufSize;
			curFinishMode = LZMA_FINISH_ANY;
		}
		else {
			outSizeCur = dicPos + outSize;
			curFinishMode = finishMode;
		}
		res = LzmaDec_DecodeToDic(p, outSizeCur, src, &inSizeCur, curFinishMode, status);
		src += inSizeCur;
		inSize -= inSizeCur;
		*srcLen += inSizeCur;
		outSizeCur = p->dicPos - dicPos;
		memcpy(dest, p->dic + dicPos, outSizeCur);
		dest += outSizeCur;
		outSize -= outSizeCur;
		*destLen += outSizeCur;
		if(res != 0)
			return res;
		if(outSizeCur == 0 || outSize == 0)
			return SZ_OK;
	}
}

void LzmaDec_FreeProbs(CLzmaDec * p, ISzAllocPtr alloc)
{
	ISzAlloc_Free(alloc, p->probs);
	p->probs = NULL;
}

static void LzmaDec_FreeDict(CLzmaDec * p, ISzAllocPtr alloc)
{
	ISzAlloc_Free(alloc, p->dic);
	p->dic = NULL;
}

void LzmaDec_Free(CLzmaDec * p, ISzAllocPtr alloc)
{
	LzmaDec_FreeProbs(p, alloc);
	LzmaDec_FreeDict(p, alloc);
}

SRes LzmaProps_Decode(CLzmaProps * p, const Byte * data, uint size)
{
	uint32 dicSize;
	Byte d;
	if(size < LZMA_PROPS_SIZE)
		return SZ_ERROR_UNSUPPORTED;
	else
		dicSize = data[1] | ((uint32)data[2] << 8) | ((uint32)data[3] << 16) | ((uint32)data[4] << 24);
	if(dicSize < LZMA_DIC_MIN)
		dicSize = LZMA_DIC_MIN;
	p->dicSize = dicSize;
	d = data[0];
	if(d >= (9 * 5 * 5))
		return SZ_ERROR_UNSUPPORTED;
	p->lc = d % 9;
	d /= 9;
	p->pb = d / 5;
	p->lp = d % 5;
	return SZ_OK;
}

static SRes LzmaDec_AllocateProbs2(CLzmaDec * p, const CLzmaProps * propNew, ISzAllocPtr alloc)
{
	uint32 numProbs = LzmaProps_GetNumProbs(propNew);
	if(!p->probs || numProbs != p->numProbs) {
		LzmaDec_FreeProbs(p, alloc);
		p->probs = (CLzmaProb*)ISzAlloc_Alloc(alloc, numProbs * sizeof(CLzmaProb));
		p->numProbs = numProbs;
		if(!p->probs)
			return SZ_ERROR_MEM;
	}
	return SZ_OK;
}

SRes LzmaDec_AllocateProbs(CLzmaDec * p, const Byte * props, unsigned propsSize, ISzAllocPtr alloc)
{
	CLzmaProps propNew;
	RINOK(LzmaProps_Decode(&propNew, props, propsSize));
	RINOK(LzmaDec_AllocateProbs2(p, &propNew, alloc));
	p->prop = propNew;
	return SZ_OK;
}

SRes LzmaDec_Allocate(CLzmaDec * p, const Byte * props, unsigned propsSize, ISzAllocPtr alloc)
{
	CLzmaProps propNew;
	SizeT dicBufSize;
	RINOK(LzmaProps_Decode(&propNew, props, propsSize));
	RINOK(LzmaDec_AllocateProbs2(p, &propNew, alloc));
	{
		uint32 dictSize = propNew.dicSize;
		SizeT mask = ((uint32)1 << 12) - 1;
		if(dictSize >= ((uint32)1 << 30)) mask = ((uint32)1 << 22) - 1;
		else if(dictSize >= ((uint32)1 << 22)) mask = ((uint32)1 << 20) - 1; ;
		dicBufSize = ((SizeT)dictSize + mask) & ~mask;
		if(dicBufSize < dictSize)
			dicBufSize = dictSize;
	}

	if(!p->dic || dicBufSize != p->dicBufSize) {
		LzmaDec_FreeDict(p, alloc);
		p->dic = (Byte *)ISzAlloc_Alloc(alloc, dicBufSize);
		if(!p->dic) {
			LzmaDec_FreeProbs(p, alloc);
			return SZ_ERROR_MEM;
		}
	}
	p->dicBufSize = dicBufSize;
	p->prop = propNew;
	return SZ_OK;
}

SRes LzmaDecode(Byte * dest, SizeT * destLen, const Byte * src, SizeT * srcLen,
    const Byte * propData, unsigned propSize, ELzmaFinishMode finishMode, ELzmaStatus * status, ISzAllocPtr alloc)
{
	CLzmaDec p;
	SRes res;
	SizeT outSize = *destLen;
	SizeT inSize = *srcLen;
	*destLen = *srcLen = 0;
	*status = LZMA_STATUS_NOT_SPECIFIED;
	if(inSize < RC_INIT_SIZE)
		return SZ_ERROR_INPUT_EOF;
	LzmaDec_Construct(&p);
	RINOK(LzmaDec_AllocateProbs(&p, propData, propSize, alloc));
	p.dic = dest;
	p.dicBufSize = outSize;
	LzmaDec_Init(&p);
	*srcLen = inSize;
	res = LzmaDec_DecodeToDic(&p, outSize, src, srcLen, finishMode, status);
	*destLen = p.dicPos;
	if(res == SZ_OK && *status == LZMA_STATUS_NEEDS_MORE_INPUT)
		res = SZ_ERROR_INPUT_EOF;
	LzmaDec_FreeProbs(&p, alloc);
	return res;
}
//
// Lzma2Enc.c -- LZMA2 Encoder
//
#ifndef _7ZIP_ST
	//#include <MtCoder.h>
#else
	#define NUM_MT_CODER_THREADS_MAX 1
#endif
#define LZMA2_CONTROL_LZMA (1 << 7)
#define LZMA2_CONTROL_COPY_NO_RESET 2
#define LZMA2_CONTROL_COPY_RESET_DIC 1
#define LZMA2_CONTROL_EOF 0
#define LZMA2_LCLP_MAX 4
#define LZMA2_DIC_SIZE_FROM_PROP(p) (((uint32)2 | ((p) & 1)) << ((p) / 2 + 11))
#define LZMA2_PACK_SIZE_MAX (1 << 16)
#define LZMA2_COPY_CHUNK_SIZE LZMA2_PACK_SIZE_MAX
#define LZMA2_UNPACK_SIZE_MAX (1 << 21)
#define LZMA2_KEEP_WINDOW_SIZE LZMA2_UNPACK_SIZE_MAX
#define LZMA2_CHUNK_SIZE_COMPRESSED_MAX ((1 << 16) + 16)
#define PRF(x) /* x */

/* ---------- CLzma2EncInt ---------- */

typedef struct {
	CLzmaEncHandle enc;
	uint64 srcPos;
	Byte props;
	Bool needInitState;
	Bool needInitProp;
} CLzma2EncInt;

static SRes Lzma2EncInt_Init(CLzma2EncInt * p, const CLzma2EncProps * props)
{
	Byte propsEncoded[LZMA_PROPS_SIZE];
	SizeT propsSize = LZMA_PROPS_SIZE;
	RINOK(LzmaEnc_SetProps(p->enc, &props->lzmaProps));
	RINOK(LzmaEnc_WriteProperties(p->enc, propsEncoded, &propsSize));
	p->srcPos = 0;
	p->props = propsEncoded[0];
	p->needInitState = True;
	p->needInitProp = True;
	return SZ_OK;
}

SRes LzmaEnc_PrepareForLzma2(CLzmaEncHandle pp, ISeqInStream * inStream, uint32 keepWindowSize, ISzAllocPtr alloc, ISzAllocPtr allocBig);
SRes LzmaEnc_MemPrepare(CLzmaEncHandle pp, const Byte * src, SizeT srcLen, uint32 keepWindowSize, ISzAllocPtr alloc, ISzAllocPtr allocBig);
SRes LzmaEnc_CodeOneMemBlock(CLzmaEncHandle pp, Bool reInit, Byte * dest, size_t * destLen, uint32 desiredPackSize, uint32 * unpackSize);
const Byte * LzmaEnc_GetCurBuf(CLzmaEncHandle pp);
void LzmaEnc_Finish(CLzmaEncHandle pp);
void LzmaEnc_SaveState(CLzmaEncHandle pp);
void LzmaEnc_RestoreState(CLzmaEncHandle pp);

static SRes Lzma2EncInt_EncodeSubblock(CLzma2EncInt * p, Byte * outBuf, size_t * packSizeRes, ISeqOutStream * outStream)
{
	size_t packSizeLimit = *packSizeRes;
	size_t packSize = packSizeLimit;
	uint32 unpackSize = LZMA2_UNPACK_SIZE_MAX;
	uint   lzHeaderSize = 5 + (p->needInitProp ? 1 : 0);
	Bool   useCopyBlock;
	SRes   res;
	*packSizeRes = 0;
	if(packSize < lzHeaderSize)
		return SZ_ERROR_OUTPUT_EOF;
	packSize -= lzHeaderSize;
	LzmaEnc_SaveState(p->enc);
	res = LzmaEnc_CodeOneMemBlock(p->enc, p->needInitState, outBuf + lzHeaderSize, &packSize, LZMA2_PACK_SIZE_MAX, &unpackSize);
	PRF(printf("\npackSize = %7d unpackSize = %7d  ", packSize, unpackSize));
	if(unpackSize == 0)
		return res;
	if(res == SZ_OK)
		useCopyBlock = (packSize + 2 >= unpackSize || packSize > (1 << 16));
	else {
		if(res != SZ_ERROR_OUTPUT_EOF)
			return res;
		res = SZ_OK;
		useCopyBlock = True;
	}
	if(useCopyBlock) {
		size_t destPos = 0;
		PRF(printf("################# COPY           "));
		while(unpackSize > 0) {
			uint32 u = (unpackSize < LZMA2_COPY_CHUNK_SIZE) ? unpackSize : LZMA2_COPY_CHUNK_SIZE;
			if(packSizeLimit - destPos < u + 3)
				return SZ_ERROR_OUTPUT_EOF;
			outBuf[destPos++] = (Byte)(p->srcPos == 0 ? LZMA2_CONTROL_COPY_RESET_DIC : LZMA2_CONTROL_COPY_NO_RESET);
			outBuf[destPos++] = (Byte)((u - 1) >> 8);
			outBuf[destPos++] = (Byte)(u - 1);
			memcpy(outBuf + destPos, LzmaEnc_GetCurBuf(p->enc) - unpackSize, u);
			unpackSize -= u;
			destPos += u;
			p->srcPos += u;
			if(outStream) {
				*packSizeRes += destPos;
				if(ISeqOutStream_Write(outStream, outBuf, destPos) != destPos)
					return SZ_ERROR_WRITE;
				destPos = 0;
			}
			else
				*packSizeRes = destPos;
			/* needInitState = True; */
		}
		LzmaEnc_RestoreState(p->enc);
		return SZ_OK;
	}
	{
		size_t destPos = 0;
		uint32 u = unpackSize - 1;
		uint32 pm = (uint32)(packSize - 1);
		uint   mode = (p->srcPos == 0) ? 3 : (p->needInitState ? (p->needInitProp ? 2 : 1) : 0);
		PRF(printf("               "));
		outBuf[destPos++] = (Byte)(LZMA2_CONTROL_LZMA | (mode << 5) | ((u >> 16) & 0x1F));
		outBuf[destPos++] = (Byte)(u >> 8);
		outBuf[destPos++] = (Byte)u;
		outBuf[destPos++] = (Byte)(pm >> 8);
		outBuf[destPos++] = (Byte)pm;
		if(p->needInitProp)
			outBuf[destPos++] = p->props;
		p->needInitProp = False;
		p->needInitState = False;
		destPos += packSize;
		p->srcPos += unpackSize;
		if(outStream)
			if(ISeqOutStream_Write(outStream, outBuf, destPos) != destPos)
				return SZ_ERROR_WRITE;
		*packSizeRes = destPos;
		return SZ_OK;
	}
}

/* ---------- Lzma2 Props ---------- */

void Lzma2EncProps_Init(CLzma2EncProps * p)
{
	LzmaEncProps_Init(&p->lzmaProps);
	p->numTotalThreads = -1;
	p->numBlockThreads = -1;
	p->blockSize = 0;
}

void Lzma2EncProps_Normalize(CLzma2EncProps * p)
{
	int t1, t1n, t2, t3;
	{
		CLzmaEncProps lzmaProps = p->lzmaProps;
		LzmaEncProps_Normalize(&lzmaProps);
		t1n = lzmaProps.numThreads;
	}
	t1 = p->lzmaProps.numThreads;
	t2 = p->numBlockThreads;
	t3 = p->numTotalThreads;
	if(t2 > NUM_MT_CODER_THREADS_MAX)
		t2 = NUM_MT_CODER_THREADS_MAX;
	if(t3 <= 0) {
		if(t2 <= 0)
			t2 = 1;
		t3 = t1n * t2;
	}
	else if(t2 <= 0) {
		t2 = t3 / t1n;
		if(t2 == 0) {
			t1 = 1;
			t2 = t3;
		}
		if(t2 > NUM_MT_CODER_THREADS_MAX)
			t2 = NUM_MT_CODER_THREADS_MAX;
	}
	else if(t1 <= 0) {
		t1 = t3 / t2;
		if(t1 == 0)
			t1 = 1;
	}
	else
		t3 = t1n * t2;
	p->lzmaProps.numThreads = t1;
	LzmaEncProps_Normalize(&p->lzmaProps);
	t1 = p->lzmaProps.numThreads;
	if(p->blockSize == 0) {
		uint32 dictSize = p->lzmaProps.dictSize;
		uint64 blockSize = (uint64)dictSize << 2;
		const uint32 kMinSize = (uint32)1 << 20;
		const uint32 kMaxSize = (uint32)1 << 28;
		SETMAX(blockSize, kMinSize);
		SETMIN(blockSize, kMaxSize);
		SETMAX(blockSize, dictSize);
		p->blockSize = (size_t)blockSize;
	}
	if(t2 > 1 && p->lzmaProps.reduceSize != (uint64)(-1LL)) {
		uint64 temp = p->lzmaProps.reduceSize + p->blockSize - 1;
		if(temp > p->lzmaProps.reduceSize) {
			uint64 numBlocks = temp / p->blockSize;
			if(numBlocks < (uint)t2) {
				t2 = (uint)numBlocks;
				SETIFZ(t2, 1);
				t3 = t1 * t2;
			}
		}
	}
	p->numBlockThreads = t2;
	p->numTotalThreads = t3;
}

static SRes Progress(ICompressProgress * p, uint64 inSize, uint64 outSize)
{
	return (p && ICompressProgress_Progress(p, inSize, outSize) != SZ_OK) ? SZ_ERROR_PROGRESS : SZ_OK;
}

/* ---------- Lzma2 ---------- */

typedef struct {
	Byte propEncoded;
	CLzma2EncProps props;
	Byte * outBuf;
	ISzAllocPtr alloc;
	ISzAllocPtr allocBig;
	CLzma2EncInt coders[NUM_MT_CODER_THREADS_MAX];
#ifndef _7ZIP_ST
	CMtCoder mtCoder;
#endif
} CLzma2Enc;

/* ---------- Lzma2EncThread ---------- */

static SRes Lzma2Enc_EncodeMt1(CLzma2EncInt * p, CLzma2Enc * mainEncoder, ISeqOutStream * outStream, ISeqInStream * inStream, ICompressProgress * progress)
{
	uint64 packTotal = 0;
	SRes res = SZ_OK;
	if(!mainEncoder->outBuf) {
		mainEncoder->outBuf = (Byte *)ISzAlloc_Alloc(mainEncoder->alloc, LZMA2_CHUNK_SIZE_COMPRESSED_MAX);
		if(!mainEncoder->outBuf)
			return SZ_ERROR_MEM;
	}
	RINOK(Lzma2EncInt_Init(p, &mainEncoder->props));
	RINOK(LzmaEnc_PrepareForLzma2(p->enc, inStream, LZMA2_KEEP_WINDOW_SIZE, mainEncoder->alloc, mainEncoder->allocBig));
	for(;; ) {
		size_t packSize = LZMA2_CHUNK_SIZE_COMPRESSED_MAX;
		res = Lzma2EncInt_EncodeSubblock(p, mainEncoder->outBuf, &packSize, outStream);
		if(res != SZ_OK)
			break;
		packTotal += packSize;
		res = Progress(progress, p->srcPos, packTotal);
		if(res != SZ_OK)
			break;
		if(packSize == 0)
			break;
	}
	LzmaEnc_Finish(p->enc);
	if(res == SZ_OK) {
		Byte b = 0;
		if(ISeqOutStream_Write(outStream, &b, 1) != 1)
			return SZ_ERROR_WRITE;
	}
	return res;
}

#ifndef _7ZIP_ST

typedef struct {
	IMtCoderCallback funcTable;
	CLzma2Enc * lzma2Enc;
} CMtCallbackImp;

static SRes MtCallbackImp_Code(const IMtCoderCallback * pp, unsigned index, Byte * dest, size_t * destSize,
    const Byte * src, size_t srcSize, int finished)
{
	CMtCallbackImp * imp = CONTAINER_FROM_VTBL(pp, CMtCallbackImp, funcTable);
	CLzma2Enc * mainEncoder = imp->lzma2Enc;
	CLzma2EncInt * p = &mainEncoder->coders[index];
	SRes res = SZ_OK;
	{
		size_t destLim = *destSize;
		*destSize = 0;
		if(srcSize != 0) {
			RINOK(Lzma2EncInt_Init(p, &mainEncoder->props));
			RINOK(LzmaEnc_MemPrepare(p->enc, src, srcSize, LZMA2_KEEP_WINDOW_SIZE, mainEncoder->alloc, mainEncoder->allocBig));
			while(p->srcPos < srcSize) {
				size_t packSize = destLim - *destSize;
				res = Lzma2EncInt_EncodeSubblock(p, dest + *destSize, &packSize, NULL);
				if(res != SZ_OK)
					break;
				*destSize += packSize;
				if(packSize == 0) {
					res = SZ_ERROR_FAIL;
					break;
				}
				if(MtProgress_Set(&mainEncoder->mtCoder.mtProgress, index, p->srcPos, *destSize) != SZ_OK) {
					res = SZ_ERROR_PROGRESS;
					break;
				}
			}
			LzmaEnc_Finish(p->enc);
			if(res != SZ_OK)
				return res;
		}
		if(finished) {
			if(*destSize == destLim)
				return SZ_ERROR_OUTPUT_EOF;
			dest[(*destSize)++] = 0;
		}
	}
	return res;
}

#endif

/* ---------- Lzma2Enc ---------- */

CLzma2EncHandle Lzma2Enc_Create(ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
	CLzma2Enc * p = (CLzma2Enc*)ISzAlloc_Alloc(alloc, sizeof(CLzma2Enc));
	if(!p)
		return NULL;
	Lzma2EncProps_Init(&p->props);
	Lzma2EncProps_Normalize(&p->props);
	p->outBuf = 0;
	p->alloc = alloc;
	p->allocBig = allocBig;
	{
		for(uint i = 0; i < NUM_MT_CODER_THREADS_MAX; i++)
			p->coders[i].enc = 0;
	}
  #ifndef _7ZIP_ST
	MtCoder_Construct(&p->mtCoder);
  #endif
	return p;
}

void Lzma2Enc_Destroy(CLzma2EncHandle pp)
{
	CLzma2Enc * p = (CLzma2Enc*)pp;
	uint i;
	for(i = 0; i < NUM_MT_CODER_THREADS_MAX; i++) {
		CLzma2EncInt * t = &p->coders[i];
		if(t->enc) {
			LzmaEnc_Destroy(t->enc, p->alloc, p->allocBig);
			t->enc = 0;
		}
	}
  #ifndef _7ZIP_ST
	MtCoder_Destruct(&p->mtCoder);
  #endif
	ISzAlloc_Free(p->alloc, p->outBuf);
	ISzAlloc_Free(p->alloc, pp);
}

SRes Lzma2Enc_SetProps(CLzma2EncHandle pp, const CLzma2EncProps * props)
{
	CLzma2Enc * p = (CLzma2Enc*)pp;
	CLzmaEncProps lzmaProps = props->lzmaProps;
	LzmaEncProps_Normalize(&lzmaProps);
	if(lzmaProps.lc + lzmaProps.lp > LZMA2_LCLP_MAX)
		return SZ_ERROR_PARAM;
	p->props = *props;
	Lzma2EncProps_Normalize(&p->props);
	return SZ_OK;
}

Byte Lzma2Enc_WriteProperties(CLzma2EncHandle pp)
{
	CLzma2Enc * p = (CLzma2Enc*)pp;
	uint i;
	uint32 dicSize = LzmaEncProps_GetDictSize(&p->props.lzmaProps);
	for(i = 0; i < 40; i++)
		if(dicSize <= LZMA2_DIC_SIZE_FROM_PROP(i))
			break;
	return (Byte)i;
}

SRes Lzma2Enc_Encode(CLzma2EncHandle pp, ISeqOutStream * outStream, ISeqInStream * inStream, ICompressProgress * progress)
{
	CLzma2Enc * p = (CLzma2Enc*)pp;
	int i;
	for(i = 0; i < p->props.numBlockThreads; i++) {
		CLzma2EncInt * t = &p->coders[(uint)i];
		if(!t->enc) {
			t->enc = LzmaEnc_Create(p->alloc);
			if(!t->enc)
				return SZ_ERROR_MEM;
		}
	}
  #ifndef _7ZIP_ST
	if(p->props.numBlockThreads > 1) {
		CMtCallbackImp mtCallback;
		mtCallback.funcTable.Code = MtCallbackImp_Code;
		mtCallback.lzma2Enc = p;
		p->mtCoder.progress = progress;
		p->mtCoder.inStream = inStream;
		p->mtCoder.outStream = outStream;
		p->mtCoder.alloc = p->alloc;
		p->mtCoder.mtCallback = &mtCallback.funcTable;
		p->mtCoder.blockSize = p->props.blockSize;
		p->mtCoder.destBlockSize = p->props.blockSize + (p->props.blockSize >> 10) + 16;
		if(p->mtCoder.destBlockSize < p->props.blockSize) {
			p->mtCoder.destBlockSize = (size_t)0 - 1;
			if(p->mtCoder.destBlockSize < p->props.blockSize)
				return SZ_ERROR_FAIL;
		}
		p->mtCoder.numThreads = p->props.numBlockThreads;
		return MtCoder_Code(&p->mtCoder);
	}
  #endif
	return Lzma2Enc_EncodeMt1(&p->coders[0], p, outStream, inStream, progress);
}
//
// Lzma2Dec.c -- LZMA2 Decoder
// #define SHOW_DEBUG_INFO 
/*
   00000000  -  EOS
   00000001 U U  -  Uncompressed Reset Dic
   00000010 U U  -  Uncompressed No Reset
   100uuuuu U U P P  -  LZMA no reset
   101uuuuu U U P P  -  LZMA reset state
   110uuuuu U U P P S  -  LZMA reset state + new prop
   111uuuuu U U P P S  -  LZMA reset state + new prop + reset dic

   u, U - Unpack Size
   P - Pack Size
   S - Props
 */
#define LZMA2_CONTROL_LZMA (1 << 7)
#define LZMA2_CONTROL_COPY_NO_RESET 2
#define LZMA2_CONTROL_COPY_RESET_DIC 1
#define LZMA2_CONTROL_EOF 0
#define LZMA2_IS_UNCOMPRESSED_STATE(p) (((p)->control & LZMA2_CONTROL_LZMA) == 0)
#define LZMA2_GET_LZMA_MODE(p) (((p)->control >> 5) & 3)
#define LZMA2_IS_THERE_PROP(mode) ((mode) >= 2)
#define LZMA2_LCLP_MAX 4
#define LZMA2_DIC_SIZE_FROM_PROP(p) (((uint32)2 | ((p) & 1)) << ((p) / 2 + 11))
#ifdef SHOW_DEBUG_INFO
	#define PRF(x) x
#else
	#define PRF(x)
#endif

typedef enum {
	LZMA2_STATE_CONTROL,
	LZMA2_STATE_UNPACK0,
	LZMA2_STATE_UNPACK1,
	LZMA2_STATE_PACK0,
	LZMA2_STATE_PACK1,
	LZMA2_STATE_PROP,
	LZMA2_STATE_DATA,
	LZMA2_STATE_DATA_CONT,
	LZMA2_STATE_FINISHED,
	LZMA2_STATE_ERROR
} ELzma2State;

static SRes Lzma2Dec_GetOldProps(Byte prop, Byte * props)
{
	if(prop > 40)
		return SZ_ERROR_UNSUPPORTED;
	else {
		const uint32 dicSize = (prop == 40) ? 0xFFFFFFFF : LZMA2_DIC_SIZE_FROM_PROP(prop);
		props[0] = (Byte)LZMA2_LCLP_MAX;
		props[1] = (Byte)(dicSize);
		props[2] = (Byte)(dicSize >> 8);
		props[3] = (Byte)(dicSize >> 16);
		props[4] = (Byte)(dicSize >> 24);
		return SZ_OK;
	}
}

SRes Lzma2Dec_AllocateProbs(CLzma2Dec * p, Byte prop, ISzAllocPtr alloc)
{
	Byte props[LZMA_PROPS_SIZE];
	RINOK(Lzma2Dec_GetOldProps(prop, props));
	return LzmaDec_AllocateProbs(&p->decoder, props, LZMA_PROPS_SIZE, alloc);
}

SRes Lzma2Dec_Allocate(CLzma2Dec * p, Byte prop, ISzAllocPtr alloc)
{
	Byte props[LZMA_PROPS_SIZE];
	RINOK(Lzma2Dec_GetOldProps(prop, props));
	return LzmaDec_Allocate(&p->decoder, props, LZMA_PROPS_SIZE, alloc);
}

void Lzma2Dec_Init(CLzma2Dec * p)
{
	p->state = LZMA2_STATE_CONTROL;
	p->needInitDic = True;
	p->needInitState = True;
	p->needInitProp = True;
	LzmaDec_Init(&p->decoder);
}

static ELzma2State Lzma2Dec_UpdateState(CLzma2Dec * p, Byte b)
{
	switch(p->state) {
		case LZMA2_STATE_CONTROL:
		    p->control = b;
		    PRF(printf("\n %4X ", (uint)p->decoder.dicPos));
		    PRF(printf(" %2X", (uint)b));
		    if(b == 0)
			    return LZMA2_STATE_FINISHED;
		    if(LZMA2_IS_UNCOMPRESSED_STATE(p)) {
			    if(b > 2)
				    return LZMA2_STATE_ERROR;
			    p->unpackSize = 0;
		    }
		    else
			    p->unpackSize = (uint32)(b & 0x1F) << 16;
		    return LZMA2_STATE_UNPACK0;
		case LZMA2_STATE_UNPACK0:
		    p->unpackSize |= (uint32)b << 8;
		    return LZMA2_STATE_UNPACK1;
		case LZMA2_STATE_UNPACK1:
		    p->unpackSize |= (uint32)b;
		    p->unpackSize++;
		    PRF(printf(" %8u", (uint)p->unpackSize));
		    return (LZMA2_IS_UNCOMPRESSED_STATE(p)) ? LZMA2_STATE_DATA : LZMA2_STATE_PACK0;
		case LZMA2_STATE_PACK0:
		    p->packSize = (uint32)b << 8;
		    return LZMA2_STATE_PACK1;
		case LZMA2_STATE_PACK1:
		    p->packSize |= (uint32)b;
		    p->packSize++;
		    PRF(printf(" %8u", (uint)p->packSize));
		    return LZMA2_IS_THERE_PROP(LZMA2_GET_LZMA_MODE(p)) ? LZMA2_STATE_PROP : (p->needInitProp ? LZMA2_STATE_ERROR : LZMA2_STATE_DATA);
		case LZMA2_STATE_PROP:
	    {
		    uint   lc, lp;
		    if(b >= (9 * 5 * 5))
			    return LZMA2_STATE_ERROR;
			else {
				lc = b % 9;
				b /= 9;
				p->decoder.prop.pb = b / 5;
				lp = b % 5;
				if((lc + lp) > LZMA2_LCLP_MAX)
					return LZMA2_STATE_ERROR;
				else {
					p->decoder.prop.lc = lc;
					p->decoder.prop.lp = lp;
					p->needInitProp = False;
					return LZMA2_STATE_DATA;
				}
			}
	    }
	}
	return LZMA2_STATE_ERROR;
}

static void LzmaDec_UpdateWithUncompressed(CLzmaDec * p, const Byte * src, SizeT size)
{
	memcpy(p->dic + p->dicPos, src, size);
	p->dicPos += size;
	if(p->checkDicSize == 0 && p->prop.dicSize - p->processedPos <= size)
		p->checkDicSize = p->prop.dicSize;
	p->processedPos += (uint32)size;
}

void LzmaDec_InitDicAndState(CLzmaDec * p, Bool initDic, Bool initState);

SRes Lzma2Dec_DecodeToDic(CLzma2Dec * p, SizeT dicLimit, const Byte * src, SizeT * srcLen, ELzmaFinishMode finishMode, ELzmaStatus * status)
{
	SizeT inSize = *srcLen;
	*srcLen = 0;
	*status = LZMA_STATUS_NOT_SPECIFIED;
	while(p->state != LZMA2_STATE_ERROR) {
		SizeT dicPos;
		if(p->state == LZMA2_STATE_FINISHED) {
			*status = LZMA_STATUS_FINISHED_WITH_MARK;
			return SZ_OK;
		}
		else {
			dicPos = p->decoder.dicPos;
			if(dicPos == dicLimit && finishMode == LZMA_FINISH_ANY) {
				*status = LZMA_STATUS_NOT_FINISHED;
				return SZ_OK;
			}
			else {
				if(!oneof2(p->state, LZMA2_STATE_DATA, LZMA2_STATE_DATA_CONT)) {
					if(*srcLen == inSize) {
						*status = LZMA_STATUS_NEEDS_MORE_INPUT;
						return SZ_OK;
					}
					else {
						(*srcLen)++;
						p->state = Lzma2Dec_UpdateState(p, *src++);
						if(dicPos == dicLimit && p->state != LZMA2_STATE_FINISHED)
							break;
						continue;
					}
				}
				else {
					SizeT inCur = inSize - *srcLen;
					SizeT outCur = dicLimit - dicPos;
					ELzmaFinishMode curFinishMode = LZMA_FINISH_ANY;
					if(outCur >= p->unpackSize) {
						outCur = (SizeT)p->unpackSize;
						curFinishMode = LZMA_FINISH_END;
					}
					if(LZMA2_IS_UNCOMPRESSED_STATE(p)) {
						if(inCur == 0) {
							*status = LZMA_STATUS_NEEDS_MORE_INPUT;
							return SZ_OK;
						}
						else {
							if(p->state == LZMA2_STATE_DATA) {
								Bool initDic = (p->control == LZMA2_CONTROL_COPY_RESET_DIC);
								if(initDic)
									p->needInitProp = p->needInitState = True;
								else if(p->needInitDic)
									break;
								p->needInitDic = False;
								LzmaDec_InitDicAndState(&p->decoder, initDic, False);
							}
							SETMIN(inCur, outCur);
							if(inCur == 0)
								break;
							LzmaDec_UpdateWithUncompressed(&p->decoder, src, inCur);
							src += inCur;
							*srcLen += inCur;
							p->unpackSize -= (uint32)inCur;
							p->state = (p->unpackSize == 0) ? LZMA2_STATE_CONTROL : LZMA2_STATE_DATA_CONT;
						}
					}
					else {
						SRes res;
						if(p->state == LZMA2_STATE_DATA) {
							uint   mode = LZMA2_GET_LZMA_MODE(p);
							Bool   initDic = (mode == 3);
							Bool   initState = (mode != 0);
							if((!initDic && p->needInitDic) || (!initState && p->needInitState))
								break;
							LzmaDec_InitDicAndState(&p->decoder, initDic, initState);
							p->needInitDic = False;
							p->needInitState = False;
							p->state = LZMA2_STATE_DATA_CONT;
						}
						SETMIN(inCur, (SizeT)p->packSize);
						res = LzmaDec_DecodeToDic(&p->decoder, dicPos + outCur, src, &inCur, curFinishMode, status);
						src += inCur;
						*srcLen += inCur;
						p->packSize -= (uint32)inCur;
						outCur = p->decoder.dicPos - dicPos;
						p->unpackSize -= (uint32)outCur;
						if(res != 0)
							break;
						if(*status == LZMA_STATUS_NEEDS_MORE_INPUT) {
							if(p->packSize == 0)
								break;
							return SZ_OK;
						}
						else {
							if(inCur == 0 && outCur == 0) {
								if(*status != LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK || p->unpackSize != 0 || p->packSize != 0)
									break;
								p->state = LZMA2_STATE_CONTROL;
							}
							*status = LZMA_STATUS_NOT_SPECIFIED;
						}
					}
				}
			}
		}
	}
	*status = LZMA_STATUS_NOT_SPECIFIED;
	p->state = LZMA2_STATE_ERROR;
	return SZ_ERROR_DATA;
}

SRes Lzma2Dec_DecodeToBuf(CLzma2Dec * p, Byte * dest, SizeT * destLen, const Byte * src, SizeT * srcLen, ELzmaFinishMode finishMode, ELzmaStatus * status)
{
	SizeT outSize = *destLen, inSize = *srcLen;
	*srcLen = *destLen = 0;
	for(;; ) {
		SizeT inCur = inSize;
		SizeT outCur;
		SizeT dicPos;
		ELzmaFinishMode curFinishMode;
		SRes res;
		if(p->decoder.dicPos == p->decoder.dicBufSize)
			p->decoder.dicPos = 0;
		dicPos = p->decoder.dicPos;
		curFinishMode = LZMA_FINISH_ANY;
		outCur = p->decoder.dicBufSize - dicPos;
		if(outCur >= outSize) {
			outCur = outSize;
			curFinishMode = finishMode;
		}
		res = Lzma2Dec_DecodeToDic(p, dicPos + outCur, src, &inCur, curFinishMode, status);
		src += inCur;
		inSize -= inCur;
		*srcLen += inCur;
		outCur = p->decoder.dicPos - dicPos;
		memcpy(dest, p->decoder.dic + dicPos, outCur);
		dest += outCur;
		outSize -= outCur;
		*destLen += outCur;
		if(res != 0)
			return res;
		if(outCur == 0 || outSize == 0)
			return SZ_OK;
	}
}

SRes Lzma2Decode(Byte * dest, SizeT * destLen, const Byte * src, SizeT * srcLen, Byte prop, ELzmaFinishMode finishMode, ELzmaStatus * status, ISzAllocPtr alloc)
{
	CLzma2Dec p;
	SRes res;
	SizeT outSize = *destLen, inSize = *srcLen;
	*destLen = *srcLen = 0;
	*status = LZMA_STATUS_NOT_SPECIFIED;
	Lzma2Dec_Construct(&p);
	RINOK(Lzma2Dec_AllocateProbs(&p, prop, alloc));
	p.decoder.dic = dest;
	p.decoder.dicBufSize = outSize;
	Lzma2Dec_Init(&p);
	*srcLen = inSize;
	res = Lzma2Dec_DecodeToDic(&p, outSize, src, srcLen, finishMode, status);
	*destLen = p.decoder.dicPos;
	if(res == SZ_OK && *status == LZMA_STATUS_NEEDS_MORE_INPUT)
		res = SZ_ERROR_INPUT_EOF;
	Lzma2Dec_FreeProbs(&p, alloc);
	return res;
}
