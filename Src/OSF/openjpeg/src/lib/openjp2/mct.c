/*
 * The copyright in this software is being made available under the 2-clauses
 * BSD License, included below. This software may be subject to other third
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2002-2014, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2014, Professor Benoit Macq
 * Copyright (c) 2001-2003, David Janssens
 * Copyright (c) 2002-2003, Yannick Verschueren
 * Copyright (c) 2003-2007, Francois-Olivier Devaux
 * Copyright (c) 2003-2014, Antonin Descampe
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
 * Copyright (c) 2008, 2011-2012, Centre National d'Etudes Spatiales (CNES), FR
 * Copyright (c) 2012, CS Systemes d'Information, France
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 */
#include "opj_includes.h"
#pragma hdrstop
#ifdef __SSE__
#include <xmmintrin.h>
#endif
#ifdef __SSE2__
#include <emmintrin.h>
#endif
#ifdef __SSE4_1__
#include <smmintrin.h>
#endif

/* <summary> */
/* This table contains the norms of the basis function of the reversible MCT. */
/* </summary> */
static const double opj_mct_norms[3] = { 1.732, .8292, .8292 };

/* <summary> */
/* This table contains the norms of the basis function of the irreversible MCT. */
/* </summary> */
static const double opj_mct_norms_real[3] = { 1.732, 1.805, 1.573 };

const double * opj_mct_get_mct_norms()
{
	return opj_mct_norms;
}

const double * opj_mct_get_mct_norms_real()
{
	return opj_mct_norms_real;
}

/* <summary> */
/* Forward reversible MCT. */
/* </summary> */
#ifdef __SSE2__
void opj_mct_encode(int32_t* OPJ_RESTRICT c0,
    int32_t* OPJ_RESTRICT c1,
    int32_t* OPJ_RESTRICT c2,
    size_t n)
{
	size_t i;
	const size_t len = n;
	/* buffer are aligned on 16 bytes */
	assert(((size_t)c0 & 0xf) == 0);
	assert(((size_t)c1 & 0xf) == 0);
	assert(((size_t)c2 & 0xf) == 0);

	for(i = 0; i < (len & ~3U); i += 4) {
		__m128i y, u, v;
		__m128i r = _mm_load_si128((const __m128i*)&(c0[i]));
		__m128i g = _mm_load_si128((const __m128i*)&(c1[i]));
		__m128i b = _mm_load_si128((const __m128i*)&(c2[i]));
		y = _mm_add_epi32(g, g);
		y = _mm_add_epi32(y, b);
		y = _mm_add_epi32(y, r);
		y = _mm_srai_epi32(y, 2);
		u = _mm_sub_epi32(b, g);
		v = _mm_sub_epi32(r, g);
		_mm_store_si128((__m128i*)&(c0[i]), y);
		_mm_store_si128((__m128i*)&(c1[i]), u);
		_mm_store_si128((__m128i*)&(c2[i]), v);
	}

	for(; i < len; ++i) {
		int32_t r = c0[i];
		int32_t g = c1[i];
		int32_t b = c2[i];
		int32_t y = (r + (g * 2) + b) >> 2;
		int32_t u = b - g;
		int32_t v = r - g;
		c0[i] = y;
		c1[i] = u;
		c2[i] = v;
	}
}

#else
void opj_mct_encode(int32_t* OPJ_RESTRICT c0,
    int32_t* OPJ_RESTRICT c1,
    int32_t* OPJ_RESTRICT c2,
    size_t n)
{
	size_t i;
	const size_t len = n;

	for(i = 0; i < len; ++i) {
		int32_t r = c0[i];
		int32_t g = c1[i];
		int32_t b = c2[i];
		int32_t y = (r + (g * 2) + b) >> 2;
		int32_t u = b - g;
		int32_t v = r - g;
		c0[i] = y;
		c1[i] = u;
		c2[i] = v;
	}
}

#endif

/* <summary> */
/* Inverse reversible MCT. */
/* </summary> */
#ifdef __SSE2__
void opj_mct_decode(int32_t* OPJ_RESTRICT c0,
    int32_t* OPJ_RESTRICT c1,
    int32_t* OPJ_RESTRICT c2,
    size_t n)
{
	size_t i;
	const size_t len = n;

	for(i = 0; i < (len & ~3U); i += 4) {
		__m128i r, g, b;
		__m128i y = _mm_load_si128((const __m128i*)&(c0[i]));
		__m128i u = _mm_load_si128((const __m128i*)&(c1[i]));
		__m128i v = _mm_load_si128((const __m128i*)&(c2[i]));
		g = y;
		g = _mm_sub_epi32(g, _mm_srai_epi32(_mm_add_epi32(u, v), 2));
		r = _mm_add_epi32(v, g);
		b = _mm_add_epi32(u, g);
		_mm_store_si128((__m128i*)&(c0[i]), r);
		_mm_store_si128((__m128i*)&(c1[i]), g);
		_mm_store_si128((__m128i*)&(c2[i]), b);
	}
	for(; i < len; ++i) {
		int32_t y = c0[i];
		int32_t u = c1[i];
		int32_t v = c2[i];
		int32_t g = y - ((u + v) >> 2);
		int32_t r = v + g;
		int32_t b = u + g;
		c0[i] = r;
		c1[i] = g;
		c2[i] = b;
	}
}

#else
void opj_mct_decode(int32_t* OPJ_RESTRICT c0,
    int32_t* OPJ_RESTRICT c1,
    int32_t* OPJ_RESTRICT c2,
    size_t n)
{
	size_t i;
	for(i = 0; i < n; ++i) {
		int32_t y = c0[i];
		int32_t u = c1[i];
		int32_t v = c2[i];
		int32_t g = y - ((u + v) >> 2);
		int32_t r = v + g;
		int32_t b = u + g;
		c0[i] = r;
		c1[i] = g;
		c2[i] = b;
	}
}

#endif

/* <summary> */
/* Get norm of basis function of reversible MCT. */
/* </summary> */
double opj_mct_getnorm(uint32_t compno)
{
	return opj_mct_norms[compno];
}

/* <summary> */
/* Forward irreversible MCT. */
/* </summary> */
void opj_mct_encode_real(float* OPJ_RESTRICT c0,
    float* OPJ_RESTRICT c1,
    float* OPJ_RESTRICT c2,
    size_t n)
{
	size_t i;
#ifdef __SSE__
	const __m128 YR = _mm_set1_ps(0.299f);
	const __m128 YG = _mm_set1_ps(0.587f);
	const __m128 YB = _mm_set1_ps(0.114f);
	const __m128 UR = _mm_set1_ps(-0.16875f);
	const __m128 UG = _mm_set1_ps(-0.331260f);
	const __m128 UB = _mm_set1_ps(0.5f);
	const __m128 VR = _mm_set1_ps(0.5f);
	const __m128 VG = _mm_set1_ps(-0.41869f);
	const __m128 VB = _mm_set1_ps(-0.08131f);
	for(i = 0; i < (n >> 3); i++) {
		__m128 r, g, b, y, u, v;

		r = _mm_load_ps(c0);
		g = _mm_load_ps(c1);
		b = _mm_load_ps(c2);
		y = _mm_add_ps(_mm_add_ps(_mm_mul_ps(r, YR), _mm_mul_ps(g, YG)),
			_mm_mul_ps(b, YB));
		u = _mm_add_ps(_mm_add_ps(_mm_mul_ps(r, UR), _mm_mul_ps(g, UG)),
			_mm_mul_ps(b, UB));
		v = _mm_add_ps(_mm_add_ps(_mm_mul_ps(r, VR), _mm_mul_ps(g, VG)),
			_mm_mul_ps(b, VB));
		_mm_store_ps(c0, y);
		_mm_store_ps(c1, u);
		_mm_store_ps(c2, v);
		c0 += 4;
		c1 += 4;
		c2 += 4;

		r = _mm_load_ps(c0);
		g = _mm_load_ps(c1);
		b = _mm_load_ps(c2);
		y = _mm_add_ps(_mm_add_ps(_mm_mul_ps(r, YR), _mm_mul_ps(g, YG)),
			_mm_mul_ps(b, YB));
		u = _mm_add_ps(_mm_add_ps(_mm_mul_ps(r, UR), _mm_mul_ps(g, UG)),
			_mm_mul_ps(b, UB));
		v = _mm_add_ps(_mm_add_ps(_mm_mul_ps(r, VR), _mm_mul_ps(g, VG)),
			_mm_mul_ps(b, VB));
		_mm_store_ps(c0, y);
		_mm_store_ps(c1, u);
		_mm_store_ps(c2, v);
		c0 += 4;
		c1 += 4;
		c2 += 4;
	}
	n &= 7;
#endif
	for(i = 0; i < n; ++i) {
		float r = c0[i];
		float g = c1[i];
		float b = c2[i];
		float y = 0.299f * r + 0.587f * g + 0.114f * b;
		float u = -0.16875f * r - 0.331260f * g + 0.5f * b;
		float v = 0.5f * r - 0.41869f * g - 0.08131f * b;
		c0[i] = y;
		c1[i] = u;
		c2[i] = v;
	}
}

/* <summary> */
/* Inverse irreversible MCT. */
/* </summary> */
void opj_mct_decode_real(float* OPJ_RESTRICT c0,
    float* OPJ_RESTRICT c1,
    float* OPJ_RESTRICT c2,
    size_t n)
{
	size_t i;
#ifdef __SSE__
	__m128 vrv, vgu, vgv, vbu;
	vrv = _mm_set1_ps(1.402f);
	vgu = _mm_set1_ps(0.34413f);
	vgv = _mm_set1_ps(0.71414f);
	vbu = _mm_set1_ps(1.772f);
	for(i = 0; i < (n >> 3); ++i) {
		__m128 vy, vu, vv;
		__m128 vr, vg, vb;

		vy = _mm_load_ps(c0);
		vu = _mm_load_ps(c1);
		vv = _mm_load_ps(c2);
		vr = _mm_add_ps(vy, _mm_mul_ps(vv, vrv));
		vg = _mm_sub_ps(_mm_sub_ps(vy, _mm_mul_ps(vu, vgu)), _mm_mul_ps(vv, vgv));
		vb = _mm_add_ps(vy, _mm_mul_ps(vu, vbu));
		_mm_store_ps(c0, vr);
		_mm_store_ps(c1, vg);
		_mm_store_ps(c2, vb);
		c0 += 4;
		c1 += 4;
		c2 += 4;

		vy = _mm_load_ps(c0);
		vu = _mm_load_ps(c1);
		vv = _mm_load_ps(c2);
		vr = _mm_add_ps(vy, _mm_mul_ps(vv, vrv));
		vg = _mm_sub_ps(_mm_sub_ps(vy, _mm_mul_ps(vu, vgu)), _mm_mul_ps(vv, vgv));
		vb = _mm_add_ps(vy, _mm_mul_ps(vu, vbu));
		_mm_store_ps(c0, vr);
		_mm_store_ps(c1, vg);
		_mm_store_ps(c2, vb);
		c0 += 4;
		c1 += 4;
		c2 += 4;
	}
	n &= 7;
#endif
	for(i = 0; i < n; ++i) {
		float y = c0[i];
		float u = c1[i];
		float v = c2[i];
		float r = y + (v * 1.402f);
		float g = y - (u * 0.34413f) - (v * (0.71414f));
		float b = y + (u * 1.772f);
		c0[i] = r;
		c1[i] = g;
		c2[i] = b;
	}
}

/* <summary> */
/* Get norm of basis function of irreversible MCT. */
/* </summary> */
double opj_mct_getnorm_real(uint32_t compno)
{
	return opj_mct_norms_real[compno];
}

boolint opj_mct_encode_custom(uint8 * pCodingdata,
    size_t n,
    uint8 ** pData,
    uint32_t pNbComp,
    uint32_t isSigned)
{
	float * lMct = (float*)pCodingdata;
	size_t i;
	uint32_t j;
	uint32_t k;
	uint32_t lNbMatCoeff = pNbComp * pNbComp;
	int32_t * lCurrentData = 0;
	int32_t * lCurrentMatrix = 0;
	int32_t ** lData = (int32_t**)pData;
	uint32_t lMultiplicator = 1 << 13;
	int32_t * lMctPtr;

	OPJ_ARG_NOT_USED(isSigned);

	lCurrentData = (int32_t*)opj_malloc((pNbComp + lNbMatCoeff) * sizeof(
			int32_t));
	if(!lCurrentData) {
		return FALSE;
	}

	lCurrentMatrix = lCurrentData + pNbComp;

	for(i = 0; i < lNbMatCoeff; ++i) {
		lCurrentMatrix[i] = (int32_t)(*(lMct++) * (float)lMultiplicator);
	}

	for(i = 0; i < n; ++i) {
		lMctPtr = lCurrentMatrix;
		for(j = 0; j < pNbComp; ++j) {
			lCurrentData[j] = (*(lData[j]));
		}

		for(j = 0; j < pNbComp; ++j) {
			*(lData[j]) = 0;
			for(k = 0; k < pNbComp; ++k) {
				*(lData[j]) += opj_int_fix_mul(*lMctPtr, lCurrentData[k]);
				++lMctPtr;
			}

			++lData[j];
		}
	}

	SAlloc::F(lCurrentData);

	return TRUE;
}

boolint opj_mct_decode_custom(uint8 * pDecodingData,
    size_t n,
    uint8 ** pData,
    uint32_t pNbComp,
    uint32_t isSigned)
{
	float * lMct;
	size_t i;
	uint32_t j;
	uint32_t k;

	float * lCurrentData = 0;
	float * lCurrentResult = 0;
	float ** lData = (float**)pData;

	OPJ_ARG_NOT_USED(isSigned);

	lCurrentData = (float*)opj_malloc(2 * pNbComp * sizeof(float));
	if(!lCurrentData) {
		return FALSE;
	}
	lCurrentResult = lCurrentData + pNbComp;

	for(i = 0; i < n; ++i) {
		lMct = (float*)pDecodingData;
		for(j = 0; j < pNbComp; ++j) {
			lCurrentData[j] = (float)(*(lData[j]));
		}
		for(j = 0; j < pNbComp; ++j) {
			lCurrentResult[j] = 0;
			for(k = 0; k < pNbComp; ++k) {
				lCurrentResult[j] += *(lMct++) * lCurrentData[k];
			}
			*(lData[j]++) = (float)(lCurrentResult[j]);
		}
	}
	SAlloc::F(lCurrentData);
	return TRUE;
}

void opj_calculate_norms(double * pNorms,
    uint32_t pNbComps,
    float * pMatrix)
{
	uint32_t i, j, lIndex;
	float lCurrentValue;
	double * lNorms = (double*)pNorms;
	float * lMatrix = (float*)pMatrix;

	for(i = 0; i < pNbComps; ++i) {
		lNorms[i] = 0;
		lIndex = i;

		for(j = 0; j < pNbComps; ++j) {
			lCurrentValue = lMatrix[lIndex];
			lIndex += pNbComps;
			lNorms[i] += (double)lCurrentValue * lCurrentValue;
		}
		lNorms[i] = sqrt(lNorms[i]);
	}
}
