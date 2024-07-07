/* 2of5.c - Handles Code 2 of 5 barcodes */

/*
    libzint - the open source barcode library
    Copyright (C) 2008-2016 Robin Stuart <rstuart114@gmail.com>

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

static const char * C25MatrixTable[10] = {
	"113311", "311131", "131131", "331111", "113131", "313111",
	"133111", "111331", "311311", "131311"
};

static const char * C25IndustTable[10] = {
	"1111313111", "3111111131", "1131111131", "3131111111", "1111311131",
	"3111311111", "1131311111", "1111113131", "3111113111", "1131113111"
};

static const char * C25InterTable[10] = {
	"11331", "31113", "13113", "33111", "11313", "31311", "13311", "11133",
	"31131", "13131"
};

static inline char check_digit(uint count)
{
	return itoc((10 - (count % 10)) % 10);
}

/* Code 2 of 5 Standard (Code 2 of 5 Matrix) */
int matrix_two_of_five(struct ZintSymbol * symbol, const uchar source[], int length)
{
	int i, error_number;
	char dest[512]; /* 6 + 80 * 6 + 6 + 1 ~ 512*/
	if(length > 80) {
		sstrcpy(symbol->errtxt, "Input too long (C01)");
		return ZINT_ERROR_TOO_LONG;
	}
	error_number = is_sane(NEON, source, length);
	if(error_number == ZINT_ERROR_INVALID_DATA) {
		ZintMakeErrText_InvCharInData("C02", symbol->errtxt, sizeof(symbol->errtxt));
		return error_number;
	}
	/* start character */
	sstrcpy(dest, "411111");
	for(i = 0; i < length; i++) {
		lookup(NEON, C25MatrixTable, source[i], dest);
	}
	/* Stop character */
	strcat(dest, "41111");
	expand(symbol, dest);
	sstrcpy(symbol->text, source);
	return error_number;
}

/* Code 2 of 5 Industrial */
int industrial_two_of_five(struct ZintSymbol * symbol, const uchar source[], int length)
{
	int i, error_number;
	char dest[512]; /* 6 + 40 * 10 + 6 + 1 */
	if(length > 45) {
		sstrcpy(symbol->errtxt, "Input too long (C03)");
		return ZINT_ERROR_TOO_LONG;
	}
	error_number = is_sane(NEON, source, length);
	if(error_number == ZINT_ERROR_INVALID_DATA) {
		sstrcpy(symbol->errtxt, "Invalid character in data (C04)");
		return error_number;
	}
	/* start character */
	sstrcpy(dest, "313111");
	for(i = 0; i < length; i++) {
		lookup(NEON, C25IndustTable, source[i], dest);
	}
	/* Stop character */
	strcat(dest, "31113");
	expand(symbol, dest);
	sstrcpy(symbol->text, source);
	return error_number;
}

/* Code 2 of 5 IATA */
int iata_two_of_five(struct ZintSymbol * symbol, const uchar source[], int length)
{
	int i, error_number;
	char dest[512]; /* 4 + 45 * 10 + 3 + 1 */
	if(length > 45) {
		sstrcpy(symbol->errtxt, "Input too long (C05)");
		return ZINT_ERROR_TOO_LONG;
	}
	error_number = is_sane(NEON, source, length);
	if(error_number == ZINT_ERROR_INVALID_DATA) {
		ZintMakeErrText_InvCharInData("C06", symbol->errtxt, sizeof(symbol->errtxt));
		return error_number;
	}
	/* start */
	sstrcpy(dest, "1111");
	for(i = 0; i < length; i++) {
		lookup(NEON, C25IndustTable, source[i], dest);
	}
	/* stop */
	strcat(dest, "311");
	expand(symbol, dest);
	sstrcpy(symbol->text, source);
	return error_number;
}

/* Code 2 of 5 Data Logic */
int logic_two_of_five(struct ZintSymbol * symbol, const uchar source[], int length)
{
	int i, error_number;
	char dest[512]; /* 4 + 80 * 6 + 3 + 1 */
	if(length > 80) {
		sstrcpy(symbol->errtxt, "Input too long (C07)");
		return ZINT_ERROR_TOO_LONG;
	}
	error_number = is_sane(NEON, source, length);
	if(error_number == ZINT_ERROR_INVALID_DATA) {
		ZintMakeErrText_InvCharInData("C08", symbol->errtxt, sizeof(symbol->errtxt));
		return error_number;
	}
	/* start character */
	sstrcpy(dest, "1111");
	for(i = 0; i < length; i++) {
		lookup(NEON, C25MatrixTable, source[i], dest);
	}
	/* Stop character */
	strcat(dest, "311");
	expand(symbol, dest);
	sstrcpy(symbol->text, source);
	return error_number;
}

/* Code 2 of 5 Interleaved */
int interleaved_two_of_five(struct ZintSymbol * symbol, const uchar source[], size_t length)
{
	int    j, k, error_number;
	char   bars[7], spaces[7], mixed[14], dest[1000];
#ifndef _MSC_VER
	uchar temp[length + 2];
#else
	uchar * temp = (uchar *)_alloca((length + 2) * sizeof(uchar));
#endif
	if(length > 89) {
		sstrcpy(symbol->errtxt, "Input too long (C09)");
		return ZINT_ERROR_TOO_LONG;
	}
	error_number = is_sane(NEON, source, length);
	if(error_number == ZINT_ERROR_INVALID_DATA) {
		ZintMakeErrText_InvCharInData("C0A", symbol->errtxt, sizeof(symbol->errtxt));
		return error_number;
	}
	sstrcpy(temp, (uchar *)"");
	/* Input must be an even number of characters for Interlaced 2 of 5 to work: if an odd number of characters has been entered then add a leading zero */
	if(length & 1) {
		sstrcpy(temp, (uchar *)"0");
		length++;
	}
	strcat((char *)temp, (char *)source);
	/* start character */
	sstrcpy(dest, "1111");
	{
		for(size_t ti_ = 0; ti_ < length; ti_ += 2) {
			/* look up the bars and the spaces and put them in two strings */
			sstrcpy(bars, "");
			lookup(NEON, C25InterTable, temp[ti_], bars);
			sstrcpy(spaces, "");
			lookup(NEON, C25InterTable, temp[ti_ + 1], spaces);
			/* then merge (interlace) the strings together */
			k = 0;
			for(j = 0; j <= 4; j++) {
				mixed[k] = bars[j];
				k++;
				mixed[k] = spaces[j];
				k++;
			}
			mixed[k] = '\0';
			strcat(dest, mixed);
		}
	}
	/* Stop character */
	strcat(dest, "311");
	expand(symbol, dest);
	sstrcpy(symbol->text, temp);
	return error_number;
}

/* Interleaved 2-of-5 (ITF) */
int itf14(struct ZintSymbol * symbol, const uchar source[], int length)
{
	int    i, error_number, zeroes;
	char   localstr[16];
	uint   count = 0;
	if(length > 13) {
		sstrcpy(symbol->errtxt, "Input too long (C0B)");
		return ZINT_ERROR_TOO_LONG;
	}
	error_number = is_sane(NEON, source, length);
	if(error_number == ZINT_ERROR_INVALID_DATA) {
		sstrcpy(symbol->errtxt, "Invalid character in data (C0D)");
		return error_number;
	}
	/* Add leading zeros as required */
	zeroes = 13 - length;
	for(i = 0; i < zeroes; i++) {
		localstr[i] = '0';
	}
	sstrcpy(localstr + zeroes, reinterpret_cast<const char *>(source));
	/* Calculate the check digit - the same method used for EAN-13 */
	for(i = 12; i >= 0; i--) {
		count += hex(localstr[i]);
		if(!(i & 1)) {
			count += 2 * hex(localstr[i]);
		}
	}
	localstr[13] = check_digit(count);
	localstr[14] = '\0';
	error_number = interleaved_two_of_five(symbol, (uchar *)localstr, strlen(localstr));
	sstrcpy(symbol->text, (uchar *)localstr);
	return error_number;
}

/* Deutshe Post Leitcode */
int dpleit(struct ZintSymbol * symbol, const uchar source[], int length)
{
	int    i, error_number;
	char   localstr[16];
	int    zeroes;
	uint   count = 0;
	if(length > 13) {
		sstrcpy(symbol->errtxt, "Input wrong length (C0E)");
		return ZINT_ERROR_TOO_LONG;
	}
	error_number = is_sane(NEON, source, length);
	if(error_number == ZINT_ERROR_INVALID_DATA) {
		ZintMakeErrText_InvCharInData("C0D", symbol->errtxt, sizeof(symbol->errtxt));
		return error_number;
	}
	zeroes = 13 - length;
	for(i = 0; i < zeroes; i++)
		localstr[i] = '0';
	sstrcpy(localstr + zeroes, (char *)source);
	for(i = 12; i >= 0; i--) {
		count += 4 * hex(localstr[i]);
		if(i & 1) {
			count += 5 * hex(localstr[i]);
		}
	}
	localstr[13] = check_digit(count);
	localstr[14] = '\0';
	error_number = interleaved_two_of_five(symbol, (uchar *)localstr, strlen(localstr));
	sstrcpy(symbol->text, (uchar *)localstr);
	return error_number;
}

/* Deutsche Post Identcode */
int dpident(struct ZintSymbol * symbol, const uchar source[], int length)
{
	int    error_number;
	int    i, zeroes;
	char   localstr[16];
	uint   count = 0;
	if(length > 11) {
		sstrcpy(symbol->errtxt, "Input wrong length (C0E)");
		return ZINT_ERROR_TOO_LONG;
	}
	error_number = is_sane(NEON, source, length);
	if(error_number == ZINT_ERROR_INVALID_DATA) {
		ZintMakeErrText_InvCharInData("C0F", symbol->errtxt, sizeof(symbol->errtxt));
		return error_number;
	}
	zeroes = 11 - length;
	for(i = 0; i < zeroes; i++)
		localstr[i] = '0';
	sstrcpy(localstr + zeroes, (char *)source);
	for(i = 10; i >= 0; i--) {
		count += 4 * hex(localstr[i]);
		if(i & 1) {
			count += 5 * hex(localstr[i]);
		}
	}
	localstr[11] = check_digit(count);
	localstr[12] = '\0';
	error_number = interleaved_two_of_five(symbol, (uchar *)localstr, strlen(localstr));
	sstrcpy(symbol->text, (uchar *)localstr);
	return error_number;
}

