// ZSTD-INTERNAL.H
//
#ifndef __ZSTD_INTERNAL_H
#define __ZSTD_INTERNAL_H

#include <slib.h>
#if defined(_MSC_VER)
	#include <intrin.h>
#endif
#include <xxhash.h> /* XXH64_* */
//#include <zstd_internal.h> /* includes zstd.h */
#define ZSTD_MULTITHREAD
#define ZSTD_GZCOMPRESS
#define ZSTD_GZDECOMPRESS
#define ZSTD_LZMACOMPRESS
#define ZSTD_LZMADECOMPRESS
#define ZSTD_LZ4COMPRESS
#define ZSTD_LZ4DECOMPRESS

// @sobolev {
inline constexpr uint MEM_32bits() { return sizeof(size_t)==4; }
inline constexpr uint MEM_64bits() { return sizeof(size_t)==8; }

FORCEINLINE constexpr uint MEM_isLittleEndian()
{
#if defined(SL_LITTLEENDIAN)
	return 1;
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
	return 1;
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
	return 0;
#elif defined(__clang__) && __LITTLE_ENDIAN__
	return 1;
#elif defined(__clang__) && __BIG_ENDIAN__
	return 0;
#elif defined(_MSC_VER) && (_M_AMD64 || _M_IX86)
	return 1;
#elif defined(__DMC__) && defined(_M_IX86)
	return 1;
#else
	constexpr union { uint32 u; BYTE c[4]; } one = { 1 }; // don't use static : performance detrimental
	return one.c[0];
#endif
}

#define HUF_MAX_SYMBOL_VALUE     255
#define HUF_MAX_TABLELOG          12 // max configured tableLog (for static allocation); can be modified up to HUF_ABSOLUTEMAX_TABLELOG
#define HUF_DEFAULT_TABLELOG  HUF_MAX_TABLELOG // tableLog by default, when not specified
#define HUF_ABSOLUTEMAX_TABLELOG  16 // absolute limit of HUF_MAX_TABLELOG. Beyond that value, code does not work
#if (HUF_MAX_TABLELOG > HUF_ABSOLUTEMAX_TABLELOG)
	#error "HUF_MAX_TABLELOG is too large !"
#endif

typedef enum { 
	BIT_DStream_unfinished = 0,
	BIT_DStream_endOfBuffer = 1,
	BIT_DStream_completed = 2,
	BIT_DStream_overflow = 3 
} BIT_DStream_status;  // result of BIT_reloadDStream()
// 1,2,4,8 would be better for bitmap combinations, but slows down performance a bit ... :( 
// 
// HEAPMODE :
// Select how default decompression function ZSTD_decompress() allocates its context,
// on stack (0), or into heap (1, default; requires SAlloc::M()).
// Note that functions with explicit context such as ZSTD_decompressDCtx() are unaffected.
// 
#ifndef ZSTD_HEAPMODE
	#define ZSTD_HEAPMODE 1
#endif

// } @sobolev

#endif // !__ZSTD_INTERNAL_H
