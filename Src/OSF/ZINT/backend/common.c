/* common.c - Contains functions needed for a number of barcodes */

/*
    libzint - the open source barcode library
    Copyright (C) 2008-2016 Robin Stuart <rstuart114@gmail.com>

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

// Local replacement for strlen() with uchar strings 
/*size_t ustrlen_(const uchar data[])
{
	return strlen((const char *)data);
}*/

// Converts a character 0-9 to its equivalent integer value 
/* @sobolev int FASTCALL ctoi_ReplacedWith_hex(const char source)
{
	return ((source >= '0') && (source <= '9')) ? (source - '0') : (source - 'A' + 10);
}*/

// Converts an integer value to its hexadecimal character 
char FASTCALL itoc(const int source)
{
	return ((source >= 0) && (source <= 9)) ? ('0' + source) : ('A' + (source - 10));
}

// Converts lower case characters to upper case in a string source[] 
void to_upper(uchar source[])
{
	const size_t src_len = sstrlen(source);
	for(size_t i = 0; i < src_len; i++) {
		if((source[i] >= 'a') && (source[i] <= 'z')) {
			source [i] = (source[i] - 'a') + 'A';
		}
	}
}

// Verifies that a string only uses valid characters 
int FASTCALL is_sane(const char test_string[], const uchar source[], const size_t length)
{
	const size_t lt = strlen(test_string);
	for(size_t i = 0; i < length; i++) {
		uint latch = FALSE;
		for(size_t j = 0; j < lt; j++) {
			if(source[i] == test_string[j]) {
				latch = TRUE;
				break;
			}
		}
		if(!(latch)) {
			return ZINT_ERROR_INVALID_DATA;
		}
	}
	return 0;
}

/* Returns the position of data in set_string */
int FASTCALL posn(const char set_string[], const char data)
{
	const size_t n = strlen(set_string);
	for(size_t i = 0; i < n; i++) {
		if(data == set_string[i]) {
			return i;
		}
	}
	return 0;
}

/* Replaces huge switch statements for looking up in tables */
void FASTCALL lookup(const char set_string[], const char * table[], const char data, char dest[])
{
	const size_t n = strlen(set_string);
	for(size_t i = 0; i < n; i++) {
		if(data == set_string[i]) {
			strcat(dest, table[i]);
		}
	}
}
//
// Return true (1) if a module is dark/black, otherwise false (0) 
//
int module_is_set(const struct ZintSymbol * symbol, const int y_coord, const int x_coord)
{
	return (symbol->encoded_data[y_coord][x_coord / 7] >> (x_coord % 7)) & 1;
}
//
// Set a module to dark/black 
//
void FASTCALL set_module(struct ZintSymbol * symbol, const int y_coord, const int x_coord)
{
	symbol->encoded_data[y_coord][x_coord / 7] |= 1 << (x_coord % 7);
}
//
// Set (or unset) a module to white 
//
void FASTCALL unset_module(struct ZintSymbol * symbol, const int y_coord, const int x_coord)
{
	symbol->encoded_data[y_coord][x_coord / 7] &= ~(1 << (x_coord % 7));
}
//
// Expands from a width pattern to a bit pattern 
//
void FASTCALL expand(struct ZintSymbol * symbol, const char data[])
{
	const size_t n = strlen(data);
	int writer = 0;
	char latch = '1';
	for(size_t reader = 0; reader < n; reader++) {
		for(uint i = 0; i < hex(data[reader]); i++) {
			if(latch == '1') {
				set_module(symbol, symbol->rows, writer);
			}
			writer++;
		}
		latch = (latch == '1' ? '0' : '1');
	}
	if(symbol->Std != BARCODE_PHARMA) {
		if(writer > symbol->width) {
			symbol->width = writer;
		}
	}
	else {
		// Pharmacode One ends with a space - adjust for this 
		if(writer > (symbol->width + 2)) {
			symbol->width = (writer - 2);
		}
	}
	symbol->rows = symbol->rows + 1;
}
//
// Indicates which symbologies can have row binding 
//
int FASTCALL is_stackable(const int symbology)
{
	int retval = 0;
	if(symbology < BARCODE_PDF417) {
		retval = 1;
	}
	switch(symbology) {
		case BARCODE_CODE128B:
		case BARCODE_ISBNX:
		case BARCODE_EAN14:
		case BARCODE_NVE18:
		case BARCODE_KOREAPOST:
		case BARCODE_PLESSEY:
		case BARCODE_TELEPEN_NUM:
		case BARCODE_ITF14:
		case BARCODE_CODE32:
		case BARCODE_CODABLOCKF:
		    retval = 1;
	}
	return retval;
}
//
// Indicates which symbols can have addon (EAN-2 and EAN-5)
//
int FASTCALL is_extendable(const int symbology)
{
	return BIN(oneof7(symbology, BARCODE_EANX, BARCODE_UPCA, BARCODE_UPCE, BARCODE_ISBNX, BARCODE_UPCA_CC, BARCODE_UPCE_CC, BARCODE_EANX_CC));
}

int istwodigits(const uchar source[], const int position)
{
	if((source[position] >= '0') && (source[position] <= '9')) {
		if((source[position + 1] >= '0') && (source[position + 1] <= '9')) {
			return 1;
		}
	}
	return 0;
}

int utf8toutf16(struct ZintSymbol * symbol, const uchar source[], int vals[], int * length)
{
	int bpos = 0;
	int jpos = 0;
	int error_number = 0;
	int next = 0;
	do {
		if(source[bpos] <= 0x7f) {
			/* 1 byte mode (7-bit ASCII) */
			vals[jpos] = source[bpos];
			next = bpos + 1;
			jpos++;
		}
		else {
			if((source[bpos] >= 0x80) && (source[bpos] <= 0xbf)) {
				sstrcpy(symbol->errtxt, "Corrupt Unicode data (B40)");
				return ZINT_ERROR_INVALID_DATA;
			}
			if((source[bpos] >= 0xc0) && (source[bpos] <= 0xc1)) {
				sstrcpy(symbol->errtxt, "Overlong encoding not supported (B41)");
				return ZINT_ERROR_INVALID_DATA;
			}
			if((source[bpos] >= 0xc2) && (source[bpos] <= 0xdf)) {
				/* 2 byte mode */
				vals[jpos] = ((source[bpos] & 0x1f) << 6) + (source[bpos + 1] & 0x3f);
				next = bpos + 2;
				jpos++;
			}
			else if((source[bpos] >= 0xe0) && (source[bpos] <= 0xef))    {
				/* 3 byte mode */
				vals[jpos] = ((source[bpos] & 0x0f) << 12) + ((source[bpos + 1] & 0x3f) << 6) + (source[bpos + 2] & 0x3f);
				next = bpos + 3;
				jpos++;
			}
			else if(source[bpos] >= 0xf0)    {
				sstrcpy(symbol->errtxt, "Unicode sequences of more than 3 bytes not supported (B42)");
				return ZINT_ERROR_INVALID_DATA;
			}
		}
		bpos = next;
	} while(bpos < *length);
	*length = jpos;
	return error_number;
}

void set_minimum_height(struct ZintSymbol * symbol, int min_height)
{
	/* Enforce minimum permissable height of rows */
	int fixed_height = 0;
	int zero_count = 0;
	int i;
	for(i = 0; i < symbol->rows; i++) {
		fixed_height += symbol->row_height[i];

		if(symbol->row_height[i] == 0) {
			zero_count++;
		}
	}
	if(zero_count > 0) {
		if(((symbol->height - fixed_height) / zero_count) < min_height) {
			for(i = 0; i < symbol->rows; i++) {
				if(symbol->row_height[i] == 0) {
					symbol->row_height[i] = min_height;
				}
			}
		}
	}
}

