/* aztec.c - Handles Aztec 2D Symbols */

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

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
 */
#include "common.h"
#pragma hdrstop
//#include "aztec.h"

#define UPPER	1
#define LOWER	2
#define MIXED	4
#define PUNC	8
#define DIGIT	16
#define BINARY	32

static const int16 CompactAztecMap[] = { // @sobolev int-->int16
    // 27 x 27 data grid 
    609, 608, 411, 413, 415, 417, 419, 421, 423, 425, 427, 429, 431, 433, 435, 437, 439, 441, 443, 445, 447, 449, 451, 453, 455, 457, 459,
    607, 606, 410, 412, 414, 416, 418, 420, 422, 424, 426, 428, 430, 432, 434, 436, 438, 440, 442, 444, 446, 448, 450, 452, 454, 456, 458,
    605, 604, 409, 408, 243, 245, 247, 249, 251, 253, 255, 257, 259, 261, 263, 265, 267, 269, 271, 273, 275, 277, 279, 281, 283, 460, 461,
    603, 602, 407, 406, 242, 244, 246, 248, 250, 252, 254, 256, 258, 260, 262, 264, 266, 268, 270, 272, 274, 276, 278, 280, 282, 462, 463,
    601, 600, 405, 404, 241, 240, 107, 109, 111, 113, 115, 117, 119, 121, 123, 125, 127, 129, 131, 133, 135, 137, 139, 284, 285, 464, 465,
    599, 598, 403, 402, 239, 238, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 130, 132, 134, 136, 138, 286, 287, 466, 467,
    597, 596, 401, 400, 237, 236, 105, 104, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 140, 141, 288, 289, 468, 469,
    595, 594, 399, 398, 235, 234, 103, 102, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 142, 143, 290, 291, 470, 471,
    593, 592, 397, 396, 233, 232, 101, 100, 1, 1, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 0, 1, 28, 29, 144, 145, 292, 293, 472, 473,
    591, 590, 395, 394, 231, 230, 99, 98, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 30, 31, 146, 147, 294, 295, 474, 475,
    589, 588, 393, 392, 229, 228, 97, 96, 2027, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2007, 32, 33, 148, 149, 296, 297, 476, 477,
    587, 586, 391, 390, 227, 226, 95, 94, 2026, 1, 0, 1, 1, 1, 1, 1, 0, 1, 2008, 34, 35, 150, 151, 298, 299, 478, 479,
    585, 584, 389, 388, 225, 224, 93, 92, 2025, 1, 0, 1, 0, 0, 0, 1, 0, 1, 2009, 36, 37, 152, 153, 300, 301, 480, 481,
    583, 582, 387, 386, 223, 222, 91, 90, 2024, 1, 0, 1, 0, 1, 0, 1, 0, 1, 2010, 38, 39, 154, 155, 302, 303, 482, 483,
    581, 580, 385, 384, 221, 220, 89, 88, 2023, 1, 0, 1, 0, 0, 0, 1, 0, 1, 2011, 40, 41, 156, 157, 304, 305, 484, 485,
    579, 578, 383, 382, 219, 218, 87, 86, 2022, 1, 0, 1, 1, 1, 1, 1, 0, 1, 2012, 42, 43, 158, 159, 306, 307, 486, 487,
    577, 576, 381, 380, 217, 216, 85, 84, 2021, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2013, 44, 45, 160, 161, 308, 309, 488, 489,
    575, 574, 379, 378, 215, 214, 83, 82, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 46, 47, 162, 163, 310, 311, 490, 491,
    573, 572, 377, 376, 213, 212, 81, 80, 0, 0, 2020, 2019, 2018, 2017, 2016, 2015, 2014, 0, 0, 48, 49, 164, 165, 312, 313, 492, 493,
    571, 570, 375, 374, 211, 210, 78, 76, 74, 72, 70, 68, 66, 64, 62, 60, 58, 56, 54, 50, 51, 166, 167, 314, 315, 494, 495,
    569, 568, 373, 372, 209, 208, 79, 77, 75, 73, 71, 69, 67, 65, 63, 61, 59, 57, 55, 52, 53, 168, 169, 316, 317, 496, 497,
    567, 566, 371, 370, 206, 204, 202, 200, 198, 196, 194, 192, 190, 188, 186, 184, 182, 180, 178, 176, 174, 170, 171, 318, 319, 498, 499,
    565, 564, 369, 368, 207, 205, 203, 201, 199, 197, 195, 193, 191, 189, 187, 185, 183, 181, 179, 177, 175, 172, 173, 320, 321, 500, 501,
    563, 562, 366, 364, 362, 360, 358, 356, 354, 352, 350, 348, 346, 344, 342, 340, 338, 336, 334, 332, 330, 328, 326, 322, 323, 502, 503,
    561, 560, 367, 365, 363, 361, 359, 357, 355, 353, 351, 349, 347, 345, 343, 341, 339, 337, 335, 333, 331, 329, 327, 324, 325, 504, 505,
    558, 556, 554, 552, 550, 548, 546, 544, 542, 540, 538, 536, 534, 532, 530, 528, 526, 524, 522, 520, 518, 516, 514, 512, 510, 506, 507,
    559, 557, 555, 553, 551, 549, 547, 545, 543, 541, 539, 537, 535, 533, 531, 529, 527, 525, 523, 521, 519, 517, 515, 513, 511, 508, 509
};

const int8 AztecCodeSet[128] = { // @sobolev int-->int8
    // From Table 2 
    32, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 12, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 4, 4, 4, 4, 4, 23, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 24, 8, 24, 8, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 8, 8,
    8, 8, 8, 8, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 4, 8, 4, 4, 4, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 8, 4, 8, 4, 4
};

const int16 AztecSymbolChar[128] = { // @sobolev int-->int16
    // From Table 2 
    0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 300, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 15, 16, 17, 18, 19, 1, 6, 7, 8, 9, 10, 11, 12,
    13, 14, 15, 16, 301, 18, 302, 20, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 21, 22,
    23, 24, 25, 26, 20, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 27, 21, 28, 22, 23, 24, 2, 3, 4,
    5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 29, 25, 30, 26, 27
};

/* Problem characters are:
        300: Carriage Return (ASCII 13)
        301: Comma (ASCII 44)
        302: Full Stop (ASCII 46)
 */

static const char *pentbit[32] = {
    "00000", "00001", "00010", "00011", "00100", "00101", "00110", "00111", "01000", "01001",
    "01010", "01011", "01100", "01101", "01110", "01111", "10000", "10001", "10010", "10011", "10100", "10101",
    "10110", "10111", "11000", "11001", "11010", "11011", "11100", "11101", "11110", "11111"
};

static const char *quadbit[16] = {
    "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", "1000", "1001",
    "1010", "1011", "1100", "1101", "1110", "1111"
};

static const char *tribit[8] = {
    "000", "001", "010", "011", "100", "101", "110", "111"
};

static const int AztecSizes[32] = {
    // Codewords per symbol 
    21, 48, 60, 88, 120, 156, 196, 240, 230, 272, 316, 364, 416, 470, 528, 588, 652, 720, 790,
    864, 940, 1020, 920, 992, 1066, 1144, 1224, 1306, 1392, 1480, 1570, 1664
};

static const int AztecCompactSizes[4] = {
    17, 40, 51, 76
};

static const int Aztec10DataSizes[32] = {
    // Data bits per symbol maximum with 10% error correction 
    96, 246, 408, 616, 840, 1104, 1392, 1704, 2040, 2420, 2820, 3250, 3720, 4200, 4730,
    5270, 5840, 6450, 7080, 7750, 8430, 9150, 9900, 10680, 11484, 12324, 13188, 14076,
    15000, 15948, 16920, 17940
};

static const int Aztec23DataSizes[32] = {
    // Data bits per symbol maximum with 23% error correction 
    84, 204, 352, 520, 720, 944, 1184, 1456, 1750, 2070, 2410, 2780, 3180, 3590, 4040,
    4500, 5000, 5520, 6060, 6630, 7210, 7830, 8472, 9132, 9816, 10536, 11280, 12036,
    12828, 13644, 14472, 15348
};

static const int Aztec36DataSizes[32] = {
    // Data bits per symbol maximum with 36% error correction 
    66, 168, 288, 432, 592, 776, 984, 1208, 1450, 1720, 2000, 2300, 2640, 2980, 3350,
    3740, 4150, 4580, 5030, 5500, 5990, 6500, 7032, 7584, 8160, 8760, 9372, 9996, 10656,
    11340, 12024, 12744
};

static const int Aztec50DataSizes[32] = {
    // Data bits per symbol maximum with 50% error correction 
    48, 126, 216, 328, 456, 600, 760, 936, 1120, 1330, 1550, 1790, 2050, 2320, 2610,
    2910, 3230, 3570, 3920, 4290, 4670, 5070, 5484, 5916, 6360, 6828, 7308, 7800, 8316,
    8844, 9384, 9948
};

static const int AztecCompact10DataSizes [4] = { 78, 198, 336, 520 };
static const int AztecCompact23DataSizes [4] = { 66, 168, 288, 440 };
static const int AztecCompact36DataSizes [4] = { 48, 138, 232, 360 };
static const int AztecCompact50DataSizes [4] = { 36, 102, 176, 280 };

static const int AztecOffset[32] = {
    66, 64, 62, 60, 57, 55, 53, 51, 49, 47, 45, 42, 40, 38, 36, 34, 32, 30, 28, 25, 23, 21,
    19, 17, 15, 13, 10, 8, 6, 4, 2, 0
};

static const int AztecCompactOffset[4] = { 6, 4, 2, 0 };
static int AztecMap[22801]; // @global
//
// Shorten the string by one character
//
static void mapshorten(int * charmap, int * typemap, const int start, const int length)
{
	memmove(charmap + start + 1, charmap + start + 2, (length - 1) * sizeof(int));
	memmove(typemap + start + 1, typemap + start + 2, (length - 1) * sizeof(int));
}

/**
 * Insert a character into the middle of a string at position posn
 */
/*
   static void insert(char binary_string[], const size_t posn, const char newbit) {
    size_t i, end;

    end = strlen(binary_string);
    for (i = end; i > posn; i--) {
        binary_string[i] = binary_string[i - 1];
    }
    binary_string[posn] = newbit;
   }
 */

/**
 * Encode input data into a binary string
 */
static int aztec_text_process(const uchar source[], const uint src_len, char binary_string[], const int gs1, const int eci)
{
	int i, j, k, p, bytes;
	int curtable, newtable, lasttable, chartype, maplength, blocks, debug;
#ifndef _MSC_VER
	int charmap[src_len * 2], typemap[src_len * 2];
	int blockmap[2][src_len];
#else
	int* charmap = (int *)_alloca(src_len * 2 * sizeof(int));
	int* typemap = (int *)_alloca(src_len * 2 * sizeof(int));
	int* blockmap[2];
	blockmap[0] = (int *)_alloca(src_len * sizeof(int));
	blockmap[1] = (int *)_alloca(src_len * sizeof(int));
#endif
	/* Lookup input string in encoding table */
	maplength = 0;
	debug = 0;

	if(gs1) {
		/* Add FNC1 to beginning of GS1 messages */
		charmap[maplength] = 0;
		typemap[maplength++] = PUNC;
		charmap[maplength] = 400;
		typemap[maplength++] = PUNC;
	}
	else if(eci != 3) {
		/* Set ECI mode */
		charmap[maplength] = 0;
		typemap[maplength++] = PUNC;
		if(eci < 10) {
			charmap[maplength] = 401; // FLG(1)
			typemap[maplength++] = PUNC;
			charmap[maplength] = 502 + eci;
			typemap[maplength++] = PUNC;
		}
		else {
			charmap[maplength] = 402; // FLG(2)
			typemap[maplength++] = PUNC;
			charmap[maplength] = 502 + (eci / 10);
			typemap[maplength++] = PUNC;
			charmap[maplength] = 502 + (eci % 10);
			typemap[maplength++] = PUNC;
		}
	}

	for(i = 0; i < (int)src_len; i++) {
		if((gs1) && (source[i] == '[')) {
			/* FNC1 represented by FLG(0) */
			charmap[maplength] = 0;
			typemap[maplength++] = PUNC;
			charmap[maplength] = 400;
			typemap[maplength++] = PUNC;
		}
		else {
			if((source[i] > 127) || (source[i] == 0)) {
				charmap[maplength] = source[i];
				typemap[maplength++] = BINARY;
			}
			else {
				charmap[maplength] = AztecSymbolChar[source[i]];
				typemap[maplength++] = AztecCodeSet[source[i]];
			}
		}
	}

	/* Look for double character encoding possibilities */
	i = 0;
	do {
		if(((charmap[i] == 300) && (charmap[i+1] == 11))
		    && ((typemap[i] == PUNC) && (typemap[i+1] == PUNC))) {
			/* CR LF combination */
			charmap[i] = 2;
			typemap[i] = PUNC;
			mapshorten(charmap, typemap, i, maplength);
			maplength--;
		}
		if(((charmap[i] == 302) && (charmap[i+1] == 1)) && ((typemap[i] == 24) && (typemap[i+1] == 23))) {
			/* . SP combination */
			charmap[i] = 3;
			typemap[i] = PUNC;
			mapshorten(charmap, typemap, i, maplength);
			maplength--;
		}
		if(((charmap[i] == 301) && (charmap[i+1] == 1)) && ((typemap[i] == 24) && (typemap[i+1] == 23))) {
			/* , SP combination */
			charmap[i] = 4;
			typemap[i] = PUNC;
			mapshorten(charmap, typemap, i, maplength);
			maplength--;
		}
		if(((charmap[i] == 21) && (charmap[i+1] == 1)) && ((typemap[i] == PUNC) && (typemap[i+1] == 23))) {
			/* : SP combination */
			charmap[i] = 5;
			typemap[i] = PUNC;
			mapshorten(charmap, typemap, i, maplength);
			maplength--;
		}

		i++;
	} while(i < (maplength - 1));

	/* look for blocks of characters which use the same table */
	blocks = 1;
	blockmap[0][0] = typemap[0];
	blockmap[1][0] = 1;
	for(i = 1; i < maplength; i++) {
		if(typemap[i] == typemap[i - 1]) {
			blockmap[1][blocks - 1]++;
		}
		else {
			blocks++;
			blockmap[0][blocks - 1] = typemap[i];
			blockmap[1][blocks - 1] = 1;
		}
	}
	if(blockmap[0][0] & 1) {
		blockmap[0][0] = 1;
	}
	if(blockmap[0][0] & 2) {
		blockmap[0][0] = 2;
	}
	if(blockmap[0][0] & 4) {
		blockmap[0][0] = 4;
	}
	if(blockmap[0][0] & 8) {
		blockmap[0][0] = 8;
	}
	if(blocks > 1) {
		// look for adjacent blocks which can use the same table (left to right search) 
		for(i = 1; i < blocks; i++) {
			if(blockmap[0][i] & blockmap[0][i - 1]) {
				blockmap[0][i] = (blockmap[0][i] & blockmap[0][i - 1]);
			}
		}
		if(blockmap[0][blocks - 1] & 1) {
			blockmap[0][blocks - 1] = 1;
		}
		if(blockmap[0][blocks - 1] & 2) {
			blockmap[0][blocks - 1] = 2;
		}
		if(blockmap[0][blocks - 1] & 4) {
			blockmap[0][blocks - 1] = 4;
		}
		if(blockmap[0][blocks - 1] & 8) {
			blockmap[0][blocks - 1] = 8;
		}
		// look for adjacent blocks which can use the same table (right to left search) 
		for(i = blocks - 1 - 1; i >= 0; i--) {
			if(blockmap[0][i] & blockmap[0][i+1]) {
				blockmap[0][i] = (blockmap[0][i] & blockmap[0][i+1]);
			}
		}
		// determine the encoding table for characters which do not fit with adjacent blocks 
		for(i = 1; i < blocks; i++) {
			if(blockmap[0][i] & 8) {
				blockmap[0][i] = 8;
			}
			if(blockmap[0][i] & 4) {
				blockmap[0][i] = 4;
			}
			if(blockmap[0][i] & 2) {
				blockmap[0][i] = 2;
			}
			if(blockmap[0][i] & 1) {
				blockmap[0][i] = 1;
			}
		}
		// Combine blocks of the same type 
		i = 0;
		do {
			if(blockmap[0][i] == blockmap[0][i+1]) {
				blockmap[1][i] += blockmap[1][i+1];
				for(j = i + 1; j < blocks - 1; j++) {
					blockmap[0][j] = blockmap[0][j + 1];
					blockmap[1][j] = blockmap[1][j + 1];
				}
				blocks--;
			}
			else {
				i++;
			}
		} while(i < blocks - 1);
	}

	/* Put the adjusted block data back into typemap */
	j = 0;
	for(i = 0; i < blocks; i++) {
		if((blockmap[1][i] < 3) && (blockmap[0][i] != 32)) {
			/* Shift character(s) needed */
			for(k = 0; k < blockmap[1][i]; k++) {
				typemap[j + k] = blockmap[0][i] + 64;
			}
		}
		else { /* Latch character (or byte mode) needed */
			for(k = 0; k < blockmap[1][i]; k++) {
				typemap[j + k] = blockmap[0][i];
			}
		}
		j += blockmap[1][i];
	}

	/* Don't shift an initial capital letter */
	if(typemap[0] == 65) {
		typemap[0] = 1;
	}
	;

	/* Problem characters (those that appear in different tables with
	 * different values) can now be resolved into their tables */
	for(i = 0; i < maplength; i++) {
		if((charmap[i] >= 300) && (charmap[i] < 400)) {
			curtable = typemap[i];
			if(curtable > 64) {
				curtable -= 64;
			}
			switch(charmap[i]) {
				case 300: /* Carriage Return */
				    switch(curtable) {
					    case PUNC: charmap[i] = 1;
						break;
					    case MIXED: charmap[i] = 14;
						break;
				    }
				    break;
				case 301: /* Comma */
				    switch(curtable) {
					    case PUNC: charmap[i] = 17;
						break;
					    case DIGIT: charmap[i] = 12;
						break;
				    }
				    break;
				case 302: /* Full Stop */
				    switch(curtable) {
					    case PUNC: charmap[i] = 19;
						break;
					    case DIGIT: charmap[i] = 13;
						break;
				    }
				    break;
			}
		}
	}
	*binary_string = '\0';

	curtable = UPPER; /* start with UPPER table */
	lasttable = UPPER;
	for(i = 0; i < maplength; i++) {
		newtable = curtable;
		if((typemap[i] != curtable) && (charmap[i] < 400)) {
			/* Change table */
			if(curtable == BINARY) {
				/* If ending binary mode the current table is the same as when entering binary mode */
				curtable = lasttable;
				newtable = lasttable;
			}
			if(typemap[i] > 64) {
				/* Shift character */
				switch(typemap[i]) {
					case (64 + UPPER): /* To UPPER */
					    switch(curtable) {
						    case LOWER: /* US */
							strcat(binary_string, pentbit[28]);
							if(debug) printf("US ");
							break;
						    case MIXED: /* UL */
							strcat(binary_string, pentbit[29]);
							if(debug) printf("UL ");
							newtable = UPPER;
							break;
						    case PUNC: /* UL */
							strcat(binary_string, pentbit[31]);
							if(debug) printf("UL ");
							newtable = UPPER;
							break;
						    case DIGIT: /* US */
							strcat(binary_string, quadbit[15]);
							if(debug) printf("US ");
							break;
					    }
					    break;
					case (64 + LOWER): /* To LOWER */
					    switch(curtable) {
						    case UPPER: /* LL */
							strcat(binary_string, pentbit[28]);
							if(debug) printf("LL ");
							newtable = LOWER;
							break;
						    case MIXED: /* LL */
							strcat(binary_string, pentbit[28]);
							if(debug) printf("LL ");
							newtable = LOWER;
							break;
						    case PUNC: /* UL LL */
							strcat(binary_string, pentbit[31]);
							if(debug) printf("UL ");
							strcat(binary_string, pentbit[28]);
							if(debug) printf("LL ");
							newtable = LOWER;
							break;
						    case DIGIT: /* UL LL */
							strcat(binary_string, quadbit[14]);
							if(debug) printf("UL ");
							strcat(binary_string, pentbit[28]);
							if(debug) printf("LL ");
							newtable = LOWER;
							break;
					    }
					    break;
					case (64 + MIXED): /* To MIXED */
					    switch(curtable) {
						    case UPPER: /* ML */
							strcat(binary_string, pentbit[29]);
							if(debug) printf("ML ");
							newtable = MIXED;
							break;
						    case LOWER: /* ML */
							strcat(binary_string, pentbit[29]);
							if(debug) printf("ML ");
							newtable = MIXED;
							break;
						    case PUNC: /* UL ML */
							strcat(binary_string, pentbit[31]);
							if(debug) printf("UL ");
							strcat(binary_string, pentbit[29]);
							if(debug) printf("ML ");
							newtable = MIXED;
							break;
						    case DIGIT: /* UL ML */
							strcat(binary_string, quadbit[14]);
							if(debug) printf("UL ");
							strcat(binary_string, pentbit[29]);
							if(debug) printf("ML ");
							newtable = MIXED;
							break;
					    }
					    break;
					case (64 + PUNC): /* To PUNC */
					    switch(curtable) {
						    case UPPER: /* PS */
							strcat(binary_string, pentbit[0]);
							if(debug) printf("PS ");
							break;
						    case LOWER: /* PS */
							strcat(binary_string, pentbit[0]);
							if(debug) printf("PS ");
							break;
						    case MIXED: /* PS */
							strcat(binary_string, pentbit[0]);
							if(debug) printf("PS ");
							break;
						    case DIGIT: /* PS */
							strcat(binary_string, quadbit[0]);
							if(debug) printf("PS ");
							break;
					    }
					    break;
					case (64 + DIGIT): /* To DIGIT */
					    switch(curtable) {
						    case UPPER: /* DL */
							strcat(binary_string, pentbit[30]);
							if(debug) printf("DL ");
							newtable = DIGIT;
							break;
						    case LOWER: /* DL */
							strcat(binary_string, pentbit[30]);
							if(debug) printf("DL ");
							newtable = DIGIT;
							break;
						    case MIXED: /* UL DL */
							strcat(binary_string, pentbit[29]);
							if(debug) printf("UL ");
							strcat(binary_string, pentbit[30]);
							if(debug) printf("DL ");
							newtable = DIGIT;
							break;
						    case PUNC: /* UL DL */
							strcat(binary_string, pentbit[31]);
							if(debug) printf("UL ");
							strcat(binary_string, pentbit[30]);
							if(debug) printf("DL ");
							newtable = DIGIT;
							break;
					    }
					    break;
				}
			}
			else {
				/* Latch character */
				switch(typemap[i]) {
					case UPPER: /* To UPPER */
					    switch(curtable) {
						    case LOWER: /* ML UL */
							strcat(binary_string, pentbit[29]);
							if(debug) printf("ML ");
							strcat(binary_string, pentbit[29]);
							if(debug) printf("UL ");
							newtable = UPPER;
							break;
						    case MIXED: /* UL */
							strcat(binary_string, pentbit[29]);
							if(debug) printf("UL ");
							newtable = UPPER;
							break;
						    case PUNC: /* UL */
							strcat(binary_string, pentbit[31]);
							if(debug) printf("UL ");
							newtable = UPPER;
							break;
						    case DIGIT: /* UL */
							strcat(binary_string, quadbit[14]);
							if(debug) printf("UL ");
							newtable = UPPER;
							break;
					    }
					    break;
					case LOWER: /* To LOWER */
					    switch(curtable) {
						    case UPPER: /* LL */
							strcat(binary_string, pentbit[28]);
							if(debug) printf("LL ");
							newtable = LOWER;
							break;
						    case MIXED: /* LL */
							strcat(binary_string, pentbit[28]);
							if(debug) printf("LL ");
							newtable = LOWER;
							break;
						    case PUNC: /* UL LL */
							strcat(binary_string, pentbit[31]);
							if(debug) printf("UL ");
							strcat(binary_string, pentbit[28]);
							if(debug) printf("LL ");
							newtable = LOWER;
							break;
						    case DIGIT: /* UL LL */
							strcat(binary_string, quadbit[14]);
							if(debug) printf("UL ");
							strcat(binary_string, pentbit[28]);
							if(debug) printf("LL ");
							newtable = LOWER;
							break;
					    }
					    break;
					case MIXED: /* To MIXED */
					    switch(curtable) {
						    case UPPER: /* ML */
							strcat(binary_string, pentbit[29]);
							if(debug) printf("ML ");
							newtable = MIXED;
							break;
						    case LOWER: /* ML */
							strcat(binary_string, pentbit[29]);
							if(debug) printf("ML ");
							newtable = MIXED;
							break;
						    case PUNC: /* UL ML */
							strcat(binary_string, pentbit[31]);
							if(debug) printf("UL ");
							strcat(binary_string, pentbit[29]);
							if(debug) printf("ML ");
							newtable = MIXED;
							break;
						    case DIGIT: /* UL ML */
							strcat(binary_string, quadbit[14]);
							if(debug) printf("UL ");
							strcat(binary_string, pentbit[29]);
							if(debug) printf("ML ");
							newtable = MIXED;
							break;
					    }
					    break;
					case PUNC: /* To PUNC */
					    switch(curtable) {
						    case UPPER: /* ML PL */
							strcat(binary_string, pentbit[29]);
							if(debug) printf("ML ");
							strcat(binary_string, pentbit[30]);
							if(debug) printf("PL ");
							newtable = PUNC;
							break;
						    case LOWER: /* ML PL */
							strcat(binary_string, pentbit[29]);
							if(debug) printf("ML ");
							strcat(binary_string, pentbit[30]);
							if(debug) printf("PL ");
							newtable = PUNC;
							break;
						    case MIXED: /* PL */
							strcat(binary_string, pentbit[30]);
							if(debug) printf("PL ");
							newtable = PUNC;
							break;
						    case DIGIT: /* UL ML PL */
							strcat(binary_string, quadbit[14]);
							if(debug) printf("UL ");
							strcat(binary_string, pentbit[29]);
							if(debug) printf("ML ");
							strcat(binary_string, pentbit[30]);
							if(debug) printf("PL ");
							newtable = PUNC;
							break;
					    }
					    break;
					case DIGIT: /* To DIGIT */
					    switch(curtable) {
						    case UPPER: /* DL */
							strcat(binary_string, pentbit[30]);
							if(debug) printf("DL ");
							newtable = DIGIT;
							break;
						    case LOWER: /* DL */
							strcat(binary_string, pentbit[30]);
							if(debug) printf("DL ");
							newtable = DIGIT;
							break;
						    case MIXED: /* UL DL */
							strcat(binary_string, pentbit[29]);
							if(debug) printf("UL ");
							strcat(binary_string, pentbit[30]);
							if(debug) printf("DL ");
							newtable = DIGIT;
							break;
						    case PUNC: /* UL DL */
							strcat(binary_string, pentbit[31]);
							if(debug) printf("UL ");
							strcat(binary_string, pentbit[30]);
							if(debug) printf("DL ");
							newtable = DIGIT;
							break;
					    }
					    break;
					case BINARY: /* To BINARY */
					    lasttable = curtable;
					    switch(curtable) {
						    case UPPER: /* BS */
							strcat(binary_string, pentbit[31]);
							if(debug) printf("BS ");
							newtable = BINARY;
							break;
						    case LOWER: /* BS */
							strcat(binary_string, pentbit[31]);
							if(debug) printf("BS ");
							newtable = BINARY;
							break;
						    case MIXED: /* BS */
							strcat(binary_string, pentbit[31]);
							if(debug) printf("BS ");
							newtable = BINARY;
							break;
						    case PUNC: /* UL BS */
							strcat(binary_string, pentbit[31]);
							if(debug) printf("UL ");
							strcat(binary_string, pentbit[31]);
							if(debug) printf("BS ");
							lasttable = UPPER;
							newtable = BINARY;
							break;
						    case DIGIT: /* UL BS */
							strcat(binary_string, quadbit[14]);
							if(debug) printf("UL ");
							strcat(binary_string, pentbit[31]);
							if(debug) printf("BS ");
							lasttable = UPPER;
							newtable = BINARY;
							break;
					    }

					    bytes = 0;
					    do {
						    bytes++;
					    } while(typemap[i + (bytes - 1)] == BINARY);
					    bytes--;

					    if(bytes > 2079) {
						    return ZINT_ERROR_TOO_LONG;
					    }

					    if(bytes > 31) {
						    /* Put 00000 followed by 11-bit number of bytes less 31 */
						    strcat(binary_string, "00000");

						    for(p = 0; p < 11; p++) {
							    if((bytes - 31) & (0x400 >> p)) {
								    strcat(binary_string, "1");
							    }
							    else {
								    strcat(binary_string, "0");
							    }
						    }
					    }
					    else {
						    /* Put 5-bit number of bytes */
						    for(p = 0; p < 5; p++) {
							    if(bytes & (0x10 >> p)) {
								    strcat(binary_string, "1");
							    }
							    else {
								    strcat(binary_string, "0");
							    }
						    }
					    }
					    if(debug) printf("(%d bytes) ", bytes);

					    break;
				}
			}
		}
		/* Add data to the binary string */
		curtable = newtable;
		chartype = typemap[i];
		if(chartype > 64) {
			chartype -= 64;
		}
		switch(chartype) {
			case UPPER:
			case LOWER:
			case MIXED:
			case PUNC:
			    if((charmap[i] >= 400) && (charmap[i] < 500)) {
				    strcat(binary_string, tribit[charmap[i] - 400]);
				    if(debug) printf("FLG(%d) ", charmap[i] - 400);
			    }
			    else if(charmap[i] >= 500) {
				    strcat(binary_string, quadbit[charmap[i] - 500]);
				    if(debug) printf("[%d] ", charmap[i] - 500);
			    }
			    else {
				    strcat(binary_string, pentbit[charmap[i]]);
				    if(!((chartype == PUNC) && (charmap[i] == 0)))
					    if(debug) printf("%d ", charmap[i]);
			    }
			    break;
			case DIGIT:
			    strcat(binary_string, quadbit[charmap[i]]);
			    if(debug) printf("%d ", charmap[i]);
			    break;
			case BINARY:
			    for(p = 0; p < 8; p++) {
				    if(charmap[i] & (0x80 >> p)) {
					    strcat(binary_string, "1");
				    }
				    else {
					    strcat(binary_string, "0");
				    }
			    }
			    if(debug) printf("%d ", charmap[i]);
			    break;
		}
	}

	if(debug) printf("\n");

	if(strlen(binary_string) > 14970) {
		return ZINT_ERROR_TOO_LONG;
	}

	return 0;
}

/* Prevent data from obscuring reference grid */
static int avoidReferenceGrid(int output)
{
	if(output > 10) {
		output++;
	}
	if(output > 26) {
		output++;
	}
	if(output > 42) {
		output++;
	}
	if(output > 58) {
		output++;
	}
	if(output > 74) {
		output++;
	}
	if(output > 90) {
		output++;
	}
	if(output > 106) {
		output++;
	}
	if(output > 122) {
		output++;
	}
	if(output > 138) {
		output++;
	}
	return output;
}
//
// Calculate the position of the bits in the grid 
//
static void populate_map()
{
	int layer, start, length, n, i;
	int x, y;
	for(x = 0; x < 151; x++) {
		for(y = 0; y < 151; y++) {
			AztecMap[(x * 151) + y] = 0;
		}
	}
	for(layer = 1; layer < 33; layer++) {
		start = (112 * (layer - 1)) + (16 * (layer - 1) * (layer - 1)) + 2;
		length = 28 + ((layer - 1) * 4) + (layer * 4);
		/* Top */
		i = 0;
		x = 64 - ((layer - 1) * 2);
		y = 63 - ((layer - 1) * 2);
		for(n = start; n < (start + length); n += 2) {
			AztecMap[(avoidReferenceGrid(x + i) * 151) + avoidReferenceGrid(y)] = n;
			AztecMap[(avoidReferenceGrid(x + i) * 151) + avoidReferenceGrid(y - 1)] = n + 1;
			i++;
		}
		/* Right */
		i = 0;
		x = 78 + ((layer - 1) * 2);
		y = 64 - ((layer - 1) * 2);
		for(n = start + length; n < (start + (length * 2)); n += 2) {
			AztecMap[(avoidReferenceGrid(x) * 151) + avoidReferenceGrid(y + i)] = n;
			AztecMap[(avoidReferenceGrid(x + 1) * 151) + avoidReferenceGrid(y + i)] = n + 1;
			i++;
		}
		/* Bottom */
		i = 0;
		x = 77 + ((layer - 1) * 2);
		y = 78 + ((layer - 1) * 2);
		for(n = start + (length * 2); n < (start + (length * 3)); n += 2) {
			AztecMap[(avoidReferenceGrid(x - i) * 151) + avoidReferenceGrid(y)] = n;
			AztecMap[(avoidReferenceGrid(x - i) * 151) + avoidReferenceGrid(y + 1)] = n + 1;
			i++;
		}
		/* Left */
		i = 0;
		x = 63 - ((layer - 1) * 2);
		y = 77 + ((layer - 1) * 2);
		for(n = start + (length * 3); n < (start + (length * 4)); n += 2) {
			AztecMap[(avoidReferenceGrid(x) * 151) + avoidReferenceGrid(y - i)] = n;
			AztecMap[(avoidReferenceGrid(x - 1) * 151) + avoidReferenceGrid(y - i)] = n + 1;
			i++;
		}
	}

	/* Central finder pattern */
	for(y = 69; y <= 81; y++) {
		for(x = 69; x <= 81; x++) {
			AztecMap[(x * 151) + y] = 1;
		}
	}
	for(y = 70; y <= 80; y++) {
		for(x = 70; x <= 80; x++) {
			AztecMap[(x * 151) + y] = 0;
		}
	}
	for(y = 71; y <= 79; y++) {
		for(x = 71; x <= 79; x++) {
			AztecMap[(x * 151) + y] = 1;
		}
	}
	for(y = 72; y <= 78; y++) {
		for(x = 72; x <= 78; x++) {
			AztecMap[(x * 151) + y] = 0;
		}
	}
	for(y = 73; y <= 77; y++) {
		for(x = 73; x <= 77; x++) {
			AztecMap[(x * 151) + y] = 1;
		}
	}
	for(y = 74; y <= 76; y++) {
		for(x = 74; x <= 76; x++) {
			AztecMap[(x * 151) + y] = 0;
		}
	}
	// Guide bars 
	for(y = 11; y < 151; y += 16) {
		for(x = 1; x < 151; x += 2) {
			AztecMap[(x * 151) + y] = 1;
			AztecMap[(y * 151) + x] = 1;
		}
	}
	// Descriptor 
	for(i = 0; i < 10; i++) {
		/* Top */
		AztecMap[(avoidReferenceGrid(66 + i) * 151) + avoidReferenceGrid(64)] = 20000 + i;
	}
	for(i = 0; i < 10; i++) {
		/* Right */
		AztecMap[(avoidReferenceGrid(77) * 151) + avoidReferenceGrid(66 + i)] = 20010 + i;
	}
	for(i = 0; i < 10; i++) {
		/* Bottom */
		AztecMap[(avoidReferenceGrid(75 - i) * 151) + avoidReferenceGrid(77)] = 20020 + i;
	}
	for(i = 0; i < 10; i++) {
		/* Left */
		AztecMap[(avoidReferenceGrid(64) * 151) + avoidReferenceGrid(75 - i)] = 20030 + i;
	}
	// Orientation 
	AztecMap[(avoidReferenceGrid(64) * 151) + avoidReferenceGrid(64)] = 1;
	AztecMap[(avoidReferenceGrid(65) * 151) + avoidReferenceGrid(64)] = 1;
	AztecMap[(avoidReferenceGrid(64) * 151) + avoidReferenceGrid(65)] = 1;
	AztecMap[(avoidReferenceGrid(77) * 151) + avoidReferenceGrid(64)] = 1;
	AztecMap[(avoidReferenceGrid(77) * 151) + avoidReferenceGrid(65)] = 1;
	AztecMap[(avoidReferenceGrid(77) * 151) + avoidReferenceGrid(76)] = 1;
}

int aztec(struct ZintSymbol * symbol, uchar source[], int length)
{
	int x, y, i, j, p, data_blocks, ecc_blocks, layers, total_bits;
	char binary_string[20000], bit_pattern[20045], descriptor[42];
	char adjusted_string[20000];
	uchar desc_data[4], desc_ecc[6];
	int err_code, ecc_level, compact, data_length, data_maxsize, codeword_size, adjusted_length;
	int remainder, padbits, count, gs1, adjustment_size;
	int debug = 0, reader = 0;
	int comp_loop = 4;

#ifdef _MSC_VER
	uint* data_part;
	uint* ecc_part;
#endif
	memzero(binary_string, sizeof(binary_string));
	memzero(adjusted_string, sizeof(adjusted_string));
	if(symbol->input_mode == GS1_MODE) {
		gs1 = 1;
	}
	else {
		gs1 = 0;
	}
	if(symbol->output_options & READER_INIT) {
		reader = 1;
		comp_loop = 1;
	}
	if(gs1 && reader) {
		sstrcpy(symbol->errtxt, "Cannot encode in GS1 and Reader Initialisation mode at the same time (E01)");
		return ZINT_ERROR_INVALID_OPTION;
	}
	populate_map();
	err_code = aztec_text_process(source, length, binary_string, gs1, symbol->eci);
	if(err_code != 0) {
		sstrcpy(symbol->errtxt, "Input too long or too many extended ASCII characters (E02)");
		return err_code;
	}
	if(!((symbol->option_1 >= -1) && (symbol->option_1 <= 4))) {
		sstrcpy(symbol->errtxt, "Invalid error correction level - using default instead (E03)");
		err_code = ZINT_WARN_INVALID_OPTION;
		symbol->option_1 = -1;
	}
	ecc_level = symbol->option_1;
	if((ecc_level == -1) || (ecc_level == 0)) {
		ecc_level = 2;
	}
	data_length = (int)strlen(binary_string);
	layers = 0; /* Keep compiler happy! */
	data_maxsize = 0; /* Keep compiler happy! */
	adjustment_size = 0;
	if(symbol->option_2 == 0) { /* The size of the symbol can be determined by Zint */
		do {
			/* Decide what size symbol to use - the smallest that fits the data */
			compact = 0; /* 1 = Aztec Compact, 0 = Normal Aztec */
			layers = 0;
			switch(ecc_level) {
				/* For each level of error correction work out the smallest symbol which
				   the data will fit in */
				case 1: 
					for(i = 32; i > 0; i--) {
					    if((data_length + adjustment_size) < Aztec10DataSizes[i - 1]) {
						    layers = i;
						    compact = 0;
						    data_maxsize = Aztec10DataSizes[i - 1];
					    }
					}
				    for(i = comp_loop; i > 0; i--) {
					    if((data_length + adjustment_size) < AztecCompact10DataSizes[i - 1]) {
						    layers = i;
						    compact = 1;
						    data_maxsize = AztecCompact10DataSizes[i - 1];
					    }
				    }
				    break;
				case 2: 
					for(i = 32; i > 0; i--) {
					    if((data_length + adjustment_size) < Aztec23DataSizes[i - 1]) {
						    layers = i;
						    compact = 0;
						    data_maxsize = Aztec23DataSizes[i - 1];
					    }
					}
				    for(i = comp_loop; i > 0; i--) {
					    if((data_length + adjustment_size) < AztecCompact23DataSizes[i - 1]) {
						    layers = i;
						    compact = 1;
						    data_maxsize = AztecCompact23DataSizes[i - 1];
					    }
				    }
				    break;
				case 3: for(i = 32; i > 0; i--) {
					    if((data_length + adjustment_size) < Aztec36DataSizes[i - 1]) {
						    layers = i;
						    compact = 0;
						    data_maxsize = Aztec36DataSizes[i - 1];
					    }
				}
				    for(i = comp_loop; i > 0; i--) {
					    if((data_length + adjustment_size) < AztecCompact36DataSizes[i - 1]) {
						    layers = i;
						    compact = 1;
						    data_maxsize = AztecCompact36DataSizes[i - 1];
					    }
				    }
				    break;
				case 4: for(i = 32; i > 0; i--) {
					    if((data_length + adjustment_size) < Aztec50DataSizes[i - 1]) {
						    layers = i;
						    compact = 0;
						    data_maxsize = Aztec50DataSizes[i - 1];
					    }
				}
				    for(i = comp_loop; i > 0; i--) {
					    if((data_length + adjustment_size) < AztecCompact50DataSizes[i - 1]) {
						    layers = i;
						    compact = 1;
						    data_maxsize = AztecCompact50DataSizes[i - 1];
					    }
				    }
				    break;
			}

			if(layers == 0) { /* Couldn't find a symbol which fits the data */
				sstrcpy(symbol->errtxt, "Input too long (too many bits for selected ECC) (E04)");
				return ZINT_ERROR_TOO_LONG;
			}
			// Determine codeword bitlength - Table 3 
			codeword_size = 6; // if (layers <= 2) 
			if((layers >= 3) && (layers <= 8)) {
				codeword_size = 8;
			}
			if((layers >= 9) && (layers <= 22)) {
				codeword_size = 10;
			}
			if(layers >= 23) {
				codeword_size = 12;
			}
			j = 0;
			i = 0;
			do {
				if((j + 1) % codeword_size == 0) {
					/* Last bit of codeword */
					int t, done = 0;
					count = 0;
					// Discover how many '1's in current codeword 
					for(t = 0; t < (codeword_size - 1); t++) {
						if(binary_string[(i - (codeword_size - 1)) + t] == '1') count++;
					}
					if(count == (codeword_size - 1)) {
						adjusted_string[j] = '0';
						j++;
						done = 1;
					}
					if(count == 0) {
						adjusted_string[j] = '1';
						j++;
						done = 1;
					}
					if(done == 0) {
						adjusted_string[j] = binary_string[i];
						j++;
						i++;
					}
				}
				adjusted_string[j] = binary_string[i];
				j++;
				i++;
			} while(i <= (data_length + 1));
			adjusted_string[j] = '\0';
			adjusted_length = (int)strlen(adjusted_string);
			adjustment_size = adjusted_length - data_length;
			// Add padding 
			remainder = adjusted_length % codeword_size;
			padbits = codeword_size - remainder;
			if(padbits == codeword_size) {
				padbits = 0;
			}
			for(i = 0; i < padbits; i++) {
				strcat(adjusted_string, "1");
			}
			adjusted_length = (int)strlen(adjusted_string);
			count = 0;
			for(i = (adjusted_length - codeword_size); i < adjusted_length; i++) {
				if(adjusted_string[i] == '1') {
					count++;
				}
			}
			if(count == codeword_size) {
				adjusted_string[adjusted_length - 1] = '0';
			}
			if(debug) {
				printf("Codewords:\n");
				for(i = 0; i < (adjusted_length / codeword_size); i++) {
					for(j = 0; j < codeword_size; j++) {
						printf("%c", adjusted_string[(i * codeword_size) + j]);
					}
					printf("\n");
				}
			}
		} while(adjusted_length > data_maxsize);
		/* This loop will only repeat on the rare occasions when the rule about not having all 1s or all 0s
		   means that the binary string has had to be lengthened beyond the maximum number of bits that can
		   be encoded in a symbol of the selected size */
	}
	else { /* The size of the symbol has been specified by the user */
		if((reader == 1) && ((symbol->option_2 >= 2) && (symbol->option_2 <= 4))) {
			symbol->option_2 = 5;
		}
		if((symbol->option_2 >= 1) && (symbol->option_2 <= 4)) {
			compact = 1;
			layers = symbol->option_2;
		}
		if((symbol->option_2 >= 5) && (symbol->option_2 <= 36)) {
			compact = 0;
			layers = symbol->option_2 - 4;
		}
		if((symbol->option_2 < 0) || (symbol->option_2 > 36)) {
			sstrcpy(symbol->errtxt, "Invalid Aztec Code size");
			return ZINT_ERROR_INVALID_OPTION;
		}
		// Determine codeword bitlength - Table 3 
		if((layers >= 0) && (layers <= 2)) {
			codeword_size = 6;
		}
		if((layers >= 3) && (layers <= 8)) {
			codeword_size = 8;
		}
		if((layers >= 9) && (layers <= 22)) {
			codeword_size = 10;
		}
		if(layers >= 23) {
			codeword_size = 12;
		}
		j = 0;
		i = 0;
		do {
			if((j + 1) % codeword_size == 0) {
				/* Last bit of codeword */
				int t, done = 0;
				count = 0;

				/* Discover how many '1's in current codeword */
				for(t = 0; t < (codeword_size - 1); t++) {
					if(binary_string[(i - (codeword_size - 1)) + t] == '1') count++;
				}

				if(count == (codeword_size - 1)) {
					adjusted_string[j] = '0';
					j++;
					done = 1;
				}

				if(count == 0) {
					adjusted_string[j] = '1';
					j++;
					done = 1;
				}

				if(done == 0) {
					adjusted_string[j] = binary_string[i];
					j++;
					i++;
				}
			}
			adjusted_string[j] = binary_string[i];
			j++;
			i++;
		} while(i <= (data_length + 1));
		adjusted_string[j] = '\0';
		adjusted_length = (int)strlen(adjusted_string);

		remainder = adjusted_length % codeword_size;

		padbits = codeword_size - remainder;
		if(padbits == codeword_size) {
			padbits = 0;
		}

		for(i = 0; i < padbits; i++) {
			strcat(adjusted_string, "1");
		}
		adjusted_length = (int)strlen(adjusted_string);

		count = 0;
		for(i = (adjusted_length - codeword_size); i < adjusted_length; i++) {
			if(adjusted_string[i] == '1') {
				count++;
			}
		}
		if(count == codeword_size) {
			adjusted_string[adjusted_length - 1] = '0';
		}
		// Check if the data actually fits into the selected symbol size 
		if(compact) {
			data_maxsize = codeword_size * (AztecCompactSizes[layers - 1] - 3);
		}
		else {
			data_maxsize = codeword_size * (AztecSizes[layers - 1] - 3);
		}
		if(adjusted_length > data_maxsize) {
			sstrcpy(symbol->errtxt, "Data too long for specified Aztec Code symbol size (E05)");
			return ZINT_ERROR_TOO_LONG;
		}
		if(debug) {
			printf("Codewords:\n");
			for(i = 0; i < (adjusted_length / codeword_size); i++) {
				for(j = 0; j < codeword_size; j++) {
					printf("%c", adjusted_string[(i * codeword_size) + j]);
				}
				printf("\n");
			}
		}
	}
	if(reader && (layers > 22)) {
		sstrcpy(symbol->errtxt, "Data too long for reader initialisation symbol (E06)");
		return ZINT_ERROR_TOO_LONG;
	}
	data_blocks = adjusted_length / codeword_size;
	if(compact) {
		ecc_blocks = AztecCompactSizes[layers - 1] - data_blocks;
	}
	else {
		ecc_blocks = AztecSizes[layers - 1] - data_blocks;
	}
	if(debug) {
		printf("Generating a ");
		if(compact) {
			printf("compact");
		}
		else {
			printf("full-size");
		}
		printf(" symbol with %d layers\n", layers);
		printf("Requires ");
		if(compact) {
			printf("%d", AztecCompactSizes[layers - 1]);
		}
		else {
			printf("%d", AztecSizes[layers - 1]);
		}
		printf(" codewords of %d-bits\n", codeword_size);
		printf("    (%d data words, %d ecc words)\n", data_blocks, ecc_blocks);
	}
#ifndef _MSC_VER
	uint data_part[data_blocks + 3], ecc_part[ecc_blocks + 3];
#else
	data_part = (uint *)_alloca((data_blocks + 3) * sizeof(uint));
	ecc_part = (uint *)_alloca((ecc_blocks + 3) * sizeof(uint));
#endif
	// Copy across data into separate integers 
	memzero(data_part, (data_blocks + 2) * sizeof(int));
	memzero(ecc_part, (ecc_blocks + 2) * sizeof(int));
	// Split into codewords and calculate reed-colomon error correction codes 
	switch(codeword_size) {
		case 6:
		    for(i = 0; i < data_blocks; i++) {
			    for(p = 0; p < 6; p++) {
				    if(adjusted_string[i * codeword_size + p] == '1') {
					    data_part[i] += (0x20 >> p);
				    }
			    }
		    }
		    rs_init_gf(0x43);
		    rs_init_code(ecc_blocks, 1);
		    rs_encode_long(data_blocks, data_part, ecc_part);
		    for(i = (ecc_blocks - 1); i >= 0; i--) {
			    for(p = 0; p < 6; p++) {
				    if(ecc_part[i] & (0x20 >> p)) {
					    strcat(adjusted_string, "1");
				    }
				    else {
					    strcat(adjusted_string, "0");
				    }
			    }
		    }
		    rs_free();
		    break;
		case 8:
		    for(i = 0; i < data_blocks; i++) {
			    for(p = 0; p < 8; p++) {
				    if(adjusted_string[i * codeword_size + p] == '1') {
					    data_part[i] += (0x80 >> p);
				    }
			    }
		    }
		    rs_init_gf(0x12d);
		    rs_init_code(ecc_blocks, 1);
		    rs_encode_long(data_blocks, data_part, ecc_part);
		    for(i = (ecc_blocks - 1); i >= 0; i--) {
			    for(p = 0; p < 8; p++) {
				    if(ecc_part[i] & (0x80 >> p)) {
					    strcat(adjusted_string, "1");
				    }
				    else {
					    strcat(adjusted_string, "0");
				    }
			    }
		    }
		    rs_free();
		    break;
		case 10:
		    for(i = 0; i < data_blocks; i++) {
			    for(p = 0; p < 10; p++) {
				    if(adjusted_string[i * codeword_size + p] == '1') {
					    data_part[i] += (0x200 >> p);
				    }
			    }
		    }
		    rs_init_gf(0x409);
		    rs_init_code(ecc_blocks, 1);
		    rs_encode_long(data_blocks, data_part, ecc_part);
		    for(i = (ecc_blocks - 1); i >= 0; i--) {
			    for(p = 0; p < 10; p++) {
				    if(ecc_part[i] & (0x200 >> p)) {
					    strcat(adjusted_string, "1");
				    }
				    else {
					    strcat(adjusted_string, "0");
				    }
			    }
		    }
		    rs_free();
		    break;
		case 12:
		    for(i = 0; i < data_blocks; i++) {
			    for(p = 0; p < 12; p++) {
				    if(adjusted_string[i * codeword_size + p] == '1') {
					    data_part[i] += (0x800 >> p);
				    }
			    }
		    }
		    rs_init_gf(0x1069);
		    rs_init_code(ecc_blocks, 1);
		    rs_encode_long(data_blocks, data_part, ecc_part);
		    for(i = (ecc_blocks - 1); i >= 0; i--) {
			    for(p = 0; p < 12; p++) {
				    if(ecc_part[i] & (0x800 >> p)) {
					    strcat(adjusted_string, "1");
				    }
				    else {
					    strcat(adjusted_string, "0");
				    }
			    }
		    }
		    rs_free();
		    break;
	}
	/* Invert the data so that actual data is on the outside and reed-solomon on the inside */
	memset(bit_pattern, '0', 20045);
	total_bits = (data_blocks + ecc_blocks) * codeword_size;
	for(i = 0; i < total_bits; i++) {
		bit_pattern[i] = adjusted_string[total_bits - i - 1];
	}
	/* Now add the symbol descriptor */
	memzero(desc_data, 4);
	memzero(desc_ecc, 6);
	memzero(descriptor, 42);
	if(compact) {
		/* The first 2 bits represent the number of layers minus 1 */
		if((layers - 1) & 0x02) {
			descriptor[0] = '1';
		}
		else {
			descriptor[0] = '0';
		}
		if((layers - 1) & 0x01) {
			descriptor[1] = '1';
		}
		else {
			descriptor[1] = '0';
		}
		/* The next 6 bits represent the number of data blocks minus 1 */
		if(reader) {
			descriptor[2] = '1';
		}
		else {
			if((data_blocks - 1) & 0x20) {
				descriptor[2] = '1';
			}
			else {
				descriptor[2] = '0';
			}
		}

		for(i = 3; i < 8; i++) {
			if((data_blocks - 1) & (0x10 >> (i - 3))) {
				descriptor[i] = '1';
			}
			else {
				descriptor[i] = '0';
			}
		}

		descriptor[8] = '\0';
		if(debug) printf("Mode Message = %s\n", descriptor);
	}
	else {
		/* The first 5 bits represent the number of layers minus 1 */
		for(i = 0; i < 5; i++) {
			if((layers - 1) & (0x10 >> i)) {
				descriptor[i] = '1';
			}
			else {
				descriptor[i] = '0';
			}
		}
		// The next 11 bits represent the number of data blocks minus 1 
		if(reader) {
			descriptor[5] = '1';
		}
		else {
			if((data_blocks - 1) & 0x400) {
				descriptor[5] = '1';
			}
			else {
				descriptor[5] = '0';
			}
		}
		for(i = 6; i < 16; i++) {
			if((data_blocks - 1) & (0x200 >> (i - 6))) {
				descriptor[i] = '1';
			}
			else {
				descriptor[i] = '0';
			}
		}
		descriptor[16] = '\0';
		if(debug) printf("Mode Message = %s\n", descriptor);
	}
	// Split into 4-bit codewords 
	for(i = 0; i < 4; i++) {
		if(descriptor[i * 4] == '1') {
			desc_data[i] += 8;
		}
		if(descriptor[(i * 4) + 1] == '1') {
			desc_data[i] += 4;
		}
		if(descriptor[(i * 4) + 2] == '1') {
			desc_data[i] += 2;
		}
		if(descriptor[(i * 4) + 3] == '1') {
			desc_data[i] += 1;
		}
	}
	// Add reed-solomon error correction with Galois field GF(16) and prime modulus
	// x^4 + x + 1 (section 7.2.3)
	rs_init_gf(0x13);
	if(compact) {
		rs_init_code(5, 1);
		rs_encode(2, desc_data, desc_ecc);
		for(i = 0; i < 5; i++) {
			if(desc_ecc[4 - i] & 0x08) {
				descriptor[(i * 4) + 8] = '1';
			}
			else {
				descriptor[(i * 4) + 8] = '0';
			}
			if(desc_ecc[4 - i] & 0x04) {
				descriptor[(i * 4) + 9] = '1';
			}
			else {
				descriptor[(i * 4) + 9] = '0';
			}
			if(desc_ecc[4 - i] & 0x02) {
				descriptor[(i * 4) + 10] = '1';
			}
			else {
				descriptor[(i * 4) + 10] = '0';
			}
			if(desc_ecc[4 - i] & 0x01) {
				descriptor[(i * 4) + 11] = '1';
			}
			else {
				descriptor[(i * 4) + 11] = '0';
			}
		}
	}
	else {
		rs_init_code(6, 1);
		rs_encode(4, desc_data, desc_ecc);
		for(i = 0; i < 6; i++) {
			if(desc_ecc[5 - i] & 0x08) {
				descriptor[(i * 4) + 16] = '1';
			}
			else {
				descriptor[(i * 4) + 16] = '0';
			}
			if(desc_ecc[5 - i] & 0x04) {
				descriptor[(i * 4) + 17] = '1';
			}
			else {
				descriptor[(i * 4) + 17] = '0';
			}
			if(desc_ecc[5 - i] & 0x02) {
				descriptor[(i * 4) + 18] = '1';
			}
			else {
				descriptor[(i * 4) + 18] = '0';
			}
			if(desc_ecc[5 - i] & 0x01) {
				descriptor[(i * 4) + 19] = '1';
			}
			else {
				descriptor[(i * 4) + 19] = '0';
			}
		}
	}
	rs_free();
	// Merge descriptor with the rest of the symbol 
	for(i = 0; i < 40; i++) {
		if(compact) {
			bit_pattern[2000 + i - 2] = descriptor[i];
		}
		else {
			bit_pattern[20000 + i - 2] = descriptor[i];
		}
	}
	// Plot all of the data into the symbol in pre-defined spiral pattern 
	if(compact) {
		for(y = AztecCompactOffset[layers - 1]; y < (27 - AztecCompactOffset[layers - 1]); y++) {
			for(x = AztecCompactOffset[layers - 1]; x < (27 - AztecCompactOffset[layers - 1]); x++) {
				if(CompactAztecMap[(y * 27) + x] == 1) {
					set_module(symbol, y - AztecCompactOffset[layers - 1], x - AztecCompactOffset[layers - 1]);
				}
				if(CompactAztecMap[(y * 27) + x] >= 2) {
					if(bit_pattern[CompactAztecMap[(y * 27) + x] - 2] == '1') {
						set_module(symbol, y - AztecCompactOffset[layers - 1], x - AztecCompactOffset[layers - 1]);
					}
				}
			}
			symbol->row_height[y - AztecCompactOffset[layers - 1]] = 1;
		}
		symbol->rows = 27 - (2 * AztecCompactOffset[layers - 1]);
		symbol->width = 27 - (2 * AztecCompactOffset[layers - 1]);
	}
	else {
		for(y = AztecOffset[layers - 1]; y < (151 - AztecOffset[layers - 1]); y++) {
			for(x = AztecOffset[layers - 1]; x < (151 - AztecOffset[layers - 1]); x++) {
				if(AztecMap[(y * 151) + x] == 1) {
					set_module(symbol, y - AztecOffset[layers - 1], x - AztecOffset[layers - 1]);
				}
				if(AztecMap[(y * 151) + x] >= 2) {
					if(bit_pattern[AztecMap[(y * 151) + x] - 2] == '1') {
						set_module(symbol, y - AztecOffset[layers - 1], x - AztecOffset[layers - 1]);
					}
				}
			}
			symbol->row_height[y - AztecOffset[layers - 1]] = 1;
		}
		symbol->rows = 151 - (2 * AztecOffset[layers - 1]);
		symbol->width = 151 - (2 * AztecOffset[layers - 1]);
	}
	return err_code;
}
//
// Encodes Aztec runes as specified in ISO/IEC 24778:2008 Annex A 
//
int aztec_runes(struct ZintSymbol * symbol, const uchar source[], int length)
{
	int i, p, y, x;
	char binary_string[28];
	uchar data_codewords[3], ecc_codewords[6];
	int error_number = 0;
	int input_value = 0;
	if(length > 3) {
		sstrcpy(symbol->errtxt, "Input too large (E07)");
		return ZINT_ERROR_INVALID_DATA;
	}
	error_number = is_sane(NEON, source, length);
	if(error_number != 0) {
		sstrcpy(symbol->errtxt, "Invalid characters in input (E08)");
		return ZINT_ERROR_INVALID_DATA;
	}
	switch(length) {
		case 3: input_value = 100 * hex(source[0]);
		    input_value += 10 * hex(source[1]);
		    input_value += hex(source[2]);
		    break;
		case 2: input_value = 10 * hex(source[0]);
		    input_value += hex(source[1]);
		    break;
		case 1: input_value = hex(source[0]);
		    break;
	}
	if(input_value > 255) {
		sstrcpy(symbol->errtxt, "Input too large (E09)");
		return ZINT_ERROR_INVALID_DATA;
	}
	else {
		sstrcpy(binary_string, "");
		for(p = 0; p < 8; p++) {
			if(input_value & (0x80 >> p)) {
				strcat(binary_string, "1");
			}
			else {
				strcat(binary_string, "0");
			}
		}
		data_codewords[0] = 0;
		data_codewords[1] = 0;
		for(i = 0; i < 2; i++) {
			if(binary_string[i * 4] == '1') {
				data_codewords[i] += 8;
			}
			if(binary_string[(i * 4) + 1] == '1') {
				data_codewords[i] += 4;
			}
			if(binary_string[(i * 4) + 2] == '1') {
				data_codewords[i] += 2;
			}
			if(binary_string[(i * 4) + 3] == '1') {
				data_codewords[i] += 1;
			}
		}
		rs_init_gf(0x13);
		rs_init_code(5, 1);
		rs_encode(2, data_codewords, ecc_codewords);
		rs_free();
		sstrcpy(binary_string, "");
		for(i = 0; i < 5; i++) {
			if(ecc_codewords[4 - i] & 0x08) {
				binary_string[(i * 4) + 8] = '1';
			}
			else {
				binary_string[(i * 4) + 8] = '0';
			}
			if(ecc_codewords[4 - i] & 0x04) {
				binary_string[(i * 4) + 9] = '1';
			}
			else {
				binary_string[(i * 4) + 9] = '0';
			}
			if(ecc_codewords[4 - i] & 0x02) {
				binary_string[(i * 4) + 10] = '1';
			}
			else {
				binary_string[(i * 4) + 10] = '0';
			}
			if(ecc_codewords[4 - i] & 0x01) {
				binary_string[(i * 4) + 11] = '1';
			}
			else {
				binary_string[(i * 4) + 11] = '0';
			}
		}
		for(i = 0; i < 28; i += 2) {
			binary_string[i] = (binary_string[i] == '1') ? '0' : '1';
		}
		for(y = 8; y < 19; y++) {
			for(x = 8; x < 19; x++) {
				if(CompactAztecMap[(y * 27) + x] == 1) {
					set_module(symbol, y - 8, x - 8);
				}
				if(CompactAztecMap[(y * 27) + x] >= 2) {
					if(binary_string[CompactAztecMap[(y * 27) + x] - 2000] == '1') {
						set_module(symbol, y - 8, x - 8);
					}
				}
			}
			symbol->row_height[y - 8] = 1;
		}
		symbol->rows = 11;
		symbol->width = 11;
		return 0;
	}
}

