/*
 * The copyright in this software is being made available under the 2-clauses
 * BSD License, included below. This software may be subject to other third
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2017, IntoPix SA <contact@intopix.com>
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

#ifndef OPJ_SPARSE_ARRAY_H
#define OPJ_SPARSE_ARRAY_H
/**
   @file sparse_array.h
   @brief Sparse array management

   The functions in this file manage sparse arrays. Sparse arrays are arrays with
   potential big dimensions, but with very few samples actually set. Such sparse
   arrays require allocating a low amount of memory, by just allocating memory
   for blocks of the array that are set. The minimum memory allocation unit is a
   a block. There is a trade-off to pick up an appropriate dimension for blocks.
   If it is too big, and pixels set are far from each other, too much memory will
   be used. If blocks are too small, the book-keeping costs of blocks will raise.
 */

/** @defgroup SPARSE_ARRAY SPARSE ARRAYS - Sparse arrays */
/*@{*/

/** Opaque type for sparse arrays that contain int32 values */
typedef struct opj_sparse_array_int32 opj_sparse_array_int32_t;

/** Creates a new sparse array.
 * @param width total width of the array.
 * @param height total height of the array
 * @param block_width width of a block.
 * @param block_height height of a block.
 * @return a new sparse array instance, or NULL in case of failure.
 */
opj_sparse_array_int32_t* opj_sparse_array_int32_create(OPJ_UINT32 width,
    OPJ_UINT32 height,
    OPJ_UINT32 block_width,
    OPJ_UINT32 block_height);

/** Frees a sparse array.
 * @param sa sparse array instance.
 */
void opj_sparse_array_int32_free(opj_sparse_array_int32_t* sa);

/** Returns whether region bounds are valid (non empty and within array bounds)
 * @param sa sparse array instance.
 * @param x0 left x coordinate of the region.
 * @param y0 top x coordinate of the region.
 * @param x1 right x coordinate (not included) of the region. Must be greater than x0.
 * @param y1 bottom y coordinate (not included) of the region. Must be greater than y0.
 * @return TRUE or FALSE.
 */
boolint opj_sparse_array_is_region_valid(const opj_sparse_array_int32_t* sa,
    OPJ_UINT32 x0,
    OPJ_UINT32 y0,
    OPJ_UINT32 x1,
    OPJ_UINT32 y1);

/** Read the content of a rectangular region of the sparse array into a
 * user buffer.
 *
 * Regions not written with opj_sparse_array_int32_write() are read as 0.
 *
 * @param sa sparse array instance.
 * @param x0 left x coordinate of the region to read in the sparse array.
 * @param y0 top x coordinate of the region to read in the sparse array.
 * @param x1 right x coordinate (not included) of the region to read in the sparse array. Must be greater than x0.
 * @param y1 bottom y coordinate (not included) of the region to read in the sparse array. Must be greater than y0.
 * @param dest user buffer to fill. Must be at least sizeof(int32) * ( (y1 - y0 - 1) * dest_line_stride + (x1 - x0 - 1)
 ** dest_col_stride + 1) bytes large.
 * @param dest_col_stride spacing (in elements, not in bytes) in x dimension between consecutive elements of the user
 *buffer.
 * @param dest_line_stride spacing (in elements, not in bytes) in y dimension between consecutive elements of the user
 *buffer.
 * @param forgiving if set to TRUE and the region is invalid, TRUE will still be returned.
 * @return TRUE in case of success.
 */
boolint opj_sparse_array_int32_read(const opj_sparse_array_int32_t* sa,
    OPJ_UINT32 x0,
    OPJ_UINT32 y0,
    OPJ_UINT32 x1,
    OPJ_UINT32 y1,
    OPJ_INT32* dest,
    OPJ_UINT32 dest_col_stride,
    OPJ_UINT32 dest_line_stride,
    boolint forgiving);

/** Write the content of a rectangular region into the sparse array from a
 * user buffer.
 *
 * Blocks intersecting the region are allocated, if not already done.
 *
 * @param sa sparse array instance.
 * @param x0 left x coordinate of the region to write into the sparse array.
 * @param y0 top x coordinate of the region to write into the sparse array.
 * @param x1 right x coordinate (not included) of the region to write into the sparse array. Must be greater than x0.
 * @param y1 bottom y coordinate (not included) of the region to write into the sparse array. Must be greater than y0.
 * @param src user buffer to fill. Must be at least sizeof(int32) * ( (y1 - y0 - 1) * src_line_stride + (x1 - x0 - 1) *
 *src_col_stride + 1) bytes large.
 * @param src_col_stride spacing (in elements, not in bytes) in x dimension between consecutive elements of the user
 *buffer.
 * @param src_line_stride spacing (in elements, not in bytes) in y dimension between consecutive elements of the user
 *buffer.
 * @param forgiving if set to TRUE and the region is invalid, TRUE will still be returned.
 * @return TRUE in case of success.
 */
boolint opj_sparse_array_int32_write(opj_sparse_array_int32_t* sa, OPJ_UINT32 x0, OPJ_UINT32 y0, OPJ_UINT32 x1, OPJ_UINT32 y1, const OPJ_INT32* src,
    OPJ_UINT32 src_col_stride, OPJ_UINT32 src_line_stride, boolint forgiving);
/*@}*/

#endif /* OPJ_SPARSE_ARRAY_H */
