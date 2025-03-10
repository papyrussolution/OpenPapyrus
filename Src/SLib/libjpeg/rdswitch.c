/*
 * rdswitch.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * Modified 2003-2015 by Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to process some of cjpeg's more complicated
 * command-line switches.  Switches processed here are:
 *	-qtables file		Read quantization tables from text file
 *	-scans file		Read scan script from text file
 *	-quality N[,N,...]	Set quality ratings
 *	-qslots N[,N,...]	Set component quantization table selectors
 *	-sample HxV[,HxV,...]	Set component sampling factors
 */
#include <slib-internal.h>
#pragma hdrstop
#define JPEG_INTERNALS
#include "cdjpeg.h"
// 
// Descr: Read next char, skipping over any comments (# to end of line) 
//   A comment/newline sequence is returned as a newline 
// 
static int FASTCALL text_getc(FILE * file)
{
	int ch = getc(file);
	if(ch == '#') {
		do {
			ch = getc(file);
		} while(ch != '\n' && ch != EOF);
	}
	return ch;
}
//
// Descr: Read an unsigned decimal integer from a file, store it in result 
//   Reads one trailing character after the integer; returns it in termchar 
//
static boolean read_text_integer(FILE * file, long * result, int * termchar)
{
	int ch;
	long val;
	/* Skip any leading whitespace, detect EOF */
	do {
		ch = text_getc(file);
		if(ch == EOF) {
			*termchar = ch;
			return FALSE;
		}
	} while(isspace(ch));
	if(!isdec(ch)) {
		*termchar = ch;
		return FALSE;
	}
	val = ch - '0';
	while((ch = text_getc(file)) != EOF) {
		if(!isdec(ch))
			break;
		val *= 10;
		val += ch - '0';
	}
	*result = val;
	*termchar = ch;
	return TRUE;
}
// 
// Descr: Read a set of quantization tables from the specified file.
//   The file is plain ASCII text: decimal numbers with whitespace between.
//   Comments preceded by '#' may be included in the file.
//   There may be one to NUM_QUANT_TBLS tables in the file, each of 64 values.
//   The tables are implicitly numbered 0,1,etc.
// NOTE: does not affect the qslots mapping, which will default to selecting
//   table 0 for luminance (or primary) components, 1 for chrominance components.
//   You must use -qslots if you want a different component->table mapping.
// 
boolean read_quant_tables(j_compress_ptr cinfo, char * filename, boolean force_baseline)
{
	int    i, termchar;
	long   val;
	uint   table[DCTSIZE2];
	FILE * fp = fopen(filename, "r");
	int    tblno = 0;
	if(!fp) {
		slfprintf_stderr("Can't open table file %s\n", filename);
		return FALSE;
	}
	while(read_text_integer(fp, &val, &termchar)) { /* read 1st element of table */
		if(tblno >= NUM_QUANT_TBLS) {
			slfprintf_stderr("Too many tables in file %s\n", filename);
			fclose(fp);
			return FALSE;
		}
		table[0] = (uint)val;
		for(i = 1; i < DCTSIZE2; i++) {
			if(!read_text_integer(fp, &val, &termchar)) {
				slfprintf_stderr("Invalid table data in file %s\n", filename);
				fclose(fp);
				return FALSE;
			}
			table[i] = (uint)val;
		}
		jpeg_add_quant_table(cinfo, tblno, table, cinfo->q_scale_factor[tblno], force_baseline);
		tblno++;
	}
	if(termchar != EOF) {
		slfprintf_stderr("Non-numeric data in file %s\n", filename);
		fclose(fp);
		return FALSE;
	}
	fclose(fp);
	return TRUE;
}

#ifdef C_MULTISCAN_FILES_SUPPORTED
// 
// Descr: Variant of read_text_integer that always looks for a non-space termchar;
//   this simplifies parsing of punctuation in scan scripts.
// 
static boolean FASTCALL read_scan_integer(FILE * file, long * result, int * termchar)
{
	int ch;
	if(!read_text_integer(file, result, termchar))
		return FALSE;
	ch = *termchar;
	while(ch != EOF && isspace(ch))
		ch = text_getc(file);
	if(isdec(ch)) { // oops, put it back 
		if(ungetc(ch, file) == EOF)
			return FALSE;
		ch = ' ';
	}
	else {
		// Any separators other than ';' and ':' are ignored;
		// this allows user to insert commas, etc, if desired.
		if(ch != EOF && ch != ';' && ch != ':')
			ch = ' ';
	}
	*termchar = ch;
	return TRUE;
}
//
// Descr: Read a scan script from the specified text file.
//   Each entry in the file defines one scan to be emitted.
//   Entries are separated by semicolons ';'.
//   An entry contains one to four component indexes,
//   optionally followed by a colon ':' and four progressive-JPEG parameters.
//   The component indexes denote which component(s) are to be transmitted
//   in the current scan.  The first component has index 0.
//   Sequential JPEG is used if the progressive-JPEG parameters are omitted.
//   The file is free format text: any whitespace may appear between numbers
//   and the ':' and ';' punctuation marks.  Also, other punctuation (such
//   as commas or dashes) can be placed between numbers if desired.
//   Comments preceded by '#' may be included in the file.
// Note: we do very little validity checking here;
//   jcmaster.c will validate the script parameters.
//
boolean read_scan_script(j_compress_ptr cinfo, char * filename)
{
	int scanno, ncomps, termchar;
	long val;
	jpeg_scan_info * scanptr;
#define MAX_SCANS  100          /* quite arbitrary limit */
	jpeg_scan_info scans[MAX_SCANS];
	FILE * fp = fopen(filename, "r");
	if(!fp) {
		slfprintf_stderr("Can't open scan definition file %s\n", filename);
		return FALSE;
	}
	scanptr = scans;
	scanno = 0;
	while(read_scan_integer(fp, &val, &termchar)) {
		if(scanno >= MAX_SCANS) {
			slfprintf_stderr("Too many scans defined in file %s\n", filename);
			fclose(fp);
			return FALSE;
		}
		scanptr->component_index[0] = (int)val;
		ncomps = 1;
		while(termchar == ' ') {
			if(ncomps >= MAX_COMPS_IN_SCAN) {
				slfprintf_stderr("Too many components in one scan in file %s\n", filename);
				fclose(fp);
				return FALSE;
			}
			if(!read_scan_integer(fp, &val, &termchar))
				goto bogus;
			scanptr->component_index[ncomps] = (int)val;
			ncomps++;
		}
		scanptr->comps_in_scan = ncomps;
		if(termchar == ':') {
			if(!read_scan_integer(fp, &val, &termchar) || termchar != ' ')
				goto bogus;
			scanptr->Ss = (int)val;
			if(!read_scan_integer(fp, &val, &termchar) || termchar != ' ')
				goto bogus;
			scanptr->Se = (int)val;
			if(!read_scan_integer(fp, &val, &termchar) || termchar != ' ')
				goto bogus;
			scanptr->Ah = (int)val;
			if(!read_scan_integer(fp, &val, &termchar))
				goto bogus;
			scanptr->Al = (int)val;
		}
		else {
			/* set non-progressive parameters */
			scanptr->Ss = 0;
			scanptr->Se = DCTSIZE2-1;
			scanptr->Ah = 0;
			scanptr->Al = 0;
		}
		if(termchar != ';' && termchar != EOF) {
bogus:
			slfprintf_stderr("Invalid scan entry format in file %s\n", filename);
			fclose(fp);
			return FALSE;
		}
		scanptr++, scanno++;
	}
	if(termchar != EOF) {
		slfprintf_stderr("Non-numeric data in file %s\n", filename);
		fclose(fp);
		return FALSE;
	}
	if(scanno > 0) {
		/* Stash completed scan list in cinfo structure.
		 * NOTE: for cjpeg's use, JPOOL_IMAGE is the right lifetime for this data,
		 * but if you want to compress multiple images you'd want JPOOL_PERMANENT.
		 */
		scanptr = static_cast<jpeg_scan_info *>((*cinfo->mem->alloc_small)(reinterpret_cast<j_common_ptr>(cinfo), JPOOL_IMAGE, scanno * sizeof(jpeg_scan_info)));
		memcpy(scanptr, scans, scanno * sizeof(jpeg_scan_info));
		cinfo->scan_info = scanptr;
		cinfo->num_scans = scanno;
	}
	fclose(fp);
	return TRUE;
}
#endif /* C_MULTISCAN_FILES_SUPPORTED */
// 
// Descr: Process a quality-ratings parameter string, of the form N[,N,...]
//   If there are more q-table slots than parameters, the last value is replicated.
// 
boolean set_quality_ratings(j_compress_ptr cinfo, char * arg, boolean force_baseline)
{
	int val = 75; // default value 
	for(int tblno = 0; tblno < NUM_QUANT_TBLS; tblno++) {
		if(*arg) {
			char ch = ','; /* if not set by sscanf, will be ',' */
			if(sscanf(arg, "%d%c", &val, &ch) < 1)
				return FALSE;
			if(ch != ',') /* syntax check */
				return FALSE;
			/* Convert user 0-100 rating to percentage scaling */
			cinfo->q_scale_factor[tblno] = jpeg_quality_scaling(val);
			while(*arg && *arg++ != ',') /* advance to next segment of arg string */
				;
		}
		else {
			/* reached end of parameter, set remaining factors to last value */
			cinfo->q_scale_factor[tblno] = jpeg_quality_scaling(val);
		}
	}
	jpeg_default_qtables(cinfo, force_baseline);
	return TRUE;
}
// 
// Process a quantization-table-selectors parameter string, of the form N[,N,...]
// If there are more components than parameters, the last value is replicated.
// 
boolean set_quant_slots(j_compress_ptr cinfo, char * arg)
{
	int val = 0; // default table # 
	for(int ci = 0; ci < MAX_COMPONENTS; ci++) {
		if(*arg) {
			char ch = ','; // if not set by sscanf, will be ',' 
			if(sscanf(arg, "%d%c", &val, &ch) < 1)
				return FALSE;
			if(ch != ',') /* syntax check */
				return FALSE;
			if(val < 0 || val >= NUM_QUANT_TBLS) {
				slfprintf_stderr("JPEG quantization tables are numbered 0..%d\n", NUM_QUANT_TBLS-1);
				return FALSE;
			}
			cinfo->comp_info[ci].quant_tbl_no = val;
			while(*arg && *arg++ != ',') /* advance to next segment of arg string */
				;
		}
		else { // reached end of parameter, set remaining components to last table 
			cinfo->comp_info[ci].quant_tbl_no = val;
		}
	}
	return TRUE;
}
// 
// Descr: Process a sample-factors parameter string, of the form HxV[,HxV,...]
// If there are more components than parameters, "1x1" is assumed for the rest.
// 
boolean set_sample_factors(j_compress_ptr cinfo, char * arg)
{
	int val1, val2;
	char ch1, ch2;
	for(int ci = 0; ci < MAX_COMPONENTS; ci++) {
		if(*arg) {
			ch2 = ','; /* if not set by sscanf, will be ',' */
			if(sscanf(arg, "%d%c%d%c", &val1, &ch1, &val2, &ch2) < 3)
				return FALSE;
			if(!oneof2(ch1, 'x', 'X') || ch2 != ',') /* syntax check */
				return FALSE;
			if(val1 <= 0 || val1 > MAX_SAMP_FACTOR || val2 <= 0 || val2 > MAX_SAMP_FACTOR) {
				slfprintf_stderr("JPEG sampling factors must be 1..%d\n", MAX_SAMP_FACTOR);
				return FALSE;
			}
			cinfo->comp_info[ci].h_samp_factor = val1;
			cinfo->comp_info[ci].v_samp_factor = val2;
			while(*arg && *arg++ != ',') /* advance to next segment of arg string */
				;
		}
		else {
			/* reached end of parameter, set remaining components to 1x1 sampling */
			cinfo->comp_info[ci].h_samp_factor = 1;
			cinfo->comp_info[ci].v_samp_factor = 1;
		}
	}
	return TRUE;
}
