/*
 * Copyright (c) Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */
#ifndef ZSTD_BITS_H
#define ZSTD_BITS_H

#include "zstd_mem.h"

MEM_STATIC uint ZSTD_countTrailingZeros32_fallback(uint32 val)
{
	assert(val != 0);
	{
		static const int DeBruijnBytePos[32] = {0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9};
		return DeBruijnBytePos[((uint32)((val & -(int32)val) * 0x077CB531U)) >> 27];
	}
}

MEM_STATIC uint ZSTD_countTrailingZeros32(uint32 val)
{
	assert(val != 0);
#if defined(_MSC_VER)
#if STATIC_BMI2 == 1
	return _tzcnt_u32(val);
#else
	if(val != 0) {
		ulong r;
		_BitScanForward(&r, val);
		return (uint)r;
	}
	else {
		/* Should not reach this code path */
		__assume(0);
	}
#endif
#elif defined(__GNUC__) && (__GNUC__ >= 4)
	return (uint)__builtin_ctz(val);
#else
	return ZSTD_countTrailingZeros32_fallback(val);
#endif
}

MEM_STATIC uint ZSTD_countLeadingZeros32_fallback(uint32 val) 
{
	assert(val != 0);
	{
		static const uint32 DeBruijnClz[32] = {0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31};
		val |= val >> 1;
		val |= val >> 2;
		val |= val >> 4;
		val |= val >> 8;
		val |= val >> 16;
		return 31 - DeBruijnClz[(val * 0x07C4ACDDU) >> 27];
	}
}

MEM_STATIC uint ZSTD_countLeadingZeros32(uint32 val)
{
	assert(val != 0);
#if defined(_MSC_VER)
#if STATIC_BMI2 == 1
	return _lzcnt_u32(val);
#else
	if(val != 0) {
		ulong r;
		_BitScanReverse(&r, val);
		return (uint)(31 - r);
	}
	else {
		/* Should not reach this code path */
		__assume(0);
	}
#endif
#elif defined(__GNUC__) && (__GNUC__ >= 4)
	return (uint)__builtin_clz(val);
#else
	return ZSTD_countLeadingZeros32_fallback(val);
#   endif
}

MEM_STATIC uint ZSTD_countTrailingZeros64(uint64 val)
{
	assert(val != 0);
#if defined(_MSC_VER) && defined(_WIN64)
#if STATIC_BMI2 == 1
	return _tzcnt_u64(val);
#else
	if(val != 0) {
		ulong r;
		_BitScanForward64(&r, val);
		return (uint)r;
	}
	else {
		/* Should not reach this code path */
		__assume(0);
	}
#endif
#elif defined(__GNUC__) && (__GNUC__ >= 4) && defined(__LP64__)
	return (uint)__builtin_ctzll(val);
#else
	{
		uint32 mostSignificantWord = (uint32)(val >> 32);
		uint32 leastSignificantWord = (uint32)val;
		if(leastSignificantWord == 0) {
			return 32 + ZSTD_countTrailingZeros32(mostSignificantWord);
		}
		else {
			return ZSTD_countTrailingZeros32(leastSignificantWord);
		}
	}
#   endif
}

MEM_STATIC uint ZSTD_countLeadingZeros64(uint64 val)
{
	assert(val != 0);
#if defined(_MSC_VER) && defined(_WIN64)
#if STATIC_BMI2 == 1
	return _lzcnt_u64(val);
#else
	if(val != 0) {
		ulong r;
		_BitScanReverse64(&r, val);
		return (uint)(63 - r);
	}
	else {
		/* Should not reach this code path */
		__assume(0);
	}
#endif
#elif defined(__GNUC__) && (__GNUC__ >= 4)
	return (uint)(__builtin_clzll(val));
#else
	{
		uint32 mostSignificantWord = (uint32)(val >> 32);
		uint32 leastSignificantWord = (uint32)val;
		if(mostSignificantWord == 0) {
			return 32 + ZSTD_countLeadingZeros32(leastSignificantWord);
		}
		else {
			return ZSTD_countLeadingZeros32(mostSignificantWord);
		}
	}
#   endif
}

MEM_STATIC uint ZSTD_NbCommonBytes(size_t val)
{
	if(MEM_isLittleEndian()) {
		if(MEM_64bits()) {
			return ZSTD_countTrailingZeros64((uint64)val) >> 3;
		}
		else {
			return ZSTD_countTrailingZeros32((uint32)val) >> 3;
		}
	}
	else { /* Big Endian CPU */
		if(MEM_64bits()) {
			return ZSTD_countLeadingZeros64((uint64)val) >> 3;
		}
		else {
			return ZSTD_countLeadingZeros32((uint32)val) >> 3;
		}
	}
}

MEM_STATIC uint ZSTD_highbit32(uint32 val)   /* compress, dictBuilder, decodeCorpus */
{
	assert(val != 0);
	return 31 - ZSTD_countLeadingZeros32(val);
}

#endif /* ZSTD_BITS_H */
