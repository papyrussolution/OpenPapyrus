/**************************  vector_convert.h   *******************************
 * Author:        Agner Fog
 * Date created:  2014-07-23
 * Last modified: 2019-08-02
 * Version:       1.40.00
 * Project:       vector class library
 * Description:
 * Header file for conversion between different vector classes with different
 * sizes.
 *
 * (c) Copyright 2012-2019 Agner Fog.
 * Apache License version 2.0 or later.
 *****************************************************************************/

#ifndef VECTOR_CONVERT_H
#define VECTOR_CONVERT_H

#ifndef VECTORCLASS_H
	#include "vectorclass.h"
#endif

#if VECTORCLASS_H > 19999
	#error Incompatible versions of vector class library mixed
#endif
#ifdef VCL_NAMESPACE
namespace VCL_NAMESPACE {
#endif

#if MAX_VECTOR_SIZE >= 256

/*****************************************************************************
*
*          Extend from 128 to 256 bit vectors
*
*****************************************************************************/

#if INSTRSET >= 8  // AVX2. 256 bit integer vectors

// sign extend
static inline Vec16s extend(Vec16c const a) { return _mm256_cvtepi8_epi16(a); }
// zero extend
static inline Vec16us extend(Vec16uc const a) { return _mm256_cvtepu8_epi16(a); }
// sign extend
static inline Vec8i extend(Vec8s const a) { return _mm256_cvtepi16_epi32(a); }
// zero extend
static inline Vec8ui extend(Vec8us const a) { return _mm256_cvtepu16_epi32(a); }
// sign extend
static inline Vec4q extend(Vec4i const a) { return _mm256_cvtepi32_epi64(a); }
// zero extend
static inline Vec4uq extend(Vec4ui const a) { return _mm256_cvtepu32_epi64(a); }

#else  // no AVX2. 256 bit vectors are emulated

// sign extend and zero extend functions:
static inline Vec16s extend(Vec16c const a) { return Vec16s(extend_low(a), extend_high(a)); }
static inline Vec16us extend(Vec16uc const a) { return Vec16us(extend_low(a), extend_high(a)); }
static inline Vec8i extend(Vec8s const a) { return Vec8i(extend_low(a), extend_high(a)); }
static inline Vec8ui extend(Vec8us const a) { return Vec8ui(extend_low(a), extend_high(a)); }
static inline Vec4q extend(Vec4i const a) { return Vec4q(extend_low(a), extend_high(a)); }
static inline Vec4uq extend(Vec4ui const a) { return Vec4uq(extend_low(a), extend_high(a)); }

#endif  // AVX2

/*****************************************************************************
*
*          Conversions between float and double
*
*****************************************************************************/
#if INSTRSET >= 7  // AVX. 256 bit float vectors

// float to double
static inline Vec4d to_double(Vec4f const a) { return _mm256_cvtps_pd(a); }
// double to float
static inline Vec4f to_float(Vec4d const a) { return _mm256_cvtpd_ps(a); }

#else  // no AVX2. 256 bit float vectors are emulated

// float to double
static inline Vec4d to_double(Vec4f const a) 
{
	Vec2d lo = _mm_cvtps_pd(a);
	Vec2d hi = _mm_cvtps_pd(_mm_movehl_ps(a, a));
	return Vec4d(lo, hi);
}

// double to float
static inline Vec4f to_float(Vec4d const a) 
{
	Vec4f lo = _mm_cvtpd_ps(a.get_low());
	Vec4f hi = _mm_cvtpd_ps(a.get_high());
	return _mm_movelh_ps(lo, hi);
}

#endif

/*****************************************************************************
*
*          Reduce from 256 to 128 bit vectors
*
*****************************************************************************/
#if INSTRSET >= 10  // AVX512VL
	// compress functions. overflow wraps around
	static inline Vec16c compress(Vec16s const a) { return _mm256_cvtepi16_epi8(a); }
	static inline Vec16uc compress(Vec16us const a) { return _mm256_cvtepi16_epi8(a); }
	static inline Vec8s compress(Vec8i const a) { return _mm256_cvtepi32_epi16(a); }
	static inline Vec8us compress(Vec8ui const a) { return _mm256_cvtepi32_epi16(a); }
	static inline Vec4i compress(Vec4q const a) { return _mm256_cvtepi64_epi32(a); }
	static inline Vec4ui compress(Vec4uq const a) { return _mm256_cvtepi64_epi32(a); }
#else  // no AVX512
	// compress functions. overflow wraps around
	static inline Vec16c compress(Vec16s const a) { return compress(a.get_low(), a.get_high()); }
	static inline Vec16uc compress(Vec16us const a) { return compress(a.get_low(), a.get_high()); }
	static inline Vec8s compress(Vec8i const a) { return compress(a.get_low(), a.get_high()); }
	static inline Vec8us compress(Vec8ui const a) { return compress(a.get_low(), a.get_high()); }
	static inline Vec4i compress(Vec4q const a) { return compress(a.get_low(), a.get_high()); }
	static inline Vec4ui compress(Vec4uq const a) { return compress(a.get_low(), a.get_high()); }
#endif  // AVX512

#endif // MAX_VECTOR_SIZE >= 256

#if MAX_VECTOR_SIZE >= 512

/*****************************************************************************
*
*          Extend from 256 to 512 bit vectors
*
*****************************************************************************/

#if INSTRSET >= 9  // AVX512. 512 bit integer vectors

// sign extend
static inline Vec32s extend(Vec32c const a) 
{
#if INSTRSET >= 10
	return _mm512_cvtepi8_epi16(a);
#else
	return Vec32s(extend_low(a), extend_high(a));
#endif
}

// zero extend
static inline Vec32us extend(Vec32uc const a) 
{
#if INSTRSET >= 10
	return _mm512_cvtepu8_epi16(a);
#else
	return Vec32us(extend_low(a), extend_high(a));
#endif
}

// sign extend
static inline Vec16i extend(Vec16s const a) { return _mm512_cvtepi16_epi32(a); }
// zero extend
static inline Vec16ui extend(Vec16us const a) { return _mm512_cvtepu16_epi32(a); }
// sign extend
static inline Vec8q extend(Vec8i const a) { return _mm512_cvtepi32_epi64(a); }
// zero extend
static inline Vec8uq extend(Vec8ui const a) { return _mm512_cvtepu32_epi64(a); }

#else  // no AVX512. 512 bit vectors are emulated

// sign extend
static inline Vec32s extend(Vec32c const a) { return Vec32s(extend_low(a), extend_high(a)); }
// zero extend
static inline Vec32us extend(Vec32uc const a) { return Vec32us(extend_low(a), extend_high(a)); }
// sign extend
static inline Vec16i extend(Vec16s const a) { return Vec16i(extend_low(a), extend_high(a)); }
// zero extend
static inline Vec16ui extend(Vec16us const a) { return Vec16ui(extend_low(a), extend_high(a)); }
// sign extend
static inline Vec8q extend(Vec8i const a) { return Vec8q(extend_low(a), extend_high(a)); }
// zero extend
static inline Vec8uq extend(Vec8ui const a) { return Vec8uq(extend_low(a), extend_high(a)); }

#endif  // AVX512

/*****************************************************************************
*
*          Reduce from 512 to 256 bit vectors
*
*****************************************************************************/
#if INSTRSET >= 9  // AVX512F
	// compress functions. overflow wraps around
	static inline Vec32c compress(Vec32s const a) 
	{
	#if INSTRSET >= 10  // AVVX512BW
		return _mm512_cvtepi16_epi8(a);
	#else
		return compress(a.get_low(), a.get_high());
	#endif
	}

	static inline Vec32uc compress(Vec32us const a) { return Vec32uc(compress(Vec32s(a))); }
	static inline Vec16s compress(Vec16i const a) { return _mm512_cvtepi32_epi16(a); }
	static inline Vec16us compress(Vec16ui const a) { return _mm512_cvtepi32_epi16(a); }
	static inline Vec8i compress(Vec8q const a) { return _mm512_cvtepi64_epi32(a); }
	static inline Vec8ui compress(Vec8uq const a) { return _mm512_cvtepi64_epi32(a); }
#else  // no AVX512
	// compress functions. overflow wraps around
	static inline Vec32c compress(Vec32s const a) { return compress(a.get_low(), a.get_high()); }
	static inline Vec32uc compress(Vec32us const a) { return compress(a.get_low(), a.get_high()); }
	static inline Vec16s compress(Vec16i const a) { return compress(a.get_low(), a.get_high()); }
	static inline Vec16us compress(Vec16ui const a) { return compress(a.get_low(), a.get_high()); }
	static inline Vec8i compress(Vec8q const a) { return compress(a.get_low(), a.get_high()); }
	static inline Vec8ui compress(Vec8uq const a) { return compress(a.get_low(), a.get_high()); }
#endif  // AVX512

/*****************************************************************************
*
*          Conversions between float and double
*
*****************************************************************************/

#if INSTRSET >= 9  // AVX512. 512 bit float vectors
	// float to double
	static inline Vec8d to_double(Vec8f const a) { return _mm512_cvtps_pd(a); }
	// double to float
	static inline Vec8f to_float(Vec8d const a) { return _mm512_cvtpd_ps(a); }
#else  // no AVX512. 512 bit float vectors are emulated
	// float to double
	static inline Vec8d to_double(Vec8f const a) 
	{
		Vec4d lo = to_double(a.get_low());
		Vec4d hi = to_double(a.get_high());
		return Vec8d(lo, hi);
	}

	// double to float
	static inline Vec8f to_float(Vec8d const a) 
	{
		Vec4f lo = to_float(a.get_low());
		Vec4f hi = to_float(a.get_high());
		return Vec8f(lo, hi);
	}
#endif

#endif // MAX_VECTOR_SIZE >= 512

// double to float
static inline Vec4f to_float(Vec2d const a) {
	return _mm_cvtpd_ps(a);
}

#ifdef VCL_NAMESPACE
}
#endif

#endif // VECTOR_CONVERT_H
