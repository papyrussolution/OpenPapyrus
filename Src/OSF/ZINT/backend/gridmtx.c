/* gridmtx.c - Grid Matrix

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

/* This file impliments Grid Matrix as specified in
   AIM Global Document Number AIMD014 Rev. 1.63 Revised 9 Dec 2008 */
#include "common.h"
#pragma hdrstop
#include "gb2312.h"
//
// Definitions for Grid Matrix
//
#define GM_NUMBER	1
#define GM_LOWER	2
#define GM_UPPER	3
#define GM_MIXED	4
#define GM_CONTROL	5
#define GM_BYTE		6
#define GM_CHINESE	7

#define EUROPIUM	"0123456789ABCDEFGHIJKLMOPRSTUVWXYZabcdefghijklmnopqrstuvwxyz "

static const char shift_set[] = {
    /* From Table 7 - Encoding of control characters */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, /* NULL -> SI */
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, /* DLE -> US */
    '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', ':',
    ';', '<', '=', '>', '?', '@', '[', '\\', ']', '^', '_', '`', '{', '|', '}', '~'
};

static const int gm_recommend_cw[] = { 9, 30, 59, 114, 170, 237, 315, 405, 506, 618, 741, 875, 1021 };
static const int gm_max_cw[] = { 11, 40, 79, 146, 218, 305, 405, 521, 650, 794, 953, 1125, 1313 };

static const int gm_data_codewords[] = {
    0, 15, 13, 11, 9,
    45, 40, 35, 30, 25,
    89, 79, 69, 59, 49,
    146, 130, 114, 98, 81,
    218, 194, 170, 146, 121,
    305, 271, 237, 203, 169,
    405, 360, 315, 270, 225,
    521, 463, 405, 347, 289,
    650, 578, 506, 434, 361,
    794, 706, 618, 530, 441,
    953, 847, 741, 635, 529,
    1125, 1000, 875, 750, 625,
    1313, 1167, 1021, 875, 729
};

static const int gm_n1[] = { 18, 50, 98, 81, 121, 113, 113, 116, 121, 126, 118, 125, 122 };
static const int gm_b1[] = { 1, 1, 1, 2, 2, 2, 2, 3, 2, 7, 5, 10, 6 };
static const int gm_b2[] = { 0, 0, 0, 0, 0, 1, 2, 2, 4, 0, 4, 0, 6 };

/* Values from table A.1 */
static const int gm_ebeb[] = {
    /* E1 B3 E2 B4 */
    0, 0, 0, 0, // version 1
    3, 1, 0, 0,
    5, 1, 0, 0,
    7, 1, 0, 0,
    9, 1, 0, 0,
    5, 1, 0, 0, // version 2
    10, 1, 0, 0,
    15, 1, 0, 0,
    20, 1, 0, 0,
    25, 1, 0, 0,
    9, 1, 0, 0, // version 3
    19, 1, 0, 0,
    29, 1, 0, 0,
    39, 1, 0, 0,
    49, 1, 0, 0,
    8, 2, 0, 0, // version 4
    16, 2, 0, 0,
    24, 2, 0, 0,
    32, 2, 0, 0,
    41, 1, 40, 1,
    12, 2, 0, 0, // version 5
    24, 2, 0, 0,
    36, 2, 0, 0,
    48, 2, 0, 0,
    61, 1, 60, 1,
    11, 3, 0, 0, // version 6
    23, 1, 22, 2,
    34, 2, 33, 1,
    45, 3, 0, 0,
    57, 1, 56, 2,
    12, 1, 11, 3, // version 7
    23, 2, 22, 2,
    34, 3, 33, 1,
    45, 4, 0, 0,
    57, 1, 56, 3,
    12, 2, 11, 3, // version 8
    23, 5, 0, 0,
    35, 3, 34, 2,
    47, 1, 46, 4,
    58, 4, 57, 1,
    12, 6, 0, 0, // version 9
    24, 6, 0, 0,
    36, 6, 0, 0,
    48, 6, 0, 0,
    61, 1, 60, 5,
    13, 4, 12, 3, // version 10
    26, 1, 25, 6,
    38, 5, 37, 2,
    51, 2, 50, 5,
    63, 7, 0, 0,
    12, 6, 11, 3, // version 11
    24, 4, 23, 5,
    36, 2, 35, 7,
    47, 9, 0, 0,
    59, 7, 58, 2,
    13, 5, 12, 5, // version 12
    25, 10, 0, 0,
    38, 5, 37, 5,
    50, 10, 0, 0,
    63, 5, 62, 5,
    13, 1, 12, 11, //version 13
    25, 3, 24, 9,
    37, 5, 36, 7,
    49, 7, 48, 5,
    61, 9, 60, 3
};

static const int gm_macro_matrix[] = {
    728, 625, 626, 627, 628, 629, 630, 631, 632, 633, 634, 635, 636, 637, 638, 639, 640, 641, 642, 643, 644, 645, 646, 647, 648, 649, 650,
    727, 624, 529, 530, 531, 532, 533, 534, 535, 536, 537, 538, 539, 540, 541, 542, 543, 544, 545, 546, 547, 548, 549, 550, 551, 552, 651,
    726, 623, 528, 441, 442, 443, 444, 445, 446, 447, 448, 449, 450, 451, 452, 453, 454, 455, 456, 457, 458, 459, 460, 461, 462, 553, 652,
    725, 622, 527, 440, 361, 362, 363, 364, 365, 366, 367, 368, 369, 370, 371, 372, 373, 374, 375, 376, 377, 378, 379, 380, 463, 554, 653,
    724, 621, 526, 439, 360, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300, 301, 302, 303, 304, 305, 306, 381, 464, 555, 654,
    723, 620, 525, 438, 359, 288, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 307, 382, 465, 556, 655,
    722, 619, 524, 437, 358, 287, 224, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 241, 308, 383, 466, 557, 656,
    721, 618, 523, 436, 357, 286, 223, 168, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 183, 242, 309, 384, 467, 558, 657,
    720, 617, 522, 435, 356, 285, 222, 167, 120, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 133, 184, 243, 310, 385, 468, 559, 658,
    719, 616, 521, 434, 355, 284, 221, 166, 119, 80, 49, 50, 51, 52, 53, 54, 55, 56, 91, 134, 185, 244, 311, 386, 469, 560, 659,
    718, 615, 520, 433, 354, 283, 220, 165, 118, 79, 48, 25, 26, 27, 28, 29, 30, 57, 92, 135, 186, 245, 312, 387, 470, 561, 660,
    717, 614, 519, 432, 353, 282, 219, 164, 117, 78, 47, 24, 9, 10, 11, 12, 31, 58, 93, 136, 187, 246, 313, 388, 471, 562, 661,
    716, 613, 518, 431, 352, 281, 218, 163, 116, 77, 46, 23, 8, 1, 2, 13, 32, 59, 94, 137, 188, 247, 314, 389, 472, 563, 662,
    715, 612, 517, 430, 351, 280, 217, 162, 115, 76, 45, 22, 7, 0, 3, 14, 33, 60, 95, 138, 189, 248, 315, 390, 473, 564, 663,
    714, 611, 516, 429, 350, 279, 216, 161, 114, 75, 44, 21, 6, 5, 4, 15, 34, 61, 96, 139, 190, 249, 316, 391, 474, 565, 664,
    713, 610, 515, 428, 349, 278, 215, 160, 113, 74, 43, 20, 19, 18, 17, 16, 35, 62, 97, 140, 191, 250, 317, 392, 475, 566, 665,
    712, 609, 514, 427, 348, 277, 214, 159, 112, 73, 42, 41, 40, 39, 38, 37, 36, 63, 98, 141, 192, 251, 318, 393, 476, 567, 666,
    711, 608, 513, 426, 347, 276, 213, 158, 111, 72, 71, 70, 69, 68, 67, 66, 65, 64, 99, 142, 193, 252, 319, 394, 477, 568, 667,
    710, 607, 512, 425, 346, 275, 212, 157, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 143, 194, 253, 320, 395, 478, 569, 668,
    709, 606, 511, 424, 345, 274, 211, 156, 155, 154, 153, 152, 151, 150, 149, 148, 147, 146, 145, 144, 195, 254, 321, 396, 479, 570, 669,
    708, 605, 510, 423, 344, 273, 210, 209, 208, 207, 206, 205, 204, 203, 202, 201, 200, 199, 198, 197, 196, 255, 322, 397, 480, 571, 670,
    707, 604, 509, 422, 343, 272, 271, 270, 269, 268, 267, 266, 265, 264, 263, 262, 261, 260, 259, 258, 257, 256, 323, 398, 481, 572, 671,
    706, 603, 508, 421, 342, 341, 340, 339, 338, 337, 336, 335, 334, 333, 332, 331, 330, 329, 328, 327, 326, 325, 324, 399, 482, 573, 672,
    705, 602, 507, 420, 419, 418, 417, 416, 415, 414, 413, 412, 411, 410, 409, 408, 407, 406, 405, 404, 403, 402, 401, 400, 483, 574, 673,
    704, 601, 506, 505, 504, 503, 502, 501, 500, 499, 498, 497, 496, 495, 494, 493, 492, 491, 490, 489, 488, 487, 486, 485, 484, 575, 674,
    703, 600, 599, 598, 597, 596, 595, 594, 593, 592, 591, 590, 589, 588, 587, 586, 585, 584, 583, 582, 581, 580, 579, 578, 577, 576, 675,
    702, 701, 700, 699, 698, 697, 696, 695, 694, 693, 692, 691, 690, 689, 688, 687, 686, 685, 684, 683, 682, 681, 680, 679, 678, 677, 676,
};
//
//
//
int number_lat(int gbdata[], int length, int position)
{
	/* Attempt to calculate the 'cost' of using numeric mode from a given position in number of bits */
	/* Also ensures that numeric mode is not selected when it cannot be used: for example in
	   a string which has "2.2.0" (cannot have more than one non-numeric character for each
	   block of three numeric characters) */
	int    numb = 0, nonum = 0, done;
	int    tally = 0;
	int    sp = position;
	do {
		done = 0;
		if((gbdata[sp] >= '0') && (gbdata[sp] <= '9')) {
			numb++;
			done = 1;
		}
		switch(gbdata[sp]) {
			case ' ':
			case '+':
			case '-':
			case '.':
			case ',':
			    nonum++;
			    done = 1;
		}
		if((sp + 1) < length) {
			if((gbdata[sp] == 0x13) && (gbdata[sp + 1] == 0x10)) {
				nonum++;
				done = 1;
				sp++;
			}
		}

		if(done == 0) {
			tally += 80;
		}
		else {
			if(numb == 3) {
				if(nonum == 0) {
					tally += 10;
				}
				if(nonum == 1) {
					tally += 20;
				}
				if(nonum > 1) {
					tally += 80;
				}
				numb = 0;
				nonum = 0;
			}
		}

		sp++;
	} while((sp < length) && (sp <= (position + 8)));

	if(numb == 0) {
		tally += 80;
	}

	if(numb > 1) {
		if(nonum == 0) {
			tally += 10;
		}
		if(nonum == 1) {
			tally += 20;
		}
		if(nonum > 1) {
			tally += 80;
		}
	}

	return tally;
}

int seek_forward(int gbdata[], int length, int position, int current_mode)
{
	/* In complete contrast to the method recommended in Annex D of the ANSI standard this
	   code uses a look-ahead test in the same manner as Data Matrix. This decision was made
	   because the "official" algorithm does not provide clear methods for dealing with all
	   possible combinations of input data */

	int number_count, byte_count, mixed_count, upper_count, lower_count, chinese_count;
	int sp, best_mode, done;
	int best_count, last = -1;
	int debug = 0;

	if(gbdata[position] > 0xff) {
		return GM_CHINESE;
	}

	switch(current_mode) {
		case GM_CHINESE:
		    number_count = 13;
		    byte_count = 13;
		    mixed_count = 13;
		    upper_count = 13;
		    lower_count = 13;
		    chinese_count = 0;
		    break;
		case GM_NUMBER:
		    number_count = 0;
		    byte_count = 10;
		    mixed_count = 10;
		    upper_count = 10;
		    lower_count = 10;
		    chinese_count = 10;
		    break;
		case GM_LOWER:
		    number_count = 5;
		    byte_count = 7;
		    mixed_count = 7;
		    upper_count = 5;
		    lower_count = 0;
		    chinese_count = 5;
		    break;
		case GM_UPPER:
		    number_count = 5;
		    byte_count = 7;
		    mixed_count = 7;
		    upper_count = 0;
		    lower_count = 5;
		    chinese_count = 5;
		    break;
		case GM_MIXED:
		    number_count = 10;
		    byte_count = 10;
		    mixed_count = 0;
		    upper_count = 10;
		    lower_count = 10;
		    chinese_count = 10;
		    break;
		case GM_BYTE:
		    number_count = 4;
		    byte_count = 0;
		    mixed_count = 4;
		    upper_count = 4;
		    lower_count = 4;
		    chinese_count = 4;
		    break;
		default: /* Start of symbol */
		    number_count = 4;
		    byte_count = 4;
		    mixed_count = 4;
		    upper_count = 4;
		    lower_count = 4;
		    chinese_count = 4;
	}

	for(sp = position; (sp < length) && (sp <= (position + 8)); sp++) {
		done = 0;

		if(gbdata[sp] >= 0xff) {
			byte_count += 17;
			mixed_count += 23;
			upper_count += 18;
			lower_count += 18;
			chinese_count += 13;
			done = 1;
		}

		if((gbdata[sp] >= 'a') && (gbdata[sp] <= 'z')) {
			byte_count += 8;
			mixed_count += 6;
			upper_count += 10;
			lower_count += 5;
			chinese_count += 13;
			done = 1;
		}

		if((gbdata[sp] >= 'A') && (gbdata[sp] <= 'Z')) {
			byte_count += 8;
			mixed_count += 6;
			upper_count += 5;
			lower_count += 10;
			chinese_count += 13;
			done = 1;
		}

		if((gbdata[sp] >= '0') && (gbdata[sp] <= '9')) {
			byte_count += 8;
			mixed_count += 6;
			upper_count += 8;
			lower_count += 8;
			chinese_count += 13;
			done = 1;
		}

		if(gbdata[sp] == ' ') {
			byte_count += 8;
			mixed_count += 6;
			upper_count += 5;
			lower_count += 5;
			chinese_count += 13;
			done = 1;
		}

		if(done == 0) {
			/* Control character */
			byte_count += 8;
			mixed_count += 16;
			upper_count += 13;
			lower_count += 13;
			chinese_count += 13;
		}

		if(gbdata[sp] >= 0x7f) {
			mixed_count += 20;
			upper_count += 20;
			lower_count += 20;
		}
	}

	/* Adjust for <end of line> */
	for(sp = position; (sp < (length - 1)) && (sp <= (position + 7)); sp++) {
		if((gbdata[sp] == 0x13) && (gbdata[sp + 1] == 0x10)) {
			chinese_count -= 13;
		}
	}

	/* Adjust for double digits */
	for(sp = position; (sp < (length - 1)) && (sp <= (position + 7)); sp++) {
		if(sp != last) {
			if(((gbdata[sp] >= '0') && (gbdata[sp] <= '9')) && ((gbdata[sp + 1] >= '0') && (gbdata[sp + 1] <= '9'))) {
				chinese_count -= 13;
				last = sp + 1;
			}
		}
	}

	/* Numeric mode is more complex */
	number_count += number_lat(gbdata, length, position);

	if(debug) {
		printf("C %d / B %d / M %d / U %d / L %d / N %d\n",
		    chinese_count,
		    byte_count,
		    mixed_count,
		    upper_count,
		    lower_count,
		    number_count);
	}

	best_count = chinese_count;
	best_mode = GM_CHINESE;

	if(byte_count <= best_count) {
		best_count = byte_count;
		best_mode = GM_BYTE;
	}

	if(mixed_count <= best_count) {
		best_count = mixed_count;
		best_mode = GM_MIXED;
	}

	if(upper_count <= best_count) {
		best_count = upper_count;
		best_mode = GM_UPPER;
	}

	if(lower_count <= best_count) {
		best_count = lower_count;
		best_mode = GM_LOWER;
	}

	if(number_count <= best_count) {
		best_count = number_count;
		best_mode = GM_NUMBER;
	}

	return best_mode;
}

/* Add the length indicator for byte encoded blocks */
static void add_byte_count(char binary[], const size_t byte_count_posn, const int byte_count)
{
	int p;

	for(p = 0; p < 8; p++) {
		if(byte_count & (0x100 >> p)) {
			binary[byte_count_posn + p] = '0';
		}
		else {
			binary[byte_count_posn + p] = '1';
		}
	}
}

/* Add a control character to the data stream */
void add_shift_char(char binary[], int shifty)
{
	int i, p, debug = 0;
	int glyph = 0;

	for(i = 0; i < 64; i++) {
		if(shift_set[i] == shifty) {
			glyph = i;
		}
	}

	if(debug) {
		printf("SHIFT [%d] ", glyph);
	}

	for(p = 0; p < 6; p++) {
		if(glyph & (0x20 >> p)) {
			strcat(binary, "1");
		}
		else {
			strcat(binary, "0");
		}
	}
}

int gm_encode(int gbdata[], int length, char binary[], int reader, int eci)
{
	/* Create a binary stream representation of the input data.
	   7 sets are defined - Chinese characters, Numerals, Lower case letters, Upper case letters,
	   Mixed numerals and latters, Control characters and 8-bit binary data */
	int sp, current_mode, next_mode, last_mode, glyph = 0, q;
	int c1, c2, done;
	int p = 0, ppos;
	int numbuf[3], punt = 0;
	size_t number_pad_posn, byte_count_posn = 0;
	int byte_count = 0, debug = 0;
	int shift, i;

	sstrcpy(binary, "");

	sp = 0;
	current_mode = 0;
	last_mode = 0;
	number_pad_posn = 0;

	if(reader) {
		strcat(binary, "1010"); /* FNC3 - Reader Initialisation */
	}

	if(eci != 3) {
		strcat(binary, "11000"); /* ECI */
		for(q = 0; q < 10; q++) {
			if(eci & (0x100 >> q)) {
				strcat(binary, "1");
			}
			else {
				strcat(binary, "0");
			}
		}
	}

	do {
		next_mode = seek_forward(gbdata, length, sp, current_mode);

		if(next_mode != current_mode) {
			switch(current_mode) {
				case 0:
				    switch(next_mode) {
					    case GM_CHINESE: strcat(binary, "0001");
						break;
					    case GM_NUMBER: strcat(binary, "0010");
						break;
					    case GM_LOWER: strcat(binary, "0011");
						break;
					    case GM_UPPER: strcat(binary, "0100");
						break;
					    case GM_MIXED: strcat(binary, "0101");
						break;
					    case GM_BYTE: strcat(binary, "0111");
						break;
				    }
				    break;
				case GM_CHINESE:
				    switch(next_mode) {
					    case GM_NUMBER: strcat(binary, "1111111100001");
						break; // 8161
					    case GM_LOWER: strcat(binary, "1111111100010");
						break; // 8162
					    case GM_UPPER: strcat(binary, "1111111100011");
						break; // 8163
					    case GM_MIXED: strcat(binary, "1111111100100");
						break; // 8164
					    case GM_BYTE: strcat(binary, "1111111100101");
						break; // 8165
				    }
				    break;
				case GM_NUMBER:
				    /* add numeric block padding value */
				    switch(p) {
					    case 1: binary[number_pad_posn] = '1';
						binary[number_pad_posn + 1] = '0';
						break; // 2 pad digits
					    case 2: binary[number_pad_posn] = '0';
						binary[number_pad_posn + 1] = '1';
						break; // 1 pad digit
					    case 3: binary[number_pad_posn] = '0';
						binary[number_pad_posn + 1] = '0';
						break; // 0 pad digits
				    }
				    switch(next_mode) {
					    case GM_CHINESE: strcat(binary, "1111111011");
						break; // 1019
					    case GM_LOWER: strcat(binary, "1111111100");
						break; // 1020
					    case GM_UPPER: strcat(binary, "1111111101");
						break; // 1021
					    case GM_MIXED: strcat(binary, "1111111110");
						break; // 1022
					    case GM_BYTE: strcat(binary, "1111111111");
						break; // 1023
				    }
				    break;
				case GM_LOWER:
				case GM_UPPER:
				    switch(next_mode) {
					    case GM_CHINESE: strcat(binary, "11100");
						break; // 28
					    case GM_NUMBER: strcat(binary, "11101");
						break; // 29
					    case GM_LOWER:
					    case GM_UPPER: strcat(binary, "11110");
						break; // 30
					    case GM_MIXED: strcat(binary, "1111100");
						break; // 124
					    case GM_BYTE: strcat(binary, "1111110");
						break; // 126
				    }
				    break;
				case GM_MIXED:
				    switch(next_mode) {
					    case GM_CHINESE: strcat(binary, "1111110001");
						break; // 1009
					    case GM_NUMBER: strcat(binary, "1111110010");
						break; // 1010
					    case GM_LOWER: strcat(binary, "1111110011");
						break; // 1011
					    case GM_UPPER: strcat(binary, "1111110100");
						break; // 1012
					    case GM_BYTE: strcat(binary, "1111110111");
						break; // 1015
				    }
				    break;
				case GM_BYTE:
				    /* add byte block length indicator */
				    add_byte_count(binary, byte_count_posn, byte_count);
				    byte_count = 0;
				    switch(next_mode) {
					    case GM_CHINESE: strcat(binary, "0001");
						break; // 1
					    case GM_NUMBER: strcat(binary, "0010");
						break; // 2
					    case GM_LOWER: strcat(binary, "0011");
						break; // 3
					    case GM_UPPER: strcat(binary, "0100");
						break; // 4
					    case GM_MIXED: strcat(binary, "0101");
						break; // 5
				    }
				    break;
			}
			if(debug) {
				switch(next_mode) {
					case GM_CHINESE: printf("CHIN ");
					    break;
					case GM_NUMBER: printf("NUMB ");
					    break;
					case GM_LOWER: printf("LOWR ");
					    break;
					case GM_UPPER: printf("UPPR ");
					    break;
					case GM_MIXED: printf("MIXD ");
					    break;
					case GM_BYTE: printf("BYTE ");
					    break;
				}
			}
		}
		last_mode = current_mode;
		current_mode = next_mode;

		switch(current_mode) {
			case GM_CHINESE:
			    done = 0;
			    if(gbdata[sp] > 0xff) {
				    /* GB2312 character */
				    c1 = (gbdata[sp] & 0xff00) >> 8;
				    c2 = gbdata[sp] & 0xff;

				    if((c1 >= 0xa0) && (c1 <= 0xa9)) {
					    glyph = (0x60 * (c1 - 0xa1)) + (c2 - 0xa0);
				    }
				    if((c1 >= 0xb0) && (c1 <= 0xf7)) {
					    glyph = (0x60 * (c1 - 0xb0 + 9)) + (c2 - 0xa0);
				    }
				    done = 1;
			    }
			    if(!(done)) {
				    if(sp != (length - 1)) {
					    if((gbdata[sp] == 0x13) && (gbdata[sp + 1] == 0x10)) {
						    /* End of Line */
						    glyph = 7776;
						    sp++;
					    }
					    done = 1;
				    }
			    }
			    if(!(done)) {
				    if(sp != (length - 1)) {
					    if(((gbdata[sp] >= '0') && (gbdata[sp] <= '9')) &&
					    ((gbdata[sp + 1] >= '0') && (gbdata[sp + 1] <= '9'))) {
						    /* Two digits */
						    glyph = 8033 + (10 * (gbdata[sp] - '0')) + (gbdata[sp + 1] - '0');
						    sp++;
					    }
				    }
			    }
			    if(!(done)) {
				    /* Byte value */
				    glyph = 7777 + gbdata[sp];
			    }
			    if(debug) {
				    printf("[%d] ", glyph);
			    }
			    for(q = 0; q < 13; q++) {
				    if(glyph & (0x1000 >> q)) {
					    strcat(binary, "1");
				    }
				    else {
					    strcat(binary, "0");
				    }
			    }
			    sp++;
			    break;

			case GM_NUMBER:
			    if(last_mode != current_mode) {
				    /* Reserve a space for numeric digit padding value (2 bits) */
				    number_pad_posn = strlen(binary);
				    strcat(binary, "XX");
			    }
			    p = 0;
			    ppos = -1;

			    /* Numeric compression can also include certain combinations of
			       non-numeric character */

			    numbuf[0] = '0';
			    numbuf[1] = '0';
			    numbuf[2] = '0';
			    do {
				    if((gbdata[sp] >= '0') && (gbdata[sp] <= '9')) {
					    numbuf[p] = gbdata[sp];
					    sp++;
					    p++;
				    }
				    switch(gbdata[sp]) {
					    case ' ':
					    case '+':
					    case '-':
					    case '.':
					    case ',':
						punt = gbdata[sp];
						sp++;
						ppos = p;
						break;
				    }
				    if(sp < (length - 1)) {
					    if((gbdata[sp] == 0x13) && (gbdata[sp + 1] == 0x10)) {
						    /* <end of line> */
						    punt = gbdata[sp];
						    sp += 2;
						    ppos = p;
					    }
				    }
			    } while((p < 3) && (sp < length));
			    if(ppos != -1) {
				    switch(punt) {
					    case ' ': glyph = 0; break;
					    case '+': glyph = 3; break;
					    case '-': glyph = 6; break;
					    case '.': glyph = 9; break;
					    case ',': glyph = 12; break;
					    case 0x13: glyph = 15; break;
				    }
				    glyph += ppos;
				    glyph += 1000;

				    if(debug) {
					    printf("[%d] ", glyph);
				    }

				    for(q = 0; q < 10; q++) {
					    if(glyph & (0x200 >> q)) {
						    strcat(binary, "1");
					    }
					    else {
						    strcat(binary, "0");
					    }
				    }
			    }

			    glyph = (100 * (numbuf[0] - '0')) + (10 * (numbuf[1] - '0')) + (numbuf[2] - '0');
			    if(debug) {
				    printf("[%d] ", glyph);
			    }

			    for(q = 0; q < 10; q++) {
				    if(glyph & (0x200 >> q)) {
					    strcat(binary, "1");
				    }
				    else {
					    strcat(binary, "0");
				    }
			    }
			    break;

			case GM_BYTE:
			    if(last_mode != current_mode) {
				    /* Reserve space for byte block length indicator (9 bits) */
				    byte_count_posn = strlen(binary);
				    strcat(binary, "LLLLLLLLL");
			    }
			    if(byte_count == 512) {
				    /* Maximum byte block size is 512 bytes. If longer is needed then start a new block */
				    add_byte_count(binary, byte_count_posn, byte_count);
				    strcat(binary, "0111");
				    byte_count_posn = strlen(binary);
				    strcat(binary, "LLLLLLLLL");
				    byte_count = 0;
			    }

			    glyph = gbdata[sp];
			    if(debug) {
				    printf("[%d] ", glyph);
			    }
			    for(q = 0; q < 8; q++) {
				    if(glyph & (0x80 >> q)) {
					    strcat(binary, "1");
				    }
				    else {
					    strcat(binary, "0");
				    }
			    }
			    sp++;
			    byte_count++;
			    break;

			case GM_MIXED:
			    shift = 1;
			    if((gbdata[sp] >= '0') && (gbdata[sp] <= '9')) {
				    shift = 0;
			    }
			    if((gbdata[sp] >= 'A') && (gbdata[sp] <= 'Z')) {
				    shift = 0;
			    }
			    if((gbdata[sp] >= 'a') && (gbdata[sp] <= 'z')) {
				    shift = 0;
			    }
			    if(gbdata[sp] == ' ') {
				    shift = 0;
			    }

			    if(shift == 0) {
				    /* Mixed Mode character */
				    glyph = posn(EUROPIUM, gbdata[sp]);
				    if(debug) {
					    printf("[%d] ", glyph);
				    }

				    for(q = 0; q < 6; q++) {
					    if(glyph & (0x20 >> q)) {
						    strcat(binary, "1");
					    }
					    else {
						    strcat(binary, "0");
					    }
				    }
			    }
			    else {
				    /* Shift Mode character */
				    strcat(binary, "1111110110"); /* 1014 - shift indicator */
				    add_shift_char(binary, gbdata[sp]);
			    }

			    sp++;
			    break;

			case GM_UPPER:
			    shift = 1;
			    if((gbdata[sp] >= 'A') && (gbdata[sp] <= 'Z')) {
				    shift = 0;
			    }
			    if(gbdata[sp] == ' ') {
				    shift = 0;
			    }

			    if(shift == 0) {
				    /* Upper Case character */
				    glyph = posn("ABCDEFGHIJKLMNOPQRSTUVWXYZ ", gbdata[sp]);
				    if(debug) {
					    printf("[%d] ", glyph);
				    }

				    for(q = 0; q < 5; q++) {
					    if(glyph & (0x10 >> q)) {
						    strcat(binary, "1");
					    }
					    else {
						    strcat(binary, "0");
					    }
				    }
			    }
			    else {
				    /* Shift Mode character */
				    strcat(binary, "1111101"); /* 127 - shift indicator */
				    add_shift_char(binary, gbdata[sp]);
			    }

			    sp++;
			    break;

			case GM_LOWER:
			    shift = 1;
			    if((gbdata[sp] >= 'a') && (gbdata[sp] <= 'z')) {
				    shift = 0;
			    }
			    if(gbdata[sp] == ' ') {
				    shift = 0;
			    }

			    if(shift == 0) {
				    /* Lower Case character */
				    glyph = posn("abcdefghijklmnopqrstuvwxyz ", gbdata[sp]);
				    if(debug) {
					    printf("[%d] ", glyph);
				    }

				    for(q = 0; q < 5; q++) {
					    if(glyph & (0x10 >> q)) {
						    strcat(binary, "1");
					    }
					    else {
						    strcat(binary, "0");
					    }
				    }
			    }
			    else {
				    /* Shift Mode character */
				    strcat(binary, "1111101"); /* 127 - shift indicator */
				    add_shift_char(binary, gbdata[sp]);
			    }

			    sp++;
			    break;
		}
		if(strlen(binary) > 9191) {
			return ZINT_ERROR_TOO_LONG;
		}
	} while(sp < length);
	if(current_mode == GM_NUMBER) {
		/* add numeric block padding value */
		switch(p) {
			case 1: 
				binary[number_pad_posn] = '1';
			    binary[number_pad_posn + 1] = '0';
			    break; // 2 pad digits
			case 2: 
				binary[number_pad_posn] = '0';
			    binary[number_pad_posn + 1] = '1';
			    break; // 1 pad digit
			case 3: 
				binary[number_pad_posn] = '0';
			    binary[number_pad_posn + 1] = '0';
			    break; // 0 pad digits
		}
	}
	if(current_mode == GM_BYTE) {
		/* Add byte block length indicator */
		add_byte_count(binary, byte_count_posn, byte_count);
	}

	/* Add "end of data" character */
	switch(current_mode) {
		case GM_CHINESE: strcat(binary, "1111111100000");
		    break; // 8160
		case GM_NUMBER: strcat(binary, "1111111010");
		    break; // 1018
		case GM_LOWER:
		case GM_UPPER: strcat(binary, "11011");
		    break; // 27
		case GM_MIXED: strcat(binary, "1111110000");
		    break; // 1008
		case GM_BYTE: strcat(binary, "0000");
		    break; // 0
	}

	/* Add padding bits if required */
	p = 7 - (strlen(binary) % 7);
	if(p == 7) {
		p = 0;
	}
	for(i = 0; i < p; i++) {
		strcat(binary, "0");
	}

	if(strlen(binary) > 9191) {
		return ZINT_ERROR_TOO_LONG;
	}
	return 0;
}

static void gm_add_ecc(const char binary[], const size_t data_posn, const int layers, const int ecc_level, int word[])
{
	int    i, j, wp, p;
	int    n1, b1, n2, b2, e1, b3, e2;
	int    block_size, data_size, ecc_size;
	int    data[1320], block[130];
	uchar  data_block[115], ecc_block[70];
	int    data_cw = gm_data_codewords[((layers - 1) * 5) + (ecc_level - 1)];
	for(i = 0; i < 1320; i++) {
		data[i] = 0;
	}
	// Convert from binary sream to 7-bit codewords 
	for(i = 0; i < (int)data_posn; i++) {
		for(p = 0; p < 7; p++) {
			if(binary[i * 7 + p] == '1') {
				data[i] += (0x40 >> p);
			}
		}
	}
	// Add padding codewords 
	data[data_posn] = 0x00;
	for(i = (int)(data_posn + 1); i < data_cw; i++) {
		if(i & 1) {
			data[i] = 0x7e;
		}
		else {
			data[i] = 0x00;
		}
	}
	// Get block sizes 
	n1 = gm_n1[(layers - 1)];
	b1 = gm_b1[(layers - 1)];
	n2 = n1 - 1;
	b2 = gm_b2[(layers - 1)];
	e1 = gm_ebeb[((layers - 1) * 20) + ((ecc_level - 1) * 4)];
	b3 = gm_ebeb[((layers - 1) * 20) + ((ecc_level - 1) * 4) + 1];
	e2 = gm_ebeb[((layers - 1) * 20) + ((ecc_level - 1) * 4) + 2];

	/* Split the data into blocks */
	wp = 0;
	for(i = 0; i < (b1 + b2); i++) {
		if(i < b1) {
			block_size = n1;
		}
		else {
			block_size = n2;
		}
		if(i < b3) {
			ecc_size = e1;
		}
		else {
			ecc_size = e2;
		}
		data_size = block_size - ecc_size;

		/* printf("block %d/%d: data %d / ecc %d\n", i + 1, (b1 + b2), data_size, ecc_size);*/

		for(j = 0; j < data_size; j++) {
			data_block[j] = data[wp];
			wp++;
		}

		/* Calculate ECC data for this block */
		rs_init_gf(0x89);
		rs_init_code(ecc_size, 1);
		rs_encode(data_size, data_block, ecc_block);
		rs_free();

		/* Correct error correction data but in reverse order */
		for(j = 0; j < data_size; j++) {
			block[j] = data_block[j];
		}
		for(j = 0; j < ecc_size; j++) {
			block[(j + data_size)] = ecc_block[ecc_size - j - 1];
		}

		for(j = 0; j < n2; j++) {
			word[((b1 + b2) * j) + i] = block[j];
		}
		if(block_size == n1) {
			word[((b1 + b2) * (n1 - 1)) + i] = block[(n1 - 1)];
		}
	}
}

void place_macromodule(char grid[], int x, int y, int word1, int word2, int size)
{
	int i, j;

	i = (x * 6) + 1;
	j = (y * 6) + 1;

	if(word2 & 0x40) {
		grid[(j * size) + i + 2] = '1';
	}
	if(word2 & 0x20) {
		grid[(j * size) + i + 3] = '1';
	}
	if(word2 & 0x10) {
		grid[((j + 1) * size) + i] = '1';
	}
	if(word2 & 0x08) {
		grid[((j + 1) * size) + i + 1] = '1';
	}
	if(word2 & 0x04) {
		grid[((j + 1) * size) + i + 2] = '1';
	}
	if(word2 & 0x02) {
		grid[((j + 1) * size) + i + 3] = '1';
	}
	if(word2 & 0x01) {
		grid[((j + 2) * size) + i] = '1';
	}
	if(word1 & 0x40) {
		grid[((j + 2) * size) + i + 1] = '1';
	}
	if(word1 & 0x20) {
		grid[((j + 2) * size) + i + 2] = '1';
	}
	if(word1 & 0x10) {
		grid[((j + 2) * size) + i + 3] = '1';
	}
	if(word1 & 0x08) {
		grid[((j + 3) * size) + i] = '1';
	}
	if(word1 & 0x04) {
		grid[((j + 3) * size) + i + 1] = '1';
	}
	if(word1 & 0x02) {
		grid[((j + 3) * size) + i + 2] = '1';
	}
	if(word1 & 0x01) {
		grid[((j + 3) * size) + i + 3] = '1';
	}
}

void place_data_in_grid(int word[], char grid[], int modules, int size)
{
	int x, y, macromodule, offset;

	offset = 13 - ((modules - 1) / 2);
	for(y = 0; y < modules; y++) {
		for(x = 0; x < modules; x++) {
			macromodule = gm_macro_matrix[((y + offset) * 27) + (x + offset)];
			place_macromodule(grid, x, y, word[macromodule * 2], word[(macromodule * 2) + 1], size);
		}
	}
}

/* Place the layer ID into each macromodule */
void place_layer_id(char* grid, int size, int layers, int modules, int ecc_level)
{
	int i, j, layer, start, stop;

#ifndef _MSC_VER
	int layerid[layers + 1];
	int id[modules * modules];
#else
	int* layerid = (int *)_alloca((layers + 1) * sizeof(int));
	int* id = (int *)_alloca((modules * modules) * sizeof(int));
#endif

	/* Calculate Layer IDs */
	for(i = 0; i <= layers; i++) {
		if(ecc_level == 1) {
			layerid[i] = 3 - (i % 4);
		}
		else {
			layerid[i] = (i + 5 - ecc_level) % 4;
		}
	}

	for(i = 0; i < modules; i++) {
		for(j = 0; j < modules; j++) {
			id[(i * modules) + j] = 0;
		}
	}

	/* Calculate which value goes in each macromodule */
	start = modules / 2;
	stop = modules / 2;
	for(layer = 0; layer <= layers; layer++) {
		for(i = start; i <= stop; i++) {
			id[(start * modules) + i] = layerid[layer];
			id[(i * modules) + start] = layerid[layer];
			id[((modules - start - 1) * modules) + i] = layerid[layer];
			id[(i * modules) + (modules - start - 1)] = layerid[layer];
		}
		start--;
		stop++;
	}

	/* Place the data in the grid */
	for(i = 0; i < modules; i++) {
		for(j = 0; j < modules; j++) {
			if(id[(i * modules) + j] & 0x02) {
				grid[(((i * 6) + 1) * size) + (j * 6) + 1] = '1';
			}
			if(id[(i * modules) + j] & 0x01) {
				grid[(((i * 6) + 1) * size) + (j * 6) + 2] = '1';
			}
		}
	}
}

int grid_matrix(struct ZintSymbol * symbol, const uchar source[], int length)
{
	int size, modules, dark, error_number;
	int auto_layers, min_layers, layers, auto_ecc_level, min_ecc_level, ecc_level;
	int x, y, i, j, glyph;
	char binary[9300];
	int data_cw, input_latch = 0;
	int word[1460], data_max, reader = 0;

#ifndef _MSC_VER
	int utfdata[length + 1];
	int gbdata[length + 1];
#else
	char* grid;
	int* utfdata = (int *)_alloca((length + 1) * sizeof(int));
	int* gbdata = (int *)_alloca((length + 1) * sizeof(int));
#endif

	for(i = 0; i < 1460; i++) {
		word[i] = 0;
	}
	if((symbol->input_mode == DATA_MODE) || (symbol->eci != 3)) {
		for(i = 0; i < length; i++) {
			gbdata[i] = (int)source[i];
		}
	}
	else {
		/* Convert Unicode input to GB-2312 */
		error_number = utf8toutf16(symbol, source, utfdata, &length);
		if(error_number != 0) {
			return error_number;
		}
		for(i = 0; i < length; i++) {
			if(utfdata[i] <= 0xff) {
				gbdata[i] = utfdata[i];
			}
			else {
				j = 0;
				glyph = 0;
				do {
					if(gb2312_lookup[j * 2] == utfdata[i]) {
						glyph = gb2312_lookup[(j * 2) + 1];
					}
					j++;
				} while((j < 7445) && (glyph == 0));
				if(glyph == 0) {
					sstrcpy(symbol->errtxt, "Invalid character in input data (E30)");
					return ZINT_ERROR_INVALID_DATA;
				}
				gbdata[i] = glyph;
			}
		}
	}

	if(symbol->output_options & READER_INIT) reader = 1;

	error_number = gm_encode(gbdata, length, binary, reader, symbol->eci);
	if(error_number != 0) {
		sstrcpy(symbol->errtxt, "Input data too long (E31)");
		return error_number;
	}

	/* Determine the size of the symbol */
	data_cw = strlen(binary) / 7;

	auto_layers = 13;
	for(i = 12; i > 0; i--) {
		if(gm_recommend_cw[(i - 1)] >= data_cw) {
			auto_layers = i;
		}
	}
	min_layers = 13;
	for(i = 12; i > 0; i--) {
		if(gm_max_cw[(i - 1)] >= data_cw) {
			min_layers = i;
		}
	}
	layers = auto_layers;
	auto_ecc_level = 3;
	if(layers == 1) {
		auto_ecc_level = 5;
	}
	if((layers == 2) || (layers == 3)) {
		auto_ecc_level = 4;
	}
	min_ecc_level = 1;
	if(layers == 1) {
		min_ecc_level = 4;
	}
	if((layers == 2) || (layers == 3)) {
		min_ecc_level = 2;
	}
	ecc_level = auto_ecc_level;

	if((symbol->option_2 >= 1) && (symbol->option_2 <= 13)) {
		input_latch = 1;
		if(symbol->option_2 > min_layers) {
			layers = symbol->option_2;
		}
		else {
			layers = min_layers;
		}
	}

	if(input_latch == 1) {
		auto_ecc_level = 3;
		if(layers == 1) {
			auto_ecc_level = 5;
		}
		if((layers == 2) || (layers == 3)) {
			auto_ecc_level = 4;
		}
		ecc_level = auto_ecc_level;
		if(data_cw > gm_data_codewords[(5 * (layers - 1)) + (ecc_level - 1)]) {
			layers++;
		}
	}
	if(input_latch == 0) {
		if((symbol->option_1 >= 1) && (symbol->option_1 <= 5)) {
			if(symbol->option_1 > min_ecc_level) {
				ecc_level = symbol->option_1;
			}
			else {
				ecc_level = min_ecc_level;
			}
		}
		if(data_cw > gm_data_codewords[(5 * (layers - 1)) + (ecc_level - 1)]) {
			do {
				layers++;
			} while((data_cw > gm_data_codewords[(5 * (layers - 1)) + (ecc_level - 1)]) && (layers <= 13));
		}
	}
	data_max = 1313;
	switch(ecc_level) {
		case 2: data_max = 1167; break;
		case 3: data_max = 1021; break;
		case 4: data_max = 875; break;
		case 5: data_max = 729; break;
	}

	if(data_cw > data_max) {
		sstrcpy(symbol->errtxt, "Input data too long (E32)");
		return ZINT_ERROR_TOO_LONG;
	}

	gm_add_ecc(binary, data_cw, layers, ecc_level, word);
	size = 6 + (layers * 12);
	modules = 1 + (layers * 2);

#ifndef _MSC_VER
	char grid[size * size];
#else
	grid = (char *)_alloca((size * size) * sizeof(char));
#endif
	for(x = 0; x < size; x++) {
		for(y = 0; y < size; y++) {
			grid[(y * size) + x] = '0';
		}
	}
	place_data_in_grid(word, grid, modules, size);
	place_layer_id(grid, size, layers, modules, ecc_level);
	/* Add macromodule frames */
	for(x = 0; x < modules; x++) {
		dark = 1 - (x & 1);
		for(y = 0; y < modules; y++) {
			if(dark == 1) {
				for(i = 0; i < 5; i++) {
					grid[((y * 6) * size) + (x * 6) + i] = '1';
					grid[(((y * 6) + 5) * size) + (x * 6) + i] = '1';
					grid[(((y * 6) + i) * size) + (x * 6)] = '1';
					grid[(((y * 6) + i) * size) + (x * 6) + 5] = '1';
				}
				grid[(((y * 6) + 5) * size) + (x * 6) + 5] = '1';
				dark = 0;
			}
			else {
				dark = 1;
			}
		}
	}
	/* Copy values to symbol */
	symbol->width = size;
	symbol->rows = size;
	for(x = 0; x < size; x++) {
		for(y = 0; y < size; y++) {
			if(grid[(y * size) + x] == '1') {
				set_module(symbol, y, x);
			}
		}
		symbol->row_height[x] = 1;
	}
	return 0;
}

