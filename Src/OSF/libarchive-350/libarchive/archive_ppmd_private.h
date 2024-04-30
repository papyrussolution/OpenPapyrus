/* Ppmd.h -- PPMD codec common code
   2010-03-12 : Igor Pavlov : Public domain
   This code is based on PPMd var.H (2001): Dmitry Shkarin : Public domain */

#ifndef ARCHIVE_PPMD_PRIVATE_H_INCLUDED
#define ARCHIVE_PPMD_PRIVATE_H_INCLUDED
#ifndef __LIBARCHIVE_BUILD
	#error This header is only to be used internally to libarchive.
#endif

/*** Begin defined in Types.h ***/

#if !defined(ZCONF_H)
typedef uchar Byte;
#endif
typedef short Int16;
typedef unsigned short UInt16;
#ifdef _LZMA_UINT32_IS_ULONG
	typedef long Int32;
	typedef unsigned long UInt32;
#else
	typedef int Int32;
	typedef uint UInt32;
#endif
//#ifdef _SZ_NO_INT_64
	// define _SZ_NO_INT_64, if your compiler doesn't support 64-bit integers.
	// NOTES: Some code will work incorrectly in that case!
	//typedef long Int64_Removed;
	//typedef unsigned long UInt64_Removed;
//#else
	//#if defined(_MSC_VER) || defined(__BORLANDC__)
		//typedef __int64 Int64_Removed;
		//typedef unsigned __int64 UInt64_Removed;
		//#define UINT64_CONST(n) n
	//#else
		//typedef long long int Int64_Removed;
		//typedef unsigned long long int UInt64_Removed;
		//#define UINT64_CONST(n) n ## ULL
	//#endif
//#endif
// @sobolev typedef int Bool;
// @sobolev #define True 1
// @sobolev #define False 0

/* The following interfaces use first parameter as pointer to structure */

typedef struct {
	ArchiveRead * a;
	Byte (* Read)(void * p); // reads one byte, returns 0 in case of EOF or error
} IByteIn;

typedef struct {
	struct archive_write * a;
	void (* Write)(void * p, Byte b);
} IByteOut;

/*** End defined in Types.h ***/
/*** Begin defined in CpuArch.h ***/

#if defined(_M_IX86) || defined(__i386__)
	#define MY_CPU_X86
#endif
#if defined(MY_CPU_X86) || defined(_M_ARM)
	#define MY_CPU_32BIT
#endif
#ifdef MY_CPU_32BIT
	#define PPMD_32BIT
#endif

/*** End defined in CpuArch.h ***/

#define PPMD_INT_BITS 7
#define PPMD_PERIOD_BITS 7
#define PPMD_BIN_SCALE (1 << (PPMD_INT_BITS + PPMD_PERIOD_BITS))

#define PPMD_GET_MEAN_SPEC(summ, shift, round) (((summ) + (1 << ((shift) - (round)))) >> (shift))
#define PPMD_GET_MEAN(summ) PPMD_GET_MEAN_SPEC((summ), PPMD_PERIOD_BITS, 2)
#define PPMD_UPDATE_PROB_0(prob) ((prob) + (1 << PPMD_INT_BITS) - PPMD_GET_MEAN(prob))
#define PPMD_UPDATE_PROB_1(prob) ((prob) - PPMD_GET_MEAN(prob))

#define PPMD_N1 4
#define PPMD_N2 4
#define PPMD_N3 4
#define PPMD_N4 ((128 + 3 - 1 * PPMD_N1 - 2 * PPMD_N2 - 3 * PPMD_N3) / 4)
#define PPMD_NUM_INDEXES (PPMD_N1 + PPMD_N2 + PPMD_N3 + PPMD_N4)

/* SEE-contexts for PPM-contexts with masked symbols */
typedef struct {
	UInt16 Summ; /* Freq */
	Byte Shift; /* Speed of Freq change; low Shift is for fast change */
	Byte Count; /* Count to next change of Shift */
} CPpmd_See;

#define Ppmd_See_Update(p)  if((p)->Shift < PPMD_PERIOD_BITS && --(p)->Count == 0) \
	{ (p)->Summ <<= 1; (p)->Count = (Byte)(3 << (p)->Shift++); }

typedef struct {
	Byte Symbol;
	Byte Freq;
	UInt16 SuccessorLow;
	UInt16 SuccessorHigh;
} CPpmd_State;

typedef
  #ifdef PPMD_32BIT
    CPpmd_State *
  #else
    UInt32
  #endif
    CPpmd_State_Ref;

typedef
  #ifdef PPMD_32BIT
    void *
  #else
    UInt32
  #endif
    CPpmd_Void_Ref;

typedef
  #ifdef PPMD_32BIT
    Byte *
  #else
    UInt32
  #endif
    CPpmd_Byte_Ref;

#define PPMD_SetAllBitsIn256Bytes(p) \
	{ for(uint j = 0; j < 256 / sizeof(p[0]); j += 8) { p[j+7] = p[j+6] = p[j+5] = p[j+4] = p[j+3] = p[j+2] = p[j+1] = p[j+0] = ~(size_t)0; }}

#endif
