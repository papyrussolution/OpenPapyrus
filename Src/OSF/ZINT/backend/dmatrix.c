/* dmatrix.c Handles Data Matrix ECC 200 symbols */
/*
    libzint - the open source barcode library
    Copyright (C) 2009-2016 Robin Stuart <rstuart114@gmail.com>

    developed from and including some functions from:
        IEC16022 bar code generation
        Adrian Kennard, Andrews & Arnold Ltd
        with help from Cliff Hones on the RS coding

        (c) 2004 Adrian Kennard, Andrews & Arnold Ltd
        (c) 2006 Stefan Schmidt <stefan@datenfreihafen.org>

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name of the project nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.
 */
#include "common.h"
#pragma hdrstop
//#include "dmatrix.h"
#ifdef __cplusplus
	extern "C" {
#endif
extern int data_matrix_200(struct ZintSymbol *symbol, const uchar source[], const int length);
#ifdef __cplusplus
}
#endif
//
// Handles Data Matrix ECC 200
//
// Containes Extended Rectangular Data Matrix (DMRE)
// See http://www.eurodatacouncil.org for information
// Contact: harald.oehlmann@eurodatacouncil.org
//
#define MAXBARCODE 3116

#define DM_NULL         0
#define DM_ASCII	1
#define DM_C40		2
#define DM_TEXT		3
#define DM_X12		4
#define DM_EDIFACT	5
#define DM_BASE256	6

static const int8 c40_shift[] = { // @sobolev int-->int8
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
};

static const int8 c40_value[] = { // @sobolev int-->int8
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    3, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
    15, 16, 17, 18, 19, 20, 21, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
    22, 23, 24, 25, 26, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
};

static const int8 text_shift[] = { // @sobolev int-->int8
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3
};

static const int8 text_value[] = { // @sobolev int-->int8
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    3, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
    15, 16, 17, 18, 19, 20, 21, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
    22, 23, 24, 25, 26, 0, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 27, 28, 29, 30, 31
};

// Position in option array [symbol option value - 1]
// The position in the option array is by increasing total data codewords with square first

static const int8 intsymbol[] = { // @sobolev int-->int8
    0, /* 1: 10x10 ,  3*/ 1, /* 2: 12x12 ,  5*/ 3, /* 3: 14x14 ,  8*/ 5, /* 4: 16x16 , 12*/
    7, /* 5: 18x18 , 18*/ 9, /* 6: 20x20 , 22*/ 12, /* 7: 22x22 , 30*/ 14, /* 8: 24x24 , 36*/
    16, /* 9: 26x26 , 44*/ 21, /* 10: 32x32 , 62*/ 25, /* 11: 36x36 , 86*/ 28, /* 12: 40x40 ,114*/
    30, /* 13: 44x44 ,144*/ 31, /* 14: 48x48 ,174*/ 32, /* 15: 52x52 ,204*/ 33, /* 16: 64x64 ,280*/
    34, /* 17: 72x72 ,368*/ 35, /* 18: 80x80 ,456*/ 36, /* 19: 88x88 ,576*/ 37, /* 20: 96x96 ,696*/
    38, /* 21:104x104,816*/ 39, /* 22:120x120,1050*/40, /* 23:132x132,1304*/41, /* 24:144x144,1558*/
    2, /* 25:  8x18 ,  5*/ 4, /* 26:  8x32 , 10*/ 6, /* 27: 12x26 , 16*/ 10, /* 28: 12x36 , 22*/
    13, /* 29: 16x36 , 32*/ 17, /* 30: 16x48 , 49*/ 8, /* 31:  8x48 , 18*/ 11, /* 32:  8x64 , 24*/
    15, /* 33: 12x64 , 43*/ 22, /* 34: 16x64 , 62*/ 18, /* 35: 24x32 , 49*/ 20, /* 36: 24x36 , 55*/
    24, /* 37: 24x48 , 80*/ 27, /* 38: 24x64 ,108*/ 19, /* 39: 26x32 , 52*/ 23, /* 40: 26x40 , 70*/
    26, /* 41: 26x48 , 90*/ 29, /* 42: 26x64 ,118*/
    0
};

#define DMSIZESCOUNT 42 // Number of DM Sizes
#define INTSYMBOL144 41 // Number of 144x144 for special interlace

// Is the current code a DMRE code ?
// This is the case, if intsymbol index >= 30

static const int8 isDMRE[] = { // @sobolev int-->int8
    /*0*/ 0, /* 10x10 ,3 */ 0, /* 12x12 ,5 */ 0, /* 8x18 ,5 */ 0, /* 14x14 , 8 */
    /*4*/ 0, /* 8x32 ,10 */ 0, /* 16x16 ,12 */ 0, /* 12x26 ,16 */ 0, /* 18x18 ,18 */
    /*8*/ 1, /* 8x48 ,18 */ 0, /* 20x20 ,22 */ 0, /* 12x36 ,22 */ 1, /* 8x64 ,24 */
    /*12*/ 0, /* 22x22 ,30 */ 0, /* 16x36 ,32 */ 0, /* 24x24 ,36 */ 1, /* 12x64 ,43 */
    /*16*/ 0, /* 26x26 ,44 */ 0, /* 16x48 ,49 */ 1, /* 24x32 ,49 */ 1, /* 26x32 ,52 */
    /*20*/ 1, /* 24x36 ,55 */ 0, /* 32x32 ,62 */ 1, /* 16x64 ,62 */ 1, /* 26x40 ,70 */
    /*24*/ 1, /* 24x48 ,80 */ 0, /* 36x36 ,86 */ 1, /* 26x48 ,90 */ 1, /* 24x64 ,108*/
    /*28*/ 0, /* 40x40 ,114*/ 1, /* 26x64 ,118*/ 0, /* 44x44 ,144*/ 0, /* 48x48,174 */
    /*32*/ 0, /* 52x52,204 */ 0, /* 64x64,280 */ 0, /* 72x72,368 */ 0, /* 80x80,456 */
    /*36*/ 0, /* 88x88,576 */ 0, /* 96x96,696 */ 0, /*104x104,816*/ 0, /*120x120,1050*/
    /*40*/ 0, /*132x132,1304*/0 /*144x144,1558*/
};

// Horizontal matrix size

static const int16 matrixH[] = { // @sobolev int-->int16
    /*0*/ 10, /* 10x10 ,3 */ 12, /* 12x12 ,5 */ 8, /* 8x18 ,5 */ 14, /* 14x14 , 8 */
    /*4*/ 8, /* 8x32 ,10 */ 16, /* 16x16 ,12 */ 12, /* 12x26 ,16 */ 18, /* 18x18 ,18 */
    /*8*/ 8, /* 8x48 ,18 */ 20, /* 20x20 ,22 */ 12, /* 12x36 ,22 */ 8, /* 8x64 ,24 */
    /*12*/ 22, /* 22x22 ,30 */ 16, /* 16x36 ,32 */ 24, /* 24x24 ,36 */ 12, /* 12x64 ,43 */
    /*16*/ 26, /* 26x26 ,44 */ 16, /* 16x48 ,49 */ 24, /* 24x32 ,49 */ 26, /* 26x32 ,52 */
    /*20*/ 24, /* 24x36 ,55 */ 32, /* 32x32 ,62 */ 16, /* 16x64 ,62 */ 26, /* 26x40 ,70 */
    /*24*/ 24, /* 24x48 ,80 */ 36, /* 36x36 ,86 */ 26, /* 26x48 ,90 */ 24, /* 24x64 ,108*/
    /*28*/ 40, /* 40x40 ,114*/ 26, /* 26x64 ,118*/ 44, /* 44x44 ,144*/ 48, /* 48x48,174 */
    /*32*/ 52, /* 52x52,204 */ 64, /* 64x64,280 */ 72, /* 72x72,368 */ 80, /* 80x80,456 */
    /*36*/ 88, /* 88x88,576 */ 96, /* 96x96,696 */ 104, /*104x104,816*/ 120, /*120x120,1050*/
    /*40*/ 132, /*132x132,1304*/144/*144x144,1558*/
};

// Vertical matrix sizes

static const int16 matrixW[] = { // @sobolev int-->int16
    /*0*/ 10, /* 10x10 */ 12, /* 12x12 */ 18, /* 8x18 */ 14, /* 14x14 */
    /*4*/ 32, /* 8x32 */ 16, /* 16x16 */ 26, /* 12x26 */ 18, /* 18x18 */
    /*8*/ 48, /* 8x48 */ 20, /* 20x20 */ 36, /* 12x36 */ 64, /* 8x64 */
    /*12*/ 22, /* 22x22 */ 36, /* 16x36 */ 24, /* 24x24 */ 64, /* 12x64 */
    /*16*/ 26, /* 26x26 */ 48, /* 16x48 */ 32, /* 24x32 */ 32, /* 26x32 */
    /*20*/ 36, /* 24x36 */ 32, /* 32x32 */ 64, /* 16x64 */ 40, /* 26x40 */
    /*24*/ 48, /* 24x48 */ 36, /* 36x36 */ 48, /* 26x48 */ 64, /* 24x64 */
    /*28*/ 40, /* 40x40 */ 64, /* 26x64 */ 44, /* 44x44 */ 48, /* 48x48 */
    /*32*/ 52, /* 52x52 */ 64, /* 64x64 */ 72, /* 72x72 */ 80, /* 80x80 */
    /*36*/ 88, /* 88x88 */ 96, /* 96x96 */ 104, /*104x104*/ 120, /*120x120*/
    /*40*/ 132, /*132x132*/ 144 /*144x144*/
};

// Horizontal submodule size (including subfinder)

static const int8 matrixFH[] = { // @sobolev int-->int8
    /*0*/ 10, /* 10x10 */ 12, /* 12x12 */ 8, /* 8x18 */ 14, /* 14x14 */
    /*4*/ 8, /* 8x32 */ 16, /* 16x16 */ 12, /* 12x26 */ 18, /* 18x18 */
    /*8*/ 8, /* 8x48 */ 20, /* 20x20 */ 12, /* 12x36 */ 8, /* 8x64 */
    /*12*/ 22, /* 22x22 */ 16, /* 16x36 */ 24, /* 24x24 */ 12, /* 12x64 */
    /*16*/ 26, /* 26x26 */ 16, /* 16x48 */ 24, /* 24x32 */ 26, /* 26x32 */
    /*20*/ 24, /* 24x36 */ 16, /* 32x32 */ 16, /* 16x64 */ 26, /* 26x40 */
    /*24*/ 24, /* 24x48 */ 18, /* 36x36 */ 26, /* 26x48 */ 24, /* 24x64 */
    /*28*/ 20, /* 40x40 */ 26, /* 26x64 */ 22, /* 44x44 */ 24, /* 48x48 */
    /*32*/ 26, /* 52x52 */ 16, /* 64x64 */ 18, /* 72x72 */ 20, /* 80x80 */
    /*36*/ 22, /* 88x88 */ 24, /* 96x96 */ 26, /*104x104*/ 20, /*120x120*/
    /*40*/ 22, /*132x132*/ 24 /*144x144*/
};

// Vertical submodule size (including subfinder)

static const int8 matrixFW[] = { // @sobolev int-->int8
    /*0*/ 10, /* 10x10 */ 12, /* 12x12 */ 18, /* 8x18 */ 14, /* 14x14 */
    /*4*/ 16, /* 8x32 */ 16, /* 16x16 */ 26, /* 12x26 */ 18, /* 18x18 */
    /*8*/ 24, /* 8x48 */ 20, /* 20x20 */ 18, /* 12x36 */ 16, /* 8x64 */
    /*12*/ 22, /* 22x22 */ 18, /* 16x36 */ 24, /* 24x24 */ 16, /* 12x64 */
    /*16*/ 26, /* 26x26 */ 24, /* 16x48 */ 16, /* 24x32 */ 16, /* 26x32 */
    /*20*/ 18, /* 24x36 */ 16, /* 32x32 */ 16, /* 16x64 */ 20, /* 26x40 */
    /*24*/ 24, /* 24x48 */ 18, /* 36x36 */ 24, /* 26x48 */ 16, /* 24x64 */
    /*28*/ 20, /* 40x40 */ 16, /* 26x64 */ 22, /* 44x44 */ 24, /* 48x48 */
    /*32*/ 26, /* 52x52 */ 16, /* 64x64 */ 18, /* 72x72 */ 20, /* 80x80 */
    /*36*/ 22, /* 88x88 */ 24, /* 96x96 */ 26, /*104x104*/ 20, /*120x120*/
    /*40*/ 22, /*132x132*/ 24 /*144x144*/
};

// Total Data Codewords

static const int16 matrixbytes[] = { // @sobolev int-->int16
    /*0*/ 3, /* 10x10 */ 5, /* 12x12 */ 5, /* 8x18 */ 8, /* 14x14 */
    /*4*/ 10, /* 8x32 */ 12, /* 16x16 */ 16, /* 12x26 */ 18, /* 18x18 */
    /*8*/ 18, /* 8x48 */ 22, /* 20x20 */ 22, /* 12x36 */ 24, /* 8x64 */
    /*12*/ 30, /* 22x22 */ 32, /* 16x36 */ 36, /* 24x24 */ 43, /* 12x64 */
    /*16*/ 44, /* 26x26 */ 49, /* 16x48 */ 49, /* 24x32 */ 52, /* 26x32 */
    /*20*/ 55, /* 24x36 */ 62, /* 32x32 */ 62, /* 16x64 */ 70, /* 26x40 */
    /*24*/ 80, /* 24x48 */ 86, /* 36x36 */ 90, /* 26x48 */ 108, /* 24x64 */
    /*28*/ 114, /* 40x40 */ 118, /* 26x64 */ 144, /* 44x44 */ 174, /* 48x48 */
    /*32*/ 204, /* 52x52 */ 280, /* 64x64 */ 368, /* 72x72 */ 456, /* 80x80 */
    /*36*/ 576, /* 88x88 */ 696, /* 96x96 */ 816, /*104x104*/ 1050, /*120x120*/
    /*40*/ 1304, /*132x132*/ 1558 /*144x144*/
};

// Data Codewords per RS-Block

static const int16 matrixdatablock[] = { // @sobolev int-->int16
    /*0*/ 3, /* 10x10 */ 5, /* 12x12 */ 5, /* 8x18 */ 8, /* 14x14 */
    /*4*/ 10, /* 8x32 */ 12, /* 16x16 */ 16, /* 12x26 */ 18, /* 18x18 */
    /*8*/ 18, /* 8x48 */ 22, /* 20x20 */ 22, /* 12x36 */ 24, /* 8x64 */
    /*12*/ 30, /* 22x22 */ 32, /* 16x36 */ 36, /* 24x24 */ 43, /* 12x64 */
    /*16*/ 44, /* 26x26 */ 49, /* 16x48 */ 49, /* 24x32 */ 52, /* 26x32 */
    /*20*/ 55, /* 24x36 */ 62, /* 32x32 */ 62, /* 16x64 */ 70, /* 26x40 */
    /*24*/ 80, /* 24x48 */ 86, /* 36x36 */ 90, /* 26x48 */ 108, /* 24x64 */
    /*28*/ 114, /* 40x40 */ 118, /* 26x64 */ 144, /* 44x44 */ 174, /* 48x48 */
    /*32*/ 102, /* 52x52 */ 140, /* 64x64 */ 92, /* 72x72 */ 114, /* 80x80 */
    /*36*/ 144, /* 88x88 */ 174, /* 96x96 */ 136, /*104x104*/ 175, /*120x120*/
    /*40*/ 163, /*132x132*/ 156 /*144x144*/
};

// ECC Codewords per RS-Block

static const int8 matrixrsblock[] = { // @sobolev int-->int8
    /*0*/ 5, /* 10x10 */ 7, /* 12x12 */ 7, /* 8x18 */ 10, /* 14x14 */
    /*4*/ 11, /* 8x32 */ 12, /* 16x16 */ 14, /* 12x26 */ 14, /* 18x18 */
    /*8*/ 15, /* 8x48 */ 18, /* 20x20 */ 18, /* 12x36 */ 18, /* 8x64 */
    /*12*/ 20, /* 22x22 */ 24, /* 16x36 */ 24, /* 24x24 */ 27, /* 12x64 */
    /*16*/ 28, /* 26x26 */ 28, /* 16x48 */ 28, /* 24x32 */ 32, /* 26x32 */
    /*20*/ 33, /* 24x36 */ 36, /* 32x32 */ 36, /* 16x64 */ 38, /* 26x40 */
    /*24*/ 41, /* 24x48 */ 42, /* 36x36 */ 42, /* 26x48 */ 46, /* 24x64 */
    /*28*/ 48, /* 40x40 */ 50, /* 26x64 */ 56, /* 44x44 */ 68, /* 48x48 */
    /*32*/ 42, /* 52x52 */ 56, /* 64x64 */ 36, /* 72x72 */ 48, /* 80x80 */
    /*36*/ 56, /* 88x88 */ 68, /* 96x96 */ 56, /*104x104*/ 68, /*120x120*/
    /*40*/ 62, /*132x132*/ 62 /*144x144*/
};
//
// Annex M placement alorithm low level 
//
static void ecc200placementbit(int * array, const int NR, const int NC, int r, int c, const int p, const char b)
{
	if(r < 0) {
		r += NR;
		c += 4 - ((NR + 4) % 8);
	}
	if(c < 0) {
		c += NC;
		r += 4 - ((NC + 4) % 8);
	}
	// Necessary for 26x32,26x40,26x48,36x120,36x144,72x120,72x144
	if(r >= NR) {
#ifdef DEBUG
		slfprintf_stderr("r >= NR:%i,%i at r=%i->", p, b, r);
#endif
		r -= NR;
#ifdef DEBUG
		slfprintf_stderr("%i,c=%i\n", r, c);
#endif
	}
#ifdef DEBUG
	if(0 != array[r * NC + c]) {
		int a = array[r * NC + c];
		slfprintf_stderr("Double:%i,%i->%i,%i at r=%i,c=%i\n", a >> 3, a & 7, p, b, r, c);
		return;
	}
#endif
	// Check index limits
	assert(r < NR);
	assert(c < NC);
	// Check double-assignment
	assert(0 == array[r * NC + c]);
	array[r * NC + c] = (p << 3) + b;
}

static void ecc200placementblock(int * array, const int NR, const int NC, const int r, const int c, const int p)
{
	ecc200placementbit(array, NR, NC, r - 2, c - 2, p, 7);
	ecc200placementbit(array, NR, NC, r - 2, c - 1, p, 6);
	ecc200placementbit(array, NR, NC, r - 1, c - 2, p, 5);
	ecc200placementbit(array, NR, NC, r - 1, c - 1, p, 4);
	ecc200placementbit(array, NR, NC, r - 1, c - 0, p, 3);
	ecc200placementbit(array, NR, NC, r - 0, c - 2, p, 2);
	ecc200placementbit(array, NR, NC, r - 0, c - 1, p, 1);
	ecc200placementbit(array, NR, NC, r - 0, c - 0, p, 0);
}

static void ecc200placementcornerA(int * array, const int NR, const int NC, const int p)
{
	ecc200placementbit(array, NR, NC, NR - 1, 0, p, 7);
	ecc200placementbit(array, NR, NC, NR - 1, 1, p, 6);
	ecc200placementbit(array, NR, NC, NR - 1, 2, p, 5);
	ecc200placementbit(array, NR, NC, 0, NC - 2, p, 4);
	ecc200placementbit(array, NR, NC, 0, NC - 1, p, 3);
	ecc200placementbit(array, NR, NC, 1, NC - 1, p, 2);
	ecc200placementbit(array, NR, NC, 2, NC - 1, p, 1);
	ecc200placementbit(array, NR, NC, 3, NC - 1, p, 0);
}

static void ecc200placementcornerB(int * array, const int NR, const int NC, const int p)
{
	ecc200placementbit(array, NR, NC, NR - 3, 0, p, 7);
	ecc200placementbit(array, NR, NC, NR - 2, 0, p, 6);
	ecc200placementbit(array, NR, NC, NR - 1, 0, p, 5);
	ecc200placementbit(array, NR, NC, 0, NC - 4, p, 4);
	ecc200placementbit(array, NR, NC, 0, NC - 3, p, 3);
	ecc200placementbit(array, NR, NC, 0, NC - 2, p, 2);
	ecc200placementbit(array, NR, NC, 0, NC - 1, p, 1);
	ecc200placementbit(array, NR, NC, 1, NC - 1, p, 0);
}

static void ecc200placementcornerC(int * array, const int NR, const int NC, const int p)
{
	ecc200placementbit(array, NR, NC, NR - 3, 0, p, 7);
	ecc200placementbit(array, NR, NC, NR - 2, 0, p, 6);
	ecc200placementbit(array, NR, NC, NR - 1, 0, p, 5);
	ecc200placementbit(array, NR, NC, 0, NC - 2, p, 4);
	ecc200placementbit(array, NR, NC, 0, NC - 1, p, 3);
	ecc200placementbit(array, NR, NC, 1, NC - 1, p, 2);
	ecc200placementbit(array, NR, NC, 2, NC - 1, p, 1);
	ecc200placementbit(array, NR, NC, 3, NC - 1, p, 0);
}

static void ecc200placementcornerD(int * array, const int NR, const int NC, const int p)
{
	ecc200placementbit(array, NR, NC, NR - 1, 0, p, 7);
	ecc200placementbit(array, NR, NC, NR - 1, NC - 1, p, 6);
	ecc200placementbit(array, NR, NC, 0, NC - 3, p, 5);
	ecc200placementbit(array, NR, NC, 0, NC - 2, p, 4);
	ecc200placementbit(array, NR, NC, 0, NC - 1, p, 3);
	ecc200placementbit(array, NR, NC, 1, NC - 3, p, 2);
	ecc200placementbit(array, NR, NC, 1, NC - 2, p, 1);
	ecc200placementbit(array, NR, NC, 1, NC - 1, p, 0);
}

/* Annex M placement alorithm main function */
static void ecc200placement(int * array, const int NR, const int NC)
{
	int r, c, p;
	// invalidate
	for(r = 0; r < NR; r++)
		for(c = 0; c < NC; c++)
			array[r * NC + c] = 0;
	// start
	p = 1;
	r = 4;
	c = 0;
	do {
		// check corner
		if(r == NR && !c)
			ecc200placementcornerA(array, NR, NC, p++);
		if(r == NR - 2 && !c && NC % 4)
			ecc200placementcornerB(array, NR, NC, p++);
		if(r == NR - 2 && !c && (NC % 8) == 4)
			ecc200placementcornerC(array, NR, NC, p++);
		if(r == NR + 4 && c == 2 && !(NC % 8))
			ecc200placementcornerD(array, NR, NC, p++);
		// up/right
		do {
			if(r < NR && c >= 0 && !array[r * NC + c])
				ecc200placementblock(array, NR, NC, r, c, p++);
			r -= 2;
			c += 2;
		} while(r >= 0 && c < NC);
		r++;
		c += 3;
		// down/left
		do {
			if(r >= 0 && c < NC && !array[r * NC + c])
				ecc200placementblock(array, NR, NC, r, c, p++);
			r += 2;
			c -= 2;
		} while(r < NR && c >= 0);
		r += 3;
		c++;
	} while(r < NR || c < NC);
	// unfilled corner
	if(!array[NR * NC - 1])
		array[NR * NC - 1] = array[NR * NC - NC - 2] = 1;
}

/* calculate and append ecc code, and if necessary interleave */
static void ecc200(uchar * binary, const int bytes, const int datablock, const int rsblock, const int skew)
{
	int blocks = (bytes + 2) / datablock, b;
	int n, p;
	rs_init_gf(0x12d);
	rs_init_code(rsblock, 1);
	for(b = 0; b < blocks; b++) {
		uchar buf[256], ecc[256];
		p = 0;
		for(n = b; n < bytes; n += blocks)
			buf[p++] = binary[n];
		rs_encode(p, buf, ecc);
		p = rsblock - 1; // comes back reversed
		for(n = b; n < rsblock * blocks; n += blocks) {
			if(skew) {
				/* Rotate ecc data to make 144x144 size symbols acceptable */
				/* See http://groups.google.com/group/postscriptbarcode/msg/5ae8fda7757477da */
				if(b < 8) {
					binary[bytes + n + 2] = ecc[p--];
				}
				else {
					binary[bytes + n - 8] = ecc[p--];
				}
			}
			else {
				binary[bytes + n] = ecc[p--];
			}
		}
	}
	rs_free();
}
//
// Return true (1) if a character is valid in X12 set 
//
static int FASTCALL isX12(const int source)
{
	return BIN(oneof4(source, 13, 42, 62, 32) || ((source >= '0') && (source <= '9')) || ((source >= 'A') && (source <= 'Z')));
	/*
	if(source == 13)
		return 1;
	else if(source == 42)
		return 1;
	else if(source == 62)
		return 1;
	else if(source == 32)
		return 1;
	else if((source >= '0') && (source <= '9'))
		return 1;
	else if((source >= 'A') && (source <= 'Z'))
		return 1;
	else
		return 0;
	*/
}

/* Insert a character into the middle of a string at position posn */
static void dminsert(char binary_string[], const int posn, const char newbit)
{
	const int end = (int)strlen(binary_string);
	for(int i = end + 1; i > posn; i--) {
		binary_string[i] = binary_string[i - 1];
	}
	binary_string[posn] = newbit;
}

static void insert_value(uchar binary_stream[], const int posn, const int streamlen, const int newbit)
{
	for(int i = streamlen; i > posn; i--) {
		binary_stream[i] = binary_stream[i - 1];
	}
	binary_stream[posn] = (uchar)newbit;
}

static int p_r_6_2_1(const uchar inputData[], const int position, const int sourcelen)
{
	/* Annex P section (r)(6)(ii)(I)
	   "If one of the three X12 terminator/separator characters first
	    occurs in the yet to be processed data before a non-X12 character..."
	 */

	int i;
	int nonX12Position = 0;
	int specialX12Position = 0;
	int retval = 0;
	for(i = position; i < sourcelen; i++) {
		if(nonX12Position == 0) {
			if(isX12(inputData[i]) != 1) {
				nonX12Position = i;
			}
		}
		if(specialX12Position == 0) {
			if((inputData[i] == (char)13) || (inputData[i] == '*') || (inputData[i] == '>')) {
				specialX12Position = i;
			}
		}
	}
	if((nonX12Position != 0) && (specialX12Position != 0)) {
		if(specialX12Position < nonX12Position) {
			retval = 1;
		}
	}
	return retval;
}

/* 'look ahead test' from Annex P */
static int look_ahead_test(const uchar inputData[], const int sourcelen, const int position, const int current_mode, const int gs1)
{
	float ascii_count, c40_count, text_count, x12_count, edf_count, b256_count, best_count;
	const float stiction = (1.0F / 24.0F); // smallest change to act on, to get around floating point inaccuracies
	int sp;
	int best_scheme = DM_NULL;
	/* step (j) */
	if(current_mode == DM_ASCII) {
		ascii_count = 0.0F;
		c40_count = 1.0F;
		text_count = 1.0F;
		x12_count = 1.0F;
		edf_count = 1.0F;
		b256_count = 1.25F;
	}
	else {
		ascii_count = 1.0F;
		c40_count = 2.0F;
		text_count = 2.0F;
		x12_count = 2.0F;
		edf_count = 2.0F;
		b256_count = 2.25F;
	}

	switch(current_mode) {
		case DM_C40: c40_count = 0.0F; break;
		case DM_TEXT: text_count = 0.0F; break;
		case DM_X12: x12_count = 0.0F; break;
		case DM_EDIFACT: edf_count = 0.0F; break;
		case DM_BASE256: b256_count = 0.0F; break;
	}
	sp = position;
	do {
		if(sp == sourcelen) {
			/* At the end of data ... step (k) */
			ascii_count = ceilf(ascii_count);
			b256_count = ceilf(b256_count);
			edf_count = ceilf(edf_count);
			text_count = ceilf(text_count);
			x12_count = ceilf(x12_count);
			c40_count = ceilf(c40_count);

			best_count = c40_count;
			best_scheme = DM_C40; // (k)(7)

			if(x12_count < (best_count - stiction)) {
				best_count = x12_count;
				best_scheme = DM_X12; // (k)(6)
			}

			if(text_count < (best_count - stiction)) {
				best_count = text_count;
				best_scheme = DM_TEXT; // (k)(5)
			}

			if(edf_count < (best_count - stiction)) {
				best_count = edf_count;
				best_scheme = DM_EDIFACT; // (k)(4)
			}

			if(b256_count < (best_count - stiction)) {
				best_count = b256_count;
				best_scheme = DM_BASE256; // (k)(3)
			}

			if(ascii_count <= (best_count + stiction)) {
				best_scheme = DM_ASCII; // (k)(2)
			}
		}
		else {
			/* ascii ... step (l) */
			if((inputData[sp] >= '0') && (inputData[sp] <= '9')) {
				ascii_count += 0.5F; // (l)(1)
			}
			else {
				if(inputData[sp] > 127) {
					ascii_count = ceilf(ascii_count) + 2.0F; // (l)(2)
				}
				else {
					ascii_count = ceilf(ascii_count) + 1.0F; // (l)(3)
				}
			}
			/* c40 ... step (m) */
			if((inputData[sp] == ' ') || (((inputData[sp] >= '0') && (inputData[sp] <= '9')) || ((inputData[sp] >= 'A') && (inputData[sp] <= 'Z')))) {
				c40_count += (2.0F / 3.0F); // (m)(1)
			}
			else {
				if(inputData[sp] > 127) {
					c40_count += (8.0F / 3.0F); // (m)(2)
				}
				else {
					c40_count += (4.0F / 3.0F); // (m)(3)
				}
			}

			/* text ... step (n) */
			if((inputData[sp] == ' ') || (((inputData[sp] >= '0') && (inputData[sp] <= '9')) || ((inputData[sp] >= 'a') && (inputData[sp] <= 'z')))) {
				text_count += (2.0F / 3.0F); // (n)(1)
			}
			else {
				if(inputData[sp] > 127) {
					text_count += (8.0F / 3.0F); // (n)(2)
				}
				else {
					text_count += (4.0F / 3.0F); // (n)(3)
				}
			}
			/* x12 ... step (o) */
			if(isX12(inputData[sp])) {
				x12_count += (2.0F / 3.0F); // (o)(1)
			}
			else {
				if(inputData[sp] > 127) {
					x12_count += (13.0F / 3.0F); // (o)(2)
				}
				else {
					x12_count += (10.0F / 3.0F); // (o)(3)
				}
			}

			/* edifact ... step (p) */
			if((inputData[sp] >= ' ') && (inputData[sp] <= '^')) {
				edf_count += (3.0F / 4.0F); // (p)(1)
			}
			else {
				if(inputData[sp] > 127) {
					edf_count += 17.0F; // (p)(2) > Value changed from ISO
				}
				else {
					edf_count += 13.0F; // (p)(3) > Value changed from ISO
				}
			}
			if((gs1 == 1) && (inputData[sp] == '[')) {
				edf_count += 13.0F; //  > Value changed from ISO
			}

			/* base 256 ... step (q) */
			if((gs1 == 1) && (inputData[sp] == '[')) {
				b256_count += 4.0F; // (q)(1)
			}
			else {
				b256_count += 1.0F; // (q)(2)
			}
		}

		if(sp > (position + 3)) {
			/* 4 data characters processed ... step (r) */

			/* step (r)(6) */
			if(((c40_count + 1.0F) < (ascii_count - stiction)) &&
			    ((c40_count + 1.0F) < (b256_count - stiction)) &&
			    ((c40_count + 1.0F) < (edf_count - stiction)) &&
			    ((c40_count + 1.0F) < (text_count - stiction))) {
				if(c40_count < (x12_count - stiction)) {
					best_scheme = DM_C40;
				}

				if((c40_count >= (x12_count - stiction))
				    && (c40_count <= (x12_count + stiction))) {
					if(p_r_6_2_1(inputData, sp, sourcelen) == 1) {
						// Test (r)(6)(ii)(i)
						best_scheme = DM_X12;
					}
					else {
						best_scheme = DM_C40;
					}
				}
			}

			/* step (r)(5) */
			if(((x12_count + 1.0F) < (ascii_count - stiction)) &&
			    ((x12_count + 1.0F) < (b256_count - stiction)) &&
			    ((x12_count + 1.0F) < (edf_count - stiction)) &&
			    ((x12_count + 1.0F) < (text_count - stiction)) &&
			    ((x12_count + 1.0F) < (c40_count - stiction))) {
				best_scheme = DM_X12;
			}

			/* step (r)(4) */
			if(((text_count + 1.0F) < (ascii_count - stiction)) &&
			    ((text_count + 1.0F) < (b256_count - stiction)) &&
			    ((text_count + 1.0F) < (edf_count - stiction)) &&
			    ((text_count + 1.0F) < (x12_count - stiction)) &&
			    ((text_count + 1.0F) < (c40_count - stiction))) {
				best_scheme = DM_TEXT;
			}

			/* step (r)(3) */
			if(((edf_count + 1.0F) < (ascii_count - stiction)) &&
			    ((edf_count + 1.0F) < (b256_count - stiction)) &&
			    ((edf_count + 1.0F) < (text_count - stiction)) &&
			    ((edf_count + 1.0F) < (x12_count - stiction)) &&
			    ((edf_count + 1.0F) < (c40_count - stiction))) {
				best_scheme = DM_EDIFACT;
			}

			/* step (r)(2) */
			if(((b256_count + 1.0F) <= (ascii_count + stiction)) || (((b256_count + 1.0F) < (edf_count - stiction)) &&
				    ((b256_count + 1.0F) < (text_count - stiction)) && ((b256_count + 1.0F) < (x12_count - stiction)) &&
				    ((b256_count + 1.0F) < (c40_count - stiction)))) {
				best_scheme = DM_BASE256;
			}

			/* step (r)(1) */
			if(((ascii_count + 1.0F) <= (b256_count + stiction)) && ((ascii_count + 1.0F) <= (edf_count + stiction)) &&
			    ((ascii_count + 1.0F) <= (text_count + stiction)) && ((ascii_count + 1.0F) <= (x12_count + stiction)) &&
			    ((ascii_count + 1.0F) <= (c40_count + stiction))) {
				best_scheme = DM_ASCII;
			}
		}

		//printf("Char %d[%c]: ASC:%.2f C40:%.2f X12:%.2f TXT:%.2f EDI:%.2f BIN:%.2f\n", sp,
		//        inputData[sp], ascii_count, c40_count, x12_count, text_count, edf_count, b256_count);

		sp++;
	} while(best_scheme == DM_NULL); // step (s)

	return best_scheme;
}

/* Encodes data using ASCII, C40, Text, X12, EDIFACT or Base 256 modes as appropriate
   Supports encoding FNC1 in supporting systems */
static int dm200encode(struct ZintSymbol * symbol, const uchar source[],
    uchar target[], int * last_mode, int * length_p, int process_buffer[], int * process_p)
{
	int sp, tp, i, gs1;
	int current_mode, next_mode;
	int inputlen = *length_p;
	int debug = 0;
#ifndef _MSC_VER
	char binary[2 * inputlen];
#else
	char* binary = (char *)_alloca(2 * inputlen);
#endif
	sp = 0;
	tp = 0;
	memzero(process_buffer, 8);
	*process_p = 0;
	sstrcpy(binary, "");
	/* step (a) */
	current_mode = DM_ASCII;
	next_mode = DM_ASCII;

	if(symbol->input_mode == GS1_MODE) {
		gs1 = 1;
	}
	else {
		gs1 = 0;
	}

	if(gs1) {
		target[tp] = 232;
		tp++;
		strcat(binary, " ");
		if(debug) printf("FN1 ");
	} /* FNC1 */

	if(symbol->output_options & READER_INIT) {
		if(gs1) {
			sstrcpy(symbol->errtxt, "Cannot encode in GS1 mode and Reader Initialisation at the same time (E10)");
			return ZINT_ERROR_INVALID_OPTION;
		}
		else {
			target[tp] = 234;
			tp++; /* Reader Programming */
			strcat(binary, " ");
			if(debug) printf("RP ");
		}
	}

	if(symbol->eci > 3) {
		target[tp] = 241; /* ECI Character */
		tp++;
		target[tp] = (uchar)(symbol->eci + 1);
		tp++;
		if(debug) printf("ECI %d ", symbol->eci + 1);
	}
	/* Check for Macro05/Macro06 */
	/* "[)>[RS]05[GS]...[RS][EOT]" -> CW 236 */
	/* "[)>[RS]06[GS]...[RS][EOT]" -> CW 237 */
	if(tp == 0 && sp == 0 && inputlen >= 9 && source[0] == '[' && source[1] == ')' && source[2] == '>' && 
		source[3] == '\x1e' && source[4] == '0' && (source[5] == '5' || source[5] == '6') && source[6] == '\x1d' && source[inputlen-2] == '\x1e' && source[inputlen-1] == '\x04') {
		/* Output macro Codeword */
		if(source[5] == '5') {
			target[tp] = 236;
			if(debug) printf("Macro05 ");
		}
		else {
			target[tp] = 237;
			if(debug) printf("Macro06 ");
		}
		tp++;
		strcat(binary, " ");
		/* Remove macro characters from input string */
		sp = 7;
		inputlen -= 2;
		*length_p -= 2;
	}
	while(sp < inputlen) {
		current_mode = next_mode;
		/* step (b) - ASCII encodation */
		if(current_mode == DM_ASCII) {
			next_mode = DM_ASCII;

			if(istwodigits(source, sp) && ((sp + 1) != inputlen)) {
				target[tp] = (uchar)((10 * hex(source[sp])) + hex(source[sp+1]) + 130);
				if(debug) printf("N%d ", target[tp] - 130);
				tp++;
				strcat(binary, " ");
				sp += 2;
			}
			else {
				next_mode = look_ahead_test(source, inputlen, sp, current_mode, gs1);

				if(next_mode != DM_ASCII) {
					switch(next_mode) {
						case DM_C40: target[tp] = 230;
						    tp++;
						    strcat(binary, " ");
						    if(debug) printf("C40 ");
						    break;
						case DM_TEXT: target[tp] = 239;
						    tp++;
						    strcat(binary, " ");
						    if(debug) printf("TEX ");
						    break;
						case DM_X12: target[tp] = 238;
						    tp++;
						    strcat(binary, " ");
						    if(debug) printf("X12 ");
						    break;
						case DM_EDIFACT: target[tp] = 240;
						    tp++;
						    strcat(binary, " ");
						    if(debug) printf("EDI ");
						    break;
						case DM_BASE256: target[tp] = 231;
						    tp++;
						    strcat(binary, " ");
						    if(debug) printf("BAS ");
						    break;
					}
				}
				else {
					if(source[sp] > 127) {
						target[tp] = 235; /* FNC4 */
						if(debug) printf("FN4 ");
						tp++;
						target[tp] = (source[sp] - 128) + 1;
						if(debug) printf("A%02X ", target[tp] - 1);
						tp++;
						strcat(binary, "  ");
					}
					else {
						if(gs1 && (source[sp] == '[')) {
							target[tp] = 232; /* FNC1 */
							if(debug) printf("FN1 ");
						}
						else {
							target[tp] = source[sp] + 1;
							if(debug) printf("A%02X ", target[tp] - 1);
						}
						tp++;
						strcat(binary, " ");
					}
					sp++;
				}
			}
		}

		/* step (c) C40 encodation */
		if(current_mode == DM_C40) {
			int shift_set, value;

			next_mode = DM_C40;
			if(*process_p == 0) {
				next_mode = look_ahead_test(source, inputlen, sp, current_mode, gs1);
			}

			if(next_mode != DM_C40) {
				target[tp] = 254;
				tp++;
				strcat(binary, " "); /* Unlatch */
				next_mode = DM_ASCII;
				if(debug) printf("ASC ");
			}
			else {
				if(source[sp] > 127) {
					process_buffer[*process_p] = 1;
					(*process_p)++;
					process_buffer[*process_p] = 30;
					(*process_p)++; /* Upper Shift */
					shift_set = c40_shift[source[sp] - 128];
					value = c40_value[source[sp] - 128];
				}
				else {
					shift_set = c40_shift[source[sp]];
					value = c40_value[source[sp]];
				}

				if(gs1 && (source[sp] == '[')) {
					shift_set = 2;
					value = 27; /* FNC1 */
				}

				if(shift_set != 0) {
					process_buffer[*process_p] = shift_set - 1;
					(*process_p)++;
				}
				process_buffer[*process_p] = value;
				(*process_p)++;

				if(*process_p >= 3) {
					int iv;

					iv = (1600 * process_buffer[0]) + (40 * process_buffer[1]) + (process_buffer[2]) + 1;
					target[tp] = (uchar)(iv / 256);
					tp++;
					target[tp] = iv % 256;
					tp++;
					strcat(binary, "  ");
					if(debug) printf("[%d %d %d] ", process_buffer[0], process_buffer[1], process_buffer[2]);

					process_buffer[0] = process_buffer[3];
					process_buffer[1] = process_buffer[4];
					process_buffer[2] = process_buffer[5];
					process_buffer[3] = 0;
					process_buffer[4] = 0;
					process_buffer[5] = 0;
					*process_p -= 3;
				}
				sp++;
			}
		}

		/* step (d) Text encodation */
		if(current_mode == DM_TEXT) {
			int shift_set, value;
			next_mode = DM_TEXT;
			if(*process_p == 0) {
				next_mode = look_ahead_test(source, inputlen, sp, current_mode, gs1);
			}
			if(next_mode != DM_TEXT) {
				target[tp++] = 254;
				strcat(binary, " "); /* Unlatch */
				next_mode = DM_ASCII;
				if(debug) printf("ASC ");
			}
			else {
				if(source[sp] > 127) {
					process_buffer[*process_p] = 1;
					(*process_p)++;
					process_buffer[*process_p] = 30;
					(*process_p)++; /* Upper Shift */
					shift_set = text_shift[source[sp] - 128];
					value = text_value[source[sp] - 128];
				}
				else {
					shift_set = text_shift[source[sp]];
					value = text_value[source[sp]];
				}
				if(gs1 && (source[sp] == '[')) {
					shift_set = 2;
					value = 27; /* FNC1 */
				}
				if(shift_set != 0) {
					process_buffer[*process_p] = shift_set - 1;
					(*process_p)++;
				}
				process_buffer[*process_p] = value;
				(*process_p)++;

				if(*process_p >= 3) {
					int iv = (1600 * process_buffer[0]) + (40 * process_buffer[1]) + (process_buffer[2]) + 1;
					target[tp++] = (uchar)(iv / 256);
					target[tp++] = iv % 256;
					strcat(binary, "  ");
					if(debug) printf("[%d %d %d] ", process_buffer[0], process_buffer[1], process_buffer[2]);

					process_buffer[0] = process_buffer[3];
					process_buffer[1] = process_buffer[4];
					process_buffer[2] = process_buffer[5];
					process_buffer[3] = 0;
					process_buffer[4] = 0;
					process_buffer[5] = 0;
					*process_p -= 3;
				}
				sp++;
			}
		}

		/* step (e) X12 encodation */
		if(current_mode == DM_X12) {
			int value = 0;
			next_mode = DM_X12;
			if(*process_p == 0) {
				next_mode = look_ahead_test(source, inputlen, sp, current_mode, gs1);
			}
			if(next_mode != DM_X12) {
				target[tp++] = 254;
				strcat(binary, " "); /* Unlatch */
				next_mode = DM_ASCII;
				if(debug) printf("ASC ");
			}
			else {
				if(source[sp] == 13) {
					value = 0;
				}
				if(source[sp] == '*') {
					value = 1;
				}
				if(source[sp] == '>') {
					value = 2;
				}
				if(source[sp] == ' ') {
					value = 3;
				}
				if(isdec(source[sp])) {
					value = (source[sp] - '0') + 4;
				}
				if((source[sp] >= 'A') && (source[sp] <= 'Z')) {
					value = (source[sp] - 'A') + 14;
				}
				process_buffer[*process_p] = value;
				(*process_p)++;
				if(*process_p >= 3) {
					int iv = (1600 * process_buffer[0]) + (40 * process_buffer[1]) + (process_buffer[2]) + 1;
					target[tp++] = (uchar)(iv / 256);
					target[tp++] = iv % 256;
					strcat(binary, "  ");
					if(debug) printf("[%d %d %d] ", process_buffer[0], process_buffer[1], process_buffer[2]);

					process_buffer[0] = process_buffer[3];
					process_buffer[1] = process_buffer[4];
					process_buffer[2] = process_buffer[5];
					process_buffer[3] = 0;
					process_buffer[4] = 0;
					process_buffer[5] = 0;
					*process_p -= 3;
				}
				sp++;
			}
		}

		/* step (f) EDIFACT encodation */
		if(current_mode == DM_EDIFACT) {
			int value = 0;
			next_mode = DM_EDIFACT;
			if(*process_p == 3) {
				next_mode = look_ahead_test(source, inputlen, sp, current_mode, gs1);
			}
			if(next_mode != DM_EDIFACT) {
				process_buffer[*process_p] = 31;
				(*process_p)++;
				next_mode = DM_ASCII;
			}
			else {
				value = source[sp];
				if(source[sp] >= 64) { // '@'
					value -= 64;
				}
				process_buffer[*process_p] = value;
				(*process_p)++;
				sp++;
			}
			if(*process_p >= 4) {
				target[tp++] = (uchar)((process_buffer[0] << 2) + ((process_buffer[1] & 0x30) >> 4));
				target[tp++] = ((process_buffer[1] & 0x0f) << 4) + ((process_buffer[2] & 0x3c) >> 2);
				target[tp++] = (uchar)(((process_buffer[2] & 0x03) << 6) + process_buffer[3]);
				strcat(binary, "   ");
				if(debug) printf("[%d %d %d %d] ", process_buffer[0], process_buffer[1], process_buffer[2], process_buffer[3]);

				process_buffer[0] = process_buffer[4];
				process_buffer[1] = process_buffer[5];
				process_buffer[2] = process_buffer[6];
				process_buffer[3] = process_buffer[7];
				process_buffer[4] = 0;
				process_buffer[5] = 0;
				process_buffer[6] = 0;
				process_buffer[7] = 0;
				*process_p -= 4;
			}
		}

		/* step (g) Base 256 encodation */
		if(current_mode == DM_BASE256) {
			next_mode = look_ahead_test(source, inputlen, sp, current_mode, gs1);
			if(next_mode == DM_BASE256) {
				target[tp] = source[sp];
				if(debug) printf("B%02X ", target[tp]);
				tp++;
				sp++;
				strcat(binary, "b");
			}
			else {
				next_mode = DM_ASCII;
				if(debug) printf("ASC ");
			}
		}
		if(tp > 1558) {
			return 0;
		}
	} /* while */

	/* Add length and randomising algorithm to b256 */
	i = 0;
	while(i < tp) {
		if(binary[i] == 'b') {
			if((i == 0) || ((i != 0) && (binary[i - 1] != 'b'))) {
				/* start of binary data */
				int binary_count; /* length of b256 data */
				for(binary_count = 0; binary_count + i < tp && binary[binary_count + i] == 'b'; binary_count++) 
					;
				if(binary_count <= 249) {
					dminsert(binary, i, 'b');
					insert_value(target, i, tp, binary_count);
					tp++;
				}
				else {
					dminsert(binary, i, 'b');
					dminsert(binary, i + 1, 'b');
					insert_value(target, i, tp, (binary_count / 250) + 249);
					tp++;
					insert_value(target, i + 1, tp, binary_count % 250);
					tp++;
				}
			}
		}
		i++;
	}
	for(i = 0; i < tp; i++) {
		if(binary[i] == 'b') {
			int prn = ((149 * (i + 1)) % 255) + 1;
			int temp = target[i] + prn;
			target[i] = (temp <= 255) ? (uchar)(temp) : (uchar)(temp - 256);
		}
	}
	*(last_mode) = current_mode;
	return tp;
}

static int dm200encode_remainder(uchar target[],
    int target_length,
    const uchar source[],
    const int inputlen,
    const int last_mode,
    const int process_buffer[],
    const int process_p,
    const int symbols_left)
{
	int debug = 0;
	switch(last_mode) {
		case DM_C40:
		case DM_TEXT:
		    if(process_p == 1) { // 1 data character left to encode.
			    if(symbols_left > 1) {
				    target[target_length] = 254;
				    target_length++; // Unlatch and encode remaining data in ascii.
			    }
			    target[target_length] = source[inputlen - 1] + 1;
			    target_length++;
		    }
		    else if(process_p == 2) { // 2 data characters left to encode.
			    // Pad with shift 1 value (0) and encode as double.
			    int intValue = (1600 * process_buffer[0]) + (40 * process_buffer[1]) + 1; // ie (0 + 1).
			    target[target_length] = (uchar)(intValue / 256);
			    target_length++;
			    target[target_length] = (uchar)(intValue % 256);
			    target_length++;
			    if(symbols_left > 2) {
				    target[target_length] = 254; // Unlatch
				    target_length++;
			    }
		    }
		    else {
			    if(symbols_left > 0) {
				    target[target_length] = 254; // Unlatch
				    target_length++;
			    }
		    }
		    break;

		case DM_X12:
		    if((symbols_left == process_p) && (process_p == 1)) {
			    // Unlatch not required!
			    target[target_length] = source[inputlen - 1] + 1;
			    target_length++;
		    }
		    else {
			    target[target_length] = (254);
			    target_length++; // Unlatch.

			    if(process_p == 1) {
				    target[target_length++] = source[inputlen - 1] + 1;
			    }
			    if(process_p == 2) {
				    target[target_length++] = source[inputlen - 2] + 1;
				    target[target_length++] = source[inputlen - 1] + 1;
			    }
		    }
		    break;

		case DM_EDIFACT:
		    if(symbols_left <= 2) { // Unlatch not required!
			    if(process_p == 1) {
				    target[target_length++] = source[inputlen - 1] + 1;
			    }
			    if(process_p == 2) {
				    target[target_length++] = source[inputlen - 2] + 1;
				    target[target_length++] = source[inputlen - 1] + 1;
			    }
		    }
		    else {
			    // Append edifact unlatch value (31) and encode as triple
			    if(process_p == 0) {
				    target[target_length++] = (uchar)(31 << 2);
				    target[target_length++] = 0;
				    target[target_length++] = 0;
			    }
			    if(process_p == 1) {
				    target[target_length++] = (uchar)((process_buffer[0] << 2) + ((31 & 0x30) >> 4));
				    target[target_length++] = (uchar)((31 & 0x0f) << 4);
				    target[target_length++] = (uchar)0;
			    }
			    if(process_p == 2) {
				    target[target_length++] = (uchar)((process_buffer[0] << 2) + ((process_buffer[1] & 0x30) >> 4));
				    target[target_length++] = (uchar)(((process_buffer[1] & 0x0f) << 4) + ((31 & 0x3c) >> 2));
				    target[target_length++] = (uchar)(((31 & 0x03) << 6));
			    }
			    if(process_p == 3) {
				    target[target_length++] = (uchar)((process_buffer[0] << 2) + ((process_buffer[1] & 0x30) >> 4));
				    target[target_length++] = (uchar)(((process_buffer[1] & 0x0f) << 4) + ((process_buffer[2] & 0x3c) >> 2));
				    target[target_length++] = (uchar)(((process_buffer[2] & 0x03) << 6) + 31);
			    }
		    }
		    break;
	}
	if(debug) {
		int i;
		printf("\n\n");
		for(i = 0; i < target_length; i++)
			printf("%03d ", target[i]);
		printf("\n");
	}
	return target_length;
}
//
// add pad bits 
//
static void add_tail(uchar target[], int tp, const int tail_length)
{
	for(int i = tail_length; i > 0; i--) {
		if(i == tail_length) {
			target[tp++] = 129; // Pad 
		}
		else {
			int prn = ((149 * (tp + 1)) % 253) + 1;
			int temp = 129 + prn;
			if(temp <= 254)
				target[tp++] = (uchar)(temp);
			else
				target[tp++] = (uchar)(temp - 254);
		}
	}
}

int data_matrix_200(struct ZintSymbol * symbol, const uchar source[], const int in_length)
{
	int i, inputlen = in_length, skew = 0;
	uchar binary[2200];
	int binlen;
	int process_buffer[8]; /* holds remaining data to finalised */
	int process_p; /* number of characters left to finalise */
	int symbolsize, optionsize, calcsize;
	int taillength, error_number = 0;
	int H, W, FH, FW, datablock, bytes, rsblock;
	int last_mode = DM_ASCII;
	uchar * grid = 0;
	int symbols_left;
	/* inputlen may be decremented by 2 if macro character is used */
	binlen = dm200encode(symbol, source, binary, &last_mode, &inputlen, process_buffer, &process_p);
	if(binlen == 0) {
		sstrcpy(symbol->errtxt, "Data too long to fit in symbol (E11)");
		return ZINT_ERROR_TOO_LONG;
	}
	if((symbol->option_2 >= 1) && (symbol->option_2 <= DMSIZESCOUNT)) {
		optionsize = intsymbol[symbol->option_2 - 1];
	}
	else {
		optionsize = -1;
	}
	calcsize = DMSIZESCOUNT - 1;
	for(i = DMSIZESCOUNT - 1; i > -1; i--) {
		if(matrixbytes[i] >= (binlen + process_p)) {
			// Allow for the remaining data characters
			calcsize = i;
		}
	}

	if(symbol->option_3 == DM_SQUARE) {
		/* Skip rectangular symbols in square only mode */
		while(matrixH[calcsize] != matrixW[calcsize]) {
			calcsize++;
		}
	}
	else if(symbol->option_3 != DM_DMRE) {
		/* Skip DMRE symbols */
		while(isDMRE[calcsize]) {
			calcsize++;
		}
	}
	symbolsize = optionsize;
	if(calcsize > optionsize) {
		symbolsize = calcsize;
		if(optionsize != -1) {
			/* flag an error */
			error_number = ZINT_WARN_INVALID_OPTION;
			sstrcpy(symbol->errtxt, "Data does not fit in selected symbol size (E12)");
		}
	}
	// Now we know the symbol size we can handle the remaining data in the process buffer.
	symbols_left = matrixbytes[symbolsize] - binlen;
	binlen = dm200encode_remainder(binary, binlen, source, inputlen, last_mode, process_buffer, process_p, symbols_left);
	if(binlen > matrixbytes[symbolsize]) {
		sstrcpy(symbol->errtxt, "Data too long to fit in symbol (E12A)");
		return ZINT_ERROR_TOO_LONG;
	}
	H = matrixH[symbolsize];
	W = matrixW[symbolsize];
	FH = matrixFH[symbolsize];
	FW = matrixFW[symbolsize];
	bytes = matrixbytes[symbolsize];
	datablock = matrixdatablock[symbolsize];
	rsblock = matrixrsblock[symbolsize];
	taillength = bytes - binlen;
	if(taillength != 0) {
		add_tail(binary, binlen, taillength);
	}
	// ecc code
	if(symbolsize == INTSYMBOL144) {
		skew = 1;
	}
	ecc200(binary, bytes, datablock, rsblock, skew);
	// Print Codewords
#ifdef DEBUG
	{
		int CWCount;
		if(skew)
			CWCount = 1558 + 620;
		else
			CWCount = bytes + rsblock * (bytes / datablock);
		printf("Codewords (%i):", CWCount);
		for(int posCur = 0; posCur < CWCount; posCur++)
			printf(" %3i", binary[posCur]);
		puts("\n");
	}
#endif
	{ // placement
		int y;
		int NC = W - 2 * (W / FW);
		int NR = H - 2 * (H / FH);
		int * places = (int *)SAlloc::M(NC * NR * sizeof(int));
		ecc200placement(places, NR, NC);
		grid = (uchar *)SAlloc::M(W * H);
		memzero(grid, W * H);
		for(y = 0; y < H; y += FH) {
			int x;
			for(x = 0; x < W; x++)
				grid[y * W + x] = 1;
			for(x = 0; x < W; x += 2)
				grid[(y + FH - 1) * W + x] = 1;
		}
		{
			for(int x = 0; x < W; x += FW) {
				for(y = 0; y < H; y++)
					grid[y * W + x] = 1;
				for(y = 0; y < H; y += 2)
					grid[y * W + x + FW - 1] = 1;
			}
		}
#ifdef DEBUG
		// Print position matrix as in standard
		for(y = NR - 1; y >= 0; y--) {
			for(int x = 0; x < NC; x++) {
				if(x != 0)
					slfprintf_stderr("|");
				int v = places[(NR - y - 1) * NC + x];
				slfprintf_stderr("%3d.%2d", (v >> 3), 8 - (v & 7));
			}
			slfprintf_stderr("\n");
		}
#endif
		for(y = 0; y < NR; y++) {
			for(int x = 0; x < NC; x++) {
				int v = places[(NR - y - 1) * NC + x];
				//fprintf (stderr, "%4d", v);
				if(v == 1 || (v > 7 && (binary[(v >> 3) - 1] & (1 << (v & 7)))))
					grid[(1 + y + 2 * (y / (FH - 2))) * W + 1 + x + 2 * (x / (FW - 2))] = 1;
			}
			//fprintf (stderr, "\n");
		}
		for(y = H - 1; y >= 0; y--) {
			for(int x = 0; x < W; x++) {
				if(grid[W * y + x]) {
					set_module(symbol, (H - y) - 1, x);
				}
			}
			symbol->row_height[(H - y) - 1] = 1;
		}
		SAlloc::F(grid);
		SAlloc::F(places);
	}
	symbol->rows = H;
	symbol->width = W;
	return error_number;
}

int dmatrix(struct ZintSymbol * symbol, const uchar source[], const int in_length)
{
	int error_number;
	if(symbol->option_1 <= 1) { /* ECC 200 */
		error_number = data_matrix_200(symbol, source, in_length);
	}
	else { /* ECC 000 - 140 */
		sstrcpy(symbol->errtxt, "Older Data Matrix standards are no longer supported (E13)");
		error_number = ZINT_ERROR_INVALID_OPTION;
	}
	return error_number;
}

