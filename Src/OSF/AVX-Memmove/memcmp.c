// Compile with GCC -O3 for best performance
// It pretty much entirely negates the need to write these by hand in asm.

#include <slib-internal.h>
#pragma hdrstop
#include <emmintrin.h>
#include "avxmem.h"

int memcmp(const void * str1, const void * str2, size_t count)
{
	const unsigned char * s1 = (unsigned char *)str1;
	const unsigned char * s2 = (unsigned char *)str2;
	while(count-- > 0) {
		if(*s1++ != *s2++) {
			return s1[-1] < s2[-1] ? -1 : 1;
		}
	}
	return 0;
}

// Equality-only version
int memcmp_eq(const void * str1, const void * str2, size_t count)
{
	const unsigned char * s1 = (unsigned char *)str1;
	const unsigned char * s2 = (unsigned char *)str2;

	while(count-- > 0) {
		if(*s1++ != *s2++) {
			return -1; // Makes more sense to me if -1 means unequal.
		}
	}
	return 0; // Return 0 if equal to match normal memcmp
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
//  AVX Memory Functions: AVX Memcmp
//==============================================================================
//
// Version 1.2
//
// Author:
//  KNNSpeed
//
// Source Code:
//  https://github.com/KNNSpeed/AVX-Memmove
//
// Minimum requirement:
//  x86_64 CPU with SSE4.2, but AVX2 or later is recommended
//
// This file provides a highly optimized version of memcmp.
// It allows for selection of modes, too: "check for equality" or perform the full
// greater-than/less-than comparison. For equality-only, pass 0 to the equality
// argument. Pass 1 for full comparison (or really any nonzero int).
//
// In equality mode, a return value of 0 means equal, -1 means unequal.
// In full comparison mode, -1 -> str1 is less, 0 -> equal, 1 -> str1 is greater.
//

#ifdef __clang__
#define __m128i_u __m128i
#define __m256i_u __m256i
#define __m512i_u __m512i
#define _mm_cvtsi128_si64x _mm_cvtsi128_si64
#define _mm_cvtsi64x_si128 _mm_cvtsi64_si128
#endif

#ifdef __AVX512F__
#define BYTE_ALIGNMENT 0x3F // For 64-byte alignment
#elif __AVX2__
#define BYTE_ALIGNMENT 0x1F // For 32-byte alignment
#else
#define BYTE_ALIGNMENT 0x0F // For 16-byte alignment
#endif

//-----------------------------------------------------------------------------
// Individual Functions:
//-----------------------------------------------------------------------------
//
// The following memcmps return -1 or 1 depending on the sign of the first unit
// of their respective sizes, as opposed to the first byte (it seems memcmp(3)
// is only defined for byte-by-byte comparisons, not, e.g., 16-byte-by-16-byte).
//
// The way these functions are made allows them to work properly even if they
// run off the edge of the desired memory area (e.g. numbytes was larger than the
// desired area for whatever reason). The returned value won't necessarily be
// indicative of the memory area in this case.
//

// 16-bit (2 bytes at a time)
// Count is (# of total bytes/2), so it's "# of 16-bits"

int memcmp_16bit(const void * str1, const void * str2, size_t count)
{
	const uint16_t * s1 = (uint16_t*)str1;
	const uint16_t * s2 = (uint16_t*)str2;

	while(count-- > 0) {
		if(*s1++ != *s2++) {
			return s1[-1] < s2[-1] ? -1 : 1;
		}
	}
	return 0;
}

// Equality-only version
int memcmp_16bit_eq(const void * str1, const void * str2, size_t count)
{
	const uint16_t * s1 = (uint16_t*)str1;
	const uint16_t * s2 = (uint16_t*)str2;

	while(count--) {
		if(*s1++ != *s2++) {
			return -1;
		}
	}
	return 0;
}

// 32-bit (4 bytes at a time - 1 pixel in a 32-bit linear frame buffer)
// Count is (# of total bytes/4), so it's "# of 32-bits"

int memcmp_32bit(const void * str1, const void * str2, size_t count)
{
	const uint32_t * s1 = (uint32_t*)str1;
	const uint32_t * s2 = (uint32_t*)str2;

	while(count--) {
		if(*s1++ != *s2++) {
			return s1[-1] < s2[-1] ? -1 : 1;
		}
	}
	return 0;
}

// Equality-only version
int memcmp_32bit_eq(const void * str1, const void * str2, size_t count)
{
	const uint32_t * s1 = (uint32_t*)str1;
	const uint32_t * s2 = (uint32_t*)str2;

	while(count--) {
		if(*s1++ != *s2++) {
			return -1;
		}
	}
	return 0;
}

// 64-bit (8 bytes at a time - 2 pixels in a 32-bit linear frame buffer)
// Count is (# of total bytes/8), so it's "# of 64-bits"

int memcmp_64bit(const void * str1, const void * str2, size_t count)
{
	const uint64_t * s1 = (uint64_t*)str1;
	const uint64_t * s2 = (uint64_t*)str2;

	while(count--) {
		if(*s1++ != *s2++) {
			return s1[-1] < s2[-1] ? -1 : 1;
		}
	}
	return 0;
}

// Equality-only version
int memcmp_64bit_eq(const void * str1, const void * str2, size_t count)
{
	const uint64_t * s1 = (uint64_t*)str1;
	const uint64_t * s2 = (uint64_t*)str2;

	while(count--) {
		if(*s1++ != *s2++) {
			return -1;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
// SSE4.2 Unaligned:
//-----------------------------------------------------------------------------

// SSE4.2 (128-bit, 16 bytes at a time - 4 pixels in a 32-bit linear frame buffer)
// Count is (# of total bytes/16), so it's "# of 128-bits"

int memcmp_128bit_u(const void * str1, const void * str2, size_t count)
{
	const __m128i_u * s1 = (__m128i_u*)str1;
	const __m128i_u * s2 = (__m128i_u*)str2;

	while(count--) {
		__m128i item1 = _mm_lddqu_si128(s1++);
		__m128i item2 = _mm_lddqu_si128(s2++);
		__m128i result = _mm_cmpeq_epi64(item1, item2);
		// cmpeq returns 0xFFFFFFFFFFFFFFFF per 64-bit portion where equality is
		// true, and 0 per 64-bit portion where false

		// If result is not all ones, then there is a difference here
		if(!(unsigned int)_mm_test_all_ones(result)) {// Ok, now we know they're not equal somewhere
			                                      // In the case where both halves of the 128-bit result integer are
			                                      // 0x0000000000000000, that's the same as
			                                      // 0x0000000000000000FFFFFFFFFFFFFFFF. Only the MSB matters here as the
			                                      // comparison is a greater-than check.

			// Do the greater than comparison here to have it done before the conditional
			// Also make it an unsigned compare:
			// https://stackoverflow.com/questions/52805528/how-does-the-mm-cmpgt-epi64-intrinsic-work
			const __m128i rangeshift = _mm_set1_epi64x(0x8000000000000000);
			__m128i resultgt = _mm_cmpgt_epi64(_mm_xor_si128(item1, rangeshift), _mm_xor_si128(item2, rangeshift));
			// cmpgt returns 0xFFFFFFFFFFFFFFFF per 64-bit portion where item1 > item2 is true

			// _mm_cvtsi64x_si128(0xFFFFFFFFFFFFFFFF) makes 0x0000000000000000FFFFFFFFFFFFFFFF,
			// which is the desired mask inverted.
			// AND the mask with result such that it returns 1 if all zeroes
			if((unsigned int)_mm_test_all_zeros(result, ~_mm_cvtsi64x_si128(0xFFFFFFFFFFFFFFFF))) {
				// Returned a 1, therefore equality comparison gave 0x0000000000000000
				// for both 64-bits or 0x0000000000000000FFFFFFFFFFFFFFFF - this
				// particular case highlights why an unsigned compare is very important.
				// CMPGT will have given 0xFFFFFFFFFFFFFFFFYYYYYYYYYYYYYYYY or
				// 0x0000000000000000YYYYYYYYYYYYYYYY

				// Right shift to put the desired bits into the lower part of the
				// register (overwrite the Ys)
				resultgt = _mm_bsrli_si128(resultgt, 8);
				// Will either be all ones or all zeros. If all ones, item1 > item2, if
				// all zeros, item1 < item2
				if((uint64_t)_mm_cvtsi128_si64x(resultgt)) { // Lop off upper half
					return 1; // 0x[0000000000000000]0000000000000000
				}
				else{
					return -1; // 0x[0000000000000000]FFFFFFFFFFFFFFFF
				}
			}
			else{ // AND mask produced a nonzero value, so the test returned 0.
				// Therefore equality comparison gave 0xFFFFFFFFFFFFFFFF0000000000000000
				// (which is the same as the mask) and CMPGT will have given
				// 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF or 0xFFFFFFFFFFFFFFFF0000000000000000
				// Lower register bits will either be all ones or all zeros. If all ones,
				// item1 > item2, if all zeros, item1 < item2
				if((uint64_t)_mm_cvtsi128_si64x(resultgt)) { // Lop off upper half
					return 1; // 0x[FFFFFFFFFFFFFFFF]FFFFFFFFFFFFFFFF
				}
				else{
					return -1; // 0x[FFFFFFFFFFFFFFFF]0000000000000000
				}
			}
		}
	}
	return 0;
}

// Equality-only version
int memcmp_128bit_eq_u(const void * str1, const void * str2, size_t count)
{
	const __m128i_u * s1 = (__m128i_u*)str1;
	const __m128i_u * s2 = (__m128i_u*)str2;

	while(count--) {
		__m128i item1 = _mm_lddqu_si128(s1++);
		__m128i item2 = _mm_lddqu_si128(s2++);
		__m128i result = _mm_cmpeq_epi64(item1, item2);
		// cmpeq returns 0xFFFFFFFFFFFFFFFF per 64-bit portion where equality is
		// true, and 0 per 64-bit portion where false

		// If result is not all ones, then there is a difference here
		if(!(unsigned int)_mm_test_all_ones(result)) {
			return -1;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
// AVX2+ Unaligned:
//-----------------------------------------------------------------------------

// AVX2 (256-bit, 32 bytes at a time - 8 pixels in a 32-bit linear frame buffer)
// Count is (# of total bytes/32), so it's "# of 256-bits"
// Haswell and Ryzen and up

#ifdef __AVX2__
int memcmp_256bit_u(const void * str1, const void * str2, size_t count)
{
	const __m256i_u * s1 = (__m256i_u*)str1;
	const __m256i_u * s2 = (__m256i_u*)str2;

	while(count--) {
		__m256i item1 = _mm256_lddqu_si256(s1++);
		__m256i item2 = _mm256_lddqu_si256(s2++);
		__m256i result = _mm256_cmpeq_epi64(item1, item2);
		// cmpeq returns 0xFFFFFFFFFFFFFFFF per 64-bit portion where equality is
		// true, and 0 per 64-bit portion where false

		// If result is not all ones, then there is a difference here.
		// This is the same thing as _mm_test_all_ones, but 256-bit
		if(!(unsigned int)_mm256_testc_si256(result, _mm256_set1_epi64x(0xFFFFFFFFFFFFFFFF))) { // Using 0xFFFFFFFFFFFFFFFF explicitly instead of -1 for clarity.
			                                                                                // It really makes no difference on two's complement machines.

			// Ok, now we know they're not equal somewhere. Man, doing a pure != is
			// sooo much simpler than > or <....

			// Unsigned greater-than compare using signed operations, see:
			// https://stackoverflow.com/questions/52805528/how-does-the-mm-cmpgt-epi64-intrinsic-work
			const __m256i rangeshift = _mm256_set1_epi64x(0x8000000000000000);
			__m256i resultgt = _mm256_cmpgt_epi64(_mm256_xor_si256(item1, rangeshift), _mm256_xor_si256(item2, rangeshift));
			// Returns 0xFFFFFFFFFFFFFFFF per 64-bit portion where item1 > item2 is true

			// 32-bit value, 4 outcomes we care about from cmpeq -> movemask:
			// 00YYYYYY FF00YYYY FFFF00YY FFFFFF00, where Y is "don't care." The most
			// significant zeroed byte is the inequality we care about.

			// This is the fastest we can do on AVX2.
			unsigned int result_to_scan = (unsigned int)_mm256_movemask_epi8(result);
			unsigned int resultgt_to_scan = (unsigned int)_mm256_movemask_epi8(resultgt);
			// Outcomes from cmpgt are ZZYYYYYY 00ZZYYYY 0000ZZYY 000000ZZ, where
			// Z is F if item1 > item2, 0 if item1 < item2, and Y is "don't care."
			// The ZZ position of cmpgt will match the corresponding 00 of cmpeq.

			// result_to_scan: 00YYYYYY FF00YYYY FFFF00YY FFFFFF00 --inverted-->
			// FFYYYYYY 00FFYYYY 0000FFYY 000000FF. This will either be
			// > resultgt_to_scan (ZZ = 00) or it won't (ZZ = FF).
			if(~result_to_scan > resultgt_to_scan) {
				return -1; // If ZZ = 00, item1 < item2
			}
			else{
				return 1; // If ZZ = FF, item1 > item2
			}
		}
	}
	return 0;
}

// Equality-only version
int memcmp_256bit_eq_u(const void * str1, const void * str2, size_t count)
{
	const __m256i_u * s1 = (__m256i_u*)str1;
	const __m256i_u * s2 = (__m256i_u*)str2;

	while(count--) {
		__m256i item1 = _mm256_lddqu_si256(s1++);
		__m256i item2 = _mm256_lddqu_si256(s2++);
		__m256i result = _mm256_cmpeq_epi64(item1, item2);
		// cmpeq returns 0xFFFFFFFFFFFFFFFF per 64-bit portion where equality is
		// true, and 0 per 64-bit portion where false

		// If result is not all ones, then there is a difference here.
		// This is the same thing as _mm_test_all_ones, but 256-bit
		if(!(unsigned int)_mm256_testc_si256(result, _mm256_set1_epi64x(0xFFFFFFFFFFFFFFFF))) { // Using 0xFFFFFFFFFFFFFFFF explicitly instead of -1 for clarity.
			                                                                                // It really makes no difference on two's complement machines.
			return -1;
		}
	}
	return 0;
}

#endif

// AVX-512 (512-bit, 64 bytes at a time - 16 pixels in a 32-bit linear frame buffer)
// Count is (# of total bytes/64), so it's "# of 512-bits"
// Requires AVX512F

#ifdef __AVX512F__
int memcmp_512bit_u(const void * str1, const void * str2, size_t count)
{
	const __m512i_u * s1 = (__m512i_u*)str1;
	const __m512i_u * s2 = (__m512i_u*)str2;

	while(count--) {
		__m512i item1 = _mm512_loadu_si512(s1++);
		__m512i item2 = _mm512_loadu_si512(s2++);
		unsigned char result = _mm512_cmpneq_epu64_mask(item1, item2);
		// All bits == 0 means equal

		if(result) { // I don't believe this. I really need a CPU with AVX-512, lol.
//  if(_mm512_mask_cmp_epu64_mask(0xFF, item1, item2, 4)) // 0 is CMPEQ, 4 is CMP_NE, this is the same thing
			unsigned char resultgt = _mm512_cmpgt_epu64_mask(item1, item2);
			// For every set of 64-bits where item1 > item2, the mask will have a 1 bit
			// there, else 0

			if(result > resultgt) { // Similar deal as AVX2
				return -1;
			}
			else{
				return 1;
			}
		}
	}
	return 0;
}

// Equality-only version
int memcmp_512bit_eq_u(const void * str1, const void * str2, size_t count)
{
	const __m512i_u * s1 = (__m512i_u*)str1;
	const __m512i_u * s2 = (__m512i_u*)str2;

	while(count--) {
		__m512i item1 = _mm512_loadu_si512(s1++);
		__m512i item2 = _mm512_loadu_si512(s2++);
		unsigned char result = _mm512_cmpneq_epu64_mask(item1, item2);
		// All bits == 0 means equal

		if(result) { // This is barely bigger than 1-byte memcmp_eq
			return -1;
		}
	}
	return 0;
}

#endif

//-----------------------------------------------------------------------------
// SSE4.2 Aligned:
//-----------------------------------------------------------------------------

// SSE4.2 (128-bit, 16 bytes at a time - 4 pixels in a 32-bit linear frame buffer)
// Count is (# of total bytes/16), so it's "# of 128-bits"

int memcmp_128bit_a(const void * str1, const void * str2, size_t count)
{
	const __m128i * s1 = (__m128i*)str1;
	const __m128i * s2 = (__m128i*)str2;

	while(count--) {
		__m128i item1 = _mm_load_si128(s1++);
		__m128i item2 = _mm_load_si128(s2++);
		__m128i result = _mm_cmpeq_epi64(item1, item2);
		// cmpeq returns 0xFFFFFFFFFFFFFFFF per 64-bit portion where equality is
		// true, and 0 per 64-bit portion where false

		// If result is not all ones, then there is a difference here
		if(!(unsigned int)_mm_test_all_ones(result)) {// Ok, now we know they're not equal somewhere
			                                      // In the case where both halves of the 128-bit result integer are
			                                      // 0x0000000000000000, that's the same as
			                                      // 0x0000000000000000FFFFFFFFFFFFFFFF. Only the MSB matters here as the
			                                      // comparison is a greater-than check.

			// Do the greater than comparison here to have it done before the conditional
			// Also make it an unsigned compare:
			// https://stackoverflow.com/questions/52805528/how-does-the-mm-cmpgt-epi64-intrinsic-work
			const __m128i rangeshift = _mm_set1_epi64x(0x8000000000000000);
			__m128i resultgt = _mm_cmpgt_epi64(_mm_xor_si128(item1, rangeshift), _mm_xor_si128(item2, rangeshift));
			// cmpgt returns 0xFFFFFFFFFFFFFFFF per 64-bit portion where item1 > item2 is true

			// _mm_cvtsi64x_si128(0xFFFFFFFFFFFFFFFF) makes 0x0000000000000000FFFFFFFFFFFFFFFF,
			// which is the desired mask inverted.
			// AND the mask with result such that it returns 1 if all zeroes
			if((unsigned int)_mm_test_all_zeros(result, ~_mm_cvtsi64x_si128(0xFFFFFFFFFFFFFFFF))) {
				// Returned a 1, therefore equality comparison gave 0x0000000000000000
				// for both 64-bits or 0x0000000000000000FFFFFFFFFFFFFFFF - this
				// particular case highlights why an unsigned compare is very important.
				// CMPGT will have given 0xFFFFFFFFFFFFFFFFYYYYYYYYYYYYYYYY or
				// 0x0000000000000000YYYYYYYYYYYYYYYY

				// Right shift to put the desired bits into the lower part of the
				// register (overwrite the Ys)
				resultgt = _mm_bsrli_si128(resultgt, 8);
				// Will either be all ones or all zeros. If all ones, item1 > item2, if
				// all zeros, item1 < item2
				if((uint64_t)_mm_cvtsi128_si64x(resultgt)) { // Lop off upper half
					return 1; // 0x[0000000000000000]0000000000000000
				}
				else{
					return -1; // 0x[0000000000000000]FFFFFFFFFFFFFFFF
				}
			}
			else{ // AND mask produced a nonzero value, so the test returned 0.
				// Therefore equality comparison gave 0xFFFFFFFFFFFFFFFF0000000000000000
				// (which is the same as the mask) and CMPGT will have given
				// 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF or 0xFFFFFFFFFFFFFFFF0000000000000000
				// Lower register bits will either be all ones or all zeros. If all ones,
				// item1 > item2, if all zeros, item1 < item2
				if((uint64_t)_mm_cvtsi128_si64x(resultgt)) { // Lop off upper half
					return 1; // 0x[FFFFFFFFFFFFFFFF]FFFFFFFFFFFFFFFF
				}
				else{
					return -1; // 0x[FFFFFFFFFFFFFFFF]0000000000000000
				}
			}
		}
	}
	return 0;
}

// Equality-only version
int memcmp_128bit_eq_a(const void * str1, const void * str2, size_t count)
{
	const __m128i * s1 = (__m128i*)str1;
	const __m128i * s2 = (__m128i*)str2;

	while(count--) {
		__m128i item1 = _mm_load_si128(s1++);
		__m128i item2 = _mm_load_si128(s2++);
		__m128i result = _mm_cmpeq_epi64(item1, item2);
		// cmpeq returns 0xFFFFFFFFFFFFFFFF per 64-bit portion where equality is
		// true, and 0 per 64-bit portion where false

		// If result is not all ones, then there is a difference here
		if(!(unsigned int)_mm_test_all_ones(result)) {
			return -1;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
// AVX2+ Aligned:
//-----------------------------------------------------------------------------

// AVX2 (256-bit, 32 bytes at a time - 8 pixels in a 32-bit linear frame buffer)
// Count is (# of total bytes/32), so it's "# of 256-bits"
// Haswell and Ryzen and up

#ifdef __AVX2__
int memcmp_256bit_a(const void * str1, const void * str2, size_t count)
{
	const __m256i * s1 = (__m256i*)str1;
	const __m256i * s2 = (__m256i*)str2;

	while(count--) {
		__m256i item1 = _mm256_load_si256(s1++);
		__m256i item2 = _mm256_load_si256(s2++);
		__m256i result = _mm256_cmpeq_epi64(item1, item2);
		// cmpeq returns 0xFFFFFFFFFFFFFFFF per 64-bit portion where equality is
		// true, and 0 per 64-bit portion where false

		// If result is not all ones, then there is a difference here.
		// This is the same thing as _mm_test_all_ones, but 256-bit
		if(!(unsigned int)_mm256_testc_si256(result, _mm256_set1_epi64x(0xFFFFFFFFFFFFFFFF))) { // Using 0xFFFFFFFFFFFFFFFF explicitly instead of -1 for clarity.
			                                                                                // It really makes no difference on two's complement machines.

			// Ok, now we know they're not equal somewhere. Man, doing a pure != is
			// sooo much simpler than > or <....

			// Unsigned greater-than compare using signed operations, see:
			// https://stackoverflow.com/questions/52805528/how-does-the-mm-cmpgt-epi64-intrinsic-work
			const __m256i rangeshift = _mm256_set1_epi64x(0x8000000000000000);
			__m256i resultgt = _mm256_cmpgt_epi64(_mm256_xor_si256(item1, rangeshift), _mm256_xor_si256(item2, rangeshift));
			// Returns 0xFFFFFFFFFFFFFFFF per 64-bit portion where item1 > item2 is true

			// 32-bit value, 4 outcomes we care about from cmpeq -> movemask:
			// 00YYYYYY FF00YYYY FFFF00YY FFFFFF00, where Y is "don't care." The most
			// significant zeroed byte is the inequality we care about.

			// This is the fastest we can do on AVX2.
			unsigned int result_to_scan = (unsigned int)_mm256_movemask_epi8(result);
			unsigned int resultgt_to_scan = (unsigned int)_mm256_movemask_epi8(resultgt);
			// Outcomes from cmpgt are ZZYYYYYY 00ZZYYYY 0000ZZYY 000000ZZ, where
			// Z is F if item1 > item2, 0 if item1 < item2, and Y is "don't care."
			// The ZZ position of cmpgt will match the corresponding 00 of cmpeq.

			// result_to_scan: 00YYYYYY FF00YYYY FFFF00YY FFFFFF00 --inverted-->
			// FFYYYYYY 00FFYYYY 0000FFYY 000000FF. This will either be
			// > resultgt_to_scan (ZZ = 00) or it won't (ZZ = FF).
			if(~result_to_scan > resultgt_to_scan) {
				return -1; // If ZZ = 00, item1 < item2
			}
			else{
				return 1; // If ZZ = FF, item1 > item2
			}
		}
	}
	return 0;
}

// Equality-only version
int memcmp_256bit_eq_a(const void * str1, const void * str2, size_t count)
{
	const __m256i * s1 = (__m256i*)str1;
	const __m256i * s2 = (__m256i*)str2;

	while(count--) {
		__m256i item1 = _mm256_load_si256(s1++);
		__m256i item2 = _mm256_load_si256(s2++);
		__m256i result = _mm256_cmpeq_epi64(item1, item2);
		// cmpeq returns 0xFFFFFFFFFFFFFFFF per 64-bit portion where equality is
		// true, and 0 per 64-bit portion where false

		// If result is not all ones, then there is a difference here.
		// This is the same thing as _mm_test_all_ones, but 256-bit
		if(!(unsigned int)_mm256_testc_si256(result, _mm256_set1_epi64x(0xFFFFFFFFFFFFFFFF))) { // Using 0xFFFFFFFFFFFFFFFF explicitly instead of -1 for clarity.
			                                                                                // It really makes no difference on two's complement machines.
			return -1;
		}
	}
	return 0;
}

#endif

// AVX-512 (512-bit, 64 bytes at a time - 16 pixels in a 32-bit linear frame buffer)
// Count is (# of total bytes/64), so it's "# of 512-bits"
// Requires AVX512F

#ifdef __AVX512F__
int memcmp_512bit_a(const void * str1, const void * str2, size_t count)
{
	const __m512i * s1 = (__m512i*)str1;
	const __m512i * s2 = (__m512i*)str2;

	while(count--) {
		__m512i item1 = _mm512_load_si512(s1++);
		__m512i item2 = _mm512_load_si512(s2++);
		unsigned char result = _mm512_cmpneq_epu64_mask(item1, item2);
		// All bits == 0 means equal

		if(result) { // I don't believe this. I really need a CPU with AVX-512, lol.
//  if(_mm512_mask_cmp_epu64_mask(0xFF, item1, item2, 4)) // 0 is CMPEQ, 4 is CMP_NE, this is the same thing
			unsigned char resultgt = _mm512_cmpgt_epu64_mask(item1, item2);
			// For every set of 64-bits where item1 > item2, the mask will have a 1 bit
			// there, else 0

			if(result > resultgt) { // Similar deal as AVX2
				return -1;
			}
			else{
				return 1;
			}
		}
	}
	return 0;
}

// GCC -O3 makes memcmp_512bit_a(...) take 25 lines of assembly. This version
// (~10 cycles) is around 5 or so cycles slower per set of memory regions than
// memcmp (~5 cycles). It's the mask operations that take ~3 cycles each...
//
// When the latency of jumps are taken into account, that means this function can
// compare 64 BYTES of data at around the same speed that memcmp does only 1 byte.
// The AVX2 version is 1 cycle slower than the AVX512 version in its main loop
// (i.e. it takes ~11 cycles). When an inequality is found, memcmp takes 3 cycles,
// AVX2 takes 16 cycles, and AVX512 takes 10 cycles to determine which input is
// greater.
//
// NOTE: These are estimates based solely on instruction latencies per Agner
// Fog's optimization tables: https://www.agner.org/optimize/.

// Equality-only version
int memcmp_512bit_eq_a(const void * str1, const void * str2, size_t count)
{
	const __m512i * s1 = (__m512i*)str1;
	const __m512i * s2 = (__m512i*)str2;

	while(count--) {
		__m512i item1 = _mm512_load_si512(s1++);
		__m512i item2 = _mm512_load_si512(s2++);
		unsigned char result = _mm512_cmpneq_epu64_mask(item1, item2);
		// All bits == 0 means equal

		if(result) { // This is barely bigger than byte-by-byte memcmp_eq
			return -1;
		}
	}
	return 0;
}

#endif

//-----------------------------------------------------------------------------
// Dispatch Functions (Unaligned):
//-----------------------------------------------------------------------------

// memcmp for large chunks of memory with arbitrary sizes
int memcmp_large(const void * str1, const void * str2, size_t numbytes) // Worst-case scenario: 127 bytes.
{
	int returnval = 0; // Return value if equal... or numbytes is 0
	size_t offset = 0;

	while(numbytes) {
		// This loop will, at most, get evaluated 7 times, ending sooner each time.
		// At minimum non-trivial case, once. Each memcmp has its own loop.
		if(numbytes < 2) { // 1 byte
			returnval = memcmp(str1, str2, numbytes);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -1;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes = 0;
		}
		else if(numbytes < 4) { // 2 bytes
			returnval = memcmp_16bit(str1, str2, numbytes >> 1);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -2;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 1;
		}
		else if(numbytes < 8) { // 4 bytes
			returnval = memcmp_32bit(str1, str2, numbytes >> 2);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -4;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 3;
		}
		else if(numbytes < 16) { // 8 bytes
			returnval = memcmp_64bit(str1, str2, numbytes >> 3);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -8;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 7;
		}
#ifdef __AVX512F__
		else if(numbytes < 32) { // 16 bytes
			returnval = memcmp_128bit_u(str1, str2, numbytes >> 4);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -16;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			returnval = memcmp_256bit_u(str1, str2, numbytes >> 5);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -32;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 31;
		}
		else{ // 64 bytes
			returnval = memcmp_512bit_u(str1, str2, numbytes >> 6);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -64;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 63;
		}
#elif __AVX2__
		else if(numbytes < 32) { // 16 bytes
			returnval = memcmp_128bit_u(str1, str2, numbytes >> 4);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -16;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 15;
		}
		else{ // 32 bytes
			returnval = memcmp_256bit_u(str1, str2, numbytes >> 5);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -32;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 31;
		}
#else // SSE4.2 only
		else{ // 16 bytes
			returnval = memcmp_128bit_u(str1, str2, numbytes >> 4);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -16;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 15;
		}
#endif
	}
	return returnval;
}

// Equality-only version
int memcmp_large_eq(const void * str1, const void * str2, size_t numbytes) // Worst-case scenario: 127 bytes.
{
	int returnval = 0; // Return value if equal... or numbytes is 0
	size_t offset = 0;

	while(numbytes) {
		if(numbytes < 2) { // 1 byte
			returnval = memcmp_eq(str1, str2, numbytes);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -1;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes = 0;
		}
		else if(numbytes < 4) { // 2 bytes
			returnval = memcmp_16bit_eq(str1, str2, numbytes >> 1);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -2;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 1;
		}
		else if(numbytes < 8) { // 4 bytes
			returnval = memcmp_32bit_eq(str1, str2, numbytes >> 2);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -4;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 3;
		}
		else if(numbytes < 16) { // 8 bytes
			returnval = memcmp_64bit_eq(str1, str2, numbytes >> 3);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -8;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 7;
		}
#ifdef __AVX512F__
		else if(numbytes < 32) { // 16 bytes
			returnval = memcmp_128bit_eq_u(str1, str2, numbytes >> 4);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -16;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			returnval = memcmp_256bit_eq_u(str1, str2, numbytes >> 5);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -32;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 31;
		}
		else{ // 64 bytes
			returnval = memcmp_512bit_eq_u(str1, str2, numbytes >> 6);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -64;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 63;
		}
#elif __AVX2__
		else if(numbytes < 32) { // 16 bytes
			returnval = memcmp_128bit_eq_u(str1, str2, numbytes >> 4);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -16;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 15;
		}
		else{ // 32 bytes
			returnval = memcmp_256bit_eq_u(str1, str2, numbytes >> 5);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -32;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 31;
		}
#else // SSE4.2 only
		else{ // 16 bytes
			returnval = memcmp_128bit_eq_u(str1, str2, numbytes >> 4);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -16;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 15;
		}
#endif
	}
	return returnval;
}

//-----------------------------------------------------------------------------
// Dispatch Functions (Aligned):
//-----------------------------------------------------------------------------

// memcmp for large chunks of memory with arbitrary sizes (aligned)
int memcmp_large_a(const void * str1, const void * str2, size_t numbytes) // Worst-case scenario: 127 bytes.
{
	int returnval = 0; // Return value if equal... or numbytes is 0
	size_t offset = 0;

	while(numbytes) {
		// This loop will, at most, get evaulated 7 times, ending sooner each time.
		// At minimum non-trivial case, once. Each memcmp has its own loop.
		if(numbytes < 2) { // 1 byte
			returnval = memcmp(str1, str2, numbytes);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -1;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes = 0;
		}
		else if(numbytes < 4) { // 2 bytes
			returnval = memcmp_16bit(str1, str2, numbytes >> 1);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -2;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 1;
		}
		else if(numbytes < 8) { // 4 bytes
			returnval = memcmp_32bit(str1, str2, numbytes >> 2);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -4;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 3;
		}
		else if(numbytes < 16) { // 8 bytes
			returnval = memcmp_64bit(str1, str2, numbytes >> 3);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -8;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 7;
		}
#ifdef __AVX512F__
		else if(numbytes < 32) { // 16 bytes
			returnval = memcmp_128bit_a(str1, str2, numbytes >> 4);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -16;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			returnval = memcmp_256bit_a(str1, str2, numbytes >> 5);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -32;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 31;
		}
		else{ // 64 bytes
			returnval = memcmp_512bit_a(str1, str2, numbytes >> 6);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -64;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 63;
		}
#elif __AVX2__
		else if(numbytes < 32) { // 16 bytes
			returnval = memcmp_128bit_a(str1, str2, numbytes >> 4);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -16;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 15;
		}
		else{ // 32 bytes
			returnval = memcmp_256bit_a(str1, str2, numbytes >> 5);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -32;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 31;
		}
#else // SSE4.2 only
		else{ // 16 bytes
			returnval = memcmp_128bit_a(str1, str2, numbytes >> 4);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -16;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 15;
		}
#endif
	}
	return returnval;
}

// Equality-only version (aligned)
int memcmp_large_eq_a(const void * str1, const void * str2, size_t numbytes) // Worst-case scenario: 127 bytes.
{
	int returnval = 0; // Return value if equal... or numbytes is 0
	size_t offset = 0;

	while(numbytes) {
		if(numbytes < 2) { // 1 byte
			returnval = memcmp_eq(str1, str2, numbytes);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -1;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes = 0;
		}
		else if(numbytes < 4) { // 2 bytes
			returnval = memcmp_16bit_eq(str1, str2, numbytes >> 1);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -2;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 1;
		}
		else if(numbytes < 8) { // 4 bytes
			returnval = memcmp_32bit_eq(str1, str2, numbytes >> 2);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -4;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 3;
		}
		else if(numbytes < 16) { // 8 bytes
			returnval = memcmp_64bit_eq(str1, str2, numbytes >> 3);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -8;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 7;
		}
#ifdef __AVX512F__
		else if(numbytes < 32) { // 16 bytes
			returnval = memcmp_128bit_eq_a(str1, str2, numbytes >> 4);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -16;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 15;
		}
		else if(numbytes < 64) { // 32 bytes
			returnval = memcmp_256bit_eq_a(str1, str2, numbytes >> 5);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -32;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 31;
		}
		else{ // 64 bytes
			returnval = memcmp_512bit_eq_a(str1, str2, numbytes >> 6);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -64;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 63;
		}
#elif __AVX2__
		else if(numbytes < 32) { // 16 bytes
			returnval = memcmp_128bit_eq_a(str1, str2, numbytes >> 4);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -16;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 15;
		}
		else{ // 32 bytes
			returnval = memcmp_256bit_eq_a(str1, str2, numbytes >> 5);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -32;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 31;
		}
#else // SSE4.2 only
		else{ // 16 bytes
			returnval = memcmp_128bit_eq_a(str1, str2, numbytes >> 4);
			if(returnval) {
				return returnval;
			}
			offset = numbytes & -16;
			str1 = (char *)str1 + offset;
			str2 = (char *)str2 + offset;
			numbytes &= 15;
		}
#endif
	}
	return returnval;
}

//-----------------------------------------------------------------------------
// Main Function:
//-----------------------------------------------------------------------------

// Main memcmp function
int AVX_memcmp(const void * str1, const void * str2, size_t numbytes, int equality)
{
	int returnval = 0;

	if(
		( ((uintptr_t)str1 & BYTE_ALIGNMENT) == 0 )
		&&
		( ((uintptr_t)str2 & BYTE_ALIGNMENT) == 0 )
		) { // Check alignment
		// See memmove.c for why it's worth doing special aligned versions of memcmp, which
		// is a function that involves 2 loads.
		if(equality == 0) {
			returnval = memcmp_large_eq_a(str1, str2, numbytes);
		}
		else{
			returnval = memcmp_large_a(str1, str2, numbytes);
		}
	}
	else{
		if(equality == 0) {
			returnval = memcmp_large_eq(str1, str2, numbytes);
		}
		else{
			returnval = memcmp_large(str1, str2, numbytes);
		}
	}

	return returnval;
}

// AVX-1024+ support pending existence of the standard.
