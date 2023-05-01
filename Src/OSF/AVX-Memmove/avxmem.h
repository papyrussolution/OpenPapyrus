//==================================================================================================================================
//  AVX Memory Functions: Main Header
//==================================================================================================================================
//
// Version 1.35
//
// Author:
//  KNNSpeed
//
// Source Code:
//  https://github.com/KNNSpeed/AVX-Memmove
//
// Minimum requirement:
//  x86_64 CPU with SSE4.2, but AVX2 or later is *highly* recommended
//
// This file provides function prototypes for AVX_memmove, AVX_memcpy, AVX_memset, and AVX_memcmp
//
// NOTE: If you need to move/copy memory between overlapping regions, use AVX_memmove instead of AVX_memcpy.
// AVX_memcpy does contain a redirect to AVX_memmove if an overlapping region is found, but it is disabled by default
// since it adds extra latency that really can be avoided by using AVX_memmove directly.
//

#ifndef _avxmem_H
#define _avxmem_H

#include <stddef.h>
#include <stdint.h>
//#include <x86intrin.h>
#include <emmintrin.h>

// Size limit (in bytes) before switching to non-temporal/streaming loads & stores
// Applies to: AVX_memmove, AVX_memset, and AVX_memcpy
#define CACHESIZELIMIT 3*1024*1024 // 3 MB

//-----------------------------------------------------------------------------
// Main Functions:
//-----------------------------------------------------------------------------

// Calling the individual subfunctions directly is also OK. That's why this header is so huge!

void * AVX_memmove(void * dest, void * src, size_t numbytes);
void * AVX_memcpy(void * dest, void * src, size_t numbytes);
void * AVX_memset(void * dest, const uint8_t val, size_t numbytes);
int AVX_memcmp(const void * str1, const void * str2, size_t numbytes, int equality);

// Numbytes_div_4 is total number of bytes / 4 (since they only do 4 at a time).
void * AVX_memset_4B(void * dest, const uint32_t val, size_t numbytes_div_4);

//-----------------------------------------------------------------------------
// MEMSET:
//-----------------------------------------------------------------------------

void * memset_large(void * dest, const uint8_t val, size_t numbytes);
void * memset_large_a(void * dest, const uint8_t val, size_t numbytes);
void * memset_large_as(void * dest, const uint8_t val, size_t numbytes);

void * memset_zeroes(void * dest, size_t numbytes);
void * memset_zeroes_a(void * dest, size_t numbytes);
void * memset_zeroes_as(void * dest, size_t numbytes);

void * memset_large_4B(void * dest, const uint32_t val, size_t numbytes_div_4);
void * memset_large_4B_a(void * dest, const uint32_t val, size_t numbytes_div_4);
void * memset_large_4B_as(void * dest, const uint32_t val, size_t numbytes_div_4);

// Scalar
void * memset(void * dest, uint8_t val, size_t len); // 1 byte
void * memset_16bit(void * dest, uint16_t val, size_t len); // 2 bytes
void * memset_32bit(void * dest, uint32_t val, size_t len); // 4 bytes
void * memset_64bit(void * dest, uint64_t val, size_t len); // 8 bytes

// SSE2 (Unaligned)
void * memset_128bit_u(void * dest, __m128i val, size_t len); // 16 bytes
void * memset_128bit_32B_u(void * dest, __m128i val, size_t len); // 32 bytes
void * memset_128bit_64B_u(void * dest, __m128i val, size_t len); // 64 bytes
void * memset_128bit_128B_u(void * dest, __m128i val, size_t len); // 128 bytes
void * memset_128bit_256B_u(void * dest, __m128i val, size_t len); // 256 bytes

// SSE2 (Aligned)
void * memset_128bit_a(void * dest, __m128i val, size_t len); // 16 bytes
void * memset_128bit_32B_a(void * dest, __m128i val, size_t len); // 32 bytes
void * memset_128bit_64B_a(void * dest, __m128i val, size_t len); // 64 bytes
void * memset_128bit_128B_a(void * dest, __m128i val, size_t len); // 128 bytes
void * memset_128bit_256B_a(void * dest, __m128i val, size_t len); // 256 bytes

//SSE2 (Aligned, streaming)
void * memset_128bit_as(void * dest, __m128i val, size_t len); // 16 bytes
void * memset_128bit_32B_as(void * dest, __m128i val, size_t len); // 32 bytes
void * memset_128bit_64B_as(void * dest, __m128i val, size_t len); // 64 bytes
void * memset_128bit_128B_as(void * dest, __m128i val, size_t len); // 128 bytes
void * memset_128bit_256B_as(void * dest, __m128i val, size_t len); // 256 bytes

// AVX
#ifdef __AVX__
// Unaligned
void * memset_256bit_u(void * dest, __m256i_u val, size_t len); // 32 bytes
void * memset_256bit_64B_u(void * dest, __m256i_u val, size_t len); // 64 bytes
void * memset_256bit_128B_u(void * dest, __m256i_u val, size_t len); // 128 bytes
void * memset_256bit_256B_u(void * dest, __m256i_u val, size_t len); // 256 bytes
void * memset_256bit_512B_u(void * dest, __m256i_u val, size_t len); // 512 bytes

// Aligned
void * memset_256bit_a(void * dest, __m256i_u val, size_t len); // 32 bytes
void * memset_256bit_64B_a(void * dest, __m256i_u val, size_t len); // 64 bytes
void * memset_256bit_128B_a(void * dest, __m256i_u val, size_t len); // 128 bytes
void * memset_256bit_256B_a(void * dest, __m256i_u val, size_t len); // 256 bytes
void * memset_256bit_512B_a(void * dest, __m256i_u val, size_t len); // 512 bytes

// Aligned, Streaming
void * memset_256bit_as(void * dest, __m256i_u val, size_t len); // 32 bytes
void * memset_256bit_64B_as(void * dest, __m256i_u val, size_t len); // 64 bytes
void * memset_256bit_128B_as(void * dest, __m256i_u val, size_t len); // 128 bytes
void * memset_256bit_256B_as(void * dest, __m256i_u val, size_t len); // 256 bytes
void * memset_256bit_512B_as(void * dest, __m256i_u val, size_t len); // 512 bytes
#endif

// AVX512
#ifdef __AVX512F__
// Unaligned
void * memset_512bit_u(void * dest, __m512i_u val, size_t len); // 64 bytes
void * memset_512bit_128B_u(void * dest, __m512i_u val, size_t len); // 128 bytes
void * memset_512bit_256B_u(void * dest, __m512i_u val, size_t len); // 256 bytes
void * memset_512bit_512B_u(void * dest, __m512i_u val, size_t len); // 512 bytes
void * memset_512bit_1kB_u(void * dest, __m512i_u val, size_t len); // 1024 bytes
void * memset_512bit_2kB_u(void * dest, __m512i_u val, size_t len); // 2048 bytes
void * memset_512bit_4kB_u(void * dest, __m512i_u val, size_t len); // 4096 bytes
// Yes that's a whole page. AVX-512 maxes at 2 kB stored at one time in its registers, though.

// Aligned
void * memset_512bit_a(void * dest, __m512i_u val, size_t len); // 64 bytes
void * memset_512bit_128B_a(void * dest, __m512i_u val, size_t len); // 128 bytes
void * memset_512bit_256B_a(void * dest, __m512i_u val, size_t len); // 256 bytes
void * memset_512bit_512B_a(void * dest, __m512i_u val, size_t len); // 512 bytes
void * memset_512bit_1kB_a(void * dest, __m512i_u val, size_t len); // 1024 bytes
void * memset_512bit_2kB_a(void * dest, __m512i_u val, size_t len); // 2048 bytes
void * memset_512bit_4kB_a(void * dest, __m512i_u val, size_t len); // 4096 bytes

// Aligned, Streaming
void * memset_512bit_as(void * dest, __m512i_u val, size_t len); // 64 bytes
void * memset_512bit_128B_as(void * dest, __m512i_u val, size_t len); // 128 bytes
void * memset_512bit_256B_as(void * dest, __m512i_u val, size_t len); // 256 bytes
void * memset_512bit_512B_as(void * dest, __m512i_u val, size_t len); // 512 bytes
void * memset_512bit_1kB_as(void * dest, __m512i_u val, size_t len); // 1024 bytes
void * memset_512bit_2kB_as(void * dest, __m512i_u val, size_t len); // 2048 bytes
void * memset_512bit_4kB_as(void * dest, __m512i_u val, size_t len); // 4096 bytes
#endif
// END MEMSET

//-----------------------------------------------------------------------------
// MEMMOVE:
//-----------------------------------------------------------------------------

//
// The following also applies to memcpy:
//
// Len: Can be thought of as number of times to run the loop in each function
// (i.e. the quantity of that function's # of bytes, like 512 bytes for the 512B
// ones. Giving memmove_512bit_512B a Len of 4 means "move 2 kB.")
// numbytes: Total number of bytes
//
// _a functions require source & destination addresses to be aligned according to
// their x-bit in the function name. E.g. memmove_256bit_64B_a needs to be 32-byte
// aligned (256/8 = 32). The functions will crash/raise an exception otherwise.
//

void * memmove_large(void * dest, void * src, size_t numbytes);
void * memmove_large_a(void * dest, void * src, size_t numbytes);
void * memmove_large_as(void * dest, void * src, size_t numbytes);

void * memmove_large_reverse(void * dest, void * src, size_t numbytes);
void * memmove_large_reverse_a(void * dest, void * src, size_t numbytes);
void * memmove_large_reverse_as(void * dest, void * src, size_t numbytes);

// Scalar
void * memmove(void * dest, const void * src, size_t len); // 1 byte
void * memmove_16bit(void * dest, const void * src, size_t len); // 2 bytes
void * memmove_32bit(void * dest, const void * src, size_t len); // 4 bytes
void * memmove_64bit(void * dest, const void * src, size_t len); // 8 bytes

// SSE2 (Unaligned)
void * memmove_128bit_u(void * dest, const void * src, size_t len); // 16 bytes
void * memmove_128bit_32B_u(void * dest, const void * src, size_t len); // 32 bytes
void * memmove_128bit_64B_u(void * dest, const void * src, size_t len); // 64 bytes
void * memmove_128bit_128B_u(void * dest, const void * src, size_t len); // 128 bytes
void * memmove_128bit_256B_u(void * dest, const void * src, size_t len); // 256 bytes

// SSE2 (Aligned)
void * memmove_128bit_a(void * dest, const void * src, size_t len); // 16 bytes
void * memmove_128bit_32B_a(void * dest, const void * src, size_t len); // 32 bytes
void * memmove_128bit_64B_a(void * dest, const void * src, size_t len); // 64 bytes
void * memmove_128bit_128B_a(void * dest, const void * src, size_t len); // 128 bytes
void * memmove_128bit_256B_a(void * dest, const void * src, size_t len); // 256 bytes

// SSE4.1 (Aligned, Streaming)
void * memmove_128bit_as(void * dest, const void * src, size_t len); // 16 bytes
void * memmove_128bit_32B_as(void * dest, const void * src, size_t len); // 32 bytes
void * memmove_128bit_64B_as(void * dest, const void * src, size_t len); // 64 bytes
void * memmove_128bit_128B_as(void * dest, const void * src, size_t len); // 128 bytes
void * memmove_128bit_256B_as(void * dest, const void * src, size_t len); // 256 bytes

// AVX
#ifdef __AVX__
// Unaligned
void * memmove_256bit_u(void * dest, const void * src, size_t len); // 32 bytes
void * memmove_256bit_64B_u(void * dest, const void * src, size_t len); // 64 bytes
void * memmove_256bit_128B_u(void * dest, const void * src, size_t len); // 128 bytes
void * memmove_256bit_256B_u(void * dest, const void * src, size_t len); // 256 bytes
void * memmove_256bit_512B_u(void * dest, const void * src, size_t len); // 512 bytes

// Aligned
void * memmove_256bit_a(void * dest, const void * src, size_t len); // 32 bytes
void * memmove_256bit_64B_a(void * dest, const void * src, size_t len); // 64 bytes
void * memmove_256bit_128B_a(void * dest, const void * src, size_t len); // 128 bytes
void * memmove_256bit_256B_a(void * dest, const void * src, size_t len); // 256 bytes
void * memmove_256bit_512B_a(void * dest, const void * src, size_t len); // 512 bytes

// Aligned, Streaming
#ifdef __AVX2__
void * memmove_256bit_as(void * dest, const void * src, size_t len); // 32 bytes
void * memmove_256bit_64B_as(void * dest, const void * src, size_t len); // 64 bytes
void * memmove_256bit_128B_as(void * dest, const void * src, size_t len); // 128 bytes
void * memmove_256bit_256B_as(void * dest, const void * src, size_t len); // 256 bytes
void * memmove_256bit_512B_as(void * dest, const void * src, size_t len); // 512 bytes
#endif
#endif

// AVX512
#ifdef __AVX512F__
// Unaligned
void * memmove_512bit_u(void * dest, const void * src, size_t len); // 64 bytes
void * memmove_512bit_128B_u(void * dest, const void * src, size_t len); // 128 bytes
void * memmove_512bit_256B_u(void * dest, const void * src, size_t len); // 256 bytes
void * memmove_512bit_512B_u(void * dest, const void * src, size_t len); // 512 bytes
void * memmove_512bit_1kB_u(void * dest, const void * src, size_t len); // 1024 bytes
void * memmove_512bit_2kB_u(void * dest, const void * src, size_t len); // 2048 bytes
void * memmove_512bit_4kB_u(void * dest, const void * src, size_t len); // 4096 bytes
// Yes that's a whole page. AVX-512 maxes at 2 kB stored at one time in its registers, though.

// Aligned
void * memmove_512bit_a(void * dest, const void * src, size_t len); // 64 bytes
void * memmove_512bit_128B_a(void * dest, const void * src, size_t len); // 128 bytes
void * memmove_512bit_256B_a(void * dest, const void * src, size_t len); // 256 bytes
void * memmove_512bit_512B_a(void * dest, const void * src, size_t len); // 512 bytes
void * memmove_512bit_1kB_a(void * dest, const void * src, size_t len); // 1024 bytes
void * memmove_512bit_2kB_a(void * dest, const void * src, size_t len); // 2048 bytes
void * memmove_512bit_4kB_a(void * dest, const void * src, size_t len); // 4096 bytes

// Aligned, Streaming
void * memmove_512bit_as(void * dest, const void * src, size_t len); // 64 bytes
void * memmove_512bit_128B_as(void * dest, const void * src, size_t len); // 128 bytes
void * memmove_512bit_256B_as(void * dest, const void * src, size_t len); // 256 bytes
void * memmove_512bit_512B_as(void * dest, const void * src, size_t len); // 512 bytes
void * memmove_512bit_1kB_as(void * dest, const void * src, size_t len); // 1024 bytes
void * memmove_512bit_2kB_as(void * dest, const void * src, size_t len); // 2048 bytes
void * memmove_512bit_4kB_as(void * dest, const void * src, size_t len); // 4096 bytes
#endif
// END MEMMOVE

//-----------------------------------------------------------------------------
// MEMCPY:
//-----------------------------------------------------------------------------

void * memcpy_large(void * dest, void * src, size_t numbytes);
void * memcpy_large_a(void * dest, void * src, size_t numbytes);
void * memcpy_large_as(void * dest, void * src, size_t numbytes);

// Scalar
void * memcpy(void * dest, const void * src, size_t len); // 1 byte
void * memcpy_16bit(void * dest, const void * src, size_t len); // 2 bytes
void * memcpy_32bit(void * dest, const void * src, size_t len); // 4 bytes
void * memcpy_64bit(void * dest, const void * src, size_t len); // 8 bytes

// SSE2 (Unaligned)
void * memcpy_128bit_u(void * dest, const void * src, size_t len); // 16 bytes
void * memcpy_128bit_32B_u(void * dest, const void * src, size_t len); // 32 bytes
void * memcpy_128bit_64B_u(void * dest, const void * src, size_t len); // 64 bytes
void * memcpy_128bit_128B_u(void * dest, const void * src, size_t len); // 128 bytes
void * memcpy_128bit_256B_u(void * dest, const void * src, size_t len); // 256 bytes

// SSE2 (aligned)
void * memcpy_128bit_a(void * dest, const void * src, size_t len); // 16 bytes
void * memcpy_128bit_32B_a(void * dest, const void * src, size_t len); // 32 bytes
void * memcpy_128bit_64B_a(void * dest, const void * src, size_t len); // 64 bytes
void * memcpy_128bit_128B_a(void * dest, const void * src, size_t len); // 128 bytes
void * memcpy_128bit_256B_a(void * dest, const void * src, size_t len); // 256 bytes

// SSE4.1 (Aligned, Streaming)
void * memcpy_128bit_as(void * dest, const void * src, size_t len); // 16 bytes
void * memcpy_128bit_32B_as(void * dest, const void * src, size_t len); // 32 bytes
void * memcpy_128bit_64B_as(void * dest, const void * src, size_t len); // 64 bytes
void * memcpy_128bit_128B_as(void * dest, const void * src, size_t len); // 128 bytes
void * memcpy_128bit_256B_as(void * dest, const void * src, size_t len); // 256 bytes

// AVX
#ifdef __AVX__
// Unaligned
void * memcpy_256bit_u(void * dest, const void * src, size_t len); // 32 bytes
void * memcpy_256bit_64B_u(void * dest, const void * src, size_t len); // 64 bytes
void * memcpy_256bit_128B_u(void * dest, const void * src, size_t len); // 128 bytes
void * memcpy_256bit_256B_u(void * dest, const void * src, size_t len); // 256 bytes
void * memcpy_256bit_512B_u(void * dest, const void * src, size_t len); // 512 bytes

// Aligned
void * memcpy_256bit_a(void * dest, const void * src, size_t len); // 32 bytes
void * memcpy_256bit_64B_a(void * dest, const void * src, size_t len); // 64 bytes
void * memcpy_256bit_128B_a(void * dest, const void * src, size_t len); // 128 bytes
void * memcpy_256bit_256B_a(void * dest, const void * src, size_t len); // 256 bytes
void * memcpy_256bit_512B_a(void * dest, const void * src, size_t len); // 512 bytes

// Aligned, Streaming
#ifdef __AVX2__
void * memcpy_256bit_as(void * dest, const void * src, size_t len); // 32 bytes
void * memcpy_256bit_64B_as(void * dest, const void * src, size_t len); // 64 bytes
void * memcpy_256bit_128B_as(void * dest, const void * src, size_t len); // 128 bytes
void * memcpy_256bit_256B_as(void * dest, const void * src, size_t len); // 256 bytes
void * memcpy_256bit_512B_as(void * dest, const void * src, size_t len); // 512 bytes
#endif
#endif

// AVX512
#ifdef __AVX512F__
// Unaligned
void * memcpy_512bit_u(void * dest, const void * src, size_t len); // 64 bytes
void * memcpy_512bit_128B_u(void * dest, const void * src, size_t len); // 128 bytes
void * memcpy_512bit_256B_u(void * dest, const void * src, size_t len); // 256 bytes
void * memcpy_512bit_512B_u(void * dest, const void * src, size_t len); // 512 bytes
void * memcpy_512bit_1kB_u(void * dest, const void * src, size_t len); // 1024 bytes
void * memcpy_512bit_2kB_u(void * dest, const void * src, size_t len); // 2048 bytes
void * memcpy_512bit_4kB_u(void * dest, const void * src, size_t len); // 4096 bytes
// Yes that's a whole page. AVX-512 maxes at 2 kB stored at one time in its registers, though.

// Aligned
void * memcpy_512bit_a(void * dest, const void * src, size_t len); // 64 bytes
void * memcpy_512bit_128B_a(void * dest, const void * src, size_t len); // 128 bytes
void * memcpy_512bit_256B_a(void * dest, const void * src, size_t len); // 256 bytes
void * memcpy_512bit_512B_a(void * dest, const void * src, size_t len); // 512 bytes
void * memcpy_512bit_1kB_a(void * dest, const void * src, size_t len); // 1024 bytes
void * memcpy_512bit_2kB_a(void * dest, const void * src, size_t len); // 2048 bytes
void * memcpy_512bit_4kB_a(void * dest, const void * src, size_t len); // 4096 bytes

// Aligned, Streaming
void * memcpy_512bit_as(void * dest, const void * src, size_t len); // 64 bytes
void * memcpy_512bit_128B_as(void * dest, const void * src, size_t len); // 128 bytes
void * memcpy_512bit_256B_as(void * dest, const void * src, size_t len); // 256 bytes
void * memcpy_512bit_512B_as(void * dest, const void * src, size_t len); // 512 bytes
void * memcpy_512bit_1kB_as(void * dest, const void * src, size_t len); // 1024 bytes
void * memcpy_512bit_2kB_as(void * dest, const void * src, size_t len); // 2048 bytes
void * memcpy_512bit_4kB_as(void * dest, const void * src, size_t len); // 4096 bytes
#endif
// END MEMCPY

//-----------------------------------------------------------------------------
// MEMCMP:
//-----------------------------------------------------------------------------

int memcmp_large(const void * str1, const void * str2, size_t numbytes);
int memcmp_large_eq(const void * str1, const void * str2, size_t numbytes);

int memcmp_large_a(const void * str1, const void * str2, size_t numbytes);
int memcmp_large_eq_a(const void * str1, const void * str2, size_t numbytes);

// Scalar
int memcmp_16bit(const void * str1, const void * str2, size_t count);
int memcmp_16bit_eq(const void * str1, const void * str2, size_t count);
int memcmp_32bit(const void * str1, const void * str2, size_t count);
int memcmp_32bit_eq(const void * str1, const void * str2, size_t count);
int memcmp_64bit(const void * str1, const void * str2, size_t count);
int memcmp_64bit_eq(const void * str1, const void * str2, size_t count);

// SSE4.2 (Unaligned)
int memcmp_128bit_u(const void * str1, const void * str2, size_t count);
int memcmp_128bit_eq_u(const void * str1, const void * str2, size_t count);

// SSE4.2 (Aligned)
int memcmp_128bit_a(const void * str1, const void * str2, size_t count);
int memcmp_128bit_eq_a(const void * str1, const void * str2, size_t count);

// AVX2
#ifdef __AVX2__
// Unaligned
int memcmp_256bit_u(const void * str1, const void * str2, size_t count);
int memcmp_256bit_eq_u(const void * str1, const void * str2, size_t count);

// Aligned
int memcmp_256bit_a(const void * str1, const void * str2, size_t count);
int memcmp_256bit_eq_a(const void * str1, const void * str2, size_t count);
#endif

// AVX512
#ifdef __AVX512F__
// Unaligned
int memcmp_512bit_u(const void * str1, const void * str2, size_t count);
int memcmp_512bit_eq_u(const void * str1, const void * str2, size_t count);

// Aligned
int memcmp_512bit_a(const void * str1, const void * str2, size_t count);
int memcmp_512bit_eq_a(const void * str1, const void * str2, size_t count);
#endif
// END MEMCMP

#endif /* _avxmem_H */
