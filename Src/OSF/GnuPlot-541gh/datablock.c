// GNUPLOT - datablock.c 
// Copyright Ethan A Merritt 2012
//
#include <gnuplot.h>
#pragma hdrstop

static int enlarge_datablock(GpValue * datablock_value, int extra);
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
//void datablock_command()
void GpProgram::DatablockCommand()
{
	FILE * fin;
	char * name, * eod;
	int nlines;
	int nsize = 4;
	udvt_entry * datablock;
	char * dataline = NULL;
	if(!IsLetter(GetCurTokenIdx()+1))
		GPO.IntErrorCurToken("illegal datablock name");
	// Create or recycle a datablock with the requested name 
	name = ParseDatablockName();
	datablock = GPO.Ev.AddUdvByName(name);
	if(!EqualsCur("<<") || !IsLetter(GetCurTokenIdx()+1))
		GPO.IntErrorCurToken("data block name must be followed by << EODmarker");
	if(datablock->udv_value.type != NOTDEFINED)
		gpfree_datablock(&datablock->udv_value);
	datablock->udv_value.type = DATABLOCK;
	datablock->udv_value.v.data_array = NULL;
	Shift();
	eod = (char *)gp_alloc(P_Token[CToken].length +2, "datablock");
	CopyStr(&eod[0], CToken, P_Token[CToken].length+2);
	Shift();
	// Read in and store data lines until EOD 
	fin = (lf_head == NULL) ? stdin : lf_head->fp;
	if(!fin)
		GPO.IntError(NO_CARET, "attempt to define data block from invalid context");
	for(nlines = 0; (dataline = df_fgets(fin)); nlines++) {
		int n;
		if(!strncmp(eod, dataline, strlen(eod)))
			break;
		// Allocate space for data lines plus at least 2 empty lines at the end. 
		if(nlines >= nsize-4) {
			nsize *= 2;
			datablock->udv_value.v.data_array = (char**)gp_realloc(datablock->udv_value.v.data_array, nsize * sizeof(char *), "datablock");
			memzero(&datablock->udv_value.v.data_array[nlines], (nsize - nlines) * sizeof(char *));
		}
		// Strip trailing newline character 
		n = strlen(dataline);
		if(n > 0 && dataline[n - 1] == '\n')
			dataline[n - 1] = NUL;
		datablock->udv_value.v.data_array[nlines] = gp_strdup(dataline);
	}
	inline_num += nlines + 1; // Update position in input file 
	// make sure that we can safely add lines to this datablock later on 
	enlarge_datablock(&datablock->udv_value, 0);
	SAlloc::F(eod);
}

//char * parse_datablock_name()
char * GpProgram::ParseDatablockName()
{
	// Datablock names begin with $, but the scanner puts  
	// the $ in a separate token.  Merge it with the next. 
	// Caller must _not_ free the string that is returned. 
	static char * name = NULL;
	SAlloc::F(name);
	Shift();
	name = (char *)gp_alloc(P_Token[CToken].length + 2, "datablock");
	name[0] = '$';
	CopyStr(&name[1], CToken, P_Token[CToken].length + 2);
	Shift();
	return name;
}

char ** get_datablock(char * name)
{
	udvt_entry * datablock = GPO.Ev.GetUdvByName(name);
	if(!datablock || datablock->udv_value.type != DATABLOCK ||  datablock->udv_value.v.data_array == NULL)
		GPO.IntError(NO_CARET, "no datablock named %s", name);
	return datablock->udv_value.v.data_array;
}

void gpfree_datablock(GpValue * datablock_value)
{
	if(datablock_value->type == DATABLOCK) {
		char ** pp_stored_data = datablock_value->v.data_array;
		if(pp_stored_data)
			for(int i = 0; pp_stored_data[i] != NULL; i++)
				SAlloc::F(pp_stored_data[i]);
		SAlloc::F(pp_stored_data);
		datablock_value->v.data_array = NULL;
		datablock_value->type = NOTDEFINED;
	}
}
//
// count number of lines in a datablock 
//
int datablock_size(GpValue * datablock_value)
{
	int nlines = 0;
	char ** dataline = datablock_value->v.data_array;
	if(dataline) {
		while(*dataline++)
			nlines++;
	}
	return nlines;
}

/* resize or allocate a datablock; allocate memory in chunks */
static int enlarge_datablock(GpValue * datablock_value, int extra)
{
	const int blocksize = 512;
	int nlines = datablock_size(datablock_value);
	/* reserve space in multiples of blocksize */
	int osize = ((nlines+1 + blocksize-1) / blocksize) * blocksize;
	int nsize = ((nlines+1 + extra + blocksize-1) / blocksize) * blocksize;
	/* only resize if necessary */
	if((osize != nsize) || (extra == 0) || (nlines == 0)) {
		datablock_value->v.data_array = (char**)gp_realloc(datablock_value->v.data_array,  nsize * sizeof(char *), "resize_datablock");
		datablock_value->v.data_array[nlines] = NULL;
	}
	return nlines;
}

/* append a single line to a datablock */
void append_to_datablock(GpValue * datablock_value, const char * line)
{
	int nlines = enlarge_datablock(datablock_value, 1);
	datablock_value->v.data_array[nlines] = (char*)line;
	datablock_value->v.data_array[nlines + 1] = NULL;
}

/* append multiple lines which are separated by linebreaks to a datablock */
void append_multiline_to_datablock(GpValue * datablock_value, const char * lines)
{
	char * l = (char*)lines;
	bool inquote = FALSE;
	bool escaped = FALSE;
	/* handle lines with line-breaks, one at a time;
	   take care of quoted strings
	 */
	char * p = l;
	while(*p != NUL) {
		if(*p == '\'' && !escaped)
			inquote = !inquote;
		else if(*p == '\\' && !escaped)
			escaped = TRUE;
		else if(*p == '\n' && !inquote) {
			*p = NUL;
			append_to_datablock(datablock_value, sstrdup(l));
			l = p + 1;
		}
		else
			escaped = FALSE;
		p++;
	}
	if(l == lines) {
		/* no line-breaks, just a single line */
		append_to_datablock(datablock_value, l);
	}
	else {
		if(strlen(l) > 0) /* remainder after last line-break */
			append_to_datablock(datablock_value, sstrdup(l));
		/* we allocated new sub-strings, free the original */
		SAlloc::F((char*)lines);
	}
}
