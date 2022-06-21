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
 * Copyright (c) 2007, Callum Lerwick <seg@haxxed.com>
 * Copyright (c) 2012, Carl Hetherington
 * Copyright (c) 2017, IntoPIX SA <support@intopix.com>
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
#define OPJ_SKIP_POISON
#ifdef __SSE__
#include <xmmintrin.h>
#endif
#ifdef __SSE2__
#include <emmintrin.h>
#endif

#if defined(__GNUC__)
#pragma GCC poison malloc calloc realloc free
#endif

#include "t1_luts.h"

/** @defgroup T1 T1 - Implementation of the tier-1 coding */
/*@{*/

#define T1_FLAGS(x, y) (t1->flags[x + 1 + ((y / 4) + 1) * (t1->w+2)])

#define opj_t1_setcurctx(curctx, ctxno)  curctx = &(mqc)->ctxs[(uint32_t)(ctxno)]

/* Macros to deal with signed integer with just MSB bit set for
 * negative values (smr = signed magnitude representation) */
#define opj_smr_abs(x)  (((uint32_t)(x)) & 0x7FFFFFFFU)
#define opj_smr_sign(x) (((uint32_t)(x)) >> 31)
#define opj_to_smr(x)   ((x) >= 0 ? (uint32_t)(x) : ((uint32_t)(-x) | 0x80000000U))

/** @name Local static functions */
/*@{*/

static INLINE uint8 opj_t1_getctxno_zc(opj_mqc_t * mqc, uint32_t f);
static INLINE uint32_t opj_t1_getctxno_mag(uint32_t f);
static int16 opj_t1_getnmsedec_sig(uint32_t x, uint32_t bitpos);
static int16 opj_t1_getnmsedec_ref(uint32_t x, uint32_t bitpos);
static INLINE void opj_t1_update_flags(opj_flag_t * flagsp, uint32_t ci, uint32_t s, uint32_t stride, uint32_t vsc);
/**
   Decode significant pass
 */

static INLINE void opj_t1_dec_sigpass_step_raw(opj_t1_t * t1, opj_flag_t * flagsp, int32_t * datap, int32_t oneplushalf, uint32_t vsc, uint32_t row);
static INLINE void opj_t1_dec_sigpass_step_mqc(opj_t1_t * t1, opj_flag_t * flagsp, int32_t * datap, int32_t oneplushalf, uint32_t row, uint32_t flags_stride, uint32_t vsc);
/**
   Encode significant pass
 */
static void opj_t1_enc_sigpass(opj_t1_t * t1, int32_t bpno, int32_t * nmsedec, uint8 type, uint32_t cblksty);
/**
   Decode significant pass
 */
static void opj_t1_dec_sigpass_raw(opj_t1_t * t1,
    int32_t bpno,
    int32_t cblksty);

/**
   Encode refinement pass
 */
static void opj_t1_enc_refpass(opj_t1_t * t1, int32_t bpno, int32_t * nmsedec, uint8 type);
/**
   Decode refinement pass
 */
static void opj_t1_dec_refpass_raw(opj_t1_t * t1, int32_t bpno);
/**
   Decode refinement pass
 */

static INLINE void  opj_t1_dec_refpass_step_raw(opj_t1_t * t1, opj_flag_t * flagsp, int32_t * datap, int32_t poshalf, uint32_t row);
static INLINE void opj_t1_dec_refpass_step_mqc(opj_t1_t * t1, opj_flag_t * flagsp, int32_t * datap, int32_t poshalf, uint32_t row);
/**
   Decode clean-up pass
 */

static void opj_t1_dec_clnpass_step(opj_t1_t * t1, opj_flag_t * flagsp, int32_t * datap, int32_t oneplushalf, uint32_t row, uint32_t vsc);
/**
   Encode clean-up pass
 */
static void opj_t1_enc_clnpass(opj_t1_t * t1, int32_t bpno, int32_t * nmsedec, uint32_t cblksty);
static double opj_t1_getwmsedec(int32_t nmsedec, uint32_t compno, uint32_t level, uint32_t orient, int32_t bpno,
    uint32_t qmfbid, double stepsize, uint32_t numcomps, const double * mct_norms, uint32_t mct_numcomps);
/** Return "cumwmsedec" that should be used to increase tile->distotile */
static double opj_t1_encode_cblk(opj_t1_t * t1, opj_tcd_cblk_enc_t* cblk, uint32_t orient, uint32_t compno, uint32_t level,
    uint32_t qmfbid, double stepsize, uint32_t cblksty, uint32_t numcomps, const double * mct_norms, uint32_t mct_numcomps);
/**
   Decode 1 code-block
   @param t1 T1 handle
   @param cblk Code-block coding parameters
   @param orient
   @param roishift Region of interest shifting value
   @param cblksty Code-block style
   @param p_manager the event manager
   @param p_manager_mutex mutex for the event manager
   @param check_pterm whether PTERM correct termination should be checked
 */
static boolint opj_t1_decode_cblk(opj_t1_t * t1,
    opj_tcd_cblk_dec_t* cblk,
    uint32_t orient,
    uint32_t roishift,
    uint32_t cblksty,
    opj_event_mgr_t * p_manager,
    opj_mutex_t* p_manager_mutex,
    boolint check_pterm);

/**
   Decode 1 HT code-block
   @param t1 T1 handle
   @param cblk Code-block coding parameters
   @param orient
   @param roishift Region of interest shifting value
   @param cblksty Code-block style
   @param p_manager the event manager
   @param p_manager_mutex mutex for the event manager
   @param check_pterm whether PTERM correct termination should be checked
 */
boolint opj_t1_ht_decode_cblk(opj_t1_t * t1, opj_tcd_cblk_dec_t* cblk, uint32_t orient, uint32_t roishift,
    uint32_t cblksty, opj_event_mgr_t * p_manager, opj_mutex_t* p_manager_mutex, boolint check_pterm);
static boolint opj_t1_allocate_buffers(opj_t1_t * t1, uint32_t w, uint32_t h);
/*@}*/
/*@}*/

static INLINE uint8 opj_t1_getctxno_zc(opj_mqc_t * mqc, uint32_t f)
{
	return mqc->lut_ctxno_zc_orient[(f & T1_SIGMA_NEIGHBOURS)];
}

static INLINE uint32_t opj_t1_getctxtno_sc_or_spb_index(uint32_t fX, uint32_t pfX, uint32_t nfX, uint32_t ci)
{
	/*
	   0 pfX T1_CHI_THIS           T1_LUT_SGN_W
	   1 tfX T1_SIGMA_1            T1_LUT_SIG_N
	   2 nfX T1_CHI_THIS           T1_LUT_SGN_E
	   3 tfX T1_SIGMA_3            T1_LUT_SIG_W
	   4  fX T1_CHI_(THIS - 1)     T1_LUT_SGN_N
	   5 tfX T1_SIGMA_5            T1_LUT_SIG_E
	   6  fX T1_CHI_(THIS + 1)     T1_LUT_SGN_S
	   7 tfX T1_SIGMA_7            T1_LUT_SIG_S
	 */

	uint32_t lu = (fX >> (ci * 3U)) & (T1_SIGMA_1 | T1_SIGMA_3 | T1_SIGMA_5 |
	    T1_SIGMA_7);

	lu |= (pfX >> (T1_CHI_THIS_I      + (ci * 3U))) & (1U << 0);
	lu |= (nfX >> (T1_CHI_THIS_I - 2U + (ci * 3U))) & (1U << 2);
	if(ci == 0U) {
		lu |= (fX >> (T1_CHI_0_I - 4U)) & (1U << 4);
	}
	else {
		lu |= (fX >> (T1_CHI_1_I - 4U + ((ci - 1U) * 3U))) & (1U << 4);
	}
	lu |= (fX >> (T1_CHI_2_I - 6U + (ci * 3U))) & (1U << 6);
	return lu;
}

static INLINE uint8 opj_t1_getctxno_sc(uint32_t lu)
{
	return lut_ctxno_sc[lu];
}

static INLINE uint32_t opj_t1_getctxno_mag(uint32_t f)
{
	uint32_t tmp = (f & T1_SIGMA_NEIGHBOURS) ? T1_CTXNO_MAG + 1 : T1_CTXNO_MAG;
	uint32_t tmp2 = (f & T1_MU_0) ? T1_CTXNO_MAG + 2 : tmp;
	return tmp2;
}

static INLINE uint8 opj_t1_getspb(uint32_t lu)
{
	return lut_spb[lu];
}

static int16 opj_t1_getnmsedec_sig(uint32_t x, uint32_t bitpos)
{
	if(bitpos > 0) {
		return lut_nmsedec_sig[(x >> (bitpos)) & ((1 << T1_NMSEDEC_BITS) - 1)];
	}
	return lut_nmsedec_sig0[x & ((1 << T1_NMSEDEC_BITS) - 1)];
}

static int16 opj_t1_getnmsedec_ref(uint32_t x, uint32_t bitpos)
{
	if(bitpos > 0) {
		return lut_nmsedec_ref[(x >> (bitpos)) & ((1 << T1_NMSEDEC_BITS) - 1)];
	}
	return lut_nmsedec_ref0[x & ((1 << T1_NMSEDEC_BITS) - 1)];
}

#define opj_t1_update_flags_macro(flags, flagsp, ci, s, stride, vsc) \
	{ \
		/* east */ \
		flagsp[-1] |= T1_SIGMA_5 << (3U * ci); \
		/* mark target as significant */ \
		flags |= ((s << T1_CHI_1_I) | T1_SIGMA_4) << (3U * ci); \
		/* west */ \
		flagsp[1] |= T1_SIGMA_3 << (3U * ci); \
		/* north-west, north, north-east */ \
		if(ci == 0U && !(vsc)) { \
			opj_flag_t* north = flagsp - (stride); \
			*north |= (s << T1_CHI_5_I) | T1_SIGMA_16; \
			north[-1] |= T1_SIGMA_17; \
			north[1] |= T1_SIGMA_15; \
		} \
 \
		/* south-west, south, south-east */ \
		if(ci == 3U) { \
			opj_flag_t* south = flagsp + (stride); \
			*south |= (s << T1_CHI_0_I) | T1_SIGMA_1; \
			south[-1] |= T1_SIGMA_2; \
			south[1] |= T1_SIGMA_0; \
		} \
	}

static INLINE void opj_t1_update_flags(opj_flag_t * flagsp, uint32_t ci, uint32_t s, uint32_t stride, uint32_t vsc)
{
	opj_t1_update_flags_macro(*flagsp, flagsp, ci, s, stride, vsc);
}

/**
   Encode significant pass
 */
#define opj_t1_enc_sigpass_step_macro(mqc, curctx, a, c, ct, flagspIn, datapIn, bpno, one, nmsedec, type, ciIn, vscIn) \
	{ \
		uint32_t v; \
		const uint32_t ci = (ciIn); \
		const uint32_t vsc = (vscIn); \
		const int32_t* l_datap = (datapIn); \
		opj_flag_t* flagsp = (flagspIn); \
		uint32_t const flags = *flagsp; \
		if((flags & ((T1_SIGMA_THIS | T1_PI_THIS) << (ci * 3U))) == 0U && \
		    (flags & (T1_SIGMA_NEIGHBOURS << (ci * 3U))) != 0U) { \
			uint32_t ctxt1 = opj_t1_getctxno_zc(mqc, flags >> (ci * 3U)); \
			v = (opj_smr_abs(*l_datap) & (uint32_t)one) ? 1 : 0; \
/* #ifdef DEBUG_ENC_SIG */ \
/*        fprintf(stderr, "   ctxt1=%d\n", ctxt1); */ \
/* #endif */ \
			opj_t1_setcurctx(curctx, ctxt1); \
			if(type == T1_TYPE_RAW) { /* BYPASS/LAZY MODE */ \
				opj_mqc_bypass_enc_macro(mqc, c, ct, v); \
			} else { \
				opj_mqc_encode_macro(mqc, curctx, a, c, ct, v); \
			} \
			if(v) { \
				uint32_t lu = opj_t1_getctxtno_sc_or_spb_index( \
					*flagsp, \
					flagsp[-1], flagsp[1], \
					ci); \
				uint32_t ctxt2 = opj_t1_getctxno_sc(lu); \
				v = opj_smr_sign(*l_datap); \
				*nmsedec += opj_t1_getnmsedec_sig(opj_smr_abs(*l_datap), \
					(uint32_t)bpno); \
/* #ifdef DEBUG_ENC_SIG */ \
/*            fprintf(stderr, "   ctxt2=%d\n", ctxt2); */ \
/* #endif */ \
				opj_t1_setcurctx(curctx, ctxt2); \
				if(type == T1_TYPE_RAW) { /* BYPASS/LAZY MODE */ \
					opj_mqc_bypass_enc_macro(mqc, c, ct, v); \
				} else { \
					uint32_t spb = opj_t1_getspb(lu); \
/* #ifdef DEBUG_ENC_SIG */ \
/*                fprintf(stderr, "   spb=%d\n", spb); */ \
/* #endif */ \
					opj_mqc_encode_macro(mqc, curctx, a, c, ct, v ^ spb); \
				} \
				opj_t1_update_flags(flagsp, ci, v, t1->w + 2, vsc); \
			} \
			*flagsp |= T1_PI_THIS << (ci * 3U); \
		} \
	}

static INLINE void opj_t1_dec_sigpass_step_raw(opj_t1_t * t1, opj_flag_t * flagsp, int32_t * datap, int32_t oneplushalf, uint32_t vsc, uint32_t ci)
{
	uint32_t v;
	opj_mqc_t * mqc = &(t1->mqc);  /* RAW component */
	uint32_t const flags = *flagsp;
	if((flags & ((T1_SIGMA_THIS | T1_PI_THIS) << (ci * 3U))) == 0U && (flags & (T1_SIGMA_NEIGHBOURS << (ci * 3U))) != 0U) {
		if(opj_mqc_raw_decode(mqc)) {
			v = opj_mqc_raw_decode(mqc);
			*datap = v ? -oneplushalf : oneplushalf;
			opj_t1_update_flags(flagsp, ci, v, t1->w + 2, vsc);
		}
		*flagsp |= T1_PI_THIS << (ci * 3U);
	}
}

#define opj_t1_dec_sigpass_step_mqc_macro(flags, flagsp, flags_stride, data, \
	    data_stride, ci, mqc, curctx, \
	    v, a, c, ct, oneplushalf, vsc) \
	{ \
		if((flags & ((T1_SIGMA_THIS | T1_PI_THIS) << (ci * 3U))) == 0U && \
		    (flags & (T1_SIGMA_NEIGHBOURS << (ci * 3U))) != 0U) { \
			uint32_t ctxt1 = opj_t1_getctxno_zc(mqc, flags >> (ci * 3U)); \
			opj_t1_setcurctx(curctx, ctxt1); \
			opj_mqc_decode_macro(v, mqc, curctx, a, c, ct); \
			if(v) { \
				uint32_t lu = opj_t1_getctxtno_sc_or_spb_index( \
					flags, \
					flagsp[-1], flagsp[1], \
					ci); \
				uint32_t ctxt2 = opj_t1_getctxno_sc(lu); \
				uint32_t spb = opj_t1_getspb(lu); \
				opj_t1_setcurctx(curctx, ctxt2); \
				opj_mqc_decode_macro(v, mqc, curctx, a, c, ct); \
				v = v ^ spb; \
				data[ci*data_stride] = v ? -oneplushalf : oneplushalf; \
				opj_t1_update_flags_macro(flags, flagsp, ci, v, flags_stride, vsc); \
			} \
			flags |= T1_PI_THIS << (ci * 3U); \
		} \
	}

static INLINE void opj_t1_dec_sigpass_step_mqc(opj_t1_t * t1, opj_flag_t * flagsp, int32_t * datap, int32_t oneplushalf,
    uint32_t ci, uint32_t flags_stride, uint32_t vsc)
{
	uint32_t v;
	opj_mqc_t * mqc = &(t1->mqc);  /* MQC component */
	opj_t1_dec_sigpass_step_mqc_macro(*flagsp, flagsp, flags_stride, datap, 0, ci, mqc, mqc->curctx, v, mqc->a, mqc->c, mqc->ct, oneplushalf, vsc);
}

static void opj_t1_enc_sigpass(opj_t1_t * t1, int32_t bpno, int32_t * nmsedec, uint8 type, uint32_t cblksty)
{
	uint32_t i, k;
	int32_t const one = 1 << (bpno + T1_NMSEDEC_FRACBITS);
	opj_flag_t* f = &T1_FLAGS(0, 0);
	uint32_t const extra = 2;
	opj_mqc_t* mqc = &(t1->mqc);
	DOWNLOAD_MQC_VARIABLES(mqc, curctx, a, c, ct);
	const int32_t* datap = t1->data;
	*nmsedec = 0;
#ifdef DEBUG_ENC_SIG
	fprintf(stderr, "enc_sigpass: bpno=%d\n", bpno);
#endif
	for(k = 0; k < (t1->h & ~3U); k += 4, f += extra) {
		const uint32_t w = t1->w;
#ifdef DEBUG_ENC_SIG
		fprintf(stderr, " k=%d\n", k);
#endif
		for(i = 0; i < w; ++i, ++f, datap += 4) {
#ifdef DEBUG_ENC_SIG
			fprintf(stderr, " i=%d\n", i);
#endif
			if(*f == 0U) {
				/* Nothing to do for any of the 4 data points */
				continue;
			}
			opj_t1_enc_sigpass_step_macro(mqc, curctx, a, c, ct, f, &datap[0], bpno, one, nmsedec, type, 0, cblksty & J2K_CCP_CBLKSTY_VSC);
			opj_t1_enc_sigpass_step_macro(mqc, curctx, a, c, ct, f, &datap[1], bpno, one, nmsedec, type, 1, 0);
			opj_t1_enc_sigpass_step_macro(mqc, curctx, a, c, ct, f, &datap[2], bpno, one, nmsedec, type, 2, 0);
			opj_t1_enc_sigpass_step_macro(mqc, curctx, a, c, ct, f, &datap[3], bpno, one, nmsedec, type, 3, 0);
		}
	}

	if(k < t1->h) {
		uint32_t j;
#ifdef DEBUG_ENC_SIG
		fprintf(stderr, " k=%d\n", k);
#endif
		for(i = 0; i < t1->w; ++i, ++f) {
#ifdef DEBUG_ENC_SIG
			fprintf(stderr, " i=%d\n", i);
#endif
			if(*f == 0U) {
				/* Nothing to do for any of the 4 data points */
				datap += (t1->h - k);
				continue;
			}
			for(j = k; j < t1->h; ++j, ++datap) {
				opj_t1_enc_sigpass_step_macro(mqc, curctx, a, c, ct, f, &datap[0], bpno, one, nmsedec, type, j - k, (j == k && (cblksty & J2K_CCP_CBLKSTY_VSC) != 0));
			}
		}
	}
	UPLOAD_MQC_VARIABLES(mqc, curctx, a, c, ct);
}

static void opj_t1_dec_sigpass_raw(opj_t1_t * t1, int32_t bpno, int32_t cblksty)
{
	int32_t one, half, oneplushalf;
	uint32_t i, j, k;
	int32_t * data = t1->data;
	opj_flag_t * flagsp = &T1_FLAGS(0, 0);
	const uint32_t l_w = t1->w;
	one = 1 << bpno;
	half = one >> 1;
	oneplushalf = one | half;
	for(k = 0; k < (t1->h & ~3U); k += 4, flagsp += 2, data += 3 * l_w) {
		for(i = 0; i < l_w; ++i, ++flagsp, ++data) {
			opj_flag_t flags = *flagsp;
			if(flags != 0) {
				opj_t1_dec_sigpass_step_raw(t1, flagsp, data, oneplushalf, cblksty & J2K_CCP_CBLKSTY_VSC, /* vsc */ 0U);
				opj_t1_dec_sigpass_step_raw(t1, flagsp, data + l_w, oneplushalf, FALSE, /* vsc */ 1U);
				opj_t1_dec_sigpass_step_raw(t1, flagsp, data + 2 * l_w, oneplushalf, FALSE, /* vsc */ 2U);
				opj_t1_dec_sigpass_step_raw(t1, flagsp, data + 3 * l_w, oneplushalf, FALSE, /* vsc */ 3U);
			}
		}
	}
	if(k < t1->h) {
		for(i = 0; i < l_w; ++i, ++flagsp, ++data) {
			for(j = 0; j < t1->h - k; ++j) {
				opj_t1_dec_sigpass_step_raw(t1, flagsp, data + j * l_w, oneplushalf, cblksty & J2K_CCP_CBLKSTY_VSC, /* vsc */j);
			}
		}
	}
}

#define opj_t1_dec_sigpass_mqc_internal(t1, bpno, vsc, w, h, flags_stride) \
	{ \
		int32_t one, half, oneplushalf; \
		uint32_t i, j, k; \
		int32_t * data = t1->data; \
		opj_flag_t * flagsp = &t1->flags[(flags_stride) + 1]; \
		const uint32_t l_w = w; \
		opj_mqc_t* mqc = &(t1->mqc); \
		DOWNLOAD_MQC_VARIABLES(mqc, curctx, a, c, ct); \
		uint32_t v; \
		one = 1 << bpno; \
		half = one >> 1; \
		oneplushalf = one | half; \
		for(k = 0; k < (h & ~3u); k += 4, data += 3*l_w, flagsp += 2) { \
			for(i = 0; i < l_w; ++i, ++data, ++flagsp) { \
				opj_flag_t flags = *flagsp; \
				if(flags != 0) { \
					opj_t1_dec_sigpass_step_mqc_macro(flags, flagsp, flags_stride, data, l_w, 0, mqc, curctx, v, a, c, ct, oneplushalf, vsc); \
					opj_t1_dec_sigpass_step_mqc_macro(flags, flagsp, flags_stride, data, l_w, 1, mqc, curctx, v, a, c, ct, oneplushalf, FALSE); \
					opj_t1_dec_sigpass_step_mqc_macro(flags, flagsp, flags_stride, data, l_w, 2, mqc, curctx, v, a, c, ct, oneplushalf, FALSE); \
					opj_t1_dec_sigpass_step_mqc_macro(flags, flagsp, flags_stride, data, l_w, 3, mqc, curctx, v, a, c, ct, oneplushalf, FALSE); \
					*flagsp = flags; \
				} \
			} \
		} \
		UPLOAD_MQC_VARIABLES(mqc, curctx, a, c, ct); \
		if(k < h) { \
			for(i = 0; i < l_w; ++i, ++data, ++flagsp) { \
				for(j = 0; j < h - k; ++j) { \
					opj_t1_dec_sigpass_step_mqc(t1, flagsp, data + j * l_w, oneplushalf, j, flags_stride, vsc); \
				} \
			} \
		} \
	}

static void opj_t1_dec_sigpass_mqc_64x64_novsc(opj_t1_t * t1, int32_t bpno)
{
	opj_t1_dec_sigpass_mqc_internal(t1, bpno, FALSE, 64, 64, 66);
}

static void opj_t1_dec_sigpass_mqc_64x64_vsc(opj_t1_t * t1, int32_t bpno)
{
	opj_t1_dec_sigpass_mqc_internal(t1, bpno, TRUE, 64, 64, 66);
}

static void opj_t1_dec_sigpass_mqc_generic_novsc(opj_t1_t * t1, int32_t bpno)
{
	opj_t1_dec_sigpass_mqc_internal(t1, bpno, FALSE, t1->w, t1->h, t1->w + 2U);
}

static void opj_t1_dec_sigpass_mqc_generic_vsc(opj_t1_t * t1, int32_t bpno)
{
	opj_t1_dec_sigpass_mqc_internal(t1, bpno, TRUE, t1->w, t1->h, t1->w + 2U);
}

static void opj_t1_dec_sigpass_mqc(opj_t1_t * t1, int32_t bpno, int32_t cblksty)
{
	if(t1->w == 64 && t1->h == 64) {
		if(cblksty & J2K_CCP_CBLKSTY_VSC) {
			opj_t1_dec_sigpass_mqc_64x64_vsc(t1, bpno);
		}
		else {
			opj_t1_dec_sigpass_mqc_64x64_novsc(t1, bpno);
		}
	}
	else {
		if(cblksty & J2K_CCP_CBLKSTY_VSC) {
			opj_t1_dec_sigpass_mqc_generic_vsc(t1, bpno);
		}
		else {
			opj_t1_dec_sigpass_mqc_generic_novsc(t1, bpno);
		}
	}
}

/**
   Encode refinement pass step
 */
#define opj_t1_enc_refpass_step_macro(mqc, curctx, a, c, ct, flags, flagsUpdated, datap, bpno, one, nmsedec, type, ci) \
	{ \
		uint32_t v; \
		if((flags & ((T1_SIGMA_THIS | T1_PI_THIS) << ((ci) * 3U))) == (T1_SIGMA_THIS << ((ci) * 3U))) { \
			const uint32_t shift_flags = (flags >> ((ci) * 3U)); \
			uint32_t ctxt = opj_t1_getctxno_mag(shift_flags); \
			uint32_t abs_data = opj_smr_abs(*datap); \
			*nmsedec += opj_t1_getnmsedec_ref(abs_data, (uint32_t)bpno); \
			v = ((int32_t)abs_data & one) ? 1 : 0; \
/* #ifdef DEBUG_ENC_REF */ \
/*        fprintf(stderr, "  ctxt=%d\n", ctxt); */ \
/* #endif */ \
			opj_t1_setcurctx(curctx, ctxt); \
			if(type == T1_TYPE_RAW) { /* BYPASS/LAZY MODE */ \
				opj_mqc_bypass_enc_macro(mqc, c, ct, v); \
			} else { \
				opj_mqc_encode_macro(mqc, curctx, a, c, ct, v); \
			} \
			flagsUpdated |= T1_MU_THIS << ((ci) * 3U); \
		} \
	}

static INLINE void opj_t1_dec_refpass_step_raw(opj_t1_t * t1, opj_flag_t * flagsp, int32_t * datap, int32_t poshalf, uint32_t ci)
{
	uint32_t v;
	opj_mqc_t * mqc = &(t1->mqc);  /* RAW component */
	if((*flagsp & ((T1_SIGMA_THIS | T1_PI_THIS) << (ci * 3U))) == (T1_SIGMA_THIS << (ci * 3U))) {
		v = opj_mqc_raw_decode(mqc);
		*datap += (v ^ (*datap < 0)) ? poshalf : -poshalf;
		*flagsp |= T1_MU_THIS << (ci * 3U);
	}
}

#define opj_t1_dec_refpass_step_mqc_macro(flags, data, data_stride, ci, mqc, curctx, v, a, c, ct, poshalf) \
	{ \
		if((flags & ((T1_SIGMA_THIS | T1_PI_THIS) << (ci * 3U))) == (T1_SIGMA_THIS << (ci * 3U))) { \
			uint32_t ctxt = opj_t1_getctxno_mag(flags >> (ci * 3U)); \
			opj_t1_setcurctx(curctx, ctxt); \
			opj_mqc_decode_macro(v, mqc, curctx, a, c, ct); \
			data[ci*data_stride] += (v ^ (data[ci*data_stride] < 0)) ? poshalf : -poshalf; \
			flags |= T1_MU_THIS << (ci * 3U); \
		} \
	}

static INLINE void opj_t1_dec_refpass_step_mqc(opj_t1_t * t1, opj_flag_t * flagsp, int32_t * datap, int32_t poshalf, uint32_t ci)
{
	uint32_t v;
	opj_mqc_t * mqc = &(t1->mqc);  /* MQC component */
	opj_t1_dec_refpass_step_mqc_macro(*flagsp, datap, 0, ci, mqc, mqc->curctx, v, mqc->a, mqc->c, mqc->ct, poshalf);
}

static void opj_t1_enc_refpass(opj_t1_t * t1, int32_t bpno, int32_t * nmsedec, uint8 type)
{
	uint32_t i, k;
	const int32_t one = 1 << (bpno + T1_NMSEDEC_FRACBITS);
	opj_flag_t* f = &T1_FLAGS(0, 0);
	const uint32_t extra = 2U;
	opj_mqc_t* mqc = &(t1->mqc);
	DOWNLOAD_MQC_VARIABLES(mqc, curctx, a, c, ct);
	const int32_t* datap = t1->data;
	*nmsedec = 0;
#ifdef DEBUG_ENC_REF
	fprintf(stderr, "enc_refpass: bpno=%d\n", bpno);
#endif
	for(k = 0; k < (t1->h & ~3U); k += 4, f += extra) {
#ifdef DEBUG_ENC_REF
		fprintf(stderr, " k=%d\n", k);
#endif
		for(i = 0; i < t1->w; ++i, f++, datap += 4) {
			const uint32_t flags = *f;
			uint32_t flagsUpdated = flags;
#ifdef DEBUG_ENC_REF
			fprintf(stderr, " i=%d\n", i);
#endif
			if((flags & (T1_SIGMA_4 | T1_SIGMA_7 | T1_SIGMA_10 | T1_SIGMA_13)) == 0) {
				/* none significant */
				continue;
			}
			if((flags & (T1_PI_0 | T1_PI_1 | T1_PI_2 | T1_PI_3)) ==
			    (T1_PI_0 | T1_PI_1 | T1_PI_2 | T1_PI_3)) {
				/* all processed by sigpass */
				continue;
			}

			opj_t1_enc_refpass_step_macro(
				mqc, curctx, a, c, ct,
				flags, flagsUpdated,
				&datap[0],
				bpno,
				one,
				nmsedec,
				type,
				0);
			opj_t1_enc_refpass_step_macro(
				mqc, curctx, a, c, ct,
				flags, flagsUpdated,
				&datap[1],
				bpno,
				one,
				nmsedec,
				type,
				1);
			opj_t1_enc_refpass_step_macro(
				mqc, curctx, a, c, ct,
				flags, flagsUpdated,
				&datap[2],
				bpno,
				one,
				nmsedec,
				type,
				2);
			opj_t1_enc_refpass_step_macro(
				mqc, curctx, a, c, ct,
				flags, flagsUpdated,
				&datap[3],
				bpno,
				one,
				nmsedec,
				type,
				3);
			*f = flagsUpdated;
		}
	}

	if(k < t1->h) {
		uint32_t j;
		const uint32_t remaining_lines = t1->h - k;
#ifdef DEBUG_ENC_REF
		fprintf(stderr, " k=%d\n", k);
#endif
		for(i = 0; i < t1->w; ++i, ++f) {
#ifdef DEBUG_ENC_REF
			fprintf(stderr, " i=%d\n", i);
#endif
			if((*f & (T1_SIGMA_4 | T1_SIGMA_7 | T1_SIGMA_10 | T1_SIGMA_13)) == 0) {
				/* none significant */
				datap += remaining_lines;
				continue;
			}
			for(j = 0; j < remaining_lines; ++j, datap++) {
				opj_t1_enc_refpass_step_macro(
					mqc, curctx, a, c, ct,
					*f, *f,
					&datap[0],
					bpno,
					one,
					nmsedec,
					type,
					j);
			}
		}
	}
	UPLOAD_MQC_VARIABLES(mqc, curctx, a, c, ct);
}

static void opj_t1_dec_refpass_raw(opj_t1_t * t1, int32_t bpno)
{
	int32_t one, poshalf;
	uint32_t i, j, k;
	int32_t * data = t1->data;
	opj_flag_t * flagsp = &T1_FLAGS(0, 0);
	const uint32_t l_w = t1->w;
	one = 1 << bpno;
	poshalf = one >> 1;
	for(k = 0; k < (t1->h & ~3U); k += 4, flagsp += 2, data += 3 * l_w) {
		for(i = 0; i < l_w; ++i, ++flagsp, ++data) {
			opj_flag_t flags = *flagsp;
			if(flags != 0) {
				opj_t1_dec_refpass_step_raw(t1, flagsp, data, poshalf, 0U);
				opj_t1_dec_refpass_step_raw(t1, flagsp, data + l_w, poshalf, 1U);
				opj_t1_dec_refpass_step_raw(t1, flagsp, data + 2 * l_w, poshalf, 2U);
				opj_t1_dec_refpass_step_raw(t1, flagsp, data + 3 * l_w, poshalf, 3U);
			}
		}
	}
	if(k < t1->h) {
		for(i = 0; i < l_w; ++i, ++flagsp, ++data) {
			for(j = 0; j < t1->h - k; ++j) {
				opj_t1_dec_refpass_step_raw(t1, flagsp, data + j * l_w, poshalf, j);
			}
		}
	}
}

#define opj_t1_dec_refpass_mqc_internal(t1, bpno, w, h, flags_stride) \
	{ \
		int32_t one, poshalf; \
		uint32_t i, j, k; \
		int32_t * data = t1->data; \
		opj_flag_t * flagsp = &t1->flags[flags_stride + 1]; \
		const uint32_t l_w = w; \
		opj_mqc_t* mqc = &(t1->mqc); \
		DOWNLOAD_MQC_VARIABLES(mqc, curctx, a, c, ct); \
		uint32_t v; \
		one = 1 << bpno; \
		poshalf = one >> 1; \
		for(k = 0; k < (h & ~3u); k += 4, data += 3*l_w, flagsp += 2) { \
			for(i = 0; i < l_w; ++i, ++data, ++flagsp) { \
				opj_flag_t flags = *flagsp; \
				if(flags != 0) { \
					opj_t1_dec_refpass_step_mqc_macro( \
						flags, data, l_w, 0, \
						mqc, curctx, v, a, c, ct, poshalf); \
					opj_t1_dec_refpass_step_mqc_macro( \
						flags, data, l_w, 1, \
						mqc, curctx, v, a, c, ct, poshalf); \
					opj_t1_dec_refpass_step_mqc_macro( \
						flags, data, l_w, 2, \
						mqc, curctx, v, a, c, ct, poshalf); \
					opj_t1_dec_refpass_step_mqc_macro( \
						flags, data, l_w, 3, \
						mqc, curctx, v, a, c, ct, poshalf); \
					*flagsp = flags; \
				} \
			} \
		} \
		UPLOAD_MQC_VARIABLES(mqc, curctx, a, c, ct); \
		if(k < h) { \
			for(i = 0; i < l_w; ++i, ++data, ++flagsp) { \
				for(j = 0; j < h - k; ++j) { \
					opj_t1_dec_refpass_step_mqc(t1, flagsp, data + j * l_w, poshalf, j); \
				} \
			} \
		} \
	}

static void opj_t1_dec_refpass_mqc_64x64(opj_t1_t * t1,
    int32_t bpno)
{
	opj_t1_dec_refpass_mqc_internal(t1, bpno, 64, 64, 66);
}

static void opj_t1_dec_refpass_mqc_generic(opj_t1_t * t1,
    int32_t bpno)
{
	opj_t1_dec_refpass_mqc_internal(t1, bpno, t1->w, t1->h, t1->w + 2U);
}

static void opj_t1_dec_refpass_mqc(opj_t1_t * t1,
    int32_t bpno)
{
	if(t1->w == 64 && t1->h == 64) {
		opj_t1_dec_refpass_mqc_64x64(t1, bpno);
	}
	else {
		opj_t1_dec_refpass_mqc_generic(t1, bpno);
	}
}

/**
   Encode clean-up pass step
 */
#define opj_t1_enc_clnpass_step_macro(mqc, curctx, a, c, ct, flagspIn, datapIn, bpno, one, nmsedec, agg, runlen, lim, cblksty) \
	{ \
		uint32_t v; \
		uint32_t ci; \
		opj_flag_t* const flagsp = (flagspIn); \
		const int32_t* l_datap = (datapIn); \
		const uint32_t check = (T1_SIGMA_4 | T1_SIGMA_7 | T1_SIGMA_10 | T1_SIGMA_13 | \
		    T1_PI_0 | T1_PI_1 | T1_PI_2 | T1_PI_3); \
 \
		if((*flagsp & check) == check) { \
			if(runlen == 0) { \
				*flagsp &= ~(T1_PI_0 | T1_PI_1 | T1_PI_2 | T1_PI_3); \
			} else if(runlen == 1) { \
				*flagsp &= ~(T1_PI_1 | T1_PI_2 | T1_PI_3); \
			} else if(runlen == 2) { \
				*flagsp &= ~(T1_PI_2 | T1_PI_3); \
			} else if(runlen == 3) { \
				*flagsp &= ~(T1_PI_3); \
			} \
		} \
		else \
			for(ci = runlen; ci < lim; ++ci) { \
				boolint goto_PARTIAL = FALSE; \
				if((agg != 0) && (ci == runlen)) { \
					goto_PARTIAL = TRUE; \
				} \
				else if(!(*flagsp & ((T1_SIGMA_THIS | T1_PI_THIS) << (ci * 3U)))) { \
					uint32_t ctxt1 = opj_t1_getctxno_zc(mqc, *flagsp >> (ci * 3U)); \
/* #ifdef DEBUG_ENC_CLN */ \
/*            printf("   ctxt1=%d\n", ctxt1); */ \
/* #endif */ \
					opj_t1_setcurctx(curctx, ctxt1); \
					v = (opj_smr_abs(*l_datap) & (uint32_t)one) ? 1 : 0; \
					opj_mqc_encode_macro(mqc, curctx, a, c, ct, v); \
					if(v) { \
						goto_PARTIAL = TRUE; \
					} \
				} \
				if(goto_PARTIAL) { \
					uint32_t vsc; \
					uint32_t ctxt2, spb; \
					uint32_t lu = opj_t1_getctxtno_sc_or_spb_index( \
						*flagsp, \
						flagsp[-1], flagsp[1], \
						ci); \
					*nmsedec += opj_t1_getnmsedec_sig(opj_smr_abs(*l_datap), \
						(uint32_t)bpno); \
					ctxt2 = opj_t1_getctxno_sc(lu); \
/* #ifdef DEBUG_ENC_CLN */ \
/*           printf("   ctxt2=%d\n", ctxt2); */ \
/* #endif */ \
					opj_t1_setcurctx(curctx, ctxt2); \
 \
					v = opj_smr_sign(*l_datap); \
					spb = opj_t1_getspb(lu); \
/* #ifdef DEBUG_ENC_CLN */ \
/*           printf("   spb=%d\n", spb); */ \
/* #endif */ \
					opj_mqc_encode_macro(mqc, curctx, a, c, ct, v ^ spb); \
					vsc = ((cblksty & J2K_CCP_CBLKSTY_VSC) && (ci == 0)) ? 1 : 0; \
					opj_t1_update_flags(flagsp, ci, v, t1->w + 2U, vsc); \
				} \
				*flagsp &= ~(T1_PI_THIS << (3U * ci)); \
				l_datap++; \
			} \
	}

#define opj_t1_dec_clnpass_step_macro(check_flags, partial, \
	    flags, flagsp, flags_stride, data, \
	    data_stride, ci, mqc, curctx, \
	    v, a, c, ct, oneplushalf, vsc) \
	{ \
		if(!check_flags || !(flags & ((T1_SIGMA_THIS | T1_PI_THIS) << (ci * 3U)))) { \
			do { \
				if(!partial) { \
					uint32_t ctxt1 = opj_t1_getctxno_zc(mqc, flags >> (ci * 3U)); \
					opj_t1_setcurctx(curctx, ctxt1); \
					opj_mqc_decode_macro(v, mqc, curctx, a, c, ct); \
					if(!v) \
						break; \
				} \
				{ \
					uint32_t lu = opj_t1_getctxtno_sc_or_spb_index( \
						flags, flagsp[-1], flagsp[1], \
						ci); \
					opj_t1_setcurctx(curctx, opj_t1_getctxno_sc(lu)); \
					opj_mqc_decode_macro(v, mqc, curctx, a, c, ct); \
					v = v ^ opj_t1_getspb(lu); \
					data[ci*data_stride] = v ? -oneplushalf : oneplushalf; \
					opj_t1_update_flags_macro(flags, flagsp, ci, v, flags_stride, vsc); \
				} \
			} while(0); \
		} \
	}

static void opj_t1_dec_clnpass_step(opj_t1_t * t1,
    opj_flag_t * flagsp,
    int32_t * datap,
    int32_t oneplushalf,
    uint32_t ci,
    uint32_t vsc)
{
	uint32_t v;

	opj_mqc_t * mqc = &(t1->mqc); /* MQC component */
	opj_t1_dec_clnpass_step_macro(TRUE, FALSE,
	    *flagsp, flagsp, t1->w + 2U, datap,
	    0, ci, mqc, mqc->curctx,
	    v, mqc->a, mqc->c, mqc->ct, oneplushalf, vsc);
}

static void opj_t1_enc_clnpass(opj_t1_t * t1,
    int32_t bpno,
    int32_t * nmsedec,
    uint32_t cblksty)
{
	uint32_t i, k;
	const int32_t one = 1 << (bpno + T1_NMSEDEC_FRACBITS);
	opj_mqc_t* mqc = &(t1->mqc);
	DOWNLOAD_MQC_VARIABLES(mqc, curctx, a, c, ct);
	const int32_t* datap = t1->data;
	opj_flag_t * f = &T1_FLAGS(0, 0);
	const uint32_t extra = 2U;

	*nmsedec = 0;
#ifdef DEBUG_ENC_CLN
	printf("enc_clnpass: bpno=%d\n", bpno);
#endif
	for(k = 0; k < (t1->h & ~3U); k += 4, f += extra) {
#ifdef DEBUG_ENC_CLN
		printf(" k=%d\n", k);
#endif
		for(i = 0; i < t1->w; ++i, f++) {
			uint32_t agg, runlen;
#ifdef DEBUG_ENC_CLN
			printf("  i=%d\n", i);
#endif
			agg = !*f;
#ifdef DEBUG_ENC_CLN
			printf("   agg=%d\n", agg);
#endif
			if(agg) {
				for(runlen = 0; runlen < 4; ++runlen, ++datap) {
					if(opj_smr_abs(*datap) & (uint32_t)one) {
						break;
					}
				}
				opj_t1_setcurctx(curctx, T1_CTXNO_AGG);
				opj_mqc_encode_macro(mqc, curctx, a, c, ct, runlen != 4);
				if(runlen == 4) {
					continue;
				}
				opj_t1_setcurctx(curctx, T1_CTXNO_UNI);
				opj_mqc_encode_macro(mqc, curctx, a, c, ct, runlen >> 1);
				opj_mqc_encode_macro(mqc, curctx, a, c, ct, runlen & 1);
			}
			else {
				runlen = 0;
			}
			opj_t1_enc_clnpass_step_macro(
				mqc, curctx, a, c, ct,
				f,
				datap,
				bpno,
				one,
				nmsedec,
				agg,
				runlen,
				4U,
				cblksty);
			datap += 4 - runlen;
		}
	}
	if(k < t1->h) {
		const uint32_t agg = 0;
		const uint32_t runlen = 0;
#ifdef DEBUG_ENC_CLN
		printf(" k=%d\n", k);
#endif
		for(i = 0; i < t1->w; ++i, f++) {
#ifdef DEBUG_ENC_CLN
			printf("  i=%d\n", i);
			printf("   agg=%d\n", agg);
#endif
			opj_t1_enc_clnpass_step_macro(
				mqc, curctx, a, c, ct,
				f,
				datap,
				bpno,
				one,
				nmsedec,
				agg,
				runlen,
				t1->h - k,
				cblksty);
			datap += t1->h - k;
		}
	}

	UPLOAD_MQC_VARIABLES(mqc, curctx, a, c, ct);
}

#define opj_t1_dec_clnpass_internal(t1, bpno, vsc, w, h, flags_stride) \
	{ \
		int32_t one, half, oneplushalf; \
		uint32_t runlen; \
		uint32_t i, j, k; \
		const uint32_t l_w = w; \
		opj_mqc_t* mqc = &(t1->mqc); \
		int32_t * data = t1->data; \
		opj_flag_t * flagsp = &t1->flags[flags_stride + 1]; \
		DOWNLOAD_MQC_VARIABLES(mqc, curctx, a, c, ct); \
		uint32_t v; \
		one = 1 << bpno; \
		half = one >> 1; \
		oneplushalf = one | half; \
		for(k = 0; k < (h & ~3u); k += 4, data += 3*l_w, flagsp += 2) { \
			for(i = 0; i < l_w; ++i, ++data, ++flagsp) { \
				opj_flag_t flags = *flagsp; \
				if(flags == 0) { \
					uint32_t partial = TRUE; \
					opj_t1_setcurctx(curctx, T1_CTXNO_AGG); \
					opj_mqc_decode_macro(v, mqc, curctx, a, c, ct); \
					if(!v) { \
						continue; \
					} \
					opj_t1_setcurctx(curctx, T1_CTXNO_UNI); \
					opj_mqc_decode_macro(runlen, mqc, curctx, a, c, ct); \
					opj_mqc_decode_macro(v, mqc, curctx, a, c, ct); \
					runlen = (runlen << 1) | v; \
					switch(runlen) { \
						case 0: \
						    opj_t1_dec_clnpass_step_macro(FALSE, TRUE, \
							flags, flagsp, flags_stride, data, \
							l_w, 0, mqc, curctx, \
							v, a, c, ct, oneplushalf, vsc); \
						    partial = FALSE; \
						/* FALLTHRU */ \
						case 1: \
						    opj_t1_dec_clnpass_step_macro(FALSE, partial, \
							flags, flagsp, flags_stride, data, \
							l_w, 1, mqc, curctx, \
							v, a, c, ct, oneplushalf, FALSE); \
						    partial = FALSE; \
						/* FALLTHRU */ \
						case 2: \
						    opj_t1_dec_clnpass_step_macro(FALSE, partial, \
							flags, flagsp, flags_stride, data, \
							l_w, 2, mqc, curctx, \
							v, a, c, ct, oneplushalf, FALSE); \
						    partial = FALSE; \
						/* FALLTHRU */ \
						case 3: \
						    opj_t1_dec_clnpass_step_macro(FALSE, partial, \
							flags, flagsp, flags_stride, data, \
							l_w, 3, mqc, curctx, \
							v, a, c, ct, oneplushalf, FALSE); \
						    break; \
					} \
				} else { \
					opj_t1_dec_clnpass_step_macro(TRUE, FALSE, \
					    flags, flagsp, flags_stride, data, \
					    l_w, 0, mqc, curctx, \
					    v, a, c, ct, oneplushalf, vsc); \
					opj_t1_dec_clnpass_step_macro(TRUE, FALSE, \
					    flags, flagsp, flags_stride, data, \
					    l_w, 1, mqc, curctx, \
					    v, a, c, ct, oneplushalf, FALSE); \
					opj_t1_dec_clnpass_step_macro(TRUE, FALSE, \
					    flags, flagsp, flags_stride, data, \
					    l_w, 2, mqc, curctx, \
					    v, a, c, ct, oneplushalf, FALSE); \
					opj_t1_dec_clnpass_step_macro(TRUE, FALSE, \
					    flags, flagsp, flags_stride, data, \
					    l_w, 3, mqc, curctx, \
					    v, a, c, ct, oneplushalf, FALSE); \
				} \
				*flagsp = flags & ~(T1_PI_0 | T1_PI_1 | T1_PI_2 | T1_PI_3); \
			} \
		} \
		UPLOAD_MQC_VARIABLES(mqc, curctx, a, c, ct); \
		if(k < h) { \
			for(i = 0; i < l_w; ++i, ++flagsp, ++data) { \
				for(j = 0; j < h - k; ++j) { \
					opj_t1_dec_clnpass_step(t1, flagsp, data + j * l_w, oneplushalf, j, vsc); \
				} \
				*flagsp &= ~(T1_PI_0 | T1_PI_1 | T1_PI_2 | T1_PI_3); \
			} \
		} \
	}

static void opj_t1_dec_clnpass_check_segsym(opj_t1_t * t1, int32_t cblksty)
{
	if(cblksty & J2K_CCP_CBLKSTY_SEGSYM) {
		opj_mqc_t* mqc = &(t1->mqc);
		uint32_t v, v2;
		opj_mqc_setcurctx(mqc, T1_CTXNO_UNI);
		opj_mqc_decode(v, mqc);
		opj_mqc_decode(v2, mqc);
		v = (v << 1) | v2;
		opj_mqc_decode(v2, mqc);
		v = (v << 1) | v2;
		opj_mqc_decode(v2, mqc);
		v = (v << 1) | v2;
		/*
		   if (v!=0xa) {
		    opj_event_msg(t1->cinfo, EVT_WARNING, "Bad segmentation symbol %x\n", v);
		   }
		 */
	}
}

static void opj_t1_dec_clnpass_64x64_novsc(opj_t1_t * t1,
    int32_t bpno)
{
	opj_t1_dec_clnpass_internal(t1, bpno, FALSE, 64, 64, 66);
}

static void opj_t1_dec_clnpass_64x64_vsc(opj_t1_t * t1,
    int32_t bpno)
{
	opj_t1_dec_clnpass_internal(t1, bpno, TRUE, 64, 64, 66);
}

static void opj_t1_dec_clnpass_generic_novsc(opj_t1_t * t1,
    int32_t bpno)
{
	opj_t1_dec_clnpass_internal(t1, bpno, FALSE, t1->w, t1->h,
	    t1->w + 2U);
}

static void opj_t1_dec_clnpass_generic_vsc(opj_t1_t * t1,
    int32_t bpno)
{
	opj_t1_dec_clnpass_internal(t1, bpno, TRUE, t1->w, t1->h,
	    t1->w + 2U);
}

static void opj_t1_dec_clnpass(opj_t1_t * t1,
    int32_t bpno,
    int32_t cblksty)
{
	if(t1->w == 64 && t1->h == 64) {
		if(cblksty & J2K_CCP_CBLKSTY_VSC) {
			opj_t1_dec_clnpass_64x64_vsc(t1, bpno);
		}
		else {
			opj_t1_dec_clnpass_64x64_novsc(t1, bpno);
		}
	}
	else {
		if(cblksty & J2K_CCP_CBLKSTY_VSC) {
			opj_t1_dec_clnpass_generic_vsc(t1, bpno);
		}
		else {
			opj_t1_dec_clnpass_generic_novsc(t1, bpno);
		}
	}
	opj_t1_dec_clnpass_check_segsym(t1, cblksty);
}

/** mod fixed_quality */
static double opj_t1_getwmsedec(int32_t nmsedec,
    uint32_t compno,
    uint32_t level,
    uint32_t orient,
    int32_t bpno,
    uint32_t qmfbid,
    double stepsize,
    uint32_t numcomps,
    const double * mct_norms,
    uint32_t mct_numcomps)
{
	double w1 = 1, w2, wmsedec;
	OPJ_ARG_NOT_USED(numcomps);

	if(mct_norms && (compno < mct_numcomps)) {
		w1 = mct_norms[compno];
	}

	if(qmfbid == 1) {
		w2 = opj_dwt_getnorm(level, orient);
	}
	else {  /* if (qmfbid == 0) */
		const int32_t log2_gain = (orient == 0) ? 0 :
		    (orient == 3) ? 2 : 1;
		w2 = opj_dwt_getnorm_real(level, orient);
		/* Not sure this is right. But preserves past behaviour */
		stepsize /= (1 << log2_gain);
	}

	wmsedec = w1 * w2 * stepsize * (1 << bpno);
	wmsedec *= wmsedec * nmsedec / 8192.0;

	return wmsedec;
}

static boolint opj_t1_allocate_buffers(opj_t1_t * t1,
    uint32_t w,
    uint32_t h)
{
	uint32_t flagssize;
	uint32_t flags_stride;

	/* No risk of overflow. Prior checks ensure those assert are met */
	/* They are per the specification */
	assert(w <= 1024);
	assert(h <= 1024);
	assert(w * h <= 4096);

	/* encoder uses tile buffer, so no need to allocate */
	{
		uint32_t datasize = w * h;
		if(datasize > t1->datasize) {
			opj_aligned_free(t1->data);
			t1->data = (int32_t*)opj_aligned_malloc(datasize * sizeof(int32_t));
			if(!t1->data) {
				/* FIXME event manager error callback */
				return FALSE;
			}
			t1->datasize = datasize;
		}
		/* memset first arg is declared to never be null by gcc */
		if(t1->data != NULL) {
			memzero(t1->data, datasize * sizeof(int32_t));
		}
	}
	flags_stride = w + 2U; /* can't be 0U */
	flagssize = (h + 3U) / 4U + 2U;
	flagssize *= flags_stride;
	{
		opj_flag_t* p;
		uint32_t x;
		uint32_t flags_height = (h + 3U) / 4U;
		if(flagssize > t1->flagssize) {
			opj_aligned_free(t1->flags);
			t1->flags = (opj_flag_t*)opj_aligned_malloc(flagssize * sizeof(opj_flag_t));
			if(!t1->flags) {
				/* FIXME event manager error callback */
				return FALSE;
			}
		}
		t1->flagssize = flagssize;
		memzero(t1->flags, flagssize * sizeof(opj_flag_t));
		p = &t1->flags[0];
		for(x = 0; x < flags_stride; ++x) {
			/* magic value to hopefully stop any passes being interested in this entry */
			*p++ = (T1_PI_0 | T1_PI_1 | T1_PI_2 | T1_PI_3);
		}
		p = &t1->flags[((flags_height + 1) * flags_stride)];
		for(x = 0; x < flags_stride; ++x) {
			/* magic value to hopefully stop any passes being interested in this entry */
			*p++ = (T1_PI_0 | T1_PI_1 | T1_PI_2 | T1_PI_3);
		}
		if(h % 4) {
			uint32_t v = 0;
			p = &t1->flags[((flags_height) * flags_stride)];
			if(h % 4 == 1) {
				v |= T1_PI_1 | T1_PI_2 | T1_PI_3;
			}
			else if(h % 4 == 2) {
				v |= T1_PI_2 | T1_PI_3;
			}
			else if(h % 4 == 3) {
				v |= T1_PI_3;
			}
			for(x = 0; x < flags_stride; ++x) {
				*p++ = v;
			}
		}
	}
	t1->w = w;
	t1->h = h;
	return TRUE;
}
/**
 * Creates a new Tier 1 handle
 * and initializes the look-up tables of the Tier-1 coder/decoder
 * @return a new T1 handle if successful, returns NULL otherwise
 */
opj_t1_t* opj_t1_create(boolint isEncoder)
{
	opj_t1_t * l_t1 = 0;
	l_t1 = (opj_t1_t*)opj_calloc(1, sizeof(opj_t1_t));
	if(!l_t1) {
		return 0;
	}
	l_t1->encoder = isEncoder;
	return l_t1;
}
/**
 * Destroys a previously created T1 handle
 *
 * @param p_t1 Tier 1 handle to destroy
 */
void opj_t1_destroy(opj_t1_t * p_t1)
{
	if(!p_t1) {
		return;
	}

	if(p_t1->data) {
		opj_aligned_free(p_t1->data);
		p_t1->data = 0;
	}

	if(p_t1->flags) {
		opj_aligned_free(p_t1->flags);
		p_t1->flags = 0;
	}

	SAlloc::F(p_t1->cblkdatabuffer);

	SAlloc::F(p_t1);
}

typedef struct {
	boolint whole_tile_decoding;
	uint32_t resno;
	opj_tcd_cblk_dec_t* cblk;
	opj_tcd_band_t* band;
	opj_tcd_tilecomp_t* tilec;
	opj_tccp_t* tccp;
	boolint mustuse_cblkdatabuffer;
	volatile boolint* pret;
	opj_event_mgr_t * p_manager;
	opj_mutex_t* p_manager_mutex;
	boolint check_pterm;
} opj_t1_cblk_decode_processing_job_t;

static void opj_t1_destroy_wrapper(void* t1)
{
	opj_t1_destroy((opj_t1_t*)t1);
}

static void opj_t1_clbl_decode_processor(void* user_data, opj_tls_t* tls)
{
	opj_tcd_cblk_dec_t* cblk;
	opj_tcd_band_t* band;
	opj_tcd_tilecomp_t* tilec;
	opj_tccp_t* tccp;
	int32_t* OPJ_RESTRICT datap;
	uint32_t cblk_w, cblk_h;
	int32_t x, y;
	uint32_t i, j;
	opj_t1_cblk_decode_processing_job_t* job;
	opj_t1_t* t1;
	uint32_t resno;
	uint32_t tile_w;

	job = (opj_t1_cblk_decode_processing_job_t*)user_data;

	cblk = job->cblk;

	if(!job->whole_tile_decoding) {
		cblk_w = (uint32_t)(cblk->x1 - cblk->x0);
		cblk_h = (uint32_t)(cblk->y1 - cblk->y0);

		cblk->decoded_data = (int32_t*)opj_aligned_malloc(sizeof(int32_t) *
			cblk_w * cblk_h);
		if(cblk->decoded_data == NULL) {
			if(job->p_manager_mutex) {
				opj_mutex_lock(job->p_manager_mutex);
			}
			opj_event_msg(job->p_manager, EVT_ERROR, "Cannot allocate cblk->decoded_data\n");
			if(job->p_manager_mutex) {
				opj_mutex_unlock(job->p_manager_mutex);
			}
			*(job->pret) = FALSE;
			SAlloc::F(job);
			return;
		}
		/* Zero-init required */
		memzero(cblk->decoded_data, sizeof(int32_t) * cblk_w * cblk_h);
	}
	else if(cblk->decoded_data) {
		/* Not sure if that code path can happen, but better be */
		/* safe than sorry */
		opj_aligned_free(cblk->decoded_data);
		cblk->decoded_data = NULL;
	}

	resno = job->resno;
	band = job->band;
	tilec = job->tilec;
	tccp = job->tccp;
	tile_w = (uint32_t)(tilec->resolutions[tilec->minimum_num_resolutions - 1].x1
	    -
	    tilec->resolutions[tilec->minimum_num_resolutions - 1].x0);

	if(!*(job->pret)) {
		SAlloc::F(job);
		return;
	}

	t1 = (opj_t1_t*)opj_tls_get(tls, OPJ_TLS_KEY_T1);
	if(t1 == NULL) {
		t1 = opj_t1_create(FALSE);
		if(t1 == NULL) {
			opj_event_msg(job->p_manager, EVT_ERROR,
			    "Cannot allocate Tier 1 handle\n");
			*(job->pret) = FALSE;
			SAlloc::F(job);
			return;
		}
		if(!opj_tls_set(tls, OPJ_TLS_KEY_T1, t1, opj_t1_destroy_wrapper)) {
			opj_event_msg(job->p_manager, EVT_ERROR, "Unable to set t1 handle as TLS\n");
			opj_t1_destroy(t1);
			*(job->pret) = FALSE;
			SAlloc::F(job);
			return;
		}
	}
	t1->mustuse_cblkdatabuffer = job->mustuse_cblkdatabuffer;
	if((tccp->cblksty & J2K_CCP_CBLKSTY_HT) != 0) {
		if(FALSE == opj_t1_ht_decode_cblk(t1, cblk, band->bandno, (uint32_t)tccp->roishift, tccp->cblksty, job->p_manager, job->p_manager_mutex, job->check_pterm)) {
			*(job->pret) = FALSE;
			SAlloc::F(job);
			return;
		}
	}
	else {
		if(FALSE == opj_t1_decode_cblk(t1, cblk, band->bandno, (uint32_t)tccp->roishift, tccp->cblksty, job->p_manager, job->p_manager_mutex, job->check_pterm)) {
			*(job->pret) = FALSE;
			SAlloc::F(job);
			return;
		}
	}
	x = cblk->x0 - band->x0;
	y = cblk->y0 - band->y0;
	if(band->bandno & 1) {
		opj_tcd_resolution_t* pres = &tilec->resolutions[resno - 1];
		x += pres->x1 - pres->x0;
	}
	if(band->bandno & 2) {
		opj_tcd_resolution_t* pres = &tilec->resolutions[resno - 1];
		y += pres->y1 - pres->y0;
	}
	datap = cblk->decoded_data ? cblk->decoded_data : t1->data;
	cblk_w = t1->w;
	cblk_h = t1->h;
	if(tccp->roishift) {
		if(tccp->roishift >= 31) {
			for(j = 0; j < cblk_h; ++j) {
				for(i = 0; i < cblk_w; ++i) {
					datap[(j * cblk_w) + i] = 0;
				}
			}
		}
		else {
			int32_t thresh = 1 << tccp->roishift;
			for(j = 0; j < cblk_h; ++j) {
				for(i = 0; i < cblk_w; ++i) {
					int32_t val = datap[(j * cblk_w) + i];
					int32_t mag = abs(val);
					if(mag >= thresh) {
						mag >>= tccp->roishift;
						datap[(j * cblk_w) + i] = val < 0 ? -mag : mag;
					}
				}
			}
		}
	}
	// Both can be non NULL if for example decoding a full tile and then */
	// partially a tile. In which case partial decoding should be the priority
	assert((cblk->decoded_data != NULL) || (tilec->data != NULL));
	if(cblk->decoded_data) {
		uint32_t cblk_size = cblk_w * cblk_h;
		if(tccp->qmfbid == 1) {
			for(i = 0; i < cblk_size; ++i) {
				datap[i] /= 2;
			}
		}
		else {  /* if (tccp->qmfbid == 0) */
			const float stepsize = 0.5f * band->stepsize;
			i = 0;
#ifdef __SSE2__
			{
				const __m128 xmm_stepsize = _mm_set1_ps(stepsize);
				for(; i < (cblk_size & ~15U); i += 16) {
					__m128 xmm0_data = _mm_cvtepi32_ps(_mm_load_si128((__m128i* const)(datap + 0)));
					__m128 xmm1_data = _mm_cvtepi32_ps(_mm_load_si128((__m128i* const)(datap + 4)));
					__m128 xmm2_data = _mm_cvtepi32_ps(_mm_load_si128((__m128i* const)(datap + 8)));
					__m128 xmm3_data = _mm_cvtepi32_ps(_mm_load_si128((__m128i* const)(datap + 12)));
					_mm_store_ps((float*)(datap +  0), _mm_mul_ps(xmm0_data, xmm_stepsize));
					_mm_store_ps((float*)(datap +  4), _mm_mul_ps(xmm1_data, xmm_stepsize));
					_mm_store_ps((float*)(datap +  8), _mm_mul_ps(xmm2_data, xmm_stepsize));
					_mm_store_ps((float*)(datap + 12), _mm_mul_ps(xmm3_data, xmm_stepsize));
					datap += 16;
				}
			}
#endif
			for(; i < cblk_size; ++i) {
				float tmp = ((float)(*datap)) * stepsize;
				memcpy(datap, &tmp, sizeof(tmp));
				datap++;
			}
		}
	}
	else if(tccp->qmfbid == 1) {
		int32_t* OPJ_RESTRICT tiledp = &tilec->data[(size_t)y * tile_w + (size_t)x];
		for(j = 0; j < cblk_h; ++j) {
			i = 0;
			for(; i < (cblk_w & ~(uint32_t)3U); i += 4U) {
				int32_t tmp0 = datap[(j * cblk_w) + i + 0U];
				int32_t tmp1 = datap[(j * cblk_w) + i + 1U];
				int32_t tmp2 = datap[(j * cblk_w) + i + 2U];
				int32_t tmp3 = datap[(j * cblk_w) + i + 3U];
				((int32_t*)tiledp)[(j * (size_t)tile_w) + i + 0U] = tmp0 / 2;
				((int32_t*)tiledp)[(j * (size_t)tile_w) + i + 1U] = tmp1 / 2;
				((int32_t*)tiledp)[(j * (size_t)tile_w) + i + 2U] = tmp2 / 2;
				((int32_t*)tiledp)[(j * (size_t)tile_w) + i + 3U] = tmp3 / 2;
			}
			for(; i < cblk_w; ++i) {
				int32_t tmp = datap[(j * cblk_w) + i];
				((int32_t*)tiledp)[(j * (size_t)tile_w) + i] = tmp / 2;
			}
		}
	}
	else {      /* if (tccp->qmfbid == 0) */
		const float stepsize = 0.5f * band->stepsize;
		float* OPJ_RESTRICT tiledp = (float*)&tilec->data[(size_t)y *
		    tile_w + (size_t)x];
		for(j = 0; j < cblk_h; ++j) {
			float* OPJ_RESTRICT tiledp2 = tiledp;
			for(i = 0; i < cblk_w; ++i) {
				float tmp = (float)*datap * stepsize;
				*tiledp2 = tmp;
				datap++;
				tiledp2++;
			}
			tiledp += tile_w;
		}
	}
	SAlloc::F(job);
}

void opj_t1_decode_cblks(opj_tcd_t* tcd, volatile boolint* pret, opj_tcd_tilecomp_t* tilec, opj_tccp_t* tccp,
    opj_event_mgr_t * p_manager, opj_mutex_t* p_manager_mutex, boolint check_pterm)
{
	opj_thread_pool_t* tp = tcd->thread_pool;
	uint32_t resno, bandno, precno, cblkno;
#ifdef DEBUG_VERBOSE
	uint32_t codeblocks_decoded = 0;
	printf("Enter opj_t1_decode_cblks()\n");
#endif
	for(resno = 0; resno < tilec->minimum_num_resolutions; ++resno) {
		opj_tcd_resolution_t* res = &tilec->resolutions[resno];
		for(bandno = 0; bandno < res->numbands; ++bandno) {
			opj_tcd_band_t* OPJ_RESTRICT band = &res->bands[bandno];
			for(precno = 0; precno < res->pw * res->ph; ++precno) {
				opj_tcd_precinct_t* precinct = &band->precincts[precno];
				if(!opj_tcd_is_subband_area_of_interest(tcd, tilec->compno, resno, band->bandno, (uint32_t)precinct->x0, (uint32_t)precinct->y0, (uint32_t)precinct->x1, (uint32_t)precinct->y1)) {
					for(cblkno = 0; cblkno < precinct->cw * precinct->ch; ++cblkno) {
						opj_tcd_cblk_dec_t* cblk = &precinct->cblks.dec[cblkno];
						if(cblk->decoded_data) {
#ifdef DEBUG_VERBOSE
							printf("Discarding codeblock %d,%d at resno=%d, bandno=%d\n", cblk->x0, cblk->y0, resno, bandno);
#endif
							opj_aligned_free(cblk->decoded_data);
							cblk->decoded_data = NULL;
						}
					}
					continue;
				}
				for(cblkno = 0; cblkno < precinct->cw * precinct->ch; ++cblkno) {
					opj_tcd_cblk_dec_t* cblk = &precinct->cblks.dec[cblkno];
					opj_t1_cblk_decode_processing_job_t* job;
					if(!opj_tcd_is_subband_area_of_interest(tcd, tilec->compno, resno, band->bandno,
					    (uint32_t)cblk->x0, (uint32_t)cblk->y0, (uint32_t)cblk->x1, (uint32_t)cblk->y1)) {
						if(cblk->decoded_data) {
#ifdef DEBUG_VERBOSE
							printf("Discarding codeblock %d,%d at resno=%d, bandno=%d\n", cblk->x0, cblk->y0, resno, bandno);
#endif
							opj_aligned_free(cblk->decoded_data);
							cblk->decoded_data = NULL;
						}
						continue;
					}
					if(!tcd->whole_tile_decoding) {
						uint32_t cblk_w = (uint32_t)(cblk->x1 - cblk->x0);
						uint32_t cblk_h = (uint32_t)(cblk->y1 - cblk->y0);
						if(cblk->decoded_data != NULL) {
#ifdef DEBUG_VERBOSE
							printf("Reusing codeblock %d,%d at resno=%d, bandno=%d\n", cblk->x0, cblk->y0, resno, bandno);
#endif
							continue;
						}
						if(cblk_w == 0 || cblk_h == 0) {
							continue;
						}
#ifdef DEBUG_VERBOSE
						printf("Decoding codeblock %d,%d at resno=%d, bandno=%d\n", cblk->x0, cblk->y0, resno, bandno);
#endif
					}
					job = (opj_t1_cblk_decode_processing_job_t*)opj_calloc(1, sizeof(opj_t1_cblk_decode_processing_job_t));
					if(!job) {
						*pret = FALSE;
						return;
					}
					job->whole_tile_decoding = tcd->whole_tile_decoding;
					job->resno = resno;
					job->cblk = cblk;
					job->band = band;
					job->tilec = tilec;
					job->tccp = tccp;
					job->pret = pret;
					job->p_manager_mutex = p_manager_mutex;
					job->p_manager = p_manager;
					job->check_pterm = check_pterm;
					job->mustuse_cblkdatabuffer = opj_thread_pool_get_thread_count(tp) > 1;
					opj_thread_pool_submit_job(tp, opj_t1_clbl_decode_processor, job);
#ifdef DEBUG_VERBOSE
					codeblocks_decoded++;
#endif
					if(!(*pret)) {
						return;
					}
				} /* cblkno */
			} /* precno */
		} /* bandno */
	} /* resno */

#ifdef DEBUG_VERBOSE
	printf("Leave opj_t1_decode_cblks(). Number decoded: %d\n", codeblocks_decoded);
#endif
	return;
}

static boolint opj_t1_decode_cblk(opj_t1_t * t1,
    opj_tcd_cblk_dec_t* cblk,
    uint32_t orient,
    uint32_t roishift,
    uint32_t cblksty,
    opj_event_mgr_t * p_manager,
    opj_mutex_t* p_manager_mutex,
    boolint check_pterm)
{
	opj_mqc_t * mqc = &(t1->mqc); /* MQC component */

	int32_t bpno_plus_one;
	uint32_t passtype;
	uint32_t segno, passno;
	uint8* cblkdata = NULL;
	uint32_t cblkdataindex = 0;
	uint8 type = T1_TYPE_MQ; /* BYPASS mode */
	int32_t* original_t1_data = NULL;

	mqc->lut_ctxno_zc_orient = lut_ctxno_zc + (orient << 9);

	if(!opj_t1_allocate_buffers(
		    t1,
		    (uint32_t)(cblk->x1 - cblk->x0),
		    (uint32_t)(cblk->y1 - cblk->y0))) {
		return FALSE;
	}

	bpno_plus_one = (int32_t)(roishift + cblk->numbps);
	if(bpno_plus_one >= 31) {
		if(p_manager_mutex) {
			opj_mutex_lock(p_manager_mutex);
		}
		opj_event_msg(p_manager, EVT_WARNING,
		    "opj_t1_decode_cblk(): unsupported bpno_plus_one = %d >= 31\n",
		    bpno_plus_one);
		if(p_manager_mutex) {
			opj_mutex_unlock(p_manager_mutex);
		}
		return FALSE;
	}
	passtype = 2;

	opj_mqc_resetstates(mqc);
	opj_mqc_setstate(mqc, T1_CTXNO_UNI, 0, 46);
	opj_mqc_setstate(mqc, T1_CTXNO_AGG, 0, 3);
	opj_mqc_setstate(mqc, T1_CTXNO_ZC, 0, 4);

	/* Even if we have a single chunk, in multi-threaded decoding */
	/* the insertion of our synthetic marker might potentially override */
	/* valid codestream of other codeblocks decoded in parallel. */
	if(cblk->numchunks > 1 || t1->mustuse_cblkdatabuffer) {
		uint32_t i;
		uint32_t cblk_len;

		/* Compute whole codeblock length from chunk lengths */
		cblk_len = 0;
		for(i = 0; i < cblk->numchunks; i++) {
			cblk_len += cblk->chunks[i].len;
		}

		/* Allocate temporary memory if needed */
		if(cblk_len + OPJ_COMMON_CBLK_DATA_EXTRA > t1->cblkdatabuffersize) {
			cblkdata = (uint8*)opj_realloc(t1->cblkdatabuffer,
				cblk_len + OPJ_COMMON_CBLK_DATA_EXTRA);
			if(cblkdata == NULL) {
				return FALSE;
			}
			t1->cblkdatabuffer = cblkdata;
			memzero(t1->cblkdatabuffer + cblk_len, OPJ_COMMON_CBLK_DATA_EXTRA);
			t1->cblkdatabuffersize = cblk_len + OPJ_COMMON_CBLK_DATA_EXTRA;
		}

		/* Concatenate all chunks */
		cblkdata = t1->cblkdatabuffer;
		cblk_len = 0;
		for(i = 0; i < cblk->numchunks; i++) {
			memcpy(cblkdata + cblk_len, cblk->chunks[i].data, cblk->chunks[i].len);
			cblk_len += cblk->chunks[i].len;
		}
	}
	else if(cblk->numchunks == 1) {
		cblkdata = cblk->chunks[0].data;
	}
	else {
		/* Not sure if that can happen in practice, but avoid Coverity to */
		/* think we will dereference a null cblkdta pointer */
		return TRUE;
	}

	/* For subtile decoding, directly decode in the decoded_data buffer of */
	/* the code-block. Hack t1->data to point to it, and restore it later */
	if(cblk->decoded_data) {
		original_t1_data = t1->data;
		t1->data = cblk->decoded_data;
	}

	for(segno = 0; segno < cblk->real_num_segs; ++segno) {
		opj_tcd_seg_t * seg = &cblk->segs[segno];

		/* BYPASS mode */
		type = ((bpno_plus_one <= ((int32_t)(cblk->numbps)) - 4) && (passtype < 2) &&
		    (cblksty & J2K_CCP_CBLKSTY_LAZY)) ? T1_TYPE_RAW : T1_TYPE_MQ;

		if(type == T1_TYPE_RAW) {
			opj_mqc_raw_init_dec(mqc, cblkdata + cblkdataindex, seg->len,
			    OPJ_COMMON_CBLK_DATA_EXTRA);
		}
		else {
			opj_mqc_init_dec(mqc, cblkdata + cblkdataindex, seg->len,
			    OPJ_COMMON_CBLK_DATA_EXTRA);
		}
		cblkdataindex += seg->len;

		for(passno = 0; (passno < seg->real_num_passes) &&
		    (bpno_plus_one >= 1); ++passno) {
			switch(passtype) {
				case 0:
				    if(type == T1_TYPE_RAW) {
					    opj_t1_dec_sigpass_raw(t1, bpno_plus_one, (int32_t)cblksty);
				    }
				    else {
					    opj_t1_dec_sigpass_mqc(t1, bpno_plus_one, (int32_t)cblksty);
				    }
				    break;
				case 1:
				    if(type == T1_TYPE_RAW) {
					    opj_t1_dec_refpass_raw(t1, bpno_plus_one);
				    }
				    else {
					    opj_t1_dec_refpass_mqc(t1, bpno_plus_one);
				    }
				    break;
				case 2:
				    opj_t1_dec_clnpass(t1, bpno_plus_one, (int32_t)cblksty);
				    break;
			}

			if((cblksty & J2K_CCP_CBLKSTY_RESET) && type == T1_TYPE_MQ) {
				opj_mqc_resetstates(mqc);
				opj_mqc_setstate(mqc, T1_CTXNO_UNI, 0, 46);
				opj_mqc_setstate(mqc, T1_CTXNO_AGG, 0, 3);
				opj_mqc_setstate(mqc, T1_CTXNO_ZC, 0, 4);
			}
			if(++passtype == 3) {
				passtype = 0;
				bpno_plus_one--;
			}
		}

		opq_mqc_finish_dec(mqc);
	}

	if(check_pterm) {
		if(mqc->bp + 2 < mqc->end) {
			if(p_manager_mutex) {
				opj_mutex_lock(p_manager_mutex);
			}
			opj_event_msg(p_manager, EVT_WARNING,
			    "PTERM check failure: %d remaining bytes in code block (%d used / %d)\n",
			    (int)(mqc->end - mqc->bp) - 2,
			    (int)(mqc->bp - mqc->start),
			    (int)(mqc->end - mqc->start));
			if(p_manager_mutex) {
				opj_mutex_unlock(p_manager_mutex);
			}
		}
		else if(mqc->end_of_byte_stream_counter > 2) {
			if(p_manager_mutex) {
				opj_mutex_lock(p_manager_mutex);
			}
			opj_event_msg(p_manager, EVT_WARNING,
			    "PTERM check failure: %d synthetized 0xFF markers read\n",
			    mqc->end_of_byte_stream_counter);
			if(p_manager_mutex) {
				opj_mutex_unlock(p_manager_mutex);
			}
		}
	}

	/* Restore original t1->data is needed */
	if(cblk->decoded_data) {
		t1->data = original_t1_data;
	}

	return TRUE;
}

typedef struct {
	uint32_t compno;
	uint32_t resno;
	opj_tcd_cblk_enc_t* cblk;
	opj_tcd_tile_t * tile;
	opj_tcd_band_t* band;
	opj_tcd_tilecomp_t* tilec;
	opj_tccp_t* tccp;
	const double * mct_norms;
	uint32_t mct_numcomps;
	volatile boolint* pret;
	opj_mutex_t* mutex;
} opj_t1_cblk_encode_processing_job_t;

/** Procedure to deal with a asynchronous code-block encoding job.
 *
 * @param user_data Pointer to a opj_t1_cblk_encode_processing_job_t* structure
 * @param tls       TLS handle.
 */
static void opj_t1_cblk_encode_processor(void* user_data, opj_tls_t* tls)
{
	opj_t1_cblk_encode_processing_job_t* job =
	    (opj_t1_cblk_encode_processing_job_t*)user_data;
	opj_tcd_cblk_enc_t* cblk = job->cblk;
	const opj_tcd_band_t* band = job->band;
	const opj_tcd_tilecomp_t* tilec = job->tilec;
	const opj_tccp_t* tccp = job->tccp;
	const uint32_t resno = job->resno;
	opj_t1_t* t1;
	const uint32_t tile_w = (uint32_t)(tilec->x1 - tilec->x0);

	int32_t* OPJ_RESTRICT tiledp;
	uint32_t cblk_w;
	uint32_t cblk_h;
	uint32_t i, j;

	int32_t x = cblk->x0 - band->x0;
	int32_t y = cblk->y0 - band->y0;

	if(!*(job->pret)) {
		SAlloc::F(job);
		return;
	}

	t1 = (opj_t1_t*)opj_tls_get(tls, OPJ_TLS_KEY_T1);
	if(t1 == NULL) {
		t1 = opj_t1_create(TRUE); /* TRUE == T1 for encoding */
		opj_tls_set(tls, OPJ_TLS_KEY_T1, t1, opj_t1_destroy_wrapper);
	}

	if(band->bandno & 1) {
		opj_tcd_resolution_t * pres = &tilec->resolutions[resno - 1];
		x += pres->x1 - pres->x0;
	}
	if(band->bandno & 2) {
		opj_tcd_resolution_t * pres = &tilec->resolutions[resno - 1];
		y += pres->y1 - pres->y0;
	}

	if(!opj_t1_allocate_buffers(
		    t1,
		    (uint32_t)(cblk->x1 - cblk->x0),
		    (uint32_t)(cblk->y1 - cblk->y0))) {
		*(job->pret) = FALSE;
		SAlloc::F(job);
		return;
	}

	cblk_w = t1->w;
	cblk_h = t1->h;

	tiledp = &tilec->data[(size_t)y * tile_w + (size_t)x];

	if(tccp->qmfbid == 1) {
		/* Do multiplication on unsigned type, even if the
		 * underlying type is signed, to avoid potential
		 * int overflow on large value (the output will be
		 * incorrect in such situation, but whatever...)
		 * This assumes complement-to-2 signed integer
		 * representation
		 * Fixes https://github.com/uclouvain/openjpeg/issues/1053
		 */
		uint32_t* OPJ_RESTRICT tiledp_u = (uint32_t*)tiledp;
		uint32_t* OPJ_RESTRICT t1data = (uint32_t*)t1->data;
		/* Change from "natural" order to "zigzag" order of T1 passes */
		for(j = 0; j < (cblk_h & ~3U); j += 4) {
			for(i = 0; i < cblk_w; ++i) {
				t1data[0] = tiledp_u[(j + 0) * tile_w + i] << T1_NMSEDEC_FRACBITS;
				t1data[1] = tiledp_u[(j + 1) * tile_w + i] << T1_NMSEDEC_FRACBITS;
				t1data[2] = tiledp_u[(j + 2) * tile_w + i] << T1_NMSEDEC_FRACBITS;
				t1data[3] = tiledp_u[(j + 3) * tile_w + i] << T1_NMSEDEC_FRACBITS;
				t1data += 4;
			}
		}
		if(j < cblk_h) {
			for(i = 0; i < cblk_w; ++i) {
				uint32_t k;
				for(k = j; k < cblk_h; k++) {
					t1data[0] = tiledp_u[k * tile_w + i] << T1_NMSEDEC_FRACBITS;
					t1data++;
				}
			}
		}
	}
	else {      /* if (tccp->qmfbid == 0) */
		float* OPJ_RESTRICT tiledp_f = (float*)tiledp;
		int32_t* OPJ_RESTRICT t1data = t1->data;
		/* Change from "natural" order to "zigzag" order of T1 passes */
		for(j = 0; j < (cblk_h & ~3U); j += 4) {
			for(i = 0; i < cblk_w; ++i) {
				t1data[0] = (int32_t)opj_lrintf((tiledp_f[(j + 0) * tile_w + i] /
					band->stepsize) * (1 << T1_NMSEDEC_FRACBITS));
				t1data[1] = (int32_t)opj_lrintf((tiledp_f[(j + 1) * tile_w + i] /
					band->stepsize) * (1 << T1_NMSEDEC_FRACBITS));
				t1data[2] = (int32_t)opj_lrintf((tiledp_f[(j + 2) * tile_w + i] /
					band->stepsize) * (1 << T1_NMSEDEC_FRACBITS));
				t1data[3] = (int32_t)opj_lrintf((tiledp_f[(j + 3) * tile_w + i] /
					band->stepsize) * (1 << T1_NMSEDEC_FRACBITS));
				t1data += 4;
			}
		}
		if(j < cblk_h) {
			for(i = 0; i < cblk_w; ++i) {
				uint32_t k;
				for(k = j; k < cblk_h; k++) {
					t1data[0] = (int32_t)opj_lrintf((tiledp_f[k * tile_w + i] / band->stepsize)
						* (1 << T1_NMSEDEC_FRACBITS));
					t1data++;
				}
			}
		}
	}

	{
		double cumwmsedec =
		    opj_t1_encode_cblk(
			t1,
			cblk,
			band->bandno,
			job->compno,
			tilec->numresolutions - 1 - resno,
			tccp->qmfbid,
			band->stepsize,
			tccp->cblksty,
			job->tile->numcomps,
			job->mct_norms,
			job->mct_numcomps);
		if(job->mutex) {
			opj_mutex_lock(job->mutex);
		}
		job->tile->distotile += cumwmsedec;
		if(job->mutex) {
			opj_mutex_unlock(job->mutex);
		}
	}

	SAlloc::F(job);
}

boolint opj_t1_encode_cblks(opj_tcd_t* tcd,
    opj_tcd_tile_t * tile,
    opj_tcp_t * tcp,
    const double * mct_norms,
    uint32_t mct_numcomps
    )
{
	volatile boolint ret = TRUE;
	opj_thread_pool_t* tp = tcd->thread_pool;
	uint32_t compno, resno, bandno, precno, cblkno;
	opj_mutex_t* mutex = opj_mutex_create();

	tile->distotile = 0;    /* fixed_quality */

	for(compno = 0; compno < tile->numcomps; ++compno) {
		opj_tcd_tilecomp_t* tilec = &tile->comps[compno];
		opj_tccp_t* tccp = &tcp->tccps[compno];

		for(resno = 0; resno < tilec->numresolutions; ++resno) {
			opj_tcd_resolution_t * res = &tilec->resolutions[resno];

			for(bandno = 0; bandno < res->numbands; ++bandno) {
				opj_tcd_band_t* OPJ_RESTRICT band = &res->bands[bandno];

				/* Skip empty bands */
				if(opj_tcd_is_band_empty(band)) {
					continue;
				}
				for(precno = 0; precno < res->pw * res->ph; ++precno) {
					opj_tcd_precinct_t * prc = &band->precincts[precno];

					for(cblkno = 0; cblkno < prc->cw * prc->ch; ++cblkno) {
						opj_tcd_cblk_enc_t* cblk = &prc->cblks.enc[cblkno];

						opj_t1_cblk_encode_processing_job_t* job =
						    (opj_t1_cblk_encode_processing_job_t*)opj_calloc(1,
							sizeof(opj_t1_cblk_encode_processing_job_t));
						if(!job) {
							ret = FALSE;
							goto end;
						}
						job->compno = compno;
						job->tile = tile;
						job->resno = resno;
						job->cblk = cblk;
						job->band = band;
						job->tilec = tilec;
						job->tccp = tccp;
						job->mct_norms = mct_norms;
						job->mct_numcomps = mct_numcomps;
						job->pret = &ret;
						job->mutex = mutex;
						opj_thread_pool_submit_job(tp, opj_t1_cblk_encode_processor, job);
					} /* cblkno */
				} /* precno */
			} /* bandno */
		} /* resno  */
	} /* compno  */

end:
	opj_thread_pool_wait_completion(tcd->thread_pool, 0);
	if(mutex) {
		opj_mutex_destroy(mutex);
	}

	return ret;
}

/* Returns whether the pass (bpno, passtype) is terminated */
static int opj_t1_enc_is_term_pass(opj_tcd_cblk_enc_t* cblk,
    uint32_t cblksty,
    int32_t bpno,
    uint32_t passtype)
{
	/* Is it the last cleanup pass ? */
	if(passtype == 2 && bpno == 0) {
		return TRUE;
	}

	if(cblksty & J2K_CCP_CBLKSTY_TERMALL) {
		return TRUE;
	}

	if((cblksty & J2K_CCP_CBLKSTY_LAZY)) {
		/* For bypass arithmetic bypass, terminate the 4th cleanup pass */
		if((bpno == ((int32_t)cblk->numbps - 4)) && (passtype == 2)) {
			return TRUE;
		}
		/* and beyond terminate all the magnitude refinement passes (in raw) */
		/* and cleanup passes (in MQC) */
		if((bpno < ((int32_t)(cblk->numbps) - 4)) && (passtype > 0)) {
			return TRUE;
		}
	}

	return FALSE;
}

/** mod fixed_quality */
static double opj_t1_encode_cblk(opj_t1_t * t1,
    opj_tcd_cblk_enc_t* cblk,
    uint32_t orient,
    uint32_t compno,
    uint32_t level,
    uint32_t qmfbid,
    double stepsize,
    uint32_t cblksty,
    uint32_t numcomps,
    const double * mct_norms,
    uint32_t mct_numcomps)
{
	double cumwmsedec = 0.0;

	opj_mqc_t * mqc = &(t1->mqc); /* MQC component */

	uint32_t passno;
	int32_t bpno;
	uint32_t passtype;
	int32_t nmsedec = 0;
	int32_t max;
	uint32_t i, j;
	uint8 type = T1_TYPE_MQ;
	double tempwmsedec;
	int32_t* datap;

#ifdef EXTRA_DEBUG
	printf("encode_cblk(x=%d,y=%d,x1=%d,y1=%d,orient=%d,compno=%d,level=%d\n",
	    cblk->x0, cblk->y0, cblk->x1, cblk->y1, orient, compno, level);
#endif

	mqc->lut_ctxno_zc_orient = lut_ctxno_zc + (orient << 9);

	max = 0;
	datap = t1->data;
	for(j = 0; j < t1->h; ++j) {
		const uint32_t w = t1->w;
		for(i = 0; i < w; ++i, ++datap) {
			int32_t tmp = *datap;
			if(tmp < 0) {
				uint32_t tmp_unsigned;
				max = opj_int_max(max, -tmp);
				tmp_unsigned = opj_to_smr(tmp);
				memcpy(datap, &tmp_unsigned, sizeof(int32_t));
			}
			else {
				max = opj_int_max(max, tmp);
			}
		}
	}

	cblk->numbps = max ? (uint32_t)((opj_int_floorlog2(max) + 1) -
	    T1_NMSEDEC_FRACBITS) : 0;
	if(cblk->numbps == 0) {
		cblk->totalpasses = 0;
		return cumwmsedec;
	}

	bpno = (int32_t)(cblk->numbps - 1);
	passtype = 2;

	opj_mqc_resetstates(mqc);
	opj_mqc_setstate(mqc, T1_CTXNO_UNI, 0, 46);
	opj_mqc_setstate(mqc, T1_CTXNO_AGG, 0, 3);
	opj_mqc_setstate(mqc, T1_CTXNO_ZC, 0, 4);
	opj_mqc_init_enc(mqc, cblk->data);

	for(passno = 0; bpno >= 0; ++passno) {
		opj_tcd_pass_t * pass = &cblk->passes[passno];
		type = ((bpno < ((int32_t)(cblk->numbps) - 4)) && (passtype < 2) &&
		    (cblksty & J2K_CCP_CBLKSTY_LAZY)) ? T1_TYPE_RAW : T1_TYPE_MQ;

		/* If the previous pass was terminating, we need to reset the encoder */
		if(passno > 0 && cblk->passes[passno - 1].term) {
			if(type == T1_TYPE_RAW) {
				opj_mqc_bypass_init_enc(mqc);
			}
			else {
				opj_mqc_restart_init_enc(mqc);
			}
		}

		switch(passtype) {
			case 0:
			    opj_t1_enc_sigpass(t1, bpno, &nmsedec, type, cblksty);
			    break;
			case 1:
			    opj_t1_enc_refpass(t1, bpno, &nmsedec, type);
			    break;
			case 2:
			    opj_t1_enc_clnpass(t1, bpno, &nmsedec, cblksty);
			    /* code switch SEGMARK (i.e. SEGSYM) */
			    if(cblksty & J2K_CCP_CBLKSTY_SEGSYM) {
				    opj_mqc_segmark_enc(mqc);
			    }
			    break;
		}

		/* fixed_quality */
		tempwmsedec = opj_t1_getwmsedec(nmsedec, compno, level, orient, bpno, qmfbid,
			stepsize, numcomps, mct_norms, mct_numcomps);
		cumwmsedec += tempwmsedec;
		pass->distortiondec = cumwmsedec;

		if(opj_t1_enc_is_term_pass(cblk, cblksty, bpno, passtype)) {
			/* If it is a terminated pass, terminate it */
			if(type == T1_TYPE_RAW) {
				opj_mqc_bypass_flush_enc(mqc, cblksty & J2K_CCP_CBLKSTY_PTERM);
			}
			else {
				if(cblksty & J2K_CCP_CBLKSTY_PTERM) {
					opj_mqc_erterm_enc(mqc);
				}
				else {
					opj_mqc_flush(mqc);
				}
			}
			pass->term = 1;
			pass->rate = opj_mqc_numbytes(mqc);
		}
		else {
			/* Non terminated pass */
			uint32_t rate_extra_bytes;
			if(type == T1_TYPE_RAW) {
				rate_extra_bytes = opj_mqc_bypass_get_extra_bytes(
					mqc, (cblksty & J2K_CCP_CBLKSTY_PTERM));
			}
			else {
				rate_extra_bytes = 3;
			}
			pass->term = 0;
			pass->rate = opj_mqc_numbytes(mqc) + rate_extra_bytes;
		}

		if(++passtype == 3) {
			passtype = 0;
			bpno--;
		}

		/* Code-switch "RESET" */
		if(cblksty & J2K_CCP_CBLKSTY_RESET) {
			opj_mqc_reset_enc(mqc);
		}
	}

	cblk->totalpasses = passno;

	if(cblk->totalpasses) {
		/* Make sure that pass rates are increasing */
		uint32_t last_pass_rate = opj_mqc_numbytes(mqc);
		for(passno = cblk->totalpasses; passno > 0;) {
			opj_tcd_pass_t * pass = &cblk->passes[--passno];
			if(pass->rate > last_pass_rate) {
				pass->rate = last_pass_rate;
			}
			else {
				last_pass_rate = pass->rate;
			}
		}
	}
	for(passno = 0; passno < cblk->totalpasses; passno++) {
		opj_tcd_pass_t * pass = &cblk->passes[passno];
		/* Prevent generation of FF as last data byte of a pass*/
		/* For terminating passes, the flushing procedure ensured this already */
		assert(pass->rate > 0);
		if(cblk->data[pass->rate - 1] == 0xFF) {
			pass->rate--;
		}
		pass->len = pass->rate - (passno == 0 ? 0 : cblk->passes[passno - 1].rate);
	}
#ifdef EXTRA_DEBUG
	printf(" len=%d\n", (cblk->totalpasses) ? opj_mqc_numbytes(mqc) : 0);
	/* Check that there not 0xff >=0x90 sequences */
	if(cblk->totalpasses) {
		uint32_t i;
		uint32_t len = opj_mqc_numbytes(mqc);
		for(i = 1; i < len; ++i) {
			if(cblk->data[i - 1] == 0xff && cblk->data[i] >= 0x90) {
				printf("0xff %02x at offset %d\n", cblk->data[i], i - 1);
				abort();
			}
		}
	}
#endif
	return cumwmsedec;
}
