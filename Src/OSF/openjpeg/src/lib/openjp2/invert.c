/*
 * The copyright in this software is being made available under the 2-clauses
 * BSD License, included below. This software may be subject to other third
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2008, Jerome Fimes, Communications & Systemes <jerome.fimes@c-s.fr>
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
/**
 * LUP decomposition
 */
static boolint opj_lupDecompose(float * matrix, uint32_t * permutations, float * p_swap_area, uint32_t nb_compo);
/**
 * LUP solving
 */
static void opj_lupSolve(float * pResult, float* pMatrix, float* pVector, uint32_t* pPermutations, uint32_t nb_compo, float * p_intermediate_data);
/**
 * LUP inversion (call with the result of lupDecompose)
 */
static void opj_lupInvert(float * pSrcMatrix, float * pDestMatrix, uint32_t nb_compo, uint32_t * pPermutations,
    float * p_src_temp, float * p_dest_temp, float * p_swap_area);
/*
   ==========================================================
   Matric inversion interface
   ==========================================================
 */
/**
 * Matrix inversion.
 */
boolint opj_matrix_inversion_f(float * pSrcMatrix, float * pDestMatrix, uint32_t nb_compo)
{
	uint8 * l_data = 0;
	uint32_t l_permutation_size = nb_compo * (uint32_t)sizeof(uint32_t);
	uint32_t l_swap_size = nb_compo * (uint32_t)sizeof(float);
	uint32_t l_total_size = l_permutation_size + 3 * l_swap_size;
	uint32_t * lPermutations = 0;
	float * l_double_data = 0;
	l_data = (uint8*)opj_malloc(l_total_size);
	if(l_data == 0) {
		return FALSE;
	}
	lPermutations = (uint32_t*)l_data;
	l_double_data = (float*)(l_data + l_permutation_size);
	memzero(lPermutations, l_permutation_size);
	if(!opj_lupDecompose(pSrcMatrix, lPermutations, l_double_data, nb_compo)) {
		SAlloc::F(l_data);
		return FALSE;
	}
	opj_lupInvert(pSrcMatrix, pDestMatrix, nb_compo, lPermutations, l_double_data, l_double_data + nb_compo, l_double_data + 2 * nb_compo);
	SAlloc::F(l_data);
	return TRUE;
}
/*
   ==========================================================
   Local functions
   ==========================================================
 */
static boolint opj_lupDecompose(float * matrix, uint32_t * permutations, float * p_swap_area, uint32_t nb_compo)
{
	uint32_t * tmpPermutations = permutations;
	uint32_t * dstPermutations;
	uint32_t k2 = 0, t;
	float temp;
	uint32_t i, j, k;
	float p;
	uint32_t lLastColum = nb_compo - 1;
	uint32_t lSwapSize = nb_compo * (uint32_t)sizeof(float);
	float * lTmpMatrix = matrix;
	float * lColumnMatrix, * lDestMatrix;
	uint32_t offset = 1;
	uint32_t lStride = nb_compo - 1;
	/*initialize permutations */
	for(i = 0; i < nb_compo; ++i) {
		*tmpPermutations++ = i;
	}
	/* now make a pivot with column switch */
	tmpPermutations = permutations;
	for(k = 0; k < lLastColum; ++k) {
		p = 0.0;
		/* take the middle element */
		lColumnMatrix = lTmpMatrix + k;
		/* make permutation with the biggest value in the column */
		for(i = k; i < nb_compo; ++i) {
			temp = ((*lColumnMatrix > 0) ? *lColumnMatrix : -(*lColumnMatrix));
			if(temp > p) {
				p = temp;
				k2 = i;
			}
			/* next line */
			lColumnMatrix += nb_compo;
		}

		/* a whole rest of 0 -> non singular */
		if(p == 0.0) {
			return FALSE;
		}

		/* should we permute ? */
		if(k2 != k) {
			/*exchange of line */
			/* k2 > k */
			dstPermutations = tmpPermutations + k2 - k;
			/* swap indices */
			t = *tmpPermutations;
			*tmpPermutations = *dstPermutations;
			*dstPermutations = t;

			/* and swap entire line. */
			lColumnMatrix = lTmpMatrix + (k2 - k) * nb_compo;
			memcpy(p_swap_area, lColumnMatrix, lSwapSize);
			memcpy(lColumnMatrix, lTmpMatrix, lSwapSize);
			memcpy(lTmpMatrix, p_swap_area, lSwapSize);
		}

		/* now update data in the rest of the line and line after */
		lDestMatrix = lTmpMatrix + k;
		lColumnMatrix = lDestMatrix + nb_compo;
		/* take the middle element */
		temp = *(lDestMatrix++);

		/* now compute up data (i.e. coeff up of the diagonal). */
		for(i = offset; i < nb_compo; ++i) {
			/*lColumnMatrix; */
			/* divide the lower column elements by the diagonal value */

			/* matrix[i][k] /= matrix[k][k]; */
			/* p = matrix[i][k] */
			p = *lColumnMatrix / temp;
			*(lColumnMatrix++) = p;

			for(j = /* k + 1 */ offset; j < nb_compo; ++j) {
				/* matrix[i][j] -= matrix[i][k] * matrix[k][j]; */
				*(lColumnMatrix++) -= p * (*(lDestMatrix++));
			}
			/* come back to the k+1th element */
			lDestMatrix -= lStride;
			/* go to kth element of the next line */
			lColumnMatrix += k;
		}

		/* offset is now k+2 */
		++offset;
		/* 1 element less for stride */
		--lStride;
		/* next line */
		lTmpMatrix += nb_compo;
		/* next permutation element */
		++tmpPermutations;
	}
	return TRUE;
}

static void opj_lupSolve(float * pResult, float * pMatrix, float * pVector,
    uint32_t* pPermutations, uint32_t nb_compo, float * p_intermediate_data)
{
	int32_t k;
	uint32_t i, j;
	float sum;
	float u;
	uint32_t lStride = nb_compo + 1;
	float * lCurrentPtr;
	float * lDestPtr;
	float * lTmpMatrix;
	float * lLineMatrix = pMatrix;
	float * lBeginPtr = pResult + nb_compo - 1;
	uint32_t * lCurrentPermutationPtr = pPermutations;
	float * lIntermediatePtr = p_intermediate_data;
	float * lGeneratedData = p_intermediate_data + nb_compo - 1;
	for(i = 0; i < nb_compo; ++i) {
		sum = 0.0;
		lCurrentPtr = p_intermediate_data;
		lTmpMatrix = lLineMatrix;
		for(j = 1; j <= i; ++j) {
			/* sum += matrix[i][j-1] * y[j-1]; */
			sum += (*(lTmpMatrix++)) * (*(lCurrentPtr++));
		}
		/*y[i] = pVector[pPermutations[i]] - sum; */
		*(lIntermediatePtr++) = pVector[*(lCurrentPermutationPtr++)] - sum;
		lLineMatrix += nb_compo;
	}
	/* we take the last point of the matrix */
	lLineMatrix = pMatrix + nb_compo * nb_compo - 1;
	/* and we take after the last point of the destination vector */
	lDestPtr = pResult + nb_compo;
	assert(nb_compo != 0);
	for(k = (int32_t)nb_compo - 1; k != -1; --k) {
		sum = 0.0;
		lTmpMatrix = lLineMatrix;
		u = *(lTmpMatrix++);
		lCurrentPtr = lDestPtr--;
		for(j = (uint32_t)(k + 1); j < nb_compo; ++j) {
			/* sum += matrix[k][j] * x[j] */
			sum += (*(lTmpMatrix++)) * (*(lCurrentPtr++));
		}
		/*x[k] = (y[k] - sum) / u; */
		*(lBeginPtr--) = (*(lGeneratedData--) - sum) / u;
		lLineMatrix -= lStride;
	}
}

static void opj_lupInvert(float * pSrcMatrix, float * pDestMatrix, uint32_t nb_compo,
    uint32_t * pPermutations, float * p_src_temp, float * p_dest_temp, float * p_swap_area)
{
	uint32_t j, i;
	float * lCurrentPtr;
	float * lLineMatrix = pDestMatrix;
	uint32_t lSwapSize = nb_compo * (uint32_t)sizeof(float);
	for(j = 0; j < nb_compo; ++j) {
		lCurrentPtr = lLineMatrix++;
		memzero(p_src_temp, lSwapSize);
		p_src_temp[j] = 1.0;
		opj_lupSolve(p_dest_temp, pSrcMatrix, p_src_temp, pPermutations, nb_compo, p_swap_area);
		for(i = 0; i < nb_compo; ++i) {
			*(lCurrentPtr) = p_dest_temp[i];
			lCurrentPtr += nb_compo;
		}
	}
}
