// Compile with GCC -O3 for best performance
// It pretty much entirely negates the need to write these by hand in asm.
#include <slib-internal.h>
#pragma hdrstop
#include <emmintrin.h>
#include "avxmem.h"

// Default (8-bit, 1 byte at a time)
void * memmove(void * dest, const void * src, size_t len)
{
	const char * s = (char *)src;
	char * d = (char *)dest;
	const char * nexts = s + len;
	char * nextd = d + len;
	if(d < s) {
		while(d != nextd) {
			*d++ = *s++;
		}
	}
	else{
		while(nextd != d) {
			*--nextd = *--nexts;
		}
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
//  AVX Memory Functions: AVX Memmove
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
// This file provides a highly optimized version of memmove.
// Overlapping memory regions are supported.
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
// The "len" argument is "# of x bytes to move," e.g. memmove_512bit_u/a needs
// to know "how many multiples of 512 bit (64 bytes) to move." All functions
// with len follow the same pattern, e.g. memmove_512bit_512B_u/a needs to know
// how many multiples of 512 bytes to move, so a len of 4 tells it to move 2kB.
//
// The "numbytes" argument for functions that use it is just the total
// number of bytes to move.
//

// Some microarchitectural information:
//
// Sources:
// https://www.agner.org/optimize/
// https://software.intel.com/en-us/articles/intel-sdm
// http://blog.stuffedcow.net/2014/01/x86-memory-disambiguation/
//
// It looks as though Haswell and up can do 2 simultaneous aligned loads or 1
// unaligned load in 1 cycle. Alignment means the data is at an address that is
// a multiple of the cache line size, and the CPU most easily loads one cache
// line at a time. All AVX-supporting CPUs have a 64-byte cacheline as of Q4 2018.
// The bottleneck here is stores: only 1 store per cycle can be done (there is
// only 1 store port despite 2 load ports). Unaligned loads/stores that cross
// cache line boundaries typically incur relatively significant cycle penalties,
// though Haswell and up fixed that specifically for unaligned loads.
//
// Unaligned loads on Haswell require both load ports, but, since there is only
// one store port, the store port has to do double-duty for stores that cross
// cache line boundaries. So stores should be contained within cache line sizes
// for best performance. For memmove, this also means there's no point in doing
// 2 separate aligned loads simultaneously if only one can be written at a time.
//
// BUT it turns out that's not the whole story. We can do 2 aligned loads to
// ensure that no cycle is wasted. i.e. instead of this (comma = simultaneously):
// load 1 -> store 1, load 2-> store 2, load 3 -> store 3, load 4 -> store 4 etc.
// we can do this with aligned AVX2 loads:
// load 1, load 2 -> store 1, load 3, load 4 -> store 2, load 5, load 6 -> store 3, etc.
// And this is just per core.
//
// For pure memmove, this provides no real improvement, but loops with many
// iterations that require loading two values, doing math on them, and storing a
// single result can see significant throughput gains. Sandy Bridge could perform
// similarly, but in 2 cycles instead of Haswell's 1 and only for the fewer
// 256-bit AVX calculations it had (Haswell can do any size, AVX2 or otherwise).
//
// Skylake-X, with AVX512, extends Haswell's behavior to include 512-bit values.
//
// If an architecture ever adds 2 store ports, the AVX/(VEX-encoded) SSE
// functions in this file will need to be modified to do 2 loads and 2 stores.
//

//-----------------------------------------------------------------------------
// Individual Functions:
//-----------------------------------------------------------------------------

// 16-bit (2 bytes at a time)
// Len is (# of total bytes/2), so it's "# of 16-bits"

void * memmove_16bit(void * dest, const void * src, size_t len)
{
	const uint16_t* s = (uint16_t*)src;
	uint16_t* d = (uint16_t*)dest;

	const uint16_t * nexts = s + len;
	uint16_t * nextd = d + len;

	if(d < s) {
		while(d != nextd) {
			*d++ = *s++;
		}
	}
	else{
		while(nextd != d) {
			*--nextd = *--nexts;
		}
	}
	return dest;
}

// 32-bit (4 bytes at a time - 1 pixel in a 32-bit linear frame buffer)
// Len is (# of total bytes/4), so it's "# of 32-bits"

void * memmove_32bit(void * dest, const void * src, size_t len)
{
	const uint32_t* s = (uint32_t*)src;
	uint32_t* d = (uint32_t*)dest;

	const uint32_t * nexts = s + len;
	uint32_t * nextd = d + len;

	if(d < s) {
		while(d != nextd) {
			*d++ = *s++;
		}
	}
	else{
		while(nextd != d) {
			*--nextd = *--nexts;
		}
	}
	return dest;
}

// 64-bit (8 bytes at a time - 2 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/8), so it's "# of 64-bits"

void * memmove_64bit(void * dest, const void * src, size_t len)
{
	const uint64_t* s = (uint64_t*)src;
	uint64_t* d = (uint64_t*)dest;

	const uint64_t * nexts = s + len;
	uint64_t * nextd = d + len;

	if(d < s) {
		while(d != nextd) {
			*d++ = *s++;
		}
	}
	else{
		while(nextd != d) {
			*--nextd = *--nexts;
		}
	}
	return dest;
}

//-----------------------------------------------------------------------------
// SSE2 Unaligned:
//-----------------------------------------------------------------------------

// SSE2 (128-bit, 16 bytes at a time - 4 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/16), so it's "# of 128-bits"

void * memmove_128bit_u(void * dest, const void * src, size_t len)
{
	const __m128i_u* s = (__m128i_u*)src;
	__m128i_u* d = (__m128i_u*)dest;

	const __m128i_u * nexts = s + len;
	__m128i_u * nextd = d + len;

	if(d < s) {
		while(d != nextd) {
			_mm_storeu_si128(d++, _mm_lddqu_si128(s++));
		}
	}
	else{
		while(nextd != d) {
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts));
		}
	}
	return dest;
}

// 32 bytes at a time
void * memmove_128bit_32B_u(void * dest, const void * src, size_t len)
{
	const __m128i_u* s = (__m128i_u*)src;
	__m128i_u* d = (__m128i_u*)dest;

	const __m128i_u * nexts = s + (len << 1);
	__m128i_u * nextd = d + (len << 1);

	if(d < s) {
		while(d != nextd) {
			_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 1
			_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 2
		}
	}
	else{
		while(nextd != d) {
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 1
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 2
		}
	}
	return dest;
}

// 64 bytes at a time
void * memmove_128bit_64B_u(void * dest, const void * src, size_t len)
{
	const __m128i_u* s = (__m128i_u*)src;
	__m128i_u* d = (__m128i_u*)dest;

	const __m128i_u * nexts = s + (len << 2);
	__m128i_u * nextd = d + (len << 2);

	if(d < s) {
		while(d != nextd) {
			_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 1
			_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 2
			_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 3
			_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 4
		}
	}
	else{
		while(nextd != d) {
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 1
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 2
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 3
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 4
		}
	}
	return dest;
}

// 128 bytes at a time
void * memmove_128bit_128B_u(void * dest, const void * src, size_t len)
{
	const __m128i_u* s = (__m128i_u*)src;
	__m128i_u* d = (__m128i_u*)dest;

	const __m128i_u * nexts = s + (len << 3);
	__m128i_u * nextd = d + (len << 3);

	if(d < s) {
		while(d != nextd) {
			_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 1
			_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 2
			_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 3
			_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 4
			_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 5
			_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 6
			_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 7
			_mm_storeu_si128(d++, _mm_lddqu_si128(s++)); // 8
		}
	}
	else{
		while(nextd != d) {
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 1
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 2
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 3
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 4
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 5
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 6
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 7
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 8
		}
	}
	return dest;
}

// For fun: 1 load->store for every xmm register
// 256 bytes
void * memmove_128bit_256B_u(void * dest, const void * src, size_t len)
{
	const __m128i_u* s = (__m128i_u*)src;
	__m128i_u* d = (__m128i_u*)dest;

	const __m128i_u * nexts = s + (len << 4);
	__m128i_u * nextd = d + (len << 4);

	if(d < s) {
		while(d != nextd) {
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
	}
	else{
		while(nextd != d) {
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 1
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 2
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 3
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 4
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 5
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 6
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 7
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 8
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 9
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 10
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 11
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 12
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 13
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 14
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 15
			_mm_storeu_si128(--nextd, _mm_lddqu_si128(--nexts)); // 16
		}
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

void * memmove_256bit_u(void * dest, const void * src, size_t len)
{
	const __m256i_u* s = (__m256i_u*)src;
	__m256i_u* d = (__m256i_u*)dest;

	const __m256i_u * nexts = s + len;
	__m256i_u * nextd = d + len;

	if(d < s) {
		while(d != nextd) {
			_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++));
		}
	}
	else{
		while(nextd != d) {
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts));
		}
	}
	return dest;
}

// 64 bytes at a time
void * memmove_256bit_64B_u(void * dest, const void * src, size_t len)
{
	const __m256i_u* s = (__m256i_u*)src;
	__m256i_u* d = (__m256i_u*)dest;

	const __m256i_u * nexts = s + (len << 1);
	__m256i_u * nextd = d + (len << 1);

	if(d < s) {
		while(d != nextd) {
			_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 1
			_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 2
		}
	}
	else{
		while(nextd != d) {
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 1
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 2
		}
	}
	return dest;
}

// 128 bytes at a time
void * memmove_256bit_128B_u(void * dest, const void * src, size_t len)
{
	const __m256i_u* s = (__m256i_u*)src;
	__m256i_u* d = (__m256i_u*)dest;

	const __m256i_u * nexts = s + (len << 2);
	__m256i_u * nextd = d + (len << 2);

	if(d < s) {
		while(d != nextd) {
			_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 1
			_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 2
			_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 3
			_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 4
		}
	}
	else{
		while(nextd != d) {
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 1
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 2
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 3
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 4
		}
	}
	return dest;
}

// 256 bytes at a time
void * memmove_256bit_256B_u(void * dest, const void * src, size_t len)
{
	const __m256i_u* s = (__m256i_u*)src;
	__m256i_u* d = (__m256i_u*)dest;

	const __m256i_u * nexts = s + (len << 3);
	__m256i_u * nextd = d + (len << 3);

	if(d < s) {
		while(d != nextd) {
			_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 1
			_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 2
			_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 3
			_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 4
			_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 5
			_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 6
			_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 7
			_mm256_storeu_si256(d++, _mm256_lddqu_si256(s++)); // 8
		}
	}
	else{
		while(nextd != d) {
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 1
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 2
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 3
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 4
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 5
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 6
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 7
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 8
		}
	}
	return dest;
}

// For fun:
// 512 bytes at a time, one load->store for every ymm register (there are 16)
void * memmove_256bit_512B_u(void * dest, const void * src, size_t len)
{
	const __m256i_u* s = (__m256i_u*)src;
	__m256i_u* d = (__m256i_u*)dest;

	const __m256i_u * nexts = s + (len << 4);
	__m256i_u * nextd = d + (len << 4);

	if(d < s) {
		while(d != nextd) {
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
	}
	else{
		while(nextd != d) {
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 1
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 2
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 3
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 4
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 5
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 6
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 7
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 8
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 9
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 10
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 11
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 12
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 13
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 14
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 15
			_mm256_storeu_si256(--nextd, _mm256_lddqu_si256(--nexts)); // 16
		}
	}
	return dest;
}

#endif

// AVX-512 (512-bit, 64 bytes at a time - 16 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/64), so it's "# of 512-bits"
// Requires AVX512F

#ifdef __AVX512F__
void * memmove_512bit_u(void * dest, const void * src, size_t len)
{
	const __m512i_u* s = (__m512i_u*)src;
	__m512i_u* d = (__m512i_u*)dest;

	const __m512i_u * nexts = s + len;
	__m512i_u * nextd = d + len;

	if(d < s) {
		while(d != nextd) {
			_mm512_storeu_si512(d++, _mm512_loadu_si512(s++));
		}
	}
	else{
		while(nextd != d) {
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts));
		}
	}

	return dest;
}

// 128 bytes at a time
void * memmove_512bit_128B_u(void * dest, const void * src, size_t len)
{
	const __m512i_u* s = (__m512i_u*)src;
	__m512i_u* d = (__m512i_u*)dest;

	const __m512i_u * nexts = s + (len << 1);
	__m512i_u * nextd = d + (len << 1);

	if(d < s) {
		while(d != nextd) {
			_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 1
			_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 2
		}
	}
	else{
		while(nextd != d) {
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 1
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 2
		}
	}
	return dest;
}

// 256 bytes at a time
void * memmove_512bit_256B_u(void * dest, const void * src, size_t len)
{
	const __m512i_u* s = (__m512i_u*)src;
	__m512i_u* d = (__m512i_u*)dest;

	const __m512i_u * nexts = s + (len << 2);
	__m512i_u * nextd = d + (len << 2);

	if(d < s) {
		while(d != nextd) {
			_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 1
			_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 2
			_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 3
			_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 4
		}
	}
	else{
		while(nextd != d) {
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 1
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 2
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 3
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 4
		}
	}
	return dest;
}

// 512 bytes (half a KB!!) at a time
void * memmove_512bit_512B_u(void * dest, const void * src, size_t len)
{
	const __m512i_u* s = (__m512i_u*)src;
	__m512i_u* d = (__m512i_u*)dest;

	const __m512i_u * nexts = s + (len << 3);
	__m512i_u * nextd = d + (len << 3);

	if(d < s) {
		while(d != nextd) {
			_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 1
			_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 2
			_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 3
			_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 4
			_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 5
			_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 6
			_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 7
			_mm512_storeu_si512(d++, _mm512_loadu_si512(s++)); // 8
		}
	}
	else{
		while(nextd != d) {
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 1
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 2
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 3
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 4
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 5
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 6
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 7
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 8
		}
	}
	return dest;
}

// Alright I'll admit I got a little carried away...

// 1024 bytes, or 1 kB
void * memmove_512bit_1kB_u(void * dest, const void * src, size_t len)
{
	const __m512i_u* s = (__m512i_u*)src;
	__m512i_u* d = (__m512i_u*)dest;

	const __m512i_u * nexts = s + (len << 4);
	__m512i_u * nextd = d + (len << 4);

	if(d < s) {
		while(d != nextd) {
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
	}
	else{
		while(nextd != d) {
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 1
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 2
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 3
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 4
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 5
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 6
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 7
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 8
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 9
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 10
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 11
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 12
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 13
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 14
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 15
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 16
		}
	}
	return dest;
}

// 2048 bytes, or 2 kB
void * memmove_512bit_2kB_u(void * dest, const void * src, size_t len)
{
	const __m512i_u* s = (__m512i_u*)src;
	__m512i_u* d = (__m512i_u*)dest;

	const __m512i_u * nexts = s + (len << 5);
	__m512i_u * nextd = d + (len << 5);

	if(d < s) {
		while(d != nextd) {
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
	}
	else{
		while(nextd != d) {
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 1
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 2
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 3
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 4
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 5
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 6
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 7
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 8
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 9
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 10
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 11
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 12
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 13
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 14
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 15
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 16
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 17
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 18
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 19
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 20
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 21
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 22
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 23
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 24
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 25
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 26
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 27
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 28
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 29
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 30
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 31
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 32
		}
	}
	return dest;
}

// Y'know what? Here's a whole page.
// 4096 bytes, or 4 kB
void * memmove_512bit_4kB_u(void * dest, const void * src, size_t len)
{
	const __m512i_u* s = (__m512i_u*)src;
	__m512i_u* d = (__m512i_u*)dest;

	const __m512i_u * nexts = s + (len << 6);
	__m512i_u * nextd = d + (len << 6);

	if(d < s) {
		while(d != nextd) {
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
	}
	else{
		while(nextd != d) {
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 1
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 2
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 3
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 4
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 5
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 6
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 7
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 8
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 9
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 10
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 11
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 12
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 13
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 14
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 15
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 16
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 17
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 18
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 19
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 20
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 21
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 22
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 23
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 24
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 25
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 26
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 27
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 28
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 29
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 30
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 31
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 32
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 1
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 2
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 3
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 4
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 5
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 6
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 7
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 8
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 9
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 10
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 11
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 12
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 13
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 14
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 15
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 16
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 17
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 18
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 19
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 20
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 21
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 22
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 23
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 24
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 25
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 26
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 27
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 28
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 29
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 30
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 31
			_mm512_storeu_si512(--nextd, _mm512_loadu_si512(--nexts)); // 32
		}
	}
	return dest;
}

#endif

// AVX-1024 support pending existence of the standard. It would be able to fit
// an entire 4 kB page in its registers at one time. Imagine that!
// (AVX-512 maxes at 2 kB, which is why I only used numbers 1-32 above.)

//-----------------------------------------------------------------------------
// SSE2 Aligned:
//-----------------------------------------------------------------------------

// SSE2 (128-bit, 16 bytes at a time - 4 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/16), so it's "# of 128-bits"

void * memmove_128bit_a(void * dest, const void * src, size_t len)
{
	const __m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	const __m128i * nexts = s + len;
	__m128i * nextd = d + len;

	if(d < s) {
		while(d != nextd) {
			_mm_store_si128(d++, _mm_load_si128(s++));
		}
	}
	else{
		while(nextd != d) {
			_mm_store_si128(--nextd, _mm_load_si128(--nexts));
		}
	}
	return dest;
}

// 32 bytes at a time
void * memmove_128bit_32B_a(void * dest, const void * src, size_t len)
{
	const __m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	const __m128i * nexts = s + (len << 1);
	__m128i * nextd = d + (len << 1);

	if(d < s) {
		while(d != nextd) {
			_mm_store_si128(d++, _mm_load_si128(s++)); // 1
			_mm_store_si128(d++, _mm_load_si128(s++)); // 2
		}
	}
	else{
		while(nextd != d) {
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 1
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 2
		}
	}
	return dest;
}

// 64 bytes at a time
void * memmove_128bit_64B_a(void * dest, const void * src, size_t len)
{
	const __m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	const __m128i * nexts = s + (len << 2);
	__m128i * nextd = d + (len << 2);

	if(d < s) {
		while(d != nextd) {
			_mm_store_si128(d++, _mm_load_si128(s++)); // 1
			_mm_store_si128(d++, _mm_load_si128(s++)); // 2
			_mm_store_si128(d++, _mm_load_si128(s++)); // 3
			_mm_store_si128(d++, _mm_load_si128(s++)); // 4
		}
	}
	else{
		while(nextd != d) {
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 1
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 2
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 3
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 4
		}
	}
	return dest;
}

// 128 bytes at a time
void * memmove_128bit_128B_a(void * dest, const void * src, size_t len)
{
	const __m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	const __m128i * nexts = s + (len << 3);
	__m128i * nextd = d + (len << 3);

	if(d < s) {
		while(d != nextd) {
			_mm_store_si128(d++, _mm_load_si128(s++)); // 1
			_mm_store_si128(d++, _mm_load_si128(s++)); // 2
			_mm_store_si128(d++, _mm_load_si128(s++)); // 3
			_mm_store_si128(d++, _mm_load_si128(s++)); // 4
			_mm_store_si128(d++, _mm_load_si128(s++)); // 5
			_mm_store_si128(d++, _mm_load_si128(s++)); // 6
			_mm_store_si128(d++, _mm_load_si128(s++)); // 7
			_mm_store_si128(d++, _mm_load_si128(s++)); // 8
		}
	}
	else{
		while(nextd != d) {
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 1
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 2
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 3
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 4
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 5
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 6
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 7
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 8
		}
	}
	return dest;
}

// For fun: 1 load->store for every xmm register (there are 16)
// 256 bytes
void * memmove_128bit_256B_a(void * dest, const void * src, size_t len)
{
	const __m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	const __m128i * nexts = s + (len << 4);
	__m128i * nextd = d + (len << 4);

	if(d < s) {
		while(d != nextd) {
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
	}
	else{
		while(nextd != d) {
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 1
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 2
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 3
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 4
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 5
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 6
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 7
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 8
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 9
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 10
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 11
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 12
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 13
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 14
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 15
			_mm_store_si128(--nextd, _mm_load_si128(--nexts)); // 16
		}
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
void * memmove_256bit_a(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	const __m256i * nexts = s + len;
	__m256i * nextd = d + len;

	if(d < s) {
		while(d != nextd) {
			_mm256_store_si256(d++, _mm256_load_si256(s++));
		}
	}
	else{
		while(nextd != d) {
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts));
		}
	}
	return dest;
}

// 64 bytes at a time
void * memmove_256bit_64B_a(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	const __m256i * nexts = s + (len << 1);
	__m256i * nextd = d + (len << 1);

	if(d < s) {
		while(d != nextd) {
			_mm256_store_si256(d++, _mm256_load_si256(s++)); // 1
			_mm256_store_si256(d++, _mm256_load_si256(s++)); // 2
		}
	}
	else{
		while(nextd != d) {
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 1
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 2
		}
	}
	return dest;
}

// 128 bytes at a time
void * memmove_256bit_128B_a(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	const __m256i * nexts = s + (len << 2);
	__m256i * nextd = d + (len << 2);

	if(d < s) {
		while(d != nextd) {
			_mm256_store_si256(d++, _mm256_load_si256(s++)); // 1
			_mm256_store_si256(d++, _mm256_load_si256(s++)); // 2
			_mm256_store_si256(d++, _mm256_load_si256(s++)); // 3
			_mm256_store_si256(d++, _mm256_load_si256(s++)); // 4
		}
	}
	else{
		while(nextd != d) {
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 1
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 2
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 3
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 4
		}
	}
	return dest;
}

// 256 bytes at a time
void * memmove_256bit_256B_a(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	const __m256i * nexts = s + (len << 3);
	__m256i * nextd = d + (len << 3);

	if(d < s) {
		while(d != nextd) {
			_mm256_store_si256(d++, _mm256_load_si256(s++)); // 1
			_mm256_store_si256(d++, _mm256_load_si256(s++)); // 2
			_mm256_store_si256(d++, _mm256_load_si256(s++)); // 3
			_mm256_store_si256(d++, _mm256_load_si256(s++)); // 4
			_mm256_store_si256(d++, _mm256_load_si256(s++)); // 5
			_mm256_store_si256(d++, _mm256_load_si256(s++)); // 6
			_mm256_store_si256(d++, _mm256_load_si256(s++)); // 7
			_mm256_store_si256(d++, _mm256_load_si256(s++)); // 8
		}
	}
	else{
		while(nextd != d) {
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 1
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 2
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 3
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 4
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 5
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 6
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 7
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 8
		}
	}
	return dest;
}

// I just wanted to see what doing one move for every ymm register looks like.
// There are 16 256-bit (ymm) registers.
void * memmove_256bit_512B_a(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	const __m256i * nexts = s + (len << 4);
	__m256i * nextd = d + (len << 4);

	if(d < s) {
		while(d != nextd) {
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
	}
	else{
		while(nextd != d) {
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 1
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 2
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 3
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 4
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 5
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 6
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 7
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 8
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 9
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 10
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 11
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 12
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 13
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 14
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 15
			_mm256_store_si256(--nextd, _mm256_load_si256(--nexts)); // 16
		}
	}
	return dest;
}

#endif

// AVX-512 (512-bit, 64 bytes at a time - 16 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/64), so it's "# of 512-bits"
// Requires AVX512F

#ifdef __AVX512F__
void * memmove_512bit_a(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	const __m512i * nexts = s + len;
	__m512i * nextd = d + len;

	if(d < s) {
		while(d != nextd) {
			_mm512_store_si512(d++, _mm512_load_si512(s++));
		}
	}
	else{
		while(nextd != d) {
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts));
		}
	}
	return dest;
}

// 128 bytes at a time
void * memmove_512bit_128B_a(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	const __m512i * nexts = s + (len << 1);
	__m512i * nextd = d + (len << 1);

	if(d < s) {
		while(d != nextd) {
			_mm512_store_si512(d++, _mm512_load_si512(s++)); // 1
			_mm512_store_si512(d++, _mm512_load_si512(s++)); // 2
		}
	}
	else{
		while(nextd != d) {
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 1
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 2
		}
	}
	return dest;
}

// 256 bytes at a time
void * memmove_512bit_256B_a(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	const __m512i * nexts = s + (len << 2);
	__m512i * nextd = d + (len << 2);

	if(d < s) {
		while(d != nextd) {
			_mm512_store_si512(d++, _mm512_load_si512(s++)); // 1
			_mm512_store_si512(d++, _mm512_load_si512(s++)); // 2
			_mm512_store_si512(d++, _mm512_load_si512(s++)); // 3
			_mm512_store_si512(d++, _mm512_load_si512(s++)); // 4
		}
	}
	else{
		while(nextd != d) {
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 1
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 2
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 3
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 4
		}
	}
	return dest;
}

// 512 bytes (half a KB!!) at a time
void * memmove_512bit_512B_a(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	const __m512i * nexts = s + (len << 3);
	__m512i * nextd = d + (len << 3);

	if(d < s) {
		while(d != nextd) { // Post-increment: use d then increment
			_mm512_store_si512(d++, _mm512_load_si512(s++)); // 1
			_mm512_store_si512(d++, _mm512_load_si512(s++)); // 2
			_mm512_store_si512(d++, _mm512_load_si512(s++)); // 3
			_mm512_store_si512(d++, _mm512_load_si512(s++)); // 4
			_mm512_store_si512(d++, _mm512_load_si512(s++)); // 5
			_mm512_store_si512(d++, _mm512_load_si512(s++)); // 6
			_mm512_store_si512(d++, _mm512_load_si512(s++)); // 7
			_mm512_store_si512(d++, _mm512_load_si512(s++)); // 8
		}
	}
	else{
		while(nextd != d) { // Pre-increment: increment nextd then use
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 1
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 2
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 3
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 4
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 5
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 6
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 7
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 8
		}
	}
	return dest;
}

// The functions below I made just for fun to see what doing one move for every
// zmm register looks like. I think the insanity speaks for itself. :)

// 1024 bytes, or 1 kB
void * memmove_512bit_1kB_a(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	const __m512i * nexts = s + (len << 4);
	__m512i * nextd = d + (len << 4);

	if(d < s) {
		while(d != nextd) {
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
	}
	else{
		while(nextd != d) {
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 1
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 2
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 3
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 4
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 5
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 6
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 7
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 8
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 9
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 10
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 11
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 12
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 13
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 14
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 15
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 16
		}
	}
	return dest;
}

// 2048 bytes, or 2 kB
// AVX512 has 32x 512-bit registers, so......
void * memmove_512bit_2kB_a(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	const __m512i * nexts = s + (len << 5);
	__m512i * nextd = d + (len << 5);

	if(d < s) {
		while(d != nextd) {
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
	}
	else{
		while(nextd != d) {
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 1
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 2
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 3
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 4
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 5
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 6
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 7
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 8
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 9
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 10
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 11
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 12
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 13
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 14
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 15
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 16
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 17
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 18
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 19
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 20
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 21
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 22
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 23
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 24
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 25
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 26
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 27
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 28
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 29
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 30
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 31
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 32
		}
	}
	return dest;
}

// Y'know what? Here's a whole page.
// 4096 bytes, or 4 kB
void * memmove_512bit_4kB_a(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	const __m512i * nexts = s + (len << 6);
	__m512i * nextd = d + (len << 6);

	if(d < s) {
		while(d != nextd) {
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
	}
	else{
		while(nextd != d) {
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 1
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 2
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 3
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 4
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 5
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 6
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 7
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 8
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 9
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 10
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 11
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 12
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 13
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 14
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 15
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 16
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 17
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 18
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 19
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 20
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 21
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 22
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 23
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 24
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 25
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 26
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 27
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 28
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 29
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 30
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 31
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 32
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 1
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 2
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 3
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 4
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 5
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 6
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 7
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 8
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 9
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 10
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 11
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 12
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 13
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 14
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 15
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 16
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 17
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 18
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 19
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 20
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 21
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 22
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 23
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 24
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 25
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 26
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 27
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 28
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 29
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 30
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 31
			_mm512_store_si512(--nextd, _mm512_load_si512(--nexts)); // 32
		}
	}
	return dest;
}

#endif

//-----------------------------------------------------------------------------
// SSE4.1 Streaming:
//-----------------------------------------------------------------------------

// SSE4.1 (128-bit, 16 bytes at a time - 4 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/16), so it's "# of 128-bits"

void * memmove_128bit_as(void * dest, const void * src, size_t len)
{
	__m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	__m128i * nexts = s + len;
	__m128i * nextd = d + len;

	if(d < s) {
		while(d != nextd) {
			_mm_stream_si128(d++, _mm_stream_load_si128(s++));
		}
	}
	else{
		while(nextd != d) {
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts));
		}
	}
	_mm_sfence();

	return dest;
}

// 32 bytes at a time
void * memmove_128bit_32B_as(void * dest, const void * src, size_t len)
{
	__m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	__m128i * nexts = s + (len << 1);
	__m128i * nextd = d + (len << 1);

	if(d < s) {
		while(d != nextd) {
			_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 1
			_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 2
		}
	}
	else{
		while(nextd != d) {
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 1
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 2
		}
	}
	_mm_sfence();

	return dest;
}

// 64 bytes at a time
void * memmove_128bit_64B_as(void * dest, const void * src, size_t len)
{
	__m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	__m128i * nexts = s + (len << 2);
	__m128i * nextd = d + (len << 2);

	if(d < s) {
		while(d != nextd) {
			_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 1
			_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 2
			_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 3
			_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 4
		}
	}
	else{
		while(nextd != d) {
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 1
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 2
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 3
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 4
		}
	}
	_mm_sfence();

	return dest;
}

// 128 bytes at a time
void * memmove_128bit_128B_as(void * dest, const void * src, size_t len)
{
	__m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	__m128i * nexts = s + (len << 3);
	__m128i * nextd = d + (len << 3);

	if(d < s) {
		while(d != nextd) {
			_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 1
			_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 2
			_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 3
			_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 4
			_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 5
			_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 6
			_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 7
			_mm_stream_si128(d++, _mm_stream_load_si128(s++)); // 8
		}
	}
	else{
		while(nextd != d) {
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 1
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 2
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 3
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 4
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 5
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 6
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 7
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 8
		}
	}
	_mm_sfence();

	return dest;
}

// For fun: 1 load->store for every xmm register (there are 16)
// 256 bytes
void * memmove_128bit_256B_as(void * dest, const void * src, size_t len)
{
	__m128i* s = (__m128i*)src;
	__m128i* d = (__m128i*)dest;

	__m128i * nexts = s + (len << 4);
	__m128i * nextd = d + (len << 4);

	if(d < s) {
		while(d != nextd) {
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
	}
	else{
		while(nextd != d) {
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 1
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 2
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 3
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 4
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 5
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 6
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 7
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 8
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 9
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 10
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 11
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 12
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 13
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 14
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 15
			_mm_stream_si128(--nextd, _mm_stream_load_si128(--nexts)); // 16
		}
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
void * memmove_256bit_as(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	const __m256i * nexts = s + len;
	__m256i * nextd = d + len;

	if(d < s) {
		while(d != nextd) {
			_mm256_stream_si256(d++, _mm256_stream_load_si256(s++));
		}
	}
	else{
		while(nextd != d) {
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts));
		}
	}
	_mm_sfence();

	return dest;
}

// 64 bytes at a time
void * memmove_256bit_64B_as(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	const __m256i * nexts = s + (len << 1);
	__m256i * nextd = d + (len << 1);

	if(d < s) {
		while(d != nextd) {
			_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 1
			_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 2
		}
	}
	else{
		while(nextd != d) {
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 1
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 2
		}
	}
	_mm_sfence();

	return dest;
}

// 128 bytes at a time
void * memmove_256bit_128B_as(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	const __m256i * nexts = s + (len << 2);
	__m256i * nextd = d + (len << 2);

	if(d < s) {
		while(d != nextd) {
			_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 1
			_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 2
			_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 3
			_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 4
		}
	}
	else{
		while(nextd != d) {
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 1
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 2
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 3
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 4
		}
	}
	_mm_sfence();

	return dest;
}

// 256 bytes at a time
void * memmove_256bit_256B_as(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	const __m256i * nexts = s + (len << 3);
	__m256i * nextd = d + (len << 3);

	if(d < s) {
		while(d != nextd) {
			_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 1
			_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 2
			_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 3
			_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 4
			_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 5
			_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 6
			_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 7
			_mm256_stream_si256(d++, _mm256_stream_load_si256(s++)); // 8
		}
	}
	else{
		while(nextd != d) {
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 1
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 2
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 3
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 4
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 5
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 6
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 7
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 8
		}
	}
	_mm_sfence();

	return dest;
}

// I just wanted to see what doing one move for every ymm register looks like.
// There are 16 256-bit (ymm) registers.
void * memmove_256bit_512B_as(void * dest, const void * src, size_t len)
{
	const __m256i* s = (__m256i*)src;
	__m256i* d = (__m256i*)dest;

	const __m256i * nexts = s + (len << 4);
	__m256i * nextd = d + (len << 4);

	if(d < s) {
		while(d != nextd) {
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
	}
	else{
		while(nextd != d) {
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 1
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 2
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 3
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 4
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 5
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 6
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 7
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 8
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 9
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 10
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 11
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 12
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 13
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 14
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 15
			_mm256_stream_si256(--nextd, _mm256_stream_load_si256(--nexts)); // 16
		}
	}
	_mm_sfence();

	return dest;
}

#endif

// AVX-512 (512-bit, 64 bytes at a time - 16 pixels in a 32-bit linear frame buffer)
// Len is (# of total bytes/64), so it's "# of 512-bits"
// Requires AVX512F

#ifdef __AVX512F__
void * memmove_512bit_as(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	const __m512i * nexts = s + len;
	__m512i * nextd = d + len;

	if(d < s) {
		while(d != nextd) {
			_mm512_stream_si512(d++, _mm512_stream_load_si512(s++));
		}
	}
	else{
		while(nextd != d) {
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts));
		}
	}
	_mm_sfence();

	return dest;
}

// 128 bytes at a time
void * memmove_512bit_128B_as(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	const __m512i * nexts = s + (len << 1);
	__m512i * nextd = d + (len << 1);

	if(d < s) {
		while(d != nextd) {
			_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 1
			_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 2
		}
	}
	else{
		while(nextd != d) {
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 1
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 2
		}
	}
	_mm_sfence();

	return dest;
}

// 256 bytes at a time
void * memmove_512bit_256B_as(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	const __m512i * nexts = s + (len << 2);
	__m512i * nextd = d + (len << 2);

	if(d < s) {
		while(d != nextd) {
			_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 1
			_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 2
			_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 3
			_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 4
		}
	}
	else{
		while(nextd != d) {
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 1
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 2
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 3
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 4
		}
	}
	_mm_sfence();

	return dest;
}

// 512 bytes (half a KB!!) at a time
void * memmove_512bit_512B_as(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	const __m512i * nexts = s + (len << 3);
	__m512i * nextd = d + (len << 3);

	if(d < s) {
		while(d != nextd) { // Post-increment: use d then increment
			_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 1
			_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 2
			_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 3
			_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 4
			_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 5
			_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 6
			_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 7
			_mm512_stream_si512(d++, _mm512_stream_load_si512(s++)); // 8
		}
	}
	else{
		while(nextd != d) { // Pre-increment: increment nextd then use
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 1
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 2
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 3
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 4
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 5
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 6
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 7
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 8
		}
	}
	_mm_sfence();

	return dest;
}

// The functions below I made just for fun to see what doing one move for every
// zmm register looks like. I think the insanity speaks for itself. :)

// 1024 bytes, or 1 kB
void * memmove_512bit_1kB_as(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	const __m512i * nexts = s + (len << 4);
	__m512i * nextd = d + (len << 4);

	if(d < s) {
		while(d != nextd) {
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
	}
	else{
		while(nextd != d) {
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 1
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 2
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 3
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 4
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 5
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 6
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 7
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 8
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 9
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 10
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 11
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 12
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 13
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 14
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 15
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 16
		}
	}
	_mm_sfence();

	return dest;
}

// 2048 bytes, or 2 kB
// AVX512 has 32x 512-bit registers, so......
void * memmove_512bit_2kB_as(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	const __m512i * nexts = s + (len << 5);
	__m512i * nextd = d + (len << 5);

	if(d < s) {
		while(d != nextd) {
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
	}
	else{
		while(nextd != d) {
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 1
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 2
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 3
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 4
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 5
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 6
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 7
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 8
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 9
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 10
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 11
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 12
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 13
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 14
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 15
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 16
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 17
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 18
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 19
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 20
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 21
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 22
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 23
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 24
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 25
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 26
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 27
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 28
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 29
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 30
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 31
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 32
		}
	}
	_mm_sfence();

	return dest;
}

// Y'know what? Here's a whole page.
// 4096 bytes, or 4 kB
void * memmove_512bit_4kB_as(void * dest, const void * src, size_t len)
{
	const __m512i* s = (__m512i*)src;
	__m512i* d = (__m512i*)dest;

	const __m512i * nexts = s + (len << 6);
	__m512i * nextd = d + (len << 6);

	if(d < s) {
		while(d != nextd) {
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
	}
	else{
		while(nextd != d) {
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 1
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 2
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 3
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 4
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 5
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 6
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 7
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 8
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 9
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 10
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 11
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 12
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 13
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 14
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 15
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 16
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 17
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 18
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 19
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 20
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 21
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 22
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 23
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 24
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 25
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 26
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 27
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 28
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 29
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 30
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 31
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 32
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 1
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 2
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 3
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 4
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 5
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 6
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 7
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 8
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 9
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 10
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 11
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 12
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 13
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 14
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 15
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 16
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 17
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 18
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 19
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 20
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 21
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 22
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 23
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 24
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 25
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 26
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 27
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 28
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 29
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 30
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 31
			_mm512_stream_si512(--nextd, _mm512_stream_load_si512(--nexts)); // 32
		}
	}
	_mm_sfence();

	return dest;
}

#endif

//-----------------------------------------------------------------------------
// Dispatch Functions:
//-----------------------------------------------------------------------------

// Move arbitrarily large amounts of data (dest addr < src addr)
void * memmove_large(void * dest, void * src, size_t numbytes)
{
	void * returnval = dest; // memmove is supposed to return the destination
	size_t offset = 0; // Offset size needs to match the size of a pointer

	while(numbytes) {
		// The biggest sizes will go first for alignment. There's no benefit to using
		// aligned loads over unaligned loads here, so all are unaligned.
		// NOTE: Each memmove has its own loop so that any one can be used individually.
		if(numbytes < 2) { // 1 byte
			memmove(dest, src, numbytes);
			offset = numbytes & -1;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes = 0;
		}
		else if(numbytes < 4) { // 2 bytes
			memmove_16bit(dest, src, numbytes >> 1);
			offset = numbytes & -2;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 1;
		}
		else if(numbytes < 8) { // 4 bytes
			memmove_32bit(dest, src, numbytes >> 2);
			offset = numbytes & -4;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 3;
		}
		else if(numbytes < 16) { // 8 bytes
			memmove_64bit(dest, src, numbytes >> 3);
			offset = numbytes & -8;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 7;
		}
#ifdef __AVX512F__
		else if(numbytes < 32) { // 16 bytes
			memmove_128bit_u(dest, src, numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memmove_256bit_u(dest, src, numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memmove_512bit_u(dest, src, numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memmove_512bit_128B_u(dest, src, numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memmove_512bit_256B_u(dest, src, numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 255;
		}
		else if(numbytes < 1024) { // 512 bytes
			memmove_512bit_512B_u(dest, src, numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 511;
		}
		else if(numbytes < 2048) { // 1024 bytes (1 kB)
			memmove_512bit_1kB_u(dest, src, numbytes >> 10);
			offset = numbytes & -1024;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 1023;
		}
		else if(numbytes < 4096) { // 2048 bytes (2 kB)
			memmove_512bit_2kB_u(dest, src, numbytes >> 11);
			offset = numbytes & -2048;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 2047;
		}
		else{ // 4096 bytes (4 kB)
			memmove_512bit_4kB_u(dest, src, numbytes >> 12);
			offset = numbytes & -4096;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 4095;
		}
#elif __AVX__
		else if(numbytes < 32) { // 16 bytes
			memmove_128bit_u(dest, src, numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memmove_256bit_u(dest, src, numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memmove_256bit_64B_u(dest, src, numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memmove_256bit_128B_u(dest, src, numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memmove_256bit_256B_u(dest, src, numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 255;
		}
		else{ // 512 bytes
			memmove_256bit_512B_u(dest, src, numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 511;
		}
#else // SSE2 only
		else if(numbytes < 32) { // 16 bytes
			memmove_128bit_u(dest, src, numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memmove_128bit_32B_u(dest, src, numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memmove_128bit_64B_u(dest, src, numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memmove_128bit_128B_u(dest, src, numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 127;
		}
		else{ // 256 bytes
			memmove_128bit_256B_u(dest, src, numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 255;
		}
#endif
	}
	return returnval;
} // END MEMMOVE LARGE, UNALIGNED

// Move arbitrarily large amounts of data (dest addr < src addr)
// Aligned version
void * memmove_large_a(void * dest, void * src, size_t numbytes)
{
	void * returnval = dest; // memmove is supposed to return the destination
	size_t offset = 0; // Offset size needs to match the size of a pointer

	while(numbytes) {
		// The biggest sizes will go first for alignment. There's no benefit to using
		// aligned loads over unaligned loads here, so all are unaligned.
		// NOTE: Each memmove has its own loop so that any one can be used individually.
		if(numbytes < 2) { // 1 byte
			memmove(dest, src, numbytes);
			offset = numbytes & -1;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes = 0;
		}
		else if(numbytes < 4) { // 2 bytes
			memmove_16bit(dest, src, numbytes >> 1);
			offset = numbytes & -2;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 1;
		}
		else if(numbytes < 8) { // 4 bytes
			memmove_32bit(dest, src, numbytes >> 2);
			offset = numbytes & -4;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 3;
		}
		else if(numbytes < 16) { // 8 bytes
			memmove_64bit(dest, src, numbytes >> 3);
			offset = numbytes & -8;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 7;
		}
#ifdef __AVX512F__
		else if(numbytes < 32) { // 16 bytes
			memmove_128bit_a(dest, src, numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memmove_256bit_a(dest, src, numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memmove_512bit_a(dest, src, numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memmove_512bit_128B_a(dest, src, numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memmove_512bit_256B_a(dest, src, numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 255;
		}
		else if(numbytes < 1024) { // 512 bytes
			memmove_512bit_512B_a(dest, src, numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 511;
		}
		else if(numbytes < 2048) { // 1024 bytes (1 kB)
			memmove_512bit_1kB_a(dest, src, numbytes >> 10);
			offset = numbytes & -1024;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 1023;
		}
		else if(numbytes < 4096) { // 2048 bytes (2 kB)
			memmove_512bit_2kB_a(dest, src, numbytes >> 11);
			offset = numbytes & -2048;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 2047;
		}
		else{ // 4096 bytes (4 kB)
			memmove_512bit_4kB_a(dest, src, numbytes >> 12);
			offset = numbytes & -4096;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 4095;
		}
#elif __AVX__
		else if(numbytes < 32) { // 16 bytes
			memmove_128bit_a(dest, src, numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memmove_256bit_a(dest, src, numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memmove_256bit_64B_a(dest, src, numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memmove_256bit_128B_a(dest, src, numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memmove_256bit_256B_a(dest, src, numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 255;
		}
		else{ // 512 bytes
			memmove_256bit_512B_a(dest, src, numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 511;
		}
#else // SSE2 only
		else if(numbytes < 32) { // 16 bytes
			memmove_128bit_a(dest, src, numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memmove_128bit_32B_a(dest, src, numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memmove_128bit_64B_a(dest, src, numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memmove_128bit_128B_a(dest, src, numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 127;
		}
		else{ // 256 bytes
			memmove_128bit_256B_a(dest, src, numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 255;
		}
#endif
	}
	return returnval;
} // END MEMMOVE LARGE, ALIGNED

// Move arbitrarily large amounts of data (dest addr < src addr)
// Aligned, streaming version
void * memmove_large_as(void * dest, void * src, size_t numbytes)
{
	void * returnval = dest; // memmove is supposed to return the destination
	size_t offset = 0; // Offset size needs to match the size of a pointer

	while(numbytes) {
		// The biggest sizes will go first for alignment. There's no benefit to using
		// aligned loads over unaligned loads here, so all are unaligned.
		// NOTE: Each memmove has its own loop so that any one can be used individually.
		if(numbytes < 2) { // 1 byte
			memmove(dest, src, numbytes);
			offset = numbytes & -1;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes = 0;
		}
		else if(numbytes < 4) { // 2 bytes
			memmove_16bit(dest, src, numbytes >> 1);
			offset = numbytes & -2;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 1;
		}
		else if(numbytes < 8) { // 4 bytes
			memmove_32bit(dest, src, numbytes >> 2);
			offset = numbytes & -4;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 3;
		}
		else if(numbytes < 16) { // 8 bytes
			memmove_64bit(dest, src, numbytes >> 3);
			offset = numbytes & -8;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 7;
		}
#ifdef __AVX512F__
		else if(numbytes < 32) { // 16 bytes
			memmove_128bit_as(dest, src, numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memmove_256bit_as(dest, src, numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memmove_512bit_as(dest, src, numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memmove_512bit_128B_as(dest, src, numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memmove_512bit_256B_as(dest, src, numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 255;
		}
		else if(numbytes < 1024) { // 512 bytes
			memmove_512bit_512B_as(dest, src, numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 511;
		}
		else if(numbytes < 2048) { // 1024 bytes (1 kB)
			memmove_512bit_1kB_as(dest, src, numbytes >> 10);
			offset = numbytes & -1024;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 1023;
		}
		else if(numbytes < 4096) { // 2048 bytes (2 kB)
			memmove_512bit_2kB_as(dest, src, numbytes >> 11);
			offset = numbytes & -2048;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 2047;
		}
		else{ // 4096 bytes (4 kB)
			memmove_512bit_4kB_as(dest, src, numbytes >> 12);
			offset = numbytes & -4096;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 4095;
		}
#elif __AVX2__
		else if(numbytes < 32) { // 16 bytes
			memmove_128bit_as(dest, src, numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memmove_256bit_as(dest, src, numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memmove_256bit_64B_as(dest, src, numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memmove_256bit_128B_as(dest, src, numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 127;
		}
		else if(numbytes < 512) { // 256 bytes
			memmove_256bit_256B_as(dest, src, numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 255;
		}
		else{ // 512 bytes
			memmove_256bit_512B_as(dest, src, numbytes >> 9);
			offset = numbytes & -512;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 511;
		}
#else // SSE4.1 only
		else if(numbytes < 32) { // 16 bytes
			memmove_128bit_as(dest, src, numbytes >> 4);
			offset = numbytes & -16;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			memmove_128bit_32B_as(dest, src, numbytes >> 5);
			offset = numbytes & -32;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 31;
		}
		else if(numbytes < 128) { // 64 bytes
			memmove_128bit_64B_as(dest, src, numbytes >> 6);
			offset = numbytes & -64;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 63;
		}
		else if(numbytes < 256) { // 128 bytes
			memmove_128bit_128B_as(dest, src, numbytes >> 7);
			offset = numbytes & -128;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 127;
		}
		else{ // 256 bytes
			memmove_128bit_256B_as(dest, src, numbytes >> 8);
			offset = numbytes & -256;
			dest = (char *)dest + offset;
			src = (char *)src + offset;
			numbytes &= 255;
		}
#endif
	}
	return returnval;
} // END MEMMOVE LARGE, ALIGNED, STREAMING

// Move arbitrarily large amounts of data in reverse order (ends first)
// src addr < dest addr
void * memmove_large_reverse(void * dest, void * src, size_t numbytes)
{
	void * returnval = dest; // memmove is supposed to return the destination
	size_t offset = 0; // Offset size needs to match the size of a pointer

	void * nextdest = (char *)dest + numbytes;
	void * nextsrc = (char *)src + numbytes;

	while(numbytes) {
		// Want smallest sizes to go first, at the tail end, so that the biggest sizes
		// are aligned later in this operation (AVX_memmove sets the alignment up for
		// this to work).
		// NOTE: Each memmove has its own loop so that any one can be used individually.
		if(numbytes & 1) { // 1 byte
			offset = numbytes & 1;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove(nextdest, nextsrc, 1);
			numbytes &= -2;
		}
		else if(numbytes & 2) { // 2 bytes
			offset = numbytes & 3;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_16bit(nextdest, nextsrc, 1);
			numbytes &= -4;
		}
		else if(numbytes & 4) { // 4 bytes
			offset = numbytes & 7;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_32bit(nextdest, nextsrc, 1);
			numbytes &= -8;
		}
		else if(numbytes & 8) { // 8 bytes
			offset = numbytes & 15;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_64bit(nextdest, nextsrc, 1);
			numbytes &= -16;
		}
#ifdef __AVX512F__
		else if(numbytes & 16) { // 16 bytes
			offset = numbytes & 31;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_u(nextdest, nextsrc, 1);
			numbytes &= -32;
		}
		else if(numbytes & 32) { // 32 bytes
			offset = numbytes & 63;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_256bit_u(nextdest, nextsrc, 1);
			numbytes &= -64;
		}
		else if(numbytes & 64) { // 64 bytes
			offset = numbytes & 127;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_u(nextdest, nextsrc, 1);
			numbytes &= -128;
		}
		else if(numbytes & 128) { // 128 bytes
			offset = numbytes & 255;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_128B_u(nextdest, nextsrc, 1);
			numbytes &= -256;
		}
		else if(numbytes & 256) { // 256 bytes
			offset = numbytes & 511;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_256B_u(nextdest, nextsrc, 1);
			numbytes &= -512;
		}
		else if(numbytes & 512) { // 512 bytes
			offset = numbytes & 1023;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_512B_u(nextdest, nextsrc, 1);
			numbytes &= -1024;
		}
		else if(numbytes & 1024) { // 1024 bytes (1 kB)
			offset = numbytes & 2047;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_1kB_u(nextdest, nextsrc, 1);
			numbytes &= -2048;
		}
		else if(numbytes & 2048) { // 2048 bytes (2 kB)
			offset = numbytes & 4095;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_2kB_u(nextdest, nextsrc, 1);
			numbytes &= -4096;
		}
		else{ // 4096 bytes (4 kB)
			offset = numbytes;
			nextdest = (char *)nextdest - offset; // These should match initial src/dest
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_4kB_u(nextdest, nextsrc, numbytes >> 12);
			numbytes = 0;
		}
#elif __AVX__
		else if(numbytes & 16) { // 16 bytes
			offset = numbytes & 31;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_u(nextdest, nextsrc, 1);
			numbytes &= -32;
		}
		else if(numbytes & 32) { // 32 bytes
			offset = numbytes & 63;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_256bit_u(nextdest, nextsrc, 1);
			numbytes &= -64;
		}
		else if(numbytes & 64) { // 64 bytes
			offset = numbytes & 127;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_256bit_64B_u(nextdest, nextsrc, 1);
			numbytes &= -128;
		}
		else if(numbytes & 128) { // 128 bytes
			offset = numbytes & 255;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_256bit_128B_u(nextdest, nextsrc, 1);
			numbytes &= -256;
		}
		else if(numbytes & 256) { // 256 bytes
			offset = numbytes & 511;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_256bit_256B_u(nextdest, nextsrc, 1);
			numbytes &= -512;
		}
		else{ // 512 bytes
			offset = numbytes;
			nextdest = (char *)nextdest - offset; // These should match initial src/dest
			nextsrc = (char *)nextsrc - offset;
			memmove_256bit_512B_u(nextdest, nextsrc, numbytes >> 9);
			numbytes = 0;
		}
#else // SSE2 only
		else if(numbytes & 16) { // 16 bytes
			offset = numbytes & 31;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_u(nextdest, nextsrc, 1);
			numbytes &= -32;
		}
		else if(numbytes & 32) { // 32 bytes
			offset = numbytes & 63;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_32B_u(nextdest, nextsrc, 1);
			numbytes &= -64;
		}
		else if(numbytes & 64) { // 64 bytes
			offset = numbytes & 127;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_64B_u(nextdest, nextsrc, 1);
			numbytes &= -128;
		}
		else if(numbytes & 128) {// 128 bytes
			offset = numbytes & 255;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_128B_u(nextdest, nextsrc, 1);
			numbytes &= -256;
		}
		else{ // 256 bytes
			offset = numbytes;
			nextdest = (char *)nextdest - offset; // These should match initial src/dest
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_256B_u(nextdest, nextsrc, numbytes >> 8);
			numbytes = 0;
		}
#endif
	}
	return returnval;
} // END MEMMOVE LARGE REVERSE, UNALIGNED

// Move arbitrarily large amounts of data in reverse order (ends first)
// src addr < dest addr
// Aligned version
void * memmove_large_reverse_a(void * dest, void * src, size_t numbytes)
{
	void * returnval = dest; // memmove is supposed to return the destination
	size_t offset = 0; // Offset size needs to match the size of a pointer

	void * nextdest = (char *)dest + numbytes;
	void * nextsrc = (char *)src + numbytes;

	while(numbytes) {
		// Want smallest sizes to go first, at the tail end, so that the biggest sizes
		// are aligned later in this operation (AVX_memmove sets the alignment up for
		// this to work).
		// NOTE: Each memmove has its own loop so that any one can be used individually.
		if(numbytes & 1) { // 1 byte
			offset = numbytes & 1;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove(nextdest, nextsrc, 1);
			numbytes &= -2;
		}
		else if(numbytes & 2) { // 2 bytes
			offset = numbytes & 3;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_16bit(nextdest, nextsrc, 1);
			numbytes &= -4;
		}
		else if(numbytes & 4) { // 4 bytes
			offset = numbytes & 7;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_32bit(nextdest, nextsrc, 1);
			numbytes &= -8;
		}
		else if(numbytes & 8) { // 8 bytes
			offset = numbytes & 15;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_64bit(nextdest, nextsrc, 1);
			numbytes &= -16;
		}
#ifdef __AVX512F__
		else if(numbytes & 16) { // 16 bytes
			offset = numbytes & 31;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_a(nextdest, nextsrc, 1);
			numbytes &= -32;
		}
		else if(numbytes & 32) { // 32 bytes
			offset = numbytes & 63;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_256bit_a(nextdest, nextsrc, 1);
			numbytes &= -64;
		}
		else if(numbytes & 64) { // 64 bytes
			offset = numbytes & 127;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_a(nextdest, nextsrc, 1);
			numbytes &= -128;
		}
		else if(numbytes & 128) { // 128 bytes
			offset = numbytes & 255;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_128B_a(nextdest, nextsrc, 1);
			numbytes &= -256;
		}
		else if(numbytes & 256) { // 256 bytes
			offset = numbytes & 511;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_256B_a(nextdest, nextsrc, 1);
			numbytes &= -512;
		}
		else if(numbytes & 512) { // 512 bytes
			offset = numbytes & 1023;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_512B_a(nextdest, nextsrc, 1);
			numbytes &= -1024;
		}
		else if(numbytes & 1024) { // 1024 bytes (1 kB)
			offset = numbytes & 2047;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_1kB_a(nextdest, nextsrc, 1);
			numbytes &= -2048;
		}
		else if(numbytes & 2048) { // 2048 bytes (2 kB)
			offset = numbytes & 4095;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_2kB_a(nextdest, nextsrc, 1);
			numbytes &= -4096;
		}
		else{ // 4096 bytes (4 kB)
			offset = numbytes;
			nextdest = (char *)nextdest - offset; // These should match initial src/dest
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_4kB_a(nextdest, nextsrc, numbytes >> 12);
			numbytes = 0;
		}
#elif __AVX__
		else if(numbytes & 16) { // 16 bytes
			offset = numbytes & 31;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_a(nextdest, nextsrc, 1);
			numbytes &= -32;
		}
		else if(numbytes & 32) { // 32 bytes
			offset = numbytes & 63;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_256bit_a(nextdest, nextsrc, 1);
			numbytes &= -64;
		}
		else if(numbytes & 64) { // 64 bytes
			offset = numbytes & 127;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_256bit_64B_a(nextdest, nextsrc, 1);
			numbytes &= -128;
		}
		else if(numbytes & 128) { // 128 bytes
			offset = numbytes & 255;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_256bit_128B_a(nextdest, nextsrc, 1);
			numbytes &= -256;
		}
		else if(numbytes & 256) { // 256 bytes
			offset = numbytes & 511;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_256bit_256B_a(nextdest, nextsrc, 1);
			numbytes &= -512;
		}
		else{ // 512 bytes
			offset = numbytes;
			nextdest = (char *)nextdest - offset; // These should match initial src/dest
			nextsrc = (char *)nextsrc - offset;
			memmove_256bit_512B_a(nextdest, nextsrc, numbytes >> 9);
			numbytes = 0;
		}
#else // SSE2 only
		else if(numbytes & 16) { // 16 bytes
			offset = numbytes & 31;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_a(nextdest, nextsrc, 1);
			numbytes &= -32;
		}
		else if(numbytes & 32) { // 32 bytes
			offset = numbytes & 63;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_32B_a(nextdest, nextsrc, 1);
			numbytes &= -64;
		}
		else if(numbytes & 64) { // 64 bytes
			offset = numbytes & 127;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_64B_a(nextdest, nextsrc, 1);
			numbytes &= -128;
		}
		else if(numbytes & 128) {// 128 bytes
			offset = numbytes & 255;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_128B_a(nextdest, nextsrc, 1);
			numbytes &= -256;
		}
		else{ // 256 bytes
			offset = numbytes;
			nextdest = (char *)nextdest - offset; // These should match initial src/dest
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_256B_a(nextdest, nextsrc, numbytes >> 8);
			numbytes = 0;
		}
#endif
	}
	return returnval;
} // END MEMMOVE LARGE REVERSE, ALIGNED

// Move arbitrarily large amounts of data in reverse order (ends first)
// src addr < dest addr
// Aligned, streaming version
void * memmove_large_reverse_as(void * dest, void * src, size_t numbytes)
{
	void * returnval = dest; // memmove is supposed to return the destination
	size_t offset = 0; // Offset size needs to match the size of a pointer

	void * nextdest = (char *)dest + numbytes;
	void * nextsrc = (char *)src + numbytes;

	while(numbytes) {
		// Want smallest sizes to go first, at the tail end, so that the biggest sizes
		// are aligned later in this operation (AVX_memmove sets the alignment up for
		// this to work).
		// NOTE: Each memmove has its own loop so that any one can be used individually.
		if(numbytes & 1) { // 1 byte
			offset = numbytes & 1;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove(nextdest, nextsrc, 1);
			numbytes &= -2;
		}
		else if(numbytes & 2) { // 2 bytes
			offset = numbytes & 3;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_16bit(nextdest, nextsrc, 1);
			numbytes &= -4;
		}
		else if(numbytes & 4) { // 4 bytes
			offset = numbytes & 7;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_32bit(nextdest, nextsrc, 1);
			numbytes &= -8;
		}
		else if(numbytes & 8) { // 8 bytes
			offset = numbytes & 15;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_64bit(nextdest, nextsrc, 1);
			numbytes &= -16;
		}
#ifdef __AVX512F__
		else if(numbytes & 16) { // 16 bytes
			offset = numbytes & 31;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_as(nextdest, nextsrc, 1);
			numbytes &= -32;
		}
		else if(numbytes & 32) { // 32 bytes
			offset = numbytes & 63;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_256bit_as(nextdest, nextsrc, 1);
			numbytes &= -64;
		}
		else if(numbytes & 64) { // 64 bytes
			offset = numbytes & 127;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_as(nextdest, nextsrc, 1);
			numbytes &= -128;
		}
		else if(numbytes & 128) { // 128 bytes
			offset = numbytes & 255;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_128B_as(nextdest, nextsrc, 1);
			numbytes &= -256;
		}
		else if(numbytes & 256) { // 256 bytes
			offset = numbytes & 511;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_256B_as(nextdest, nextsrc, 1);
			numbytes &= -512;
		}
		else if(numbytes & 512) { // 512 bytes
			offset = numbytes & 1023;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_512B_as(nextdest, nextsrc, 1);
			numbytes &= -1024;
		}
		else if(numbytes & 1024) { // 1024 bytes (1 kB)
			offset = numbytes & 2047;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_1kB_as(nextdest, nextsrc, 1);
			numbytes &= -2048;
		}
		else if(numbytes & 2048) { // 2048 bytes (2 kB)
			offset = numbytes & 4095;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_2kB_as(nextdest, nextsrc, 1);
			numbytes &= -4096;
		}
		else{ // 4096 bytes (4 kB)
			offset = numbytes;
			nextdest = (char *)nextdest - offset; // These should match initial src/dest
			nextsrc = (char *)nextsrc - offset;
			memmove_512bit_4kB_as(nextdest, nextsrc, numbytes >> 12);
			numbytes = 0;
		}
#elif __AVX2__
		else if(numbytes & 16) { // 16 bytes
			offset = numbytes & 31;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_as(nextdest, nextsrc, 1);
			numbytes &= -32;
		}
		else if(numbytes & 32) { // 32 bytes
			offset = numbytes & 63;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_256bit_as(nextdest, nextsrc, 1);
			numbytes &= -64;
		}
		else if(numbytes & 64) { // 64 bytes
			offset = numbytes & 127;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_256bit_64B_as(nextdest, nextsrc, 1);
			numbytes &= -128;
		}
		else if(numbytes & 128) { // 128 bytes
			offset = numbytes & 255;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_256bit_128B_as(nextdest, nextsrc, 1);
			numbytes &= -256;
		}
		else if(numbytes & 256) { // 256 bytes
			offset = numbytes & 511;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_256bit_256B_as(nextdest, nextsrc, 1);
			numbytes &= -512;
		}
		else{ // 512 bytes
			offset = numbytes;
			nextdest = (char *)nextdest - offset; // These should match initial src/dest
			nextsrc = (char *)nextsrc - offset;
			memmove_256bit_512B_as(nextdest, nextsrc, numbytes >> 9);
			numbytes = 0;
		}
#else // SSE4.1 only
		else if(numbytes & 16) { // 16 bytes
			offset = numbytes & 31;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_as(nextdest, nextsrc, 1);
			numbytes &= -32;
		}
		else if(numbytes & 32) { // 32 bytes
			offset = numbytes & 63;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_32B_as(nextdest, nextsrc, 1);
			numbytes &= -64;
		}
		else if(numbytes & 64) { // 64 bytes
			offset = numbytes & 127;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_64B_as(nextdest, nextsrc, 1);
			numbytes &= -128;
		}
		else if(numbytes & 128) {// 128 bytes
			offset = numbytes & 255;
			nextdest = (char *)nextdest - offset;
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_128B_as(nextdest, nextsrc, 1);
			numbytes &= -256;
		}
		else{ // 256 bytes
			offset = numbytes;
			nextdest = (char *)nextdest - offset; // These should match initial src/dest
			nextsrc = (char *)nextsrc - offset;
			memmove_128bit_256B_as(nextdest, nextsrc, numbytes >> 8);
			numbytes = 0;
		}
#endif
	}
	return returnval;
} // END MEMMOVE LARGE REVERSE, ALIGNED, STREAMING

//-----------------------------------------------------------------------------
// Main Function:
//-----------------------------------------------------------------------------

// General-purpose function to call
void * AVX_memmove(void * dest, void * src, size_t numbytes)
{
	void * returnval = dest;

	if((char*)src == (char*)dest) {
		return returnval;
	}

	if(
		( ((uintptr_t)src & BYTE_ALIGNMENT) == 0 )
		&&
		( ((uintptr_t)dest & BYTE_ALIGNMENT) == 0 )
		) { // Check alignment
		if((char *)dest < (char *)src) {
			// This is the fastest case: src and dest are both cache line aligned.
			if(numbytes > CACHESIZELIMIT) {
				memmove_large_as(dest, src, numbytes);
			}
			else{
				memmove_large_a(dest, src, numbytes); // Even if numbytes is small this'll work
			}
		}
		else{ // src < dest
			// Need to move ends first
			if(numbytes > CACHESIZELIMIT) {
				memmove_large_reverse_as(dest, src, numbytes);
			}
			else{
				memmove_large_reverse_a(dest, src, numbytes);
			}
		}
	}
	else{ // Unaligned
		size_t numbytes_to_align = (BYTE_ALIGNMENT + 1) - ((uintptr_t)dest & BYTE_ALIGNMENT);

		void * destoffset = (char*)dest + numbytes_to_align;
		void * srcoffset = (char*)src + numbytes_to_align;

		if((char *)dest < (char *)src) {
			if(numbytes > numbytes_to_align) {
				// Get to an aligned position.
				// This may be a little slower, but since it'll be mostly scalar operations
				// alignment doesn't matter. Worst case it uses two vector functions, and
				// this process only needs to be done once per call if dest is unaligned.
				memmove_large(dest, src, numbytes_to_align);
				// Now this should be faster since stores are aligned.
				memmove_large(destoffset, srcoffset, numbytes - numbytes_to_align); // NOTE: Can't use streaming due to potential src misalignment
				// On Haswell and up, cross cache line loads have a negligible penalty.
				// Thus this will be slower on Sandy & Ivy Bridge, though Ivy Bridge will
				// fare a little better (~2x, maybe?). Ryzen should generally fall somewhere
				// inbetween Sandy Bridge and Haswell/Skylake on that front.
				// NOTE: These are just rough theoretical estimates.
			}
			else{ // Small size
				memmove_large(dest, src, numbytes);
			}
		}
		else{ // src < dest
			if(numbytes > numbytes_to_align) {
				// Move bulk, up to lowest alignment line
				memmove_large_reverse(destoffset, srcoffset, numbytes - numbytes_to_align);
				// Move remainder
				memmove_large_reverse(dest, src, numbytes_to_align);
			}
			else{ // Small size
				memmove_large_reverse(dest, src, numbytes);
			}
		}
	}

	return returnval;
}

// AVX-1024+ support pending existence of the standard.
