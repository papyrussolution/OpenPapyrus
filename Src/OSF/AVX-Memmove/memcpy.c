// Compile with GCC -O3 for best performance
// It pretty much entirely negates the need to write these by hand in asm.
#include <slib-internal.h>
#pragma hdrstop
#include <emmintrin.h>
#include "avxmem.h"

// Default (8-bit, 1 byte at a time)
void * memcpy(void * dest, const void * src, size_t len)
{
	const char * s = (char*)src;
	char * d = (char*)dest;
	while(len--) {
		*d++ = *s++;
	}
	return dest;
}

///=============================================================================
/// LICENSING INFORMATION
///=============================================================================
//
// The code above this comment is in the public domain.
// The code below this comment is subject to the custom attribution license found
// here: https://github.com/KNNSpeed/AVX-Memmove/blob/master/LICENSE
//
//==============================================================================
//  AVX Memory Functions: AVX Memcpy
//==============================================================================
//
// Version 1.4
//
// Author:
//  KNNSpeed
//
// Source Code:
//  https://github.com/KNNSpeed/AVX-Memmove
//
// Minimum requirement:
//  x86_64 CPU with SSE4.1, but AVX2 or later is recommended
//
// This file provides a highly optimized version of memcpy.
// Overlapping memory regions are not supported by default: use memmove instead.
//
// NOTE: The discussion about microarchitecture in the memmove file applies to
// this memcpy, as well.
//
// ...If for some reason you absolutely, desperately need to use AVX_memcpy
// instead of AVX_memmove, and you need to use it on overlapping areas, enable
// the below definition. It will check for overlap and automatically redirect to
// AVX_memmove if overlap is found.
// #define OVERLAP_CHECK
//

#ifdef __clang__
#define __m128i_u __m128i
#define __m256i_u __m256i
#define __m512i_u __m512i
#endif

#ifdef __AVX512F__
#define BYTE_ALIGNMENT 0x3F // For 64-byte alignment
#elif __AVX__
#define BYTE_ALIGNMENT 0x1F // For 32-byte alignment
#else
#define BYTE_ALIGNMENT 0x0F // For 16-byte alignment
#endif

//
// USAGE INFORMATION:
//
// The "len" argument is "# of x bytes to copy," e.g. memcpy_512bit_u/a needs
// to know "how many multiples of 512 bit (64 bytes) to copy." The functions
// with byte sizes larger than their bit/8 sizes follow the same pattern:
// memcpy_512bit_512B_u/a needs to know how many multiples of 512 bytes to copy.
//
// The "numbytes" argument in AVX_memcpy and memcpy_large is just the total
// number of bytes to copy.
//

//-----------------------------------------------------------------------------
// Individual Functions:
//-----------------------------------------------------------------------------

// 16-bit (2 bytes at a time)
// Len is (# of total bytes/2), so it's "# of 16-bits"

void * memcpy_16bit(void * dest, const void * src, size_t len)
{
	const uint16_t* s = (uint16_t*)src;
	uint16_t* d = (uint16_t*)dest;

	while(len--) {
		*d++ = *s++;
	}

	return dest;
}

// 32-bit (4 bytes at a time - 1 pixel in a 32-bit linear frame buffer)
// Len is (# of total bytes/4), so it's "# of 32-bits"

void * memcpy_32bit(void * dest, const void * src, size_t len)
{
	const uint32_t* s = (uint32_t*)src;
	uint32_t* d = (uint32_t*)dest;

	while(len--) {
		*d++ = *s++;
	}

	return dest;
}

// 64-bit (8 bytes at a time - 2 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/8), so it's "# of 64-bits"

void * memcpy_64bit(void * dest, const void * src, size_t len)
{
	const uint64_t* s = (uint64_t*)src;
	uint64_t* d = (uint64_t*)dest;

	while(len--) {
		*d++ = *s++;
	}

	return dest;
}

//-----------------------------------------------------------------------------
// SSE2 Unaligned:
//-----------------------------------------------------------------------------

// SSE2 (128-bit, 16 bytes at a time - 4 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/16), so it's "# of 128-bits"

void * memcpy_128bit_u(void * dest, const void * src, size_t len)
{
	const __m128i_u* s = (__m128i_u*)src;
	__m128i_u* d = (__m128i_u*)dest;

	while(len--) {
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++));
	}

	return dest;
}

// 32 bytes at a time
void * memcpy_128bit_32B_u(void * dest, const void * src, size_t len)
{
	const __m128i_u* s = (__m128i_u*)src;
	__m128i_u* d = (__m128i_u*)dest;

	while(len--) {
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 1
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 2
	}

	return dest;
}

// 64 bytes at a time
void * memcpy_128bit_64B_u(void * dest, const void * src, size_t len)
{
	const __m128i_u* s = (__m128i_u*)src;
	__m128i_u* d = (__m128i_u*)dest;

	while(len--) {
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 1
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 2
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 3
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 4
	}

	return dest;
}

// 128 bytes at a time
void * memcpy_128bit_128B_u(void * dest, const void * src, size_t len)
{
	const __m128i_u* s = (__m128i_u*)src;
	__m128i_u* d = (__m128i_u*)dest;

	while(len--) {
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 1
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 2
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 3
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 4
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 5
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 6
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 7
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 8
	}

	return dest;
}

// 256 bytes
void * memcpy_128bit_256B_u(void * dest, const void * src, size_t len)
{
	const __m128i_u* s = (__m128i_u*)src;
	__m128i_u* d = (__m128i_u*)dest;

	while(len--) {
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 1
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 2
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 3
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 4
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 5
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 6
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 7
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 8
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 9
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 10
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 11
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 12
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 13
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 14
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 15
		_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 16
	}

	return dest;
}

//-----------------------------------------------------------------------------
// AVX+ Unaligned:
//-----------------------------------------------------------------------------

// AVX (256-bit, 32 bytes at a time - 8 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/32), so it's "# of 256-bits"
// Sandybridge and Ryzen and up, Haswell and up for better performance

#ifdef __AVX__
void * memcpy_256bit_u(void * dest, const void * src, size_t len)
{
	const __m256i_u* s = (__m256i_u*)src;
	__m256i_u* d = (__m256i_u*)dest;

	while(len--) {
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++));
	}

	return dest;
}

// 64 bytes at a time
void * memcpy_256bit_64B_u(void * dest, const void * src, size_t len)
{
	const __m256i_u* s = (__m256i_u*)src;
	__m256i_u* d = (__m256i_u*)dest;

	while(len--) {
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 1
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 2
	}

	return dest;
}

// 128 bytes at a time
void * memcpy_256bit_128B_u(void * dest, const void * src, size_t len)
{
	const __m256i_u* s = (__m256i_u*)src;
	__m256i_u* d = (__m256i_u*)dest;

	while(len--) {
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 1
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 2
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 3
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 4
	}

	return dest;
}

// 256 bytes at a time
void * memcpy_256bit_256B_u(void * dest, const void * src, size_t len)
{
	const __m256i_u* s = (__m256i_u*)src;
	__m256i_u* d = (__m256i_u*)dest;

	while(len--) {
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 1
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 2
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 3
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 4
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 5
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 6
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 7
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 8
	}

	return dest;
}

// 512 bytes at a time, one load->store for every ymm register (there are 16)
void * memcpy_256bit_512B_u(void * dest, const void * src, size_t len)
{
	const __m256i_u* s = (__m256i_u*)src;
	__m256i_u* d = (__m256i_u*)dest;

	while(len--) {
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 1
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 2
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 3
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 4
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 5
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 6
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 7
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 8
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 9
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 10
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 11
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 12
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 13
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 14
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 15
		_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 16
	}

	return dest;
}

#endif

// AVX-512 (512-bit, 64 bytes at a time - 16 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/64), so it's "# of 512-bits"
// Requires AVX512F

#ifdef __AVX512F__
void * memcpy_512bit_u(void * dest, const void * src, size_t len)
{
	const __m512i_u* s = (__m512i_u*)src;
	__m512i_u* d = (__m512i_u*)dest;

	while(len--) {
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++));
	}

	return dest;
}

// 128 bytes at a time
void * memcpy_512bit_128B_u(void * dest, const void * src, size_t len)
{
	const __m512i_u* s = (__m512i_u*)src;
	__m512i_u* d = (__m512i_u*)dest;

	while(len--) {
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 1
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 2
	}

	return dest;
}

// 256 bytes at a time
void * memcpy_512bit_256B_u(void * dest, const void * src, size_t len)
{
	const __m512i_u* s = (__m512i_u*)src;
	__m512i_u* d = (__m512i_u*)dest;

	while(len--) {
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 1
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 2
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 3
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 4
	}

	return dest;
}

// 512 bytes (half a KB!!) at a time
void * memcpy_512bit_512B_u(void * dest, const void * src, size_t len)
{
	const __m512i_u* s = (__m512i_u*)src;
	__m512i_u* d = (__m512i_u*)dest;

	while(len--) {
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 1
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 2
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 3
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 4
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 5
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 6
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 7
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 8
	}

	return dest;
}

// 1024 bytes, or 1 kB
void * memcpy_512bit_1kB_u(void * dest, const void * src, size_t len)
{
	const __m512i_u* s = (__m512i_u*)src;
	__m512i_u* d = (__m512i_u*)dest;

	while(len--) {
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 1
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 2
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 3
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 4
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 5
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 6
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 7
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 8
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 9
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 10
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 11
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 12
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 13
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 14
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 15
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 16
	}

	return dest;
}

// 2048 bytes, or 2 kB
void * memcpy_512bit_2kB_u(void * dest, const void * src, size_t len)
{
	const __m512i_u* s = (__m512i_u*)src;
	__m512i_u* d = (__m512i_u*)dest;

	while(len--) {
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 1
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 2
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 3
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 4
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 5
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 6
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 7
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 8
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 9
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 10
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 11
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 12
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 13
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 14
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 15
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 16
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 17
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 18
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 19
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 20
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 21
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 22
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 23
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 24
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 25
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 26
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 27
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 28
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 29
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 30
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 31
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 32
	}

	return dest;
}

// 4096 bytes, or 4 kB
void * memcpy_512bit_4kB_u(void * dest, const void * src, size_t len)
{
	const __m512i_u* s = (__m512i_u*)src;
	__m512i_u* d = (__m512i_u*)dest;

	while(len--) {
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 1
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 2
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 3
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 4
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 5
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 6
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 7
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 8
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 9
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 10
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 11
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 12
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 13
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 14
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 15
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 16
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 17
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 18
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 19
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 20
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 21
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 22
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 23
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 24
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 25
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 26
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 27
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 28
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 29
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 30
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 31
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 32
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 1
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 2
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 3
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 4
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 5
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 6
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 7
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 8
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 9
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 10
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 11
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 12
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 13
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 14
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 15
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 16
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 17
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 18
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 19
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 20
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 21
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 22
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 23
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 24
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 25
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 26
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 27
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 28
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 29
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 30
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 31
		_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 32
	}

	return dest;
}

#endif

// AVX-1024 support pending existence of the standard. It would be able to fit
// an entire 4 kB page in its registers at one time. Imagine that!

//-----------------------------------------------------------------------------
// SSE2 Aligned:
//-----------------------------------------------------------------------------

// SSE2 (128-bit, 16 bytes at a time - 4 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/16), so it's "# of 128-bits"

void * memcpy_128bit_a(void * dest, const void * src, size_t len)
{
	const __m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	while(len--) {
		_mm_store_si128(d++, _mm_load_si128(s++));
	}

	return dest;
}

// 32 bytes at a time
void * memcpy_128bit_32B_a(void * dest, const void * src, size_t len)
{
	const __m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	while(len--) {
		_mm_store_si128(d++, _mm_load_si128(s++)); // 1
		_mm_store_si128(d++, _mm_load_si128(s++)); // 2
	}

	return dest;
}

// 64 bytes at a time
void * memcpy_128bit_64B_a(void * dest, const void * src, size_t len)
{
	const __m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	while(len--) {
		_mm_store_si128(d++, _mm_load_si128(s++)); // 1
		_mm_store_si128(d++, _mm_load_si128(s++)); // 2
		_mm_store_si128(d++, _mm_load_si128(s++)); // 3
		_mm_store_si128(d++, _mm_load_si128(s++)); // 4
	}

	return dest;
}

// 128 bytes at a time
void * memcpy_128bit_128B_a(void * dest, const void * src, size_t len)
{
	const __m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	while(len--) {
		_mm_store_si128(d++, _mm_load_si128(s++)); // 1
		_mm_store_si128(d++, _mm_load_si128(s++)); // 2
		_mm_store_si128(d++, _mm_load_si128(s++)); // 3
		_mm_store_si128(d++, _mm_load_si128(s++)); // 4
		_mm_store_si128(d++, _mm_load_si128(s++)); // 5
		_mm_store_si128(d++, _mm_load_si128(s++)); // 6
		_mm_store_si128(d++, _mm_load_si128(s++)); // 7
		_mm_store_si128(d++, _mm_load_si128(s++)); // 8
	}

	return dest;
}

// 256 bytes
void * memcpy_128bit_256B_a(void * dest, const void * src, size_t len)
{
	const __m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	while(len--) {
		_mm_store_si128(d++, _mm_load_si128(s++)); // 1
		_mm_store_si128(d++, _mm_load_si128(s++)); // 2
		_mm_store_si128(d++, _mm_load_si128(s++)); // 3
		_mm_store_si128(d++, _mm_load_si128(s++)); // 4
		_mm_store_si128(d++, _mm_load_si128(s++)); // 5
		_mm_store_si128(d++, _mm_load_si128(s++)); // 6
		_mm_store_si128(d++, _mm_load_si128(s++)); // 7
		_mm_store_si128(d++, _mm_load_si128(s++)); // 8
		_mm_store_si128(d++, _mm_load_si128(s++)); // 9
		_mm_store_si128(d++, _mm_load_si128(s++)); // 10
		_mm_store_si128(d++, _mm_load_si128(s++)); // 11
		_mm_store_si128(d++, _mm_load_si128(s++)); // 12
		_mm_store_si128(d++, _mm_load_si128(s++)); // 13
		_mm_store_si128(d++, _mm_load_si128(s++)); // 14
		_mm_store_si128(d++, _mm_load_si128(s++)); // 15
		_mm_store_si128(d++, _mm_load_si128(s++)); // 16
	}

	return dest;
}

//-----------------------------------------------------------------------------
// AVX+ Aligned:
//-----------------------------------------------------------------------------

// AVX (256-bit, 32 bytes at a time - 8 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/32), so it's "# of 256-bits"
// Sandybridge and Ryzen and up

#ifdef __AVX__
void * memcpy_256bit_a(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	while(len--) {
		_mm256_store_si256(d++, _mm256_load_si256(s++));
	}

	return dest;
}

// 64 bytes at a time
void * memcpy_256bit_64B_a(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	while(len--) {
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 1
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 2
	}

	return dest;
}

// 128 bytes at a time
void * memcpy_256bit_128B_a(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	while(len--) {
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 1
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 2
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 3
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 4
	}

	return dest;
}

// 256 bytes at a time
void * memcpy_256bit_256B_a(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	while(len--) {
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 1
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 2
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 3
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 4
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 5
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 6
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 7
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 8
	}

	return dest;
}

// 512 bytes
void * memcpy_256bit_512B_a(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	while(len--) {
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 1
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 2
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 3
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 4
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 5
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 6
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 7
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 8
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 9
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 10
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 11
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 12
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 13
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 14
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 15
		_mm256_store_si256(d++, _mm256_load_si256(s++)); // 16
	}

	return dest;
}

#endif

// AVX-512 (512-bit, 64 bytes at a time - 16 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/64), so it's "# of 512-bits"
// Requires AVX512F

#ifdef __AVX512F__
void * memcpy_512bit_a(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	while(len--) {
		_mm512_store_si512(d++, _mm512_load_si512(s++));
	}

	return dest;
}

// 128 bytes at a time
void * memcpy_512bit_128B_a(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	while(len--) {
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 1
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 2
	}

	return dest;
}

// 256 bytes at a time
void * memcpy_512bit_256B_a(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	while(len--) {
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 1
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 2
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 3
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 4
	}

	return dest;
}

// 512 bytes (half a KB!!) at a time
void * memcpy_512bit_512B_a(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	while(len--) {
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 1
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 2
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 3
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 4
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 5
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 6
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 7
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 8
	}

	return dest;
}

// 1024 bytes, or 1 kB
void * memcpy_512bit_1kB_a(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	while(len--) {
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 1
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 2
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 3
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 4
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 5
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 6
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 7
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 8
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 9
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 10
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 11
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 12
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 13
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 14
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 15
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 16
	}

	return dest;
}

// 2048 bytes, or 2 kB
void * memcpy_512bit_2kB_a(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	while(len--) {
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 1
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 2
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 3
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 4
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 5
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 6
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 7
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 8
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 9
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 10
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 11
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 12
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 13
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 14
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 15
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 16
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 17
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 18
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 19
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 20
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 21
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 22
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 23
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 24
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 25
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 26
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 27
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 28
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 29
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 30
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 31
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 32
	}

	return dest;
}

// 4096 bytes, or 4 kB
void * memcpy_512bit_4kB_a(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	while(len--) {
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 1
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 2
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 3
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 4
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 5
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 6
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 7
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 8
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 9
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 10
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 11
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 12
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 13
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 14
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 15
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 16
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 17
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 18
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 19
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 20
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 21
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 22
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 23
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 24
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 25
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 26
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 27
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 28
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 29
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 30
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 31
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 32
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 1
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 2
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 3
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 4
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 5
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 6
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 7
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 8
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 9
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 10
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 11
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 12
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 13
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 14
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 15
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 16
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 17
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 18
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 19
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 20
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 21
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 22
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 23
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 24
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 25
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 26
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 27
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 28
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 29
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 30
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 31
		_mm512_store_si512(d++, _mm512_load_si512(s++)); // 32
	}

	return dest;
}

#endif

//-----------------------------------------------------------------------------
// SSE4.1 Streaming:
//-----------------------------------------------------------------------------

// SSE4.1 (128-bit, 16 bytes at a time - 4 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/16), so it's "# of 128-bits"

void * memcpy_128bit_as(void * dest, const void * src, size_t len)
{
	__m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	while(len--) {
		_mm_stream_si128(d++, _mm_stream_load_si128(s++));
	}
	_mm_sfence();

	return dest;
}

// 32 bytes at a time
void * memcpy_128bit_32B_as(void * dest, const void * src, size_t len)
{
	__m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	while(len--) {
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 1
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 2
	}
	_mm_sfence();

	return dest;
}

// 64 bytes at a time
void * memcpy_128bit_64B_as(void * dest, const void * src, size_t len)
{
	__m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	while(len--) {
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 1
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 2
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 3
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 4
	}
	_mm_sfence();

	return dest;
}

// 128 bytes at a time
void * memcpy_128bit_128B_as(void * dest, const void * src, size_t len)
{
	__m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	while(len--) {
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 1
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 2
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 3
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 4
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 5
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 6
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 7
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 8
	}
	_mm_sfence();

	return dest;
}

// 256 bytes
void * memcpy_128bit_256B_as(void * dest, const void * src, size_t len)
{
	__m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	while(len--) {
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 1
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 2
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 3
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 4
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 5
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 6
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 7
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 8
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 9
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 10
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 11
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 12
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 13
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 14
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 15
		_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 16
	}
	_mm_sfence();

	return dest;
}

//-----------------------------------------------------------------------------
// AVX2+ Streaming:
//-----------------------------------------------------------------------------

// AVX2 (256-bit, 32 bytes at a time - 8 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/32), so it's "# of 256-bits"
// Haswell and Ryzen and up

#ifdef __AVX2__
void * memcpy_256bit_as(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	while(len--) {
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++));
	}
	_mm_sfence();

	return dest;
}

// 64 bytes at a time
void * memcpy_256bit_64B_as(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	while(len--) {
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 1
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 2
	}
	_mm_sfence();

	return dest;
}

// 128 bytes at a time
void * memcpy_256bit_128B_as(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	while(len--) {
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 1
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 2
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 3
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 4
	}
	_mm_sfence();

	return dest;
}

// 256 bytes at a time
void * memcpy_256bit_256B_as(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	while(len--) {
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 1
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 2
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 3
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 4
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 5
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 6
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 7
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 8
	}
	_mm_sfence();

	return dest;
}

// 512 bytes
void * memcpy_256bit_512B_as(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	while(len--) {
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 1
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 2
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 3
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 4
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 5
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 6
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 7
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 8
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 9
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 10
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 11
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 12
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 13
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 14
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 15
		_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 16
	}
	_mm_sfence();

	return dest;
}

#endif

// AVX-512 (512-bit, 64 bytes at a time - 16 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/64), so it's "# of 512-bits"
// Requires AVX512F

#ifdef __AVX512F__
void * memcpy_512bit_as(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	while(len--) {
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++));
	}
	_mm_sfence();

	return dest;
}

// 128 bytes at a time
void * memcpy_512bit_128B_as(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	while(len--) {
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 1
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 2
	}
	_mm_sfence();

	return dest;
}

// 256 bytes at a time
void * memcpy_512bit_256B_as(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	while(len--) {
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 1
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 2
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 3
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 4
	}
	_mm_sfence();

	return dest;
}

// 512 bytes (half a KB!!) at a time
void * memcpy_512bit_512B_as(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	while(len--) {
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 1
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 2
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 3
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 4
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 5
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 6
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 7
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 8
	}
	_mm_sfence();

	return dest;
}

// 1024 bytes, or 1 kB
void * memcpy_512bit_1kB_as(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	while(len--) {
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 1
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 2
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 3
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 4
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 5
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 6
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 7
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 8
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 9
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 10
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 11
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 12
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 13
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 14
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 15
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 16
	}
	_mm_sfence();

	return dest;
}

// 2048 bytes, or 2 kB
void * memcpy_512bit_2kB_as(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	while(len--) {
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 1
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 2
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 3
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 4
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 5
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 6
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 7
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 8
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 9
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 10
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 11
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 12
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 13
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 14
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 15
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 16
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 17
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 18
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 19
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 20
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 21
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 22
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 23
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 24
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 25
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 26
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 27
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 28
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 29
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 30
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 31
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 32
	}
	_mm_sfence();

	return dest;
}

// 4096 bytes, or 4 kB
void * memcpy_512bit_4kB_as(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	while(len--) {
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 1
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 2
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 3
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 4
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 5
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 6
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 7
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 8
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 9
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 10
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 11
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 12
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 13
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 14
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 15
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 16
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 17
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 18
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 19
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 20
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 21
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 22
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 23
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 24
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 25
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 26
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 27
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 28
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 29
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 30
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 31
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 32
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 1
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 2
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 3
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 4
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 5
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 6
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 7
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 8
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 9
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 10
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 11
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 12
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 13
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 14
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 15
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 16
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 17
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 18
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 19
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 20
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 21
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 22
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 23
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 24
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 25
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 26
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 27
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 28
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 29
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 30
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 31
		_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 32
	}
	_mm_sfence();

	return dest;
}

#endif

//-----------------------------------------------------------------------------
// Dispatch Functions:
//-----------------------------------------------------------------------------

// Copy arbitrarily large amounts of data between 2 non-overlapping regions
void * memcpy_large(void * dest, void * src, size_t numbytes)
{
	void * returnval = dest; // memcpy is supposed to return the destination
	size_t offset = 0; // Offset size needs to match the size of a pointer

	while(numbytes) {
		// The biggest sizes will go first for alignment. There's no benefit to using
		// aligned loads over unaligned loads here, so all are unaligned.
		// NOTE: Each memcpy has its own loop so that any one can be used individually.
		if(numbytes < 2) { // 1 byte
			memcpy(dest, src, numbytes);
			offset = numbytes & -1;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes = 0;
		}
		else if(numbytes < 4) { // 2 bytes
			memcpy_16bit(dest, src, numbytes >> 1);
			offset = numbytes & -2;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 1;
		}
		else if(numbytes < 8) { // 4 bytes
			memcpy_32bit(dest, src, numbytes >> 2);
			offset = numbytes & -4;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 3;
		}
		else if(numbytes < 16) { // 8 bytes
			memcpy_64bit(dest, src, numbytes >> 3);
			offset = numbytes & -8;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 7;
		}
#ifdef __AVX512F__
		else if(numbytes < 32) { // 16 bytes
			memcpy_128bit_u(dest, src, numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memcpy_256bit_u(dest, src, numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memcpy_512bit_u(dest, src, numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memcpy_512bit_128B_u(dest, src, numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memcpy_512bit_256B_u(dest, src, numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 255;
		}
		else if(numbytes < 1024) { // 512 bytes
			memcpy_512bit_512B_u(dest, src, numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 511;
		}
		else if(numbytes < 2048) { // 1024 bytes (1 kB)
			memcpy_512bit_1kB_u(dest, src, numbytes >> 10);
			offset = numbytes & -1024;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 1023;
		}
		else if(numbytes < 4096) { // 2048 bytes (2 kB)
			memcpy_512bit_2kB_u(dest, src, numbytes >> 11);
			offset = numbytes & -2048;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 2047;
		}
		else{ // 4096 bytes (4 kB)
			memcpy_512bit_4kB_u(dest, src, numbytes >> 12);
			offset = numbytes & -4096;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 4095;
		}
#elif __AVX__
		else if(numbytes < 32) { // 16 bytes
			memcpy_128bit_u(dest, src, numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memcpy_256bit_u(dest, src, numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memcpy_256bit_64B_u(dest, src, numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memcpy_256bit_128B_u(dest, src, numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memcpy_256bit_256B_u(dest, src, numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 255;
		}
		else{ // 512 bytes
			memcpy_256bit_512B_u(dest, src, numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 511;
		}
#else // SSE2 only
		else if(numbytes < 32) { // 16 bytes
			memcpy_128bit_u(dest, src, numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memcpy_128bit_32B_u(dest, src, numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memcpy_128bit_64B_u(dest, src, numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memcpy_128bit_128B_u(dest, src, numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 127;
		}
		else{ // 256 bytes
			memcpy_128bit_256B_u(dest, src, numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 255;
		}
#endif
	}

	return returnval;
} // END MEMCPY LARGE, UNALIGNED

// Copy arbitrarily large amounts of data between 2 non-overlapping regions
// Aligned version
void * memcpy_large_a(void * dest, void * src, size_t numbytes)
{
	void * returnval = dest; // memcpy is supposed to return the destination
	size_t offset = 0; // Offset size needs to match the size of a pointer

	while(numbytes) {
		// The biggest sizes will go first for alignment. There's no benefit to using
		// aligned loads over unaligned loads here, so all are unaligned.
		// NOTE: Each memcpy has its own loop so that any one can be used individually.
		if(numbytes < 2) { // 1 byte
			memcpy(dest, src, numbytes);
			offset = numbytes & -1;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes = 0;
		}
		else if(numbytes < 4) { // 2 bytes
			memcpy_16bit(dest, src, numbytes >> 1);
			offset = numbytes & -2;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 1;
		}
		else if(numbytes < 8) { // 4 bytes
			memcpy_32bit(dest, src, numbytes >> 2);
			offset = numbytes & -4;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 3;
		}
		else if(numbytes < 16) { // 8 bytes
			memcpy_64bit(dest, src, numbytes >> 3);
			offset = numbytes & -8;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 7;
		}
#ifdef __AVX512F__
		else if(numbytes < 32) { // 16 bytes
			memcpy_128bit_a(dest, src, numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memcpy_256bit_a(dest, src, numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memcpy_512bit_a(dest, src, numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memcpy_512bit_128B_a(dest, src, numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memcpy_512bit_256B_a(dest, src, numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 255;
		}
		else if(numbytes < 1024) { // 512 bytes
			memcpy_512bit_512B_a(dest, src, numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 511;
		}
		else if(numbytes < 2048) { // 1024 bytes (1 kB)
			memcpy_512bit_1kB_a(dest, src, numbytes >> 10);
			offset = numbytes & -1024;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 1023;
		}
		else if(numbytes < 4096) { // 2048 bytes (2 kB)
			memcpy_512bit_2kB_a(dest, src, numbytes >> 11);
			offset = numbytes & -2048;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 2047;
		}
		else{ // 4096 bytes (4 kB)
			memcpy_512bit_4kB_a(dest, src, numbytes >> 12);
			offset = numbytes & -4096;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 4095;
		}
#elif __AVX__
		else if(numbytes < 32) { // 16 bytes
			memcpy_128bit_a(dest, src, numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memcpy_256bit_a(dest, src, numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memcpy_256bit_64B_a(dest, src, numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memcpy_256bit_128B_a(dest, src, numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memcpy_256bit_256B_a(dest, src, numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 255;
		}
		else{ // 512 bytes
			memcpy_256bit_512B_a(dest, src, numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 511;
		}
#else // SSE2 only
		else if(numbytes < 32) { // 16 bytes
			memcpy_128bit_a(dest, src, numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memcpy_128bit_32B_a(dest, src, numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memcpy_128bit_64B_a(dest, src, numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memcpy_128bit_128B_a(dest, src, numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 127;
		}
		else{ // 256 bytes
			memcpy_128bit_256B_a(dest, src, numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 255;
		}
#endif
	}

	return returnval;
} // END MEMCPY LARGE, ALIGNED

// Copy arbitrarily large amounts of data between 2 non-overlapping regions
// Aligned, streaming version
void * memcpy_large_as(void * dest, void * src, size_t numbytes)
{
	void * returnval = dest; // memcpy is supposed to return the destination
	size_t offset = 0; // Offset size needs to match the size of a pointer

	while(numbytes) {
		// The biggest sizes will go first for alignment. There's no benefit to using
		// aligned loads over unaligned loads here, so all are unaligned.
		// NOTE: Each memcpy has its own loop so that any one can be used individually.
		if(numbytes < 2) { // 1 byte
			memcpy(dest, src, numbytes);
			offset = numbytes & -1;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes = 0;
		}
		else if(numbytes < 4) { // 2 bytes
			memcpy_16bit(dest, src, numbytes >> 1);
			offset = numbytes & -2;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 1;
		}
		else if(numbytes < 8) { // 4 bytes
			memcpy_32bit(dest, src, numbytes >> 2);
			offset = numbytes & -4;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 3;
		}
		else if(numbytes < 16) { // 8 bytes
			memcpy_64bit(dest, src, numbytes >> 3);
			offset = numbytes & -8;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 7;
		}
#ifdef __AVX512F__
		else if(numbytes < 32) { // 16 bytes
			memcpy_128bit_as(dest, src, numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memcpy_256bit_as(dest, src, numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memcpy_512bit_as(dest, src, numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memcpy_512bit_128B_as(dest, src, numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memcpy_512bit_256B_as(dest, src, numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 255;
		}
		else if(numbytes < 1024) { // 512 bytes
			memcpy_512bit_512B_as(dest, src, numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 511;
		}
		else if(numbytes < 2048) { // 1024 bytes (1 kB)
			memcpy_512bit_1kB_as(dest, src, numbytes >> 10);
			offset = numbytes & -1024;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 1023;
		}
		else if(numbytes < 4096) { // 2048 bytes (2 kB)
			memcpy_512bit_2kB_as(dest, src, numbytes >> 11);
			offset = numbytes & -2048;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 2047;
		}
		else{ // 4096 bytes (4 kB)
			memcpy_512bit_4kB_as(dest, src, numbytes >> 12);
			offset = numbytes & -4096;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 4095;
		}
#elif __AVX2__
		else if(numbytes < 32) { // 16 bytes
			memcpy_128bit_as(dest, src, numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memcpy_256bit_as(dest, src, numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memcpy_256bit_64B_as(dest, src, numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memcpy_256bit_128B_as(dest, src, numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memcpy_256bit_256B_as(dest, src, numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 255;
		}
		else{ // 512 bytes
			memcpy_256bit_512B_as(dest, src, numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 511;
		}
#else // SSE4.1 only
		else if(numbytes < 32) { // 16 bytes
			memcpy_128bit_as(dest, src, numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memcpy_128bit_32B_as(dest, src, numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memcpy_128bit_64B_as(dest, src, numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memcpy_128bit_128B_as(dest, src, numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 127;
		}
		else{ // 256 bytes
			memcpy_128bit_256B_as(dest, src, numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 255;
		}
#endif
	}

	return returnval;
} // END MEMCPY LARGE, ALIGNED, STREAMING

//-----------------------------------------------------------------------------
// Main Function:
//-----------------------------------------------------------------------------

// General-purpose function to call
void * AVX_memcpy(void * dest, void * src, size_t numbytes)
{
	void * returnval = dest;

	if((char*)src == (char*)dest) {
		// Lol.
		return returnval;
	}

#ifdef OVERLAP_CHECK
	// Overlap check
	if(
		(
			(
				(char*)dest > (char*)src
			)
			&&
			(
				(char*)dest < ((char*)src + numbytes)
			)
		)
		||
		(
			(
				(char*)src > (char*)dest
			)
			&&
			(
				(char*)src < ((char*)dest + numbytes)
			)
		)
		) { // Why didn't you just use memmove directly???
		returnval = AVX_memmove(dest, src, numbytes);
		return returnval;
	}
#endif

	if(
		( ((uintptr_t)src & BYTE_ALIGNMENT) == 0 )
		&&
		( ((uintptr_t)dest & BYTE_ALIGNMENT) == 0 )
		) { // Check alignment
		// This is the fastest case: src and dest are both cache line aligned.
		if(numbytes > CACHESIZELIMIT) {
			memcpy_large_as(dest, src, numbytes);
		}
		else{
			memcpy_large_a(dest, src, numbytes); // Even if numbytes is small this'll work
		}
	}
	else{ // Unaligned
		size_t numbytes_to_align = (BYTE_ALIGNMENT + 1) - ((uintptr_t)dest & BYTE_ALIGNMENT);

		if(numbytes > numbytes_to_align) {
			void * destoffset = (char*)dest + numbytes_to_align;
			void * srcoffset = (char*)src + numbytes_to_align;

			// Get to an aligned position.
			// This may be a little slower, but since it'll be mostly scalar operations
			// alignment doesn't matter. Worst case it uses two vector functions, and
			// this process only needs to be done once per call if dest is unaligned.
			memcpy_large(dest, src, numbytes_to_align);
			// Now this should be faster since stores are aligned.
			memcpy_large(destoffset, srcoffset, numbytes - numbytes_to_align); // Can't use streaming due to potential src misalignment
			// On Haswell and up, cross cache line loads have a negligible penalty.
			// Thus this will be slower on Sandy & Ivy Bridge, though Ivy Bridge will
			// fare a little better (~2x, maybe?). Ryzen should generally fall somewhere
			// inbetween Sandy Bridge and Haswell/Skylake on that front.
			// NOTE: These are just rough theoretical estimates.
		}
		else{ // Small size
			memcpy_large(dest, src, numbytes);
		}
	}
	return returnval;
}

// AVX-1024+ support pending existence of the standard.
