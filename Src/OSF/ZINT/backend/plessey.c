/* plessey.c - Handles Plessey and MSI Plessey */

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

// @sobolev #define SSET    "0123456789ABCDEF"
static const char * PlessTable[16] = {
	"13131313", "31131313", "13311313", "31311313", "13133113", "31133113",
	"13313113", "31313113", "13131331", "31131331", "13311331", "31311331", "13133131",
	"31133131", "13313131", "31313131"
};

static const char * MSITable[10] = {
	"12121212", "12121221", "12122112", "12122121", "12211212", "12211221",
	"12212112", "12212121", "21121212", "21121221"
};
//
// Not MSI/Plessey but the older Plessey standard 
//
int plessey(struct ZintSymbol * symbol, const uchar source[], int length)
{
	static const char grid[9] = {1, 1, 1, 1, 0, 1, 0, 0, 1};
	char dest[1024]; /* 8 + 65 * 8 + 8 * 2 + 9 + 1 ~ 1024 */
	int error_number = 0;
	if(length > 65) {
		sstrcpy(symbol->errtxt, "Input too long (C70)");
		error_number = ZINT_ERROR_TOO_LONG;
	}
	else {
		error_number = is_sane(SlConst::P_HxDigU, source, length);
		if(error_number == ZINT_ERROR_INVALID_DATA) {
			ZintMakeErrText_InvCharInData("C71", symbol->errtxt, sizeof(symbol->errtxt));
		}
		else {
			int    i;
			uchar * checkptr = static_cast<uchar *>(SAlloc::C(1, length * 4 + 8));
			// Start character 
			sstrcpy(dest, "31311331");
			// Data area 
			for(i = 0; i < length; i++) {
				const uint check = posn(SlConst::P_HxDigU, source[i]);
				lookup(SlConst::P_HxDigU, PlessTable, source[i], dest);
				checkptr[4 * i] = check & 1;
				checkptr[4 * i + 1] = (check >> 1) & 1;
				checkptr[4 * i + 2] = (check >> 2) & 1;
				checkptr[4 * i + 3] = (check >> 3) & 1;
			}
			//
			// CRC check digit code adapted from code by Leonid A. Broukhis used in GNU Barcode 
			//
			for(i = 0; i < (4 * length); i++) {
				if(checkptr[i])
					for(int j = 0; j < 9; j++)
						checkptr[i + j] ^= grid[j];
			}
			for(i = 0; i < 8; i++) {
				switch(checkptr[length * 4 + i]) {
					case 0: strcat(dest, "13"); break;
					case 1: strcat(dest, "31"); break;
				}
			}
			// Stop character 
			strcat(dest, "331311313");
			expand(symbol, dest);
			sstrcpy(symbol->text, source);
			SAlloc::F(checkptr);
		}
	}
	return error_number;
}
//
// Plain MSI Plessey - does not calculate any check character 
//
static int msi_plessey(struct ZintSymbol * symbol, const uchar source[], int length)
{
	if(length > 55) {
		sstrcpy(symbol->errtxt, "Input too long (C72)");
		return ZINT_ERROR_TOO_LONG;
	}
	else {
		char dest[512]; /* 2 + 55 * 8 + 3 + 1 ~ 512 */
		// start character 
		sstrcpy(dest, "21");
		for(int i = 0; i < length; i++) {
			lookup(NEON, MSITable, source[i], dest);
		}
		// Stop character 
		strcat(dest, "121");
		expand(symbol, dest);
		sstrcpy(symbol->text, source);
		return 0;
	}
}
//
// MSI Plessey with Modulo 10 check digit - algorithm from Barcode Island
// http://www.barcodeisland.com/
//
static int msi_plessey_mod10(struct ZintSymbol * symbol, const uchar source[], int length)
{
	int    error_number = 0;
	if(length > 18) {
		sstrcpy(symbol->errtxt, "Input too long (C73)");
		error_number = ZINT_ERROR_TOO_LONG;
	}
	else {
		ulong  wright, dau, pedwar, pump/*, n*/;
		char   un[200], tri[32];
		char dest[1000];
		// start character 
		sstrcpy(dest, "21");
		// draw data section 
		{
			int    i;
			for(i = 0; i < length; i++) {
				lookup(NEON, MSITable, source[i], dest);
			}
			{
				// calculate check digit 
				wright = 0;
				const ulong n = !(length & 1);
				for(i = n; i < length; i += 2) {
					un[wright++] = source[i];
				}
				un[wright] = '\0';
				dau = strtoul(un, NULL, 10);
				dau *= 2;
				sprintf(tri, "%ld", dau);
			}
		}
		{
			pedwar = 0;
			const ulong h = strlen(tri);
			for(ulong i = 0; i < h; i++) {
				pedwar += hex(tri[i]);
			}
		}
		{
			const int n = (int)(length & 1);
			for(int i = n; i < length; i += 2) {
				pedwar += hex(source[i]);
			}
		}
		pump = (10 - pedwar % 10);
		if(pump == 10) {
			pump = 0;
		}
		// draw check digit 
		lookup(NEON, MSITable, itoc(pump), dest);
		// Stop character 
		strcat(dest, "121");
		expand(symbol, dest);
		sstrcpy(symbol->text, source);
		symbol->text[length] = itoc(pump);
		symbol->text[length + 1] = '\0';
	}
	return error_number;
}
//
// MSI Plessey with two Modulo 10 check digits - algorithm from
// Barcode Island http://www.barcodeisland.com/ 
//
static int msi_plessey_mod1010(struct ZintSymbol * symbol, const uchar source[], const uint src_len)
{
	int    error_number = 0;
	if(src_len > 18) { // No Entry Stack Smashers! limit because of str->number conversion
		sstrcpy(symbol->errtxt, "Input too long (C74)");
		error_number = ZINT_ERROR_TOO_LONG;
	}
	else {
		ulong  i, n, wright, dau, pedwar, pump, chwech;
		char   un[16], tri[32];
		ulong  h;
		char   dest[1000];
		// start character 
		sstrcpy(dest, "21");
		// draw data section 
		for(i = 0; i < src_len; i++) {
			lookup(NEON, MSITable, source[i], dest);
		}
		// calculate first check digit 
		wright = 0;
		n = !(src_len & 1);
		for(i = n; i < src_len; i += 2) {
			un[wright++] = source[i];
		}
		un[wright] = '\0';
		dau = strtoul(un, NULL, 10);
		dau *= 2;
		sprintf(tri, "%ld", dau);
		pedwar = 0;
		h = strlen(tri);
		for(i = 0; i < h; i++) {
			pedwar += hex(tri[i]);
		}
		n = src_len & 1;
		for(i = n; i < src_len; i += 2) {
			pedwar += hex(source[i]);
		}
		pump = 10 - pedwar % 10;
		if(pump == 10) {
			pump = 0;
		}
		// calculate second check digit 
		wright = 0;
		n = src_len & 1;
		for(i = n; i < src_len; i += 2) {
			un[wright++] = source[i];
		}
		un[wright++] = itoc(pump);
		un[wright] = '\0';

		dau = strtoul(un, NULL, 10);
		dau *= 2;

		sprintf(tri, "%ld", dau);

		pedwar = 0;
		h = strlen(tri);
		for(i = 0; i < h; i++) {
			pedwar += hex(tri[i]);
		}
		i = !(src_len & 1);
		for(; i < src_len; i += 2) {
			pedwar += hex(source[i]);
		}
		chwech = 10 - pedwar % 10;
		if(chwech == 10) {
			chwech = 0;
		}
		// Draw check digits 
		lookup(NEON, MSITable, itoc(pump), dest);
		lookup(NEON, MSITable, itoc(chwech), dest);
		// Stop character 
		strcat(dest, "121");
		expand(symbol, dest);
		sstrcpy(symbol->text, source);
		symbol->text[src_len] = itoc(pump);
		symbol->text[src_len + 1] = itoc(chwech);
		symbol->text[src_len + 2] = '\0';
	}
	return error_number;
}

/* Calculate a Modulo 11 check digit using the system discussed on Wikipedia -
    see http://en.wikipedia.org/wiki/Talk:MSI_Barcode */
static int msi_plessey_mod11(struct ZintSymbol * symbol, const uchar source[], const uint src_len)
{
	// uses the IBM weight system 
	int    error_number = 0;
	if(src_len > 55) {
		sstrcpy(symbol->errtxt, "Input too long (C75)");
		return ZINT_ERROR_TOO_LONG;
	}
	else {
		char   dest[1000];
		// start character 
		sstrcpy(dest, "21");
		// draw data section 
		{
			for(uint i = 0; i < src_len; i++) {
				lookup(NEON, MSITable, source[i], dest);
			}
		}
		{
			// calculate check digit 
			int    x = 0;
			{
				int    weight = 2;
				for(int i = src_len - 1; i >= 0; i--) {
					x += weight * hex(source[i]);
					weight++;
					if(weight > 7)
						weight = 2;
				}
			}
			{
				const int check = (11 - (x % 11)) % 11;
				if(check == 10) {
					lookup(NEON, MSITable, '1', dest);
					lookup(NEON, MSITable, '0', dest);
				}
				else {
					lookup(NEON, MSITable, itoc(check), dest);
				}
				// stop character 
				strcat(dest, "121");
				expand(symbol, dest);
				sstrcpy(symbol->text, source);
				if(check == 10) {
					strcat((char *)symbol->text, "10");
				}
				else {
					symbol->text[src_len] = itoc(check);
					symbol->text[src_len + 1] = '\0';
				}
			}
		}
		return error_number;
	}
}

/* Combining the Barcode Island and Wikipedia code
 * Verified against http://www.bokai.com/BarcodeJSP/applet/BarcodeSampleApplet.htm */
static int msi_plessey_mod1110(struct ZintSymbol * symbol, const uchar source[], const uint src_len)
{
	// Weighted using the IBM system 
	ulong i, weight, x, check, wright, dau, pedwar, pump, h;
	long si;
	char un[16], tri[16];
	char dest[1000];
	uchar temp[32];
	uint temp_len;
	int error_number = 0;
	if(src_len > 18) {
		sstrcpy(symbol->errtxt, "Input too long (C76)");
		return ZINT_ERROR_TOO_LONG;
	}
	else {
		// start character 
		sstrcpy(dest, "21");
		// draw data section 
		for(i = 0; i < src_len; i++) {
			lookup(NEON, MSITable, source[i], dest);
		}
		// calculate first (mod 11) digit 
		x = 0;
		weight = 2;
		for(si = src_len - 1; si >= 0; si--) {
			x += weight * hex(source[si]);
			weight++;
			if(weight > 7) {
				weight = 2;
			}
		}
		check = (11 - (x % 11)) % 11;
		sstrcpy(temp, source);
		temp_len = src_len;
		if(check == 10) {
			lookup(NEON, MSITable, '1', dest);
			lookup(NEON, MSITable, '0', dest);
			strcat((char *)temp, "10");
			temp_len += 2;
		}
		else {
			lookup(NEON, MSITable, itoc(check), dest);
			temp[temp_len++] = itoc(check);
			temp[temp_len] = '\0';
		}
		// calculate second (mod 10) check digit 
		wright = 0;
		i = !(temp_len & 1);
		for(; i < temp_len; i += 2) {
			un[wright++] = temp[i];
		}
		un[wright] = '\0';
		dau = strtoul(un, NULL, 10);
		dau *= 2;
		sprintf(tri, "%ld", dau);
		pedwar = 0;
		h = strlen(tri);
		for(i = 0; i < h; i++) {
			pedwar += hex(tri[i]);
		}
		i = temp_len & 1;
		for(; i < temp_len; i += 2) {
			pedwar += hex(temp[i]);
		}
		pump = 10 - pedwar % 10;
		if(pump == 10) {
			pump = 0;
		}
		// draw check digit 
		lookup(NEON, MSITable, itoc(pump), dest);
		// stop character 
		strcat(dest, "121");
		expand(symbol, dest);
		temp[temp_len++] = itoc(pump);
		temp[temp_len] = '\0';
		sstrcpy(symbol->text, temp);
	}
	return error_number;
}

int msi_handle(struct ZintSymbol * symbol, uchar source[], int length)
{
	int error_number = is_sane(NEON, source, length);
	if(error_number != 0) {
		sstrcpy(symbol->errtxt, "Invalid characters in input data (C77)");
		error_number = ZINT_ERROR_INVALID_DATA;
	}
	else {
		if((symbol->option_2 < 0) || (symbol->option_2 > 4)) {
			symbol->option_2 = 0;
		}
		switch(symbol->option_2) {
			case 0: error_number = msi_plessey(symbol, source, length); break;
			case 1: error_number = msi_plessey_mod10(symbol, source, length); break;
			case 2: error_number = msi_plessey_mod1010(symbol, source, length); break;
			case 3: error_number = msi_plessey_mod11(symbol, source, length); break;
			case 4: error_number = msi_plessey_mod1110(symbol, source, length); break;
		}
	}
	return error_number;
}

