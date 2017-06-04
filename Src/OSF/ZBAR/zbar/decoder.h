/*------------------------------------------------------------------------
 *  Copyright 2007-2010 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the ZBar Bar Code Reader.
 *
 *  The ZBar Bar Code Reader is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The ZBar Bar Code Reader is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the ZBar Bar Code Reader; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zbar
 *------------------------------------------------------------------------*/
#ifndef _DECODER_H_
#define _DECODER_H_

#include <zbar.h>
#ifdef ENABLE_QRCODE
	#include "decoder/qr_finder.h"
#endif

/* initial data buffer allocation */
#ifndef BUFFER_MIN
	#define BUFFER_MIN   0x20
#endif

/* maximum data buffer allocation
 * (longer symbols are rejected)
 */
#ifndef BUFFER_MAX
	#define BUFFER_MAX  0x100
#endif
/* buffer allocation increment */
#ifndef BUFFER_INCR
	#define BUFFER_INCR  0x10
#endif

#define CFG(dcode, cfg) ((dcode).configs[(cfg) - ZBAR_CFG_MIN_LEN])
#define TEST_CFG(config, cfg) (((config) >> (cfg)) & 1)
#define MOD(mod) (1 << (mod))
//
// return current element color 
//
static inline char get_color(const zbar_decoder_t * dcode)
{
	return (dcode->idx & 1);
}
//
// retrieve i-th previous element width 
//
static inline uint get_width(const zbar_decoder_t * dcode, uchar offset)
{
	return (dcode->w[(dcode->idx - offset) & (DECODE_WINDOW - 1)]);
}
//
// retrieve bar+space pair width starting at offset i 
//
static inline uint pair_width(const zbar_decoder_t * dcode, uchar offset)
{
	return (get_width(dcode, offset) + get_width(dcode, offset + 1));
}
/* calculate total character width "s"
 *   - start of character identified by context sensitive offset
 *     (<= DECODE_WINDOW - n)
 *   - size of character is n elements
 */
static inline uint calc_s(const zbar_decoder_t * dcode, uchar offset, uchar n)
{
	/* FIXME check that this gets unrolled for constant n */
	uint s = 0;
	while(n--)
		s += get_width(dcode, offset++);
	return(s);
}

/* fixed character width decode assist
 * bar+space width are compared as a fraction of the reference dimension "x"
 *   - +/- 1/2 x tolerance
 *   - measured total character width (s) compared to symbology baseline (n)
 *     (n = 7 for EAN/UPC, 11 for Code 128)
 *   - bar+space *pair width* "e" is used to factor out bad "exposures"
 *     ("blooming" or "swelling" of dark or light areas)
 *     => using like-edge measurements avoids these issues
 *   - n should be > 3
 */
static inline int decode_e(uint e, uint s, uint n)
{
	/* result is encoded number of units - 2
	 * (for use as zero based index)
	 * or -1 if invalid
	 */
	uchar E = ((e * n * 2 + 1) / s - 3) / 2;
	return((E >= n - 3) ? -1 : E);
}

/* sort three like-colored elements and return ordering
 */
static inline uint decode_sort3(zbar_decoder_t * dcode, int i0)
{
	const uint w0 = get_width(dcode, i0);
	const uint w2 = get_width(dcode, i0 + 2);
	const uint w4 = get_width(dcode, i0 + 4);
	if(w0 < w2) {
		if(w2 < w4)
			return((i0 << 8) | ((i0 + 2) << 4) | (i0 + 4));
		else if(w0 < w4)
			return((i0 << 8) | ((i0 + 4) << 4) | (i0 + 2));
		else
			return(((i0 + 4) << 8) | (i0 << 4) | (i0 + 2));
	}
	else if(w4 < w2)
		return(((i0 + 4) << 8) | ((i0 + 2) << 4) | i0);
	else if(w0 < w4)
		return(((i0 + 2) << 8) | (i0 << 4) | (i0 + 4));
	else
		return(((i0 + 2) << 8) | ((i0 + 4) << 4) | i0);
}
//
// sort N like-colored elements and return ordering
//
static inline uint decode_sortn(zbar_decoder_t * dcode, int n, int i0)
{
	uint mask = 0, sort = 0;
	for(int i = n - 1; i >= 0; i--) {
		uint wmin = UINT_MAX;
		int jmin = -1, j;
		for(j = n - 1; j >= 0; j--) {
			if(((mask >> j) & 1) == 0) {
				const uint w = get_width(dcode, i0 + j * 2);
				if(wmin >= w) {
					wmin = w;
					jmin = j;
				}
			}
		}
		//zassert(jmin >= 0, 0, "sortn(%d,%d) jmin=%d", n, i0, jmin);
		assert(jmin >= 0);
		sort <<= 4;
		mask |= 1 << jmin;
		sort |= i0 + jmin * 2;
	}
	return sort;
}
//
// acquire shared state lock 
//
static inline char acquire_lock(zbar_decoder_t * dcode, zbar_symbol_type_t req)
{
	if(dcode->lock) {
		dbprintf(2, " [locked %d]\n", dcode->lock);
		return 1;
	}
	else {
		dcode->lock = req;
		return 0;
	}
}
//
// check and release shared state lock 
//
static inline char release_lock(zbar_decoder_t * dcode, zbar_symbol_type_t req)
{
	//zassert(dcode->lock == req, 1, "lock=%d req=%d\n", dcode->lock, req);
	assert(dcode->lock == req);
	dcode->lock = ZBAR_NONE;
	return 0;
}
//
// ensure output buffer has sufficient allocation for request 
//
static inline char size_buf(zbar_decoder_t * dcode, uint len)
{
	if(len <= BUFFER_MIN)
		return 0;
	else if(len < dcode->buf_alloc) // FIXME size reduction heuristic?
		return 0;
	else if(len > BUFFER_MAX)
		return 1;
	else {
		if(len < (dcode->buf_alloc + BUFFER_INCR)) {
			len = dcode->buf_alloc + BUFFER_INCR;
			SETMIN(len, BUFFER_MAX);
		}
		{
			uchar * buf = (uchar *)SAlloc::R(dcode->buf, len);
			if(!buf)
				return 1;
			else {
				dcode->buf = buf;
				dcode->buf_alloc = len;
				return 0;
			}
		}
	}
}

extern const char * _zbar_decoder_buf_dump(uchar * buf, uint buflen);

#endif
