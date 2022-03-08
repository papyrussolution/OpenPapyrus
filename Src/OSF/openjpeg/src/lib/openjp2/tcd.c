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
 * Copyright (c) 2006-2007, Parvatha Elangovan
 * Copyright (c) 2008, 2011-2012, Centre National d'Etudes Spatiales (CNES), FR
 * Copyright (c) 2012, CS Systemes d'Information, France
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
#include "opj_common.h"

/* ----------------------------------------------------------------------- */

/* TODO MSD: */
#ifdef TODO_MSD
void tcd_dump(FILE * fd, opj_tcd_t * tcd, opj_tcd_image_t * img)
{
	int tileno, compno, resno, bandno, precno; /*, cblkno;*/
	fprintf(fd, "image {\n");
	fprintf(fd, "  tw=%d, th=%d x0=%d x1=%d y0=%d y1=%d\n",
	    img->tw, img->th, tcd->image->x0, tcd->image->x1, tcd->image->y0, tcd->image->y1);
	for(tileno = 0; tileno < img->th * img->tw; tileno++) {
		opj_tcd_tile_t * tile = &tcd->tcd_image->tiles[tileno];
		fprintf(fd, "  tile {\n");
		fprintf(fd, "    x0=%d, y0=%d, x1=%d, y1=%d, numcomps=%d\n",
		    tile->x0, tile->y0, tile->x1, tile->y1, tile->numcomps);
		for(compno = 0; compno < tile->numcomps; compno++) {
			opj_tcd_tilecomp_t * tilec = &tile->comps[compno];
			fprintf(fd, "    tilec {\n");
			fprintf(fd,
			    "      x0=%d, y0=%d, x1=%d, y1=%d, numresolutions=%d\n",
			    tilec->x0, tilec->y0, tilec->x1, tilec->y1, tilec->numresolutions);
			for(resno = 0; resno < tilec->numresolutions; resno++) {
				opj_tcd_resolution_t * res = &tilec->resolutions[resno];
				fprintf(fd, "\n   res {\n");
				fprintf(fd,
				    "          x0=%d, y0=%d, x1=%d, y1=%d, pw=%d, ph=%d, numbands=%d\n",
				    res->x0, res->y0, res->x1, res->y1, res->pw, res->ph, res->numbands);
				for(bandno = 0; bandno < res->numbands; bandno++) {
					opj_tcd_band_t * band = &res->bands[bandno];
					fprintf(fd, "        band {\n");
					fprintf(fd,
					    "          x0=%d, y0=%d, x1=%d, y1=%d, stepsize=%f, numbps=%d\n",
					    band->x0, band->y0, band->x1, band->y1, band->stepsize, band->numbps);
					for(precno = 0; precno < res->pw * res->ph; precno++) {
						opj_tcd_precinct_t * prec = &band->precincts[precno];
						fprintf(fd, "          prec {\n");
						fprintf(fd,
						    "            x0=%d, y0=%d, x1=%d, y1=%d, cw=%d, ch=%d\n",
						    prec->x0, prec->y0, prec->x1, prec->y1, prec->cw, prec->ch);
						/*
						   for (cblkno = 0; cblkno < prec->cw * prec->ch; cblkno++) {
						        opj_tcd_cblk_t *cblk = &prec->cblks[cblkno];
						        fprintf(fd, "            cblk {\n");
						        fprintf(fd,
						                "              x0=%d, y0=%d, x1=%d, y1=%d\n",
						                cblk->x0, cblk->y0, cblk->x1, cblk->y1);
						        fprintf(fd, "            }\n");
						   }
						 */
						fprintf(fd, "          }\n");
					}
					fprintf(fd, "        }\n");
				}
				fprintf(fd, "      }\n");
			}
			fprintf(fd, "    }\n");
		}
		fprintf(fd, "  }\n");
	}
	fprintf(fd, "}\n");
}

#endif
/**
 * Initializes tile coding/decoding
 */
static INLINE boolint opj_tcd_init_tile(opj_tcd_t * p_tcd, OPJ_UINT32 p_tile_no, boolint isEncoder, OPJ_SIZE_T sizeof_block, opj_event_mgr_t* manager);
/**
 * Allocates memory for a decoding code block.
 */
static boolint opj_tcd_code_block_dec_allocate(opj_tcd_cblk_dec_t * p_code_block);
/**
 * Deallocates the decoding data of the given precinct.
 */
static void opj_tcd_code_block_dec_deallocate(opj_tcd_precinct_t * p_precinct);
/**
 * Allocates memory for an encoding code block (but not data).
 */
static boolint opj_tcd_code_block_enc_allocate(opj_tcd_cblk_enc_t * p_code_block);
/**
 * Allocates data for an encoding code block
 */
static boolint opj_tcd_code_block_enc_allocate_data(opj_tcd_cblk_enc_t * p_code_block);
/**
 * Deallocates the encoding data of the given precinct.
 */
static void opj_tcd_code_block_enc_deallocate(opj_tcd_precinct_t * p_precinct);

/**
   Free the memory allocated for encoding
   @param tcd TCD handle
 */
static void opj_tcd_free_tile(opj_tcd_t * tcd);
static boolint opj_tcd_t2_decode(opj_tcd_t * p_tcd,
    uint8 * p_src_data,
    OPJ_UINT32 * p_data_read,
    OPJ_UINT32 p_max_src_size,
    opj_codestream_index_t * p_cstr_index,
    opj_event_mgr_t * p_manager);
static boolint opj_tcd_t1_decode(opj_tcd_t * p_tcd, opj_event_mgr_t * p_manager);
static boolint opj_tcd_dwt_decode(opj_tcd_t * p_tcd);
static boolint opj_tcd_mct_decode(opj_tcd_t * p_tcd, opj_event_mgr_t * p_manager);
static boolint opj_tcd_dc_level_shift_decode(opj_tcd_t * p_tcd);
static boolint opj_tcd_dc_level_shift_encode(opj_tcd_t * p_tcd);
static boolint opj_tcd_mct_encode(opj_tcd_t * p_tcd);
static boolint opj_tcd_dwt_encode(opj_tcd_t * p_tcd);
static boolint opj_tcd_t1_encode(opj_tcd_t * p_tcd);
static boolint opj_tcd_t2_encode(opj_tcd_t * p_tcd,
    uint8 * p_dest_data,
    OPJ_UINT32 * p_data_written,
    OPJ_UINT32 p_max_dest_size,
    opj_codestream_info_t * p_cstr_info,
    opj_tcd_marker_info_t* p_marker_info,
    opj_event_mgr_t * p_manager);

static boolint opj_tcd_rate_allocate_encode(opj_tcd_t * p_tcd, uint8 * p_dest_data, OPJ_UINT32 p_max_dest_size, opj_codestream_info_t * p_cstr_info, opj_event_mgr_t * p_manager);
static boolint opj_tcd_is_whole_tilecomp_decoding(opj_tcd_t * tcd, OPJ_UINT32 compno);

/* ----------------------------------------------------------------------- */

/**
   Create a new TCD handle
 */
opj_tcd_t* opj_tcd_create(boolint p_is_decoder)
{
	/* create the tcd structure */
	opj_tcd_t * l_tcd = (opj_tcd_t*)SAlloc::C(1, sizeof(opj_tcd_t));
	if(l_tcd) {
		l_tcd->m_is_decoder = p_is_decoder ? 1 : 0;
		l_tcd->tcd_image = (opj_tcd_image_t*)SAlloc::C(1, sizeof(opj_tcd_image_t));
		if(!l_tcd->tcd_image)
			ZFREE(l_tcd);
	}
	return l_tcd;
}

void opj_tcd_rateallocate_fixed(opj_tcd_t * tcd)
{
	for(OPJ_UINT32 layno = 0; layno < tcd->tcp->numlayers; layno++) {
		opj_tcd_makelayer_fixed(tcd, layno, 1);
	}
}

void opj_tcd_makelayer(opj_tcd_t * tcd, OPJ_UINT32 layno, double thresh, OPJ_UINT32 final)
{
	OPJ_UINT32 compno, resno, bandno, precno, cblkno;
	OPJ_UINT32 passno;
	opj_tcd_tile_t * tcd_tile = tcd->tcd_image->tiles;
	tcd_tile->distolayer[layno] = 0; /* fixed_quality */
	for(compno = 0; compno < tcd_tile->numcomps; compno++) {
		opj_tcd_tilecomp_t * tilec = &tcd_tile->comps[compno];
		for(resno = 0; resno < tilec->numresolutions; resno++) {
			opj_tcd_resolution_t * res = &tilec->resolutions[resno];
			for(bandno = 0; bandno < res->numbands; bandno++) {
				opj_tcd_band_t * band = &res->bands[bandno];
				/* Skip empty bands */
				if(opj_tcd_is_band_empty(band)) {
					continue;
				}
				for(precno = 0; precno < res->pw * res->ph; precno++) {
					opj_tcd_precinct_t * prc = &band->precincts[precno];
					for(cblkno = 0; cblkno < prc->cw * prc->ch; cblkno++) {
						opj_tcd_cblk_enc_t * cblk = &prc->cblks.enc[cblkno];
						opj_tcd_layer_t * layer = &cblk->layers[layno];
						OPJ_UINT32 n;
						if(layno == 0) {
							cblk->numpassesinlayers = 0;
						}
						n = cblk->numpassesinlayers;
						if(thresh < 0) {
							/* Special value to indicate to use all passes */
							n = cblk->totalpasses;
						}
						else {
							for(passno = cblk->numpassesinlayers; passno < cblk->totalpasses; passno++) {
								OPJ_UINT32 dr;
								double dd;
								opj_tcd_pass_t * pass = &cblk->passes[passno];
								if(n == 0) {
									dr = pass->rate;
									dd = pass->distortiondec;
								}
								else {
									dr = pass->rate - cblk->passes[n - 1].rate;
									dd = pass->distortiondec - cblk->passes[n - 1].distortiondec;
								}

								if(!dr) {
									if(dd != 0) {
										n = passno + 1;
									}
									continue;
								}
								if(thresh - (dd / dr) <
								    DBL_EPSILON) { /* do not rely on float equality,
									              check with DBL_EPSILON margin */
									n = passno + 1;
								}
							}
						}

						layer->numpasses = n - cblk->numpassesinlayers;

						if(!layer->numpasses) {
							layer->disto = 0;
							continue;
						}

						if(cblk->numpassesinlayers == 0) {
							layer->len = cblk->passes[n - 1].rate;
							layer->data = cblk->data;
							layer->disto = cblk->passes[n - 1].distortiondec;
						}
						else {
							layer->len = cblk->passes[n - 1].rate - cblk->passes[cblk->numpassesinlayers -
							    1].rate;
							layer->data = cblk->data + cblk->passes[cblk->numpassesinlayers - 1].rate;
							layer->disto = cblk->passes[n - 1].distortiondec -
							    cblk->passes[cblk->numpassesinlayers - 1].distortiondec;
						}

						tcd_tile->distolayer[layno] += layer->disto; /* fixed_quality */

						if(final) {
							cblk->numpassesinlayers = n;
						}
					}
				}
			}
		}
	}
}

void opj_tcd_makelayer_fixed(opj_tcd_t * tcd, OPJ_UINT32 layno, OPJ_UINT32 final)
{
	OPJ_UINT32 compno, resno, bandno, precno, cblkno;
	OPJ_INT32 value;                    /*, matrice[tcd_tcp->numlayers][tcd_tile->comps[0].numresolutions][3]; */
	OPJ_INT32 matrice[10][10][3];
	OPJ_UINT32 i, j, k;
	opj_cp_t * cp = tcd->cp;
	opj_tcd_tile_t * tcd_tile = tcd->tcd_image->tiles;
	opj_tcp_t * tcd_tcp = tcd->tcp;
	for(compno = 0; compno < tcd_tile->numcomps; compno++) {
		opj_tcd_tilecomp_t * tilec = &tcd_tile->comps[compno];
		for(i = 0; i < tcd_tcp->numlayers; i++) {
			for(j = 0; j < tilec->numresolutions; j++) {
				for(k = 0; k < 3; k++) {
					matrice[i][j][k] = (OPJ_INT32)((float)cp->m_specific_param.m_enc.m_matrice[i *
					    tilec->numresolutions * 3 + j * 3 + k] * (float)(tcd->image->comps[compno].prec / 16.0));
				}
			}
		}
		for(resno = 0; resno < tilec->numresolutions; resno++) {
			opj_tcd_resolution_t * res = &tilec->resolutions[resno];
			for(bandno = 0; bandno < res->numbands; bandno++) {
				opj_tcd_band_t * band = &res->bands[bandno];
				/* Skip empty bands */
				if(opj_tcd_is_band_empty(band)) {
					continue;
				}
				for(precno = 0; precno < res->pw * res->ph; precno++) {
					opj_tcd_precinct_t * prc = &band->precincts[precno];
					for(cblkno = 0; cblkno < prc->cw * prc->ch; cblkno++) {
						opj_tcd_cblk_enc_t * cblk = &prc->cblks.enc[cblkno];
						opj_tcd_layer_t * layer = &cblk->layers[layno];
						OPJ_UINT32 n;
						OPJ_INT32 imsb = (OPJ_INT32)(tcd->image->comps[compno].prec -
						    cblk->numbps); /* number of bit-plan equal to zero */

						/* Correction of the matrix of coefficient to include the IMSB
						   information */
						if(layno == 0) {
							value = matrice[layno][resno][bandno];
							if(imsb >= value) {
								value = 0;
							}
							else {
								value -= imsb;
							}
						}
						else {
							value = matrice[layno][resno][bandno] - matrice[layno - 1][resno][bandno];
							if(imsb >= matrice[layno - 1][resno][bandno]) {
								value -= (imsb - matrice[layno - 1][resno][bandno]);
								if(value < 0) {
									value = 0;
								}
							}
						}

						if(layno == 0) {
							cblk->numpassesinlayers = 0;
						}

						n = cblk->numpassesinlayers;
						if(cblk->numpassesinlayers == 0) {
							if(value != 0) {
								n = 3 * (OPJ_UINT32)value - 2 + cblk->numpassesinlayers;
							}
							else {
								n = cblk->numpassesinlayers;
							}
						}
						else {
							n = 3 * (OPJ_UINT32)value + cblk->numpassesinlayers;
						}
						layer->numpasses = n - cblk->numpassesinlayers;
						if(!layer->numpasses) {
							continue;
						}
						if(cblk->numpassesinlayers == 0) {
							layer->len = cblk->passes[n - 1].rate;
							layer->data = cblk->data;
						}
						else {
							layer->len = cblk->passes[n - 1].rate - cblk->passes[cblk->numpassesinlayers - 1].rate;
							layer->data = cblk->data + cblk->passes[cblk->numpassesinlayers - 1].rate;
						}
						if(final) {
							cblk->numpassesinlayers = n;
						}
					}
				}
			}
		}
	}
}

boolint opj_tcd_rateallocate(opj_tcd_t * tcd, uint8 * dest, OPJ_UINT32 * p_data_written,
    OPJ_UINT32 len, opj_codestream_info_t * cstr_info, opj_event_mgr_t * p_manager)
{
	OPJ_UINT32 compno, resno, bandno, precno, cblkno, layno;
	OPJ_UINT32 passno;
	double cumdisto[100]; /* fixed_quality */
	const double K = 1; /* 1.1; fixed_quality */
	double maxSE = 0;
	opj_cp_t * cp = tcd->cp;
	opj_tcd_tile_t * tcd_tile = tcd->tcd_image->tiles;
	opj_tcp_t * tcd_tcp = tcd->tcp;
	double min = DBL_MAX;
	double max = 0;
	tcd_tile->numpix = 0; /* fixed_quality */
	for(compno = 0; compno < tcd_tile->numcomps; compno++) {
		opj_tcd_tilecomp_t * tilec = &tcd_tile->comps[compno];
		tilec->numpix = 0;
		for(resno = 0; resno < tilec->numresolutions; resno++) {
			opj_tcd_resolution_t * res = &tilec->resolutions[resno];
			for(bandno = 0; bandno < res->numbands; bandno++) {
				opj_tcd_band_t * band = &res->bands[bandno];
				/* Skip empty bands */
				if(opj_tcd_is_band_empty(band)) {
					continue;
				}
				for(precno = 0; precno < res->pw * res->ph; precno++) {
					opj_tcd_precinct_t * prc = &band->precincts[precno];
					for(cblkno = 0; cblkno < prc->cw * prc->ch; cblkno++) {
						opj_tcd_cblk_enc_t * cblk = &prc->cblks.enc[cblkno];
						for(passno = 0; passno < cblk->totalpasses; passno++) {
							opj_tcd_pass_t * pass = &cblk->passes[passno];
							OPJ_INT32 dr;
							double dd, rdslope;
							if(passno == 0) {
								dr = (OPJ_INT32)pass->rate;
								dd = pass->distortiondec;
							}
							else {
								dr = (OPJ_INT32)(pass->rate - cblk->passes[passno - 1].rate);
								dd = pass->distortiondec - cblk->passes[passno - 1].distortiondec;
							}
							if(dr == 0) {
								continue;
							}
							rdslope = dd / dr;
							SETMIN(min, rdslope);
							SETMAX(max, rdslope);
						} /* passno */

						/* fixed_quality */
						tcd_tile->numpix += ((cblk->x1 - cblk->x0) * (cblk->y1 - cblk->y0));
						tilec->numpix += ((cblk->x1 - cblk->x0) * (cblk->y1 - cblk->y0));
					} /* cbklno */
				} /* precno */
			} /* bandno */
		} /* resno */
		maxSE += (((double)(1 << tcd->image->comps[compno].prec) - 1.0)
		    * ((double)(1 << tcd->image->comps[compno].prec) - 1.0))
		    * ((double)(tilec->numpix));
	} /* compno */
	/* index file */
	if(cstr_info) {
		opj_tile_info_t * tile_info = &cstr_info->tile[tcd->tcd_tileno];
		tile_info->numpix = tcd_tile->numpix;
		tile_info->distotile = tcd_tile->distotile;
		tile_info->thresh = (double *)SAlloc::M(tcd_tcp->numlayers * sizeof(double));
		if(!tile_info->thresh) {
			/* FIXME event manager error callback */
			return FALSE;
		}
	}
	for(layno = 0; layno < tcd_tcp->numlayers; layno++) {
		double lo = min;
		double hi = max;
		OPJ_UINT32 maxlen = tcd_tcp->rates[layno] > 0.0f ? smin(((OPJ_UINT32)ceil(tcd_tcp->rates[layno])), len) : len;
		double goodthresh = 0;
		double stable_thresh = 0;
		OPJ_UINT32 i;
		double distotarget; /* fixed_quality */
		/* fixed_quality */
		distotarget = tcd_tile->distotile - ((K * maxSE) / pow((float)10, tcd_tcp->distoratio[layno] / 10));
		/* Don't try to find an optimal threshold but rather take everything not included yet, if
		   -r xx,yy,zz,0   (disto_alloc == 1 and rates == 0)
		   -q xx,yy,zz,0   (fixed_quality == 1 and distoratio == 0)
		   ==> possible to have some lossy layers and the last layer for sure lossless */
		if(((cp->m_specific_param.m_enc.m_disto_alloc == 1) &&
		    (tcd_tcp->rates[layno] > 0.0f)) ||
		    ((cp->m_specific_param.m_enc.m_fixed_quality == 1) &&
		    (tcd_tcp->distoratio[layno] > 0.0))) {
			opj_t2_t* t2 = opj_t2_create(tcd->image, cp);
			double thresh = 0;

			if(t2 == 00) {
				return FALSE;
			}

			for(i = 0; i < 128; ++i) {
				double distoachieved = 0; /* fixed_quality */

				thresh = (lo + hi) / 2;

				opj_tcd_makelayer(tcd, layno, thresh, 0);

				if(cp->m_specific_param.m_enc.m_fixed_quality) { /* fixed_quality */
					if(OPJ_IS_CINEMA(cp->rsiz) || OPJ_IS_IMF(cp->rsiz)) {
						if(!opj_t2_encode_packets(t2, tcd->tcd_tileno, tcd_tile, layno + 1, dest,
						    p_data_written, maxlen, cstr_info, NULL, tcd->cur_tp_num, tcd->tp_pos,
						    tcd->cur_pino,
						    THRESH_CALC, p_manager)) {
							lo = thresh;
							continue;
						}
						else {
							distoachieved = layno == 0 ? tcd_tile->distolayer[0] : cumdisto[layno - 1] + tcd_tile->distolayer[layno];
							if(distoachieved < distotarget) {
								hi = thresh;
								stable_thresh = thresh;
								continue;
							}
							else {
								lo = thresh;
							}
						}
					}
					else {
						distoachieved = (layno == 0) ? tcd_tile->distolayer[0] : (cumdisto[layno - 1] + tcd_tile->distolayer[layno]);
						if(distoachieved < distotarget) {
							hi = thresh;
							stable_thresh = thresh;
							continue;
						}
						lo = thresh;
					}
				}
				else {
					if(!opj_t2_encode_packets(t2, tcd->tcd_tileno, tcd_tile, layno + 1, dest, p_data_written, maxlen, cstr_info, NULL, tcd->cur_tp_num, tcd->tp_pos,
					    tcd->cur_pino, THRESH_CALC, p_manager)) {
						/* TODO: what to do with l ??? seek / tell ??? */
						/* opj_event_msg(tcd->cinfo, EVT_INFO, "rate alloc: len=%d, max=%d\n",
						   l, maxlen); */
						lo = thresh;
						continue;
					}
					hi = thresh;
					stable_thresh = thresh;
				}
			}
			goodthresh = stable_thresh == 0 ? thresh : stable_thresh;
			opj_t2_destroy(t2);
		}
		else {
			/* Special value to indicate to use all passes */
			goodthresh = -1;
		}
		if(cstr_info) { /* Threshold for Marcela Index */
			cstr_info->tile[tcd->tcd_tileno].thresh[layno] = goodthresh;
		}
		opj_tcd_makelayer(tcd, layno, goodthresh, 1);
		/* fixed_quality */
		cumdisto[layno] = (layno == 0) ? tcd_tile->distolayer[0] : (cumdisto[layno - 1] + tcd_tile->distolayer[layno]);
	}
	return TRUE;
}

boolint opj_tcd_init(opj_tcd_t * p_tcd, opj_image_t * p_image, opj_cp_t * p_cp, opj_thread_pool_t* p_tp)
{
	p_tcd->image = p_image;
	p_tcd->cp = p_cp;
	p_tcd->tcd_image->tiles = (opj_tcd_tile_t*)SAlloc::C(1, sizeof(opj_tcd_tile_t));
	if(!p_tcd->tcd_image->tiles) {
		return FALSE;
	}
	p_tcd->tcd_image->tiles->comps = (opj_tcd_tilecomp_t*)SAlloc::C(p_image->numcomps, sizeof(opj_tcd_tilecomp_t));
	if(!p_tcd->tcd_image->tiles->comps) {
		return FALSE;
	}
	p_tcd->tcd_image->tiles->numcomps = p_image->numcomps;
	p_tcd->tp_pos = p_cp->m_specific_param.m_enc.m_tp_pos;
	p_tcd->thread_pool = p_tp;
	return TRUE;
}

/**
   Destroy a previously created TCD handle
 */
void opj_tcd_destroy(opj_tcd_t * tcd)
{
	if(tcd) {
		opj_tcd_free_tile(tcd);
		ZFREE(tcd->tcd_image);
		SAlloc::F(tcd->used_component);
		SAlloc::F(tcd);
	}
}

boolint opj_alloc_tile_component_data(opj_tcd_tilecomp_t * l_tilec)
{
	if((l_tilec->data == 00) || ((l_tilec->data_size_needed > l_tilec->data_size) && (l_tilec->ownsData == FALSE))) {
		l_tilec->data = (OPJ_INT32*)opj_image_data_alloc(l_tilec->data_size_needed);
		if(!l_tilec->data && l_tilec->data_size_needed != 0) {
			return FALSE;
		}
		/*slfprintf_stderr("tAllocate data of tilec (int): %d x OPJ_UINT32n",l_data_size);*/
		l_tilec->data_size = l_tilec->data_size_needed;
		l_tilec->ownsData = TRUE;
	}
	else if(l_tilec->data_size_needed > l_tilec->data_size) {
		/* We don't need to keep old data */
		opj_image_data_free(l_tilec->data);
		l_tilec->data = (OPJ_INT32*)opj_image_data_alloc(l_tilec->data_size_needed);
		if(!l_tilec->data) {
			l_tilec->data_size = 0;
			l_tilec->data_size_needed = 0;
			l_tilec->ownsData = FALSE;
			return FALSE;
		}
		/*slfprintf_stderr("tReallocate data of tilec (int): from %d to %d x OPJ_UINT32n", l_tilec->data_size,
		   l_data_size);*/
		l_tilec->data_size = l_tilec->data_size_needed;
		l_tilec->ownsData = TRUE;
	}
	return TRUE;
}

/* ----------------------------------------------------------------------- */

static INLINE boolint opj_tcd_init_tile(opj_tcd_t * p_tcd, OPJ_UINT32 p_tile_no,
    boolint isEncoder, OPJ_SIZE_T sizeof_block,
    opj_event_mgr_t* manager)
{
	OPJ_UINT32 compno, resno, bandno, precno, cblkno;
	opj_tcp_t * l_tcp = 00;
	opj_cp_t * l_cp = 00;
	opj_tcd_tile_t * l_tile = 00;
	opj_tccp_t * l_tccp = 00;
	opj_tcd_tilecomp_t * l_tilec = 00;
	opj_image_comp_t * l_image_comp = 00;
	opj_tcd_resolution_t * l_res = 00;
	opj_tcd_band_t * l_band = 00;
	opj_stepsize_t * l_step_size = 00;
	opj_tcd_precinct_t * l_current_precinct = 00;
	opj_image_t * l_image = 00;
	OPJ_UINT32 p, q;
	OPJ_UINT32 l_level_no;
	OPJ_UINT32 l_pdx, l_pdy;
	OPJ_INT32 l_x0b, l_y0b;
	OPJ_UINT32 l_tx0, l_ty0;
	/* extent of precincts , top left, bottom right**/
	OPJ_INT32 l_tl_prc_x_start, l_tl_prc_y_start, l_br_prc_x_end, l_br_prc_y_end;
	/* number of precinct for a resolution */
	OPJ_UINT32 l_nb_precincts;
	/* room needed to store l_nb_precinct precinct for a resolution */
	OPJ_UINT32 l_nb_precinct_size;
	/* number of code blocks for a precinct*/
	OPJ_UINT32 l_nb_code_blocks;
	/* room needed to store l_nb_code_blocks code blocks for a precinct*/
	OPJ_UINT32 l_nb_code_blocks_size;
	/* size of data for a tile */
	OPJ_UINT32 l_data_size;

	l_cp = p_tcd->cp;
	l_tcp = &(l_cp->tcps[p_tile_no]);
	l_tile = p_tcd->tcd_image->tiles;
	l_tccp = l_tcp->tccps;
	l_tilec = l_tile->comps;
	l_image = p_tcd->image;
	l_image_comp = p_tcd->image->comps;

	p = p_tile_no % l_cp->tw; /* tile coordinates */
	q = p_tile_no / l_cp->tw;
	/*slfprintf_stderr("Tile coordinate = %d,%d\n", p, q);*/

	/* 4 borders of the tile rescale on the image if necessary */
	l_tx0 = l_cp->tx0 + p *
	    l_cp->tdx; /* can't be greater than l_image->x1 so won't overflow */
	l_tile->x0 = (OPJ_INT32)smax(l_tx0, l_image->x0);
	l_tile->x1 = (OPJ_INT32)smin(opj_uint_adds(l_tx0, l_cp->tdx),
		l_image->x1);
	/* all those OPJ_UINT32 are casted to OPJ_INT32, let's do some sanity check */
	if((l_tile->x0 < 0) || (l_tile->x1 <= l_tile->x0)) {
		opj_event_msg(manager, EVT_ERROR, "Tile X coordinates are not supported\n");
		return FALSE;
	}
	l_ty0 = l_cp->ty0 + q *
	    l_cp->tdy; /* can't be greater than l_image->y1 so won't overflow */
	l_tile->y0 = (OPJ_INT32)smax(l_ty0, l_image->y0);
	l_tile->y1 = (OPJ_INT32)smin(opj_uint_adds(l_ty0, l_cp->tdy),
		l_image->y1);
	/* all those OPJ_UINT32 are casted to OPJ_INT32, let's do some sanity check */
	if((l_tile->y0 < 0) || (l_tile->y1 <= l_tile->y0)) {
		opj_event_msg(manager, EVT_ERROR, "Tile Y coordinates are not supported\n");
		return FALSE;
	}

	/* testcase 1888.pdf.asan.35.988 */
	if(l_tccp->numresolutions == 0) {
		opj_event_msg(manager, EVT_ERROR, "tiles require at least one resolution\n");
		return FALSE;
	}
	/*slfprintf_stderr("Tile border = %d,%d,%d,%d\n", l_tile->x0, l_tile->y0,l_tile->x1,l_tile->y1);*/

	/*tile->numcomps = image->numcomps; */
	for(compno = 0; compno < l_tile->numcomps; ++compno) {
		/*slfprintf_stderr("compno = %d/%d\n", compno, l_tile->numcomps);*/
		l_image_comp->resno_decoded = 0;
		/* border of each l_tile component (global) */
		l_tilec->x0 = opj_int_ceildiv(l_tile->x0, (OPJ_INT32)l_image_comp->dx);
		l_tilec->y0 = opj_int_ceildiv(l_tile->y0, (OPJ_INT32)l_image_comp->dy);
		l_tilec->x1 = opj_int_ceildiv(l_tile->x1, (OPJ_INT32)l_image_comp->dx);
		l_tilec->y1 = opj_int_ceildiv(l_tile->y1, (OPJ_INT32)l_image_comp->dy);
		l_tilec->compno = compno;
		/*slfprintf_stderr("\tTile compo border = %d,%d,%d,%d\n", l_tilec->x0,
		   l_tilec->y0,l_tilec->x1,l_tilec->y1);*/

		l_tilec->numresolutions = l_tccp->numresolutions;
		if(l_tccp->numresolutions < l_cp->m_specific_param.m_dec.m_reduce) {
			l_tilec->minimum_num_resolutions = 1;
		}
		else {
			l_tilec->minimum_num_resolutions = l_tccp->numresolutions -
			    l_cp->m_specific_param.m_dec.m_reduce;
		}

		if(isEncoder) {
			OPJ_SIZE_T l_tile_data_size;

			/* compute l_data_size with overflow check */
			OPJ_SIZE_T w = (OPJ_SIZE_T)(l_tilec->x1 - l_tilec->x0);
			OPJ_SIZE_T h = (OPJ_SIZE_T)(l_tilec->y1 - l_tilec->y0);

			/* issue 733, l_data_size == 0U, probably something wrong should be checked before getting here
			   */
			if(h > 0 && w > SIZE_MAX / h) {
				opj_event_msg(manager, EVT_ERROR, "Size of tile data exceeds system limits\n");
				return FALSE;
			}
			l_tile_data_size = w * h;

			if(SIZE_MAX / sizeof(OPJ_UINT32) < l_tile_data_size) {
				opj_event_msg(manager, EVT_ERROR, "Size of tile data exceeds system limits\n");
				return FALSE;
			}
			l_tile_data_size = l_tile_data_size * sizeof(OPJ_UINT32);
			l_tilec->data_size_needed = l_tile_data_size;
		}
		l_data_size = l_tilec->numresolutions * (OPJ_UINT32)sizeof(opj_tcd_resolution_t);
		opj_image_data_free(l_tilec->data_win);
		l_tilec->data_win = NULL;
		l_tilec->win_x0 = 0;
		l_tilec->win_y0 = 0;
		l_tilec->win_x1 = 0;
		l_tilec->win_y1 = 0;
		if(l_tilec->resolutions == 00) {
			l_tilec->resolutions = (opj_tcd_resolution_t*)SAlloc::M(l_data_size);
			if(!l_tilec->resolutions) {
				return FALSE;
			}
			/*slfprintf_stderr("\tAllocate resolutions of tilec (opj_tcd_resolution_t): %d\n",l_data_size);*/
			l_tilec->resolutions_size = l_data_size;
			memzero(l_tilec->resolutions, l_data_size);
		}
		else if(l_data_size > l_tilec->resolutions_size) {
			opj_tcd_resolution_t* new_resolutions = (opj_tcd_resolution_t*)SAlloc::R(l_tilec->resolutions, l_data_size);
			if(!new_resolutions) {
				opj_event_msg(manager, EVT_ERROR, "Not enough memory for tile resolutions\n");
				SAlloc::F(l_tilec->resolutions);
				l_tilec->resolutions = NULL;
				l_tilec->resolutions_size = 0;
				return FALSE;
			}
			l_tilec->resolutions = new_resolutions;
			/*slfprintf_stderr("\tReallocate data of tilec (int): from %d to %d x OPJ_UINT32\n",
			   l_tilec->resolutions_size, l_data_size);*/
			memzero(((uint8 *)l_tilec->resolutions) + l_tilec->resolutions_size, l_data_size - l_tilec->resolutions_size);
			l_tilec->resolutions_size = l_data_size;
		}

		l_level_no = l_tilec->numresolutions;
		l_res = l_tilec->resolutions;
		l_step_size = l_tccp->stepsizes;
		/*slfprintf_stderr("\tlevel_no=%d\n",l_level_no);*/

		for(resno = 0; resno < l_tilec->numresolutions; ++resno) {
			/*slfprintf_stderr("\t\tresno = %d/%d\n", resno, l_tilec->numresolutions);*/
			OPJ_INT32 tlcbgxstart, tlcbgystart /*, brcbgxend, brcbgyend*/;
			OPJ_UINT32 cbgwidthexpn, cbgheightexpn;
			OPJ_UINT32 cblkwidthexpn, cblkheightexpn;

			--l_level_no;

			/* border for each resolution level (global) */
			l_res->x0 = opj_int_ceildivpow2(l_tilec->x0, (OPJ_INT32)l_level_no);
			l_res->y0 = opj_int_ceildivpow2(l_tilec->y0, (OPJ_INT32)l_level_no);
			l_res->x1 = opj_int_ceildivpow2(l_tilec->x1, (OPJ_INT32)l_level_no);
			l_res->y1 = opj_int_ceildivpow2(l_tilec->y1, (OPJ_INT32)l_level_no);

			/*slfprintf_stderr("\t\t\tres_x0= %d, res_y0 =%d, res_x1=%d, res_y1=%d\n", l_res->x0, l_res->y0,
			   l_res->x1, l_res->y1);*/
			/* p. 35, table A-23, ISO/IEC FDIS154444-1 : 2000 (18 august 2000) */
			l_pdx = l_tccp->prcw[resno];
			l_pdy = l_tccp->prch[resno];
			/*slfprintf_stderr("\t\t\tpdx=%d, pdy=%d\n", l_pdx, l_pdy);*/
			/* p. 64, B.6, ISO/IEC FDIS15444-1 : 2000 (18 august 2000)  */
			l_tl_prc_x_start = opj_int_floordivpow2(l_res->x0, (OPJ_INT32)l_pdx) << l_pdx;
			l_tl_prc_y_start = opj_int_floordivpow2(l_res->y0, (OPJ_INT32)l_pdy) << l_pdy;
			{
				OPJ_UINT32 tmp = ((OPJ_UINT32)opj_int_ceildivpow2(l_res->x1,
				    (OPJ_INT32)l_pdx)) << l_pdx;
				if(tmp > (OPJ_UINT32)INT_MAX) {
					opj_event_msg(manager, EVT_ERROR, "Integer overflow\n");
					return FALSE;
				}
				l_br_prc_x_end = (OPJ_INT32)tmp;
			}
			{
				OPJ_UINT32 tmp = ((OPJ_UINT32)opj_int_ceildivpow2(l_res->y1,
				    (OPJ_INT32)l_pdy)) << l_pdy;
				if(tmp > (OPJ_UINT32)INT_MAX) {
					opj_event_msg(manager, EVT_ERROR, "Integer overflow\n");
					return FALSE;
				}
				l_br_prc_y_end = (OPJ_INT32)tmp;
			}
			/*slfprintf_stderr("\t\t\tprc_x_start=%d, prc_y_start=%d, br_prc_x_end=%d, br_prc_y_end=%d \n",
			   l_tl_prc_x_start, l_tl_prc_y_start, l_br_prc_x_end ,l_br_prc_y_end );*/

			l_res->pw = (l_res->x0 == l_res->x1) ? 0U : (OPJ_UINT32)((
				    l_br_prc_x_end - l_tl_prc_x_start) >> l_pdx);
			l_res->ph = (l_res->y0 == l_res->y1) ? 0U : (OPJ_UINT32)((
				    l_br_prc_y_end - l_tl_prc_y_start) >> l_pdy);
			/*slfprintf_stderr("\t\t\tres_pw=%d, res_ph=%d\n", l_res->pw, l_res->ph );*/

			if((l_res->pw != 0U) && ((((OPJ_UINT32)-1) / l_res->pw) < l_res->ph)) {
				opj_event_msg(manager, EVT_ERROR, "Size of tile data exceeds system limits\n");
				return FALSE;
			}
			l_nb_precincts = l_res->pw * l_res->ph;

			if((((OPJ_UINT32)-1) / (OPJ_UINT32)sizeof(opj_tcd_precinct_t)) <
			    l_nb_precincts) {
				opj_event_msg(manager, EVT_ERROR, "Size of tile data exceeds system limits\n");
				return FALSE;
			}
			l_nb_precinct_size = l_nb_precincts * (OPJ_UINT32)sizeof(opj_tcd_precinct_t);

			if(resno == 0) {
				tlcbgxstart = l_tl_prc_x_start;
				tlcbgystart = l_tl_prc_y_start;
				/*brcbgxend = l_br_prc_x_end;*/
				/* brcbgyend = l_br_prc_y_end;*/
				cbgwidthexpn = l_pdx;
				cbgheightexpn = l_pdy;
				l_res->numbands = 1;
			}
			else {
				tlcbgxstart = opj_int_ceildivpow2(l_tl_prc_x_start, 1);
				tlcbgystart = opj_int_ceildivpow2(l_tl_prc_y_start, 1);
				/*brcbgxend = opj_int_ceildivpow2(l_br_prc_x_end, 1);*/
				/*brcbgyend = opj_int_ceildivpow2(l_br_prc_y_end, 1);*/
				cbgwidthexpn = l_pdx - 1;
				cbgheightexpn = l_pdy - 1;
				l_res->numbands = 3;
			}

			cblkwidthexpn = smin(l_tccp->cblkw, cbgwidthexpn);
			cblkheightexpn = smin(l_tccp->cblkh, cbgheightexpn);
			l_band = l_res->bands;

			for(bandno = 0; bandno < l_res->numbands; ++bandno, ++l_band, ++l_step_size) {
				/*slfprintf_stderr("\t\t\tband_no=%d/%d\n", bandno, l_res->numbands );*/

				if(resno == 0) {
					l_band->bandno = 0;
					l_band->x0 = opj_int_ceildivpow2(l_tilec->x0, (OPJ_INT32)l_level_no);
					l_band->y0 = opj_int_ceildivpow2(l_tilec->y0, (OPJ_INT32)l_level_no);
					l_band->x1 = opj_int_ceildivpow2(l_tilec->x1, (OPJ_INT32)l_level_no);
					l_band->y1 = opj_int_ceildivpow2(l_tilec->y1, (OPJ_INT32)l_level_no);
				}
				else {
					l_band->bandno = bandno + 1;
					/* x0b = 1 if bandno = 1 or 3 */
					l_x0b = l_band->bandno & 1;
					/* y0b = 1 if bandno = 2 or 3 */
					l_y0b = (OPJ_INT32)((l_band->bandno) >> 1);
					/* l_band border (global) */
					l_band->x0 = opj_int64_ceildivpow2(l_tilec->x0 - ((OPJ_INT64)l_x0b <<
						l_level_no), (OPJ_INT32)(l_level_no + 1));
					l_band->y0 = opj_int64_ceildivpow2(l_tilec->y0 - ((OPJ_INT64)l_y0b <<
						l_level_no), (OPJ_INT32)(l_level_no + 1));
					l_band->x1 = opj_int64_ceildivpow2(l_tilec->x1 - ((OPJ_INT64)l_x0b <<
						l_level_no), (OPJ_INT32)(l_level_no + 1));
					l_band->y1 = opj_int64_ceildivpow2(l_tilec->y1 - ((OPJ_INT64)l_y0b <<
						l_level_no), (OPJ_INT32)(l_level_no + 1));
				}

				if(isEncoder) {
					/* Skip empty bands */
					if(opj_tcd_is_band_empty(l_band)) {
						/* Do not zero l_band->precints to avoid leaks */
						/* but make sure we don't use it later, since */
						/* it will point to precincts of previous bands... */
						continue;
					}
				}

				{
					/* Table E-1 - Sub-band gains */
					/* BUG_WEIRD_TWO_INVK (look for this identifier in dwt.c): */
					/* the test (!isEncoder && l_tccp->qmfbid == 0) is strongly */
					/* linked to the use of two_invK instead of invK */
					const OPJ_INT32 log2_gain = (!isEncoder &&
					    l_tccp->qmfbid == 0) ? 0 : (l_band->bandno == 0) ? 0 :
					    (l_band->bandno == 3) ? 2 : 1;

					/* Nominal dynamic range. Equation E-4 */
					const OPJ_INT32 Rb = (OPJ_INT32)l_image_comp->prec + log2_gain;

					/* Delta_b value of Equation E-3 in "E.1 Inverse quantization
					 * procedure" of the standard */
					l_band->stepsize = (float)(((1.0 + l_step_size->mant / 2048.0) * pow(2.0,
					    (OPJ_INT32)(Rb - l_step_size->expn))));
				}

				/* Mb value of Equation E-2 in "E.1 Inverse quantization
				 * procedure" of the standard */
				l_band->numbps = l_step_size->expn + (OPJ_INT32)l_tccp->numgbits - 1;
				if(!l_band->precincts && (l_nb_precincts > 0U)) {
					l_band->precincts = (opj_tcd_precinct_t*)SAlloc::M(/*3 * */l_nb_precinct_size);
					if(!l_band->precincts) {
						opj_event_msg(manager, EVT_ERROR,
						    "Not enough memory to handle band precints\n");
						return FALSE;
					}
					/*slfprintf_stderr("\t\t\t\tAllocate precincts of a band (opj_tcd_precinct_t):
					   %d\n",l_nb_precinct_size);     */
					memzero(l_band->precincts, l_nb_precinct_size);
					l_band->precincts_data_size = l_nb_precinct_size;
				}
				else if(l_band->precincts_data_size < l_nb_precinct_size) {
					opj_tcd_precinct_t * new_precincts = (opj_tcd_precinct_t*)SAlloc::R(
						l_band->precincts, /*3 * */ l_nb_precinct_size);
					if(!new_precincts) {
						opj_event_msg(manager, EVT_ERROR,
						    "Not enough memory to handle band precints\n");
						SAlloc::F(l_band->precincts);
						l_band->precincts = NULL;
						l_band->precincts_data_size = 0;
						return FALSE;
					}
					l_band->precincts = new_precincts;
					/*slfprintf_stderr("\t\t\t\tReallocate precincts of a band (opj_tcd_precinct_t):
					   from %d to %d\n",l_band->precincts_data_size, l_nb_precinct_size);*/
					memzero(((uint8 *)l_band->precincts) + l_band->precincts_data_size, l_nb_precinct_size - l_band->precincts_data_size);
					l_band->precincts_data_size = l_nb_precinct_size;
				}

				l_current_precinct = l_band->precincts;
				for(precno = 0; precno < l_nb_precincts; ++precno) {
					OPJ_INT32 tlcblkxstart, tlcblkystart, brcblkxend, brcblkyend;
					OPJ_INT32 cbgxstart = tlcbgxstart + (OPJ_INT32)(precno % l_res->pw) *
					    (1 << cbgwidthexpn);
					OPJ_INT32 cbgystart = tlcbgystart + (OPJ_INT32)(precno / l_res->pw) *
					    (1 << cbgheightexpn);
					OPJ_INT32 cbgxend = cbgxstart + (1 << cbgwidthexpn);
					OPJ_INT32 cbgyend = cbgystart + (1 << cbgheightexpn);
					/*slfprintf_stderr("\t precno=%d; bandno=%d, resno=%d; compno=%d\n", precno,
					   bandno , resno, compno);*/
					/*slfprintf_stderr("\t tlcbgxstart(=%d) + (precno(=%d) percent res->pw(=%d)) * (1
					   << cbgwidthexpn(=%d)) \n",tlcbgxstart,precno,l_res->pw,cbgwidthexpn);*/

					/* precinct size (global) */
					/*slfprintf_stderr("\t cbgxstart=%d, l_band->x0 = %d \n",cbgxstart,
					   l_band->x0);*/

					l_current_precinct->x0 = smax(cbgxstart, l_band->x0);
					l_current_precinct->y0 = smax(cbgystart, l_band->y0);
					l_current_precinct->x1 = smin(cbgxend, l_band->x1);
					l_current_precinct->y1 = smin(cbgyend, l_band->y1);
					/*slfprintf_stderr("\t prc_x0=%d; prc_y0=%d, prc_x1=%d;
					   prc_y1=%d\n",l_current_precinct->x0, l_current_precinct->y0
					   ,l_current_precinct->x1, l_current_precinct->y1);*/

					tlcblkxstart = opj_int_floordivpow2(l_current_precinct->x0,
						(OPJ_INT32)cblkwidthexpn) << cblkwidthexpn;
					/*slfprintf_stderr("\t tlcblkxstart =%d\n",tlcblkxstart );*/
					tlcblkystart = opj_int_floordivpow2(l_current_precinct->y0,
						(OPJ_INT32)cblkheightexpn) << cblkheightexpn;
					/*slfprintf_stderr("\t tlcblkystart =%d\n",tlcblkystart );*/
					brcblkxend = opj_int_ceildivpow2(l_current_precinct->x1,
						(OPJ_INT32)cblkwidthexpn) << cblkwidthexpn;
					/*slfprintf_stderr("\t brcblkxend =%d\n",brcblkxend );*/
					brcblkyend = opj_int_ceildivpow2(l_current_precinct->y1,
						(OPJ_INT32)cblkheightexpn) << cblkheightexpn;
					/*slfprintf_stderr("\t brcblkyend =%d\n",brcblkyend );*/
					l_current_precinct->cw = (OPJ_UINT32)((brcblkxend - tlcblkxstart) >>
					    cblkwidthexpn);
					l_current_precinct->ch = (OPJ_UINT32)((brcblkyend - tlcblkystart) >>
					    cblkheightexpn);

					l_nb_code_blocks = l_current_precinct->cw * l_current_precinct->ch;
					/*slfprintf_stderr("\t\t\t\t precinct_cw = %d x recinct_ch =
					   %d\n",l_current_precinct->cw, l_current_precinct->ch);      */
					if((((OPJ_UINT32)-1) / (OPJ_UINT32)sizeof_block) <
					    l_nb_code_blocks) {
						opj_event_msg(manager, EVT_ERROR,
						    "Size of code block data exceeds system limits\n");
						return FALSE;
					}
					l_nb_code_blocks_size = l_nb_code_blocks * (OPJ_UINT32)sizeof_block;

					if(!l_current_precinct->cblks.blocks && (l_nb_code_blocks > 0U)) {
						l_current_precinct->cblks.blocks = SAlloc::M(l_nb_code_blocks_size);
						if(!l_current_precinct->cblks.blocks) {
							return FALSE;
						}
						/*slfprintf_stderr("\t\t\t\tAllocate cblks of a precinct
						   (opj_tcd_cblk_dec_t): %d\n",l_nb_code_blocks_size);*/
						memzero(l_current_precinct->cblks.blocks, l_nb_code_blocks_size);
						l_current_precinct->block_size = l_nb_code_blocks_size;
					}
					else if(l_nb_code_blocks_size > l_current_precinct->block_size) {
						void * new_blocks = SAlloc::R(l_current_precinct->cblks.blocks, l_nb_code_blocks_size);
						if(!new_blocks) {
							SAlloc::F(l_current_precinct->cblks.blocks);
							l_current_precinct->cblks.blocks = NULL;
							l_current_precinct->block_size = 0;
							opj_event_msg(manager, EVT_ERROR,
							    "Not enough memory for current precinct codeblock element\n");
							return FALSE;
						}
						l_current_precinct->cblks.blocks = new_blocks;
						/*slfprintf_stderr("\t\t\t\tReallocate cblks of a precinct
						   (opj_tcd_cblk_dec_t): from %d to
						   %d\n",l_current_precinct->block_size, l_nb_code_blocks_size);     */
						memzero(((uint8 *)l_current_precinct->cblks.blocks) + l_current_precinct->block_size, l_nb_code_blocks_size - l_current_precinct->block_size);
						l_current_precinct->block_size = l_nb_code_blocks_size;
					}

					if(!l_current_precinct->incltree) {
						l_current_precinct->incltree = opj_tgt_create(l_current_precinct->cw,
							l_current_precinct->ch, manager);
					}
					else {
						l_current_precinct->incltree = opj_tgt_init(l_current_precinct->incltree,
							l_current_precinct->cw, l_current_precinct->ch, manager);
					}

					if(!l_current_precinct->imsbtree) {
						l_current_precinct->imsbtree = opj_tgt_create(l_current_precinct->cw,
							l_current_precinct->ch, manager);
					}
					else {
						l_current_precinct->imsbtree = opj_tgt_init(l_current_precinct->imsbtree,
							l_current_precinct->cw, l_current_precinct->ch, manager);
					}

					for(cblkno = 0; cblkno < l_nb_code_blocks; ++cblkno) {
						OPJ_INT32 cblkxstart = tlcblkxstart + (OPJ_INT32)(cblkno %
						    l_current_precinct->cw) * (1 << cblkwidthexpn);
						OPJ_INT32 cblkystart = tlcblkystart + (OPJ_INT32)(cblkno /
						    l_current_precinct->cw) * (1 << cblkheightexpn);
						OPJ_INT32 cblkxend = cblkxstart + (1 << cblkwidthexpn);
						OPJ_INT32 cblkyend = cblkystart + (1 << cblkheightexpn);

						if(isEncoder) {
							opj_tcd_cblk_enc_t* l_code_block = l_current_precinct->cblks.enc + cblkno;

							if(!opj_tcd_code_block_enc_allocate(l_code_block)) {
								return FALSE;
							}
							/* code-block size (global) */
							l_code_block->x0 = smax(cblkxstart, l_current_precinct->x0);
							l_code_block->y0 = smax(cblkystart, l_current_precinct->y0);
							l_code_block->x1 = smin(cblkxend, l_current_precinct->x1);
							l_code_block->y1 = smin(cblkyend, l_current_precinct->y1);

							if(!opj_tcd_code_block_enc_allocate_data(l_code_block)) {
								return FALSE;
							}
						}
						else {
							opj_tcd_cblk_dec_t* l_code_block = l_current_precinct->cblks.dec + cblkno;

							if(!opj_tcd_code_block_dec_allocate(l_code_block)) {
								return FALSE;
							}
							/* code-block size (global) */
							l_code_block->x0 = smax(cblkxstart, l_current_precinct->x0);
							l_code_block->y0 = smax(cblkystart, l_current_precinct->y0);
							l_code_block->x1 = smin(cblkxend, l_current_precinct->x1);
							l_code_block->y1 = smin(cblkyend, l_current_precinct->y1);
						}
					}
					++l_current_precinct;
				} /* precno */
			} /* bandno */
			++l_res;
		} /* resno */
		++l_tccp;
		++l_tilec;
		++l_image_comp;
	} /* compno */
	return TRUE;
}

boolint opj_tcd_init_encode_tile(opj_tcd_t * p_tcd, OPJ_UINT32 p_tile_no,
    opj_event_mgr_t* p_manager)
{
	return opj_tcd_init_tile(p_tcd, p_tile_no, TRUE,
		   sizeof(opj_tcd_cblk_enc_t), p_manager);
}

boolint opj_tcd_init_decode_tile(opj_tcd_t * p_tcd, OPJ_UINT32 p_tile_no, opj_event_mgr_t* p_manager)
{
	return opj_tcd_init_tile(p_tcd, p_tile_no, FALSE, sizeof(opj_tcd_cblk_dec_t), p_manager);
}
/**
 * Allocates memory for an encoding code block (but not data memory).
 */
static boolint opj_tcd_code_block_enc_allocate(opj_tcd_cblk_enc_t * p_code_block)
{
	if(!p_code_block->layers) {
		/* no memset since data */
		p_code_block->layers = (opj_tcd_layer_t*)SAlloc::C(100, sizeof(opj_tcd_layer_t));
		if(!p_code_block->layers) {
			return FALSE;
		}
	}
	if(!p_code_block->passes) {
		p_code_block->passes = (opj_tcd_pass_t*)SAlloc::C(100, sizeof(opj_tcd_pass_t));
		if(!p_code_block->passes) {
			return FALSE;
		}
	}
	return TRUE;
}

/**
 * Allocates data memory for an encoding code block.
 */
static boolint opj_tcd_code_block_enc_allocate_data(opj_tcd_cblk_enc_t *
    p_code_block)
{
	OPJ_UINT32 l_data_size;

	/* +1 is needed for https://github.com/uclouvain/openjpeg/issues/835 */
	/* and actually +2 required for https://github.com/uclouvain/openjpeg/issues/982 */
	/* and +7 for https://github.com/uclouvain/openjpeg/issues/1283 (-M 3) */
	/* and +26 for https://github.com/uclouvain/openjpeg/issues/1283 (-M 7) */
	/* and +28 for https://github.com/uclouvain/openjpeg/issues/1283 (-M 44) */
	/* and +33 for https://github.com/uclouvain/openjpeg/issues/1283 (-M 4) */
	/* and +63 for https://github.com/uclouvain/openjpeg/issues/1283 (-M 4 -IMF 2K) */
	/* and +74 for https://github.com/uclouvain/openjpeg/issues/1283 (-M 4 -n 8 -s 7,7 -I) */
	/* TODO: is there a theoretical upper-bound for the compressed code */
	/* block size ? */
	l_data_size = 74 + (OPJ_UINT32)((p_code_block->x1 - p_code_block->x0) *
	    (p_code_block->y1 - p_code_block->y0) * (OPJ_INT32)sizeof(OPJ_UINT32));

	if(l_data_size > p_code_block->data_size) {
		if(p_code_block->data) {
			/* We refer to data - 1 since below we incremented it */
			SAlloc::F(p_code_block->data - 1);
		}
		p_code_block->data = (uint8 *)SAlloc::M(l_data_size + 1);
		if(!p_code_block->data) {
			p_code_block->data_size = 0U;
			return FALSE;
		}
		p_code_block->data_size = l_data_size;

		/* We reserve the initial byte as a fake byte to a non-FF value */
		/* and increment the data pointer, so that opj_mqc_init_enc() */
		/* can do bp = data - 1, and opj_mqc_byteout() can safely dereference */
		/* it. */
		p_code_block->data[0] = 0;
		p_code_block->data += 1; /*why +1 ?*/
	}
	return TRUE;
}

void opj_tcd_reinit_segment(opj_tcd_seg_t* seg)
{
	memzero(seg, sizeof(opj_tcd_seg_t));
}
/**
 * Allocates memory for a decoding code block.
 */
static boolint opj_tcd_code_block_dec_allocate(opj_tcd_cblk_dec_t *
    p_code_block)
{
	if(!p_code_block->segs) {
		p_code_block->segs = (opj_tcd_seg_t*)SAlloc::C(OPJ_J2K_DEFAULT_NB_SEGS,
			sizeof(opj_tcd_seg_t));
		if(!p_code_block->segs) {
			return FALSE;
		}
		/*slfprintf_stderr("Allocate %d elements of code_block->data\n", OPJ_J2K_DEFAULT_NB_SEGS *
		   sizeof(opj_tcd_seg_t));*/

		p_code_block->m_current_max_segs = OPJ_J2K_DEFAULT_NB_SEGS;
		/*slfprintf_stderr("m_current_max_segs of code_block->data = %d\n", p_code_block->m_current_max_segs);*/
	}
	else {
		/* sanitize */
		opj_tcd_seg_t * l_segs = p_code_block->segs;
		OPJ_UINT32 l_current_max_segs = p_code_block->m_current_max_segs;
		opj_tcd_seg_data_chunk_t* l_chunks = p_code_block->chunks;
		OPJ_UINT32 l_numchunksalloc = p_code_block->numchunksalloc;
		OPJ_UINT32 i;
		opj_aligned_free(p_code_block->decoded_data);
		p_code_block->decoded_data = 00;
		memzero(p_code_block, sizeof(opj_tcd_cblk_dec_t));
		p_code_block->segs = l_segs;
		p_code_block->m_current_max_segs = l_current_max_segs;
		for(i = 0; i < l_current_max_segs; ++i) {
			opj_tcd_reinit_segment(&l_segs[i]);
		}
		p_code_block->chunks = l_chunks;
		p_code_block->numchunksalloc = l_numchunksalloc;
	}

	return TRUE;
}

OPJ_UINT32 opj_tcd_get_decoded_tile_size(opj_tcd_t * p_tcd,
    boolint take_into_account_partial_decoding)
{
	OPJ_UINT32 i;
	OPJ_UINT32 l_data_size = 0;
	opj_image_comp_t * l_img_comp = 00;
	opj_tcd_tilecomp_t * l_tile_comp = 00;
	opj_tcd_resolution_t * l_res = 00;
	OPJ_UINT32 l_size_comp, l_remaining;
	OPJ_UINT32 l_temp;

	l_tile_comp = p_tcd->tcd_image->tiles->comps;
	l_img_comp = p_tcd->image->comps;

	for(i = 0; i < p_tcd->image->numcomps; ++i) {
		OPJ_UINT32 w, h;
		l_size_comp = l_img_comp->prec >> 3; /*(/ 8)*/
		l_remaining = l_img_comp->prec & 7; /* (%8) */

		if(l_remaining) {
			++l_size_comp;
		}

		if(l_size_comp == 3) {
			l_size_comp = 4;
		}

		l_res = l_tile_comp->resolutions + l_tile_comp->minimum_num_resolutions - 1;
		if(take_into_account_partial_decoding && !p_tcd->whole_tile_decoding) {
			w = l_res->win_x1 - l_res->win_x0;
			h = l_res->win_y1 - l_res->win_y0;
		}
		else {
			w = (OPJ_UINT32)(l_res->x1 - l_res->x0);
			h = (OPJ_UINT32)(l_res->y1 - l_res->y0);
		}
		if(h > 0 && UINT_MAX / w < h) {
			return UINT_MAX;
		}
		l_temp = w * h;
		if(l_size_comp && UINT_MAX / l_size_comp < l_temp) {
			return UINT_MAX;
		}
		l_temp *= l_size_comp;

		if(l_temp > UINT_MAX - l_data_size) {
			return UINT_MAX;
		}
		l_data_size += l_temp;
		++l_img_comp;
		++l_tile_comp;
	}

	return l_data_size;
}

boolint opj_tcd_encode_tile(opj_tcd_t * p_tcd,
    OPJ_UINT32 p_tile_no,
    uint8 * p_dest,
    OPJ_UINT32 * p_data_written,
    OPJ_UINT32 p_max_length,
    opj_codestream_info_t * p_cstr_info,
    opj_tcd_marker_info_t* p_marker_info,
    opj_event_mgr_t * p_manager)
{
	if(p_tcd->cur_tp_num == 0) {
		p_tcd->tcd_tileno = p_tile_no;
		p_tcd->tcp = &p_tcd->cp->tcps[p_tile_no];

		/* INDEX >> "Precinct_nb_X et Precinct_nb_Y" */
		if(p_cstr_info) {
			OPJ_UINT32 l_num_packs = 0;
			OPJ_UINT32 i;
			opj_tcd_tilecomp_t * l_tilec_idx =
			    &p_tcd->tcd_image->tiles->comps[0]; /* based on component 0 */
			opj_tccp_t * l_tccp = p_tcd->tcp->tccps; /* based on component 0 */

			for(i = 0; i < l_tilec_idx->numresolutions; i++) {
				opj_tcd_resolution_t * l_res_idx = &l_tilec_idx->resolutions[i];

				p_cstr_info->tile[p_tile_no].pw[i] = (int)l_res_idx->pw;
				p_cstr_info->tile[p_tile_no].ph[i] = (int)l_res_idx->ph;

				l_num_packs += l_res_idx->pw * l_res_idx->ph;
				p_cstr_info->tile[p_tile_no].pdx[i] = (int)l_tccp->prcw[i];
				p_cstr_info->tile[p_tile_no].pdy[i] = (int)l_tccp->prch[i];
			}
			p_cstr_info->tile[p_tile_no].packet = (opj_packet_info_t*)SAlloc::C((
					OPJ_SIZE_T)p_cstr_info->numcomps * (OPJ_SIZE_T)p_cstr_info->numlayers *
				l_num_packs,
				sizeof(opj_packet_info_t));
			if(!p_cstr_info->tile[p_tile_no].packet) {
				/* FIXME event manager error callback */
				return FALSE;
			}
		}
		/* << INDEX */

		/* FIXME _ProfStart(PGROUP_DC_SHIFT); */
		/*---------------TILE-------------------*/
		if(!opj_tcd_dc_level_shift_encode(p_tcd)) {
			return FALSE;
		}
		/* FIXME _ProfStop(PGROUP_DC_SHIFT); */

		/* FIXME _ProfStart(PGROUP_MCT); */
		if(!opj_tcd_mct_encode(p_tcd)) {
			return FALSE;
		}
		/* FIXME _ProfStop(PGROUP_MCT); */

		/* FIXME _ProfStart(PGROUP_DWT); */
		if(!opj_tcd_dwt_encode(p_tcd)) {
			return FALSE;
		}
		/* FIXME  _ProfStop(PGROUP_DWT); */

		/* FIXME  _ProfStart(PGROUP_T1); */
		if(!opj_tcd_t1_encode(p_tcd)) {
			return FALSE;
		}
		/* FIXME _ProfStop(PGROUP_T1); */

		/* FIXME _ProfStart(PGROUP_RATE); */
		if(!opj_tcd_rate_allocate_encode(p_tcd, p_dest, p_max_length,
		    p_cstr_info, p_manager)) {
			return FALSE;
		}
		/* FIXME _ProfStop(PGROUP_RATE); */
	}
	/*--------------TIER2------------------*/

	/* INDEX */
	if(p_cstr_info) {
		p_cstr_info->index_write = 1;
	}
	/* FIXME _ProfStart(PGROUP_T2); */

	if(!opj_tcd_t2_encode(p_tcd, p_dest, p_data_written, p_max_length,
	    p_cstr_info, p_marker_info, p_manager)) {
		return FALSE;
	}
	/* FIXME _ProfStop(PGROUP_T2); */

	/*---------------CLEAN-------------------*/

	return TRUE;
}

boolint opj_tcd_decode_tile(opj_tcd_t * p_tcd,
    OPJ_UINT32 win_x0,
    OPJ_UINT32 win_y0,
    OPJ_UINT32 win_x1,
    OPJ_UINT32 win_y1,
    OPJ_UINT32 numcomps_to_decode,
    const OPJ_UINT32 * comps_indices,
    uint8 * p_src,
    OPJ_UINT32 p_max_length,
    OPJ_UINT32 p_tile_no,
    opj_codestream_index_t * p_cstr_index,
    opj_event_mgr_t * p_manager
    )
{
	OPJ_UINT32 l_data_read;
	OPJ_UINT32 compno;

	p_tcd->tcd_tileno = p_tile_no;
	p_tcd->tcp = &(p_tcd->cp->tcps[p_tile_no]);
	p_tcd->win_x0 = win_x0;
	p_tcd->win_y0 = win_y0;
	p_tcd->win_x1 = win_x1;
	p_tcd->win_y1 = win_y1;
	p_tcd->whole_tile_decoding = TRUE;

	SAlloc::F(p_tcd->used_component);
	p_tcd->used_component = NULL;

	if(numcomps_to_decode) {
		boolint* used_component = (boolint*)SAlloc::C(sizeof(boolint),
			p_tcd->image->numcomps);
		if(used_component == NULL) {
			return FALSE;
		}
		for(compno = 0; compno < numcomps_to_decode; compno++) {
			used_component[ comps_indices[compno] ] = TRUE;
		}

		p_tcd->used_component = used_component;
	}

	for(compno = 0; compno < p_tcd->image->numcomps; compno++) {
		if(p_tcd->used_component != NULL && !p_tcd->used_component[compno]) {
			continue;
		}

		if(!opj_tcd_is_whole_tilecomp_decoding(p_tcd, compno)) {
			p_tcd->whole_tile_decoding = FALSE;
			break;
		}
	}

	if(p_tcd->whole_tile_decoding) {
		for(compno = 0; compno < p_tcd->image->numcomps; compno++) {
			opj_tcd_tilecomp_t* tilec = &(p_tcd->tcd_image->tiles->comps[compno]);
			opj_tcd_resolution_t * l_res = &(tilec->resolutions[tilec->minimum_num_resolutions - 1]);
			OPJ_SIZE_T l_data_size;
			/* compute l_data_size with overflow check */
			OPJ_SIZE_T res_w = (OPJ_SIZE_T)(l_res->x1 - l_res->x0);
			OPJ_SIZE_T res_h = (OPJ_SIZE_T)(l_res->y1 - l_res->y0);
			if(p_tcd->used_component != NULL && !p_tcd->used_component[compno]) {
				continue;
			}
			/* issue 733, l_data_size == 0U, probably something wrong should be checked before getting here
			   */
			if(res_h > 0 && res_w > SIZE_MAX / res_h) {
				opj_event_msg(p_manager, EVT_ERROR, "Size of tile data exceeds system limits\n");
				return FALSE;
			}
			l_data_size = res_w * res_h;
			if(SIZE_MAX / sizeof(OPJ_UINT32) < l_data_size) {
				opj_event_msg(p_manager, EVT_ERROR, "Size of tile data exceeds system limits\n");
				return FALSE;
			}
			l_data_size *= sizeof(OPJ_UINT32);
			tilec->data_size_needed = l_data_size;
			if(!opj_alloc_tile_component_data(tilec)) {
				opj_event_msg(p_manager, EVT_ERROR, "Size of tile data exceeds system limits\n");
				return FALSE;
			}
		}
	}
	else {
		/* Compute restricted tile-component and tile-resolution coordinates */
		/* of the window of interest, but defer the memory allocation until */
		/* we know the resno_decoded */
		for(compno = 0; compno < p_tcd->image->numcomps; compno++) {
			OPJ_UINT32 resno;
			opj_tcd_tilecomp_t* tilec = &(p_tcd->tcd_image->tiles->comps[compno]);
			opj_image_comp_t* image_comp = &(p_tcd->image->comps[compno]);

			if(p_tcd->used_component != NULL && !p_tcd->used_component[compno]) {
				continue;
			}

			/* Compute the intersection of the area of interest, expressed in tile coordinates */
			/* with the tile coordinates */
			tilec->win_x0 = smax(
				(OPJ_UINT32)tilec->x0,
				opj_uint_ceildiv(p_tcd->win_x0, image_comp->dx));
			tilec->win_y0 = smax(
				(OPJ_UINT32)tilec->y0,
				opj_uint_ceildiv(p_tcd->win_y0, image_comp->dy));
			tilec->win_x1 = smin(
				(OPJ_UINT32)tilec->x1,
				opj_uint_ceildiv(p_tcd->win_x1, image_comp->dx));
			tilec->win_y1 = smin(
				(OPJ_UINT32)tilec->y1,
				opj_uint_ceildiv(p_tcd->win_y1, image_comp->dy));
			if(tilec->win_x1 < tilec->win_x0 ||
			    tilec->win_y1 < tilec->win_y0) {
				/* We should not normally go there. The circumstance is when */
				/* the tile coordinates do not intersect the area of interest */
				/* Upper level logic should not even try to decode that tile */
				opj_event_msg(p_manager, EVT_ERROR, "Invalid tilec->win_xxx values\n");
				return FALSE;
			}
			for(resno = 0; resno < tilec->numresolutions; ++resno) {
				opj_tcd_resolution_t * res = tilec->resolutions + resno;
				res->win_x0 = opj_uint_ceildivpow2(tilec->win_x0, tilec->numresolutions - 1 - resno);
				res->win_y0 = opj_uint_ceildivpow2(tilec->win_y0, tilec->numresolutions - 1 - resno);
				res->win_x1 = opj_uint_ceildivpow2(tilec->win_x1, tilec->numresolutions - 1 - resno);
				res->win_y1 = opj_uint_ceildivpow2(tilec->win_y1, tilec->numresolutions - 1 - resno);
			}
		}
	}

#ifdef TODO_MSD /* FIXME */
	/* INDEX >>  */
	if(p_cstr_info) {
		OPJ_UINT32 resno, compno, numprec = 0;
		for(compno = 0; compno < (OPJ_UINT32)p_cstr_info->numcomps; compno++) {
			opj_tcp_t * tcp = &p_tcd->cp->tcps[0];
			opj_tccp_t * tccp = &tcp->tccps[compno];
			opj_tcd_tilecomp_t * tilec_idx = &p_tcd->tcd_image->tiles->comps[compno];
			for(resno = 0; resno < tilec_idx->numresolutions; resno++) {
				opj_tcd_resolution_t * res_idx = &tilec_idx->resolutions[resno];
				p_cstr_info->tile[p_tile_no].pw[resno] = res_idx->pw;
				p_cstr_info->tile[p_tile_no].ph[resno] = res_idx->ph;
				numprec += res_idx->pw * res_idx->ph;
				p_cstr_info->tile[p_tile_no].pdx[resno] = tccp->prcw[resno];
				p_cstr_info->tile[p_tile_no].pdy[resno] = tccp->prch[resno];
			}
		}
		p_cstr_info->tile[p_tile_no].packet = (opj_packet_info_t*)SAlloc::M(p_cstr_info->numlayers * numprec * sizeof(opj_packet_info_t));
		p_cstr_info->packno = 0;
	}
	/* << INDEX */
#endif

	/*--------------TIER2------------------*/
	/* FIXME _ProfStart(PGROUP_T2); */
	l_data_read = 0;
	if(!opj_tcd_t2_decode(p_tcd, p_src, &l_data_read, p_max_length, p_cstr_index,
	    p_manager)) {
		return FALSE;
	}
	/* FIXME _ProfStop(PGROUP_T2); */

	/*------------------TIER1-----------------*/

	/* FIXME _ProfStart(PGROUP_T1); */
	if(!opj_tcd_t1_decode(p_tcd, p_manager)) {
		return FALSE;
	}
	/* FIXME _ProfStop(PGROUP_T1); */

	/* For subtile decoding, now we know the resno_decoded, we can allocate */
	/* the tile data buffer */
	if(!p_tcd->whole_tile_decoding) {
		for(compno = 0; compno < p_tcd->image->numcomps; compno++) {
			opj_tcd_tilecomp_t* tilec = &(p_tcd->tcd_image->tiles->comps[compno]);
			opj_image_comp_t* image_comp = &(p_tcd->image->comps[compno]);
			opj_tcd_resolution_t * res = tilec->resolutions + image_comp->resno_decoded;
			OPJ_SIZE_T w = res->win_x1 - res->win_x0;
			OPJ_SIZE_T h = res->win_y1 - res->win_y0;
			OPJ_SIZE_T l_data_size;
			opj_image_data_free(tilec->data_win);
			tilec->data_win = NULL;
			if(p_tcd->used_component != NULL && !p_tcd->used_component[compno]) {
				continue;
			}
			if(w > 0 && h > 0) {
				if(w > SIZE_MAX / h) {
					opj_event_msg(p_manager, EVT_ERROR, "Size of tile data exceeds system limits\n");
					return FALSE;
				}
				l_data_size = w * h;
				if(l_data_size > SIZE_MAX / sizeof(OPJ_INT32)) {
					opj_event_msg(p_manager, EVT_ERROR, "Size of tile data exceeds system limits\n");
					return FALSE;
				}
				l_data_size *= sizeof(OPJ_INT32);
				tilec->data_win = (OPJ_INT32*)opj_image_data_alloc(l_data_size);
				if(tilec->data_win == NULL) {
					opj_event_msg(p_manager, EVT_ERROR, "Size of tile data exceeds system limits\n");
					return FALSE;
				}
			}
		}
	}
	/*----------------DWT---------------------*/

	/* FIXME _ProfStart(PGROUP_DWT); */
	if(!opj_tcd_dwt_decode(p_tcd)) {
		return FALSE;
	}
	/* FIXME _ProfStop(PGROUP_DWT); */

	/*----------------MCT-------------------*/
	/* FIXME _ProfStart(PGROUP_MCT); */
	if(!opj_tcd_mct_decode(p_tcd, p_manager)) {
		return FALSE;
	}
	/* FIXME _ProfStop(PGROUP_MCT); */

	/* FIXME _ProfStart(PGROUP_DC_SHIFT); */
	if(!opj_tcd_dc_level_shift_decode(p_tcd)) {
		return FALSE;
	}
	/* FIXME _ProfStop(PGROUP_DC_SHIFT); */

	/*---------------TILE-------------------*/
	return TRUE;
}

boolint opj_tcd_update_tile_data(opj_tcd_t * p_tcd, uint8 * p_dest, OPJ_UINT32 p_dest_length)
{
	OPJ_UINT32 i, j, k;
	opj_image_comp_t * l_img_comp = 00;
	opj_tcd_tilecomp_t * l_tilec = 00;
	opj_tcd_resolution_t * l_res;
	OPJ_UINT32 l_size_comp, l_remaining;
	OPJ_UINT32 l_stride, l_width, l_height;
	OPJ_UINT32 l_data_size = opj_tcd_get_decoded_tile_size(p_tcd, TRUE);
	if(l_data_size == UINT_MAX || l_data_size > p_dest_length) {
		return FALSE;
	}
	l_tilec = p_tcd->tcd_image->tiles->comps;
	l_img_comp = p_tcd->image->comps;
	for(i = 0; i < p_tcd->image->numcomps; ++i) {
		const OPJ_INT32* l_src_data;
		l_size_comp = l_img_comp->prec >> 3; /*(/ 8)*/
		l_remaining = l_img_comp->prec & 7; /* (%8) */
		l_res = l_tilec->resolutions + l_img_comp->resno_decoded;
		if(p_tcd->whole_tile_decoding) {
			l_width = (OPJ_UINT32)(l_res->x1 - l_res->x0);
			l_height = (OPJ_UINT32)(l_res->y1 - l_res->y0);
			l_stride = (OPJ_UINT32)(l_tilec->resolutions[l_tilec->minimum_num_resolutions - 1].x1 - l_tilec->resolutions[l_tilec->minimum_num_resolutions - 1].x0) - l_width;
			l_src_data = l_tilec->data;
		}
		else {
			l_width = l_res->win_x1 - l_res->win_x0;
			l_height = l_res->win_y1 - l_res->win_y0;
			l_stride = 0;
			l_src_data = l_tilec->data_win;
		}
		if(l_remaining) {
			++l_size_comp;
		}
		if(l_size_comp == 3) {
			l_size_comp = 4;
		}
		switch(l_size_comp) {
			case 1: {
			    OPJ_CHAR * l_dest_ptr = (OPJ_CHAR*)p_dest;
			    const OPJ_INT32 * l_src_ptr = l_src_data;
			    if(l_img_comp->sgnd) {
				    for(j = 0; j < l_height; ++j) {
					    for(k = 0; k < l_width; ++k) {
						    *(l_dest_ptr++) = (OPJ_CHAR)(*(l_src_ptr++));
					    }
					    l_src_ptr += l_stride;
				    }
			    }
			    else {
				    for(j = 0; j < l_height; ++j) {
					    for(k = 0; k < l_width; ++k) {
						    *(l_dest_ptr++) = (OPJ_CHAR)((*(l_src_ptr++)) & 0xff);
					    }
					    l_src_ptr += l_stride;
				    }
			    }

			    p_dest = (uint8 *)l_dest_ptr;
		    }
		    break;
			case 2: {
			    const OPJ_INT32 * l_src_ptr = l_src_data;
			    OPJ_INT16 * l_dest_ptr = (OPJ_INT16*)p_dest;
			    if(l_img_comp->sgnd) {
				    for(j = 0; j < l_height; ++j) {
					    for(k = 0; k < l_width; ++k) {
						    OPJ_INT16 val = (OPJ_INT16)(*(l_src_ptr++));
						    memcpy(l_dest_ptr, &val, sizeof(val));
						    l_dest_ptr++;
					    }
					    l_src_ptr += l_stride;
				    }
			    }
			    else {
				    for(j = 0; j < l_height; ++j) {
					    for(k = 0; k < l_width; ++k) {
						    OPJ_INT16 val = (OPJ_INT16)((*(l_src_ptr++)) & 0xffff);
						    memcpy(l_dest_ptr, &val, sizeof(val));
						    l_dest_ptr++;
					    }
					    l_src_ptr += l_stride;
				    }
			    }

			    p_dest = (uint8 *)l_dest_ptr;
		    }
		    break;
			case 4: {
			    OPJ_INT32 * l_dest_ptr = (OPJ_INT32*)p_dest;
			    const OPJ_INT32 * l_src_ptr = l_src_data;

			    for(j = 0; j < l_height; ++j) {
				    memcpy(l_dest_ptr, l_src_ptr, l_width * sizeof(OPJ_INT32));
				    l_dest_ptr += l_width;
				    l_src_ptr += l_width + l_stride;
			    }

			    p_dest = (uint8 *)l_dest_ptr;
		    }
		    break;
		}

		++l_img_comp;
		++l_tilec;
	}

	return TRUE;
}

static void opj_tcd_free_tile(opj_tcd_t * p_tcd)
{
	OPJ_UINT32 compno, resno, bandno, precno;
	opj_tcd_tile_t * l_tile = 00;
	opj_tcd_tilecomp_t * l_tile_comp = 00;
	opj_tcd_resolution_t * l_res = 00;
	opj_tcd_band_t * l_band = 00;
	opj_tcd_precinct_t * l_precinct = 00;
	OPJ_UINT32 l_nb_resolutions, l_nb_precincts;
	void (* l_tcd_code_block_deallocate)(opj_tcd_precinct_t *) = 00;

	if(!p_tcd) {
		return;
	}

	if(!p_tcd->tcd_image) {
		return;
	}

	if(p_tcd->m_is_decoder) {
		l_tcd_code_block_deallocate = opj_tcd_code_block_dec_deallocate;
	}
	else {
		l_tcd_code_block_deallocate = opj_tcd_code_block_enc_deallocate;
	}

	l_tile = p_tcd->tcd_image->tiles;
	if(!l_tile) {
		return;
	}

	l_tile_comp = l_tile->comps;

	for(compno = 0; compno < l_tile->numcomps; ++compno) {
		l_res = l_tile_comp->resolutions;
		if(l_res) {
			l_nb_resolutions = l_tile_comp->resolutions_size / (OPJ_UINT32)sizeof(
				opj_tcd_resolution_t);
			for(resno = 0; resno < l_nb_resolutions; ++resno) {
				l_band = l_res->bands;
				for(bandno = 0; bandno < 3; ++bandno) {
					l_precinct = l_band->precincts;
					if(l_precinct) {
						l_nb_precincts = l_band->precincts_data_size / (OPJ_UINT32)sizeof(
							opj_tcd_precinct_t);
						for(precno = 0; precno < l_nb_precincts; ++precno) {
							opj_tgt_destroy(l_precinct->incltree);
							l_precinct->incltree = 00;
							opj_tgt_destroy(l_precinct->imsbtree);
							l_precinct->imsbtree = 00;
							(*l_tcd_code_block_deallocate)(l_precinct);
							++l_precinct;
						}
						ZFREE(l_band->precincts);
					}
					++l_band;
				} /* for (resno */
				++l_res;
			}
			ZFREE(l_tile_comp->resolutions);
		}
		if(l_tile_comp->ownsData && l_tile_comp->data) {
			opj_image_data_free(l_tile_comp->data);
			l_tile_comp->data = 00;
			l_tile_comp->ownsData = 0;
			l_tile_comp->data_size = 0;
			l_tile_comp->data_size_needed = 0;
		}
		opj_image_data_free(l_tile_comp->data_win);
		++l_tile_comp;
	}
	ZFREE(l_tile->comps);
	ZFREE(p_tcd->tcd_image->tiles);
}

static boolint opj_tcd_t2_decode(opj_tcd_t * p_tcd, uint8 * p_src_data, OPJ_UINT32 * p_data_read, 
	OPJ_UINT32 p_max_src_size, opj_codestream_index_t * p_cstr_index, opj_event_mgr_t * p_manager)
{
	opj_t2_t * l_t2 = opj_t2_create(p_tcd->image, p_tcd->cp);
	if(l_t2 == 00) {
		return FALSE;
	}
	if(!opj_t2_decode_packets(p_tcd, l_t2, p_tcd->tcd_tileno, p_tcd->tcd_image->tiles, p_src_data,
		    p_data_read, p_max_src_size, p_cstr_index, p_manager)) {
		opj_t2_destroy(l_t2);
		return FALSE;
	}
	opj_t2_destroy(l_t2);
	/*---------------CLEAN-------------------*/
	return TRUE;
}

static boolint opj_tcd_t1_decode(opj_tcd_t * p_tcd, opj_event_mgr_t * p_manager)
{
	OPJ_UINT32 compno;
	opj_tcd_tile_t * l_tile = p_tcd->tcd_image->tiles;
	opj_tcd_tilecomp_t* l_tile_comp = l_tile->comps;
	opj_tccp_t * l_tccp = p_tcd->tcp->tccps;
	volatile boolint ret = TRUE;
	boolint check_pterm = FALSE;
	opj_mutex_t* p_manager_mutex = opj_mutex_create();
	/* Only enable PTERM check if we decode all layers */
	if(p_tcd->tcp->num_layers_to_decode == p_tcd->tcp->numlayers && (l_tccp->cblksty & J2K_CCP_CBLKSTY_PTERM) != 0) {
		check_pterm = TRUE;
	}
	for(compno = 0; compno < l_tile->numcomps; ++compno, ++l_tile_comp, ++l_tccp) {
		if(p_tcd->used_component != NULL && !p_tcd->used_component[compno]) {
			continue;
		}
		opj_t1_decode_cblks(p_tcd, &ret, l_tile_comp, l_tccp, p_manager, p_manager_mutex, check_pterm);
		if(!ret) {
			break;
		}
	}
	opj_thread_pool_wait_completion(p_tcd->thread_pool, 0);
	opj_mutex_destroy(p_manager_mutex);
	return ret;
}

static boolint opj_tcd_dwt_decode(opj_tcd_t * p_tcd)
{
	OPJ_UINT32 compno;
	opj_tcd_tile_t * l_tile = p_tcd->tcd_image->tiles;
	opj_tcd_tilecomp_t * l_tile_comp = l_tile->comps;
	opj_tccp_t * l_tccp = p_tcd->tcp->tccps;
	opj_image_comp_t * l_img_comp = p_tcd->image->comps;
	for(compno = 0; compno < l_tile->numcomps; compno++, ++l_tile_comp, ++l_img_comp, ++l_tccp) {
		if(p_tcd->used_component != NULL && !p_tcd->used_component[compno]) {
			continue;
		}
		if(l_tccp->qmfbid == 1) {
			if(!opj_dwt_decode(p_tcd, l_tile_comp, l_img_comp->resno_decoded + 1)) {
				return FALSE;
			}
		}
		else {
			if(!opj_dwt_decode_real(p_tcd, l_tile_comp, l_img_comp->resno_decoded + 1)) {
				return FALSE;
			}
		}
	}
	return TRUE;
}

static boolint opj_tcd_mct_decode(opj_tcd_t * p_tcd, opj_event_mgr_t * p_manager)
{
	opj_tcd_tile_t * l_tile = p_tcd->tcd_image->tiles;
	opj_tcp_t * l_tcp = p_tcd->tcp;
	opj_tcd_tilecomp_t * l_tile_comp = l_tile->comps;
	OPJ_SIZE_T l_samples;
	OPJ_UINT32 i;

	if(l_tcp->mct == 0 || p_tcd->used_component != NULL) {
		return TRUE;
	}

	if(p_tcd->whole_tile_decoding) {
		opj_tcd_resolution_t* res_comp0 = l_tile->comps[0].resolutions +
		    l_tile_comp->minimum_num_resolutions - 1;

		/* A bit inefficient: we process more data than needed if */
		/* resno_decoded < l_tile_comp->minimum_num_resolutions-1, */
		/* but we would need to take into account a stride then */
		l_samples = (OPJ_SIZE_T)(res_comp0->x1 - res_comp0->x0) *
		    (OPJ_SIZE_T)(res_comp0->y1 - res_comp0->y0);
		if(l_tile->numcomps >= 3) {
			if(l_tile_comp->minimum_num_resolutions !=
			    l_tile->comps[1].minimum_num_resolutions ||
			    l_tile_comp->minimum_num_resolutions !=
			    l_tile->comps[2].minimum_num_resolutions) {
				opj_event_msg(p_manager, EVT_ERROR,
				    "Tiles don't all have the same dimension. Skip the MCT step.\n");
				return FALSE;
			}
		}
		if(l_tile->numcomps >= 3) {
			opj_tcd_resolution_t* res_comp1 = l_tile->comps[1].resolutions +
			    l_tile_comp->minimum_num_resolutions - 1;
			opj_tcd_resolution_t* res_comp2 = l_tile->comps[2].resolutions +
			    l_tile_comp->minimum_num_resolutions - 1;
			/* testcase 1336.pdf.asan.47.376 */
			if(p_tcd->image->comps[0].resno_decoded !=
			    p_tcd->image->comps[1].resno_decoded ||
			    p_tcd->image->comps[0].resno_decoded !=
			    p_tcd->image->comps[2].resno_decoded ||
			    (OPJ_SIZE_T)(res_comp1->x1 - res_comp1->x0) *
			    (OPJ_SIZE_T)(res_comp1->y1 - res_comp1->y0) != l_samples ||
			    (OPJ_SIZE_T)(res_comp2->x1 - res_comp2->x0) *
			    (OPJ_SIZE_T)(res_comp2->y1 - res_comp2->y0) != l_samples) {
				opj_event_msg(p_manager, EVT_ERROR,
				    "Tiles don't all have the same dimension. Skip the MCT step.\n");
				return FALSE;
			}
		}
	}
	else {
		opj_tcd_resolution_t* res_comp0 = l_tile->comps[0].resolutions +
		    p_tcd->image->comps[0].resno_decoded;

		l_samples = (OPJ_SIZE_T)(res_comp0->win_x1 - res_comp0->win_x0) *
		    (OPJ_SIZE_T)(res_comp0->win_y1 - res_comp0->win_y0);
		if(l_tile->numcomps >= 3) {
			opj_tcd_resolution_t* res_comp1 = l_tile->comps[1].resolutions +
			    p_tcd->image->comps[1].resno_decoded;
			opj_tcd_resolution_t* res_comp2 = l_tile->comps[2].resolutions +
			    p_tcd->image->comps[2].resno_decoded;
			/* testcase 1336.pdf.asan.47.376 */
			if(p_tcd->image->comps[0].resno_decoded !=
			    p_tcd->image->comps[1].resno_decoded ||
			    p_tcd->image->comps[0].resno_decoded !=
			    p_tcd->image->comps[2].resno_decoded ||
			    (OPJ_SIZE_T)(res_comp1->win_x1 - res_comp1->win_x0) *
			    (OPJ_SIZE_T)(res_comp1->win_y1 - res_comp1->win_y0) != l_samples ||
			    (OPJ_SIZE_T)(res_comp2->win_x1 - res_comp2->win_x0) *
			    (OPJ_SIZE_T)(res_comp2->win_y1 - res_comp2->win_y0) != l_samples) {
				opj_event_msg(p_manager, EVT_ERROR,
				    "Tiles don't all have the same dimension. Skip the MCT step.\n");
				return FALSE;
			}
		}
	}

	if(l_tile->numcomps >= 3) {
		if(l_tcp->mct == 2) {
			uint8 ** l_data;
			if(!l_tcp->m_mct_decoding_matrix) {
				return TRUE;
			}
			l_data = (uint8 **)SAlloc::M(l_tile->numcomps * sizeof(uint8 *));
			if(!l_data) {
				return FALSE;
			}
			for(i = 0; i < l_tile->numcomps; ++i) {
				if(p_tcd->whole_tile_decoding) {
					l_data[i] = (uint8 *)l_tile_comp->data;
				}
				else {
					l_data[i] = (uint8 *)l_tile_comp->data_win;
				}
				++l_tile_comp;
			}
			if(!opj_mct_decode_custom(/* MCT data */(uint8 *)l_tcp->m_mct_decoding_matrix, /* size of components */l_samples,
				    /* components */l_data,
				    /* nb of components (i.e. size of pData) */l_tile->numcomps, /* tells if the data is signed */p_tcd->image->comps->sgnd)) {
				SAlloc::F(l_data);
				return FALSE;
			}
			SAlloc::F(l_data);
		}
		else {
			if(l_tcp->tccps->qmfbid == 1) {
				if(p_tcd->whole_tile_decoding) {
					opj_mct_decode(l_tile->comps[0].data, l_tile->comps[1].data, l_tile->comps[2].data, l_samples);
				}
				else {
					opj_mct_decode(l_tile->comps[0].data_win, l_tile->comps[1].data_win, l_tile->comps[2].data_win, l_samples);
				}
			}
			else {
				if(p_tcd->whole_tile_decoding) {
					opj_mct_decode_real((float *)l_tile->comps[0].data,
					    (float *)l_tile->comps[1].data,
					    (float *)l_tile->comps[2].data,
					    l_samples);
				}
				else {
					opj_mct_decode_real((float *)l_tile->comps[0].data_win,
					    (float *)l_tile->comps[1].data_win,
					    (float *)l_tile->comps[2].data_win,
					    l_samples);
				}
			}
		}
	}
	else {
		opj_event_msg(p_manager, EVT_ERROR, "Number of components (%d) is inconsistent with a MCT. Skip the MCT step.\n", l_tile->numcomps);
	}
	return TRUE;
}

static boolint opj_tcd_dc_level_shift_decode(opj_tcd_t * p_tcd)
{
	OPJ_UINT32 compno;
	opj_tcd_tilecomp_t * l_tile_comp = 00;
	opj_tccp_t * l_tccp = 00;
	opj_image_comp_t * l_img_comp = 00;
	opj_tcd_resolution_t* l_res = 00;
	opj_tcd_tile_t * l_tile;
	OPJ_UINT32 l_width, l_height, i, j;
	OPJ_INT32 * l_current_ptr;
	OPJ_INT32 l_min, l_max;
	OPJ_UINT32 l_stride;
	l_tile = p_tcd->tcd_image->tiles;
	l_tile_comp = l_tile->comps;
	l_tccp = p_tcd->tcp->tccps;
	l_img_comp = p_tcd->image->comps;
	for(compno = 0; compno < l_tile->numcomps;
	    compno++, ++l_img_comp, ++l_tccp, ++l_tile_comp) {
		if(p_tcd->used_component != NULL && !p_tcd->used_component[compno]) {
			continue;
		}
		l_res = l_tile_comp->resolutions + l_img_comp->resno_decoded;
		if(!p_tcd->whole_tile_decoding) {
			l_width = l_res->win_x1 - l_res->win_x0;
			l_height = l_res->win_y1 - l_res->win_y0;
			l_stride = 0;
			l_current_ptr = l_tile_comp->data_win;
		}
		else {
			l_width = (OPJ_UINT32)(l_res->x1 - l_res->x0);
			l_height = (OPJ_UINT32)(l_res->y1 - l_res->y0);
			l_stride = (OPJ_UINT32)(
				l_tile_comp->resolutions[l_tile_comp->minimum_num_resolutions - 1].x1 -
				l_tile_comp->resolutions[l_tile_comp->minimum_num_resolutions - 1].x0)
			    - l_width;
			l_current_ptr = l_tile_comp->data;
			assert(l_height == 0 || l_width + l_stride <= l_tile_comp->data_size / l_height); /*MUPDF*/
		}
		if(l_img_comp->sgnd) {
			l_min = -(1 << (l_img_comp->prec - 1));
			l_max = (1 << (l_img_comp->prec - 1)) - 1;
		}
		else {
			l_min = 0;
			l_max = (OPJ_INT32)((1U << l_img_comp->prec) - 1);
		}
		if(l_tccp->qmfbid == 1) {
			for(j = 0; j < l_height; ++j) {
				for(i = 0; i < l_width; ++i) {
					/* TODO: do addition on int64 ? */
					*l_current_ptr = sclamp(*l_current_ptr + l_tccp->m_dc_level_shift, l_min, l_max);
					++l_current_ptr;
				}
				l_current_ptr += l_stride;
			}
		}
		else {
			for(j = 0; j < l_height; ++j) {
				for(i = 0; i < l_width; ++i) {
					float l_value = *((float *)l_current_ptr);
					if(l_value > INT_MAX) {
						*l_current_ptr = l_max;
					}
					else if(l_value < INT_MIN) {
						*l_current_ptr = l_min;
					}
					else {
						/* Do addition on int64 to avoid overflows */
						OPJ_INT64 l_value_int = (OPJ_INT64)opj_lrintf(l_value);
						*l_current_ptr = (OPJ_INT32)sclamp(l_value_int + l_tccp->m_dc_level_shift, static_cast<int64>(l_min), static_cast<int64>(l_max));
					}
					++l_current_ptr;
				}
				l_current_ptr += l_stride;
			}
		}
	}
	return TRUE;
}
/**
 * Deallocates the encoding data of the given precinct.
 */
static void opj_tcd_code_block_dec_deallocate(opj_tcd_precinct_t * p_precinct)
{
	OPJ_UINT32 cblkno, l_nb_code_blocks;
	opj_tcd_cblk_dec_t * l_code_block = p_precinct->cblks.dec;
	if(l_code_block) {
		/*slfprintf_stderr("deallocate codeblock:{\n");*/
		/*slfprintf_stderr("\t x0=%d, y0=%d, x1=%d, y1=%d\n",l_code_block->x0, l_code_block->y0, l_code_block->x1,
		   l_code_block->y1);*/
		/*slfprintf_stderr("\t numbps=%d, numlenbits=%d, len=%d, numnewpasses=%d, real_num_segs=%d,
		   m_current_max_segs=%d\n ",
		                l_code_block->numbps, l_code_block->numlenbits, l_code_block->len,
		                   l_code_block->numnewpasses, l_code_block->real_num_segs,
		                   l_code_block->m_current_max_segs );*/

		l_nb_code_blocks = p_precinct->block_size / (OPJ_UINT32)sizeof(opj_tcd_cblk_dec_t);
		/*slfprintf_stderr("nb_code_blocks =%d\t}\n", l_nb_code_blocks);*/
		for(cblkno = 0; cblkno < l_nb_code_blocks; ++cblkno) {
			ZFREE(l_code_block->segs);
			ZFREE(l_code_block->chunks);
			opj_aligned_free(l_code_block->decoded_data);
			l_code_block->decoded_data = NULL;
			++l_code_block;
		}
		ZFREE(p_precinct->cblks.dec);
	}
}

/**
 * Deallocates the encoding data of the given precinct.
 */
static void opj_tcd_code_block_enc_deallocate(opj_tcd_precinct_t * p_precinct)
{
	OPJ_UINT32 cblkno, l_nb_code_blocks;
	opj_tcd_cblk_enc_t * l_code_block = p_precinct->cblks.enc;
	if(l_code_block) {
		l_nb_code_blocks = p_precinct->block_size / (OPJ_UINT32)sizeof(opj_tcd_cblk_enc_t);
		for(cblkno = 0; cblkno < l_nb_code_blocks; ++cblkno) {
			if(l_code_block->data) {
				/* We refer to data - 1 since below we incremented it */
				/* in opj_tcd_code_block_enc_allocate_data() */
				SAlloc::F(l_code_block->data - 1);
				l_code_block->data = 00;
			}
			ZFREE(l_code_block->layers);
			ZFREE(l_code_block->passes);
			++l_code_block;
		}
		ZFREE(p_precinct->cblks.enc);
	}
}

OPJ_SIZE_T opj_tcd_get_encoder_input_buffer_size(opj_tcd_t * p_tcd)
{
	OPJ_UINT32 i;
	OPJ_SIZE_T l_data_size = 0;
	opj_image_comp_t * l_img_comp = 00;
	opj_tcd_tilecomp_t * l_tilec = 00;
	OPJ_UINT32 l_size_comp, l_remaining;

	l_tilec = p_tcd->tcd_image->tiles->comps;
	l_img_comp = p_tcd->image->comps;
	for(i = 0; i < p_tcd->image->numcomps; ++i) {
		l_size_comp = l_img_comp->prec >> 3; /*(/ 8)*/
		l_remaining = l_img_comp->prec & 7; /* (%8) */
		if(l_remaining) {
			++l_size_comp;
		}
		if(l_size_comp == 3) {
			l_size_comp = 4;
		}
		l_data_size += l_size_comp * ((OPJ_SIZE_T)(l_tilec->x1 - l_tilec->x0) * (OPJ_SIZE_T)(l_tilec->y1 - l_tilec->y0));
		++l_img_comp;
		++l_tilec;
	}
	return l_data_size;
}

static boolint opj_tcd_dc_level_shift_encode(opj_tcd_t * p_tcd)
{
	OPJ_UINT32 compno;
	opj_tcd_tilecomp_t * l_tile_comp = 00;
	opj_tccp_t * l_tccp = 00;
	opj_image_comp_t * l_img_comp = 00;
	opj_tcd_tile_t * l_tile;
	OPJ_SIZE_T l_nb_elem, i;
	OPJ_INT32 * l_current_ptr;

	l_tile = p_tcd->tcd_image->tiles;
	l_tile_comp = l_tile->comps;
	l_tccp = p_tcd->tcp->tccps;
	l_img_comp = p_tcd->image->comps;

	for(compno = 0; compno < l_tile->numcomps; compno++) {
		l_current_ptr = l_tile_comp->data;
		l_nb_elem = (OPJ_SIZE_T)(l_tile_comp->x1 - l_tile_comp->x0) *
		    (OPJ_SIZE_T)(l_tile_comp->y1 - l_tile_comp->y0);

		if(l_tccp->qmfbid == 1) {
			for(i = 0; i < l_nb_elem; ++i) {
				*l_current_ptr -= l_tccp->m_dc_level_shift;
				++l_current_ptr;
			}
		}
		else {
			for(i = 0; i < l_nb_elem; ++i) {
				*((float *)l_current_ptr) = (float)(*l_current_ptr -
				    l_tccp->m_dc_level_shift);
				++l_current_ptr;
			}
		}

		++l_img_comp;
		++l_tccp;
		++l_tile_comp;
	}

	return TRUE;
}

static boolint opj_tcd_mct_encode(opj_tcd_t * p_tcd)
{
	opj_tcd_tile_t * l_tile = p_tcd->tcd_image->tiles;
	opj_tcd_tilecomp_t * l_tile_comp = p_tcd->tcd_image->tiles->comps;
	OPJ_SIZE_T samples = (OPJ_SIZE_T)(l_tile_comp->x1 - l_tile_comp->x0) * (OPJ_SIZE_T)(l_tile_comp->y1 - l_tile_comp->y0);
	OPJ_UINT32 i;
	uint8 ** l_data = 00;
	opj_tcp_t * l_tcp = p_tcd->tcp;
	if(!p_tcd->tcp->mct) {
		return TRUE;
	}
	if(p_tcd->tcp->mct == 2) {
		if(!p_tcd->tcp->m_mct_coding_matrix) {
			return TRUE;
		}
		l_data = (uint8 **)SAlloc::M(l_tile->numcomps * sizeof(uint8 *));
		if(!l_data) {
			return FALSE;
		}
		for(i = 0; i < l_tile->numcomps; ++i) {
			l_data[i] = (uint8 *)l_tile_comp->data;
			++l_tile_comp;
		}
		if(!opj_mct_encode_custom(/* MCT data */ (uint8 *)p_tcd->tcp->m_mct_coding_matrix,
			    /* size of components */
			    samples,
			    /* components */
			    l_data,
			    /* nb of components (i.e. size of pData) */
			    l_tile->numcomps,
			    /* tells if the data is signed */
			    p_tcd->image->comps->sgnd)) {
			SAlloc::F(l_data);
			return FALSE;
		}
		SAlloc::F(l_data);
	}
	else if(l_tcp->tccps->qmfbid == 0) {
		opj_mct_encode_real((float *)l_tile->comps[0].data, (float *)l_tile->comps[1].data, (float *)l_tile->comps[2].data, samples);
	}
	else {
		opj_mct_encode(l_tile->comps[0].data, l_tile->comps[1].data, l_tile->comps[2].data, samples);
	}
	return TRUE;
}

static boolint opj_tcd_dwt_encode(opj_tcd_t * p_tcd)
{
	opj_tcd_tile_t * l_tile = p_tcd->tcd_image->tiles;
	opj_tcd_tilecomp_t * l_tile_comp = p_tcd->tcd_image->tiles->comps;
	opj_tccp_t * l_tccp = p_tcd->tcp->tccps;
	OPJ_UINT32 compno;
	for(compno = 0; compno < l_tile->numcomps; ++compno) {
		if(l_tccp->qmfbid == 1) {
			if(!opj_dwt_encode(p_tcd, l_tile_comp)) {
				return FALSE;
			}
		}
		else if(l_tccp->qmfbid == 0) {
			if(!opj_dwt_encode_real(p_tcd, l_tile_comp)) {
				return FALSE;
			}
		}
		++l_tile_comp;
		++l_tccp;
	}
	return TRUE;
}

static boolint opj_tcd_t1_encode(opj_tcd_t * p_tcd)
{
	const double * l_mct_norms;
	OPJ_UINT32 l_mct_numcomps = 0U;
	opj_tcp_t * l_tcp = p_tcd->tcp;
	if(l_tcp->mct == 1) {
		l_mct_numcomps = 3U;
		/* irreversible encoding */
		if(l_tcp->tccps->qmfbid == 0) {
			l_mct_norms = opj_mct_get_mct_norms_real();
		}
		else {
			l_mct_norms = opj_mct_get_mct_norms();
		}
	}
	else {
		l_mct_numcomps = p_tcd->image->numcomps;
		l_mct_norms = (const double*)(l_tcp->mct_norms);
	}

	return opj_t1_encode_cblks(p_tcd,
		   p_tcd->tcd_image->tiles, l_tcp, l_mct_norms,
		   l_mct_numcomps);

	return TRUE;
}

static boolint opj_tcd_t2_encode(opj_tcd_t * p_tcd,
    uint8 * p_dest_data,
    OPJ_UINT32 * p_data_written,
    OPJ_UINT32 p_max_dest_size,
    opj_codestream_info_t * p_cstr_info,
    opj_tcd_marker_info_t* p_marker_info,
    opj_event_mgr_t * p_manager)
{
	opj_t2_t * l_t2;

	l_t2 = opj_t2_create(p_tcd->image, p_tcd->cp);
	if(l_t2 == 00) {
		return FALSE;
	}

	if(!opj_t2_encode_packets(
		    l_t2,
		    p_tcd->tcd_tileno,
		    p_tcd->tcd_image->tiles,
		    p_tcd->tcp->numlayers,
		    p_dest_data,
		    p_data_written,
		    p_max_dest_size,
		    p_cstr_info,
		    p_marker_info,
		    p_tcd->tp_num,
		    p_tcd->tp_pos,
		    p_tcd->cur_pino,
		    FINAL_PASS,
		    p_manager)) {
		opj_t2_destroy(l_t2);
		return FALSE;
	}

	opj_t2_destroy(l_t2);

	/*---------------CLEAN-------------------*/
	return TRUE;
}

static boolint opj_tcd_rate_allocate_encode(opj_tcd_t * p_tcd,
    uint8 * p_dest_data,
    OPJ_UINT32 p_max_dest_size,
    opj_codestream_info_t * p_cstr_info,
    opj_event_mgr_t * p_manager)
{
	opj_cp_t * l_cp = p_tcd->cp;
	OPJ_UINT32 l_nb_written = 0;

	if(p_cstr_info) {
		p_cstr_info->index_write = 0;
	}

	if(l_cp->m_specific_param.m_enc.m_disto_alloc ||
	    l_cp->m_specific_param.m_enc.m_fixed_quality) {
		/* fixed_quality */
		/* Normal Rate/distortion allocation */
		if(!opj_tcd_rateallocate(p_tcd, p_dest_data, &l_nb_written, p_max_dest_size,
		    p_cstr_info, p_manager)) {
			return FALSE;
		}
	}
	else {
		/* Fixed layer allocation */
		opj_tcd_rateallocate_fixed(p_tcd);
	}

	return TRUE;
}

boolint opj_tcd_copy_tile_data(opj_tcd_t * p_tcd,
    uint8 * p_src,
    OPJ_SIZE_T p_src_length)
{
	OPJ_UINT32 i;
	OPJ_SIZE_T j;
	OPJ_SIZE_T l_data_size = 0;
	opj_image_comp_t * l_img_comp = 00;
	opj_tcd_tilecomp_t * l_tilec = 00;
	OPJ_UINT32 l_size_comp, l_remaining;
	OPJ_SIZE_T l_nb_elem;

	l_data_size = opj_tcd_get_encoder_input_buffer_size(p_tcd);
	if(l_data_size != p_src_length) {
		return FALSE;
	}

	l_tilec = p_tcd->tcd_image->tiles->comps;
	l_img_comp = p_tcd->image->comps;
	for(i = 0; i < p_tcd->image->numcomps; ++i) {
		l_size_comp = l_img_comp->prec >> 3; /*(/ 8)*/
		l_remaining = l_img_comp->prec & 7; /* (%8) */
		l_nb_elem = (OPJ_SIZE_T)(l_tilec->x1 - l_tilec->x0) *
		    (OPJ_SIZE_T)(l_tilec->y1 - l_tilec->y0);

		if(l_remaining) {
			++l_size_comp;
		}

		if(l_size_comp == 3) {
			l_size_comp = 4;
		}

		switch(l_size_comp) {
			case 1: {
			    OPJ_CHAR * l_src_ptr = (OPJ_CHAR*)p_src;
			    OPJ_INT32 * l_dest_ptr = l_tilec->data;

			    if(l_img_comp->sgnd) {
				    for(j = 0; j < l_nb_elem; ++j) {
					    *(l_dest_ptr++) = (OPJ_INT32)(*(l_src_ptr++));
				    }
			    }
			    else {
				    for(j = 0; j < l_nb_elem; ++j) {
					    *(l_dest_ptr++) = (*(l_src_ptr++)) & 0xff;
				    }
			    }

			    p_src = (uint8 *)l_src_ptr;
		    }
		    break;
			case 2: {
			    OPJ_INT32 * l_dest_ptr = l_tilec->data;
			    OPJ_INT16 * l_src_ptr = (OPJ_INT16*)p_src;
			    if(l_img_comp->sgnd) {
				    for(j = 0; j < l_nb_elem; ++j) {
					    *(l_dest_ptr++) = (OPJ_INT32)(*(l_src_ptr++));
				    }
			    }
			    else {
				    for(j = 0; j < l_nb_elem; ++j) {
					    *(l_dest_ptr++) = (*(l_src_ptr++)) & 0xffff;
				    }
			    }
			    p_src = (uint8 *)l_src_ptr;
		    }
		    break;
			case 4: {
			    OPJ_INT32 * l_src_ptr = (OPJ_INT32*)p_src;
			    OPJ_INT32 * l_dest_ptr = l_tilec->data;
			    for(j = 0; j < l_nb_elem; ++j) {
				    *(l_dest_ptr++) = (OPJ_INT32)(*(l_src_ptr++));
			    }
			    p_src = (uint8 *)l_src_ptr;
		    }
		    break;
		}
		++l_img_comp;
		++l_tilec;
	}
	return TRUE;
}

boolint opj_tcd_is_band_empty(opj_tcd_band_t* band)
{
	return (band->x1 - band->x0 == 0) || (band->y1 - band->y0 == 0);
}

boolint opj_tcd_is_subband_area_of_interest(opj_tcd_t * tcd, OPJ_UINT32 compno, OPJ_UINT32 resno, OPJ_UINT32 bandno,
    OPJ_UINT32 band_x0, OPJ_UINT32 band_y0, OPJ_UINT32 band_x1, OPJ_UINT32 band_y1)
{
	/* Note: those values for filter_margin are in part the result of */
	/* experimentation. The value 2 for QMFBID=1 (5x3 filter) can be linked */
	/* to the maximum left/right extension given in tables F.2 and F.3 of the */
	/* standard. The value 3 for QMFBID=0 (9x7 filter) is more suspicious, */
	/* since F.2 and F.3 would lead to 4 instead, so the current 3 might be */
	/* needed to be bumped to 4, in case inconsistencies are found while */
	/* decoding parts of irreversible coded images. */
	/* See opj_dwt_decode_partial_53 and opj_dwt_decode_partial_97 as well */
	OPJ_UINT32 filter_margin = (tcd->tcp->tccps[compno].qmfbid == 1) ? 2 : 3;
	opj_tcd_tilecomp_t * tilec = &(tcd->tcd_image->tiles->comps[compno]);
	opj_image_comp_t* image_comp = &(tcd->image->comps[compno]);
	/* Compute the intersection of the area of interest, expressed in tile coordinates */
	/* with the tile coordinates */
	OPJ_UINT32 tcx0 = smax((OPJ_UINT32)tilec->x0, opj_uint_ceildiv(tcd->win_x0, image_comp->dx));
	OPJ_UINT32 tcy0 = smax((OPJ_UINT32)tilec->y0, opj_uint_ceildiv(tcd->win_y0, image_comp->dy));
	OPJ_UINT32 tcx1 = smin((OPJ_UINT32)tilec->x1, opj_uint_ceildiv(tcd->win_x1, image_comp->dx));
	OPJ_UINT32 tcy1 = smin((OPJ_UINT32)tilec->y1, opj_uint_ceildiv(tcd->win_y1, image_comp->dy));
	/* Compute number of decomposition for this band. See table F-1 */
	OPJ_UINT32 nb = (resno == 0) ? tilec->numresolutions - 1 : tilec->numresolutions - resno;
	/* Map above tile-based coordinates to sub-band-based coordinates per */
	/* equation B-15 of the standard */
	OPJ_UINT32 x0b = bandno & 1;
	OPJ_UINT32 y0b = bandno >> 1;
	OPJ_UINT32 tbx0 = (nb == 0) ? tcx0 : (tcx0 <= (1U << (nb - 1)) * x0b) ? 0 : opj_uint_ceildivpow2(tcx0 - (1U << (nb - 1)) * x0b, nb);
	OPJ_UINT32 tby0 = (nb == 0) ? tcy0 : (tcy0 <= (1U << (nb - 1)) * y0b) ? 0 : opj_uint_ceildivpow2(tcy0 - (1U << (nb - 1)) * y0b, nb);
	OPJ_UINT32 tbx1 = (nb == 0) ? tcx1 : (tcx1 <= (1U << (nb - 1)) * x0b) ? 0 : opj_uint_ceildivpow2(tcx1 - (1U << (nb - 1)) * x0b, nb);
	OPJ_UINT32 tby1 = (nb == 0) ? tcy1 : (tcy1 <= (1U << (nb - 1)) * y0b) ? 0 : opj_uint_ceildivpow2(tcy1 - (1U << (nb - 1)) * y0b, nb);
	boolint intersects;

	if(tbx0 < filter_margin) {
		tbx0 = 0;
	}
	else {
		tbx0 -= filter_margin;
	}
	if(tby0 < filter_margin) {
		tby0 = 0;
	}
	else {
		tby0 -= filter_margin;
	}
	tbx1 = opj_uint_adds(tbx1, filter_margin);
	tby1 = opj_uint_adds(tby1, filter_margin);
	intersects = band_x0 < tbx1 && band_y0 < tby1 && band_x1 > tbx0 && band_y1 > tby0;
#ifdef DEBUG_VERBOSE
	printf("compno=%u resno=%u nb=%u bandno=%u x0b=%u y0b=%u band=%u,%u,%u,%u tb=%u,%u,%u,%u -> %u\n",
	    compno, resno, nb, bandno, x0b, y0b,
	    band_x0, band_y0, band_x1, band_y1,
	    tbx0, tby0, tbx1, tby1, intersects);
#endif
	return intersects;
}

/** Returns whether a tile componenent is fully decoded, taking into account
 * p_tcd->win_* members.
 *
 * @param p_tcd    TCD handle.
 * @param compno Component number
 * @return TRUE whether the tile componenent is fully decoded
 */
static boolint opj_tcd_is_whole_tilecomp_decoding(opj_tcd_t * p_tcd, OPJ_UINT32 compno)
{
	opj_tcd_tilecomp_t* tilec = &(p_tcd->tcd_image->tiles->comps[compno]);
	opj_image_comp_t* image_comp = &(p_tcd->image->comps[compno]);
	/* Compute the intersection of the area of interest, expressed in tile coordinates */
	/* with the tile coordinates */
	OPJ_UINT32 tcx0 = smax((OPJ_UINT32)tilec->x0, opj_uint_ceildiv(p_tcd->win_x0, image_comp->dx));
	OPJ_UINT32 tcy0 = smax((OPJ_UINT32)tilec->y0, opj_uint_ceildiv(p_tcd->win_y0, image_comp->dy));
	OPJ_UINT32 tcx1 = smin((OPJ_UINT32)tilec->x1, opj_uint_ceildiv(p_tcd->win_x1, image_comp->dx));
	OPJ_UINT32 tcy1 = smin((OPJ_UINT32)tilec->y1, opj_uint_ceildiv(p_tcd->win_y1, image_comp->dy));
	OPJ_UINT32 shift = tilec->numresolutions - tilec->minimum_num_resolutions;
	/* Tolerate small margin within the reduced resolution factor to consider if */
	/* the whole tile path must be taken */
	return (tcx0 >= (OPJ_UINT32)tilec->x0 &&
	       tcy0 >= (OPJ_UINT32)tilec->y0 &&
	       tcx1 <= (OPJ_UINT32)tilec->x1 &&
	       tcy1 <= (OPJ_UINT32)tilec->y1 &&
	       (shift >= 32 ||
	       (((tcx0 - (OPJ_UINT32)tilec->x0) >> shift) == 0 &&
	       ((tcy0 - (OPJ_UINT32)tilec->y0) >> shift) == 0 &&
	       (((OPJ_UINT32)tilec->x1 - tcx1) >> shift) == 0 &&
	       (((OPJ_UINT32)tilec->y1 - tcy1) >> shift) == 0)));
}

/* ----------------------------------------------------------------------- */

opj_tcd_marker_info_t* opj_tcd_marker_info_create(boolint need_PLT)
{
	opj_tcd_marker_info_t * l_tcd_marker_info = (opj_tcd_marker_info_t*)SAlloc::C(1, sizeof(opj_tcd_marker_info_t));
	if(l_tcd_marker_info)
		l_tcd_marker_info->need_PLT = need_PLT;
	return l_tcd_marker_info;
}

/* ----------------------------------------------------------------------- */

void opj_tcd_marker_info_destroy(opj_tcd_marker_info_t * p_tcd_marker_info)
{
	if(p_tcd_marker_info) {
		SAlloc::F(p_tcd_marker_info->p_packet_size);
		SAlloc::F(p_tcd_marker_info);
	}
}
