/* qr.c Handles QR Code */

/*
    libzint - the open source barcode library
    Copyright (C) 2009 -2016Robin Stuart <rstuart114@gmail.com>

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
#include "sjis.h"
//#include "qr.h"
//
// Data for QR Code
//
#define LEVEL_L	1
#define LEVEL_M	2
#define LEVEL_Q	3
#define LEVEL_H	4

#define RHODIUM "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:"

// From ISO/IEC 18004:2006 Table 7 
static const int16 qr_data_codewords_L[] = { // @sobolev int-->int16
    19, 34, 55, 80, 108, 136, 156, 194, 232, 274, 324, 370, 428, 461, 523, 589, 647,
    721, 795, 861, 932, 1006, 1094, 1174, 1276, 1370, 1468, 1531, 1631,
    1735, 1843, 1955, 2071, 2191, 2306, 2434, 2566, 2702, 2812, 2956
};

static const int16 qr_data_codewords_M[] = { // @sobolev int-->int16
    16, 28, 44, 64, 86, 108, 124, 154, 182, 216, 254, 290, 334, 365, 415, 453, 507,
    563, 627, 669, 714, 782, 860, 914, 1000, 1062, 1128, 1193, 1267,
    1373, 1455, 1541, 1631, 1725, 1812, 1914, 1992, 2102, 2216, 2334
};

static const int16 qr_data_codewords_Q[] = { // @sobolev int-->int16
    13, 22, 34, 48, 62, 76, 88, 110, 132, 154, 180, 206, 244, 261, 295, 325, 367,
    397, 445, 485, 512, 568, 614, 664, 718, 754, 808, 871, 911,
    985, 1033, 1115, 1171, 1231, 1286, 1354, 1426, 1502, 1582, 1666
};

static const int16 qr_data_codewords_H[] = { // @sobolev int-->int16
    9, 16, 26, 36, 46, 60, 66, 86, 100, 122, 140, 158, 180, 197, 223, 253, 283,
    313, 341, 385, 406, 442, 464, 514, 538, 596, 628, 661, 701,
    745, 793, 845, 901, 961, 986, 1054, 1096, 1142, 1222, 1276
};

static const int16 qr_total_codewords[] = { // @sobolev int-->int16
    26, 44, 70, 100, 134, 172, 196, 242, 292, 346, 404, 466, 532, 581, 655, 733, 815,
    901, 991, 1085, 1156, 1258, 1364, 1474, 1588, 1706, 1828, 1921, 2051,
    2185, 2323, 2465, 2611, 2761, 2876, 3034, 3196, 3362, 3532, 3706
};

static const int8 qr_blocks_L[] = { // @sobolev int-->int8
    1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 4, 4, 6, 6, 6, 6, 7, 8, 8, 9, 9, 10, 12, 12,
    12, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 24, 25
};

static const int8 qr_blocks_M[] = { // @sobolev int-->int8
    1, 1, 1, 2, 2, 4, 4, 4, 5, 5, 5, 8, 9, 9, 10, 10, 11, 13, 14, 16, 17, 17, 18, 20,
    21, 23, 25, 26, 28, 29, 31, 33, 35, 37, 38, 40, 43, 45, 47, 49
};

static const int8 qr_blocks_Q[] = { // @sobolev int-->int8
    1, 1, 2, 2, 4, 4, 6, 6, 8, 8, 8, 10, 12, 16, 12, 17, 16, 18, 21, 20, 23, 23, 25,
    27, 29, 34, 34, 35, 38, 40, 43, 45, 48, 51, 53, 56, 59, 62, 65, 68
};

static const int8 qr_blocks_H[] = { // @sobolev int-->int8
    1, 1, 2, 4, 4, 4, 5, 6, 8, 8, 11, 11, 16, 16, 18, 16, 19, 21, 25, 25, 25, 34, 30,
    32, 35, 37, 40, 42, 45, 48, 51, 54, 57, 60, 63, 66, 70, 74, 77, 81
};

static const int16 qr_sizes[] = { // @sobolev int-->int16
    21, 25, 29, 33, 37, 41, 45, 49, 53, 57, 61, 65, 69, 73, 77, 81, 85, 89, 93, 97,
    101, 105, 109, 113, 117, 121, 125, 129, 133, 137, 141, 145, 149, 153, 157, 161, 165, 169, 173, 177
};

static const int8 micro_qr_sizes[] = { // @sobolev int-->int8
    11, 13, 15, 17
};

static const int8 qr_align_loopsize[] = { // @sobolev int-->int8
    0, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7
};

static const int16 qr_table_e1[] = { // @sobolev int-->int16
    6, 18, 0, 0, 0, 0, 0,
    6, 22, 0, 0, 0, 0, 0,
    6, 26, 0, 0, 0, 0, 0,
    6, 30, 0, 0, 0, 0, 0,
    6, 34, 0, 0, 0, 0, 0,
    6, 22, 38, 0, 0, 0, 0,
    6, 24, 42, 0, 0, 0, 0,
    6, 26, 46, 0, 0, 0, 0,
    6, 28, 50, 0, 0, 0, 0,
    6, 30, 54, 0, 0, 0, 0,
    6, 32, 58, 0, 0, 0, 0,
    6, 34, 62, 0, 0, 0, 0,
    6, 26, 46, 66, 0, 0, 0,
    6, 26, 48, 70, 0, 0, 0,
    6, 26, 50, 74, 0, 0, 0,
    6, 30, 54, 78, 0, 0, 0,
    6, 30, 56, 82, 0, 0, 0,
    6, 30, 58, 86, 0, 0, 0,
    6, 34, 62, 90, 0, 0, 0,
    6, 28, 50, 72, 94, 0, 0,
    6, 26, 50, 74, 98, 0, 0,
    6, 30, 54, 78, 102, 0, 0,
    6, 28, 54, 80, 106, 0, 0,
    6, 32, 58, 84, 110, 0, 0,
    6, 30, 58, 86, 114, 0, 0,
    6, 34, 62, 90, 118, 0, 0,
    6, 26, 50, 74, 98, 122, 0,
    6, 30, 54, 78, 102, 126, 0,
    6, 26, 52, 78, 104, 130, 0,
    6, 30, 56, 82, 108, 134, 0,
    6, 34, 60, 86, 112, 138, 0,
    6, 30, 58, 86, 114, 142, 0,
    6, 34, 62, 90, 118, 146, 0,
    6, 30, 54, 78, 102, 126, 150,
    6, 24, 50, 76, 102, 128, 154,
    6, 28, 54, 80, 106, 132, 158,
    6, 32, 58, 84, 110, 136, 162,
    6, 26, 54, 82, 110, 138, 166,
    6, 30, 58, 86, 114, 142, 170
};

static const uint qr_annex_c[] = {
    // Format information bit sequences 
    0x5412, 0x5125, 0x5e7c, 0x5b4b, 0x45f9, 0x40ce, 0x4f97, 0x4aa0, 0x77c4, 0x72f3, 0x7daa, 0x789d,
    0x662f, 0x6318, 0x6c41, 0x6976, 0x1689, 0x13be, 0x1ce7, 0x19d0, 0x0762, 0x0255, 0x0d0c, 0x083b,
    0x355f, 0x3068, 0x3f31, 0x3a06, 0x24b4, 0x2183, 0x2eda, 0x2bed
};

static const long qr_annex_d[] = {
    // Version information bit sequences 
    0x07c94, 0x085bc, 0x09a99, 0x0a4d3, 0x0bbf6, 0x0c762, 0x0d847, 0x0e60d, 0x0f928, 0x10b78,
    0x1145d, 0x12a17, 0x13532, 0x149a6, 0x15683, 0x168c9, 0x177ec, 0x18ec4, 0x191e1, 0x1afab,
    0x1b08e, 0x1cc1a, 0x1d33f, 0x1ed75, 0x1f250, 0x209d5, 0x216f0, 0x228ba, 0x2379f, 0x24b0b,
    0x2542e, 0x26a64, 0x27541, 0x28c69
};

static const int qr_annex_c1[] = {
    // Micro QR Code format information 
    0x4445, 0x4172, 0x4e2b, 0x4b1c, 0x55ae, 0x5099, 0x5fc0, 0x5af7, 0x6793, 0x62a4, 0x6dfd, 0x68ca, 0x7678, 0x734f,
    0x7c16, 0x7921, 0x06de, 0x03e9, 0x0cb0, 0x0987, 0x1735, 0x1202, 0x1d5b, 0x186c, 0x2508, 0x203f, 0x2f66, 0x2a51, 0x34e3,
    0x31d4, 0x3e8d, 0x3bba
};
//
// Returns true if input glyph is in the Alphanumeric set 
//
static int in_alpha(int glyph)
{
	int retval = 0;
	char cglyph = (char)glyph;
	if((cglyph >= '0') && (cglyph <= '9')) {
		retval = 1;
	}
	if((cglyph >= 'A') && (cglyph <= 'Z')) {
		retval = 1;
	}
	switch(cglyph) {
		case ' ':
		case '$':
		case '%':
		case '*':
		case '+':
		case '-':
		case '.':
		case '/':
		case ':':
		    retval = 1;
		    break;
	}

	return retval;
}

static void define_mode(char mode[], int jisdata[], int length, int gs1)
{
	/* Values placed into mode[] are: K = Kanji, B = Binary, A = Alphanumeric, N = Numeric */
	int i, mlen, j;
	for(i = 0; i < length; i++) {
		if(jisdata[i] > 0xff) {
			mode[i] = 'K';
		}
		else {
			mode[i] = 'B';
			if(in_alpha(jisdata[i])) {
				mode[i] = 'A';
			}
			if(gs1 && (jisdata[i] == '[')) {
				mode[i] = 'A';
			}
			if((jisdata[i] >= '0') && (jisdata[i] <= '9')) {
				mode[i] = 'N';
			}
		}
	}
	/* If less than 6 numeric digits together then don't use numeric mode */
	for(i = 0; i < length; i++) {
		if(mode[i] == 'N') {
			if(((i != 0) && (mode[i - 1] != 'N')) || (i == 0)) {
				mlen = 0;
				while(((mlen + i) < length) && (mode[mlen + i] == 'N')) {
					mlen++;
				}
				;
				if(mlen < 6) {
					for(j = 0; j < mlen; j++) {
						mode[i + j] = 'A';
					}
				}
			}
		}
	}

	/* If less than 4 alphanumeric characters together then don't use alphanumeric mode */
	for(i = 0; i < length; i++) {
		if(mode[i] == 'A') {
			if(((i != 0) && (mode[i - 1] != 'A')) || (i == 0)) {
				mlen = 0;
				while(((mlen + i) < length) && (mode[mlen + i] == 'A')) {
					mlen++;
				}
				;
				if(mlen < 6) {
					for(j = 0; j < mlen; j++) {
						mode[i + j] = 'B';
					}
				}
			}
		}
	}
}
//
// Make an estimate (worst case scenario) of how long the binary string will be 
//
static int estimate_binary_length(char mode[], int length, int gs1, int eci)
{
	int i, count = 0;
	char current = 0;
	int a_count = 0;
	int n_count = 0;
	if(gs1) {
		count += 4;
	}
	if(eci != 3) {
		count += 12;
	}
	for(i = 0; i < length; i++) {
		if(mode[i] != current) {
			switch(mode[i]) {
				case 'K': count += 12 + 4; current = 'K'; break;
				case 'B': count += 16 + 4; current = 'B'; break;
				case 'A': count += 13 + 4; current = 'A'; a_count = 0; break; 
				case 'N': count += 14 + 4; current = 'N'; n_count = 0; break;
			}
		}
		switch(mode[i]) {
			case 'K': count += 13; break;
			case 'B': count += 8; break;
			case 'A':
			    a_count++;
			    if((a_count & 1) == 0) {
				    count += 5; // 11 in total
				    a_count = 0;
			    }
			    else
				    count += 6;
			    break;
			case 'N':
			    n_count++;
			    if((n_count % 3) == 0) {
				    count += 3; // 10 in total
				    n_count = 0;
			    }
			    else if((n_count & 1) == 0)
				    count += 3;  // 7 in total
			    else
				    count += 4;
			    break;
		}
	}
	return count;
}

static void qr_bscan(char * binary, int data, int h)
{
	for(; h; h >>= 1) {
		strcat(binary, data & h ? "1" : "0");
	}
}
//
// Convert input data to a binary stream and add padding 
//
static void qr_binary(int datastream[], int version, int target_binlen, char mode[], int jisdata[], int length, int gs1, int eci, int est_binlen)
{
	int position = 0, debug = 0;
	int short_data_block_length, i, scheme = 1;
	char data_block, padbits;
	int current_binlen, current_bytes;
	int toggle, percent;
#ifndef _MSC_VER
	char binary[est_binlen + 12];
#else
	char* binary = (char *)_alloca(est_binlen + 12);
#endif
	sstrcpy(binary, "");
	if(gs1) {
		strcat(binary, "0101"); /* FNC1 */
	}
	if(eci != 3) {
		strcat(binary, "0111"); /* ECI */
		qr_bscan(binary, eci, 0x80);
	}
	if(version <= 9) {
		scheme = 1;
	}
	else if((version >= 10) && (version <= 26)) {
		scheme = 2;
	}
	else if(version >= 27) {
		scheme = 3;
	}
	if(debug) {
		for(i = 0; i < length; i++) {
			printf("%c", mode[i]);
		}
		printf("\n");
	}
	percent = 0;
	do {
		data_block = mode[position];
		short_data_block_length = 0;
		do {
			short_data_block_length++;
		} while(((short_data_block_length + position) < length) && (mode[position + short_data_block_length] == data_block));
		switch(data_block) {
			case 'K':
			    /* Kanji mode */
			    /* Mode indicator */
			    strcat(binary, "1000");
			    /* Character count indicator */
			    qr_bscan(binary, short_data_block_length, 0x20 << (scheme * 2)); /* scheme = 1..3 */
			    if(debug) {
				    printf("Kanji block (length %d)\n\t", short_data_block_length);
			    }
			    /* Character representation */
			    for(i = 0; i < short_data_block_length; i++) {
				    int jis = jisdata[position + i];
				    int msb, lsb, prod;
				    if(jis > 0x9fff) {
					    jis -= 0xc140;
				    }
				    msb = (jis & 0xff00) >> 4;
				    lsb = (jis & 0xff);
				    prod = (msb * 0xc0) + lsb;
				    qr_bscan(binary, prod, 0x1000);
				    if(debug) {
					    printf("0x%4X ", prod);
				    }
			    }
			    if(debug) {
				    printf("\n");
			    }

			    break;
			case 'B':
			    /* Byte mode */
			    /* Mode indicator */
			    strcat(binary, "0100");

			    /* Character count indicator */
			    qr_bscan(binary, short_data_block_length, scheme > 1 ? 0x8000 : 0x80); /* scheme = 1..3 */

			    if(debug) {
				    printf("Byte block (length %d)\n\t", short_data_block_length);
			    }

			    /* Character representation */
			    for(i = 0; i < short_data_block_length; i++) {
				    int byte = jisdata[position + i];
				    if(gs1 && (byte == '[')) {
					    byte = 0x1d; /* FNC1 */
				    }
				    qr_bscan(binary, byte, 0x80);
				    if(debug) {
					    printf("0x%2X(%d) ", byte, byte);
				    }
			    }
			    if(debug) {
				    printf("\n");
			    }
			    break;
			case 'A':
			    /* Alphanumeric mode */
			    /* Mode indicator */
			    strcat(binary, "0010");
			    /* Character count indicator */
			    qr_bscan(binary, short_data_block_length, 0x40 << (2 * scheme)); /* scheme = 1..3 */
			    if(debug) {
				    printf("Alpha block (length %d)\n\t", short_data_block_length);
			    }
			    /* Character representation */
			    i = 0;
			    while(i < short_data_block_length) {
				    int count;
				    int first = 0, second = 0, prod;
				    if(percent == 0) {
					    if(gs1 && (jisdata[position + i] == '%')) {
						    first = posn(RHODIUM, '%');
						    second = posn(RHODIUM, '%');
						    count = 2;
						    prod = (first * 45) + second;
						    i++;
					    }
					    else {
						    if(gs1 && (jisdata[position + i] == '[')) {
							    first = posn(RHODIUM, '%'); /* FNC1 */
						    }
						    else {
							    first = posn(RHODIUM, (char)jisdata[position + i]);
						    }
						    count = 1;
						    i++;
						    prod = first;

						    if(i < short_data_block_length && mode[position + i] == 'A') {
							    if(gs1 && (jisdata[position + i] == '%')) {
								    second = posn(RHODIUM, '%');
								    count = 2;
								    prod = (first * 45) + second;
								    percent = 1;
							    }
							    else {
								    if(gs1 && (jisdata[position + i] == '[')) {
									    second = posn(RHODIUM, '%'); /* FNC1 */
								    }
								    else {
									    second = posn(RHODIUM, (char)jisdata[position + i]);
								    }
								    count = 2;
								    i++;
								    prod = (first * 45) + second;
							    }
						    }
					    }
				    }
				    else {
					    first = posn(RHODIUM, '%');
					    count = 1;
					    i++;
					    prod = first;
					    percent = 0;
					    if(i < short_data_block_length && mode[position + i] == 'A') {
						    if(gs1 && (jisdata[position + i] == '%')) {
							    second = posn(RHODIUM, '%');
							    count = 2;
							    prod = (first * 45) + second;
							    percent = 1;
						    }
						    else {
							    if(gs1 && (jisdata[position + i] == '[')) {
								    second = posn(RHODIUM, '%'); /* FNC1 */
							    }
							    else {
								    second = posn(RHODIUM, (char)jisdata[position + i]);
							    }
							    count = 2;
							    i++;
							    prod = (first * 45) + second;
						    }
					    }
				    }

				    qr_bscan(binary, prod, count == 2 ? 0x400 : 0x20); /* count = 1..2 */

				    if(debug) {
					    printf("0x%4X ", prod);
				    }
			    }
			    ;
			    if(debug) {
				    printf("\n");
			    }
			    break;
			case 'N':
			    /* Numeric mode */
			    /* Mode indicator */
			    strcat(binary, "0001");
			    /* Character count indicator */
			    qr_bscan(binary, short_data_block_length, 0x80 << (2 * scheme)); /* scheme = 1..3 */
			    if(debug) {
				    printf("Number block (length %d)\n\t", short_data_block_length);
			    }
			    /* Character representation */
			    i = 0;
			    while(i < short_data_block_length) {
				    int second = 0, third = 0;
				    int first = posn(NEON, (char)jisdata[position + i]);
				    int count = 1;
				    int prod = first;
				    if(i + 1 < short_data_block_length && mode[position + i + 1] == 'N') {
					    second = posn(NEON, (char)jisdata[position + i + 1]);
					    count = 2;
					    prod = (prod * 10) + second;
					    if(i + 2 < short_data_block_length && mode[position + i + 2] == 'N') {
						    third = posn(NEON, (char)jisdata[position + i + 2]);
						    count = 3;
						    prod = (prod * 10) + third;
					    }
				    }
				    qr_bscan(binary, prod, 1 << (3 * count)); /* count = 1..3 */
				    if(debug) {
					    printf("0x%4X (%d)", prod, prod);
				    }
				    i += count;
			    }
			    if(debug) {
				    printf("\n");
			    }
			    break;
		}
		position += short_data_block_length;
	} while(position < length);
	/* Terminator */
	strcat(binary, "0000");
	current_binlen = strlen(binary);
	padbits = 8 - (current_binlen % 8);
	if(padbits == 8) {
		padbits = 0;
	}
	current_bytes = (current_binlen + padbits) / 8;
	/* Padding bits */
	for(i = 0; i < padbits; i++) {
		strcat(binary, "0");
	}
	/* Put data into 8-bit codewords */
	for(i = 0; i < current_bytes; i++) {
		int p;
		datastream[i] = 0x00;
		for(p = 0; p < 8; p++) {
			if(binary[i * 8 + p] == '1') {
				datastream[i] += (0x80 >> p);
			}
		}
	}
	/* Add pad codewords */
	toggle = 0;
	for(i = current_bytes; i < target_binlen; i++) {
		if(toggle == 0) {
			datastream[i] = 0xec;
			toggle = 1;
		}
		else {
			datastream[i] = 0x11;
			toggle = 0;
		}
	}
	if(debug) {
		printf("Resulting codewords:\n\t");
		for(i = 0; i < target_binlen; i++) {
			printf("0x%2X ", datastream[i]);
		}
		printf("\n");
	}
}
//
// Split data into blocks, add error correction and then interleave the blocks and error correction data 
//
static void add_ecc(int fullstream[], int datastream[], int version, int data_cw, int blocks)
{
	int ecc_cw = qr_total_codewords[version - 1] - data_cw;
	int short_data_block_length = data_cw / blocks;
	int qty_long_blocks = data_cw % blocks;
	int qty_short_blocks = blocks - qty_long_blocks;
	int ecc_block_length = ecc_cw / blocks;
	int i, j, length_this_block, posn, debug = 0;
#ifndef _MSC_VER
	uchar data_block[short_data_block_length + 2];
	uchar ecc_block[ecc_block_length + 2];
	int interleaved_data[data_cw + 2];
	int interleaved_ecc[ecc_cw + 2];
#else
	uchar* data_block = (uchar *)_alloca(short_data_block_length + 2);
	uchar* ecc_block = (uchar *)_alloca(ecc_block_length + 2);
	int* interleaved_data = (int *)_alloca((data_cw + 2) * sizeof(int));
	int* interleaved_ecc = (int *)_alloca((ecc_cw + 2) * sizeof(int));
#endif
	posn = 0;
	for(i = 0; i < blocks; i++) {
		if(i < qty_short_blocks) {
			length_this_block = short_data_block_length;
		}
		else {
			length_this_block = short_data_block_length + 1;
		}

		for(j = 0; j < ecc_block_length; j++) {
			ecc_block[j] = 0;
		}
		for(j = 0; j < length_this_block; j++) {
			data_block[j] = (uchar)datastream[posn + j];
		}
		rs_init_gf(0x11d);
		rs_init_code(ecc_block_length, 0);
		rs_encode(length_this_block, data_block, ecc_block);
		rs_free();
		if(debug) {
			printf("Block %d: ", i + 1);
			for(j = 0; j < length_this_block; j++) {
				printf("%2X ", data_block[j]);
			}
			if(i < qty_short_blocks) {
				printf("   ");
			}
			printf(" // ");
			for(j = 0; j < ecc_block_length; j++) {
				printf("%2X ", ecc_block[ecc_block_length - j - 1]);
			}
			printf("\n");
		}

		for(j = 0; j < short_data_block_length; j++) {
			interleaved_data[(j * blocks) + i] = (int)data_block[j];
		}

		if(i >= qty_short_blocks) {
			interleaved_data[(short_data_block_length *
				    blocks) + (i - qty_short_blocks)] = (int)data_block[short_data_block_length];
		}
		for(j = 0; j < ecc_block_length; j++) {
			interleaved_ecc[(j * blocks) + i] = (int)ecc_block[ecc_block_length - j - 1];
		}
		posn += length_this_block;
	}
	for(j = 0; j < data_cw; j++) {
		fullstream[j] = interleaved_data[j];
	}
	for(j = 0; j < ecc_cw; j++) {
		fullstream[j + data_cw] = interleaved_ecc[j];
	}
	if(debug) {
		printf("\nData Stream: \n");
		for(j = 0; j < (data_cw + ecc_cw); j++) {
			printf("%2X ", fullstream[j]);
		}
		printf("\n");
	}
}

static void place_finder(uchar grid[], int size, int x, int y)
{
	const int8 finder[] = { // @sobolev int-->int8
		1, 1, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 0, 0, 1,
		1, 0, 1, 1, 1, 0, 1,
		1, 0, 1, 1, 1, 0, 1,
		1, 0, 1, 1, 1, 0, 1,
		1, 0, 0, 0, 0, 0, 1,
		1, 1, 1, 1, 1, 1, 1
	};
	for(int xp = 0; xp < 7; xp++) {
		for(int yp = 0; yp < 7; yp++) {
			if(finder[xp + (7 * yp)] == 1) {
				grid[((yp + y) * size) + (xp + x)] = 0x11;
			}
			else {
				grid[((yp + y) * size) + (xp + x)] = 0x10;
			}
		}
	}
}

static void place_align(uchar grid[], int size, int x, int y)
{
	static int8 alignment[] = { // @sobolev int-->int8
		1, 1, 1, 1, 1,
		1, 0, 0, 0, 1,
		1, 0, 1, 0, 1,
		1, 0, 0, 0, 1,
		1, 1, 1, 1, 1
	};
	x -= 2;
	y -= 2; // Input values represent centre of pattern 
	for(int xp = 0; xp < 5; xp++) {
		for(int yp = 0; yp < 5; yp++) {
			if(alignment[xp + (5 * yp)] == 1) {
				grid[((yp + y) * size) + (xp + x)] = 0x11;
			}
			else {
				grid[((yp + y) * size) + (xp + x)] = 0x10;
			}
		}
	}
}

static void setup_grid(uchar* grid, int size, int version)
{
	int i, toggle = 1;
	int loopsize, x, y, xcoord, ycoord;
	// Add timing patterns 
	for(i = 0; i < size; i++) {
		if(toggle == 1) {
			grid[(6 * size) + i] = 0x21;
			grid[(i * size) + 6] = 0x21;
			toggle = 0;
		}
		else {
			grid[(6 * size) + i] = 0x20;
			grid[(i * size) + 6] = 0x20;
			toggle = 1;
		}
	}
	// Add finder patterns 
	place_finder(grid, size, 0, 0);
	place_finder(grid, size, 0, size - 7);
	place_finder(grid, size, size - 7, 0);
	// Add separators 
	for(i = 0; i < 7; i++) {
		grid[(7 * size) + i] = 0x10;
		grid[(i * size) + 7] = 0x10;
		grid[(7 * size) + (size - 1 - i)] = 0x10;
		grid[(i * size) + (size - 8)] = 0x10;
		grid[((size - 8) * size) + i] = 0x10;
		grid[((size - 1 - i) * size) + 7] = 0x10;
	}
	grid[(7 * size) + 7] = 0x10;
	grid[(7 * size) + (size - 8)] = 0x10;
	grid[((size - 8) * size) + 7] = 0x10;

	/* Add alignment patterns */
	if(version != 1) {
		/* Version 1 does not have alignment patterns */

		loopsize = qr_align_loopsize[version - 1];
		for(x = 0; x < loopsize; x++) {
			for(y = 0; y < loopsize; y++) {
				xcoord = qr_table_e1[((version - 2) * 7) + x];
				ycoord = qr_table_e1[((version - 2) * 7) + y];

				if(!(grid[(ycoord * size) + xcoord] & 0x10)) {
					place_align(grid, size, xcoord, ycoord);
				}
			}
		}
	}

	/* Reserve space for format information */
	for(i = 0; i < 8; i++) {
		grid[(8 * size) + i] += 0x20;
		grid[(i * size) + 8] += 0x20;
		grid[(8 * size) + (size - 1 - i)] = 0x20;
		grid[((size - 1 - i) * size) + 8] = 0x20;
	}
	grid[(8 * size) + 8] += 20;
	grid[((size - 1 - 7) * size) + 8] = 0x21; /* Dark Module from Figure 25 */

	/* Reserve space for version information */
	if(version >= 7) {
		for(i = 0; i < 6; i++) {
			grid[((size - 9) * size) + i] = 0x20;
			grid[((size - 10) * size) + i] = 0x20;
			grid[((size - 11) * size) + i] = 0x20;
			grid[(i * size) + (size - 9)] = 0x20;
			grid[(i * size) + (size - 10)] = 0x20;
			grid[(i * size) + (size - 11)] = 0x20;
		}
	}
}

static int FASTCALL cwbit(int * datastream, int i)
{
	int resultant = 0;
	if(datastream[(i / 8)] & (0x80 >> (i % 8))) {
		resultant = 1;
	}
	return resultant;
}

static void populate_grid(uchar* grid, int size, int* datastream, int cw)
{
	int direction = 1; /* up */
	int row = 0; /* right hand side */
	int x;
	int n = cw * 8;
	int y = size - 1;
	int i = 0;
	do {
		x = (size - 2) - (row * 2);
		if(x < 6)
			x--; // skip over vertical timing pattern 
		if(!(grid[(y * size) + (x + 1)] & 0xf0)) {
			if(cwbit(datastream, i)) {
				grid[(y * size) + (x + 1)] = 0x01;
			}
			else {
				grid[(y * size) + (x + 1)] = 0x00;
			}
			i++;
		}
		if(i < n) {
			if(!(grid[(y * size) + x] & 0xf0)) {
				if(cwbit(datastream, i)) {
					grid[(y * size) + x] = 0x01;
				}
				else {
					grid[(y * size) + x] = 0x00;
				}
				i++;
			}
		}
		if(direction) {
			y--;
		}
		else {
			y++;
		}
		if(y == -1) {
			// reached the top 
			row++;
			y = 0;
			direction = 0;
		}
		if(y == size) {
			/* reached the bottom */
			row++;
			y = size - 1;
			direction = 1;
		}
	} while(i < n);
}

#ifdef ZINTLOG

int append_log(char log)
{
	FILE * file = fopen("zintlog.txt", "a+");
	fprintf(file, "%c", log);
	fclose(file);
	return 0;
}

int write_log(char log[])
{
	FILE * file = fopen("zintlog.txt", "a+");
	fprintf(file, log); /*writes*/
	fprintf(file, "\r\n"); /*writes*/
	fclose(file);
	return 0;
}

#endif

static int evaluate(uchar * eval, int size, int pattern)
{
	int x, y, block, weight;
	int result = 0;
	char state;
	int p;
	int dark_mods;
	int percentage, k;
	int a, b, afterCount, beforeCount;
#ifdef ZINTLOG
	int result_b = 0;
	char str[15];
#endif

#ifndef _MSC_VER
	char local[size * size];
#else
	char* local = (char *)_alloca((size * size) * sizeof(char));
#endif

#ifdef ZINTLOG
	write_log("");
	sprintf(str, "%d", pattern);
	write_log(str);
#endif

	/* all eight bitmask variants have been encoded in the 8 bits of the bytes
	 * that make up the grid array. select them for evaluation according to the
	 * desired pattern.*/
	for(x = 0; x < size; x++) {
		for(y = 0; y < size; y++) {
			if((eval[(y * size) + x] & (0x01 << pattern)) != 0) {
				local[(y * size) + x] = '1';
			}
			else {
				local[(y * size) + x] = '0';
			}
		}
	}

#ifdef ZINTLOG
	//bitmask output
	for(y = 0; y < size; y++) {
		sstrcpy(str, "");
		for(x = 0; x < size; x++) {
			state = local[(y * size) + x];
			append_log(state);
		}
		write_log("");
	}
	write_log("");
#endif

	/* Test 1: Adjacent modules in row/column in same colour */
	/* Vertical */
	for(x = 0; x < size; x++) {
		state = local[x];
		block = 0;
		for(y = 0; y < size; y++) {
			if(local[(y * size) + x] == state) {
				block++;
			}
			else {
				if(block > 5) {
					result += (3 + (block - 5));
				}
				block = 0;
				state = local[(y * size) + x];
			}
		}
		if(block > 5) {
			result += (3 + (block - 5));
		}
	}

	/* Horizontal */
	for(y = 0; y < size; y++) {
		state = local[y * size];
		block = 0;
		for(x = 0; x < size; x++) {
			if(local[(y * size) + x] == state) {
				block++;
			}
			else {
				if(block > 5) {
					result += (3 + (block - 5));
				}
				block = 0;
				state = local[(y * size) + x];
			}
		}
		if(block > 5) {
			result += (3 + (block - 5));
		}
	}

#ifdef ZINTLOG
	/* output Test 1 */
	sprintf(str, "%d", result);
	result_b = result;
	write_log(str);
#endif

	/* Test 2: Block of modules in same color */
	for(x = 0; x < size - 1; x++) {
		for(y = 0; y < size - 1; y++) {
			if(((local[(y * size) + x] == local[((y + 1) * size) + x]) &&
				    (local[(y * size) + x] == local[(y * size) + (x + 1)])) &&
			    (local[(y * size) + x] == local[((y + 1) * size) + (x + 1)])) {
				result += 3;
			}
		}
	}

#ifdef ZINTLOG
	/* output Test 2 */
	sprintf(str, "%d", result - result_b);
	result_b = result;
	write_log(str);
#endif

	/* Test 3: 1:1:3:1:1 ratio pattern in row/column */
	/* Vertical */
	for(x = 0; x < size; x++) {
		for(y = 0; y < (size - 7); y++) {
			p = 0;
			for(weight = 0; weight < 7; weight++) {
				if(local[((y + weight) * size) + x] == '1') {
					p += (0x40 >> weight);
				}
			}
			if(p == 0x5d) {
				/* Pattern found, check before and after */
				beforeCount = 0;
				for(b = (y - 4); b < y; b++) {
					if(b < 0) {
						beforeCount++;
					}
					else {
						if(local[(b * size) + x] == '0') {
							beforeCount++;
						}
						else {
							beforeCount = 0;
						}
					}
				}

				afterCount = 0;
				for(a = (y + 7); a <= (y + 10); a++) {
					if(a >= size) {
						afterCount++;
					}
					else {
						if(local[(a * size) + x] == '0') {
							afterCount++;
						}
						else {
							afterCount = 0;
						}
					}
				}

				if((beforeCount == 4) || (afterCount == 4)) {
					/* Pattern is preceeded or followed by light area
					   4 modules wide */
					result += 40;
				}
			}
		}
	}

	/* Horizontal */
	for(y = 0; y < size; y++) {
		for(x = 0; x < (size - 7); x++) {
			p = 0;
			for(weight = 0; weight < 7; weight++) {
				if(local[(y * size) + x + weight] == '1') {
					p += (0x40 >> weight);
				}
			}
			if(p == 0x5d) {
				/* Pattern found, check before and after */
				beforeCount = 0;
				for(b = (x - 4); b < x; b++) {
					if(b < 0) {
						beforeCount++;
					}
					else {
						if(local[(y * size) + b] == '0') {
							beforeCount++;
						}
						else {
							beforeCount = 0;
						}
					}
				}

				afterCount = 0;
				for(a = (x + 7); a <= (x + 10); a++) {
					if(a >= size) {
						afterCount++;
					}
					else {
						if(local[(y * size) + a] == '0') {
							afterCount++;
						}
						else {
							afterCount = 0;
						}
					}
				}

				if((beforeCount == 4) || (afterCount == 4)) {
					/* Pattern is preceeded or followed by light area
					   4 modules wide */
					result += 40;
				}
			}
		}
	}

#ifdef ZINTLOG
	/* output Test 3 */
	sprintf(str, "%d", result - result_b);
	result_b = result;
	write_log(str);
#endif

	/* Test 4: Proportion of dark modules in entire symbol */
	dark_mods = 0;
	for(x = 0; x < size; x++) {
		for(y = 0; y < size; y++) {
			if(local[(y * size) + x] == '1') {
				dark_mods++;
			}
		}
	}
	percentage = 100 * (dark_mods / (size * size));
	if(percentage <= 50) {
		k = ((100 - percentage) - 50) / 5;
	}
	else {
		k = (percentage - 50) / 5;
	}

	result += 10 * k;

#ifdef ZINTLOG
	/* output Test 4+summary */
	sprintf(str, "%d", result - result_b);
	write_log(str);
	write_log("==========");
	sprintf(str, "%d", result);
	write_log(str);
#endif

	return result;
}

static void add_format_info_eval(uchar * eval, int size, int ecc_level, int pattern)
{
	// Add format information to grid 
	int format = pattern;
	uint seq;
	int i;
	switch(ecc_level) {
		case LEVEL_L: format += 0x08; break;
		case LEVEL_Q: format += 0x18; break;
		case LEVEL_H: format += 0x10; break;
	}
	seq = qr_annex_c[format];
	for(i = 0; i < 6; i++) {
		eval[(i * size) + 8] = (seq >> i) & 0x01 ? (0x01 >> pattern) : 0x00;
	}
	for(i = 0; i < 8; i++) {
		eval[(8 * size) + (size - i - 1)] = (seq >> i) & 0x01 ? (0x01 >> pattern) : 0x00;
	}
	for(i = 0; i < 6; i++) {
		eval[(8 * size) + (5 - i)] = (seq >> (i + 9)) & 0x01 ? (0x01 >> pattern) : 0x00;
	}
	for(i = 0; i < 7; i++) {
		eval[(((size - 7) + i) * size) + 8] = (seq >> (i + 8)) & 0x01 ? (0x01 >> pattern) : 0x00;
	}
	eval[(7 * size) + 8] = (seq >> 6) & 0x01 ? (0x01 >> pattern) : 0x00;
	eval[(8 * size) + 8] = (seq >> 7) & 0x01 ? (0x01 >> pattern) : 0x00;
	eval[(8 * size) + 7] = (seq >> 8) & 0x01 ? (0x01 >> pattern) : 0x00;
}

static int apply_bitmask(uchar * grid, int size, int ecc_level)
{
	int x, y;
	uchar p;
	int pattern, penalty[8];
	int best_val, best_pattern;
#ifndef _MSC_VER
	uchar mask[size * size];
	uchar eval[size * size];
#else
	uchar* mask = (uchar *)_alloca((size * size) * sizeof(uchar));
	uchar* eval = (uchar *)_alloca((size * size) * sizeof(uchar));
#endif
	// Perform data masking 
	for(x = 0; x < size; x++) {
		for(y = 0; y < size; y++) {
			mask[(y * size) + x] = 0x00;
			// all eight bitmask variants are encoded in the 8 bits of the bytes that make up the mask array.
			if(!(grid[(y * size) + x] & 0xf0)) { // exclude areas not to be masked.
				if(((y + x) & 1) == 0) {
					mask[(y * size) + x] += 0x01;
				}
				if((y & 1) == 0) {
					mask[(y * size) + x] += 0x02;
				}
				if((x % 3) == 0) {
					mask[(y * size) + x] += 0x04;
				}
				if(((y + x) % 3) == 0) {
					mask[(y * size) + x] += 0x08;
				}
				if((((y / 2) + (x / 3)) & 1) == 0) {
					mask[(y * size) + x] += 0x10;
				}
				if((((y * x) & 1) + ((y * x) % 3)) == 0) {
					mask[(y * size) + x] += 0x20;
				}
				if(((((y * x) & 1) + ((y * x) % 3)) & 1) == 0) {
					mask[(y * size) + x] += 0x40;
				}
				if(((((y + x) & 1) + ((y * x) % 3)) & 1) == 0) {
					mask[(y * size) + x] += 0x80;
				}
			}
		}
	}
	// apply data masks to grid, result in eval
	for(x = 0; x < size; x++) {
		for(y = 0; y < size; y++) {
			if(grid[(y * size) + x] & 0x01) {
				p = 0xff;
			}
			else {
				p = 0x00;
			}
			eval[(y * size) + x] = mask[(y * size) + x] ^ p;
		}
	}
	// Evaluate result 
	for(pattern = 0; pattern < 8; pattern++) {
		add_format_info_eval(eval, size, ecc_level, pattern);
		penalty[pattern] = evaluate(eval, size, pattern);
	}
	best_pattern = 0;
	best_val = penalty[0];
	for(pattern = 1; pattern < 8; pattern++) {
		if(penalty[pattern] < best_val) {
			best_pattern = pattern;
			best_val = penalty[pattern];
		}
	}
#ifdef ZINTLOG
	char str[15];
	sprintf(str, "%d", best_val);
	write_log("choosed pattern:");
	write_log(str);
#endif
	// Apply mask 
	for(x = 0; x < size; x++) {
		for(y = 0; y < size; y++) {
			if(mask[(y * size) + x] & (0x01 << best_pattern)) {
				if(grid[(y * size) + x] & 0x01) {
					grid[(y * size) + x] = 0x00;
				}
				else {
					grid[(y * size) + x] = 0x01;
				}
			}
		}
	}
	return best_pattern;
}
//
// Add format information to grid 
//
static void add_format_info(uchar * grid, int size, int ecc_level, int pattern)
{
	int format = pattern;
	switch(ecc_level) {
		case LEVEL_L: format += 0x08; break;
		case LEVEL_Q: format += 0x18; break;
		case LEVEL_H: format += 0x10; break;
	}
	{
		const  uint  seq = qr_annex_c[format];
		int    i;
		for(i = 0; i < 6; i++) {
			grid[(i * size) + 8] += (seq >> i) & 0x01;
		}
		for(i = 0; i < 8; i++) {
			grid[(8 * size) + (size - i - 1)] += (seq >> i) & 0x01;
		}
		for(i = 0; i < 6; i++) {
			grid[(8 * size) + (5 - i)] += (seq >> (i + 9)) & 0x01;
		}
		for(i = 0; i < 7; i++) {
			grid[(((size - 7) + i) * size) + 8] += (seq >> (i + 8)) & 0x01;
		}
		grid[(7 * size) + 8] += (seq >> 6) & 0x01;
		grid[(8 * size) + 8] += (seq >> 7) & 0x01;
		grid[(8 * size) + 7] += (seq >> 8) & 0x01;
	}
}
//
// Add version information 
//
static void add_version_info(uchar * grid, int size, int version)
{
	long int version_data = qr_annex_d[version - 7];
	for(int i = 0; i < 6; i++) {
		grid[((size - 11) * size) + i] += (uchar)((version_data >> (i * 3)) & 0x41);
		grid[((size - 10) * size) + i] += (uchar)((version_data >> ((i * 3) + 1)) & 0x41);
		grid[((size - 9) * size) + i] += (uchar)((version_data >> ((i * 3) + 2)) & 0x41);
		grid[(i * size) + (size - 11)] += (uchar)((version_data >> (i * 3)) & 0x41);
		grid[(i * size) + (size - 10)] += (uchar)((version_data >> ((i * 3) + 1)) & 0x41);
		grid[(i * size) + (size - 9)] += (uchar)((version_data >> ((i * 3) + 2)) & 0x41);
	}
}
//
// Choose from three numbers based on version 
//
static int tribus(int version, int a, int b, int c)
{
	int RetVal = c;
	if(version < 10) {
		RetVal = a;
	}
	if((version >= 10) && (version <= 26)) {
		RetVal = b;
	}
	return RetVal;
}
//
// Implements a custom optimisation algorithm, more efficient than that given in Annex J. 
//
static void applyOptimisation(int version, char inputMode[], int inputLength)
{
	int blockCount = 0, block;
	int i, j;
	char currentMode = ' '; // Null
	int * p_block_length;
	char * blockMode;
	for(i = 0; i < inputLength; i++) {
		if(inputMode[i] != currentMode) {
			currentMode = inputMode[i];
			blockCount++;
		}
	}
	p_block_length = (int *)SAlloc::M(sizeof(int)*blockCount);
	assert(p_block_length);
	if(!p_block_length) 
		return;
	blockMode = (char *)SAlloc::M(sizeof(char)*blockCount);
	assert(blockMode);
	if(!blockMode) {
		SAlloc::F(p_block_length);
		return;
	}
	j = -1;
	currentMode = ' '; // Null
	for(i = 0; i < inputLength; i++) {
		if(inputMode[i] != currentMode) {
			j++;
			p_block_length[j] = 1;
			blockMode[j] = inputMode[i];
			currentMode = inputMode[i];
		}
		else {
			p_block_length[j]++;
		}
	}
	if(blockCount > 1) {
		// Search forward
		for(i = 0; i <= (blockCount - 2); i++) {
			if(blockMode[i] == 'B') {
				switch(blockMode[i + 1]) {
					case 'K':
					    if(p_block_length[i + 1] < tribus(version, 4, 5, 6)) {
						    blockMode[i + 1] = 'B';
					    }
					    break;
					case 'A':
					    if(p_block_length[i + 1] < tribus(version, 7, 8, 9)) {
						    blockMode[i + 1] = 'B';
					    }
					    break;
					case 'N':
					    if(p_block_length[i + 1] < tribus(version, 3, 4, 5)) {
						    blockMode[i + 1] = 'B';
					    }
					    break;
				}
			}
			if((blockMode[i] == 'A') && (blockMode[i + 1] == 'N')) {
				if(p_block_length[i + 1] < tribus(version, 6, 8, 10)) {
					blockMode[i + 1] = 'A';
				}
			}
		}
		// Search backward
		for(i = blockCount - 1; i > 0; i--) {
			if(blockMode[i] == 'B') {
				switch(blockMode[i - 1]) {
					case 'K':
					    if(p_block_length[i - 1] < tribus(version, 4, 5, 6)) {
						    blockMode[i - 1] = 'B';
					    }
					    break;
					case 'A':
					    if(p_block_length[i - 1] < tribus(version, 7, 8, 9)) {
						    blockMode[i - 1] = 'B';
					    }
					    break;
					case 'N':
					    if(p_block_length[i - 1] < tribus(version, 3, 4, 5)) {
						    blockMode[i - 1] = 'B';
					    }
					    break;
				}
			}

			if((blockMode[i] == 'A') && (blockMode[i - 1] == 'N')) {
				if(p_block_length[i - 1] < tribus(version, 6, 8, 10)) {
					blockMode[i - 1] = 'A';
				}
			}
		}
	}
	j = 0;
	for(block = 0; block < blockCount; block++) {
		currentMode = blockMode[block];
		for(i = 0; i < p_block_length[block]; i++) {
			inputMode[j] = currentMode;
			j++;
		}
	}
	SAlloc::F(p_block_length);
	SAlloc::F(blockMode);
}

static int blockLength(int start, char inputMode[], int inputLength)
{
	// Find the length of the block starting from 'start' 
	char mode = inputMode[start];
	int count = 0;
	int i = start;
	do {
		count++;
	} while(((i + count) < inputLength) && (inputMode[i + count] == mode));
	return count;
}

static int getBinaryLength(int version, char inputMode[], int inputData[], int inputLength, int gs1, int eci)
{
	// Calculate the actual bitlength of the proposed binary string 
	char currentMode;
	int i, j;
	int count = 0;
	applyOptimisation(version, inputMode, inputLength);
	currentMode = ' '; // Null
	if(gs1 == 1) {
		count += 4;
	}
	if(eci != 3) {
		count += 12;
	}
	for(i = 0; i < inputLength; i++) {
		if(inputMode[i] != currentMode) {
			count += 4;
			switch(inputMode[i]) {
				case 'K':
				    count += tribus(version, 8, 10, 12);
				    count += (blockLength(i, inputMode, inputLength) * 13);
				    break;
				case 'B':
				    count += tribus(version, 8, 16, 16);
				    for(j = i; j < (i + blockLength(i, inputMode, inputLength)); j++) {
					    if(inputData[j] > 0xff) {
						    count += 16;
					    }
					    else {
						    count += 8;
					    }
				    }
				    break;
				case 'A':
				    count += tribus(version, 9, 11, 13);
				    switch(blockLength(i, inputMode, inputLength) % 2) {
					    case 0: count += (blockLength(i, inputMode, inputLength) / 2) * 11; break;
					    case 1: count += ((blockLength(i, inputMode, inputLength) - 1) / 2) * 11; count += 6; break;
				    }
				    break;
				case 'N':
				    count += tribus(version, 10, 12, 14);
				    switch(blockLength(i, inputMode, inputLength) % 3) {
					    case 0: count += (blockLength(i, inputMode, inputLength) / 3) * 10; break;
					    case 1: count += ((blockLength(i, inputMode, inputLength) - 1) / 3) * 10; count += 4; break;
					    case 2: count += ((blockLength(i, inputMode, inputLength) - 2) / 3) * 10; count += 7; break;
				    }
				    break;
			}
			currentMode = inputMode[i];
		}
	}

	return count;
}

int qr_code(struct ZintSymbol * symbol, const uchar source[], int length)
{
	int error_number, i, j, glyph, est_binlen;
	int ecc_level, autosize, version, max_cw, target_binlen, blocks, size;
	int bitmask, gs1;
	int canShrink;

#ifndef _MSC_VER
	int utfdata[length + 1];
	int jisdata[length + 1];
	char mode[length + 1];
#else
	int* datastream;
	int* fullstream;
	uchar* grid;
	int* utfdata = (int *)_alloca((length + 1) * sizeof(int));
	int* jisdata = (int *)_alloca((length + 1) * sizeof(int));
	char* mode = (char *)_alloca(length + 1);
#endif
	gs1 = (symbol->input_mode == GS1_MODE);
	if((symbol->input_mode == DATA_MODE) || (symbol->eci != 3)) {
		for(i = 0; i < length; i++) {
			jisdata[i] = (int)source[i];
		}
	}
	else {
		/* Convert Unicode input to Shift-JIS */
		error_number = utf8toutf16(symbol, source, utfdata, &length);
		if(error_number != 0) {
			return error_number;
		}
		for(i = 0; i < length; i++) {
			if(utfdata[i] <= 0xff) {
				jisdata[i] = utfdata[i];
			}
			else {
				j = 0;
				glyph = 0;
				do {
					if(sjis_lookup[j * 2] == utfdata[i]) {
						glyph = sjis_lookup[(j * 2) + 1];
					}
					j++;
				} while((j < 6843) && (glyph == 0));
				if(glyph == 0) {
					sstrcpy(symbol->errtxt, "Invalid character in input data (E60)");
					return ZINT_ERROR_INVALID_DATA;
				}
				jisdata[i] = glyph;
			}
		}
	}
	define_mode(mode, jisdata, length, gs1);
	est_binlen = estimate_binary_length(mode, length, gs1, symbol->eci);
	ecc_level = LEVEL_L;
	max_cw = 2956;
	if((symbol->option_1 >= 1) && (symbol->option_1 <= 4)) {
		switch(symbol->option_1) {
			case 1: ecc_level = LEVEL_L; max_cw = 2956; break;
			case 2: ecc_level = LEVEL_M; max_cw = 2334; break;
			case 3: ecc_level = LEVEL_Q; max_cw = 1666; break;
			case 4: ecc_level = LEVEL_H; max_cw = 1276; break;
		}
	}
	if(est_binlen > (8 * max_cw)) {
		sstrcpy(symbol->errtxt, "Input too long for selected error correction level (E61)");
		return ZINT_ERROR_TOO_LONG;
	}
	autosize = 40;
	for(i = 39; i >= 0; i--) {
		switch(ecc_level) {
			case LEVEL_L:
			    if((8 * qr_data_codewords_L[i]) >= est_binlen) {
				    autosize = i + 1;
			    }
			    break;
			case LEVEL_M:
			    if((8 * qr_data_codewords_M[i]) >= est_binlen) {
				    autosize = i + 1;
			    }
			    break;
			case LEVEL_Q:
			    if((8 * qr_data_codewords_Q[i]) >= est_binlen) {
				    autosize = i + 1;
			    }
			    break;
			case LEVEL_H:
			    if((8 * qr_data_codewords_H[i]) >= est_binlen) {
				    autosize = i + 1;
			    }
			    break;
		}
	}
	// Now see if the optimised binary will fit in a smaller symbol.
	canShrink = 1;
	do {
		if(autosize == 1) {
			canShrink = 0;
		}
		else {
			if(tribus(autosize - 1, 1, 2, 3) != tribus(autosize, 1, 2, 3)) {
				// Length of binary needed to encode the data in the smaller symbol is different, recalculate
				est_binlen = getBinaryLength(autosize - 1, mode, jisdata, length, gs1, symbol->eci);
			}
			switch(ecc_level) {
				case LEVEL_L:
				    if((8 * qr_data_codewords_L[autosize - 2]) < est_binlen) {
					    canShrink = 0;
				    }
				    break;
				case LEVEL_M:
				    if((8 * qr_data_codewords_M[autosize - 2]) < est_binlen) {
					    canShrink = 0;
				    }
				    break;
				case LEVEL_Q:
				    if((8 * qr_data_codewords_Q[autosize - 2]) < est_binlen) {
					    canShrink = 0;
				    }
				    break;
				case LEVEL_H:
				    if((8 * qr_data_codewords_H[autosize - 2]) < est_binlen) {
					    canShrink = 0;
				    }
				    break;
			}
			if(canShrink == 1) { // Optimisation worked - data will fit in a smaller symbol
				autosize--;
			}
			else { // Data did not fit in the smaller symbol, revert to original size
				if(tribus(autosize - 1, 1, 2, 3) != tribus(autosize, 1, 2, 3)) {
					est_binlen = getBinaryLength(autosize, mode, jisdata, length, gs1, symbol->eci);
				}
			}
		}
	} while(canShrink == 1);
	version = autosize;
	if((symbol->option_2 >= 1) && (symbol->option_2 <= 40)) {
		// If the user has selected a larger symbol than the smallest available,
		// then use the size the user has selected, and re-optimise for this symbol size.
		if(symbol->option_2 > version) {
			version = symbol->option_2;
			est_binlen = getBinaryLength(symbol->option_2, mode, jisdata, length, gs1, symbol->eci);
		}
	}
	// Ensure maxium error correction capacity 
	if(est_binlen <= qr_data_codewords_M[version-1]) {
		ecc_level = LEVEL_M;
	}
	if(est_binlen <= qr_data_codewords_Q[version-1]) {
		ecc_level = LEVEL_Q;
	}
	if(est_binlen <= qr_data_codewords_H[version-1]) {
		ecc_level = LEVEL_H;
	}
	target_binlen = qr_data_codewords_L[version-1];
	blocks = qr_blocks_L[version - 1];
	switch(ecc_level) {
		case LEVEL_M: target_binlen = qr_data_codewords_M[version-1]; blocks = qr_blocks_M[version-1]; break;
		case LEVEL_Q: target_binlen = qr_data_codewords_Q[version-1]; blocks = qr_blocks_Q[version-1]; break;
		case LEVEL_H: target_binlen = qr_data_codewords_H[version-1]; blocks = qr_blocks_H[version-1]; break;
	}
#ifndef _MSC_VER
	int datastream[target_binlen + 1];
	int fullstream[qr_total_codewords[version - 1] + 1];
#else
	datastream = (int *)_alloca((target_binlen + 1) * sizeof(int));
	fullstream = (int *)_alloca((qr_total_codewords[version - 1] + 1) * sizeof(int));
#endif
	qr_binary(datastream, version, target_binlen, mode, jisdata, length, gs1, symbol->eci, est_binlen);
	add_ecc(fullstream, datastream, version, target_binlen, blocks);
	size = qr_sizes[version - 1];
#ifndef _MSC_VER
	uchar grid[size * size];
#else
	grid = (uchar *)_alloca((size * size) * sizeof(uchar));
#endif
	for(i = 0; i < size; i++) {
		for(j = 0; j < size; j++) {
			grid[(i * size) + j] = 0;
		}
	}
	setup_grid(grid, size, version);
	populate_grid(grid, size, fullstream, qr_total_codewords[version - 1]);
	if(version >= 7) {
		add_version_info(grid, size, version);
	}
	bitmask = apply_bitmask(grid, size, ecc_level);
	add_format_info(grid, size, ecc_level, bitmask);
	symbol->width = size;
	symbol->rows = size;
	for(i = 0; i < size; i++) {
		for(j = 0; j < size; j++) {
			if(grid[(i * size) + j] & 0x01) {
				set_module(symbol, i, j);
			}
		}
		symbol->row_height[i] = 1;
	}
	return 0;
}
//
// NOTE: From this point forward concerns Micro QR Code only 
//
static int micro_qr_intermediate(char binary[], int jisdata[], char mode[], int length, int * kanji_used, int * alphanum_used, int * byte_used)
{
	// Convert input data to an "intermediate stage" where data is binary encoded but control information is not 
	int position = 0, debug = 0;
	int short_data_block_length, i;
	char data_block;
	char buffer[2];
	sstrcpy(binary, "");
	if(debug) {
		for(i = 0; i < length; i++) {
			printf("%c", mode[i]);
		}
		printf("\n");
	}
	do {
		if(strlen(binary) > 128) {
			return ZINT_ERROR_TOO_LONG;
		}
		data_block = mode[position];
		short_data_block_length = 0;
		do {
			short_data_block_length++;
		} while(((short_data_block_length + position) < length) && (mode[position + short_data_block_length] == data_block));
		switch(data_block) {
			case 'K':
			    // Kanji mode 
			    // Mode indicator 
			    strcat(binary, "K");
			    *kanji_used = 1;
			    // Character count indicator 
			    buffer[0] = short_data_block_length;
			    buffer[1] = '\0';
			    strcat(binary, buffer);
			    if(debug) {
				    printf("Kanji block (length %d)\n\t", short_data_block_length);
			    }
			    // Character representation 
			    for(i = 0; i < short_data_block_length; i++) {
				    int jis = jisdata[position + i];
				    int msb, lsb, prod;
				    if(jis > 0x9fff) {
					    jis -= 0xc140;
				    }
				    msb = (jis & 0xff00) >> 4;
				    lsb = (jis & 0xff);
				    prod = (msb * 0xc0) + lsb;
				    qr_bscan(binary, prod, 0x1000);
				    if(debug) {
					    printf("0x%4X ", prod);
				    }
				    if(strlen(binary) > 128) {
					    return ZINT_ERROR_TOO_LONG;
				    }
			    }

			    if(debug) {
				    printf("\n");
			    }

			    break;
			case 'B':
			    /* Byte mode */
			    /* Mode indicator */
			    strcat(binary, "B");
			    *byte_used = 1;

			    /* Character count indicator */
			    buffer[0] = short_data_block_length;
			    buffer[1] = '\0';
			    strcat(binary, buffer);

			    if(debug) {
				    printf("Byte block (length %d)\n\t", short_data_block_length);
			    }
			    /* Character representation */
			    for(i = 0; i < short_data_block_length; i++) {
				    int byte = jisdata[position + i];
				    qr_bscan(binary, byte, 0x80);
				    if(debug) {
					    printf("0x%4X ", byte);
				    }
				    if(strlen(binary) > 128) {
					    return ZINT_ERROR_TOO_LONG;
				    }
			    }
			    if(debug) {
				    printf("\n");
			    }
			    break;
			case 'A':
			    /* Alphanumeric mode */
			    /* Mode indicator */
			    strcat(binary, "A");
			    *alphanum_used = 1;

			    /* Character count indicator */
			    buffer[0] = short_data_block_length;
			    buffer[1] = '\0';
			    strcat(binary, buffer);

			    if(debug) {
				    printf("Alpha block (length %d)\n\t", short_data_block_length);
			    }

			    /* Character representation */
			    i = 0;
			    while(i < short_data_block_length) {
				    int count;
				    int first = 0, second = 0, prod;
				    first = posn(RHODIUM, (char)jisdata[position + i]);
				    count = 1;
				    prod = first;
				    if(i + 1 < short_data_block_length && mode[position + i + 1] == 'A') {
					    second = posn(RHODIUM, (char)jisdata[position + i + 1]);
					    count = 2;
					    prod = (first * 45) + second;
				    }
				    qr_bscan(binary, prod, 1 << (5 * count)); /* count = 1..2 */
				    if(debug) {
					    printf("0x%4X ", prod);
				    }
				    if(strlen(binary) > 128) {
					    return ZINT_ERROR_TOO_LONG;
				    }
				    i += 2;
			    }
			    ;
			    if(debug) {
				    printf("\n");
			    }
			    break;
			case 'N':
			    /* Numeric mode */
			    /* Mode indicator */
			    strcat(binary, "N");
			    /* Character count indicator */
			    buffer[0] = short_data_block_length;
			    buffer[1] = '\0';
			    strcat(binary, buffer);
			    if(debug) {
				    printf("Number block (length %d)\n\t", short_data_block_length);
			    }
			    /* Character representation */
			    i = 0;
			    while(i < short_data_block_length) {
				    int count;
				    int first = 0, second = 0, third = 0, prod;
				    first = posn(NEON, (char)jisdata[position + i]);
				    count = 1;
				    prod = first;
				    if(i + 1 < short_data_block_length && mode[position + i + 1] == 'N') {
					    second = posn(NEON, (char)jisdata[position + i + 1]);
					    count = 2;
					    prod = (prod * 10) + second;
				    }
				    if(i + 2 < short_data_block_length && mode[position + i + 2] == 'N') {
					    third = posn(NEON, (char)jisdata[position + i + 2]);
					    count = 3;
					    prod = (prod * 10) + third;
				    }
				    qr_bscan(binary, prod, 1 << (3 * count)); /* count = 1..3 */
				    if(debug) {
					    printf("0x%4X (%d)", prod, prod);
				    }
				    if(strlen(binary) > 128) {
					    return ZINT_ERROR_TOO_LONG;
				    }
				    i += 3;
			    }
			    ;
			    if(debug) {
				    printf("\n");
			    }
			    break;
		}
		position += short_data_block_length;
	} while(position < length - 1);
	return 0;
}

static void get_bitlength(int count[], char stream[])
{
	const int length = strlen(stream);
	int i;
	for(i = 0; i < 4; i++) {
		count[i] = 0;
	}
	i = 0;
	do {
		if((stream[i] == '0') || (stream[i] == '1')) {
			count[0]++;
			count[1]++;
			count[2]++;
			count[3]++;
			i++;
		}
		else {
			switch(stream[i]) {
				case 'K':
				    count[2] += 5;
				    count[3] += 7;
				    i += 2;
				    break;
				case 'B':
				    count[2] += 6;
				    count[3] += 8;
				    i += 2;
				    break;
				case 'A':
				    count[1] += 4;
				    count[2] += 6;
				    count[3] += 8;
				    i += 2;
				    break;
				case 'N':
				    count[0] += 3;
				    count[1] += 5;
				    count[2] += 7;
				    count[3] += 9;
				    i += 2;
				    break;
			}
		}
	} while(i < length);
}

static void microqr_expand_binary(const char binary_stream[], char full_stream[], int version)
{
	const int length = strlen(binary_stream);
	int i = 0;
	do {
		switch(binary_stream[i]) {
			case '1': strcat(full_stream, "1"); i++; break;
			case '0': strcat(full_stream, "0"); i++; break;
			case 'N':
			    /* Numeric Mode */
			    /* Mode indicator */
			    switch(version) {
				    case 1: strcat(full_stream, "0"); break;
				    case 2: strcat(full_stream, "00"); break;
				    case 3: strcat(full_stream, "000"); break;
			    }
			    /* Character count indicator */
			    qr_bscan(full_stream, binary_stream[i + 1], 4 << version); /* version = 0..3 */

			    i += 2;
			    break;
			case 'A':
			    /* Alphanumeric Mode */
			    /* Mode indicator */
			    switch(version) {
				    case 1: strcat(full_stream, "1"); break;
				    case 2: strcat(full_stream, "01"); break;
				    case 3: strcat(full_stream, "001"); break;
			    }
			    /* Character count indicator */
			    qr_bscan(full_stream, binary_stream[i + 1], 2 << version); /* version = 1..3 */
			    i += 2;
			    break;
			case 'B':
			    /* Byte Mode */
			    /* Mode indicator */
			    switch(version) {
				    case 2: strcat(full_stream, "10"); break;
				    case 3: strcat(full_stream, "010"); break;
			    }
			    /* Character count indicator */
			    qr_bscan(full_stream, binary_stream[i + 1], 2 << version); /* version = 2..3 */
			    i += 2;
			    break;
			case 'K':
			    /* Kanji Mode */
			    /* Mode indicator */
			    switch(version) {
				    case 2: strcat(full_stream, "11"); break;
				    case 3: strcat(full_stream, "011"); break;
			    }
			    /* Character count indicator */
			    qr_bscan(full_stream, binary_stream[i + 1], 1 << version); /* version = 2..3 */
			    i += 2;
			    break;
		}
	} while(i < length);
}

static void micro_qr_m1(char binary_data[])
{
	int i, j;
	int remainder;
	int data_codewords, ecc_codewords;
	uchar data_blocks[4], ecc_blocks[3];
	int bits_total = 20;
	int latch = 0;
	// Add terminator 
	int bits_left = bits_total - strlen(binary_data);
	if(bits_left <= 3) {
		for(i = 0; i < bits_left; i++) {
			strcat(binary_data, "0");
		}
		latch = 1;
	}
	else {
		strcat(binary_data, "000");
	}
	if(latch == 0) {
		// Manage last (4-bit) block 
		bits_left = bits_total - strlen(binary_data);
		if(bits_left <= 4) {
			for(i = 0; i < bits_left; i++) {
				strcat(binary_data, "0");
			}
			latch = 1;
		}
	}
	if(latch == 0) {
		// Complete current byte 
		remainder = 8 - (strlen(binary_data) % 8);
		if(remainder == 8) {
			remainder = 0;
		}
		for(i = 0; i < remainder; i++) {
			strcat(binary_data, "0");
		}
		// Add padding 
		bits_left = bits_total - strlen(binary_data);
		if(bits_left > 4) {
			remainder = (bits_left - 4) / 8;
			for(i = 0; i < remainder; i++) {
				strcat(binary_data, i & 1 ? "00010001" : "11101100");
			}
		}
		strcat(binary_data, "0000");
	}
	data_codewords = 3;
	ecc_codewords = 2;
	/* Copy data into codewords */
	for(i = 0; i < (data_codewords - 1); i++) {
		data_blocks[i] = 0;
		for(j = 0; j < 8; j++) {
			if(binary_data[(i * 8) + j] == '1') {
				data_blocks[i] += 0x80 >> j;
			}
		}
	}
	data_blocks[2] = 0;
	for(j = 0; j < 4; j++) {
		if(binary_data[16 + j] == '1') {
			data_blocks[2] += 0x80 >> j;
		}
	}
	/* Calculate Reed-Solomon error codewords */
	rs_init_gf(0x11d);
	rs_init_code(ecc_codewords, 0);
	rs_encode(data_codewords, data_blocks, ecc_blocks);
	rs_free();
	/* Add Reed-Solomon codewords to binary data */
	for(i = 0; i < ecc_codewords; i++) {
		qr_bscan(binary_data, ecc_blocks[ecc_codewords - i - 1], 0x80);
	}
}

static void micro_qr_m2(char binary_data[], int ecc_mode)
{
	int i, j;
	int bits_total, bits_left, remainder;
	int data_codewords, ecc_codewords;
	uchar data_blocks[6], ecc_blocks[7];
	int latch = 0;
	if(ecc_mode == LEVEL_L) {
		bits_total = 40;
	}
	if(ecc_mode == LEVEL_M) {
		bits_total = 32;
	}
	/* Add terminator */
	bits_left = bits_total - strlen(binary_data);
	if(bits_left <= 5) {
		for(i = 0; i < bits_left; i++) {
			strcat(binary_data, "0");
		}
		latch = 1;
	}
	else {
		strcat(binary_data, "00000");
	}
	if(latch == 0) {
		/* Complete current byte */
		remainder = 8 - (strlen(binary_data) % 8);
		if(remainder == 8) {
			remainder = 0;
		}
		for(i = 0; i < remainder; i++) {
			strcat(binary_data, "0");
		}
		/* Add padding */
		bits_left = bits_total - strlen(binary_data);
		remainder = bits_left / 8;
		for(i = 0; i < remainder; i++) {
			strcat(binary_data, i & 1 ? "00010001" : "11101100");
		}
	}
	if(ecc_mode == LEVEL_L) {
		data_codewords = 5;
		ecc_codewords = 5;
	}
	if(ecc_mode == LEVEL_M) {
		data_codewords = 4;
		ecc_codewords = 6;
	}
	/* Copy data into codewords */
	for(i = 0; i < data_codewords; i++) {
		data_blocks[i] = 0;
		for(j = 0; j < 8; j++) {
			if(binary_data[(i * 8) + j] == '1') {
				data_blocks[i] += 0x80 >> j;
			}
		}
	}
	/* Calculate Reed-Solomon error codewords */
	rs_init_gf(0x11d);
	rs_init_code(ecc_codewords, 0);
	rs_encode(data_codewords, data_blocks, ecc_blocks);
	rs_free();
	/* Add Reed-Solomon codewords to binary data */
	for(i = 0; i < ecc_codewords; i++) {
		qr_bscan(binary_data, ecc_blocks[ecc_codewords - i - 1], 0x80);
	}
}

static void micro_qr_m3(char binary_data[], int ecc_mode)
{
	int i, j;
	int bits_total, bits_left, remainder;
	int data_codewords, ecc_codewords;
	uchar data_blocks[12], ecc_blocks[9];
	int latch = 0;
	if(ecc_mode == LEVEL_L) {
		bits_total = 84;
	}
	if(ecc_mode == LEVEL_M) {
		bits_total = 68;
	}
	// Add terminator 
	bits_left = bits_total - strlen(binary_data);
	if(bits_left <= 7) {
		for(i = 0; i < bits_left; i++) {
			strcat(binary_data, "0");
		}
		latch = 1;
	}
	else {
		strcat(binary_data, "0000000");
	}
	if(latch == 0) {
		// Manage last (4-bit) block 
		bits_left = bits_total - strlen(binary_data);
		if(bits_left <= 4) {
			for(i = 0; i < bits_left; i++) {
				strcat(binary_data, "0");
			}
			latch = 1;
		}
	}
	if(latch == 0) {
		// Complete current byte 
		remainder = 8 - (strlen(binary_data) % 8);
		if(remainder == 8) {
			remainder = 0;
		}
		for(i = 0; i < remainder; i++) {
			strcat(binary_data, "0");
		}
		// Add padding 
		bits_left = bits_total - strlen(binary_data);
		if(bits_left > 4) {
			remainder = (bits_left - 4) / 8;
			for(i = 0; i < remainder; i++) {
				strcat(binary_data, i & 1 ? "00010001" : "11101100");
			}
		}
		strcat(binary_data, "0000");
	}

	if(ecc_mode == LEVEL_L) {
		data_codewords = 11;
		ecc_codewords = 6;
	}
	if(ecc_mode == LEVEL_M) {
		data_codewords = 9;
		ecc_codewords = 8;
	}
	// Copy data into codewords 
	for(i = 0; i < (data_codewords - 1); i++) {
		data_blocks[i] = 0;
		for(j = 0; j < 8; j++) {
			if(binary_data[(i * 8) + j] == '1') {
				data_blocks[i] += 0x80 >> j;
			}
		}
	}
	if(ecc_mode == LEVEL_L) {
		data_blocks[10] = 0;
		for(j = 0; j < 4; j++) {
			if(binary_data[80 + j] == '1') {
				data_blocks[10] += 0x80 >> j;
			}
		}
	}
	if(ecc_mode == LEVEL_M) {
		data_blocks[8] = 0;
		for(j = 0; j < 4; j++) {
			if(binary_data[64 + j] == '1') {
				data_blocks[8] += 0x80 >> j;
			}
		}
	}
	// Calculate Reed-Solomon error codewords 
	rs_init_gf(0x11d);
	rs_init_code(ecc_codewords, 0);
	rs_encode(data_codewords, data_blocks, ecc_blocks);
	rs_free();
	// Add Reed-Solomon codewords to binary data 
	for(i = 0; i < ecc_codewords; i++) {
		qr_bscan(binary_data, ecc_blocks[ecc_codewords - i - 1], 0x80);
	}
}

static void micro_qr_m4(char binary_data[], int ecc_mode)
{
	int i, j;
	int bits_total, bits_left, remainder;
	int data_codewords, ecc_codewords;
	uchar data_blocks[17], ecc_blocks[15];
	int latch = 0;
	if(ecc_mode == LEVEL_L) {
		bits_total = 128;
	}
	if(ecc_mode == LEVEL_M) {
		bits_total = 112;
	}
	if(ecc_mode == LEVEL_Q) {
		bits_total = 80;
	}
	// Add terminator 
	bits_left = bits_total - strlen(binary_data);
	if(bits_left <= 9) {
		for(i = 0; i < bits_left; i++) {
			strcat(binary_data, "0");
		}
		latch = 1;
	}
	else {
		strcat(binary_data, "000000000");
	}
	if(latch == 0) {
		// Complete current byte 
		remainder = 8 - (strlen(binary_data) % 8);
		if(remainder == 8) {
			remainder = 0;
		}
		for(i = 0; i < remainder; i++) {
			strcat(binary_data, "0");
		}
		// Add padding 
		bits_left = bits_total - strlen(binary_data);
		remainder = bits_left / 8;
		for(i = 0; i < remainder; i++) {
			strcat(binary_data, i & 1 ? "00010001" : "11101100");
		}
	}
	if(ecc_mode == LEVEL_L) {
		data_codewords = 16;
		ecc_codewords = 8;
	}
	if(ecc_mode == LEVEL_M) {
		data_codewords = 14;
		ecc_codewords = 10;
	}
	if(ecc_mode == LEVEL_Q) {
		data_codewords = 10;
		ecc_codewords = 14;
	}
	// Copy data into codewords 
	for(i = 0; i < data_codewords; i++) {
		data_blocks[i] = 0;
		for(j = 0; j < 8; j++) {
			if(binary_data[(i * 8) + j] == '1') {
				data_blocks[i] += 0x80 >> j;
			}
		}
	}
	// Calculate Reed-Solomon error codewords 
	rs_init_gf(0x11d);
	rs_init_code(ecc_codewords, 0);
	rs_encode(data_codewords, data_blocks, ecc_blocks);
	rs_free();
	// Add Reed-Solomon codewords to binary data 
	for(i = 0; i < ecc_codewords; i++) {
		qr_bscan(binary_data, ecc_blocks[ecc_codewords - i - 1], 0x80);
	}
}

static void micro_setup_grid(uchar* grid, int size)
{
	int i, toggle = 1;
	// Add timing patterns 
	for(i = 0; i < size; i++) {
		if(toggle == 1) {
			grid[i] = 0x21;
			grid[(i * size)] = 0x21;
			toggle = 0;
		}
		else {
			grid[i] = 0x20;
			grid[(i * size)] = 0x20;
			toggle = 1;
		}
	}
	// Add finder patterns 
	place_finder(grid, size, 0, 0);
	// Add separators 
	for(i = 0; i < 7; i++) {
		grid[(7 * size) + i] = 0x10;
		grid[(i * size) + 7] = 0x10;
	}
	grid[(7 * size) + 7] = 0x10;
	// Reserve space for format information 
	for(i = 0; i < 8; i++) {
		grid[(8 * size) + i] += 0x20;
		grid[(i * size) + 8] += 0x20;
	}
	grid[(8 * size) + 8] += 20;
}

static void micro_populate_grid(uchar* grid, int size, char full_stream[])
{
	int direction = 1; /* up */
	int row = 0; /* right hand side */
	int x;
	int n = strlen(full_stream);
	int y = size - 1;
	int i = 0;
	do {
		x = (size - 2) - (row * 2);
		if(!(grid[(y * size) + (x + 1)] & 0xf0)) {
			if(full_stream[i] == '1') {
				grid[(y * size) + (x + 1)] = 0x01;
			}
			else {
				grid[(y * size) + (x + 1)] = 0x00;
			}
			i++;
		}
		if(i < n) {
			if(!(grid[(y * size) + x] & 0xf0)) {
				if(full_stream[i] == '1') {
					grid[(y * size) + x] = 0x01;
				}
				else {
					grid[(y * size) + x] = 0x00;
				}
				i++;
			}
		}
		if(direction) {
			y--;
		}
		else {
			y++;
		}
		if(y == 0) {
			/* reached the top */
			row++;
			y = 1;
			direction = 0;
		}
		if(y == size) {
			/* reached the bottom */
			row++;
			y = size - 1;
			direction = 1;
		}
	} while(i < n);
}

static int micro_evaluate(uchar * grid, int size, int pattern)
{
	int sum1, sum2, i, filter = 0, retval;
	switch(pattern) {
		case 0: filter = 0x01; break;
		case 1: filter = 0x02; break;
		case 2: filter = 0x04; break;
		case 3: filter = 0x08; break;
	}
	sum1 = 0;
	sum2 = 0;
	for(i = 1; i < size; i++) {
		if(grid[(i * size) + size - 1] & filter) {
			sum1++;
		}
		if(grid[((size - 1) * size) + i] & filter) {
			sum2++;
		}
	}
	if(sum1 <= sum2) {
		retval = (sum1 * 16) + sum2;
	}
	else {
		retval = (sum2 * 16) + sum1;
	}
	return retval;
}

static int micro_apply_bitmask(uchar * grid, int size)
{
	int x, y;
	uchar p;
	int pattern, value[8];
	int best_val, best_pattern;

#ifndef _MSC_VER
	uchar mask[size * size];
	uchar eval[size * size];
#else
	uchar* mask = (uchar *)_alloca((size * size) * sizeof(uchar));
	uchar* eval = (uchar *)_alloca((size * size) * sizeof(uchar));
#endif
	// Perform data masking 
	for(x = 0; x < size; x++) {
		for(y = 0; y < size; y++) {
			mask[(y * size) + x] = 0x00;
			if(!(grid[(y * size) + x] & 0xf0)) {
				if((y & 1) == 0) {
					mask[(y * size) + x] += 0x01;
				}
				if((((y / 2) + (x / 3)) & 1) == 0) {
					mask[(y * size) + x] += 0x02;
				}
				if(((((y * x) & 1) + ((y * x) % 3)) & 1) == 0) {
					mask[(y * size) + x] += 0x04;
				}
				if(((((y + x) & 1) + ((y * x) % 3)) & 1) == 0) {
					mask[(y * size) + x] += 0x08;
				}
			}
		}
	}
	for(x = 0; x < size; x++) {
		for(y = 0; y < size; y++) {
			if(grid[(y * size) + x] & 0x01) {
				p = 0xff;
			}
			else {
				p = 0x00;
			}
			eval[(y * size) + x] = mask[(y * size) + x] ^ p;
		}
	}
	// Evaluate result 
	for(pattern = 0; pattern < 8; pattern++) {
		value[pattern] = micro_evaluate(eval, size, pattern);
	}
	best_pattern = 0;
	best_val = value[0];
	for(pattern = 1; pattern < 4; pattern++) {
		if(value[pattern] > best_val) {
			best_pattern = pattern;
			best_val = value[pattern];
		}
	}
	// Apply mask 
	for(x = 0; x < size; x++) {
		for(y = 0; y < size; y++) {
			if(mask[(y * size) + x] & (0x01 << best_pattern)) {
				if(grid[(y * size) + x] & 0x01) {
					grid[(y * size) + x] = 0x00;
				}
				else {
					grid[(y * size) + x] = 0x01;
				}
			}
		}
	}

	return best_pattern;
}

int microqr(struct ZintSymbol * symbol, const uchar source[], int length)
{
	int i, j, glyph, size;
	char binary_stream[200];
	char full_stream[200];
	int utfdata[40];
	int jisdata[40];
	char mode[40];
	int error_number, kanji_used = 0, alphanum_used = 0, byte_used = 0;
	int version_valid[4];
	int binary_count[4];
	int ecc_level, autoversion;
	int n_count, a_count, bitmask, format, format_full;
	int version;
#ifdef _MSC_VER
	uchar* grid;
#endif
	if(length > 35) {
		sstrcpy(symbol->errtxt, "Input data too long (E62)");
		return ZINT_ERROR_TOO_LONG;
	}
	for(i = 0; i < 4; i++) {
		version_valid[i] = 1;
	}
	if(symbol->input_mode == DATA_MODE) {
		for(i = 0; i < length; i++) {
			jisdata[i] = (int)source[i];
		}
	}
	else {
		// Convert Unicode input to Shift-JIS 
		error_number = utf8toutf16(symbol, source, utfdata, &length);
		if(error_number != 0) {
			return error_number;
		}
		for(i = 0; i < length; i++) {
			if(utfdata[i] <= 0xff) {
				jisdata[i] = utfdata[i];
			}
			else {
				j = 0;
				glyph = 0;
				do {
					if(sjis_lookup[j * 2] == utfdata[i]) {
						glyph = sjis_lookup[(j * 2) + 1];
					}
					j++;
				} while((j < 6843) && (glyph == 0));
				if(glyph == 0) {
					sstrcpy(symbol->errtxt, "Invalid character in input data (E63)");
					return ZINT_ERROR_INVALID_DATA;
				}
				jisdata[i] = glyph;
			}
		}
	}
	define_mode(mode, jisdata, length, 0);
	n_count = 0;
	a_count = 0;
	for(i = 0; i < length; i++) {
		if((jisdata[i] >= '0') && (jisdata[i] <= '9')) {
			n_count++;
		}
		if(in_alpha(jisdata[i])) {
			a_count++;
		}
	}
	if(a_count == length) {
		/* All data can be encoded in Alphanumeric mode */
		for(i = 0; i < length; i++) {
			mode[i] = 'A';
		}
	}
	if(n_count == length) {
		/* All data can be encoded in Numeric mode */
		for(i = 0; i < length; i++) {
			mode[i] = 'N';
		}
	}
	error_number = micro_qr_intermediate(binary_stream, jisdata, mode, length, &kanji_used, &alphanum_used, &byte_used);
	if(error_number != 0) {
		sstrcpy(symbol->errtxt, "Input data too long (E64)");
		return error_number;
	}
	get_bitlength(binary_count, binary_stream);
	// Eliminate possivle versions depending on type of content 
	if(byte_used) {
		version_valid[0] = 0;
		version_valid[1] = 0;
	}
	if(alphanum_used) {
		version_valid[0] = 0;
	}
	if(kanji_used) {
		version_valid[0] = 0;
		version_valid[1] = 0;
	}
	/* Eliminate possible versions depending on length of binary data */
	if(binary_count[0] > 20) {
		version_valid[0] = 0;
	}
	if(binary_count[1] > 40) {
		version_valid[1] = 0;
	}
	if(binary_count[2] > 84) {
		version_valid[2] = 0;
	}
	if(binary_count[3] > 128) {
		sstrcpy(symbol->errtxt, "Input data too long (E65)");
		return ZINT_ERROR_TOO_LONG;
	}
	/* Eliminate possible versions depending on error correction level specified */
	ecc_level = LEVEL_L;
	if((symbol->option_1 >= 1) && (symbol->option_2 <= 4)) {
		ecc_level = symbol->option_1;
	}
	if(ecc_level == LEVEL_H) {
		sstrcpy(symbol->errtxt, "Error correction level H not available (E66)");
		return ZINT_ERROR_INVALID_OPTION;
	}
	if(ecc_level == LEVEL_Q) {
		version_valid[0] = 0;
		version_valid[1] = 0;
		version_valid[2] = 0;
		if(binary_count[3] > 80) {
			sstrcpy(symbol->errtxt, "Input data too long (E67)");
			return ZINT_ERROR_TOO_LONG;
		}
	}
	if(ecc_level == LEVEL_M) {
		version_valid[0] = 0;
		if(binary_count[1] > 32) {
			version_valid[1] = 0;
		}
		if(binary_count[2] > 68) {
			version_valid[2] = 0;
		}
		if(binary_count[3] > 112) {
			sstrcpy(symbol->errtxt, "Input data too long (E68)");
			return ZINT_ERROR_TOO_LONG;
		}
	}
	autoversion = 3;
	if(version_valid[2]) {
		autoversion = 2;
	}
	if(version_valid[1]) {
		autoversion = 1;
	}
	if(version_valid[0]) {
		autoversion = 0;
	}
	version = autoversion;
	// Get version from user 
	if((symbol->option_2 >= 1) && (symbol->option_2 <= 4)) {
		if(symbol->option_2 >= autoversion) {
			version = symbol->option_2;
		}
	}
	// If there is enough unused space then increase the error correction level 
	if(version == 3) {
		if(binary_count[3] <= 112) {
			ecc_level = LEVEL_M;
		}
		if(binary_count[3] <= 80) {
			ecc_level = LEVEL_Q;
		}
	}
	if(version == 2) {
		if(binary_count[2] <= 68) {
			ecc_level = LEVEL_M;
		}
	}
	if(version == 1) {
		if(binary_count[1] <= 32) {
			ecc_level = LEVEL_M;
		}
	}
	sstrcpy(full_stream, "");
	microqr_expand_binary(binary_stream, full_stream, version);
	switch(version) {
		case 0: micro_qr_m1(full_stream); break;
		case 1: micro_qr_m2(full_stream, ecc_level); break;
		case 2: micro_qr_m3(full_stream, ecc_level); break;
		case 3: micro_qr_m4(full_stream, ecc_level); break;
	}
	size = micro_qr_sizes[version];
#ifndef _MSC_VER
	uchar grid[size * size];
#else
	grid = (uchar *)_alloca((size * size) * sizeof(uchar));
#endif
	for(i = 0; i < size; i++) {
		for(j = 0; j < size; j++) {
			grid[(i * size) + j] = 0;
		}
	}
	micro_setup_grid(grid, size);
	micro_populate_grid(grid, size, full_stream);
	bitmask = micro_apply_bitmask(grid, size);
	/* Add format data */
	format = 0;
	switch(version) {
		case 1: 
			switch(ecc_level) {
			    case 1: format = 1; break;
			    case 2: format = 2; break;
			}
		    break;
		case 2: 
			switch(ecc_level) {
			    case 1: format = 3; break;
			    case 2: format = 4; break;
			}
		    break;
		case 3: 
			switch(ecc_level) {
			    case 1: format = 5; break;
			    case 2: format = 6; break;
			    case 3: format = 7; break;
			}
		    break;
	}
	format_full = qr_annex_c1[(format << 2) + bitmask];
	if(format_full & 0x4000) {
		grid[(8 * size) + 1] += 0x01;
	}
	if(format_full & 0x2000) {
		grid[(8 * size) + 2] += 0x01;
	}
	if(format_full & 0x1000) {
		grid[(8 * size) + 3] += 0x01;
	}
	if(format_full & 0x800) {
		grid[(8 * size) + 4] += 0x01;
	}
	if(format_full & 0x400) {
		grid[(8 * size) + 5] += 0x01;
	}
	if(format_full & 0x200) {
		grid[(8 * size) + 6] += 0x01;
	}
	if(format_full & 0x100) {
		grid[(8 * size) + 7] += 0x01;
	}
	if(format_full & 0x80) {
		grid[(8 * size) + 8] += 0x01;
	}
	if(format_full & 0x40) {
		grid[(7 * size) + 8] += 0x01;
	}
	if(format_full & 0x20) {
		grid[(6 * size) + 8] += 0x01;
	}
	if(format_full & 0x10) {
		grid[(5 * size) + 8] += 0x01;
	}
	if(format_full & 0x08) {
		grid[(4 * size) + 8] += 0x01;
	}
	if(format_full & 0x04) {
		grid[(3 * size) + 8] += 0x01;
	}
	if(format_full & 0x02) {
		grid[(2 * size) + 8] += 0x01;
	}
	if(format_full & 0x01) {
		grid[(1 * size) + 8] += 0x01;
	}
	symbol->width = size;
	symbol->rows = size;
	for(i = 0; i < size; i++) {
		for(j = 0; j < size; j++) {
			if(grid[(i * size) + j] & 0x01) {
				set_module(symbol, i, j);
			}
		}
		symbol->row_height[i] = 1;
	}
	return 0;
}
