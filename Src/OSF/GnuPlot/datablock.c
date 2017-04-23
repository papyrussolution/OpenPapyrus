/*
 * $Id: datablock.c,v 1.8 2016/03/18 05:43:54 sfeam Exp $
 */
/* GNUPLOT - datablock.c */

/*[
 * Copyright Ethan A Merritt 2012
 *
 * Gnuplot license:
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
 *
 * Alternative license:
 *
 * As an alternative to distributing code in this file under the gnuplot license,
 * you may instead comply with the terms below. In this case, redistribution and
 * use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.  Redistributions in binary
 * form must reproduce the above copyright notice, this list of conditions and
 * the following disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
   ]*/
#include <gnuplot.h>
#pragma hdrstop
#include <io.h>

static int enlarge_datablock(t_value * datablock_value, int extra);

/*
 * In-line data blocks are implemented as a here-document:
 * $FOO << EOD
 *  data line 1
 *  data line 2
 *  ...
 * EOD
 *
 * The data block name must begin with $ followed by a letter.
 * The string EOD is arbitrary; lines of data will be read from the input stream
 * until the leading characters on the line match the given character string.
 * No attempt is made to parse the data at the time it is read in.
 */
void datablock_command(GpCommand & rC)
{
	FILE * fin;
	char * name, * eod;
	int nlines;
	int nsize = 4;
	UdvtEntry * datablock;
	char * dataline = NULL;
	if(!rC.IsLetter(rC.CToken+1))
		GpGg.IntErrorCurToken("illegal datablock name");
	// Create or recycle a datablock with the requested name 
	name = rC.ParseDataBlockName();
	datablock = GpGg.Ev.AddUdvByName(name);
	if(datablock->udv_value.type != NOTDEFINED)
		gpfree_datablock(&datablock->udv_value);
	datablock->udv_value.type = DATABLOCK;
	datablock->udv_value.v.data_array = NULL;
	if(!rC.Eq("<<") || !rC.IsLetter(rC.CToken+1))
		GpGg.IntErrorCurToken("data block name must be followed by << EODmarker");
	rC.CToken++;
	eod = (char*)malloc(rC.P_Token[rC.CToken].length +2);
	rC.CopyStr(&eod[0], rC.CToken, rC.P_Token[rC.CToken].length + 2);
	rC.CToken++;
	// Read in and store data lines until EOD 
	fin = (lf_head == NULL) ? stdin : lf_head->fp;
	if(!fin)
		GpGg.IntErrorNoCaret("attempt to define data block from invalid context");
	for(nlines = 0; (dataline = GpDf.DfFGets(rC, fin)); nlines++) {
		int n;
		if(!strncmp(eod, dataline, strlen(eod)))
			break;
		// Allocate space for data lines plus at least 2 empty lines at the end.
		if(nlines >= nsize-4) {
			nsize *= 2;
			datablock->udv_value.v.data_array = (char**)gp_realloc(datablock->udv_value.v.data_array,
				nsize * sizeof(char *), "datablock");
			memzero(&datablock->udv_value.v.data_array[nlines], (nsize - nlines) * sizeof(char *));
		}
		// Strip trailing newline character
		n = strlen(dataline);
		if(n > 0 && dataline[n - 1] == '\n')
			dataline[n - 1] = NUL;
		datablock->udv_value.v.data_array[nlines] = gp_strdup(dataline);
	}
	rC.InlineNum += nlines + 1; // Update position in input file
	// make sure that we can safely add lines to this datablock later on
	enlarge_datablock(&datablock->udv_value, 0);
	free(eod);
}

//char * parse_datablock_name()
char * GpCommand::ParseDataBlockName()
{
	// Datablock names begin with $, but the scanner puts
	// the $ in a separate token.  Merge it with the next.
	// Caller must _not_ free the string that is returned.
	static char * name = NULL; // @global
	free(name);
	CToken++;
	name = (char*)malloc(P_Token[CToken].length + 2);
	name[0] = '$';
	CopyStr(&name[1], CToken, P_Token[CToken].length + 2);
	CToken++;
	return name;
}

char ** get_datablock(char * name)
{
	UdvtEntry * datablock = GpGg.Ev.GetUdvByName(name);
	if(!datablock || datablock->udv_value.type == NOTDEFINED ||  datablock->udv_value.v.data_array == NULL)
		GpGg.IntErrorNoCaret("no datablock named %s", name);
	return datablock->udv_value.v.data_array;
}

void gpfree_datablock(t_value * datablock_value)
{
	int i;
	char ** stored_data = datablock_value->v.data_array;
	if(datablock_value->type != DATABLOCK)
		return;
	if(stored_data)
		for(i = 0; stored_data[i] != NULL; i++)
			free(stored_data[i]);
	free(stored_data);
	datablock_value->v.data_array = NULL;
}
//
// resize or allocate a datablock; allocate memory in chuncks
//
static int enlarge_datablock(t_value * datablock_value, int extra)
{
	char ** dataline;
	int nlines = 0;
	int osize, nsize;
	const int blocksize = 512;
	// count number of lines in datablock
	dataline = datablock_value->v.data_array;
	if(dataline) {
		while(*dataline++)
			nlines++;
	}

	/* reserve space in multiples of blocksize */
	osize = ((nlines+1 + blocksize-1) / blocksize) * blocksize;
	nsize = ((nlines+1 + extra + blocksize-1) / blocksize) * blocksize;

	/* only resize if necessary */
	if((osize != nsize) || (extra == 0) || (nlines == 0)) {
		datablock_value->v.data_array =
		    (char**)gp_realloc(datablock_value->v.data_array,  nsize * sizeof(char *), "resize_datablock");
		datablock_value->v.data_array[nlines] = NULL;
	}

	return nlines;
}

/* append a single line to a datablock */
void append_to_datablock(t_value * datablock_value, const char * line)
{
	int nlines = enlarge_datablock(datablock_value, 1);
	datablock_value->v.data_array[nlines] = (char*)line;
	datablock_value->v.data_array[nlines + 1] = NULL;
}

