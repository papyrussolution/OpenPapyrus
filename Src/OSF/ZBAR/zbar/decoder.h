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
char FASTCALL get_color(const zbar_decoder_t * dcode);
//
// retrieve i-th previous element width 
//
uint FASTCALL get_width(const zbar_decoder_t * dcode, uchar offset);
//
// retrieve bar+space pair width starting at offset i 
//
uint FASTCALL pair_width(const zbar_decoder_t * dcode, uchar offset);
// 
// calculate total character width "s"
//   - start of character identified by context sensitive offset (<= DECODE_WINDOW - n)
//   - size of character is n elements
// 
uint calc_s(const zbar_decoder_t * dcode, uchar offset, uchar n);
// 
// fixed character width decode assist
// bar+space width are compared as a fraction of the reference dimension "x"
//   - +/- 1/2 x tolerance
//   - measured total character width (s) compared to symbology baseline (n)
//     (n = 7 for EAN/UPC, 11 for Code 128)
//   - bar+space *pair width* "e" is used to factor out bad "exposures" ("blooming" or "swelling" of dark or light areas)
//     => using like-edge measurements avoids these issues
// - n should be > 3
// 
int FASTCALL decode_e(uint e, uint s, uint n);
// 
// sort three like-colored elements and return ordering
// 
uint FASTCALL decode_sort3(const zbar_decoder_t * dcode, int i0);
//
// sort N like-colored elements and return ordering
//
uint FASTCALL decode_sortn(const zbar_decoder_t * dcode, int n, int i0);
//
// acquire shared state lock 
//
char FASTCALL acquire_lock(zbar_decoder_t * dcode, zbar_symbol_type_t req);
//
// check and release shared state lock 
//
char FASTCALL release_lock(zbar_decoder_t * dcode, zbar_symbol_type_t req);
//
// ensure output buffer has sufficient allocation for request 
//
char FASTCALL size_buf(zbar_decoder_t * dcode, uint len);
extern const char * _zbar_decoder_buf_dump(uchar * buf, uint buflen);

#endif
