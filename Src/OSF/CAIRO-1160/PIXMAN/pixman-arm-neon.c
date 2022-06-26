/*
 * Copyright © 2009 ARM Ltd, Movial Creative Technologies Oy
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of ARM Ltd not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  ARM Ltd makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author:  Ian Rickards (ian.rickards@arm.com)
 * Author:  Jonathan Morton (jonathan.morton@movial.com)
 * Author:  Markku Vire (markku.vire@movial.com)
 *
 */
#include "cairoint.h"
#pragma hdrstop
#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif
#undef IN
//#include <string.h>
//#include "pixman-private.h"
#include "pixman-arm-common.h"

PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(neon, src_8888_8888, uint32, 1, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(neon, src_x888_8888, uint32, 1, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(neon, src_0565_0565, uint16, 1, uint16, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(neon, src_0888_0888, uint8, 3, uint8, 3)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(neon, src_8888_0565, uint32, 1, uint16, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(neon, src_0565_8888, uint16, 1, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(neon, src_0888_8888_rev, uint8, 3, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(neon, src_0888_0565_rev, uint8, 3, uint16, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(neon, src_pixbuf_8888, uint32, 1, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(neon, src_rpixbuf_8888, uint32, 1, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(neon, add_8_8, uint8, 1, uint8, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(neon, add_8888_8888, uint32, 1, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(neon, over_8888_0565, uint32, 1, uint16, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(neon, over_8888_8888, uint32, 1, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(neon, out_reverse_8_0565, uint8, 1, uint16, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(neon, out_reverse_8_8888, uint8, 1, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_N_DST(SKIP_ZERO_SRC, neon, over_n_0565, uint16, 1)
PIXMAN_ARM_BIND_FAST_PATH_N_DST(SKIP_ZERO_SRC, neon, over_n_8888, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_N_DST(SKIP_ZERO_SRC, neon, over_reverse_n_8888, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_N_DST(0, neon, in_n_8, uint8, 1)
PIXMAN_ARM_BIND_FAST_PATH_N_MASK_DST(SKIP_ZERO_SRC, neon, over_n_8_0565, uint8, 1, uint16, 1)
PIXMAN_ARM_BIND_FAST_PATH_N_MASK_DST(SKIP_ZERO_SRC, neon, over_n_8_8888, uint8, 1, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_N_MASK_DST(SKIP_ZERO_SRC, neon, over_n_8888_8888_ca, uint32, 1, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_N_MASK_DST(SKIP_ZERO_SRC, neon, over_n_8888_0565_ca, uint32, 1, uint16, 1)
PIXMAN_ARM_BIND_FAST_PATH_N_MASK_DST(SKIP_ZERO_SRC, neon, over_n_8_8, uint8, 1, uint8, 1)
PIXMAN_ARM_BIND_FAST_PATH_N_MASK_DST(SKIP_ZERO_SRC, neon, add_n_8_8, uint8, 1, uint8, 1)
PIXMAN_ARM_BIND_FAST_PATH_N_MASK_DST(SKIP_ZERO_SRC, neon, add_n_8_8888, uint8, 1, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_N_MASK_DST(0, neon, src_n_8_8888, uint8, 1, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_N_MASK_DST(0, neon, src_n_8_8, uint8, 1, uint8, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_N_DST(SKIP_ZERO_MASK, neon, over_8888_n_8888, uint32, 1, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_N_DST(SKIP_ZERO_MASK, neon, over_8888_n_0565, uint32, 1, uint16, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_N_DST(SKIP_ZERO_MASK, neon, over_0565_n_0565, uint16, 1, uint16, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_N_DST(SKIP_ZERO_MASK, neon, add_8888_n_8888, uint32, 1, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_MASK_DST(neon, add_8_8_8, uint8, 1, uint8, 1, uint8, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_MASK_DST(neon, add_0565_8_0565, uint16, 1, uint8, 1, uint16, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_MASK_DST(neon, add_8888_8_8888, uint32, 1, uint8, 1, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_MASK_DST(neon, add_8888_8888_8888, uint32, 1, uint32, 1, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_MASK_DST(neon, over_8888_8_8888, uint32, 1, uint8, 1, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_MASK_DST(neon, over_8888_8888_8888, uint32, 1, uint32, 1, uint32, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_MASK_DST(neon, over_8888_8_0565, uint32, 1, uint8, 1, uint16, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_MASK_DST(neon, over_0565_8_0565, uint16, 1, uint8, 1, uint16, 1)
PIXMAN_ARM_BIND_SCALED_NEAREST_SRC_DST(neon, 8888_8888, OVER, uint32, uint32)
PIXMAN_ARM_BIND_SCALED_NEAREST_SRC_DST(neon, 8888_0565, OVER, uint32, uint16)
PIXMAN_ARM_BIND_SCALED_NEAREST_SRC_DST(neon, 8888_0565, SRC, uint32, uint16)
PIXMAN_ARM_BIND_SCALED_NEAREST_SRC_DST(neon, 0565_8888, SRC, uint16, uint32)
PIXMAN_ARM_BIND_SCALED_NEAREST_SRC_A8_DST(SKIP_ZERO_SRC, neon, 8888_8_0565, OVER, uint32, uint16)
PIXMAN_ARM_BIND_SCALED_NEAREST_SRC_A8_DST(SKIP_ZERO_SRC, neon, 0565_8_0565, OVER, uint16, uint16)
PIXMAN_ARM_BIND_SCALED_BILINEAR_SRC_DST(0, neon, 8888_8888, SRC, uint32, uint32)
PIXMAN_ARM_BIND_SCALED_BILINEAR_SRC_DST(0, neon, 8888_0565, SRC, uint32, uint16)
PIXMAN_ARM_BIND_SCALED_BILINEAR_SRC_DST(0, neon, 0565_x888, SRC, uint16, uint32)
PIXMAN_ARM_BIND_SCALED_BILINEAR_SRC_DST(0, neon, 0565_0565, SRC, uint16, uint16)
PIXMAN_ARM_BIND_SCALED_BILINEAR_SRC_DST(SKIP_ZERO_SRC, neon, 8888_8888, OVER, uint32, uint32)
PIXMAN_ARM_BIND_SCALED_BILINEAR_SRC_DST(SKIP_ZERO_SRC, neon, 8888_8888, ADD, uint32, uint32)
PIXMAN_ARM_BIND_SCALED_BILINEAR_SRC_A8_DST(0, neon, 8888_8_8888, SRC, uint32, uint32)
PIXMAN_ARM_BIND_SCALED_BILINEAR_SRC_A8_DST(0, neon, 8888_8_0565, SRC, uint32, uint16)
PIXMAN_ARM_BIND_SCALED_BILINEAR_SRC_A8_DST(0, neon, 0565_8_x888, SRC, uint16, uint32)
PIXMAN_ARM_BIND_SCALED_BILINEAR_SRC_A8_DST(0, neon, 0565_8_0565, SRC, uint16, uint16)
PIXMAN_ARM_BIND_SCALED_BILINEAR_SRC_A8_DST(SKIP_ZERO_SRC, neon, 8888_8_8888, OVER, uint32, uint32)
PIXMAN_ARM_BIND_SCALED_BILINEAR_SRC_A8_DST(SKIP_ZERO_SRC, neon, 8888_8_8888, ADD, uint32, uint32)

void pixman_composite_src_n_8_asm_neon(int32 w, int32 h, uint8 * dst, int32 dst_stride, uint8 src);
void pixman_composite_src_n_0565_asm_neon(int32 w, int32 h, uint16 * dst, int32 dst_stride, uint16 src);
void pixman_composite_src_n_8888_asm_neon(int32 w, int32 h, uint32 * dst, int32 dst_stride, uint32 src);

static boolint arm_neon_fill(pixman_implementation_t * imp, uint32 * bits, int stride, int bpp, int x, int y, int width, int height, uint32 _xor)
{
	/* stride is always multiple of 32bit units in pixman */
	uint32 byte_stride = stride * sizeof(uint32);
	switch(bpp) {
		case 8:
		    pixman_composite_src_n_8_asm_neon(width, height, (uint8 *)(((char *)bits) + y * byte_stride + x), byte_stride, _xor & 0xff);
		    return TRUE;
		case 16:
		    pixman_composite_src_n_0565_asm_neon(width, height, (uint16 *)(((char *)bits) + y * byte_stride + x * 2), byte_stride / 2, _xor & 0xffff);
		    return TRUE;
		case 32:
		    pixman_composite_src_n_8888_asm_neon(width, height, (uint32 *)(((char *)bits) + y * byte_stride + x * 4), byte_stride / 4, _xor);
		    return TRUE;
		default:
		    return FALSE;
	}
}

static boolint arm_neon_blt(pixman_implementation_t * imp, uint32 * src_bits, uint32 * dst_bits, int src_stride, int dst_stride,
    int src_bpp, int dst_bpp, int src_x, int src_y, int dest_x, int dest_y, int width, int height)
{
	if(src_bpp != dst_bpp)
		return FALSE;
	switch(src_bpp) {
		case 16:
		    pixman_composite_src_0565_0565_asm_neon(width, height, (uint16 *)(((char *)dst_bits) + dest_y * dst_stride * 4 + dest_x * 2), dst_stride * 2,
			    (uint16 *)(((char *)src_bits) + src_y * src_stride * 4 + src_x * 2), src_stride * 2);
		    return TRUE;
		case 32:
		    pixman_composite_src_8888_8888_asm_neon(width, height, (uint32 *)(((char *)dst_bits) + dest_y * dst_stride * 4 + dest_x * 4), dst_stride,
			    (uint32 *)(((char *)src_bits) + src_y * src_stride * 4 + src_x * 4), src_stride);
		    return TRUE;
		default:
		    return FALSE;
	}
}

static const pixman_fast_path_t arm_neon_fast_paths[] = {
	PIXMAN_STD_FAST_PATH(SRC,  r5g6b5,   null,     r5g6b5,   neon_composite_src_0565_0565),
	PIXMAN_STD_FAST_PATH(SRC,  b5g6r5,   null,     b5g6r5,   neon_composite_src_0565_0565),
	PIXMAN_STD_FAST_PATH(SRC,  a8r8g8b8, null,     r5g6b5,   neon_composite_src_8888_0565),
	PIXMAN_STD_FAST_PATH(SRC,  x8r8g8b8, null,     r5g6b5,   neon_composite_src_8888_0565),
	PIXMAN_STD_FAST_PATH(SRC,  a8b8g8r8, null,     b5g6r5,   neon_composite_src_8888_0565),
	PIXMAN_STD_FAST_PATH(SRC,  x8b8g8r8, null,     b5g6r5,   neon_composite_src_8888_0565),
	PIXMAN_STD_FAST_PATH(SRC,  r5g6b5,   null,     a8r8g8b8, neon_composite_src_0565_8888),
	PIXMAN_STD_FAST_PATH(SRC,  r5g6b5,   null,     x8r8g8b8, neon_composite_src_0565_8888),
	PIXMAN_STD_FAST_PATH(SRC,  b5g6r5,   null,     a8b8g8r8, neon_composite_src_0565_8888),
	PIXMAN_STD_FAST_PATH(SRC,  b5g6r5,   null,     x8b8g8r8, neon_composite_src_0565_8888),
	PIXMAN_STD_FAST_PATH(SRC,  a8r8g8b8, null,     x8r8g8b8, neon_composite_src_8888_8888),
	PIXMAN_STD_FAST_PATH(SRC,  x8r8g8b8, null,     x8r8g8b8, neon_composite_src_8888_8888),
	PIXMAN_STD_FAST_PATH(SRC,  a8b8g8r8, null,     x8b8g8r8, neon_composite_src_8888_8888),
	PIXMAN_STD_FAST_PATH(SRC,  x8b8g8r8, null,     x8b8g8r8, neon_composite_src_8888_8888),
	PIXMAN_STD_FAST_PATH(SRC,  a8r8g8b8, null,     a8r8g8b8, neon_composite_src_8888_8888),
	PIXMAN_STD_FAST_PATH(SRC,  a8b8g8r8, null,     a8b8g8r8, neon_composite_src_8888_8888),
	PIXMAN_STD_FAST_PATH(SRC,  x8r8g8b8, null,     a8r8g8b8, neon_composite_src_x888_8888),
	PIXMAN_STD_FAST_PATH(SRC,  x8b8g8r8, null,     a8b8g8r8, neon_composite_src_x888_8888),
	PIXMAN_STD_FAST_PATH(SRC,  r8g8b8,   null,     r8g8b8,   neon_composite_src_0888_0888),
	PIXMAN_STD_FAST_PATH(SRC,  b8g8r8,   null,     x8r8g8b8, neon_composite_src_0888_8888_rev),
	PIXMAN_STD_FAST_PATH(SRC,  b8g8r8,   null,     r5g6b5,   neon_composite_src_0888_0565_rev),
	PIXMAN_STD_FAST_PATH(SRC,  pixbuf,   pixbuf,   a8r8g8b8, neon_composite_src_pixbuf_8888),
	PIXMAN_STD_FAST_PATH(SRC,  pixbuf,   pixbuf,   a8b8g8r8, neon_composite_src_rpixbuf_8888),
	PIXMAN_STD_FAST_PATH(SRC,  rpixbuf,  rpixbuf,  a8r8g8b8, neon_composite_src_rpixbuf_8888),
	PIXMAN_STD_FAST_PATH(SRC,  rpixbuf,  rpixbuf,  a8b8g8r8, neon_composite_src_pixbuf_8888),
	PIXMAN_STD_FAST_PATH(SRC,  solid,    a8,       a8r8g8b8, neon_composite_src_n_8_8888),
	PIXMAN_STD_FAST_PATH(SRC,  solid,    a8,       x8r8g8b8, neon_composite_src_n_8_8888),
	PIXMAN_STD_FAST_PATH(SRC,  solid,    a8,       a8b8g8r8, neon_composite_src_n_8_8888),
	PIXMAN_STD_FAST_PATH(SRC,  solid,    a8,       x8b8g8r8, neon_composite_src_n_8_8888),
	PIXMAN_STD_FAST_PATH(SRC,  solid,    a8,       a8,       neon_composite_src_n_8_8),
	PIXMAN_STD_FAST_PATH(OVER, solid,    a8,       a8,       neon_composite_over_n_8_8),
	PIXMAN_STD_FAST_PATH(OVER, solid,    a8,       r5g6b5,   neon_composite_over_n_8_0565),
	PIXMAN_STD_FAST_PATH(OVER, solid,    a8,       b5g6r5,   neon_composite_over_n_8_0565),
	PIXMAN_STD_FAST_PATH(OVER, solid,    a8,       a8r8g8b8, neon_composite_over_n_8_8888),
	PIXMAN_STD_FAST_PATH(OVER, solid,    a8,       x8r8g8b8, neon_composite_over_n_8_8888),
	PIXMAN_STD_FAST_PATH(OVER, solid,    a8,       a8b8g8r8, neon_composite_over_n_8_8888),
	PIXMAN_STD_FAST_PATH(OVER, solid,    a8,       x8b8g8r8, neon_composite_over_n_8_8888),
	PIXMAN_STD_FAST_PATH(OVER, solid,    null,     r5g6b5,   neon_composite_over_n_0565),
	PIXMAN_STD_FAST_PATH(OVER, solid,    null,     a8r8g8b8, neon_composite_over_n_8888),
	PIXMAN_STD_FAST_PATH(OVER, solid,    null,     x8r8g8b8, neon_composite_over_n_8888),
	PIXMAN_STD_FAST_PATH_CA(OVER, solid, a8r8g8b8, a8r8g8b8, neon_composite_over_n_8888_8888_ca),
	PIXMAN_STD_FAST_PATH_CA(OVER, solid, a8r8g8b8, x8r8g8b8, neon_composite_over_n_8888_8888_ca),
	PIXMAN_STD_FAST_PATH_CA(OVER, solid, a8b8g8r8, a8b8g8r8, neon_composite_over_n_8888_8888_ca),
	PIXMAN_STD_FAST_PATH_CA(OVER, solid, a8b8g8r8, x8b8g8r8, neon_composite_over_n_8888_8888_ca),
	PIXMAN_STD_FAST_PATH_CA(OVER, solid, a8r8g8b8, r5g6b5,   neon_composite_over_n_8888_0565_ca),
	PIXMAN_STD_FAST_PATH_CA(OVER, solid, a8b8g8r8, b5g6r5,   neon_composite_over_n_8888_0565_ca),
	PIXMAN_STD_FAST_PATH(OVER, a8r8g8b8, solid,    a8r8g8b8, neon_composite_over_8888_n_8888),
	PIXMAN_STD_FAST_PATH(OVER, a8r8g8b8, solid,    x8r8g8b8, neon_composite_over_8888_n_8888),
	PIXMAN_STD_FAST_PATH(OVER, a8r8g8b8, solid,    r5g6b5,   neon_composite_over_8888_n_0565),
	PIXMAN_STD_FAST_PATH(OVER, a8b8g8r8, solid,    b5g6r5,   neon_composite_over_8888_n_0565),
	PIXMAN_STD_FAST_PATH(OVER, r5g6b5,   solid,    r5g6b5,   neon_composite_over_0565_n_0565),
	PIXMAN_STD_FAST_PATH(OVER, b5g6r5,   solid,    b5g6r5,   neon_composite_over_0565_n_0565),
	PIXMAN_STD_FAST_PATH(OVER, a8r8g8b8, a8,       a8r8g8b8, neon_composite_over_8888_8_8888),
	PIXMAN_STD_FAST_PATH(OVER, a8r8g8b8, a8,       x8r8g8b8, neon_composite_over_8888_8_8888),
	PIXMAN_STD_FAST_PATH(OVER, a8b8g8r8, a8,       a8b8g8r8, neon_composite_over_8888_8_8888),
	PIXMAN_STD_FAST_PATH(OVER, a8b8g8r8, a8,       x8b8g8r8, neon_composite_over_8888_8_8888),
	PIXMAN_STD_FAST_PATH(OVER, a8r8g8b8, a8,       r5g6b5,   neon_composite_over_8888_8_0565),
	PIXMAN_STD_FAST_PATH(OVER, a8b8g8r8, a8,       b5g6r5,   neon_composite_over_8888_8_0565),
	PIXMAN_STD_FAST_PATH(OVER, r5g6b5,   a8,       r5g6b5,   neon_composite_over_0565_8_0565),
	PIXMAN_STD_FAST_PATH(OVER, b5g6r5,   a8,       b5g6r5,   neon_composite_over_0565_8_0565),
	PIXMAN_STD_FAST_PATH(OVER, a8r8g8b8, a8r8g8b8, a8r8g8b8, neon_composite_over_8888_8888_8888),
	PIXMAN_STD_FAST_PATH(OVER, a8r8g8b8, null,     r5g6b5,   neon_composite_over_8888_0565),
	PIXMAN_STD_FAST_PATH(OVER, a8b8g8r8, null,     b5g6r5,   neon_composite_over_8888_0565),
	PIXMAN_STD_FAST_PATH(OVER, a8r8g8b8, null,     a8r8g8b8, neon_composite_over_8888_8888),
	PIXMAN_STD_FAST_PATH(OVER, a8r8g8b8, null,     x8r8g8b8, neon_composite_over_8888_8888),
	PIXMAN_STD_FAST_PATH(OVER, a8b8g8r8, null,     a8b8g8r8, neon_composite_over_8888_8888),
	PIXMAN_STD_FAST_PATH(OVER, a8b8g8r8, null,     x8b8g8r8, neon_composite_over_8888_8888),
	PIXMAN_STD_FAST_PATH(OVER, x8r8g8b8, null,     a8r8g8b8, neon_composite_src_x888_8888),
	PIXMAN_STD_FAST_PATH(OVER, x8b8g8r8, null,     a8b8g8r8, neon_composite_src_x888_8888),
	PIXMAN_STD_FAST_PATH(ADD,  solid,    a8,       a8,       neon_composite_add_n_8_8),
	PIXMAN_STD_FAST_PATH(ADD,  solid,    a8,       a8r8g8b8, neon_composite_add_n_8_8888),
	PIXMAN_STD_FAST_PATH(ADD,  solid,    a8,       a8b8g8r8, neon_composite_add_n_8_8888),
	PIXMAN_STD_FAST_PATH(ADD,  a8,       a8,       a8,       neon_composite_add_8_8_8),
	PIXMAN_STD_FAST_PATH(ADD,  r5g6b5,   a8,       r5g6b5,   neon_composite_add_0565_8_0565),
	PIXMAN_STD_FAST_PATH(ADD,  b5g6r5,   a8,       b5g6r5,   neon_composite_add_0565_8_0565),
	PIXMAN_STD_FAST_PATH(ADD,  a8r8g8b8, a8,       a8r8g8b8, neon_composite_add_8888_8_8888),
	PIXMAN_STD_FAST_PATH(ADD,  a8b8g8r8, a8,       a8b8g8r8, neon_composite_add_8888_8_8888),
	PIXMAN_STD_FAST_PATH(ADD,  a8r8g8b8, a8r8g8b8, a8r8g8b8, neon_composite_add_8888_8888_8888),
	PIXMAN_STD_FAST_PATH(ADD,  a8r8g8b8, solid,    a8r8g8b8, neon_composite_add_8888_n_8888),
	PIXMAN_STD_FAST_PATH(ADD,  a8b8g8r8, solid,    a8b8g8r8, neon_composite_add_8888_n_8888),
	PIXMAN_STD_FAST_PATH(ADD,  a8,       null,     a8,       neon_composite_add_8_8),
	PIXMAN_STD_FAST_PATH(ADD,  a8r8g8b8, null,     a8r8g8b8, neon_composite_add_8888_8888),
	PIXMAN_STD_FAST_PATH(ADD,  a8b8g8r8, null,     a8b8g8r8, neon_composite_add_8888_8888),
	PIXMAN_STD_FAST_PATH(IN,   solid,    null,     a8,       neon_composite_in_n_8),
	PIXMAN_STD_FAST_PATH(OVER_REVERSE, solid, null, a8r8g8b8, neon_composite_over_reverse_n_8888),
	PIXMAN_STD_FAST_PATH(OVER_REVERSE, solid, null, a8b8g8r8, neon_composite_over_reverse_n_8888),
	PIXMAN_STD_FAST_PATH(OUT_REVERSE,  a8,    null, r5g6b5,   neon_composite_out_reverse_8_0565),
	PIXMAN_STD_FAST_PATH(OUT_REVERSE,  a8,    null, b5g6r5,   neon_composite_out_reverse_8_0565),
	PIXMAN_STD_FAST_PATH(OUT_REVERSE,  a8,    null, a8r8g8b8, neon_composite_out_reverse_8_8888),
	PIXMAN_STD_FAST_PATH(OUT_REVERSE,  a8,    null, a8b8g8r8, neon_composite_out_reverse_8_8888),

	SIMPLE_NEAREST_FAST_PATH(OVER, a8r8g8b8, a8r8g8b8, neon_8888_8888),
	SIMPLE_NEAREST_FAST_PATH(OVER, a8b8g8r8, a8b8g8r8, neon_8888_8888),
	SIMPLE_NEAREST_FAST_PATH(OVER, a8r8g8b8, x8r8g8b8, neon_8888_8888),
	SIMPLE_NEAREST_FAST_PATH(OVER, a8b8g8r8, x8b8g8r8, neon_8888_8888),

	SIMPLE_NEAREST_FAST_PATH(OVER, a8r8g8b8, r5g6b5, neon_8888_0565),
	SIMPLE_NEAREST_FAST_PATH(OVER, a8b8g8r8, b5g6r5, neon_8888_0565),

	SIMPLE_NEAREST_FAST_PATH(SRC, a8r8g8b8, r5g6b5, neon_8888_0565),
	SIMPLE_NEAREST_FAST_PATH(SRC, x8r8g8b8, r5g6b5, neon_8888_0565),
	SIMPLE_NEAREST_FAST_PATH(SRC, a8b8g8r8, b5g6r5, neon_8888_0565),
	SIMPLE_NEAREST_FAST_PATH(SRC, x8b8g8r8, b5g6r5, neon_8888_0565),
	SIMPLE_NEAREST_FAST_PATH(SRC, b5g6r5, x8b8g8r8, neon_0565_8888),
	SIMPLE_NEAREST_FAST_PATH(SRC, r5g6b5, x8r8g8b8, neon_0565_8888),
	/* Note: NONE repeat is not supported yet */
	SIMPLE_NEAREST_FAST_PATH_COVER(SRC, r5g6b5, a8r8g8b8, neon_0565_8888),
	SIMPLE_NEAREST_FAST_PATH_COVER(SRC, b5g6r5, a8b8g8r8, neon_0565_8888),
	SIMPLE_NEAREST_FAST_PATH_PAD(SRC, r5g6b5, a8r8g8b8, neon_0565_8888),
	SIMPLE_NEAREST_FAST_PATH_PAD(SRC, b5g6r5, a8b8g8r8, neon_0565_8888),
	PIXMAN_ARM_SIMPLE_NEAREST_A8_MASK_FAST_PATH(OVER, a8r8g8b8, r5g6b5, neon_8888_8_0565),
	PIXMAN_ARM_SIMPLE_NEAREST_A8_MASK_FAST_PATH(OVER, a8b8g8r8, b5g6r5, neon_8888_8_0565),
	PIXMAN_ARM_SIMPLE_NEAREST_A8_MASK_FAST_PATH(OVER, r5g6b5, r5g6b5, neon_0565_8_0565),
	PIXMAN_ARM_SIMPLE_NEAREST_A8_MASK_FAST_PATH(OVER, b5g6r5, b5g6r5, neon_0565_8_0565),
	SIMPLE_BILINEAR_FAST_PATH(SRC, a8r8g8b8, a8r8g8b8, neon_8888_8888),
	SIMPLE_BILINEAR_FAST_PATH(SRC, a8r8g8b8, x8r8g8b8, neon_8888_8888),
	SIMPLE_BILINEAR_FAST_PATH(SRC, x8r8g8b8, x8r8g8b8, neon_8888_8888),
	SIMPLE_BILINEAR_FAST_PATH(SRC, a8r8g8b8, r5g6b5, neon_8888_0565),
	SIMPLE_BILINEAR_FAST_PATH(SRC, x8r8g8b8, r5g6b5, neon_8888_0565),
	SIMPLE_BILINEAR_FAST_PATH(SRC, r5g6b5, x8r8g8b8, neon_0565_x888),
	SIMPLE_BILINEAR_FAST_PATH(SRC, r5g6b5, r5g6b5, neon_0565_0565),
	SIMPLE_BILINEAR_FAST_PATH(OVER, a8r8g8b8, a8r8g8b8, neon_8888_8888),
	SIMPLE_BILINEAR_FAST_PATH(OVER, a8r8g8b8, x8r8g8b8, neon_8888_8888),
	SIMPLE_BILINEAR_FAST_PATH(ADD, a8r8g8b8, a8r8g8b8, neon_8888_8888),
	SIMPLE_BILINEAR_FAST_PATH(ADD, a8r8g8b8, x8r8g8b8, neon_8888_8888),
	SIMPLE_BILINEAR_A8_MASK_FAST_PATH(SRC, a8r8g8b8, a8r8g8b8, neon_8888_8_8888),
	SIMPLE_BILINEAR_A8_MASK_FAST_PATH(SRC, a8r8g8b8, x8r8g8b8, neon_8888_8_8888),
	SIMPLE_BILINEAR_A8_MASK_FAST_PATH(SRC, x8r8g8b8, x8r8g8b8, neon_8888_8_8888),
	SIMPLE_BILINEAR_A8_MASK_FAST_PATH(SRC, a8r8g8b8, r5g6b5, neon_8888_8_0565),
	SIMPLE_BILINEAR_A8_MASK_FAST_PATH(SRC, x8r8g8b8, r5g6b5, neon_8888_8_0565),
	SIMPLE_BILINEAR_A8_MASK_FAST_PATH(SRC, r5g6b5, x8r8g8b8, neon_0565_8_x888),
	SIMPLE_BILINEAR_A8_MASK_FAST_PATH(SRC, r5g6b5, r5g6b5, neon_0565_8_0565),
	SIMPLE_BILINEAR_A8_MASK_FAST_PATH(OVER, a8r8g8b8, a8r8g8b8, neon_8888_8_8888),
	SIMPLE_BILINEAR_A8_MASK_FAST_PATH(OVER, a8r8g8b8, x8r8g8b8, neon_8888_8_8888),
	SIMPLE_BILINEAR_A8_MASK_FAST_PATH(ADD, a8r8g8b8, a8r8g8b8, neon_8888_8_8888),
	SIMPLE_BILINEAR_A8_MASK_FAST_PATH(ADD, a8r8g8b8, x8r8g8b8, neon_8888_8_8888),
	{ PIXMAN_OP_NONE },
};

#define BIND_COMBINE_U(name)                                             \
	void pixman_composite_scanline_ ## name ## _mask_asm_neon(int32 w, const uint32 *dst, const uint32 *src, const uint32 *mask); \
	void pixman_composite_scanline_ ## name ## _asm_neon(int32 w, const uint32 *dst, const uint32 *src); \
                                                                         \
	static void neon_combine_ ## name ## _u(pixman_implementation_t *imp, pixman_op_t op, uint32 * dest, const uint32 * src, const uint32 * mask, int width)                 \
	{                                                                        \
		if(mask)                                                            \
			pixman_composite_scanline_ ## name ## _mask_asm_neon(width, dest, src, mask);    \
		else                                                                 \
			pixman_composite_scanline_ ## name ## _asm_neon(width, dest, src);  \
	}

BIND_COMBINE_U(over)
BIND_COMBINE_U(add)
BIND_COMBINE_U(out_reverse)

pixman_implementation_t * _pixman_implementation_create_arm_neon(pixman_implementation_t *fallback)
{
	pixman_implementation_t * imp = _pixman_implementation_create(fallback, arm_neon_fast_paths);
	imp->combine_32[PIXMAN_OP_OVER] = neon_combine_over_u;
	imp->combine_32[PIXMAN_OP_ADD] = neon_combine_add_u;
	imp->combine_32[PIXMAN_OP_OUT_REVERSE] = neon_combine_out_reverse_u;
	imp->blt = arm_neon_blt;
	imp->fill = arm_neon_fill;
	return imp;
}
