/* library.c - external functions of libzint

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
//#include "gs1.h"

#define TECHNETIUM      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%"

struct ZintSymbol * ZBarcode_Create()
{
	struct ZintSymbol * symbol = static_cast<struct ZintSymbol *>(SAlloc::M(sizeof(*symbol)));
	if(symbol) {
		memzero(symbol, sizeof(*symbol));
		symbol->Std = BARCODE_CODE128;
		symbol->height = 50;
		symbol->whitespace_width = 0;
		symbol->border_width = 0;
		symbol->output_options = 0;
		symbol->rows = 0;
		symbol->width = 0;
		// @sobolev sstrcpy(symbol->fgcolour, "000000");
		// @sobolev sstrcpy(symbol->bgcolour, "ffffff");
		symbol->ColorFg = SClrBlack;
		symbol->ColorBg = SClrWhite;
		sstrcpy(symbol->outfile, "");
		symbol->scale = 1.0;
		symbol->option_1 = -1;
		symbol->option_2 = 0;
		symbol->option_3 = 928; // PDF_MAX
		symbol->show_hrt = 1; // Show human readable text
		symbol->input_mode = DATA_MODE;
		sstrcpy(symbol->primary, "");
		memzero(&(symbol->encoded_data[0][0]), sizeof(symbol->encoded_data));
		memzero(&(symbol->row_height[0]), sizeof(symbol->row_height));
		symbol->bitmap = NULL;
		symbol->bitmap_width = 0;
		symbol->bitmap_height = 0;
		symbol->eci = 3;
		symbol->dot_size = 4.0f / 5.0f;
	}
	return symbol;
}

void ZBarcode_Clear(struct ZintSymbol * symbol)
{
	for(int i = 0; i < symbol->rows; i++) {
		for(int j = 0; j < symbol->width; j++) {
			unset_module(symbol, i, j);
		}
	}
	symbol->rows = 0;
	symbol->width = 0;
	memzero(symbol->text, sizeof(symbol->text));
	symbol->errtxt[0] = '\0';
	ZFREE(symbol->bitmap);
	symbol->bitmap_width = 0;
	symbol->bitmap_height = 0;
}

void ZBarcode_Delete(struct ZintSymbol * symbol)
{
	SAlloc::F(symbol->bitmap);
	// If there is a rendered version, ensure its memory is released
	if(symbol->rendered != NULL) {
		struct zint_render_line * line, * l;
		struct zint_render_string * string, * s;
		struct zint_render_ring * ring, * r;
		struct zint_render_hexagon * hexagon, * h;
		// Free lines
		line = symbol->rendered->lines;
		while(line) {
			l = line;
			line = line->next;
			SAlloc::F(l);
		}
		// Free Strings
		string = symbol->rendered->strings;
		while(string) {
			s = string;
			string = string->next;
			SAlloc::F(s->text);
			SAlloc::F(s);
		}
		// Free Rings
		ring = symbol->rendered->rings;
		while(ring) {
			r = ring;
			ring = ring->next;
			SAlloc::F(r);
		}
		// Free Hexagons
		hexagon = symbol->rendered->hexagons;
		while(hexagon) {
			h = hexagon;
			hexagon = hexagon->next;
			SAlloc::F(h);
		}
		// Free Render
		SAlloc::F(symbol->rendered);
	}
	SAlloc::F(symbol);
}

extern int get_best_eci(uchar source[], int length); /* Calculate suitable ECI mode */
extern int utf_to_eci(int eci, const uchar source[], uchar dest[], int * length); /* Convert Unicode to other encodings */

extern int eanx(struct ZintSymbol * symbol, const uchar source[], int length); /* EAN system barcodes */
extern int c39(struct ZintSymbol * symbol, uchar source[], const size_t length); /* Code 3 from 9 (or Code 39) */
extern int pharmazentral(struct ZintSymbol * symbol, uchar source[], int length); /* Pharmazentral Nummer (PZN) */
extern int ec39(struct ZintSymbol * symbol, const uchar source[], int length); /* Extended Code 3 from 9 (or Code 39+) */
extern int codabar(struct ZintSymbol * symbol, uchar source[], int length); /* Codabar - a simple substitution cipher */
extern int matrix_two_of_five(struct ZintSymbol * symbol, const uchar source[], int length); /* Code 2 of 5 Standard (& Matrix) */
extern int industrial_two_of_five(struct ZintSymbol * symbol, const uchar source[], int length); /* Code 2 of 5 Industrial */
extern int iata_two_of_five(struct ZintSymbol * symbol, const uchar source[], int length); /* Code 2 of 5 IATA */
extern int interleaved_two_of_five(struct ZintSymbol * symbol, const uchar source[], size_t length); /* Code 2 of 5 Interleaved */
extern int logic_two_of_five(struct ZintSymbol * symbol, const uchar source[], int length); /* Code 2 of 5 Data Logic */
extern int itf14(struct ZintSymbol * symbol, const uchar source[], int length); /* ITF-14 */
extern int dpleit(struct ZintSymbol * symbol, const uchar source[], int length); /* Deutsche Post Leitcode */
extern int dpident(struct ZintSymbol * symbol, const uchar source[], int length); /* Deutsche Post Identcode */
extern int c93(struct ZintSymbol * symbol, const uchar source[], int length); /* Code 93 - a re-working of Code 39+, generates 2 check digits */
extern int code_128(struct ZintSymbol * symbol, const uchar source[], int length); /* Code 128 and NVE-18 */
extern int ean_128(struct ZintSymbol * symbol, const uchar source[], const size_t length); /* EAN-128 (GS1-128) */
extern int code_11(struct ZintSymbol * symbol, const uchar source[], int length); /* Code 11 */
extern int msi_handle(struct ZintSymbol * symbol, uchar source[], int length); /* MSI Plessey */
extern int telepen(struct ZintSymbol * symbol, uchar source[], int length); /* Telepen ASCII */
extern int telepen_num(struct ZintSymbol * symbol, uchar source[], int length); /* Telepen Numeric */
extern int plessey(struct ZintSymbol * symbol, const uchar source[], int length); /* Plessey Code */
extern int pharma_one(struct ZintSymbol * symbol, const uchar source[], int length); /* Pharmacode One Track */
extern int flattermarken(struct ZintSymbol * symbol, uchar source[], int length); /* Flattermarken */
extern int fim(struct ZintSymbol * symbol, const uchar source[], int length); /* Facing Identification Mark */
extern int pharma_two(struct ZintSymbol * symbol, uchar source[], int length); /* Pharmacode Two Track */
extern int post_plot(struct ZintSymbol * symbol, uchar source[], int length); /* Postnet */
extern int planet_plot(struct ZintSymbol * symbol, uchar source[], int length); /* PLANET */
extern int imail(struct ZintSymbol * symbol, const uchar source[], int length); /* Intelligent Mail (aka USPS OneCode) */
extern int royal_plot(struct ZintSymbol * symbol, uchar source[], int length); /* RM4SCC */
extern int australia_post(struct ZintSymbol * symbol, uchar source[], int length); /* Australia Post 4-state */
extern int code16k(struct ZintSymbol * symbol, const uchar source[], int length); /* Code 16k */
extern int pdf417enc(struct ZintSymbol * symbol, uchar source[], int length); /* PDF417 */
extern int dmatrix(struct ZintSymbol * symbol, const uchar source[], int length); /* Data Matrix (IEC16022) */
extern int qr_code(struct ZintSymbol * symbol, const uchar source[], int length); /* QR Code */
extern int micro_pdf417(struct ZintSymbol * symbol, uchar chaine[], int length); /* Micro PDF417 */
extern int maxicode(struct ZintSymbol * symbol, uchar source[], int length); /* Maxicode */
extern int rss14(struct ZintSymbol * symbol, uchar source[], int length); /* RSS-14 */
extern int rsslimited(struct ZintSymbol * symbol, uchar source[], int length); /* RSS Limited */
extern int rssexpanded(struct ZintSymbol * symbol, const uchar source[], int length); /* RSS Expanded */
extern int composite(struct ZintSymbol * symbol, uchar source[], int length); /* Composite Symbology */
extern int kix_code(struct ZintSymbol * symbol, uchar source[], int length); /* TNT KIX Code */
extern int aztec(struct ZintSymbol * symbol, uchar source[], int length); /* Aztec Code */
extern int code32(struct ZintSymbol * symbol, uchar source[], int length); /* Italian Pharmacode */
extern int daft_code(struct ZintSymbol * symbol, uchar source[], int length); /* DAFT Code */
extern int ean_14(struct ZintSymbol * symbol, const uchar source[], int length); /* EAN-14 */
extern int nve_18(struct ZintSymbol * symbol, uchar source[], int length); /* NVE-18 */
extern int microqr(struct ZintSymbol * symbol, const uchar source[], int length); /* Micro QR Code */
extern int aztec_runes(struct ZintSymbol * symbol, const uchar source[], int length); /* Aztec Runes */
extern int korea_post(struct ZintSymbol * symbol, uchar source[], int length); /* Korea Post */
extern int japan_post(struct ZintSymbol * symbol, uchar source[], int length); /* Japanese Post */
extern int code_49(struct ZintSymbol * symbol, const uchar source[], const int length); /* Code 49 */
extern int channel_code(struct ZintSymbol * symbol, const uchar source[], int length); /* Channel Code */
extern int code_one(struct ZintSymbol * symbol, const uchar source[], int length); /* Code One */
extern int grid_matrix(struct ZintSymbol * symbol, const uchar source[], int length); /* Grid Matrix */
extern int han_xin(struct ZintSymbol * symbol, const uchar source[], int length); /* Han Xin */
extern int dotcode(struct ZintSymbol * symbol, const uchar source[], int length); /* DotCode */
extern int codablock(struct ZintSymbol * symbol, uchar source[], int length); /* Codablock */
extern int plot_raster(struct ZintSymbol * symbol, int rotate_angle, int file_type); /* Plot to PNG/BMP/PCX */
extern int render_plot(struct ZintSymbol * symbol, float width, float height); /* Plot to gLabels */
extern int ps_plot(struct ZintSymbol * symbol); /* Plot to EPS */
extern int svg_plot(struct ZintSymbol * symbol); /* Plot to SVG */

void error_tag(char error_string[], int error_number)
{
	char error_buffer[100];
	if(error_number != 0) {
		sstrcpy(error_buffer, error_string);
		if(error_number > 4) {
			sstrcpy(error_string, "error: ");
		}
		else {
			sstrcpy(error_string, "warning: ");
		}
		strcat(error_string, error_buffer);
	}
}

void FASTCALL ZintMakeErrText_InvCharInData(const char * pAddSymb, char * pBuf, size_t bufLen)
{
	SString temp_buf("Invalid characters in data");
	if(pAddSymb)
		temp_buf.Space().CatParStr(pAddSymb);
	strnzcpy(pBuf, temp_buf, bufLen);
}

void FASTCALL ZintMakeErrText_InvCheckDigit(const char * pAddSymb, char * pBuf, size_t bufLen)
{
	SString temp_buf("Invalid check digit");
	if(pAddSymb)
		temp_buf.Space().CatParStr(pAddSymb);
	strnzcpy(pBuf, temp_buf, bufLen);
}
//
// Output a hexadecimal representation of the rendered symbol
//
int dump_plot(struct ZintSymbol * symbol)
{
	FILE * f;
	int i, r;
	const char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	int space = 0;
	if(symbol->output_options & BARCODE_STDOUT) {
		f = stdout;
	}
	else {
		f = fopen(symbol->outfile, "w");
		if(!f) {
			sstrcpy(symbol->errtxt, "Could not open output file (B01)");
			return ZINT_ERROR_FILE_ACCESS;
		}
	}
	for(r = 0; r < symbol->rows; r++) {
		int byt = 0;
		for(i = 0; i < symbol->width; i++) {
			byt = byt << 1;
			if(module_is_set(symbol, r, i)) {
				byt += 1;
			}
			if(((i + 1) % 4) == 0) {
				fputc(hex[byt], f);
				space++;
				byt = 0;
			}
			if(space == 2) {
				fputc(' ', f);
				space = 0;
			}
		}
		if((symbol->width % 4) != 0) {
			byt = byt << (4 - (symbol->width % 4));
			fputc(hex[byt], f);
		}
		fputs("\n", f);
		space = 0;
	}
	if(symbol->output_options & BARCODE_STDOUT) {
		fflush(f);
	}
	else {
		fclose(f);
	}
	return 0;
}
//
// Process health industry bar code data
//
static int hibc(struct ZintSymbol * symbol, uchar source[], size_t length)
{
	int    counter, error_number;
	char   to_process[113], temp[2], check_digit;
	// without "+" and check: max 110 characters in HIBC 2.6
	if(length > 110) {
		sstrcpy(symbol->errtxt, "Data too long for HIBC LIC (B02)");
		return ZINT_ERROR_TOO_LONG;
	}
	to_upper(source);
	error_number = is_sane(TECHNETIUM, source, length);
	if(error_number == ZINT_ERROR_INVALID_DATA) {
		ZintMakeErrText_InvCharInData("B03", symbol->errtxt, sizeof(symbol->errtxt));
		return error_number;
	}
	sstrcpy(to_process, "+");
	{
		counter = 41;
		for(size_t i = 0; i < length; i++)
			counter += posn(TECHNETIUM, source[i]);
		counter = counter % 43;
	}
	if(counter < 10) {
		check_digit = itoc(counter);
	}
	else {
		if(counter < 36) {
			check_digit = (counter - 10) + 'A';
		}
		else {
			switch(counter) {
				case 36: check_digit = '-'; break;
				case 37: check_digit = '.'; break;
				case 38: check_digit = ' '; break;
				case 39: check_digit = '$'; break;
				case 40: check_digit = '/'; break;
				case 41: check_digit = '+'; break;
				case 42: check_digit = '%'; break;
				default: check_digit = ' '; break; /* Keep compiler happy */
			}
		}
	}
	temp[0] = check_digit;
	temp[1] = '\0';
	strcat(to_process, (char *)source);
	strcat(to_process, temp);
	length = strlen(to_process);
	switch(symbol->Std) {
		case BARCODE_HIBC_128:
		    error_number = code_128(symbol, (uchar *)to_process, length);
		    sstrcpy(symbol->text, (uchar *)"*");
		    strcat((char *)symbol->text, to_process);
		    strcat((char *)symbol->text, "*");
		    break;
		case BARCODE_HIBC_39:
		    symbol->option_2 = 0;
		    error_number = c39(symbol, (uchar *)to_process, length);
		    sstrcpy(symbol->text, (uchar *)"*");
		    strcat((char *)symbol->text, to_process);
		    strcat((char *)symbol->text, "*");
		    break;
		case BARCODE_HIBC_DM: error_number = dmatrix(symbol, (uchar *)to_process, length); break;
		case BARCODE_HIBC_QR: error_number = qr_code(symbol, (uchar *)to_process, length); break;
		case BARCODE_HIBC_PDF: error_number = pdf417enc(symbol, (uchar *)to_process, length); break;
		case BARCODE_HIBC_MICPDF: error_number = micro_pdf417(symbol, (uchar *)to_process, length); break;
		case BARCODE_HIBC_AZTEC: error_number = aztec(symbol, (uchar *)to_process, length); break;
		case BARCODE_HIBC_BLOCKF: error_number = codablock(symbol, (uchar *)to_process, length); break;
	}
	return error_number;
}

static void check_row_heights(struct ZintSymbol * symbol)
{
	// Check that rows with undefined heights are never less than 5x
	int large_bar_count = 0;
	int i;
	int preset_height = 0;
	int large_bar_height = 0;
	for(i = 0; i < symbol->rows; i++) {
		preset_height += symbol->row_height[i];
		if(symbol->row_height[i] == 0) {
			large_bar_count++;
		}
	}
	if(large_bar_count == 0) {
		symbol->height = preset_height;
	}
	else {
		large_bar_height = (symbol->height - preset_height) / large_bar_count;
	}
	if(large_bar_height < 5) {
		for(i = 0; i < symbol->rows; i++) {
			if(symbol->row_height[i] == 0) {
				symbol->row_height[i] = 5;
				preset_height += 5;
			}
		}
		symbol->height = preset_height;
	}
}

static int gs1_compliant(const int symbology)
{
	// Returns 1 if symbology supports GS1 data
	int result = 0;
	switch(symbology) {
		case BARCODE_EAN128:
		case BARCODE_RSS_EXP:
		case BARCODE_RSS_EXPSTACK:
		case BARCODE_EANX_CC:
		case BARCODE_EAN128_CC:
		case BARCODE_RSS14_CC:
		case BARCODE_RSS_LTD_CC:
		case BARCODE_RSS_EXP_CC:
		case BARCODE_UPCA_CC:
		case BARCODE_UPCE_CC:
		case BARCODE_RSS14STACK_CC:
		case BARCODE_RSS14_OMNI_CC:
		case BARCODE_RSS_EXPSTACK_CC:
		case BARCODE_CODE16K:
		case BARCODE_AZTEC:
		case BARCODE_DATAMATRIX:
		case BARCODE_CODEONE:
		case BARCODE_CODE49:
		case BARCODE_QRCODE:
		case BARCODE_DOTCODE:
		    result = 1;
		    break;
	}
	return result;
}

static int is_matrix(const int symbology)
{
	// Returns 1 if symbology is a matrix design
	int result = 0;
	switch(symbology) {
		case BARCODE_QRCODE:
		case BARCODE_DATAMATRIX:
		case BARCODE_MICROQR:
		case BARCODE_HIBC_DM:
		case BARCODE_AZTEC:
		case BARCODE_HIBC_QR:
		case BARCODE_HIBC_AZTEC:
		case BARCODE_AZRUNE:
		case BARCODE_CODEONE:
		case BARCODE_GRIDMATRIX:
		case BARCODE_HANXIN:
		case BARCODE_DOTCODE:
		    result = 1;
		    break;
	}
	return result;
}

static int supports_eci(const int symbology)
{
	// Returns 1 if symbology can encode the ECI character
	int result = 0;
	switch(symbology) {
		case BARCODE_AZTEC:
		case BARCODE_DATAMATRIX:
		case BARCODE_MAXICODE:
		case BARCODE_MICROPDF417:
		case BARCODE_PDF417:
		case BARCODE_PDF417TRUNC:
		case BARCODE_QRCODE:
		case BARCODE_DOTCODE:
		case BARCODE_GRIDMATRIX:
		case BARCODE_HANXIN:
		    result = 1;
		    break;
	}
	return result;
}

int ZBarcode_ValidID(int symbol_id)
{
	// Checks whether a symbology is supported
	int result = 0;
	switch(symbol_id) {
		case BARCODE_CODE11:
		case BARCODE_C25MATRIX:
		case BARCODE_C25INTER:
		case BARCODE_C25IATA:
		case BARCODE_C25LOGIC:
		case BARCODE_C25IND:
		case BARCODE_CODE39:
		case BARCODE_EXCODE39:
		case BARCODE_EANX:
		case BARCODE_EANX_CHK:
		case BARCODE_EAN128:
		case BARCODE_CODABAR:
		case BARCODE_CODE128:
		case BARCODE_DPLEIT:
		case BARCODE_DPIDENT:
		case BARCODE_CODE16K:
		case BARCODE_CODE49:
		case BARCODE_CODE93:
		case BARCODE_FLAT:
		case BARCODE_RSS14:
		case BARCODE_RSS_LTD:
		case BARCODE_RSS_EXP:
		case BARCODE_TELEPEN:
		case BARCODE_UPCA:
		case BARCODE_UPCA_CHK:
		case BARCODE_UPCE:
		case BARCODE_UPCE_CHK:
		case BARCODE_POSTNET:
		case BARCODE_MSI_PLESSEY:
		case BARCODE_FIM:
		case BARCODE_LOGMARS:
		case BARCODE_PHARMA:
		case BARCODE_PZN:
		case BARCODE_PHARMA_TWO:
		case BARCODE_PDF417:
		case BARCODE_PDF417TRUNC:
		case BARCODE_MAXICODE:
		case BARCODE_QRCODE:
		case BARCODE_CODE128B:
		case BARCODE_AUSPOST:
		case BARCODE_AUSREPLY:
		case BARCODE_AUSROUTE:
		case BARCODE_AUSREDIRECT:
		case BARCODE_ISBNX:
		case BARCODE_RM4SCC:
		case BARCODE_DATAMATRIX:
		case BARCODE_EAN14:
		case BARCODE_NVE18:
		case BARCODE_JAPANPOST:
		case BARCODE_KOREAPOST:
		case BARCODE_RSS14STACK:
		case BARCODE_RSS14STACK_OMNI:
		case BARCODE_RSS_EXPSTACK:
		case BARCODE_PLANET:
		case BARCODE_MICROPDF417:
		case BARCODE_ONECODE:
		case BARCODE_PLESSEY:
		case BARCODE_TELEPEN_NUM:
		case BARCODE_ITF14:
		case BARCODE_KIX:
		case BARCODE_AZTEC:
		case BARCODE_DAFT:
		case BARCODE_MICROQR:
		case BARCODE_HIBC_128:
		case BARCODE_HIBC_39:
		case BARCODE_HIBC_DM:
		case BARCODE_HIBC_QR:
		case BARCODE_HIBC_PDF:
		case BARCODE_HIBC_MICPDF:
		case BARCODE_HIBC_AZTEC:
		case BARCODE_HIBC_BLOCKF:
		case BARCODE_AZRUNE:
		case BARCODE_CODE32:
		case BARCODE_EANX_CC:
		case BARCODE_EAN128_CC:
		case BARCODE_RSS14_CC:
		case BARCODE_RSS_LTD_CC:
		case BARCODE_RSS_EXP_CC:
		case BARCODE_UPCA_CC:
		case BARCODE_UPCE_CC:
		case BARCODE_RSS14STACK_CC:
		case BARCODE_RSS14_OMNI_CC:
		case BARCODE_RSS_EXPSTACK_CC:
		case BARCODE_CHANNEL:
		case BARCODE_CODEONE:
		case BARCODE_GRIDMATRIX:
		case BARCODE_HANXIN:
		case BARCODE_DOTCODE:
		case BARCODE_CODABLOCKF:
		    result = 1;
		    break;
	}
	return result;
}

static int extended_charset(struct ZintSymbol * symbol, const uchar * source, const int length)
{
	int error_number = 0;
	// These are the "elite" standards which can support multiple character sets
	switch(symbol->Std) {
		case BARCODE_QRCODE: error_number = qr_code(symbol, source, length); break;
		case BARCODE_MICROQR: error_number = microqr(symbol, source, length); break;
		case BARCODE_GRIDMATRIX: error_number = grid_matrix(symbol, source, length); break;
		case BARCODE_HANXIN: error_number = han_xin(symbol, source, length); break;
	}
	return error_number;
}

static int reduced_charset(struct ZintSymbol * symbol, const uchar * source, int in_length)
{
	// These are the "norm" standards which only support Latin-1 at most
	int error_number = 0;
#ifndef _MSC_VER
	uchar preprocessed[in_length + 1];
#else
	uchar* preprocessed = (uchar *)_alloca(in_length + 1);
#endif
	if(symbol->Std == BARCODE_CODE16K) {
		symbol->whitespace_width = 16;
		symbol->border_width = 2;
		if(!(symbol->output_options & BARCODE_BIND)) {
			symbol->output_options += BARCODE_BIND;
		}
	}
	else if(symbol->Std == BARCODE_ITF14) {
		symbol->whitespace_width = 20;
		symbol->border_width = 8;
		if(!(symbol->output_options & BARCODE_BOX)) {
			symbol->output_options += BARCODE_BOX;
		}
	}
	switch(symbol->input_mode) {
		case DATA_MODE:
		case GS1_MODE:
		    memcpy(preprocessed, source, in_length);
		    preprocessed[in_length] = '\0';
		    break;
		case UNICODE_MODE:
		    error_number = utf_to_eci(symbol->eci, source, preprocessed, &in_length);
		    if(error_number != 0) {
			    sstrcpy(symbol->errtxt, "Invalid characters in input data (B04)");
			    return error_number;
		    }
		    break;
	}
	switch(symbol->Std) {
		case BARCODE_C25MATRIX: error_number = matrix_two_of_five(symbol, preprocessed, in_length); break;
		case BARCODE_C25IND: error_number = industrial_two_of_five(symbol, preprocessed, in_length); break;
		case BARCODE_C25INTER: error_number = interleaved_two_of_five(symbol, preprocessed, in_length); break;
		case BARCODE_C25IATA: error_number = iata_two_of_five(symbol, preprocessed, in_length); break;
		case BARCODE_C25LOGIC: error_number = logic_two_of_five(symbol, preprocessed, in_length); break;
		case BARCODE_DPLEIT: error_number = dpleit(symbol, preprocessed, in_length); break;
		case BARCODE_DPIDENT: error_number = dpident(symbol, preprocessed, in_length); break;
		case BARCODE_UPCA:
		case BARCODE_UPCA_CHK:
		case BARCODE_UPCE:
		case BARCODE_UPCE_CHK:
		case BARCODE_EANX:
		case BARCODE_EANX_CHK: error_number = eanx(symbol, preprocessed, in_length); break;
		case BARCODE_EAN128: error_number = ean_128(symbol, preprocessed, in_length); break;
		case BARCODE_CODE39: error_number = c39(symbol, preprocessed, in_length); break;
		case BARCODE_PZN: error_number = pharmazentral(symbol, preprocessed, in_length); break;
		case BARCODE_EXCODE39: error_number = ec39(symbol, preprocessed, in_length); break;
		case BARCODE_CODABAR: error_number = codabar(symbol, preprocessed, in_length); break;
		case BARCODE_CODE93: error_number = c93(symbol, preprocessed, in_length); break;
		case BARCODE_LOGMARS: error_number = c39(symbol, preprocessed, in_length); break;
		case BARCODE_CODE128:
		case BARCODE_CODE128B: error_number = code_128(symbol, preprocessed, in_length); break;
		case BARCODE_NVE18: error_number = nve_18(symbol, preprocessed, in_length); break;
		case BARCODE_CODE11: error_number = code_11(symbol, preprocessed, in_length); break;
		case BARCODE_MSI_PLESSEY: error_number = msi_handle(symbol, preprocessed, in_length); break;
		case BARCODE_TELEPEN: error_number = telepen(symbol, preprocessed, in_length); break;
		case BARCODE_TELEPEN_NUM: error_number = telepen_num(symbol, preprocessed, in_length); break;
		case BARCODE_PHARMA: error_number = pharma_one(symbol, preprocessed, in_length); break;
		case BARCODE_PLESSEY: error_number = plessey(symbol, preprocessed, in_length); break;
		case BARCODE_ITF14: error_number = itf14(symbol, preprocessed, in_length); break;
		case BARCODE_FLAT: error_number = flattermarken(symbol, preprocessed, in_length); break;
		case BARCODE_FIM: error_number = fim(symbol, preprocessed, in_length); break;
		case BARCODE_POSTNET: error_number = post_plot(symbol, preprocessed, in_length); break;
		case BARCODE_PLANET: error_number = planet_plot(symbol, preprocessed, in_length); break;
		case BARCODE_RM4SCC: error_number = royal_plot(symbol, preprocessed, in_length); break;
		case BARCODE_AUSPOST:
		case BARCODE_AUSREPLY:
		case BARCODE_AUSROUTE:
		case BARCODE_AUSREDIRECT: error_number = australia_post(symbol, preprocessed, in_length); break;
		case BARCODE_CODE16K: error_number = code16k(symbol, preprocessed, in_length); break;
		case BARCODE_PHARMA_TWO: error_number = pharma_two(symbol, preprocessed, in_length); break;
		case BARCODE_ONECODE: error_number = imail(symbol, preprocessed, in_length); break;
		case BARCODE_ISBNX: error_number = eanx(symbol, preprocessed, in_length); break;
		case BARCODE_RSS14:
		case BARCODE_RSS14STACK:
		case BARCODE_RSS14STACK_OMNI: error_number = rss14(symbol, preprocessed, in_length); break;
		case BARCODE_RSS_LTD: error_number = rsslimited(symbol, preprocessed, in_length); break;
		case BARCODE_RSS_EXP:
		case BARCODE_RSS_EXPSTACK: error_number = rssexpanded(symbol, preprocessed, in_length); break;
		case BARCODE_EANX_CC:
		case BARCODE_EAN128_CC:
		case BARCODE_RSS14_CC:
		case BARCODE_RSS_LTD_CC:
		case BARCODE_RSS_EXP_CC:
		case BARCODE_UPCA_CC:
		case BARCODE_UPCE_CC:
		case BARCODE_RSS14STACK_CC:
		case BARCODE_RSS14_OMNI_CC:
		case BARCODE_RSS_EXPSTACK_CC: error_number = composite(symbol, preprocessed, in_length); break;
		case BARCODE_KIX: error_number = kix_code(symbol, preprocessed, in_length); break;
		case BARCODE_CODE32: error_number = code32(symbol, preprocessed, in_length); break;
		case BARCODE_DAFT: error_number = daft_code(symbol, preprocessed, in_length); break;
		case BARCODE_EAN14: error_number = ean_14(symbol, preprocessed, in_length); break;
		case BARCODE_AZRUNE: error_number = aztec_runes(symbol, preprocessed, in_length); break;
		case BARCODE_KOREAPOST: error_number = korea_post(symbol, preprocessed, in_length); break;
		case BARCODE_HIBC_128:
		case BARCODE_HIBC_39:
		case BARCODE_HIBC_DM:
		case BARCODE_HIBC_QR:
		case BARCODE_HIBC_PDF:
		case BARCODE_HIBC_MICPDF:
		case BARCODE_HIBC_AZTEC:
		case BARCODE_HIBC_BLOCKF: error_number = hibc(symbol, preprocessed, in_length); break;
		case BARCODE_JAPANPOST: error_number = japan_post(symbol, preprocessed, in_length); break;
		case BARCODE_CODE49: error_number = code_49(symbol, preprocessed, in_length); break;
		case BARCODE_CHANNEL: error_number = channel_code(symbol, preprocessed, in_length); break;
		case BARCODE_CODEONE: error_number = code_one(symbol, preprocessed, in_length); break;
		case BARCODE_DATAMATRIX: error_number = dmatrix(symbol, preprocessed, in_length); break;
		case BARCODE_PDF417:
		case BARCODE_PDF417TRUNC: error_number = pdf417enc(symbol, preprocessed, in_length); break;
		case BARCODE_MICROPDF417: error_number = micro_pdf417(symbol, preprocessed, in_length); break;
		case BARCODE_MAXICODE: error_number = maxicode(symbol, preprocessed, in_length); break;
		case BARCODE_AZTEC: error_number = aztec(symbol, preprocessed, in_length); break;
		case BARCODE_DOTCODE: error_number = dotcode(symbol, preprocessed, in_length); break;
		case BARCODE_CODABLOCKF: error_number = codablock(symbol, preprocessed, in_length); break;
	}
	return error_number;
}

int ZBarcode_Encode(struct ZintSymbol * symbol, const uchar * source, int length)
{
	int error_number, error_buffer, i;
#ifdef _MSC_VER
	uchar * local_source;
#endif
	error_number = 0;
	SETIFZ(length, sstrleni(source));
	if(length == 0) {
		sstrcpy(symbol->errtxt, "No input data (B05)");
		error_tag(symbol->errtxt, ZINT_ERROR_INVALID_DATA);
		return ZINT_ERROR_INVALID_DATA;
	}
	if(strcmp(symbol->outfile, "") == 0) {
#ifdef NO_PNG
		sstrcpy(symbol->outfile, "out.gif");
#else
		sstrcpy(symbol->outfile, "out.png");
#endif
	}
#ifndef _MSC_VER
	uchar local_source[length + 1];
#else
	local_source = (uchar *)_alloca(length + 1);
#endif
	// First check the symbology field
	if(symbol->Std < 1) {
		sstrcpy(symbol->errtxt, "Symbology out of range, using Code 128 (B06)");
		symbol->Std = BARCODE_CODE128;
		error_number = ZINT_WARN_INVALID_OPTION;
	}
	// symbol->symbologys 1 to 86 are defined by tbarcode
	if(symbol->Std == 5) { symbol->Std = BARCODE_C25MATRIX; }
	if((symbol->Std >= 10) && (symbol->Std <= 12)) { symbol->Std = BARCODE_EANX; }
	if(symbol->Std == 15) { symbol->Std = BARCODE_EANX; }
	if(symbol->Std == 17) { symbol->Std = BARCODE_UPCA; }
	if(symbol->Std == 19) {
		sstrcpy(symbol->errtxt, "Codabar 18 not supported, using Codabar (B07)");
		symbol->Std = BARCODE_CODABAR;
		error_number = ZINT_WARN_INVALID_OPTION;
	}
	if(symbol->Std == 26) { symbol->Std = BARCODE_UPCA; }
	if(symbol->Std == 27) {
		sstrcpy(symbol->errtxt, "UPCD1 not supported (B08)");
		error_number = ZINT_ERROR_INVALID_OPTION;
	}
	if(symbol->Std == 33) { symbol->Std = BARCODE_EAN128; }
	if(symbol->Std == 36) { symbol->Std = BARCODE_UPCA; }
	if(symbol->Std == 38) { symbol->Std = BARCODE_UPCE; }
	if((symbol->Std >= 41) && (symbol->Std <= 45)) { symbol->Std = BARCODE_POSTNET; }
	if(symbol->Std == 46) { symbol->Std = BARCODE_PLESSEY; }
	if(symbol->Std == 48) { symbol->Std = BARCODE_NVE18; }
	if(symbol->Std == 54) {
		sstrcpy(symbol->errtxt, "General Parcel Code not supported, using Code 128 (B10)");
		symbol->Std = BARCODE_CODE128;
		error_number = ZINT_WARN_INVALID_OPTION;
	}
	if(oneof2(symbol->Std, 59, 61)) { symbol->Std = BARCODE_CODE128; }
	if(symbol->Std == 62) { symbol->Std = BARCODE_CODE93; }
	if(oneof2(symbol->Std, 64, 65)) { symbol->Std = BARCODE_AUSPOST; }
	if(symbol->Std == 73) {
		sstrcpy(symbol->errtxt, "Symbology out of range, using Code 128 (B11)");
		symbol->Std = BARCODE_CODE128;
		error_number = ZINT_WARN_INVALID_OPTION;
	}
	if(symbol->Std == 78) { symbol->Std = BARCODE_RSS14; }
	if(symbol->Std == 83) { symbol->Std = BARCODE_PLANET; }
	if(symbol->Std == 88) { symbol->Std = BARCODE_EAN128; }
	if(symbol->Std == 91) {
		sstrcpy(symbol->errtxt, "Symbology out of range, using Code 128 (B12)");
		symbol->Std = BARCODE_CODE128;
		error_number = ZINT_WARN_INVALID_OPTION;
	}
	if((symbol->Std >= 94) && (symbol->Std <= 96)) {
		sstrcpy(symbol->errtxt, "Symbology out of range, using Code 128 (B13)");
		symbol->Std = BARCODE_CODE128;
		error_number = ZINT_WARN_INVALID_OPTION;
	}
	if(symbol->Std == 100) { symbol->Std = BARCODE_HIBC_128; }
	if(symbol->Std == 101) { symbol->Std = BARCODE_HIBC_39; }
	if(symbol->Std == 103) { symbol->Std = BARCODE_HIBC_DM; }
	if(symbol->Std == 105) { symbol->Std = BARCODE_HIBC_QR; }
	if(symbol->Std == 107) { symbol->Std = BARCODE_HIBC_PDF; }
	if(symbol->Std == 109) { symbol->Std = BARCODE_HIBC_MICPDF; }
	if(symbol->Std == 111) { symbol->Std = BARCODE_HIBC_BLOCKF; }
	if(oneof2(symbol->Std, 113, 114)) {
		sstrcpy(symbol->errtxt, "Symbology out of range, using Code 128 (B14)");
		symbol->Std = BARCODE_CODE128;
		error_number = ZINT_WARN_INVALID_OPTION;
	}
	if(symbol->Std == 115) { symbol->Std = BARCODE_DOTCODE; }
	if((symbol->Std >= 117) && (symbol->Std <= 127)) {
		sstrcpy(symbol->errtxt, "Symbology out of range, using Code 128 (B15)");
		symbol->Std = BARCODE_CODE128;
		error_number = ZINT_WARN_INVALID_OPTION;
	}
	// Everything from 128 up is Zint-specific
	if(symbol->Std >= 143) {
		sstrcpy(symbol->errtxt, "Symbology out of range, using Code 128 (B16)");
		symbol->Std = BARCODE_CODE128;
		error_number = ZINT_WARN_INVALID_OPTION;
	}
	if(error_number > 4) {
		error_tag(symbol->errtxt, error_number);
		return error_number;
	}
	else {
		error_buffer = error_number;
	}
	if((!(supports_eci(symbol->Std))) && (symbol->eci != 3)) {
		sstrcpy(symbol->errtxt, "Symbology does not support ECI switching (B17)");
		error_number = ZINT_ERROR_INVALID_OPTION;
	}
	if((symbol->eci < 3) || (symbol->eci > 26) || (symbol->eci == 14) || (symbol->eci == 19) || (symbol->eci == 25)) {
		sstrcpy(symbol->errtxt, "Invalid or unsupported ECI mode (B18)");
		error_number = ZINT_ERROR_INVALID_OPTION;
	}
	if((symbol->input_mode < 0) || (symbol->input_mode > 2)) {
		symbol->input_mode = DATA_MODE;
	}
	if(symbol->input_mode == GS1_MODE) {
		for(i = 0; i < length; i++) {
			if(source[i] == '\0') {
				sstrcpy(symbol->errtxt, "NULL characters not permitted in GS1 mode (B19)");
				return ZINT_ERROR_INVALID_DATA;
			}
		}
		if(gs1_compliant(symbol->Std) == 1) {
			error_number = ugs1_verify(symbol, source, length, local_source);
			if(error_number != 0) {
				return error_number;
			}
			length = sstrlen(local_source);
		}
		else {
			sstrcpy(symbol->errtxt, "Selected symbology does not support GS1 mode (B20)");
			return ZINT_ERROR_INVALID_OPTION;
		}
	}
	else {
		memcpy(local_source, source, length);
		local_source[length] = '\0';
	}
	if((symbol->dot_size < 0.01) || (symbol->dot_size > 20.0)) {
		sstrcpy(symbol->errtxt, "Invalid dot size (B21)");
		return ZINT_ERROR_INVALID_OPTION;
	}
	switch(symbol->Std) {
		case BARCODE_QRCODE:
		case BARCODE_MICROQR:
		case BARCODE_GRIDMATRIX:
		case BARCODE_HANXIN: error_number = extended_charset(symbol, local_source, length); break;
		default: error_number = reduced_charset(symbol, local_source, length); break;
	}
	if((error_number == ZINT_ERROR_INVALID_DATA) && (supports_eci(symbol->Std) && (symbol->input_mode == UNICODE_MODE))) {
		// Try another ECI mode
		symbol->eci = get_best_eci(local_source, length);
		error_number = ZINT_WARN_USES_ECI;
		sstrcpy(symbol->errtxt, "Encoded data includes ECI codes (B22)");
		//printf("Data will encode with ECI %d\n", symbol->eci);
		switch(symbol->Std) {
			case BARCODE_QRCODE:
			case BARCODE_MICROQR:
			case BARCODE_GRIDMATRIX:
			case BARCODE_HANXIN:
			    error_number = utf_to_eci(symbol->eci, source, local_source, &length);
			    error_number = extended_charset(symbol, local_source, length);
			    break;
			default:
			    error_number = reduced_charset(symbol, local_source, length);
			    break;
		}
	}
	if((symbol->Std == BARCODE_CODE128) || (symbol->Std == BARCODE_CODE128B)) {
		for(i = 0; i < length; i++) {
			symbol->text[i] = (local_source[i] == '\0') ? ' ' : local_source[i];
		}
	}
	SETIFZ(error_number, error_buffer);
	error_tag(symbol->errtxt, error_number);
	if(error_number <= 5) {
		check_row_heights(symbol);
	}
	return error_number;
}

int ZBarcode_Print(struct ZintSymbol * symbol, int rotate_angle)
{
	int    error_number;
	char   output[4];
	switch(rotate_angle) {
		case 0:
		case 90:
		case 180:
		case 270:
		    break;
		default:
		    sstrcpy(symbol->errtxt, "Invalid rotation angle (B23)");
		    return ZINT_ERROR_INVALID_OPTION;
	}
	if(symbol->output_options & BARCODE_DOTTY_MODE) {
		if(!(is_matrix(symbol->Std))) {
			sstrcpy(symbol->errtxt, "Selected symbology cannot be rendered as dots (B24)");
			return ZINT_ERROR_INVALID_OPTION;
		}
	}
	if(strlen(symbol->outfile) > 3) {
		output[0] = symbol->outfile[strlen(symbol->outfile) - 3];
		output[1] = symbol->outfile[strlen(symbol->outfile) - 2];
		output[2] = symbol->outfile[strlen(symbol->outfile) - 1];
		output[3] = '\0';
		to_upper((uchar *)output);
		if(sstreqi_ascii(output, "PNG")) {
			if(symbol->scale < 1.0) {
				symbol->text[0] = '\0';
			}
			error_number = plot_raster(symbol, rotate_angle, OUT_PNG_FILE);
		}
		else if(sstreqi_ascii(output, "BMP")) {
			if(symbol->scale < 1.0) {
				symbol->text[0] = '\0';
			}
			error_number = plot_raster(symbol, rotate_angle, OUT_BMP_FILE);
		}
		else if(sstreqi_ascii(output, "PCX")) {
			if(symbol->scale < 1.0) {
				symbol->text[0] = '\0';
			}
			error_number = plot_raster(symbol, rotate_angle, OUT_PCX_FILE);
		}
		else if(sstreqi_ascii(output, "GIF")) {
			if(symbol->scale < 1.0) {
				symbol->text[0] = '\0';
			}
			error_number = plot_raster(symbol, rotate_angle, OUT_GIF_FILE);
		}
		else if(sstreqi_ascii(output, "TXT")) {
			error_number = dump_plot(symbol);
		}
		else if(sstreqi_ascii(output, "EPS")) {
			error_number = ps_plot(symbol);
		}
		else if(sstreqi_ascii(output, "SVG")) {
			error_number = svg_plot(symbol);
		}
		else {
			sstrcpy(symbol->errtxt, "Unknown output format (B25)");
			error_tag(symbol->errtxt, ZINT_ERROR_INVALID_OPTION);
			return ZINT_ERROR_INVALID_OPTION;
		}
	}
	else {
		sstrcpy(symbol->errtxt, "Unknown output format (B26)");
		error_tag(symbol->errtxt, ZINT_ERROR_INVALID_OPTION);
		return ZINT_ERROR_INVALID_OPTION;
	}
	if(error_number == ZINT_ERROR_INVALID_OPTION) {
		// If libpng is not installed
		sstrcpy(symbol->errtxt, "Unknown output format (B27)");
	}
	error_tag(symbol->errtxt, error_number);
	return error_number;
}

int ZBarcode_Buffer(struct ZintSymbol * symbol, int rotate_angle)
{
	int error_number;
	switch(rotate_angle) {
		case 0:
		case 90:
		case 180:
		case 270:
		    break;
		default:
		    sstrcpy(symbol->errtxt, "Invalid rotation angle (B28)");
		    return ZINT_ERROR_INVALID_OPTION;
	}
	error_number = plot_raster(symbol, rotate_angle, OUT_BUFFER);
	error_tag(symbol->errtxt, error_number);
	return error_number;
}

int ZBarcode_Encode_and_Print(struct ZintSymbol * symbol, const uchar * input, int length, int rotate_angle)
{
	int error_number = ZBarcode_Encode(symbol, input, length);
	SETIFZ(error_number, ZBarcode_Print(symbol, rotate_angle));
	return error_number;
}

int ZBarcode_Encode_and_Buffer(struct ZintSymbol * symbol, const uchar * input, int length, int rotate_angle)
{
	int error_number = ZBarcode_Encode(symbol, input, length);
	SETIFZ(error_number, ZBarcode_Buffer(symbol, rotate_angle));
	return error_number;
}

int ZBarcode_Encode_File(struct ZintSymbol * symbol, char * filename)
{
	FILE * file;
	uchar * buffer;
	ulong fileLen;
	uint nRead = 0, n = 0;
	int ret;
	if(sstreq(filename, "-")) {
		file = stdin;
		fileLen = 7100;
	}
	else {
		file = fopen(filename, "rb");
		if(!file) {
			sstrcpy(symbol->errtxt, "Unable to read input file (B29)");
			return ZINT_ERROR_INVALID_DATA;
		}
		// Get file length 
		fseek(file, 0, SEEK_END);
		fileLen = ftell(file);
		fseek(file, 0, SEEK_SET);
		if(fileLen > 7100) {
			// The largest amount of data that can be encoded is 7089 numeric digits in QR Code 
			sstrcpy(symbol->errtxt, "Input file too long (B30)");
			fclose(file);
			return ZINT_ERROR_INVALID_DATA;
		}
	}
	// Allocate memory 
	buffer = (uchar *)SAlloc::M(fileLen * sizeof(uchar));
	if(!buffer) {
		sstrcpy(symbol->errtxt, "Internal memory error (B31)");
		if(!sstreq(filename, "-"))
			fclose(file);
		return ZINT_ERROR_MEMORY;
	}
	//
	// Read file contents into buffer 
	//
	do {
		n = fread(buffer + nRead, 1, fileLen - nRead, file);
		if(ferror(file)) {
			sstrcpy(symbol->errtxt, strerror(errno));
			return ZINT_ERROR_INVALID_DATA;
		}
		nRead += n;
	} while(!feof(file) && (0 < n) && (nRead < fileLen));
	fclose(file);
	ret = ZBarcode_Encode(symbol, buffer, nRead);
	SAlloc::F(buffer);
	return ret;
}

int ZBarcode_Encode_File_and_Print(struct ZintSymbol * symbol, char * filename, int rotate_angle)
{
	int error_number = ZBarcode_Encode_File(symbol, filename);
	return error_number ? error_number : ZBarcode_Print(symbol, rotate_angle);
}

int ZBarcode_Encode_File_and_Buffer(struct ZintSymbol * symbol, char * filename, int rotate_angle)
{
	int error_number;
	error_number = ZBarcode_Encode_File(symbol, filename);
	return error_number ? error_number : ZBarcode_Buffer(symbol, rotate_angle);
}
/*
 * Rendering support, initially added by Sam Lown.
 *
 * Converts encoded data into an intermediate format to be interpreted
 * in other applications using this library.
 *
 * If the width and height are not set to zero, the barcode will be resized to those
 * dimensions. The symbol->scale and symbol->height values are totally ignored in this case.
 *
 */
int ZBarcode_Render(struct ZintSymbol * symbol, const float width, const float height)
{
	// Send the request to the render_plot method
	return render_plot(symbol, width, height);
}

