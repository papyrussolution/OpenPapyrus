/* -*- Mode: c; c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t; -*- */
/*
 * Copyright © 2000 SuSE, Inc.
 * Copyright © 2007 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */
#include "cairoint.h"
#pragma hdrstop
#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif
#undef IN
//#include <string.h>
//#include <stdlib.h>
//#include "pixman-private.h"
#include "pixman-combine32.h"
#include "pixman-inlines.h"

static force_inline uint32 fetch_24(uint8 * a)
{
	if(((uintptr_t)a) & 1) {
#ifdef WORDS_BIGENDIAN
		return (*a << 16) | (*(uint16 *)(a + 1));
#else
		return *a | (*(uint16 *)(a + 1) << 8);
#endif
	}
	else {
#ifdef WORDS_BIGENDIAN
		return (*(uint16 *)a << 8) | *(a + 2);
#else
		return *(uint16 *)a | (*(a + 2) << 16);
#endif
	}
}

static force_inline void store_24(uint8 * a,
    uint32 v)
{
	if(((uintptr_t)a) & 1) {
#ifdef WORDS_BIGENDIAN
		*a = (uint8)(v >> 16);
		*(uint16 *)(a + 1) = (uint16)(v);
#else
		*a = (uint8)(v);
		*(uint16 *)(a + 1) = (uint16)(v >> 8);
#endif
	}
	else {
#ifdef WORDS_BIGENDIAN
		*(uint16 *)a = (uint16)(v >> 8);
		*(a + 2) = (uint8)v;
#else
		*(uint16 *)a = (uint16)v;
		*(a + 2) = (uint8)(v >> 16);
#endif
	}
}

static force_inline uint32 over(uint32 src,
    uint32 dest)
{
	uint32 a = ~src >> 24;

	UN8x4_MUL_UN8_ADD_UN8x4(dest, a, src);

	return dest;
}

static force_inline uint32 in(uint32 x,
    uint8 y)
{
	uint16 a = y;

	UN8x4_MUL_UN8(x, a);

	return x;
}

/*
 * Naming convention:
 *
 *  op_src_mask_dest
 */
static void fast_composite_over_x888_8_8888(pixman_implementation_t * imp,
    pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint32 * src, * src_line;
	uint32 * dst, * dst_line;
	uint8 * mask, * mask_line;
	int src_stride, mask_stride, dst_stride;
	uint8 m;
	uint32 s, d;
	int32 w;

	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint32, dst_stride, dst_line, 1);
	PIXMAN_IMAGE_GET_LINE(mask_image, mask_x, mask_y, uint8, mask_stride, mask_line, 1);
	PIXMAN_IMAGE_GET_LINE(src_image, src_x, src_y, uint32, src_stride, src_line, 1);

	while(height--) {
		src = src_line;
		src_line += src_stride;
		dst = dst_line;
		dst_line += dst_stride;
		mask = mask_line;
		mask_line += mask_stride;

		w = width;
		while(w--) {
			m = *mask++;
			if(m) {
				s = *src | 0xff000000;

				if(m == 0xff) {
					*dst = s;
				}
				else {
					d = in(s, m);
					*dst = over(d, *dst);
				}
			}
			src++;
			dst++;
		}
	}
}

static void fast_composite_in_n_8_8(pixman_implementation_t * imp,
    pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint32 src, srca;
	uint8 * dst_line, * dst;
	uint8 * mask_line, * mask, m;
	int dst_stride, mask_stride;
	int32 w;
	uint16 t;

	src = _pixman_image_get_solid(imp, src_image, dest_image->bits.format);

	srca = src >> 24;

	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint8, dst_stride, dst_line, 1);
	PIXMAN_IMAGE_GET_LINE(mask_image, mask_x, mask_y, uint8, mask_stride, mask_line, 1);

	if(srca == 0xff) {
		while(height--) {
			dst = dst_line;
			dst_line += dst_stride;
			mask = mask_line;
			mask_line += mask_stride;
			w = width;

			while(w--) {
				m = *mask++;

				if(m == 0)
					*dst = 0;
				else if(m != 0xff)
					*dst = MUL_UN8(m, *dst, t);

				dst++;
			}
		}
	}
	else {
		while(height--) {
			dst = dst_line;
			dst_line += dst_stride;
			mask = mask_line;
			mask_line += mask_stride;
			w = width;

			while(w--) {
				m = *mask++;
				m = MUL_UN8(m, srca, t);

				if(m == 0)
					*dst = 0;
				else if(m != 0xff)
					*dst = MUL_UN8(m, *dst, t);

				dst++;
			}
		}
	}
}

static void fast_composite_in_8_8(pixman_implementation_t * imp,
    pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint8 * dst_line, * dst;
	uint8 * src_line, * src;
	int dst_stride, src_stride;
	int32 w;
	uint8 s;
	uint16 t;

	PIXMAN_IMAGE_GET_LINE(src_image, src_x, src_y, uint8, src_stride, src_line, 1);
	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint8, dst_stride, dst_line, 1);

	while(height--) {
		dst = dst_line;
		dst_line += dst_stride;
		src = src_line;
		src_line += src_stride;
		w = width;

		while(w--) {
			s = *src++;

			if(s == 0)
				*dst = 0;
			else if(s != 0xff)
				*dst = MUL_UN8(s, *dst, t);

			dst++;
		}
	}
}

static void fast_composite_over_n_8_8888(pixman_implementation_t * imp,
    pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint32 src, srca;
	uint32 * dst_line, * dst, d;
	uint8 * mask_line, * mask, m;
	int dst_stride, mask_stride;
	int32 w;

	src = _pixman_image_get_solid(imp, src_image, dest_image->bits.format);

	srca = src >> 24;
	if(src == 0)
		return;

	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint32, dst_stride, dst_line, 1);
	PIXMAN_IMAGE_GET_LINE(mask_image, mask_x, mask_y, uint8, mask_stride, mask_line, 1);

	while(height--) {
		dst = dst_line;
		dst_line += dst_stride;
		mask = mask_line;
		mask_line += mask_stride;
		w = width;

		while(w--) {
			m = *mask++;
			if(m == 0xff) {
				if(srca == 0xff)
					*dst = src;
				else
					*dst = over(src, *dst);
			}
			else if(m) {
				d = in(src, m);
				*dst = over(d, *dst);
			}
			dst++;
		}
	}
}

static void fast_composite_add_n_8888_8888_ca(pixman_implementation_t * imp,
    pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint32 src, s;
	uint32 * dst_line, * dst, d;
	uint32 * mask_line, * mask, ma;
	int dst_stride, mask_stride;
	int32 w;

	src = _pixman_image_get_solid(imp, src_image, dest_image->bits.format);

	if(src == 0)
		return;

	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint32, dst_stride, dst_line, 1);
	PIXMAN_IMAGE_GET_LINE(mask_image, mask_x, mask_y, uint32, mask_stride, mask_line, 1);

	while(height--) {
		dst = dst_line;
		dst_line += dst_stride;
		mask = mask_line;
		mask_line += mask_stride;
		w = width;

		while(w--) {
			ma = *mask++;

			if(ma) {
				d = *dst;
				s = src;

				UN8x4_MUL_UN8x4_ADD_UN8x4(s, ma, d);

				*dst = s;
			}

			dst++;
		}
	}
}

static void fast_composite_over_n_8888_8888_ca(pixman_implementation_t * imp,
    pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint32 src, srca, s;
	uint32 * dst_line, * dst, d;
	uint32 * mask_line, * mask, ma;
	int dst_stride, mask_stride;
	int32 w;

	src = _pixman_image_get_solid(imp, src_image, dest_image->bits.format);

	srca = src >> 24;
	if(src == 0)
		return;

	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint32, dst_stride, dst_line, 1);
	PIXMAN_IMAGE_GET_LINE(mask_image, mask_x, mask_y, uint32, mask_stride, mask_line, 1);

	while(height--) {
		dst = dst_line;
		dst_line += dst_stride;
		mask = mask_line;
		mask_line += mask_stride;
		w = width;

		while(w--) {
			ma = *mask++;
			if(ma == 0xffffffff) {
				if(srca == 0xff)
					*dst = src;
				else
					*dst = over(src, *dst);
			}
			else if(ma) {
				d = *dst;
				s = src;

				UN8x4_MUL_UN8x4(s, ma);
				UN8x4_MUL_UN8(ma, srca);
				ma = ~ma;
				UN8x4_MUL_UN8x4_ADD_UN8x4(d, ma, s);

				*dst = d;
			}

			dst++;
		}
	}
}

static void fast_composite_over_n_8_0888(pixman_implementation_t * imp,
    pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint32 src, srca;
	uint8 * dst_line, * dst;
	uint32 d;
	uint8 * mask_line, * mask, m;
	int dst_stride, mask_stride;
	int32 w;

	src = _pixman_image_get_solid(imp, src_image, dest_image->bits.format);

	srca = src >> 24;
	if(src == 0)
		return;

	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint8, dst_stride, dst_line, 3);
	PIXMAN_IMAGE_GET_LINE(mask_image, mask_x, mask_y, uint8, mask_stride, mask_line, 1);

	while(height--) {
		dst = dst_line;
		dst_line += dst_stride;
		mask = mask_line;
		mask_line += mask_stride;
		w = width;

		while(w--) {
			m = *mask++;
			if(m == 0xff) {
				if(srca == 0xff) {
					d = src;
				}
				else {
					d = fetch_24(dst);
					d = over(src, d);
				}
				store_24(dst, d);
			}
			else if(m) {
				d = over(in(src, m), fetch_24(dst));
				store_24(dst, d);
			}
			dst += 3;
		}
	}
}

static void fast_composite_over_n_8_0565(pixman_implementation_t * imp, pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint16 * dst_line, * dst;
	uint32 d;
	uint8 * mask_line, * mask, m;
	int dst_stride, mask_stride;
	int32 w;
	uint32 src = _pixman_image_get_solid(imp, src_image, dest_image->bits.format);
	uint32 srca = src >> 24;
	if(src == 0)
		return;
	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint16, dst_stride, dst_line, 1);
	PIXMAN_IMAGE_GET_LINE(mask_image, mask_x, mask_y, uint8, mask_stride, mask_line, 1);
	while(height--) {
		dst = dst_line;
		dst_line += dst_stride;
		mask = mask_line;
		mask_line += mask_stride;
		w = width;
		while(w--) {
			m = *mask++;
			if(m == 0xff) {
				if(srca == 0xff) {
					d = src;
				}
				else {
					d = *dst;
					d = over(src, convert_0565_to_0888(static_cast<uint16>(d)));
				}
				*dst = convert_8888_to_0565(d);
			}
			else if(m) {
				d = *dst;
				d = over(in(src, m), convert_0565_to_0888(static_cast<uint16>(d)));
				*dst = convert_8888_to_0565(d);
			}
			dst++;
		}
	}
}

static void fast_composite_over_n_8888_0565_ca(pixman_implementation_t * imp, pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint32 src, srca, s;
	uint16 src16;
	uint16 * dst_line, * dst;
	uint32 d;
	uint32 * mask_line, * mask, ma;
	int dst_stride, mask_stride;
	int32 w;

	src = _pixman_image_get_solid(imp, src_image, dest_image->bits.format);

	srca = src >> 24;
	if(src == 0)
		return;

	src16 = convert_8888_to_0565(src);

	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint16, dst_stride, dst_line, 1);
	PIXMAN_IMAGE_GET_LINE(mask_image, mask_x, mask_y, uint32, mask_stride, mask_line, 1);

	while(height--) {
		dst = dst_line;
		dst_line += dst_stride;
		mask = mask_line;
		mask_line += mask_stride;
		w = width;

		while(w--) {
			ma = *mask++;
			if(ma == 0xffffffff) {
				if(srca == 0xff) {
					*dst = src16;
				}
				else {
					d = *dst;
					d = over(src, convert_0565_to_0888(static_cast<uint16>(d)));
					*dst = convert_8888_to_0565(d);
				}
			}
			else if(ma) {
				d = *dst;
				d = convert_0565_to_0888(static_cast<uint16>(d));
				s = src;
				UN8x4_MUL_UN8x4(s, ma);
				UN8x4_MUL_UN8(ma, srca);
				ma = ~ma;
				UN8x4_MUL_UN8x4_ADD_UN8x4(d, ma, s);
				*dst = convert_8888_to_0565(d);
			}
			dst++;
		}
	}
}

static void fast_composite_over_8888_8888(pixman_implementation_t * imp,
    pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint32 * dst_line, * dst;
	uint32 * src_line, * src, s;
	int dst_stride, src_stride;
	uint8 a;
	int32 w;

	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint32, dst_stride, dst_line, 1);
	PIXMAN_IMAGE_GET_LINE(src_image, src_x, src_y, uint32, src_stride, src_line, 1);

	while(height--) {
		dst = dst_line;
		dst_line += dst_stride;
		src = src_line;
		src_line += src_stride;
		w = width;

		while(w--) {
			s = *src++;
			a = s >> 24;
			if(a == 0xff)
				*dst = s;
			else if(s)
				*dst = over(s, *dst);
			dst++;
		}
	}
}

static void fast_composite_src_x888_8888(pixman_implementation_t * imp,
    pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint32 * dst_line, * dst;
	uint32 * src_line, * src;
	int dst_stride, src_stride;
	int32 w;

	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint32, dst_stride, dst_line, 1);
	PIXMAN_IMAGE_GET_LINE(src_image, src_x, src_y, uint32, src_stride, src_line, 1);

	while(height--) {
		dst = dst_line;
		dst_line += dst_stride;
		src = src_line;
		src_line += src_stride;
		w = width;

		while(w--)
			*dst++ = (*src++) | 0xff000000;
	}
}

#if 0
static void fast_composite_over_8888_0888(pixman_implementation_t * imp,
    pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint8 * dst_line, * dst;
	uint32 d;
	uint32 * src_line, * src, s;
	uint8 a;
	int dst_stride, src_stride;
	int32 w;

	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint8, dst_stride, dst_line, 3);
	PIXMAN_IMAGE_GET_LINE(src_image, src_x, src_y, uint32, src_stride, src_line, 1);

	while(height--) {
		dst = dst_line;
		dst_line += dst_stride;
		src = src_line;
		src_line += src_stride;
		w = width;

		while(w--) {
			s = *src++;
			a = s >> 24;
			if(a) {
				if(a == 0xff)
					d = s;
				else
					d = over(s, fetch_24(dst));

				store_24(dst, d);
			}
			dst += 3;
		}
	}
}

#endif

static void fast_composite_over_8888_0565(pixman_implementation_t * imp,
    pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint16 * dst_line, * dst;
	uint32 d;
	uint32 * src_line, * src, s;
	uint8 a;
	int dst_stride, src_stride;
	int32 w;

	PIXMAN_IMAGE_GET_LINE(src_image, src_x, src_y, uint32, src_stride, src_line, 1);
	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint16, dst_stride, dst_line, 1);

	while(height--) {
		dst = dst_line;
		dst_line += dst_stride;
		src = src_line;
		src_line += src_stride;
		w = width;

		while(w--) {
			s = *src++;
			a = s >> 24;
			if(s) {
				if(a == 0xff) {
					d = s;
				}
				else {
					d = *dst;
					d = over(s, convert_0565_to_0888(static_cast<uint16>(d)));
				}
				*dst = convert_8888_to_0565(d);
			}
			dst++;
		}
	}
}

static void fast_composite_add_8_8(pixman_implementation_t * imp,
    pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint8 * dst_line, * dst;
	uint8 * src_line, * src;
	int dst_stride, src_stride;
	int32 w;
	uint8 s, d;
	uint16 t;

	PIXMAN_IMAGE_GET_LINE(src_image, src_x, src_y, uint8, src_stride, src_line, 1);
	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint8, dst_stride, dst_line, 1);

	while(height--) {
		dst = dst_line;
		dst_line += dst_stride;
		src = src_line;
		src_line += src_stride;
		w = width;

		while(w--) {
			s = *src++;
			if(s) {
				if(s != 0xff) {
					d = *dst;
					t = d + s;
					s = t | (0 - (t >> 8));
				}
				*dst = s;
			}
			dst++;
		}
	}
}

static void fast_composite_add_0565_0565(pixman_implementation_t * imp, pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint16 * dst_line, * dst;
	uint32 d;
	uint16 * src_line, * src;
	uint32 s;
	int dst_stride, src_stride;
	int32 w;
	PIXMAN_IMAGE_GET_LINE(src_image, src_x, src_y, uint16, src_stride, src_line, 1);
	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint16, dst_stride, dst_line, 1);
	while(height--) {
		dst = dst_line;
		dst_line += dst_stride;
		src = src_line;
		src_line += src_stride;
		w = width;
		while(w--) {
			s = *src++;
			if(s) {
				d = *dst;
				s = convert_0565_to_8888(static_cast<uint16>(s));
				if(d) {
					d = convert_0565_to_8888(static_cast<uint16>(d));
					UN8x4_ADD_UN8x4(s, d);
				}
				*dst = convert_8888_to_0565(s);
			}
			dst++;
		}
	}
}

static void fast_composite_add_8888_8888(pixman_implementation_t * imp,
    pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint32 * dst_line, * dst;
	uint32 * src_line, * src;
	int dst_stride, src_stride;
	int32 w;
	uint32 s, d;

	PIXMAN_IMAGE_GET_LINE(src_image, src_x, src_y, uint32, src_stride, src_line, 1);
	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint32, dst_stride, dst_line, 1);

	while(height--) {
		dst = dst_line;
		dst_line += dst_stride;
		src = src_line;
		src_line += src_stride;
		w = width;

		while(w--) {
			s = *src++;
			if(s) {
				if(s != 0xffffffff) {
					d = *dst;
					if(d)
						UN8x4_ADD_UN8x4(s, d);
				}
				*dst = s;
			}
			dst++;
		}
	}
}

static void fast_composite_add_n_8_8(pixman_implementation_t * imp, pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint8 * dst_line, * dst;
	uint8 * mask_line, * mask;
	int dst_stride, mask_stride;
	int32 w;
	uint32 src;
	uint8 sa;
	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint8, dst_stride, dst_line, 1);
	PIXMAN_IMAGE_GET_LINE(mask_image, mask_x, mask_y, uint8, mask_stride, mask_line, 1);
	src = _pixman_image_get_solid(imp, src_image, dest_image->bits.format);
	sa = (src >> 24);
	while(height--) {
		dst = dst_line;
		dst_line += dst_stride;
		mask = mask_line;
		mask_line += mask_stride;
		w = width;
		while(w--) {
			uint16 tmp;
			uint16 a = *mask++;
			uint32 d = *dst;
			uint32 m = MUL_UN8(sa, a, tmp);
			uint32 r = ADD_UN8(m, d, tmp);
			*dst++ = static_cast<uint8>(r);
		}
	}
}

#ifdef WORDS_BIGENDIAN
	#define CREATE_BITMASK(n) (0x80000000 >> (n))
	#define UPDATE_BITMASK(n) ((n) >> 1)
#else
	#define CREATE_BITMASK(n) (1 << (n))
	#define UPDATE_BITMASK(n) ((n) << 1)
#endif

#define TEST_BIT(p, n) (*((p) + ((n) >> 5)) & CREATE_BITMASK((n) & 31))
#define SET_BIT(p, n)  do { *((p) + ((n) >> 5)) |= CREATE_BITMASK((n) & 31); } while(0);

static void fast_composite_add_1_1(pixman_implementation_t * imp, pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint32 * dst_line, * dst;
	uint32 * src_line, * src;
	int dst_stride, src_stride;
	int32 w;
	PIXMAN_IMAGE_GET_LINE(src_image, 0, src_y, uint32, src_stride, src_line, 1);
	PIXMAN_IMAGE_GET_LINE(dest_image, 0, dest_y, uint32, dst_stride, dst_line, 1);
	while(height--) {
		dst = dst_line;
		dst_line += dst_stride;
		src = src_line;
		src_line += src_stride;
		w = width;
		while(w--) {
			/*
			 * TODO: improve performance by processing uint32 data instead
			 *  of individual bits
			 */
			if(TEST_BIT(src, src_x + w))
				SET_BIT(dst, dest_x + w);
		}
	}
}

static void fast_composite_over_n_1_8888(pixman_implementation_t * imp, pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint32 src, srca;
	uint32 * dst, * dst_line;
	uint32 * mask, * mask_line;
	int mask_stride, dst_stride;
	uint32 bitcache, bitmask;
	int32 w;
	if(width <= 0)
		return;
	src = _pixman_image_get_solid(imp, src_image, dest_image->bits.format);
	srca = src >> 24;
	if(src == 0)
		return;
	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint32, dst_stride, dst_line, 1);
	PIXMAN_IMAGE_GET_LINE(mask_image, 0, mask_y, uint32, mask_stride, mask_line, 1);
	mask_line += mask_x >> 5;
	if(srca == 0xff) {
		while(height--) {
			dst = dst_line;
			dst_line += dst_stride;
			mask = mask_line;
			mask_line += mask_stride;
			w = width;
			bitcache = *mask++;
			bitmask = CREATE_BITMASK(mask_x & 31);
			while(w--) {
				if(bitmask == 0) {
					bitcache = *mask++;
					bitmask = CREATE_BITMASK(0);
				}
				if(bitcache & bitmask)
					*dst = src;
				bitmask = UPDATE_BITMASK(bitmask);
				dst++;
			}
		}
	}
	else {
		while(height--) {
			dst = dst_line;
			dst_line += dst_stride;
			mask = mask_line;
			mask_line += mask_stride;
			w = width;
			bitcache = *mask++;
			bitmask = CREATE_BITMASK(mask_x & 31);
			while(w--) {
				if(bitmask == 0) {
					bitcache = *mask++;
					bitmask = CREATE_BITMASK(0);
				}
				if(bitcache & bitmask)
					*dst = over(src, *dst);
				bitmask = UPDATE_BITMASK(bitmask);
				dst++;
			}
		}
	}
}

static void fast_composite_over_n_1_0565(pixman_implementation_t * imp, pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint32 src, srca;
	uint16 * dst, * dst_line;
	uint32 * mask, * mask_line;
	int mask_stride, dst_stride;
	uint32 bitcache, bitmask;
	int32 w;
	uint32 d;
	uint16 src565;
	if(width <= 0)
		return;
	src = _pixman_image_get_solid(imp, src_image, dest_image->bits.format);
	srca = src >> 24;
	if(src == 0)
		return;
	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint16, dst_stride, dst_line, 1);
	PIXMAN_IMAGE_GET_LINE(mask_image, 0, mask_y, uint32, mask_stride, mask_line, 1);
	mask_line += mask_x >> 5;
	if(srca == 0xff) {
		src565 = convert_8888_to_0565(src);
		while(height--) {
			dst = dst_line;
			dst_line += dst_stride;
			mask = mask_line;
			mask_line += mask_stride;
			w = width;
			bitcache = *mask++;
			bitmask = CREATE_BITMASK(mask_x & 31);
			while(w--) {
				if(bitmask == 0) {
					bitcache = *mask++;
					bitmask = CREATE_BITMASK(0);
				}
				if(bitcache & bitmask)
					*dst = src565;
				bitmask = UPDATE_BITMASK(bitmask);
				dst++;
			}
		}
	}
	else {
		while(height--) {
			dst = dst_line;
			dst_line += dst_stride;
			mask = mask_line;
			mask_line += mask_stride;
			w = width;
			bitcache = *mask++;
			bitmask = CREATE_BITMASK(mask_x & 31);
			while(w--) {
				if(bitmask == 0) {
					bitcache = *mask++;
					bitmask = CREATE_BITMASK(0);
				}
				if(bitcache & bitmask) {
					d = over(src, convert_0565_to_0888(*dst));
					*dst = convert_8888_to_0565(d);
				}
				bitmask = UPDATE_BITMASK(bitmask);
				dst++;
			}
		}
	}
}
/*
 * Simple bitblt
 */
static void fast_composite_solid_fill(pixman_implementation_t * imp, pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint32 src = _pixman_image_get_solid(imp, src_image, dest_image->bits.format);
	if(dest_image->bits.format == PIXMAN_a1) {
		src = src >> 31;
	}
	else if(dest_image->bits.format == PIXMAN_a8) {
		src = src >> 24;
	}
	else if(dest_image->bits.format == PIXMAN_r5g6b5 || dest_image->bits.format == PIXMAN_b5g6r5) {
		src = convert_8888_to_0565(src);
	}
	pixman_fill(dest_image->bits.bits, dest_image->bits.rowstride, PIXMAN_FORMAT_BPP(dest_image->bits.format), dest_x, dest_y, width, height, src);
}

static void fast_composite_src_memcpy(pixman_implementation_t * imp, pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	int bpp = PIXMAN_FORMAT_BPP(dest_image->bits.format) / 8;
	uint32 n_bytes = width * bpp;
	const int src_stride = src_image->bits.rowstride * 4;
	const int dst_stride = dest_image->bits.rowstride * 4;
	const uint8 * src = (const uint8 *)src_image->bits.bits + src_y * src_stride + src_x * bpp;
	uint8 * dst = (uint8 *)dest_image->bits.bits + dest_y * dst_stride + dest_x * bpp;
	while(height--) {
		memcpy(dst, src, n_bytes);
		dst += dst_stride;
		src += src_stride;
	}
}

FAST_NEAREST(8888_8888_cover, 8888, 8888, uint32, uint32, SRC, COVER)
FAST_NEAREST(8888_8888_none, 8888, 8888, uint32, uint32, SRC, NONE)
FAST_NEAREST(8888_8888_pad, 8888, 8888, uint32, uint32, SRC, PAD)
FAST_NEAREST(8888_8888_normal, 8888, 8888, uint32, uint32, SRC, NORMAL)
FAST_NEAREST(x888_8888_cover, x888, 8888, uint32, uint32, SRC, COVER)
FAST_NEAREST(x888_8888_pad, x888, 8888, uint32, uint32, SRC, PAD)
FAST_NEAREST(x888_8888_normal, x888, 8888, uint32, uint32, SRC, NORMAL)
FAST_NEAREST(8888_8888_cover, 8888, 8888, uint32, uint32, OVER, COVER)
FAST_NEAREST(8888_8888_none, 8888, 8888, uint32, uint32, OVER, NONE)
FAST_NEAREST(8888_8888_pad, 8888, 8888, uint32, uint32, OVER, PAD)
FAST_NEAREST(8888_8888_normal, 8888, 8888, uint32, uint32, OVER, NORMAL)
FAST_NEAREST(8888_565_cover, 8888, 0565, uint32, uint16, SRC, COVER)
FAST_NEAREST(8888_565_none, 8888, 0565, uint32, uint16, SRC, NONE)
FAST_NEAREST(8888_565_pad, 8888, 0565, uint32, uint16, SRC, PAD)
FAST_NEAREST(8888_565_normal, 8888, 0565, uint32, uint16, SRC, NORMAL)
FAST_NEAREST(565_565_normal, 0565, 0565, uint16, uint16, SRC, NORMAL)
FAST_NEAREST(8888_565_cover, 8888, 0565, uint32, uint16, OVER, COVER)
FAST_NEAREST(8888_565_none, 8888, 0565, uint32, uint16, OVER, NONE)
FAST_NEAREST(8888_565_pad, 8888, 0565, uint32, uint16, OVER, PAD)
FAST_NEAREST(8888_565_normal, 8888, 0565, uint32, uint16, OVER, NORMAL)

#define REPEAT_MIN_WIDTH    32

static void fast_composite_tiled_repeat(pixman_implementation_t * imp, pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	pixman_composite_func_t func;
	pixman_format_code_t mask_format;
	uint32 src_flags, mask_flags;
	int32 sx, sy;
	int32 width_remain;
	int32 num_pixels;
	int32 src_width;
	int32 i, j;
	pixman_image_t extended_src_image;
	uint32 extended_src[REPEAT_MIN_WIDTH * 2];
	pixman_bool_t need_src_extension;
	uint32 * src_line;
	int32 src_stride;
	int32 src_bpp;
	pixman_composite_info_t info2 = *info;
	src_flags = (info->src_flags & ~FAST_PATH_NORMAL_REPEAT) | FAST_PATH_SAMPLES_COVER_CLIP_NEAREST;
	if(mask_image) {
		mask_format = mask_image->common.extended_format_code;
		mask_flags = info->mask_flags;
	}
	else {
		mask_format = PIXMAN_null;
		mask_flags = FAST_PATH_IS_OPAQUE;
	}
	_pixman_implementation_lookup_composite(imp->toplevel, info->op,
		src_image->common.extended_format_code, src_flags,
		mask_format, mask_flags,
		dest_image->common.extended_format_code, info->dest_flags,
		&imp, &func);
	src_bpp = PIXMAN_FORMAT_BPP(src_image->bits.format);
	if(src_image->bits.width < REPEAT_MIN_WIDTH && (src_bpp == 32 || src_bpp == 16 || src_bpp == 8) && !src_image->bits.indexed) {
		sx = src_x;
		sx = MOD(sx, src_image->bits.width);
		sx += width;
		src_width = 0;

		while(src_width < REPEAT_MIN_WIDTH && src_width <= sx)
			src_width += src_image->bits.width;

		src_stride = (src_width * (src_bpp >> 3) + 3) / (int)sizeof(uint32);

		/* Initialize/validate stack-allocated temporary image */
		_pixman_bits_image_init(&extended_src_image, src_image->bits.format,
		    src_width, 1, &extended_src[0], src_stride,
		    FALSE);
		_pixman_image_validate(&extended_src_image);

		info2.src_image = &extended_src_image;
		need_src_extension = TRUE;
	}
	else {
		src_width = src_image->bits.width;
		need_src_extension = FALSE;
	}

	sx = src_x;
	sy = src_y;

	while(--height >= 0) {
		sx = MOD(sx, src_width);
		sy = MOD(sy, src_image->bits.height);

		if(need_src_extension) {
			if(src_bpp == 32) {
				PIXMAN_IMAGE_GET_LINE(src_image, 0, sy, uint32, src_stride, src_line, 1);

				for(i = 0; i < src_width;) {
					for(j = 0; j < src_image->bits.width; j++, i++)
						extended_src[i] = src_line[j];
				}
			}
			else if(src_bpp == 16) {
				uint16 * src_line_16;

				PIXMAN_IMAGE_GET_LINE(src_image, 0, sy, uint16, src_stride,
				    src_line_16, 1);
				src_line = (uint32 *)src_line_16;

				for(i = 0; i < src_width;) {
					for(j = 0; j < src_image->bits.width; j++, i++)
						((uint16 *)extended_src)[i] = ((uint16 *)src_line)[j];
				}
			}
			else if(src_bpp == 8) {
				uint8 * src_line_8;

				PIXMAN_IMAGE_GET_LINE(src_image, 0, sy, uint8, src_stride,
				    src_line_8, 1);
				src_line = (uint32 *)src_line_8;

				for(i = 0; i < src_width;) {
					for(j = 0; j < src_image->bits.width; j++, i++)
						((uint8 *)extended_src)[i] = ((uint8 *)src_line)[j];
				}
			}

			info2.src_y = 0;
		}
		else {
			info2.src_y = sy;
		}

		width_remain = width;

		while(width_remain > 0) {
			num_pixels = src_width - sx;

			if(num_pixels > width_remain)
				num_pixels = width_remain;

			info2.src_x = sx;
			info2.width = num_pixels;
			info2.height = 1;

			func(imp, &info2);

			width_remain -= num_pixels;
			info2.mask_x += num_pixels;
			info2.dest_x += num_pixels;
			sx = 0;
		}

		sx = src_x;
		sy++;
		info2.mask_x = info->mask_x;
		info2.mask_y++;
		info2.dest_x = info->dest_x;
		info2.dest_y++;
	}

	if(need_src_extension)
		_pixman_image_fini(&extended_src_image);
}

/* Use more unrolling for src_0565_0565 because it is typically CPU bound */
static force_inline void scaled_nearest_scanline_565_565_SRC(uint16 *  dst,
    const uint16 * src,
    int32 w,
    pixman_fixed_t vx,
    pixman_fixed_t unit_x,
    pixman_fixed_t max_vx,
    pixman_bool_t fully_transparent_src)
{
	uint16 tmp1, tmp2, tmp3, tmp4;
	while((w -= 4) >= 0) {
		tmp1 = *(src + pixman_fixed_to_int(vx));
		vx += unit_x;
		tmp2 = *(src + pixman_fixed_to_int(vx));
		vx += unit_x;
		tmp3 = *(src + pixman_fixed_to_int(vx));
		vx += unit_x;
		tmp4 = *(src + pixman_fixed_to_int(vx));
		vx += unit_x;
		*dst++ = tmp1;
		*dst++ = tmp2;
		*dst++ = tmp3;
		*dst++ = tmp4;
	}
	if(w & 2) {
		tmp1 = *(src + pixman_fixed_to_int(vx));
		vx += unit_x;
		tmp2 = *(src + pixman_fixed_to_int(vx));
		vx += unit_x;
		*dst++ = tmp1;
		*dst++ = tmp2;
	}
	if(w & 1)
		*dst = *(src + pixman_fixed_to_int(vx));
}

FAST_NEAREST_MAINLOOP(565_565_cover_SRC, scaled_nearest_scanline_565_565_SRC, uint16, uint16, COVER)
FAST_NEAREST_MAINLOOP(565_565_none_SRC, scaled_nearest_scanline_565_565_SRC, uint16, uint16, NONE)
FAST_NEAREST_MAINLOOP(565_565_pad_SRC, scaled_nearest_scanline_565_565_SRC, uint16, uint16, PAD)

static force_inline uint32 fetch_nearest(pixman_repeat_t src_repeat, pixman_format_code_t format, uint32 * src, int32 x, int src_width)
{
	if(repeat(src_repeat, &x, src_width)) {
		if(format == PIXMAN_x8r8g8b8 || format == PIXMAN_x8b8g8r8)
			return *(src + x) | 0xff000000;
		else
			return *(src + x);
	}
	else {
		return 0;
	}
}

static force_inline void combine_over(uint32 s, uint32 * dst)
{
	if(s) {
		uint8 ia = 0xff - (s >> 24);

		if(ia)
			UN8x4_MUL_UN8_ADD_UN8x4(*dst, ia, s);
		else
			*dst = s;
	}
}

static force_inline void combine_src(uint32 s, uint32 * dst)
{
	*dst = s;
}

static void fast_composite_scaled_nearest(pixman_implementation_t * imp,
    pixman_composite_info_t * info)
{
	PIXMAN_COMPOSITE_ARGS(info);
	uint32 * dst_line;
	uint32 * src_line;
	int dst_stride, src_stride;
	int src_width, src_height;
	pixman_repeat_t src_repeat;
	pixman_fixed_t unit_x, unit_y;
	pixman_format_code_t src_format;
	pixman_vector_t v;
	pixman_fixed_t vy;

	PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, uint32, dst_stride, dst_line, 1);
	/* pass in 0 instead of src_x and src_y because src_x and src_y need to be
	 * transformed from destination space to source space
	 */
	PIXMAN_IMAGE_GET_LINE(src_image, 0, 0, uint32, src_stride, src_line, 1);

	/* reference point is the center of the pixel */
	v.vector[0] = pixman_int_to_fixed(src_x) + pixman_fixed_1 / 2;
	v.vector[1] = pixman_int_to_fixed(src_y) + pixman_fixed_1 / 2;
	v.vector[2] = pixman_fixed_1;

	if(!pixman_transform_point_3d(src_image->common.transform, &v))
		return;

	unit_x = src_image->common.transform->matrix[0][0];
	unit_y = src_image->common.transform->matrix[1][1];

	/* Round down to closest integer, ensuring that 0.5 rounds to 0, not 1 */
	v.vector[0] -= pixman_fixed_e;
	v.vector[1] -= pixman_fixed_e;

	src_height = src_image->bits.height;
	src_width = src_image->bits.width;
	src_repeat = src_image->common.repeat;
	src_format = src_image->bits.format;

	vy = v.vector[1];
	while(height--) {
		pixman_fixed_t vx = v.vector[0];
		int32 y = pixman_fixed_to_int(vy);
		uint32 * dst = dst_line;
		dst_line += dst_stride;
		/* adjust the y location by a unit vector in the y direction
		 * this is equivalent to transforming y+1 of the destination point to source space */
		vy += unit_y;
		if(!repeat(src_repeat, &y, src_height)) {
			if(op == PIXMAN_OP_SRC)
				memzero(dst, sizeof(*dst) * width);
		}
		else {
			int w = width;
			uint32 * src = src_line + y * src_stride;
			while(w >= 2) {
				uint32 s1, s2;
				int x1, x2;
				x1 = pixman_fixed_to_int(vx);
				vx += unit_x;
				x2 = pixman_fixed_to_int(vx);
				vx += unit_x;
				w -= 2;
				s1 = fetch_nearest(src_repeat, src_format, src, x1, src_width);
				s2 = fetch_nearest(src_repeat, src_format, src, x2, src_width);
				if(op == PIXMAN_OP_OVER) {
					combine_over(s1, dst++);
					combine_over(s2, dst++);
				}
				else {
					combine_src(s1, dst++);
					combine_src(s2, dst++);
				}
			}
			while(w--) {
				uint32 s;
				int x;

				x = pixman_fixed_to_int(vx);
				vx += unit_x;

				s = fetch_nearest(src_repeat, src_format, src, x, src_width);

				if(op == PIXMAN_OP_OVER)
					combine_over(s, dst++);
				else
					combine_src(s, dst++);
			}
		}
	}
}

#define CACHE_LINE_SIZE 64

#define FAST_SIMPLE_ROTATE(suffix, pix_type)                                  \
                                                                              \
	static void                                                    \
	blt_rotated_90_trivial_ ## suffix(pix_type       *dst,                         \
	    int dst_stride,                  \
	    const pix_type *src,                         \
	    int src_stride,                  \
	    int w,                           \
	    int h)                           \
	{                                                                             \
		int x, y;                                                                 \
		for(y = 0; y < h; y++)                                                   \
		{                                                                         \
			const pix_type * s = src + (h - y - 1);                                \
			pix_type * d = dst + dst_stride * y;                                   \
			for(x = 0; x < w; x++)                                               \
			{                                                                     \
				*d++ = *s;                                                        \
				s += src_stride;                                                  \
			}                                                                     \
		}                                                                         \
	}                                                                             \
                                                                              \
	static void                                                    \
	    blt_rotated_270_trivial_ ## suffix(pix_type       *dst,                        \
	    int dst_stride,                 \
	    const pix_type *src,                        \
	    int src_stride,                 \
	    int w,                          \
	    int h)                          \
	{                                                                             \
		int x, y;                                                                 \
		for(y = 0; y < h; y++)                                                   \
		{                                                                         \
			const pix_type * s = src + src_stride * (w - 1) + y;                   \
			pix_type * d = dst + dst_stride * y;                                   \
			for(x = 0; x < w; x++)                                               \
			{                                                                     \
				*d++ = *s;                                                        \
				s -= src_stride;                                                  \
			}                                                                     \
		}                                                                         \
	}                                                                             \
                                                                              \
	static void                                                    \
	    blt_rotated_90_ ## suffix(pix_type       *dst,                                 \
	    int dst_stride,                          \
	    const pix_type *src,                                 \
	    int src_stride,                          \
	    int W,                                   \
	    int H)                                   \
	{                                                                             \
		int x;                                                                    \
		int leading_pixels = 0, trailing_pixels = 0;                              \
		const int TILE_SIZE = CACHE_LINE_SIZE / sizeof(pix_type);                 \
                                                                              \
		/* \
		 * split processing into handling destination as TILE_SIZExH cache line \
		 * aligned vertical stripes (optimistically assuming that destination \
		 * stride is a multiple of cache line, if not - it will be just a bit \
		 * slower) \
		 */                                                           \
                                                                              \
		if((uintptr_t)dst & (CACHE_LINE_SIZE - 1))                               \
		{                                                                         \
			leading_pixels = TILE_SIZE - (((uintptr_t)dst &                       \
			    (CACHE_LINE_SIZE - 1)) / sizeof(pix_type));       \
			if(leading_pixels > W)                                               \
				leading_pixels = W;                                               \
                                                                              \
			/* unaligned leading part NxH (where N < TILE_SIZE) */                \
			blt_rotated_90_trivial_ ## suffix(                                     \
				dst,                                                              \
				dst_stride,                                                       \
				src,                                                              \
				src_stride,                                                       \
				leading_pixels,                                                   \
				H);                                                               \
                                                                              \
			dst += leading_pixels;                                                \
			src += leading_pixels * src_stride;                                   \
			W -= leading_pixels;                                                  \
		}                                                                         \
                                                                              \
		if((uintptr_t)(dst + W) & (CACHE_LINE_SIZE - 1))                         \
		{                                                                         \
			trailing_pixels = (((uintptr_t)(dst + W) &                            \
			    (CACHE_LINE_SIZE - 1)) / sizeof(pix_type));       \
			if(trailing_pixels > W)                                              \
				trailing_pixels = W;                                              \
			W -= trailing_pixels;                                                 \
		}                                                                         \
                                                                              \
		for(x = 0; x < W; x += TILE_SIZE)                                        \
		{                                                                         \
			/* aligned middle part TILE_SIZExH */                                 \
			blt_rotated_90_trivial_ ## suffix(                                     \
				dst + x,                                                          \
				dst_stride,                                                       \
				src + src_stride * x,                                             \
				src_stride,                                                       \
				TILE_SIZE,                                                        \
				H);                                                               \
		}                                                                         \
                                                                              \
		if(trailing_pixels)                                                      \
		{                                                                         \
			/* unaligned trailing part NxH (where N < TILE_SIZE) */               \
			blt_rotated_90_trivial_ ## suffix(                                     \
				dst + W,                                                          \
				dst_stride,                                                       \
				src + W * src_stride,                                             \
				src_stride,                                                       \
				trailing_pixels,                                                  \
				H);                                                               \
		}                                                                         \
	}                                                                             \
                                                                              \
	static void                                                    \
	    blt_rotated_270_ ## suffix(pix_type       *dst,                                \
	    int dst_stride,                         \
	    const pix_type *src,                                \
	    int src_stride,                         \
	    int W,                                  \
	    int H)                                  \
	{                                                                             \
		int x;                                                                    \
		int leading_pixels = 0, trailing_pixels = 0;                              \
		const int TILE_SIZE = CACHE_LINE_SIZE / sizeof(pix_type);                 \
                                                                              \
		/* \
		 * split processing into handling destination as TILE_SIZExH cache line \
		 * aligned vertical stripes (optimistically assuming that destination \
		 * stride is a multiple of cache line, if not - it will be just a bit \
		 * slower) \
		 */                                                           \
                                                                              \
		if((uintptr_t)dst & (CACHE_LINE_SIZE - 1))                               \
		{                                                                         \
			leading_pixels = TILE_SIZE - (((uintptr_t)dst &                       \
			    (CACHE_LINE_SIZE - 1)) / sizeof(pix_type));       \
			if(leading_pixels > W)                                               \
				leading_pixels = W;                                               \
                                                                              \
			/* unaligned leading part NxH (where N < TILE_SIZE) */                \
			blt_rotated_270_trivial_ ## suffix(                                    \
				dst,                                                              \
				dst_stride,                                                       \
				src + src_stride * (W - leading_pixels),                          \
				src_stride,                                                       \
				leading_pixels,                                                   \
				H);                                                               \
                                                                              \
			dst += leading_pixels;                                                \
			W -= leading_pixels;                                                  \
		}                                                                         \
                                                                              \
		if((uintptr_t)(dst + W) & (CACHE_LINE_SIZE - 1))                         \
		{                                                                         \
			trailing_pixels = (((uintptr_t)(dst + W) &                            \
			    (CACHE_LINE_SIZE - 1)) / sizeof(pix_type));       \
			if(trailing_pixels > W)                                              \
				trailing_pixels = W;                                              \
			W -= trailing_pixels;                                                 \
			src += trailing_pixels * src_stride;                                  \
		}                                                                         \
                                                                              \
		for(x = 0; x < W; x += TILE_SIZE)                                        \
		{                                                                         \
			/* aligned middle part TILE_SIZExH */                                 \
			blt_rotated_270_trivial_ ## suffix(                                    \
				dst + x,                                                          \
				dst_stride,                                                       \
				src + src_stride * (W - x - TILE_SIZE),                           \
				src_stride,                                                       \
				TILE_SIZE,                                                        \
				H);                                                               \
		}                                                                         \
                                                                              \
		if(trailing_pixels)                                                      \
		{                                                                         \
			/* unaligned trailing part NxH (where N < TILE_SIZE) */               \
			blt_rotated_270_trivial_ ## suffix(                                    \
				dst + W,                                                          \
				dst_stride,                                                       \
				src - trailing_pixels * src_stride,                               \
				src_stride,                                                       \
				trailing_pixels,                                                  \
				H);                                                               \
		}                                                                         \
	}                                                                             \
                                                                              \
	static void                                                    \
	    fast_composite_rotate_90_ ## suffix(pixman_implementation_t *imp,              \
	    pixman_composite_info_t *info)             \
	{                                                                             \
		PIXMAN_COMPOSITE_ARGS(info);                                             \
		pix_type * dst_line;                                                 \
		pix_type * src_line;                                                 \
		int dst_stride, src_stride;                                   \
		int src_x_t, src_y_t;                                         \
                                                                              \
		PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, pix_type,              \
		    dst_stride, dst_line, 1);                          \
		src_x_t = -src_y + pixman_fixed_to_int(                                  \
			src_image->common.transform->matrix[0][2] +   \
			pixman_fixed_1 / 2 - pixman_fixed_e) - height; \
		src_y_t = src_x + pixman_fixed_to_int(                                   \
			src_image->common.transform->matrix[1][2] +   \
			pixman_fixed_1 / 2 - pixman_fixed_e);         \
		PIXMAN_IMAGE_GET_LINE(src_image, src_x_t, src_y_t, pix_type,             \
		    src_stride, src_line, 1);                          \
		blt_rotated_90_ ## suffix(dst_line, dst_stride, src_line, src_stride,      \
		    width, height);                                  \
	}                                                                             \
                                                                              \
	static void                                                    \
	    fast_composite_rotate_270_ ## suffix(pixman_implementation_t *imp,             \
	    pixman_composite_info_t *info)            \
	{                                                                             \
		PIXMAN_COMPOSITE_ARGS(info);                                             \
		pix_type * dst_line;                                                 \
		pix_type * src_line;                                                 \
		int dst_stride, src_stride;                                   \
		int src_x_t, src_y_t;                                         \
                                                                              \
		PIXMAN_IMAGE_GET_LINE(dest_image, dest_x, dest_y, pix_type,              \
		    dst_stride, dst_line, 1);                          \
		src_x_t = src_y + pixman_fixed_to_int(                                   \
			src_image->common.transform->matrix[0][2] +   \
			pixman_fixed_1 / 2 - pixman_fixed_e);         \
		src_y_t = -src_x + pixman_fixed_to_int(                                  \
			src_image->common.transform->matrix[1][2] +   \
			pixman_fixed_1 / 2 - pixman_fixed_e) - width; \
		PIXMAN_IMAGE_GET_LINE(src_image, src_x_t, src_y_t, pix_type,             \
		    src_stride, src_line, 1);                          \
		blt_rotated_270_ ## suffix(dst_line, dst_stride, src_line, src_stride,     \
		    width, height);                                 \
	}

FAST_SIMPLE_ROTATE(8, uint8)
FAST_SIMPLE_ROTATE(565, uint16)
FAST_SIMPLE_ROTATE(8888, uint32)

static const pixman_fast_path_t c_fast_paths[] =
{
	PIXMAN_STD_FAST_PATH(OVER, solid, a8, r5g6b5, fast_composite_over_n_8_0565),
	PIXMAN_STD_FAST_PATH(OVER, solid, a8, b5g6r5, fast_composite_over_n_8_0565),
	PIXMAN_STD_FAST_PATH(OVER, solid, a8, r8g8b8, fast_composite_over_n_8_0888),
	PIXMAN_STD_FAST_PATH(OVER, solid, a8, b8g8r8, fast_composite_over_n_8_0888),
	PIXMAN_STD_FAST_PATH(OVER, solid, a8, a8r8g8b8, fast_composite_over_n_8_8888),
	PIXMAN_STD_FAST_PATH(OVER, solid, a8, x8r8g8b8, fast_composite_over_n_8_8888),
	PIXMAN_STD_FAST_PATH(OVER, solid, a8, a8b8g8r8, fast_composite_over_n_8_8888),
	PIXMAN_STD_FAST_PATH(OVER, solid, a8, x8b8g8r8, fast_composite_over_n_8_8888),
	PIXMAN_STD_FAST_PATH(OVER, solid, a1, a8r8g8b8, fast_composite_over_n_1_8888),
	PIXMAN_STD_FAST_PATH(OVER, solid, a1, x8r8g8b8, fast_composite_over_n_1_8888),
	PIXMAN_STD_FAST_PATH(OVER, solid, a1, a8b8g8r8, fast_composite_over_n_1_8888),
	PIXMAN_STD_FAST_PATH(OVER, solid, a1, x8b8g8r8, fast_composite_over_n_1_8888),
	PIXMAN_STD_FAST_PATH(OVER, solid, a1, r5g6b5,   fast_composite_over_n_1_0565),
	PIXMAN_STD_FAST_PATH(OVER, solid, a1, b5g6r5,   fast_composite_over_n_1_0565),
	PIXMAN_STD_FAST_PATH_CA(OVER, solid, a8r8g8b8, a8r8g8b8, fast_composite_over_n_8888_8888_ca),
	PIXMAN_STD_FAST_PATH_CA(OVER, solid, a8r8g8b8, x8r8g8b8, fast_composite_over_n_8888_8888_ca),
	PIXMAN_STD_FAST_PATH_CA(OVER, solid, a8r8g8b8, r5g6b5, fast_composite_over_n_8888_0565_ca),
	PIXMAN_STD_FAST_PATH_CA(OVER, solid, a8b8g8r8, a8b8g8r8, fast_composite_over_n_8888_8888_ca),
	PIXMAN_STD_FAST_PATH_CA(OVER, solid, a8b8g8r8, x8b8g8r8, fast_composite_over_n_8888_8888_ca),
	PIXMAN_STD_FAST_PATH_CA(OVER, solid, a8b8g8r8, b5g6r5, fast_composite_over_n_8888_0565_ca),
	PIXMAN_STD_FAST_PATH(OVER, x8r8g8b8, a8, x8r8g8b8, fast_composite_over_x888_8_8888),
	PIXMAN_STD_FAST_PATH(OVER, x8r8g8b8, a8, a8r8g8b8, fast_composite_over_x888_8_8888),
	PIXMAN_STD_FAST_PATH(OVER, x8b8g8r8, a8, x8b8g8r8, fast_composite_over_x888_8_8888),
	PIXMAN_STD_FAST_PATH(OVER, x8b8g8r8, a8, a8b8g8r8, fast_composite_over_x888_8_8888),
	PIXMAN_STD_FAST_PATH(OVER, a8r8g8b8, null, a8r8g8b8, fast_composite_over_8888_8888),
	PIXMAN_STD_FAST_PATH(OVER, a8r8g8b8, null, x8r8g8b8, fast_composite_over_8888_8888),
	PIXMAN_STD_FAST_PATH(OVER, a8r8g8b8, null, r5g6b5, fast_composite_over_8888_0565),
	PIXMAN_STD_FAST_PATH(OVER, a8b8g8r8, null, a8b8g8r8, fast_composite_over_8888_8888),
	PIXMAN_STD_FAST_PATH(OVER, a8b8g8r8, null, x8b8g8r8, fast_composite_over_8888_8888),
	PIXMAN_STD_FAST_PATH(OVER, a8b8g8r8, null, b5g6r5, fast_composite_over_8888_0565),
	PIXMAN_STD_FAST_PATH(ADD, r5g6b5, null, r5g6b5, fast_composite_add_0565_0565),
	PIXMAN_STD_FAST_PATH(ADD, b5g6r5, null, b5g6r5, fast_composite_add_0565_0565),
	PIXMAN_STD_FAST_PATH(ADD, a8r8g8b8, null, a8r8g8b8, fast_composite_add_8888_8888),
	PIXMAN_STD_FAST_PATH(ADD, a8b8g8r8, null, a8b8g8r8, fast_composite_add_8888_8888),
	PIXMAN_STD_FAST_PATH(ADD, a8, null, a8, fast_composite_add_8_8),
	PIXMAN_STD_FAST_PATH(ADD, a1, null, a1, fast_composite_add_1_1),
	PIXMAN_STD_FAST_PATH_CA(ADD, solid, a8r8g8b8, a8r8g8b8, fast_composite_add_n_8888_8888_ca),
	PIXMAN_STD_FAST_PATH(ADD, solid, a8, a8, fast_composite_add_n_8_8),
	PIXMAN_STD_FAST_PATH(SRC, solid, null, a8r8g8b8, fast_composite_solid_fill),
	PIXMAN_STD_FAST_PATH(SRC, solid, null, x8r8g8b8, fast_composite_solid_fill),
	PIXMAN_STD_FAST_PATH(SRC, solid, null, a8b8g8r8, fast_composite_solid_fill),
	PIXMAN_STD_FAST_PATH(SRC, solid, null, x8b8g8r8, fast_composite_solid_fill),
	PIXMAN_STD_FAST_PATH(SRC, solid, null, a1, fast_composite_solid_fill),
	PIXMAN_STD_FAST_PATH(SRC, solid, null, a8, fast_composite_solid_fill),
	PIXMAN_STD_FAST_PATH(SRC, solid, null, r5g6b5, fast_composite_solid_fill),
	PIXMAN_STD_FAST_PATH(SRC, x8r8g8b8, null, a8r8g8b8, fast_composite_src_x888_8888),
	PIXMAN_STD_FAST_PATH(SRC, x8b8g8r8, null, a8b8g8r8, fast_composite_src_x888_8888),
	PIXMAN_STD_FAST_PATH(SRC, a8r8g8b8, null, x8r8g8b8, fast_composite_src_memcpy),
	PIXMAN_STD_FAST_PATH(SRC, a8r8g8b8, null, a8r8g8b8, fast_composite_src_memcpy),
	PIXMAN_STD_FAST_PATH(SRC, x8r8g8b8, null, x8r8g8b8, fast_composite_src_memcpy),
	PIXMAN_STD_FAST_PATH(SRC, a8b8g8r8, null, x8b8g8r8, fast_composite_src_memcpy),
	PIXMAN_STD_FAST_PATH(SRC, a8b8g8r8, null, a8b8g8r8, fast_composite_src_memcpy),
	PIXMAN_STD_FAST_PATH(SRC, x8b8g8r8, null, x8b8g8r8, fast_composite_src_memcpy),
	PIXMAN_STD_FAST_PATH(SRC, b8g8r8a8, null, b8g8r8x8, fast_composite_src_memcpy),
	PIXMAN_STD_FAST_PATH(SRC, b8g8r8a8, null, b8g8r8a8, fast_composite_src_memcpy),
	PIXMAN_STD_FAST_PATH(SRC, b8g8r8x8, null, b8g8r8x8, fast_composite_src_memcpy),
	PIXMAN_STD_FAST_PATH(SRC, r5g6b5, null, r5g6b5, fast_composite_src_memcpy),
	PIXMAN_STD_FAST_PATH(SRC, b5g6r5, null, b5g6r5, fast_composite_src_memcpy),
	PIXMAN_STD_FAST_PATH(SRC, r8g8b8, null, r8g8b8, fast_composite_src_memcpy),
	PIXMAN_STD_FAST_PATH(SRC, b8g8r8, null, b8g8r8, fast_composite_src_memcpy),
	PIXMAN_STD_FAST_PATH(SRC, x1r5g5b5, null, x1r5g5b5, fast_composite_src_memcpy),
	PIXMAN_STD_FAST_PATH(SRC, a1r5g5b5, null, x1r5g5b5, fast_composite_src_memcpy),
	PIXMAN_STD_FAST_PATH(SRC, a8, null, a8, fast_composite_src_memcpy),
	PIXMAN_STD_FAST_PATH(IN, a8, null, a8, fast_composite_in_8_8),
	PIXMAN_STD_FAST_PATH(IN, solid, a8, a8, fast_composite_in_n_8_8),

	SIMPLE_NEAREST_FAST_PATH(SRC, x8r8g8b8, x8r8g8b8, 8888_8888),
	SIMPLE_NEAREST_FAST_PATH(SRC, a8r8g8b8, x8r8g8b8, 8888_8888),
	SIMPLE_NEAREST_FAST_PATH(SRC, x8b8g8r8, x8b8g8r8, 8888_8888),
	SIMPLE_NEAREST_FAST_PATH(SRC, a8b8g8r8, x8b8g8r8, 8888_8888),

	SIMPLE_NEAREST_FAST_PATH(SRC, a8r8g8b8, a8r8g8b8, 8888_8888),
	SIMPLE_NEAREST_FAST_PATH(SRC, a8b8g8r8, a8b8g8r8, 8888_8888),

	SIMPLE_NEAREST_FAST_PATH(SRC, x8r8g8b8, r5g6b5, 8888_565),
	SIMPLE_NEAREST_FAST_PATH(SRC, a8r8g8b8, r5g6b5, 8888_565),

	SIMPLE_NEAREST_FAST_PATH(SRC, r5g6b5, r5g6b5, 565_565),

	SIMPLE_NEAREST_FAST_PATH_COVER(SRC, x8r8g8b8, a8r8g8b8, x888_8888),
	SIMPLE_NEAREST_FAST_PATH_COVER(SRC, x8b8g8r8, a8b8g8r8, x888_8888),
	SIMPLE_NEAREST_FAST_PATH_PAD(SRC, x8r8g8b8, a8r8g8b8, x888_8888),
	SIMPLE_NEAREST_FAST_PATH_PAD(SRC, x8b8g8r8, a8b8g8r8, x888_8888),
	SIMPLE_NEAREST_FAST_PATH_NORMAL(SRC, x8r8g8b8, a8r8g8b8, x888_8888),
	SIMPLE_NEAREST_FAST_PATH_NORMAL(SRC, x8b8g8r8, a8b8g8r8, x888_8888),

	SIMPLE_NEAREST_FAST_PATH(OVER, a8r8g8b8, x8r8g8b8, 8888_8888),
	SIMPLE_NEAREST_FAST_PATH(OVER, a8b8g8r8, x8b8g8r8, 8888_8888),
	SIMPLE_NEAREST_FAST_PATH(OVER, a8r8g8b8, a8r8g8b8, 8888_8888),
	SIMPLE_NEAREST_FAST_PATH(OVER, a8b8g8r8, a8b8g8r8, 8888_8888),

	SIMPLE_NEAREST_FAST_PATH(OVER, a8r8g8b8, r5g6b5, 8888_565),

#define NEAREST_FAST_PATH(op, s, d)               \
	{   PIXMAN_OP_ ## op,                       \
	    PIXMAN_ ## s, SCALED_NEAREST_FLAGS,     \
	    PIXMAN_null, 0,                         \
	    PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS, \
	    fast_composite_scaled_nearest,          \
	}

	NEAREST_FAST_PATH(SRC, x8r8g8b8, x8r8g8b8),
	NEAREST_FAST_PATH(SRC, a8r8g8b8, x8r8g8b8),
	NEAREST_FAST_PATH(SRC, x8b8g8r8, x8b8g8r8),
	NEAREST_FAST_PATH(SRC, a8b8g8r8, x8b8g8r8),

	NEAREST_FAST_PATH(SRC, x8r8g8b8, a8r8g8b8),
	NEAREST_FAST_PATH(SRC, a8r8g8b8, a8r8g8b8),
	NEAREST_FAST_PATH(SRC, x8b8g8r8, a8b8g8r8),
	NEAREST_FAST_PATH(SRC, a8b8g8r8, a8b8g8r8),

	NEAREST_FAST_PATH(OVER, x8r8g8b8, x8r8g8b8),
	NEAREST_FAST_PATH(OVER, a8r8g8b8, x8r8g8b8),
	NEAREST_FAST_PATH(OVER, x8b8g8r8, x8b8g8r8),
	NEAREST_FAST_PATH(OVER, a8b8g8r8, x8b8g8r8),

	NEAREST_FAST_PATH(OVER, x8r8g8b8, a8r8g8b8),
	NEAREST_FAST_PATH(OVER, a8r8g8b8, a8r8g8b8),
	NEAREST_FAST_PATH(OVER, x8b8g8r8, a8b8g8r8),
	NEAREST_FAST_PATH(OVER, a8b8g8r8, a8b8g8r8),

#define SIMPLE_ROTATE_FLAGS(angle)                                        \
	(FAST_PATH_ROTATE_ ## angle ## _TRANSFORM   |                         \
	FAST_PATH_NEAREST_FILTER                   |                         \
	FAST_PATH_SAMPLES_COVER_CLIP_NEAREST       |                         \
	FAST_PATH_STANDARD_FLAGS)

#define SIMPLE_ROTATE_FAST_PATH(op, s, d, suffix)                            \
	{   PIXMAN_OP_ ## op,                                                 \
	    PIXMAN_ ## s, SIMPLE_ROTATE_FLAGS(90),                           \
	    PIXMAN_null, 0,                                                   \
	    PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,                           \
	    fast_composite_rotate_90_ ## suffix,                                \
	},                                                                    \
	{   PIXMAN_OP_ ## op,                                                 \
	    PIXMAN_ ## s, SIMPLE_ROTATE_FLAGS(270),                          \
	    PIXMAN_null, 0,                                                   \
	    PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,                           \
	    fast_composite_rotate_270_ ## suffix,                               \
	}

	SIMPLE_ROTATE_FAST_PATH(SRC, a8r8g8b8, a8r8g8b8, 8888),
	SIMPLE_ROTATE_FAST_PATH(SRC, a8r8g8b8, x8r8g8b8, 8888),
	SIMPLE_ROTATE_FAST_PATH(SRC, x8r8g8b8, x8r8g8b8, 8888),
	SIMPLE_ROTATE_FAST_PATH(SRC, r5g6b5, r5g6b5, 565),
	SIMPLE_ROTATE_FAST_PATH(SRC, a8, a8, 8),

	/* Simple repeat fast path entry. */
	{   PIXMAN_OP_any,
	    PIXMAN_any,
	    (FAST_PATH_STANDARD_FLAGS | FAST_PATH_ID_TRANSFORM | FAST_PATH_BITS_IMAGE |
	    FAST_PATH_NORMAL_REPEAT),
	    PIXMAN_any, 0,
	    PIXMAN_any, FAST_PATH_STD_DEST_FLAGS,
	    fast_composite_tiled_repeat},

	{   PIXMAN_OP_NONE  },
};

#ifdef WORDS_BIGENDIAN
#define A1_FILL_MASK(n, offs) (((1U << (n)) - 1) << (32 - (offs) - (n)))
#else
#define A1_FILL_MASK(n, offs) (((1U << (n)) - 1) << (offs))
#endif

static force_inline void pixman_fill1_line(uint32 * dst, int offs, int width, int v)
{
	if(offs) {
		int leading_pixels = 32 - offs;
		if(leading_pixels >= width) {
			if(v)
				*dst |= A1_FILL_MASK(width, offs);
			else
				*dst &= ~A1_FILL_MASK (width, offs);
			return;
		}
		else {
			if(v)
				*dst++ |= A1_FILL_MASK(leading_pixels, offs);
			else
				*dst++ &= ~A1_FILL_MASK (leading_pixels, offs);
			width -= leading_pixels;
		}
	}
	while(width >= 32) {
		if(v)
			*dst++ = 0xFFFFFFFF;
		else
			*dst++ = 0;
		width -= 32;
	}
	if(width > 0) {
		if(v)
			*dst |= A1_FILL_MASK(width, 0);
		else
			*dst &= ~A1_FILL_MASK (width, 0);
	}
}

static void pixman_fill1(uint32 * bits,
    int stride,
    int x,
    int y,
    int width,
    int height,
    uint32 filler)
{
	uint32 * dst = bits + y * stride + (x >> 5);
	int offs = x & 31;

	if(filler & 1) {
		while(height--) {
			pixman_fill1_line(dst, offs, width, 1);
			dst += stride;
		}
	}
	else {
		while(height--) {
			pixman_fill1_line(dst, offs, width, 0);
			dst += stride;
		}
	}
}

static void pixman_fill8(uint32 * bits,
    int stride,
    int x,
    int y,
    int width,
    int height,
    uint32 filler)
{
	int byte_stride = stride * (int)sizeof(uint32);
	uint8 * dst = (uint8 *)bits;
	uint8 v = filler & 0xff;
	int i;

	dst = dst + y * byte_stride + x;

	while(height--) {
		for(i = 0; i < width; ++i)
			dst[i] = v;

		dst += byte_stride;
	}
}

static void pixman_fill16(uint32 * bits,
    int stride,
    int x,
    int y,
    int width,
    int height,
    uint32 filler)
{
	int short_stride =
	    (stride * (int)sizeof(uint32)) / (int)sizeof(uint16);
	uint16 * dst = (uint16 *)bits;
	uint16 v = filler & 0xffff;
	int i;

	dst = dst + y * short_stride + x;

	while(height--) {
		for(i = 0; i < width; ++i)
			dst[i] = v;

		dst += short_stride;
	}
}

static void pixman_fill32(uint32 * bits,
    int stride,
    int x,
    int y,
    int width,
    int height,
    uint32 filler)
{
	int i;

	bits = bits + y * stride + x;

	while(height--) {
		for(i = 0; i < width; ++i)
			bits[i] = filler;

		bits += stride;
	}
}

static pixman_bool_t fast_path_fill(pixman_implementation_t * imp,
    uint32 * bits,
    int stride,
    int bpp,
    int x,
    int y,
    int width,
    int height,
    uint32 filler)
{
	switch(bpp)
	{
		case 1:
		    pixman_fill1(bits, stride, x, y, width, height, filler);
		    break;

		case 8:
		    pixman_fill8(bits, stride, x, y, width, height, filler);
		    break;

		case 16:
		    pixman_fill16(bits, stride, x, y, width, height, filler);
		    break;

		case 32:
		    pixman_fill32(bits, stride, x, y, width, height, filler);
		    break;

		default:
		    return FALSE;
	}

	return TRUE;
}

// 

static uint32 * fast_fetch_r5g6b5(pixman_iter_t * iter, const uint32 * mask)
{
	int32 w = iter->width;
	uint32 * dst = iter->buffer;
	const uint16 * src = (const uint16*)iter->bits;

	iter->bits += iter->stride;

	/* Align the source buffer at 4 bytes boundary */
	if(w > 0 && ((uintptr_t)src & 3)) {
		*dst++ = convert_0565_to_8888(*src++);
		w--;
	}
	/* Process two pixels per iteration */
	while((w -= 2) >= 0) {
		uint32 sr, sb, sg, t0, t1;
		uint32 s = *(const uint32*)src;
		src += 2;
		sr = (s >> 8) & 0x00F800F8;
		sb = (s << 3) & 0x00F800F8;
		sg = (s >> 3) & 0x00FC00FC;
		sr |= sr >> 5;
		sb |= sb >> 5;
		sg |= sg >> 6;
		t0 = ((sr << 16) & 0x00FF0000) | ((sg << 8) & 0x0000FF00) |
		    (sb & 0xFF) | 0xFF000000;
		t1 = (sr & 0x00FF0000) | ((sg >> 8) & 0x0000FF00) |
		    (sb >> 16) | 0xFF000000;
#ifdef WORDS_BIGENDIAN
		*dst++ = t1;
		*dst++ = t0;
#else
		*dst++ = t0;
		*dst++ = t1;
#endif
	}
	if(w & 1) {
		*dst = convert_0565_to_8888(*src);
	}

	return iter->buffer;
}

static uint32 * fast_dest_fetch_noop(pixman_iter_t * iter, const uint32 * mask)
{
	iter->bits += iter->stride;
	return iter->buffer;
}

/* Helper function for a workaround, which tries to ensure that 0x1F001F
 * constant is always allocated in a register on RISC architectures.
 */
static force_inline uint32 convert_8888_to_0565_workaround(uint32 s, uint32 x1F001F)
{
	uint32 a, b;
	a = (s >> 3) & x1F001F;
	b = s & 0xFC00;
	a |= a >> 5;
	a |= b >> 5;
	return a;
}

static void fast_write_back_r5g6b5(pixman_iter_t * iter)
{
	int32 w = iter->width;
	uint16 * dst = (uint16 *)(iter->bits - iter->stride);
	const uint32 * src = iter->buffer;
	/* Workaround to ensure that x1F001F variable is allocated in a register */
	static volatile uint32 volatile_x1F001F = 0x1F001F;
	uint32 x1F001F = volatile_x1F001F;
	while((w -= 4) >= 0) {
		uint32 s1 = *src++;
		uint32 s2 = *src++;
		uint32 s3 = *src++;
		uint32 s4 = *src++;
		*dst++ = static_cast<uint16>(convert_8888_to_0565_workaround(s1, x1F001F));
		*dst++ = static_cast<uint16>(convert_8888_to_0565_workaround(s2, x1F001F));
		*dst++ = static_cast<uint16>(convert_8888_to_0565_workaround(s3, x1F001F));
		*dst++ = static_cast<uint16>(convert_8888_to_0565_workaround(s4, x1F001F));
	}
	if(w & 2) {
		*dst++ = static_cast<uint16>(convert_8888_to_0565_workaround(*src++, x1F001F));
		*dst++ = static_cast<uint16>(convert_8888_to_0565_workaround(*src++, x1F001F));
	}
	if(w & 1) {
		*dst = static_cast<uint16>(convert_8888_to_0565_workaround(*src, x1F001F));
	}
}

typedef struct {
	int y;
	uint64 *  buffer;
} line_t;

typedef struct {
	line_t lines[2];
	pixman_fixed_t y;
	pixman_fixed_t x;
	uint64 data[1];
} bilinear_info_t;

static void fetch_horizontal(bits_image_t * image, line_t * line,
    int y, pixman_fixed_t x, pixman_fixed_t ux, int n)
{
	uint32 * bits = image->bits + y * image->rowstride;
	int i;

	for(i = 0; i < n; ++i) {
		int x0 = pixman_fixed_to_int(x);
		int x1 = x0 + 1;
		int32 dist_x;

		uint32 left = *(bits + x0);
		uint32 right = *(bits + x1);

		dist_x = pixman_fixed_to_bilinear_weight(x);
		dist_x <<= (8 - BILINEAR_INTERPOLATION_BITS);

#if SIZEOF_LONG <= 4
		{
			uint32 lag, rag, ag;
			uint32 lrb, rrb, rb;

			lag = (left & 0xff00ff00) >> 8;
			rag = (right & 0xff00ff00) >> 8;
			ag = (lag << 8) + dist_x * (rag - lag);

			lrb = (left & 0x00ff00ff);
			rrb = (right & 0x00ff00ff);
			rb = (lrb << 8) + dist_x * (rrb - lrb);

			*((uint32 *)(line->buffer + i)) = ag;
			*((uint32 *)(line->buffer + i) + 1) = rb;
		}
#else
		{
			uint64 lagrb, ragrb;
			uint32 lag, rag;
			uint32 lrb, rrb;

			lag = (left & 0xff00ff00);
			lrb = (left & 0x00ff00ff);
			rag = (right & 0xff00ff00);
			rrb = (right & 0x00ff00ff);
			lagrb = (((uint64)lag) << 24) | lrb;
			ragrb = (((uint64)rag) << 24) | rrb;

			line->buffer[i] = (lagrb << 8) + dist_x * (ragrb - lagrb);
		}
#endif

		x += ux;
	}

	line->y = y;
}

static uint32 * fast_fetch_bilinear_cover(pixman_iter_t * iter, const uint32 * mask)
{
	pixman_fixed_t fx, ux;
	bilinear_info_t * info = (bilinear_info_t *)iter->data;
	line_t * line0, * line1;
	int y0, y1;
	int32 dist_y;
	int i;
	COMPILE_TIME_ASSERT(BILINEAR_INTERPOLATION_BITS < 8);
	fx = info->x;
	ux = iter->image->common.transform->matrix[0][0];
	y0 = pixman_fixed_to_int(info->y);
	y1 = y0 + 1;
	dist_y = pixman_fixed_to_bilinear_weight(info->y);
	dist_y <<= (8 - BILINEAR_INTERPOLATION_BITS);
	line0 = &info->lines[y0 & 0x01];
	line1 = &info->lines[y1 & 0x01];
	if(line0->y != y0) {
		fetch_horizontal(&iter->image->bits, line0, y0, fx, ux, iter->width);
	}
	if(line1->y != y1) {
		fetch_horizontal(&iter->image->bits, line1, y1, fx, ux, iter->width);
	}
	for(i = 0; i < iter->width; ++i) {
#if SIZEOF_LONG <= 4
		uint32 ta, tr, tg, tb;
		uint32 ba, br, bg, bb;
		uint32 tag, trb;
		uint32 bag, brb;
		uint32 a, r, g, b;

		tag = *((uint32 *)(line0->buffer + i));
		trb = *((uint32 *)(line0->buffer + i) + 1);
		bag = *((uint32 *)(line1->buffer + i));
		brb = *((uint32 *)(line1->buffer + i) + 1);

		ta = tag >> 16;
		ba = bag >> 16;
		a = (ta << 8) + dist_y * (ba - ta);

		tr = trb >> 16;
		br = brb >> 16;
		r = (tr << 8) + dist_y * (br - tr);

		tg = tag & 0xffff;
		bg = bag & 0xffff;
		g = (tg << 8) + dist_y * (bg - tg);

		tb = trb & 0xffff;
		bb = brb & 0xffff;
		b = (tb << 8) + dist_y * (bb - tb);

		a = (a <<  8) & 0xff000000;
		r = (r <<  0) & 0x00ff0000;
		g = (g >>  8) & 0x0000ff00;
		b = (b >> 16) & 0x000000ff;
#else
		uint64 top = line0->buffer[i];
		uint64 bot = line1->buffer[i];
		uint64 tar = (top & 0xffff0000ffff0000ULL) >> 16;
		uint64 bar = (bot & 0xffff0000ffff0000ULL) >> 16;
		uint64 tgb = (top & 0x0000ffff0000ffffULL);
		uint64 bgb = (bot & 0x0000ffff0000ffffULL);
		uint64 ar, gb;
		uint32 a, r, g, b;

		ar = (tar << 8) + dist_y * (bar - tar);
		gb = (tgb << 8) + dist_y * (bgb - tgb);

		a = ((ar >> 24) & 0xff000000);
		r = ((ar >>  0) & 0x00ff0000);
		g = ((gb >> 40) & 0x0000ff00);
		b = ((gb >> 16) & 0x000000ff);
#endif

		iter->buffer[i] = a | r | g | b;
	}

	info->y += iter->image->common.transform->matrix[1][1];

	return iter->buffer;
}

static void bilinear_cover_iter_fini(pixman_iter_t * iter)
{
	SAlloc::F(iter->data);
}

static void fast_bilinear_cover_iter_init(pixman_iter_t * iter, const pixman_iter_info_t * iter_info)
{
	int width = iter->width;
	bilinear_info_t * info;
	pixman_vector_t v;

	/* Reference point is the center of the pixel */
	v.vector[0] = pixman_int_to_fixed(iter->x) + pixman_fixed_1 / 2;
	v.vector[1] = pixman_int_to_fixed(iter->y) + pixman_fixed_1 / 2;
	v.vector[2] = pixman_fixed_1;
	if(!pixman_transform_point_3d(iter->image->common.transform, &v))
		goto fail;
	info = (bilinear_info_t *)SAlloc::M(sizeof(*info) + (2 * width - 1) * sizeof(uint64));
	if(!info)
		goto fail;

	info->x = v.vector[0] - pixman_fixed_1 / 2;
	info->y = v.vector[1] - pixman_fixed_1 / 2;

	/* It is safe to set the y coordinates to -1 initially
	 * because COVER_CLIP_BILINEAR ensures that we will only
	 * be asked to fetch lines in the [0, height) interval
	 */
	info->lines[0].y = -1;
	info->lines[0].buffer = &(info->data[0]);
	info->lines[1].y = -1;
	info->lines[1].buffer = &(info->data[width]);

	iter->get_scanline = fast_fetch_bilinear_cover;
	iter->fini = bilinear_cover_iter_fini;

	iter->data = info;
	return;

fail:
	/* Something went wrong, either a bad matrix or OOM; in such cases,
	 * we don't guarantee any particular rendering.
	 */
	_pixman_log_error(FUNC, "Allocation failure or bad matrix, skipping rendering\n");
	iter->get_scanline = _pixman_iter_get_scanline_noop;
	iter->fini = NULL;
}

static uint32 * bits_image_fetch_bilinear_no_repeat_8888(pixman_iter_t * iter, const uint32 * mask)
{
	pixman_image_t * ima = iter->image;
	int offset = iter->x;
	int line = iter->y++;
	int width = iter->width;
	uint32 *  buffer = iter->buffer;
	bits_image_t * bits = &ima->bits;
	pixman_fixed_t x_top, x_bottom, x;
	pixman_fixed_t ux_top, ux_bottom, ux;
	pixman_vector_t v;
	uint32 top_mask, bottom_mask;
	uint32 * top_row;
	uint32 * bottom_row;
	uint32 * end;
	uint32 zero[2] = { 0, 0 };
	uint32 one = 1;
	int y, y1, y2;
	int disty;
	int mask_inc;
	int w;

	/* reference point is the center of the pixel */
	v.vector[0] = pixman_int_to_fixed(offset) + pixman_fixed_1 / 2;
	v.vector[1] = pixman_int_to_fixed(line) + pixman_fixed_1 / 2;
	v.vector[2] = pixman_fixed_1;

	if(!pixman_transform_point_3d(bits->common.transform, &v))
		return iter->buffer;

	ux = ux_top = ux_bottom = bits->common.transform->matrix[0][0];
	x = x_top = x_bottom = v.vector[0] - pixman_fixed_1/2;

	y = v.vector[1] - pixman_fixed_1/2;
	disty = pixman_fixed_to_bilinear_weight(y);

	/* Load the pointers to the first and second lines from the source
	 * image that bilinear code must read.
	 *
	 * The main trick in this code is about the check if any line are
	 * outside of the image;
	 *
	 * When I realize that a line (any one) is outside, I change
	 * the pointer to a dummy area with zeros. Once I change this, I
	 * must be sure the pointer will not change, so I set the
	 * variables to each pointer increments inside the loop.
	 */
	y1 = pixman_fixed_to_int(y);
	y2 = y1 + 1;

	if(y1 < 0 || y1 >= bits->height) {
		top_row = zero;
		x_top = 0;
		ux_top = 0;
	}
	else {
		top_row = bits->bits + y1 * bits->rowstride;
		x_top = x;
		ux_top = ux;
	}

	if(y2 < 0 || y2 >= bits->height) {
		bottom_row = zero;
		x_bottom = 0;
		ux_bottom = 0;
	}
	else {
		bottom_row = bits->bits + y2 * bits->rowstride;
		x_bottom = x;
		ux_bottom = ux;
	}

	/* Instead of checking whether the operation uses the mast in
	 * each loop iteration, verify this only once and prepare the
	 * variables to make the code smaller inside the loop.
	 */
	if(!mask) {
		mask_inc = 0;
		mask = &one;
	}
	else {
		/* If have a mask, prepare the variables to check it */
		mask_inc = 1;
	}
	/* If both are zero, then the whole thing is zero */
	if(top_row == zero && bottom_row == zero) {
		memzero(buffer, width * sizeof(uint32));
		return iter->buffer;
	}
	else if(bits->format == PIXMAN_x8r8g8b8) {
		if(top_row == zero) {
			top_mask = 0;
			bottom_mask = 0xff000000;
		}
		else if(bottom_row == zero) {
			top_mask = 0xff000000;
			bottom_mask = 0;
		}
		else {
			top_mask = 0xff000000;
			bottom_mask = 0xff000000;
		}
	}
	else {
		top_mask = 0;
		bottom_mask = 0;
	}

	end = buffer + width;

	/* Zero fill to the left of the image */
	while(buffer < end && x < pixman_fixed_minus_1) {
		*buffer++ = 0;
		x += ux;
		x_top += ux_top;
		x_bottom += ux_bottom;
		mask += mask_inc;
	}

	/* Left edge
	 */
	while(buffer < end && x < 0) {
		uint32 tr, br;
		int32 distx;

		tr = top_row[pixman_fixed_to_int(x_top) + 1] | top_mask;
		br = bottom_row[pixman_fixed_to_int(x_bottom) + 1] | bottom_mask;

		distx = pixman_fixed_to_bilinear_weight(x);

		*buffer++ = bilinear_interpolation(0, tr, 0, br, distx, disty);

		x += ux;
		x_top += ux_top;
		x_bottom += ux_bottom;
		mask += mask_inc;
	}

	/* Main part */
	w = pixman_int_to_fixed(bits->width - 1);

	while(buffer < end &&  x < w) {
		if(*mask) {
			uint32 tl, tr, bl, br;
			int32 distx;

			tl = top_row [pixman_fixed_to_int(x_top)] | top_mask;
			tr = top_row [pixman_fixed_to_int(x_top) + 1] | top_mask;
			bl = bottom_row [pixman_fixed_to_int(x_bottom)] | bottom_mask;
			br = bottom_row [pixman_fixed_to_int(x_bottom) + 1] | bottom_mask;

			distx = pixman_fixed_to_bilinear_weight(x);

			*buffer = bilinear_interpolation(tl, tr, bl, br, distx, disty);
		}

		buffer++;
		x += ux;
		x_top += ux_top;
		x_bottom += ux_bottom;
		mask += mask_inc;
	}

	/* Right Edge */
	w = pixman_int_to_fixed(bits->width);
	while(buffer < end &&  x < w) {
		if(*mask) {
			uint32 tl, bl;
			int32 distx;

			tl = top_row [pixman_fixed_to_int(x_top)] | top_mask;
			bl = bottom_row [pixman_fixed_to_int(x_bottom)] | bottom_mask;

			distx = pixman_fixed_to_bilinear_weight(x);

			*buffer = bilinear_interpolation(tl, 0, bl, 0, distx, disty);
		}

		buffer++;
		x += ux;
		x_top += ux_top;
		x_bottom += ux_bottom;
		mask += mask_inc;
	}
	/* Zero fill to the left of the image */
	while(buffer < end)
		*buffer++ = 0;
	return iter->buffer;
}

typedef uint32 (* convert_pixel_t) (const uint8 * row, int x);

static force_inline void bits_image_fetch_separable_convolution_affine(pixman_image_t * image,
    int offset, int line, int width, uint32 *  buffer, const uint32 * mask,
    convert_pixel_t convert_pixel, pixman_format_code_t format, pixman_repeat_t repeat_mode)
{
	bits_image_t * bits = &image->bits;
	pixman_fixed_t * params = image->common.filter_params;
	int cwidth = pixman_fixed_to_int(params[0]);
	int cheight = pixman_fixed_to_int(params[1]);
	int x_off = ((cwidth << 16) - pixman_fixed_1) >> 1;
	int y_off = ((cheight << 16) - pixman_fixed_1) >> 1;
	int x_phase_bits = pixman_fixed_to_int(params[2]);
	int y_phase_bits = pixman_fixed_to_int(params[3]);
	int x_phase_shift = 16 - x_phase_bits;
	int y_phase_shift = 16 - y_phase_bits;
	pixman_fixed_t vx, vy;
	pixman_fixed_t ux, uy;
	pixman_vector_t v;
	int k;

	/* reference point is the center of the pixel */
	v.vector[0] = pixman_int_to_fixed(offset) + pixman_fixed_1 / 2;
	v.vector[1] = pixman_int_to_fixed(line) + pixman_fixed_1 / 2;
	v.vector[2] = pixman_fixed_1;

	if(!pixman_transform_point_3d(image->common.transform, &v))
		return;

	ux = image->common.transform->matrix[0][0];
	uy = image->common.transform->matrix[1][0];

	vx = v.vector[0];
	vy = v.vector[1];

	for(k = 0; k < width; ++k) {
		pixman_fixed_t * y_params;
		int satot, srtot, sgtot, sbtot;
		pixman_fixed_t x, y;
		int32 x1, x2, y1, y2;
		int32 px, py;
		int i, j;

		if(mask && !mask[k])
			goto next;

		/* Round x and y to the middle of the closest phase before continuing. This
		 * ensures that the convolution matrix is aligned right, since it was
		 * positioned relative to a particular phase (and not relative to whatever
		 * exact fraction we happen to get here).
		 */
		x = ((vx >> x_phase_shift) << x_phase_shift) + ((1 << x_phase_shift) >> 1);
		y = ((vy >> y_phase_shift) << y_phase_shift) + ((1 << y_phase_shift) >> 1);
		px = (x & 0xffff) >> x_phase_shift;
		py = (y & 0xffff) >> y_phase_shift;
		x1 = pixman_fixed_to_int(x - pixman_fixed_e - x_off);
		y1 = pixman_fixed_to_int(y - pixman_fixed_e - y_off);
		x2 = x1 + cwidth;
		y2 = y1 + cheight;
		satot = srtot = sgtot = sbtot = 0;
		y_params = params + 4 + (1 << x_phase_bits) * cwidth + py * cheight;
		for(i = y1; i < y2; ++i) {
			pixman_fixed_t fy = *y_params++;
			if(fy) {
				pixman_fixed_t * x_params = params + 4 + px * cwidth;
				for(j = x1; j < x2; ++j) {
					pixman_fixed_t fx = *x_params++;
					int32 rx = j;
					int32 ry = i;
					if(fx) {
						pixman_fixed_t f;
						uint32 pixel;
						uint8 * row;
						uint32 mask = PIXMAN_FORMAT_A(format) ? 0 : 0xff000000;
						if(repeat_mode != PIXMAN_REPEAT_NONE) {
							repeat(repeat_mode, &rx, bits->width);
							repeat(repeat_mode, &ry, bits->height);
							row = (uint8 *)bits->bits + bits->rowstride * 4 * ry;
							pixel = convert_pixel(row, rx) | mask;
						}
						else {
							if(rx < 0 || ry < 0 || rx >= bits->width || ry >= bits->height) {
								pixel = 0;
							}
							else {
								row = (uint8 *)bits->bits + bits->rowstride * 4 * ry;
								pixel = convert_pixel(row, rx) | mask;
							}
						}

						f = ((pixman_fixed_32_32_t)fx * fy + 0x8000) >> 16;
						srtot += (int)RED_8(pixel) * f;
						sgtot += (int)GREEN_8(pixel) * f;
						sbtot += (int)BLUE_8(pixel) * f;
						satot += (int)ALPHA_8(pixel) * f;
					}
				}
			}
		}

		satot = (satot + 0x8000) >> 16;
		srtot = (srtot + 0x8000) >> 16;
		sgtot = (sgtot + 0x8000) >> 16;
		sbtot = (sbtot + 0x8000) >> 16;

		satot = CLIP(satot, 0, 0xff);
		srtot = CLIP(srtot, 0, 0xff);
		sgtot = CLIP(sgtot, 0, 0xff);
		sbtot = CLIP(sbtot, 0, 0xff);

		buffer[k] = (satot << 24) | (srtot << 16) | (sgtot << 8) | (sbtot << 0);

next:
		vx += ux;
		vy += uy;
	}
}

static const uint8 zero[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

static force_inline void bits_image_fetch_bilinear_affine(pixman_image_t * image,
    int offset,
    int line,
    int width,
    uint32 *  buffer,
    const uint32 * mask,

    convert_pixel_t convert_pixel,
    pixman_format_code_t format,
    pixman_repeat_t repeat_mode)
{
	pixman_fixed_t x, y;
	pixman_fixed_t ux, uy;
	pixman_vector_t v;
	bits_image_t * bits = &image->bits;
	int i;

	/* reference point is the center of the pixel */
	v.vector[0] = pixman_int_to_fixed(offset) + pixman_fixed_1 / 2;
	v.vector[1] = pixman_int_to_fixed(line) + pixman_fixed_1 / 2;
	v.vector[2] = pixman_fixed_1;

	if(!pixman_transform_point_3d(image->common.transform, &v))
		return;

	ux = image->common.transform->matrix[0][0];
	uy = image->common.transform->matrix[1][0];

	x = v.vector[0];
	y = v.vector[1];

	for(i = 0; i < width; ++i) {
		int32 x1;
		int32 y1;
		int32 x2;
		int32 y2;
		uint32 tl, tr, bl, br;
		int32 distx, disty;
		int width = image->bits.width;
		int height = image->bits.height;
		const uint8 * row1;
		const uint8 * row2;
		if(mask && !mask[i])
			goto next;
		x1 = x - pixman_fixed_1 / 2;
		y1 = y - pixman_fixed_1 / 2;
		distx = pixman_fixed_to_bilinear_weight(x1);
		disty = pixman_fixed_to_bilinear_weight(y1);
		y1 = pixman_fixed_to_int(y1);
		y2 = y1 + 1;
		x1 = pixman_fixed_to_int(x1);
		x2 = x1 + 1;
		if(repeat_mode != PIXMAN_REPEAT_NONE) {
			uint32 mask = PIXMAN_FORMAT_A(format) ? 0 : 0xff000000;
			repeat(repeat_mode, &x1, width);
			repeat(repeat_mode, &y1, height);
			repeat(repeat_mode, &x2, width);
			repeat(repeat_mode, &y2, height);

			row1 = (uint8 *)bits->bits + bits->rowstride * 4 * y1;
			row2 = (uint8 *)bits->bits + bits->rowstride * 4 * y2;

			tl = convert_pixel(row1, x1) | mask;
			tr = convert_pixel(row1, x2) | mask;
			bl = convert_pixel(row2, x1) | mask;
			br = convert_pixel(row2, x2) | mask;
		}
		else {
			uint32 mask1, mask2;
			int bpp;

			/* Note: PIXMAN_FORMAT_BPP() returns an unsigned value,
			 * which means if you use it in expressions, those
			 * expressions become unsigned themselves. Since
			 * the variables below can be negative in some cases,
			 * that will lead to crashes on 64 bit architectures.
			 *
			 * So this line makes sure bpp is signed
			 */
			bpp = PIXMAN_FORMAT_BPP(format);

			if(x1 >= width || x2 < 0 || y1 >= height || y2 < 0) {
				buffer[i] = 0;
				goto next;
			}

			if(y2 == 0) {
				row1 = zero;
				mask1 = 0;
			}
			else {
				row1 = (uint8 *)bits->bits + bits->rowstride * 4 * y1;
				row1 += bpp / 8 * x1;

				mask1 = PIXMAN_FORMAT_A(format) ? 0 : 0xff000000;
			}

			if(y1 == height - 1) {
				row2 = zero;
				mask2 = 0;
			}
			else {
				row2 = (uint8 *)bits->bits + bits->rowstride * 4 * y2;
				row2 += bpp / 8 * x1;

				mask2 = PIXMAN_FORMAT_A(format) ? 0 : 0xff000000;
			}

			if(x2 == 0) {
				tl = 0;
				bl = 0;
			}
			else {
				tl = convert_pixel(row1, 0) | mask1;
				bl = convert_pixel(row2, 0) | mask2;
			}

			if(x1 == width - 1) {
				tr = 0;
				br = 0;
			}
			else {
				tr = convert_pixel(row1, 1) | mask1;
				br = convert_pixel(row2, 1) | mask2;
			}
		}

		buffer[i] = bilinear_interpolation(
			tl, tr, bl, br, distx, disty);

next:
		x += ux;
		y += uy;
	}
}

static force_inline void bits_image_fetch_nearest_affine(pixman_image_t * image, int offset, int line, int width,
    uint32 *  buffer, const uint32 * mask, convert_pixel_t convert_pixel, pixman_format_code_t format, pixman_repeat_t repeat_mode)
{
	pixman_fixed_t x, y;
	pixman_fixed_t ux, uy;
	pixman_vector_t v;
	bits_image_t * bits = &image->bits;
	int i;
	/* reference point is the center of the pixel */
	v.vector[0] = pixman_int_to_fixed(offset) + pixman_fixed_1 / 2;
	v.vector[1] = pixman_int_to_fixed(line) + pixman_fixed_1 / 2;
	v.vector[2] = pixman_fixed_1;
	if(!pixman_transform_point_3d(image->common.transform, &v))
		return;
	ux = image->common.transform->matrix[0][0];
	uy = image->common.transform->matrix[1][0];
	x = v.vector[0];
	y = v.vector[1];
	for(i = 0; i < width; ++i) {
		int width;
		int height;
		int32 x0;
		int32 y0;
		const uint8 * row;
		if(mask && !mask[i])
			goto next;
		width = image->bits.width;
		height = image->bits.height;
		x0 = pixman_fixed_to_int(x - pixman_fixed_e);
		y0 = pixman_fixed_to_int(y - pixman_fixed_e);
		if(repeat_mode == PIXMAN_REPEAT_NONE && (y0 < 0 || y0 >= height || x0 < 0 || x0 >= width)) {
			buffer[i] = 0;
		}
		else {
			uint32 mask = PIXMAN_FORMAT_A(format) ? 0 : 0xff000000;
			if(repeat_mode != PIXMAN_REPEAT_NONE) {
				repeat(repeat_mode, &x0, width);
				repeat(repeat_mode, &y0, height);
			}
			row = (uint8 *)bits->bits + bits->rowstride * 4 * y0;
			buffer[i] = convert_pixel(row, x0) | mask;
		}
next:
		x += ux;
		y += uy;
	}
}

static force_inline uint32 convert_a8r8g8b8(const uint8 * row, int x)
{
	return *(((uint32 *)row) + x);
}

static force_inline uint32 convert_x8r8g8b8(const uint8 * row, int x)
{
	return *(((uint32 *)row) + x);
}

static force_inline uint32 convert_a8(const uint8 * row, int x)
{
	return *(row + x) << 24;
}

static force_inline uint32 convert_r5g6b5(const uint8 * row, int x)
{
	return convert_0565_to_0888(*((uint16 *)row + x));
}

#define MAKE_SEPARABLE_CONVOLUTION_FETCHER(name, format, repeat_mode)  \
	static uint32 *      \
	bits_image_fetch_separable_convolution_affine_ ## name(pixman_iter_t   *iter, \
	    const uint32 * mask) \
	{                                                                   \
		bits_image_fetch_separable_convolution_affine(                 \
			iter->image,                                                \
			iter->x, iter->y++,                                         \
			iter->width,                                                \
			iter->buffer, mask,                                         \
			convert_ ## format,                                         \
			PIXMAN_ ## format,                                          \
			repeat_mode);                                               \
                                                                        \
		return iter->buffer;                                            \
	}

#define MAKE_BILINEAR_FETCHER(name, format, repeat_mode)                \
	static uint32 *      \
	bits_image_fetch_bilinear_affine_ ## name(pixman_iter_t   *iter,   \
	    const uint32 * mask)   \
	{                                                                   \
		bits_image_fetch_bilinear_affine(iter->image,                  \
		    iter->x, iter->y++,           \
		    iter->width,                  \
		    iter->buffer, mask,           \
		    convert_ ## format,           \
		    PIXMAN_ ## format,            \
		    repeat_mode);                 \
		return iter->buffer;                                            \
	}

#define MAKE_NEAREST_FETCHER(name, format, repeat_mode)                 \
	static uint32 *      \
	bits_image_fetch_nearest_affine_ ## name(pixman_iter_t   *iter,    \
	    const uint32 * mask)    \
	{                                                                   \
		bits_image_fetch_nearest_affine(iter->image,                   \
		    iter->x, iter->y++,            \
		    iter->width,                   \
		    iter->buffer, mask,            \
		    convert_ ## format,            \
		    PIXMAN_ ## format,             \
		    repeat_mode);                  \
		return iter->buffer;                                            \
	}

#define MAKE_FETCHERS(name, format, repeat_mode)                        \
	MAKE_NEAREST_FETCHER(name, format, repeat_mode)                    \
	MAKE_BILINEAR_FETCHER(name, format, repeat_mode)                   \
	MAKE_SEPARABLE_CONVOLUTION_FETCHER(name, format, repeat_mode)

MAKE_FETCHERS(pad_a8r8g8b8,     a8r8g8b8, PIXMAN_REPEAT_PAD)
MAKE_FETCHERS(none_a8r8g8b8,    a8r8g8b8, PIXMAN_REPEAT_NONE)
MAKE_FETCHERS(reflect_a8r8g8b8, a8r8g8b8, PIXMAN_REPEAT_REFLECT)
MAKE_FETCHERS(normal_a8r8g8b8,  a8r8g8b8, PIXMAN_REPEAT_NORMAL)
MAKE_FETCHERS(pad_x8r8g8b8,     x8r8g8b8, PIXMAN_REPEAT_PAD)
MAKE_FETCHERS(none_x8r8g8b8,    x8r8g8b8, PIXMAN_REPEAT_NONE)
MAKE_FETCHERS(reflect_x8r8g8b8, x8r8g8b8, PIXMAN_REPEAT_REFLECT)
MAKE_FETCHERS(normal_x8r8g8b8,  x8r8g8b8, PIXMAN_REPEAT_NORMAL)
MAKE_FETCHERS(pad_a8,           a8,       PIXMAN_REPEAT_PAD)
MAKE_FETCHERS(none_a8,          a8,       PIXMAN_REPEAT_NONE)
MAKE_FETCHERS(reflect_a8,       a8,       PIXMAN_REPEAT_REFLECT)
MAKE_FETCHERS(normal_a8,        a8,       PIXMAN_REPEAT_NORMAL)
MAKE_FETCHERS(pad_r5g6b5,       r5g6b5,   PIXMAN_REPEAT_PAD)
MAKE_FETCHERS(none_r5g6b5,      r5g6b5,   PIXMAN_REPEAT_NONE)
MAKE_FETCHERS(reflect_r5g6b5,   r5g6b5,   PIXMAN_REPEAT_REFLECT)
MAKE_FETCHERS(normal_r5g6b5,    r5g6b5,   PIXMAN_REPEAT_NORMAL)

#define IMAGE_FLAGS                                                     \
	(FAST_PATH_STANDARD_FLAGS | FAST_PATH_ID_TRANSFORM |                \
	FAST_PATH_BITS_IMAGE | FAST_PATH_SAMPLES_COVER_CLIP_NEAREST)

static const pixman_iter_info_t fast_iters[] =
{
	{ PIXMAN_r5g6b5, IMAGE_FLAGS, (iter_flags_t)(ITER_NARROW | ITER_SRC), _pixman_iter_init_bits_stride, fast_fetch_r5g6b5, NULL },
	{ PIXMAN_r5g6b5, FAST_PATH_STD_DEST_FLAGS, (iter_flags_t)(ITER_NARROW | ITER_DEST), _pixman_iter_init_bits_stride, fast_fetch_r5g6b5, fast_write_back_r5g6b5 },
	{ PIXMAN_r5g6b5, FAST_PATH_STD_DEST_FLAGS,
	  (iter_flags_t)(ITER_NARROW | ITER_DEST | ITER_IGNORE_RGB | ITER_IGNORE_ALPHA),
	  _pixman_iter_init_bits_stride,
	  fast_dest_fetch_noop, fast_write_back_r5g6b5 },

	{ PIXMAN_a8r8g8b8,
	  (FAST_PATH_STANDARD_FLAGS                 |
	  FAST_PATH_SCALE_TRANSFORM                |
	  FAST_PATH_BILINEAR_FILTER                |
	  FAST_PATH_SAMPLES_COVER_CLIP_BILINEAR),
	  (iter_flags_t)(ITER_NARROW | ITER_SRC),
	  fast_bilinear_cover_iter_init,
	  NULL, NULL},

#define FAST_BILINEAR_FLAGS                                             \
	(FAST_PATH_NO_ALPHA_MAP             |                               \
	FAST_PATH_NO_ACCESSORS             |                               \
	FAST_PATH_HAS_TRANSFORM            |                               \
	FAST_PATH_AFFINE_TRANSFORM         |                               \
	FAST_PATH_X_UNIT_POSITIVE          |                               \
	FAST_PATH_Y_UNIT_ZERO              |                               \
	FAST_PATH_NONE_REPEAT              |                               \
	FAST_PATH_BILINEAR_FILTER)

	{ PIXMAN_a8r8g8b8,
	  FAST_BILINEAR_FLAGS,
	  (iter_flags_t)(ITER_NARROW | ITER_SRC),
	  NULL, bits_image_fetch_bilinear_no_repeat_8888, NULL},

	{ PIXMAN_x8r8g8b8,
	  FAST_BILINEAR_FLAGS,
	  (iter_flags_t)(ITER_NARROW | ITER_SRC),
	  NULL, bits_image_fetch_bilinear_no_repeat_8888, NULL},

#define GENERAL_BILINEAR_FLAGS                                          \
	(FAST_PATH_NO_ALPHA_MAP             |                               \
	FAST_PATH_NO_ACCESSORS             |                               \
	FAST_PATH_HAS_TRANSFORM            |                               \
	FAST_PATH_AFFINE_TRANSFORM         |                               \
	FAST_PATH_BILINEAR_FILTER)

#define GENERAL_NEAREST_FLAGS                                           \
	(FAST_PATH_NO_ALPHA_MAP             |                               \
	FAST_PATH_NO_ACCESSORS             |                               \
	FAST_PATH_HAS_TRANSFORM            |                               \
	FAST_PATH_AFFINE_TRANSFORM         |                               \
	FAST_PATH_NEAREST_FILTER)

#define GENERAL_SEPARABLE_CONVOLUTION_FLAGS                             \
	(FAST_PATH_NO_ALPHA_MAP            |                                \
	FAST_PATH_NO_ACCESSORS            |                                \
	FAST_PATH_HAS_TRANSFORM           |                                \
	FAST_PATH_AFFINE_TRANSFORM        |                                \
	FAST_PATH_SEPARABLE_CONVOLUTION_FILTER)

#define SEPARABLE_CONVOLUTION_AFFINE_FAST_PATH(name, format, repeat)   \
	{ PIXMAN_ ## format,                                                \
	  GENERAL_SEPARABLE_CONVOLUTION_FLAGS | FAST_PATH_ ## repeat ## _REPEAT, \
	  (iter_flags_t)(ITER_NARROW | ITER_SRC), \
	  NULL, bits_image_fetch_separable_convolution_affine_ ## name, NULL \
	},

#define BILINEAR_AFFINE_FAST_PATH(name, format, repeat)                 \
	{ PIXMAN_ ## format,                                                \
	  GENERAL_BILINEAR_FLAGS | FAST_PATH_ ## repeat ## _REPEAT,         \
	  (iter_flags_t)(ITER_NARROW | ITER_SRC), \
	  NULL, bits_image_fetch_bilinear_affine_ ## name, NULL,            \
	},

#define NEAREST_AFFINE_FAST_PATH(name, format, repeat)                  \
	{ PIXMAN_ ## format,                                                \
	  GENERAL_NEAREST_FLAGS | FAST_PATH_ ## repeat ## _REPEAT,          \
	  (iter_flags_t)(ITER_NARROW | ITER_SRC), \
	  NULL, bits_image_fetch_nearest_affine_ ## name, NULL              \
	},

#define AFFINE_FAST_PATHS(name, format, repeat)                         \
	NEAREST_AFFINE_FAST_PATH(name, format, repeat)                      \
	BILINEAR_AFFINE_FAST_PATH(name, format, repeat)                     \
	SEPARABLE_CONVOLUTION_AFFINE_FAST_PATH(name, format, repeat)

	AFFINE_FAST_PATHS(pad_a8r8g8b8, a8r8g8b8, PAD)
	AFFINE_FAST_PATHS(none_a8r8g8b8, a8r8g8b8, NONE)
	AFFINE_FAST_PATHS(reflect_a8r8g8b8, a8r8g8b8, REFLECT)
	AFFINE_FAST_PATHS(normal_a8r8g8b8, a8r8g8b8, NORMAL)
	AFFINE_FAST_PATHS(pad_x8r8g8b8, x8r8g8b8, PAD)
	AFFINE_FAST_PATHS(none_x8r8g8b8, x8r8g8b8, NONE)
	AFFINE_FAST_PATHS(reflect_x8r8g8b8, x8r8g8b8, REFLECT)
	AFFINE_FAST_PATHS(normal_x8r8g8b8, x8r8g8b8, NORMAL)
	AFFINE_FAST_PATHS(pad_a8, a8, PAD)
	AFFINE_FAST_PATHS(none_a8, a8, NONE)
	AFFINE_FAST_PATHS(reflect_a8, a8, REFLECT)
	AFFINE_FAST_PATHS(normal_a8, a8, NORMAL)
	AFFINE_FAST_PATHS(pad_r5g6b5, r5g6b5, PAD)
	AFFINE_FAST_PATHS(none_r5g6b5, r5g6b5, NONE)
	AFFINE_FAST_PATHS(reflect_r5g6b5, r5g6b5, REFLECT)
	AFFINE_FAST_PATHS(normal_r5g6b5, r5g6b5, NORMAL)

	{
		PIXMAN_null
	},
};

pixman_implementation_t * _pixman_implementation_create_fast_path(pixman_implementation_t * fallback)
{
	pixman_implementation_t * imp = _pixman_implementation_create(fallback, c_fast_paths);
	imp->fill = fast_path_fill;
	imp->iter_info = fast_iters;
	return imp;
}
