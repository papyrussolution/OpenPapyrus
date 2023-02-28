/*
 * The copyright in this software is being made available under the 2-clauses
 * BSD License, included below. This software may be subject to other third
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2017, IntoPix SA <contact@intopix.com> All rights reserved.
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

struct opj_sparse_array_int32 {
	uint32_t width;
	uint32_t height;
	uint32_t block_width;
	uint32_t block_height;
	uint32_t block_count_hor;
	uint32_t block_count_ver;
	int32_t** data_blocks;
};

opj_sparse_array_int32_t* opj_sparse_array_int32_create(uint32_t width, uint32_t height, uint32_t block_width, uint32_t block_height)
{
	opj_sparse_array_int32_t* sa;
	if(width == 0 || height == 0 || block_width == 0 || block_height == 0) {
		return NULL;
	}
	if(block_width > ((uint32_t) ~0U) / block_height / sizeof(int32_t)) {
		return NULL;
	}
	sa = (opj_sparse_array_int32_t*)opj_calloc(1, sizeof(opj_sparse_array_int32_t));
	sa->width = width;
	sa->height = height;
	sa->block_width = block_width;
	sa->block_height = block_height;
	sa->block_count_hor = opj_uint_ceildiv(width, block_width);
	sa->block_count_ver = opj_uint_ceildiv(height, block_height);
	if(sa->block_count_hor > ((uint32_t) ~0U) / sa->block_count_ver) {
		SAlloc::F(sa);
		return NULL;
	}
	sa->data_blocks = (int32_t**)opj_calloc(sizeof(int32_t*), (size_t)sa->block_count_hor * sa->block_count_ver);
	if(sa->data_blocks == NULL) {
		SAlloc::F(sa);
		return NULL;
	}
	return sa;
}

void opj_sparse_array_int32_free(opj_sparse_array_int32_t* sa)
{
	if(sa) {
		for(uint32_t i = 0; i < sa->block_count_hor * sa->block_count_ver; i++) {
			SAlloc::F(sa->data_blocks[i]);
		}
		SAlloc::F(sa->data_blocks);
		SAlloc::F(sa);
	}
}

boolint opj_sparse_array_is_region_valid(const opj_sparse_array_int32_t* sa, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1)
{
	return !(x0 >= sa->width || x1 <= x0 || x1 > sa->width || y0 >= sa->height || y1 <= y0 || y1 > sa->height);
}

static boolint opj_sparse_array_int32_read_or_write(const opj_sparse_array_int32_t* sa, uint32_t x0, uint32_t y0, uint32_t x1,
    uint32_t y1, int32_t* buf, uint32_t buf_col_stride, uint32_t buf_line_stride, boolint forgiving, boolint is_read_op)
{
	uint32_t y, block_y;
	uint32_t y_incr = 0;
	const uint32_t block_width = sa->block_width;
	if(!opj_sparse_array_is_region_valid(sa, x0, y0, x1, y1)) {
		return forgiving;
	}
	block_y = y0 / sa->block_height;
	for(y = y0; y < y1; block_y++, y += y_incr) {
		uint32_t x, block_x;
		uint32_t x_incr = 0;
		uint32_t block_y_offset;
		y_incr = (y == y0) ? sa->block_height - (y0 % sa->block_height) : sa->block_height;
		block_y_offset = sa->block_height - y_incr;
		y_incr = opj_uint_min(y_incr, y1 - y);
		block_x = x0 / block_width;
		for(x = x0; x < x1; block_x++, x += x_incr) {
			uint32_t j;
			uint32_t block_x_offset;
			int32_t* src_block;
			x_incr = (x == x0) ? block_width - (x0 % block_width) : block_width;
			block_x_offset = block_width - x_incr;
			x_incr = opj_uint_min(x_incr, x1 - x);
			src_block = sa->data_blocks[block_y * sa->block_count_hor + block_x];
			if(is_read_op) {
				if(src_block == NULL) {
					if(buf_col_stride == 1) {
						int32_t* dest_ptr = buf + (y - y0) * (size_t)buf_line_stride +
						    (x - x0) * buf_col_stride;
						for(j = 0; j < y_incr; j++) {
							memzero(dest_ptr, sizeof(int32_t) * x_incr);
							dest_ptr += buf_line_stride;
						}
					}
					else {
						int32_t* dest_ptr = buf + (y - y0) * (size_t)buf_line_stride + (x - x0) * buf_col_stride;
						for(j = 0; j < y_incr; j++) {
							for(uint32_t k = 0; k < x_incr; k++) {
								dest_ptr[k * buf_col_stride] = 0;
							}
							dest_ptr += buf_line_stride;
						}
					}
				}
				else {
					const int32_t* OPJ_RESTRICT src_ptr = src_block + block_y_offset * (size_t)block_width + block_x_offset;
					if(buf_col_stride == 1) {
						int32_t* OPJ_RESTRICT dest_ptr = buf + (y - y0) * (size_t)buf_line_stride + (x - x0) * buf_col_stride;
						if(x_incr == 4) {
							/* Same code as general branch, but the compiler */
							/* can have an efficient memcpy() */
							(void)(x_incr); /* trick to silent cppcheck duplicateBranch warning */
							for(j = 0; j < y_incr; j++) {
								memcpy(dest_ptr, src_ptr, sizeof(int32_t) * x_incr);
								dest_ptr += buf_line_stride;
								src_ptr += block_width;
							}
						}
						else {
							for(j = 0; j < y_incr; j++) {
								memcpy(dest_ptr, src_ptr, sizeof(int32_t) * x_incr);
								dest_ptr += buf_line_stride;
								src_ptr += block_width;
							}
						}
					}
					else {
						int32_t* OPJ_RESTRICT dest_ptr = buf + (y - y0) * (size_t)buf_line_stride + (x - x0) * buf_col_stride;
						if(x_incr == 1) {
							for(j = 0; j < y_incr; j++) {
								*dest_ptr = *src_ptr;
								dest_ptr += buf_line_stride;
								src_ptr += block_width;
							}
						}
						else if(y_incr == 1 && buf_col_stride == 2) {
							uint32_t k;
							for(k = 0; k < (x_incr & ~3U); k += 4) {
								dest_ptr[k * buf_col_stride] = src_ptr[k];
								dest_ptr[(k + 1) * buf_col_stride] = src_ptr[k + 1];
								dest_ptr[(k + 2) * buf_col_stride] = src_ptr[k + 2];
								dest_ptr[(k + 3) * buf_col_stride] = src_ptr[k + 3];
							}
							for(; k < x_incr; k++) {
								dest_ptr[k * buf_col_stride] = src_ptr[k];
							}
						}
						else if(x_incr >= 8 && buf_col_stride == 8) {
							for(j = 0; j < y_incr; j++) {
								uint32_t k;
								for(k = 0; k < (x_incr & ~3U); k += 4) {
									dest_ptr[k * buf_col_stride] = src_ptr[k];
									dest_ptr[(k + 1) * buf_col_stride] = src_ptr[k + 1];
									dest_ptr[(k + 2) * buf_col_stride] = src_ptr[k + 2];
									dest_ptr[(k + 3) * buf_col_stride] = src_ptr[k + 3];
								}
								for(; k < x_incr; k++) {
									dest_ptr[k * buf_col_stride] = src_ptr[k];
								}
								dest_ptr += buf_line_stride;
								src_ptr += block_width;
							}
						}
						else {
							/* General case */
							for(j = 0; j < y_incr; j++) {
								for(uint32_t k = 0; k < x_incr; k++) {
									dest_ptr[k * buf_col_stride] = src_ptr[k];
								}
								dest_ptr += buf_line_stride;
								src_ptr += block_width;
							}
						}
					}
				}
			}
			else {
				if(src_block == NULL) {
					src_block = (int32_t*)opj_calloc(1, (size_t)sa->block_width * sa->block_height * sizeof(int32_t));
					if(src_block == NULL) {
						return FALSE;
					}
					sa->data_blocks[block_y * sa->block_count_hor + block_x] = src_block;
				}
				if(buf_col_stride == 1) {
					int32_t* OPJ_RESTRICT dest_ptr = src_block + block_y_offset * (size_t)block_width + block_x_offset;
					const int32_t* OPJ_RESTRICT src_ptr = buf + (y - y0) * (size_t)buf_line_stride + (x - x0) * buf_col_stride;
					if(x_incr == 4) {
						/* Same code as general branch, but the compiler */
						/* can have an efficient memcpy() */
						(void)(x_incr); /* trick to silent cppcheck duplicateBranch warning */
						for(j = 0; j < y_incr; j++) {
							memcpy(dest_ptr, src_ptr, sizeof(int32_t) * x_incr);
							dest_ptr += block_width;
							src_ptr += buf_line_stride;
						}
					}
					else {
						for(j = 0; j < y_incr; j++) {
							memcpy(dest_ptr, src_ptr, sizeof(int32_t) * x_incr);
							dest_ptr += block_width;
							src_ptr += buf_line_stride;
						}
					}
				}
				else {
					int32_t* OPJ_RESTRICT dest_ptr = src_block + block_y_offset * (size_t)block_width + block_x_offset;
					const int32_t* OPJ_RESTRICT src_ptr = buf + (y - y0) * (size_t)buf_line_stride + (x - x0) * buf_col_stride;
					if(x_incr == 1) {
						for(j = 0; j < y_incr; j++) {
							*dest_ptr = *src_ptr;
							src_ptr += buf_line_stride;
							dest_ptr += block_width;
						}
					}
					else if(x_incr >= 8 && buf_col_stride == 8) {
						for(j = 0; j < y_incr; j++) {
							uint32_t k;
							for(k = 0; k < (x_incr & ~3U); k += 4) {
								dest_ptr[k] = src_ptr[k * buf_col_stride];
								dest_ptr[k + 1] = src_ptr[(k + 1) * buf_col_stride];
								dest_ptr[k + 2] = src_ptr[(k + 2) * buf_col_stride];
								dest_ptr[k + 3] = src_ptr[(k + 3) * buf_col_stride];
							}
							for(; k < x_incr; k++) {
								dest_ptr[k] = src_ptr[k * buf_col_stride];
							}
							src_ptr += buf_line_stride;
							dest_ptr += block_width;
						}
					}
					else {
						/* General case */
						for(j = 0; j < y_incr; j++) {
							for(uint32_t k = 0; k < x_incr; k++) {
								dest_ptr[k] = src_ptr[k * buf_col_stride];
							}
							src_ptr += buf_line_stride;
							dest_ptr += block_width;
						}
					}
				}
			}
		}
	}

	return TRUE;
}

boolint opj_sparse_array_int32_read(const opj_sparse_array_int32_t* sa, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, int32_t* dest, 
	uint32_t dest_col_stride, uint32_t dest_line_stride, boolint forgiving)
{
	return opj_sparse_array_int32_read_or_write((opj_sparse_array_int32_t*)sa, x0, y0, x1, y1, dest, dest_col_stride, dest_line_stride, forgiving, TRUE);
}

boolint opj_sparse_array_int32_write(opj_sparse_array_int32_t* sa, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, const int32_t* src, 
	uint32_t src_col_stride, uint32_t src_line_stride, boolint forgiving)
{
	return opj_sparse_array_int32_read_or_write(sa, x0, y0, x1, y1, (int32_t*)src, src_col_stride, src_line_stride, forgiving, FALSE);
}
