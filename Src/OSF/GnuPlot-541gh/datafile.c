// GNUPLOT - datafile.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
// AUTHOR : David Denholm 
//
// this file provides the functions to handle data-file reading..
// takes care of all the pipe / stdin / index / using worries
//
/*{{{  notes */
/*
 * every a:b:c:d:e:f  - plot every a'th point from c to e,
 * in every b lines from d to f
 * ie for (line=d; line<=f; line+=b)
 *     for (point=c; point >=e; point+=a)
 *
 * public variables declared in this file.
 *    int df_no_use_specs - number of columns specified with 'using'
 *    int df_no_tic_specs - count of additional ticlabel columns
 *    int df_line_number  - for error reporting
 *    int df_datum        - increases with each data point
 *    int df_eof          - end of file
 *
 * functions
 *   int df_open(char *file_name, int max_using, plot_header *plot)
 *      parses index / using on command line
 *      max_using is max no of 'using' columns allowed (obsolete?)
 *	plot_header is NULL if called from fit or set_palette code
 *      returns number of 'using' cols specified, or -1 on error (?)
 *
 *   int df_readline(double vector[], int max)
 *      reads a line, does all the 'index' and 'using' manipulation
 *      deposits values into vector[]
 *      returns
 *    number of columns parsed  [0 = not a blank line, but no valid data],
 *    DF_EOF - end of file
 *    DF_UNDEFINED - undefined result during eval of extended using spec
 *    DF_MISSING - requested column matched that of 'set missing <foo>'
 *    DF_FIRST_BLANK - first consecutive blank line
 *    DF_SECOND_BLANK - second consecutive blank line
 *    DF_FOUND_KEY_TITLE  - only relevant to first line of data
 *    DF_KEY_TITLE_MISSING  and only for 'set key autotitle columnhead'
 *    DF_STRINGDATA - not currently used by anyone
 *    DF_COLUMN_HEADERS - first row used as headers rather than data
 *
 * if a using spec was given, lines not fulfilling spec are ignored.
 * we will always return exactly the number of items specified
 *
 * if no spec given, we return number of consecutive columns we parsed.
 *
 * if we are processing indexes, separated by 'n' blank lines,
 * we will return n-1 blank lines before noticing the index change
 *
 *   void df_close()
 *     closes a currently open file.
 *
 *    void f_dollars(x)
 *    void f_column()    actions for expressions using $i, column(j), etc
 *    void f_valid()
 *
 *
 * Line parsing is slightly differently from previous versions of gnuplot...
 * given a line containing fewer columns than asked for, gnuplot used to make
 * up values... Now if I have explicitly said 'using 1:2:3', then if
 * column 3 doesn't exist, I dont want this point...
 *
 */
/*}}} */
//
// Daniel Sebald: added general binary 2d data support. (20 August 2004)
//
#include <gnuplot.h>
#pragma hdrstop
#include <sys/stat.h>

static const char * too_many_cols_msg = "Too many columns in using specification and implied sampling array";

#define is_EOF(c) oneof2(c, 'e', 'E') /* test to see if the end of an inline datafile is reached */
#define is_comment(c) ((c) && (strchr(_Df.df_commentschars, (c)) != NULL)) /* is it a comment line? */
#define NOTSEP (!_Df.df_separators || !strchr(_Df.df_separators, *s)) /* Used to skip whitespace but not cross a field boundary */

enum COLUMN_TYPE { 
	CT_DEFAULT, 
	CT_STRING, 
	CT_KEYLABEL, 
	CT_MUST_HAVE, 
	CT_XTICLABEL, 
	CT_X2TICLABEL, 
	CT_YTICLABEL, 
	CT_Y2TICLABEL, 
	CT_ZTICLABEL, 
	CT_CBTICLABEL 
};

/*{{{  static fns */
//static int  check_missing(const char * s);
//static bool valid_format(const char *);
//static int  axcol_for_ticlabel(enum COLUMN_TYPE type, int * axis);

#define DATA_LINE_BUFSIZ 160

#if defined(PIPES)
	//static bool df_pipe_open = FALSE;
#endif
#if defined(HAVE_FDOPEN)
	//static int data_fd = -2; // only used for file redirection
#endif
#ifndef MAXINT                  /* should there be one already defined ? */
	#define MAXINT INT_MAX        /* from <limits.h> */
#endif

//static void initialize_use_spec();
//static void df_insert_scanned_use_spec(int);
//static void clear_binary_records(df_records_type);
//static bool rotation_matrix_2D(double R[][2], double);
//static bool rotation_matrix_3D(double P[][3], double *);
//static void df_swap_bytes_by_endianess(char *, int, int);

const char * df_endian[DF_ENDIAN_TYPE_LENGTH] = { "little", "pdp (middle)", "swapped pdp (dimmle)", "big" };

#define SUPPORT_MIDDLE_ENDIAN 1

#if SUPPORT_MIDDLE_ENDIAN
/* To generate a swap, take the bit-wise complement of the lowest two bits. */
typedef enum df_byte_read_order_type {
	DF_0123,
	DF_1032,
	DF_2301,
	DF_3210
} df_byte_read_order_type;

// First argument, this program's endianess.  Second argument, file's endianess.
// Don't use directly.  Use 'byte_read_order()' function instead.
static const char df_byte_read_order_map[4][4] = {
	{DF_0123, DF_1032, DF_2301, DF_3210},
	{DF_1032, DF_0123, DF_1032, DF_2301},
	{DF_2301, DF_1032, DF_0123, DF_1032},
	{DF_3210, DF_2301, DF_1032, DF_0123}
};

static const long long_0x2468 = 0x2468;
#define TEST_BIG_PDP         ( (((char *)&long_0x2468)[0] < 3) ? DF_BIG_ENDIAN : DF_PDP_ENDIAN )
#define THIS_COMPILER_ENDIAN ( (((char *)&long_0x2468)[0] < 5) ? TEST_BIG_PDP : DF_LITTLE_ENDIAN )

// Argument is file's endianess type
static df_byte_read_order_type byte_read_order(df_endianess_type);

// Settings that are transferred to default upon reset.
const df_binary_file_record_struct df_bin_record_reset = {
	{-1, 0, 0},
	{1, 1, 1},
	{1, 1, 1},
	DF_TRANSLATE_DEFAULT,
	{0, 0, 0},
	0,
	{0, 0, 1},

	{DF_SCAN_POINT, DF_SCAN_LINE, DF_SCAN_PLANE},
	FALSE,
	{0, 0, 0},

	{0, 0, 0},
	{1, 1, 1},
	{0, 0, 0},
	DF_TRANSLATE_DEFAULT,
	{0, 0, 0},

	0, 0,      /* submatrix size */
	NULL       /* data_memory */
};

// Used to mark the location of a blank line in the original data input file 
//GpCoordinate blank_data_line = {UNDEFINED, -999, -999, -999, -999, -999, -999, -999};
const GpCoordinate blank_data_line(GpCoordinate::ctrBlank);

//static void gpbin_filetype_function();
//static void raw_filetype_function();
//static void avs_filetype_function();
static void (* binary_input_function)(); /* Will point to one of the above */
static void auto_filetype_function() {} // Just a placeholder for auto    

/*
	GnuPlot 
		enum {
			filtypUndef = 0,
			filtypAvs = 1,
			filtypBin,
			filtypEdf, // edf, ehf
			filtypGif,
			filtypGpBin,
			filtypJpeg, // jpeg, jpg
			filtypPng,
			filtypRaw,
			filtypRgb,
			filtypAuto,
		};
*/

static const SIntToSymbTabEntry DfBinFileTypeTable[] = {
	{ GnuPlot::filtypAvs, "avs"},
	{ GnuPlot::filtypBin, "bin"},
	{ GnuPlot::filtypEdf, "edf"},
	{ GnuPlot::filtypEhf, "ehf"},
	{ GnuPlot::filtypGif, "gif"},
	{ GnuPlot::filtypGpBin, "gpbin"},
	{ GnuPlot::filtypJpeg, "jpeg"},
	{ GnuPlot::filtypJpeg, "jpg"},
	{ GnuPlot::filtypPng, "png"},
	{ GnuPlot::filtypRaw, "raw"},
	{ GnuPlot::filtypRgb, "rgb"},
	{ GnuPlot::filtypAuto, "auto"},
};

/*static const gen_ftable df_bin_filetype_table[] = {
	{"avs", avs_filetype_function},
	{"bin", raw_filetype_function},
	{"edf", edf_filetype_function},
	{"ehf", edf_filetype_function},
	{"gif", gif_filetype_function},
	{"gpbin", gpbin_filetype_function},
	{"jpeg", jpeg_filetype_function},
	{"jpg", jpeg_filetype_function},
	{"png", png_filetype_function},
	{"raw", raw_filetype_function},
	{"rgb", raw_filetype_function},
	{"auto", auto_filetype_function},
	{NULL,   NULL}
};*/
#define RAW_FILETYPE 1

#define DF_BIN_FILE_ENDIANESS_RESET THIS_COMPILER_ENDIAN
//df_endianess_type df_bin_file_endianess; // This one is needed by breaders.c 

struct df_bin_scan_table_2D_struct {
	const char * string;
	df_sample_scan_type scan[3];
};

static const df_bin_scan_table_2D_struct df_bin_scan_table_2D[] = {
	{"xy", {DF_SCAN_POINT, DF_SCAN_LINE,  DF_SCAN_PLANE}},
	{"yx", {DF_SCAN_LINE,  DF_SCAN_POINT, DF_SCAN_PLANE}},
	{"tr", {DF_SCAN_POINT, DF_SCAN_LINE,  DF_SCAN_PLANE}},
	{"rt", {DF_SCAN_LINE,  DF_SCAN_POINT, DF_SCAN_PLANE}}
};
#define TRANSPOSE_INDEX 1

struct df_bin_scan_table_3D_struct {
	const char * string;
	df_sample_scan_type scan[3];
};

static const df_bin_scan_table_3D_struct df_bin_scan_table_3D[] = {
	{"xyz", {DF_SCAN_POINT, DF_SCAN_LINE,  DF_SCAN_PLANE}},
	{"zxy", {DF_SCAN_LINE,  DF_SCAN_PLANE, DF_SCAN_POINT}},
	{"yzx", {DF_SCAN_PLANE, DF_SCAN_POINT, DF_SCAN_LINE}},
	{"yxz", {DF_SCAN_LINE,  DF_SCAN_POINT, DF_SCAN_PLANE}},
	{"xzy", {DF_SCAN_POINT, DF_SCAN_PLANE, DF_SCAN_LINE}},
	{"zyx", {DF_SCAN_PLANE, DF_SCAN_LINE,  DF_SCAN_POINT}},
	{"trz", {DF_SCAN_POINT, DF_SCAN_LINE,  DF_SCAN_PLANE}},
	{"ztr", {DF_SCAN_LINE,  DF_SCAN_PLANE, DF_SCAN_POINT}},
	{"rzt", {DF_SCAN_PLANE, DF_SCAN_POINT, DF_SCAN_LINE}},
	{"rtz", {DF_SCAN_LINE,  DF_SCAN_POINT, DF_SCAN_PLANE}},
	{"tzr", {DF_SCAN_POINT, DF_SCAN_PLANE, DF_SCAN_LINE}},
	{"zrt", {DF_SCAN_PLANE, DF_SCAN_LINE,  DF_SCAN_POINT}}
};
//
// Names for machine dependent field sizes. 
//
static const char * ch_names[] = {"char", "schar", "c"};
static const char * uc_names[] = {"uchar"};
static const char * sh_names[] = {"short"};
static const char * us_names[] = {"ushort"};
static const char * in_names[] = {"int", "sint", "i", "d"};
static const char * ui_names[] = {"uint", "u"};
static const char * lo_names[] = {"long", "ld"};
static const char * ul_names[] = {"ulong", "lu"};
static const char * fl_names[] = {"float", "f"};
static const char * db_names[] = {"double", "lf"};
//
// Machine independent names. 
//
static const char * byte_names[]   = {"int8", "byte"};
static const char * ubyte_names[]  = {"uint8", "ubyte"};
static const char * word_names[]   = {"int16", "word"};
static const char * uword_names[]  = {"uint16", "uword"};
static const char * word2_names[]  = {"int32"};
static const char * uword2_names[] = {"uint32"};
static const char * word4_names[]  = {"int64"};
static const char * uword4_names[] = {"uint64"};
static const char * float_names[]  = {"float32"};
static const char * float2_names[] = {"float64"};

struct df_binary_details_struct {
	const char ** name;
	ushort no_names;
	df_binary_type_struct type;
};

struct df_binary_tables_struct {
	const df_binary_details_struct * group;
	ushort group_length;
};

static const df_binary_details_struct df_binary_details[] = {
	{ch_names, SIZEOFARRAY(ch_names), {DF_CHAR, sizeof(char)}},
	{uc_names, SIZEOFARRAY(uc_names), {DF_UCHAR, sizeof(uchar)}},
	{sh_names, SIZEOFARRAY(sh_names), {DF_SHORT, sizeof(short)}},
	{us_names, SIZEOFARRAY(us_names), {DF_USHORT, sizeof(ushort)}},
	{in_names, SIZEOFARRAY(in_names), {DF_INT, sizeof(int)}},
	{ui_names, SIZEOFARRAY(ui_names), {DF_UINT, sizeof(uint)}},
	{lo_names, SIZEOFARRAY(lo_names), {DF_LONG, sizeof(long)}},
	{ul_names, SIZEOFARRAY(ul_names), {DF_ULONG, sizeof(ulong)}},
	{fl_names, SIZEOFARRAY(fl_names), {DF_FLOAT, sizeof(float)}},
	{db_names, SIZEOFARRAY(db_names), {DF_DOUBLE, sizeof(double)}},
	{NULL, 0, {DF_LONGLONG, sizeof(int64)}},
	{NULL, 0, {DF_ULONGLONG, sizeof(uint64)}}
};

static const df_binary_details_struct df_binary_details_independent[] = {
	{byte_names,   SIZEOFARRAY(byte_names), {SIGNED_TEST(1), 1}},
	{ubyte_names,  SIZEOFARRAY(ubyte_names), {UNSIGNED_TEST(1), 1}},
	{word_names,   SIZEOFARRAY(word_names), {SIGNED_TEST(2), 2}},
	{uword_names,  SIZEOFARRAY(uword_names), {UNSIGNED_TEST(2), 2}},
	{word2_names,  SIZEOFARRAY(word2_names), {SIGNED_TEST(4), 4}},
	{uword2_names, SIZEOFARRAY(uword2_names), {UNSIGNED_TEST(4), 4}},
	{word4_names,  SIZEOFARRAY(word4_names), {SIGNED_TEST(8), 8}},
	{uword4_names, SIZEOFARRAY(uword4_names), {UNSIGNED_TEST(8), 8}},
	{float_names,  SIZEOFARRAY(float_names), {FLOAT_TEST(4), 4}},
	{float2_names, SIZEOFARRAY(float2_names), {FLOAT_TEST(8), 8}}
};

static const df_binary_tables_struct df_binary_tables[] = {
	{df_binary_details, SIZEOFARRAY(df_binary_details)},
	{df_binary_details_independent, SIZEOFARRAY(df_binary_details_independent)}
};
//
// Information about binary data structure, to be determined by the
// using and format options.  This should be one greater than df_no_bin_cols.
//
static const char * matrix_general_binary_conflict_msg = "Conflict between some matrix binary and general binary keywords";
static const char * equal_symbol_msg = "Equal ('=') symbol required";

#endif

/*}}} */
//
// Initialize input buffer used by df_gets and df_fgets. 
// Called via reset_command() on program entry.		 
//
//void df_init()
void GnuPlot::DfInit()
{
	if(_Df.MaxLineLen < DATA_LINE_BUFSIZ) {
		_Df.MaxLineLen = DATA_LINE_BUFSIZ;
		_Df.df_line = (char *)SAlloc::M(_Df.MaxLineLen);
	}
}

//static char * df_gets()
char * GnuPlot::DfGets()
{
	// HBB 20000526: prompt user for inline data, if in interactive mode 
	if(_Df.mixed_data_fp && _Plt.interactive)
		fputs("input data ('e' ends) > ", stderr);
	// Special pseudofiles '+' and '++' return coords of sample 
	if(_Df.df_pseudodata)
		return DfGeneratePseudodata();
	if(_Df.df_datablock)
		return *(_Df.df_datablock_line++);
	if(_Pb.df_array)
		return DfGenerateAsciiArrayEntry();
	return DfGets(_Df.data_fp);
}
// 
// This one is shared by df_gets() and by datablock.c:datablock_command
// 
//char * df_fgets(FILE * fin)
char * GnuPlot::DfGets(FILE * fin)
{
	int len = 0;
	if(!fgets(_Df.df_line, _Df.MaxLineLen, fin))
		return NULL;
	if(_Df.mixed_data_fp)
		++Pgm.inline_num;
	for(;;) {
		len += strlen(_Df.df_line + len);
		if(len > 0 && _Df.df_line[len-1] == '\n') {
			// we have read an entire text-file line. Strip the trailing linefeed and return
			_Df.df_line[len-1] = 0;
			return _Df.df_line;
		}
		if((_Df.MaxLineLen - len) < 32)
			_Df.df_line = (char *)SAlloc::R(_Df.df_line, _Df.MaxLineLen *= 2);
		if(!fgets(_Df.df_line + len, _Df.MaxLineLen - len, fin))
			return _Df.df_line; // unexpected end of file, but we have something to do 
	}
	/* NOTREACHED */
	return NULL;
}

/*}}} */

//static int df_tokenise(char * s)
int GnuPlot::DfTokenise(char * s)
{
	// implement our own sscanf that takes 'missing' into account,
	// and can understand fortran quad format
	bool in_string;
	int i;
	// "here data" lines may end in \n rather than \0. 
	// DOS/Windows lines may end in \r rather than \0. 
	if(s[strlen(s)-1] == '\n' || s[strlen(s)-1] == '\r')
		s[strlen(s)-1] = '\0';
	for(i = 0; i < MAXDATACOLS; i++)
		_Df.df_tokens[i] = NULL;
	_Df.df_no_cols = 0;
	while(*s) {
		// We may poke at 2 new fields before coming back here - make sure there is room 
		if(_Df.df_max_cols <= _Df.df_no_cols + 2)
			_Df.ExpandColumn((_Df.df_max_cols < 20) ? _Df.df_max_cols+20 : 2*_Df.df_max_cols);
		// have always skipped spaces at this point 
		_Df.df_column[_Df.df_no_cols].position = s;
		in_string = FALSE;
		// Keep pointer to start of this token if user wanted it for anything, particularly if it is a string 
		for(i = 0; i<MAXDATACOLS; i++) {
			if(_Df.df_no_cols == _Df.UseSpec[i].column-1) {
				_Df.df_tokens[i] = s;
				if(_Df.UseSpec[i].expected_type == CT_STRING)
					_Df.df_column[_Df.df_no_cols].good = DF_GOOD;
			}
		}
		// CSV files must accept numbers inside quotes also,
		// so we step past the quote 
		if(*s == '"' && _Df.df_separators) {
			in_string = TRUE;
			_Df.df_column[_Df.df_no_cols].position = ++s;
		}
		if(*s == '"') {
			// treat contents of a quoted string as single column 
			in_string = !in_string;
			_Df.df_column[_Df.df_no_cols].good = DF_STRINGDATA;
		}
		else if(CheckMissing(s)) {
			_Df.df_column[_Df.df_no_cols].good = DF_MISSING;
			_Df.df_column[_Df.df_no_cols].datum = fgetnan();
			_Df.df_column[_Df.df_no_cols].position = NULL;
		}
		else {
			int used;
			int count;
			int dfncp1 = _Df.df_no_cols + 1;
			// optimizations by Corey Satten, corey@cac.washington.edu 
			// only scanf the field if it is mentioned in one of the using specs 
			if(!_Df.fast_columns || (_Df.df_no_use_specs == 0) || ((_Df.df_no_use_specs > 0) && (_Df.UseSpec[0].column == dfncp1 || 
				(_Df.df_no_use_specs > 1 && (_Df.UseSpec[1].column == dfncp1 || (_Df.df_no_use_specs > 2 && (_Df.UseSpec[2].column == dfncp1 || 
				(_Df.df_no_use_specs > 3 && (_Df.UseSpec[3].column == dfncp1 || (_Df.df_no_use_specs > 4 && (_Df.UseSpec[4].column == dfncp1 || 
				_Df.df_no_use_specs > 5))))))))))) {
				/* This was the [slow] code used through version 4.0
				 *   count = sscanf(s, "%lf%n", &df_column[df_no_cols].datum, &used);
				 */
				/* Use strtod() because
				 *  - it is faster than sscanf()
				 *  - sscanf(... %n ...) may not be portable
				 *  - it allows error checking
				 *  - atof() does not return a count or new position
				 */
				char * next;
				_Df.df_column[_Df.df_no_cols].datum = strtod(s, &next);
				used = next - s;
				count = (used) ? 1 : 0;
			}
			else {
				// skip any space at start of column 
				while(isspace((uchar)*s) && NOTSEP)
					++s;
				count = (*s && NOTSEP) ? 1 : 0;
				// skip chars to end of column 
				used = 0;
				if(_Df.df_separators && in_string) {
					do {
						++s;
					} while(*s && *s != '"');
					in_string = FALSE;
				}
				while(!isspace((uchar)*s) && *s && NOTSEP)
					++s;
			}
			// it might be a fortran double or quad precision. 'used' is only safe if count is 1
			if(_Df.df_fortran_constants && count == 1 && (s[used] == 'd' || s[used] == 'D' || s[used] == 'q' || s[used] == 'Q')) {
				// HBB 20001221: avoid breaking parsing of time/date
				// strings like 01Dec2000 that would be caused by
				// overwriting the 'D' with an 'e'... 
				char * endptr;
				char save_char = s[used];
				// might be fortran double 
				s[used] = 'e';
				// and try again 
				_Df.df_column[_Df.df_no_cols].datum = strtod(s, &endptr);
				count = (endptr == s) ? 0 : 1;
				s[used] = save_char;
			}
			_Df.df_column[_Df.df_no_cols].good = count == 1 ? DF_GOOD : DF_BAD;
			if(isnan(_Df.df_column[_Df.df_no_cols].datum)) {
				_Df.df_column[_Df.df_no_cols].good = DF_UNDEFINED;
				FPRINTF((stderr, "NaN in column %d\n", _Df.df_no_cols));
			}
		}
		++_Df.df_no_cols;
		// If we are in a quoted string, skip to end of quote 
		if(in_string) {
			do {
				s++;
			} while(*s && (uchar)*s != '"');
		}
		// skip to 1st character in the next field 
		if(_Df.df_separators) {
			// skip to next separator or end of line 
			while((*s != '\0') && (*s != '\n') && NOTSEP)
				++s;
			if((*s == '\0') || (*s == '\n')) /* End of line; we're done */
				break;
			/* step over field separator */
			++s;
			/* skip whitespace at start of next field */
			while((*s == ' ' || *s == '\t') && NOTSEP)
				++s;
			if((*s == '\0') || (*s == '\n')) { /* Last field is empty */
				_Df.df_column[_Df.df_no_cols].good = DF_MISSING;
				_Df.df_column[_Df.df_no_cols].datum = fgetnan();
				++_Df.df_no_cols;
				break;
			}
		}
		else {
			// skip trash chars remaining in this column 
			while((*s != '\0') && (*s != '\n') && !isspace((uchar)*s))
				++s;
			// skip whitespace to start of next column 
			while(isspace((uchar)*s) && *s != '\n')
				++s;
		}
	}
	return _Df.df_no_cols;
}

/*{{{  static double *df_read_matrix() */
/* Reads a matrix from a text file and stores it in allocated memory.
 *
 * IMPORTANT NOTE:  The routine returns the memory pointer for that matrix,
 * but does not retain the pointer.  Maintenance of the memory is left to
 * the calling code.
 */
//static double * df_read_matrix(int * pRows, int * pCols)
double * GnuPlot::DfReadMatrix(int * pRows, int * pCols)
{
	int    max_rows = 0;
	int    c;
	double * linearized_matrix = NULL;
	char * s;
	int    index = 0;
	_Df.df_bad_matrix_values = 0;
	*pRows = 0;
	*pCols = 0;
	for(;;) {
		if(!(s = DfGets())) {
			_Df.df_eof = 1;
			// NULL if we have not read anything yet 
			return linearized_matrix;
		}
		// skip leading spaces 
		while(isspace((uchar)*s) && NOTSEP)
			++s;
		// skip blank lines and comments 
		if(!*s || is_comment(*s)) {
			// except that some comments hide an index name 
			if(_Df.P_IndexName) {
				while(is_comment(*s) || isspace((uchar)*s))
					++s;
				if(*s && !strncmp(s, _Df.P_IndexName, strlen(_Df.P_IndexName)))
					_Df.IndexFound = true;
			}
			// This whole section copied with minor tweaks from df_readascii() 
			if(++_Df.BlankCount == 1) {
				// first blank line 
				if(linearized_matrix)
					return linearized_matrix;
				if(_Df.P_IndexName && !_Df.IndexFound)
					continue;
				if(_Df.df_current_index < _Df.df_lower_index)
					continue;
			}
			if(_Df.BlankCount == 2) {
				// just reached the end of a data block 
				++_Df.df_current_index;
				if(_Df.P_IndexName && _Df.IndexFound) {
					_Df.df_eof = 1;
					return linearized_matrix;
				}
				if(_Df.df_current_index <= _Df.df_lower_index)
					continue;
				if(_Df.df_current_index > _Df.df_upper_index) {
					_Df.df_eof = 1;
					return linearized_matrix;
				}
			}
			else {
				continue; // Ignore any blank lines beyond the 2nd 
			}
		}
		// get here => was not blank 
		_Df.df_last_index_read = _Df.df_current_index;
		// TODO:  Handle columnheaders for 2nd and subsequent data blocks?
		// if (blank_count >= 2) { do something }
		_Df.BlankCount = 0;
		if(_Df.mixed_data_fp && is_EOF(*s)) {
			_Df.df_eof = 1;
			return linearized_matrix;
		}
		c = DfTokenise(s);
		if(!c)
			return linearized_matrix;
		// If the first row of matrix data contains column headers 
		if(!_Df.df_already_got_headers && _Df.df_matrix_columnheaders && *pRows == 0) {
			int i;
			char * temp_string;
			_Df.df_already_got_headers = true;
			for(i = (_Df.df_matrix_rowheaders ? 1 : 0); i < c; i++) {
				double xpos = _Df.df_matrix_rowheaders ? (i-1) : i;
				if(_Df.UseSpec[0].at) {
					GpValue a;
					_Df.df_column[0].datum = xpos;
					_Df.df_column[0].good = DF_GOOD;
					_Df.evaluate_inside_using = true;
					EvaluateAt(_Df.UseSpec[0].at, &a);
					_Df.evaluate_inside_using = false;
					xpos = Real(&a);
				}
				temp_string = DfParseStringField(_Df.df_column[i].position);
				AddTicUser(&AxS[FIRST_X_AXIS], temp_string, xpos, -1);
				SAlloc::F(temp_string);
			}
			continue;
		}
		if(*pCols && c != *pCols) {
			// it's not regular 
			SAlloc::F(linearized_matrix);
			IntError(NO_CARET, "Matrix does not represent a grid");
		}
		*pCols = c;
		++*pRows;
		if(*pRows > max_rows) {
			max_rows = MAX(2*max_rows, 1);
			linearized_matrix = (double *)SAlloc::R(linearized_matrix, *pCols * max_rows * sizeof(double));
		}
		// store data 
		{
			for(int i = 0; i < c; ++i) {
				// First column in "matrix rowheaders" is a ytic label 
				if(_Df.df_matrix_rowheaders && i == 0) {
					char * temp_string;
					double ypos = *pRows - 1;
					if(_Df.UseSpec[1].at) {
						// The save/restore is to make sure 1:(f($2)):3 works 
						GpValue a;
						const double save = _Df.df_column[1].datum;
						_Df.df_column[1].datum = ypos;
						_Df.evaluate_inside_using = true;
						EvaluateAt(_Df.UseSpec[1].at, &a);
						_Df.evaluate_inside_using = false;
						ypos = Real(&a);
						_Df.df_column[1].datum = save;
					}
					temp_string = DfParseStringField(_Df.df_column[0].position);
					AddTicUser(&AxS[FIRST_Y_AXIS], temp_string, ypos, -1);
					SAlloc::F(temp_string);
					continue;
				}
				if(i < _Df.FirstPoint && _Df.df_column[i].good != DF_GOOD) {
					// It's going to be skipped anyhow, so... 
					linearized_matrix[index++] = 0;
				}
				else
					linearized_matrix[index++] = _Df.df_column[i].datum;
				if(_Df.df_column[i].good != DF_GOOD) {
					if(_Df.df_nonuniform_matrix && index == 1)
						; // This field is typically a label or comment 
					else if(_Df.df_bad_matrix_values++ == 0)
						IntWarn(NO_CARET, "matrix contains missing or undefined values");
				}
			}
		}
	}
}

/*}}} */

//static void initialize_use_spec()
void GnuPlot::InitializeUseSpec()
{
	_Df.df_no_use_specs = 0;
	for(int i = 0; i < MAXDATACOLS; ++i) {
		_Df.UseSpec[i].column = i + 1; // default column 
		_Df.UseSpec[i].expected_type = CT_DEFAULT; // no particular expectation 
		if(_Df.UseSpec[i].at) {
			free_at(_Df.UseSpec[i].at);
			_Df.UseSpec[i].at = NULL; // no expression 
		}
		_Df.UseSpec[i].depends_on_column = -1; // we don't know of any dependence 
		_Df.df_axis[i] = NO_AXIS; // no P_TimeFormat for this output column 
	}
}

//static void initialize_plot_style(curve_points * plot)
void GnuPlot::InitializePlotStyle(curve_points * pPlot)
{
	if(pPlot) {
		const int save_token = Pgm.GetCurTokenIdx();
		for(; !Pgm.EndOfCommand(); /*c_token++*/Pgm.Shift()) {
			if(Pgm.AlmostEqualsCur("w$ith")) {
				pPlot->plot_style = GetStyle();
				break;
			}
		}
		Pgm.SetTokenIdx(save_token);
	}
}

/*{{{  int df_open(char *file_name, int max_using, plot_header *plot) */
// 
// open file, parsing using/index stuff return number of using
// specs [well, we have to return something !]
// 
//int df_open(const char * cmd_filename, int max_using, curve_points * plot)
int GnuPlot::DfOpen(const char * pCmdFileName, int maxUsing, curve_points * pPlot)
{
	int    name_token = Pgm.GetPrevTokenIdx();
	bool   duplication = FALSE;
	bool   set_index = FALSE, set_skip = FALSE;
	bool   set_using = FALSE;
	bool   set_matrix = FALSE;
	_Df.fast_columns = 1; /* corey@cac */
	// close file if necessary 
	if(_Df.data_fp) {
		DfClose();
		_Df.data_fp = NULL;
	}
	ZFREE(_Df.df_format); /* no format string */
	_Df.df_no_tic_specs = 0;
	ZFREE(_Df.df_key_title);
	InitializeUseSpec();
	_Df.ClearColumnHeaders();
	_Df.df_datum = -1; /* it will be preincremented before use */
	_Df.df_line_number = 0; /* ditto */
	_Df.df_lower_index = 0;
	_Df.df_index_step = 1;
	_Df.df_upper_index = MAXINT;
	ZFREE(_Df.P_IndexName);
	_Df.df_current_index = 0;
	_Df.df_last_index_read = 0;
	_Df.BlankCount = 2;
	// by initialising blank_count, leading blanks will be ignored 
	_Df.SetEvery = FALSE;
	_Df.EveryPoint = _Df.everyline = 1; /* unless there is an every spec */
	_Df.FirstPoint = _Df.firstline = 0;
	_Df.LastPoint = _Df.lastline = MAXINT;
	_Df.df_binary_file = _Df.df_matrix_file = false;
	_Df.df_pixeldata = NULL;
	_Df.NumBinRecords = 0;
	_Df.df_matrix = false;
	_Df.df_nonuniform_matrix = false;
	_Df.df_matrix_columnheaders = false;
	_Df.df_matrix_rowheaders = false;
	_Df.df_skip_at_front = 0;
	_Df.df_xpixels = 0;
	_Df.df_ypixels = 0;
	_Df.df_transpose = false;
	_Df.df_voxelgrid = false;
	_Df.df_eof = 0;
	// Save for use by df_readline(). 
	// Perhaps it should be a parameter to df_readline? 
	_Df.df_current_plot = pPlot;
	// If either 'set datafile columnhead' or 'set key autotitle columnhead'
	// is in effect we always treat the * first data row as non-data
	// (df_readline() will return DF_COLUMN_HEADERS rather than the column count).
	// This is true even if the key is off or the data is read from 'stats'
	// or from 'fit' rather than plot.
	_Df.ColumnForKeyTitle = NO_COLUMN_HEADER;
	_Df.df_already_got_headers = FALSE;
	if(Gg.KeyT.auto_titles == COLUMNHEAD_KEYTITLES)
		_Pb.parse_1st_row_as_headers = TRUE;
	else if(_Df.df_columnheaders)
		_Pb.parse_1st_row_as_headers = TRUE;
	else
		_Pb.parse_1st_row_as_headers = FALSE;
	if(!pCmdFileName)
		IntErrorCurToken("missing filename");
	if(!pCmdFileName[0]) {
		if(isempty(_Df.df_filename))
			IntError(Pgm.GetPrevTokenIdx(), "No previous filename");
		if(sstreq(_Df.df_filename, "@@") && _Df.df_arrayname) {
			_Pb.df_array = Ev.GetUdvByName(_Df.df_arrayname);
			if(_Pb.df_array->udv_value.Type != ARRAY)
				IntError(Pgm.GetPrevTokenIdx(), "Array %s invalid", _Df.df_arrayname);
		}
	}
	else if(pCmdFileName[0] == '$' && GetVGridByName(pCmdFileName)) {
		// The rest of the df_open() processing is not relevant 
		_Df.df_voxelgrid = true;
		return 1;
	}
	else {
		FREEANDASSIGN(_Df.df_filename, sstrdup(pCmdFileName));
	}
	// defer opening until we have parsed the modifiers... 
	// pm 25.11.2001 allow any order of options 
	while(!Pgm.EndOfCommand()) {
		// look for binary / matrix 
		if(Pgm.AlmostEqualsCur("bin$ary")) {
			if(_Df.df_filename[0] == '$')
				IntErrorCurToken("data blocks cannot be binary");
			if(sstreq(_Df.df_filename, "+") || sstreq(_Df.df_filename, "++"))
				IntErrorCurToken("pseudofiles '+' and '++' cannot be binary");
			Pgm.Shift();
			if(_Df.df_binary_file || set_skip) {
				duplication = true;
				break;
			}
			GpExpandTilde(&_Df.df_filename);
			_Df.df_binary_file = true;
			// Up to the time of adding the general binary code, only matrix
			// binary for 3d was defined.  So, use matrix binary by default.
			_Df.df_matrix_file = true;
			InitializeBinaryVars();
			PlotOptionBinary(set_matrix, FALSE);
			continue;
		}
		// deal with matrix 
		if(Pgm.AlmostEqualsCur("mat$rix")) {
			Pgm.Shift();
			if(set_matrix) {
				duplication = TRUE;
				break;
			}
			// `binary` default is both df_matrix_file and df_binary_file.
			// So if df_binary_file is true, but df_matrix_file isn't, then
			// some keyword specific to general binary has been given.
			if(!_Df.df_matrix_file && _Df.df_binary_file)
				IntErrorCurToken(matrix_general_binary_conflict_msg);
			_Df.df_matrix_file = true;
			set_matrix = TRUE;
			_Df.fast_columns = 0;
			continue;
		}
		// May 2011 - "nonuniform matrix" indicates an ascii data file
		// with the same row/column layout as "binary matrix" 
		if(Pgm.AlmostEqualsCur("nonuni$form")) {
			Pgm.Shift();
			_Df.df_matrix_file = true;
			_Df.df_nonuniform_matrix = true;
			_Df.fast_columns = 0;
			if(_Df.df_matrix_rowheaders || _Df.df_matrix_columnheaders)
				duplication = true;
			continue;
		}
		// Jul 2014 - "matrix columnheaders" indicates an ascii data file
		// in uniform grid format but with column labels in row 1 
		if(Pgm.AlmostEqualsCur("columnhead$ers")) {
			Pgm.Shift();
			_Df.df_matrix_file = true;
			_Df.df_matrix_columnheaders = true;
			if(_Df.df_nonuniform_matrix || !set_matrix)
				duplication = TRUE;
			continue;
		}
		// Jul 2014 - "matrix rowheaders" indicates an ascii data file
		// in uniform grid format but with row labels in column 1 
		if(Pgm.AlmostEqualsCur("rowhead$ers")) {
			Pgm.Shift();
			_Df.df_matrix_file = true;
			_Df.df_matrix_rowheaders = true;
			if(_Df.df_nonuniform_matrix || !set_matrix)
				duplication = true;
			continue;
		}
		// deal with index 
		if(Pgm.AlmostEqualsCur("i$ndex")) {
			if(set_index) {
				duplication = TRUE; 
				break;
			}
			PlotOptionIndex();
			set_index = TRUE;
			continue;
		}
		// deal with every 
		if(Pgm.AlmostEqualsCur("ev$ery")) {
			if(_Df.SetEvery) {
				duplication = TRUE; 
				break;
			}
			PlotOptionEvery();
			_Df.SetEvery = TRUE;
			continue;
		}
		// deal with skip 
		if(Pgm.EqualsCur("skip")) {
			if(set_skip || _Df.df_binary_file) {
				duplication = true; 
				break;
			}
			set_skip = true;
			Pgm.Shift();
			_Df.df_skip_at_front = IntExpression();
			if(_Df.df_skip_at_front < 0)
				_Df.df_skip_at_front = 0;
			continue;
		}
		// deal with using 
		if(Pgm.AlmostEqualsCur("u$sing")) {
			if(set_using) {
				duplication = TRUE; 
				break;
			}
			PlotOptionUsing(maxUsing);
			set_using = TRUE;
			continue;
		}
		// deal with volatile 
		if(Pgm.AlmostEqualsCur("volatile")) {
			Pgm.Shift();
			Gg.VolatileData = TRUE;
			continue;
		}
		// Allow this plot not to affect autoscaling 
		if(Pgm.AlmostEqualsCur("noauto$scale")) {
			Pgm.Shift();
			if(pPlot)
				pPlot->noautoscale = TRUE;
			continue;
		}
		break; /* unknown option */
	} // while (!Pgm.EndOfCommand()) 
	if(duplication)
		IntErrorCurToken("duplicated or contradicting arguments in datafile options");
	// Check for auto-generation of key title from column header  
	if(Gg.KeyT.auto_titles == COLUMNHEAD_KEYTITLES) {
		if(_Df.df_no_use_specs == 1)
			_Df.ColumnForKeyTitle = _Df.UseSpec[0].column;
		else if(pPlot && pPlot->plot_style == HISTOGRAMS)
			_Df.ColumnForKeyTitle = _Df.UseSpec[0].column;
		else if(pPlot && pPlot->plot_type == DATA3D)
			_Df.ColumnForKeyTitle = _Df.UseSpec[2].column;
		else
			_Df.ColumnForKeyTitle = _Df.UseSpec[1].column;
	}
	// {{{  more variable inits 
	_Df.PointCount = -1; // we preincrement 
	_Df.LineCount = 0;
	_Df.df_pseudodata = 0;
	_Df.df_pseudorecord = 0;
	_Df.df_pseudospan = 0;
	_Df.df_datablock = FALSE;
	_Df.df_datablock_line = NULL;
	_Df.df_tabulate_strings = FALSE;
	if(pPlot) {
		// Save the matrix/array/image dimensions for binary image plot styles	
		pPlot->image_properties.ncols = _Df.df_xpixels;
		pPlot->image_properties.nrows = _Df.df_ypixels;
		FPRINTF((stderr, "datafile.c:%d (ncols,nrows) set to (%d,%d)\n", __LINE__, _Df.df_xpixels, _Df.df_ypixels));
		if(_Df.SetEvery && _Df.df_xpixels && _Df.df_ypixels) {
			pPlot->image_properties.ncols = 1 + ((int)(MIN(_Df.LastPoint, static_cast<int>(_Df.df_xpixels-1))) - _Df.FirstPoint) / _Df.EveryPoint;
			pPlot->image_properties.nrows = 1 + ((int)(MIN(_Df.lastline, static_cast<int>(_Df.df_ypixels-1))) - _Df.firstline) / _Df.everyline;
			FPRINTF((stderr, "datafile.c:%d  adjusting to (%d, %d)\n", __LINE__, pPlot->image_properties.ncols, pPlot->image_properties.nrows));
		}
		if(_Df.df_transpose) {
			uint temp = pPlot->image_properties.ncols;
			pPlot->image_properties.ncols = pPlot->image_properties.nrows;
			pPlot->image_properties.nrows = temp;
			FPRINTF((stderr, "datafile.c:%d  adjusting to (%d, %d)\n", __LINE__, pPlot->image_properties.ncols, pPlot->image_properties.nrows));
		}
	}
	// }}} 
	// {{{  open file 
#if defined(HAVE_FDOPEN)
	if(*_Df.df_filename == '<' && strlen(_Df.df_filename) > 1 && _Df.df_filename[1] == '&') {
		char * substr;
		// read from an already open file descriptor 
		_Df.data_fd = strtol(_Df.df_filename + 2, &substr, 10);
		if(*substr != '\0' || _Df.data_fd < 0 || substr == _Df.df_filename+2)
			IntError(name_token, "invalid file descriptor integer");
		else if(_Df.data_fd == _fileno(stdin) || _Df.data_fd == _fileno(stdout) || _Df.data_fd == _fileno(stderr))
			IntError(name_token, "cannot plot from stdin/stdout/stderr");
		else if((_Df.data_fp = _fdopen(_Df.data_fd, "r")) == (FILE*)NULL)
			IntError(name_token, "cannot open file descriptor for reading data");
		// if this stream isn't seekable, set it to volatile 
		if(fseek(_Df.data_fp, 0, SEEK_CUR) < 0)
			Gg.VolatileData = true;
	}
	else
#endif /* HAVE_FDOPEN */
#if defined(PIPES)
	if(*df_filename == '<') {
		RestrictPOpen();
		if((data_fp = popen(df_filename + 1, "r")) == (FILE*)NULL)
			OsError(name_token, "cannot create pipe for data");
		else
			_Df.df_pipe_open = TRUE;
	}
	else
#endif /* PIPES */
	// Special filenames '-' '+' '++' '$DATABLOCK' 
	if(*_Df.df_filename == '-' && strlen(_Df.df_filename) == 1) {
		_Df.plotted_data_from_stdin = true;
		Gg.VolatileData = true;
		_Df.data_fp = LfTop();
		SETIFZ(_Df.data_fp, stdin);
		_Df.mixed_data_fp = true; // don't close command file 
	}
	else if(sstreq(_Df.df_filename, "+")) {
		_Df.df_pseudodata = 1;
	}
	else if(sstreq(_Df.df_filename, "++")) {
		_Df.df_pseudodata = 2;
	}
	else if(_Df.df_filename[0] == '$') {
		_Df.df_datablock = true;
		_Df.df_datablock_line = GetDatablock(_Df.df_filename);
		// Better safe than sorry. Check for inblock != outblock 
		if(pPlot && Tab.P_Var && Tab.P_Var->udv_value.v.data_array == _Df.df_datablock_line)
			IntError(NO_CARET, "input and output datablock are the same");
	}
	else if(sstreq(_Df.df_filename, "@@") && _Pb.df_array) {
		// df_array was set in string_or_express() 
		_Df.df_array_index = 0;
		// save name so we can refer to it later 
		_Df.df_arrayname = _Pb.df_array->udv_name;
	}
	else {
		// filename cannot be static array! 
		GpExpandTilde(&_Df.df_filename);
		// Open failure generates a warning rather than an immediate fatal error.
		// We assume success (GPVAL_ERRNO == 0) and let the caller change this to
		// something else if it considers DF_EOF a serious error.
		Ev.FillGpValInteger("GPVAL_ERRNO", 0);
#ifdef HAVE_SYS_STAT_H
		{
			struct stat statbuf;
			if((stat(_Df.df_filename, &statbuf) > -1) && S_ISDIR(statbuf.st_mode)) {
				char * errmsg = (char *)SAlloc::M(32 + strlen(_Df.df_filename));
				sprintf(errmsg, "\"%s\" is a directory", _Df.df_filename);
				Ev.FillGpValString("GPVAL_ERRMSG", errmsg);
				SAlloc::F(errmsg);
				IntWarn(name_token, "\"%s\" is a directory", _Df.df_filename);
				_Df.df_eof = 1;
				return DF_EOF;
			}
		}
#endif
		if((_Df.data_fp = LoadPath_fopen(_Df.df_filename, _Df.df_binary_file ? "rb" : "r")) == NULL) {
			char * errmsg = (char *)SAlloc::M(32 + strlen(_Df.df_filename));
			sprintf(errmsg, "Cannot find or open file \"%s\"", _Df.df_filename);
			Ev.FillGpValString("GPVAL_ERRMSG", errmsg);
			SAlloc::F(errmsg);
			if(pPlot) // suppress message if this is a stats command 
				IntWarn(NO_CARET, "Cannot find or open file \"%s\"", _Df.df_filename);
			_Df.df_eof = 1;
			return DF_EOF;
		}
	}
	// }}} 
	// Binary file options are handled differently depending on the plot style. 
	// Peek ahead in the command line to see if there is a "with <style>" later.
	if(_Df.df_binary_file || _Df.df_matrix_file)
		InitializePlotStyle(pPlot);
	// If the data is in binary matrix form, read in some values
	// to determine the number of columns and rows.  If data is in
	// ASCII matrix form, read in all the data to memory in preparation
	// for using df_readbinary() routine.
	if(_Df.df_matrix_file) {
		DfDetermineMatrix_info(_Df.data_fp);
		// NB: If we're inside a 'stats' command there is no plot 
		if(pPlot) {
			// Image size bookkeeping for ascii uniform matrices 
			if(!_Df.df_binary_file) {
				pPlot->image_properties.ncols = _Df.df_xpixels;
				pPlot->image_properties.nrows = _Df.df_ypixels;
			}
		}
	}
	// General binary, matrix binary and ASCII matrix all use the
	// df_readbinary() routine.
	if(_Df.df_binary_file || _Df.df_matrix_file) {
		_Df.df_read_binary = true;
		AdjustBinaryUseSpec(pPlot);
	}
	else
		_Df.df_read_binary = FALSE;
	// Make information about whether the data forms a grid or not
	// available to the outside world.  */
	_Df.df_matrix = (_Df.df_matrix_file || ((_Df.NumBinRecords == 1) && ((_Df.df_bin_record[0].cart_dim[1] > 0) || (_Df.df_bin_record[0].scan_dim[1] > 0))));
	return _Df.df_no_use_specs;
}
/*}}} */

// {{{  void df_close() 
//void df_close()
void GnuPlot::DfClose()
{
	int i;
	// paranoid - mark $n and column(n) as invalid 
	_Df.df_no_cols = 0;
	if(_Df.data_fp || _Df.df_datablock) {
		// free any use expression storage 
		for(i = 0; i < MAXDATACOLS; ++i)
			if(_Df.UseSpec[i].at) {
				free_at(_Df.UseSpec[i].at);
				_Df.UseSpec[i].at = NULL;
			}
		// free binary matrix data 
		if(_Df.df_matrix) {
			for(i = 0; i < _Df.NumBinRecords; i++) {
				ZFREE(_Df.df_bin_record[i].memory_data);
			}
		}
		if(!_Df.mixed_data_fp && !_Df.df_datablock) {
	#if defined(HAVE_FDOPEN)
			if(_Df.data_fd == _fileno(_Df.data_fp)) {
				// This will allow replotting if this stream is backed by a file,
				// and hopefully is harmless if it connects to a pipe.
				// Leave it open in either case.
				rewind(_Df.data_fp);
				fprintf(stderr, "Rewinding fd %d\n", _Df.data_fd);
			}
			else
	#endif
	#if defined(PIPES)
			if(_Df.df_pipe_open) {
				pclose(_Df.data_fp);
				_Df.df_pipe_open = FALSE;
			}
			else
	#endif
			fclose(_Df.data_fp);
		}
		_Df.mixed_data_fp = FALSE;
		_Df.data_fp = NULL;
	}
}

/*}}} */

/*{{{  void df_showdata() */
/* display the current data file line for an error message
 */
//void df_showdata()
void GnuPlot::DfShowData()
{
	if(_Df.data_fp && _Df.df_filename && _Df.df_line) {
		// display no more than 77 characters 
		fprintf(stderr, "%.77s%s\n%s:%d:", _Df.df_line, (strlen(_Df.df_line) > 77) ? "..." : "", _Df.df_filename, _Df.df_line_number);
	}
}

/*}}} */

//static void plot_option_every()
void GnuPlot::PlotOptionEvery()
{
	_Df.fast_columns = 0; /* corey@cac */
	// allow empty fields - every a:b:c::e we have already established the defaults 
	Pgm.Shift();
	if(!Pgm.EqualsCur(":")) {
		_Df.EveryPoint = IntExpression();
		if(_Df.EveryPoint < 0) 
			_Df.EveryPoint = 1;
		else if(_Df.EveryPoint < 1)
			IntErrorCurToken("Expected positive integer");
	}
	// if it fails on first test, no more tests will succeed. If it
	// fails on second test, next test will succeed with correct c_token 
	if(Pgm.EqualsCur(":") && !Pgm.Equals(++Pgm.CToken, ":")) {
		_Df.everyline = IntExpression();
		if(_Df.everyline < 0) 
			_Df.everyline = 1;
		else if(_Df.everyline < 1)
			IntErrorCurToken("Expected positive integer");
	}
	if(Pgm.EqualsCur(":") && !Pgm.Equals(++Pgm.CToken, ":")) {
		_Df.FirstPoint = IntExpression();
		if(_Df.FirstPoint < 0) 
			_Df.FirstPoint = 0;
	}
	if(Pgm.EqualsCur(":") && !Pgm.Equals(++Pgm.CToken, ":")) {
		_Df.firstline = IntExpression();
		if(_Df.firstline < 0) 
			_Df.firstline = 0;
	}
	if(Pgm.EqualsCur(":") && !Pgm.Equals(++Pgm.CToken, ":")) {
		_Df.LastPoint = IntExpression();
		if(_Df.LastPoint < 0) 
			_Df.LastPoint = MAXINT;
		else if(_Df.LastPoint < _Df.FirstPoint)
			IntErrorCurToken("Last point must not be before first point");
	}
	if(Pgm.EqualsCur(":")) {
		Pgm.Shift();
		_Df.lastline = IntExpression();
		if(_Df.lastline < 0) 
			_Df.lastline = MAXINT;
		else if(_Df.lastline < _Df.firstline)
			IntErrorCurToken("Last line must not be before first line");
	}
}

//static void plot_option_index()
void GnuPlot::PlotOptionIndex()
{
	if(_Df.df_binary_file && _Df.df_matrix_file)
		IntErrorCurToken("Binary matrix file format does not allow more than one surface per file");
	Pgm.Shift();
	// Check for named index 
	if((_Df.P_IndexName = TryToGetString())) {
		_Df.IndexFound = FALSE;
	}
	else {
		// Numerical index list 
		_Df.df_lower_index = IntExpression();
		if(_Df.df_lower_index < 0)
			IntErrorCurToken("index must be non-negative");
		if(Pgm.EqualsCur(":")) {
			Pgm.Shift();
			if(Pgm.EqualsCur(":")) {
				_Df.df_upper_index = MAXINT; /* If end index not specified */
			}
			else {
				_Df.df_upper_index = IntExpression();
				if(_Df.df_upper_index < _Df.df_lower_index)
					IntErrorCurToken("Upper index should be bigger than lower index");
			}
			if(Pgm.EqualsCur(":")) {
				Pgm.Shift();
				_Df.df_index_step = IntExpression();
				if(_Df.df_index_step < 1)
					IntErrorCurToken("Index step must be positive");
			}
		}
		else
			_Df.df_upper_index = _Df.df_lower_index;
	}
}
// 
// formerly in misc.c, but only used here 
// check user defined format strings for valid double conversions 
// HBB 20040601: Added check that the number of format specifiers is
// workable (between 0 and 7) 
// 
static bool valid_format(const char * format)
{
	int formats_found = 0;
	if(!format)
		return false;
	for(;;) {
		if(!(format = strchr(format, '%'))) /* look for format spec  */
			return (formats_found > 0 && formats_found <= 7);
		// Found a % to check --- scan past option specifiers: 
		do {
			format++;
		} while(*format && strchr("+-#0123456789.", *format));
		// Now at format modifier 
		switch(*format) {
			case '*': /* Ignore '*' statements */
			case '%': /* Char   '%' itself     */
			    format++;
			    continue;
			case 'l': /* Now we found it !!! */
			    if(!strchr("fFeEgG", format[1])) /* looking for a valid format */
				    return false;
			    formats_found++;
			    format++;
			    break;
			default:
			    return false;
		}
	}
}

//static void plot_option_using(int max_using)
void GnuPlot::PlotOptionUsing(int max_using)
{
	int no_cols = 0; /* For general binary only. */
	char * column_label;
	// The filetype function may have set the using specs, so reset them before processing tokens.
	if(_Df.df_binary_file)
		InitializeUseSpec();
	// Try to distinguish between 'using "A":"B"' and 'using "%lf %lf" 
	if(!Pgm.EndOfCommand() && Pgm.IsString(++Pgm.CToken)) {
		int save_token = Pgm.GetCurTokenIdx();
		_Df.df_format = TryToGetString();
		if(valid_format(_Df.df_format))
			return;
		ZFREE(_Df.df_format);
		Pgm.SetTokenIdx(save_token);
	}
	if(!Pgm.EndOfCommand()) {
		do {            /* must be at least one */
			if(_Df.df_no_use_specs >= MAXDATACOLS)
				IntErrorCurToken("at most %d columns allowed in using spec", MAXDATACOLS);
			if(_Df.df_no_use_specs >= max_using)
				IntErrorCurToken("Too many columns in using specification");
			if(Pgm.EqualsCur(":")) {
				// empty specification - use default 
				_Df.UseSpec[_Df.df_no_use_specs].column = _Df.df_no_use_specs;
				if(_Df.df_no_use_specs > no_cols)
					no_cols = _Df.df_no_use_specs;
				++_Df.df_no_use_specs;
				// do not increment c+token ; let while() find the : 
			}
			else if(Pgm.EqualsCur("(")) {
				int i;
				use_spec_s * spec = &_Df.UseSpec[_Df.df_no_use_specs];
				_Df.fast_columns = 0; /* corey@cac */
				Pgm.dummy_func = NULL; /* no dummy variables active */
				_Pb.at_highest_column_used = NO_COLUMN_HEADER;
				spec->at = PermAt();
				if(no_cols < _Pb.at_highest_column_used)
					no_cols = _Pb.at_highest_column_used;
				// Try to detect dependence on a particular column so that
				// if it contains a "missing value" placeholder we can skip
				// evaluation altogether.
				for(i = 0; i < spec->at->a_count; i++) {
					if(spec->at->actions[i].index == DOLLARS)
						spec->depends_on_column = (int)spec->at->actions[i].arg.v_arg.v.int_val;
				}
				// Catch at least the simplest case of 'autotitle columnhead' using an expression 
				spec->column = _Pb.at_highest_column_used;
				_Df.df_no_use_specs++;
				// It would be nice to handle these like any other      
				// internal function via perm_at() but it doesn't work. 
			}
			else if(Pgm.AlmostEqualsCur("xtic$labels")) {
				PlotTicLabelUsing(CT_XTICLABEL);
			}
			else if(Pgm.AlmostEqualsCur("x2tic$labels")) {
				PlotTicLabelUsing(CT_X2TICLABEL);
			}
			else if(Pgm.AlmostEqualsCur("ytic$labels")) {
				PlotTicLabelUsing(CT_YTICLABEL);
			}
			else if(Pgm.AlmostEqualsCur("y2tic$labels")) {
				PlotTicLabelUsing(CT_Y2TICLABEL);
			}
			else if(Pgm.AlmostEqualsCur("ztic$labels")) {
				PlotTicLabelUsing(CT_ZTICLABEL);
			}
			else if(Pgm.AlmostEqualsCur("cbtic$labels")) {
				PlotTicLabelUsing(CT_CBTICLABEL);
			}
			else if(Pgm.AlmostEqualsCur("key")) {
				PlotTicLabelUsing(CT_KEYLABEL);
			}
			else if((column_label = TryToGetString())) {
				// ...using "A"... Dummy up a call to column(column_label) 
				_Df.UseSpec[_Df.df_no_use_specs].at = create_call_column_at(column_label);
				_Df.UseSpec[_Df.df_no_use_specs++].column = NO_COLUMN_HEADER;
				_Pb.parse_1st_row_as_headers = TRUE;
				_Df.fast_columns = 0;
				// FIXME - is it safe to always take the title from the 2nd use spec? 
				if(_Df.df_no_use_specs == 2) {
					FREEANDASSIGN(_Df.df_key_title, sstrdup(column_label));
				}
			}
			else {
				int col = IntExpression();
				if(col == -3) /* pseudocolumn -3 means "last column" */
					_Df.fast_columns = 0;
				else if(col < -2)
					IntErrorCurToken("Column must be >= -2");
				_Df.UseSpec[_Df.df_no_use_specs++].column = col;
				// Supposedly only happens for binary files, but don't bet on it 
				if(col > no_cols)
					no_cols = col;
			}
		} while(Pgm.EqualsCur(":") && ++Pgm.CToken);
	}
	if(_Df.df_binary_file) {
		// If the highest user column number is greater than number of binary
		// columns, set the uninitialized columns binary info to that of the last
		// specified column or the default.
		DfExtendBinaryColumns(no_cols);
	}
	// Allow a format specifier after the enumeration of columns.
	// Note: This was left out by mistake in versions 4.6.0 + 4.6.1 
	if(!Pgm.EndOfCommand() && Pgm.IsString(Pgm.GetCurTokenIdx())) {
		_Df.df_format = TryToGetString();
		if(!valid_format(_Df.df_format))
			IntErrorCurToken("format must have 1-7 conversions of type double (%%lf)");
	}
}

//static void plot_ticlabel_using(int axis)
void GnuPlot::PlotTicLabelUsing(int axis)
{
	int col = 0;
	Pgm.Shift();
	if(!Pgm.EqualsCur("("))
		IntErrorCurToken("missing '('");
	Pgm.Shift();
	// FIXME: What we really want is a test for a constant expression as  
	// opposed to a dummy expression. This is similar to the problem with 
	// with parsing the first argument of the plot command itself.        
	if(Pgm.IsANumber(Pgm.GetCurTokenIdx()) || TypeUdv(Pgm.GetCurTokenIdx()) == INTGR) {
		col = IntExpression();
		_Df.UseSpec[_Df.df_no_use_specs+_Df.df_no_tic_specs].at = NULL;
	}
	else {
		_Df.UseSpec[_Df.df_no_use_specs+_Df.df_no_tic_specs].at = PermAt();
		_Df.fast_columns = 0; // Force all columns to be evaluated 
		col = 1; // Redundant because of the above 
	}
	if(col < 1)
		IntErrorCurToken("ticlabels must come from a real column");
	if(!Pgm.EqualsCur(")"))
		IntErrorCurToken("missing ')'");
	Pgm.Shift();
	_Df.UseSpec[_Df.df_no_use_specs+_Df.df_no_tic_specs].expected_type = axis;
	_Df.UseSpec[_Df.df_no_use_specs+_Df.df_no_tic_specs].column = col;
	_Df.df_no_tic_specs++;
}

//int df_readline(double v[], int max)
int GnuPlot::DfReadLine(double v[], int maxSize)
{
	if(!_Df.data_fp && !_Df.df_pseudodata && !_Df.df_datablock && !_Pb.df_array)
		return DF_EOF;
	else if(_Df.df_read_binary) // General binary, matrix binary or matrix ascii converted to binary 
		return DfReadBinary(v, maxSize);
	else
		return DfReadAscii(v, maxSize);
}

/* do the hard work... read lines from file,
 * - use blanks to get index number
 * - ignore lines outside range of indices required
 * - fill v[] based on using spec if given
 */

//int df_readascii(double v[], int max)
int GnuPlot::DfReadAscii(double v[], int maxSize)
{
	char * s;
	int return_value = DF_GOOD;
	/* Version 5.3
	 * Some plot styles (e.g. PARALLELPLOT) must guarantee that every line
	 * of data will return some input value even if it is missing or bad.
	 * This flag will force the line to return NaN rather than being skipped.
	 * FIXME: it would be better to make this flag generic and set before entry.
	 */
	bool df_bad_returns_NaN = (_Df.df_current_plot && oneof2(_Df.df_current_plot->plot_style, PARALLELPLOT, TABLESTYLE));
	assert(maxSize <= MAXDATACOLS);
	// catch attempt to read past EOF on mixed-input 
	if(_Df.df_eof)
		return DF_EOF;
#if (1)
	/* DEBUG FIXME
	 * Normally 'plot ARRAY ...' wants each array entry as a separate input "line".
	 * However spiderplots want only a single line, not one line per array entry.
	 * This code forces the single line but loses the content.
	 * Is there a better solution?
	 * Work-around
	 *    plot for [i=1:|ARRAY|] [t=1:1:1] '+' using (ARRAY[i]) with spiderplot
	 */
	if(Gg.SpiderPlot && _Pb.df_array && _Df.df_datum >= 0)
		return DF_EOF;
#endif
	/*{{{  process line */
	while((s = DfGets()) != NULL) {
		bool line_okay = TRUE;
		int output = 0; /* how many numbers written to v[] */
		return_value = DF_GOOD;
		// "skip" option 
		if(_Df.df_skip_at_front > 0) {
			_Df.df_skip_at_front--;
			continue;
		}
		++_Df.df_line_number;
		_Df.df_no_cols = 0;
		/*{{{  check for blank lines, and reject by index/every */
		/*{{{  skip leading spaces */
		while(isspace((uchar)*s) && NOTSEP)
			++s; /* will skip the \n too, to point at \0  */
		/*}}} */

		/*{{{  skip comments */
		if(is_comment(*s)) {
			if(_Df.P_IndexName) { /* Look for index name in comment */
				while(is_comment(*s) || isspace((uchar)*s))
					++s;
				if(*s && !strncmp(s, _Df.P_IndexName, strlen(_Df.P_IndexName)))
					_Df.IndexFound = TRUE;
			}
			continue; /* ignore comments */
		}
		/*}}} */

		/*{{{  check EOF on mixed data */
		if(_Df.mixed_data_fp && is_EOF(*s)) {
			_Df.df_eof = 1; /* trap attempts to read past EOF */
			return DF_EOF;
		}
		/*}}} */

		/*{{{  its a blank line - update counters and continue or return */
		if(*s == 0) {
			/* argh - this is complicated !  we need to
			 *   ignore it if we haven't reached first index
			 *   report EOF if passed last index
			 *   report blank line unless we've already done 2 blank lines
			 *
			 * - I have probably missed some obvious way of doing all this,
			 * but its getting late
			 */
			_Df.PointCount = -1; /* restart counter within line */
			if(++_Df.BlankCount == 1) {
				// first blank line 
				++_Df.LineCount;
			}
			// just reached end of a group/surface 
			if(_Df.BlankCount == 2) {
				++_Df.df_current_index;
				_Df.LineCount = 0;
				_Df.df_datum = -1;
				// Found two blank lines after a block of data with a named index 
				if(_Df.P_IndexName && _Df.IndexFound) {
					_Df.df_eof = 1;
					return DF_EOF;
				}
				/* ignore line if current_index has just become
				 * first required one - client doesn't want this
				 * blank line. While we're here, check for <=
				 * - we need to do it outside this conditional, but
				 * probably no extra cost at assembler level
				 */
				if(_Df.df_current_index <= _Df.df_lower_index)
					continue; /* dont tell client */
				// df_upper_index is MAXINT-1 if we are not doing index 
				if(_Df.df_current_index > _Df.df_upper_index) {
					// oops - need to gobble rest of input if mixed 
					if(_Df.mixed_data_fp)
						continue;
					else {
						_Df.df_eof = 1;
						return DF_EOF; /* no point continuing */
					}
				}
			}
			// dont tell client if we haven't reached first index 
			if(_Df.P_IndexName && !_Df.IndexFound)
				continue;
			if(_Df.df_current_index < _Df.df_lower_index)
				continue;
			// ignore blank lines after blank_index 
			if(_Df.BlankCount > 2)
				continue;
			return DF_FIRST_BLANK - (_Df.BlankCount - 1);
		}
		/*}}} */
		// get here => was not blank 
		_Df.df_last_index_read = _Df.df_current_index;
		_Df.BlankCount = 0;
		/*{{{  ignore points outside range of index */
		/* we try to return end-of-file as soon as we pass upper index,
		 * but for mixed input stream, we must skip garbage
		 */
		if(_Df.P_IndexName && !_Df.IndexFound)
			continue;
		if(_Df.df_current_index < _Df.df_lower_index || _Df.df_current_index > _Df.df_upper_index || ((_Df.df_current_index - _Df.df_lower_index) % _Df.df_index_step) != 0)
			continue;
		/*}}} */
		// Bookkeeping for the plot ... every N:M:etc option 
		if((_Pb.parse_1st_row_as_headers || _Df.ColumnForKeyTitle > 0) && !_Df.df_already_got_headers) {
			FPRINTF((stderr, "skipping 'every' test in order to read column headers\n"));
		}
		else {
			// Accept only lines with (line_count%everyline) == 0 
			if(_Df.LineCount < _Df.firstline || _Df.LineCount > _Df.lastline || (_Df.LineCount - _Df.firstline) % _Df.everyline != 0)
				continue;
			// update point_count. ignore point if point_count%everypoint != 0 
			if(++_Df.PointCount < _Df.FirstPoint || _Df.PointCount > _Df.LastPoint || (_Df.PointCount - _Df.FirstPoint) % _Df.EveryPoint != 0)
				continue;
		}
		/*}}} */
		++_Df.df_datum;
		if(_Df.df_format) {
			/*{{{  do a sscanf */
			int i;
			// check we have room for at least 7 columns 
			if(_Df.df_max_cols < 7)
				_Df.ExpandColumn(7);
			_Df.df_no_cols = sscanf(s, _Df.df_format, &_Df.df_column[0].datum, &_Df.df_column[1].datum, &_Df.df_column[2].datum, &_Df.df_column[3].datum, &_Df.df_column[4].datum, &_Df.df_column[5].datum, &_Df.df_column[6].datum);
			if(_Df.df_no_cols == EOF) {
				_Df.df_eof = 1;
				return DF_EOF; /* tell client */
			}
			for(i = 0; i < _Df.df_no_cols; ++i) { /* may be zero */
				_Df.df_column[i].good = DF_GOOD;
				_Df.df_column[i].position = NULL; /* cant get a time */
			}
			/*}}} */
		}
		else
			DfTokenise(s);
		/* df_tokenise already processed everything, but in the case of pseudodata
		 * '+' or '++' the value itself was passed as an ascii string formatted by
		 * "%g".  We can do better than this by substituting in the binary value.
		 */
		if(_Df.df_pseudodata > 0)
			_Df.df_column[0].datum = _Df.df_pseudovalue_0;
		if(_Df.df_pseudodata > 1)
			_Df.df_column[1].datum = _Df.df_pseudovalue_1;
		// Similar to above, we can go back to the original numerical value of A[i] 
		if(_Pb.df_array && _Pb.df_array->udv_value.v.value_array[_Df.df_array_index].Type == CMPLX) {
			_Df.df_column[1].datum = _Pb.df_array->udv_value.v.value_array[_Df.df_array_index].v.cmplx_val.real;
			_Df.df_column[2].datum = _Pb.df_array->udv_value.v.value_array[_Df.df_array_index].v.cmplx_val.imag;
		}
		/* Always save the contents of the first row in case it is needed for
		 * later access via column("header").  However, unless we know for certain that
		 * it contains headers only, e.g. via parse_1st_row_as_headers or
		 * (_Df.ColumnForKeyTitle > 0), also treat it as a data row.
		 */
		if(_Df.df_datum == 0 && !_Df.df_already_got_headers) {
			int j;
			FPRINTF((stderr, "datafile.c:%d processing %d column headers\n", __LINE__, _Df.df_no_cols));
			for(j = 0; j < _Df.df_no_cols; j++) {
				FREEANDASSIGN(_Df.df_column[j].header, DfParseStringField(_Df.df_column[j].position));
				if(_Df.df_column[j].header) {
					SETMAX(_Df.LongestColumnHead, sstrleni(_Df.df_column[j].header));
					FPRINTF((stderr, "Col %d: \"%s\"\n", j+1, _Df.df_column[j].header));
				}
			}
			_Df.df_already_got_headers = true;
			// Restrict the column number to possible values 
			SETMIN(_Df.ColumnForKeyTitle, _Df.df_no_cols);
			if(_Df.ColumnForKeyTitle == -3) /* last column in file */
				_Df.ColumnForKeyTitle = _Df.df_no_cols;
			if(_Df.ColumnForKeyTitle > 0) {
				_Df.df_key_title = sstrdup(_Df.df_column[_Df.ColumnForKeyTitle-1].header);
				if(!_Df.df_key_title) {
					FPRINTF((stderr, "df_readline: missing column head for key title\n"));
					return(DF_KEY_TITLE_MISSING);
				}
				_Df.df_datum--;
				_Df.ColumnForKeyTitle = NO_COLUMN_HEADER;
				_Pb.parse_1st_row_as_headers = false;
				return DF_FOUND_KEY_TITLE;
			}
			else if(_Pb.parse_1st_row_as_headers) {
				_Df.df_datum--;
				_Pb.parse_1st_row_as_headers = false;
				return DF_COLUMN_HEADERS;
			}
		}
		// Used by stats to set STATS_columns 
		if(_Df.df_datum == 0)
			_Df.df_last_col = _Df.df_no_cols;
		//{{{  copy column[] to v[] via use[] 
		{
			int limit = (_Df.df_no_use_specs ? (_Df.df_no_use_specs + _Df.df_no_tic_specs) : MAXDATACOLS);
			SETMIN(limit, maxSize + _Df.df_no_tic_specs);
			// Used only by TABLESTYLE 
			if(_Df.df_tabulate_strings)
				for(output = 0; output < limit; ++output)
					gpfree_string(&_Df.df_strings[output]);
			// The real processing starts here 
			for(output = 0; output < limit; ++output) {
				// if there was no using spec, column is output+1 and at=NULL 
				int column = _Df.UseSpec[output].column;
				if(column == -3) // pseudocolumn -3 means "last column" 
					column = _Df.UseSpec[output].column = _Df.df_no_cols;
				// Handle cases where column holds a meta-data string 
				// Axis labels, plot titles, etc.                     
				if(_Df.UseSpec[output].expected_type >= CT_XTICLABEL) {
					int axis, axcol;
					double xpos;
					// EAM FIXME - skip columnstacked histograms also 
					if(_Df.df_current_plot) {
						if(_Df.df_current_plot->plot_style == BOXPLOT)
							continue;
					}
					axcol = AxColForTicLabel((COLUMN_TYPE)_Df.UseSpec[output].expected_type, &axis);
					// Trap special case of only a single 'using' column 
					if(output == 1)
						xpos = (axcol == 0) ? _Df.df_datum : v[axcol-1];
					else
						xpos = v[axcol];
					if(_Df.df_current_plot && _Df.df_current_plot->plot_style == HISTOGRAMS) {
						if(output > 1) // Can only happen for HT_ERRORBARS 
							xpos = (axcol == 0) ? _Df.df_datum : v[axcol-1];
						xpos += _Df.df_current_plot->histogram->start;
					}
					// Tic label is generated by a string-valued function 
					if(_Df.UseSpec[output].at) {
						GpValue a;
						_Df.evaluate_inside_using = true;
						EvaluateAt(_Df.UseSpec[output].at, &a);
						_Df.evaluate_inside_using = false;
						if(a.Type == STRING) {
							AddTicUser(&AxS[axis], a.v.string_val, xpos, -1);
							gpfree_string(&a);
						}
					}
					else {
						char * temp_string = DfParseStringField(_Df.df_tokens[output]);
						AddTicUser(&AxS[axis], temp_string, xpos, -1);
						SAlloc::F(temp_string);
					}
				}
				else if(_Df.UseSpec[output].expected_type == CT_KEYLABEL) {
					char * temp_string = DfParseStringField(_Df.df_tokens[output]);
					if(_Df.df_current_plot)
						AddKeyEntry(temp_string, _Df.df_datum);
					SAlloc::F(temp_string);
				}
				else if(_Df.UseSpec[output].at) {
					GpValue a;
					bool timefield = FALSE;
					// Don't try to evaluate an expression that depends on a data field value that is missing.
					if(_Df.UseSpec[output].depends_on_column > 0) {
						if((_Df.UseSpec[output].depends_on_column > _Df.df_no_cols) ||  _Df.df_column[_Df.UseSpec[output].depends_on_column-1].good == DF_MISSING) {
							FPRINTF((stderr, "df_readascii: skipping evaluation that uses missing value in $%d\n", _Df.UseSpec[output].depends_on_column));
							v[output] = fgetnan();
							return_value = DF_MISSING;
							continue;
						}
					}
					a.SetNotDefined();
					_Df.evaluate_inside_using = true;
					EvaluateAt(_Df.UseSpec[output].at, &a);
					_Df.evaluate_inside_using = false;
					// If column N contains the "missing" flag and is referenced by
					// 'using N' or 'using (func($N)) then we caught it already.
					// Here we check for indirect references like 'using "header_of_N"'.
					if((a.Type == CMPLX) && isnan(a.v.cmplx_val.real) && (a.v.cmplx_val.imag == DF_MISSING)) {
						return_value = DF_MISSING;
						v[output] = fgetnan();
						continue;
					}
					// June 2018: CHANGE.  For consistency with function plots,	
					// treat imaginary result as UNDEFINED.			
					if(a.Type == CMPLX && (fabs(Imag(&a)) > Gg.Zero) && !isnan(Real(&a))) {
						return_value = DF_COMPLEX_VALUE;
						v[output] = fgetnan();
						continue;
					}
					if(Ev.IsUndefined_) {
						return_value = DF_UNDEFINED;
						v[output] = fgetnan();
						continue;
					}
					if((_Df.df_axis[output] != NO_AXIS) && AxS[_Df.df_axis[output]].datatype == DT_TIMEDATE)
						timefield = TRUE;
					if(timefield && (a.Type != STRING) && sstreq(AxS.P_TimeFormat, "%s")) {
						// Handle the case of P_TimeFormat "%s" which expects a string 
						// containing a number. If evaluate_at() above returned a 
						// bare number then we must convert it to a sting before  
						// falling through to the usual processing case.          
						// NB: We only accept time values of +/- 10^12 seconds.   
						char * timestring = (char *)SAlloc::M(20);
						sprintf(timestring, "%16.3f", Real(&a));
						a.Type = STRING;
						a.v.string_val = timestring;
					}
					if(a.Type == STRING) {
						v[output] = fgetnan(); /* found a string, not a number */
						if(_Df.df_tabulate_strings) {
							// Save for TABLESTYLE 
							_Df.df_strings[output].Type = STRING;
							_Df.df_strings[output].v.string_val = sstrdup(a.v.string_val);
						}
						// This string value will get parsed as if it were a data column 
						// so put it in quotes to allow embedded whitespace.             
						if(_Df.UseSpec[output].expected_type == CT_STRING) {
							char * s = (char *)SAlloc::M(strlen(a.v.string_val)+3);
							*s = '"';
							strcpy(s+1, a.v.string_val);
							strcat(s, "\"");
							SAlloc::F(_Df.df_stringexpression[output]);
							_Df.df_tokens[output] = _Df.df_stringexpression[output] = s;
						}
						// Check for P_TimeFormat string generated by a function 
						if(timefield) {
							struct tm tm;
							double reltime;
							double usec = 0.0;
							td_type status = GStrPTime(a.v.string_val, AxS.P_TimeFormat, &tm, &usec, &reltime);
							if(status == DT_TIMEDATE)
								v[output] = (double)gtimegm(&tm) + usec;
							else if(status == DT_DMS)
								v[output] = reltime;
							else
								return_value = DF_BAD;
						}
						// Expecting a numerical type but got a string value 
						else
						// 'with points pt variable' is the only current user 
						if(_Df.df_current_plot && (_Df.df_current_plot->lp_properties.PtType == PT_VARIABLE)) {
							static char varchar[8];
							strnzcpy(varchar, a.v.string_val, 8);
							_Df.df_tokens[output] = varchar;
						}
						gpfree_string(&a);
					}
					else {
						v[output] = Real(&a);
						if(isnan(v[output]))
							return_value = DF_UNDEFINED;
					}
				}
				else if(column == -2) {
					v[output] = _Df.df_current_index;
				}
				else if(column == -1) {
					v[output] = _Df.LineCount;
				}
				else if(column == 0) {
					v[output] = _Df.df_datum; // using 0 
				}
				else if(column <= 0) /* really < -2, but */
					IntError(NO_CARET, "internal error: column <= 0 in datafile.c");
				else if((_Df.df_axis[output] != NO_AXIS) && (AxS[_Df.df_axis[output]].datatype == DT_TIMEDATE)) {
					struct tm tm;
					double usec = 0.0;
					double reltime;
					int status;
					if(column > _Df.df_no_cols || _Df.df_column[column-1].good == DF_MISSING || !_Df.df_column[column-1].position ||
					    (status = GStrPTime(_Df.df_column[column-1].position, AxS.P_TimeFormat, &tm, &usec, &reltime), status == DT_BAD)) {
						// line bad only if user explicitly asked for this column 
						if(_Df.df_no_use_specs) {
							line_okay = false;
							if(df_bad_returns_NaN) {
								v[output] = fgetnan();
								return DF_UNDEFINED;
							}
						}
						// return or ignore line depending on line_okay 
						break;
					}
					if(status == DT_DMS)
						v[output] = reltime;
					else
						v[output] = (double)gtimegm(&tm) + usec;
				}
				else if(_Df.UseSpec[output].expected_type == CT_STRING) {
					// Do nothing. /
					// String tokens were loaded into df_tokens already. 
				}
				else {
					// column > 0 
					if((column <= _Df.df_no_cols) && _Df.df_column[column-1].good == DF_GOOD) {
						v[output] = _Df.df_column[column-1].datum;
						// Version 5:
						// Do not return immediately on DF_MISSING or DF_UNDEFINED.
						// THIS IS A CHANGE.
					}
					else if((column <= _Df.df_no_cols) && _Df.UseSpec[output].expected_type == CT_MUST_HAVE) {
						// This catches cases where the plot style cannot tolerate
						// silently missed points (e.g. stacked histograms)
						v[output] = fgetnan();
						return DF_UNDEFINED;
					}
					else if((column <= _Df.df_no_cols) && (_Df.df_column[column-1].good == DF_MISSING)) {
						v[output] = fgetnan();
						if(_Df.missing_val && _Df.df_current_plot->plot_style == TABLESTYLE) {
							_Df.df_strings[output].Type = STRING;
							_Df.df_strings[output].v.string_val = sstrdup(_Df.missing_val);
						}
						return_value = DF_MISSING;
					}
					else if((column <= _Df.df_no_cols) && (_Df.df_column[column-1].good == DF_UNDEFINED)) {
						v[output] = _Df.df_column[column-1].datum;
						return_value = DF_UNDEFINED;
					}
					else if((_Df.df_current_plot && _Df.df_current_plot->plot_style == POLYGONS && _Df.df_no_use_specs && column == _Df.df_no_cols+1)) {
						/* DEBUG - the idea here is to forgive a missing color value in
						 *   polygon vertices after the first one. The test is not
						 *   quite correct since we don't track the vertex number.
						 */
						v[output] = fgetnan();
					}
					else {
						// line bad only if user explicitly asked for this column 
						if(_Df.df_no_use_specs) {
							if(df_bad_returns_NaN) {
								v[output] = fgetnan();
								return_value = DF_UNDEFINED;
							}
							else {
								line_okay = FALSE;
								break; /* return or ignore depending on line_okay */
							}
						}
						else
							break; /* return or ignore depending on line_okay */
					}
				}
				// Special case to make 'using 0' et al. to work with labels 
				if(_Df.UseSpec[output].expected_type == CT_STRING && (!(_Df.UseSpec[output].at) || !_Df.df_tokens[output]) && oneof3(column, -2, -1, 0)) {
					char * s = (char *)SAlloc::M(32*sizeof(char));
					sprintf(s, "%d", (int)v[output]);
					SAlloc::F(_Df.df_stringexpression[output]);
					_Df.df_tokens[output] = _Df.df_stringexpression[output] = s;
				}
			}
		}
		/*}}} */

		if(!line_okay) /* Ignore this line (pretend we never read it) */
			continue;

		/* output == df_no_use_specs if using was specified -
		 * actually, smaller of df_no_use_specs and max */
		/* FIXME EAM - In theory it might be useful for the caller to
		 * know whether or not tic specs were read from this line, but
		 * all callers would have to be modified to deal with it one
		 * way or the other. */
		output -= _Df.df_no_tic_specs;
		/*
		 * EAM Apr 2012 - If there is no using spec, then whatever we found on
		 * the first line becomes the expectation for the rest of the input file.
		 * THIS IS A CHANGE!
		 */
		SETIFZ(_Df.df_no_use_specs, output);
		/* Version 5:
		 * If all requested values were OK, return number of columns read.
		 * If a requested column was bad, return an error but nevertheless
		 * return the other requested columns. The number of columns is
		 * available to the caller in df_no_use_specs.
		 * THIS IS A CHANGE!
		 */
		switch(return_value) {
			case DF_MISSING:
			case DF_UNDEFINED:
			case DF_COMPLEX_VALUE:
			case DF_BAD:
			    return return_value;
			    break;
			default:
			    return output;
			    break;
		}
	}
	/*}}} */
	// get here => fgets failed */
	// no longer needed - mark column(x) as invalid 
	_Df.df_no_cols = 0;
	_Df.df_eof = 1;
	return DF_EOF;
}

/*}}} */

const char * read_error_msg = "Data file read error";
double df_matrix_corner[2][2]; /* First argument is corner, second argument is x (0) or y(1). */

void df_swap_bytes_by_endianess(char * data, int read_order, int read_size)
{
	if((read_order == DF_3210)
#if SUPPORT_MIDDLE_ENDIAN
	   || (read_order == DF_2301)
#endif
	    ) {
		int k = read_size - 1;
		for(int j = 0; j < k; j++, k--) {
			char temp = data[j];
			data[j] = data[k];
			data[k] = temp;
		}
	}
#if SUPPORT_MIDDLE_ENDIAN
	if(oneof2(read_order, DF_1032, DF_2301)) {
		for(int j = read_size - 1; j > 0; j -= 2) {
			char temp = data[j-1];
			data[j-1] = data[j];
			data[j] = temp;
		}
	}
#endif
}

//static double df_read_a_float(FILE * fin) 
double GnuPlot::DfReadAFloat(FILE * fin)
{
	float fdummy;
	// FIXME:  What if the file contains doubles rather than floats? 
	if(fread(&fdummy, sizeof(fdummy), 1, fin) != 1) {
		if(feof(fin))
			IntError(NO_CARET, "Data file is empty");
		else
			IntError(NO_CARET, read_error_msg);
	}
	df_swap_bytes_by_endianess((char *)&fdummy, byte_read_order(_Df.BinFileEndianess), sizeof(fdummy));
	return (double)fdummy;
}

//void df_determine_matrix_info(FILE * fin)
void GnuPlot::DfDetermineMatrix_info(FILE * fin)
{
	if(_Df.df_binary_file) {
		// Binary matrix format. 
		off_t nr; // off_t because they contribute to fseek offset 
		off_t flength;
		int ierr;
		// Read first value for number of columns. 
		float fdummy = static_cast<float>(DfReadAFloat(fin));
		off_t nc = static_cast<off_t>(fdummy); // off_t because they contribute to fseek offset 
		if(nc == 0)
			IntError(NO_CARET, "Read grid of zero width");
		else if(nc > 1e8)
			IntError(NO_CARET, "Read grid width too large");
		// Read second value for corner_0 x. 
		fdummy = static_cast<float>(DfReadAFloat(fin));
		df_matrix_corner[0][0] = fdummy;
		// Read nc+1 value for corner_1 x. 
		if(nc > 1) {
			ierr = fseek(fin, (nc-2)*sizeof(float), SEEK_CUR);
			if(ierr < 0)
				IntError(NO_CARET, "seek error in binary input stream - %s", strerror(errno));
			fdummy = static_cast<float>(DfReadAFloat(fin));
		}
		df_matrix_corner[1][0] = fdummy;
		// Read nc+2 value for corner_0 y. 
		df_matrix_corner[0][1] = DfReadAFloat(fin);
		// Compute length of file and number of columns. 
		ierr = fseek(fin, 0L, SEEK_END);
		if(ierr < 0)
			IntError(NO_CARET, "seek error in binary input stream - %s", strerror(errno));
		flength = ftell(fin)/sizeof(float);
		nr = flength/(nc + 1);
		if(nr*(nc + 1) != flength)
			IntError(NO_CARET, "File doesn't factorize into full matrix");
		// Read last value for corner_1 y 
		ierr = fseek(fin, -(nc + 1)*sizeof(float), SEEK_END);
		if(ierr < 0)
			IntError(NO_CARET, "seek error in binary input stream - %s", strerror(errno));
		df_matrix_corner[1][1] = DfReadAFloat(fin);
		// Set up scan information for df_readbinary(). 
		_Df.df_bin_record[0].scan_dim[0] = static_cast<int>(nc);
		_Df.df_bin_record[0].scan_dim[1] = static_cast<int>(nr);
		// Save submatrix size for stats 
		if(_Df.SetEvery) {
			_Df.df_bin_record[0].submatrix_ncols = 1 + ((MIN(_Df.LastPoint, static_cast<int>(nc-1))) - _Df.FirstPoint) / _Df.EveryPoint;
			_Df.df_bin_record[0].submatrix_nrows = 1 + ((MIN(_Df.lastline,  static_cast<int>(nr-1))) - _Df.firstline) / _Df.everyline;
		}
		else {
			_Df.df_bin_record[0].submatrix_ncols = static_cast<int>(nc);
			_Df.df_bin_record[0].submatrix_nrows = static_cast<int>(nr);
		}
		// Reset counter file pointer. 
		ierr = fseek(fin, 0L, SEEK_SET);
		if(ierr < 0)
			IntError(NO_CARET, "seek error in binary input stream - %s", strerror(errno));
	}
	else {
		// ASCII matrix format, converted to binary memory format. 
		static double * matrix = NULL;
		int nr, nc;
		// Insurance against creating a matrix with df_read_matrix()
		// and then erroring out through DfAddBinaryRecords().
		ZFREE(matrix);
		// Set important binary variables, then free memory for all default
		// binary records and set number of records to 0. */
		InitializeBinaryVars();
		ClearBinaryRecords(DF_CURRENT_RECORDS);
		// The default binary record type is currently float (Dec 2019)
		// but we have changed df_read_matrix to return doubles.
		// Make sure the binary record type is marked accordingly.
		DfExtendBinaryColumns(1);
		DfSetReadType(1, DF_DOUBLE);
		// If the user has set an explicit locale for numeric input, apply it 
		// here so that it affects data fields read from the input file.      
		set_numeric_locale();
		// "skip" option to skip lines at start of ascii file 
		while(_Df.df_skip_at_front > 0) {
			DfGets();
			_Df.df_skip_at_front--;
		}
		// Keep reading matrices until file is empty. 
		while(!_Df.df_eof) {
			if((matrix = DfReadMatrix(&nr, &nc)) != NULL) {
				int index = _Df.NumBinRecords;
				// Ascii matrix with explicit y in first row, x in first column 
				if(_Df.df_nonuniform_matrix) {
					nc--;
					nr--;
				}
				// First column contains row labels, not data 
				if(_Df.df_matrix_rowheaders)
					nc--;
				DfAddBinaryRecords(1, DF_CURRENT_RECORDS);
				_Df.df_bin_record[index].memory_data = (char *)matrix;
				matrix = NULL;
				_Df.df_bin_record[index].scan_dim[0] = nc;
				_Df.df_bin_record[index].scan_dim[1] = nr;
				_Df.df_bin_record[index].scan_dim[2] = 0;
				_Df.BinFileEndianess = THIS_COMPILER_ENDIAN;
				// Save matrix dimensions in case it contains an image 
				_Df.df_xpixels = nc;
				_Df.df_ypixels = nr;
				if(_Df.SetEvery) {
					_Df.df_xpixels = 1 + ((int)(MIN(_Df.LastPoint, static_cast<int>(_Df.df_xpixels-1))) - _Df.FirstPoint) / _Df.EveryPoint;
					_Df.df_ypixels = 1 + ((int)(MIN(_Df.lastline,  static_cast<int>(_Df.df_ypixels-1))) - _Df.firstline) / _Df.everyline;
					FPRINTF((stderr, "datafile.c:%d filtering (%d,%d) to (%d,%d)\n", __LINE__, nc, nr, _Df.df_xpixels, _Df.df_ypixels));
				}
				// Save submatrix dimensions for stats 
				_Df.df_bin_record[index].submatrix_ncols = _Df.df_xpixels;
				_Df.df_bin_record[index].submatrix_nrows = _Df.df_ypixels;
				// This matrix is the one (and only) requested by name.	
				// Dummy up index range and skip rest of file.		
				if(_Df.P_IndexName) {
					if(_Df.IndexFound) {
						_Df.df_lower_index = _Df.df_upper_index = index;
						break;
					}
					else
						_Df.df_lower_index = index+1;
				}
			}
			else
				break;
		}
		// We are finished reading user input; return to C locale for internal use 
		reset_numeric_locale();
		// Data from file is now in memory.  Make the rest of gnuplot think
		// that the data stream has not yet reached the end of file.
		_Df.df_eof = 0;
	}
}
//
// stuff for implementing the call-backs for picking up data values
// do it here so we can make the variables private to this file
//
//void f_dollars(union argument * x)
void GnuPlot::F_Dollars(union argument * x)
{
	Push(&x->v_arg);
	F_Column(x);
}

/*}}} */

// {{{  void f_column() 
//void f_column(union argument * /*arg*/)
void GnuPlot::F_Column(union argument * arg)
{
	GpValue a;
	int column;
	Pop(&a);
	if(!_Df.evaluate_inside_using)
		IntError(Pgm.GetPrevTokenIdx(), "column() called from invalid context");
	if(a.Type == STRING) {
		int j;
		char * name = a.v.string_val;
		column = DF_COLUMN_HEADERS;
		for(j = 0; j < _Df.df_no_cols; j++) {
			if(_Df.df_column[j].header) {
				int offset = (*_Df.df_column[j].header == '"') ? 1 : 0;
				if(streq(name, _Df.df_column[j].header + offset)) {
					column = j+1;
					SETIFZ(_Df.df_key_title, sstrdup(_Df.df_column[j].header));
					break;
				}
			}
		}
		// This warning should only trigger once per problematic input file 
		if(column == DF_COLUMN_HEADERS && (*name) && _Df.df_warn_on_missing_columnheader) {
			_Df.df_warn_on_missing_columnheader = false;
			IntWarn(NO_CARET, "no column with header \"%s\"", a.v.string_val);
			for(j = 0; j < _Df.df_no_cols; j++) {
				if(_Df.df_column[j].header) {
					int offset = (*_Df.df_column[j].header == '"') ? 1 : 0;
					if(!strncmp(name, _Df.df_column[j].header + offset, strlen(name)))
						IntWarn(NO_CARET, "partial match against column %d header \"%s\"", j+1, _Df.df_column[j].header);
				}
			}
		}
		gpfree_string(&a);
	}
	else
		column = (int)Real(&a);
	if(column == -2)
		Push(Ginteger(&a, _Df.df_last_index_read));
	else if(column == -1)
		Push(Ginteger(&a, _Df.LineCount));
	else if(column == 0) // $0 = df_datum 
		Push(Gcomplex(&a, (double)_Df.df_datum, 0.0));
	else if(column == -3) // pseudocolumn -3 means "last column" 
		Push(Gcomplex(&a, _Df.df_column[_Df.df_no_cols-1].datum, 0.0));
	else if(column < 1 || column > _Df.df_no_cols) {
		Ev.IsUndefined_ = true;
		// Nov 2014: This is needed in case the value is referenced 
		// in an expression inside a 'using' clause.		    
		Push(Gcomplex(&a, fgetnan(), 0.0));
	}
	else if(_Df.df_column[column-1].good == DF_MISSING) {
		// Doesn't set undefined to TRUE although perhaps it should 
		Push(Gcomplex(&a, fgetnan(), (double)DF_MISSING));
	}
	else if(_Df.df_column[column-1].good != DF_GOOD) {
		Ev.IsUndefined_ = true;
		Push(Gcomplex(&a, fgetnan(), 0.0));
	}
	else
		Push(Gcomplex(&a, _Df.df_column[column-1].datum, 0.0));
}
//
// Called from GnuPlot::IntError() 
//
//void df_reset_after_error()
void GnuPlot::DfResetAfterError()
{
	_Df.evaluate_inside_using = false;
}

//void f_stringcolumn(union argument * /*arg*/)
void GnuPlot::F_StringColumn(union argument * /*arg*/)
{
	GpValue a;
	int column;
	Pop(&a);
	if(!_Df.evaluate_inside_using || _Df.df_matrix)
		IntError(Pgm.GetPrevTokenIdx(), "stringcolumn() called from invalid context");
	if(a.Type == STRING) {
		int j;
		char * name = a.v.string_val;
		column = DF_COLUMN_HEADERS;
		for(j = 0; j < _Df.df_no_cols; j++) {
			if(_Df.df_column[j].header) {
				int offset = (*_Df.df_column[j].header == '"') ? 1 : 0;
				if(streq(name, _Df.df_column[j].header + offset)) {
					column = j+1;
					SETIFZ(_Df.df_key_title, sstrdup(_Df.df_column[j].header));
					break;
				}
			}
		}
		// This warning should only trigger once per problematic input file 
		if(column == DF_COLUMN_HEADERS && (*name) && _Df.df_warn_on_missing_columnheader) {
			_Df.df_warn_on_missing_columnheader = false;
			IntWarn(NO_CARET, "no column with header \"%s\"", a.v.string_val);
			for(j = 0; j < _Df.df_no_cols; j++) {
				if(_Df.df_column[j].header) {
					int offset = (*_Df.df_column[j].header == '"') ? 1 : 0;
					if(!strncmp(name, _Df.df_column[j].header + offset, strlen(name)))
						IntWarn(NO_CARET, "partial match against column %d header \"%s\"", j+1, _Df.df_column[j].header);
				}
			}
		}
		gpfree_string(&a);
	}
	else
		column = (int)Real(&a);
	if(column == -3) // pseudocolumn -3 means "last column" 
		column = _Df.df_no_cols;
	if(column == -2) { // pseudocolumn -2 means "index" 
		Push(Gstring(&a, _Df.P_IndexName));
	}
	else if(column == -1) {
		char temp_string[32];
		sprintf(temp_string, "%d", _Df.LineCount);
		Push(Gstring(&a, temp_string));
	}
	else if(column == 0) { /* $0 = df_datum */
		char temp_string[32];
		sprintf(temp_string, "%d", _Df.df_datum);
		Push(Gstring(&a, temp_string));
	}
	else if(column < 1 || column > _Df.df_no_cols) {
		Ev.IsUndefined_ = true;
		Push(&a); // any objection to this ? 
	}
	else {
		char * temp_string = DfParseStringField(_Df.df_column[column-1].position);
		Push(Gstring(&a, temp_string));
		SAlloc::F(temp_string);
	}
}

// {{{  void f_columnhead() 
//void f_columnhead(union argument * /*arg*/)
void GnuPlot::F_Columnhead(union argument * /*arg*/)
{
	GpValue a;
	if(!_Df.evaluate_inside_using)
		IntError(Pgm.GetPrevTokenIdx(), "columnhead() called from invalid context");
	Pop(&a);
	_Df.ColumnForKeyTitle = (int)Real(&a);
	// This handles the case: plot ... using (column("FOO")) ... title columnhead 
	if(_Df.ColumnForKeyTitle == -1) {
		Push(Gstring(&a, _Df.df_key_title));
		return;
	}
	if(_Df.ColumnForKeyTitle < 0 || _Df.ColumnForKeyTitle > 9999)
		_Df.ColumnForKeyTitle = 0;
	/* Sep 2017 - It used to be that the program could be in either of two states,
	 * depending on where the call to columnheader(N) appeared.
	 * 1) called from a using spec;  we already read the header line
	 *    This means that df_column points to a structure containing column headers
	 * 2) called from 'title columnheader(N)'; we have not yet read the header line
	 *    and df_column == NULL
	 * Now we defer evaluation of the title via df_title_at, so case 2 never happens.
	 */
	if(_Df.df_column) {
		if((0 < _Df.ColumnForKeyTitle && _Df.ColumnForKeyTitle <= _Df.df_max_cols) && (_Df.df_column[_Df.ColumnForKeyTitle-1].header))
			Push(Gstring(&a, _Df.df_column[_Df.ColumnForKeyTitle-1].header));
		else {
			static char unknown_column[2] = {0, 0};
			Push(Gstring(&a, &unknown_column[0]));
		}
		_Pb.parse_1st_row_as_headers = true;
	}
	else {
		/* DEBUG */IntError(NO_CARET, "Internal error: df_column[] not initialized\n");
	}
}

/*{{{  void f_valid() */
//void f_valid(union argument * arg)
void GnuPlot::F_Valid(union argument * arg)
{
	GpValue a;
	Pop(&a);
	int column = (int)Magnitude(&a) - 1;
	int good = column >= 0 && column < _Df.df_no_cols && _Df.df_column[column].good == DF_GOOD;
	Push(Ginteger(&a, good));
}

/*}}} */
// 
// {{{  void f_timecolumn() 
// Version 5 - replace the old and very broken timecolumn(N) with
// a 2-parameter version that requires an explicit time format
// timecolumn(N, "format").
// 
//void f_timecolumn(union argument * arg)
void GnuPlot::F_TimeColumn(union argument * arg)
{
	GpValue a;
	GpValue b;
	struct tm tm;
	int num_param;
	int column;
	double usec = 0.0;
	Pop(&b); // this is the number of parameters 
	num_param = b.v.int_val;
	Pop(&b); // this is the time format string 
	switch(num_param) {
		case 2:
		    column = (int)Magnitude(Pop(&a));
		    break;
		case 1:
		    // No format parameter passed (v4-style call) 
		    // Only needed for backward compatibility 
		    column = static_cast<int>(Magnitude(&b));
		    b.v.string_val = sstrdup(AxS.P_TimeFormat);
		    b.Type = STRING;
		    break;
		default:
		    IntError(NO_CARET, "wrong number of parameters to timecolumn");
	}
	if(!_Df.evaluate_inside_using)
		IntError(Pgm.GetPrevTokenIdx(), "timecolumn() called from invalid context");
	if(b.Type != STRING)
		IntError(NO_CARET, "non-string passed as a format to timecolumn");
	if(column < 1 || column > _Df.df_no_cols || !_Df.df_column[column-1].position) {
		Ev.IsUndefined_ = true;
		Push(&a);
	}
	else {
		double reltime;
		td_type status = GStrPTime(_Df.df_column[column-1].position, b.v.string_val, &tm, &usec, &reltime);
		if(status == DT_TIMEDATE)
			Gcomplex(&a, gtimegm(&tm) + usec, 0.0);
		else if(status == DT_DMS)
			Gcomplex(&a, reltime, 0.0);
		else
			Ev.IsUndefined_ = true;
		Push(&a);
	}
	gpfree_string(&b);
}
//
// called by stats to load array STATS_column_header[]
//
//char * df_retrieve_columnhead(int column)
char * GnuPlot::DfRetrieveColumnHead(int column)
{
	return (column <= 0 || !_Df.df_column) ? NULL : _Df.df_column[column-1].header;
}

/*}}} */

//static int check_missing(const char * s)
int GnuPlot::CheckMissing(const char * s)
{
	// Match the string specified by 'set datafile missing' 
	if(_Df.missing_val) {
		size_t len = strlen(_Df.missing_val);
		if(strncmp(s, _Df.missing_val, len) == 0) {
			s += len;
			if(!(*s))
				return 1;
			if(!_Df.df_separators && isspace((uchar)*s))
				return 1;
			// s now points to the character after the "missing" sequence 
		}
	}
	// April 2013 - Treat an empty csv field as "missing" 
	if(_Df.df_separators && strchr(_Df.df_separators, *s))
		return 1;
	return 0;
}
// 
// Plotting routines can call this prior to invoking df_readline() to indicate
// that they expect a certain column to contain an ascii string rather than a number.
// 
//int expect_string(const char column)
int GnuPlot::ExpectString(const char column)
{
	// Used only by TABLESTYLE 
	if(column <= 0) {
		_Df.df_tabulate_strings = TRUE;
		return -1;
	}
	else {
		_Df.UseSpec[column-1].expected_type = CT_STRING;
		// Nasty hack to make 'plot "file" using "A":"B":"C" with labels' work.
		// The case of named columns is handled by create_call_column_at(),
		// which fakes an action table as if '(column("string"))' was written
		// in the using spec instead of simply "string". In this specific case, however,
		// we need the values as strings - so we change the action table to call
		// f_stringcolumn() instead of f_column. 
		if(_Df.UseSpec[column-1].at && (_Df.UseSpec[column-1].at->a_count == 2) && (_Df.UseSpec[column-1].at->actions[1].index == COLUMN))
			_Df.UseSpec[column-1].at->actions[1].index = STRINGCOLUMN;
		return _Df.UseSpec[column-1].column;
	}
}
// 
// Plotting routines can call this prior to invoking df_readline() to indicate
// that they cannot tolerate failing to return some value for this column,
// even if the input field is junk.
// 
//void require_value(const char column)
void GnuPlot::RequireValue(const char column)
{
	_Df.UseSpec[column-1].expected_type = CT_MUST_HAVE;
}
// 
// Load plot title used in key box from the string found earlier by df_readline.
// Called from get_data().
// 
//void df_set_key_title(curve_points * pPlot)
void GnuPlot::DfSetKeyTitle(curve_points * pPlot)
{
	if(_Df.df_key_title) {
		if(pPlot->plot_style == HISTOGRAMS && Gg.histogram_opts.type == HT_STACKED_IN_TOWERS) {
			// In this case it makes no sense to treat key titles in the usual
			// way, so we assume that it is supposed to be an xtic label.
			// Only for "plot ... title columnhead" 
			double xpos = pPlot->histogram_sequence + pPlot->histogram->start;
			AddTicUser(&AxS[FIRST_X_AXIS], _Df.df_key_title, xpos, -1);
			ZFREE(_Df.df_key_title);
		}
		else if(_Df.df_plot_title_at) { // What if there was already a title specified? 
			ReevaluatePlotTitle(pPlot);
		}
		else if(pPlot->title_is_suppressed) // "notitle <whatever-the-title-would-have-been>" 
			;
		else {
			// Note: I think we can only get here for histogram labels 
			FREEANDASSIGN(pPlot->title, _Df.df_key_title);
			_Df.df_key_title = NULL;
			pPlot->title_no_enhanced = !Gg.KeyT.enhanced;
		}
	}
}
//
// Load plot title for key box from columnheader.
// Called from eval_plots(), eval_3dplots() while parsing the plot title option
//
//void df_set_key_title_columnhead(const curve_points * plot)
void GnuPlot::DfSetKeyTitleColumnHead(const curve_points * pPlot)
{
	Pgm.Shift();
	if(Pgm.EqualsCur("(")) {
		Pgm.Shift();
		_Df.ColumnForKeyTitle = IntExpression();
		Pgm.Shift();
	}
	else if(!Pgm.EndOfCommand() && Pgm.IsANumber(Pgm.GetCurTokenIdx())) {
		_Df.ColumnForKeyTitle = IntExpression();
	}
	else {
		if(!pPlot) /* stats "name" option rather than plot title */
			_Df.ColumnForKeyTitle = _Df.UseSpec[0].column;
		else if(pPlot->plot_style == HISTOGRAMS)
			_Df.ColumnForKeyTitle = _Df.UseSpec[0].column;
		else if(pPlot->plot_style == PARALLELPLOT)
			_Df.ColumnForKeyTitle = _Df.UseSpec[0].column;
		else if(_Df.df_no_use_specs == 1)
			_Df.ColumnForKeyTitle = _Df.UseSpec[0].column;
		else if(pPlot->plot_type == DATA3D)
			_Df.ColumnForKeyTitle = _Df.UseSpec[2].column;
		else
			_Df.ColumnForKeyTitle = _Df.UseSpec[1].column;
	}
	// This results from  plot 'foo' using (column("name")) title columnhead 
	// FIXME:  builds on top of older circuitous method
	// - here we dummy up a call to columnhead(-1)
	// - somewhere else the real columnhead is stored in df_key_title
	// - when columnhead(-1) is evaluated, it returns the content of df_key_title
	// Why can't we dummy up columhead(real column) and skip the intermediate step?
	if(_Df.ColumnForKeyTitle == NO_COLUMN_HEADER) {
		free_at(_Df.df_plot_title_at);
		_Df.df_plot_title_at = create_call_columnhead();
	}
	_Pb.parse_1st_row_as_headers = true; // This may be redundant, but can't hurt 
}

//char * df_parse_string_field(const char * field)
char * GnuPlot::DfParseStringField(const char * field)
{
	char * temp_string = 0;
	if(field) {
		int length = 0;
		if(*field == '"') {
			field++;
			length = strcspn(field, "\"");
		}
		else if(_Df.df_separators) {
			length = strcspn(field, _Df.df_separators);
			if(length > static_cast<int>(strcspn(field, "\""))) /* Why? */
				length = strcspn(field, "\"");
		}
		else {
			length = strcspn(field, "\t ");
		}
		// If we are fed a file with unrecognized line termination then 
		// memory use can become excessive. Truncate and report error.  
		if(length > MAX_LINE_LEN) {
			length = MAX_LINE_LEN;
			IntWarn(NO_CARET, "input file contains very long line with no separators, truncating");
			if(strcspn(field, "\r") < MAX_LINE_LEN)
				IntError(NO_CARET, "      line contains embedded <CR>, wrong file format?");
		}
		temp_string = (char *)SAlloc::M(length+1);
		strncpy(temp_string, field, length);
		temp_string[length] = '\0';
		ParseEsc(temp_string);
	}
	return temp_string;
}

//static void add_key_entry(char * temp_string, int df_datum)
void GnuPlot::AddKeyEntry(char * pTempString, int dfDatum)
{
	// Only process key entries for first clause of spiderplot 
	if(_Df.df_current_plot->plot_style == SPIDERPLOT && _Df.df_current_plot->AxIdx_P != 1)
		return;
	// Associate this entry with the histogram or spiderplot it belongs to. 
	text_label * p_new_entry = (text_label *)SAlloc::M(sizeof(text_label));
	if(!_Df.df_current_plot->labels) {
		// The first text_label structure in the list is a place-holder 
		_Df.df_current_plot->labels = (text_label *)SAlloc::M(sizeof(text_label));
		memzero(_Df.df_current_plot->labels, sizeof(text_label));
		_Df.df_current_plot->labels->tag  = -1;
	}
	p_new_entry->text = sstrdup(pTempString);
	p_new_entry->tag = dfDatum;
	p_new_entry->font = NULL;
	p_new_entry->next = _Df.df_current_plot->labels->next;
	_Df.df_current_plot->labels->next = p_new_entry;
	FPRINTF((stderr, "add_key_entry( \"%s\", %d )\n", p_new_entry->text, p_new_entry->tag));
}
// 
// Construct 2D rotation matrix.
// R - Matrix to construct.
// alpha - Rotation angle.
// return - TRUE means a translation is required. 
// 
bool rotation_matrix_2D(double R[][2], double alpha)
{
	static const double I[2][2] = {{1, 0}, {0, 1}};
#define ANGLE_TOLERANCE 0.001
	if(fabs(alpha) < ANGLE_TOLERANCE) {
		/* Zero angle.  Unity rotation. */
		memcpy(R, I, sizeof(I));
		return FALSE;
	}
	else {
		R[0][0] = cos(alpha);
		R[0][1] = -sin(alpha);
		R[1][0] = sin(alpha);
		R[1][1] = cos(alpha);
		return TRUE;
	}
}
// 
// Construct 3D rotation matrix. 
// P - Matrix to construct. 
// p - Pointer to perpendicular vector. 
// return - TRUE means a translation is required. 
// 
bool rotation_matrix_3D(double P[][3], double * p)
{
	static const double I[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
	double scale, C1, C2;
#define x p[0]
#define y p[1]
#define z p[2]
	C1 = sqrt(x*x + y*y + z*z);
	C2 = sqrt(x*x + y*y);
	/* ????? Is there a precision constant for doubles similar to what is in limits.h for other types? */
	if((C1 < 10e-10) || (C2 < (10e-5*C1))) {
		/* Zero vector (invalid) || vector perpendiculat to x/y plane.  Unity rotation. */
		memcpy(P, I, sizeof(I));
		return FALSE;
	}
	else {
		scale = 1.0/(C1*C2);
		P[0][0] =    x*z * scale;
		P[0][1] =  -y*C1 * scale;
		P[0][2] =   x*C2 * scale;
		P[1][0] =    y*z * scale;
		P[1][1] =   x*C1 * scale;
		P[1][2] =   y*C2 * scale;
		P[2][0] = -C2*C2 * scale;
		P[2][1] =      0;
		P[2][2] =   z*C2 * scale;
		return TRUE;
	}
#undef x
#undef y
#undef z
}

df_byte_read_order_type byte_read_order(df_endianess_type file_endian)
{
	// Range limit file endianess to ensure that future file type function
	// programmer doesn't incorrectly access array and cause segmentation
	// fault unknowingly.
	return (df_byte_read_order_type)df_byte_read_order_map[THIS_COMPILER_ENDIAN][MIN(file_endian, DF_ENDIAN_TYPE_LENGTH-1)];
}

//void df_unset_datafile_binary()
void GnuPlot::DfUnsetDatafileBinary()
{
	ClearBinaryRecords(DF_DEFAULT_RECORDS);
	_Df.BinFileTypeDefault = _Df.BinFileTypeReset;
	_Df.df_bin_file_endianess_default = DF_BIN_FILE_ENDIANESS_RESET;
}

//void df_set_datafile_binary()
void GnuPlot::DfSetDataFileBinary()
{
	Pgm.Shift();
	if(Pgm.EndOfCommand())
		IntErrorCurToken("option expected");
	ClearBinaryRecords(DF_CURRENT_RECORDS);
	// Set current records to default in order to retain current default settings. 
	if(_Df.df_bin_record_default) {
		_Df.BinFileType = _Df.BinFileTypeDefault;
		_Df.BinFileEndianess = _Df.df_bin_file_endianess_default;
		DfAddBinaryRecords(_Df.df_num_bin_records_default, DF_CURRENT_RECORDS);
		memcpy(_Df.df_bin_record, _Df.df_bin_record_default, _Df.NumBinRecords*sizeof(df_binary_file_record_struct));
	}
	else {
		_Df.BinFileType = _Df.BinFileTypeReset;
		_Df.BinFileEndianess = DF_BIN_FILE_ENDIANESS_RESET;
		DfAddBinaryRecords(1, DF_CURRENT_RECORDS);
	}
	// Process the binary tokens. 
	DfSetPlotMode(MODE_QUERY);
	PlotOptionBinary(FALSE, TRUE);
	// Copy the modified settings as the new default settings. 
	_Df.BinFileTypeDefault = _Df.BinFileType;
	_Df.df_bin_file_endianess_default = _Df.BinFileEndianess;
	ClearBinaryRecords(DF_DEFAULT_RECORDS);
	DfAddBinaryRecords(_Df.NumBinRecords, DF_DEFAULT_RECORDS);
	memcpy(_Df.df_bin_record_default, _Df.df_bin_record, _Df.df_num_bin_records_default*sizeof(df_binary_file_record_struct));
}

//void gpbin_filetype_function()
void GnuPlot::FileTypeFunction_GpBin()
{
	// Gnuplot binary. 
	_Df.df_matrix_file = true;
	_Df.df_binary_file = true;
}

//void raw_filetype_function()
void GnuPlot::FileTypeFunction_Raw()
{
	// No information in file, just data. 
	_Df.df_matrix_file = false;
	_Df.df_binary_file = true;
}

//void avs_filetype_function()
void GnuPlot::FileTypeFunction_Avs()
{
	// A very simple file format:
	// 8 byte header (width and height, 4 bytes each), unknown endian
	// followed by 4 bytes per pixel (alpha, red, green, blue).
	ulong M, N;
	int read_order = 0;
	// open (header) file 
	FILE * fp = LoadPath_fopen(_Df.df_filename, "rb");
	if(!fp)
		OsError(NO_CARET, "Can't open data file \"%s\"", _Df.df_filename);
	// read header: it is only 8 bytes 
	if(!fread(&M, 4, 1, fp))
		OsError(NO_CARET, "Can't read first dimension in data file \"%s\"", _Df.df_filename);
	if(M > 0xFFFF)
		read_order = DF_3210;
	df_swap_bytes_by_endianess((char *)&M, read_order, 4);
	if(!fread(&N, 4, 1, fp))
		OsError(NO_CARET, "Can't read second dimension in data file \"%s\"", _Df.df_filename);
	df_swap_bytes_by_endianess((char *)&N, read_order, 4);

	fclose(fp);
	_Df.df_matrix_file = false;
	_Df.df_binary_file = true;
	_Df.df_bin_record[0].scan_skip[0] = 8;
	_Df.df_bin_record[0].scan_dim[0] = M;
	_Df.df_bin_record[0].scan_dim[1] = N;

	_Df.df_bin_record[0].scan_dir[0] = 1;
	_Df.df_bin_record[0].scan_dir[1] = -1;
	_Df.df_bin_record[0].scan_generate_coord = TRUE;
	_Df.df_bin_record[0].cart_scan[0] = DF_SCAN_POINT;
	_Df.df_bin_record[0].cart_scan[1] = DF_SCAN_LINE;

	// The four components are 1 byte each. Permute ARGB to RGBA 
	DfExtendBinaryColumns(4);
	DfSetReadType(1, DF_UCHAR);
	DfSetReadType(2, DF_UCHAR);
	DfSetReadType(3, DF_UCHAR);
	DfSetReadType(4, DF_UCHAR);
	DfSetSkipBefore(1, 0);

	_Df.df_no_use_specs = 4;
	_Df.UseSpec[0].column = 2;
	_Df.UseSpec[1].column = 3;
	_Df.UseSpec[2].column = 4;
	_Df.UseSpec[3].column = 1;
}

//static void initialize_binary_vars()
void GnuPlot::InitializeBinaryVars()
{
	// Initialize for the df_readline() routine. 
	_Df.df_bin_record_count = 0;
	_Df.df_M_count = _Df.df_N_count = _Df.df_O_count = 0;
	// Set default binary data widths and skip paratemers. 
	_Df.df_no_bin_cols = 0;
	DfSetSkipBefore(1, 0);
	// Copy the default binary records to the active binary records.  The number
	// of records will always be at least one in case "record", "array",
	// or "filetype" are not issued by the user.
	ClearBinaryRecords(DF_CURRENT_RECORDS);
	if(_Df.df_num_bin_records_default) {
		_Df.BinFileType = _Df.BinFileTypeDefault;
		_Df.BinFileEndianess = _Df.df_bin_file_endianess_default;
		DfAddBinaryRecords(_Df.df_num_bin_records_default, DF_CURRENT_RECORDS);
		memcpy(_Df.df_bin_record, _Df.df_bin_record_default, _Df.NumBinRecords*sizeof(df_binary_file_record_struct));
	}
	else {
		_Df.BinFileType = _Df.BinFileTypeReset;
		_Df.BinFileEndianess = DF_BIN_FILE_ENDIANESS_RESET;
		DfAddBinaryRecords(1, DF_CURRENT_RECORDS);
	}
}
// 
// Place a special marker in the using list to derive the x/y/z value
// from the appropriate dimensional counter.
// 
//void df_insert_scanned_use_spec(int uspec)
void GnuPlot::DfInsertScannedUseSpec(int uspec)
{
	// Place a special marker in the using list to derive the z value
	// from the third dimensional counter, which will be zero.
	if(_Df.df_no_use_specs >= MAXDATACOLS)
		IntError(NO_CARET, too_many_cols_msg);
	else {
		int j;
		for(j = _Df.df_no_use_specs; j > uspec; j--)
			_Df.UseSpec[j] = _Df.UseSpec[j-1];
		_Df.UseSpec[uspec].column = (uspec == 2 ? DF_SCAN_PLANE : DF_SCAN_LINE);
		// The at portion is set to NULL here, but this doesn't mash
		// a valid memory pointer because any valid memory pointers
		// were copied to new locations in the previous for loop.
		_Df.UseSpec[uspec].at = NULL; /* Not a bad memory pointer overwrite!! */
		_Df.df_no_use_specs++;
	}
}

/* Not the most elegant way of defining the default columns, but I prefer
 * this to switch and conditional statements when there are so many styles.
 */
typedef struct df_bin_default_columns {
	PLOT_STYLE plot_style;
	short excluding_gen_coords; /* Number of columns of information excluding generated coordinates. */
	short dimen_in_2d; /* Number of additional columns required (in 2D plot) if coordinates not generated. */
} df_bin_default_columns;
df_bin_default_columns default_style_cols[] = {
	{LINES, 1, 1},
	{POINTSTYLE, 1, 1},
	{IMPULSES, 1, 1},
	{LINESPOINTS, 1, 1},
	{DOTS, 1, 1},
	{XERRORBARS, 2, 1},
	{YERRORBARS, 2, 1},
	{XYERRORBARS, 3, 1},
	{BOXXYERROR, 3, 1},
	{BOXES, 1, 1},
	{BOXERROR, 3, 1},
	{STEPS, 1, 1},
	{FSTEPS, 1, 1},
	{FILLSTEPS, 1, 1},
	{HISTEPS, 1, 1},
	{VECTOR, 2, 2},
	{CANDLESTICKS, 4, 1},
	{FINANCEBARS, 4, 1},
	{BOXPLOT, 2, 1},
	{XERRORLINES, 2, 1},
	{YERRORLINES, 2, 1},
	{XYERRORLINES, 3, 1},
	{FILLEDCURVES, 1, 1},
	{PM3DSURFACE, 1, 2},
	{LABELPOINTS, 2, 1},
	{HISTOGRAMS, 1, 0},
	{IMAGE, 1, 2},
	{RGBIMAGE, 3, 2},
	{RGBA_IMAGE, 4, 2},
	{CIRCLES, 2, 1},
	{ELLIPSES, 2, 3},
	{TABLESTYLE, 0, 0}
};

/* FIXME!!!
 * EAM Feb 2008:
 * This whole routine is a disaster.  It makes so many broken assumptions it's not funny.
 * Other than filling in the first two columns of an implicit matrix, I suspect we can
 * do away with it altogether. Frankly, we _don't care_ how many columns there are,
 * so long as the ones that are present are mapped to the right ordering.
 */
//static void adjust_binary_use_spec(curve_points * plot)
void GnuPlot::AdjustBinaryUseSpec(curve_points * pPlot)
{
	const char * nothing_known = "a 'using' specifier is required for that binary plot style";
	uint ps_index;
	enum PLOT_STYLE plot_style = pPlot ? pPlot->plot_style : LINES;
	// The default binary matrix format is nonuniform, i.e.
	// it has an extra row and column for sample coordinates.
	if(_Df.df_matrix_file && _Df.df_binary_file)
		_Df.df_nonuniform_matrix = true;
	// Determine index. 
	for(ps_index = 0; ps_index < sizeof(default_style_cols)/sizeof(default_style_cols[0]); ps_index++) {
		if(default_style_cols[ps_index].plot_style == plot_style)
			break;
	}
	// A known default is all very well, but if there was an actual using spec
	// that's all we need.
	if(ps_index == sizeof(default_style_cols)/sizeof(default_style_cols[0]) && !_Df.df_no_use_specs)
		IntError(NO_CARET, nothing_known);
	// Matrix format is interpreted as always having three columns. 
	if(_Df.df_matrix_file) {
		if(_Df.df_no_bin_cols > 3)
			IntError(NO_CARET, "Matrix data contains only three columns");
		DfExtendBinaryColumns(3);
	}
	// If nothing has been done to set the using specs, use the default using
	// characteristics for the style.
	if(!_Df.df_no_use_specs) {
		if(!_Df.df_matrix_file) {
			int no_cols = default_style_cols[ps_index].excluding_gen_coords;
			if(!no_cols)
				IntError(NO_CARET, nothing_known);
			// If coordinates are generated, make sure this plot style allows it.
			// Otherwise, add in the number of generated coordinates and add an
			// extra column if using `splot`.
			if(_Df.NumBinRecords && _Df.df_bin_record[0].scan_generate_coord) {
				if(default_style_cols[ps_index].dimen_in_2d == 0)
					IntError(NO_CARET, "Cannot generate coords for that plot style");
			}
			else {
				// If there aren't generated coordinates, then add the
				// amount of columns that would be generated.
				no_cols += default_style_cols[ps_index].dimen_in_2d;
				if(_Df.df_plot_mode == MODE_SPLOT)
					no_cols++;
			}
			assert(no_cols <= MAXDATACOLS);
			// Nothing need be done here to set the using specs because they
			// will have been initialized appropriately and left unaltered.
			// So just set the number of specs.
			_Df.df_no_use_specs = no_cols;
			DfExtendBinaryColumns(no_cols);
		}
		else {
			// Number of columns is fixed at three and no using specs given.  Do what we can.
			// The obvious best combination is two dimensional coordinates and one information
			// value.  One wonders what to do if a matrix is only one column; can be treated
			// as linear?  This isn't implemented here, but if it were, this is where it should go.
			if((default_style_cols[ps_index].dimen_in_2d == 2) && (default_style_cols[ps_index].excluding_gen_coords == 1)) {
				_Df.df_no_use_specs = 3;
			}
			else if((default_style_cols[ps_index].dimen_in_2d == 1) && (default_style_cols[ps_index].excluding_gen_coords == 1) ) {
				if(_Df.df_plot_mode == MODE_SPLOT)
					_Df.df_no_use_specs = 3;
				else {
					// Command:  plot 'foo' matrix       with no using spec 
					// Matrix element treated as y value rather than z value 
					_Df.df_no_use_specs = 2;
					_Df.UseSpec[1].column = 3;
				}
			}
			else
				IntError(NO_CARET, "Plot style does not conform to three column data in this graph mode");
		}
	}
	if(_Df.NumBinRecords && _Df.df_bin_record[0].scan_generate_coord && !_Df.df_matrix_file) {
		int i;
		use_spec_s original_use_spec[MAXDATACOLS];
		int added_columns = 0;
		// Keep record of the original using specs. 
		memcpy(original_use_spec, _Df.UseSpec, sizeof(_Df.UseSpec));
		// Put in columns at front for generated variables. 
		for(i = 0; i < 3; i++) {
			if(_Df.df_bin_record[0].cart_dim[i] || _Df.df_bin_record[0].scan_dim[i])
				added_columns++;
			else
				break;
		}
		if((_Df.df_no_use_specs + added_columns) >= MAXDATACOLS)
			IntError(NO_CARET, too_many_cols_msg);
		else {
			// Shift the original columns over by added number of columns, but only if not matrix data.
			memcpy(&_Df.UseSpec[added_columns], original_use_spec, _Df.df_no_use_specs*sizeof(_Df.UseSpec[0]));
			// The at portion is set to NULL here, but this doesn't mash
			// a valid memory pointer because any valid memory pointers
			// were copied to new locations in the previous memcpy().
			for(i = 0; i < added_columns; i++) {
				_Df.UseSpec[i].column = _Df.df_bin_record[0].cart_scan[i];
				_Df.UseSpec[i].at = 0; // Not a bad memory pointer overwrite!! 
			}
			_Df.df_no_use_specs += added_columns; // Do not extend columns for generated coordinates. 
		}
		if(_Df.df_plot_mode == MODE_SPLOT) {
			// For binary data having an implied uniformly sampled grid, treat
			// less than three-dimensional data in special ways based upon what
			// is being plotted.
			for(int k = 0; k < _Df.NumBinRecords; k++) {
				if((_Df.df_bin_record[k].cart_dim[2] == 0) && (_Df.df_bin_record[k].scan_dim[2] == 0)) {
					if(default_style_cols[ps_index].dimen_in_2d > 2)
						IntError(NO_CARET, "Plot style requires higher than two-dimensional sampling array");
					else {
						if((_Df.df_bin_record[k].cart_dim[1] == 0) && (_Df.df_bin_record[k].scan_dim[1] == 0)) {
							if(default_style_cols[ps_index].dimen_in_2d > 1)
								IntError(NO_CARET, "Plot style requires higher than one-dimensional sampling array");
							else {
								// Place a special marker in the using list to derive the y value
								// from the second dimensional counter.
								DfInsertScannedUseSpec(1);
							}
						}
						// Place a special marker in the using list to derive the z value
						// from the third dimensional counter.
						DfInsertScannedUseSpec(2);
					}
				}
			}
		}
	}
}

//static void plot_option_binary(bool set_matrix, bool set_default)
void GnuPlot::PlotOptionBinary(bool setMatrix, bool setDefault)
{
	bool duplication = FALSE;
	bool set_record = FALSE;
	bool set_array = FALSE;
	bool set_dx = FALSE;
	bool set_dy = FALSE;
	bool set_dz = FALSE;
	bool set_center = FALSE;
	bool set_origin = FALSE;
	bool set_skip = FALSE;
	bool set_endian = FALSE;
	bool set_rotation = FALSE;
	bool set_perpendicular = FALSE;
	bool set_flip = FALSE;
	bool set_noflip = FALSE;
	bool set_flipx = FALSE;
	bool set_flipy = FALSE;
	bool set_flipz = FALSE;
	bool set_scan = FALSE;
	bool set_format = FALSE;
	// Binary file type must be the first word in the command following `binary`" 
	if(_Df.BinFileTypeDefault)
		_Df.BinFileType = _Df.BinFileTypeDefault;
	if(Pgm.AlmostEqualsCur("file$type") || (_Df.BinFileType >= 0)) {
		int i;
		char file_ext[8] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
		// Above keyword not part of pre-existing binary definition. So use general binary. 
		if(setMatrix)
			IntErrorCurToken(matrix_general_binary_conflict_msg);
		_Df.df_matrix_file = false;
		if(Pgm.AlmostEqualsCur("file$type")) {
			Pgm.Shift();
			if(!Pgm.EqualsCur("="))
				IntErrorCurToken(equal_symbol_msg);
			Pgm.Shift();
			Pgm.CopyStr(file_ext, Pgm.GetCurTokenIdx(), 8);
			{
				int r = SIntToSymbTab_GetId(DfBinFileTypeTable, SIZEOFARRAY(DfBinFileTypeTable), file_ext);
				if(r)
					_Df.BinFileType = static_cast<FileType>(r);
				else {
					// Maybe set to "auto" and continue? 
					_Df.BinFileType = filtypAuto; // @sobolev
					IntErrorCurToken("Unrecognized filetype; try \"show datafile binary filetypes\"");
				}
			}
			/*for(i = 0; df_bin_filetype_table[i].key; i++) {
				if(sstreqi_ascii(file_ext, df_bin_filetype_table[i].key)) {
					binary_input_function = df_bin_filetype_table[i].value;
					_Df.BinFileType = i;
					break;
				}
			}
			if(_Df.BinFileType != i) // Maybe set to "auto" and continue? 
				IntErrorCurToken("Unrecognized filetype; try \"show datafile binary filetypes\"");*/
			Pgm.Shift();
		}
		{
			FileType _file_type = _Df.BinFileType;
			//if(_Df.df_plot_mode != MODE_QUERY && sstreq("auto", df_bin_filetype_table[_Df.BinFileType].key)) {
			if(_Df.df_plot_mode != MODE_QUERY && oneof2(_file_type, filtypUndef, filtypAuto)) {
				const char * file_ext = strrchr(_Df.df_filename, '.');
				if(file_ext) {
					int r = SIntToSymbTab_GetId(DfBinFileTypeTable, SIZEOFARRAY(DfBinFileTypeTable), file_ext+1);
					_file_type = static_cast<FileType>(r);
				}
				else {
					_file_type = filtypBin;
				}
				if(_file_type == filtypUndef) {
					IntError(NO_CARET, "Unrecognized filename extension; try \"show datafile binary filetypes\"");
				}
				/*
				if(file_ext++) {
					for(int i = 0; df_bin_filetype_table[i].key; i++)
						if(sstreqi_ascii(file_ext, df_bin_filetype_table[i].key))
							binary_input_function = df_bin_filetype_table[i].value;
				}
				if(binary_input_function == auto_filetype_function)
					IntError(NO_CARET, "Unrecognized filename extension; try \"show datafile binary filetypes\"");
				*/
			}
			// Unless only querying settings, call the routine to prep binary data parameters. 
			if(_Df.df_plot_mode != MODE_QUERY) {
				switch(_file_type) {
					case filtypAvs:
						FileTypeFunction_Avs();
						break;
					case filtypBin:
						FileTypeFunction_Raw();
						break;
					case filtypEdf:
						FileTypeFunction_Edf();
						break;
					case filtypEhf:
						FileTypeFunction_Edf();
						break;
					case filtypGif:
						FileTypeFunction_Gif();
						break;
					case filtypGpBin:
						FileTypeFunction_GpBin();
						break;
					case filtypJpeg:
						FileTypeFunction_Jpeg();
						break;
					case filtypPng:
						FileTypeFunction_Png();
						break;
					case filtypRaw:
						FileTypeFunction_Raw();
						break;
					case filtypRgb:
						FileTypeFunction_Raw();
						break;
					case filtypAuto:
						auto_filetype_function();
						break;
					default:
						// @error
						break;
				}
				//(*binary_input_function)();
				_Df.df_xpixels = _Df.df_bin_record[0].scan_dim[0];
				_Df.df_ypixels = _Df.df_bin_record[0].scan_dim[1];
				FPRINTF((stderr, "datafile.c:%d  image dimensions %d x %d\n", __LINE__, _Df.df_xpixels, _Df.df_ypixels));
			}
		}
		// Now, at this point anything that was filled in for "scan" should override the "cart" variables.
		for(i = 0; i < _Df.NumBinRecords; i++) {
			int j;
			// Dimension 
			if(_Df.df_bin_record[i].scan_dim[0] != df_bin_record_reset.scan_dim[0])
				for(j = 0; j < 3; j++)
					_Df.df_bin_record[i].cart_dim[j] = 0;
			// Delta 
			for(j = 0; j < 3; j++)
				if(_Df.df_bin_record[i].scan_delta[j] != 0.0) {
					for(int k = 0; k < 3; k++)
						if(_Df.df_bin_record[i].cart_scan[k] == (DF_SCAN_POINT - j))
							_Df.df_bin_record[i].cart_delta[k] = 0;
				}
			// Translation 
			if(_Df.df_bin_record[i].scan_trans != DF_TRANSLATE_DEFAULT)
				_Df.df_bin_record[i].cart_trans = DF_TRANSLATE_DEFAULT;
		}
	}
	while(!Pgm.EndOfCommand()) {
		char origin_and_center_conflict_message[] = "Can specify `origin` or `center`, but not both";
		// look for record 
		if(Pgm.AlmostEqualsCur("rec$ord")) {
			if(set_record) {
				duplication = true; 
				break;
			}
			Pgm.Shift();
			// Above keyword not part of pre-existing binary definition.  So use general binary.
			if(setMatrix)
				IntErrorCurToken(matrix_general_binary_conflict_msg);
			_Df.df_matrix_file = false;
			PlotOptionArray();
			set_record = true;
			_Df.df_xpixels = _Df.df_bin_record[_Df.NumBinRecords-1].cart_dim[0];
			_Df.df_ypixels = _Df.df_bin_record[_Df.NumBinRecords-1].cart_dim[1];
			FPRINTF((stderr, "datafile.c:%d  record dimensions %d x %d\n", __LINE__, _Df.df_xpixels, _Df.df_ypixels));
			continue;
		}
		// look for array 
		if(Pgm.AlmostEqualsCur("arr$ay")) {
			int i;
			if(set_array) {
				duplication = TRUE; 
				break;
			}
			Pgm.Shift();
			// Above keyword not part of pre-existing binary definition.  So use general binary. 
			if(setMatrix)
				IntErrorCurToken(matrix_general_binary_conflict_msg);
			_Df.df_matrix_file = false;
			PlotOptionArray();
			for(i = 0; i < _Df.NumBinRecords; i++) {
				// Indicate that coordinate info should be generated internally 
				_Df.df_bin_record[i].scan_generate_coord = TRUE;
			}
			set_array = TRUE;
			_Df.df_xpixels = _Df.df_bin_record[_Df.NumBinRecords-1].cart_dim[0];
			_Df.df_ypixels = _Df.df_bin_record[_Df.NumBinRecords-1].cart_dim[1];
			FPRINTF((stderr, "datafile.c:%d  array dimensions %d x %d\n", __LINE__, _Df.df_xpixels, _Df.df_ypixels));
			continue;
		}
		// deal with spacing between array points 
		if(Pgm.EqualsCur("dx") || Pgm.EqualsCur("dt")) {
			if(set_dx) {
				duplication = TRUE; 
				break;
			}
			Pgm.Shift();
			PlotOptionMultiValued(DF_DELTA, 0);
			if(!set_dy) {
				for(int i = 0; i < _Df.NumBinRecords; i++)
					_Df.df_bin_record[i].cart_delta[1] = _Df.df_bin_record[i].cart_delta[0];
			}
			if(!set_dz) {
				for(int i = 0; i < _Df.NumBinRecords; i++)
					_Df.df_bin_record[i].cart_delta[2] = _Df.df_bin_record[i].cart_delta[0];
			}
			set_dx = TRUE;
			continue;
		}
		if(Pgm.EqualsCur("dy") || Pgm.EqualsCur("dr")) {
			if(set_dy) {
				duplication = TRUE; break;
			}
			if(!set_array && !_Df.df_bin_record)
				IntErrorCurToken("Must specify a sampling array size before indicating spacing in second dimension");
			Pgm.Shift();
			PlotOptionMultiValued(DF_DELTA, 1);
			if(!set_dz) {
				for(int i = 0; i < _Df.NumBinRecords; i++)
					_Df.df_bin_record[i].cart_delta[2] = _Df.df_bin_record[i].cart_delta[1];
			}
			set_dy = TRUE;
			continue;
		}
		if(Pgm.EqualsCur("dz")) {
			IntErrorCurToken("Currently not supporting three-dimensional sampling");
			if(set_dz) {
				duplication = TRUE; break;
			}
			if(!set_array && !_Df.df_bin_record)
				IntErrorCurToken("Must specify a sampling array size before indicating spacing in third dimension");
			Pgm.Shift();
			PlotOptionMultiValued(DF_DELTA, 2);
			set_dz = TRUE;
			continue;
		}
		// deal with direction in which sampling increments 
		if(Pgm.EqualsCur("flipx")) {
			if(set_flipx) {
				duplication = TRUE; 
				break;
			}
			Pgm.Shift();
			// If no equal sign, then set flip true for all records. 
			if(!Pgm.EqualsCur("=")) {
				for(int i = 0; i < _Df.NumBinRecords; i++)
					_Df.df_bin_record[i].cart_dir[0] = -1;
			}
			else {
				PlotOptionMultiValued(DF_FLIP_AXIS, 0);
			}
			set_flipx = TRUE;
			continue;
		}
		if(Pgm.EqualsCur("flipy")) {
			if(set_flipy) {
				duplication = TRUE; break;
			}
			if(!set_array && !_Df.df_bin_record)
				IntErrorCurToken("Must specify a sampling array size before indicating flip in second dimension");
			Pgm.Shift();
			// If no equal sign, then set flip true for all records. 
			if(!Pgm.EqualsCur("=")) {
				for(int i = 0; i < _Df.NumBinRecords; i++)
					_Df.df_bin_record[i].cart_dir[1] = -1;
			}
			else {
				PlotOptionMultiValued(DF_FLIP_AXIS, 1);
			}
			set_flipy = true;
			continue;
		}
		if(Pgm.EqualsCur("flipz")) {
			IntErrorCurToken("Currently not supporting three-dimensional sampling");
			if(set_flipz) {
				duplication = true;
				break;
			}
			if(!set_array && !_Df.df_bin_record)
				IntErrorCurToken("Must specify a sampling array size before indicating spacing in third dimension");
			Pgm.Shift();
			// If no equal sign, then set flip true for all records. 
			if(!Pgm.EqualsCur("=")) {
				for(int i = 0; i < _Df.NumBinRecords; i++)
					_Df.df_bin_record[i].cart_dir[2] = -1;
			}
			else {
				PlotOptionMultiValued(DF_FLIP_AXIS, 2);
			}
			set_flipz = true;
			continue;
		}
		// Deal with flipping data for individual records. 
		if(Pgm.EqualsCur("flip")) {
			if(set_flip) {
				duplication = true;
				break;
			}
			Pgm.Shift();
			PlotOptionMultiValued(DF_FLIP, -1);
			set_flip = true;
			continue;
		}
		// Deal with flipping data for individual records. 
		if(Pgm.EqualsCur("noflip")) {
			if(set_noflip) {
				duplication = true;
				break;
			}
			Pgm.Shift();
			PlotOptionMultiValued(DF_FLIP, 1);
			set_noflip = true;
			continue;
		}
		// Deal with manner in which dimensions are scanned from file. 
		if(Pgm.EqualsCur("scan")) {
			if(set_scan) {
				duplication = true;
				break;
			}
			Pgm.Shift();
			if(Pgm.AlmostEquals(Pgm.GetCurTokenIdx()+1, "yx$z"))
				_Df.df_transpose = true;
			PlotOptionMultiValued(DF_SCAN, 0);
			set_scan = true;
			continue;
		}
		// Deal with manner in which dimensions are scanned from file. 
		if(Pgm.AlmostEqualsCur("trans$pose")) {
			if(set_scan) {
				duplication = true;
				break;
			}
			Pgm.Shift();
			for(int i = 0; i < _Df.NumBinRecords; i++)
				memcpy(_Df.df_bin_record[i].cart_scan, df_bin_scan_table_2D[TRANSPOSE_INDEX].scan, sizeof(_Df.df_bin_record[0].cart_scan));
			set_scan = true;
			_Df.df_transpose = true;
			continue;
		}
		// deal with origin 
		if(Pgm.AlmostEqualsCur("orig$in")) {
			if(set_center)
				IntErrorCurToken(origin_and_center_conflict_message);
			if(set_origin) {
				duplication = true;
				break;
			}
			Pgm.Shift();
			PlotOptionMultiValued(DF_ORIGIN, _Df.df_plot_mode);
			set_origin = true;
			continue;
		}
		// deal with origin 
		if(Pgm.AlmostEqualsCur("cen$ter")) {
			if(set_origin)
				IntErrorCurToken(origin_and_center_conflict_message);
			if(set_center) {
				duplication = true; 
				break;
			}
			Pgm.Shift();
			PlotOptionMultiValued(DF_CENTER, _Df.df_plot_mode);
			set_center = true;
			continue;
		}
		// deal with rotation angle 
		if(Pgm.AlmostEqualsCur("rot$ation") || Pgm.AlmostEqualsCur("rot$ate")) {
			if(set_rotation) {
				duplication = TRUE; break;
			}
			Pgm.Shift();
			PlotOptionMultiValued(DF_ROTATION, 0);
			set_rotation = TRUE;
			continue;
		}
		// deal with rotation angle 
		if(Pgm.AlmostEqualsCur("perp$endicular")) {
			if(_Df.df_plot_mode == MODE_PLOT)
				IntErrorCurToken("Key word `perpendicular` is not allowed with `plot` command");
			if(set_perpendicular) {
				duplication = TRUE; break;
			}
			Pgm.Shift();
			PlotOptionMultiValued(DF_PERPENDICULAR, 0);
			set_perpendicular = TRUE;
			continue;
		}
		// deal with number of bytes to skip before record 
		if(Pgm.AlmostEqualsCur("skip")) {
			if(set_skip) {
				duplication = TRUE; 
				break;
			}
			Pgm.Shift();
			PlotOptionMultiValued(DF_SKIP, 0);
			set_skip = TRUE;
			continue;
		}
		// deal with byte order 
		if(Pgm.AlmostEqualsCur("end$ian")) {
			if(set_endian) {
				duplication = TRUE; 
				break;
			}
			Pgm.Shift();
			// Require equal symbol. 
			if(!Pgm.EqualsCur("="))
				IntErrorCurToken(equal_symbol_msg);
			Pgm.Shift();
			if(Pgm.AlmostEqualsCur("def$ault"))
				_Df.BinFileEndianess = THIS_COMPILER_ENDIAN;
			else if(Pgm.EqualsCur("swap") || Pgm.EqualsCur("swab"))
				_Df.BinFileEndianess = (df_endianess_type)((~_Df.BinFileEndianess)&3); /* complement and isolate lowest two its */
			else if(Pgm.AlmostEqualsCur("lit$tle"))
				_Df.BinFileEndianess = DF_LITTLE_ENDIAN;
			else if(Pgm.EqualsCur("big"))
				_Df.BinFileEndianess = DF_BIG_ENDIAN;
#if SUPPORT_MIDDLE_ENDIAN
			else if(Pgm.AlmostEqualsCur("mid$dle") || Pgm.EqualsCur("pdp"))
				_Df.BinFileEndianess = DF_PDP_ENDIAN;
			else
				IntErrorCurToken("Options are default, swap (swab), little, big, middle (pdp)");
#else
			else
				IntErrorCurToken("Options are default, swap (swab), little, big");
#endif
			Pgm.Shift();
			set_endian = true;
			continue;
		}
		// deal with various types of binary files 
		if(Pgm.AlmostEqualsCur("form$at")) {
			if(set_format) {
				duplication = true; 
				break;
			}
			Pgm.Shift();
			// Format string not part of pre-existing binary definition.  So use general binary. 
			if(setMatrix)
				IntErrorCurToken(matrix_general_binary_conflict_msg);
			_Df.df_matrix_file = false;
			// Require equal sign 
			if(!Pgm.EqualsCur("="))
				IntErrorCurToken(equal_symbol_msg);
			Pgm.Shift();
			if(setDefault) {
				FREEANDASSIGN(_Df.df_binary_format, TryToGetString());
			}
			else {
				char * format_string = TryToGetString();
				if(!format_string)
					IntErrorCurToken("missing format string");
				PlotOptionBinaryFormat(format_string);
				SAlloc::F(format_string);
			}
			set_format = TRUE;
			continue;
		}
		break; /* unknown option */
	} /* while (!Pgm.EndOfCommand()) */
	if(duplication)
		IntErrorCurToken("Duplicated or contradicting arguments in datafile options");
	if(!setDefault && !setMatrix && _Df.df_num_bin_records_default) {
		IntWarn(NO_CARET, "using default binary record/array structure");
	}
	if(!set_format && !_Df.df_matrix_file) {
		if(_Df.df_binary_format) {
			PlotOptionBinaryFormat(_Df.df_binary_format);
			IntWarn(NO_CARET, "using default binary format");
		}
	}
}

//void df_add_binary_records(int num_records_to_add, df_records_type records_type)
void GnuPlot::DfAddBinaryRecords(int num_records_to_add, df_records_type records_type)
{
	int i;
	int new_number;
	df_binary_file_record_struct ** bin_record;
	int * num_bin_records;
	int * max_num_bin_records;
	if(records_type == DF_CURRENT_RECORDS) {
		bin_record = &_Df.df_bin_record;
		num_bin_records = &_Df.NumBinRecords;
		max_num_bin_records = &_Df.df_max_num_bin_records;
	}
	else {
		bin_record = &_Df.df_bin_record_default;
		num_bin_records = &_Df.df_num_bin_records_default;
		max_num_bin_records = &_Df.df_max_num_bin_records_default;
	}
	new_number = *num_bin_records + num_records_to_add;
	if(new_number > *max_num_bin_records) {
		*bin_record = (df_binary_file_record_struct *)SAlloc::R(*bin_record, new_number * sizeof(df_binary_file_record_struct));
		*max_num_bin_records = new_number;
	}
	for(i = 0; i < num_records_to_add; i++) {
		memcpy(*bin_record + *num_bin_records, &df_bin_record_reset, sizeof(df_binary_file_record_struct));
		(*num_bin_records)++;
	}
}

//static void clear_binary_records(df_records_type records_type)
void GnuPlot::ClearBinaryRecords(df_records_type records_type)
{
	df_binary_file_record_struct * temp_bin_record = 0;
	int * temp_num_bin_records = 0;
	if(records_type == DF_CURRENT_RECORDS) {
		temp_bin_record = _Df.df_bin_record;
		temp_num_bin_records = &_Df.NumBinRecords;
	}
	else {
		temp_bin_record = _Df.df_bin_record_default;
		temp_num_bin_records = &_Df.df_num_bin_records_default;
	}
	for(int i = 0; i < *temp_num_bin_records; i++) {
		if(temp_bin_record[i].memory_data) {
			ZFREE(temp_bin_record[i].memory_data);
		}
	}
	*temp_num_bin_records = 0;
}
//
// Syntax is:   array=(xdim,ydim):(xdim,ydim):CONST:(xdim) etc
//
//static void plot_option_array()
void GnuPlot::PlotOptionArray()
{
	int number_of_records = 0;
	if(!Pgm.EqualsCur("="))
		IntErrorCurToken(equal_symbol_msg);
	do {
		Pgm.Shift();
		// Partial backward compatibility with syntax up to 4.2.4 
		if(Pgm.IsANumber(Pgm.GetCurTokenIdx())) {
			if(++number_of_records > _Df.NumBinRecords)
				DfAddBinaryRecords(1, DF_CURRENT_RECORDS);
			_Df.df_bin_record[_Df.NumBinRecords-1].cart_dim[0] = IntExpression();
			// Handle the old syntax:  array=123x456 
			if(!Pgm.EndOfCommand()) {
				char xguy[8]; int itmp = 0;
				Pgm.CopyStr(xguy, Pgm.GetCurTokenIdx(), 6);
				if(xguy[0] == 'x') {
					sscanf(&xguy[1], "%d", &itmp);
					_Df.df_bin_record[_Df.NumBinRecords-1].cart_dim[1] = itmp;
					Pgm.Shift();
				}
			}
		}
		else if(Pgm.EqualsCur("(")) {
			Pgm.Shift();
			if(++number_of_records > _Df.NumBinRecords)
				DfAddBinaryRecords(1, DF_CURRENT_RECORDS);
			_Df.df_bin_record[_Df.NumBinRecords-1].cart_dim[0] = IntExpression();
			if(Pgm.EqualsCur(",")) {
				Pgm.Shift();
				_Df.df_bin_record[_Df.NumBinRecords-1].cart_dim[1] = IntExpression();
			}
			if(!Pgm.EqualsCur(")"))
				IntErrorCurToken("tuple syntax error");
			Pgm.Shift();
		}
	} while(Pgm.EqualsCur(":"));
}

/* Evaluate a tuple of up to specified dimension. */
#define TUPLE_SEPARATOR_CHAR ":"
#define LEFT_TUPLE_CHAR "("
#define RIGHT_TUPLE_CHAR ")"

//int token2tuple(double * tuple, int dimension)
int GnuPlot::Token2Tuple(double * pTuple, int dimension)
{
	if(Pgm.EqualsCur(LEFT_TUPLE_CHAR)) {
		bool expecting_number = TRUE;
		int N = 0;
		Pgm.Shift();
		while(!Pgm.EndOfCommand()) {
			if(expecting_number) {
				if(++N <= dimension)
					*pTuple++ = RealExpression();
				else
					IntError(Pgm.GetPrevTokenIdx(), "More than %d elements", N);
				expecting_number = FALSE;
			}
			else if(Pgm.EqualsCur(",")) {
				Pgm.Shift();
				expecting_number = TRUE;
			}
			else if(Pgm.EqualsCur(RIGHT_TUPLE_CHAR)) {
				Pgm.Shift();
				return N;
			}
			else
				IntErrorCurToken("Expecting ',' or '" RIGHT_TUPLE_CHAR "'");
		}
	}
	return 0; // Not a tuple 
}
//
// Determine the 2D rotational matrix from the "rotation" qualifier. 
//
//void plot_option_multivalued(df_multivalue_type type, int arg)
void GnuPlot::PlotOptionMultiValued(df_multivalue_type type, int arg)
{
	int bin_record_count = 0;
	int test_val;
	// Require equal symbol. 
	if(!Pgm.EqualsCur("="))
		IntErrorCurToken(equal_symbol_msg);
	Pgm.Shift();
	while(!Pgm.EndOfCommand()) {
		double tuple[3];
		switch(type) {
			case DF_ORIGIN:
			case DF_CENTER:
			case DF_PERPENDICULAR:
			    test_val = Token2Tuple(tuple, sizeof(tuple)/sizeof(tuple[0]));
			    break;
			case DF_SCAN:
			case DF_FLIP:
			    // Will check later 
			    test_val = 1;
			    break;
			default: {
			    // Check if a valid number. 
			    tuple[0] = RealExpression();
			    test_val = 1;
		    }
		}
		if(test_val) {
			char const * cannot_flip_msg = "Cannot flip a non-existent dimension";
			char flip_list[4];
			if(bin_record_count >= _Df.NumBinRecords)
				IntErrorCurToken("More parameters specified than data records specified");
			switch(type) {
				case DF_DELTA:
				    // Set the spacing between grid points in the specified dimension. 
				    *(_Df.df_bin_record[bin_record_count].cart_delta + arg) = tuple[0];
				    if(_Df.df_bin_record[bin_record_count].cart_delta[arg] <= 0)
					    IntError(Pgm.GetCurTokenIdx()-2, "Sample period must be positive. Try `flip` for changing direction");
				    break;
				case DF_FLIP_AXIS:
				    // Set the direction of grid points increment in the specified dimension. 
				    if(_Df.df_bin_record[bin_record_count].cart_dim[0] != 0) {
					    if(tuple[0] == 0.0)
						    _Df.df_bin_record[bin_record_count].cart_dir[arg] = 0;
					    else if(tuple[0] == 1.0)
						    _Df.df_bin_record[bin_record_count].cart_dir[arg] = 1;
					    else
						    IntError(Pgm.GetPrevTokenIdx(), "Flipping dimension direction must be 1 or 0");
				    }
				    else
					    IntErrorCurToken(cannot_flip_msg);
				    break;
				case DF_FLIP:
				    /* Set the direction of grid points increment in
				    * based upon letters for axes. Check if there are
				    * any characters in string that shouldn't be. */
				    Pgm.CopyStr(flip_list, Pgm.GetCurTokenIdx(), 4);
				    if(strlen(flip_list) != strspn(flip_list, "xXyYzZ"))
					    IntErrorCurToken("Can only flip x, y, and/or z");
				    // Check for valid dimensions. 
				    if(strpbrk(flip_list, "xX")) {
					    if(_Df.df_bin_record[bin_record_count].cart_dim[0] != 0)
						    _Df.df_bin_record[bin_record_count].cart_dir[0] = arg;
					    else
						    IntErrorCurToken(cannot_flip_msg);
				    }
				    if(strpbrk(flip_list, "yY")) {
					    if(_Df.df_bin_record[bin_record_count].cart_dim[1] != 0)
						    _Df.df_bin_record[bin_record_count].cart_dir[1] = arg;
					    else
						    IntErrorCurToken(cannot_flip_msg);
				    }
				    if(strpbrk(flip_list, "zZ")) {
					    if(_Df.df_bin_record[bin_record_count].cart_dim[2] != 0)
						    _Df.df_bin_record[bin_record_count].cart_dir[2] = arg;
					    else
						    IntErrorCurToken(cannot_flip_msg);
				    }
				    Pgm.Shift();
				    break;
				case DF_SCAN: {
				    // Set the method in which data is scanned from file.  Compare against a set number of strings.
				    int i;
				    if(!(_Df.df_bin_record[bin_record_count].cart_dim[0] || _Df.df_bin_record[bin_record_count].scan_dim[0]) || 
						!(_Df.df_bin_record[bin_record_count].cart_dim[1] || _Df.df_bin_record[bin_record_count].scan_dim[1]))
					    IntErrorCurToken("Cannot alter scanning method for one-dimensional data");
				    else if(_Df.df_bin_record[bin_record_count].cart_dim[2] || _Df.df_bin_record[bin_record_count].scan_dim[2]) {
					    for(i = 0; i < sizeof(df_bin_scan_table_3D)/sizeof(df_bin_scan_table_3D_struct); i++)
						    if(Pgm.EqualsCur(df_bin_scan_table_3D[i].string)) {
							    memcpy(_Df.df_bin_record[bin_record_count].cart_scan, df_bin_scan_table_3D[i].scan, sizeof(_Df.df_bin_record[0].cart_scan));
							    break;
						    }
					    if(i == sizeof(df_bin_scan_table_3D) / sizeof(df_bin_scan_table_3D_struct))
						    IntErrorCurToken("Improper scanning string. Try 3 character string for 3D data");
				    }
				    else {
					    for(i = 0; i < sizeof(df_bin_scan_table_2D)/sizeof(df_bin_scan_table_2D_struct); i++)
						    if(Pgm.EqualsCur(df_bin_scan_table_2D[i].string)) {
							    memcpy(_Df.df_bin_record[bin_record_count].cart_scan, df_bin_scan_table_2D[i].scan, sizeof(_Df.df_bin_record[0].cart_scan));
							    break;
						    }
					    if(i == sizeof(df_bin_scan_table_2D) / sizeof(df_bin_scan_table_2D_struct))
						    IntErrorCurToken("Improper scanning string. Try 2 character string for 2D data");
				    }
				    // Remove the file supplied scan direction. 
				    memcpy(_Df.df_bin_record[bin_record_count].scan_dir, df_bin_record_reset.scan_dir, sizeof(_Df.df_bin_record[0].scan_dir));
				    Pgm.Shift();
				    break;
			    }
				case DF_SKIP:
				    // Set the number of bytes to skip before reading record. 
				    _Df.df_bin_record[bin_record_count].scan_skip[0] = static_cast<off_t>(tuple[0]);
				    if((_Df.df_bin_record[bin_record_count].scan_skip[0] != tuple[0]) || (_Df.df_bin_record[bin_record_count].scan_skip[0] < 0))
					    IntErrorCurToken("Number of bytes to skip must be positive integer");
				    break;
				case DF_ORIGIN:
				case DF_CENTER:
				    // Set the origin or center of the image based upon the plot mode. 
				    _Df.df_bin_record[bin_record_count].cart_trans = (type == DF_ORIGIN) ? DF_TRANSLATE_VIA_ORIGIN : DF_TRANSLATE_VIA_CENTER;
				    if(arg == MODE_PLOT) {
					    if(test_val != 2)
						    IntErrorCurToken("Two-dimensional tuple required for 2D plot");
					    tuple[2] = 0.0;
				    }
				    else if(arg == MODE_SPLOT) {
					    if(test_val != 3)
						    IntErrorCurToken("Three-dimensional tuple required for 3D plot");
				    }
				    else if(arg == MODE_QUERY) {
					    if(test_val != 3)
						    IntErrorCurToken("Three-dimensional tuple required for setting binary parameters");
				    }
				    else {
					    IntErrorCurToken("Internal error (datafile.c): Unknown plot mode");
				    }
				    memcpy(_Df.df_bin_record[bin_record_count].cart_cen_or_ori, tuple, sizeof(tuple));
				    break;
				case DF_ROTATION:
				    // Allow user to enter angle in terms of pi or degrees. 
				    if(Pgm.EqualsCur("pi")) {
					    tuple[0] *= SMathConst::Pi;
					    Pgm.Shift();
				    }
				    else if(Pgm.AlmostEqualsCur("d$egrees")) {
					    tuple[0] *= SMathConst::PiDiv180;
					    Pgm.Shift();
				    }
				    // Construct 2D rotation matrix. 
				    _Df.df_bin_record[bin_record_count].cart_alpha = tuple[0];
				    break;
				case DF_PERPENDICULAR:
				    // Make sure in three dimensional plotting mode before
				    // accepting the perpendicular vector for translation. 
				    if(test_val != 3)
					    IntErrorCurToken("Three-dimensional tuple required");
				    // Compare vector length against variable precision
				    // to determine if this is the null vector 
				    if((tuple[0]*tuple[0] + tuple[1]*tuple[1] + tuple[2]*tuple[2]) < 100.*DBL_EPSILON)
					    IntErrorCurToken("Perpendicular vector cannot be zero");
				    memcpy(_Df.df_bin_record[bin_record_count].cart_p, tuple, sizeof(tuple));
				    break;
				default:
				    IntError(NO_CARET, "Internal error: Invalid comma separated type");
			}
		}
		else {
			IntErrorCurToken("Invalid numeric or tuple form");
		}
		if(Pgm.EqualsCur(TUPLE_SEPARATOR_CHAR)) {
			bin_record_count++;
			Pgm.Shift();
		}
		else
			break;
	}
}
//
// Set the 'bytes' to skip before column 'col'. 
//
//void df_set_skip_before(int col, int bytes)
void GnuPlot::DfSetSkipBefore(int col, int bytes)
{
	assert(col > 0);
	// Check if we have room at least col columns 
	if(col > _Df.df_max_bininfo_cols) {
		_Df.df_column_bininfo = (df_column_bininfo_struct *)SAlloc::R(_Df.df_column_bininfo, col * sizeof(df_column_bininfo_struct));
		_Df.df_max_bininfo_cols = col;
	}
	_Df.df_column_bininfo[col-1].skip_bytes = bytes;
}
//
// Set the column data type. 
//
//void df_set_read_type(int col, df_data_type type)
void GnuPlot::DfSetReadType(int col, df_data_type type)
{
	assert(col > 0);
	assert(type < DF_BAD_TYPE);
	// Check if we have room at least col columns 
	if(col > _Df.df_max_bininfo_cols) {
		_Df.df_column_bininfo = (df_column_bininfo_struct *)SAlloc::R(_Df.df_column_bininfo, col * sizeof(df_column_bininfo_struct));
		_Df.df_max_bininfo_cols = col;
	}
	_Df.df_column_bininfo[col-1].column.read_type = type;
	_Df.df_column_bininfo[col-1].column.read_size = df_binary_details[type].type.read_size;
}
// 
// If the column number is greater than number of binary columns, set
// the uninitialized columns binary info to that of the last specified
// column or the default if none were set.  
// 
//void df_extend_binary_columns(int no_cols)
void GnuPlot::DfExtendBinaryColumns(int no_cols)
{
	if(no_cols > _Df.df_no_bin_cols) {
		const df_data_type type = (_Df.df_no_bin_cols > 0) ? _Df.df_column_bininfo[_Df.df_no_bin_cols-1].column.read_type : DF_DEFAULT_TYPE;
		for(int i = no_cols; i > _Df.df_no_bin_cols; i--) {
			df_set_skip_after(i, 0);
			DfSetReadType(i, type);
		}
		_Df.df_no_bin_cols = no_cols;
	}
}
// 
// Determine binary data widths from the `using` (or `binary`) format specification. 
// 
//void plot_option_binary_format(char * format_string)
void GnuPlot::PlotOptionBinaryFormat(char * pFormatString)
{
	int prev_read_type = DF_DEFAULT_TYPE; /* Defaults when none specified. */
	int no_fields = 0;
	char * substr = pFormatString;
	while(*substr != '\0' && *substr != '\"' && *substr != '\'') {
		if(*substr == ' ') {
			substr++;
			continue;
		} /* Ignore spaces. */
		if(*substr == '%') {
			int field_repeat, j = 0, k = 0, m = 0, breakout;
			substr++;
			int ignore = (*substr == '*');
			if(ignore)
				substr++;
			// Check for field repeat number. 
			field_repeat = isdigit((uchar)*substr) ? strtol(substr, &substr, 10) : 1;
			// Try finding the word among the valid type names. 
			for(j = 0, breakout = 0; j < (sizeof(df_binary_tables)/sizeof(df_binary_tables[0])); j++) {
				for(k = 0, breakout = 0; k < df_binary_tables[j].group_length; k++) {
					for(m = 0; m < df_binary_tables[j].group[k].no_names; m++) {
						int strl = strlen(df_binary_tables[j].group[k].name[m]);
						// Check for exact match, which includes character
						// after the substring being non-alphanumeric. 
						if(!strncmp(substr, df_binary_tables[j].group[k].name[m], strl) && strchr("%\'\" ", *(substr + strl))) {
							substr += strl; /* Advance pointer in array to next text. */
							if(!ignore) {
								for(int n = 0; n < field_repeat; n++) {
									no_fields++;
									df_set_skip_after(no_fields, 0);
									DfSetReadType(no_fields, df_binary_tables[j].group[k].type.read_type);
									prev_read_type = df_binary_tables[j].group[k].type.read_type;
								}
							}
							else {
								if(!_Df.df_column_bininfo)
									IntError(NO_CARET, "Failure in binary table initialization");
								_Df.df_column_bininfo[no_fields].skip_bytes += field_repeat * df_binary_tables[j].group[k].type.read_size;
							}
							breakout = 1;
							break;
						}
					}
					if(breakout)
						break;
				}
				if(breakout)
					break;
			}
			if(j == (sizeof(df_binary_tables)/sizeof(df_binary_tables[0])) && (k == df_binary_tables[j-1].group_length) && (m == df_binary_tables[j-1].group[k-1].no_names)) {
				IntErrorCurToken("Unrecognized binary format specification");
			}
		}
		else {
			IntErrorCurToken("Format specifier must begin with '%'");
		}
	}
	// Any remaining unspecified fields are assumed to be of the same type
	// as the last specified field.
	for(; no_fields < _Df.df_no_bin_cols; no_fields++) {
		df_set_skip_after(no_fields, 0);
		DfSetSkipBefore(no_fields, 0);
		DfSetReadType(no_fields, (df_data_type)prev_read_type);
	}
	_Df.df_no_bin_cols = no_fields;
}

//void df_show_binary(FILE * fp)
void GnuPlot::DfShowBinary(FILE * fp)
{
	int i, num_record;
	const df_binary_file_record_struct * bin_record;
	fprintf(fp, "\tDefault binary data file settings (in-file settings may override):\n");
	if(!_Df.df_num_bin_records_default) {
		bin_record = &df_bin_record_reset;
		num_record = 1;
	}
	else {
		bin_record = _Df.df_bin_record_default;
		num_record = _Df.df_num_bin_records_default;
	}
	fprintf(fp, "\n\t  File Type: ");
	{
		SString bin_file_type_ext;
		if(SIntToSymbTab_GetSymb(DfBinFileTypeTable, SIZEOFARRAY(DfBinFileTypeTable), _Df.BinFileTypeDefault, bin_file_type_ext))
			fprintf(fp, "%s", bin_file_type_ext.cptr());
		else
			fprintf(fp, "%s", "none");
	}
	fprintf(fp, "\n\t  File Endianess: %s", df_endian[_Df.df_bin_file_endianess_default]);
	fprintf(fp, "\n\t  Default binary format: %s", NZOR(_Df.df_binary_format, "none"));
	for(i = 0; i < num_record; i++) {
		int dimension = 1;
		fprintf(fp, "\n\t  Record %d:\n", i);
		fprintf(fp, "\t    Dimension: ");
		if(bin_record[i].cart_dim[0] < 0)
			fprintf(fp, "Inf");
		else {
			fprintf(fp, "%d", bin_record[i].cart_dim[0]);
			if(bin_record[i].cart_dim[1] > 0) {
				dimension = 2;
				fprintf(fp, "x%d", bin_record[i].cart_dim[1]);
				if(bin_record[i].cart_dim[2] > 0) {
					dimension = 3;
					fprintf(fp, "x%d", bin_record[i].cart_dim[2]);
				}
			}
		}
		fprintf(fp, "\n\t    Generate coordinates: %s", (bin_record[i].scan_generate_coord ? "yes" : "no"));
		if(bin_record[i].scan_generate_coord) {
			int j;
			bool no_flip = TRUE;
			fprintf(fp, "\n\t    Direction: ");
			if(bin_record[i].cart_dir[0] == -1) {
				fprintf(fp, "flip x");
				no_flip = FALSE;
			}
			if((dimension > 1) && (bin_record[i].cart_dir[1] == -1)) {
				fprintf(fp, "%sflip y", (no_flip ? "" : ", "));
				no_flip = FALSE;
			}
			if((dimension > 2) && (bin_record[i].cart_dir[2] == -1)) {
				fprintf(fp, "%sflip z", (no_flip ? "" : ", "));
				no_flip = FALSE;
			}
			if(no_flip)
				fprintf(fp, "all forward");
			fprintf(fp, "\n\t    Sample periods: dx=%f", bin_record[i].cart_delta[0]);
			if(dimension > 1)
				fprintf(fp, ", dy=%f", bin_record[i].cart_delta[1]);
			if(dimension > 2)
				fprintf(fp, ", dz=%f", bin_record[i].cart_delta[2]);
			if(bin_record[i].cart_trans == DF_TRANSLATE_VIA_ORIGIN)
				fprintf(fp, "\n\t    Origin:");
			else if(bin_record[i].cart_trans == DF_TRANSLATE_VIA_CENTER)
				fprintf(fp, "\n\t    Center:");
			if(oneof2(bin_record[i].cart_trans, DF_TRANSLATE_VIA_ORIGIN, DF_TRANSLATE_VIA_CENTER))
				fprintf(fp, " (%f, %f, %f)", bin_record[i].cart_cen_or_ori[0], bin_record[i].cart_cen_or_ori[1], bin_record[i].cart_cen_or_ori[2]);
			fprintf(fp, "\n\t    2D rotation angle: %f", bin_record[i].cart_alpha);
			fprintf(fp, "\n\t    3D normal vector: (%f, %f, %f)", bin_record[i].cart_p[0], bin_record[i].cart_p[1], bin_record[i].cart_p[2]);
			for(j = 0; j < (sizeof(df_bin_scan_table_3D)/sizeof(df_bin_scan_table_3D[0])); j++) {
				if(!strncmp((char *)bin_record[i].cart_scan, (char *)df_bin_scan_table_3D[j].scan, sizeof(bin_record[0].cart_scan))) {
					fprintf(fp, "\n\t    Scan: ");
					fprintf(fp, (bin_record[i].cart_dim[2] ? "%s" : "%2.2s"), df_bin_scan_table_3D[j].string);
					break;
				}
			}
			fprintf(fp, "\n\t    Skip bytes: %lld before record", (int64)bin_record[i].scan_skip[0]);
			if(dimension > 1)
				fprintf(fp, ", %lld before line", (int64)bin_record[i].scan_skip[1]);
			if(dimension > 2)
				fprintf(fp, ", %lld before plane", (int64)bin_record[i].scan_skip[2]);
		}
		fprintf(fp, "\n");
	}
}

void df_show_datasizes(FILE * fp)
{
	int i;
	fprintf(fp, "\tThe following binary data sizes are machine dependent:\n\n\t  name (size in bytes)\n\n");
	for(i = 0; i < SIZEOFARRAY(df_binary_details); i++) {
		fprintf(fp, "\t  ");
		for(int j = 0; j < df_binary_details[i].no_names; j++) {
			fprintf(fp, "\"%s\" ", df_binary_details[i].name[j]);
		}
		fprintf(fp, "(%d)\n", df_binary_details[i].type.read_size);
	}
	fprintf(fp, "\n\\tThe following binary data sizes attempt to be machine independent:\n\n\\t  name (size in bytes)\n\n");
	for(i = 0; i < SIZEOFARRAY(df_binary_details_independent); i++) {
		fprintf(fp, "\t  ");
		for(int j = 0; j < df_binary_details_independent[i].no_names; j++) {
			fprintf(fp, "\"%s\" ", df_binary_details_independent[i].name[j]);
		}
		fprintf(fp, "(%d)", df_binary_details_independent[i].type.read_size);
		if(df_binary_details_independent[i].type.read_type == DF_BAD_TYPE)
			fprintf(fp, " -- processor does not support this size");
		fputc('\n', fp);
	}
}

void df_show_filetypes(FILE * fp)
{
	fprintf(fp, "\tThis version of gnuplot understands the following binary file types:\n");
	for(uint i = 0; i < SIZEOFARRAY(DfBinFileTypeTable); i++) {
		fprintf(fp, "\t  %s", DfBinFileTypeTable[i].P_Symb);
	}
	/*for(int i = 0; df_bin_filetype_table[i].key;) fprintf(fp, "\t  %s", df_bin_filetype_table[i++].key);*/
	fputs("\n", fp);
}

//static int df_skip_bytes(off_t nbytes)
int GnuPlot::DfSkipBytes(off_t nbytes)
{
#if defined(PIPES)
	char cval;
	if(_Df.df_pipe_open || plotted_data_from_stdin) {
		while(nbytes--) {
			if(1 == fread(&cval, 1, 1, data_fp))
				continue;
			if(feof(data_fp)) {
				_Df.df_eof = 1;
				return DF_EOF;
			}
			IntError(NO_CARET, read_error_msg);
		}
	}
	else
#endif
	if(fseek(_Df.data_fp, nbytes, SEEK_CUR)) {
		if(feof(_Df.data_fp)) {
			_Df.df_eof = 1;
			return DF_EOF;
		}
		IntError(NO_CARET, read_error_msg);
	}
	return 0;
}

/*{{{  int df_readbinary(v, max) */
/* do the hard work... read lines from file,
 * - use blanks to get index number
 * - ignore lines outside range of indices required
 * - fill v[] based on using spec if given
 */

//int df_readbinary(double v[], int max)
int GnuPlot::DfReadBinary(double v[], int maxSize)
{
	// For general column structured binary. 
	static int scan_size[3];
	static double delta[3]; /* sampling periods */
	static double o[3]; /* add after rotations */
	static double c[3]; /* subtract before doing rotations */
	static double P[3][3]; /* 3D rotation matrix (perpendicular) */
	static double R[2][2]; /* 2D rotation matrix (rotate) */
	static int read_order;
	static off_t record_skip;
	static bool end_of_scan_line;
	static bool end_of_block;
	static bool translation_required;
	static char * memory_data;
	/* For matrix data structure (i.e., gnuplot binary). */
	static double first_matrix_column;
	static double * scanned_matrix_row = 0;
	static int first_matrix_row_col_count;
	bool saved_first_matrix_column = FALSE;
	assert(maxSize <= MAXDATACOLS);
	assert(_Df.df_max_bininfo_cols > _Df.df_no_bin_cols);
	assert(_Df.df_no_bin_cols);
	// catch attempt to read past EOF on mixed-input 
	if(_Df.df_eof)
		return DF_EOF;
	// Check if we have room for at least df_no_bin_cols columns 
	if(_Df.df_max_cols < _Df.df_no_bin_cols)
		_Df.ExpandColumn(_Df.df_no_bin_cols);
	// In binary mode, the number of user specs was increased by the
	// number of dimensions in the underlying uniformly sampled grid
	// previously.  Fill in those values.  Also, compute elements of
	// formula x' = P*R*(x - c) + o 
	if(!_Df.df_M_count && !_Df.df_N_count && !_Df.df_O_count) {
		int i;
		bool D2, D3;
		df_binary_file_record_struct * this_record = _Df.df_bin_record + _Df.df_bin_record_count;
		scan_size[0] = scan_size[1] = scan_size[2] = 0;
		D2 = rotation_matrix_2D(R, this_record->cart_alpha);
		D3 = rotation_matrix_3D(P, this_record->cart_p);
		translation_required = D2 || D3;
		if(_Df.df_matrix_file) {
			// Dimensions 
			scan_size[0] = this_record->scan_dim[0];
			scan_size[1] = this_record->scan_dim[1];
			if(_Df.df_xpixels == 0) {
				/* df_xpixels and df_ypixels were corrected for ascii `matrix every`
				 * but scan_size was not. For that case we must not overwrite here.
				 * For binary matrix, df_xpixels is still 0.
				 */
				FPRINTF((stderr, "datafile.c:%d matrix dimensions %d x %d\n", __LINE__, scan_size[1], scan_size[0]));
				_Df.df_xpixels = scan_size[1];
				_Df.df_ypixels = scan_size[0];
			}
			if(scan_size[0] == 0)
				IntError(NO_CARET, "Scan size of matrix is zero");
			// To accomplish flipping in this case, multiply the
			// appropriate column of the rotation matrix by -1.  
			for(i = 0; i < 2; i++) {
				for(int j = 0; j < 2; j++) {
					R[i][j] *= this_record->cart_dir[i];
				}
			}
			// o 
			for(i = 0; i < 3; i++) {
				if(this_record->cart_trans != DF_TRANSLATE_DEFAULT)
					o[i] = this_record->cart_cen_or_ori[i];
				else if(i < 2) // Default is translate by center. 
					o[i] = (df_matrix_corner[1][i] + df_matrix_corner[0][i]) / 2;
				else
					o[i] = 0.0;
			}
			// c 
			for(i = 0; i < 3; i++) {
				if(this_record->cart_trans == DF_TRANSLATE_VIA_ORIGIN)
					c[i] = (i < 2) ? df_matrix_corner[0][i] : 0.0;
				else
					c[i] = (i < 2) ? ((df_matrix_corner[1][i] + df_matrix_corner[0][i]) / 2) : 0.0;
			}
			first_matrix_row_col_count = 0;
		}
		else { // general binary 
			for(i = 0; i < 3; i++) {
				// How to direct the generated coordinates in regard to scan direction 
				if(this_record->cart_dim[i] || this_record->scan_dim[i]) {
					if(this_record->scan_generate_coord)
						_Df.UseSpec[i].column = this_record->cart_scan[i];
				}
				// Dimensions 
				const int map = DF_SCAN_POINT - this_record->cart_scan[i];
				if(this_record->cart_dim[i] > 0)
					scan_size[map] = this_record->cart_dim[i];
				else if(this_record->cart_dim[i] < 0)
					scan_size[map] = MAXINT;
				else
					scan_size[map] = this_record->scan_dim[map];
				// Sample periods 
				delta[map] = (this_record->cart_delta[i]) ? this_record->cart_delta[i] : this_record->scan_delta[map];
				delta[map] *= this_record->scan_dir[map] * this_record->cart_dir[i];
				// o 
				if(this_record->cart_trans != DF_TRANSLATE_DEFAULT)
					o[i] = this_record->cart_cen_or_ori[i];
				else if(this_record->scan_trans != DF_TRANSLATE_DEFAULT)
					o[i] = this_record->scan_cen_or_ori[map];
				else if(scan_size[map] > 0)
					o[i] = (scan_size[map] - 1)*fabs(delta[map])/2;
				else
					o[i] = 0;
				// c 
				if(this_record->cart_trans == DF_TRANSLATE_VIA_ORIGIN || (this_record->cart_trans == DF_TRANSLATE_DEFAULT && this_record->scan_trans == DF_TRANSLATE_VIA_ORIGIN))
					c[i] = (scan_size[map] > 0 && delta[map] < 0.0) ? ((scan_size[map] - 1) * delta[map]) : 0.0;
				else
					c[i] = (scan_size[map] > 0) ? ((scan_size[map] - 1) * (delta[map]/2.0)) : 0.0;
			}
		}
		// Check if c and o are the same. 
		for(i = 0; i < 3; i++)
			translation_required = translation_required || (c[i] != o[i]);
		memory_data = this_record->memory_data; // Should data come from memory? 
		read_order = byte_read_order(_Df.BinFileEndianess); // byte read order 
		record_skip = this_record->scan_skip[0]; // amount to skip before first record 
		end_of_scan_line = FALSE;
		end_of_block = FALSE;
		_Df.PointCount = -1;
		_Df.LineCount = 0;
		_Df.df_current_index = _Df.df_bin_record_count;
		_Df.df_last_index_read = _Df.df_current_index;
		// Craig DeForest Feb 2013 - Fast version of uniform binary matrix.
		// Don't apply this to ascii input or special filetypes.
		// Slurp all data from file or pipe in one shot to minimize fread calls.
		if(!memory_data && !_Df.BinFileType && _Df.df_binary_file && _Df.df_matrix && !_Df.df_nonuniform_matrix) {
			ulong bytes_per_point = 0;
			ulong bytes_per_line = 0;
			ulong bytes_per_plane = 0;
			ulong bytes_total = 0;
			size_t fread_ret;
			// Accumulate total number of bytes in this tuple 
			for(int i = 0; i < _Df.df_no_bin_cols; i++)
				bytes_per_point += _Df.df_column_bininfo[i].skip_bytes + _Df.df_column_bininfo[i].column.read_size;
			bytes_per_point += _Df.df_column_bininfo[_Df.df_no_bin_cols].skip_bytes;
			bytes_per_line  = bytes_per_point * ((scan_size[0] > 0) ? scan_size[0] : 1);
			bytes_per_plane = bytes_per_line * ((scan_size[1] > 0) ? scan_size[1] : 1);
			bytes_total     = bytes_per_plane * ((scan_size[2]>0) ? scan_size[2] : 1);
			bytes_total    += static_cast<int>(record_skip);
			// Allocate a chunk of memory and stuff it 
			memory_data = (char *)SAlloc::M(bytes_total);
			this_record->memory_data = memory_data;
			FPRINTF((stderr, "Fast matrix code:\n"));
			FPRINTF((stderr, "\t\t skip %ld bytes, read %ld bytes per point %ld total as %d x %d array\n", record_skip, bytes_per_point, bytes_total, scan_size[0], scan_size[1]));
			// Do the actual slurping 
			fread_ret = fread(memory_data, 1, bytes_total, _Df.data_fp);
			if(fread_ret != bytes_total) {
				IntWarn(NO_CARET, "Couldn't slurp %ld bytes (return was %zd)\n", bytes_total, fread_ret);
				_Df.df_eof = 1;
				return DF_EOF;
			}
		}
	}
	while(!_Df.df_eof) {
		/*{{{  process line */
		bool line_okay = TRUE;
		int output = 0; /* how many numbers written to v[] */
		int i;
		int fread_ret = 0;
		int m_value;
		int n_value;
		int o_value;
		union io_val {
			char ch;
			uchar uc;
			short sh;
			ushort us;
			int in;
			uint ui;
			long lo;
			ulong ul;
			int64  llo;
			uint64 ull;
			float fl;
			double db;
		} io_val;

		/* Scan in a number of floats based upon the largest index in
		 * the use_specs array.  If the largest index in the array is
		 * greater than maximum columns then issue an error.
		 */

		/* Handle end of line or end of block on previous read. */
		if(end_of_scan_line) {
			end_of_scan_line = FALSE;
			_Df.PointCount = -1;
			_Df.LineCount++;
			return DF_FIRST_BLANK;
		}
		if(end_of_block) {
			end_of_block = FALSE;
			_Df.LineCount = 0;
			return DF_SECOND_BLANK;
		}
		// Possibly skip bytes before starting to read record. 
		if(record_skip) {
			if(memory_data)
				memory_data += record_skip;
			else if(DfSkipBytes(record_skip))
				return DF_EOF;
			record_skip = 0;
		}
		/* Bring in variables as described by the field parameters.
		 * If less than than the appropriate number of bytes have been
		 * read, issue an error stating not enough columns were found.  */
		for(i = 0;; i++) {
			off_t skip_bytes = _Df.df_column_bininfo[i].skip_bytes;
			if(skip_bytes) {
				if(memory_data)
					memory_data += skip_bytes;
				else if(DfSkipBytes(skip_bytes))
					return DF_EOF;
			}
			// Last entry only has skip bytes, no data.
			if(i == _Df.df_no_bin_cols)
				break;
			// Read in a "column", i.e., a binary value of various types.
			if(_Df.df_pixeldata) {
				io_val.uc = df_libgd_get_pixel(_Df.df_M_count, _Df.df_N_count, i);
			}
			else if(memory_data) {
				for(fread_ret = 0; fread_ret < _Df.df_column_bininfo[i].column.read_size; fread_ret++)
					(&io_val.ch)[fread_ret] = *memory_data++;
			}
			else {
				fread_ret = fread(&io_val.ch, _Df.df_column_bininfo[i].column.read_size, 1, _Df.data_fp);
				if(fread_ret != 1) {
					_Df.df_eof = 1;
					return DF_EOF;
				}
			}
			if(read_order != 0)
				df_swap_bytes_by_endianess(&io_val.ch, read_order, _Df.df_column_bininfo[i].column.read_size);
			switch(_Df.df_column_bininfo[i].column.read_type) {
				case DF_CHAR: _Df.df_column[i].datum = io_val.ch; break;
				case DF_UCHAR: _Df.df_column[i].datum = io_val.uc; break;
				case DF_SHORT: _Df.df_column[i].datum = io_val.sh; break;
				case DF_USHORT: _Df.df_column[i].datum = io_val.us; break;
				case DF_INT: _Df.df_column[i].datum = io_val.in; break;
				case DF_UINT: _Df.df_column[i].datum = io_val.ui; break;
				case DF_LONG: _Df.df_column[i].datum = io_val.lo; break;
				case DF_ULONG: _Df.df_column[i].datum = io_val.ul; break;
				case DF_LONGLONG: _Df.df_column[i].datum = static_cast<double>(io_val.llo); break;
				case DF_ULONGLONG: _Df.df_column[i].datum = static_cast<double>(io_val.ull); break;
				case DF_FLOAT: _Df.df_column[i].datum = io_val.fl; break;
				case DF_DOUBLE: _Df.df_column[i].datum = io_val.db; break;
				default: IntError(NO_CARET, "Binary data type unknown");
			}
			_Df.df_column[i].good = DF_GOOD;
			_Df.df_column[i].position = NULL; /* cant get a time */
			// Matrix file data is a special case. After reading in just one binary value, stop and decide on what to do with it. 
			if(_Df.df_matrix_file)
				break;
		} /* for(i) */
		if(_Df.df_matrix_file) {
			if(_Df.df_nonuniform_matrix) {
				// Store just first column? 
				if(!_Df.df_M_count && !saved_first_matrix_column) {
					first_matrix_column = _Df.df_column[i].datum;
					saved_first_matrix_column = TRUE;
					continue;
				}
				// Read reset of first row? 
				if(!_Df.df_M_count && !_Df.df_N_count && !_Df.df_O_count && first_matrix_row_col_count < scan_size[0]) {
					if(!first_matrix_row_col_count)
						scanned_matrix_row = (double *)SAlloc::R(scanned_matrix_row, scan_size[0]*sizeof(double));
					scanned_matrix_row[first_matrix_row_col_count] = _Df.df_column[i].datum;
					first_matrix_row_col_count++;
					if(first_matrix_row_col_count == scan_size[0])
						saved_first_matrix_column = false; // Start of the second row
					continue;
				}
			}
			/* Update all the binary columns.  Matrix binary and
			 * matrix ASCII is a slight abuse of notation.  At the
			 * command line, 1 means first row, 2 means first
			 * column.  There can only be one column of data input
			 * because it is a matrix of data, not columns.  */
			{
				_Df.df_datum = static_cast<int>(_Df.df_column[i].datum);
				// Fill backward so that current read value is not overwritten. 
				for(int j = _Df.df_no_bin_cols-1; j >= 0; j--) {
					if(j == 0)
						_Df.df_column[j].datum = _Df.df_nonuniform_matrix ? scanned_matrix_row[_Df.df_M_count] : _Df.df_M_count;
					else if(j == 1)
						_Df.df_column[j].datum = _Df.df_nonuniform_matrix ? first_matrix_column : _Df.df_N_count;
					else
						_Df.df_column[j].datum = _Df.df_column[i].datum;
					_Df.df_column[j].good = DF_GOOD;
					_Df.df_column[j].position = NULL;
				}
			}
		}
		else { // Not matrix file, general binary. 
			_Df.df_datum = _Df.PointCount + 1;
			if(i != _Df.df_no_bin_cols) {
				if(feof(_Df.data_fp)) {
					if(i != 0)
						IntError(NO_CARET, "Last point in the binary file did not match the specified `using` columns");
					_Df.df_eof = 1;
					return DF_EOF;
				}
				else
					IntError(NO_CARET, read_error_msg);
			}
		}
		m_value = _Df.df_M_count;
		n_value = _Df.df_N_count;
		o_value = _Df.df_O_count;
		_Df.df_M_count++;
		if((scan_size[0] > 0) && (_Df.df_M_count >= scan_size[0])) {
			// This is a new "line". 
			_Df.df_M_count = 0;
			_Df.df_N_count++;
			end_of_scan_line = TRUE;
			if((scan_size[1] >= 0) && (_Df.df_N_count >= scan_size[1])) {
				// This is a "block". 
				_Df.df_N_count = 0;
				_Df.df_O_count++;
				if((scan_size[2] >= 0) && (_Df.df_O_count >= scan_size[2])) {
					_Df.df_O_count = 0;
					end_of_block = TRUE;
					if(++_Df.df_bin_record_count >= _Df.NumBinRecords) {
						_Df.df_eof = 1;
					}
				}
			}
		}
		/*{{{  ignore points outside range of index */
		/* we try to return end-of-file as soon as we pass upper
		 * index, but for mixed input stream, we must skip garbage */
		if(_Df.df_current_index < _Df.df_lower_index || _Df.df_current_index > _Df.df_upper_index || ((_Df.df_current_index - _Df.df_lower_index) % _Df.df_index_step) != 0)
			continue;
		/*}}} */
		/*{{{  reject points by every */
		// accept only lines with (line_count%everyline) == 0
		if(_Df.LineCount < _Df.firstline || _Df.LineCount > _Df.lastline || (_Df.LineCount - _Df.firstline) % _Df.everyline != 0)
			continue;
		// update point_count. ignore point if point_count%everypoint != 0 
		if(++_Df.PointCount < _Df.FirstPoint || _Df.PointCount > _Df.LastPoint || (_Df.PointCount - _Df.FirstPoint) % _Df.EveryPoint != 0)
			continue;
		/*}}} */
		/* At this point the binary columns have been read
		 * successfully.  Set df_no_cols to df_no_bin_cols for use
		 * in the interpretation code.  */
		_Df.df_no_cols = _Df.df_no_bin_cols;
		/*{{{  copy column[] to v[] via use[] */
		{
			int limit = (_Df.df_no_use_specs ? _Df.df_no_use_specs : MAXDATACOLS);
			SETMIN(limit, maxSize);
			for(output = 0; output < limit; ++output) {
				int column = _Df.UseSpec[output].column;
				// if there was no using spec, column is output+1 and at=NULL 
				if(_Df.UseSpec[output].at) {
					GpValue a;
					// no dummy values to set up prior to... 
					_Df.evaluate_inside_using = true;
					EvaluateAt(_Df.UseSpec[output].at, &a);
					_Df.evaluate_inside_using = false;
					if(Ev.IsUndefined_) {
						v[output] = fgetnan();
						return DF_UNDEFINED;
					}
					if(a.Type == STRING) {
						v[output] = fgetnan(); /* found a string, not a number */
						if(_Df.UseSpec[output].expected_type == CT_STRING) {
							char * s = (char *)SAlloc::M(strlen(a.v.string_val)+3);
							*s = '"';
							strcpy(s+1, a.v.string_val);
							strcat(s, "\"");
							SAlloc::F(_Df.df_stringexpression[output]);
							_Df.df_tokens[output] = _Df.df_stringexpression[output] = s;
						}
						// Expecting a numerical type but got a string value 
						else if(_Df.df_current_plot && (_Df.df_current_plot->lp_properties.PtType == PT_VARIABLE)) {
							static char varchar[8];
							strnzcpy(varchar, a.v.string_val, 8);
							_Df.df_tokens[output] = varchar;
						}
						gpfree_string(&a);
						continue; /* otherwise isnan(v[output]) would terminate */
					}
					else if(a.Type == CMPLX && (fabs(Imag(&a)) > Gg.Zero)) {
						// June 2018: CHANGE. For consistency with function plots, 
						// imaginary results are treated as UNDEFINED.		   
						v[output] = fgetnan();
						return DF_UNDEFINED;
					}
					else {
						v[output] = Real(&a);
					}
				}
				else if(column == DF_SCAN_PLANE) {
					if((_Df.df_current_plot->plot_style == IMAGE) || (_Df.df_current_plot->plot_style == RGBIMAGE))
						v[output] = o_value*delta[2];
					/* EAM August 2009
					 * This was supposed to be "z" in a 3D grid holding a binary
					 * value at each voxel.  But in fact the binary code does not
					 * support 3D grids, only 2D. So this always got set to 0,
					 * making the whole thing pretty useless except for inherently.
					 * planar objects like 2D images.
					 * Now I set Z to be the pixel value, which allows you
					 * to draw surfaces described by a 2D binary array.
					 */
					else
						v[output] = _Df.df_column[0].datum;
				}
				else if(column == DF_SCAN_LINE) {
					v[output] = n_value*delta[1];
				}
				else if(column == DF_SCAN_POINT) {
					v[output] = m_value*delta[0];
				}
				else if(column == -2) {
					v[output] = _Df.df_current_index;
				}
				else if(column == -1) {
					v[output] = _Df.LineCount;
				}
				else if(column == 0) {
					v[output] = _Df.df_datum;
				}
				else if(column <= 0) {
					IntError(NO_CARET, "internal error: unknown column type");
					/* July 2010 - We used to have special code to handle time data. */
					/* But time data in a binary file is just one more binary value, */
					/* so let the general case code handle it.                       */
				}
				else if((column <= _Df.df_no_cols) && _Df.df_column[column-1].good == DF_GOOD)
					v[output] = _Df.df_column[column-1].datum;

				/* EAM - Oct 2002 Distinguish between DF_MISSING
				 * and DF_BAD.  Previous versions would never
				 * notify caller of either case.  Now missing data
				 * will be noted. Bad data should arguably be
				 * noted also, but that would change existing
				 * default behavior.  */
				else if((column <= _Df.df_no_cols) && (_Df.df_column[column-1].good == DF_MISSING))
					return DF_MISSING;
				else {
					// line bad only if user explicitly asked for this column 
					if(_Df.df_no_use_specs)
						line_okay = FALSE;
					break; // return or ignore depending on line_okay 
				}
				if(isnan(v[output])) {
					// EAM April 2012 - return, continue, or ignore??? 
					FPRINTF((stderr, "NaN input value"));
					if(!_Df.df_matrix)
						return DF_UNDEFINED;
				}
			}
			/* Linear translation. */
			if(translation_required) {
				double z;
				double x = v[0] - c[0];
				double y = v[1] - c[1];
				v[0] = R[0][0] * x + R[0][1] * y;
				v[1] = R[1][0] * x + R[1][1] * y;
				if(_Df.df_plot_mode == MODE_SPLOT) {
					x = v[0];
					y = v[1];
					z = v[2] - c[2];
					v[0] = P[0][0] * x + P[0][1] * y + P[0][2] * z;
					v[1] = P[1][0] * x + P[1][1] * y + P[1][2] * z;
					v[2] = P[2][0] * x + P[2][1] * y + P[2][2] * z;
				}
				v[0] += o[0];
				v[1] += o[1];
				if(_Df.df_plot_mode == MODE_SPLOT)
					v[2] += o[2];
			}
		}
		/*}}} */
		if(!line_okay)
			continue;
		for(i = _Df.df_no_use_specs; i < _Df.df_no_use_specs+_Df.df_no_tic_specs; i++) {
			if(_Df.UseSpec[i].expected_type >= CT_XTICLABEL && _Df.UseSpec[i].at) {
				GpValue a;
				int axis, axcol;
				_Df.evaluate_inside_using = true;
				EvaluateAt(_Df.UseSpec[i].at, &a);
				_Df.evaluate_inside_using = false;
				if(a.Type == STRING) {
					axcol = AxColForTicLabel((COLUMN_TYPE)_Df.UseSpec[i].expected_type, &axis);
					AddTicUser(&AxS[axis], a.v.string_val, v[axcol], -1);
					gpfree_string(&a);
				}
			}
		}
		return output;
	}
	/*}}} */
	_Df.df_eof = 1;
	return DF_EOF;
}

//void df_set_plot_mode(int mode)
void GnuPlot::DfSetPlotMode(int mode)
{
	_Df.df_plot_mode = mode;
}
// 
// Special pseudofile '+' returns x coord of sample for 2D plots,
// Special pseudofile '++' returns x and y coordinates of grid for 3D plots.
//
//static char * df_generate_pseudodata()
char * GnuPlot::DfGeneratePseudodata()
{
	// Pseudofile '+' returns a set of (samples) x coordinates */
	// This code copied from that in second pass through eval_plots() */
	if(_Df.df_pseudodata == 1) {
		static double t;
		static RealRange t_range;
		//static double t_min;
		//static double t_max;
		static double t_step;
		if(_Df.df_pseudorecord == 0) {
			t_step = 0;
			if(AxS[SAMPLE_AXIS].range_flags & RANGE_SAMPLED) {
				t_range.Set(AxS[SAMPLE_AXIS].min, AxS[SAMPLE_AXIS].max);
				t_step = AxS[SAMPLE_AXIS].SAMPLE_INTERVAL;
			}
			else if(Gg.Parametric || Gg.Polar) {
				t_range.Set(AxS[T_AXIS].min, AxS[T_AXIS].max);
			}
			else if(AxS[T_AXIS].autoscale == AUTOSCALE_NONE) {
				t_range.Set(AxS[T_AXIS].min, AxS[T_AXIS].max);
			}
			else {
				if(AxS[FIRST_X_AXIS].max == -VERYLARGE)
					AxS[FIRST_X_AXIS].max = 10;
				if(AxS[FIRST_X_AXIS].min == VERYLARGE)
					AxS[FIRST_X_AXIS].min = -10;
				t_range.Set(AxS.__X().min, AxS.__X().max);
			}
			if(AxS[SAMPLE_AXIS].range_flags & RANGE_SAMPLED) {
				; // Nothing special 
			}
			else {
				// If the axis is nonlinear we do the sampling on the primary   
				// (hidden) axis so that the samples are evenly spaced.         
				// The extra test allows sampling on x2 after "set link x2"     
				// NB: This means "t" is in the hidden linear coordinate space. 
				if(AxS.__X().linked_to_primary && AxS.__X().link_udf->at && AxS.__X().linked_to_primary != &AxS[FIRST_X_AXIS]) {
					const GpAxis * primary = AxS.__X().linked_to_primary;
					t_range.Set(EvalLinkFunction(primary, t_range.low), EvalLinkFunction(primary, t_range.upp));
				}
				else
					CheckAxisLogLimits(&AxS.__X(), t_range.low, t_range.upp);
			}
			if(t_step == 0.0) // always true unless explicit sample interval was given 
				t_step = t_range.GetDistance() / (Gg.Samples1 - 1);
			if(t_step == 0.0) // prevent infinite loop on zero range 
				t_step = 1.0;
		}
		t = t_range.low + _Df.df_pseudorecord * t_step;
		if((AxS[SAMPLE_AXIS].range_flags & RANGE_SAMPLED)) {
			// This is the case of an explicit sampling range 
			// @v11.3.7 if(!inrange(t, t_min, t_max)) {
			if(!t_range.CheckX(t)) { // @v11.3.7 
				return NULL; 
			}
		}
		else {
			// This is the usual case 
			if(_Df.df_pseudorecord >= Gg.Samples1)
				return NULL;
			if(AxS.__X().IsNonLinear()) {
				const GpAxis * visible = AxS.__X().linked_to_primary->linked_to_secondary;
				t = EvalLinkFunction(visible, t);
			}
		}
		// This allows commands of the form
		// plot sample [foo=0:10] '+' using (sin(foo)):(cos(foo)):(foo)
		if(_Df.df_current_plot && _Df.df_current_plot->sample_var)
			Gcomplex(&(_Df.df_current_plot->sample_var->udv_value), t, 0.0);
		_Df.df_pseudovalue_0 = t;
		sprintf(_Df.df_line, "%g", t);
		++_Df.df_pseudorecord;
	}
	// Pseudofile '++' returns a (samples X isosamples) grid of x,y coordinates 
	// This code copied from that in second pass through eval_3dplots 
	if(_Df.df_pseudodata == 2) {
		static RealRange u_range;
		static RealRange v_range;
		//static double u_min;
		//static double u_max;
		static double u_step;
		//static double v_min;
		//static double v_max;
		static double v_isostep;
		static int    nusteps;
		static int    nvsteps;
		double u, v;
		// (March 2017) THIS IS A CHANGE
		// Sample on u and v rather than on x and y.
		// This decouples the sampling range from the plot range.
		// Allow explicit sampling interval in the range specifiers for u and v.
		AXIS_INDEX u_axis = U_AXIS;
		AXIS_INDEX v_axis = V_AXIS;
		// Fill in the static variables only once per plot 
		if(_Df.df_pseudospan == 0 && _Df.df_pseudorecord == 0) {
			if(Gg.Samples1 < 2 || Gg.Samples2 < 2 || Gg.IsoSamples1 < 2 || Gg.IsoSamples2 < 2)
				IntError(NO_CARET, "samples or iso_samples < 2. Must be at least 2.");
			if(Gg.Parametric) {
				u_range.Set(AxS[U_AXIS].min, AxS[U_AXIS].max);
				v_range.Set(AxS[V_AXIS].min, AxS[V_AXIS].max);
			}
			else {
				AxisCheckedExtendEmptyRange(u_axis, "u range is invalid");
				AxisCheckedExtendEmptyRange(v_axis, "v range is invalid");
				if(AxS[u_axis].IsNonLinear()) {
					u_range.Set(AxS[u_axis].linked_to_primary->min, AxS[u_axis].linked_to_primary->max);
				}
				else {
					u_range.Set(AxS[u_axis].min, AxS[u_axis].max);
				}
				if(AxS[v_axis].IsNonLinear()) {
					v_range.Set(AxS[v_axis].linked_to_primary->min, AxS[v_axis].linked_to_primary->max);
				}
				else {
					v_range.Set(AxS[v_axis].min, AxS[v_axis].max);
				}
			}
			if((AxS[u_axis].range_flags & RANGE_SAMPLED) && (AxS[u_axis].SAMPLE_INTERVAL != 0)) {
				u_step = AxS[u_axis].SAMPLE_INTERVAL;
				nusteps = ffloori(u_range.GetDistance() / u_step) + 1;
			}
			else if(_3DBlk.hidden3d) {
				u_step = u_range.GetDistance() / (Gg.IsoSamples1 - 1);
				nusteps = Gg.IsoSamples1;
			}
			else {
				u_step = u_range.GetDistance() / (Gg.Samples1 - 1);
				nusteps = Gg.Samples1;
			}
			if((AxS[v_axis].range_flags & RANGE_SAMPLED) && (AxS[v_axis].SAMPLE_INTERVAL != 0)) {
				v_isostep = AxS[v_axis].SAMPLE_INTERVAL;
				nvsteps = ffloori(v_range.GetDistance() / v_isostep) + 1;
			}
			else {
				v_isostep = v_range.GetDistance() / (Gg.IsoSamples2 - 1);
				nvsteps = Gg.IsoSamples2;
			}
		}
		// wrap at end of each line 
		if(_Df.df_pseudorecord >= nusteps) {
			_Df.df_pseudorecord = 0;
			if(++_Df.df_pseudospan >= nvsteps)
				return NULL;
			else
				return const_cast<char *>(""); // @badcast blank record for end of scan line 
		}
		// Duplicate algorithm from calculate_set_of_isolines() 
		u = u_range.low + _Df.df_pseudorecord * u_step;
		v = v_range.upp - _Df.df_pseudospan * v_isostep;
		// Round-off error is most visible at the border 
		if(_Df.df_pseudorecord == nusteps-1)
			u = u_range.upp;
		if(_Df.df_pseudospan == nvsteps-1)
			v = v_range.low;
		if(Gg.Parametric) {
			_Df.df_pseudovalue_0 = u;
			_Df.df_pseudovalue_1 = v;
		}
		else {
			_Df.df_pseudovalue_0 = AxS[u_axis].IsNonLinear() ? EvalLinkFunction(&AxS[u_axis], u) : u;
			_Df.df_pseudovalue_1 = AxS[v_axis].IsNonLinear() ? EvalLinkFunction(&AxS[v_axis], v) : v;
		}
		sprintf(_Df.df_line, "%g %g", _Df.df_pseudovalue_0, _Df.df_pseudovalue_1);
		++_Df.df_pseudorecord;
		// This allows commands of the form
		//   splot sample [foo=0:10][baz=44:55] '++' using (foo):(baz):(foo*baz)
		if(_Df.df_current_plot && _Df.df_current_plot->sample_var)
			Gcomplex(&(_Df.df_current_plot->sample_var->udv_value), _Df.df_pseudovalue_0, 0.0);
		if(_Df.df_current_plot && _Df.df_current_plot->sample_var2)
			Gcomplex(&(_Df.df_current_plot->sample_var2->udv_value), _Df.df_pseudovalue_1, 0.0);
	}
	return _Df.df_line;
}
// 
// The main loop in df_readascii wants a string to process.
// We generate one from the current array entry containing
// column 1: array index
// column 2: array value (real component)
// column 3: array value (imaginary component)
// 
//static char * df_generate_ascii_array_entry()
char * GnuPlot::DfGenerateAsciiArrayEntry()
{
	_Df.df_array_index++;
	if(_Df.df_array_index > _Pb.df_array->udv_value.v.value_array[0].v.int_val)
		return NULL;
	else {
		const GpValue * entry = &(_Pb.df_array->udv_value.v.value_array[_Df.df_array_index]);
		if(entry->Type == STRING) {
			while(_Df.MaxLineLen < strlen(entry->v.string_val))
				_Df.df_line = (char *)SAlloc::R(_Df.df_line, _Df.MaxLineLen *= 2);
			snprintf(_Df.df_line, _Df.MaxLineLen-1, "%d \"%s\"", _Df.df_array_index, entry->v.string_val);
		}
		else
			snprintf(_Df.df_line, _Df.MaxLineLen-1, "%d %g %g", _Df.df_array_index, Real(entry), Imag(entry));
		return _Df.df_line;
	}
}
//
// utility routine shared by df_readascii and df_readbinary 
//
//static int axcol_for_ticlabel(enum COLUMN_TYPE type, int * axis)
int GnuPlot::AxColForTicLabel(enum COLUMN_TYPE type, int * axis)
{
	int axcol;
	switch(type) {
		default:
		case CT_XTICLABEL:
		    *axis = FIRST_X_AXIS;
		    axcol = 0;
		    break;
		case CT_X2TICLABEL:
		    *axis = SECOND_X_AXIS;
		    axcol = 0;
		    break;
		case CT_YTICLABEL:
		    *axis = FIRST_Y_AXIS;
		    axcol = 1;
		    break;
		case CT_Y2TICLABEL:
		    *axis = SECOND_Y_AXIS;
		    axcol = 1;
		    break;
		case CT_ZTICLABEL:
		    *axis = FIRST_Z_AXIS;
		    axcol = 2;
		    break;
		case CT_CBTICLABEL:
		    *axis = COLOR_AXIS;
		    axcol = (_Df.df_axis[2] == FIRST_Z_AXIS) ? 2 : (_Df.df_no_use_specs-1);
		    break;
	}
	return axcol;
}
