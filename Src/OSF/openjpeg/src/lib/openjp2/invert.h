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
#ifndef OPJ_INVERT_H
#define OPJ_INVERT_H
/**
   @file invert.h
   @brief Implementation of the matrix inversion

   The function in INVERT.H compute a matrix inversion with a LUP method
 */

/** @defgroup INVERT INVERT - Implementation of a matrix inversion */
/*@{*/
/** @name Exported functions */
/*@{*/
/**
 * Calculates a n x n double matrix inversion with a LUP method. Data is aligned, rows after rows (or columns after
 *columns).
 * The function does not take ownership of any memory block, data must be fred by the user.
 *
 * @param pSrcMatrix    the matrix to invert.
 * @param pDestMatrix   data to store the inverted matrix.
 * @param nb_compo      size of the matrix
 * @return TRUE if the inversion is successful, FALSE if the matrix is singular.
 */
boolint opj_matrix_inversion_f(float * pSrcMatrix, float * pDestMatrix, uint32_t nb_compo);
/*@}*/

/*@}*/

#endif /* OPJ_INVERT_H */
