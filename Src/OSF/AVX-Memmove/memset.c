// Compile with GCC -O3 for best performance
// It pretty much entirely negates the need to write these by hand in asm.
#include <slib-internal.h>
#pragma hdrstop

#include <emmintrin.h>
#include "avxmem.h"

#define DO2(op)  op; op;
#define DO4(op)  DO2(op); DO2(op);
#define DO8(op)  DO4(op); DO4(op);
#define DO16(op) DO8(op); DO8(op);

void * memset(void * dest, const uint8_t val, size_t len)
{
	uint8_t * ptr = (uint8_t*)dest;
	while(len--) {
		*ptr++ = val;
	}
	return dest;
}
//
// LICENSING INFORMATION
//
// The code above this comment is in the public domain.
// The code below this comment is subject to the custom attribution license found
// here: https://github.com/KNNSpeed/AVX-Memmove/blob/master/LICENSE
//
//  AVX Memory Functions: AVX Memset
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
//  x86_64 CPU with SSE2, but AVX2 or later is recommended
//
// This file provides a highly optimized version of memset.
//
// There is also an AVX_memset_4B for mass-setting of 4-byte data types, for example
// framebuffers with 32 bits per pixel could use this to set a contiguous portion
// of the buffer as one color.
//
// If you just want to zero a big array, use plain AVX_memset since it
// implments a dedicated zeroing function (AVX_memset_4B does not).
//
#if defined(__clang__)
	#define __m128i __m128i
	#define __m256i_u __m256i
	#define __m512i_u __m512i
#else if defined(_MSC_VER)
	#define __m128i __m128i
#endif
#ifdef __AVX512F__
	#define BYTE_ALIGNMENT 0x3F // For 64-byte alignment
#elif __AVX__
	#define BYTE_ALIGNMENT 0x1F // For 32-byte alignment
#else
	#define BYTE_ALIGNMENT 0x0F // For 16-byte alignment
#endif
//
// Individual Functions:
//
// 16-bit (2 bytes at a time)
// Len is (# of total bytes/2), so it's "# of 16-bits"
//
static void * memset_16bit(void * dest, const uint16 val, size_t len)
{
	uint16_t * ptr = (uint16 *)dest;
	while(len--) {
		*ptr++ = val;
	}
	return dest;
}
//
// 32-bit (4 bytes at a time - 1 pixel in a 32-bit linear frame buffer)
// Len is (# of total bytes/4), so it's "# of 32-bits"
//
static void * memset_32bit(void * dest, const uint32_t val, size_t len)
{
	uint32_t * ptr = (uint32_t *)dest;
	while(len--) {
		*ptr++ = val;
	}
	return dest;
}
//
// 64-bit (8 bytes at a time - 2 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/8), so it's "# of 64-bits"
//
static void * memset_64bit(void * dest, const uint64_t val, size_t len)
{
	uint64_t * ptr = (uint64_t*)dest;
	while(len--) {
		*ptr++ = val;
	}
	return dest;
}
//
// SSE2 Unaligned:
//
// SSE2 (128-bit, 16 bytes at a time - 4 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/16), so it's "# of 128-bits"
//
static void * memset_128bit_u(void * dest, const __m128i val, size_t len)
{
	__m128i * ptr = (__m128i*)dest;
	while(len--) {
		_mm_storeu_si128(ptr++, val);
	}
	return dest;
}
//
// 32 bytes
//
static void * memset_128bit_32B_u(void * dest, const __m128i val, size_t len)
{
	__m128i * ptr = (__m128i*)dest;
	while(len--) {
		DO2(_mm_storeu_si128(ptr++, val));
	}
	return dest;
}
//
// 64 bytes
//
static void * memset_128bit_64B_u(void * dest, const __m128i val, size_t len)
{
	__m128i * ptr = (__m128i*)dest;
	while(len--) {
		DO4(_mm_storeu_si128(ptr++, val));
	}
	return dest;
}
//
// 128 bytes
//
static void * memset_128bit_128B_u(void * dest, const __m128i val, size_t len)
{
	__m128i * ptr = (__m128i*)dest;
	while(len--) {
		DO8(_mm_storeu_si128(ptr++, val));
	}
	return dest;
}
//
// 256 bytes
//
static void * memset_128bit_256B_u(void * dest, const __m128i val, size_t len)
{
	__m128i * ptr = (__m128i*)dest;
	while(len--) {
		DO16(_mm_storeu_si128(ptr++, val));
	}
	return dest;
}
//
// AVX+ Unaligned:
//
// AVX (256-bit, 32 bytes at a time - 8 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/32), so it's "# of 256-bits"
// Sandybridge and Ryzen and up, Haswell and up for better performance
//
#ifdef __AVX__

static void * memset_256bit_u(void * dest, const __m256i_u val, size_t len)
{
	__m256i_u * ptr = (__m256i_u*)dest;
	while(len--) {
		_mm256_storeu_si256(ptr++, val);
	}
	return dest;
}

// 64 bytes
static void * memset_256bit_64B_u(void * dest, const __m256i_u val, size_t len)
{
	__m256i_u * ptr = (__m256i_u*)dest;
	while(len--) {
		DO2(_mm256_storeu_si256(ptr++, val));
	}
	return dest;
}

// 128 bytes
static void * memset_256bit_128B_u(void * dest, const __m256i_u val, size_t len)
{
	__m256i_u * ptr = (__m256i_u*)dest;
	while(len--) {
		DO4(_mm256_storeu_si256(ptr++, val));
	}
	return dest;
}

// 256 bytes
static void * memset_256bit_256B_u(void * dest, const __m256i_u val, size_t len)
{
	__m256i_u * ptr = (__m256i_u*)dest;
	while(len--) {
		DO8(_mm256_storeu_si256(ptr++, val));
	}
	return dest;
}

// 512 bytes
static void * memset_256bit_512B_u(void * dest, const __m256i_u val, size_t len)
{
	__m256i_u * ptr = (__m256i_u*)dest;
	while(len--) {
		DO16(_mm256_storeu_si256(ptr++, val));
	}
	return dest;
}

#endif
//
// AVX-512 (512-bit, 64 bytes at a time - 16 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/64), so it's "# of 512-bits"
// Requires AVX512F
//
#ifdef __AVX512F__

static void * memset_512bit_u(void * dest, const __m512i_u val, size_t len)
{
	__m512i_u * ptr = (__m512i_u*)dest;
	while(len--) {
		_mm512_storeu_si512(ptr++, val);
	}
	return dest;
}
//
// 128 bytes
//
static void * memset_512bit_128B_u(void * dest, const __m512i_u val, size_t len)
{
	__m512i_u * ptr = (__m512i_u*)dest;
	while(len--) {
		DO2(_mm512_storeu_si512(ptr++, val));
	}
	return dest;
}
//
// 256 bytes
//
static void * memset_512bit_256B_u(void * dest, const __m512i_u val, size_t len)
{
	__m512i_u * ptr = (__m512i_u*)dest;
	while(len--) {
		DO4(_mm512_storeu_si512(ptr++, val));
	}
	return dest;
}
//
// 512 bytes
//
static void * memset_512bit_512B_u(void * dest, const __m512i_u val, size_t len)
{
	__m512i_u * ptr = (__m512i_u*)dest;
	while(len--) {
		DO8(_mm512_storeu_si512(ptr++, val));
	}
	return dest;
}

// 1024 bytes, or 1 kB
static void * memset_512bit_1kB_u(void * dest, const __m512i_u val, size_t len)
{
	__m512i_u * ptr = (__m512i_u*)dest;
	while(len--) {
		DO16(_mm512_storeu_si512(ptr++, val));
	}
	return dest;
}
//
// 2048 bytes, or 2 kB
//
static void * memset_512bit_2kB_u(void * dest, const __m512i_u val, size_t len)
{
	__m512i_u * ptr = (__m512i_u*)dest;
	while(len--) {
		DO16(_mm512_storeu_si512(ptr++, val));
		DO16(_mm512_storeu_si512(ptr++, val));
	}
	return dest;
}
//
// 4096 bytes, or 4 kB, also 1 page
//
static void * memset_512bit_4kB_u(void * dest, const __m512i_u val, size_t len)
{
	__m512i_u * ptr = (__m512i_u*)dest;
	while(len--) {
		DO16(_mm512_storeu_si512(ptr++, val));
		DO16(_mm512_storeu_si512(ptr++, val));
		DO16(_mm512_storeu_si512(ptr++, val));
		DO16(_mm512_storeu_si512(ptr++, val));
	}
	return dest;
}
#endif
//
// SSE2 Aligned:
//
// SSE2 (128-bit, 16 bytes at a time - 4 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/16), so it's "# of 128-bits"
//
static void * memset_128bit_a(void * dest, const __m128i val, size_t len)
{
	__m128i * ptr = (__m128i*)dest;
	while(len--) {
		_mm_store_si128(ptr++, val);
	}
	return dest;
}

// 32 bytes
static void * memset_128bit_32B_a(void * dest, const __m128i val, size_t len)
{
	__m128i * ptr = (__m128i*)dest;
	while(len--) {
		_mm_store_si128(ptr++, val); // 1
		_mm_store_si128(ptr++, val); // 2
	}
	return dest;
}
//
// 64 bytes
//
static void * memset_128bit_64B_a(void * dest, const __m128i val, size_t len)
{
	__m128i * ptr = (__m128i*)dest;
	while(len--) {
		_mm_store_si128(ptr++, val); // 1
		_mm_store_si128(ptr++, val); // 2
		_mm_store_si128(ptr++, val); // 3
		_mm_store_si128(ptr++, val); // 4
	}
	return dest;
}
//
// 128 bytes
//
static void * memset_128bit_128B_a(void * dest, const __m128i val, size_t len)
{
	__m128i * ptr = (__m128i*)dest;
	while(len--) {
		_mm_store_si128(ptr++, val); // 1
		_mm_store_si128(ptr++, val); // 2
		_mm_store_si128(ptr++, val); // 3
		_mm_store_si128(ptr++, val); // 4
		_mm_store_si128(ptr++, val); // 5
		_mm_store_si128(ptr++, val); // 6
		_mm_store_si128(ptr++, val); // 7
		_mm_store_si128(ptr++, val); // 8
	}
	return dest;
}

// 256 bytes
static void * memset_128bit_256B_a(void * dest, const __m128i val, size_t len)
{
	__m128i * ptr = (__m128i*)dest;
	while(len--) {
		_mm_store_si128(ptr++, val); // 1
		_mm_store_si128(ptr++, val); // 2
		_mm_store_si128(ptr++, val); // 3
		_mm_store_si128(ptr++, val); // 4
		_mm_store_si128(ptr++, val); // 5
		_mm_store_si128(ptr++, val); // 6
		_mm_store_si128(ptr++, val); // 7
		_mm_store_si128(ptr++, val); // 8
		_mm_store_si128(ptr++, val); // 9
		_mm_store_si128(ptr++, val); // 10
		_mm_store_si128(ptr++, val); // 11
		_mm_store_si128(ptr++, val); // 12
		_mm_store_si128(ptr++, val); // 13
		_mm_store_si128(ptr++, val); // 14
		_mm_store_si128(ptr++, val); // 15
		_mm_store_si128(ptr++, val); // 16
	}
	return dest;
}
//
// AVX+ Aligned:
//
// AVX (256-bit, 32 bytes at a time - 8 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/32), so it's "# of 256-bits"
// Sandybridge and Ryzen and up
//
#ifdef __AVX__

static void * memset_256bit_a(void * dest, const __m256i val, size_t len)
{
	__m256i * ptr = (__m256i*)dest;
	while(len--) {
		_mm256_store_si256(ptr++, val);
	}
	return dest;
}

// 64 bytes
static void * memset_256bit_64B_a(void * dest, const __m256i val, size_t len)
{
	__m256i * ptr = (__m256i*)dest;
	while(len--) {
		_mm256_store_si256(ptr++, val); // 1
		_mm256_store_si256(ptr++, val); // 2
	}
	return dest;
}

// 128 bytes
static void * memset_256bit_128B_a(void * dest, const __m256i val, size_t len)
{
	__m256i * ptr = (__m256i*)dest;
	while(len--) {
		_mm256_store_si256(ptr++, val); // 1
		_mm256_store_si256(ptr++, val); // 2
		_mm256_store_si256(ptr++, val); // 3
		_mm256_store_si256(ptr++, val); // 4
	}
	return dest;
}

// 256 bytes
static void * memset_256bit_256B_a(void * dest, const __m256i val, size_t len)
{
	__m256i * ptr = (__m256i*)dest;
	while(len--) {
		_mm256_store_si256(ptr++, val); // 1
		_mm256_store_si256(ptr++, val); // 2
		_mm256_store_si256(ptr++, val); // 3
		_mm256_store_si256(ptr++, val); // 4
		_mm256_store_si256(ptr++, val); // 5
		_mm256_store_si256(ptr++, val); // 6
		_mm256_store_si256(ptr++, val); // 7
		_mm256_store_si256(ptr++, val); // 8
	}
	return dest;
}

// 512 bytes
static void * memset_256bit_512B_a(void * dest, const __m256i val, size_t len)
{
	__m256i * ptr = (__m256i*)dest;
	while(len--) {
		_mm256_store_si256(ptr++, val); // 1
		_mm256_store_si256(ptr++, val); // 2
		_mm256_store_si256(ptr++, val); // 3
		_mm256_store_si256(ptr++, val); // 4
		_mm256_store_si256(ptr++, val); // 5
		_mm256_store_si256(ptr++, val); // 6
		_mm256_store_si256(ptr++, val); // 7
		_mm256_store_si256(ptr++, val); // 8
		_mm256_store_si256(ptr++, val); // 9
		_mm256_store_si256(ptr++, val); // 10
		_mm256_store_si256(ptr++, val); // 11
		_mm256_store_si256(ptr++, val); // 12
		_mm256_store_si256(ptr++, val); // 13
		_mm256_store_si256(ptr++, val); // 14
		_mm256_store_si256(ptr++, val); // 15
		_mm256_store_si256(ptr++, val); // 16
	}
	return dest;
}
#endif
//
// AVX-512 (512-bit, 64 bytes at a time - 16 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/64), so it's "# of 512-bits"
// Requires AVX512F
//
#ifdef __AVX512F__

static void * memset_512bit_a(void * dest, const __m512i val, size_t len)
{
	__m512i * ptr = (__m512i*)dest;
	while(len--) {
		_mm512_store_si512(ptr++, val);
	}
	return dest;
}

// 128 bytes
static void * memset_512bit_128B_a(void * dest, const __m512i val, size_t len)
{
	__m512i * ptr = (__m512i*)dest;
	while(len--) {
		_mm512_store_si512(ptr++, val); // 1
		_mm512_store_si512(ptr++, val); // 2
	}
	return dest;
}

// 256 bytes
static void * memset_512bit_256B_a(void * dest, const __m512i val, size_t len)
{
	__m512i * ptr = (__m512i*)dest;
	while(len--) {
		_mm512_store_si512(ptr++, val); // 1
		_mm512_store_si512(ptr++, val); // 2
		_mm512_store_si512(ptr++, val); // 3
		_mm512_store_si512(ptr++, val); // 4
	}
	return dest;
}

// 512 bytes
static void * memset_512bit_512B_a(void * dest, const __m512i val, size_t len)
{
	__m512i * ptr = (__m512i*)dest;
	while(len--) {
		_mm512_store_si512(ptr++, val); // 1
		_mm512_store_si512(ptr++, val); // 2
		_mm512_store_si512(ptr++, val); // 3
		_mm512_store_si512(ptr++, val); // 4
		_mm512_store_si512(ptr++, val); // 5
		_mm512_store_si512(ptr++, val); // 6
		_mm512_store_si512(ptr++, val); // 7
		_mm512_store_si512(ptr++, val); // 8
	}
	return dest;
}

// 1024 bytes, or 1 kB
static void * memset_512bit_1kB_a(void * dest, const __m512i val, size_t len)
{
	__m512i * ptr = (__m512i*)dest;
	while(len--) {
		_mm512_store_si512(ptr++, val); // 1
		_mm512_store_si512(ptr++, val); // 2
		_mm512_store_si512(ptr++, val); // 3
		_mm512_store_si512(ptr++, val); // 4
		_mm512_store_si512(ptr++, val); // 5
		_mm512_store_si512(ptr++, val); // 6
		_mm512_store_si512(ptr++, val); // 7
		_mm512_store_si512(ptr++, val); // 8
		_mm512_store_si512(ptr++, val); // 9
		_mm512_store_si512(ptr++, val); // 10
		_mm512_store_si512(ptr++, val); // 11
		_mm512_store_si512(ptr++, val); // 12
		_mm512_store_si512(ptr++, val); // 13
		_mm512_store_si512(ptr++, val); // 14
		_mm512_store_si512(ptr++, val); // 15
		_mm512_store_si512(ptr++, val); // 16
	}
	return dest;
}

// 2048 bytes, or 2 kB
static void * memset_512bit_2kB_a(void * dest, const __m512i val, size_t len)
{
	__m512i * ptr = (__m512i*)dest;
	while(len--) {
		_mm512_store_si512(ptr++, val); // 1
		_mm512_store_si512(ptr++, val); // 2
		_mm512_store_si512(ptr++, val); // 3
		_mm512_store_si512(ptr++, val); // 4
		_mm512_store_si512(ptr++, val); // 5
		_mm512_store_si512(ptr++, val); // 6
		_mm512_store_si512(ptr++, val); // 7
		_mm512_store_si512(ptr++, val); // 8
		_mm512_store_si512(ptr++, val); // 9
		_mm512_store_si512(ptr++, val); // 10
		_mm512_store_si512(ptr++, val); // 11
		_mm512_store_si512(ptr++, val); // 12
		_mm512_store_si512(ptr++, val); // 13
		_mm512_store_si512(ptr++, val); // 14
		_mm512_store_si512(ptr++, val); // 15
		_mm512_store_si512(ptr++, val); // 16
		_mm512_store_si512(ptr++, val); // 17
		_mm512_store_si512(ptr++, val); // 18
		_mm512_store_si512(ptr++, val); // 19
		_mm512_store_si512(ptr++, val); // 20
		_mm512_store_si512(ptr++, val); // 21
		_mm512_store_si512(ptr++, val); // 22
		_mm512_store_si512(ptr++, val); // 23
		_mm512_store_si512(ptr++, val); // 24
		_mm512_store_si512(ptr++, val); // 25
		_mm512_store_si512(ptr++, val); // 26
		_mm512_store_si512(ptr++, val); // 27
		_mm512_store_si512(ptr++, val); // 28
		_mm512_store_si512(ptr++, val); // 29
		_mm512_store_si512(ptr++, val); // 30
		_mm512_store_si512(ptr++, val); // 31
		_mm512_store_si512(ptr++, val); // 32
	}
	return dest;
}

// 4096 bytes, or 4 kB, also 1 page
static void * memset_512bit_4kB_a(void * dest, const __m512i val, size_t len)
{
	__m512i * ptr = (__m512i*)dest;
	while(len--) {
		_mm512_store_si512(ptr++, val); // 1
		_mm512_store_si512(ptr++, val); // 2
		_mm512_store_si512(ptr++, val); // 3
		_mm512_store_si512(ptr++, val); // 4
		_mm512_store_si512(ptr++, val); // 5
		_mm512_store_si512(ptr++, val); // 6
		_mm512_store_si512(ptr++, val); // 7
		_mm512_store_si512(ptr++, val); // 8
		_mm512_store_si512(ptr++, val); // 9
		_mm512_store_si512(ptr++, val); // 10
		_mm512_store_si512(ptr++, val); // 11
		_mm512_store_si512(ptr++, val); // 12
		_mm512_store_si512(ptr++, val); // 13
		_mm512_store_si512(ptr++, val); // 14
		_mm512_store_si512(ptr++, val); // 15
		_mm512_store_si512(ptr++, val); // 16
		_mm512_store_si512(ptr++, val); // 17
		_mm512_store_si512(ptr++, val); // 18
		_mm512_store_si512(ptr++, val); // 19
		_mm512_store_si512(ptr++, val); // 20
		_mm512_store_si512(ptr++, val); // 21
		_mm512_store_si512(ptr++, val); // 22
		_mm512_store_si512(ptr++, val); // 23
		_mm512_store_si512(ptr++, val); // 24
		_mm512_store_si512(ptr++, val); // 25
		_mm512_store_si512(ptr++, val); // 26
		_mm512_store_si512(ptr++, val); // 27
		_mm512_store_si512(ptr++, val); // 28
		_mm512_store_si512(ptr++, val); // 29
		_mm512_store_si512(ptr++, val); // 30
		_mm512_store_si512(ptr++, val); // 31
		_mm512_store_si512(ptr++, val); // 32
		_mm512_store_si512(ptr++, val); // 1
		_mm512_store_si512(ptr++, val); // 2
		_mm512_store_si512(ptr++, val); // 3
		_mm512_store_si512(ptr++, val); // 4
		_mm512_store_si512(ptr++, val); // 5
		_mm512_store_si512(ptr++, val); // 6
		_mm512_store_si512(ptr++, val); // 7
		_mm512_store_si512(ptr++, val); // 8
		_mm512_store_si512(ptr++, val); // 9
		_mm512_store_si512(ptr++, val); // 10
		_mm512_store_si512(ptr++, val); // 11
		_mm512_store_si512(ptr++, val); // 12
		_mm512_store_si512(ptr++, val); // 13
		_mm512_store_si512(ptr++, val); // 14
		_mm512_store_si512(ptr++, val); // 15
		_mm512_store_si512(ptr++, val); // 16
		_mm512_store_si512(ptr++, val); // 17
		_mm512_store_si512(ptr++, val); // 18
		_mm512_store_si512(ptr++, val); // 19
		_mm512_store_si512(ptr++, val); // 20
		_mm512_store_si512(ptr++, val); // 21
		_mm512_store_si512(ptr++, val); // 22
		_mm512_store_si512(ptr++, val); // 23
		_mm512_store_si512(ptr++, val); // 24
		_mm512_store_si512(ptr++, val); // 25
		_mm512_store_si512(ptr++, val); // 26
		_mm512_store_si512(ptr++, val); // 27
		_mm512_store_si512(ptr++, val); // 28
		_mm512_store_si512(ptr++, val); // 29
		_mm512_store_si512(ptr++, val); // 30
		_mm512_store_si512(ptr++, val); // 31
		_mm512_store_si512(ptr++, val); // 32
	}
	return dest;
}
#endif
//
// SSE2 Streaming:
//
// If non-temporal stores are needed, then it's a big transfer
//
static void * memset_128bit_as(void * dest, const __m128i val, size_t len)
{
	__m128i * ptr = (__m128i*)dest;
	while(len--) {
		_mm_stream_si128(ptr++, val);
	}
	_mm_sfence();
	return dest;
}
//
// 32 bytes
//
static void * memset_128bit_32B_as(void * dest, const __m128i val, size_t len)
{
	__m128i * ptr = (__m128i*)dest;
	while(len--) {
		_mm_stream_si128(ptr++, val); // 1
		_mm_stream_si128(ptr++, val); // 2
	}
	_mm_sfence();
	return dest;
}
//
// 64 bytes
//
static void * memset_128bit_64B_as(void * dest, const __m128i val, size_t len)
{
	__m128i * ptr = (__m128i*)dest;
	while(len--) {
		_mm_stream_si128(ptr++, val); // 1
		_mm_stream_si128(ptr++, val); // 2
		_mm_stream_si128(ptr++, val); // 3
		_mm_stream_si128(ptr++, val); // 4
	}
	_mm_sfence();
	return dest;
}

// 128 bytes
static void * memset_128bit_128B_as(void * dest, const __m128i val, size_t len)
{
	__m128i * ptr = (__m128i*)dest;
	while(len--) {
		_mm_stream_si128(ptr++, val); // 1
		_mm_stream_si128(ptr++, val); // 2
		_mm_stream_si128(ptr++, val); // 3
		_mm_stream_si128(ptr++, val); // 4
		_mm_stream_si128(ptr++, val); // 5
		_mm_stream_si128(ptr++, val); // 6
		_mm_stream_si128(ptr++, val); // 7
		_mm_stream_si128(ptr++, val); // 8
	}
	_mm_sfence();
	return dest;
}

// 256 bytes
static void * memset_128bit_256B_as(void * dest, const __m128i val, size_t len)
{
	__m128i * ptr = (__m128i*)dest;
	while(len--) {
		_mm_stream_si128(ptr++, val); // 1
		_mm_stream_si128(ptr++, val); // 2
		_mm_stream_si128(ptr++, val); // 3
		_mm_stream_si128(ptr++, val); // 4
		_mm_stream_si128(ptr++, val); // 5
		_mm_stream_si128(ptr++, val); // 6
		_mm_stream_si128(ptr++, val); // 7
		_mm_stream_si128(ptr++, val); // 8
		_mm_stream_si128(ptr++, val); // 9
		_mm_stream_si128(ptr++, val); // 10
		_mm_stream_si128(ptr++, val); // 11
		_mm_stream_si128(ptr++, val); // 12
		_mm_stream_si128(ptr++, val); // 13
		_mm_stream_si128(ptr++, val); // 14
		_mm_stream_si128(ptr++, val); // 15
		_mm_stream_si128(ptr++, val); // 16
	}
	_mm_sfence();
	return dest;
}
//
// AVX+ Streaming:
//
// AVX (256-bit, 32 bytes at a time - 8 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/32), so it's "# of 256-bits"
// Sandybridge and Ryzen and up
//
#ifdef __AVX__

static void * memset_256bit_as(void * dest, const __m256i val, size_t len)
{
	__m256i * ptr = (__m256i*)dest;
	while(len--) {
		_mm256_stream_si256(ptr++, val);
	}
	_mm_sfence();
	return dest;
}

// 64 bytes
static void * memset_256bit_64B_as(void * dest, const __m256i val, size_t len)
{
	__m256i * ptr = (__m256i*)dest;
	while(len--) {
		_mm256_stream_si256(ptr++, val); // 1
		_mm256_stream_si256(ptr++, val); // 2
	}
	_mm_sfence();
	return dest;
}

// 128 bytes
static void * memset_256bit_128B_as(void * dest, const __m256i val, size_t len)
{
	__m256i * ptr = (__m256i*)dest;
	while(len--) {
		_mm256_stream_si256(ptr++, val); // 1
		_mm256_stream_si256(ptr++, val); // 2
		_mm256_stream_si256(ptr++, val); // 3
		_mm256_stream_si256(ptr++, val); // 4
	}
	_mm_sfence();
	return dest;
}

// 256 bytes
static void * memset_256bit_256B_as(void * dest, const __m256i val, size_t len)
{
	__m256i * ptr = (__m256i*)dest;
	while(len--) {
		_mm256_stream_si256(ptr++, val); // 1
		_mm256_stream_si256(ptr++, val); // 2
		_mm256_stream_si256(ptr++, val); // 3
		_mm256_stream_si256(ptr++, val); // 4
		_mm256_stream_si256(ptr++, val); // 5
		_mm256_stream_si256(ptr++, val); // 6
		_mm256_stream_si256(ptr++, val); // 7
		_mm256_stream_si256(ptr++, val); // 8
	}
	_mm_sfence();
	return dest;
}

// 512 bytes
static void * memset_256bit_512B_as(void * dest, const __m256i val, size_t len)
{
	__m256i * ptr = (__m256i*)dest;
	while(len--) {
		_mm256_stream_si256(ptr++, val); // 1
		_mm256_stream_si256(ptr++, val); // 2
		_mm256_stream_si256(ptr++, val); // 3
		_mm256_stream_si256(ptr++, val); // 4
		_mm256_stream_si256(ptr++, val); // 5
		_mm256_stream_si256(ptr++, val); // 6
		_mm256_stream_si256(ptr++, val); // 7
		_mm256_stream_si256(ptr++, val); // 8
		_mm256_stream_si256(ptr++, val); // 9
		_mm256_stream_si256(ptr++, val); // 10
		_mm256_stream_si256(ptr++, val); // 11
		_mm256_stream_si256(ptr++, val); // 12
		_mm256_stream_si256(ptr++, val); // 13
		_mm256_stream_si256(ptr++, val); // 14
		_mm256_stream_si256(ptr++, val); // 15
		_mm256_stream_si256(ptr++, val); // 16
	}
	_mm_sfence();
	return dest;
}

#endif
//
// AVX-512 (512-bit, 64 bytes at a time - 16 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/64), so it's "# of 512-bits"
// Requires AVX512F
//
#ifdef __AVX512F__

static void * memset_512bit_as(void * dest, const __m512i val, size_t len)
{
	__m512i * ptr = (__m512i*)dest;
	while(len--) {
		_mm512_stream_si512(ptr++, val);
	}
	_mm_sfence();
	return dest;
}

// 128 bytes
static void * memset_512bit_128B_as(void * dest, const __m512i val, size_t len)
{
	__m512i * ptr = (__m512i*)dest;
	while(len--) {
		_mm512_stream_si512(ptr++, val); // 1
		_mm512_stream_si512(ptr++, val); // 2
	}
	_mm_sfence();
	return dest;
}

// 256 bytes
static void * memset_512bit_256B_as(void * dest, const __m512i val, size_t len)
{
	__m512i * ptr = (__m512i*)dest;
	while(len--) {
		_mm512_stream_si512(ptr++, val); // 1
		_mm512_stream_si512(ptr++, val); // 2
		_mm512_stream_si512(ptr++, val); // 3
		_mm512_stream_si512(ptr++, val); // 4
	}
	_mm_sfence();
	return dest;
}

// 512 bytes
static void * memset_512bit_512B_as(void * dest, const __m512i val, size_t len)
{
	__m512i * ptr = (__m512i*)dest;
	while(len--) {
		_mm512_stream_si512(ptr++, val); // 1
		_mm512_stream_si512(ptr++, val); // 2
		_mm512_stream_si512(ptr++, val); // 3
		_mm512_stream_si512(ptr++, val); // 4
		_mm512_stream_si512(ptr++, val); // 5
		_mm512_stream_si512(ptr++, val); // 6
		_mm512_stream_si512(ptr++, val); // 7
		_mm512_stream_si512(ptr++, val); // 8
	}
	_mm_sfence();
	return dest;
}

// 1024 bytes, or 1 kB
static void * memset_512bit_1kB_as(void * dest, const __m512i val, size_t len)
{
	__m512i * ptr = (__m512i*)dest;
	while(len--) {
		_mm512_stream_si512(ptr++, val); // 1
		_mm512_stream_si512(ptr++, val); // 2
		_mm512_stream_si512(ptr++, val); // 3
		_mm512_stream_si512(ptr++, val); // 4
		_mm512_stream_si512(ptr++, val); // 5
		_mm512_stream_si512(ptr++, val); // 6
		_mm512_stream_si512(ptr++, val); // 7
		_mm512_stream_si512(ptr++, val); // 8
		_mm512_stream_si512(ptr++, val); // 9
		_mm512_stream_si512(ptr++, val); // 10
		_mm512_stream_si512(ptr++, val); // 11
		_mm512_stream_si512(ptr++, val); // 12
		_mm512_stream_si512(ptr++, val); // 13
		_mm512_stream_si512(ptr++, val); // 14
		_mm512_stream_si512(ptr++, val); // 15
		_mm512_stream_si512(ptr++, val); // 16
	}
	_mm_sfence();
	return dest;
}

// 2048 bytes, or 2 kB
static void * memset_512bit_2kB_as(void * dest, const __m512i val, size_t len)
{
	__m512i * ptr = (__m512i*)dest;
	while(len--) {
		_mm512_stream_si512(ptr++, val); // 1
		_mm512_stream_si512(ptr++, val); // 2
		_mm512_stream_si512(ptr++, val); // 3
		_mm512_stream_si512(ptr++, val); // 4
		_mm512_stream_si512(ptr++, val); // 5
		_mm512_stream_si512(ptr++, val); // 6
		_mm512_stream_si512(ptr++, val); // 7
		_mm512_stream_si512(ptr++, val); // 8
		_mm512_stream_si512(ptr++, val); // 9
		_mm512_stream_si512(ptr++, val); // 10
		_mm512_stream_si512(ptr++, val); // 11
		_mm512_stream_si512(ptr++, val); // 12
		_mm512_stream_si512(ptr++, val); // 13
		_mm512_stream_si512(ptr++, val); // 14
		_mm512_stream_si512(ptr++, val); // 15
		_mm512_stream_si512(ptr++, val); // 16
		_mm512_stream_si512(ptr++, val); // 17
		_mm512_stream_si512(ptr++, val); // 18
		_mm512_stream_si512(ptr++, val); // 19
		_mm512_stream_si512(ptr++, val); // 20
		_mm512_stream_si512(ptr++, val); // 21
		_mm512_stream_si512(ptr++, val); // 22
		_mm512_stream_si512(ptr++, val); // 23
		_mm512_stream_si512(ptr++, val); // 24
		_mm512_stream_si512(ptr++, val); // 25
		_mm512_stream_si512(ptr++, val); // 26
		_mm512_stream_si512(ptr++, val); // 27
		_mm512_stream_si512(ptr++, val); // 28
		_mm512_stream_si512(ptr++, val); // 29
		_mm512_stream_si512(ptr++, val); // 30
		_mm512_stream_si512(ptr++, val); // 31
		_mm512_stream_si512(ptr++, val); // 32
	}
	_mm_sfence();
	return dest;
}

// 4096 bytes, or 4 kB, also 1 page
static void * memset_512bit_4kB_as(void * dest, const __m512i val, size_t len)
{
	__m512i * ptr = (__m512i*)dest;
	while(len--) {
		_mm512_stream_si512(ptr++, val); // 1
		_mm512_stream_si512(ptr++, val); // 2
		_mm512_stream_si512(ptr++, val); // 3
		_mm512_stream_si512(ptr++, val); // 4
		_mm512_stream_si512(ptr++, val); // 5
		_mm512_stream_si512(ptr++, val); // 6
		_mm512_stream_si512(ptr++, val); // 7
		_mm512_stream_si512(ptr++, val); // 8
		_mm512_stream_si512(ptr++, val); // 9
		_mm512_stream_si512(ptr++, val); // 10
		_mm512_stream_si512(ptr++, val); // 11
		_mm512_stream_si512(ptr++, val); // 12
		_mm512_stream_si512(ptr++, val); // 13
		_mm512_stream_si512(ptr++, val); // 14
		_mm512_stream_si512(ptr++, val); // 15
		_mm512_stream_si512(ptr++, val); // 16
		_mm512_stream_si512(ptr++, val); // 17
		_mm512_stream_si512(ptr++, val); // 18
		_mm512_stream_si512(ptr++, val); // 19
		_mm512_stream_si512(ptr++, val); // 20
		_mm512_stream_si512(ptr++, val); // 21
		_mm512_stream_si512(ptr++, val); // 22
		_mm512_stream_si512(ptr++, val); // 23
		_mm512_stream_si512(ptr++, val); // 24
		_mm512_stream_si512(ptr++, val); // 25
		_mm512_stream_si512(ptr++, val); // 26
		_mm512_stream_si512(ptr++, val); // 27
		_mm512_stream_si512(ptr++, val); // 28
		_mm512_stream_si512(ptr++, val); // 29
		_mm512_stream_si512(ptr++, val); // 30
		_mm512_stream_si512(ptr++, val); // 31
		_mm512_stream_si512(ptr++, val); // 32
		_mm512_stream_si512(ptr++, val); // 1
		_mm512_stream_si512(ptr++, val); // 2
		_mm512_stream_si512(ptr++, val); // 3
		_mm512_stream_si512(ptr++, val); // 4
		_mm512_stream_si512(ptr++, val); // 5
		_mm512_stream_si512(ptr++, val); // 6
		_mm512_stream_si512(ptr++, val); // 7
		_mm512_stream_si512(ptr++, val); // 8
		_mm512_stream_si512(ptr++, val); // 9
		_mm512_stream_si512(ptr++, val); // 10
		_mm512_stream_si512(ptr++, val); // 11
		_mm512_stream_si512(ptr++, val); // 12
		_mm512_stream_si512(ptr++, val); // 13
		_mm512_stream_si512(ptr++, val); // 14
		_mm512_stream_si512(ptr++, val); // 15
		_mm512_stream_si512(ptr++, val); // 16
		_mm512_stream_si512(ptr++, val); // 17
		_mm512_stream_si512(ptr++, val); // 18
		_mm512_stream_si512(ptr++, val); // 19
		_mm512_stream_si512(ptr++, val); // 20
		_mm512_stream_si512(ptr++, val); // 21
		_mm512_stream_si512(ptr++, val); // 22
		_mm512_stream_si512(ptr++, val); // 23
		_mm512_stream_si512(ptr++, val); // 24
		_mm512_stream_si512(ptr++, val); // 25
		_mm512_stream_si512(ptr++, val); // 26
		_mm512_stream_si512(ptr++, val); // 27
		_mm512_stream_si512(ptr++, val); // 28
		_mm512_stream_si512(ptr++, val); // 29
		_mm512_stream_si512(ptr++, val); // 30
		_mm512_stream_si512(ptr++, val); // 31
		_mm512_stream_si512(ptr++, val); // 32
	}
	_mm_sfence();
	return dest;
}
#endif
//
// Dispatch Functions:
//
// Set arbitrarily large amounts of a single byte
void * memset_large(void * dest, const uint8_t val, size_t numbytes)
{
	void * returnval = dest; // Memset is supposed to return the initial destination
	if(val == 0) { // Someone called this insted of memset_zeroes directly
		memset_zeroes(dest, numbytes);
		return returnval;
	}
	size_t offset = 0; // Offset size needs to match the size of a pointer
	while(numbytes) {
		// Each memset has its own loop.
		if(numbytes < 16) { // 1-15 bytes (the other scalars would need to be memset anyways)
			memset(dest, val, numbytes);
			offset = numbytes;
			dest = (char *)dest + offset;
			numbytes = 0;
		}
#ifdef __AVX512F__
		else if(numbytes < 32) { // 16 bytes
			memset_128bit_u(dest, _mm_set1_epi8((char)val), numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memset_256bit_u(dest, _mm256_set1_epi8((char)val), numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memset_512bit_u(dest, _mm512_set1_epi8((char)val), numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memset_512bit_128B_u(dest, _mm512_set1_epi8((char)val), numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memset_512bit_256B_u(dest, _mm512_set1_epi8((char)val), numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			numbytes &= 255;
		}
		else if(numbytes < 1024) { // 512 bytes
			memset_512bit_512B_u(dest, _mm512_set1_epi8((char)val), numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			numbytes &= 511;
		}
		else if(numbytes < 2048) { // 1024 bytes (1 kB)
			memset_512bit_1kB_u(dest, _mm512_set1_epi8((char)val), numbytes >> 10);
			offset = numbytes & -1024;
			dest = (char *)dest + offset;
			numbytes &= 1023;
		}
		else if(numbytes < 4096) { // 2048 bytes (2 kB)
			memset_512bit_2kB_u(dest, _mm512_set1_epi8((char)val), numbytes >> 11);
			offset = numbytes & -2048;
			dest = (char *)dest + offset;
			numbytes &= 2047;
		}
		else { // 4096 bytes (4 kB)
			memset_512bit_4kB_u(dest, _mm512_set1_epi8((char)val), numbytes >> 12);
			offset = numbytes & -4096;
			dest = (char *)dest + offset;
			numbytes &= 4095;
		}
#elif __AVX__
		else if(numbytes < 32) { // 16 bytes
			memset_128bit_u(dest, _mm_set1_epi8((char)val), numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memset_256bit_u(dest, _mm256_set1_epi8((char)val), numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memset_256bit_64B_u(dest, _mm256_set1_epi8((char)val), numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memset_256bit_128B_u(dest, _mm256_set1_epi8((char)val), numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memset_256bit_256B_u(dest, _mm256_set1_epi8((char)val), numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			numbytes &= 255;
		}
		else { // 512 bytes
			memset_256bit_512B_u(dest, _mm256_set1_epi8((char)val), numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			numbytes &= 511;
		}
#else // SSE2 only
		else if(numbytes < 32) { // 16 bytes
			memset_128bit_u(dest, _mm_set1_epi8((char)val), numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memset_128bit_32B_u(dest, _mm_set1_epi8((char)val), numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memset_128bit_64B_u(dest, _mm_set1_epi8((char)val), numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memset_128bit_128B_u(dest, _mm_set1_epi8((char)val), numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			numbytes &= 127;
		}
		else { // 256 bytes
			memset_128bit_256B_u(dest, _mm_set1_epi8((char)val), numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			numbytes &= 255;
		}
#endif
	}
	return returnval;
} // END MEMSET LARGE, UNALIGNED

// Set arbitrarily large amounts of a single byte
// Aligned version
void * memset_large_a(void * dest, const uint8_t val, size_t numbytes)
{
	void * returnval = dest; // Memset is supposed to return the initial destination
	if(val == 0) { // Someone called this insted of memset_zeroes directly
		memset_zeroes_a(dest, numbytes);
		return returnval;
	}
	size_t offset = 0; // Offset size needs to match the size of a pointer
	while(numbytes) {
		// Each memset has its own loop.
		if(numbytes < 16) { // 1-15 bytes (the other scalars would need to be memset anyways)
			memset(dest, val, numbytes);
			offset = numbytes;
			dest = (char *)dest + offset;
			numbytes = 0;
		}
#ifdef __AVX512F__
		else if(numbytes < 32) { // 16 bytes
			memset_128bit_a(dest, _mm_set1_epi8((char)val), numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memset_256bit_a(dest, _mm256_set1_epi8((char)val), numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memset_512bit_a(dest, _mm512_set1_epi8((char)val), numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memset_512bit_128B_a(dest, _mm512_set1_epi8((char)val), numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memset_512bit_256B_a(dest, _mm512_set1_epi8((char)val), numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			numbytes &= 255;
		}
		else if(numbytes < 1024) { // 512 bytes
			memset_512bit_512B_a(dest, _mm512_set1_epi8((char)val), numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			numbytes &= 511;
		}
		else if(numbytes < 2048) { // 1024 bytes (1 kB)
			memset_512bit_1kB_a(dest, _mm512_set1_epi8((char)val), numbytes >> 10);
			offset = numbytes & -1024;
			dest = (char *)dest + offset;
			numbytes &= 1023;
		}
		else if(numbytes < 4096) { // 2048 bytes (2 kB)
			memset_512bit_2kB_a(dest, _mm512_set1_epi8((char)val), numbytes >> 11);
			offset = numbytes & -2048;
			dest = (char *)dest + offset;
			numbytes &= 2047;
		}
		else { // 4096 bytes (4 kB)
			memset_512bit_4kB_a(dest, _mm512_set1_epi8((char)val), numbytes >> 12);
			offset = numbytes & -4096;
			dest = (char *)dest + offset;
			numbytes &= 4095;
		}
#elif __AVX__
		else if(numbytes < 32) { // 16 bytes
			memset_128bit_a(dest, _mm_set1_epi8((char)val), numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memset_256bit_a(dest, _mm256_set1_epi8((char)val), numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memset_256bit_64B_a(dest, _mm256_set1_epi8((char)val), numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memset_256bit_128B_a(dest, _mm256_set1_epi8((char)val), numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memset_256bit_256B_a(dest, _mm256_set1_epi8((char)val), numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			numbytes &= 255;
		}
		else { // 512 bytes
			memset_256bit_512B_a(dest, _mm256_set1_epi8((char)val), numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			numbytes &= 511;
		}
#else // SSE2 only
		else if(numbytes < 32) { // 16 bytes
			memset_128bit_a(dest, _mm_set1_epi8((char)val), numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memset_128bit_32B_a(dest, _mm_set1_epi8((char)val), numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memset_128bit_64B_a(dest, _mm_set1_epi8((char)val), numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memset_128bit_128B_a(dest, _mm_set1_epi8((char)val), numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			numbytes &= 127;
		}
		else { // 256 bytes
			memset_128bit_256B_a(dest, _mm_set1_epi8((char)val), numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			numbytes &= 255;
		}
#endif
	}
	return returnval;
} // END MEMSET LARGE, ALIGNED

// Set arbitrarily large amounts of a single byte
// Aligned, streaming version
void * memset_large_as(void * dest, const uint8_t val, size_t numbytes)
{
	void * returnval = dest; // Memset is supposed to return the initial destination
	if(val == 0) { // Someone called this insted of memset_zeroes directly
		memset_zeroes_as(dest, numbytes);
		return returnval;
	}
	size_t offset = 0; // Offset size needs to match the size of a pointer
	while(numbytes) {
		// Each memset has its own loop.
		if(numbytes < 16) { // 1-15 bytes (the other scalars would need to be memset anyways)
			memset(dest, val, numbytes);
			offset = numbytes;
			dest = (char *)dest + offset;
			numbytes = 0;
		}
#ifdef __AVX512F__
		else if(numbytes < 32) { // 16 bytes
			memset_128bit_as(dest, _mm_set1_epi8((char)val), numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memset_256bit_as(dest, _mm256_set1_epi8((char)val), numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memset_512bit_as(dest, _mm512_set1_epi8((char)val), numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memset_512bit_128B_as(dest, _mm512_set1_epi8((char)val), numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memset_512bit_256B_as(dest, _mm512_set1_epi8((char)val), numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			numbytes &= 255;
		}
		else if(numbytes < 1024) { // 512 bytes
			memset_512bit_512B_as(dest, _mm512_set1_epi8((char)val), numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			numbytes &= 511;
		}
		else if(numbytes < 2048) { // 1024 bytes (1 kB)
			memset_512bit_1kB_as(dest, _mm512_set1_epi8((char)val), numbytes >> 10);
			offset = numbytes & -1024;
			dest = (char *)dest + offset;
			numbytes &= 1023;
		}
		else if(numbytes < 4096) { // 2048 bytes (2 kB)
			memset_512bit_2kB_as(dest, _mm512_set1_epi8((char)val), numbytes >> 11);
			offset = numbytes & -2048;
			dest = (char *)dest + offset;
			numbytes &= 2047;
		}
		else { // 4096 bytes (4 kB)
			memset_512bit_4kB_as(dest, _mm512_set1_epi8((char)val), numbytes >> 12);
			offset = numbytes & -4096;
			dest = (char *)dest + offset;
			numbytes &= 4095;
		}
#elif __AVX__
		else if(numbytes < 32) { // 16 bytes
			memset_128bit_as(dest, _mm_set1_epi8((char)val), numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memset_256bit_as(dest, _mm256_set1_epi8((char)val), numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memset_256bit_64B_as(dest, _mm256_set1_epi8((char)val), numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memset_256bit_128B_as(dest, _mm256_set1_epi8((char)val), numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memset_256bit_256B_as(dest, _mm256_set1_epi8((char)val), numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			numbytes &= 255;
		}
		else { // 512 bytes
			memset_256bit_512B_as(dest, _mm256_set1_epi8((char)val), numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			numbytes &= 511;
		}
#else // SSE2 only
		else if(numbytes < 32) { // 16 bytes
			memset_128bit_as(dest, _mm_set1_epi8((char)val), numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memset_128bit_32B_as(dest, _mm_set1_epi8((char)val), numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memset_128bit_64B_as(dest, _mm_set1_epi8((char)val), numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memset_128bit_128B_as(dest, _mm_set1_epi8((char)val), numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			numbytes &= 127;
		}
		else { // 256 bytes
			memset_128bit_256B_as(dest, _mm_set1_epi8((char)val), numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			numbytes &= 255;
		}
#endif
	}
	return returnval;
} // END MEMSET LARGE, ALIGNED, STREAMING

// Set arbitrarily large amounts of only zeroes
void * memset_zeroes(void * dest, size_t numbytes) // Worst-case scenario: 127 bytes
{
	void * returnval = dest; // Memset is supposed to return the initial destination
	size_t offset = 0;
	while(numbytes) {
		// Each memset has its own loop.
		if(numbytes < 2) { // 1 byte
			memset(dest, 0, numbytes);
			offset = numbytes & -1;
			dest = (char *)dest + offset;
			numbytes = 0;
		}
		else if(numbytes < 4) { // 2 bytes
			memset_16bit(dest, 0, numbytes >> 1);
			offset = numbytes & -2;
			dest = (char *)dest + offset;
			numbytes &= 1;
		}
		else if(numbytes < 8) { // 4 bytes
			memset_32bit(dest, 0, numbytes >> 2);
			offset = numbytes & -4;
			dest = (char *)dest + offset;
			numbytes &= 3;
		}
		else if(numbytes < 16) { // 8 bytes
			memset_64bit(dest, 0, numbytes >> 3);
			offset = numbytes & -8;
			dest = (char *)dest + offset;
			numbytes &= 7;
		}
#ifdef __AVX512F__
		else if(numbytes < 32) { // 16 bytes
			memset_128bit_u(dest, _mm_setzero_si128(), numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memset_256bit_u(dest, _mm256_setzero_si256(), numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memset_512bit_u(dest, _mm512_setzero_si512(), numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memset_512bit_128B_u(dest, _mm512_setzero_si512(), numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memset_512bit_256B_u(dest, _mm512_setzero_si512(), numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			numbytes &= 255;
		}
		else if(numbytes < 1024) { // 512 bytes
			memset_512bit_512B_u(dest, _mm512_setzero_si512(), numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			numbytes &= 511;
		}
		else if(numbytes < 2048) { // 1024 bytes (1 kB)
			memset_512bit_1kB_u(dest, _mm512_setzero_si512(), numbytes >> 10);
			offset = numbytes & -1024;
			dest = (char *)dest + offset;
			numbytes &= 1023;
		}
		else if(numbytes < 4096) { // 2048 bytes (2 kB)
			memset_512bit_2kB_u(dest, _mm512_setzero_si512(), numbytes >> 11);
			offset = numbytes & -2048;
			dest = (char *)dest + offset;
			numbytes &= 2047;
		}
		else { // 4096 bytes (4 kB)
			memset_512bit_4kB_u(dest, _mm512_setzero_si512(), numbytes >> 12);
			offset = numbytes & -4096;
			dest = (char *)dest + offset;
			numbytes &= 4095;
		}
#elif __AVX__
		else if(numbytes < 32) { // 16 bytes
			memset_128bit_u(dest, _mm_setzero_si128(), numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memset_256bit_u(dest, _mm256_setzero_si256(), numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memset_256bit_64B_u(dest, _mm256_setzero_si256(), numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memset_256bit_128B_u(dest, _mm256_setzero_si256(), numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memset_256bit_256B_u(dest, _mm256_setzero_si256(), numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			numbytes &= 255;
		}
		else { // 512 bytes
			memset_256bit_512B_u(dest, _mm256_setzero_si256(), numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			numbytes &= 511;
		}
#else // SSE2 only
		else if(numbytes < 32) { // 16 bytes
			memset_128bit_u(dest, _mm_setzero_si128(), numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memset_128bit_32B_u(dest, _mm_setzero_si128(), numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memset_128bit_64B_u(dest, _mm_setzero_si128(), numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memset_128bit_128B_u(dest, _mm_setzero_si128(), numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			numbytes &= 127;
		}
		else { // 256 bytes
			memset_128bit_256B_u(dest, _mm_setzero_si128(), numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			numbytes &= 255;
		}
#endif
	}
	return returnval;
} // END MEMSET ZEROES, UNALIGNED

// Set arbitrarily large amounts of only zeroes
// Aligned version
void * memset_zeroes_a(void * dest, size_t numbytes) // Worst-case scenario: 127 bytes
{
	void * returnval = dest; // Memset is supposed to return the initial destination
	size_t offset = 0;
	while(numbytes) {
		// Each memset has its own loop.
		if(numbytes < 2) { // 1 byte
			memset(dest, 0, numbytes);
			offset = numbytes & -1;
			dest = (char *)dest + offset;
			numbytes = 0;
		}
		else if(numbytes < 4) { // 2 bytes
			memset_16bit(dest, 0, numbytes >> 1);
			offset = numbytes & -2;
			dest = (char *)dest + offset;
			numbytes &= 1;
		}
		else if(numbytes < 8) { // 4 bytes
			memset_32bit(dest, 0, numbytes >> 2);
			offset = numbytes & -4;
			dest = (char *)dest + offset;
			numbytes &= 3;
		}
		else if(numbytes < 16) { // 8 bytes
			memset_64bit(dest, 0, numbytes >> 3);
			offset = numbytes & -8;
			dest = (char *)dest + offset;
			numbytes &= 7;
		}
#ifdef __AVX512F__
		else if(numbytes < 32) { // 16 bytes
			memset_128bit_a(dest, _mm_setzero_si128(), numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memset_256bit_a(dest, _mm256_setzero_si256(), numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memset_512bit_a(dest, _mm512_setzero_si512(), numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memset_512bit_128B_a(dest, _mm512_setzero_si512(), numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memset_512bit_256B_a(dest, _mm512_setzero_si512(), numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			numbytes &= 255;
		}
		else if(numbytes < 1024) { // 512 bytes
			memset_512bit_512B_a(dest, _mm512_setzero_si512(), numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			numbytes &= 511;
		}
		else if(numbytes < 2048) { // 1024 bytes (1 kB)
			memset_512bit_1kB_a(dest, _mm512_setzero_si512(), numbytes >> 10);
			offset = numbytes & -1024;
			dest = (char *)dest + offset;
			numbytes &= 1023;
		}
		else if(numbytes < 4096) { // 2048 bytes (2 kB)
			memset_512bit_2kB_a(dest, _mm512_setzero_si512(), numbytes >> 11);
			offset = numbytes & -2048;
			dest = (char *)dest + offset;
			numbytes &= 2047;
		}
		else { // 4096 bytes (4 kB)
			memset_512bit_4kB_a(dest, _mm512_setzero_si512(), numbytes >> 12);
			offset = numbytes & -4096;
			dest = (char *)dest + offset;
			numbytes &= 4095;
		}
#elif __AVX__
		else if(numbytes < 32) { // 16 bytes
			memset_128bit_a(dest, _mm_setzero_si128(), numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memset_256bit_a(dest, _mm256_setzero_si256(), numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memset_256bit_64B_a(dest, _mm256_setzero_si256(), numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memset_256bit_128B_a(dest, _mm256_setzero_si256(), numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memset_256bit_256B_a(dest, _mm256_setzero_si256(), numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			numbytes &= 255;
		}
		else { // 512 bytes
			memset_256bit_512B_a(dest, _mm256_setzero_si256(), numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			numbytes &= 511;
		}
#else // SSE2 only
		else if(numbytes < 32) { // 16 bytes
			memset_128bit_a(dest, _mm_setzero_si128(), numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memset_128bit_32B_a(dest, _mm_setzero_si128(), numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memset_128bit_64B_a(dest, _mm_setzero_si128(), numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memset_128bit_128B_a(dest, _mm_setzero_si128(), numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			numbytes &= 127;
		}
		else { // 256 bytes
			memset_128bit_256B_a(dest, _mm_setzero_si128(), numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			numbytes &= 255;
		}
#endif
	}
	return returnval;
} // END MEMSET ZEROES, ALIGNED

// Set arbitrarily large amounts of only zeroes
// Aligned, streaming version
void * memset_zeroes_as(void * dest, size_t numbytes) // Worst-case scenario: 127 bytes
{
	void * returnval = dest; // Memset is supposed to return the initial destination
	size_t offset = 0;
	while(numbytes) {
		// Each memset has its own loop.
		if(numbytes < 2) { // 1 byte
			memset(dest, 0, numbytes);
			offset = numbytes & -1;
			dest = (char *)dest + offset;
			numbytes = 0;
		}
		else if(numbytes < 4) { // 2 bytes
			memset_16bit(dest, 0, numbytes >> 1);
			offset = numbytes & -2;
			dest = (char *)dest + offset;
			numbytes &= 1;
		}
		else if(numbytes < 8) { // 4 bytes
			memset_32bit(dest, 0, numbytes >> 2);
			offset = numbytes & -4;
			dest = (char *)dest + offset;
			numbytes &= 3;
		}
		else if(numbytes < 16) { // 8 bytes
			memset_64bit(dest, 0, numbytes >> 3);
			offset = numbytes & -8;
			dest = (char *)dest + offset;
			numbytes &= 7;
		}
#ifdef __AVX512F__
		else if(numbytes < 32) { // 16 bytes
			memset_128bit_as(dest, _mm_setzero_si128(), numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memset_256bit_as(dest, _mm256_setzero_si256(), numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memset_512bit_as(dest, _mm512_setzero_si512(), numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memset_512bit_128B_as(dest, _mm512_setzero_si512(), numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memset_512bit_256B_as(dest, _mm512_setzero_si512(), numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			numbytes &= 255;
		}
		else if(numbytes < 1024) { // 512 bytes
			memset_512bit_512B_as(dest, _mm512_setzero_si512(), numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			numbytes &= 511;
		}
		else if(numbytes < 2048) { // 1024 bytes (1 kB)
			memset_512bit_1kB_as(dest, _mm512_setzero_si512(), numbytes >> 10);
			offset = numbytes & -1024;
			dest = (char *)dest + offset;
			numbytes &= 1023;
		}
		else if(numbytes < 4096) { // 2048 bytes (2 kB)
			memset_512bit_2kB_as(dest, _mm512_setzero_si512(), numbytes >> 11);
			offset = numbytes & -2048;
			dest = (char *)dest + offset;
			numbytes &= 2047;
		}
		else { // 4096 bytes (4 kB)
			memset_512bit_4kB_as(dest, _mm512_setzero_si512(), numbytes >> 12);
			offset = numbytes & -4096;
			dest = (char *)dest + offset;
			numbytes &= 4095;
		}
#elif __AVX__
		else if(numbytes < 32) { // 16 bytes
			memset_128bit_as(dest, _mm_setzero_si128(), numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memset_256bit_as(dest, _mm256_setzero_si256(), numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memset_256bit_64B_as(dest, _mm256_setzero_si256(), numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memset_256bit_128B_as(dest, _mm256_setzero_si256(), numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memset_256bit_256B_as(dest, _mm256_setzero_si256(), numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			numbytes &= 255;
		}
		else { // 512 bytes
			memset_256bit_512B_as(dest, _mm256_setzero_si256(), numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			numbytes &= 511;
		}
#else // SSE2 only
		else if(numbytes < 32) { // 16 bytes
			memset_128bit_as(dest, _mm_setzero_si128(), numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memset_128bit_32B_as(dest, _mm_setzero_si128(), numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memset_128bit_64B_as(dest, _mm_setzero_si128(), numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memset_128bit_128B_as(dest, _mm_setzero_si128(), numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			numbytes &= 127;
		}
		else { // 256 bytes
			memset_128bit_256B_as(dest, _mm_setzero_si128(), numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			numbytes &= 255;
		}
#endif
	}
	return returnval;
} // END MEMSET ZEROES, ALIGNED, STREAMING

// Set arbitrarily large amounts of 4-byte values
// numbytes_div_4 is total number of bytes / 4 (since this is 4 bytes at a time)
void * memset_large_4B(void * dest, const uint32_t val, size_t numbytes_div_4)
{
	void * returnval = dest; // Memset is supposed to return the initial destination
	size_t offset = 0; // Offset size needs to match the size of a pointer

	while(numbytes_div_4) {
		// Each memset has its own loop.
		if(numbytes_div_4 < 4) { // 4, 8, 12 bytes (the other scalars would need to be memset anyways)
			memset_32bit(dest, val, numbytes_div_4);
			offset = numbytes_div_4;
			dest = (char *)dest + offset;
			numbytes_div_4 = 0;
		}
#ifdef __AVX512F__
		else if(numbytes_div_4 < 8) { // 16 bytes
			memset_128bit_u(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 2);
			offset = numbytes_div_4 & -4;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 3;
		}
		else if(numbytes_div_4 < 16) { // 32 bytes
			memset_256bit_u(dest, _mm256_set1_epi32((int32_t)val), numbytes_div_4 >> 3);
			offset = numbytes_div_4 & -8;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 7;
		}
		else if(numbytes_div_4 < 32) { // 64 bytes
			memset_512bit_u(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 4);
			offset = numbytes_div_4 & -16;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 15;
		}
		else if(numbytes_div_4 < 64) { // 128 bytes
			memset_512bit_128B_u(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 5);
			offset = numbytes_div_4 & -32;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 31;
		}
		else if(numbytes_div_4 < 128) { // 256 bytes
			memset_512bit_256B_u(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 6);
			offset = numbytes_div_4 & -64;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 63;
		}
		else if(numbytes_div_4 < 256) { // 512 bytes
			memset_512bit_512B_u(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 7);
			offset = numbytes_div_4 & -128;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 127;
		}
		else if(numbytes_div_4 < 512) { // 1024 bytes (1 kB)
			memset_512bit_1kB_u(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 8);
			offset = numbytes_div_4 & -256;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 255;
		}
		else if(numbytes_div_4 < 1024) { // 2048 bytes (2 kB)
			memset_512bit_2kB_u(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 9);
			offset = numbytes_div_4 & -512;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 511;
		}
		else { // 4096 bytes (4 kB)
			memset_512bit_4kB_u(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 10);
			offset = numbytes_div_4 & -1024;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 1023;
		}
#elif __AVX__
		else if(numbytes_div_4 < 8) { // 16 bytes
			memset_128bit_u(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 2);
			offset = numbytes_div_4 & -4;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 3;
		}
		else if(numbytes_div_4 < 16) { // 32 bytes
			memset_256bit_u(dest, _mm256_set1_epi32((int32_t)val), numbytes_div_4 >> 3);
			offset = numbytes_div_4 & -8;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 7;
		}
		else if(numbytes_div_4 < 32) { // 64 bytes
			memset_256bit_64B_u(dest, _mm256_set1_epi32((int32_t)val), numbytes_div_4 >> 4);
			offset = numbytes_div_4 & -16;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 15;
		}
		else if(numbytes_div_4 < 64) { // 128 bytes
			memset_256bit_128B_u(dest, _mm256_set1_epi32((int32_t)val), numbytes_div_4 >> 5);
			offset = numbytes_div_4 & -32;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 31;
		}
		else if(numbytes_div_4 < 128) { // 256 bytes
			memset_256bit_256B_u(dest, _mm256_set1_epi32((int32_t)val), numbytes_div_4 >> 6);
			offset = numbytes_div_4 & -64;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 63;
		}
		else { // 512 bytes
			memset_256bit_512B_u(dest, _mm256_set1_epi32((int32_t)val), numbytes_div_4 >> 7);
			offset = numbytes_div_4 & -128;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 127;
		}
#else // SSE2 only
		else if(numbytes_div_4 < 8) { // 16 bytes
			memset_128bit_u(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 2);
			offset = numbytes_div_4 & -4;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 3;
		}
		else if(numbytes_div_4 < 16) { // 32 bytes
			memset_128bit_32B_u(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 3);
			offset = numbytes_div_4 & -8;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 7;
		}
		else if(numbytes_div_4 < 32) { // 64 bytes
			memset_128bit_64B_u(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 4);
			offset = numbytes_div_4 & -16;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 15;
		}
		else if(numbytes_div_4 < 64) { // 128 bytes
			memset_128bit_128B_u(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 5);
			offset = numbytes_div_4 & -32;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 31;
		}
		else { // 256 bytes
			memset_128bit_256B_u(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 6);
			offset = numbytes_div_4 & -64;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 63;
		}
#endif
	}
	return returnval;
} // END MEMSET LARGE, 4B, UNALIGNED

// Set arbitrarily large amounts of 4-byte values
// numbytes_div_4 is total number of bytes / 4 (since this is 4 bytes at a time)
// Aligned version
void * memset_large_4B_a(void * dest, const uint32_t val, size_t numbytes_div_4)
{
	void * returnval = dest; // Memset is supposed to return the initial destination
	size_t offset = 0; // Offset size needs to match the size of a pointer

	while(numbytes_div_4) {
		// Each memset has its own loop.
		if(numbytes_div_4 < 4) { // 4, 8, 12 bytes (the other scalars would need to be memset anyways)
			memset_32bit(dest, val, numbytes_div_4);
			offset = numbytes_div_4;
			dest = (char *)dest + offset;
			numbytes_div_4 = 0;
		}
#ifdef __AVX512F__
		else if(numbytes_div_4 < 8) { // 16 bytes
			memset_128bit_a(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 2);
			offset = numbytes_div_4 & -4;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 3;
		}
		else if(numbytes_div_4 < 16) { // 32 bytes
			memset_256bit_a(dest, _mm256_set1_epi32((int32_t)val), numbytes_div_4 >> 3);
			offset = numbytes_div_4 & -8;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 7;
		}
		else if(numbytes_div_4 < 32) { // 64 bytes
			memset_512bit_a(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 4);
			offset = numbytes_div_4 & -16;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 15;
		}
		else if(numbytes_div_4 < 64) { // 128 bytes
			memset_512bit_128B_a(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 5);
			offset = numbytes_div_4 & -32;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 31;
		}
		else if(numbytes_div_4 < 128) { // 256 bytes
			memset_512bit_256B_a(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 6);
			offset = numbytes_div_4 & -64;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 63;
		}
		else if(numbytes_div_4 < 256) { // 512 bytes
			memset_512bit_512B_a(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 7);
			offset = numbytes_div_4 & -128;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 127;
		}
		else if(numbytes_div_4 < 512) { // 1024 bytes (1 kB)
			memset_512bit_1kB_a(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 8);
			offset = numbytes_div_4 & -256;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 255;
		}
		else if(numbytes_div_4 < 1024) { // 2048 bytes (2 kB)
			memset_512bit_2kB_a(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 9);
			offset = numbytes_div_4 & -512;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 511;
		}
		else { // 4096 bytes (4 kB)
			memset_512bit_4kB_a(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 10);
			offset = numbytes_div_4 & -1024;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 1023;
		}
#elif __AVX__
		else if(numbytes_div_4 < 8) { // 16 bytes
			memset_128bit_a(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 2);
			offset = numbytes_div_4 & -4;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 3;
		}
		else if(numbytes_div_4 < 16) { // 32 bytes
			memset_256bit_a(dest, _mm256_set1_epi32((int32_t)val), numbytes_div_4 >> 3);
			offset = numbytes_div_4 & -8;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 7;
		}
		else if(numbytes_div_4 < 32) { // 64 bytes
			memset_256bit_64B_a(dest, _mm256_set1_epi32((int32_t)val), numbytes_div_4 >> 4);
			offset = numbytes_div_4 & -16;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 15;
		}
		else if(numbytes_div_4 < 64) { // 128 bytes
			memset_256bit_128B_a(dest, _mm256_set1_epi32((int32_t)val), numbytes_div_4 >> 5);
			offset = numbytes_div_4 & -32;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 31;
		}
		else if(numbytes_div_4 < 128) { // 256 bytes
			memset_256bit_256B_a(dest, _mm256_set1_epi32((int32_t)val), numbytes_div_4 >> 6);
			offset = numbytes_div_4 & -64;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 63;
		}
		else { // 512 bytes
			memset_256bit_512B_a(dest, _mm256_set1_epi32((int32_t)val), numbytes_div_4 >> 7);
			offset = numbytes_div_4 & -128;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 127;
		}
#else // SSE2 only
		else if(numbytes_div_4 < 8) { // 16 bytes
			memset_128bit_a(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 2);
			offset = numbytes_div_4 & -4;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 3;
		}
		else if(numbytes_div_4 < 16) { // 32 bytes
			memset_128bit_32B_a(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 3);
			offset = numbytes_div_4 & -8;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 7;
		}
		else if(numbytes_div_4 < 32) { // 64 bytes
			memset_128bit_64B_a(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 4);
			offset = numbytes_div_4 & -16;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 15;
		}
		else if(numbytes_div_4 < 64) { // 128 bytes
			memset_128bit_128B_a(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 5);
			offset = numbytes_div_4 & -32;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 31;
		}
		else { // 256 bytes
			memset_128bit_256B_a(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 6);
			offset = numbytes_div_4 & -64;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 63;
		}
#endif
	}
	return returnval;
} // END MEMSET LARGE, 4B, ALIGNED

// Set arbitrarily large amounts of 4-byte values
// numbytes_div_4 is total number of bytes / 4 (since this is 4 bytes at a time)
// Aligned, streaming version
void * memset_large_4B_as(void * dest, const uint32_t val, size_t numbytes_div_4)
{
	void * returnval = dest; // Memset is supposed to return the initial destination
	size_t offset = 0; // Offset size needs to match the size of a pointer

	while(numbytes_div_4) {
		// Each memset has its own loop.
		if(numbytes_div_4 < 4) { // 4, 8, 12 bytes (the other scalars would need to be memset anyways)
			memset_32bit(dest, val, numbytes_div_4);
			offset = numbytes_div_4;
			dest = (char *)dest + offset;
			numbytes_div_4 = 0;
		}
#ifdef __AVX512F__
		else if(numbytes_div_4 < 8) { // 16 bytes
			memset_128bit_as(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 2);
			offset = numbytes_div_4 & -4;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 3;
		}
		else if(numbytes_div_4 < 16) { // 32 bytes
			memset_256bit_as(dest, _mm256_set1_epi32((int32_t)val), numbytes_div_4 >> 3);
			offset = numbytes_div_4 & -8;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 7;
		}
		else if(numbytes_div_4 < 32) { // 64 bytes
			memset_512bit_as(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 4);
			offset = numbytes_div_4 & -16;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 15;
		}
		else if(numbytes_div_4 < 64) { // 128 bytes
			memset_512bit_128B_as(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 5);
			offset = numbytes_div_4 & -32;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 31;
		}
		else if(numbytes_div_4 < 128) { // 256 bytes
			memset_512bit_256B_as(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 6);
			offset = numbytes_div_4 & -64;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 63;
		}
		else if(numbytes_div_4 < 256) { // 512 bytes
			memset_512bit_512B_as(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 7);
			offset = numbytes_div_4 & -128;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 127;
		}
		else if(numbytes_div_4 < 512) { // 1024 bytes (1 kB)
			memset_512bit_1kB_as(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 8);
			offset = numbytes_div_4 & -256;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 255;
		}
		else if(numbytes_div_4 < 1024) { // 2048 bytes (2 kB)
			memset_512bit_2kB_as(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 9);
			offset = numbytes_div_4 & -512;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 511;
		}
		else { // 4096 bytes (4 kB)
			memset_512bit_4kB_as(dest, _mm512_set1_epi32((int32_t)val), numbytes_div_4 >> 10);
			offset = numbytes_div_4 & -1024;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 1023;
		}
#elif __AVX__
		else if(numbytes_div_4 < 8) { // 16 bytes
			memset_128bit_as(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 2);
			offset = numbytes_div_4 & -4;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 3;
		}
		else if(numbytes_div_4 < 16) { // 32 bytes
			memset_256bit_as(dest, _mm256_set1_epi32((int32_t)val), numbytes_div_4 >> 3);
			offset = numbytes_div_4 & -8;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 7;
		}
		else if(numbytes_div_4 < 32) { // 64 bytes
			memset_256bit_64B_as(dest, _mm256_set1_epi32((int32_t)val), numbytes_div_4 >> 4);
			offset = numbytes_div_4 & -16;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 15;
		}
		else if(numbytes_div_4 < 64) { // 128 bytes
			memset_256bit_128B_as(dest, _mm256_set1_epi32((int32_t)val), numbytes_div_4 >> 5);
			offset = numbytes_div_4 & -32;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 31;
		}
		else if(numbytes_div_4 < 128) { // 256 bytes
			memset_256bit_256B_as(dest, _mm256_set1_epi32((int32_t)val), numbytes_div_4 >> 6);
			offset = numbytes_div_4 & -64;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 63;
		}
		else { // 512 bytes
			memset_256bit_512B_as(dest, _mm256_set1_epi32((int32_t)val), numbytes_div_4 >> 7);
			offset = numbytes_div_4 & -128;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 127;
		}
#else // SSE2 only
		else if(numbytes_div_4 < 8) { // 16 bytes
			memset_128bit_as(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 2);
			offset = numbytes_div_4 & -4;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 3;
		}
		else if(numbytes_div_4 < 16) { // 32 bytes
			memset_128bit_32B_as(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 3);
			offset = numbytes_div_4 & -8;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 7;
		}
		else if(numbytes_div_4 < 32) { // 64 bytes
			memset_128bit_64B_as(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 4);
			offset = numbytes_div_4 & -16;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 15;
		}
		else if(numbytes_div_4 < 64) { // 128 bytes
			memset_128bit_128B_as(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 5);
			offset = numbytes_div_4 & -32;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 31;
		}
		else { // 256 bytes
			memset_128bit_256B_as(dest, _mm_set1_epi32((int32_t)val), numbytes_div_4 >> 6);
			offset = numbytes_div_4 & -64;
			dest = (char *)dest + offset;
			numbytes_div_4 &= 63;
		}
#endif
	}
	return returnval;
} // END MEMSET LARGE, 4B, ALIGNED, STREAMING

//-----------------------------------------------------------------------------
// Main Functions:
//-----------------------------------------------------------------------------

// To set values of sizes > 1 byte, call the desired memset functions directly
// instead. A 4-byte version exists below, however.
void * AVX_memset(void * dest, int intVal, size_t numbytes)
{
	const uint8 val = static_cast<uint8>(intVal);
	void * returnval = dest;
	if(((uintptr_t)dest & BYTE_ALIGNMENT) == 0) { // Check alignment
		if(val == 0) {
			if(numbytes > CACHESIZELIMIT) {
				memset_zeroes_as(dest, numbytes);
			}
			else {
				memset_zeroes_a(dest, numbytes);
			}
		}
		else {
			if(numbytes > CACHESIZELIMIT) {
				memset_large_as(dest, val, numbytes);
			}
			else {
				memset_large_a(dest, val, numbytes);
			}
		}
	}
	else {
		size_t numbytes_to_align = (BYTE_ALIGNMENT + 1) - ((uintptr_t)dest & BYTE_ALIGNMENT);
		void * destoffset = (char*)dest + numbytes_to_align;
		if(val == 0) {
			if(numbytes > numbytes_to_align) {
				// Get to an aligned position.
				// This may be a little slower, but since it'll be mostly scalar operations
				// alignment doesn't matter. Worst case it uses two vector functions, and
				// this process only needs to be done once per call if dest is unaligned.
				memset_zeroes(dest, numbytes_to_align);
				// Now this should be near the fastest possible since stores are aligned.
				if((numbytes - numbytes_to_align) > CACHESIZELIMIT) {
					memset_zeroes_as(destoffset, numbytes - numbytes_to_align);
				}
				else {
					memset_zeroes_a(destoffset, numbytes - numbytes_to_align);
				}
			}
			else { // Small size
				memset_zeroes(dest, numbytes);
			}
		}
		else {
			if(numbytes > numbytes_to_align) {
				// Get to an aligned position.
				// This may be a little slower, but since it'll be mostly scalar operations
				// alignment doesn't matter. Worst case it uses two vector functions, and
				// this process only needs to be done once per call if dest is unaligned.
				memset_large(dest, val, numbytes_to_align);
				// Now this should be near the fastest possible since stores are aligned.
				if((numbytes - numbytes_to_align) > CACHESIZELIMIT) {
					memset_large_as(destoffset, val, numbytes - numbytes_to_align);
				}
				else {
					memset_large_a(destoffset, val, numbytes - numbytes_to_align);
				}
			}
			else { // Small size
				memset_large(dest, val, numbytes);
			}
		}
	}
	return returnval;
}
//
// Set 4 bytes at a time, mainly for 32-bit framebuffers.
// Only use this if you know your set size is aways going to be a multiple of
// 4 bytes, for example in a video framebuffer where 4 bytes is one pixel.
// Numbytes_div_4 is total number of bytes / 4.
// Also, the destination address can, at worst, only be misaligned from the
// cacheline by a value that is a multiple of 4 bytes.
void * AVX_memset_4B(void * dest, const uint32_t val, size_t numbytes_div_4)
{
	void * returnval = dest;
	if( ((uintptr_t)dest & BYTE_ALIGNMENT) == 0) { // Check alignment
		if((numbytes_div_4 * 4) > CACHESIZELIMIT) {
			memset_large_4B_as(dest, val, numbytes_div_4);
		}
		else {
			memset_large_4B_a(dest, val, numbytes_div_4);
		}
	}
	else {
		size_t numbytes_to_align = (BYTE_ALIGNMENT + 1) - ((uintptr_t)dest & BYTE_ALIGNMENT);
		if(numbytes_to_align & 0x03) { // Sanity check, return NULL if not alignable in 4B increments
			return NULL;
		}
		void * destoffset = (char*)dest + numbytes_to_align;
		if(numbytes_div_4 > (numbytes_to_align >> 2)) {
			// Get to an aligned position.
			// This process only needs to be done once per call if dest is unaligned.
			memset_large_4B(dest, val, numbytes_to_align >> 2);
			// Now this should be near the fastest possible since stores are aligned.
			// ...and in memset there are only stores.
			if((numbytes_div_4 * 4 - numbytes_to_align) > CACHESIZELIMIT) {
				memset_large_4B_as(destoffset, val, numbytes_div_4 - (numbytes_to_align >> 2));
			}
			else {
				memset_large_4B_a(destoffset, val, numbytes_div_4 - (numbytes_to_align >> 2));
			}
		}
		else { // Small size
			memset_large_4B(dest, val, numbytes_div_4);
		}
	}
	return returnval;
}

// AVX-1024+ support pending existence of the standard.
