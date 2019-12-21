/*  upcean.c - Handles UPC, EAN and ISBN

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

#define SODIUM  "0123456789+"
#define EAN2    102
#define EAN5    105
//
// UPC and EAN tables checked against EN 797:1996 
//
static const char * UPCParity0[10] = { // Number set for UPC-E symbol (EN Table 4) 
	"BBBAAA", "BBABAA", "BBAABA", "BBAAAB", "BABBAA", "BAABBA", "BAAABB",
	"BABABA", "BABAAB", "BAABAB"
};
static const char * UPCParity1[10] = { // Not covered by BS EN 797:1995 
	"AAABBB", "AABABB", "AABBAB", "AABBBA", "ABAABB", "ABBAAB", "ABBBAA",
	"ABABAB", "ABABBA", "ABBABA"
};
static const char * EAN2Parity[4] = { // Number sets for 2-digit add-on (EN Table 6) 
	"AA", "AB", "BA", "BB"
};
static const char * EAN5Parity[10] = { // Number set for 5-digit add-on (EN Table 7) 
	"BBAAA", "BABAA", "BAABA", "BAAAB", "ABBAA", "AABBA", "AAABB", "ABABA",
	"ABAAB", "AABAB"
};
static const char * EAN13Parity[10] = { // Left hand of the EAN-13 symbol (EN Table 3) 
	"AAAAA", "ABABB", "ABBAB", "ABBBA", "BAABB", "BBAAB", "BBBAA", "BABAB",
	"BABBA", "BBABA"
};
static const char * EANsetA[10] = { // Representation set A and C (EN Table 1) 
	"3211", "2221", "2122", "1411", "1132", "1231", "1114", "1312", "1213", "3112"
};
static const char * EANsetB[10] = { // Representation set B (EN Table 1) 
	"1123", "1222", "2212", "1141", "2311", "1321", "4111", "2131", "3121", "2113"
};

// Calculate the correct check digit for a UPC barcode 
char upc_check(char source[])
{
	uint   count = 0;
	const  uint _src_len = strlen(source);
	for(uint i = 0; i < _src_len; i++) {
		count += hex(source[i]);
		if(!(i & 1))
			count += 2 * (hex(source[i]));
	}
	{
		uint   check_digit = 10 - (count % 10);
		if(check_digit == 10)
			check_digit = 0;
		return itoc(check_digit);
	}
}
//
// UPC A is usually used for 12 digit numbers, but this function takes a source of any length 
//
void upca_draw(char source[], char dest[])
{
	uint   i;
	uint   half_way = strlen(source) / 2;
	strcat(dest, "111"); // start character 
	for(i = 0; i <= strlen(source); i++) {
		if(i == half_way) {
			// middle character - separates manufacturer no. from product no. 
			// also inverts right hand characters 
			strcat(dest, "11111");
		}
		lookup(NEON, EANsetA, source[i], dest);
	}
	strcat(dest, "111"); // stop character 
}
//
// Make a UPC A barcode when we haven't been given the check digit 
//
int upca(struct ZintSymbol * symbol, uchar source[], char dest[])
{
	int length;
	char gtin[15];
	sstrcpy(gtin, (char *)source);
	length = strlen(gtin);
	if(length == 11) {
		gtin[length] = upc_check(gtin);
		gtin[length + 1] = '\0';
	}
	else {
		gtin[length - 1] = '\0';
		if(source[length - 1] != upc_check(gtin)) {
			// @v10.6.5 sstrcpy(symbol->errtxt, "Invalid check digit (C60)");
			ZintMakeErrText_InvCheckDigit("C60", symbol->errtxt, sizeof(symbol->errtxt)); // @v10.6.5
			return ZINT_ERROR_INVALID_DATA;
		}
		gtin[length - 1] = upc_check(gtin);
	}
	upca_draw(gtin, dest);
	sstrcpy(symbol->text, (uchar *)gtin);
	return 0;
}
//
// UPC E is a zero-compressed version of UPC A 
//
int upce(struct ZintSymbol * symbol, uchar source[], char dest[])
{
	uint i, num_system;
	char emode, equivalent[12], check_digit, parity[8], temp[8];
	char hrt[9];
	// Two number systems can be used - system 0 and system 1 
	if(symbol->Std != BARCODE_UPCE_CHK) {
		// No check digit in input data 
		if(sstrlen(source) == 7) {
			switch(source[0]) {
				case '0': num_system = 0; break;
				case '1': num_system = 1; break;
				default: num_system = 0; source[0] = '0'; break;
			}
			sstrcpy(temp, (char *)source);
			sstrcpy(hrt, (char *)source);
			for(i = 1; i <= 7; i++) {
				source[i - 1] = temp[i];
			}
		}
		else {
			num_system = 0;
			hrt[0] = '0';
			hrt[1] = '\0';
			strcat(hrt, (char *)source);
		}
	}
	else {
		/* Check digit is included in input data */
		if(sstrlen(source) == 8) {
			switch(source[0]) {
				case '0': num_system = 0; break;
				case '1': num_system = 1; break;
				default: num_system = 0; source[0] = '0'; break;
			}
			sstrcpy(temp, (char *)source);
			sstrcpy(hrt, (char *)source);
			for(i = 1; i <= 7; i++) {
				source[i - 1] = temp[i];
			}
		}
		else {
			num_system = 0;
			hrt[0] = '0';
			hrt[1] = '\0';
			strcat(hrt, (char *)source);
		}
	}

	/* Expand the zero-compressed UPCE code to make a UPCA equivalent (EN Table 5) */
	emode = source[5];
	for(i = 0; i < 11; i++) {
		equivalent[i] = '0';
	}
	if(num_system == 1) {
		equivalent[0] = temp[0];
	}
	equivalent[1] = source[0];
	equivalent[2] = source[1];
	equivalent[11] = '\0';

	switch(emode) {
		case '0':
		case '1':
		case '2':
		    equivalent[3] = emode;
		    equivalent[8] = source[2];
		    equivalent[9] = source[3];
		    equivalent[10] = source[4];
		    break;
		case '3':
		    equivalent[3] = source[2];
		    equivalent[9] = source[3];
		    equivalent[10] = source[4];
		    if(((source[2] == '0') || (source[2] == '1')) || (source[2] == '2')) {
			    /* Note 1 - "X3 shall not be equal to 0, 1 or 2" */
			    sstrcpy(symbol->errtxt, "Invalid UPC-E data (C61)");
			    return ZINT_ERROR_INVALID_DATA;
		    }
		    break;
		case '4':
		    equivalent[3] = source[2];
		    equivalent[4] = source[3];
		    equivalent[10] = source[4];
		    if(source[3] == '0') {
			    /* Note 2 - "X4 shall not be equal to 0" */
			    sstrcpy(symbol->errtxt, "Invalid UPC-E data (C62)");
			    return ZINT_ERROR_INVALID_DATA;
		    }
		    break;
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		    equivalent[3] = source[2];
		    equivalent[4] = source[3];
		    equivalent[5] = source[4];
		    equivalent[10] = emode;
		    if(source[4] == '0') {
			    /* Note 3 - "X5 shall not be equal to 0" */
			    sstrcpy(symbol->errtxt, "Invalid UPC-E data (C63)");
			    return ZINT_ERROR_INVALID_DATA;
		    }
		    break;
	}

	/* Get the check digit from the expanded UPCA code */

	check_digit = upc_check(equivalent);

	/* Use the number system and check digit information to choose a parity scheme */
	if(num_system == 1) {
		sstrcpy(parity, UPCParity1[hex(check_digit)]);
	}
	else {
		sstrcpy(parity, UPCParity0[hex(check_digit)]);
	}

	/* Take all this information and make the barcode pattern */

	/* start character */
	strcat(dest, "111");
	for(i = 0; i <= sstrlen(source); i++) {
		switch(parity[i]) {
			case 'A': lookup(NEON, EANsetA, source[i], dest); break;
			case 'B': lookup(NEON, EANsetB, source[i], dest); break;
		}
	}
	/* stop character */
	strcat(dest, "111111");
	if(symbol->Std != BARCODE_UPCE_CHK) {
		hrt[7] = check_digit;
		hrt[8] = '\0';
	}
	else {
		if(hrt[7] != check_digit) {
			// @v10.6.5 sstrcpy(symbol->errtxt, "Invalid check digit (C64)");
			ZintMakeErrText_InvCheckDigit("C64", symbol->errtxt, sizeof(symbol->errtxt)); // @v10.6.5
			return ZINT_ERROR_INVALID_DATA;
		}
	}
	sstrcpy(symbol->text, (uchar *)hrt);
	return 0;
}

/* EAN-2 and EAN-5 add-on codes */
void add_on(uchar source[], char dest[], int mode)
{
	char parity[6];
	uint i, code_type;
	/* If an add-on then append with space */
	if(mode != 0) {
		strcat(dest, "9");
	}
	/* Start character */
	strcat(dest, "112");
	/* Determine EAN2 or EAN5 add-on */
	if(sstrlen(source) == 2) {
		code_type = EAN2;
	}
	else {
		code_type = EAN5;
	}
	/* Calculate parity for EAN2 */
	if(code_type == EAN2) {
		int code_value = (10 * hex(source[0])) + hex(source[1]);
		int parity_bit = code_value % 4;
		sstrcpy(parity, EAN2Parity[parity_bit]);
	}
	if(code_type == EAN5) {
		int values[6], parity_sum, parity_bit;
		for(i = 0; i < 6; i++) {
			values[i] = hex(source[i]);
		}
		parity_sum = (3 * (values[0] + values[2] + values[4]));
		parity_sum += (9 * (values[1] + values[3]));
		parity_bit = parity_sum % 10;
		sstrcpy(parity, EAN5Parity[parity_bit]);
	}
	for(i = 0; i < sstrlen(source); i++) {
		switch(parity[i]) {
			case 'A': lookup(NEON, EANsetA, source[i], dest); break;
			case 'B': lookup(NEON, EANsetB, source[i], dest); break;
		}
		/* Glyph separator */
		if(i != (sstrlen(source) - 1)) {
			strcat(dest, "11");
		}
	}
}

/* ************************ EAN-13 ****************** */

// Calculate the correct check digit for a EAN-13 barcode 
char ean_check(char source[])
{
	int i;
	uint check_digit;
	uint count = 0;
	uint h = strlen(source);
	for(i = h - 1; i >= 0; i--) {
		count += hex(source[i]);
		if(i & 1) {
			count += 2 * hex(source[i]);
		}
	}
	check_digit = 10 - (count % 10);
	if(check_digit == 10) {
		check_digit = 0;
	}
	return itoc(check_digit);
}

int ean13(struct ZintSymbol * symbol, uchar source[], char dest[])
{
	uint length, i, half_way;
	char parity[6];
	char gtin[15];
	sstrcpy(parity, "");
	sstrcpy(gtin, (char *)source);
	/* Add the appropriate check digit */
	length = strlen(gtin);
	if(length == 12) {
		gtin[length] = ean_check(gtin);
		gtin[length + 1] = '\0';
	}
	else {
		gtin[length - 1] = '\0';
		if(source[length - 1] != ean_check(gtin)) {
			// @v10.6.5 sstrcpy(symbol->errtxt, "Invalid check digit (C65)");
			ZintMakeErrText_InvCheckDigit("C65", symbol->errtxt, sizeof(symbol->errtxt)); // @v10.6.5
			return ZINT_ERROR_INVALID_DATA;
		}
		gtin[length - 1] = ean_check(gtin);
	}
	/* Get parity for first half of the symbol */
	lookup(SODIUM, EAN13Parity, gtin[0], parity);
	/* Now get on with the cipher */
	half_way = 7;
	/* start character */
	strcat(dest, "111");
	length = strlen(gtin);
	for(i = 1; i <= length; i++) {
		if(i == half_way) {
			/* middle character - separates manufacturer no. from product no. */
			/* also inverses right hand characters */
			strcat(dest, "11111");
		}
		if(((i > 1) && (i < 7)) && (parity[i-2] == 'B')) {
			lookup(NEON, EANsetB, gtin[i], dest);
		}
		else {
			lookup(NEON, EANsetA, gtin[i], dest);
		}
	}
	// stop character 
	strcat(dest, "111");
	sstrcpy(symbol->text, (uchar *)gtin);
	return 0;
}

/* Make an EAN-8 barcode when we haven't been given the check digit */
int ean8(struct ZintSymbol * symbol, uchar source[], char dest[])
{
	/* EAN-8 is basically the same as UPC-A but with fewer digits */
	int length;
	char gtin[10];
	sstrcpy(gtin, (char *)source);
	length = strlen(gtin);
	if(length == 7) {
		gtin[length] = upc_check(gtin);
		gtin[length + 1] = '\0';
	}
	else {
		gtin[length - 1] = '\0';
		if(source[length - 1] != upc_check(gtin)) {
			// @v10.6.5 sstrcpy(symbol->errtxt, "Invalid check digit (C66)");
			ZintMakeErrText_InvCheckDigit("C66", symbol->errtxt, sizeof(symbol->errtxt)); // @v10.6.5
			return ZINT_ERROR_INVALID_DATA;
		}
		gtin[length - 1] = upc_check(gtin);
	}
	upca_draw(gtin, dest);
	sstrcpy(symbol->text, (uchar *)gtin);
	return 0;
}

// For ISBN(13) only 
char isbn13_check(uchar source[])
{
	uint check;
	uint sum = 0;
	uint weight = 1;
	uint h = sstrlen(source) - 1;
	for(uint i = 0; i < h; i++) {
		sum += hex(source[i]) * weight;
		if(weight == 1) 
			weight = 3;
		else 
			weight = 1;
	}
	check = sum % 10;
	check = 10 - check;
	if(check == 10) 
		check = 0;
	return itoc(check);
}

/* For ISBN(10) and SBN only */
char isbn_check(uchar source[])
{
	uint check;
	char check_char;
	uint sum = 0;
	uint weight = 1;
	uint h = sstrlen(source) - 1;
	for(uint i = 0; i < h; i++) {
		sum += hex(source[i]) * weight;
		weight++;
	}
	check = sum % 11;
	check_char = itoc(check);
	if(check == 10) {
		check_char = 'X';
	}
	return check_char;
}

// Make an EAN-13 barcode from an SBN or ISBN 
int isbn(struct ZintSymbol * symbol, uchar source[], const uint src_len, char dest[])
{
	int i, error_number;
	char check_digit;
	to_upper(source);
	error_number = is_sane("0123456789X", source, src_len);
	if(error_number == ZINT_ERROR_INVALID_DATA) {
		sstrcpy(symbol->errtxt, "Invalid characters in input (C67)");
		return error_number;
	}
	// Input must be 9, 10 or 13 characters 
	if(((src_len < 9) || (src_len > 13)) || ((src_len > 10) && (src_len < 13))) {
		sstrcpy(symbol->errtxt, "Input wrong length (C68)");
		return ZINT_ERROR_TOO_LONG;
	}
	if(src_len == 13) { /* Using 13 character ISBN */
		if(!(((source[0] == '9') && (source[1] == '7')) && ((source[2] == '8') || (source[2] == '9')))) {
			sstrcpy(symbol->errtxt, "Invalid ISBN (C69)");
			return ZINT_ERROR_INVALID_DATA;
		}
		check_digit = isbn13_check(source);
		if(source[src_len - 1] != check_digit) {
			sstrcpy(symbol->errtxt, "Incorrect ISBN check (C6A)");
			return ZINT_ERROR_INVALID_CHECK;
		}
		source[12] = '\0';
		ean13(symbol, source, dest);
	}
	if(src_len == 10) { /* Using 10 digit ISBN */
		check_digit = isbn_check(source);
		if(check_digit != source[src_len - 1]) {
			sstrcpy(symbol->errtxt, "Incorrect ISBN check (C6B)");
			return ZINT_ERROR_INVALID_CHECK;
		}
		for(i = 13; i > 0; i--) {
			source[i] = source[i - 3];
		}
		source[0] = '9';
		source[1] = '7';
		source[2] = '8';
		source[12] = '\0';
		ean13(symbol, source, dest);
	}
	if(src_len == 9) { // Using 9 digit SBN 
		// Add leading zero 
		for(i = 10; i > 0; i--) {
			source[i] = source[i - 1];
		}
		source[0] = '0';
		// Verify check digit
		check_digit = isbn_check(source);
		if(check_digit != source[sstrlen(source) - 1]) {
			sstrcpy(symbol->errtxt, "Incorrect SBN check (C6C)");
			return ZINT_ERROR_INVALID_CHECK;
		}
		// Convert to EAN-13 number 
		for(i = 13; i > 0; i--) {
			source[i] = source[i - 3];
		}
		source[0] = '9';
		source[1] = '7';
		source[2] = '8';
		source[12] = '\0';
		ean13(symbol, source, dest);
	}
	return 0;
}

// Add leading zeroes to EAN and UPC strings 
static void ean_leading_zeroes(struct ZintSymbol * symbol, const uchar source[], uchar local_source[])
{
	uchar first_part[20], second_part[20], zfirst_part[20], zsecond_part[20];
	int with_addon = 0;
	int first_len = 0, second_len = 0, zfirst_len = 0, zsecond_len = 0, i;
	int h = sstrlen(source);
	for(i = 0; i < h; i++) {
		if(source[i] == '+') {
			with_addon = 1;
		}
		else {
			if(with_addon == 0) {
				first_len++;
			}
			else {
				second_len++;
			}
		}
	}
	sstrcpy(first_part, (uchar *)"");
	sstrcpy(second_part, (uchar *)"");
	sstrcpy(zfirst_part, (uchar *)"");
	sstrcpy(zsecond_part, (uchar *)"");
	// Split input into two strings 
	for(i = 0; i < first_len; i++) {
		first_part[i] = source[i];
		first_part[i + 1] = '\0';
	}
	for(i = 0; i < second_len; i++) {
		second_part[i] = source[i + first_len + 1];
		second_part[i + 1] = '\0';
	}
	// Calculate target lengths 
	if(second_len <= 5) {
		zsecond_len = 5;
	}
	if(second_len <= 2) {
		zsecond_len = 2;
	}
	if(second_len == 0) {
		zsecond_len = 0;
	}
	switch(symbol->Std) {
		case BARCODE_EANX:
		case BARCODE_EANX_CC:
		    if(first_len <= 12) {
			    zfirst_len = 12;
		    }
		    if(first_len <= 7) {
			    zfirst_len = 7;
		    }
		    if(second_len == 0) {
			    if(first_len <= 5) {
				    zfirst_len = 5;
			    }
			    if(first_len <= 2) {
				    zfirst_len = 2;
			    }
		    }
		    break;
		case BARCODE_EANX_CHK:
		    if(first_len <= 13) {
			    zfirst_len = 13;
		    }
		    if(first_len <= 8) {
			    zfirst_len = 8;
		    }
		    if(second_len == 0) {
			    if(first_len <= 5) {
				    zfirst_len = 5;
			    }
			    if(first_len <= 2) {
				    zfirst_len = 2;
			    }
		    }
		    break;
		case BARCODE_UPCA:
		case BARCODE_UPCA_CC:
		    zfirst_len = 11;
		    break;
		case BARCODE_UPCA_CHK:
		    zfirst_len = 12;
		    break;
		case BARCODE_UPCE:
		case BARCODE_UPCE_CC:
		    if(first_len == 7) {
			    zfirst_len = 7;
		    }
		    if(first_len <= 6) {
			    zfirst_len = 6;
		    }
		    break;
		case BARCODE_UPCE_CHK:
		    if(first_len == 8) {
			    zfirst_len = 8;
		    }
		    if(first_len <= 7) {
			    zfirst_len = 7;
		    }
		    break;
		case BARCODE_ISBNX:
		    if(first_len <= 9) {
			    zfirst_len = 9;
		    }
		    break;
	}
	// Add leading zeroes 
	for(i = 0; i < (zfirst_len - first_len); i++) {
		strcat((char *)zfirst_part, "0");
	}
	strcat((char *)zfirst_part, (char *)first_part);
	for(i = 0; i < (zsecond_len - second_len); i++) {
		strcat((char *)zsecond_part, "0");
	}
	strcat((char *)zsecond_part, (char *)second_part);
	// Copy adjusted data back to local_source 
	strcat((char *)local_source, (char *)zfirst_part);
	if(zsecond_len != 0) {
		strcat((char *)local_source, "+");
		strcat((char *)local_source, (char *)zsecond_part);
	}
}
//
// splits string to parts before and after '+' parts 
//
int eanx(struct ZintSymbol * symbol, const uchar source[], int src_len)
{
	uchar  first_part[20] = {0};
	uchar  second_part[20] = {0};
	uchar  dest[1000] = {0};
	uchar  local_source[20] = {0};
	uint   reader;
	int    error_number, i;
	uint   with_addon = FALSE;
	uint   latch = FALSE;
	uint   writer = 0;
	if(src_len > 19) {
		sstrcpy(symbol->errtxt, "Input too long (C6D)");
		return ZINT_ERROR_TOO_LONG;
	}
	if(symbol->Std != BARCODE_ISBNX) {
		// ISBN has it's own checking routine 
		error_number = is_sane("0123456789+", source, src_len);
		if(error_number == ZINT_ERROR_INVALID_DATA) {
			// @v10.6.5 sstrcpy(symbol->errtxt, "Invalid characters in data (C6E)");
			ZintMakeErrText_InvCharInData("C6E", symbol->errtxt, sizeof(symbol->errtxt)); // @v10.6.5
			return error_number;
		}
	}
	else {
		error_number = is_sane("0123456789Xx", source, src_len);
		if(error_number == ZINT_ERROR_INVALID_DATA) {
			sstrcpy(symbol->errtxt, "Invalid characters in input (C6F)");
			return error_number;
		}
	}
	// Add leading zeroes 
	sstrcpy(local_source, (uchar *)"");
	if(symbol->Std == BARCODE_ISBNX) {
		to_upper(local_source);
	}
	ean_leading_zeroes(symbol, source, local_source);
	for(reader = 0; reader <= sstrlen(local_source); reader++) {
		if(source[reader] == '+') {
			with_addon = TRUE;
		}
	}
	reader = 0;
	if(with_addon) {
		do {
			if(local_source[reader] == '+') {
				first_part[writer] = '\0';
				latch = TRUE;
				reader++;
				writer = 0;
			}
			if(latch) {
				second_part[writer] = local_source[reader];
				reader++;
				writer++;
			}
			else {
				first_part[writer] = local_source[reader];
				reader++;
				writer++;
			}
		} while(reader <= sstrlen(local_source));
	}
	else {
		sstrcpy(first_part, local_source);
	}
	switch(symbol->Std) {
		case BARCODE_EANX:
		case BARCODE_EANX_CHK:
		    switch(sstrlen(first_part)) {
			    case 2: add_on(first_part, (char *)dest, 0); sstrcpy(symbol->text, first_part); break;
			    case 5: add_on(first_part, (char *)dest, 0); sstrcpy(symbol->text, first_part); break;
			    case 7:
			    case 8: error_number = ean8(symbol, first_part, (char *)dest); break;
			    case 12:
			    case 13: error_number = ean13(symbol, first_part, (char *)dest); break;
			    default: sstrcpy(symbol->errtxt, "Invalid length input (C6G)"); return ZINT_ERROR_TOO_LONG;
		    }
		    break;
		case BARCODE_EANX_CC:
		    switch(sstrlen(first_part)) { /* Adds vertical separator bars according to ISO/IEC 24723 section 11.4 */
			    case 7: set_module(symbol, symbol->rows, 1);
				set_module(symbol, symbol->rows, 67);
				set_module(symbol, symbol->rows + 1, 0);
				set_module(symbol, symbol->rows + 1, 68);
				set_module(symbol, symbol->rows + 2, 1);
				set_module(symbol, symbol->rows + 1, 67);
				symbol->row_height[symbol->rows] = 2;
				symbol->row_height[symbol->rows + 1] = 2;
				symbol->row_height[symbol->rows + 2] = 2;
				symbol->rows += 3;
				error_number = ean8(symbol, first_part, (char *)dest);
				break;
			    case 12: set_module(symbol, symbol->rows, 1);
				set_module(symbol, symbol->rows, 95);
				set_module(symbol, symbol->rows + 1, 0);
				set_module(symbol, symbol->rows + 1, 96);
				set_module(symbol, symbol->rows + 2, 1);
				set_module(symbol, symbol->rows + 2, 95);
				symbol->row_height[symbol->rows] = 2;
				symbol->row_height[symbol->rows + 1] = 2;
				symbol->row_height[symbol->rows + 2] = 2;
				symbol->rows += 3;
				error_number = ean13(symbol, first_part, (char *)dest);
				break;
			    default: sstrcpy(symbol->errtxt, "Invalid length EAN input (C6H)");
				return ZINT_ERROR_TOO_LONG;
		    }
		    break;
		case BARCODE_UPCA:
		case BARCODE_UPCA_CHK:
		    if((sstrlen(first_part) == 11) || (sstrlen(first_part) == 12)) {
			    error_number = upca(symbol, first_part, (char *)dest);
		    }
		    else {
			    sstrcpy(symbol->errtxt, "Input wrong length (C6I)");
			    return ZINT_ERROR_TOO_LONG;
		    }
		    break;
		case BARCODE_UPCA_CC:
		    if(sstrlen(first_part) == 11) {
			    set_module(symbol, symbol->rows, 1);
			    set_module(symbol, symbol->rows, 95);
			    set_module(symbol, symbol->rows + 1, 0);
			    set_module(symbol, symbol->rows + 1, 96);
			    set_module(symbol, symbol->rows + 2, 1);
			    set_module(symbol, symbol->rows + 2, 95);
			    symbol->row_height[symbol->rows] = 2;
			    symbol->row_height[symbol->rows + 1] = 2;
			    symbol->row_height[symbol->rows + 2] = 2;
			    symbol->rows += 3;
			    error_number = upca(symbol, first_part, (char *)dest);
		    }
		    else {
			    sstrcpy(symbol->errtxt, "UPCA input wrong length (C6J)");
			    return ZINT_ERROR_TOO_LONG;
		    }
		    break;
		case BARCODE_UPCE:
		case BARCODE_UPCE_CHK:
		    if((sstrlen(first_part) >= 6) && (sstrlen(first_part) <= 8)) {
			    error_number = upce(symbol, first_part, (char *)dest);
		    }
		    else {
			    sstrcpy(symbol->errtxt, "Input wrong length (C6K)");
			    return ZINT_ERROR_TOO_LONG;
		    }
		    break;
		case BARCODE_UPCE_CC:
		    if((sstrlen(first_part) >= 6) && (sstrlen(first_part) <= 7)) {
			    set_module(symbol, symbol->rows, 1);
			    set_module(symbol, symbol->rows, 51);
			    set_module(symbol, symbol->rows + 1, 0);
			    set_module(symbol, symbol->rows + 1, 52);
			    set_module(symbol, symbol->rows + 2, 1);
			    set_module(symbol, symbol->rows + 2, 51);
			    symbol->row_height[symbol->rows] = 2;
			    symbol->row_height[symbol->rows + 1] = 2;
			    symbol->row_height[symbol->rows + 2] = 2;
			    symbol->rows += 3;
			    error_number = upce(symbol, first_part, (char *)dest);
		    }
		    else {
			    sstrcpy(symbol->errtxt, "UPCE input wrong length (C6L)");
			    return ZINT_ERROR_TOO_LONG;
		    }
		    break;
		case BARCODE_ISBNX:
		    error_number = isbn(symbol, first_part, sstrlen(first_part), (char *)dest);
		    break;
	}
	if(error_number > 4) {
		return error_number;
	}
	switch(sstrlen(second_part)) {
		case 0: break;
		case 2:
		    add_on(second_part, (char *)dest, 1);
		    strcat((char *)symbol->text, "+");
		    strcat((char *)symbol->text, (char *)second_part);
		    break;
		case 5:
		    add_on(second_part, (char *)dest, 1);
		    strcat((char *)symbol->text, "+");
		    strcat((char *)symbol->text, (char *)second_part);
		    break;
		default:
		    sstrcpy(symbol->errtxt, "Invalid length input (C6M)");
		    return ZINT_ERROR_TOO_LONG;
	}
	expand(symbol, (char *)dest);
	switch(symbol->Std) {
		case BARCODE_EANX_CC:
		case BARCODE_UPCA_CC:
		case BARCODE_UPCE_CC:
			// shift the symbol to the right one space to allow for separator bars 
		    for(i = (symbol->width + 1); i >= 1; i--) {
			    if(module_is_set(symbol, symbol->rows - 1, i - 1)) {
				    set_module(symbol, symbol->rows - 1, i);
			    }
			    else {
				    unset_module(symbol, symbol->rows - 1, i);
			    }
		    }
		    unset_module(symbol, symbol->rows - 1, 0);
		    symbol->width += 2;
		    break;
	}
	return 0;
}

