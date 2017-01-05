/*
 * $Id: bitmap.h,v 1.14 2008/11/15 19:38:54 sfeam Exp $
 */

/* GNUPLOT - bitmap.h */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
]*/

#ifndef GNUPLOT_BITMAP_H
# define GNUPLOT_BITMAP_H

//#include "syscfg.h"

/* allow up to 16 bit width for character array */
typedef uint char_row;
typedef char_row const GPFAR * GPFAR char_box;

#define FNT_CHARS   96		/* Number of characters in the font set */

#define FNT5X9 0
#define FNT5X9_VCHAR 11		/* vertical spacing between characters */
#define FNT5X9_VBITS 9		/* actual number of rows of bits per char */
#define FNT5X9_HCHAR 7		/* horizontal spacing between characters */
#define FNT5X9_HBITS 5		/* actual number of bits per row per char */
extern const char_row GPFAR fnt5x9[FNT_CHARS][FNT5X9_VBITS];

#define FNT9X17 1
#define FNT9X17_VCHAR 21	/* vertical spacing between characters */
#define FNT9X17_VBITS 17	/* actual number of rows of bits per char */
#define FNT9X17_HCHAR 13	/* horizontal spacing between characters */
#define FNT9X17_HBITS 9		/* actual number of bits per row per char */
extern const char_row GPFAR fnt9x17[FNT_CHARS][FNT9X17_VBITS];

#define FNT13X25 2
#define FNT13X25_VCHAR 31	/* vertical spacing between characters */
#define FNT13X25_VBITS 25	/* actual number of rows of bits per char */
#define FNT13X25_HCHAR 19	/* horizontal spacing between characters */
#define FNT13X25_HBITS 13	/* actual number of bits per row per char */
extern const char_row GPFAR fnt13x25[FNT_CHARS][FNT13X25_VBITS];


typedef uchar pixels;  /* the type of one set of 8 pixels in bitmap */
typedef pixels *bitmap[];	/* the bitmap */

extern bitmap *b_p;		/* global pointer to bitmap */
extern uint b_xsize, b_ysize; /* the size of the bitmap */
extern uint b_planes;	/* number of color planes */
extern uint b_psize;	/* size of each plane */
extern uint b_rastermode; /* raster mode rotates -90deg */
extern uint b_linemask;	/* 16 bit mask for dotted lines */
extern uint b_angle;	/* rotation of text */
extern int b_maskcount;


/* Prototypes from file "bitmap.c" */

uint b_getpixel(uint, uint);
void b_makebitmap(uint, uint, uint);
void b_freebitmap();
void b_charsize(uint);
void b_setvalue(uint);
void b_setlinetype(int);
void b_move(uint, uint);
void b_vector(uint, uint);
void b_put_text(uint, uint, const char *);
int b_text_angle(int);
void b_boxfill(int, uint, uint, uint, uint);

#endif /* GNUPLOT_BITMAP_H */
