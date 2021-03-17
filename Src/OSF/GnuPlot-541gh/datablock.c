// GNUPLOT - datablock.c 
// Copyright Ethan A Merritt 2012
//
#include <gnuplot.h>
#pragma hdrstop

static int enlarge_datablock(GpValue * datablock_value, int extra);
// 
// In-line data blocks are implemented as a here-document:
// $FOO << EOD
// data line 1
// data line 2
// ...
// EOD
// 
// The data block name must begin with $ followed by a letter.
// The string EOD is arbitrary; lines of data will be read from the input stream
// until the leading characters on the line match the given character string.
// No attempt is made to parse the data at the time it is read in.
// 
//void datablock_command()
void GnuPlot::DatablockCommand()
{
	FILE * fin;
	char * name, * eod;
	int nlines;
	int nsize = 4;
	udvt_entry * datablock;
	char * dataline = NULL;
	if(!Pgm.IsLetter(Pgm.GetCurTokenIdx()+1))
		IntErrorCurToken("illegal datablock name");
	// Create or recycle a datablock with the requested name 
	name = Pgm.ParseDatablockName();
	datablock = Ev.AddUdvByName(name);
	if(!Pgm.EqualsCur("<<") || !Pgm.IsLetter(Pgm.GetCurTokenIdx()+1))
		IntErrorCurToken("data block name must be followed by << EODmarker");
	if(datablock->udv_value.Type != NOTDEFINED)
		gpfree_datablock(&datablock->udv_value);
	datablock->udv_value.Type = DATABLOCK;
	datablock->udv_value.v.data_array = NULL;
	Pgm.Shift();
	eod = (char *)SAlloc::M(Pgm.ÑTok().Len+2);
	Pgm.CopyStr(&eod[0], Pgm.CToken, Pgm.ÑTok().Len+2);
	Pgm.Shift();
	// Read in and store data lines until EOD 
	fin = P_LfHead ? P_LfHead->fp : stdin;
	if(!fin)
		IntError(NO_CARET, "attempt to define data block from invalid context");
	for(nlines = 0; (dataline = DfGets(fin)); nlines++) {
		int n;
		if(!strncmp(eod, dataline, strlen(eod)))
			break;
		// Allocate space for data lines plus at least 2 empty lines at the end. 
		if(nlines >= nsize-4) {
			nsize *= 2;
			datablock->udv_value.v.data_array = (char**)SAlloc::R(datablock->udv_value.v.data_array, nsize * sizeof(char *));
			memzero(&datablock->udv_value.v.data_array[nlines], (nsize - nlines) * sizeof(char *));
		}
		// Strip trailing newline character 
		n = strlen(dataline);
		if(n > 0 && dataline[n-1] == '\n')
			dataline[n-1] = NUL;
		datablock->udv_value.v.data_array[nlines] = sstrdup(dataline);
	}
	Pgm.inline_num += nlines + 1; // Update position in input file 
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
	name = (char *)SAlloc::M(P_Token[CToken].Len + 2);
	name[0] = '$';
	CopyStr(&name[1], CToken, P_Token[CToken].Len + 2);
	Shift();
	return name;
}

//char ** get_datablock(const char * pName)
char ** GnuPlot::GetDatablock(const char * pName)
{
	udvt_entry * p_datablock = Ev.GetUdvByName(pName);
	if(!p_datablock || p_datablock->udv_value.Type != DATABLOCK || p_datablock->udv_value.v.data_array == NULL)
		IntError(NO_CARET, "no datablock named %s", pName);
	return p_datablock->udv_value.v.data_array;
}

void gpfree_datablock(GpValue * datablock_value)
{
	if(datablock_value->Type == DATABLOCK) {
		char ** pp_stored_data = datablock_value->v.data_array;
		if(pp_stored_data)
			for(int i = 0; pp_stored_data[i]; i++)
				SAlloc::F(pp_stored_data[i]);
		SAlloc::F(pp_stored_data);
		datablock_value->v.data_array = NULL;
		datablock_value->Type = NOTDEFINED;
	}
}
//
// count number of lines in a datablock 
//
//int datablock_size(const GpValue * pDatablockValue)
int GpValue::GetDatablockSize() const
{
	int nlines = 0;
	const char * const * pp_dataline = v.data_array;
	if(pp_dataline) {
		while(*pp_dataline++)
			nlines++;
	}
	return nlines;
}
//
// resize or allocate a datablock; allocate memory in chunks 
//
static int enlarge_datablock(GpValue * datablock_value, int extra)
{
	const int blocksize = 512;
	int nlines = datablock_value->GetDatablockSize();
	// reserve space in multiples of blocksize 
	int osize = ((nlines+1 + blocksize-1) / blocksize) * blocksize;
	int nsize = ((nlines+1 + extra + blocksize-1) / blocksize) * blocksize;
	// only resize if necessary 
	if(osize != nsize || !extra || !nlines) {
		datablock_value->v.data_array = (char **)SAlloc::R(datablock_value->v.data_array,  nsize * sizeof(char *));
		datablock_value->v.data_array[nlines] = NULL;
	}
	return nlines;
}
//
// append a single line to a datablock 
//
void append_to_datablock(GpValue * datablock_value, const char * line)
{
	int nlines = enlarge_datablock(datablock_value, 1);
	datablock_value->v.data_array[nlines] = (char *)line;
	datablock_value->v.data_array[nlines + 1] = NULL;
}
//
// append multiple lines which are separated by linebreaks to a datablock 
//
void append_multiline_to_datablock(GpValue * datablock_value, const char * lines)
{
	char * l = (char *)lines;
	bool inquote = false;
	bool escaped = false;
	// handle lines with line-breaks, one at a time; take care of quoted strings
	char * p = l;
	while(*p) {
		if(*p == '\'' && !escaped)
			inquote = !inquote;
		else if(*p == '\\' && !escaped)
			escaped = true;
		else if(*p == '\n' && !inquote) {
			*p = NUL;
			append_to_datablock(datablock_value, sstrdup(l));
			l = p + 1;
		}
		else
			escaped = false;
		p++;
	}
	if(l == lines) { // no line-breaks, just a single line 
		append_to_datablock(datablock_value, l);
	}
	else {
		if(strlen(l) > 0) /* remainder after last line-break */
			append_to_datablock(datablock_value, sstrdup(l));
		// we allocated new sub-strings, free the original 
		SAlloc::F((char *)lines);
	}
}
