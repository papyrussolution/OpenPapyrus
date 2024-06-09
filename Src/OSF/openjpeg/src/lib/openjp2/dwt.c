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
 * Copyright (c) 2007, Jonathan Ballard <dzonatas@dzonux.net>
 * Copyright (c) 2007, Callum Lerwick <seg@haxxed.com>
 * Copyright (c) 2017, IntoPIX SA <support@intopix.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 */
// Discrete wavelet transform
#include "opj_includes.h"
#pragma hdrstop
#define OPJ_SKIP_POISON

#ifdef __SSE__
	#include <xmmintrin.h>
#endif
#ifdef __SSE2__
	#include <emmintrin.h>
#endif
#ifdef __SSSE3__
	#include <tmmintrin.h>
#endif
#ifdef __AVX2__
	#include <immintrin.h>
#endif
#if defined(__GNUC__)
	#pragma GCC poison malloc calloc realloc free
#endif
/** @defgroup DWT DWT - Implementation of a discrete wavelet transform */
/*@{*/

#define OPJ_WS(i) v->mem[(i)*2]
#define OPJ_WD(i) v->mem[(1+(i)*2)]
#ifdef __AVX2__
	#define VREG_INT_COUNT       8 /** Number of int32 values in a AVX2 register */
#else
	#define VREG_INT_COUNT       4 /** Number of int32 values in a SSE2 register */
#endif
#define PARALLEL_COLS_53     (2*VREG_INT_COUNT) /** Number of columns that we can process in parallel in the vertical pass */

/** @name Local data structures */
/*@{*/

typedef struct dwt_local {
	int32_t* mem;
	int32_t dn; /* number of elements in high pass band */
	int32_t sn; /* number of elements in low pass band */
	int32_t cas; /* 0 = start on even coord, 1 = start on odd coord */
} opj_dwt_t;

#define NB_ELTS_V8  8

typedef union {
	float f[NB_ELTS_V8];
} opj_v8_t;

typedef struct v8dwt_local {
	opj_v8_t*   wavelet;
	int32_t dn;     /* number of elements in high pass band */
	int32_t sn;     /* number of elements in low pass band */
	int32_t cas;    /* 0 = start on even coord, 1 = start on odd coord */
	uint32_t win_l_x0;  /* start coord in low pass band */
	uint32_t win_l_x1;  /* end coord in low pass band */
	uint32_t win_h_x0;  /* start coord in high pass band */
	uint32_t win_h_x1;  /* end coord in high pass band */
} opj_v8dwt_t;

/* From table F.4 from the standard */
static const float opj_dwt_alpha =  -1.586134342f;
static const float opj_dwt_beta  =  -0.052980118f;
static const float opj_dwt_gamma = 0.882911075f;
static const float opj_dwt_delta = 0.443506852f;

static const float opj_K      = 1.230174105f;
static const float opj_invK   = (float)(1.0 / 1.230174105);
/*@}*/

/** @name Local static functions */
/*@{*/

/**
   Forward lazy transform (horizontal)
 */
static void opj_dwt_deinterleave_h(const int32_t * _RESTRICT a, int32_t * _RESTRICT b, int32_t dn, int32_t sn, int32_t cas);
/**
   Forward 9-7 wavelet transform in 1-D
 */
static void opj_dwt_encode_1_real(void * a, int32_t dn, int32_t sn, int32_t cas);
/**
   Explicit calculation of the Quantization Stepsizes
 */
static void opj_dwt_encode_stepsize(int32_t stepsize, int32_t numbps, opj_stepsize_t * bandno_stepsize);
/**
   Inverse wavelet transform in 2-D.
 */
static boolint opj_dwt_decode_tile(opj_thread_pool_t* tp, opj_tcd_tilecomp_t* tilec, uint32_t i);
static boolint opj_dwt_decode_partial_tile(opj_tcd_tilecomp_t* tilec, uint32_t numres);

/* Forward transform, for the vertical pass, processing cols columns */
/* where cols <= NB_ELTS_V8 */
/* Where void* is a int32_t* for 5x3 and float* for 9x7 */
typedef void (* opj_encode_and_deinterleave_v_fnptr_type)(void * array, void * tmp, uint32_t height, boolint even, uint32_t stride_width, uint32_t cols);
/* Where void* is a int32_t* for 5x3 and float* for 9x7 */
typedef void (* opj_encode_and_deinterleave_h_one_row_fnptr_type)(void * row, void * tmp, uint32_t width, boolint even);
static boolint opj_dwt_encode_procedure(opj_thread_pool_t* tp, opj_tcd_tilecomp_t * tilec, opj_encode_and_deinterleave_v_fnptr_type p_encode_and_deinterleave_v,
    opj_encode_and_deinterleave_h_one_row_fnptr_type p_encode_and_deinterleave_h_one_row);
static uint32_t opj_dwt_max_resolution(opj_tcd_resolution_t* _RESTRICT r, uint32_t i);

/* <summary>                             */
/* Inverse 9-7 wavelet transform in 1-D. */
/* </summary>                            */

/*@}*/

/*@}*/

#define OPJ_S(i) a[(i)*2]
#define OPJ_D(i) a[(1+(i)*2)]
#define OPJ_S_(i) ((i)<0 ? OPJ_S(0) : ((i)>=sn ? OPJ_S(sn-1) : OPJ_S(i)))
#define OPJ_D_(i) ((i)<0 ? OPJ_D(0) : ((i)>=dn ? OPJ_D(dn-1) : OPJ_D(i)))
/* new */
#define OPJ_SS_(i) ((i)<0 ? OPJ_S(0) : ((i)>=dn ? OPJ_S(dn-1) : OPJ_S(i)))
#define OPJ_DD_(i) ((i)<0 ? OPJ_D(0) : ((i)>=sn ? OPJ_D(sn-1) : OPJ_D(i)))

/* <summary>                                                              */
/* This table contains the norms of the 5-3 wavelets for different bands. */
/* </summary>                                                             */
/* FIXME! the array should really be extended up to 33 resolution levels */
/* See https://github.com/uclouvain/openjpeg/issues/493 */
static const double opj_dwt_norms[4][10] = {
	{1.000, 1.500, 2.750, 5.375, 10.68, 21.34, 42.67, 85.33, 170.7, 341.3},
	{1.038, 1.592, 2.919, 5.703, 11.33, 22.64, 45.25, 90.48, 180.9},
	{1.038, 1.592, 2.919, 5.703, 11.33, 22.64, 45.25, 90.48, 180.9},
	{.7186, .9218, 1.586, 3.043, 6.019, 12.01, 24.00, 47.97, 95.93}
};

/* <summary>                                                              */
/* This table contains the norms of the 9-7 wavelets for different bands. */
/* </summary>                                                             */
/* FIXME! the array should really be extended up to 33 resolution levels */
/* See https://github.com/uclouvain/openjpeg/issues/493 */
static const double opj_dwt_norms_real[4][10] = {
	{1.000, 1.965, 4.177, 8.403, 16.90, 33.84, 67.69, 135.3, 270.6, 540.9},
	{2.022, 3.989, 8.355, 17.04, 34.27, 68.63, 137.3, 274.6, 549.0},
	{2.022, 3.989, 8.355, 17.04, 34.27, 68.63, 137.3, 274.6, 549.0},
	{2.080, 3.865, 8.307, 17.18, 34.71, 69.59, 139.3, 278.6, 557.2}
};
// 
// local functions
// 
/* <summary>                             */
/* Forward lazy transform (horizontal).  */
/* </summary>                            */
static void opj_dwt_deinterleave_h(const int32_t * _RESTRICT a, int32_t * _RESTRICT b, int32_t dn, int32_t sn, int32_t cas)
{
	int32_t i;
	int32_t * _RESTRICT l_dest = b;
	const int32_t * _RESTRICT l_src = a + cas;
	for(i = 0; i < sn; ++i) {
		*l_dest++ = *l_src;
		l_src += 2;
	}
	l_dest = b + sn;
	l_src = a + 1 - cas;
	for(i = 0; i < dn; ++i) {
		*l_dest++ = *l_src;
		l_src += 2;
	}
}

#ifdef STANDARD_SLOW_VERSION
/* <summary>                             */
/* Inverse lazy transform (horizontal).  */
/* </summary>                            */
static void opj_dwt_interleave_h(const opj_dwt_t* h, int32_t * a)
{
	const int32_t * ai = a;
	int32_t * bi = h->mem + h->cas;
	int32_t i    = h->sn;
	while(i--) {
		*bi = *(ai++);
		bi += 2;
	}
	ai  = a + h->sn;
	bi  = h->mem + 1 - h->cas;
	i   = h->dn;
	while(i--) {
		*bi = *(ai++);
		bi += 2;
	}
}

/* <summary>                             */
/* Inverse lazy transform (vertical).    */
/* </summary>                            */
static void opj_dwt_interleave_v(const opj_dwt_t* v, int32_t * a, int32_t x)
{
	const int32_t * ai = a;
	int32_t * bi = v->mem + v->cas;
	int32_t i = v->sn;
	while(i--) {
		*bi = *ai;
		bi += 2;
		ai += x;
	}
	ai = a + (v->sn * (size_t)x);
	bi = v->mem + 1 - v->cas;
	i = v->dn;
	while(i--) {
		*bi = *ai;
		bi += 2;
		ai += x;
	}
}

#endif /* STANDARD_SLOW_VERSION */

#ifdef STANDARD_SLOW_VERSION
/* <summary>                            */
/* Inverse 5-3 wavelet transform in 1-D. */
/* </summary>                           */
static void opj_dwt_decode_1_(int32_t * a, int32_t dn, int32_t sn, int32_t cas)
{
	int32_t i;
	if(!cas) {
		if((dn > 0) || (sn > 1)) { /* NEW :  CASE ONE ELEMENT */
			for(i = 0; i < sn; i++) {
				OPJ_S(i) -= (OPJ_D_(i - 1) + OPJ_D_(i) + 2) >> 2;
			}
			for(i = 0; i < dn; i++) {
				OPJ_D(i) += (OPJ_S_(i) + OPJ_S_(i + 1)) >> 1;
			}
		}
	}
	else {
		if(!sn  && dn == 1) { /* NEW :  CASE ONE ELEMENT */
			OPJ_S(0) /= 2;
		}
		else {
			for(i = 0; i < sn; i++) {
				OPJ_D(i) -= (OPJ_SS_(i) + OPJ_SS_(i + 1) + 2) >> 2;
			}
			for(i = 0; i < dn; i++) {
				OPJ_S(i) += (OPJ_DD_(i) + OPJ_DD_(i - 1)) >> 1;
			}
		}
	}
}

static void opj_dwt_decode_1(const opj_dwt_t * v)
{
	opj_dwt_decode_1_(v->mem, v->dn, v->sn, v->cas);
}

#endif /* STANDARD_SLOW_VERSION */

#if !defined(STANDARD_SLOW_VERSION)
static void  opj_idwt53_h_cas0(int32_t* tmp, const int32_t sn, const int32_t len, int32_t* tiledp)
{
	int32_t i, j;
	const int32_t* in_even = &tiledp[0];
	const int32_t* in_odd = &tiledp[sn];
#ifdef TWO_PASS_VERSION
	/* For documentation purpose: performs lifting in two iterations, */
	/* but without explicit interleaving */
	assert(len > 1);
	/* Even */
	tmp[0] = in_even[0] - ((in_odd[0] + 1) >> 1);
	for(i = 2, j = 0; i <= len - 2; i += 2, j++) {
		tmp[i] = in_even[j + 1] - ((in_odd[j] + in_odd[j + 1] + 2) >> 2);
	}
	if(len & 1) { /* if len is odd */
		tmp[len - 1] = in_even[(len - 1) / 2] - ((in_odd[(len - 2) / 2] + 1) >> 1);
	}

	/* Odd */
	for(i = 1, j = 0; i < len - 1; i += 2, j++) {
		tmp[i] = in_odd[j] + ((tmp[i - 1] + tmp[i + 1]) >> 1);
	}
	if(!(len & 1)) { /* if len is even */
		tmp[len - 1] = in_odd[(len - 1) / 2] + tmp[len - 2];
	}
#else
	int32_t d1c, d1n, s1n, s0c, s0n;
	assert(len > 1);
	/* Improved version of the TWO_PASS_VERSION: */
	/* Performs lifting in one single iteration. Saves memory */
	/* accesses and explicit interleaving. */
	s1n = in_even[0];
	d1n = in_odd[0];
	s0n = s1n - ((d1n + 1) >> 1);
	for(i = 0, j = 1; i < (len - 3); i += 2, j++) {
		d1c = d1n;
		s0c = s0n;
		s1n = in_even[j];
		d1n = in_odd[j];
		s0n = s1n - ((d1c + d1n + 2) >> 2);
		tmp[i  ] = s0c;
		tmp[i + 1] = opj_int_add_no_overflow(d1c, opj_int_add_no_overflow(s0c, s0n) >> 1);
	}
	tmp[i] = s0n;
	if(len & 1) {
		tmp[len - 1] = in_even[(len - 1) / 2] - ((d1n + 1) >> 1);
		tmp[len - 2] = d1n + ((s0n + tmp[len - 1]) >> 1);
	}
	else {
		tmp[len - 1] = d1n + s0n;
	}
#endif
	memcpy(tiledp, tmp, (uint32_t)len * sizeof(int32_t));
}

static void  opj_idwt53_h_cas1(int32_t* tmp, const int32_t sn, const int32_t len, int32_t* tiledp)
{
	int32_t i, j;
	const int32_t* in_even = &tiledp[sn];
	const int32_t* in_odd = &tiledp[0];
#ifdef TWO_PASS_VERSION
	/* For documentation purpose: performs lifting in two iterations, */
	/* but without explicit interleaving */
	assert(len > 2);
	/* Odd */
	for(i = 1, j = 0; i < len - 1; i += 2, j++) {
		tmp[i] = in_odd[j] - ((in_even[j] + in_even[j + 1] + 2) >> 2);
	}
	if(!(len & 1)) {
		tmp[len - 1] = in_odd[len / 2 - 1] - ((in_even[len / 2 - 1] + 1) >> 1);
	}

	/* Even */
	tmp[0] = in_even[0] + tmp[1];
	for(i = 2, j = 1; i < len - 1; i += 2, j++) {
		tmp[i] = in_even[j] + ((tmp[i + 1] + tmp[i - 1]) >> 1);
	}
	if(len & 1) {
		tmp[len - 1] = in_even[len / 2] + tmp[len - 2];
	}
#else
	int32_t s1, s2, dc, dn;

	assert(len > 2);

	/* Improved version of the TWO_PASS_VERSION: */
	/* Performs lifting in one single iteration. Saves memory */
	/* accesses and explicit interleaving. */

	s1 = in_even[1];
	dc = in_odd[0] - ((in_even[0] + s1 + 2) >> 2);
	tmp[0] = in_even[0] + dc;

	for(i = 1, j = 1; i < (len - 2 - !(len & 1)); i += 2, j++) {
		s2 = in_even[j + 1];
		dn = in_odd[j] - ((s1 + s2 + 2) >> 2);
		tmp[i  ] = dc;
		tmp[i + 1] = opj_int_add_no_overflow(s1, opj_int_add_no_overflow(dn, dc) >> 1);
		dc = dn;
		s1 = s2;
	}
	tmp[i] = dc;
	if(!(len & 1)) {
		dn = in_odd[len / 2 - 1] - ((s1 + 1) >> 1);
		tmp[len - 2] = s1 + ((dn + dc) >> 1);
		tmp[len - 1] = dn;
	}
	else {
		tmp[len - 1] = s1 + dc;
	}
#endif
	memcpy(tiledp, tmp, (uint32_t)len * sizeof(int32_t));
}

#endif /* !defined(STANDARD_SLOW_VERSION) */

/* <summary>                            */
/* Inverse 5-3 wavelet transform in 1-D for one row. */
/* </summary>                           */
/* Performs interleave, inverse wavelet transform and copy back to buffer */
static void opj_idwt53_h(const opj_dwt_t * dwt, int32_t* tiledp)
{
#ifdef STANDARD_SLOW_VERSION
	/* For documentation purpose */
	opj_dwt_interleave_h(dwt, tiledp);
	opj_dwt_decode_1(dwt);
	memcpy(tiledp, dwt->mem, (uint32_t)(dwt->sn + dwt->dn) * sizeof(int32_t));
#else
	const int32_t sn = dwt->sn;
	const int32_t len = sn + dwt->dn;
	if(dwt->cas == 0) { /* Left-most sample is on even coordinate */
		if(len > 1) {
			opj_idwt53_h_cas0(dwt->mem, sn, len, tiledp);
		}
		else {
			/* Unmodified value */
		}
	}
	else { /* Left-most sample is on odd coordinate */
		if(len == 1) {
			tiledp[0] /= 2;
		}
		else if(len == 2) {
			int32_t* out = dwt->mem;
			const int32_t* in_even = &tiledp[sn];
			const int32_t* in_odd = &tiledp[0];
			out[1] = in_odd[0] - ((in_even[0] + 1) >> 1);
			out[0] = in_even[0] + out[1];
			memcpy(tiledp, dwt->mem, (uint32_t)len * sizeof(int32_t));
		}
		else if(len > 2) {
			opj_idwt53_h_cas1(dwt->mem, sn, len, tiledp);
		}
	}
#endif
}

#if (defined(__SSE2__) || defined(__AVX2__)) && !defined(STANDARD_SLOW_VERSION)

/* Conveniency macros to improve the readability of the formulas */
#if __AVX2__
	#define VREG        __m256i
	#define LOAD_CST(x) _mm256_set1_epi32(x)
	#define LOAD(x)     _mm256_load_si256((const VREG*)(x))
	#define LOADU(x)    _mm256_loadu_si256((const VREG*)(x))
	#define STORE(x, y)  _mm256_store_si256((VREG*)(x), (y))
	#define STOREU(x, y) _mm256_storeu_si256((VREG*)(x), (y))
	#define ADD(x, y)    _mm256_add_epi32((x), (y))
	#define SUB(x, y)    _mm256_sub_epi32((x), (y))
	#define SAR(x, y)    _mm256_srai_epi32((x), (y))
#else
	#define VREG        __m128i
	#define LOAD_CST(x) _mm_set1_epi32(x)
	#define LOAD(x)     _mm_load_si128((const VREG*)(x))
	#define LOADU(x)    _mm_loadu_si128((const VREG*)(x))
	#define STORE(x, y)  _mm_store_si128((VREG*)(x), (y))
	#define STOREU(x, y) _mm_storeu_si128((VREG*)(x), (y))
	#define ADD(x, y)    _mm_add_epi32((x), (y))
	#define SUB(x, y)    _mm_sub_epi32((x), (y))
	#define SAR(x, y)    _mm_srai_epi32((x), (y))
#endif
#define ADD3(x, y, z) ADD(ADD(x, y), z)

static void opj_idwt53_v_final_memcpy(int32_t* tiledp_col, const int32_t* tmp, int32_t len, size_t stride)
{
	int32_t i;
	for(i = 0; i < len; ++i) {
		/* A memcpy(&tiledp_col[i * stride + 0],
		            &tmp[PARALLEL_COLS_53 * i + 0],
		            PARALLEL_COLS_53 * sizeof(int32_t))
		   would do but would be a tiny bit slower.
		   We can take here advantage of our knowledge of alignment */
		STOREU(&tiledp_col[(size_t)i * stride + 0], LOAD(&tmp[PARALLEL_COLS_53 * i + 0]));
		STOREU(&tiledp_col[(size_t)i * stride + VREG_INT_COUNT], LOAD(&tmp[PARALLEL_COLS_53 * i + VREG_INT_COUNT]));
	}
}

/** Vertical inverse 5x3 wavelet transform for 8 columns in SSE2, or
 * 16 in AVX2, when top-most pixel is on even coordinate */
static void opj_idwt53_v_cas0_mcols_SSE2_OR_AVX2(int32_t* tmp, const int32_t sn, const int32_t len, int32_t* tiledp_col, const size_t stride)
{
	const int32_t* in_even = &tiledp_col[0];
	const int32_t* in_odd = &tiledp_col[(size_t)sn * stride];
	int32_t i;
	size_t j;
	VREG d1c_0, d1n_0, s1n_0, s0c_0, s0n_0;
	VREG d1c_1, d1n_1, s1n_1, s0c_1, s0n_1;
	const VREG two = LOAD_CST(2);
	assert(len > 1);
#if __AVX2__
	assert(PARALLEL_COLS_53 == 16);
	assert(VREG_INT_COUNT == 8);
#else
	assert(PARALLEL_COLS_53 == 8);
	assert(VREG_INT_COUNT == 4);
#endif

	/* Note: loads of input even/odd values must be done in a unaligned */
	/* fashion. But stores in tmp can be done with aligned store, since */
	/* the temporary buffer is properly aligned */
	assert((size_t)tmp % (sizeof(int32_t) * VREG_INT_COUNT) == 0);

	s1n_0 = LOADU(in_even + 0);
	s1n_1 = LOADU(in_even + VREG_INT_COUNT);
	d1n_0 = LOADU(in_odd);
	d1n_1 = LOADU(in_odd + VREG_INT_COUNT);

	/* s0n = s1n - ((d1n + 1) >> 1); <==> */
	/* s0n = s1n - ((d1n + d1n + 2) >> 2); */
	s0n_0 = SUB(s1n_0, SAR(ADD3(d1n_0, d1n_0, two), 2));
	s0n_1 = SUB(s1n_1, SAR(ADD3(d1n_1, d1n_1, two), 2));

	for(i = 0, j = 1; i < (len - 3); i += 2, j++) {
		d1c_0 = d1n_0;
		s0c_0 = s0n_0;
		d1c_1 = d1n_1;
		s0c_1 = s0n_1;

		s1n_0 = LOADU(in_even + j * stride);
		s1n_1 = LOADU(in_even + j * stride + VREG_INT_COUNT);
		d1n_0 = LOADU(in_odd + j * stride);
		d1n_1 = LOADU(in_odd + j * stride + VREG_INT_COUNT);

		/*s0n = s1n - ((d1c + d1n + 2) >> 2);*/
		s0n_0 = SUB(s1n_0, SAR(ADD3(d1c_0, d1n_0, two), 2));
		s0n_1 = SUB(s1n_1, SAR(ADD3(d1c_1, d1n_1, two), 2));

		STORE(tmp + PARALLEL_COLS_53 * (i + 0), s0c_0);
		STORE(tmp + PARALLEL_COLS_53 * (i + 0) + VREG_INT_COUNT, s0c_1);

		/* d1c + ((s0c + s0n) >> 1) */
		STORE(tmp + PARALLEL_COLS_53 * (i + 1) + 0, ADD(d1c_0, SAR(ADD(s0c_0, s0n_0), 1)));
		STORE(tmp + PARALLEL_COLS_53 * (i + 1) + VREG_INT_COUNT, ADD(d1c_1, SAR(ADD(s0c_1, s0n_1), 1)));
	}
	STORE(tmp + PARALLEL_COLS_53 * (i + 0) + 0, s0n_0);
	STORE(tmp + PARALLEL_COLS_53 * (i + 0) + VREG_INT_COUNT, s0n_1);
	if(len & 1) {
		VREG tmp_len_minus_1;
		s1n_0 = LOADU(in_even + (size_t)((len - 1) / 2) * stride);
		/* tmp_len_minus_1 = s1n - ((d1n + 1) >> 1); */
		tmp_len_minus_1 = SUB(s1n_0, SAR(ADD3(d1n_0, d1n_0, two), 2));
		STORE(tmp + PARALLEL_COLS_53 * (len - 1), tmp_len_minus_1);
		/* d1n + ((s0n + tmp_len_minus_1) >> 1) */
		STORE(tmp + PARALLEL_COLS_53 * (len - 2),
		    ADD(d1n_0, SAR(ADD(s0n_0, tmp_len_minus_1), 1)));

		s1n_1 = LOADU(in_even + (size_t)((len - 1) / 2) * stride + VREG_INT_COUNT);
		/* tmp_len_minus_1 = s1n - ((d1n + 1) >> 1); */
		tmp_len_minus_1 = SUB(s1n_1, SAR(ADD3(d1n_1, d1n_1, two), 2));
		STORE(tmp + PARALLEL_COLS_53 * (len - 1) + VREG_INT_COUNT,
		    tmp_len_minus_1);
		/* d1n + ((s0n + tmp_len_minus_1) >> 1) */
		STORE(tmp + PARALLEL_COLS_53 * (len - 2) + VREG_INT_COUNT,
		    ADD(d1n_1, SAR(ADD(s0n_1, tmp_len_minus_1), 1)));
	}
	else {
		STORE(tmp + PARALLEL_COLS_53 * (len - 1) + 0,
		    ADD(d1n_0, s0n_0));
		STORE(tmp + PARALLEL_COLS_53 * (len - 1) + VREG_INT_COUNT,
		    ADD(d1n_1, s0n_1));
	}

	opj_idwt53_v_final_memcpy(tiledp_col, tmp, len, stride);
}

/** Vertical inverse 5x3 wavelet transform for 8 columns in SSE2, or
 * 16 in AVX2, when top-most pixel is on odd coordinate */
static void opj_idwt53_v_cas1_mcols_SSE2_OR_AVX2(int32_t* tmp,
    const int32_t sn,
    const int32_t len,
    int32_t* tiledp_col,
    const size_t stride)
{
	int32_t i;
	size_t j;

	VREG s1_0, s2_0, dc_0, dn_0;
	VREG s1_1, s2_1, dc_1, dn_1;
	const VREG two = LOAD_CST(2);

	const int32_t* in_even = &tiledp_col[(size_t)sn * stride];
	const int32_t* in_odd = &tiledp_col[0];

	assert(len > 2);
#if __AVX2__
	assert(PARALLEL_COLS_53 == 16);
	assert(VREG_INT_COUNT == 8);
#else
	assert(PARALLEL_COLS_53 == 8);
	assert(VREG_INT_COUNT == 4);
#endif

	/* Note: loads of input even/odd values must be done in a unaligned */
	/* fashion. But stores in tmp can be done with aligned store, since */
	/* the temporary buffer is properly aligned */
	assert((size_t)tmp % (sizeof(int32_t) * VREG_INT_COUNT) == 0);

	s1_0 = LOADU(in_even + stride);
	/* in_odd[0] - ((in_even[0] + s1 + 2) >> 2); */
	dc_0 = SUB(LOADU(in_odd + 0),
		SAR(ADD3(LOADU(in_even + 0), s1_0, two), 2));
	STORE(tmp + PARALLEL_COLS_53 * 0, ADD(LOADU(in_even + 0), dc_0));

	s1_1 = LOADU(in_even + stride + VREG_INT_COUNT);
	/* in_odd[0] - ((in_even[0] + s1 + 2) >> 2); */
	dc_1 = SUB(LOADU(in_odd + VREG_INT_COUNT),
		SAR(ADD3(LOADU(in_even + VREG_INT_COUNT), s1_1, two), 2));
	STORE(tmp + PARALLEL_COLS_53 * 0 + VREG_INT_COUNT,
	    ADD(LOADU(in_even + VREG_INT_COUNT), dc_1));

	for(i = 1, j = 1; i < (len - 2 - !(len & 1)); i += 2, j++) {
		s2_0 = LOADU(in_even + (j + 1) * stride);
		s2_1 = LOADU(in_even + (j + 1) * stride + VREG_INT_COUNT);

		/* dn = in_odd[j * stride] - ((s1 + s2 + 2) >> 2); */
		dn_0 = SUB(LOADU(in_odd + j * stride),
			SAR(ADD3(s1_0, s2_0, two), 2));
		dn_1 = SUB(LOADU(in_odd + j * stride + VREG_INT_COUNT),
			SAR(ADD3(s1_1, s2_1, two), 2));

		STORE(tmp + PARALLEL_COLS_53 * i, dc_0);
		STORE(tmp + PARALLEL_COLS_53 * i + VREG_INT_COUNT, dc_1);

		/* tmp[i + 1] = s1 + ((dn + dc) >> 1); */
		STORE(tmp + PARALLEL_COLS_53 * (i + 1) + 0,
		    ADD(s1_0, SAR(ADD(dn_0, dc_0), 1)));
		STORE(tmp + PARALLEL_COLS_53 * (i + 1) + VREG_INT_COUNT,
		    ADD(s1_1, SAR(ADD(dn_1, dc_1), 1)));

		dc_0 = dn_0;
		s1_0 = s2_0;
		dc_1 = dn_1;
		s1_1 = s2_1;
	}
	STORE(tmp + PARALLEL_COLS_53 * i, dc_0);
	STORE(tmp + PARALLEL_COLS_53 * i + VREG_INT_COUNT, dc_1);

	if(!(len & 1)) {
		/*dn = in_odd[(len / 2 - 1) * stride] - ((s1 + 1) >> 1); */
		dn_0 = SUB(LOADU(in_odd + (size_t)(len / 2 - 1) * stride),
			SAR(ADD3(s1_0, s1_0, two), 2));
		dn_1 = SUB(LOADU(in_odd + (size_t)(len / 2 - 1) * stride + VREG_INT_COUNT),
			SAR(ADD3(s1_1, s1_1, two), 2));

		/* tmp[len - 2] = s1 + ((dn + dc) >> 1); */
		STORE(tmp + PARALLEL_COLS_53 * (len - 2) + 0,
		    ADD(s1_0, SAR(ADD(dn_0, dc_0), 1)));
		STORE(tmp + PARALLEL_COLS_53 * (len - 2) + VREG_INT_COUNT,
		    ADD(s1_1, SAR(ADD(dn_1, dc_1), 1)));

		STORE(tmp + PARALLEL_COLS_53 * (len - 1) + 0, dn_0);
		STORE(tmp + PARALLEL_COLS_53 * (len - 1) + VREG_INT_COUNT, dn_1);
	}
	else {
		STORE(tmp + PARALLEL_COLS_53 * (len - 1) + 0, ADD(s1_0, dc_0));
		STORE(tmp + PARALLEL_COLS_53 * (len - 1) + VREG_INT_COUNT,
		    ADD(s1_1, dc_1));
	}

	opj_idwt53_v_final_memcpy(tiledp_col, tmp, len, stride);
}

#undef VREG
#undef LOAD_CST
#undef LOADU
#undef LOAD
#undef STORE
#undef STOREU
#undef ADD
#undef ADD3
#undef SUB
#undef SAR

#endif /* (defined(__SSE2__) || defined(__AVX2__)) && !defined(STANDARD_SLOW_VERSION) */

#if !defined(STANDARD_SLOW_VERSION)
/** Vertical inverse 5x3 wavelet transform for one column, when top-most
 * pixel is on even coordinate */
static void opj_idwt3_v_cas0(int32_t* tmp,
    const int32_t sn,
    const int32_t len,
    int32_t* tiledp_col,
    const size_t stride)
{
	int32_t i, j;
	int32_t d1c, d1n, s1n, s0c, s0n;

	assert(len > 1);

	/* Performs lifting in one single iteration. Saves memory */
	/* accesses and explicit interleaving. */

	s1n = tiledp_col[0];
	d1n = tiledp_col[(size_t)sn * stride];
	s0n = s1n - ((d1n + 1) >> 1);

	for(i = 0, j = 0; i < (len - 3); i += 2, j++) {
		d1c = d1n;
		s0c = s0n;

		s1n = tiledp_col[(size_t)(j + 1) * stride];
		d1n = tiledp_col[(size_t)(sn + j + 1) * stride];

		s0n = opj_int_sub_no_overflow(s1n,
			opj_int_add_no_overflow(opj_int_add_no_overflow(d1c, d1n), 2) >> 2);

		tmp[i  ] = s0c;
		tmp[i + 1] = opj_int_add_no_overflow(d1c, opj_int_add_no_overflow(s0c,
			s0n) >> 1);
	}

	tmp[i] = s0n;

	if(len & 1) {
		tmp[len - 1] =
		    tiledp_col[(size_t)((len - 1) / 2) * stride] -
		    ((d1n + 1) >> 1);
		tmp[len - 2] = d1n + ((s0n + tmp[len - 1]) >> 1);
	}
	else {
		tmp[len - 1] = d1n + s0n;
	}

	for(i = 0; i < len; ++i) {
		tiledp_col[(size_t)i * stride] = tmp[i];
	}
}

/** Vertical inverse 5x3 wavelet transform for one column, when top-most
 * pixel is on odd coordinate */
static void opj_idwt3_v_cas1(int32_t* tmp,
    const int32_t sn,
    const int32_t len,
    int32_t* tiledp_col,
    const size_t stride)
{
	int32_t i, j;
	int32_t s1, s2, dc, dn;
	const int32_t* in_even = &tiledp_col[(size_t)sn * stride];
	const int32_t* in_odd = &tiledp_col[0];

	assert(len > 2);

	/* Performs lifting in one single iteration. Saves memory */
	/* accesses and explicit interleaving. */

	s1 = in_even[stride];
	dc = in_odd[0] - ((in_even[0] + s1 + 2) >> 2);
	tmp[0] = in_even[0] + dc;
	for(i = 1, j = 1; i < (len - 2 - !(len & 1)); i += 2, j++) {
		s2 = in_even[(size_t)(j + 1) * stride];

		dn = in_odd[(size_t)j * stride] - ((s1 + s2 + 2) >> 2);
		tmp[i  ] = dc;
		tmp[i + 1] = s1 + ((dn + dc) >> 1);

		dc = dn;
		s1 = s2;
	}
	tmp[i] = dc;
	if(!(len & 1)) {
		dn = in_odd[(size_t)(len / 2 - 1) * stride] - ((s1 + 1) >> 1);
		tmp[len - 2] = s1 + ((dn + dc) >> 1);
		tmp[len - 1] = dn;
	}
	else {
		tmp[len - 1] = s1 + dc;
	}

	for(i = 0; i < len; ++i) {
		tiledp_col[(size_t)i * stride] = tmp[i];
	}
}

#endif /* !defined(STANDARD_SLOW_VERSION) */

/* <summary>                            */
/* Inverse vertical 5-3 wavelet transform in 1-D for several columns. */
/* </summary>                           */
/* Performs interleave, inverse wavelet transform and copy back to buffer */
static void opj_idwt53_v(const opj_dwt_t * dwt,
    int32_t* tiledp_col,
    size_t stride,
    int32_t nb_cols)
{
#ifdef STANDARD_SLOW_VERSION
	/* For documentation purpose */
	int32_t k, c;
	for(c = 0; c < nb_cols; c++) {
		opj_dwt_interleave_v(dwt, tiledp_col + c, stride);
		opj_dwt_decode_1(dwt);
		for(k = 0; k < dwt->sn + dwt->dn; ++k) {
			tiledp_col[c + k * stride] = dwt->mem[k];
		}
	}
#else
	const int32_t sn = dwt->sn;
	const int32_t len = sn + dwt->dn;
	if(dwt->cas == 0) {
		/* If len == 1, unmodified value */
#if (defined(__SSE2__) || defined(__AVX2__))
		if(len > 1 && nb_cols == PARALLEL_COLS_53) {
			/* Same as below general case, except that thanks to SSE2/AVX2 */
			/* we can efficiently process 8/16 columns in parallel */
			opj_idwt53_v_cas0_mcols_SSE2_OR_AVX2(dwt->mem, sn, len, tiledp_col, stride);
			return;
		}
#endif
		if(len > 1) {
			int32_t c;
			for(c = 0; c < nb_cols; c++, tiledp_col++) {
				opj_idwt3_v_cas0(dwt->mem, sn, len, tiledp_col, stride);
			}
			return;
		}
	}
	else {
		if(len == 1) {
			int32_t c;
			for(c = 0; c < nb_cols; c++, tiledp_col++) {
				tiledp_col[0] /= 2;
			}
			return;
		}

		if(len == 2) {
			int32_t c;
			int32_t* out = dwt->mem;
			for(c = 0; c < nb_cols; c++, tiledp_col++) {
				int32_t i;
				const int32_t* in_even = &tiledp_col[(size_t)sn * stride];
				const int32_t* in_odd = &tiledp_col[0];

				out[1] = in_odd[0] - ((in_even[0] + 1) >> 1);
				out[0] = in_even[0] + out[1];

				for(i = 0; i < len; ++i) {
					tiledp_col[(size_t)i * stride] = out[i];
				}
			}

			return;
		}

#if (defined(__SSE2__) || defined(__AVX2__))
		if(len > 2 && nb_cols == PARALLEL_COLS_53) {
			/* Same as below general case, except that thanks to SSE2/AVX2 */
			/* we can efficiently process 8/16 columns in parallel */
			opj_idwt53_v_cas1_mcols_SSE2_OR_AVX2(dwt->mem, sn, len, tiledp_col, stride);
			return;
		}
#endif
		if(len > 2) {
			int32_t c;
			for(c = 0; c < nb_cols; c++, tiledp_col++) {
				opj_idwt3_v_cas1(dwt->mem, sn, len, tiledp_col, stride);
			}
			return;
		}
	}
#endif
}

#if 0
static void opj_dwt_encode_step1(float* fw,
    uint32_t end,
    const float c)
{
	uint32_t i = 0;
	for(; i < end; ++i) {
		fw[0] *= c;
		fw += 2;
	}
}

#else
static void opj_dwt_encode_step1_combined(float* fw,
    uint32_t iters_c1,
    uint32_t iters_c2,
    const float c1,
    const float c2)
{
	uint32_t i = 0;
	const uint32_t iters_common =  smin(iters_c1, iters_c2);
	assert((((size_t)fw) & 0xf) == 0);
	assert(opj_int_abs((int32_t)iters_c1 - (int32_t)iters_c2) <= 1);
	for(; i + 3 < iters_common; i += 4) {
#ifdef __SSE__
		const __m128 vcst = _mm_set_ps(c2, c1, c2, c1);
		*(__m128*)fw = _mm_mul_ps(*(__m128*)fw, vcst);
		*(__m128*)(fw + 4) = _mm_mul_ps(*(__m128*)(fw + 4), vcst);
#else
		fw[0] *= c1;
		fw[1] *= c2;
		fw[2] *= c1;
		fw[3] *= c2;
		fw[4] *= c1;
		fw[5] *= c2;
		fw[6] *= c1;
		fw[7] *= c2;
#endif
		fw += 8;
	}
	for(; i < iters_common; i++) {
		fw[0] *= c1;
		fw[1] *= c2;
		fw += 2;
	}
	if(i < iters_c1) {
		fw[0] *= c1;
	}
	else if(i < iters_c2) {
		fw[1] *= c2;
	}
}

#endif

static void opj_dwt_encode_step2(float* fl, float* fw,
    uint32_t end,
    uint32_t m,
    float c)
{
	uint32_t i;
	uint32_t imax = smin(end, m);
	if(imax > 0) {
		fw[-1] += (fl[0] + fw[0]) * c;
		fw += 2;
		i = 1;
		for(; i + 3 < imax; i += 4) {
			fw[-1] += (fw[-2] + fw[0]) * c;
			fw[1] += (fw[0] + fw[2]) * c;
			fw[3] += (fw[2] + fw[4]) * c;
			fw[5] += (fw[4] + fw[6]) * c;
			fw += 8;
		}
		for(; i < imax; ++i) {
			fw[-1] += (fw[-2] + fw[0]) * c;
			fw += 2;
		}
	}
	if(m < end) {
		assert(m + 1 == end);
		fw[-1] += (2 * fw[-2]) * c;
	}
}

static void opj_dwt_encode_1_real(void * aIn, int32_t dn, int32_t sn,
    int32_t cas)
{
	float* w = (float*)aIn;
	int32_t a, b;
	assert(dn + sn > 1);
	if(cas == 0) {
		a = 0;
		b = 1;
	}
	else {
		a = 1;
		b = 0;
	}
	opj_dwt_encode_step2(w + a, w + b + 1, (uint32_t)dn, (uint32_t)smin(dn, sn - b), opj_dwt_alpha);
	opj_dwt_encode_step2(w + b, w + a + 1, (uint32_t)sn, (uint32_t)smin(sn, dn - a), opj_dwt_beta);
	opj_dwt_encode_step2(w + a, w + b + 1, (uint32_t)dn, (uint32_t)smin(dn, sn - b), opj_dwt_gamma);
	opj_dwt_encode_step2(w + b, w + a + 1, (uint32_t)sn, (uint32_t)smin(sn, dn - a), opj_dwt_delta);
#if 0
	opj_dwt_encode_step1(w + b, (uint32_t)dn, opj_K);
	opj_dwt_encode_step1(w + a, (uint32_t)sn, opj_invK);
#else
	if(!a) {
		opj_dwt_encode_step1_combined(w, (uint32_t)sn, (uint32_t)dn, opj_invK, opj_K);
	}
	else {
		opj_dwt_encode_step1_combined(w, (uint32_t)dn, (uint32_t)sn, opj_K, opj_invK);
	}
#endif
}

static void opj_dwt_encode_stepsize(int32_t stepsize, int32_t numbps, opj_stepsize_t * bandno_stepsize)
{
	int32_t p = opj_int_floorlog2(stepsize) - 13;
	int32_t n = 11 - opj_int_floorlog2(stepsize);
	bandno_stepsize->mant = (n < 0 ? stepsize >> -n : stepsize << n) & 0x7ff;
	bandno_stepsize->expn = numbps - p;
}
// 
// DWT interface
// 
/** Process one line for the horizontal pass of the 5x3 forward transform */
static void opj_dwt_encode_and_deinterleave_h_one_row(void* rowIn, void* tmpIn, uint32_t width, boolint even)
{
	int32_t* _RESTRICT row = (int32_t*)rowIn;
	int32_t* _RESTRICT tmp = (int32_t*)tmpIn;
	const int32_t sn = (int32_t)((width + (even ? 1 : 0)) >> 1);
	const int32_t dn = (int32_t)(width - (uint32_t)sn);
	if(even) {
		if(width > 1) {
			int32_t i;
			for(i = 0; i < sn - 1; i++) {
				tmp[sn + i] = row[2 * i + 1] - ((row[(i) * 2] + row[(i + 1) * 2]) >> 1);
			}
			if((width % 2) == 0) {
				tmp[sn + i] = row[2 * i + 1] - row[(i) * 2];
			}
			row[0] += (tmp[sn] + tmp[sn] + 2) >> 2;
			for(i = 1; i < dn; i++) {
				row[i] = row[2 * i] + ((tmp[sn + (i - 1)] + tmp[sn + i] + 2) >> 2);
			}
			if((width % 2) == 1) {
				row[i] = row[2 * i] + ((tmp[sn + (i - 1)] + tmp[sn + (i - 1)] + 2) >> 2);
			}
			memcpy(row + sn, tmp + sn, (size_t)dn * sizeof(int32_t));
		}
	}
	else {
		if(width == 1) {
			row[0] *= 2;
		}
		else {
			int32_t i;
			tmp[sn + 0] = row[0] - row[1];
			for(i = 1; i < sn; i++) {
				tmp[sn + i] = row[2 * i] - ((row[2 * i + 1] + row[2 * (i - 1) + 1]) >> 1);
			}
			if((width % 2) == 1) {
				tmp[sn + i] = row[2 * i] - row[2 * (i - 1) + 1];
			}
			for(i = 0; i < dn - 1; i++) {
				row[i] = row[2 * i + 1] + ((tmp[sn + i] + tmp[sn + i + 1] + 2) >> 2);
			}
			if((width % 2) == 0) {
				row[i] = row[2 * i + 1] + ((tmp[sn + i] + tmp[sn + i] + 2) >> 2);
			}
			memcpy(row + sn, tmp + sn, (size_t)dn * sizeof(int32_t));
		}
	}
}

/** Process one line for the horizontal pass of the 9x7 forward transform */
static void opj_dwt_encode_and_deinterleave_h_one_row_real(void* rowIn, void* tmpIn, uint32_t width, boolint even)
{
	float* _RESTRICT row = (float*)rowIn;
	float* _RESTRICT tmp = (float*)tmpIn;
	const int32_t sn = (int32_t)((width + (even ? 1 : 0)) >> 1);
	const int32_t dn = (int32_t)(width - (uint32_t)sn);
	if(width == 1) {
		return;
	}
	memcpy(tmp, row, width * sizeof(float));
	opj_dwt_encode_1_real(tmp, dn, sn, even ? 0 : 1);
	opj_dwt_deinterleave_h((int32_t * _RESTRICT)tmp, (int32_t * _RESTRICT)row, dn, sn, even ? 0 : 1);
}

typedef struct {
	opj_dwt_t h;
	uint32_t rw; /* Width of the resolution to process */
	uint32_t w; /* Width of tiledp */
	int32_t * _RESTRICT tiledp;
	uint32_t min_j;
	uint32_t max_j;
	opj_encode_and_deinterleave_h_one_row_fnptr_type p_function;
} opj_dwt_encode_h_job_t;

static void opj_dwt_encode_h_func(void* user_data, opj_tls_t* tls)
{
	opj_dwt_encode_h_job_t * job = (opj_dwt_encode_h_job_t*)user_data;
	(void)tls;
	for(uint32_t j = job->min_j; j < job->max_j; j++) {
		int32_t* _RESTRICT aj = job->tiledp + j * job->w;
		(*job->p_function)(aj, job->h.mem, job->rw, job->h.cas == 0 ? TRUE : FALSE);
	}
	opj_aligned_free(job->h.mem);
	SAlloc::F(job);
}

typedef struct {
	opj_dwt_t v;
	uint32_t rh;
	uint32_t w;
	int32_t * _RESTRICT tiledp;
	uint32_t min_j;
	uint32_t max_j;
	opj_encode_and_deinterleave_v_fnptr_type p_encode_and_deinterleave_v;
} opj_dwt_encode_v_job_t;

static void opj_dwt_encode_v_func(void* user_data, opj_tls_t* tls)
{
	uint32_t j;
	opj_dwt_encode_v_job_t * job = (opj_dwt_encode_v_job_t*)user_data;
	(void)tls;
	for(j = job->min_j; j + NB_ELTS_V8 - 1 < job->max_j; j += NB_ELTS_V8) {
		(*job->p_encode_and_deinterleave_v)(job->tiledp + j, job->v.mem, job->rh, job->v.cas == 0, job->w, NB_ELTS_V8);
	}
	if(j < job->max_j) {
		(*job->p_encode_and_deinterleave_v)(job->tiledp + j, job->v.mem, job->rh, job->v.cas == 0, job->w, job->max_j - j);
	}
	opj_aligned_free(job->v.mem);
	SAlloc::F(job);
}

/** Fetch up to cols <= NB_ELTS_V8 for each line, and put them in tmpOut */
/* that has a NB_ELTS_V8 interleave factor. */
static void opj_dwt_fetch_cols_vertical_pass(const void * arrayIn, void * tmpOut, uint32_t height, uint32_t stride_width, uint32_t cols)
{
	const int32_t* _RESTRICT array = (const int32_t * _RESTRICT)arrayIn;
	int32_t* _RESTRICT tmp = (int32_t * _RESTRICT)tmpOut;
	if(cols == NB_ELTS_V8) {
		for(uint32_t k = 0; k < height; ++k) {
			memcpy(tmp + NB_ELTS_V8 * k, array + k * stride_width, NB_ELTS_V8 * sizeof(int32_t));
		}
	}
	else {
		for(uint32_t k = 0; k < height; ++k) {
			uint32_t c;
			for(c = 0; c < cols; c++) {
				tmp[NB_ELTS_V8 * k + c] = array[c + k * stride_width];
			}
			for(; c < NB_ELTS_V8; c++) {
				tmp[NB_ELTS_V8 * k + c] = 0;
			}
		}
	}
}

/* Deinterleave result of forward transform, where cols <= NB_ELTS_V8 */
/* and src contains NB_ELTS_V8 consecutive values for up to NB_ELTS_V8 */
/* columns. */
static INLINE void opj_dwt_deinterleave_v_cols(const int32_t * _RESTRICT src, int32_t * _RESTRICT dst, int32_t dn, int32_t sn, uint32_t stride_width, int32_t cas, uint32_t cols)
{
	int32_t k;
	int32_t i = sn;
	int32_t * _RESTRICT l_dest = dst;
	const int32_t * _RESTRICT l_src = src + cas * NB_ELTS_V8;
	uint32_t c;
	for(k = 0; k < 2; k++) {
		while(i--) {
			if(cols == NB_ELTS_V8) {
				memcpy(l_dest, l_src, NB_ELTS_V8 * sizeof(int32_t));
			}
			else {
				c = 0;
				switch(cols) {
					case 7:
					    l_dest[c] = l_src[c];
					    c++; /* fallthru */
					case 6:
					    l_dest[c] = l_src[c];
					    c++; /* fallthru */
					case 5:
					    l_dest[c] = l_src[c];
					    c++; /* fallthru */
					case 4:
					    l_dest[c] = l_src[c];
					    c++; /* fallthru */
					case 3:
					    l_dest[c] = l_src[c];
					    c++; /* fallthru */
					case 2:
					    l_dest[c] = l_src[c];
					    c++; /* fallthru */
					default:
					    l_dest[c] = l_src[c];
					    break;
				}
			}
			l_dest += stride_width;
			l_src += 2 * NB_ELTS_V8;
		}
		l_dest = dst + (size_t)sn * (size_t)stride_width;
		l_src = src + (1 - cas) * NB_ELTS_V8;
		i = dn;
	}
}
//
// Forward 5-3 transform, for the vertical pass, processing cols columns  where cols <= NB_ELTS_V8 
//
static void opj_dwt_encode_and_deinterleave_v(void * arrayIn, void * tmpIn, uint32_t height, boolint even, uint32_t stride_width, uint32_t cols)
{
	int32_t* _RESTRICT array = (int32_t * _RESTRICT)arrayIn;
	int32_t* _RESTRICT tmp = (int32_t * _RESTRICT)tmpIn;
	const uint32_t sn = (height + (even ? 1 : 0)) >> 1;
	const uint32_t dn = height - sn;
	opj_dwt_fetch_cols_vertical_pass(arrayIn, tmpIn, height, stride_width, cols);
#define OPJ_Sc(i) tmp[(i)*2* NB_ELTS_V8 + c]
#define OPJ_Dc(i) tmp[((1+(i)*2))* NB_ELTS_V8 + c]
#ifdef __SSE2__
	if(height == 1) {
		if(!even) {
			for(uint32_t c = 0; c < NB_ELTS_V8; c++) {
				tmp[c] *= 2;
			}
		}
	}
	else if(even) {
		uint32_t c;
		uint32_t i = 0;
		if(i + 1 < sn) {
			__m128i xmm_Si_0 = *(const __m128i*)(tmp + 4 * 0);
			__m128i xmm_Si_1 = *(const __m128i*)(tmp + 4 * 1);
			for(; i + 1 < sn; i++) {
				__m128i xmm_Sip1_0 = *(const __m128i*)(tmp + (i + 1) * 2 * NB_ELTS_V8 + 4 * 0);
				__m128i xmm_Sip1_1 = *(const __m128i*)(tmp + (i + 1) * 2 * NB_ELTS_V8 + 4 * 1);
				__m128i xmm_Di_0 = *(const __m128i*)(tmp + (1 + i * 2) * NB_ELTS_V8 + 4 * 0);
				__m128i xmm_Di_1 = *(const __m128i*)(tmp + (1 + i * 2) * NB_ELTS_V8 + 4 * 1);
				xmm_Di_0 = _mm_sub_epi32(xmm_Di_0, _mm_srai_epi32(_mm_add_epi32(xmm_Si_0, xmm_Sip1_0), 1));
				xmm_Di_1 = _mm_sub_epi32(xmm_Di_1, _mm_srai_epi32(_mm_add_epi32(xmm_Si_1, xmm_Sip1_1), 1));
				*(__m128i*)(tmp + (1 + i * 2) * NB_ELTS_V8 + 4 * 0) =  xmm_Di_0;
				*(__m128i*)(tmp + (1 + i * 2) * NB_ELTS_V8 + 4 * 1) =  xmm_Di_1;
				xmm_Si_0 = xmm_Sip1_0;
				xmm_Si_1 = xmm_Sip1_1;
			}
		}
		if(((height) % 2) == 0) {
			for(c = 0; c < NB_ELTS_V8; c++) {
				OPJ_Dc(i) -= OPJ_Sc(i);
			}
		}
		for(c = 0; c < NB_ELTS_V8; c++) {
			OPJ_Sc(0) += (OPJ_Dc(0) + OPJ_Dc(0) + 2) >> 2;
		}
		i = 1;
		if(i < dn) {
			__m128i xmm_Dim1_0 = *(const __m128i*)(tmp + (1 +
			    (i - 1) * 2) * NB_ELTS_V8 + 4 * 0);
			__m128i xmm_Dim1_1 = *(const __m128i*)(tmp + (1 +
			    (i - 1) * 2) * NB_ELTS_V8 + 4 * 1);
			const __m128i xmm_two = _mm_set1_epi32(2);
			for(; i < dn; i++) {
				__m128i xmm_Di_0 = *(const __m128i*)(tmp + (1 + i * 2) * NB_ELTS_V8 + 4 * 0);
				__m128i xmm_Di_1 = *(const __m128i*)(tmp + (1 + i * 2) * NB_ELTS_V8 + 4 * 1);
				__m128i xmm_Si_0 = *(const __m128i*)(tmp + (i * 2) * NB_ELTS_V8 + 4 * 0);
				__m128i xmm_Si_1 = *(const __m128i*)(tmp + (i * 2) * NB_ELTS_V8 + 4 * 1);
				xmm_Si_0 = _mm_add_epi32(xmm_Si_0, _mm_srai_epi32(_mm_add_epi32(_mm_add_epi32(xmm_Dim1_0, xmm_Di_0), xmm_two), 2));
				xmm_Si_1 = _mm_add_epi32(xmm_Si_1, _mm_srai_epi32(_mm_add_epi32(_mm_add_epi32(xmm_Dim1_1, xmm_Di_1), xmm_two), 2));
				*(__m128i*)(tmp + (i * 2) * NB_ELTS_V8 + 4 * 0) = xmm_Si_0;
				*(__m128i*)(tmp + (i * 2) * NB_ELTS_V8 + 4 * 1) = xmm_Si_1;
				xmm_Dim1_0 = xmm_Di_0;
				xmm_Dim1_1 = xmm_Di_1;
			}
		}
		if(((height) % 2) == 1) {
			for(c = 0; c < NB_ELTS_V8; c++) {
				OPJ_Sc(i) += (OPJ_Dc(i - 1) + OPJ_Dc(i - 1) + 2) >> 2;
			}
		}
	}
	else {
		uint32_t c;
		uint32_t i;
		for(c = 0; c < NB_ELTS_V8; c++) {
			OPJ_Sc(0) -= OPJ_Dc(0);
		}
		i = 1;
		if(i < sn) {
			__m128i xmm_Dim1_0 = *(const __m128i*)(tmp + (1 + (i - 1) * 2) * NB_ELTS_V8 + 4 * 0);
			__m128i xmm_Dim1_1 = *(const __m128i*)(tmp + (1 + (i - 1) * 2) * NB_ELTS_V8 + 4 * 1);
			for(; i < sn; i++) {
				__m128i xmm_Di_0 = *(const __m128i*)(tmp + (1 + i * 2) * NB_ELTS_V8 + 4 * 0);
				__m128i xmm_Di_1 = *(const __m128i*)(tmp + (1 + i * 2) * NB_ELTS_V8 + 4 * 1);
				__m128i xmm_Si_0 = *(const __m128i*)(tmp + (i * 2) * NB_ELTS_V8 + 4 * 0);
				__m128i xmm_Si_1 = *(const __m128i*)(tmp + (i * 2) * NB_ELTS_V8 + 4 * 1);
				xmm_Si_0 = _mm_sub_epi32(xmm_Si_0, _mm_srai_epi32(_mm_add_epi32(xmm_Di_0, xmm_Dim1_0), 1));
				xmm_Si_1 = _mm_sub_epi32(xmm_Si_1, _mm_srai_epi32(_mm_add_epi32(xmm_Di_1, xmm_Dim1_1), 1));
				*(__m128i*)(tmp + (i * 2) * NB_ELTS_V8 + 4 * 0) = xmm_Si_0;
				*(__m128i*)(tmp + (i * 2) * NB_ELTS_V8 + 4 * 1) = xmm_Si_1;
				xmm_Dim1_0 = xmm_Di_0;
				xmm_Dim1_1 = xmm_Di_1;
			}
		}
		if(((height) % 2) == 1) {
			for(c = 0; c < NB_ELTS_V8; c++) {
				OPJ_Sc(i) -= OPJ_Dc(i - 1);
			}
		}
		i = 0;
		if(i + 1 < dn) {
			__m128i xmm_Si_0 = *((const __m128i*)(tmp + 4 * 0));
			__m128i xmm_Si_1 = *((const __m128i*)(tmp + 4 * 1));
			const __m128i xmm_two = _mm_set1_epi32(2);
			for(; i + 1 < dn; i++) {
				__m128i xmm_Sip1_0 = *(const __m128i*)(tmp + (i + 1) * 2 * NB_ELTS_V8 + 4 * 0);
				__m128i xmm_Sip1_1 = *(const __m128i*)(tmp + (i + 1) * 2 * NB_ELTS_V8 + 4 * 1);
				__m128i xmm_Di_0 = *(const __m128i*)(tmp + (1 + i * 2) * NB_ELTS_V8 + 4 * 0);
				__m128i xmm_Di_1 = *(const __m128i*)(tmp + (1 + i * 2) * NB_ELTS_V8 + 4 * 1);
				xmm_Di_0 = _mm_add_epi32(xmm_Di_0, _mm_srai_epi32(_mm_add_epi32(_mm_add_epi32(xmm_Si_0, xmm_Sip1_0), xmm_two), 2));
				xmm_Di_1 = _mm_add_epi32(xmm_Di_1, _mm_srai_epi32(_mm_add_epi32(_mm_add_epi32(xmm_Si_1, xmm_Sip1_1), xmm_two), 2));
				*(__m128i*)(tmp + (1 + i * 2) * NB_ELTS_V8 + 4 * 0) = xmm_Di_0;
				*(__m128i*)(tmp + (1 + i * 2) * NB_ELTS_V8 + 4 * 1) = xmm_Di_1;
				xmm_Si_0 = xmm_Sip1_0;
				xmm_Si_1 = xmm_Sip1_1;
			}
		}
		if(((height) % 2) == 0) {
			for(c = 0; c < NB_ELTS_V8; c++) {
				OPJ_Dc(i) += (OPJ_Sc(i) + OPJ_Sc(i) + 2) >> 2;
			}
		}
	}
#else
	if(even) {
		uint32_t c;
		if(height > 1) {
			uint32_t i;
			for(i = 0; i + 1 < sn; i++) {
				for(c = 0; c < NB_ELTS_V8; c++) {
					OPJ_Dc(i) -= (OPJ_Sc(i) + OPJ_Sc(i + 1)) >> 1;
				}
			}
			if(((height) % 2) == 0) {
				for(c = 0; c < NB_ELTS_V8; c++) {
					OPJ_Dc(i) -= OPJ_Sc(i);
				}
			}
			for(c = 0; c < NB_ELTS_V8; c++) {
				OPJ_Sc(0) += (OPJ_Dc(0) + OPJ_Dc(0) + 2) >> 2;
			}
			for(i = 1; i < dn; i++) {
				for(c = 0; c < NB_ELTS_V8; c++) {
					OPJ_Sc(i) += (OPJ_Dc(i - 1) + OPJ_Dc(i) + 2) >> 2;
				}
			}
			if(((height) % 2) == 1) {
				for(c = 0; c < NB_ELTS_V8; c++) {
					OPJ_Sc(i) += (OPJ_Dc(i - 1) + OPJ_Dc(i - 1) + 2) >> 2;
				}
			}
		}
	}
	else {
		uint32_t c;
		if(height == 1) {
			for(c = 0; c < NB_ELTS_V8; c++) {
				OPJ_Sc(0) *= 2;
			}
		}
		else {
			uint32_t i;
			for(c = 0; c < NB_ELTS_V8; c++) {
				OPJ_Sc(0) -= OPJ_Dc(0);
			}
			for(i = 1; i < sn; i++) {
				for(c = 0; c < NB_ELTS_V8; c++) {
					OPJ_Sc(i) -= (OPJ_Dc(i) + OPJ_Dc(i - 1)) >> 1;
				}
			}
			if(((height) % 2) == 1) {
				for(c = 0; c < NB_ELTS_V8; c++) {
					OPJ_Sc(i) -= OPJ_Dc(i - 1);
				}
			}
			for(i = 0; i + 1 < dn; i++) {
				for(c = 0; c < NB_ELTS_V8; c++) {
					OPJ_Dc(i) += (OPJ_Sc(i) + OPJ_Sc(i + 1) + 2) >> 2;
				}
			}
			if(((height) % 2) == 0) {
				for(c = 0; c < NB_ELTS_V8; c++) {
					OPJ_Dc(i) += (OPJ_Sc(i) + OPJ_Sc(i) + 2) >> 2;
				}
			}
		}
	}
#endif
	if(cols == NB_ELTS_V8) {
		opj_dwt_deinterleave_v_cols(tmp, array, (int32_t)dn, (int32_t)sn, stride_width, even ? 0 : 1, NB_ELTS_V8);
	}
	else {
		opj_dwt_deinterleave_v_cols(tmp, array, (int32_t)dn, (int32_t)sn, stride_width, even ? 0 : 1, cols);
	}
}

static void opj_v8dwt_encode_step1(float* fw, uint32_t end, const float cst)
{
	uint32_t i;
#ifdef __SSE__
	__m128* vw = (__m128*)fw;
	const __m128 vcst = _mm_set1_ps(cst);
	for(i = 0; i < end; ++i) {
		vw[0] = _mm_mul_ps(vw[0], vcst);
		vw[1] = _mm_mul_ps(vw[1], vcst);
		vw += 2 * (NB_ELTS_V8 * sizeof(float) / sizeof(__m128));
	}
#else
	uint32_t c;
	for(i = 0; i < end; ++i) {
		for(c = 0; c < NB_ELTS_V8; c++) {
			fw[i * 2 * NB_ELTS_V8 + c] *= cst;
		}
	}
#endif
}

static void opj_v8dwt_encode_step2(float* fl, float* fw, uint32_t end, uint32_t m, float cst)
{
	uint32_t i;
	uint32_t imax = smin(end, m);
#ifdef __SSE__
	__m128* vw = (__m128*)fw;
	__m128 vcst = _mm_set1_ps(cst);
	if(imax > 0) {
		__m128* vl = (__m128*)fl;
		vw[-2] = _mm_add_ps(vw[-2], _mm_mul_ps(_mm_add_ps(vl[0], vw[0]), vcst));
		vw[-1] = _mm_add_ps(vw[-1], _mm_mul_ps(_mm_add_ps(vl[1], vw[1]), vcst));
		vw += 2 * (NB_ELTS_V8 * sizeof(float) / sizeof(__m128));
		i = 1;
		for(; i < imax; ++i) {
			vw[-2] = _mm_add_ps(vw[-2], _mm_mul_ps(_mm_add_ps(vw[-4], vw[0]), vcst));
			vw[-1] = _mm_add_ps(vw[-1], _mm_mul_ps(_mm_add_ps(vw[-3], vw[1]), vcst));
			vw += 2 * (NB_ELTS_V8 * sizeof(float) / sizeof(__m128));
		}
	}
	if(m < end) {
		assert(m + 1 == end);
		vcst = _mm_add_ps(vcst, vcst);
		vw[-2] = _mm_add_ps(vw[-2], _mm_mul_ps(vw[-4], vcst));
		vw[-1] = _mm_add_ps(vw[-1], _mm_mul_ps(vw[-3], vcst));
	}
#else
	int32_t c;
	if(imax > 0) {
		for(c = 0; c < NB_ELTS_V8; c++) {
			fw[-1 * NB_ELTS_V8 + c] += (fl[0 * NB_ELTS_V8 + c] + fw[0 * NB_ELTS_V8 + c]) *
			    cst;
		}
		fw += 2 * NB_ELTS_V8;
		i = 1;
		for(; i < imax; ++i) {
			for(c = 0; c < NB_ELTS_V8; c++) {
				fw[-1 * NB_ELTS_V8 + c] += (fw[-2 * NB_ELTS_V8 + c] + fw[0 * NB_ELTS_V8 + c]) *
				    cst;
			}
			fw += 2 * NB_ELTS_V8;
		}
	}
	if(m < end) {
		assert(m + 1 == end);
		for(c = 0; c < NB_ELTS_V8; c++) {
			fw[-1 * NB_ELTS_V8 + c] += (2 * fw[-2 * NB_ELTS_V8 + c]) * cst;
		}
	}
#endif
}

/* Forward 9-7 transform, for the vertical pass, processing cols columns */
/* where cols <= NB_ELTS_V8 */
static void opj_dwt_encode_and_deinterleave_v_real(void * arrayIn, void * tmpIn, uint32_t height, boolint even, uint32_t stride_width, uint32_t cols)
{
	float* _RESTRICT array = (float * _RESTRICT)arrayIn;
	float* _RESTRICT tmp = (float * _RESTRICT)tmpIn;
	const int32_t sn = (int32_t)((height + (even ? 1 : 0)) >> 1);
	const int32_t dn = (int32_t)(height - (uint32_t)sn);
	int32_t a, b;
	if(height == 1) {
		return;
	}
	opj_dwt_fetch_cols_vertical_pass(arrayIn, tmpIn, height, stride_width, cols);
	if(even) {
		a = 0;
		b = 1;
	}
	else {
		a = 1;
		b = 0;
	}
	opj_v8dwt_encode_step2(tmp + a * NB_ELTS_V8, tmp + (b + 1) * NB_ELTS_V8, (uint32_t)dn, (uint32_t)smin(dn, sn - b), opj_dwt_alpha);
	opj_v8dwt_encode_step2(tmp + b * NB_ELTS_V8, tmp + (a + 1) * NB_ELTS_V8, (uint32_t)sn, (uint32_t)smin(sn, dn - a), opj_dwt_beta);
	opj_v8dwt_encode_step2(tmp + a * NB_ELTS_V8, tmp + (b + 1) * NB_ELTS_V8, (uint32_t)dn, (uint32_t)smin(dn, sn - b), opj_dwt_gamma);
	opj_v8dwt_encode_step2(tmp + b * NB_ELTS_V8, tmp + (a + 1) * NB_ELTS_V8, (uint32_t)sn, (uint32_t)smin(sn, dn - a), opj_dwt_delta);
	opj_v8dwt_encode_step1(tmp + b * NB_ELTS_V8, (uint32_t)dn, opj_K);
	opj_v8dwt_encode_step1(tmp + a * NB_ELTS_V8, (uint32_t)sn, opj_invK);
	if(cols == NB_ELTS_V8) {
		opj_dwt_deinterleave_v_cols((int32_t*)tmp, (int32_t*)array, (int32_t)dn, (int32_t)sn, stride_width, even ? 0 : 1, NB_ELTS_V8);
	}
	else {
		opj_dwt_deinterleave_v_cols((int32_t*)tmp, (int32_t*)array, (int32_t)dn, (int32_t)sn, stride_width, even ? 0 : 1, cols);
	}
}

/* <summary>                            */
/* Forward 5-3 wavelet transform in 2-D. */
/* </summary>                           */
static INLINE boolint opj_dwt_encode_procedure(opj_thread_pool_t* tp, opj_tcd_tilecomp_t * tilec,
    opj_encode_and_deinterleave_v_fnptr_type p_encode_and_deinterleave_v,
    opj_encode_and_deinterleave_h_one_row_fnptr_type p_encode_and_deinterleave_h_one_row)
{
	int32_t i;
	int32_t * bj = 0;
	const int num_threads = opj_thread_pool_get_thread_count(tp);
	int32_t * _RESTRICT tiledp = tilec->data;
	uint32_t w = (uint32_t)(tilec->x1 - tilec->x0);
	int32_t l = (int32_t)tilec->numresolutions - 1;
	opj_tcd_resolution_t * l_cur_res = tilec->resolutions + l;
	opj_tcd_resolution_t * l_last_res = l_cur_res - 1;
	size_t l_data_size = opj_dwt_max_resolution(tilec->resolutions, tilec->numresolutions);
	/* overflow check */
	if(l_data_size > (SIZE_MAX / (NB_ELTS_V8 * sizeof(int32_t)))) {
		/* FIXME event manager error callback */
		return FALSE;
	}
	l_data_size *= NB_ELTS_V8 * sizeof(int32_t);
	bj = (int32_t*)opj_aligned_32_malloc(l_data_size);
	/* l_data_size is equal to 0 when numresolutions == 1 but bj is not used */
	/* in that case, so do not error out */
	if(l_data_size != 0 && !bj) {
		return FALSE;
	}
	i = l;
	while(i--) {
		uint32_t j;
		uint32_t rw;   /* width of the resolution level computed   */
		uint32_t rh;   /* height of the resolution level computed  */
		uint32_t rw1; /* width of the resolution level once lower than computed one */
		uint32_t rh1; /* height of the resolution level once lower than computed one */
		int32_t cas_col; /* 0 = non inversion on horizontal filtering 1 = inversion between low-pass and high-pass filtering */
		int32_t cas_row; /* 0 = non inversion on vertical filtering 1 = inversion between low-pass and high-pass filtering   */
		int32_t dn, sn;
		rw  = (uint32_t)(l_cur_res->x1 - l_cur_res->x0);
		rh  = (uint32_t)(l_cur_res->y1 - l_cur_res->y0);
		rw1 = (uint32_t)(l_last_res->x1 - l_last_res->x0);
		rh1 = (uint32_t)(l_last_res->y1 - l_last_res->y0);
		cas_row = l_cur_res->x0 & 1;
		cas_col = l_cur_res->y0 & 1;
		sn = (int32_t)rh1;
		dn = (int32_t)(rh - rh1);
		/* Perform vertical pass */
		if(num_threads <= 1 || rw < 2 * NB_ELTS_V8) {
			for(j = 0; j + NB_ELTS_V8 - 1 < rw; j += NB_ELTS_V8) {
				p_encode_and_deinterleave_v(tiledp + j, bj, rh, cas_col == 0, w, NB_ELTS_V8);
			}
			if(j < rw) {
				p_encode_and_deinterleave_v(tiledp + j, bj, rh, cas_col == 0, w, rw - j);
			}
		}
		else {
			uint32_t num_jobs = (uint32_t)num_threads;
			uint32_t step_j;
			if(rw < num_jobs) {
				num_jobs = rw;
			}
			step_j = ((rw / num_jobs) / NB_ELTS_V8) * NB_ELTS_V8;
			for(j = 0; j < num_jobs; j++) {
				opj_dwt_encode_v_job_t* job = (opj_dwt_encode_v_job_t*)opj_malloc(sizeof(opj_dwt_encode_v_job_t));
				if(!job) {
					opj_thread_pool_wait_completion(tp, 0);
					opj_aligned_free(bj);
					return FALSE;
				}
				job->v.mem = (int32_t*)opj_aligned_32_malloc(l_data_size);
				if(!job->v.mem) {
					opj_thread_pool_wait_completion(tp, 0);
					SAlloc::F(job);
					opj_aligned_free(bj);
					return FALSE;
				}
				job->v.dn = dn;
				job->v.sn = sn;
				job->v.cas = cas_col;
				job->rh = rh;
				job->w = w;
				job->tiledp = tiledp;
				job->min_j = j * step_j;
				job->max_j = (j + 1 == num_jobs) ? rw : (j + 1) * step_j;
				job->p_encode_and_deinterleave_v = p_encode_and_deinterleave_v;
				opj_thread_pool_submit_job(tp, opj_dwt_encode_v_func, job);
			}
			opj_thread_pool_wait_completion(tp, 0);
		}
		sn = (int32_t)rw1;
		dn = (int32_t)(rw - rw1);
		/* Perform horizontal pass */
		if(num_threads <= 1 || rh <= 1) {
			for(j = 0; j < rh; j++) {
				int32_t* _RESTRICT aj = tiledp + j * w;
				(*p_encode_and_deinterleave_h_one_row)(aj, bj, rw,
				    cas_row == 0 ? TRUE : FALSE);
			}
		}
		else {
			uint32_t num_jobs = (uint32_t)num_threads;
			uint32_t step_j;
			if(rh < num_jobs) {
				num_jobs = rh;
			}
			step_j = (rh / num_jobs);
			for(j = 0; j < num_jobs; j++) {
				opj_dwt_encode_h_job_t * job = (opj_dwt_encode_h_job_t*)opj_malloc(sizeof(opj_dwt_encode_h_job_t));
				if(!job) {
					opj_thread_pool_wait_completion(tp, 0);
					opj_aligned_free(bj);
					return FALSE;
				}
				job->h.mem = (int32_t*)opj_aligned_32_malloc(l_data_size);
				if(!job->h.mem) {
					opj_thread_pool_wait_completion(tp, 0);
					SAlloc::F(job);
					opj_aligned_free(bj);
					return FALSE;
				}
				job->h.dn = dn;
				job->h.sn = sn;
				job->h.cas = cas_row;
				job->rw = rw;
				job->w = w;
				job->tiledp = tiledp;
				job->min_j = j * step_j;
				job->max_j = (j + 1U) * step_j; /* this can overflow */
				if(j == (num_jobs - 1U)) { /* this will take care of the overflow */
					job->max_j = rh;
				}
				job->p_function = p_encode_and_deinterleave_h_one_row;
				opj_thread_pool_submit_job(tp, opj_dwt_encode_h_func, job);
			}
			opj_thread_pool_wait_completion(tp, 0);
		}

		l_cur_res = l_last_res;

		--l_last_res;
	}

	opj_aligned_free(bj);
	return TRUE;
}

/* Forward 5-3 wavelet transform in 2-D. */
/* </summary>                           */
boolint opj_dwt_encode(opj_tcd_t * p_tcd,
    opj_tcd_tilecomp_t * tilec)
{
	return opj_dwt_encode_procedure(p_tcd->thread_pool, tilec,
		   opj_dwt_encode_and_deinterleave_v,
		   opj_dwt_encode_and_deinterleave_h_one_row);
}

/* <summary>                            */
/* Inverse 5-3 wavelet transform in 2-D. */
/* </summary>                           */
boolint opj_dwt_decode(opj_tcd_t * p_tcd, opj_tcd_tilecomp_t* tilec,
    uint32_t numres)
{
	if(p_tcd->whole_tile_decoding) {
		return opj_dwt_decode_tile(p_tcd->thread_pool, tilec, numres);
	}
	else {
		return opj_dwt_decode_partial_tile(tilec, numres);
	}
}

/* <summary>                */
/* Get norm of 5-3 wavelet. */
/* </summary>               */
double opj_dwt_getnorm(uint32_t level, uint32_t orient)
{
	/* FIXME ! This is just a band-aid to avoid a buffer overflow */
	/* but the array should really be extended up to 33 resolution levels */
	/* See https://github.com/uclouvain/openjpeg/issues/493 */
	if(orient == 0 && level >= 10) {
		level = 9;
	}
	else if(orient > 0 && level >= 9) {
		level = 8;
	}
	return opj_dwt_norms[orient][level];
}

/* <summary>                             */
/* Forward 9-7 wavelet transform in 2-D. */
/* </summary>                            */
boolint opj_dwt_encode_real(opj_tcd_t * p_tcd,
    opj_tcd_tilecomp_t * tilec)
{
	return opj_dwt_encode_procedure(p_tcd->thread_pool, tilec,
		   opj_dwt_encode_and_deinterleave_v_real,
		   opj_dwt_encode_and_deinterleave_h_one_row_real);
}

/* <summary>                */
/* Get norm of 9-7 wavelet. */
/* </summary>               */
double opj_dwt_getnorm_real(uint32_t level, uint32_t orient)
{
	/* FIXME ! This is just a band-aid to avoid a buffer overflow */
	/* but the array should really be extended up to 33 resolution levels */
	/* See https://github.com/uclouvain/openjpeg/issues/493 */
	if(orient == 0 && level >= 10) {
		level = 9;
	}
	else if(orient > 0 && level >= 9) {
		level = 8;
	}
	return opj_dwt_norms_real[orient][level];
}

void opj_dwt_calc_explicit_stepsizes(opj_tccp_t * tccp, uint32_t prec)
{
	uint32_t numbands, bandno;
	numbands = 3 * tccp->numresolutions - 2;
	for(bandno = 0; bandno < numbands; bandno++) {
		double stepsize;
		uint32_t resno, level, orient, gain;

		resno = (bandno == 0) ? 0 : ((bandno - 1) / 3 + 1);
		orient = (bandno == 0) ? 0 : ((bandno - 1) % 3 + 1);
		level = tccp->numresolutions - 1 - resno;
		gain = (tccp->qmfbid == 0) ? 0 : ((orient == 0) ? 0 : (((orient == 1) ||
		    (orient == 2)) ? 1 : 2));
		if(tccp->qntsty == J2K_CCP_QNTSTY_NOQNT) {
			stepsize = 1.0;
		}
		else {
			double norm = opj_dwt_getnorm_real(level, orient);
			stepsize = (1 << (gain)) / norm;
		}
		opj_dwt_encode_stepsize((int32_t)floor(stepsize * 8192.0),
		    (int32_t)(prec + gain), &tccp->stepsizes[bandno]);
	}
}

/* <summary>                             */
/* Determine maximum computed resolution level for inverse wavelet transform */
/* </summary>                            */
static uint32_t opj_dwt_max_resolution(opj_tcd_resolution_t* _RESTRICT r,
    uint32_t i)
{
	uint32_t mr   = 0;
	uint32_t w;
	while(--i) {
		++r;
		if(mr < (w = (uint32_t)(r->x1 - r->x0))) {
			mr = w;
		}
		if(mr < (w = (uint32_t)(r->y1 - r->y0))) {
			mr = w;
		}
	}
	return mr;
}

typedef struct {
	opj_dwt_t h;
	uint32_t rw;
	uint32_t w;
	int32_t * _RESTRICT tiledp;
	uint32_t min_j;
	uint32_t max_j;
} opj_dwt_decode_h_job_t;

static void opj_dwt_decode_h_func(void* user_data, opj_tls_t* tls)
{
	opj_dwt_decode_h_job_t* job = (opj_dwt_decode_h_job_t*)user_data;
	(void)tls;
	for(uint32_t j = job->min_j; j < job->max_j; j++) {
		opj_idwt53_h(&job->h, &job->tiledp[j * job->w]);
	}
	opj_aligned_free(job->h.mem);
	SAlloc::F(job);
}

typedef struct {
	opj_dwt_t v;
	uint32_t rh;
	uint32_t w;
	int32_t * _RESTRICT tiledp;
	uint32_t min_j;
	uint32_t max_j;
} opj_dwt_decode_v_job_t;

static void opj_dwt_decode_v_func(void* user_data, opj_tls_t* tls)
{
	uint32_t j;
	(void)tls;
	opj_dwt_decode_v_job_t * job = (opj_dwt_decode_v_job_t*)user_data;
	for(j = job->min_j; j + PARALLEL_COLS_53 <= job->max_j;
	    j += PARALLEL_COLS_53) {
		opj_idwt53_v(&job->v, &job->tiledp[j], (size_t)job->w,
		    PARALLEL_COLS_53);
	}
	if(j < job->max_j)
		opj_idwt53_v(&job->v, &job->tiledp[j], (size_t)job->w, (int32_t)(job->max_j - j));
	opj_aligned_free(job->v.mem);
	SAlloc::F(job);
}

/* <summary>                            */
/* Inverse wavelet transform in 2-D.    */
/* </summary>                           */
static boolint opj_dwt_decode_tile(opj_thread_pool_t* tp, opj_tcd_tilecomp_t* tilec, uint32_t numres)
{
	opj_dwt_t h;
	opj_dwt_t v;
	opj_tcd_resolution_t* tr = tilec->resolutions;
	uint32_t rw = (uint32_t)(tr->x1 - tr->x0); /* width of the resolution level computed */
	uint32_t rh = (uint32_t)(tr->y1 - tr->y0); /* height of the resolution level computed */
	uint32_t w = (uint32_t)(tilec->resolutions[tilec->minimum_num_resolutions - 1].x1 - tilec->resolutions[tilec->minimum_num_resolutions - 1].x0);
	size_t h_mem_size;
	int num_threads;
	if(numres == 1U) {
		return TRUE;
	}
	num_threads = opj_thread_pool_get_thread_count(tp);
	h_mem_size = opj_dwt_max_resolution(tr, numres);
	/* overflow check */
	if(h_mem_size > (SIZE_MAX / PARALLEL_COLS_53 / sizeof(int32_t))) {
		/* FIXME event manager error callback */
		return FALSE;
	}
	/* We need PARALLEL_COLS_53 times the height of the array, */
	/* since for the vertical pass */
	/* we process PARALLEL_COLS_53 columns at a time */
	h_mem_size *= PARALLEL_COLS_53 * sizeof(int32_t);
	h.mem = (int32_t*)opj_aligned_32_malloc(h_mem_size);
	if(!h.mem) {
		/* FIXME event manager error callback */
		return FALSE;
	}
	v.mem = h.mem;
	while(--numres) {
		int32_t * _RESTRICT tiledp = tilec->data;
		uint32_t j;
		++tr;
		h.sn = (int32_t)rw;
		v.sn = (int32_t)rh;
		rw = (uint32_t)(tr->x1 - tr->x0);
		rh = (uint32_t)(tr->y1 - tr->y0);
		h.dn = (int32_t)(rw - (uint32_t)h.sn);
		h.cas = tr->x0 % 2;
		if(num_threads <= 1 || rh <= 1) {
			for(j = 0; j < rh; ++j) {
				opj_idwt53_h(&h, &tiledp[(size_t)j * w]);
			}
		}
		else {
			uint32_t num_jobs = (uint32_t)num_threads;
			uint32_t step_j;
			if(rh < num_jobs) {
				num_jobs = rh;
			}
			step_j = (rh / num_jobs);
			for(j = 0; j < num_jobs; j++) {
				opj_dwt_decode_h_job_t* job;
				job = (opj_dwt_decode_h_job_t*)opj_malloc(sizeof(opj_dwt_decode_h_job_t));
				if(!job) {
					/* It would be nice to fallback to single thread case, but */
					/* unfortunately some jobs may be launched and have modified */
					/* tiledp, so it is not practical to recover from that error */
					/* FIXME event manager error callback */
					opj_thread_pool_wait_completion(tp, 0);
					opj_aligned_free(h.mem);
					return FALSE;
				}
				job->h = h;
				job->rw = rw;
				job->w = w;
				job->tiledp = tiledp;
				job->min_j = j * step_j;
				job->max_j = (j + 1U) * step_j; /* this can overflow */
				if(j == (num_jobs - 1U)) { /* this will take care of the overflow */
					job->max_j = rh;
				}
				job->h.mem = (int32_t*)opj_aligned_32_malloc(h_mem_size);
				if(!job->h.mem) {
					/* FIXME event manager error callback */
					opj_thread_pool_wait_completion(tp, 0);
					SAlloc::F(job);
					opj_aligned_free(h.mem);
					return FALSE;
				}
				opj_thread_pool_submit_job(tp, opj_dwt_decode_h_func, job);
			}
			opj_thread_pool_wait_completion(tp, 0);
		}
		v.dn = (int32_t)(rh - (uint32_t)v.sn);
		v.cas = tr->y0 % 2;
		if(num_threads <= 1 || rw <= 1) {
			for(j = 0; j + PARALLEL_COLS_53 <= rw;
			    j += PARALLEL_COLS_53) {
				opj_idwt53_v(&v, &tiledp[j], (size_t)w, PARALLEL_COLS_53);
			}
			if(j < rw) {
				opj_idwt53_v(&v, &tiledp[j], (size_t)w, (int32_t)(rw - j));
			}
		}
		else {
			uint32_t num_jobs = (uint32_t)num_threads;
			uint32_t step_j;
			if(rw < num_jobs) {
				num_jobs = rw;
			}
			step_j = (rw / num_jobs);
			for(j = 0; j < num_jobs; j++) {
				opj_dwt_decode_v_job_t* job;
				job = (opj_dwt_decode_v_job_t*)opj_malloc(sizeof(opj_dwt_decode_v_job_t));
				if(!job) {
					/* It would be nice to fallback to single thread case, but */
					/* unfortunately some jobs may be launched and have modified */
					/* tiledp, so it is not practical to recover from that error */
					/* FIXME event manager error callback */
					opj_thread_pool_wait_completion(tp, 0);
					opj_aligned_free(v.mem);
					return FALSE;
				}
				job->v = v;
				job->rh = rh;
				job->w = w;
				job->tiledp = tiledp;
				job->min_j = j * step_j;
				job->max_j = (j + 1U) * step_j; /* this can overflow */
				if(j == (num_jobs - 1U)) { /* this will take care of the overflow */
					job->max_j = rw;
				}
				job->v.mem = (int32_t*)opj_aligned_32_malloc(h_mem_size);
				if(!job->v.mem) {
					/* FIXME event manager error callback */
					opj_thread_pool_wait_completion(tp, 0);
					SAlloc::F(job);
					opj_aligned_free(v.mem);
					return FALSE;
				}
				opj_thread_pool_submit_job(tp, opj_dwt_decode_v_func, job);
			}
			opj_thread_pool_wait_completion(tp, 0);
		}
	}
	opj_aligned_free(h.mem);
	return TRUE;
}

static void opj_dwt_interleave_partial_h(int32_t * dest, int32_t cas, opj_sparse_array_int32_t* sa, uint32_t sa_line,
    uint32_t sn, uint32_t win_l_x0, uint32_t win_l_x1, uint32_t win_h_x0, uint32_t win_h_x1)
{
	boolint ret = opj_sparse_array_int32_read(sa, win_l_x0, sa_line, win_l_x1, sa_line + 1, dest + cas + 2 * win_l_x0, 2, 0, TRUE);
	assert(ret);
	ret = opj_sparse_array_int32_read(sa, sn + win_h_x0, sa_line, sn + win_h_x1, sa_line + 1, dest + 1 - cas + 2 * win_h_x0, 2, 0, TRUE);
	assert(ret);
	CXX_UNUSED(ret);
}

static void opj_dwt_interleave_partial_v(int32_t * dest, int32_t cas, opj_sparse_array_int32_t* sa, uint32_t sa_col,
    uint32_t nb_cols, uint32_t sn, uint32_t win_l_y0, uint32_t win_l_y1, uint32_t win_h_y0, uint32_t win_h_y1)
{
	boolint ret  = opj_sparse_array_int32_read(sa, sa_col, win_l_y0, sa_col + nb_cols, win_l_y1, dest + cas * 4 + 2 * 4 * win_l_y0, 1, 2 * 4, TRUE);
	assert(ret);
	ret = opj_sparse_array_int32_read(sa, sa_col, sn + win_h_y0, sa_col + nb_cols, sn + win_h_y1, dest + (1 - cas) * 4 + 2 * 4 * win_h_y0, 1, 2 * 4, TRUE);
	assert(ret);
	CXX_UNUSED(ret);
}

static void opj_dwt_decode_partial_1(int32_t * a, int32_t dn, int32_t sn, int32_t cas, int32_t win_l_x0, int32_t win_l_x1, int32_t win_h_x0, int32_t win_h_x1)
{
	int32_t i;
	if(!cas) {
		if((dn > 0) || (sn > 1)) { /* NEW :  CASE ONE ELEMENT */
			/* Naive version is :
			   for (i = win_l_x0; i < i_max; i++) {
			    OPJ_S(i) -= (OPJ_D_(i - 1) + OPJ_D_(i) + 2) >> 2;
			   }
			   for (i = win_h_x0; i < win_h_x1; i++) {
			    OPJ_D(i) += (OPJ_S_(i) + OPJ_S_(i + 1)) >> 1;
			   }
			   but the compiler doesn't manage to unroll it to avoid bound
			   checking in OPJ_S_ and OPJ_D_ macros
			 */
			i = win_l_x0;
			if(i < win_l_x1) {
				int32_t i_max;
				/* Left-most case */
				OPJ_S(i) -= (OPJ_D_(i - 1) + OPJ_D_(i) + 2) >> 2;
				i++;
				i_max = win_l_x1;
				if(i_max > dn) {
					i_max = dn;
				}
				for(; i < i_max; i++) {
					/* No bound checking */
					OPJ_S(i) -= (OPJ_D(i - 1) + OPJ_D(i) + 2) >> 2;
				}
				for(; i < win_l_x1; i++) {
					/* Right-most case */
					OPJ_S(i) -= (OPJ_D_(i - 1) + OPJ_D_(i) + 2) >> 2;
				}
			}
			i = win_h_x0;
			if(i < win_h_x1) {
				int32_t i_max = win_h_x1;
				if(i_max >= sn) {
					i_max = sn - 1;
				}
				for(; i < i_max; i++) {
					/* No bound checking */
					OPJ_D(i) += (OPJ_S(i) + OPJ_S(i + 1)) >> 1;
				}
				for(; i < win_h_x1; i++) {
					/* Right-most case */
					OPJ_D(i) += (OPJ_S_(i) + OPJ_S_(i + 1)) >> 1;
				}
			}
		}
	}
	else {
		if(!sn  && dn == 1) { /* NEW :  CASE ONE ELEMENT */
			OPJ_S(0) /= 2;
		}
		else {
			for(i = win_l_x0; i < win_l_x1; i++) {
				OPJ_D(i) = opj_int_sub_no_overflow(OPJ_D(i), opj_int_add_no_overflow(opj_int_add_no_overflow(OPJ_SS_(i), OPJ_SS_(i + 1)), 2) >> 2);
			}
			for(i = win_h_x0; i < win_h_x1; i++) {
				OPJ_S(i) = opj_int_add_no_overflow(OPJ_S(i), opj_int_add_no_overflow(OPJ_DD_(i), OPJ_DD_(i - 1)) >> 1);
			}
		}
	}
}

#define OPJ_S_off(i, off) a[(uint32_t)(i)*2*4+off]
#define OPJ_D_off(i, off) a[(1+(uint32_t)(i)*2)*4+off]
#define OPJ_S__off(i, off) ((i)<0 ? OPJ_S_off(0, off) : ((i)>=sn ? OPJ_S_off(sn-1, off) : OPJ_S_off(i, off)))
#define OPJ_D__off(i, off) ((i)<0 ? OPJ_D_off(0, off) : ((i)>=dn ? OPJ_D_off(dn-1, off) : OPJ_D_off(i, off)))
#define OPJ_SS__off(i, off) ((i)<0 ? OPJ_S_off(0, off) : ((i)>=dn ? OPJ_S_off(dn-1, off) : OPJ_S_off(i, off)))
#define OPJ_DD__off(i, off) ((i)<0 ? OPJ_D_off(0, off) : ((i)>=sn ? OPJ_D_off(sn-1, off) : OPJ_D_off(i, off)))

static void opj_dwt_decode_partial_1_parallel(int32_t * a, uint32_t nb_cols, int32_t dn, int32_t sn, int32_t cas, int32_t win_l_x0,
    int32_t win_l_x1, int32_t win_h_x0, int32_t win_h_x1)
{
	int32_t i;
	uint32_t off;
	(void)nb_cols;
	if(!cas) {
		if((dn > 0) || (sn > 1)) { /* NEW :  CASE ONE ELEMENT */
			/* Naive version is :
			   for (i = win_l_x0; i < i_max; i++) {
			    OPJ_S(i) -= (OPJ_D_(i - 1) + OPJ_D_(i) + 2) >> 2;
			   }
			   for (i = win_h_x0; i < win_h_x1; i++) {
			    OPJ_D(i) += (OPJ_S_(i) + OPJ_S_(i + 1)) >> 1;
			   }
			   but the compiler doesn't manage to unroll it to avoid bound
			   checking in OPJ_S_ and OPJ_D_ macros
			 */
			i = win_l_x0;
			if(i < win_l_x1) {
				int32_t i_max;
				/* Left-most case */
				for(off = 0; off < 4; off++) {
					OPJ_S_off(i, off) -= (OPJ_D__off(i - 1, off) + OPJ_D__off(i, off) + 2) >> 2;
				}
				i++;
				i_max = win_l_x1;
				if(i_max > dn) {
					i_max = dn;
				}
#ifdef __SSE2__
				if(i + 1 < i_max) {
					const __m128i two = _mm_set1_epi32(2);
					__m128i Dm1 = _mm_load_si128((__m128i* const)(a + 4 + (i - 1) * 8));
					for(; i + 1 < i_max; i += 2) {
						/* No bound checking */
						__m128i S = _mm_load_si128((__m128i* const)(a + i * 8));
						__m128i D = _mm_load_si128((__m128i* const)(a + 4 + i * 8));
						__m128i S1 = _mm_load_si128((__m128i* const)(a + (i + 1) * 8));
						__m128i D1 = _mm_load_si128((__m128i* const)(a + 4 + (i + 1) * 8));
						S = _mm_sub_epi32(S, _mm_srai_epi32(_mm_add_epi32(_mm_add_epi32(Dm1, D), two), 2));
						S1 = _mm_sub_epi32(S1, _mm_srai_epi32(_mm_add_epi32(_mm_add_epi32(D, D1), two), 2));
						_mm_store_si128((__m128i*)(a + i * 8), S);
						_mm_store_si128((__m128i*)(a + (i + 1) * 8), S1);
						Dm1 = D1;
					}
				}
#endif
				for(; i < i_max; i++) {
					/* No bound checking */
					for(off = 0; off < 4; off++) {
						OPJ_S_off(i, off) -= (OPJ_D_off(i - 1, off) + OPJ_D_off(i, off) + 2) >> 2;
					}
				}
				for(; i < win_l_x1; i++) {
					/* Right-most case */
					for(off = 0; off < 4; off++) {
						OPJ_S_off(i, off) -= (OPJ_D__off(i - 1, off) + OPJ_D__off(i, off) + 2) >> 2;
					}
				}
			}
			i = win_h_x0;
			if(i < win_h_x1) {
				int32_t i_max = win_h_x1;
				if(i_max >= sn) {
					i_max = sn - 1;
				}
#ifdef __SSE2__
				if(i + 1 < i_max) {
					__m128i S =  _mm_load_si128((__m128i* const)(a + i * 8));
					for(; i + 1 < i_max; i += 2) {
						/* No bound checking */
						__m128i D = _mm_load_si128((__m128i* const)(a + 4 + i * 8));
						__m128i S1 = _mm_load_si128((__m128i* const)(a + (i + 1) * 8));
						__m128i D1 = _mm_load_si128((__m128i* const)(a + 4 + (i + 1) * 8));
						__m128i S2 = _mm_load_si128((__m128i* const)(a + (i + 2) * 8));
						D = _mm_add_epi32(D, _mm_srai_epi32(_mm_add_epi32(S, S1), 1));
						D1 = _mm_add_epi32(D1, _mm_srai_epi32(_mm_add_epi32(S1, S2), 1));
						_mm_store_si128((__m128i*)(a + 4 + i * 8), D);
						_mm_store_si128((__m128i*)(a + 4 + (i + 1) * 8), D1);
						S = S2;
					}
				}
#endif
				for(; i < i_max; i++) {
					/* No bound checking */
					for(off = 0; off < 4; off++) {
						OPJ_D_off(i, off) += (OPJ_S_off(i, off) + OPJ_S_off(i + 1, off)) >> 1;
					}
				}
				for(; i < win_h_x1; i++) {
					/* Right-most case */
					for(off = 0; off < 4; off++) {
						OPJ_D_off(i, off) += (OPJ_S__off(i, off) + OPJ_S__off(i + 1, off)) >> 1;
					}
				}
			}
		}
	}
	else {
		if(!sn  && dn == 1) { /* NEW :  CASE ONE ELEMENT */
			for(off = 0; off < 4; off++) {
				OPJ_S_off(0, off) /= 2;
			}
		}
		else {
			for(i = win_l_x0; i < win_l_x1; i++) {
				for(off = 0; off < 4; off++) {
					OPJ_D_off(i, off) = opj_int_sub_no_overflow(OPJ_D_off(i, off), opj_int_add_no_overflow(opj_int_add_no_overflow(OPJ_SS__off(i, off), OPJ_SS__off(i + 1, off)), 2) >> 2);
				}
			}
			for(i = win_h_x0; i < win_h_x1; i++) {
				for(off = 0; off < 4; off++) {
					OPJ_S_off(i, off) = opj_int_add_no_overflow(OPJ_S_off(i, off), opj_int_add_no_overflow(OPJ_DD__off(i, off), OPJ_DD__off(i - 1, off)) >> 1);
				}
			}
		}
	}
}

static void opj_dwt_get_band_coordinates(opj_tcd_tilecomp_t* tilec, uint32_t resno, uint32_t bandno,
    uint32_t tcx0, uint32_t tcy0, uint32_t tcx1, uint32_t tcy1,
    uint32_t* tbx0, uint32_t* tby0, uint32_t* tbx1, uint32_t* tby1)
{
	/* Compute number of decomposition for this band. See table F-1 */
	uint32_t nb = (resno == 0) ?
	    tilec->numresolutions - 1 :
	    tilec->numresolutions - resno;
	/* Map above tile-based coordinates to sub-band-based coordinates per */
	/* equation B-15 of the standard */
	uint32_t x0b = bandno & 1;
	uint32_t y0b = bandno >> 1;
	if(tbx0) {
		*tbx0 = (nb == 0) ? tcx0 : (tcx0 <= (1U << (nb - 1)) * x0b) ? 0 : opj_uint_ceildivpow2(tcx0 - (1U << (nb - 1)) * x0b, nb);
	}
	if(tby0) {
		*tby0 = (nb == 0) ? tcy0 : (tcy0 <= (1U << (nb - 1)) * y0b) ? 0 : opj_uint_ceildivpow2(tcy0 - (1U << (nb - 1)) * y0b, nb);
	}
	if(tbx1) {
		*tbx1 = (nb == 0) ? tcx1 : (tcx1 <= (1U << (nb - 1)) * x0b) ? 0 : opj_uint_ceildivpow2(tcx1 - (1U << (nb - 1)) * x0b, nb);
	}
	if(tby1) {
		*tby1 = (nb == 0) ? tcy1 : (tcy1 <= (1U << (nb - 1)) * y0b) ? 0 : opj_uint_ceildivpow2(tcy1 - (1U << (nb - 1)) * y0b, nb);
	}
}

static void opj_dwt_segment_grow(uint32_t filter_width, uint32_t max_size, uint32_t* start, uint32_t* end)
{
	*start = opj_uint_subs(*start, filter_width);
	*end = opj_uint_adds(*end, filter_width);
	*end = smin(*end, max_size);
}

static opj_sparse_array_int32_t* opj_dwt_init_sparse_array(opj_tcd_tilecomp_t* tilec, uint32_t numres)
{
	opj_tcd_resolution_t* tr_max = &(tilec->resolutions[numres - 1]);
	uint32_t w = (uint32_t)(tr_max->x1 - tr_max->x0);
	uint32_t h = (uint32_t)(tr_max->y1 - tr_max->y0);
	uint32_t resno, bandno, precno, cblkno;
	opj_sparse_array_int32_t* sa = opj_sparse_array_int32_create(w, h, smin(w, 64U), smin(h, 64U));
	if(sa == NULL) {
		return NULL;
	}
	for(resno = 0; resno < numres; ++resno) {
		opj_tcd_resolution_t* res = &tilec->resolutions[resno];
		for(bandno = 0; bandno < res->numbands; ++bandno) {
			opj_tcd_band_t* band = &res->bands[bandno];
			for(precno = 0; precno < res->pw * res->ph; ++precno) {
				opj_tcd_precinct_t* precinct = &band->precincts[precno];
				for(cblkno = 0; cblkno < precinct->cw * precinct->ch; ++cblkno) {
					opj_tcd_cblk_dec_t* cblk = &precinct->cblks.dec[cblkno];
					if(cblk->decoded_data != NULL) {
						uint32_t x = (uint32_t)(cblk->x0 - band->x0);
						uint32_t y = (uint32_t)(cblk->y0 - band->y0);
						uint32_t cblk_w = (uint32_t)(cblk->x1 - cblk->x0);
						uint32_t cblk_h = (uint32_t)(cblk->y1 - cblk->y0);
						if(band->bandno & 1) {
							opj_tcd_resolution_t* pres = &tilec->resolutions[resno - 1];
							x += (uint32_t)(pres->x1 - pres->x0);
						}
						if(band->bandno & 2) {
							opj_tcd_resolution_t* pres = &tilec->resolutions[resno - 1];
							y += (uint32_t)(pres->y1 - pres->y0);
						}
						if(!opj_sparse_array_int32_write(sa, x, y, x + cblk_w, y + cblk_h, cblk->decoded_data, 1, cblk_w, TRUE)) {
							opj_sparse_array_int32_free(sa);
							return NULL;
						}
					}
				}
			}
		}
	}
	return sa;
}

static boolint opj_dwt_decode_partial_tile(opj_tcd_tilecomp_t* tilec, uint32_t numres)
{
	opj_sparse_array_int32_t* sa;
	opj_dwt_t h;
	opj_dwt_t v;
	uint32_t resno;
	/* This value matches the maximum left/right extension given in tables */
	/* F.2 and F.3 of the standard. */
	const uint32_t filter_width = 2U;
	opj_tcd_resolution_t* tr = tilec->resolutions;
	opj_tcd_resolution_t* tr_max = &(tilec->resolutions[numres - 1]);
	uint32_t rw = (uint32_t)(tr->x1 - tr->x0); /* width of the resolution level computed */
	uint32_t rh = (uint32_t)(tr->y1 - tr->y0); /* height of the resolution level computed */
	size_t h_mem_size;
	/* Compute the intersection of the area of interest, expressed in tile coordinates */
	/* with the tile coordinates */
	uint32_t win_tcx0 = tilec->win_x0;
	uint32_t win_tcy0 = tilec->win_y0;
	uint32_t win_tcx1 = tilec->win_x1;
	uint32_t win_tcy1 = tilec->win_y1;
	if(tr_max->x0 == tr_max->x1 || tr_max->y0 == tr_max->y1) {
		return TRUE;
	}
	sa = opj_dwt_init_sparse_array(tilec, numres);
	if(sa == NULL) {
		return FALSE;
	}
	if(numres == 1U) {
		boolint ret = opj_sparse_array_int32_read(sa, tr_max->win_x0 - (uint32_t)tr_max->x0,
			tr_max->win_y0 - (uint32_t)tr_max->y0, tr_max->win_x1 - (uint32_t)tr_max->x0, tr_max->win_y1 - (uint32_t)tr_max->y0,
			tilec->data_win, 1, tr_max->win_x1 - tr_max->win_x0, TRUE);
		assert(ret);
		CXX_UNUSED(ret);
		opj_sparse_array_int32_free(sa);
		return TRUE;
	}
	h_mem_size = opj_dwt_max_resolution(tr, numres);
	/* overflow check */
	/* in vertical pass, we process 4 columns at a time */
	if(h_mem_size > (SIZE_MAX / (4 * sizeof(int32_t)))) {
		/* FIXME event manager error callback */
		opj_sparse_array_int32_free(sa);
		return FALSE;
	}
	h_mem_size *= 4 * sizeof(int32_t);
	h.mem = (int32_t*)opj_aligned_32_malloc(h_mem_size);
	if(!h.mem) {
		/* FIXME event manager error callback */
		opj_sparse_array_int32_free(sa);
		return FALSE;
	}
	v.mem = h.mem;
	for(resno = 1; resno < numres; resno++) {
		uint32_t i, j;
		/* Window of interest subband-based coordinates */
		uint32_t win_ll_x0, win_ll_y0, win_ll_x1, win_ll_y1;
		uint32_t win_hl_x0, win_hl_x1;
		uint32_t win_lh_y0, win_lh_y1;
		/* Window of interest tile-resolution-based coordinates */
		uint32_t win_tr_x0, win_tr_x1, win_tr_y0, win_tr_y1;
		/* Tile-resolution subband-based coordinates */
		uint32_t tr_ll_x0, tr_ll_y0, tr_hl_x0, tr_lh_y0;
		++tr;
		h.sn = (int32_t)rw;
		v.sn = (int32_t)rh;
		rw = (uint32_t)(tr->x1 - tr->x0);
		rh = (uint32_t)(tr->y1 - tr->y0);
		h.dn = (int32_t)(rw - (uint32_t)h.sn);
		h.cas = tr->x0 % 2;
		v.dn = (int32_t)(rh - (uint32_t)v.sn);
		v.cas = tr->y0 % 2;
		/* Get the subband coordinates for the window of interest */
		/* LL band */
		opj_dwt_get_band_coordinates(tilec, resno, 0,
		    win_tcx0, win_tcy0, win_tcx1, win_tcy1,
		    &win_ll_x0, &win_ll_y0,
		    &win_ll_x1, &win_ll_y1);
		/* HL band */
		opj_dwt_get_band_coordinates(tilec, resno, 1,
		    win_tcx0, win_tcy0, win_tcx1, win_tcy1,
		    &win_hl_x0, NULL, &win_hl_x1, NULL);
		/* LH band */
		opj_dwt_get_band_coordinates(tilec, resno, 2,
		    win_tcx0, win_tcy0, win_tcx1, win_tcy1,
		    NULL, &win_lh_y0, NULL, &win_lh_y1);
		/* Beware: band index for non-LL0 resolution are 0=HL, 1=LH and 2=HH */
		tr_ll_x0 = (uint32_t)tr->bands[1].x0;
		tr_ll_y0 = (uint32_t)tr->bands[0].y0;
		tr_hl_x0 = (uint32_t)tr->bands[0].x0;
		tr_lh_y0 = (uint32_t)tr->bands[1].y0;
		/* Subtract the origin of the bands for this tile, to the subwindow */
		/* of interest band coordinates, so as to get them relative to the */
		/* tile */
		win_ll_x0 = opj_uint_subs(win_ll_x0, tr_ll_x0);
		win_ll_y0 = opj_uint_subs(win_ll_y0, tr_ll_y0);
		win_ll_x1 = opj_uint_subs(win_ll_x1, tr_ll_x0);
		win_ll_y1 = opj_uint_subs(win_ll_y1, tr_ll_y0);
		win_hl_x0 = opj_uint_subs(win_hl_x0, tr_hl_x0);
		win_hl_x1 = opj_uint_subs(win_hl_x1, tr_hl_x0);
		win_lh_y0 = opj_uint_subs(win_lh_y0, tr_lh_y0);
		win_lh_y1 = opj_uint_subs(win_lh_y1, tr_lh_y0);
		opj_dwt_segment_grow(filter_width, (uint32_t)h.sn, &win_ll_x0, &win_ll_x1);
		opj_dwt_segment_grow(filter_width, (uint32_t)h.dn, &win_hl_x0, &win_hl_x1);
		opj_dwt_segment_grow(filter_width, (uint32_t)v.sn, &win_ll_y0, &win_ll_y1);
		opj_dwt_segment_grow(filter_width, (uint32_t)v.dn, &win_lh_y0, &win_lh_y1);
		/* Compute the tile-resolution-based coordinates for the window of interest */
		if(h.cas == 0) {
			win_tr_x0 = smin(2 * win_ll_x0, 2 * win_hl_x0 + 1);
			win_tr_x1 = smin(smax(2 * win_ll_x1, 2 * win_hl_x1 + 1), rw);
		}
		else {
			win_tr_x0 = smin(2 * win_hl_x0, 2 * win_ll_x0 + 1);
			win_tr_x1 = smin(smax(2 * win_hl_x1, 2 * win_ll_x1 + 1), rw);
		}
		if(v.cas == 0) {
			win_tr_y0 = smin(2 * win_ll_y0, 2 * win_lh_y0 + 1);
			win_tr_y1 = smin(smax(2 * win_ll_y1, 2 * win_lh_y1 + 1), rh);
		}
		else {
			win_tr_y0 = smin(2 * win_lh_y0, 2 * win_ll_y0 + 1);
			win_tr_y1 = smin(smax(2 * win_lh_y1, 2 * win_ll_y1 + 1), rh);
		}
		for(j = 0; j < rh; ++j) {
			if((j >= win_ll_y0 && j < win_ll_y1) || (j >= win_lh_y0 + (uint32_t)v.sn && j < win_lh_y1 + (uint32_t)v.sn)) {
				/* Avoids dwt.c:1584:44 (in opj_dwt_decode_partial_1): runtime error: */
				/* signed integer overflow: -1094795586 + -1094795586 cannot be represented in type 'int' */
				/* on opj_decompress -i  ../../openjpeg/MAPA.jp2 -o out.tif -d 0,0,256,256 */
				/* This is less extreme than memsetting the whole buffer to 0 */
				/* although we could potentially do better with better handling of edge conditions */
				if(win_tr_x1 >= 1 && win_tr_x1 < rw) {
					h.mem[win_tr_x1 - 1] = 0;
				}
				if(win_tr_x1 < rw) {
					h.mem[win_tr_x1] = 0;
				}
				opj_dwt_interleave_partial_h(h.mem, h.cas, sa, j, (uint32_t)h.sn,
				    win_ll_x0, win_ll_x1, win_hl_x0, win_hl_x1);
				opj_dwt_decode_partial_1(h.mem, h.dn, h.sn, h.cas, (int32_t)win_ll_x0,
				    (int32_t)win_ll_x1, (int32_t)win_hl_x0, (int32_t)win_hl_x1);
				if(!opj_sparse_array_int32_write(sa, win_tr_x0, j, win_tr_x1, j + 1,
				    h.mem + win_tr_x0, 1, 0, TRUE)) {
					/* FIXME event manager error callback */
					opj_sparse_array_int32_free(sa);
					opj_aligned_free(h.mem);
					return FALSE;
				}
			}
		}
		for(i = win_tr_x0; i < win_tr_x1;) {
			uint32_t nb_cols = smin(4U, win_tr_x1 - i);
			opj_dwt_interleave_partial_v(v.mem, v.cas, sa, i, nb_cols,
			    (uint32_t)v.sn, win_ll_y0, win_ll_y1, win_lh_y0, win_lh_y1);
			opj_dwt_decode_partial_1_parallel(v.mem, nb_cols, v.dn, v.sn, v.cas,
			    (int32_t)win_ll_y0, (int32_t)win_ll_y1, (int32_t)win_lh_y0, (int32_t)win_lh_y1);
			if(!opj_sparse_array_int32_write(sa, i, win_tr_y0, i + nb_cols, win_tr_y1,
			    v.mem + 4 * win_tr_y0, 1, 4, TRUE)) {
				/* FIXME event manager error callback */
				opj_sparse_array_int32_free(sa);
				opj_aligned_free(h.mem);
				return FALSE;
			}
			i += nb_cols;
		}
	}
	opj_aligned_free(h.mem);
	{
		boolint ret = opj_sparse_array_int32_read(sa,
			tr_max->win_x0 - (uint32_t)tr_max->x0,
			tr_max->win_y0 - (uint32_t)tr_max->y0,
			tr_max->win_x1 - (uint32_t)tr_max->x0,
			tr_max->win_y1 - (uint32_t)tr_max->y0,
			tilec->data_win,
			1, tr_max->win_x1 - tr_max->win_x0,
			TRUE);
		assert(ret);
		CXX_UNUSED(ret);
	}
	opj_sparse_array_int32_free(sa);
	return TRUE;
}

static void opj_v8dwt_interleave_h(opj_v8dwt_t* _RESTRICT dwt,
    float* _RESTRICT a,
    uint32_t width,
    uint32_t remaining_height)
{
	float* _RESTRICT bi = (float*)(dwt->wavelet + dwt->cas);
	uint32_t i, k;
	uint32_t x0 = dwt->win_l_x0;
	uint32_t x1 = dwt->win_l_x1;

	for(k = 0; k < 2; ++k) {
		if(remaining_height >= NB_ELTS_V8 && ((size_t)a & 0x0f) == 0 &&
		    ((size_t)bi & 0x0f) == 0) {
			/* Fast code path */
			for(i = x0; i < x1; ++i) {
				uint32_t j = i;
				float* _RESTRICT dst = bi + i * 2 * NB_ELTS_V8;
				dst[0] = a[j];
				j += width;
				dst[1] = a[j];
				j += width;
				dst[2] = a[j];
				j += width;
				dst[3] = a[j];
				j += width;
				dst[4] = a[j];
				j += width;
				dst[5] = a[j];
				j += width;
				dst[6] = a[j];
				j += width;
				dst[7] = a[j];
			}
		}
		else {
			/* Slow code path */
			for(i = x0; i < x1; ++i) {
				uint32_t j = i;
				float* _RESTRICT dst = bi + i * 2 * NB_ELTS_V8;
				dst[0] = a[j];
				j += width;
				if(remaining_height == 1) {
					continue;
				}
				dst[1] = a[j];
				j += width;
				if(remaining_height == 2) {
					continue;
				}
				dst[2] = a[j];
				j += width;
				if(remaining_height == 3) {
					continue;
				}
				dst[3] = a[j];
				j += width;
				if(remaining_height == 4) {
					continue;
				}
				dst[4] = a[j];
				j += width;
				if(remaining_height == 5) {
					continue;
				}
				dst[5] = a[j];
				j += width;
				if(remaining_height == 6) {
					continue;
				}
				dst[6] = a[j];
				j += width;
				if(remaining_height == 7) {
					continue;
				}
				dst[7] = a[j];
			}
		}

		bi = (float*)(dwt->wavelet + 1 - dwt->cas);
		a += dwt->sn;
		x0 = dwt->win_h_x0;
		x1 = dwt->win_h_x1;
	}
}

static void opj_v8dwt_interleave_partial_h(opj_v8dwt_t* dwt,
    opj_sparse_array_int32_t* sa,
    uint32_t sa_line,
    uint32_t remaining_height)
{
	uint32_t i;
	for(i = 0; i < remaining_height; i++) {
		boolint ret = opj_sparse_array_int32_read(sa, dwt->win_l_x0, sa_line + i, dwt->win_l_x1, sa_line + i + 1, /* Nasty cast from float* to int32* */
			(int32_t*)(dwt->wavelet + dwt->cas + 2 * dwt->win_l_x0) + i, 2 * NB_ELTS_V8, 0, TRUE);
		assert(ret);
		ret = opj_sparse_array_int32_read(sa, (uint32_t)dwt->sn + dwt->win_h_x0, sa_line + i,
			(uint32_t)dwt->sn + dwt->win_h_x1, sa_line + i + 1, /* Nasty cast from float* to int32* */ (int32_t*)(dwt->wavelet + 1 - dwt->cas + 2 * dwt->win_h_x0) + i,
			2 * NB_ELTS_V8, 0, TRUE);
		assert(ret);
		CXX_UNUSED(ret);
	}
}

static INLINE void opj_v8dwt_interleave_v(opj_v8dwt_t* _RESTRICT dwt, float* _RESTRICT a, uint32_t width, uint32_t nb_elts_read)
{
	opj_v8_t* _RESTRICT bi = dwt->wavelet + dwt->cas;
	uint32_t i;
	for(i = dwt->win_l_x0; i < dwt->win_l_x1; ++i) {
		memcpy(&bi[i * 2], &a[i * (size_t)width], (size_t)nb_elts_read * sizeof(float));
	}
	a += (uint32_t)dwt->sn * (size_t)width;
	bi = dwt->wavelet + 1 - dwt->cas;
	for(i = dwt->win_h_x0; i < dwt->win_h_x1; ++i) {
		memcpy(&bi[i * 2], &a[i * (size_t)width], (size_t)nb_elts_read * sizeof(float));
	}
}

static void opj_v8dwt_interleave_partial_v(opj_v8dwt_t* _RESTRICT dwt, opj_sparse_array_int32_t* sa, uint32_t sa_col, uint32_t nb_elts_read)
{
	boolint ret = opj_sparse_array_int32_read(sa, sa_col, dwt->win_l_x0, sa_col + nb_elts_read, dwt->win_l_x1, (int32_t*)(dwt->wavelet + dwt->cas + 2 * dwt->win_l_x0), 1, 2 * NB_ELTS_V8, TRUE);
	assert(ret);
	ret = opj_sparse_array_int32_read(sa, sa_col, (uint32_t)dwt->sn + dwt->win_h_x0, sa_col + nb_elts_read, (uint32_t)dwt->sn + dwt->win_h_x1,
		(int32_t*)(dwt->wavelet + 1 - dwt->cas + 2 * dwt->win_h_x0), 1, 2 * NB_ELTS_V8, TRUE);
	assert(ret);
	CXX_UNUSED(ret);
}

#ifdef __SSE__

static void opj_v8dwt_decode_step1_sse(opj_v8_t* w, uint32_t start, uint32_t end, const __m128 c)
{
	__m128* _RESTRICT vw = (__m128*)w;
	uint32_t i = start;
	/* To be adapted if NB_ELTS_V8 changes */
	vw += 4 * start;
	/* Note: attempt at loop unrolling x2 doesn't help */
	for(; i < end; ++i, vw += 4) {
		vw[0] = _mm_mul_ps(vw[0], c);
		vw[1] = _mm_mul_ps(vw[1], c);
	}
}

static void opj_v8dwt_decode_step2_sse(opj_v8_t* l, opj_v8_t* w, uint32_t start, uint32_t end, uint32_t m, __m128 c)
{
	__m128* _RESTRICT vl = (__m128*)l;
	__m128* _RESTRICT vw = (__m128*)w;
	/* To be adapted if NB_ELTS_V8 changes */
	uint32_t i;
	uint32_t imax = smin(end, m);
	if(start == 0) {
		if(imax >= 1) {
			vw[-2] = _mm_add_ps(vw[-2], _mm_mul_ps(_mm_add_ps(vl[0], vw[0]), c));
			vw[-1] = _mm_add_ps(vw[-1], _mm_mul_ps(_mm_add_ps(vl[1], vw[1]), c));
			vw += 4;
			start = 1;
		}
	}
	else {
		vw += start * 4;
	}

	i = start;
	/* Note: attempt at loop unrolling x2 doesn't help */
	for(; i < imax; ++i) {
		vw[-2] = _mm_add_ps(vw[-2], _mm_mul_ps(_mm_add_ps(vw[-4], vw[0]), c));
		vw[-1] = _mm_add_ps(vw[-1], _mm_mul_ps(_mm_add_ps(vw[-3], vw[1]), c));
		vw += 4;
	}
	if(m < end) {
		assert(m + 1 == end);
		c = _mm_add_ps(c, c);
		vw[-2] = _mm_add_ps(vw[-2], _mm_mul_ps(c, vw[-4]));
		vw[-1] = _mm_add_ps(vw[-1], _mm_mul_ps(c, vw[-3]));
	}
}

#else

static void opj_v8dwt_decode_step1(opj_v8_t* w, uint32_t start, uint32_t end, const float c)
{
	float* _RESTRICT fw = (float*)w;
	/* To be adapted if NB_ELTS_V8 changes */
	for(uint32_t i = start; i < end; ++i) {
		fw[i * 2 * 8    ] = fw[i * 2 * 8    ] * c;
		fw[i * 2 * 8 + 1] = fw[i * 2 * 8 + 1] * c;
		fw[i * 2 * 8 + 2] = fw[i * 2 * 8 + 2] * c;
		fw[i * 2 * 8 + 3] = fw[i * 2 * 8 + 3] * c;
		fw[i * 2 * 8 + 4] = fw[i * 2 * 8 + 4] * c;
		fw[i * 2 * 8 + 5] = fw[i * 2 * 8 + 5] * c;
		fw[i * 2 * 8 + 6] = fw[i * 2 * 8 + 6] * c;
		fw[i * 2 * 8 + 7] = fw[i * 2 * 8 + 7] * c;
	}
}

static void opj_v8dwt_decode_step2(opj_v8_t* l, opj_v8_t* w, uint32_t start, uint32_t end, uint32_t m, float c)
{
	float* fl = (float*)l;
	float* fw = (float*)w;
	uint32_t i;
	uint32_t imax = smin(end, m);
	if(start > 0) {
		fw += 2 * NB_ELTS_V8 * start;
		fl = fw - 2 * NB_ELTS_V8;
	}
	/* To be adapted if NB_ELTS_V8 changes */
	for(i = start; i < imax; ++i) {
		fw[-8] = fw[-8] + ((fl[0] + fw[0]) * c);
		fw[-7] = fw[-7] + ((fl[1] + fw[1]) * c);
		fw[-6] = fw[-6] + ((fl[2] + fw[2]) * c);
		fw[-5] = fw[-5] + ((fl[3] + fw[3]) * c);
		fw[-4] = fw[-4] + ((fl[4] + fw[4]) * c);
		fw[-3] = fw[-3] + ((fl[5] + fw[5]) * c);
		fw[-2] = fw[-2] + ((fl[6] + fw[6]) * c);
		fw[-1] = fw[-1] + ((fl[7] + fw[7]) * c);
		fl = fw;
		fw += 2 * NB_ELTS_V8;
	}
	if(m < end) {
		assert(m + 1 == end);
		c += c;
		fw[-8] = fw[-8] + fl[0] * c;
		fw[-7] = fw[-7] + fl[1] * c;
		fw[-6] = fw[-6] + fl[2] * c;
		fw[-5] = fw[-5] + fl[3] * c;
		fw[-4] = fw[-4] + fl[4] * c;
		fw[-3] = fw[-3] + fl[5] * c;
		fw[-2] = fw[-2] + fl[6] * c;
		fw[-1] = fw[-1] + fl[7] * c;
	}
}

#endif

/* <summary>                             */
/* Inverse 9-7 wavelet transform in 1-D. */
/* </summary>                            */
static void opj_v8dwt_decode(opj_v8dwt_t* _RESTRICT dwt)
{
	int32_t a, b;
	/* BUG_WEIRD_TWO_INVK (look for this identifier in tcd.c) */
	/* Historic value for 2 / opj_invK */
	/* Normally, we should use invK, but if we do so, we have failures in the */
	/* conformance test, due to MSE and peak errors significantly higher than */
	/* accepted value */
	/* Due to using two_invK instead of invK, we have to compensate in tcd.c */
	/* the computation of the stepsize for the non LL subbands */
	const float two_invK = 1.625732422f;
	if(dwt->cas == 0) {
		if(!((dwt->dn > 0) || (dwt->sn > 1))) {
			return;
		}
		a = 0;
		b = 1;
	}
	else {
		if(!((dwt->sn > 0) || (dwt->dn > 1))) {
			return;
		}
		a = 1;
		b = 0;
	}
#ifdef __SSE__
	opj_v8dwt_decode_step1_sse(dwt->wavelet + a, dwt->win_l_x0, dwt->win_l_x1, _mm_set1_ps(opj_K));
	opj_v8dwt_decode_step1_sse(dwt->wavelet + b, dwt->win_h_x0, dwt->win_h_x1, _mm_set1_ps(two_invK));
	opj_v8dwt_decode_step2_sse(dwt->wavelet + b, dwt->wavelet + a + 1, dwt->win_l_x0, dwt->win_l_x1, (uint32_t)smin(dwt->sn, dwt->dn - a), _mm_set1_ps(-opj_dwt_delta));
	opj_v8dwt_decode_step2_sse(dwt->wavelet + a, dwt->wavelet + b + 1, dwt->win_h_x0, dwt->win_h_x1, (uint32_t)smin(dwt->dn, dwt->sn - b), _mm_set1_ps(-opj_dwt_gamma));
	opj_v8dwt_decode_step2_sse(dwt->wavelet + b, dwt->wavelet + a + 1, dwt->win_l_x0, dwt->win_l_x1, (uint32_t)smin(dwt->sn, dwt->dn - a), _mm_set1_ps(-opj_dwt_beta));
	opj_v8dwt_decode_step2_sse(dwt->wavelet + a, dwt->wavelet + b + 1, dwt->win_h_x0, dwt->win_h_x1, (uint32_t)smin(dwt->dn, dwt->sn - b), _mm_set1_ps(-opj_dwt_alpha));
#else
	opj_v8dwt_decode_step1(dwt->wavelet + a, dwt->win_l_x0, dwt->win_l_x1, opj_K);
	opj_v8dwt_decode_step1(dwt->wavelet + b, dwt->win_h_x0, dwt->win_h_x1, two_invK);
	opj_v8dwt_decode_step2(dwt->wavelet + b, dwt->wavelet + a + 1, dwt->win_l_x0, dwt->win_l_x1, (uint32_t)smin(dwt->sn, dwt->dn - a), -opj_dwt_delta);
	opj_v8dwt_decode_step2(dwt->wavelet + a, dwt->wavelet + b + 1, dwt->win_h_x0, dwt->win_h_x1, (uint32_t)smin(dwt->dn, dwt->sn - b), -opj_dwt_gamma);
	opj_v8dwt_decode_step2(dwt->wavelet + b, dwt->wavelet + a + 1, dwt->win_l_x0, dwt->win_l_x1, (uint32_t)smin(dwt->sn, dwt->dn - a), -opj_dwt_beta);
	opj_v8dwt_decode_step2(dwt->wavelet + a, dwt->wavelet + b + 1, dwt->win_h_x0, dwt->win_h_x1, (uint32_t)smin(dwt->dn, dwt->sn - b), -opj_dwt_alpha);
#endif
}

typedef struct {
	opj_v8dwt_t h;
	uint32_t rw;
	uint32_t w;
	float * _RESTRICT aj;
	uint32_t nb_rows;
} opj_dwt97_decode_h_job_t;

static void opj_dwt97_decode_h_func(void* user_data, opj_tls_t* tls)
{
	uint32_t j;
	float * _RESTRICT aj;
	opj_dwt97_decode_h_job_t* job = (opj_dwt97_decode_h_job_t*)user_data;
	uint32_t w = job->w;
	(void)tls;
	assert((job->nb_rows % NB_ELTS_V8) == 0);
	aj = job->aj;
	for(j = 0; j + NB_ELTS_V8 <= job->nb_rows; j += NB_ELTS_V8) {
		uint32_t k;
		opj_v8dwt_interleave_h(&job->h, aj, job->w, NB_ELTS_V8);
		opj_v8dwt_decode(&job->h);
		/* To be adapted if NB_ELTS_V8 changes */
		for(k = 0; k < job->rw; k++) {
			aj[k      ] = job->h.wavelet[k].f[0];
			aj[k + (size_t)w  ] = job->h.wavelet[k].f[1];
			aj[k + (size_t)w * 2] = job->h.wavelet[k].f[2];
			aj[k + (size_t)w * 3] = job->h.wavelet[k].f[3];
		}
		for(k = 0; k < job->rw; k++) {
			aj[k + (size_t)w * 4] = job->h.wavelet[k].f[4];
			aj[k + (size_t)w * 5] = job->h.wavelet[k].f[5];
			aj[k + (size_t)w * 6] = job->h.wavelet[k].f[6];
			aj[k + (size_t)w * 7] = job->h.wavelet[k].f[7];
		}
		aj += w * NB_ELTS_V8;
	}
	opj_aligned_free(job->h.wavelet);
	SAlloc::F(job);
}

typedef struct {
	opj_v8dwt_t v;
	uint32_t rh;
	uint32_t w;
	float * _RESTRICT aj;
	uint32_t nb_columns;
} opj_dwt97_decode_v_job_t;

static void opj_dwt97_decode_v_func(void* user_data, opj_tls_t* tls)
{
	uint32_t j;
	float * _RESTRICT aj;
	opj_dwt97_decode_v_job_t * job = (opj_dwt97_decode_v_job_t*)user_data;
	(void)tls;
	assert((job->nb_columns % NB_ELTS_V8) == 0);
	aj = job->aj;
	for(j = 0; j + NB_ELTS_V8 <= job->nb_columns; j += NB_ELTS_V8) {
		uint32_t k;
		opj_v8dwt_interleave_v(&job->v, aj, job->w, NB_ELTS_V8);
		opj_v8dwt_decode(&job->v);
		for(k = 0; k < job->rh; ++k) {
			memcpy(&aj[k * (size_t)job->w], &job->v.wavelet[k], NB_ELTS_V8 * sizeof(float));
		}
		aj += NB_ELTS_V8;
	}
	opj_aligned_free(job->v.wavelet);
	SAlloc::F(job);
}

/* <summary>                             */
/* Inverse 9-7 wavelet transform in 2-D. */
/* </summary>                            */
static boolint opj_dwt_decode_tile_97(opj_thread_pool_t* tp, opj_tcd_tilecomp_t* _RESTRICT tilec, uint32_t numres)
{
	opj_v8dwt_t h;
	opj_v8dwt_t v;
	opj_tcd_resolution_t* res = tilec->resolutions;
	uint32_t rw = (uint32_t)(res->x1 - res->x0); /* width of the resolution level computed */
	uint32_t rh = (uint32_t)(res->y1 - res->y0); /* height of the resolution level computed */
	uint32_t w = (uint32_t)(tilec->resolutions[tilec->minimum_num_resolutions - 1].x1 - tilec->resolutions[tilec->minimum_num_resolutions - 1].x0);
	size_t l_data_size;
	const int num_threads = opj_thread_pool_get_thread_count(tp);
	if(numres == 1) {
		return TRUE;
	}
	l_data_size = opj_dwt_max_resolution(res, numres);
	/* overflow check */
	if(l_data_size > (SIZE_MAX / sizeof(opj_v8_t))) {
		/* FIXME event manager error callback */
		return FALSE;
	}
	h.wavelet = (opj_v8_t*)opj_aligned_malloc(l_data_size * sizeof(opj_v8_t));
	if(!h.wavelet) {
		/* FIXME event manager error callback */
		return FALSE;
	}
	v.wavelet = h.wavelet;
	while(--numres) {
		float * _RESTRICT aj = (float*)tilec->data;
		uint32_t j;
		h.sn = (int32_t)rw;
		v.sn = (int32_t)rh;
		++res;
		rw = (uint32_t)(res->x1 - res->x0); /* width of the resolution level computed */
		rh = (uint32_t)(res->y1 - res->y0); /* height of the resolution level computed */
		h.dn = (int32_t)(rw - (uint32_t)h.sn);
		h.cas = res->x0 % 2;
		h.win_l_x0 = 0;
		h.win_l_x1 = (uint32_t)h.sn;
		h.win_h_x0 = 0;
		h.win_h_x1 = (uint32_t)h.dn;
		if(num_threads <= 1 || rh < 2 * NB_ELTS_V8) {
			for(j = 0; j + (NB_ELTS_V8 - 1) < rh; j += NB_ELTS_V8) {
				uint32_t k;
				opj_v8dwt_interleave_h(&h, aj, w, NB_ELTS_V8);
				opj_v8dwt_decode(&h);
				/* To be adapted if NB_ELTS_V8 changes */
				for(k = 0; k < rw; k++) {
					aj[k      ] = h.wavelet[k].f[0];
					aj[k + (size_t)w  ] = h.wavelet[k].f[1];
					aj[k + (size_t)w * 2] = h.wavelet[k].f[2];
					aj[k + (size_t)w * 3] = h.wavelet[k].f[3];
				}
				for(k = 0; k < rw; k++) {
					aj[k + (size_t)w * 4] = h.wavelet[k].f[4];
					aj[k + (size_t)w * 5] = h.wavelet[k].f[5];
					aj[k + (size_t)w * 6] = h.wavelet[k].f[6];
					aj[k + (size_t)w * 7] = h.wavelet[k].f[7];
				}
				aj += w * NB_ELTS_V8;
			}
		}
		else {
			uint32_t num_jobs = (uint32_t)num_threads;
			uint32_t step_j;
			if((rh / NB_ELTS_V8) < num_jobs) {
				num_jobs = rh / NB_ELTS_V8;
			}
			step_j = ((rh / num_jobs) / NB_ELTS_V8) * NB_ELTS_V8;
			for(j = 0; j < num_jobs; j++) {
				opj_dwt97_decode_h_job_t * job = (opj_dwt97_decode_h_job_t*)opj_malloc(sizeof(opj_dwt97_decode_h_job_t));
				if(!job) {
					opj_thread_pool_wait_completion(tp, 0);
					opj_aligned_free(h.wavelet);
					return FALSE;
				}
				job->h.wavelet = (opj_v8_t*)opj_aligned_malloc(l_data_size * sizeof(opj_v8_t));
				if(!job->h.wavelet) {
					opj_thread_pool_wait_completion(tp, 0);
					SAlloc::F(job);
					opj_aligned_free(h.wavelet);
					return FALSE;
				}
				job->h.dn = h.dn;
				job->h.sn = h.sn;
				job->h.cas = h.cas;
				job->h.win_l_x0 = h.win_l_x0;
				job->h.win_l_x1 = h.win_l_x1;
				job->h.win_h_x0 = h.win_h_x0;
				job->h.win_h_x1 = h.win_h_x1;
				job->rw = rw;
				job->w = w;
				job->aj = aj;
				job->nb_rows = (j + 1 == num_jobs) ? (rh & (uint32_t) ~ (NB_ELTS_V8 - 1)) - j * step_j : step_j;
				aj += w * job->nb_rows;
				opj_thread_pool_submit_job(tp, opj_dwt97_decode_h_func, job);
			}
			opj_thread_pool_wait_completion(tp, 0);
			j = rh & (uint32_t) ~(NB_ELTS_V8 - 1);
		}
		if(j < rh) {
			opj_v8dwt_interleave_h(&h, aj, w, rh - j);
			opj_v8dwt_decode(&h);
			for(uint32_t k = 0; k < rw; k++) {
				for(uint32_t l = 0; l < rh - j; l++) {
					aj[k + (size_t)w  * l ] = h.wavelet[k].f[l];
				}
			}
		}
		v.dn = (int32_t)(rh - (uint32_t)v.sn);
		v.cas = res->y0 % 2;
		v.win_l_x0 = 0;
		v.win_l_x1 = (uint32_t)v.sn;
		v.win_h_x0 = 0;
		v.win_h_x1 = (uint32_t)v.dn;
		aj = (float*)tilec->data;
		if(num_threads <= 1 || rw < 2 * NB_ELTS_V8) {
			for(j = rw; j > (NB_ELTS_V8 - 1); j -= NB_ELTS_V8) {
				uint32_t k;
				opj_v8dwt_interleave_v(&v, aj, w, NB_ELTS_V8);
				opj_v8dwt_decode(&v);
				for(k = 0; k < rh; ++k) {
					memcpy(&aj[k * (size_t)w], &v.wavelet[k], NB_ELTS_V8 * sizeof(float));
				}
				aj += NB_ELTS_V8;
			}
		}
		else {
			/* "bench_dwt -I" shows that scaling is poor, likely due to RAM
			    transfer being the limiting factor. So limit the number of
			    threads.
			 */
			uint32_t num_jobs = smax((uint32_t)num_threads / 2, 2U);
			uint32_t step_j;
			if((rw / NB_ELTS_V8) < num_jobs) {
				num_jobs = rw / NB_ELTS_V8;
			}
			step_j = ((rw / num_jobs) / NB_ELTS_V8) * NB_ELTS_V8;
			for(j = 0; j < num_jobs; j++) {
				opj_dwt97_decode_v_job_t* job;
				job = (opj_dwt97_decode_v_job_t*)opj_malloc(sizeof(opj_dwt97_decode_v_job_t));
				if(!job) {
					opj_thread_pool_wait_completion(tp, 0);
					opj_aligned_free(h.wavelet);
					return FALSE;
				}
				job->v.wavelet = (opj_v8_t*)opj_aligned_malloc(l_data_size * sizeof(opj_v8_t));
				if(!job->v.wavelet) {
					opj_thread_pool_wait_completion(tp, 0);
					SAlloc::F(job);
					opj_aligned_free(h.wavelet);
					return FALSE;
				}
				job->v.dn = v.dn;
				job->v.sn = v.sn;
				job->v.cas = v.cas;
				job->v.win_l_x0 = v.win_l_x0;
				job->v.win_l_x1 = v.win_l_x1;
				job->v.win_h_x0 = v.win_h_x0;
				job->v.win_h_x1 = v.win_h_x1;
				job->rh = rh;
				job->w = w;
				job->aj = aj;
				job->nb_columns = (j + 1 == num_jobs) ? (rw & (uint32_t) ~ (NB_ELTS_V8 - 1)) - j * step_j : step_j;
				aj += job->nb_columns;
				opj_thread_pool_submit_job(tp, opj_dwt97_decode_v_func, job);
			}
			opj_thread_pool_wait_completion(tp, 0);
		}
		if(rw & (NB_ELTS_V8 - 1)) {
			uint32_t k;
			j = rw & (NB_ELTS_V8 - 1);
			opj_v8dwt_interleave_v(&v, aj, w, j);
			opj_v8dwt_decode(&v);
			for(k = 0; k < rh; ++k) {
				memcpy(&aj[k * (size_t)w], &v.wavelet[k], (size_t)j * sizeof(float));
			}
		}
	}
	opj_aligned_free(h.wavelet);
	return TRUE;
}

static boolint opj_dwt_decode_partial_97(opj_tcd_tilecomp_t* _RESTRICT tilec, uint32_t numres)
{
	opj_sparse_array_int32_t* sa;
	opj_v8dwt_t h;
	opj_v8dwt_t v;
	uint32_t resno;
	/* This value matches the maximum left/right extension given in tables */
	/* F.2 and F.3 of the standard. Note: in opj_tcd_is_subband_area_of_interest() */
	/* we currently use 3. */
	const uint32_t filter_width = 4U;
	opj_tcd_resolution_t* tr = tilec->resolutions;
	opj_tcd_resolution_t* tr_max = &(tilec->resolutions[numres - 1]);
	uint32_t rw = (uint32_t)(tr->x1 - tr->x0); /* width of the resolution level computed */
	uint32_t rh = (uint32_t)(tr->y1 - tr->y0); /* height of the resolution level computed */
	size_t l_data_size;
	/* Compute the intersection of the area of interest, expressed in tile coordinates */
	/* with the tile coordinates */
	uint32_t win_tcx0 = tilec->win_x0;
	uint32_t win_tcy0 = tilec->win_y0;
	uint32_t win_tcx1 = tilec->win_x1;
	uint32_t win_tcy1 = tilec->win_y1;
	if(tr_max->x0 == tr_max->x1 || tr_max->y0 == tr_max->y1) {
		return TRUE;
	}
	sa = opj_dwt_init_sparse_array(tilec, numres);
	if(sa == NULL) {
		return FALSE;
	}
	if(numres == 1U) {
		boolint ret = opj_sparse_array_int32_read(sa,
			tr_max->win_x0 - (uint32_t)tr_max->x0,
			tr_max->win_y0 - (uint32_t)tr_max->y0,
			tr_max->win_x1 - (uint32_t)tr_max->x0,
			tr_max->win_y1 - (uint32_t)tr_max->y0,
			tilec->data_win,
			1, tr_max->win_x1 - tr_max->win_x0,
			TRUE);
		assert(ret);
		CXX_UNUSED(ret);
		opj_sparse_array_int32_free(sa);
		return TRUE;
	}
	l_data_size = opj_dwt_max_resolution(tr, numres);
	/* overflow check */
	if(l_data_size > (SIZE_MAX / sizeof(opj_v8_t))) {
		/* FIXME event manager error callback */
		opj_sparse_array_int32_free(sa);
		return FALSE;
	}
	h.wavelet = (opj_v8_t*)opj_aligned_malloc(l_data_size * sizeof(opj_v8_t));
	if(!h.wavelet) {
		/* FIXME event manager error callback */
		opj_sparse_array_int32_free(sa);
		return FALSE;
	}
	v.wavelet = h.wavelet;
	for(resno = 1; resno < numres; resno++) {
		uint32_t j;
		/* Window of interest subband-based coordinates */
		uint32_t win_ll_x0, win_ll_y0, win_ll_x1, win_ll_y1;
		uint32_t win_hl_x0, win_hl_x1;
		uint32_t win_lh_y0, win_lh_y1;
		/* Window of interest tile-resolution-based coordinates */
		uint32_t win_tr_x0, win_tr_x1, win_tr_y0, win_tr_y1;
		/* Tile-resolution subband-based coordinates */
		uint32_t tr_ll_x0, tr_ll_y0, tr_hl_x0, tr_lh_y0;
		++tr;
		h.sn = (int32_t)rw;
		v.sn = (int32_t)rh;
		rw = (uint32_t)(tr->x1 - tr->x0);
		rh = (uint32_t)(tr->y1 - tr->y0);
		h.dn = (int32_t)(rw - (uint32_t)h.sn);
		h.cas = tr->x0 % 2;
		v.dn = (int32_t)(rh - (uint32_t)v.sn);
		v.cas = tr->y0 % 2;
		/* Get the subband coordinates for the window of interest */
		/* LL band */
		opj_dwt_get_band_coordinates(tilec, resno, 0,
		    win_tcx0, win_tcy0, win_tcx1, win_tcy1,
		    &win_ll_x0, &win_ll_y0,
		    &win_ll_x1, &win_ll_y1);

		/* HL band */
		opj_dwt_get_band_coordinates(tilec, resno, 1,
		    win_tcx0, win_tcy0, win_tcx1, win_tcy1,
		    &win_hl_x0, NULL, &win_hl_x1, NULL);

		/* LH band */
		opj_dwt_get_band_coordinates(tilec, resno, 2,
		    win_tcx0, win_tcy0, win_tcx1, win_tcy1,
		    NULL, &win_lh_y0, NULL, &win_lh_y1);

		/* Beware: band index for non-LL0 resolution are 0=HL, 1=LH and 2=HH */
		tr_ll_x0 = (uint32_t)tr->bands[1].x0;
		tr_ll_y0 = (uint32_t)tr->bands[0].y0;
		tr_hl_x0 = (uint32_t)tr->bands[0].x0;
		tr_lh_y0 = (uint32_t)tr->bands[1].y0;

		/* Subtract the origin of the bands for this tile, to the subwindow */
		/* of interest band coordinates, so as to get them relative to the */
		/* tile */
		win_ll_x0 = opj_uint_subs(win_ll_x0, tr_ll_x0);
		win_ll_y0 = opj_uint_subs(win_ll_y0, tr_ll_y0);
		win_ll_x1 = opj_uint_subs(win_ll_x1, tr_ll_x0);
		win_ll_y1 = opj_uint_subs(win_ll_y1, tr_ll_y0);
		win_hl_x0 = opj_uint_subs(win_hl_x0, tr_hl_x0);
		win_hl_x1 = opj_uint_subs(win_hl_x1, tr_hl_x0);
		win_lh_y0 = opj_uint_subs(win_lh_y0, tr_lh_y0);
		win_lh_y1 = opj_uint_subs(win_lh_y1, tr_lh_y0);
		opj_dwt_segment_grow(filter_width, (uint32_t)h.sn, &win_ll_x0, &win_ll_x1);
		opj_dwt_segment_grow(filter_width, (uint32_t)h.dn, &win_hl_x0, &win_hl_x1);
		opj_dwt_segment_grow(filter_width, (uint32_t)v.sn, &win_ll_y0, &win_ll_y1);
		opj_dwt_segment_grow(filter_width, (uint32_t)v.dn, &win_lh_y0, &win_lh_y1);
		/* Compute the tile-resolution-based coordinates for the window of interest */
		if(h.cas == 0) {
			win_tr_x0 = smin(2 * win_ll_x0, 2 * win_hl_x0 + 1);
			win_tr_x1 = smin(smax(2 * win_ll_x1, 2 * win_hl_x1 + 1), rw);
		}
		else {
			win_tr_x0 = smin(2 * win_hl_x0, 2 * win_ll_x0 + 1);
			win_tr_x1 = smin(smax(2 * win_hl_x1, 2 * win_ll_x1 + 1), rw);
		}
		if(v.cas == 0) {
			win_tr_y0 = smin(2 * win_ll_y0, 2 * win_lh_y0 + 1);
			win_tr_y1 = smin(smax(2 * win_ll_y1, 2 * win_lh_y1 + 1), rh);
		}
		else {
			win_tr_y0 = smin(2 * win_lh_y0, 2 * win_ll_y0 + 1);
			win_tr_y1 = smin(smax(2 * win_lh_y1, 2 * win_ll_y1 + 1), rh);
		}
		h.win_l_x0 = win_ll_x0;
		h.win_l_x1 = win_ll_x1;
		h.win_h_x0 = win_hl_x0;
		h.win_h_x1 = win_hl_x1;
		for(j = 0; j + (NB_ELTS_V8 - 1) < rh; j += NB_ELTS_V8) {
			if((j + (NB_ELTS_V8 - 1) >= win_ll_y0 && j < win_ll_y1) ||
			    (j + (NB_ELTS_V8 - 1) >= win_lh_y0 + (uint32_t)v.sn && j < win_lh_y1 + (uint32_t)v.sn)) {
				opj_v8dwt_interleave_partial_h(&h, sa, j, smin((uint)NB_ELTS_V8, rh - j));
				opj_v8dwt_decode(&h);
				if(!opj_sparse_array_int32_write(sa, win_tr_x0, j, win_tr_x1, j + NB_ELTS_V8,
				    (int32_t*)&h.wavelet[win_tr_x0].f[0], NB_ELTS_V8, 1, TRUE)) {
					/* FIXME event manager error callback */
					opj_sparse_array_int32_free(sa);
					opj_aligned_free(h.wavelet);
					return FALSE;
				}
			}
		}
		if(j < rh &&
		    ((j + (NB_ELTS_V8 - 1) >= win_ll_y0 && j < win_ll_y1) ||
		    (j + (NB_ELTS_V8 - 1) >= win_lh_y0 + (uint32_t)v.sn &&
		    j < win_lh_y1 + (uint32_t)v.sn))) {
			opj_v8dwt_interleave_partial_h(&h, sa, j, rh - j);
			opj_v8dwt_decode(&h);
			if(!opj_sparse_array_int32_write(sa,
			    win_tr_x0, j,
			    win_tr_x1, rh,
			    (int32_t*)&h.wavelet[win_tr_x0].f[0],
			    NB_ELTS_V8, 1, TRUE)) {
				/* FIXME event manager error callback */
				opj_sparse_array_int32_free(sa);
				opj_aligned_free(h.wavelet);
				return FALSE;
			}
		}
		v.win_l_x0 = win_ll_y0;
		v.win_l_x1 = win_ll_y1;
		v.win_h_x0 = win_lh_y0;
		v.win_h_x1 = win_lh_y1;
		for(j = win_tr_x0; j < win_tr_x1; j += NB_ELTS_V8) {
			uint32_t nb_elts = smin((uint)NB_ELTS_V8, win_tr_x1 - j);
			opj_v8dwt_interleave_partial_v(&v, sa, j, nb_elts);
			opj_v8dwt_decode(&v);
			if(!opj_sparse_array_int32_write(sa, j, win_tr_y0, j + nb_elts, win_tr_y1, (int32_t*)&h.wavelet[win_tr_y0].f[0], 1, NB_ELTS_V8, TRUE)) {
				/* FIXME event manager error callback */
				opj_sparse_array_int32_free(sa);
				opj_aligned_free(h.wavelet);
				return FALSE;
			}
		}
	}
	{
		boolint ret = opj_sparse_array_int32_read(sa, tr_max->win_x0 - (uint32_t)tr_max->x0, tr_max->win_y0 - (uint32_t)tr_max->y0,
			tr_max->win_x1 - (uint32_t)tr_max->x0, tr_max->win_y1 - (uint32_t)tr_max->y0, tilec->data_win, 1, tr_max->win_x1 - tr_max->win_x0, TRUE);
		assert(ret);
		CXX_UNUSED(ret);
	}
	opj_sparse_array_int32_free(sa);
	opj_aligned_free(h.wavelet);
	return TRUE;
}

boolint opj_dwt_decode_real(opj_tcd_t * p_tcd, opj_tcd_tilecomp_t* _RESTRICT tilec, uint32_t numres)
{
	if(p_tcd->whole_tile_decoding) {
		return opj_dwt_decode_tile_97(p_tcd->thread_pool, tilec, numres);
	}
	else {
		return opj_dwt_decode_partial_97(tilec, numres);
	}
}
