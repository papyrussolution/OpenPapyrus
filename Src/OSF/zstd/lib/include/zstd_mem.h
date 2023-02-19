/*
 * Copyright (c) Yann Collet, Facebook, Inc. All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */
#ifndef __ZSTD_MEM_H_MODULE
#define __ZSTD_MEM_H_MODULE

#if defined (__cplusplus)
extern "C" {
#endif

#include "compiler.h"  /* __has_builtin */
#include "debug.h"  /* DEBUG_STATIC_ASSERT */
#include "zstd_deps.h"  /* memcpy */
//
// Compiler specifics
//
#if defined(__GNUC__)
#define MEM_STATIC static __inline __attribute__((unused))
#elif defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */)
#define MEM_STATIC static inline
#elif defined(_MSC_VER)
	#define MEM_STATIC static __inline
#else
	#define MEM_STATIC static // this version may generate warnings for unused static functions; disable the relevant warning 
#endif
// 
// Basic Types
// 
#if  !defined (__VMS) && (defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /*C99*/))
#if defined(_AIX)
	#include <inttypes.h>
#else
	#include <stdint.h> /* intptr_t */
#endif
//typedef uint8_t  BYTE_Removed;
//typedef uint8_t  U8_Removed;
//typedef int8_t   S8_Removed;
//typedef uint16_t U16_Removed;
//typedef int16_t  S16_Removed;
//typedef uint32_t U32_Removed;
//typedef int32_t  S32_Removed;
//typedef uint64_t U64_Removed;
//typedef int64_t  S64_Removed;
#else
#include <limits.h>
#if CHAR_BIT != 8
#error "this implementation requires char to be exactly 8-bit type"
#endif
//typedef unsigned char BYTE_Removed;
//typedef unsigned char U8_Removed;
//typedef signed char   S8_Removed;
#if USHRT_MAX != 65535
	#error "this implementation requires short to be exactly 16-bit type"
#endif
//typedef unsigned short U16_Removed;
//typedef   signed short S16_Removed;
#if UINT_MAX != 4294967295
	#error "this implementation requires int to be exactly 32-bit type"
#endif
//typedef unsigned int U32_Removed;
//typedef  signed int S32_Removed;
// note : there are no limits defined for long long type in C90.
// limits exist in C99, however, in such case, <stdint.h> is preferred 
//typedef uint64 U64_Removed;
//typedef   signed long long S64_Removed;
#endif
//
// Memory I/O
//
/*=== Static platform detection ===*/
MEM_STATIC uint MEM_32bits(void);
MEM_STATIC uint MEM_64bits(void);
MEM_STATIC uint MEM_isLittleEndian();

/*=== Native unaligned read/write ===*/
MEM_STATIC uint16 MEM_read16(const void* memPtr);
MEM_STATIC uint32 MEM_read32(const void* memPtr);
MEM_STATIC uint64 MEM_read64(const void* memPtr);
MEM_STATIC size_t MEM_readST(const void* memPtr);

MEM_STATIC void MEM_write16(void* memPtr, uint16 value);
MEM_STATIC void MEM_write32(void* memPtr, uint32 value);
MEM_STATIC void MEM_write64(void* memPtr, uint64 value);

/*=== Little endian unaligned read/write ===*/
MEM_STATIC uint16 MEM_readLE16(const void* memPtr);
MEM_STATIC uint32 MEM_readLE24(const void* memPtr);
MEM_STATIC uint32 MEM_readLE32(const void* memPtr);
MEM_STATIC uint64 MEM_readLE64(const void* memPtr);
MEM_STATIC size_t MEM_readLEST(const void* memPtr);

MEM_STATIC void MEM_writeLE16(void* memPtr, uint16 val);
MEM_STATIC void MEM_writeLE24(void* memPtr, uint32 val);
MEM_STATIC void MEM_writeLE32(void* memPtr, uint32 val32);
MEM_STATIC void MEM_writeLE64(void* memPtr, uint64 val64);
MEM_STATIC void MEM_writeLEST(void* memPtr, size_t val);

/*=== Big endian unaligned read/write ===*/
MEM_STATIC uint32 MEM_readBE32(const void* memPtr);
MEM_STATIC uint64 MEM_readBE64(const void* memPtr);
MEM_STATIC size_t MEM_readBEST(const void* memPtr);

MEM_STATIC void MEM_writeBE32(void* memPtr, uint32 val32);
MEM_STATIC void MEM_writeBE64(void* memPtr, uint64 val64);
MEM_STATIC void MEM_writeBEST(void* memPtr, size_t val);

/*=== Byteswap ===*/
MEM_STATIC uint32 MEM_swap32(uint32 in);
MEM_STATIC uint64 MEM_swap64(uint64 in);
MEM_STATIC size_t MEM_swapST(size_t in);
//
// Memory I/O
//
/* MEM_FORCE_MEMORY_ACCESS :
 * By default, access to unaligned memory is controlled by `memcpy()`, which is safe and portable.
 * Unfortunately, on some target/compiler combinations, the generated assembly is sub-optimal.
 * The below switch allow to select different access method for improved performance.
 * Method 0 (default) : use `memcpy()`. Safe and portable.
 * Method 1 : `__packed` statement. It depends on compiler extension (i.e., not portable).
 *            This method is safe if your compiler supports it, and *generally* as fast or faster than `memcpy`.
 * Method 2 : direct access. This method is portable but violate C standard.
 *            It can generate buggy code on targets depending on alignment.
 *            In some circumstances, it's the only known way to get the most performance (i.e. GCC + ARMv6)
 * See http://fastcompression.blogspot.fr/2015/08/accessing-unaligned-memory.html for details.
 * Prefer these methods in priority order (0 > 1 > 2)
 */
#ifndef MEM_FORCE_MEMORY_ACCESS   /* can be defined externally, on command line for example */
	#if defined(__INTEL_COMPILER) || defined(__GNUC__) || defined(__ICCARM__)
		#define MEM_FORCE_MEMORY_ACCESS 1
	#endif
#endif

inline uint MEM_32bits() { return sizeof(size_t)==4; }
inline uint MEM_64bits() { return sizeof(size_t)==8; }

MEM_STATIC uint MEM_isLittleEndian()
{
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
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
	const union { uint32 u; BYTE c[4]; } one = { 1 }; /* don't use static : performance detrimental  */
	return one.c[0];
#endif
}

#if defined(MEM_FORCE_MEMORY_ACCESS) && (MEM_FORCE_MEMORY_ACCESS==2)

/* violates C standard, by lying on structure alignment.
   Only use if no other choice to achieve best performance on target platform */
MEM_STATIC uint16 MEM_read16(const void* memPtr) { return *(const uint16*)memPtr; }
MEM_STATIC uint32 MEM_read32(const void* memPtr) { return *(const uint32 *)memPtr; }
MEM_STATIC uint64 MEM_read64(const void* memPtr) { return *(const uint64*)memPtr; }
MEM_STATIC size_t MEM_readST(const void* memPtr) { return *(const size_t*)memPtr; }
MEM_STATIC void MEM_write16(void* memPtr, uint16 value) { *(uint16*)memPtr = value; }
MEM_STATIC void MEM_write32(void* memPtr, uint32 value) { *(uint32 *)memPtr = value; }
MEM_STATIC void MEM_write64(void* memPtr, uint64 value) { *(uint64*)memPtr = value; }

#elif defined(MEM_FORCE_MEMORY_ACCESS) && (MEM_FORCE_MEMORY_ACCESS==1)

/* __pack instructions are safer, but compiler specific, hence potentially problematic for some compilers */
/* currently only defined for gcc and icc */
#if defined(_MSC_VER) || (defined(__INTEL_COMPILER) && defined(WIN32))
__pragma(pack(push, 1) )
typedef struct { uint16 v; } unalign16;
typedef struct { uint32 v; } unalign32;
typedef struct { uint64 v; } unalign64;
typedef struct { size_t v; } unalignArch;
__pragma(pack(pop) )
#else
typedef struct { uint16 v; } __attribute__((packed)) unalign16;
typedef struct { uint32 v; } __attribute__((packed)) unalign32;
typedef struct { uint64 v; } __attribute__((packed)) unalign64;
typedef struct { size_t v; } __attribute__((packed)) unalignArch;
#endif

MEM_STATIC uint16 MEM_read16(const void* ptr) { return ((const unalign16*)ptr)->v; }
MEM_STATIC uint32 MEM_read32(const void* ptr) { return ((const unalign32*)ptr)->v; }
MEM_STATIC uint64 MEM_read64(const void* ptr) { return ((const unalign64*)ptr)->v; }
MEM_STATIC size_t MEM_readST(const void* ptr) { return ((const unalignArch*)ptr)->v; }
MEM_STATIC void MEM_write16(void* memPtr, uint16 value) { ((unalign16*)memPtr)->v = value; }
MEM_STATIC void MEM_write32(void* memPtr, uint32 value) { ((unalign32*)memPtr)->v = value; }
MEM_STATIC void MEM_write64(void* memPtr, uint64 value) { ((unalign64*)memPtr)->v = value; }

#else
//
// default method, safe and standard. can sometimes prove slower 
//
MEM_STATIC uint16 MEM_read16(const void* memPtr)
{
	uint16 val; 
	memcpy(&val, memPtr, sizeof(val)); return val;
}

MEM_STATIC uint32 MEM_read32(const void* memPtr)
{
	uint32 val; 
	memcpy(&val, memPtr, sizeof(val)); return val;
}

MEM_STATIC uint64 MEM_read64(const void* memPtr)
{
	uint64 val; 
	memcpy(&val, memPtr, sizeof(val)); return val;
}

MEM_STATIC size_t MEM_readST(const void* memPtr)
{
	size_t val; 
	memcpy(&val, memPtr, sizeof(val)); return val;
}

MEM_STATIC void MEM_write16(void* memPtr, uint16 value) { memcpy(memPtr, &value, sizeof(value)); }
MEM_STATIC void MEM_write32(void* memPtr, uint32 value) { memcpy(memPtr, &value, sizeof(value)); }
MEM_STATIC void MEM_write64(void* memPtr, uint64 value) { memcpy(memPtr, &value, sizeof(value)); }

#endif /* MEM_FORCE_MEMORY_ACCESS */

MEM_STATIC uint32 MEM_swap32_fallback(uint32 in)
{
	return ((in << 24) & 0xff000000 ) | ((in <<  8) & 0x00ff0000 ) | ((in >>  8) & 0x0000ff00 ) | ((in >> 24) & 0x000000ff );
}

MEM_STATIC uint32 MEM_swap32(uint32 in)
{
#if defined(_MSC_VER)     /* Visual Studio */
	return _byteswap_ulong(in);
#elif (defined (__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 403)) || (defined(__clang__) && __has_builtin(__builtin_bswap32))
	return __builtin_bswap32(in);
#else
	return MEM_swap32_fallback(in);
#endif
}

MEM_STATIC uint64 MEM_swap64_fallback(uint64 in)
{
	return ((in << 56) & 0xff00000000000000ULL) | ((in << 40) & 0x00ff000000000000ULL) | ((in << 24) & 0x0000ff0000000000ULL) |
	       ((in << 8)  & 0x000000ff00000000ULL) | ((in >> 8)  & 0x00000000ff000000ULL) | ((in >> 24) & 0x0000000000ff0000ULL) |
	       ((in >> 40) & 0x000000000000ff00ULL) | ((in >> 56) & 0x00000000000000ffULL);
}

MEM_STATIC uint64 MEM_swap64(uint64 in)
{
#if defined(_MSC_VER)     /* Visual Studio */
	return _byteswap_uint64(in);
#elif (defined (__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 403)) || (defined(__clang__) && __has_builtin(__builtin_bswap64))
	return __builtin_bswap64(in);
#else
	return MEM_swap64_fallback(in);
#endif
}

MEM_STATIC size_t MEM_swapST(size_t in)
{
	if(MEM_32bits())
		return (size_t)MEM_swap32((uint32)in);
	else
		return (size_t)MEM_swap64((uint64)in);
}

/*=== Little endian r/w ===*/

inline uint16 MEM_readLE16(const void* memPtr)
{
	if(MEM_isLittleEndian())
		return MEM_read16(memPtr);
	else {
		const BYTE * p = (const BYTE *)memPtr;
		return (uint16)(p[0] + (p[1]<<8));
	}
}

inline void MEM_writeLE16(void* memPtr, uint16 val)
{
	if(MEM_isLittleEndian()) {
		MEM_write16(memPtr, val);
	}
	else {
		BYTE * p = (BYTE *)memPtr;
		p[0] = (BYTE)val;
		p[1] = (BYTE)(val>>8);
	}
}

inline uint32 MEM_readLE24(const void* memPtr)
{
	return (uint32)MEM_readLE16(memPtr) + ((uint32)(((const BYTE *)memPtr)[2]) << 16);
}

inline void MEM_writeLE24(void* memPtr, uint32 val)
{
	MEM_writeLE16(memPtr, (uint16)val);
	((BYTE *)memPtr)[2] = (BYTE)(val>>16);
}

MEM_STATIC uint32 MEM_readLE32(const void* memPtr)
{
	if(MEM_isLittleEndian())
		return MEM_read32(memPtr);
	else
		return MEM_swap32(MEM_read32(memPtr));
}

MEM_STATIC void MEM_writeLE32(void* memPtr, uint32 val32)
{
	if(MEM_isLittleEndian())
		MEM_write32(memPtr, val32);
	else
		MEM_write32(memPtr, MEM_swap32(val32));
}

MEM_STATIC uint64 MEM_readLE64(const void* memPtr)
{
	if(MEM_isLittleEndian())
		return MEM_read64(memPtr);
	else
		return MEM_swap64(MEM_read64(memPtr));
}

MEM_STATIC void MEM_writeLE64(void* memPtr, uint64 val64)
{
	if(MEM_isLittleEndian())
		MEM_write64(memPtr, val64);
	else
		MEM_write64(memPtr, MEM_swap64(val64));
}

MEM_STATIC size_t MEM_readLEST(const void* memPtr)
{
	if(MEM_32bits())
		return (size_t)MEM_readLE32(memPtr);
	else
		return (size_t)MEM_readLE64(memPtr);
}

MEM_STATIC void MEM_writeLEST(void* memPtr, size_t val)
{
	if(MEM_32bits())
		MEM_writeLE32(memPtr, (uint32)val);
	else
		MEM_writeLE64(memPtr, (uint64)val);
}

/*=== Big endian r/w ===*/

MEM_STATIC uint32 MEM_readBE32(const void* memPtr)
{
	if(MEM_isLittleEndian())
		return MEM_swap32(MEM_read32(memPtr));
	else
		return MEM_read32(memPtr);
}

MEM_STATIC void MEM_writeBE32(void* memPtr, uint32 val32)
{
	if(MEM_isLittleEndian())
		MEM_write32(memPtr, MEM_swap32(val32));
	else
		MEM_write32(memPtr, val32);
}

MEM_STATIC uint64 MEM_readBE64(const void* memPtr)
{
	if(MEM_isLittleEndian())
		return MEM_swap64(MEM_read64(memPtr));
	else
		return MEM_read64(memPtr);
}

MEM_STATIC void MEM_writeBE64(void* memPtr, uint64 val64)
{
	if(MEM_isLittleEndian())
		MEM_write64(memPtr, MEM_swap64(val64));
	else
		MEM_write64(memPtr, val64);
}

MEM_STATIC size_t MEM_readBEST(const void* memPtr)
{
	if(MEM_32bits())
		return (size_t)MEM_readBE32(memPtr);
	else
		return (size_t)MEM_readBE64(memPtr);
}

MEM_STATIC void MEM_writeBEST(void* memPtr, size_t val)
{
	if(MEM_32bits())
		MEM_writeBE32(memPtr, (uint32)val);
	else
		MEM_writeBE64(memPtr, (uint64)val);
}

/* code only tested on 32 and 64 bits systems */
MEM_STATIC void MEM_check(void) { DEBUG_STATIC_ASSERT((sizeof(size_t)==4) || (sizeof(size_t)==8)); }

#if defined (__cplusplus)
}
#endif

#endif // __ZSTD_MEM_H_MODULE
