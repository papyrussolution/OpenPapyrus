/* raster.c - Handles output to raster files */

/*
    libzint - the open source barcode library
    Copyright (C) 2009-2016 Robin Stuart <rstuart114@gmail.com>

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

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

#define SSET    "0123456789ABCDEF"
//
// Font for human readable text 
//
static const int8 ascii_font[] = { // @sobolev int-->int8
    // Each character is 7 x 14 pixels 
    0, 0, 8, 8, 8, 8, 8, 8, 8, 0, 8, 8, 0, 0, /* ! */
    0, 20, 20, 20, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* " */
    0, 0, 20, 20, 20, 62, 20, 20, 62, 20, 20, 20, 0, 0, /* # */
    0, 0, 8, 60, 74, 74, 40, 28, 10, 74, 74, 60, 8, 0, /* $ */
    0, 0, 50, 74, 76, 56, 8, 16, 28, 50, 82, 76, 0, 0, /* % */
    0, 0, 24, 36, 36, 36, 24, 50, 74, 68, 76, 50, 0, 0, /* & */
    0, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* ' */
    0, 2, 4, 8, 8, 16, 16, 16, 16, 16, 8, 8, 4, 2, /* ( */
    0, 32, 16, 8, 8, 4, 4, 4, 4, 4, 8, 8, 16, 32, /* ) */
    0, 0, 0, 0, 8, 42, 28, 8, 28, 42, 8, 0, 0, 0, /* * */
    0, 0, 0, 0, 8, 8, 8, 62, 8, 8, 8, 0, 0, 0, /* + */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 8, 8, 16, /* , */
    0, 0, 0, 0, 0, 0, 0, 62, 0, 0, 0, 0, 0, 0, /* - */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 28, 8, 0, /* . */
    0, 2, 2, 4, 4, 8, 8, 8, 16, 16, 32, 32, 64, 64, /* / */
    0, 0, 24, 36, 66, 66, 66, 66, 66, 66, 36, 24, 0, 0, /* 0 */
    0, 0, 8, 24, 40, 8, 8, 8, 8, 8, 8, 62, 0, 0, /* 1 */
    0, 0, 60, 66, 66, 2, 4, 4, 8, 16, 32, 126, 0, 0, /* 2 */
    0, 0, 126, 2, 4, 8, 28, 2, 2, 66, 66, 60, 0, 0, /* 3 */
    0, 0, 4, 12, 20, 20, 36, 36, 68, 126, 4, 4, 0, 0, /* 4 */
    0, 0, 126, 64, 64, 124, 66, 2, 2, 66, 66, 60, 0, 0, /* 5 */
    0, 0, 28, 32, 64, 64, 92, 98, 66, 66, 66, 60, 0, 0, /* 6 */
    0, 0, 126, 2, 4, 4, 8, 8, 16, 16, 32, 32, 0, 0, /* 7 */
    0, 0, 60, 66, 66, 36, 24, 36, 66, 66, 66, 60, 0, 0, /* 8 */
    0, 0, 60, 66, 66, 66, 70, 58, 2, 66, 68, 56, 0, 0, /* 9 */
    0, 0, 0, 0, 8, 28, 8, 0, 0, 8, 28, 8, 0, 0, /* : */
    0, 0, 0, 0, 0, 24, 24, 0, 0, 24, 8, 8, 16, 0, /* ; */
    0, 0, 0, 2, 4, 8, 16, 32, 16, 8, 4, 2, 0, 0, /* < */
    0, 0, 0, 0, 0, 126, 0, 0, 126, 0, 0, 0, 0, 0, /* = */
    0, 0, 0, 32, 16, 8, 4, 2, 4, 8, 16, 32, 0, 0, /* > */
    0, 0, 60, 66, 66, 4, 8, 8, 8, 0, 8, 8, 0, 0, /* ? */
    0, 0, 28, 34, 78, 82, 82, 82, 82, 78, 32, 30, 0, 0, /* @ */
    0, 0, 24, 36, 66, 66, 66, 126, 66, 66, 66, 66, 0, 0, /* A */
    0, 0, 120, 68, 66, 68, 120, 68, 66, 66, 68, 120, 0, 0, /* B */
    0, 0, 60, 66, 66, 64, 64, 64, 64, 66, 66, 60, 0, 0, /* C */
    0, 0, 120, 68, 66, 66, 66, 66, 66, 66, 68, 120, 0, 0, /* D */
    0, 0, 126, 64, 64, 64, 120, 64, 64, 64, 64, 126, 0, 0, /* E */
    0, 0, 126, 64, 64, 64, 120, 64, 64, 64, 64, 64, 0, 0, /* F */
    0, 0, 60, 66, 66, 64, 64, 78, 66, 66, 70, 58, 0, 0, /* G */
    0, 0, 66, 66, 66, 66, 126, 66, 66, 66, 66, 66, 0, 0, /* H */
    0, 0, 62, 8, 8, 8, 8, 8, 8, 8, 8, 62, 0, 0, /* I */
    0, 0, 14, 4, 4, 4, 4, 4, 4, 68, 68, 56, 0, 0, /* J */
    0, 0, 66, 68, 72, 80, 96, 80, 72, 68, 66, 66, 0, 0, /* K */
    0, 0, 64, 64, 64, 64, 64, 64, 64, 64, 64, 126, 0, 0, /* L */
    0, 0, 66, 102, 102, 90, 90, 66, 66, 66, 66, 66, 0, 0, /* M */
    0, 0, 66, 66, 98, 98, 82, 74, 70, 70, 66, 66, 0, 0, /* N */
    0, 0, 60, 66, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* O */
    0, 0, 124, 66, 66, 66, 66, 124, 64, 64, 64, 64, 0, 0, /* P */
    0, 0, 60, 66, 66, 66, 66, 66, 114, 74, 70, 60, 4, 2, /* Q */
    0, 0, 124, 66, 66, 66, 66, 124, 72, 68, 66, 66, 0, 0, /* R */
    0, 0, 60, 66, 66, 64, 48, 12, 2, 66, 66, 60, 0, 0, /* S */
    0, 0, 127, 8, 8, 8, 8, 8, 8, 8, 8, 8, 0, 0, /* T */
    0, 0, 66, 66, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* U */
    0, 0, 66, 66, 66, 66, 36, 36, 36, 24, 24, 24, 0, 0, /* V */
    0, 0, 34, 34, 34, 34, 34, 34, 42, 42, 42, 20, 0, 0, /* W */
    0, 0, 66, 66, 36, 36, 24, 24, 36, 36, 66, 66, 0, 0, /* X */
    0, 0, 34, 34, 34, 20, 20, 8, 8, 8, 8, 8, 0, 0, /* Y */
    0, 0, 126, 2, 4, 8, 8, 16, 32, 32, 64, 126, 0, 0, /* Z */
    0, 30, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 30, /* [ */
    0, 64, 64, 32, 32, 16, 16, 16, 8, 8, 4, 4, 2, 2, /* \ */
    0, 60, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 60, /* ] */
    0, 24, 36, 66, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* ^ */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 126, /* _ */
    0, 16, 8, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* ` */
    0, 0, 0, 0, 0, 60, 66, 2, 62, 66, 66, 62, 0, 0, /* a */
    0, 0, 64, 64, 64, 92, 98, 66, 66, 66, 98, 92, 0, 0, /* b */
    0, 0, 0, 0, 0, 60, 66, 64, 64, 64, 66, 60, 0, 0, /* c */
    0, 0, 2, 2, 2, 58, 70, 66, 66, 66, 70, 58, 0, 0, /* d */
    0, 0, 0, 0, 0, 60, 66, 66, 126, 64, 66, 60, 0, 0, /* e */
    0, 0, 12, 18, 16, 16, 124, 16, 16, 16, 16, 16, 0, 0, /* f */
    0, 0, 0, 0, 0, 58, 68, 68, 68, 56, 32, 92, 66, 60, /* g */
    0, 0, 64, 64, 64, 92, 98, 66, 66, 66, 66, 66, 0, 0, /* h */
    0, 0, 8, 8, 0, 24, 8, 8, 8, 8, 8, 62, 0, 0, /* i */
    0, 0, 2, 2, 0, 6, 2, 2, 2, 2, 2, 34, 34, 28, /* j */
    0, 0, 64, 64, 64, 68, 72, 80, 112, 72, 68, 66, 0, 0, /* k */
    0, 0, 24, 8, 8, 8, 8, 8, 8, 8, 8, 62, 0, 0, /* l */
    0, 0, 0, 0, 0, 52, 42, 42, 42, 42, 42, 34, 0, 0, /* m */
    0, 0, 0, 0, 0, 92, 98, 66, 66, 66, 66, 66, 0, 0, /* n */
    0, 0, 0, 0, 0, 60, 66, 66, 66, 66, 66, 60, 0, 0, /* o */
    0, 0, 0, 0, 0, 92, 98, 66, 66, 66, 98, 92, 64, 64, /* p */
    0, 0, 0, 0, 0, 58, 70, 66, 66, 66, 70, 58, 2, 2, /* q */
    0, 0, 0, 0, 0, 92, 98, 66, 64, 64, 64, 64, 0, 0, /* r */
    0, 0, 0, 0, 0, 60, 66, 32, 24, 4, 66, 60, 0, 0, /* s */
    0, 0, 16, 16, 16, 124, 16, 16, 16, 16, 18, 12, 0, 0, /* t */
    0, 0, 0, 0, 0, 66, 66, 66, 66, 66, 70, 58, 0, 0, /* u */
    0, 0, 0, 0, 0, 34, 34, 34, 20, 20, 8, 8, 0, 0, /* v */
    0, 0, 0, 0, 0, 34, 34, 42, 42, 42, 42, 20, 0, 0, /* w */
    0, 0, 0, 0, 0, 66, 66, 36, 24, 36, 66, 66, 0, 0, /* x */
    0, 0, 0, 0, 0, 66, 66, 66, 66, 70, 58, 2, 66, 60, /* y */
    0, 0, 0, 0, 0, 126, 4, 8, 16, 16, 32, 126, 0, 0, /* z */
    0, 6, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 6, /* { */
    0, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, /* | */
    0, 48, 8, 8, 8, 8, 8, 4, 8, 8, 8, 8, 8, 48, /* } */
    0, 32, 82, 74, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* ~ */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*  */
    0, 0, 8, 8, 0, 8, 8, 8, 8, 8, 8, 8, 0, 0, /* ¡ */
    0, 0, 0, 0, 16, 60, 82, 80, 80, 80, 82, 60, 16, 0, /* ¢ */
    0, 0, 0, 12, 18, 16, 16, 60, 16, 16, 60, 18, 0, 0, /* £ */
    0, 0, 0, 0, 66, 60, 36, 36, 60, 66, 0, 0, 0, 0, /* ¤ */
    0, 0, 34, 20, 20, 8, 62, 8, 62, 8, 8, 8, 0, 0, /* ¥ */
    0, 0, 8, 8, 8, 8, 0, 0, 8, 8, 8, 8, 0, 0, /* ¦ */
    0, 60, 66, 32, 24, 36, 66, 36, 24, 4, 66, 60, 0, 0, /* § */
    0, 36, 36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* ¨ */
    0, 60, 66, 90, 102, 98, 98, 98, 102, 90, 66, 60, 0, 0, /* © */
    0, 28, 34, 30, 34, 38, 26, 0, 62, 0, 0, 0, 0, 0, /* ª */
    0, 0, 0, 0, 0, 10, 20, 40, 80, 40, 20, 10, 0, 0, /* « */
    0, 0, 0, 0, 0, 0, 0, 0, 62, 2, 2, 2, 0, 0, /* ¬ */
    0, 0, 0, 0, 0, 0, 0, 60, 0, 0, 0, 0, 0, 0, /* ­ */
    0, 60, 66, 122, 102, 102, 122, 102, 102, 102, 66, 60, 0, 0, /* ® */
    0, 0, 62, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* ¯ */
    0, 24, 36, 36, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* ° */
    0, 0, 0, 0, 0, 0, 8, 8, 62, 8, 8, 62, 0, 0, /* ± */
    0, 24, 36, 4, 8, 16, 32, 60, 0, 0, 0, 0, 0, 0, /* ² */
    0, 24, 36, 4, 24, 4, 36, 24, 0, 0, 0, 0, 0, 0, /* ³ */
    0, 4, 8, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* ´ */
    0, 0, 0, 0, 0, 0, 34, 34, 34, 34, 54, 42, 32, 32, /* µ */
    0, 0, 30, 42, 42, 42, 42, 26, 10, 10, 10, 10, 10, 14, /* ¶ */
    0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, /* · */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 16, /* ¸ */
    0, 8, 24, 8, 8, 8, 8, 28, 0, 0, 0, 0, 0, 0, /* ¹ */
    0, 0, 24, 36, 36, 24, 0, 60, 0, 0, 0, 0, 0, 0, /* º */
    0, 0, 0, 0, 0, 80, 40, 20, 10, 20, 40, 80, 0, 0, /* » */
    0, 0, 32, 98, 36, 36, 40, 18, 22, 42, 78, 66, 0, 0, /* ¼ */
    0, 0, 32, 98, 36, 36, 40, 20, 26, 34, 68, 78, 0, 0, /* ½ */
    0, 0, 98, 18, 36, 24, 104, 18, 38, 42, 78, 2, 0, 0, /* ¾ */
    0, 0, 0, 16, 16, 0, 16, 16, 16, 16, 32, 66, 66, 60, /* ¿ */
    16, 8, 0, 24, 36, 66, 66, 126, 66, 66, 66, 66, 0, 0, /* À */
    8, 16, 0, 24, 36, 66, 66, 126, 66, 66, 66, 66, 0, 0, /* Á */
    24, 36, 0, 24, 36, 66, 66, 126, 66, 66, 66, 66, 0, 0, /* Â */
    50, 76, 0, 24, 36, 66, 66, 126, 66, 66, 66, 66, 0, 0, /* Ã */
    0, 36, 0, 24, 36, 66, 66, 126, 66, 66, 66, 66, 0, 0, /* Ä */
    0, 24, 36, 24, 36, 66, 66, 126, 66, 66, 66, 66, 0, 0, /* Å */
    0, 0, 30, 40, 72, 72, 126, 72, 72, 72, 72, 78, 0, 0, /* Æ */
    0, 0, 60, 66, 66, 64, 64, 64, 64, 66, 66, 60, 8, 16, /* Ç */
    16, 8, 0, 126, 64, 64, 64, 124, 64, 64, 64, 126, 0, 0, /* È */
    8, 16, 0, 126, 64, 64, 64, 124, 64, 64, 64, 126, 0, 0, /* É */
    24, 36, 0, 126, 64, 64, 64, 124, 64, 64, 64, 126, 0, 0, /* Ê */
    0, 36, 0, 126, 64, 64, 64, 124, 64, 64, 64, 126, 0, 0, /* Ë */
    16, 8, 0, 62, 8, 8, 8, 8, 8, 8, 8, 62, 0, 0, /* Ì */
    4, 8, 0, 62, 8, 8, 8, 8, 8, 8, 8, 62, 0, 0, /* Í */
    8, 20, 0, 62, 8, 8, 8, 8, 8, 8, 8, 62, 0, 0, /* Î */
    0, 20, 0, 62, 8, 8, 8, 8, 8, 8, 8, 62, 0, 0, /* Ï */
    0, 0, 60, 34, 33, 33, 121, 33, 33, 33, 34, 60, 0, 0, /* Ð */
    50, 76, 0, 98, 98, 82, 82, 74, 74, 74, 70, 70, 0, 0, /* Ñ */
    16, 8, 0, 60, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* Ò */
    8, 16, 0, 60, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* Ó */
    24, 36, 0, 60, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* Ô */
    50, 76, 0, 60, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* Õ */
    0, 36, 0, 60, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* Ö */
    0, 0, 0, 0, 0, 65, 34, 20, 8, 20, 34, 65, 0, 0, /* × */
    2, 2, 60, 70, 74, 74, 74, 82, 82, 82, 98, 60, 64, 64, /* Ø */
    16, 8, 0, 66, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* Ù */
    8, 16, 0, 66, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* Ú */
    24, 36, 0, 66, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* Û */
    0, 36, 0, 66, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* Ü */
    4, 8, 0, 34, 34, 20, 20, 8, 8, 8, 8, 8, 0, 0, /* Ý */
    0, 0, 64, 64, 124, 66, 66, 66, 66, 124, 64, 64, 0, 0, /* Þ */
    0, 0, 24, 36, 36, 36, 56, 36, 34, 34, 34, 124, 0, 0, /* ß */
    0, 0, 16, 8, 0, 60, 66, 14, 50, 66, 70, 58, 0, 0, /* à */
    0, 0, 4, 8, 0, 60, 66, 14, 50, 66, 70, 58, 0, 0, /* á */
    0, 0, 24, 36, 0, 60, 66, 14, 50, 66, 70, 58, 0, 0, /* â */
    0, 0, 50, 76, 0, 60, 66, 14, 50, 66, 70, 58, 0, 0, /* ã */
    0, 0, 0, 36, 0, 60, 66, 14, 50, 66, 70, 58, 0, 0, /* ä */
    0, 24, 36, 24, 0, 60, 66, 14, 50, 66, 70, 58, 0, 0, /* å */
    0, 0, 0, 0, 0, 62, 73, 25, 47, 72, 73, 62, 0, 0, /* æ */
    0, 0, 0, 0, 0, 60, 66, 64, 64, 64, 66, 60, 8, 16, /* ç */
    0, 0, 16, 8, 0, 60, 66, 66, 126, 64, 66, 60, 0, 0, /* è */
    0, 0, 8, 16, 0, 60, 66, 66, 126, 64, 66, 60, 0, 0, /* é */
    0, 0, 24, 36, 0, 60, 66, 66, 126, 64, 66, 60, 0, 0, /* ê */
    0, 0, 0, 36, 0, 60, 66, 66, 126, 64, 66, 60, 0, 0, /* ë */
    0, 0, 16, 8, 0, 24, 8, 8, 8, 8, 8, 62, 0, 0, /* ì */
    0, 0, 4, 8, 0, 24, 8, 8, 8, 8, 8, 62, 0, 0, /* í */
    0, 0, 24, 36, 0, 24, 8, 8, 8, 8, 8, 62, 0, 0, /* î */
    0, 0, 0, 20, 0, 24, 8, 8, 8, 8, 8, 62, 0, 0, /* ï */
    0, 20, 8, 20, 2, 30, 34, 34, 34, 34, 34, 28, 0, 0, /* ð */
    0, 0, 50, 76, 0, 92, 98, 66, 66, 66, 66, 66, 0, 0, /* ñ */
    0, 0, 16, 8, 0, 60, 66, 66, 66, 66, 66, 60, 0, 0, /* ò */
    0, 0, 8, 16, 0, 60, 66, 66, 66, 66, 66, 60, 0, 0, /* ó */
    0, 0, 24, 36, 0, 60, 66, 66, 66, 66, 66, 60, 0, 0, /* ô */
    0, 0, 50, 76, 0, 60, 66, 66, 66, 66, 66, 60, 0, 0, /* õ */
    0, 0, 0, 36, 0, 60, 66, 66, 66, 66, 66, 60, 0, 0, /* ö */
    0, 0, 0, 0, 0, 0, 0, 24, 0, 126, 0, 24, 0, 0, /* ÷ */
    0, 0, 0, 2, 4, 60, 74, 74, 82, 82, 98, 60, 64, 64, /* ø */
    0, 0, 16, 8, 0, 66, 66, 66, 66, 66, 70, 58, 0, 0, /* ù */
    0, 0, 8, 16, 0, 66, 66, 66, 66, 66, 70, 58, 0, 0, /* ú */
    0, 0, 24, 36, 0, 66, 66, 66, 66, 66, 70, 58, 0, 0, /* û */
    0, 0, 0, 36, 0, 66, 66, 66, 66, 66, 70, 58, 0, 0, /* ü */
    0, 0, 8, 16, 0, 66, 66, 34, 36, 20, 28, 8, 72, 48, /* ý */
    0, 0, 64, 64, 64, 92, 98, 66, 66, 66, 98, 92, 64, 64, /* þ */
    0, 0, 0, 36, 0, 66, 66, 34, 36, 20, 28, 8, 72, 48, /* ÿ */
};

static const int8 small_font[] = { // @sobolev int-->int8
    // Each character is 5 x 9 pixels 
    0, 2, 2, 2, 2, 0, 2, 0, 0, /* ! */
    0, 5, 5, 5, 0, 0, 0, 0, 0, /* " */
    0, 0, 5, 15, 5, 15, 5, 0, 0, /* # */
    0, 0, 7, 26, 7, 18, 7, 0, 0, /* $ */
    0, 8, 9, 2, 4, 25, 1, 0, 0, /* % */
    0, 0, 4, 10, 4, 10, 5, 0, 0, /* & */
    0, 2, 2, 2, 0, 0, 0, 0, 0, /* ' */
    0, 2, 4, 4, 4, 4, 2, 0, 0, /* ( */
    0, 4, 2, 2, 2, 2, 4, 0, 0, /* ) */
    0, 0, 5, 2, 7, 2, 5, 0, 0, /* * */
    0, 0, 2, 2, 15, 2, 2, 0, 0, /* + */
    0, 0, 0, 0, 16, 3, 2, 4, 0, /* , */
    0, 0, 0, 0, 15, 0, 0, 0, 0, /* - */
    0, 0, 0, 0, 0, 6, 6, 0, 0, /* . */
    0, 0, 1, 2, 4, 8, 0, 0, 0, /* / */
    0, 2, 5, 5, 5, 5, 2, 0, 0, /* 0 */
    0, 2, 6, 2, 2, 2, 7, 0, 0, /* 1 */
    0, 6, 9, 1, 2, 4, 15, 0, 0, /* 2 */
    0, 15, 1, 6, 1, 9, 6, 0, 0, /* 3 */
    0, 2, 6, 10, 15, 2, 2, 0, 0, /* 4 */
    0, 15, 8, 14, 1, 9, 6, 0, 0, /* 5 */
    0, 6, 8, 14, 9, 9, 6, 0, 0, /* 6 */
    0, 15, 1, 2, 2, 4, 4, 0, 0, /* 7 */
    0, 6, 9, 6, 9, 9, 6, 0, 0, /* 8 */
    0, 6, 9, 9, 7, 1, 6, 0, 0, /* 9 */
    0, 0, 6, 6, 0, 6, 6, 0, 0, /* : */
    0, 0, 6, 6, 0, 6, 4, 8, 0, /* ; */
    0, 0, 1, 2, 4, 2, 1, 0, 0, /* < */
    0, 0, 0, 15, 0, 15, 0, 0, 0, /* = */
    0, 0, 4, 2, 1, 2, 4, 0, 0, /* > */
    0, 2, 5, 1, 2, 0, 2, 0, 0, /* ? */
    0, 6, 9, 11, 11, 8, 6, 0, 0, /* @ */
    0, 6, 9, 9, 15, 9, 9, 0, 0, /* A */
    0, 14, 9, 14, 9, 9, 14, 0, 0, /* B */
    0, 6, 9, 8, 8, 9, 6, 0, 0, /* C */
    0, 14, 9, 9, 9, 9, 14, 0, 0, /* D */
    0, 15, 8, 14, 8, 8, 15, 0, 0, /* E */
    0, 15, 8, 14, 8, 8, 8, 0, 0, /* F */
    0, 6, 9, 8, 11, 9, 7, 0, 0, /* G */
    0, 9, 9, 15, 9, 9, 9, 0, 0, /* H */
    0, 7, 2, 2, 2, 2, 7, 0, 0, /* I */
    0, 1, 1, 1, 1, 9, 6, 0, 0, /* J */
    0, 9, 10, 12, 12, 10, 9, 0, 0, /* K */
    0, 8, 8, 8, 8, 8, 15, 0, 0, /* L */
    0, 9, 15, 15, 9, 9, 9, 0, 0, /* M */
    0, 9, 13, 13, 11, 11, 9, 0, 0, /* N */
    0, 6, 9, 9, 9, 9, 6, 0, 0, /* O */
    0, 14, 9, 9, 14, 8, 8, 0, 0, /* P */
    0, 6, 9, 9, 9, 13, 6, 1, 0, /* Q */
    0, 14, 9, 9, 14, 10, 9, 0, 0, /* R */
    0, 6, 9, 4, 2, 9, 6, 0, 0, /* S */
    0, 7, 2, 2, 2, 2, 2, 0, 0, /* T */
    0, 9, 9, 9, 9, 9, 6, 0, 0, /* U */
    0, 9, 9, 9, 9, 6, 6, 0, 0, /* V */
    0, 9, 9, 9, 15, 15, 9, 0, 0, /* W */
    0, 9, 9, 6, 6, 9, 9, 0, 0, /* X */
    0, 5, 5, 5, 2, 2, 2, 0, 0, /* Y */
    0, 15, 1, 2, 4, 8, 15, 0, 0, /* Z */
    0, 7, 4, 4, 4, 4, 7, 0, 0, /* [ */
    0, 0, 8, 4, 2, 1, 0, 0, 0, /* \ */
    0, 7, 1, 1, 1, 1, 7, 0, 0, /* ] */
    0, 2, 5, 0, 0, 0, 0, 0, 0, /* ^ */
    0, 0, 0, 0, 0, 0, 15, 0, 0, /* _ */
    0, 4, 2, 0, 0, 0, 0, 0, 0, /* ` */
    0, 0, 0, 7, 9, 11, 5, 0, 0, /* a */
    0, 8, 8, 14, 9, 9, 14, 0, 0, /* b */
    0, 0, 0, 6, 8, 8, 6, 0, 0, /* c */
    0, 1, 1, 7, 9, 9, 7, 0, 0, /* d */
    0, 0, 0, 6, 11, 12, 6, 0, 0, /* e */
    0, 2, 5, 4, 14, 4, 4, 0, 0, /* f */
    0, 0, 0, 7, 9, 6, 8, 7, 0, /* g */
    0, 8, 8, 14, 9, 9, 9, 0, 0, /* h */
    0, 2, 0, 6, 2, 2, 7, 0, 0, /* i */
    0, 1, 0, 1, 1, 1, 5, 2, 0, /* j */
    0, 8, 8, 10, 12, 10, 9, 0, 0, /* k */
    0, 6, 2, 2, 2, 2, 7, 0, 0, /* l */
    0, 0, 0, 10, 15, 9, 9, 0, 0, /* m */
    0, 0, 0, 14, 9, 9, 9, 0, 0, /* n */
    0, 0, 0, 6, 9, 9, 6, 0, 0, /* o */
    0, 0, 0, 14, 9, 9, 14, 8, 0, /* p */
    0, 0, 0, 7, 9, 9, 7, 1, 0, /* q */
    0, 0, 0, 14, 9, 8, 8, 0, 0, /* r */
    0, 0, 0, 7, 12, 3, 14, 0, 0, /* s */
    0, 4, 4, 14, 4, 4, 3, 0, 0, /* t */
    0, 0, 0, 9, 9, 9, 7, 0, 0, /* u */
    0, 0, 0, 5, 5, 5, 2, 0, 0, /* v */
    0, 0, 0, 9, 9, 15, 15, 0, 0, /* w */
    0, 0, 0, 9, 6, 6, 9, 0, 0, /* x */
    0, 0, 0, 9, 9, 5, 2, 4, 0, /* y */
    0, 0, 0, 15, 2, 4, 15, 0, 0, /* z */
    0, 1, 2, 6, 2, 2, 1, 0, 0, /* { */
    0, 2, 2, 2, 2, 2, 2, 0, 0, /* | */
    0, 4, 2, 3, 2, 2, 4, 0, 0, /* } */
    0, 5, 10, 0, 0, 0, 0, 0, 0, /* ~ */
    0, 0, 0, 0, 0, 0, 0, 0, 0, /*  */
    0, 2, 0, 2, 2, 2, 2, 0, 0, /* ¡ */
    0, 0, 2, 7, 10, 10, 7, 2, 0, /* ¢ */
    0, 0, 3, 4, 14, 4, 11, 0, 0, /* £ */
    0, 0, 8, 7, 5, 7, 8, 0, 0, /* ¤ */
    0, 5, 21, 2, 7, 2, 18, 0, 0, /* ¥ */
    0, 0, 2, 2, 0, 2, 2, 0, 0, /* ¦ */
    0, 3, 4, 6, 5, 3, 1, 6, 0, /* § */
    0, 5, 0, 0, 0, 0, 0, 0, 0, /* ¨ */
    0, 7, 8, 10, 12, 10, 8, 7, 0, /* © */
    0, 6, 26, 22, 16, 16, 16, 0, 0, /* ª */
    0, 0, 0, 4, 9, 4, 0, 0, 0, /* « */
    0, 0, 0, 16, 15, 17, 0, 0, 0, /* ¬ */
    0, 0, 0, 0, 0, 0, 0, 0, 0, /* ­ */
    0, 7, 8, 14, 12, 12, 8, 7, 0, /* ® */
    0, 15, 16, 16, 16, 16, 16, 0, 0, /* ¯ */
    0, 2, 5, 2, 0, 0, 0, 0, 0, /* ° */
    0, 2, 2, 15, 2, 2, 15, 0, 0, /* ± */
    0, 6, 2, 20, 6, 0, 16, 0, 0, /* ² */
    0, 6, 6, 2, 6, 0, 0, 0, 0, /* ³ */
    0, 2, 4, 0, 0, 0, 0, 0, 0, /* ´ */
    0, 0, 0, 9, 9, 9, 14, 8, 0, /* µ */
    0, 7, 13, 13, 5, 5, 5, 0, 0, /* ¶ */
    0, 0, 0, 6, 6, 0, 0, 0, 0, /* · */
    0, 0, 0, 0, 0, 0, 2, 4, 0, /* ¸ */
    0, 2, 6, 2, 7, 0, 0, 0, 0, /* ¹ */
    0, 4, 10, 4, 0, 0, 0, 0, 0, /* º */
    0, 0, 0, 9, 4, 9, 0, 0, 0, /* » */
    0, 8, 8, 8, 25, 3, 7, 1, 0, /* ¼ */
    0, 8, 8, 8, 11, 1, 2, 3, 0, /* ½ */
    0, 12, 12, 4, 13, 3, 7, 1, 0, /* ¾ */
    0, 2, 0, 2, 4, 5, 2, 0, 0, /* ¿ */
    0, 6, 9, 9, 15, 9, 9, 0, 0, /* À */
    0, 6, 9, 9, 15, 9, 9, 0, 0, /* Á */
    0, 6, 9, 9, 15, 9, 9, 0, 0, /* Â */
    0, 6, 9, 9, 15, 9, 9, 0, 0, /* Ã */
    0, 9, 6, 9, 15, 9, 9, 0, 0, /* Ä */
    0, 6, 6, 9, 15, 9, 9, 0, 0, /* Å */
    0, 7, 10, 11, 14, 10, 11, 0, 0, /* Æ */
    0, 6, 9, 8, 8, 9, 6, 4, 0, /* Ç */
    0, 15, 8, 14, 8, 8, 15, 0, 0, /* È */
    0, 15, 8, 14, 8, 8, 15, 0, 0, /* É */
    0, 15, 8, 14, 8, 8, 15, 0, 0, /* Ê */
    0, 15, 8, 14, 8, 8, 15, 0, 0, /* Ë */
    0, 7, 2, 2, 2, 2, 7, 0, 0, /* Ì */
    0, 7, 2, 2, 2, 2, 7, 0, 0, /* Í */
    0, 7, 2, 2, 2, 2, 7, 0, 0, /* Î */
    0, 7, 2, 2, 2, 2, 7, 0, 0, /* Ï */
    0, 14, 5, 13, 5, 5, 14, 0, 0, /* Ð */
    0, 11, 9, 13, 11, 11, 9, 0, 0, /* Ñ */
    0, 6, 9, 9, 9, 9, 6, 0, 0, /* Ò */
    0, 6, 9, 9, 9, 9, 6, 0, 0, /* Ó */
    0, 6, 9, 9, 9, 9, 6, 0, 0, /* Ô */
    0, 6, 9, 9, 9, 9, 6, 0, 0, /* Õ */
    0, 9, 6, 9, 9, 9, 6, 0, 0, /* Ö */
    0, 0, 0, 9, 6, 6, 9, 0, 0, /* × */
    0, 7, 11, 11, 13, 13, 14, 0, 0, /* Ø */
    0, 9, 9, 9, 9, 9, 6, 0, 0, /* Ù */
    0, 9, 9, 9, 9, 9, 6, 0, 0, /* Ú */
    0, 9, 9, 9, 9, 9, 6, 0, 0, /* Û */
    0, 9, 0, 9, 9, 9, 6, 0, 0, /* Ü */
    0, 5, 5, 5, 2, 2, 2, 0, 0, /* Ý */
    0, 8, 14, 9, 14, 8, 8, 0, 0, /* Þ */
    0, 6, 9, 10, 9, 9, 10, 0, 0, /* ß */
    0, 4, 2, 7, 9, 11, 5, 0, 0, /* à */
    0, 2, 4, 7, 9, 11, 5, 0, 0, /* á */
    0, 2, 5, 7, 9, 11, 5, 0, 0, /* â */
    0, 5, 10, 7, 9, 11, 5, 0, 0, /* ã */
    0, 5, 0, 7, 9, 11, 5, 0, 0, /* ä */
    0, 6, 6, 7, 9, 11, 5, 0, 0, /* å */
    0, 0, 0, 7, 11, 10, 7, 0, 0, /* æ */
    0, 0, 0, 3, 4, 4, 3, 2, 0, /* ç */
    0, 4, 2, 6, 11, 12, 6, 0, 0, /* è */
    0, 2, 4, 6, 11, 12, 6, 0, 0, /* é */
    0, 4, 10, 6, 11, 12, 6, 0, 0, /* ê */
    0, 10, 0, 6, 11, 12, 6, 0, 0, /* ë */
    0, 4, 2, 6, 2, 2, 7, 0, 0, /* ì */
    0, 2, 4, 6, 2, 2, 7, 0, 0, /* í */
    0, 2, 5, 6, 2, 2, 7, 0, 0, /* î */
    0, 5, 0, 6, 2, 2, 7, 0, 0, /* ï */
    0, 4, 3, 6, 9, 9, 6, 0, 0, /* ð */
    0, 5, 10, 14, 9, 9, 9, 0, 0, /* ñ */
    0, 4, 2, 6, 9, 9, 6, 0, 0, /* ò */
    0, 2, 4, 6, 9, 9, 6, 0, 0, /* ó */
    0, 6, 0, 6, 9, 9, 6, 0, 0, /* ô */
    0, 5, 10, 6, 9, 9, 6, 0, 0, /* õ */
    0, 5, 0, 6, 9, 9, 6, 0, 0, /* ö */
    0, 0, 6, 0, 15, 0, 6, 0, 0, /* ÷ */
    0, 0, 0, 7, 11, 13, 14, 0, 0, /* ø */
    0, 4, 2, 9, 9, 9, 7, 0, 0, /* ù */
    0, 2, 4, 9, 9, 9, 7, 0, 0, /* ú */
    0, 6, 0, 9, 9, 9, 7, 0, 0, /* û */
    0, 5, 0, 9, 9, 9, 7, 0, 0, /* ü */
    0, 2, 4, 9, 9, 5, 2, 4, 0, /* ý */
    0, 0, 8, 14, 9, 9, 14, 8, 0, /* þ */
    0, 5, 0, 9, 9, 5, 2, 4, 0, /* ÿ */
};

#ifndef NO_PNG
extern int png_pixel_plot(struct ZintSymbol * symbol, char * pixelbuf);
#endif /* NO_PNG */
extern int bmp_pixel_plot(struct ZintSymbol * symbol, char * pixelbuf);
extern int pcx_pixel_plot(struct ZintSymbol * symbol, char * pixelbuf);
extern int gif_pixel_plot(struct ZintSymbol * symbol, char * pixelbuf);

static void buffer_plot(struct ZintSymbol * symbol, const char * pixelbuf)
{
	// Place pixelbuffer into symbol 
	symbol->bitmap = static_cast<char *>(SAlloc::M(symbol->bitmap_width * symbol->bitmap_height * 3));
	/*
	int fgred = (16 * hex(symbol->fgcolour[0])) + hex(symbol->fgcolour[1]);
	int fggrn = (16 * hex(symbol->fgcolour[2])) + hex(symbol->fgcolour[3]);
	int fgblu = (16 * hex(symbol->fgcolour[4])) + hex(symbol->fgcolour[5]);
	int bgred = (16 * hex(symbol->bgcolour[0])) + hex(symbol->bgcolour[1]);
	int bggrn = (16 * hex(symbol->bgcolour[2])) + hex(symbol->bgcolour[3]);
	int bgblu = (16 * hex(symbol->bgcolour[4])) + hex(symbol->bgcolour[5]);
	*/
	int fgred = symbol->ColorFg.R;
	int fggrn = symbol->ColorFg.G;
	int fgblu = symbol->ColorFg.B;
	int bgred = symbol->ColorBg.R;
	int bggrn = symbol->ColorBg.G;
	int bgblu = symbol->ColorBg.B;
	for(int row = 0; row < symbol->bitmap_height; row++) {
		for(int column = 0; column < symbol->bitmap_width; column++) {
			int i = ((row * symbol->bitmap_width) + column) * 3;
			switch(*(pixelbuf + (symbol->bitmap_width * row) + column)) {
				case '1':
				    symbol->bitmap[i] = fgred;
				    symbol->bitmap[i+1] = fggrn;
				    symbol->bitmap[i+2] = fgblu;
				    break;
				default:
				    symbol->bitmap[i] = bgred;
				    symbol->bitmap[i+1] = bggrn;
				    symbol->bitmap[i+2] = bgblu;
				    break;
			}
		}
	}
}

static int save_raster_image_to_file(struct ZintSymbol * symbol, int image_height, int image_width, char * pixelbuf, int rotate_angle, int image_type)
{
	int error_number;
	int row, column;
	char * rotated_pixbuf;
	if(!(rotated_pixbuf = (char *)SAlloc::M(image_width * image_height))) {
		printf("Insufficient memory for pixel buffer (F50)");
		return ZINT_ERROR_ENCODING_PROBLEM;
	}
	switch(rotate_angle) {
		case 0:
		case 180:
		    symbol->bitmap_width = image_width;
		    symbol->bitmap_height = image_height;
		    break;
		case 90:
		case 270:
		    symbol->bitmap_width = image_height;
		    symbol->bitmap_height = image_width;
		    break;
	}
	// sort out colour options 
	/* @sobolev
	to_upper((uchar *)symbol->fgcolour);
	to_upper((uchar *)symbol->bgcolour);
	if(strlen(symbol->fgcolour) != 6) {
		sstrcpy(symbol->errtxt, "Malformed foreground colour target (F51)");
		return ZINT_ERROR_INVALID_OPTION;
	}
	if(strlen(symbol->bgcolour) != 6) {
		sstrcpy(symbol->errtxt, "Malformed background colour target (F52)");
		return ZINT_ERROR_INVALID_OPTION;
	}
	error_number = is_sane(SSET, (uchar *)symbol->fgcolour, strlen(symbol->fgcolour));
	if(error_number == ZINT_ERROR_INVALID_DATA) {
		sstrcpy(symbol->errtxt, "Malformed foreground colour target (F53)");
		return ZINT_ERROR_INVALID_OPTION;
	}
	error_number = is_sane(SSET, (uchar *)symbol->bgcolour, strlen(symbol->fgcolour));
	if(error_number == ZINT_ERROR_INVALID_DATA) {
		sstrcpy(symbol->errtxt, "Malformed background colour target (F54)");
		return ZINT_ERROR_INVALID_OPTION;
	}
	*/
	// Rotate image before plotting 
	switch(rotate_angle) {
		case 0: // Plot the right way up 
		    for(row = 0; row < image_height; row++) {
			    for(column = 0; column < image_width; column++) {
				    rotated_pixbuf[(row * image_width) + column] = pixelbuf[(image_width * row) + column];
			    }
		    }
		    break;
		case 90: // Plot 90 degrees clockwise 
		    for(row = 0; row < image_width; row++) {
			    for(column = 0; column < image_height; column++) {
				    rotated_pixbuf[(row * image_height) + column] = *(pixelbuf + (image_width * (image_height - column - 1)) + row);
			    }
		    }
		    break;
		case 180: // Plot upside down 
		    for(row = 0; row < image_height; row++) {
			    for(column = 0; column < image_width; column++) {
				    rotated_pixbuf[(row * image_width) + column] = *(pixelbuf + (image_width * (image_height - row - 1)) + (image_width - column - 1));
			    }
		    }
		    break;
		case 270: // Plot 90 degrees anti-clockwise 
		    for(row = 0; row < image_width; row++) {
			    for(column = 0; column < image_height; column++) {
				    rotated_pixbuf[(row * image_height) + column] = *(pixelbuf + (image_width * column) + (image_width - row - 1));
			    }
		    }
		    break;
	}
	switch(image_type) {
		case OUT_BUFFER:
		    buffer_plot(symbol, rotated_pixbuf);
		    error_number = 0;
		    break;
		case OUT_PNG_FILE:
#ifndef NO_PNG
		    error_number = png_pixel_plot(symbol, rotated_pixbuf);
#else
		    return ZINT_ERROR_INVALID_OPTION;
#endif
		    break;
		case OUT_PCX_FILE:
		    error_number = pcx_pixel_plot(symbol, rotated_pixbuf);
		    break;
		case OUT_GIF_FILE:
		    error_number = gif_pixel_plot(symbol, rotated_pixbuf);
		    break;
		default:
		    error_number = bmp_pixel_plot(symbol, rotated_pixbuf);
		    break;
	}
	SAlloc::F(rotated_pixbuf);
	return error_number;
}

static void draw_bar(char * pixelbuf, int xpos, int xlen, int ypos, int ylen, int image_width, int image_height)
{
	// Draw a rectangle 
	const int png_ypos = image_height - ypos - ylen;
	// This fudge is needed because EPS measures height from the bottom up but
	// PNG measures y position from the top down 
	for(int i = (xpos); i < (xpos + xlen); i++) {
		for(int j = (png_ypos); j < (png_ypos + ylen); j++) {
			*(pixelbuf + (image_width * j) + i) = '1';
		}
	}
}

static void draw_circle(char * pixelbuf, int image_width, int image_height, int x0, int y0, float radius, char fill)
{
	int radius_i = (int)radius;
	for(int y = -radius_i; y <= radius_i; y++) {
		for(int x = -radius_i; x <= radius_i; x++) {
			if((x * x) + (y * y) <= (radius_i * radius_i)) {
				if((y + y0 >= 0) && (y + y0 < image_height) && (x + x0 >= 0) && (x + x0 < image_width)) {
					*(pixelbuf + ((y + y0) * image_width) + (x + x0)) = fill;
				}
			}
		}
	}
}

static void draw_bullseye(char * pixelbuf, int image_width, int image_height, int xoffset, int yoffset, int scaler)
{
	// Central bullseye in Maxicode symbols 
	draw_circle(pixelbuf, image_width, image_height, (int)(14.5 * scaler) + xoffset, (int)(15 * scaler) + yoffset, (4.571f * scaler) + 1, '1');
	draw_circle(pixelbuf, image_width, image_height, (int)(14.5 * scaler) + xoffset, (int)(15 * scaler) + yoffset, (3.779f * scaler) + 1, '0');
	draw_circle(pixelbuf, image_width, image_height, (int)(14.5 * scaler) + xoffset, (int)(15 * scaler) + yoffset, (2.988f * scaler) + 1, '1');
	draw_circle(pixelbuf, image_width, image_height, (int)(14.5 * scaler) + xoffset, (int)(15 * scaler) + yoffset, (2.196f * scaler) + 1, '0');
	draw_circle(pixelbuf, image_width, image_height, (int)(14.5 * scaler) + xoffset, (int)(15 * scaler) + yoffset, (1.394f * scaler) + 1, '1');
	draw_circle(pixelbuf, image_width, image_height, (int)(14.5 * scaler) + xoffset, (int)(15 * scaler) + yoffset, (0.602f * scaler) + 1, '0');
}

static void draw_hexagon(char * pixelbuf, int image_width, char * scaled_hexagon, int hexagon_size, int xposn, int yposn)
{
	// Put a hexagon into the pixel buffer 
	for(int i = 0; i < hexagon_size; i++) {
		for(int j = 0; j < hexagon_size; j++) {
			if(scaled_hexagon[(i * hexagon_size) + j] == '1') {
				*(pixelbuf + (image_width * i) + (image_width * yposn) + xposn + j) = '1';
			}
		}
	}
}

static void draw_letter(char * pixelbuf, uchar letter, int xposn, int yposn, int textflags, int image_width, int image_height)
{
	// Put a letter into a position 
	int x, y, glyph_no, max_x, max_y;
	int skip = 0;
	if(letter < 33) {
		skip = 1;
	}
	if((letter > 127) && (letter < 161)) {
		skip = 1;
	}
	if(xposn < 0 || yposn < 0) {
		skip = 1;
	}
	if(skip == 0) {
		if(letter > 128) {
			glyph_no = letter - 66;
		}
		else {
			glyph_no = letter - 33;
		}
		switch(textflags) {
			case 1: // small font 5x9
			    max_x = 5;
			    max_y = 9;
			    if(xposn + max_x >= image_width) {
				    max_x = image_width - xposn - 1;
			    }
			    if(yposn + max_y >= image_height) {
				    max_y = image_height - yposn - 1;
			    }
			    for(y = 0; y < max_y; y++) {
				    for(x = 0; x < max_x; x++) {
					    if(small_font[(glyph_no * 9) + y] & (0x10 >> x)) {
						    *(pixelbuf + (y * image_width) + (yposn * image_width) + xposn + x) = '1';
					    }
				    }
			    }
			    break;
			case 2: // bold font -> twice the regular font
		    {
			    char * linePtr;
			    max_x = 7;
			    max_y = 14;
			    if(xposn + max_x + 1 >= image_width) {
				    max_x = image_width - xposn - 2;
			    }
			    if(yposn + max_y >= image_height) {
				    max_y = image_height - yposn - 1;
			    }
			    linePtr = pixelbuf + (yposn * image_width) + xposn + 1;
			    for(y = 0; y < max_y; y++) {
				    char * pixelPtr = linePtr;
				    int extra_dot = 0;
				    for(x = 0; x < 7; x++) {
					    if(ascii_font[(glyph_no * 14) + y] & (0x40 >> x)) {
						    *pixelPtr = '1';
						    extra_dot = 1;
					    }
					    else {
						    if(extra_dot) {
							    *pixelPtr = '1';
						    }
						    extra_dot = 0;
					    }
					    ++pixelPtr;
				    }
				    if(extra_dot) {
					    *pixelPtr = '1';
				    }
				    linePtr += image_width;
			    }
		    }
		    break;

			default: // regular font 7x15
			    max_x = 7;
			    max_y = 14;
			    if(xposn + max_x >= image_width) {
				    max_x = image_width - xposn - 1;
			    }
			    if(yposn + max_y >= image_height) {
				    max_y = image_height - yposn - 1;
			    }
			    for(y = 0; y < max_y; y++) {
				    for(x = 0; x < 7; x++) {
					    if(ascii_font[(glyph_no * 14) + y] & (0x40 >> x)) {
						    *(pixelbuf + (y * image_width) + (yposn * image_width) + xposn + x) = '1';
					    }
				    }
			    }
			    break;
		}
	}
}
//
// Plot a string into the pixel buffer 
//
static void draw_string(char * pixbuf, char input_string[], int xposn, int yposn, int textflags, int image_width, int image_height)
{
	int i, string_length, string_left_hand, letter_width = 7;
	switch(textflags) {
		case 1: // small font 5x9
		    letter_width = 5;
		    break;
		case 2: // bold font -> width of the regular font + 1 extra dot + 1 extra space
		    letter_width = 9;
		    break;
		default: // regular font 7x15
		    letter_width = 7;
		    break;
	}
	string_length = strlen(input_string);
	string_left_hand = xposn - ((letter_width * string_length) / 2);
	for(i = 0; i < string_length; i++) {
		draw_letter(pixbuf, input_string[i], string_left_hand + (i * letter_width), yposn, textflags, image_width, image_height);
	}
}

static void plot_hexline(char * scaled_hexagon, int hexagon_size, float start_x, float start_y, float end_x, float end_y)
{
	// Draw a straight line from start to end 
	const float inc_x = (end_x - start_x) / hexagon_size;
	const float inc_y = (end_y - start_y) / hexagon_size;
	for(int i = 0; i < hexagon_size; i++) {
		const float this_x = start_x + ((float)i * inc_x);
		const float this_y = start_y + ((float)i * inc_y);
		if(((this_x >= 0) && (this_x < hexagon_size)) && ((this_y >= 0) && (this_y < hexagon_size))) {
			scaled_hexagon[(hexagon_size * (int)this_y) + (int)this_x] = '1';
		}
	}
}

static void plot_hexagon(char * scaled_hexagon, int hexagon_size)
{
	// Create a hexagon shape and fill it 
	int line, i;
	char ink;
	float x_offset[6];
	float y_offset[6];
	float start_x, start_y;
	float end_x, end_y;

	x_offset[0] = 0.0f;
	x_offset[1] = 0.86f;
	x_offset[2] = 0.86f;
	x_offset[3] = 0.0f;
	x_offset[4] = -0.86f;
	x_offset[5] = -0.86f;

	y_offset[0] = 1.0f;
	y_offset[1] = 0.5f;
	y_offset[2] = -0.5f;
	y_offset[3] = -1.0f;
	y_offset[4] = -0.5f;
	y_offset[5] = 0.5f;

	/* Plot hexagon outline */
	for(line = 0; line < 5; line++) {
		start_x = ((float)hexagon_size / 2.0f) + (((float)hexagon_size / 2.0f) * x_offset[line]);
		start_y = ((float)hexagon_size / 2.0f) + (((float)hexagon_size / 2.0f) * y_offset[line]);
		end_x = ((float)hexagon_size / 2.0f) + (((float)hexagon_size / 2.0f) * x_offset[line + 1]);
		end_y = ((float)hexagon_size / 2.0f) + (((float)hexagon_size / 2.0f) * y_offset[line + 1]);
		plot_hexline(scaled_hexagon, hexagon_size, start_x, start_y, end_x, end_y);
	}
	start_x = ((float)hexagon_size / 2.0f) + (((float)hexagon_size / 2.0f) * x_offset[line]);
	start_y = ((float)hexagon_size / 2.0f) + (((float)hexagon_size / 2.0f) * y_offset[line]);
	end_x = ((float)hexagon_size / 2.0f) + (((float)hexagon_size / 2.0f) * x_offset[0]);
	end_y = ((float)hexagon_size / 2.0f) + (((float)hexagon_size / 2.0f) * y_offset[0]);
	plot_hexline(scaled_hexagon, hexagon_size, start_x, start_y, end_x, end_y);

	/* Fill hexagon */
	for(line = 0; line < hexagon_size; line++) {
		ink = '0';
		for(i = 0; i < hexagon_size; i++) {
			if(scaled_hexagon[(hexagon_size * line) + i] == '1') {
				if(i < (hexagon_size / 2)) {
					ink = '1';
				}
				else {
					ink = '0';
				}
			}
			if(ink == '1') {
				scaled_hexagon[(hexagon_size * line) + i] = ink;
			}
		}
	}
}

static int plot_raster_maxicode(struct ZintSymbol * symbol, int rotate_angle, int data_type)
{
	// Plot a MaxiCode symbol with hexagons and bullseye 
	int i, row, column, xposn, yposn;
	char * pixelbuf;
	int error_number;
	float scaler = symbol->scale;
	char * scaled_hexagon;
	int    hexagon_size;
	int    xoffset = symbol->border_width + symbol->whitespace_width;
	int    yoffset = symbol->border_width;
	int    image_width  = static_cast<int>((300 + (2 * xoffset * 2)) * scaler);
	int    image_height = static_cast<int>((300 + (2 * yoffset * 2)) * scaler);
	if(!(pixelbuf = (char *)SAlloc::M(image_width * image_height))) {
		printf("Insufficient memory for pixel buffer (F55)");
		return ZINT_ERROR_ENCODING_PROBLEM;
	}
	else {
		for(i = 0; i < (image_width * image_height); i++) {
			*(pixelbuf + i) = '0';
		}
	}
	hexagon_size = (int)scaler * 10;
	if(!(scaled_hexagon = (char *)SAlloc::M(hexagon_size * hexagon_size))) {
		printf("Insufficient memory for pixel buffer (F56)");
		SAlloc::F(scaled_hexagon);
		return ZINT_ERROR_ENCODING_PROBLEM;
	}
	else {
		for(i = 0; i < (hexagon_size * hexagon_size); i++) {
			*(scaled_hexagon + i) = '0';
		}
	}
	plot_hexagon(scaled_hexagon, hexagon_size);
	draw_bullseye(pixelbuf, image_width, image_height, (2 * xoffset), (2 * yoffset), (int)(scaler * 10));
	for(row = 0; row < symbol->rows; row++) {
		yposn = row * 9;
		for(column = 0; column < symbol->width; column++) {
			xposn = column * 10;
			if(module_is_set(symbol, row, column)) {
				if(row & 1) {
					/* Odd (reduced) row */
					xposn += 5;
					draw_hexagon(pixelbuf, image_width, scaled_hexagon, hexagon_size, (int)((xposn + (2 * xoffset)) * scaler),
						(int)((yposn + (2 * yoffset)) * scaler));
				}
				else {
					/* Even (full) row */
					draw_hexagon(pixelbuf, image_width, scaled_hexagon, hexagon_size, (int)((xposn + (2 * xoffset)) * scaler),
					    (int)((yposn + (2 * yoffset)) * scaler));
				}
			}
		}
	}

	if((symbol->output_options & BARCODE_BOX) || (symbol->output_options & BARCODE_BIND)) {
		/* boundary bars */
		draw_bar(pixelbuf, 0, image_width, 0, symbol->border_width * 2, image_width, image_height);
		draw_bar(pixelbuf, 0, image_width, 300 + (symbol->border_width * 2), symbol->border_width * 2, image_width, image_height);
	}

	if(symbol->output_options & BARCODE_BOX) {
		/* side bars */
		draw_bar(pixelbuf, 0, symbol->border_width * 2, 0, image_height, image_width, image_height);
		draw_bar(pixelbuf, 300 + ((symbol->border_width + symbol->whitespace_width + symbol->whitespace_width) * 2),
		    symbol->border_width * 2, 0, image_height, image_width, image_height);
	}
	error_number = save_raster_image_to_file(symbol, image_height, image_width, pixelbuf, rotate_angle, data_type);
	SAlloc::F(scaled_hexagon);
	SAlloc::F(pixelbuf);
	return error_number;
}
//
// Convert UTF-8 to Latin1 Codepage for the interpretation line 
//
static void to_latin1(uchar source[], uchar preprocessed[])
{
	int input_length = sstrlen(source);
	int j = 0;
	int i = 0;
	while(i < input_length) {
		switch(source[i]) {
			case 0xC2:
			    /* UTF-8 C2xxh */
			    /* Character range: C280h (latin: 80h) to C2BFh (latin: BFh) */
			    i++;
			    preprocessed[j] = source[i];
			    j++;
			    break;
			case 0xC3:
			    /* UTF-8 C3xx */
			    /* Character range: C380h (latin: C0h) to C3BFh (latin: FFh) */
			    i++;
			    preprocessed[j] = source[i] + 64;
			    j++;
			    break;
			default:
			    /* Process ASCII (< 80h), all other unicode points are ignored */
			    if(source[i] < 128) {
				    preprocessed[j] = source[i];
				    j++;
			    }
			    break;
		}
		i++;
	}
	preprocessed[j] = '\0';
	return;
}

static int plot_raster_dotty(struct ZintSymbol * symbol, int rotate_angle, int data_type)
{
	float  scaler = 2.0f * symbol->scale;
	char * scaled_pixelbuf;
	int    r, i;
	int    scale_width, scale_height;
	int    error_number = 0;
	symbol->height = symbol->rows; // This is true because only 2d matrix symbols are processed here
	int    xoffset = symbol->border_width + symbol->whitespace_width;
	int    yoffset = symbol->border_width;
	int    image_width = symbol->width + xoffset + xoffset;
	int    image_height = symbol->height + yoffset + yoffset;
	SETMAX(scaler, 2.0f);
	scale_width = (int)((image_width * scaler) + 1);
	scale_height = (int)((image_height * scaler) + 1);
	// Apply scale options by creating another pixel buffer 
	if(!(scaled_pixelbuf = (char *)SAlloc::M(scale_width * scale_height))) {
		printf("Insufficient memory for pixel buffer (F57)");
		return ZINT_ERROR_ENCODING_PROBLEM;
	}
	else {
		for(i = 0; i < (scale_width * scale_height); i++) {
			*(scaled_pixelbuf + i) = '0';
		}
	}
	// Plot the body of the symbol to the pixel buffer 
	for(r = 0; r < symbol->rows; r++) {
		for(i = 0; i < symbol->width; i++) {
			if(module_is_set(symbol, r, i)) {
				draw_circle(scaled_pixelbuf, scale_width, scale_height,
				    (int)((i + xoffset) * scaler) + (int)(scaler / 2.0f),
				    (int)((r + yoffset) * scaler) + (int)(scaler / 2.0f),
				    (symbol->dot_size / 2.0f) * scaler, '1');
			}
		}
	}
	error_number = save_raster_image_to_file(symbol, scale_height, scale_width, scaled_pixelbuf, rotate_angle, data_type);
	SAlloc::F(scaled_pixelbuf);
	return error_number;
}

static int plot_raster_default(struct ZintSymbol * symbol, int rotate_angle, int data_type)
{
	int    textdone, main_width, comp_offset, large_bar_count;
	char   textpart[10], addon[6];
	float  addon_text_posn, preset_height, large_bar_height;
	int    i, r, textoffset, yoffset, xoffset, latch, image_width, image_height;
	char * pixelbuf;
	int    addon_latch = 0, textflags = 0;
	int    this_row, block_width, plot_height, plot_yposn, textpos;
	float  row_height, row_posn;
	int    error_number;
	int    default_text_posn;
	int    next_yposn;
	float  scaler = symbol->scale;
	char * scaled_pixelbuf;
	int    horiz, vert;
	int    scale_width, scale_height;
#ifndef _MSC_VER
	uchar local_text[sstrlen(symbol->text) + 1];
#else
	uchar * local_text = (uchar *)_alloca(sstrlen(symbol->text) + 1);
#endif
	if(symbol->show_hrt) {
		to_latin1(symbol->text, local_text); // Copy text from symbol 
	}
	else {
		// No text needed 
		switch(symbol->Std) {
			case BARCODE_EANX:
			case BARCODE_EANX_CC:
			case BARCODE_ISBNX:
			case BARCODE_UPCA:
			case BARCODE_UPCE:
			case BARCODE_UPCA_CC:
			case BARCODE_UPCE_CC:
				{
					// For these symbols use dummy text to ensure formatting is done
					// properly even if no text is required
					const size_t stl_ = sstrlen(symbol->text);
					for(size_t sti = 0; sti < stl_; sti++) {
						local_text[sti] = (symbol->text[sti] == '+') ? '+' : ' ';
					}
					local_text[stl_] = '\0';
				}
			    break;
			default: // For everything else, just remove the text 
			    local_text[0] = '\0';
			    break;
		}
	}
	textdone = 0;
	main_width = symbol->width;
	sstrcpy(addon, "");
	comp_offset = 0;
	addon_text_posn = 0.0;
	row_height = 0;
	if(symbol->output_options & SMALL_TEXT) {
		textflags = 1;
	}
	else if(symbol->output_options & BOLD_TEXT) {
		textflags = 2;
	}
	SETIFZ(symbol->height, 50);
	large_bar_count = 0;
	preset_height = 0.0;
	for(i = 0; i < symbol->rows; i++) {
		preset_height += symbol->row_height[i];
		if(symbol->row_height[i] == 0) {
			large_bar_count++;
		}
	}
	if(large_bar_count == 0) {
		symbol->height = (int)preset_height;
		large_bar_height = 10;
	}
	else {
		large_bar_height = (symbol->height - preset_height) / large_bar_count;
	}
	while(!(module_is_set(symbol, symbol->rows - 1, comp_offset))) {
		comp_offset++;
	}
	// Certain symbols need whitespace otherwise characters get chopped off the sides 
	if((((symbol->Std == BARCODE_EANX) && (symbol->rows == 1)) || (symbol->Std == BARCODE_EANX_CC)) || (symbol->Std == BARCODE_ISBNX)) {
		switch(sstrlen(local_text)) {
			case 13: /* EAN 13 */
			case 16:
			case 19:
				SETIFZ(symbol->whitespace_width, 10);
			    main_width = 96 + comp_offset;
			    break;
			default:
			    main_width = 68 + comp_offset;
		}
	}
	if(((symbol->Std == BARCODE_UPCA) && (symbol->rows == 1)) || (symbol->Std == BARCODE_UPCA_CC)) {
		if(symbol->whitespace_width == 0) {
			symbol->whitespace_width = 10;
			main_width = 96 + comp_offset;
		}
	}
	if(((symbol->Std == BARCODE_UPCE) && (symbol->rows == 1)) || (symbol->Std == BARCODE_UPCE_CC)) {
		if(symbol->whitespace_width == 0) {
			symbol->whitespace_width = 10;
			main_width = 51 + comp_offset;
		}
	}
	latch = 0;
	r = 0;
	// Isolate add-on text 
	if(is_extendable(symbol->Std)) {
		const size_t ltl_ = sstrlen(local_text);
		for(size_t lti = 0; lti < ltl_; lti++) {
			if(latch == 1) {
				addon[r] = local_text[lti];
				r++;
			}
			if(symbol->text[lti] == '+')
				latch = 1;
		}
	}
	addon[r] = '\0';
	textoffset = (sstrlen(local_text) != 0) ? 9 : 0;
	xoffset = symbol->border_width + symbol->whitespace_width;
	yoffset = symbol->border_width;
	image_width = 2 * (symbol->width + xoffset + xoffset);
	image_height = 2 * (symbol->height + textoffset + yoffset + yoffset);
	if(!(pixelbuf = (char *)SAlloc::M(image_width * image_height))) {
		printf("Insufficient memory for pixel buffer (F58)");
		return ZINT_ERROR_ENCODING_PROBLEM;
	}
	else {
		/* @sobolev for(i = 0; i < (image_width * image_height); i++) {
			*(pixelbuf + i) = '0';
		}*/
		memset(pixelbuf, '0', (image_width * image_height)); // @sobolev
	}
	if((symbol->output_options & BARCODE_BOX) || (symbol->output_options & BARCODE_BIND)) {
		default_text_posn = image_height - 17;
	}
	else {
		default_text_posn = image_height - 17 - symbol->border_width - symbol->border_width;
	}
	row_posn = (float)(textoffset + yoffset);
	next_yposn = textoffset + yoffset;
	row_height = 0;
	// Plot the body of the symbol to the pixel buffer 
	for(r = 0; r < symbol->rows; r++) {
		this_row = symbol->rows - r - 1; /* invert r otherwise plots upside down */
		row_posn += row_height;
		plot_yposn = next_yposn;
		row_height = (symbol->row_height[this_row] == 0) ? large_bar_height : (float)symbol->row_height[this_row];
		next_yposn = (int)(row_posn + row_height);
		plot_height = next_yposn - plot_yposn;
		i = 0;
		latch = (module_is_set(symbol, this_row, 0)) ? 1 : 0;
		do {
			block_width = 0;
			do {
				block_width++;
			} while(module_is_set(symbol, this_row, i + block_width) == module_is_set(symbol, this_row, i));
			if((addon_latch == 0) && (r == 0) && (i > main_width)) {
				plot_height = (int)(row_height - 5.0);
				plot_yposn = (int)(row_posn - 5.0);
				addon_text_posn = row_posn + row_height - 8.0f;
				addon_latch = 1;
			}
			if(latch == 1) {
				// a bar 
				draw_bar(pixelbuf, (i + xoffset) * 2, block_width * 2, plot_yposn * 2, plot_height * 2, image_width, image_height);
				latch = 0;
			}
			else {
				// a space 
				latch = 1;
			}
			i += block_width;
		} while(i < symbol->width);
	}
	xoffset += comp_offset;
	if((((symbol->Std == BARCODE_EANX) && (symbol->rows == 1)) || (symbol->Std == BARCODE_EANX_CC)) || (symbol->Std == BARCODE_ISBNX)) {
		// guard bar extensions and text formatting for EAN8 and EAN13 
		switch(sstrlen(local_text)) {
			case 8: /* EAN-8 */
			case 11:
			case 14:
			    draw_bar(pixelbuf,  (0 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
			    draw_bar(pixelbuf,  (2 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
			    draw_bar(pixelbuf, (32 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
			    draw_bar(pixelbuf, (34 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
			    draw_bar(pixelbuf, (64 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
			    draw_bar(pixelbuf, (66 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
			    /* @sobolev for(i = 0; i < 4; i++) {
				    textpart[i] = local_text[i];
			    }*/
				memcpy(textpart, local_text, 4); // @sobolev
			    textpart[4] = '\0';
			    textpos = 2 * (17 + xoffset);
			    draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
			    /*for(i = 0; i < 4; i++) {
				    textpart[i] = local_text[i + 4];
			    }*/
				memcpy(textpart, local_text+4, 4); // @sobolev
			    textpart[4] = '\0';
			    textpos = 2 * (50 + xoffset);
			    draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
			    textdone = 1;
			    switch(strlen(addon)) {
				    case 2:
						textpos = 2 * (xoffset + 86);
						draw_string(pixelbuf, addon, textpos, (int)(image_height - (addon_text_posn * 2) - 13), textflags, image_width, image_height);
						break;
				    case 5:
						textpos = 2 * (xoffset + 100);
						draw_string(pixelbuf, addon, textpos, (int)(image_height - (addon_text_posn * 2) - 13), textflags, image_width, image_height);
						break;
			    }
			    break;
			case 13: /* EAN 13 */
			case 16:
			case 19:
			    draw_bar(pixelbuf, (0 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
			    draw_bar(pixelbuf, (2 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
			    draw_bar(pixelbuf, (46 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
			    draw_bar(pixelbuf, (48 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
			    draw_bar(pixelbuf, (92 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
			    draw_bar(pixelbuf, (94 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);

			    textpart[0] = local_text[0];
			    textpart[1] = '\0';
			    textpos = 2 * (-7 + xoffset);
			    draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
			    /* @sobolev for(i = 0; i < 6; i++) {
				    textpart[i] = local_text[i+1];
			    }*/
				memcpy(textpart, local_text+1, 6); // @sobolev
			    textpart[6] = '\0';
			    textpos = 2 * (24 + xoffset);
			    draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
			    /* @sobolev for(i = 0; i < 6; i++) {
				    textpart[i] = local_text[i + 7];
			    }*/
				memcpy(textpart, local_text+7, 6); // @sobolev
			    textpart[6] = '\0';
			    textpos = 2 * (71 + xoffset);
			    draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
			    textdone = 1;
			    switch(strlen(addon)) {
				    case 2:
						textpos = 2 * (xoffset + 114);
						draw_string(pixelbuf, addon, textpos, (int)(image_height - (addon_text_posn * 2) - 13), textflags, image_width, image_height);
						break;
				    case 5:
						textpos = 2 * (xoffset + 128);
						draw_string(pixelbuf, addon, textpos, (int)(image_height - (addon_text_posn * 2) - 13), textflags, image_width, image_height);
						break;
			    }
			    break;
		}
	}
	if(((symbol->Std == BARCODE_UPCA) && (symbol->rows == 1)) || (symbol->Std == BARCODE_UPCA_CC)) {
		// guard bar extensions and text formatting for UPCA 
		latch = 1;
		i = 0 + comp_offset;
		do {
			block_width = 0;
			do {
				block_width++;
			} while(module_is_set(symbol, symbol->rows - 1, i + block_width) == module_is_set(symbol, symbol->rows - 1, i));
			if(latch == 1) { // a bar 
				draw_bar(pixelbuf, (i + xoffset - comp_offset) * 2, block_width * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
				latch = 0;
			}
			else { // a space 
				latch = 1;
			}
			i += block_width;
		} while(i < 11 + comp_offset);
		draw_bar(pixelbuf, (46 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
		draw_bar(pixelbuf, (48 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
		latch = 1;
		i = 85 + comp_offset;
		do {
			block_width = 0;
			do {
				block_width++;
			} while(module_is_set(symbol, symbol->rows - 1, i + block_width) == module_is_set(symbol, symbol->rows - 1, i));
			if(latch == 1) { // a bar 
				draw_bar(pixelbuf, (i + xoffset - comp_offset) * 2, block_width * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
				latch = 0;
			}
			else { // a space 
				latch = 1;
			}
			i += block_width;
		} while(i < 96 + comp_offset);
		textpart[0] = local_text[0];
		textpart[1] = '\0';
		textpos = 2 * (-5 + xoffset);
		draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
		for(i = 0; i < 5; i++) {
			textpart[i] = local_text[i+1];
		}
		textpart[5] = '\0';
		textpos = 2 * (27 + xoffset);
		draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
		for(i = 0; i < 5; i++) {
			textpart[i] = local_text[i + 6];
		}
		textpart[6] = '\0';
		textpos = 2 * (68 + xoffset);
		draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
		textpart[0] = local_text[11];
		textpart[1] = '\0';
		textpos = 2 * (100 + xoffset);
		draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
		textdone = 1;
		switch(strlen(addon)) {
			case 2:
			    textpos = 2 * (xoffset + 116);
			    draw_string(pixelbuf, addon, textpos, (int)(image_height - (addon_text_posn * 2) - 13), textflags, image_width, image_height);
			    break;
			case 5:
			    textpos = 2 * (xoffset + 130);
			    draw_string(pixelbuf, addon, textpos, (int)(image_height - (addon_text_posn * 2) - 13), textflags, image_width, image_height);
			    break;
		}
	}
	if(((symbol->Std == BARCODE_UPCE) && (symbol->rows == 1)) || (symbol->Std == BARCODE_UPCE_CC)) {
		// guard bar extensions and text formatting for UPCE 
		draw_bar(pixelbuf, (0 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
		draw_bar(pixelbuf, (2 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
		draw_bar(pixelbuf, (46 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
		draw_bar(pixelbuf, (48 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);
		draw_bar(pixelbuf, (50 + xoffset) * 2, 1 * 2, (4 + (int)yoffset) * 2, 5 * 2, image_width, image_height);

		textpart[0] = local_text[0];
		textpart[1] = '\0';
		textpos = 2 * (-5 + xoffset);
		draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
		for(i = 0; i < 6; i++) {
			textpart[i] = local_text[i+1];
		}
		textpart[6] = '\0';
		textpos = 2 * (24 + xoffset);
		draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
		textpart[0] = local_text[7];
		textpart[1] = '\0';
		textpos = 2 * (55 + xoffset);
		draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
		textdone = 1;
		switch(strlen(addon)) {
			case 2:
			    textpos = 2 * (xoffset + 70);
			    draw_string(pixelbuf, addon, textpos, (int)(image_height - (addon_text_posn * 2) - 13), textflags, image_width, image_height);
			    break;
			case 5:
			    textpos = 2 * (xoffset + 84);
			    draw_string(pixelbuf, addon, textpos, (int)(image_height - (addon_text_posn * 2) - 13), textflags, image_width, image_height);
			    break;
		}
	}
	xoffset -= comp_offset;
	// Put boundary bars or box around symbol 
	if((symbol->output_options & BARCODE_BOX) || (symbol->output_options & BARCODE_BIND)) {
		// boundary bars 
		if(symbol->Std != BARCODE_CODABLOCKF) {
			draw_bar(pixelbuf, 0, (symbol->width + xoffset + xoffset) * 2, textoffset * 2, symbol->border_width * 2, image_width, image_height);
			draw_bar(pixelbuf, 0, (symbol->width + xoffset + xoffset) * 2, (textoffset + symbol->height + symbol->border_width) * 2, symbol->border_width * 2, image_width, image_height);
		}
		else {
			draw_bar(pixelbuf, xoffset * 2, symbol->width * 2, textoffset * 2, symbol->border_width * 2, image_width, image_height);
			draw_bar(pixelbuf, xoffset * 2, symbol->width * 2, (textoffset + symbol->height + symbol->border_width) * 2, symbol->border_width * 2, image_width, image_height);
		}
		if((symbol->output_options & BARCODE_BIND) != 0) {
			if((symbol->rows > 1) && (is_stackable(symbol->Std) == 1)) {
				// row binding 
				if(symbol->Std != BARCODE_CODABLOCKF) {
					for(r = 1; r < symbol->rows; r++) {
						draw_bar(pixelbuf, xoffset * 2, symbol->width * 2, (int)(((r * row_height) + textoffset + yoffset - 1) * 2), 2 * 2, image_width, image_height);
					}
				}
				else {
					for(r = 1; r < symbol->rows; r++) {
						draw_bar(pixelbuf, (xoffset + 11) * 2, (symbol->width - 25) * 2, (int)(((r * row_height) + textoffset + yoffset - 1) * 2), 2 * 2, image_width, image_height);
					}
				}
			}
		}
	}
	if(symbol->output_options & BARCODE_BOX) {
		// side bars 
		draw_bar(pixelbuf, 0, symbol->border_width * 2, textoffset * 2, (symbol->height + (2 * symbol->border_width)) * 2, image_width, image_height);
		draw_bar(pixelbuf, (symbol->width + xoffset + xoffset - symbol->border_width) * 2, symbol->border_width * 2, textoffset * 2, (symbol->height + (2 * symbol->border_width)) * 2, image_width, image_height);
	}
	// Put the human readable text at the bottom 
	if((textdone == 0) && (sstrlen(local_text) != 0)) {
		textpos = (image_width / 2);
		draw_string(pixelbuf, (char *)local_text, textpos, default_text_posn, textflags, image_width, image_height);
	}
	if(scaler == 0) {
		scaler = 0.5;
	}
	scale_width = (int)(image_width * scaler);
	scale_height = (int)(image_height * scaler);
	// Apply scale options by creating another pixel buffer 
	if(!(scaled_pixelbuf = (char *)SAlloc::M(scale_width * scale_height))) {
		SAlloc::F(pixelbuf);
		printf("Insufficient memory for pixel buffer (F59)");
		return ZINT_ERROR_ENCODING_PROBLEM;
	}
	else {
		for(i = 0; i < (scale_width * scale_height); i++) {
			*(scaled_pixelbuf + i) = '0';
		}
	}
	for(vert = 0; vert < scale_height; vert++) {
		for(horiz = 0; horiz < scale_width; horiz++) {
			*(scaled_pixelbuf + (vert * scale_width) + horiz) = *(pixelbuf + ((int)(vert / scaler) * image_width) + (int)(horiz / scaler));
		}
	}
	error_number = save_raster_image_to_file(symbol, scale_height, scale_width, scaled_pixelbuf, rotate_angle, data_type);
	SAlloc::F(scaled_pixelbuf);
	SAlloc::F(pixelbuf);
	return error_number;
}

int plot_raster(struct ZintSymbol * symbol, int rotate_angle, int file_type)
{
	int error;
#ifdef NO_PNG
	if(file_type == OUT_PNG_FILE) {
		return ZINT_ERROR_INVALID_OPTION;
	}
#endif // NO_PNG 
	if(symbol->output_options & BARCODE_DOTTY_MODE) {
		error = plot_raster_dotty(symbol, rotate_angle, file_type);
	}
	else {
		if(symbol->Std == BARCODE_MAXICODE) {
			error = plot_raster_maxicode(symbol, rotate_angle, file_type);
		}
		else {
			error = plot_raster_default(symbol, rotate_angle, file_type);
		}
	}
	return error;
}

