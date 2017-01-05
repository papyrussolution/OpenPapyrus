/* GNUPLOT - datafile.c */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
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
   ]*/

/* AUTHOR : David Denholm */

/*
 * this file provides the functions to handle data-file reading..
 * takes care of all the pipe / stdin / index / using worries
 */

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
 *      parses thru / index / using on command line
 *      max_using is max no of 'using' columns allowed (obsolete?)
 *	plot_header is NULL if called from fit or set_palette code
 *      returns number of 'using' cols specified, or -1 on error (?)
 *
 *   int df_readline(double vector[], int max)
 *      reads a line, does all the 'index' and 'using' manipulation
 *      deposits values into vector[]
 *      returns
 *          number of columns parsed  [0 = not a blank line, but no valid data],
 *          DF_EOF - end of file
 *          DF_UNDEFINED - undefined result during eval of extended using spec
 *          DF_MISSING - requested column matched that of 'set missing <foo>'
 *          DF_FIRST_BLANK - first consecutive blank line
 *          DF_SECOND_BLANK - second consecutive blank line
 *          DF_FOUND_KEY_TITLE  - only relevant to first line of data
 *          DF_KEY_TITLE_MISSING  and only for 'set key autotitle columnhead'
 *          DF_STRINGDATA - not currently used by anyone
 *          DF_COLUMN_HEADERS - first row used as headers rather than data
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

/* Daniel Sebald: added general binary 2d data support. (20 August 2004)
 */
#include <gnuplot.h>
#pragma hdrstop

GpDatafile GpDf; // @global

#define is_EOF(c) ((c) == 'e' || (c) == 'E') // test to see if the end of an inline datafile is reached
#define is_comment(c) ((c) && (strchr(df_commentschars, (c)) != NULL)) // is it a comment line?
#define NOTSEP (!df_separators || !strchr(df_separators, *s)) // Used to skip whitespace but not cross a field boundary
#define DATA_LINE_BUFSIZ 160 // These control the handling of fields in the first row of a data file. See also GpC.P.Parse1stRowAsHeaders.

static bool valid_format(const char*);
#ifdef BACKWARDS_COMPATIBLE
	static void plot_option_thru(GpCommand & rC);
#endif
/*}}} */

/*{{{  variables */

enum COLUMN_TYPE {
	CT_DEFAULT,
	CT_STRING,
	CT_KEYLABEL,
	CT_XTICLABEL,
	CT_X2TICLABEL,
	CT_YTICLABEL,
	CT_Y2TICLABEL,
	CT_ZTICLABEL,
	CT_CBTICLABEL
};

static void initialize_plot_style(GpCommand & rC, CurvePoints * pPlot);
static bool rotation_matrix_2D(double R[][2], double);
static bool rotation_matrix_3D(double P[][3], double *);
static void df_swap_bytes_by_endianess(char *, int, int);

static const char * df_endian[DF_ENDIAN_TYPE_LENGTH] = {
	"little",
	"pdp (middle)",
	"swapped pdp (dimmle)",
	"big"
};

#define SUPPORT_MIDDLE_ENDIAN 1

#if SUPPORT_MIDDLE_ENDIAN
// To generate a swap, take the bit-wise complement of the lowest two bits
enum df_byte_read_order_type {
	DF_0123,
	DF_1032,
	DF_2301,
	DF_3210
};
//
// First argument, this program's endianess.  Second argument, file's endianess.
// Don't use directly.  Use 'byte_read_order()' function instead
//
static char df_byte_read_order_map[4][4] = {
	{DF_0123, DF_1032, DF_2301, DF_3210},
	{DF_1032, DF_0123, DF_1032, DF_2301},
	{DF_2301, DF_1032, DF_0123, DF_1032},
	{DF_3210, DF_2301, DF_1032, DF_0123}
};

// Argument is file's endianess type
static df_byte_read_order_type byte_read_order(df_endianess_type);
// Logical variables indicating information about data file
bool df_binary_file;
bool df_matrix_file;

static int df_M_count;
static int df_N_count;
static int df_O_count;

//df_binary_file_record_struct * df_bin_record = 0; // Initially set to default and then possibly altered by command line
df_binary_file_record_struct * df_bin_record_default = 0; // Default settings
// Settings that are transferred to default upon reset
df_binary_file_record_struct df_bin_record_reset = {
	{-1, 0, 0},
	{1, 1, 1},
	{1, 1, 1},
	DF_TRANSLATE_DEFAULT,
	{0, 0, 0},
	0,
	{0, 0, 1},

	{DF_SCAN_POINT, DF_SCAN_LINE, DF_SCAN_PLANE},
	false,
	{0, 0, 0},

	{0, 0, 0},
	{1, 1, 1},
	{0, 0, 0},
	DF_TRANSLATE_DEFAULT,
	{0, 0, 0},
	NULL       /* data_memory */
};

// Used to mark the location of a blank line in the original data input file
//GpCoordinate blank_data_line = {UNDEFINED, -999, -999, -999, -999, -999, -999, -999};
//const GpCoordinate blank_data_line(UNDEFINED, -999.0, -999.0, -999.0, -999.0, -999.0, -999.0, -999.0);

static void gpbin_filetype_function();
static void raw_filetype_function();
//static void avs_filetype_function();
static void (* binary_input_function)();     /* Will point to one of the above */
static void auto_filetype_function()
{
}                                               /* Just a placeholder for auto    */

static GpGenFTable df_bin_filetype_table[] = {
	{"avs", GpDatafile::AvsFiletypeFunction},
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
};
#define RAW_FILETYPE 1
//
// Default setting
//
// Setting that is transferred to default upon reset.
static int df_bin_filetype_reset = -1;
df_endianess_type df_bin_file_endianess; // This one is needed by breaders.c

struct df_bin_scan_table_2D_struct {
	char * string;
	df_sample_scan_type scan[3];
};

df_bin_scan_table_2D_struct df_bin_scan_table_2D[] = {
	{"xy", {DF_SCAN_POINT, DF_SCAN_LINE,  DF_SCAN_PLANE}},
	{"yx", {DF_SCAN_LINE,  DF_SCAN_POINT, DF_SCAN_PLANE}},
	{"tr", {DF_SCAN_POINT, DF_SCAN_LINE,  DF_SCAN_PLANE}},
	{"rt", {DF_SCAN_LINE,  DF_SCAN_POINT, DF_SCAN_PLANE}}
};

#define TRANSPOSE_INDEX 1

struct df_bin_scan_table_3D_struct {
	char * string;
	df_sample_scan_type scan[3];
};

df_bin_scan_table_3D_struct df_bin_scan_table_3D[] = {
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
// Machine independent names
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

const df_binary_details_struct df_binary_details[] = {
	{ch_names, sizeof(ch_names)/sizeof(ch_names[0]), {DF_CHAR, sizeof(char)}},
	{uc_names, sizeof(uc_names)/sizeof(uc_names[0]), {DF_UCHAR, sizeof(uchar)}},
	{sh_names, sizeof(sh_names)/sizeof(sh_names[0]), {DF_SHORT, sizeof(short)}},
	{us_names, sizeof(us_names)/sizeof(us_names[0]), {DF_USHORT, sizeof(ushort)}},
	{in_names, sizeof(in_names)/sizeof(in_names[0]), {DF_INT, sizeof(int)}},
	{ui_names, sizeof(ui_names)/sizeof(ui_names[0]), {DF_UINT, sizeof(uint)}},
	{lo_names, sizeof(lo_names)/sizeof(lo_names[0]), {DF_LONG, sizeof(long)}},
	{ul_names, sizeof(ul_names)/sizeof(ul_names[0]), {DF_ULONG, sizeof(ulong)}},
	{fl_names, sizeof(fl_names)/sizeof(fl_names[0]), {DF_FLOAT, sizeof(float)}},
	{db_names, sizeof(db_names)/sizeof(db_names[0]), {DF_DOUBLE, sizeof(double)}},
	{NULL, 0,                                       {DF_LONGLONG, sizeof(int64)}},
	{NULL, 0,                                       {DF_ULONGLONG, sizeof(uint64)}}
};

df_binary_details_struct df_binary_details_independent[] = {
	{byte_names, sizeof(byte_names)/sizeof(byte_names[0]), {SIGNED_TEST(1), 1}},
	{ubyte_names, sizeof(ubyte_names)/sizeof(ubyte_names[0]), {UNSIGNED_TEST(1), 1}},
	{word_names, sizeof(word_names)/sizeof(word_names[0]), {SIGNED_TEST(2), 2}},
	{uword_names, sizeof(uword_names)/sizeof(uword_names[0]), {UNSIGNED_TEST(2), 2}},
	{word2_names, sizeof(word2_names)/sizeof(word2_names[0]), {SIGNED_TEST(4), 4}},
	{uword2_names, sizeof(uword2_names)/sizeof(uword2_names[0]), {UNSIGNED_TEST(4), 4}},
	{word4_names, sizeof(word4_names)/sizeof(word4_names[0]), {SIGNED_TEST(8), 8}},
	{uword4_names, sizeof(uword4_names)/sizeof(uword4_names[0]), {UNSIGNED_TEST(8), 8}},
	{float_names, sizeof(float_names)/sizeof(float_names[0]), {FLOAT_TEST(4), 4}},
	{float2_names, sizeof(float2_names)/sizeof(float2_names[0]), {FLOAT_TEST(8), 8}}
};

const df_binary_tables_struct df_binary_tables[] = {
	{ df_binary_details, sizeof(df_binary_details)/sizeof(df_binary_details[0]) },
	{ df_binary_details_independent, sizeof(df_binary_details_independent)/sizeof(df_binary_details_independent[0]) }
};

#endif

/*}}} */

/* Initialize input buffer used by df_gets and df_fgets. */
/* Called via reset_command() on program entry.		 */
//void df_init()
void GpDatafile::DfInit()
{
	if(max_line_len < DATA_LINE_BUFSIZ) {
		max_line_len = DATA_LINE_BUFSIZ;
		df_line = (char *)malloc(max_line_len);
	}
}

/*{{{  static char *df_gets() */
//static char * df_gets()
char * GpDatafile::DfGets(GpCommand & rC)
{
	// HBB 20000526: prompt user for inline data, if in interactive mode 
	if(mixed_data_fp && GpGg.IsInteractive)
		fputs("input data ('e' ends) > ", stderr);
	// Special pseudofiles '+' and '++' return coords of sample 
	if(df_pseudodata)
		return DfGeneratePseudoData();
	else if(df_datablock)
		return *(df_datablock_line++);
	else if(rC.P.P_DfArray)
		return DfGenerateAsciiArrayEntry();
	else
		return DfFGets(rC, data_fp);
}

/*}}} */

/*{{{  char *df_gets() */
/*
 * This one is shared by df_gets() and by datablock.c:datablock_command
 */
//char * df_fgets(FILE * fin)
char * GpDatafile::DfFGets(GpCommand & rC, FILE * fin)
{
	if(fgets(df_line, max_line_len, fin)) {
		int    len = 0;
		if(mixed_data_fp)
			++rC.InlineNum;
		for(;;) {
			len += strlen(df_line + len);
			if(len > 0 && df_line[len-1] == '\n') {
				// we have read an entire text-file line. Strip the trailing linefeed and return
				df_line[len - 1] = 0;
				return df_line;
			}
			else {
				if((max_line_len-len) < 32)
					df_line = (char *)gp_realloc(df_line, max_line_len *= 2, "datafile line buffer");
				if(!fgets(df_line + len, max_line_len - len, fin))
					return df_line;  // unexpected end of file, but we have something to do
			}
		}
	}
	return NULL;
}

/*}}} */

/*{{{  static int df_tokenise(s) */
//static int df_tokenise(char * s)
int GpDatafile::DfTokenise(char * s)
{
	// implement our own sscanf that takes 'missing' into account, and can understand fortran quad format
	bool in_string;
	int i;
	// "here data" lines may end in \n rather than \0.  DOS/Windows lines may end in \r rather than \0.
	if(s[strlen(s)-1] == '\n' || s[strlen(s)-1] == '\r')
		s[strlen(s)-1] = '\0';
	for(i = 0; i<MAXDATACOLS; i++)
		df_tokens[i] = NULL;
	df_no_cols = 0;
	while(*s) {
		// We may poke at 2 new fields before coming back here - make sure there is room
		if(df_max_cols <= df_no_cols + 2)
			ExpandDfColumn((df_max_cols < 20) ? df_max_cols+20 : 2*df_max_cols);
		// have always skipped spaces at this point
		df_column[df_no_cols].position = s;
		in_string = false;
		// Keep pointer to start of this token if user wanted it for anything, particularly if it is a string
		for(i = 0; i < MAXDATACOLS; i++) {
			if(df_no_cols == use_spec[i].column-1) {
				df_tokens[i] = s;
				if(use_spec[i].expected_type == CT_STRING)
					df_column[df_no_cols].good = DF_GOOD;
			}
		}
		// CSV files must accept numbers inside quotes also, so we step past the quote
		if(*s == '"' && df_separators != NULL) {
			in_string = true;
			df_column[df_no_cols].position = ++s;
		}
		if(*s == '"') {
			// treat contents of a quoted string as single column
			in_string = !in_string;
			df_column[df_no_cols].good = DF_STRINGDATA;
		}
		else if(CheckMissing(s)) {
			df_column[df_no_cols].good = DF_MISSING;
			df_column[df_no_cols].datum = not_a_number();
			df_column[df_no_cols].position = NULL;
		}
		else {
			int used;
			int count;
			int dfncp1 = df_no_cols + 1;
			// optimizations by Corey Satten, corey@cac.washington.edu
			// only scanf the field if it is mentioned in one of the using specs
			if(!fast_columns || !df_no_use_specs || ((df_no_use_specs > 0) && (use_spec[0].column == dfncp1 ||
				(df_no_use_specs > 1 && (use_spec[1].column == dfncp1 ||
				(df_no_use_specs > 2 && (use_spec[2].column == dfncp1 ||
				(df_no_use_specs > 3 && (use_spec[3].column == dfncp1 ||
				(df_no_use_specs > 4 && (use_spec[4].column == dfncp1 ||
				df_no_use_specs > 5))))))))))) {
				// This was the [slow] code used through version 4.0
				// count = sscanf(s, "%lf%n", &df_column[df_no_cols].datum, &used);
				//
				/* Use strtod() because
				 *  - it is faster than sscanf()
				 *  - sscanf(... %n ...) may not be portable
				 *  - it allows error checking
				 *  - atof() does not return a count or new position
				 */
				char * next;
				df_column[df_no_cols].datum = strtod(s, &next);
				used = next - s;
				count = (used) ? 1 : 0;
			}
			else {
				// skip any space at start of column
				while(isspace((uchar) *s) && NOTSEP)
					++s;
				count = (*s && NOTSEP) ? 1 : 0;
				// skip chars to end of column
				used = 0;
				if(df_separators && in_string) {
					do
						++s;
					while(*s && *s != '"');
					in_string = false;
				}
				while(!isspace((uchar) *s) && (*s != NUL) && NOTSEP)
					++s;
			}
			// it might be a fortran double or quad precision. 'used' is only safe if count is 1
			if(df_fortran_constants && count == 1 && (s[used] == 'd' || s[used] == 'D' || s[used] == 'q' || s[used] == 'Q')) {
				/* HBB 20001221: avoid breaking parsing of time/date
				 * strings like 01Dec2000 that would be caused by
				 * overwriting the 'D' with an 'e'... */
				char * endptr;
				char save_char = s[used];
				// might be fortran double
				s[used] = 'e';
				// and try again
				df_column[df_no_cols].datum = strtod(s, &endptr);
				count = (endptr == s) ? 0 : 1;
				s[used] = save_char;
			}
			df_column[df_no_cols].good = count == 1 ? DF_GOOD : DF_BAD;
			if(fisnan(df_column[df_no_cols].datum)) {
				df_column[df_no_cols].good = DF_UNDEFINED;
				FPRINTF((stderr, "NaN in column %d\n", df_no_cols));
			}
		}
		++df_no_cols;
		// If we are in a quoted string, skip to end of quote
		if(in_string) {
			do
				s++;
			while(*s && (uchar) *s != '"');
		}
		// skip to 1st character in the next field
		if(df_separators != NULL) {
			// skip to next separator or end of line
			while((*s != '\0') && (*s != '\n') && NOTSEP)
				++s;
			if((*s == '\0') || (*s == '\n')) /* End of line; we're done */
				break;
			// step over field separator
			++s;
			// skip whitespace at start of next field
			while((*s == ' ' || *s == '\t') && NOTSEP)
				++s;
			if((*s == '\0') || (*s == '\n')) { // Last field is empty
				df_column[df_no_cols].good = DF_MISSING;
				df_column[df_no_cols].datum = not_a_number();
				++df_no_cols;
				break;
			}
		}
		else {
			// skip trash chars remaining in this column
			while((*s != '\0') && (*s != '\n') && !isspace((uchar) *s))
				++s;
			// skip whitespace to start of next column
			while(isspace((uchar) *s) && *s != '\n')
				++s;
		}
	}
	return df_no_cols;
}

/*}}} */

/*{{{  static float *df_read_matrix() */
/* Reads a matrix from a text file and stores it as floats in allocated
 * memory.
 *
 * IMPORTANT NOTE:  The routine returns the memory pointer for that matrix,
 * but does not retain the pointer.  Maintenance of the memory is left to
 * the calling code.
 */
//static float * df_read_matrix(int * rows, int * cols)
float * GpDatafile::DfReadMatrix(int * rows, int * cols)
{
	int max_rows = 0;
	int c;
	float * linearized_matrix = NULL;
	int bad_data = 0;
	char * s;
	int index = 0;
	*rows = 0;
	*cols = 0;
	for(;; ) {
		if(!(s = DfGets(GpC))) {
			df_eof = 1;
			return linearized_matrix; // NULL if we have not read anything yet
		}
		// skip leading spaces
		while(isspace((uchar) *s) && NOTSEP)
			++s;
		// skip blank lines and comments
		if(!*s || is_comment(*s)) {
			/* except that some comments hide an index name */
			if(indexname) {
				while(is_comment(*s) || isspace((uchar)*s))
					++s;
				if(*s && !strncmp(s, indexname, strlen(indexname)))
					index_found = true;
			}
			if(linearized_matrix)
				return linearized_matrix;
			else
				continue;
		}
		if(mixed_data_fp && is_EOF(*s)) {
			df_eof = 1;
			return linearized_matrix;
		}
		c = DfTokenise(s);
		if(!c)
			return linearized_matrix;

		/* If the first row of matrix data contains column headers */
		if(!df_already_got_headers && df_matrix_columnheaders && *rows == 0) {
			df_already_got_headers = true;
			for(int i = (df_matrix_rowheaders ? 1 : 0); i < c; i++) {
				double xpos = df_matrix_rowheaders ? (i-1) : i;
				if(use_spec[0].at) {
					t_value a;
					df_column[0].datum = xpos;
					df_column[0].good = DF_GOOD;
					evaluate_inside_using = true;
					GpGg.Ev.EvaluateAt(use_spec[0].at, &a);
					evaluate_inside_using = false;
					xpos = a.Real();
				}
				char * temp_string = DfParseStringField(df_column[i].position);
				add_tic_user(&GpGg[FIRST_X_AXIS], temp_string, xpos, -1);
				free(temp_string);
			}
			continue;
		}
		if(*cols && c != *cols) {
			// it's not regular
			free(linearized_matrix);
			GpGg.IntError(GpC, NO_CARET, "Matrix does not represent a grid");
		}
		*cols = c;
		++*rows;
		if(*rows > max_rows) {
			max_rows = MAX(2*max_rows, 1);
			linearized_matrix = (float *)gp_realloc(linearized_matrix, *cols * max_rows * sizeof(float), "df_matrix");
		}
		// store data
		{
			for(int i = 0; i < c; ++i) {
				/* First column in "matrix rowheaders" is a ytic label */
				if(df_matrix_rowheaders && i == 0) {
					char * temp_string;
					double ypos = *rows - 1;
					if(use_spec[1].at) {
						/* The save/restore is to make sure 1:(f($2)):3 works */
						t_value a;
						double save = df_column[1].datum;
						df_column[1].datum = ypos;
						evaluate_inside_using = true;
						GpGg.Ev.EvaluateAt(use_spec[1].at, &a);
						evaluate_inside_using = false;
						ypos = a.Real();
						df_column[1].datum = save;
					}
					temp_string = DfParseStringField(df_column[0].position);
					add_tic_user(&GpGg[FIRST_Y_AXIS], temp_string, ypos, -1);
					free(temp_string);
					continue;
				}
				if(i < firstpoint && df_column[i].good != DF_GOOD) {
					/* It's going to be skipped anyhow, so... */
					linearized_matrix[index++] = 0;
				}
				else
					linearized_matrix[index++] = (float)df_column[i].datum;
				if(df_column[i].good != DF_GOOD) {
					if(bad_data++ == 0)
						int_warn(NO_CARET, "matrix contains missing or undefined values");
				}
			}
		}
	}
}

/*}}} */

//static void initialize_use_spec()
void GpDatafile::InitializeUseSpec()
{
	df_no_use_specs = 0;
	for(int i = 0; i < MAXDATACOLS; ++i) {
		use_spec[i].column = i + 1; // default column
		use_spec[i].expected_type = CT_DEFAULT; // no particular expectation
		if(use_spec[i].at) {
			AtType::Destroy(use_spec[i].at);
			use_spec[i].at = NULL; // no expression
		}
		df_axis[i] = NO_AXIS; // no P_TimeFormat for this output column
	}
}

static void initialize_plot_style(GpCommand & rC, CurvePoints * pPlot)
{
	if(pPlot) {
		const int save_token = rC.CToken;
		for(; !rC.EndOfCommand(); rC.CToken++)
			if(rC.AlmostEq("w$ith")) {
				pPlot->plot_style = get_style(rC);
				break;
			}
		rC.CToken = save_token;
	}
}

/*{{{  int df_open(char *file_name, int max_using, plot_header *plot) */
//
// open file, parsing using/thru/index stuff return number of using
// specs [well, we have to return something !]
//
//int df_open(const char * cmd_filename, int max_using, CurvePoints * plot)
int GpDatafile::DfOpen(GpCommand & rC, const char * cmd_filename, int max_using, CurvePoints * plot)
{
	int  name_token = rC.CToken - 1;
	bool duplication = false;
	bool set_index = false, set_every = false, set_skip = false;
	bool set_using = false;
	bool set_matrix = false;
	fast_columns = 1;       /* corey@cac */
	/* close file if necessary */
	if(data_fp) {
		DfClose();
		data_fp = NULL;
	}
	/*{{{  initialise static variables */
	ZFREE(df_format);
	df_no_tic_specs = 0;
	ZFREE(df_key_title);
	InitializeUseSpec();
	ClearDfColumnHeaders();
	df_datum = -1;          /* it will be preincremented before use */
	df_line_number = 0;     /* ditto */
	df_lower_index = 0;
	df_index_step = 1;
	df_upper_index = MAXINT;
	ZFREE(indexname);
	df_current_index = 0;
	blank_count = 2;
	/* by initialising blank_count, leading blanks will be ignored */
	everypoint = everyline = 1; /* unless there is an every spec */
	firstpoint = firstline = 0;
	lastpoint = lastline = MAXINT;
	df_binary_file = df_matrix_file = false;
	df_pixeldata = NULL;
	df_num_bin_records = 0;
	df_matrix = false;
	df_nonuniform_matrix = false;
	df_matrix_columnheaders = false;
	df_matrix_rowheaders = false;
	df_skip_at_front = 0;
	df_xpixels = 0;
	df_ypixels = 0;
	df_transpose = false;
	df_eof = 0;
	// Save for use by df_readline().
	// Perhaps it should be a parameter to df_readline?
	df_current_plot = plot;
	column_for_key_title = NO_COLUMN_HEADER;
	rC.P.Parse1stRowAsHeaders = false;
	df_already_got_headers = false;
	/*}}} */
	if(!cmd_filename)
		GpGg.IntError(rC, rC.CToken, "missing filename");
	if(!cmd_filename[0]) {
		if(!df_filename || !*df_filename)
			GpGg.IntError(rC, rC.CToken, "No previous filename");
	}
	else {
		free(df_filename);
		df_filename = gp_strdup(cmd_filename);
	}
	//
	// defer opening until we have parsed the modifiers...
	//
#ifdef BACKWARDS_COMPATIBLE
	AtType::Destroy(ydata_func.at);
	ydata_func.at = NULL;
#endif
	// pm 25.11.2001 allow any order of options
	while(!rC.EndOfCommand()) {
		/* look for binary / matrix */
		if(rC.AlmostEq("bin$ary")) {
			if(df_filename[0] == '$')
				GpGg.IntError(rC, rC.CToken, "data blocks cannot be binary");
			rC.CToken++;
			if(df_binary_file || set_skip) {
				duplication = true;
				break;
			}
			df_binary_file = true;
			/* Up to the time of adding the general binary code, only matrix
			 * binary for 3d was defined.  So, use matrix binary by default.
			 */
			df_matrix_file = true;
			InitializeBinaryVars();
			PlotOptionBinary(rC, set_matrix, false);
			continue;
		}
		/* deal with matrix */
		if(rC.AlmostEq("mat$rix")) {
			rC.CToken++;
			if(set_matrix) {
				duplication = true;
				break;
			}
			/* `binary` default is both df_matrix_file and df_binary_file.
			 * So if df_binary_file is true, but df_matrix_file isn't, then
			 * some keyword specific to general binary has been given.
			 */
			if(!df_matrix_file && df_binary_file)
				GpGg.IntError(rC, rC.CToken, matrix_general_binary_conflict_msg);
			df_matrix_file = true;
			set_matrix = true;
			fast_columns = 0;
			continue;
		}
		/* May 2011 - "nonuniform matrix" indicates an ascii data file
		 * with the same row/column layout as "binary matrix" */
		if(rC.AlmostEq("nonuni$form")) {
			rC.CToken++;
			df_matrix_file = true;
			df_nonuniform_matrix = true;
			fast_columns = 0;
			if(df_matrix_rowheaders || df_matrix_columnheaders)
				duplication = true;
			continue;
		}
		/* Jul 2014 - "matrix columnheaders" indicates an ascii data file
		 * in uniform grid format but with column labels in row 1 */
		if(rC.AlmostEq("columnhead$ers")) {
			rC.CToken++;
			df_matrix_file = true;
			df_matrix_columnheaders = true;
			if(df_nonuniform_matrix || !set_matrix)
				duplication = true;
			continue;
		}
		// Jul 2014 - "matrix rowheaders" indicates an ascii data file
		// in uniform grid format but with row labels in column 1 
		if(rC.AlmostEq("rowhead$ers")) {
			rC.CToken++;
			df_matrix_file = true;
			df_matrix_rowheaders = true;
			if(df_nonuniform_matrix || !set_matrix)
				duplication = true;
			continue;
		}
		// deal with index 
		if(rC.AlmostEq("i$ndex")) {
			if(set_index) {
				duplication = true;
				break;
			}
			PlotOptionIndex(rC);
			set_index = true;
			continue;
		}
		// deal with every 
		if(rC.AlmostEq("ev$ery")) {
			if(set_every) {
				duplication = true;
				break;
			}
			PlotOptionEvery(rC);
			set_every = true;
			continue;
		}
		// deal with skip 
		if(rC.Eq("skip")) {
			if(set_skip || df_binary_file) {
				duplication = true;
				break;
			}
			set_skip = true;
			rC.CToken++;
			df_skip_at_front = rC.IntExpression();
			if(df_skip_at_front < 0)
				df_skip_at_front = 0;
			continue;
		}
#ifdef BACKWARDS_COMPATIBLE
		// deal with thru 
		// jev -- support for passing data from file thru user function 
		if(rC.AlmostEq("thru$")) {
			plot_option_thru();
			continue;
		}
#endif
		// deal with using 
		if(rC.AlmostEq("u$sing")) {
			if(set_using) {
				duplication = true;
				break;
			}
			PlotOptionUsing(rC, max_using);
			set_using = true;
			continue;
		}
		// deal with volatile 
		if(rC.AlmostEq("volatile")) {
			rC.CToken++;
			GpGg.IsVolatileData = true;
			continue;
		}
		// Allow this plot not to affect autoscaling 
		if(rC.AlmostEq("noauto$scale")) {
			rC.CToken++;
			plot->noautoscale = true;
			continue;
		}
		break; // unknown option 
	}
	if(duplication)
		GpGg.IntError(rC, rC.CToken, "duplicated or contradicting arguments in datafile options");
	// Check for auto-generation of key title from column header
	// Mar 2009:  This may no longer be the best place for this!
	if(GpGg.keyT.auto_titles == COLUMNHEAD_KEYTITLES) {
		if(df_no_use_specs == 1)
			column_for_key_title = use_spec[0].column;
		else if(plot && plot->plot_type == DATA3D)
			column_for_key_title = use_spec[2].column;
		else
			column_for_key_title = use_spec[1].column;
	}
	/*{{{  more variable inits */
	point_count = -1;       /* we preincrement */
	line_count = 0;
	df_pseudodata = 0;
	df_pseudorecord = 0;
	df_pseudospan = 0;
	df_datablock = false;
	df_datablock_line = NULL;
	if(plot) {
		// Save the matrix/array/image dimensions for binary image plot styles
		plot->image_properties.ncols = df_xpixels;
		plot->image_properties.nrows = df_ypixels;
		FPRINTF((stderr, "datafile.c:%d (ncols,nrows) set to (%d,%d)\n", __LINE__, df_xpixels, df_ypixels));
		if(set_every) {
			plot->image_properties.ncols = 1 + ((int)(MIN(lastpoint, (int)(df_xpixels-1))) - firstpoint) / everypoint;
			plot->image_properties.nrows = 1 + ((int)(MIN(lastline, (int)(df_ypixels-1))) - firstline) / everyline;
			FPRINTF((stderr, "datafile.c:%d  adjusting to (%d, %d)\n", __LINE__, plot->image_properties.ncols, plot->image_properties.nrows));
		}
		if(df_transpose) {
			Exchange(&plot->image_properties.ncols, &plot->image_properties.nrows);
			FPRINTF((stderr, "datafile.c:%d  adjusting to (%d, %d)\n", __LINE__, plot->image_properties.ncols, plot->image_properties.nrows));
		}
	}
	/*}}} */
	/*{{{  open file */
#if defined(HAVE_FDOPEN)
	if(*df_filename == '<' && strlen(df_filename) > 1 && df_filename[1] == '&') {
		char * substr;
		// read from an already open file descriptor 
		data_fd = strtol(df_filename + 2, &substr, 10);
		if(*substr != '\0' || data_fd < 0 || substr == df_filename+2)
			GpGg.IntError(GpC, name_token, "invalid file descriptor integer");
		else if(data_fd == fileno(stdin) || data_fd == fileno(stdout) || data_fd == fileno(stderr))
			GpGg.IntError(GpC, name_token, "cannot plot from stdin/stdout/stderr");
		else if((data_fp = fdopen(data_fd, "r")) == (FILE*)NULL)
			GpGg.IntError(GpC, name_token, "cannot open file descriptor for reading data");
		// if this stream isn't seekable, set it to volatile
		if(fseek(data_fp, 0, SEEK_CUR) < 0)
			GpGg.IsVolatileData = true;
	}
	else
#endif /* HAVE_FDOPEN */
#if defined(PIPES)
	if(*df_filename == '<') {
		restrict_popen();
		if((data_fp = popen(df_filename + 1, "r")) == (FILE*)NULL)
			os_error(name_token, "cannot create pipe for data");
		else
			df_pipe_open = true;
	}
	else
#endif /* PIPES */
	// Special filenames '-' '+' '++' '$DATABLOCK' 
	if(*df_filename == '-' && strlen(df_filename) == 1) {
		plotted_data_from_stdin = true;
		GpGg.IsVolatileData = true;
		data_fp = lf_top();
		SETIFZ(data_fp, stdin);
		mixed_data_fp = true; /* don't close command file */
	}
	else if(!strcmp(df_filename, "+")) {
		df_pseudodata = 1;
	}
	else if(!strcmp(df_filename, "++")) {
		df_pseudodata = 2;
	}
	else if(df_filename[0] == '$') {
		df_datablock = true;
		df_datablock_line = get_datablock(df_filename);
	}
	else if(!strcmp(df_filename, "@@") && rC.P.P_DfArray) {
		// GpC.P.P_DfArray was set in string_or_express() 
		df_array_index = 0;
	}
	else {
		// filename cannot be static array! 
		gp_expand_tilde(&df_filename);
#ifdef HAVE_SYS_STAT_H
		{
			struct stat statbuf;
			if((stat(df_filename, &statbuf) > -1) && S_ISDIR(statbuf.st_mode)) {
				os_error(name_token, "\"%s\" is a directory", df_filename);
			}
		}
#endif /* HAVE_SYS_STAT_H */

		if((data_fp = loadpath_fopen(df_filename, df_binary_file ? "rb" : "r")) == NULL) {
			int_warn(NO_CARET, "Cannot find or open file \"%s\"", df_filename);
			df_eof = 1;
			return DF_EOF;
		}
	}
/*}}} */

	/* Binary file options are handled differently depending on the plot style. */
	/* Peek ahead in the command line to see if there is a "with <style>" later.*/
	if(df_binary_file || df_matrix_file)
		initialize_plot_style(rC, plot);
	/* If the data is in binary matrix form, read in some values
	 * to determine the nubmer of columns and rows.  If data is in
	 * ASCII matrix form, read in all the data to memory in preparation
	 * for using df_readbinary() routine.
	 */
	if(df_matrix_file) {
		DfDetermineMatrixInfo(data_fp);
		/* NB: If we're inside a 'stats' command there is no plot */
		if(plot) {
			/* Image size bookkeeping for ascii uniform matrices */
			if(!df_binary_file) {
				plot->image_properties.ncols = df_xpixels;
				plot->image_properties.nrows = df_ypixels;
			}
		}
	}
	// General binary, matrix binary and ASCII matrix all use the df_readbinary() routine.
	if(df_binary_file || df_matrix_file) {
		df_read_binary = true;
		AdjustBinaryUseSpec(plot);
	}
	else {
		df_read_binary = false;
	}
	// Make information about whether the data forms a grid or not available to the outside world.
	df_matrix = (df_matrix_file || ((df_num_bin_records == 1) && ((df_bin_record[0].cart_dim[1] > 0) || (df_bin_record[0].scan_dim[1] > 0))));
	return df_no_use_specs;
}

/*}}} */

//void df_close()
void GpDatafile::DfClose()
{
	int i;
	/* paranoid - mark $n and column(n) as invalid */
	df_no_cols = 0;
	if(data_fp || df_datablock) {
#ifdef BACKWARDS_COMPATIBLE
		AtType::Destroy(ydata_func.at);
		ydata_func.at = NULL;
#endif
		/* free any use expression storage */
		for(i = 0; i < MAXDATACOLS; ++i) {
			if(use_spec[i].at) {
				AtType::Destroy(use_spec[i].at);
				use_spec[i].at = NULL;
			}
		}
		/* free binary matrix data */
		if(df_matrix) {
			for(i = 0; i < df_num_bin_records; i++) {
				ZFREE(df_bin_record[i].memory_data);
			}
		}
		if(!mixed_data_fp && !df_datablock) {
#if defined(HAVE_FDOPEN)
			if(data_fd == fileno(data_fp)) {
				/* This will allow replotting if this stream is backed by a file,
				* and hopefully is harmless if it connects to a pipe.
				* Leave it open in either case.
				*/
				rewind(data_fp);
				fprintf(stderr, "Rewinding fd %d\n", data_fd);
			}
			else
#endif
#if defined(PIPES)
			if(df_pipe_open) {
				pclose(data_fp);
				df_pipe_open = false;
			}
			else
#endif
			fclose(data_fp);
		}
		mixed_data_fp = false;
		data_fp = NULL;
	}
}

/*}}} */

/*{{{  void df_showdata() */
/* display the current data file line for an error message
 */
//void df_showdata()
void GpDatafile::DfShowData()
{
	if(data_fp && df_filename && df_line) {
		// display no more than 77 characters
		fprintf(stderr, "%.77s%s\n%s:%d:", df_line, (strlen(df_line) > 77) ? "..." : "", df_filename, df_line_number);
	}
}

/*}}} */

//static void plot_option_every()
void GpDatafile::PlotOptionEvery(GpCommand & rC)
{
	fast_columns = 0;       /* corey@cac */
	/* allow empty fields - every a:b:c::e we have already established
	 * the defaults */
	if(!rC.Eq(++rC.CToken, ":")) {
		everypoint = rC.IntExpression();
		if(everypoint < 0)
			everypoint = 1;
		else if(everypoint < 1)
			GpGg.IntError(rC, rC.CToken, "Expected positive integer");
	}
	/* if it fails on first test, no more tests will succeed. If it
	 * fails on second test, next test will succeed with correct
	 * rC.CToken */
	if(rC.Eq(":") && !rC.Eq(++rC.CToken, ":")) {
		everyline = rC.IntExpression();
		if(everyline < 0)
			everyline = 1;
		else if(everyline < 1)
			GpGg.IntError(rC, rC.CToken, "Expected positive integer");
	}
	if(rC.Eq(":") && !rC.Eq(++rC.CToken, ":")) {
		firstpoint = rC.IntExpression();
		SETMAX(firstpoint, 0);
	}
	if(rC.Eq(":") && !rC.Eq(++rC.CToken, ":")) {
		firstline = rC.IntExpression();
		SETMAX(firstline, 0);
	}
	if(rC.Eq(":") && !rC.Eq(++rC.CToken, ":")) {
		lastpoint = rC.IntExpression();
		if(lastpoint < 0)
			lastpoint = MAXINT;
		else if(lastpoint < firstpoint)
			GpGg.IntError(rC, rC.CToken, "Last point must not be before first point");
	}
	if(rC.Eq(":")) {
		++rC.CToken;
		lastline = rC.IntExpression();
		if(lastline < 0) 
			lastline = MAXINT;
		else if(lastline < firstline)
			GpGg.IntError(rC, rC.CToken, "Last line must not be before first line");
	}
}

void GpDatafile::PlotOptionIndex(GpCommand & rC)
{
	if(df_binary_file && df_matrix_file)
		GpGg.IntError(rC, rC.CToken, "Binary matrix file format does not allow more than one surface per file");
	++rC.CToken;
	// Check for named index 
	if((indexname = rC.TryToGetString())) {
		index_found = false;
		return;
	}
	// Numerical index list 
	df_lower_index = rC.IntExpression();
	if(df_lower_index < 0)
		GpGg.IntError(rC, rC.CToken, "index must be non-negative");
	if(rC.Eq(":")) {
		++rC.CToken;
		if(rC.Eq(":")) {
			df_upper_index = MAXINT; /* If end index not specified */
		}
		else {
			df_upper_index = rC.IntExpression();
			if(df_upper_index < df_lower_index)
				GpGg.IntError(rC, rC.CToken, "Upper index should be bigger than lower index");
		}
		if(rC.Eq(":")) {
			++rC.CToken;
			df_index_step = abs(rC.IntExpression());
			if(df_index_step < 1)
				GpGg.IntError(rC, rC.CToken, "Index step must be positive");
		}
	}
	else {
		df_upper_index = df_lower_index;
	}
}

#ifdef BACKWARDS_COMPATIBLE
static void plot_option_thru(GpCommand & rC)
{
	rC.CToken++;
	strcpy(rC.P.CDummyVar[0], rC.P.SetDummyVar[0]);
	//
	// allow y also as a dummy variable.
	// during plot, rC.P.CDummyVar[0] and [1] are 'sacred'
	// ie may be set by  splot [u=1:2] [v=1:2], and these
	// names are stored only in rC.P.CDummyVar[]
	// so choose dummy var 2 - can anything vital be here ?
	// 
	rC.P_DummyFunc = &ydata_func;
	strcpy(rC.P.CDummyVar[2], "y");
	ydata_func.at = perm_at();
	rC.P_DummyFunc = NULL;
}

#endif

void GpDatafile::PlotOptionUsing(GpCommand & rC, int max_using)
{
	int    no_cols = 0; // For general binary only.
	char * column_label;
	// The filetype function may have set the using specs, so reset
	// them before processing tokens. 
	if(df_binary_file)
		InitializeUseSpec();
	// Try to distinguish between 'using "A":"B"' and 'using "%lf %lf"
	if(!rC.EndOfCommand() && rC.IsString(++rC.CToken)) {
		int save_token = rC.CToken;
		df_format = rC.TryToGetString();
		if(valid_format(df_format))
			return;
		ZFREE(df_format);
		rC.CToken = save_token;
	}
	if(!rC.EndOfCommand()) {
		do {            /* must be at least one */
			if(df_no_use_specs >= MAXDATACOLS)
				GpGg.IntError(rC, rC.CToken, "at most %d columns allowed in using spec", MAXDATACOLS);
			if(df_no_use_specs >= max_using)
				GpGg.IntError(rC, rC.CToken, "Too many columns in using specification");
			if(rC.Eq(":")) {
				/* empty specification - use default */
				use_spec[df_no_use_specs].column = df_no_use_specs;
				if(df_no_use_specs > no_cols)
					no_cols = df_no_use_specs;
				++df_no_use_specs;
				/* do not increment c+token ; let while() find the : */
			}
			else if(rC.Eq("(")) {
				fast_columns = 0; /* corey@cac */
				rC.P_DummyFunc = NULL; // no dummy variables active
				// this will match ()'s:
				rC.P.AtHighestColumnUsed = NO_COLUMN_HEADER;
				use_spec[df_no_use_specs].at = rC.P.PermAt();
				if(no_cols < rC.P.AtHighestColumnUsed)
					no_cols = rC.P.AtHighestColumnUsed;
				// Catch at least the simplest case of 'autotitle columnhead' using an expression
				use_spec[df_no_use_specs++].column = rC.P.AtHighestColumnUsed;
				// It would be nice to handle these like any other
				// internal function via perm_at() but it doesn't work.
			}
			else if(rC.AlmostEq("xtic$labels")) {
				PlotTicLabelUsing(rC, CT_XTICLABEL);
			}
			else if(rC.AlmostEq("x2tic$labels")) {
				PlotTicLabelUsing(rC, CT_X2TICLABEL);
			}
			else if(rC.AlmostEq("ytic$labels")) {
				PlotTicLabelUsing(rC, CT_YTICLABEL);
			}
			else if(rC.AlmostEq("y2tic$labels")) {
				PlotTicLabelUsing(rC, CT_Y2TICLABEL);
			}
			else if(rC.AlmostEq("ztic$labels")) {
				PlotTicLabelUsing(rC, CT_ZTICLABEL);
			}
			else if(rC.AlmostEq("cbtic$labels")) {
				PlotTicLabelUsing(rC, CT_CBTICLABEL);
			}
			else if(rC.AlmostEq("key")) {
				PlotTicLabelUsing(rC, CT_KEYLABEL);
			}
			else if((column_label = rC.TryToGetString())) {
				/* ...using "A"... Dummy up a call to column(column_label) */
				use_spec[df_no_use_specs].at = create_call_column_at(column_label);
				use_spec[df_no_use_specs++].column = NO_COLUMN_HEADER;
				rC.P.Parse1stRowAsHeaders = true;
				fast_columns = 0;
				// FIXME - is it safe to always take the title from the 2nd use spec? 
				if(df_no_use_specs == 2) {
					free(df_key_title);
					df_key_title = gp_strdup(column_label);
				}
			}
			else {
				const int col = rC.IntExpression();
				if(col == -3) // pseudocolumn -3 means "last column" 
					fast_columns = 0;
				else if(col < -2)
					GpGg.IntError(rC, rC.CToken, "Column must be >= -2");
				use_spec[df_no_use_specs++].column = col;
				// Supposedly only happens for binary files, but don't bet on it 
				if(col > no_cols)
					no_cols = col;
			}
		} while(rC.Eq(":") && ++rC.CToken);
	}
	if(df_binary_file) {
		// If the highest user column number is greater than number of binary
		// columns, set the unitialized columns binary info to that of the last
		// specified column or the default.
		DfExtendBinaryColumns(no_cols);
	}
	// Allow a format specifier after the enumeration of columns. 
	// Note: This was left out by mistake in versions 4.6.0 + 4.6.1 
	if(!rC.EndOfCommand() && rC.IsString(rC.CToken)) {
		df_format = rC.TryToGetString();
		if(!valid_format(df_format))
			GpGg.IntError(rC, rC.CToken, "format must have 1-7 conversions of type double (%%lf)");
	}
}

void GpDatafile::PlotTicLabelUsing(GpCommand & rC, int axis)
{
	int col = 0;
	rC.CToken++;
	if(!rC.Eq("("))
		GpGg.IntError(rC, rC.CToken, "missing '('");
	rC.CToken++;
	/* FIXME: What we really want is a test for a constant expression as  */
	/* opposed to a dummy expression. This is similar to the problem with */
	/* with parsing the first argument of the plot command itself.        */
	if(rC.IsANumber(rC.CToken) || rC.TypeDdv(rC.CToken)==INTGR) {
		col = rC.IntExpression();
		use_spec[df_no_use_specs+df_no_tic_specs].at = NULL;
	}
	else {
		use_spec[df_no_use_specs+df_no_tic_specs].at = rC.P.PermAt();
		fast_columns = 0; // Force all columns to be evaluated
		col = 1; // Redundant because of the above
	}
	if(col < 1)
		GpGg.IntError(rC, rC.CToken, "ticlabels must come from a real column");
	if(!rC.Eq(")"))
		GpGg.IntError(rC, rC.CToken, "missing ')'");
	rC.CToken++;
	use_spec[df_no_use_specs+df_no_tic_specs].expected_type = axis;
	use_spec[df_no_use_specs+df_no_tic_specs].column = col;
	df_no_tic_specs++;
}

int GpDatafile::DfReadLine(double v[], int max)
{
	if(!data_fp && !df_pseudodata && !df_datablock && !GpC.P.P_DfArray)
		return DF_EOF;
	else if(df_read_binary)
		return DfReadBinary(v, max); // General binary, matrix binary or matrix ascii converted to binary
	else
		return DfReadAscii(GpC, v, max);
}

/*}}} */

/* do the hard work... read lines from file,
 * - use blanks to get index number
 * - ignore lines outside range of indices required
 * - fill v[] based on using spec if given
 */

int GpDatafile::DfReadAscii(GpCommand & rC, double v[], int max)
{
	char * s;
	/* Version 5:
	 * We used to return DF_MISSING or DF_UNDEFINED immediately if any column
	 * could not be parsed.  Now we note this failure in return_value but
	 * continue to process any additional requested columns before returning.
	 * This is a CHANGE.
	 */
	int return_value = DF_GOOD;
	assert(max <= MAXDATACOLS);
	/* catch attempt to read past EOF on mixed-input */
	if(df_eof)
		return DF_EOF;
	/*{{{  process line */
	while((s = DfGets(rC)) != NULL) {
		int line_okay = 1;
		int output = 0; /* how many numbers written to v[] */
		return_value = DF_GOOD;
		/* "skip" option */
		if(df_skip_at_front > 0) {
			df_skip_at_front--;
			continue;
		}
		++df_line_number;
		df_no_cols = 0;

		/*{{{  check for blank lines, and reject by index/every */
		/*{{{  skip leading spaces */
		while(isspace((uchar) *s) && NOTSEP)
			++s;    /* will skip the \n too, to point at \0  */
		/*}}} */

		/*{{{  skip comments */
		if(is_comment(*s)) {
			if(indexname) { /* Look for index name in comment */
				while(is_comment(*s) || isspace((uchar)*s))
					++s;
				if(*s && !strncmp(s, indexname, strlen(indexname)))
					index_found = true;
			}
			continue; /* ignore comments */
		}
		/*}}} */

		/*{{{  check EOF on mixed data */
		if(mixed_data_fp && is_EOF(*s)) {
			df_eof = 1; /* trap attempts to read past EOF */
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
			point_count = -1; /* restart counter within line */
			if(++blank_count == 1) {
				/* first blank line */
				++line_count;
			}
			/* just reached end of a group/surface */
			if(blank_count == 2) {
				++df_current_index;
				line_count = 0;
				df_datum = -1;
				/* Found two blank lines after a block of data with a named index */
				if(indexname && index_found) {
					df_eof = 1;
					return DF_EOF;
				}

				/* ignore line if current_index has just become
				 * first required one - client doesn't want this
				 * blank line. While we're here, check for <=
				 * - we need to do it outside this conditional, but
				 * probably no extra cost at assembler level
				 */
				if(df_current_index <= df_lower_index)
					continue;  /* dont tell client */

				/* df_upper_index is MAXINT-1 if we are not doing index */
				if(df_current_index > df_upper_index) {
					/* oops - need to gobble rest of input if mixed */
					if(mixed_data_fp)
						continue;
					else {
						df_eof = 1;
						return DF_EOF; /* no point continuing */
					}
				}
			}
			/* dont tell client if we haven't reached first index */
			if(indexname && !index_found)
				continue;
			if(df_current_index < df_lower_index)
				continue;
			/* ignore blank lines after blank_index */
			if(blank_count > 2)
				continue;
			return DF_FIRST_BLANK - (blank_count - 1);
		}
		/*}}} */
		/* get here => was not blank */
		blank_count = 0;
		/*{{{  ignore points outside range of index */
		/* we try to return end-of-file as soon as we pass upper index,
		 * but for mixed input stream, we must skip garbage
		 */
		if(indexname && !index_found)
			continue;
		if(df_current_index < df_lower_index || df_current_index > df_upper_index || ((df_current_index - df_lower_index) % df_index_step) != 0)
			continue;
		/*}}} */
		/* Bookkeeping for the plot ... every N:M:etc option */
		if((rC.P.Parse1stRowAsHeaders || column_for_key_title > 0) && !df_already_got_headers) {
			FPRINTF((stderr, "skipping 'every' test in order to read column headers\n"));
		}
		else {
			/* Accept only lines with (line_count%everyline) == 0 */
			if(line_count < firstline || line_count > lastline || (line_count - firstline) % everyline != 0)
				continue;
			/* update point_count. ignore point if point_count%everypoint != 0 */
			if(++point_count < firstpoint || point_count > lastpoint || (point_count - firstpoint) % everypoint != 0)
				continue;
		}
		/*}}} */
		++df_datum;
		if(df_format) {
			/*{{{  do a sscanf */
			int i;
			/* check we have room for at least 7 columns */
			if(df_max_cols < 7)
				ExpandDfColumn(7);
			df_no_cols = sscanf(s, df_format, &df_column[0].datum,
			    &df_column[1].datum, &df_column[2].datum, &df_column[3].datum, &df_column[4].datum,
			    &df_column[5].datum, &df_column[6].datum);
			if(df_no_cols == EOF) {
				df_eof = 1;
				return DF_EOF; /* tell client */
			}
			for(i = 0; i < df_no_cols; ++i) { /* may be zero */
				df_column[i].good = DF_GOOD;
				df_column[i].position = NULL; /* cant get a time */
			}
			/*}}} */
		}
		else
			DfTokenise(s);
		//
		// df_tokenise already processed everything, but in the case of pseudodata
		// '+' or '++' the value itself was passed as an ascii string formatted by
		// "%g".  We can do better than this by substituting in the binary value.
		//
		if(df_pseudodata > 0)
			df_column[0].datum = df_pseudovalue_0;
		if(df_pseudodata > 1)
			df_column[1].datum = df_pseudovalue_1;
		// Similar to above, we can go back to the original numerical value of A[i]
		if(rC.P.P_DfArray && rC.P.P_DfArray->udv_value.v.value_array[df_array_index].type == CMPLX)
			df_column[1].datum = rC.P.P_DfArray->udv_value.v.value_array[df_array_index].v.cmplx_val.real;

		/* Always save the contents of the first row in case it is needed for
		 * later access via column("header").  However, unless we know for certain that
		 * it contains headers only, e.g. via rC.P.Parse1stRowAsHeaders or
		 * (column_for_key_title > 0), also treat it as a data row.
		 */
		if(df_datum == 0 && !df_already_got_headers) {
			int j;
			for(j = 0; j<df_no_cols; j++) {
				free(df_column[j].header);
				df_column[j].header = DfParseStringField(df_column[j].position);
				if(df_column[j].header) {
					if(df_longest_columnhead < (int)strlen(df_column[j].header))
						df_longest_columnhead = strlen(df_column[j].header);
					FPRINTF((stderr, "Col %d: \"%s\"\n", j, df_column[j].header));
				}
			}
			df_already_got_headers = true;
			/* Restrict the column number to possible values */
			if(column_for_key_title > df_no_cols)
				column_for_key_title = df_no_cols;
			if(column_for_key_title == -3) /* last column in file */
				column_for_key_title = df_no_cols;
			if(column_for_key_title > 0) {
				df_key_title = gp_strdup(df_column[column_for_key_title-1].header);
				if(!df_key_title) {
					FPRINTF((stderr, "df_readline: missing column head for key title\n"));
					return(DF_KEY_TITLE_MISSING);
				}
				df_datum--;
				column_for_key_title = NO_COLUMN_HEADER;
				rC.P.Parse1stRowAsHeaders = false;
				return DF_FOUND_KEY_TITLE;
			}
			else if(rC.P.Parse1stRowAsHeaders) {
				df_datum--;
				rC.P.Parse1stRowAsHeaders = false;
				return DF_COLUMN_HEADERS;
			}
		}
		// Used by stats to set STATS_columns
		if(df_datum == 0)
			df_last_col = df_no_cols;
		/*{{{  copy column[] to v[] via use[] */
		{
			int limit = (df_no_use_specs ? df_no_use_specs + df_no_tic_specs : MAXDATACOLS);
			SETMIN(limit, max + df_no_tic_specs);
			for(output = 0; output < limit; ++output) {
				// if there was no using spec, column is output+1 and at=NULL 
				int column = use_spec[output].column;
				if(column == -3) // pseudocolumn -3 means "last column" 
					column = use_spec[output].column = df_no_cols;
				// Handle cases where column holds a meta-data string 
				// Axis labels, plot titles, etc.                     
				if(use_spec[output].expected_type >= CT_XTICLABEL) {
					int axis, axcol;
					double xpos;
					// EAM FIXME - skip columnstacked histograms also 
					if(df_current_plot) {
						if(df_current_plot->plot_style == BOXPLOT)
							continue;
					}
					switch(use_spec[output].expected_type) {
						default:
						case CT_XTICLABEL:
						    axis = FIRST_X_AXIS;
						    axcol = 0;
						    break;
						case CT_X2TICLABEL:
						    axis = SECOND_X_AXIS;
						    axcol = 0;
						    break;
						case CT_YTICLABEL:
						    axis = FIRST_Y_AXIS;
						    axcol = 1;
						    break;
						case CT_Y2TICLABEL:
						    axis = SECOND_Y_AXIS;
						    axcol = 1;
						    break;
						case CT_ZTICLABEL:
						    axis = FIRST_Z_AXIS;
						    axcol = 2;
						    break;
						case CT_CBTICLABEL:
						    // EAM FIXME - Which column to set for cbtic? 
						    axis = COLOR_AXIS;
						    axcol = 3;
						    break;
					}
					// FIXME EAM - Trap special case of only a single
					// 'using' column. But really we need to handle
					// general case of implicit column 0 
					xpos = (output == 1) ? ((axcol == 0) ? df_datum : v[axcol-1]) : v[axcol];
					if(df_current_plot && df_current_plot->plot_style == HISTOGRAMS) {
						if(output > 1) /* Can only happen for HT_ERRORBARS */
							xpos = (axcol == 0) ? df_datum : v[axcol-1];
						xpos += df_current_plot->histogram->start;
					}
					// Tic label is generated by a string-valued function 
					if(use_spec[output].at) {
						t_value a;
						evaluate_inside_using = true;
						GpGg.Ev.EvaluateAt(use_spec[output].at, &a);
						evaluate_inside_using = false;
						if(a.type == STRING) {
							add_tic_user(&GpGg[axis], a.v.string_val, xpos, -1);
							gpfree_string(&a);
						}
						else {
							/* Version 5: In this case do not generate a tic at all. */
							/* E.g. plot $FOO using 1:2:(filter(3) ? strcol(3) : NaN) */
							/*
							   add_tic_user(&GpGg[axis], "", xpos, -1);
							   int_warn(NO_CARET,"Tic label does not evaluate as
							      string!\n");
							 */
						}
					}
					else {
						char * temp_string = DfParseStringField(df_tokens[output]);
						add_tic_user(&GpGg[axis], temp_string, xpos, -1);
						free(temp_string);
					}
				}
				else if(use_spec[output].expected_type == CT_KEYLABEL) {
					char * temp_string = DfParseStringField(df_tokens[output]);
					if(df_current_plot)
						AddKeyEntry(temp_string, df_datum);
					free(temp_string);
				}
				else if(use_spec[output].at) {
					t_value a;
					bool timefield = false;
					/* no dummy values to set up prior to... */
					evaluate_inside_using = true;
					GpGg.Ev.EvaluateAt(use_spec[output].at, &a);
					evaluate_inside_using = false;
					if(GpGg.Ev.undefined) {
						return_value = DF_UNDEFINED;
						v[output] = not_a_number();
						continue;
					}
					if((df_axis[output] != NO_AXIS) && GpGg[df_axis[output]].datatype == DT_TIMEDATE)
						timefield = true;
					if(timefield && (a.type != STRING) && !strcmp(GpGg.P_TimeFormat, "%s")) {
						/* Handle the case of P_TimeFormat "%s" which expects a string */
						/* containing a number. If evaluate_at() above returned a */
						/* bare number then we must convert it to a sting before  */
						/* falling through to the usual processing case.          */
						/* NB: We only accept time values of +/- 10^12 seconds.   */
						char * timestring = (char *)malloc(20);
						sprintf(timestring, "%16.3f", a.Real());
						a.type = STRING;
						a.v.string_val = timestring;
					}
					if(a.type == STRING) {
						v[output] = not_a_number(); /* found a string, not a number */

						/* This string value will get parsed as if it were a data column */
						/* so put it in quotes to allow embedded whitespace.             */
						if(use_spec[output].expected_type == CT_STRING) {
							char * s = (char *)malloc(strlen(a.v.string_val)+3);
							*s = '"';
							strcpy(s+1, a.v.string_val);
							strcat(s, "\"");
							free(df_stringexpression[output]);
							df_tokens[output] = df_stringexpression[output] = s;
						}
						// Check for P_TimeFormat string generated by a function 
						if(timefield) {
							struct tm tm;
							double usec = 0.0;
							if(gstrptime(a.v.string_val, GpGg.P_TimeFormat, &tm, &usec))
								v[output] = (double)gtimegm(&tm) + usec;
							else
								return_value = DF_BAD;
						}
						gpfree_string(&a);
					}

					else {
						v[output] = a.Real();
						if(fisnan(v[output]))
							return_value = DF_UNDEFINED;
					}
				}
				else if(column == -2) {
					v[output] = df_current_index;
				}
				else if(column == -1) {
					v[output] = line_count;
				}
				else if(column == 0) {
					v[output] = df_datum; /* using 0 */
				}
				else if(column <= 0) /* really < -2, but */
					GpGg.IntError(GpC, NO_CARET, "internal error: column <= 0 in datafile.c");
				else if((df_axis[output] != NO_AXIS) && (GpGg[df_axis[output]].datatype == DT_TIMEDATE)) {
					struct tm tm;
					double usec = 0.0;
					if(column > df_no_cols || df_column[column - 1].good == DF_MISSING || !df_column[column - 1].position || !gstrptime(df_column[column - 1].position, GpGg.P_TimeFormat, &tm, &usec)) {
						// line bad only if user explicitly asked for this column 
						if(df_no_use_specs)
							line_okay = 0;

						// return or ignore line depending on line_okay 
						break;
					}
					v[output] = (double)gtimegm(&tm) + usec;
				}
				else if(use_spec[output].expected_type == CT_STRING) {
					// Do nothing. String tokens were loaded into df_tokens already.
				}
				else {
					// column > 0 
					if((column <= df_no_cols) && df_column[column - 1].good == DF_GOOD) {
						v[output] = df_column[column - 1].datum;

						/* Version 5:
						 * Do not return immediately on DF_MISSING or DF_UNDEFINED.
						 * THIS IS A CHANGE.
						 */
					}
					else if((column <= df_no_cols) && (df_column[column - 1].good == DF_MISSING)) {
						v[output] = not_a_number();
						return_value = DF_MISSING;
					}
					else if((column <= df_no_cols) && (df_column[column - 1].good == DF_UNDEFINED)) {
						v[output] = df_column[column - 1].datum;
						return_value = DF_UNDEFINED;
					}
					else {
						/* line bad only if user explicitly asked for this column */
						if(df_no_use_specs)
							line_okay = 0;
						break; /* return or ignore depending on line_okay */
					}
				}
				/* Special case to make 'using 0' et al. to work with labels */
				if(use_spec[output].expected_type == CT_STRING && (!(use_spec[output].at) || !df_tokens[output]) && (column == -2 || column == -1 || column == 0)) {
					char * s = (char *)malloc(32*sizeof(char));
					sprintf(s, "%d", (int)v[output]);
					free(df_stringexpression[output]);
					df_tokens[output] = df_stringexpression[output] = s;
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
		output -= df_no_tic_specs;
		assert(df_no_use_specs == 0 || output == df_no_use_specs || output == max);
		/*
		 * EAM Apr 2012 - If there is no using spec, then whatever we found on
		 * the first line becomes the expectation for the rest of the input file.
		 * THIS IS A CHANGE!
		 */
		SETIFZ(df_no_use_specs, output);
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
			case DF_BAD:
			    return return_value;
			    break;
			default:
			    return output;
			    break;
		}
	}
	/*}}} */

	/* get here => fgets failed */

	/* no longer needed - mark column(x) as invalid */
	df_no_cols = 0;
	df_eof = 1;
	return DF_EOF;
}

/*}}} */

static const char * read_error_msg = "Data file read error";

float df_read_a_float(FILE * fin)
{
	float fdummy;
	if(fread(&fdummy, sizeof(fdummy), 1, fin) != 1)
		GpGg.IntError(GpC, NO_CARET, feof(fin) ? "Data file is empty" : read_error_msg);
	df_swap_bytes_by_endianess((char*)&fdummy, byte_read_order(df_bin_file_endianess), sizeof(fdummy));
	return fdummy;
}

void GpDatafile::DfDetermineMatrixInfo(FILE * fin)
{
	if(df_binary_file) {
		// Binary matrix format. 
		off_t nr; // off_t because they contribute to fseek offset 
		off_t flength;
		// Read first value for number of columns. 
		float fdummy = df_read_a_float(fin);
		off_t nc = ((size_t)fdummy);
		if(nc == 0)
			GpGg.IntError(GpC, NO_CARET, "Read grid of zero width");
		else if(nc > 1e8)
			GpGg.IntError(GpC, NO_CARET, "Read grid width too large");
		// Read second value for corner_0 x. 
		fdummy = df_read_a_float(fin);
		df_matrix_corner[0][0] = fdummy;
		// Read nc+1 value for corner_1 x.
		if(nc > 1) {
			fseek(fin, (nc-2)*sizeof(float), SEEK_CUR);
			fdummy = df_read_a_float(fin);
		}
		df_matrix_corner[1][0] = fdummy;
		// Read nc+2 value for corner_0 y. 
		df_matrix_corner[0][1] = df_read_a_float(fin);
		// Compute length of file and number of columns.
		fseek(fin, 0L, SEEK_END);
		flength = ftell(fin)/sizeof(float);
		nr = flength/(nc + 1);
		if(nr*(nc + 1) != flength)
			GpGg.IntError(GpC, NO_CARET, "File doesn't factorize into full matrix");
		// Read last value for corner_1 y 
		fseek(fin, -(nc + 1)*sizeof(float), SEEK_END);
		df_matrix_corner[1][1] = df_read_a_float(fin);
		// Set up scan information for df_readbinary(). 
		df_bin_record[0].scan_dim[0] = (int)nc;
		df_bin_record[0].scan_dim[1] = (int)nr;
		// Reset counter file pointer. 
		fseek(fin, 0L, SEEK_SET);
	}
	else {
		/* ASCII matrix format, converted to binary memory format. */
		static float * matrix = NULL;
		int nr, nc;
		/* Insurance against creating a matrix with df_read_matrix()
		 * and then erroring out through df_add_binary_records().
		 */
		free(matrix);
		/* Set important binary variables, then free memory for all default
		 * binary records and set number of records to 0. */
		InitializeBinaryVars();
		ClearBinaryRecords(DF_CURRENT_RECORDS);
		/* If the user has set an explicit locale for numeric input, apply it */
		/* here so that it affects data fields read from the input file.      */
		set_numeric_locale();
		/* "skip" option to skip lines at start of ascii file */
		while(df_skip_at_front > 0) {
			DfGets(GpC);
			df_skip_at_front--;
		}
		// Keep reading matrices until file is empty
		while(1) {
			if((matrix = DfReadMatrix(&nr, &nc)) != NULL) {
				int index = df_num_bin_records;

				/* Ascii matrix with explicit y in first row, x in first column */
				if(df_nonuniform_matrix) {
					nc--;
					nr--;
				}
				/* First column contains row labels, not data */
				if(df_matrix_rowheaders)
					nc--;

				DfAddBinaryRecords(1, DF_CURRENT_RECORDS);
				df_bin_record[index].memory_data = (char*)matrix;
				matrix = NULL;
				df_bin_record[index].scan_dim[0] = nc;
				df_bin_record[index].scan_dim[1] = nr;
				df_bin_record[index].scan_dim[2] = 0;
				df_bin_file_endianess = THIS_COMPILER_ENDIAN;

				/* Save matrix dimensions in case it contains an image */
				df_xpixels = nc;
				df_ypixels = nr;

				/* This matrix is the one (and only) requested by name.	*/
				/* Dummy up index range and skip rest of file.		*/
				if(indexname) {
					if(index_found) {
						df_lower_index = df_upper_index = index;
						break;
					}
					else
						df_lower_index = index+1;
				}
			}
			else
				break;
		}
		// We are finished reading user input; return to C locale for internal use 
		reset_numeric_locale();
		// Data from file is now in memory.  Make the rest of gnuplot think
		// that the data stream has not yet reached the end of file.
		df_eof = 0;
	}
}

// Called from int_error() 
void df_reset_after_error()
{
	reset_numeric_locale();
	GpDf.evaluate_inside_using = false;
}

//void f_stringcolumn(GpArgument * pArg)
//static
void GpDatafile::F_StringColumn(GpArgument * pArg)
{
	t_value a;
	int column;
	GpGg.Ev.Pop(a);
	if(!GpDf.evaluate_inside_using || GpDf.df_matrix)
		GpGg.IntError(GpC, GpC.CToken-1, "stringcolumn() called from invalid context");
	if(a.type == STRING) {
		int j;
		char * name = a.v.string_val;
		column = DF_COLUMN_HEADERS;
		for(j = 0; j < GpDf.df_no_cols; j++) {
			if(GpDf.df_column[j].header) {
				int offset = (*GpDf.df_column[j].header == '"') ? 1 : 0;
				if(streq(name, GpDf.df_column[j].header + offset)) {
					column = j+1;
					SETIFZ(GpDf.df_key_title, gp_strdup(GpDf.df_column[j].header));
					break;
				}
			}
		}
		// This warning should only trigger once per problematic input file
		if(column == DF_COLUMN_HEADERS && (*name) && GpDf.df_warn_on_missing_columnheader) {
			GpDf.df_warn_on_missing_columnheader = false;
			int_warn(NO_CARET, "no column with header \"%s\"", a.v.string_val);
			for(j = 0; j < GpDf.df_no_cols; j++) {
				if(GpDf.df_column[j].header) {
					int offset = (*GpDf.df_column[j].header == '"') ? 1 : 0;
					if(!strncmp(name, GpDf.df_column[j].header + offset, strlen(name)))
						int_warn(NO_CARET, "partial match against column %d header \"%s\"", j+1, GpDf.df_column[j].header);
				}
			}
		}
		gpfree_string(&a);
	}
	else
		column = (int)a.Real();
	if(column == -3) /* pseudocolumn -3 means "last column" */
		column = GpDf.df_no_cols;
	if(column == -2) {
		char temp_string[32];
		sprintf(temp_string, "%d", GpDf.df_current_index);
		GpGg.Ev.Push(Gstring(&a, temp_string));
	}
	else if(column == -1) {
		char temp_string[32];
		sprintf(temp_string, "%d", GpDf.line_count);
		GpGg.Ev.Push(Gstring(&a, temp_string));
	}
	else if(column == 0) {     /* $0 = df_datum */
		char temp_string[32];
		sprintf(temp_string, "%d", GpDf.df_datum);
		GpGg.Ev.Push(Gstring(&a, temp_string));
	}
	else if(column < 1 || column > GpDf.df_no_cols) {
		GpGg.Ev.undefined = true;
		GpGg.Ev.Push(&a);       /* any objection to this ? */
	}
	else {
		char * temp_string = GpDf.DfParseStringField(GpDf.df_column[column-1].position);
		GpGg.Ev.Push(Gstring(&a, temp_string ? temp_string : ""));
		free(temp_string);
	}
}

/*{{{  static int check_missing(s) */
//static int check_missing(char * s)
int GpDatafile::CheckMissing(char * s)
{
	if(missing_val) {
		size_t len = strlen(missing_val);
		if(strncmp(s, missing_val, len) == 0 && (isspace((uchar)s[len]) || !s[len]))
			return 1;  /* store undefined point in plot */
	}
	/* April 2013 - Treat an empty csv field as "missing" */
	return (df_separators && strchr(df_separators, *s)) ? 1 : 0;
}

/*}}} */

/* formerly in misc.c, but only used here */
/* check user defined format strings for valid double conversions */
/* HBB 20040601: Added check that the number of format specifiers is
 * workable (between 0 and 7) */
static bool valid_format(const char * format)
{
	int formats_found = 0;
	if(!format)
		return false;
	else {
		for(;;) {
			if(!(format = strchr(format, '%'))) /* look for format spec  */
				return (formats_found > 0 && formats_found <= 7);
			else {
				// Found a % to check --- scan past option specifiers:
				do {
					format++;
				} while(strchr("+-#0123456789.", *format));
				// Now at format modifier
				switch(*format) {
					case '*': // Ignore '*' statements
					case '%': // Char   '%' itself
						format++;
						continue;
					case 'l': // Now we found it !!!
						if(!strchr("fFeEgG", format[1])) // looking for a valid format
							return false;
						else {
							formats_found++;
							format++;
						}
						break;
					default:
						return false;
				}
			}
		}
	}
}
//
// Plotting routines can call this prior to invoking df_readline() to indicate
// that they expect a certain column to contain an ascii string rather than a number.
//
//int expect_string(const char column)
int GpDatafile::ExpectString(const char column)
{
	use_spec[column-1].expected_type = CT_STRING;
	//
	// Nasty hack to make 'plot "file" using "A":"B":"C" with labels' work.
	// The case of named columns is handled by create_call_column_at(),
	// which fakes an action table as if '(column("string"))' was written
	// in the using spec instead of simply "string". In this specific case, however,
	// we need the values as strings - so we change the action table to call
	// f_stringcolumn() instead of f_column
	//
	if(use_spec[column-1].at && (use_spec[column-1].at->a_count == 2) && use_spec[column-1].at->actions[1].Index == COLUMN)
		use_spec[column-1].at->actions[1].Index = STRINGCOLUMN;
	return use_spec[column-1].column;
}
//
// Load plot title for key box from the string found earlier by df_readline.
// Called from get_data().
//
void GpDatafile::DfSetKeyTitle(CurvePoints * plot)
{
	if(df_key_title) {
		if(plot->plot_style == HISTOGRAMS && GpGg.histogram_opts.type == HT_STACKED_IN_TOWERS) {
			/* In this case it makes no sense to treat key titles in the usual */
			/* way, so we assume that it is supposed to be an xtic label.      */
			/* FIXME EAM - This style should default to notitle!               */
			double xpos = plot->histogram_sequence + plot->histogram->start;
			add_tic_user(&GpGg[FIRST_X_AXIS], df_key_title, xpos, -1);
			ZFREE(df_key_title);
		}
		else if(plot->title && !plot->title_is_filename) { // What if there was already a title specified?
			int columnhead;
			char * placeholder = strstr(plot->title, "@COLUMNHEAD");
			while(placeholder) {
				char * newtitle = (char *)malloc(strlen(plot->title) + df_longest_columnhead);
				char * trailer = NULL;
				columnhead = strtol(placeholder+11, &trailer, 0);
				*placeholder = '\0';
				if(trailer && *trailer == '@')
					trailer++;
				sprintf(newtitle, "%s%s%s", plot->title, (columnhead <= 0) ? df_key_title : (columnhead <= df_no_cols) ? df_column[columnhead-1].header : "", trailer ? trailer : "");
				free(plot->title);
				plot->title = newtitle;
				placeholder = strstr(newtitle, "@COLUMNHEAD");
			}
		}
		else if(!plot->title_is_suppressed) {
			free(plot->title);
			plot->title_no_enhanced = !GpGg.keyT.enhanced;
			plot->title = df_key_title;
			df_key_title = NULL;
		}
	}
}
/*
 * Load plot title for key box from columnheader.
 * Called from eval_plots(), eval_3dplots() while parsing the plot title option
 */
void GpDatafile::DfSetKeyTitleColumnHead(GpCommand & rC, CurvePoints * plot)
{
	rC.CToken++;
	if(rC.Eq("(")) {
		rC.CToken++;
		column_for_key_title = rC.IntExpression();
		rC.CToken++;
	}
	else if(!rC.EndOfCommand() && rC.IsANumber(rC.CToken)) {
		column_for_key_title = rC.IntExpression();
	}
	else {
		const size_t column_idx = (df_no_use_specs == 1) ? 0 : ((plot->plot_type == DATA3D) ? 2 : 1);
		column_for_key_title = use_spec[column_idx].column;
	}
	// This results from  plot 'foo' using (column("name")) title columnhead
	if(column_for_key_title == NO_COLUMN_HEADER)
		plot->title = gp_strdup("@COLUMNHEAD-1@");
}

char * GpDatafile::DfParseStringField(char * field)
{
	char * temp_string = 0;;
	if(field) {
		int    length;
		if(*field == '"') {
			field++;
			length = strcspn(field, "\"");
		}
		else if(df_separators != NULL) {
			length = strcspn(field, df_separators);
			if(length > (int)strcspn(field, "\"")) /* Why? */
				length = strcspn(field, "\"");
		}
		else {
			length = strcspn(field, "\t ");
		}
		// If we are fed a file with unrecognized line termination then 
		// memory use can become excessive. Truncate and report error.  
		if(length > MAX_LINE_LEN) {
			length = MAX_LINE_LEN;
			int_warn(NO_CARET, "input file contains very long line with no separators, truncating");
			if(strcspn(field, "\r") < MAX_LINE_LEN)
				GpGg.IntError(GpC, NO_CARET, "      line contains embedded <CR>, wrong file format?");
		}
		temp_string = (char *)malloc(length+1);
		strncpy(temp_string, field, length);
		temp_string[length] = '\0';
		parse_esc(temp_string);
	}
	return temp_string;
}

void GpDatafile::AddKeyEntry(char * temp_string, int df_datum)
{
	GpTextLabel * new_entry = (GpTextLabel *)malloc(sizeof(GpTextLabel));
	// Associate this key list with the histogram it belongs to
	if(!df_current_plot->labels) {
		// The first GpTextLabel structure in the list is a place-holder 
		df_current_plot->labels = (GpTextLabel *)malloc(sizeof(GpTextLabel));
		memzero(df_current_plot->labels, sizeof(GpTextLabel));
		df_current_plot->labels->tag  = -1;
	}
	new_entry->text = gp_strdup(temp_string);
	new_entry->tag = df_datum;
	new_entry->font = NULL;
	new_entry->next = df_current_plot->labels->next;
	df_current_plot->labels->next = new_entry;
}

/* Construct 2D rotation matrix. */
/* R - Matrix to construct. */
/* alpha - Rotation angle. */
/* return - true means a translation is required. */
bool rotation_matrix_2D(double R[][2], double alpha)
{
	static double I[2][2] = {{1, 0}, {0, 1}};
#define ANGLE_TOLERANCE 0.001
	if(fabs(alpha) < ANGLE_TOLERANCE) {
		// Zero angle.  Unity rotation.
		memcpy(R, I, sizeof(I));
		return false;
	}
	else {
		R[0][0] = cos(alpha);
		R[0][1] = -sin(alpha);
		R[1][0] = sin(alpha);
		R[1][1] = cos(alpha);
		return true;
	}
}

/* Construct 3D rotation matrix. */
/* P - Matrix to construct. */
/* p - Pointer to perpendicular vector. */
/* return - true means a translation is required. */
bool rotation_matrix_3D(double P[][3], double * p)
{
	static double I[3][3] = {{1, 0, 0},
				 {0, 1, 0},
				 {0, 0, 1}};
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
		return false;
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
		return true;
	}
#undef x
#undef y
#undef z
}

df_byte_read_order_type byte_read_order(df_endianess_type file_endian)
{
	/* Range limit file endianess to ensure that future file type function
	 * programmer doesn't incorrectly access array and cause segmentation
	 * fault unknowingly.
	 */
	return (df_byte_read_order_type)df_byte_read_order_map[THIS_COMPILER_ENDIAN][MIN(file_endian, DF_ENDIAN_TYPE_LENGTH-1)];
}

void GpDatafile::DfUnsetDatafileBinary()
{
	ClearBinaryRecords(DF_DEFAULT_RECORDS);
	df_bin_filetype_default = df_bin_filetype_reset;
	df_bin_file_endianess_default = DF_BIN_FILE_ENDIANESS_RESET;
}

void GpDatafile::DfSetDatafileBinary(GpCommand & rC)
{
	rC.CToken++;
	if(rC.EndOfCommand())
		GpGg.IntError(rC, rC.CToken, "option expected");
	ClearBinaryRecords(DF_CURRENT_RECORDS);
	// Set current records to default in order to retain current default settings.
	if(df_bin_record_default) {
		df_bin_filetype = df_bin_filetype_default;
		df_bin_file_endianess = df_bin_file_endianess_default;
		DfAddBinaryRecords(df_num_bin_records_default, DF_CURRENT_RECORDS);
		memcpy(df_bin_record, df_bin_record_default, df_num_bin_records*sizeof(df_binary_file_record_struct));
	}
	else {
		df_bin_filetype = df_bin_filetype_reset;
		df_bin_file_endianess = DF_BIN_FILE_ENDIANESS_RESET;
		DfAddBinaryRecords(1, DF_CURRENT_RECORDS);
	}
	/* Process the binary tokens. */
	DfSetPlotMode(MODE_QUERY);
	PlotOptionBinary(rC, false, true);
	// Copy the modified settings as the new default settings
	df_bin_filetype_default = df_bin_filetype;
	df_bin_file_endianess_default = df_bin_file_endianess;
	ClearBinaryRecords(DF_DEFAULT_RECORDS);
	DfAddBinaryRecords(df_num_bin_records, DF_DEFAULT_RECORDS);
	memcpy(df_bin_record_default, df_bin_record, df_num_bin_records_default*sizeof(df_binary_file_record_struct));
}

void gpbin_filetype_function(void)
{
	/* Gnuplot binary. */
	df_matrix_file = true;
	df_binary_file = true;
}

void raw_filetype_function(void)
{
	/* No information in file, just data. */
	df_matrix_file = false;
	df_binary_file = true;
}

// static
void GpDatafile::AvsFiletypeFunction()
{
	// A very simple file format:
	// 8 byte header (width and height, 4 bytes each), unknown endian
	// followed by 4 bytes per pixel (alpha, red, green, blue).
	//
	ulong M, N;
	int read_order = 0;
	// open (header) file
	FILE * fp = loadpath_fopen(GpDf.df_filename, "rb");
	if(!fp)
		os_error(NO_CARET, "Can't open data file \"%s\"", GpDf.df_filename);

	/* read header: it is only 8 bytes */
	if(!fread(&M, 4, 1, fp))
		os_error(NO_CARET, "Can't read first dimension in data file \"%s\"", GpDf.df_filename);
	if(M > 0xFFFF)
		read_order = DF_3210;
	df_swap_bytes_by_endianess((char*)&M, read_order, 4);
	if(!fread(&N, 4, 1, fp))
		os_error(NO_CARET, "Can't read second dimension in data file \"%s\"", GpDf.df_filename);
	df_swap_bytes_by_endianess((char*)&N, read_order, 4);

	fclose(fp);

	GpDf.df_matrix_file = false;
	GpDf.df_binary_file = true;

	GpDf.df_bin_record[0].scan_skip[0] = 8;
	GpDf.df_bin_record[0].scan_dim[0] = M;
	GpDf.df_bin_record[0].scan_dim[1] = N;

	GpDf.df_bin_record[0].scan_dir[0] = 1;
	GpDf.df_bin_record[0].scan_dir[1] = -1;
	GpDf.df_bin_record[0].scan_generate_coord = true;
	GpDf.df_bin_record[0].cart_scan[0] = DF_SCAN_POINT;
	GpDf.df_bin_record[0].cart_scan[1] = DF_SCAN_LINE;

	/* The four components are 1 byte each. Permute ARGB to RGBA */
	GpDf.DfExtendBinaryColumns(4);
	GpDf.DfSetReadType(1, DF_UCHAR);
	GpDf.DfSetReadType(2, DF_UCHAR);
	GpDf.DfSetReadType(3, DF_UCHAR);
	GpDf.DfSetReadType(4, DF_UCHAR);
	GpDf.DfSetSkipBefore(1, 0);

	GpDf.df_no_use_specs = 4;
	GpDf.use_spec[0].column = 2;
	GpDf.use_spec[1].column = 3;
	GpDf.use_spec[2].column = 4;
	GpDf.use_spec[3].column = 1;
}

//static void initialize_binary_vars()
void GpDatafile::InitializeBinaryVars()
{
	/* Initialize for the df_readline() routine. */
	df_bin_record_count = 0;
	df_M_count = df_N_count = df_O_count = 0;

	/* Set default binary data widths and skip paratemers. */
	df_no_bin_cols = 0;
	DfSetSkipBefore(1, 0);

	/* Copy the default binary records to the active binary records.  The number
	 * of records will always be at least one in case "record", "array",
	 * or "filetype" are not issued by the user.
	 */
	ClearBinaryRecords(DF_CURRENT_RECORDS);
	if(df_num_bin_records_default) {
		df_bin_filetype = df_bin_filetype_default;
		df_bin_file_endianess = df_bin_file_endianess_default;
		DfAddBinaryRecords(df_num_bin_records_default, DF_CURRENT_RECORDS);
		memcpy(df_bin_record, df_bin_record_default, df_num_bin_records*sizeof(df_binary_file_record_struct));
	}
	else {
		df_bin_filetype = df_bin_filetype_reset;
		df_bin_file_endianess = DF_BIN_FILE_ENDIANESS_RESET;
		DfAddBinaryRecords(1, DF_CURRENT_RECORDS);
	}
}

static char * too_many_cols_msg = "Too many columns in using specification and implied sampling array";

/* Place a special marker in the using list to derive the x/y/z value
 * from the appropriate dimensional counter.
 */
//void df_insert_scanned_use_spec(int uspec)
void GpDatafile::DfInsertScannedUseSpec(int uspec)
{
	/* Place a special marker in the using list to derive the z value
	 * from the third dimensional counter, which will be zero.
	 */
	if(df_no_use_specs >= MAXDATACOLS)
		GpGg.IntError(GpC, NO_CARET, too_many_cols_msg);
	else {
		int j;
		for(j = df_no_use_specs; j > uspec; j--)
			use_spec[j] = use_spec[j - 1];
		use_spec[uspec].column = (uspec == 2 ? DF_SCAN_PLANE : DF_SCAN_LINE);
		/* The at portion is set to NULL here, but this doesn't mash
		 * a valid memory pointer because any valid memory pointers
		 * were copied to new locations in the previous for loop.
		 */
		use_spec[uspec].at = NULL; /* Not a bad memory pointer overwrite!! */
		df_no_use_specs++;
	}
}

/* Not the most elegant way of defining the default columns, but I prefer
 * this to switch and conditional statements when there are so many styles.
 */
typedef struct df_bin_default_columns {
	PLOT_STYLE plot_style;
	short excluding_gen_coords; /* Number of columns of information excluding generated coordinates. */
	short dimen_in_2d;      /* Number of additional columns required (in 2D plot) if coordinates not generated. */
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
	{RGBA_IMAGE, 4, 2}
#ifdef EAM_OBJECTS
	, {CIRCLES, 2, 1}
	, {ELLIPSES, 2, 3}
#endif
	, {TABLESTYLE, 0, 0}
};

/* FIXME!!!
 * EAM Feb 2008:
 * This whole routine is a disaster.  It makes so many broken assumptions it's not funny.
 * Other than filling in the first two columns of an implicit matrix, I suspect we can
 * do away with it altogether. Frankly, we _don't care_ how many columns there are,
 * so long as the ones that are present are mapped to the right ordering.
 */
//static void adjust_binary_use_spec(CurvePoints * plot)
void GpDatafile::AdjustBinaryUseSpec(CurvePoints * plot)
{
	char * nothing_known = "No default columns known for that plot style";
	uint ps_index;
	enum PLOT_STYLE plot_style = plot ? plot->plot_style : LINES;

	/* The default binary matrix format is nonuniform, i.e.
	 * it has an extra row and column for sample coordinates.
	 */
	if(df_matrix_file && df_binary_file)
		df_nonuniform_matrix = true;

	/* Determine index. */
	for(ps_index = 0; ps_index < sizeof(default_style_cols)/sizeof(default_style_cols[0]); ps_index++) {
		if(default_style_cols[ps_index].plot_style == plot_style)
			break;
	}
	if(ps_index == sizeof(default_style_cols)/sizeof(default_style_cols[0]))
		GpGg.IntError(GpC, NO_CARET, nothing_known);

	/* Matrix format is interpretted as always having three columns. */
	if(df_matrix_file) {
		if(df_no_bin_cols > 3)
			GpGg.IntError(GpC, NO_CARET, "Matrix data contains only three columns");
		DfExtendBinaryColumns(3);
	}
	/* If nothing has been done to set the using specs, use the default using
	 * characteristics for the style.
	 */
	if(!df_no_use_specs) {
		if(!df_matrix_file) {
			int no_cols = default_style_cols[ps_index].excluding_gen_coords;
			if(!no_cols)
				GpGg.IntError(GpC, NO_CARET, nothing_known);

			/* If coordinates are generated, make sure this plot style allows it.
			 * Otherwise, add in the number of generated coordinates and add an
			 * extra column if using `splot`.
			 */
			if(df_num_bin_records && df_bin_record[0].scan_generate_coord) {
				if(default_style_cols[ps_index].dimen_in_2d == 0)
					GpGg.IntError(GpC, NO_CARET, "Cannot generate coords for that plot style");
			}
			else {
				/* If there aren't generated coordinates, then add the
				 * amount of columns that would be generated.
				 */
				no_cols += default_style_cols[ps_index].dimen_in_2d;
				if(df_plot_mode == MODE_SPLOT)
					no_cols++;
			}

			assert(no_cols <= MAXDATACOLS);

			/* Nothing need be done here to set the using specs because they
			 * will have been initialized appropriately and left unaltered.
			 * So just set the number of specs.
			 */
			df_no_use_specs = no_cols;
			DfExtendBinaryColumns(no_cols);
		}
		else {
			/* Number of columns is fixed at three and no using specs given.  Do what we can.
			 * The obvious best combination is two dimensional coordinates and one information
			 * value.  One wonders what to do if a matrix is only one column; can be treated
			 * as linear?  This isn't implemented here, but if it were, this is where it
			 * should go.
			 */

			if((default_style_cols[ps_index].dimen_in_2d == 2) && (default_style_cols[ps_index].excluding_gen_coords == 1)) {
				df_no_use_specs = 3;
			}
			else if((default_style_cols[ps_index].dimen_in_2d == 1) && (default_style_cols[ps_index].excluding_gen_coords == 1) ) {
				if(df_plot_mode == MODE_SPLOT)
					df_no_use_specs = 3;
				else {
					/* Command:  plot 'foo' matrix       with no using spec */
					/* Matix element treated as y value rather than z value */
					df_no_use_specs = 2;
					use_spec[1].column = 3;
				}
			}
			else
				GpGg.IntError(GpC, NO_CARET, "Plot style does not conform to three column data in this graph mode");
		}
	}

	if(df_num_bin_records && df_bin_record[0].scan_generate_coord && !df_matrix_file) {
		int i;
		UseSpecS original_use_spec[MAXDATACOLS];
		int added_columns = 0;
		// Keep record of the original using specs
		memcpy(original_use_spec, use_spec, sizeof(use_spec));
		// Put in columns at front for generated variables
		for(i = 0; i < 3; i++) {
			if(df_bin_record[0].cart_dim[i] || df_bin_record[0].scan_dim[i])
				added_columns++;
			else
				break;
		}
		if((df_no_use_specs + added_columns) >= MAXDATACOLS)
			GpGg.IntError(GpC, NO_CARET, too_many_cols_msg);
		else {
			// Shift the original columns over by added number of columns, but only if not matrix data.
			memcpy(&use_spec[added_columns], original_use_spec, df_no_use_specs*sizeof(use_spec[0]));
			/* The at portion is set to NULL here, but this doesn't mash
			 * a valid memory pointer because any valid memory pointers
			 * were copied to new locations in the previous memcpy().
			 */
			for(i = 0; i < added_columns; i++) {
				use_spec[i].column = df_bin_record[0].cart_scan[i];
				use_spec[i].at = NULL; /* Not a bad memory pointer overwrite!! */
			}
			df_no_use_specs += added_columns; /* Do not extend columns for generated coordinates. */
		}
		if(df_plot_mode == MODE_SPLOT) {
			/* For binary data having an implied uniformly sampled grid, treat
			 * less than three-dimensional data in special ways based upon what
			 * is being plotted.
			 */
			for(int k = 0; k < df_num_bin_records; k++) {
				if((df_bin_record[k].cart_dim[2] == 0) && (df_bin_record[k].scan_dim[2] == 0)) {
					if(default_style_cols[ps_index].dimen_in_2d > 2)
						GpGg.IntError(GpC, NO_CARET, "Plot style requires higher than two-dimensional sampling array");
					else {
						if((df_bin_record[k].cart_dim[1] == 0) && (df_bin_record[k].scan_dim[1] == 0)) {
							if(default_style_cols[ps_index].dimen_in_2d > 1)
								GpGg.IntError(GpC, NO_CARET, "Plot style requires higher than one-dimensional sampling array");
							else {
								/* Place a special marker in the using list to derive
								   the y value
								 * from the second dimensional counter.
								 */
								DfInsertScannedUseSpec(1);
							}
						}
						/* Place a special marker in the using list to derive the z value
						 * from the third dimensional counter.
						 */
						DfInsertScannedUseSpec(2);
					}
				}
			}
		}
	}
}

static const char * equal_symbol_msg = "Equal ('=') symbol required";

void GpDatafile::PlotOptionBinary(GpCommand & rC, bool set_matrix, bool set_default)
{
	bool duplication = false;
	bool set_record = false;
	bool set_array = false;
	bool set_dx = false;
	bool set_dy = false;
	bool set_dz = false;
	bool set_center = false;
	bool set_origin = false;
	bool set_skip = false;
	bool set_endian = false;
	bool set_rotation = false;
	bool set_perpendicular = false;
	bool set_flip = false;
	bool set_noflip = false;
	bool set_flipx = false;
	bool set_flipy = false;
	bool set_flipz = false;
	bool set_scan = false;
	bool set_format = false;
	// Binary file type must be the first word in the command following `binary`" 
	if(df_bin_filetype_default >= 0)
		df_bin_filetype = df_bin_filetype_default;
	if(rC.AlmostEq("file$type") || (df_bin_filetype >= 0)) {
		int i;
		char file_ext[8] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
		// Above keyword not part of pre-existing binary definition.
		// So use general binary.
		if(set_matrix)
			GpGg.IntError(rC, rC.CToken, matrix_general_binary_conflict_msg);
		df_matrix_file = false;
		if(rC.AlmostEq("file$type")) {
			if(!rC.Eq(++rC.CToken, "="))
				GpGg.IntError(rC, rC.CToken, equal_symbol_msg);
			rC.CopyStr(file_ext, ++rC.CToken, 8);
			for(i = 0; df_bin_filetype_table[i].key; i++)
				if(!strcasecmp(file_ext, df_bin_filetype_table[i].key)) {
					binary_input_function = df_bin_filetype_table[i].value;
					df_bin_filetype = i;
					break;
				}
			if(df_bin_filetype != i) // Maybe set to "auto" and continue? 
				GpGg.IntError(rC, rC.CToken, "Unrecognized filetype; try \"show datafile binary filetypes\"");
			rC.CToken++;
		}
		if(df_plot_mode != MODE_QUERY && !strcmp("auto", df_bin_filetype_table[df_bin_filetype].key)) {
			int i;
			char * file_ext = strrchr(df_filename, '.');
			if(file_ext++) {
				for(i = 0; df_bin_filetype_table[i].key; i++)
					if(!strcasecmp(file_ext, df_bin_filetype_table[i].key))
						binary_input_function = df_bin_filetype_table[i].value;
			}
			if(binary_input_function == auto_filetype_function)
				GpGg.IntError(GpC, NO_CARET, "Unrecognized filename extension; try \"show datafile binary filetypes\"");
		}
		// Unless only querying settings, call the routine to prep binary data parameters.
		if(df_plot_mode != MODE_QUERY) {
			(*binary_input_function)();
			df_xpixels = df_bin_record[0].scan_dim[0];
			df_ypixels = df_bin_record[0].scan_dim[1];
			FPRINTF((stderr, "datafile.c:%d  image dimensions %d x %d\n", __LINE__, df_xpixels, df_ypixels));
		}
		// Now, at this point anything that was filled in for "scan" should
		// override the "cart" variables.
		for(i = 0; i < df_num_bin_records; i++) {
			int j;
			/* Dimension */
			if(df_bin_record[i].scan_dim[0] != df_bin_record_reset.scan_dim[0])
				for(j = 0; j < 3; j++)
					df_bin_record[i].cart_dim[j] = 0;
			/* Delta */
			for(j = 0; j < 3; j++)
				if(df_bin_record[i].scan_delta[j] != 0.0) {
					int k;
					for(k = 0; k < 3; k++)
						if(df_bin_record[i].cart_scan[k] == (DF_SCAN_POINT - j))
							df_bin_record[i].cart_delta[k] = 0;
				}
			/* Translation */
			if(df_bin_record[i].scan_trans != DF_TRANSLATE_DEFAULT)
				df_bin_record[i].cart_trans = DF_TRANSLATE_DEFAULT;
		}
	}
	while(!rC.EndOfCommand() && !duplication) {
		const char origin_and_center_conflict_message[] = "Can specify `origin` or `center`, but not both";
		if(rC.AlmostEq("rec$ord")) { // look for record 
			if(set_record)
				duplication = true; 
			else {
				rC.CToken++;
				// Above keyword not part of pre-existing binary definition.  So use general binary. 
				if(set_matrix)
					GpGg.IntError(rC, rC.CToken, matrix_general_binary_conflict_msg);
				df_matrix_file = false;
				PlotOptionArray(rC);
				set_record = true;
				df_xpixels = df_bin_record[df_num_bin_records - 1].cart_dim[1];
				df_ypixels = df_bin_record[df_num_bin_records - 1].cart_dim[0];
				FPRINTF((stderr, "datafile.c:%d  record dimensions %d x %d\n", __LINE__, df_xpixels, df_ypixels));
			}
		}
		else if(rC.AlmostEq("arr$ay")) { // look for array 
			if(set_array) 
				duplication = true; 
			else {
				rC.CToken++;
				// Above keyword not part of pre-existing binary definition.  So use general binary. 
				if(set_matrix)
					GpGg.IntError(rC, rC.CToken, matrix_general_binary_conflict_msg);
				df_matrix_file = false;
				PlotOptionArray(rC);
				for(int i = 0; i < df_num_bin_records; i++) {
					// Indicate that GpCoordinate info should be generated internally 
					df_bin_record[i].scan_generate_coord = true;
				}
				set_array = true;
				df_xpixels = df_bin_record[df_num_bin_records - 1].cart_dim[0];
				df_ypixels = df_bin_record[df_num_bin_records - 1].cart_dim[1];
				FPRINTF((stderr, "datafile.c:%d  array dimensions %d x %d\n", __LINE__, df_xpixels, df_ypixels));
			}
		}
		else if(rC.Eq("dx") || rC.Eq("dt")) { // deal with spacing between array points 
			if(set_dx)
				duplication = true; 
			else {
				rC.CToken++;
				PlotOptionMultiValued(rC, DF_DELTA, 0);
				if(!set_dy) {
					for(int i = 0; i < df_num_bin_records; i++)
						df_bin_record[i].cart_delta[1] = df_bin_record[i].cart_delta[0];
				}
				if(!set_dz) {
					for(int i = 0; i < df_num_bin_records; i++)
						df_bin_record[i].cart_delta[2] = df_bin_record[i].cart_delta[0];
				}
				set_dx = true;
			}
		}
		else if(rC.Eq("dy") || rC.Eq("dr")) {
			if(set_dy)
				duplication = true; 
			else {
				if(!set_array && !df_bin_record)
					GpGg.IntError(rC, rC.CToken, "Must specify a sampling array size before indicating spacing in second dimension");
				rC.CToken++;
				PlotOptionMultiValued(rC, DF_DELTA, 1);
				if(!set_dz) {
					for(int i = 0; i < df_num_bin_records; i++)
						df_bin_record[i].cart_delta[2] = df_bin_record[i].cart_delta[1];
				}
				set_dy = true;
			}
		}
		else if(rC.Eq("dz")) {
			GpGg.IntError(rC, rC.CToken, "Currently not supporting three-dimensional sampling");
			if(set_dz)
				duplication = true; 
			else {
				if(!set_array && !df_bin_record)
					GpGg.IntError(rC, rC.CToken, "Must specify a sampling array size before indicating spacing in third dimension");
				rC.CToken++;
				PlotOptionMultiValued(rC, DF_DELTA, 2);
				set_dz = true;
			}
		}
		else if(rC.Eq("flipx")) { // deal with direction in which sampling increments 
			if(set_flipx) {
				duplication = true; 
			}
			else {
				rC.CToken++;
				// If no equal sign, then set flip true for all records
				if(!rC.Eq("=")) {
					for(int i = 0; i < df_num_bin_records; i++)
						df_bin_record[i].cart_dir[0] = -1;
				}
				else {
					PlotOptionMultiValued(rC, DF_FLIP_AXIS, 0);
				}
				set_flipx = true;
			}
		}
		else if(rC.Eq("flipy")) {
			if(set_flipy)
				duplication = true;
			else {
				if(!set_array && !df_bin_record)
					GpGg.IntError(rC, rC.CToken, "Must specify a sampling array size before indicating flip in second dimension");
				rC.CToken++;
				// If no equal sign, then set flip true for all records. 
				if(!rC.Eq("=")) {
					for(int i = 0; i < df_num_bin_records; i++)
						df_bin_record[i].cart_dir[1] = -1;
				}
				else
					PlotOptionMultiValued(rC, DF_FLIP_AXIS, 1);
				set_flipy = true;
			}
		}
		else if(rC.Eq("flipz")) {
			GpGg.IntError(rC, rC.CToken, "Currently not supporting three-dimensional sampling");
			if(set_flipz) {
				duplication = true;
			}
			else {
				if(!set_array && !df_bin_record)
					GpGg.IntError(rC, rC.CToken, "Must specify a sampling array size before indicating spacing in third dimension");
				rC.CToken++;
				// If no equal sign, then set flip true for all records. 
				if(!rC.Eq("=")) {
					for(int i = 0; i < df_num_bin_records; i++)
						df_bin_record[i].cart_dir[2] = -1;
				}
				else {
					PlotOptionMultiValued(rC, DF_FLIP_AXIS, 2);
				}
				set_flipz = true;
			}
		}
		else if(rC.Eq("flip")) { // Deal with flipping data for individual records.
			if(set_flip) {
				duplication = true;
			}
			else {
				rC.CToken++;
				PlotOptionMultiValued(rC, DF_FLIP, -1);
				set_flip = true;
			}
		}
		else if(rC.Eq("noflip")) { // Deal with flipping data for individual records. 
			if(set_noflip) {
				duplication = true; 
			}
			else {
				rC.CToken++;
				PlotOptionMultiValued(rC, DF_FLIP, 1);
				set_noflip = true;
			}
		}
		else if(rC.Eq("scan")) { // Deal with manner in which dimensions are scanned from file. 
			if(set_scan) {
				duplication = true; 
			}
			else {
				rC.CToken++;
				if(rC.AlmostEq(rC.CToken+1, "yx$z"))
					df_transpose = true;
				PlotOptionMultiValued(rC, DF_SCAN, 0);
				set_scan = true;
			}
		}
		else if(rC.AlmostEq("trans$pose")) { // Deal with manner in which dimensions are scanned from file. 
			int i;
			if(set_scan) {
				duplication = true; 
			}
			else {
				rC.CToken++;
				for(i = 0; i < df_num_bin_records; i++)
					memcpy(df_bin_record[i].cart_scan, df_bin_scan_table_2D[TRANSPOSE_INDEX].scan, sizeof(df_bin_record[0].cart_scan));
				set_scan = true;
				df_transpose = true;
			}
		}
		else if(rC.AlmostEq("orig$in")) { // deal with origin 
			if(set_center)
				GpGg.IntError(rC, rC.CToken, origin_and_center_conflict_message);
			if(set_origin) {
				duplication = true;
			}
			else {
				rC.CToken++;
				PlotOptionMultiValued(rC, DF_ORIGIN, df_plot_mode);
				set_origin = true;
			}
		}
		else if(rC.AlmostEq("cen$ter")) { // deal with origin 
			if(set_origin)
				GpGg.IntError(rC, rC.CToken, origin_and_center_conflict_message);
			if(set_center) {
				duplication = true;
			}
			else {
				rC.CToken++;
				PlotOptionMultiValued(rC, DF_CENTER, df_plot_mode);
				set_center = true;
			}
		}
		else if(rC.AlmostEq("rot$ation") || rC.AlmostEq("rot$ate")) { // deal with rotation angle 
			if(set_rotation) {
				duplication = true; 
			}
			else {
				rC.CToken++;
				PlotOptionMultiValued(rC, DF_ROTATION, 0);
				set_rotation = true;
			}
		}
		else if(rC.AlmostEq("perp$endicular")) { // deal with rotation angle 
			if(df_plot_mode == MODE_PLOT)
				GpGg.IntError(rC, rC.CToken, "Key word `perpendicular` is not allowed with `plot` command");
			if(set_perpendicular) {
				duplication = true; 
			}
			else {
				rC.CToken++;
				PlotOptionMultiValued(rC, DF_PERPENDICULAR, 0);
				set_perpendicular = true;
			}
		}
		else if(rC.AlmostEq("skip")) { // deal with number of bytes to skip before record 
			if(set_skip) {
				duplication = true;
			}
			else {
				rC.CToken++;
				PlotOptionMultiValued(rC, DF_SKIP, 0);
				set_skip = true;
			}
		}
		else if(rC.AlmostEq("end$ian")) { // deal with byte order 
			if(set_endian) {
				duplication = true; 
			}
			else {
				rC.CToken++;
				// Require equal symbol. 
				if(!rC.Eq("="))
					GpGg.IntError(rC, rC.CToken, equal_symbol_msg);
				rC.CToken++;
				if(rC.AlmostEq("def$ault"))
					df_bin_file_endianess = THIS_COMPILER_ENDIAN;
				else if(rC.Eq("swap") || rC.Eq("swab"))
					df_bin_file_endianess = (df_endianess_type)((~df_bin_file_endianess)&3); // complement and isolate lowest two bits 
				else if(rC.AlmostEq("lit$tle"))
					df_bin_file_endianess = DF_LITTLE_ENDIAN;
				else if(rC.Eq("big"))
					df_bin_file_endianess = DF_BIG_ENDIAN;
#if SUPPORT_MIDDLE_ENDIAN
				else if(rC.AlmostEq("mid$dle") || rC.Eq("pdp"))
					df_bin_file_endianess = DF_PDP_ENDIAN;
				else
					GpGg.IntError(rC, rC.CToken, "Options are default, swap (swab), little, big, middle (pdp)");
#else
				else
					GpGg.IntError(rC, rC.CToken, "Options are default, swap (swab), little, big");
#endif
				rC.CToken++;
				set_endian = true;
			}
		}
		else if(rC.AlmostEq("form$at")) { // deal with various types of binary files 
			if(set_format) {
				duplication = true; 
			}
			else {
				rC.CToken++;
				// Format string not part of pre-existing binary definition.  So use general binary.
				if(set_matrix)
					GpGg.IntError(rC, rC.CToken, matrix_general_binary_conflict_msg);
				df_matrix_file = false;
				// Require equal sign 
				if(!rC.Eq("="))
					GpGg.IntError(rC, rC.CToken, equal_symbol_msg);
				rC.CToken++;
				if(set_default) {
					free(df_binary_format);
					df_binary_format = rC.TryToGetString();
				}
				else {
					char * format_string = rC.TryToGetString();
					if(!format_string)
						GpGg.IntError(rC, rC.CToken, "missing format string");
					PlotOptionBinaryFormat(format_string);
					free(format_string);
				}
				set_format = true;
			}
		}
		else { // unknown option 
			break; 
		}
	}
	if(duplication)
		GpGg.IntError(rC, rC.CToken, "Duplicated or contradicting arguments in datafile options");
	if(!set_default && !set_matrix && df_num_bin_records_default) {
		int_warn(NO_CARET, "using default binary record/array structure");
	}
	if(!set_format && !df_matrix_file) {
		if(df_binary_format) {
			PlotOptionBinaryFormat(df_binary_format);
			int_warn(NO_CARET, "using default binary format");
		}
	}
}

void GpDatafile::DfAddBinaryRecords(int num_records_to_add, df_records_type records_type)
{
	int i;
	int new_number;
	df_binary_file_record_struct ** bin_record;
	int * num_bin_records;
	int * max_num_bin_records;
	if(records_type == DF_CURRENT_RECORDS) {
		bin_record = &df_bin_record;
		num_bin_records = &df_num_bin_records;
		max_num_bin_records = &df_max_num_bin_records;
	}
	else {
		bin_record = &df_bin_record_default;
		num_bin_records = &df_num_bin_records_default;
		max_num_bin_records = &df_max_num_bin_records_default;
	}
	new_number = *num_bin_records + num_records_to_add;
	if(new_number > *max_num_bin_records) {
		*bin_record = (df_binary_file_record_struct *)gp_realloc(*bin_record, new_number * sizeof(df_binary_file_record_struct), "binary file data records");
		*max_num_bin_records = new_number;
	}
	for(i = 0; i < num_records_to_add; i++) {
		memcpy(*bin_record + *num_bin_records, &df_bin_record_reset, sizeof(df_binary_file_record_struct));
		(*num_bin_records)++;
	}
}

//static void clear_binary_records(df_records_type records_type)
void GpDatafile::ClearBinaryRecords(df_records_type records_type)
{
	df_binary_file_record_struct * temp_bin_record;
	int * temp_num_bin_records;
	int i;
	if(records_type == DF_CURRENT_RECORDS) {
		temp_bin_record = df_bin_record;
		temp_num_bin_records = &df_num_bin_records;
	}
	else {
		temp_bin_record = df_bin_record_default;
		temp_num_bin_records = &df_num_bin_records_default;
	}
	for(i = 0; i < *temp_num_bin_records; i++) {
		if(temp_bin_record[i].memory_data != NULL) {
			ZFREE(temp_bin_record[i].memory_data);
		}
	}
	*temp_num_bin_records = 0;
}

/*
 * Syntax is:   array=(xdim,ydim):(xdim,ydim):CONST:(xdim) etc
 */
//static void plot_option_array()
void GpDatafile::PlotOptionArray(GpCommand & rC)
{
	int number_of_records = 0;
	if(!rC.Eq("="))
		GpGg.IntError(rC, rC.CToken, equal_symbol_msg);
	do {
		rC.CToken++;
		// Partial backward compatibility with syntax up to 4.2.4
		if(rC.IsANumber(rC.CToken)) {
			if(++number_of_records > df_num_bin_records)
				DfAddBinaryRecords(1, DF_CURRENT_RECORDS);
			df_bin_record[df_num_bin_records - 1].cart_dim[0] = rC.IntExpression();
			/* Handle the old syntax:  array=123x456 */
			if(!rC.EndOfCommand()) {
				char xguy[8]; int itmp = 0;
				rC.CopyStr(xguy, rC.CToken, 6);
				if(xguy[0] == 'x') {
					sscanf(&xguy[1], "%d", &itmp);
					df_bin_record[df_num_bin_records - 1].cart_dim[1] = itmp;
					rC.CToken++;
				}
			}
		}
		else if(rC.Eq("("))        {
			rC.CToken++;
			if(++number_of_records > df_num_bin_records)
				DfAddBinaryRecords(1, DF_CURRENT_RECORDS);
			df_bin_record[df_num_bin_records - 1].cart_dim[0] = rC.IntExpression();
			if(rC.Eq(",")) {
				rC.CToken++;
				df_bin_record[df_num_bin_records - 1].cart_dim[1] = rC.IntExpression();
			}
			if(!rC.Eq(")"))
				GpGg.IntError(rC, rC.CToken, "tuple syntax error");
			rC.CToken++;
		}
	} while(rC.Eq(":"));
}

/* Evaluate a tuple of up to specified dimension. */
#define TUPLE_SEPARATOR_CHAR ":"
#define LEFT_TUPLE_CHAR "("
#define RIGHT_TUPLE_CHAR ")"

//int token2tuple(double * tuple, int dimension)
int GpCommand::Token2Tuple(double * tuple, int dimension)
{
	if(Eq(LEFT_TUPLE_CHAR)) {
		bool expecting_number = true;
		int N = 0;
		CToken++;
		while(!EndOfCommand()) {
			if(expecting_number) {
				if(++N <= dimension)
					*tuple++ = RealExpression();
				else
					GpGg.IntError(*this, CToken-1, "More than %d elements", N);
				expecting_number = false;
			}
			else if(Eq(",")) {
				CToken++;
				expecting_number = true;
			}
			else if(Eq(RIGHT_TUPLE_CHAR)) {
				CToken++;
				return N;
			}
			else
				GpGg.IntError(*this, CToken, "Expecting ',' or '" RIGHT_TUPLE_CHAR "'");
		}
	}
	return 0; // Not a tuple 
}
//
// Determine the 2D rotational matrix from the "rotation" qualifier. */
//
//void plot_option_multivalued(df_multivalue_type type, int arg)
void GpDatafile::PlotOptionMultiValued(GpCommand & rC, df_multivalue_type type, int arg)
{
	int bin_record_count = 0;
	int test_val;
	// Require equal symbol
	if(!rC.Eq("="))
		GpGg.IntError(rC, rC.CToken, equal_symbol_msg);
	rC.CToken++;
	while(!rC.EndOfCommand()) {
		double tuple[3];
		switch(type) {
			case DF_ORIGIN:
			case DF_CENTER:
			case DF_PERPENDICULAR:
			    test_val = rC.Token2Tuple(tuple, sizeof(tuple)/sizeof(tuple[0]));
			    break;
			case DF_SCAN:
			case DF_FLIP:
			    /* Will check later */
			    test_val = 1;
			    break;
			default: {
			    /* Check if a valid number. */
			    tuple[0] = rC.RealExpression();
			    test_val = 1;
		    }
		}

		if(test_val) {
			char const * cannot_flip_msg = "Cannot flip a non-existent dimension";
			char flip_list[4];
			if(bin_record_count >= df_num_bin_records)
				GpGg.IntError(rC, rC.CToken, "More parameters specified than data records specified");
			switch(type) {
				case DF_DELTA:
				    // Set the spacing between grid points in the specified dimension.
				    *(df_bin_record[bin_record_count].cart_delta + arg) = tuple[0];
				    if(df_bin_record[bin_record_count].cart_delta[arg] <= 0)
					    GpGg.IntError(rC, rC.CToken - 2, "Sample period must be positive. Try `flip` for changing direction");
				    break;
				case DF_FLIP_AXIS:
				    // Set the direction of grid points increment in the specified dimension.
				    if(df_bin_record[bin_record_count].cart_dim[0] != 0) {
					    if(tuple[0] == 0.0)
						    df_bin_record[bin_record_count].cart_dir[arg] = 0;
					    else if(tuple[0] == 1.0)
						    df_bin_record[bin_record_count].cart_dir[arg] = 1;
					    else
						    GpGg.IntError(rC, rC.CToken-1, "Flipping dimension direction must be 1 or 0");
				    }
				    else
					    GpGg.IntError(rC, rC.CToken, cannot_flip_msg);
				    break;
				case DF_FLIP:
				    /* Set the direction of grid points increment in
				    * based upon letters for axes. Check if there are
				    * any characters in string that shouldn't be. */
				    rC.CopyStr(flip_list, rC.CToken, 4);
				    if(strlen(flip_list) != strspn(flip_list, "xXyYzZ"))
					    GpGg.IntError(rC, rC.CToken, "Can only flip x, y, and/or z");
				    /* Check for valid dimensions. */
				    if(strpbrk(flip_list, "xX")) {
					    if(df_bin_record[bin_record_count].cart_dim[0] != 0)
						    df_bin_record[bin_record_count].cart_dir[0] = arg;
					    else
						    GpGg.IntError(rC, rC.CToken, cannot_flip_msg);
				    }
				    if(strpbrk(flip_list, "yY")) {
					    if(df_bin_record[bin_record_count].cart_dim[1] != 0)
						    df_bin_record[bin_record_count].cart_dir[1] = arg;
					    else
						    GpGg.IntError(rC, rC.CToken, cannot_flip_msg);
				    }
				    if(strpbrk(flip_list, "zZ")) {
					    if(df_bin_record[bin_record_count].cart_dim[2] != 0)
						    df_bin_record[bin_record_count].cart_dir[2] = arg;
					    else
						    GpGg.IntError(rC, rC.CToken, cannot_flip_msg);
				    }
				    rC.CToken++;
				    break;
				case DF_SCAN: {
				    // Set the method in which data is scanned from
				    // file.  Compare against a set number of strings. 
				    int i;
				    if(!(df_bin_record[bin_record_count].cart_dim[0] || df_bin_record[bin_record_count].scan_dim[0]) ||
						!(df_bin_record[bin_record_count].cart_dim[1] || df_bin_record[bin_record_count].scan_dim[1]))
					    GpGg.IntError(rC, rC.CToken, "Cannot alter scanning method for one-dimensional data");
				    else if(df_bin_record[bin_record_count].cart_dim[2]
					    || df_bin_record[bin_record_count].scan_dim[2]) {
					    for(i = 0; i < sizeof(df_bin_scan_table_3D)/sizeof(df_bin_scan_table_3D_struct); i++)
						    if(rC.Eq(df_bin_scan_table_3D[i].string)) {
							    memcpy(df_bin_record[bin_record_count].cart_scan, df_bin_scan_table_3D[i].scan, sizeof(df_bin_record[0].cart_scan));
							    break;
						    }
					    if(i == sizeof(df_bin_scan_table_3D) / sizeof(df_bin_scan_table_3D_struct))
						    GpGg.IntError(rC, rC.CToken, "Improper scanning string. Try 3 character string for 3D data");
				    }
				    else {
					    for(i = 0; i < sizeof(df_bin_scan_table_2D)/sizeof(df_bin_scan_table_2D_struct); i++)
						    if(rC.Eq(df_bin_scan_table_2D[i].string)) {
							    memcpy(df_bin_record[bin_record_count].cart_scan, df_bin_scan_table_2D[i].scan, sizeof(df_bin_record[0].cart_scan));
							    break;
						    }
					    if(i == sizeof(df_bin_scan_table_2D) / sizeof(df_bin_scan_table_2D_struct))
						    GpGg.IntError(rC, rC.CToken, "Improper scanning string. Try 2 character string for 2D data");
				    }
				    // Remove the file supplied scan direction. 
				    memcpy(df_bin_record[bin_record_count].scan_dir, df_bin_record_reset.scan_dir, sizeof(df_bin_record[0].scan_dir));
				    rC.CToken++;
				    break;
			    }
				case DF_SKIP:
				    // Set the number of bytes to skip before reading record.
				    df_bin_record[bin_record_count].scan_skip[0] = (off_t)tuple[0];
				    if((df_bin_record[bin_record_count].scan_skip[0] != tuple[0]) || (df_bin_record[bin_record_count].scan_skip[0] < 0))
					    GpGg.IntError(rC, rC.CToken, "Number of bytes to skip must be positive integer");
				    break;
				case DF_ORIGIN:
				case DF_CENTER:
				    // Set the origin or center of the image based upon the plot mode
				    if(type == DF_ORIGIN)
					    df_bin_record[bin_record_count].cart_trans = DF_TRANSLATE_VIA_ORIGIN;
				    else
					    df_bin_record[bin_record_count].cart_trans = DF_TRANSLATE_VIA_CENTER;
				    if(arg == MODE_PLOT) {
					    if(test_val != 2)
						    GpGg.IntError(rC, rC.CToken, "Two-dimensional tuple required for 2D plot");
					    tuple[2] = 0.0;
				    }
				    else if(arg == MODE_SPLOT) {
					    if(test_val != 3)
						    GpGg.IntError(rC, rC.CToken, "Three-dimensional tuple required for 3D plot");
				    }
				    else if(arg == MODE_QUERY) {
					    if(test_val != 3)
						    GpGg.IntError(rC, rC.CToken, "Three-dimensional tuple required for setting binary parameters");
				    }
				    else {
					    GpGg.IntError(rC, rC.CToken, "Internal error (datafile.c): Unknown plot mode");
				    }
				    memcpy(df_bin_record[bin_record_count].cart_cen_or_ori,
				    tuple, sizeof(tuple));
				    break;
				case DF_ROTATION:
				    // Allow user to enter angle in terms of pi or degrees.
				    if(rC.Eq("pi")) {
					    tuple[0] *= M_PI;
					    rC.CToken++;
				    }
				    else if(rC.AlmostEq("d$egrees")) {
					    tuple[0] *= M_PI/180;
					    rC.CToken++;
				    }
				    /* Construct 2D rotation matrix. */
				    df_bin_record[bin_record_count].cart_alpha = tuple[0];
				    break;

				case DF_PERPENDICULAR:
				    // Make sure in three dimensional plotting mode before
				    // accepting the perpendicular vector for translation. 
				    if(test_val != 3)
					    GpGg.IntError(rC, rC.CToken, "Three-dimensional tuple required");
				    // Compare vector length against variable precision
				    // to determine if this is the null vector 
				    if((tuple[0]*tuple[0] + tuple[1]*tuple[1] + tuple[2]*tuple[2]) < (100.0*SMathConst::Epsilon))
					    GpGg.IntError(rC, rC.CToken, "Perpendicular vector cannot be zero");
				    memcpy(df_bin_record[bin_record_count].cart_p,
				    tuple,
				    sizeof(tuple));
				    break;
				default:
				    GpGg.IntError(GpC, NO_CARET, "Internal error: Invalid comma separated type");
			}
		}
		else {
			GpGg.IntError(rC, rC.CToken, "Invalid numeric or tuple form");
		}
		if(rC.Eq(TUPLE_SEPARATOR_CHAR)) {
			bin_record_count++;
			rC.CToken++;
		}
		else
			break;
	}
}
//
// Set the 'bytes' to skip before column 'col'
//
void GpDatafile::DfSetSkipBefore(int col, int bytes)
{
	assert(col > 0);
	/* Check if we have room at least col columns */
	if(col > df_max_bininfo_cols) {
		df_column_bininfo = (df_column_bininfo_struct *)gp_realloc(df_column_bininfo, col * sizeof(df_column_bininfo_struct), "datafile columns binary information");
		df_max_bininfo_cols = col;
	}
	df_column_bininfo[col-1].skip_bytes = bytes;
}

// Set the column data type
//void df_set_read_type(int col, df_data_type type)
void GpDatafile::DfSetReadType(int col, df_data_type type)
{
	assert(col > 0);
	assert(type < DF_BAD_TYPE);
	/* Check if we have room at least col columns */
	if(col > df_max_bininfo_cols) {
		df_column_bininfo = (df_column_bininfo_struct *)gp_realloc(df_column_bininfo, col * sizeof(df_column_bininfo_struct), "datafile columns binary information");
		df_max_bininfo_cols = col;
	}
	df_column_bininfo[col-1].column.read_type = type;
	df_column_bininfo[col-1].column.read_size = df_binary_details[type].type.read_size;
}
//
// Get the column data type
//
//df_data_type df_get_read_type(int col)
df_data_type GpDatafile::DfGetReadType(int col)
{
	assert(col > 0);
	/* Check if we have room at least col columns */
	if(col < df_max_bininfo_cols)
		return df_column_bininfo[col].column.read_type;
	else
		return DF_UNDEF;
}
//
// Get the binary column data size
//
//int df_get_read_size(int col)
int GpDatafile::DfGetReadSize(int col)
{
	assert(col > 0);
	// Check if we have room at least col columns
	if(col < df_max_bininfo_cols)
		return(df_column_bininfo[col].column.read_size);
	else
		return -1;
}

/* If the column number is greater than number of binary columns, set
 * the unitialized columns binary info to that of the last specified
 * column or the default if none were set.  */
//void df_extend_binary_columns(int no_cols)
void GpDatafile::DfExtendBinaryColumns(int no_cols)
{
	if(no_cols > df_no_bin_cols) {
		df_data_type type = (df_no_bin_cols > 0) ? df_column_bininfo[df_no_bin_cols-1].column.read_type : DF_DEFAULT_TYPE;
		for(int i = no_cols; i > df_no_bin_cols; i--) {
			df_set_skip_after(i, 0);
			DfSetReadType(i, type);
		}
		df_no_bin_cols = no_cols;
	}
}
//
// Determine binary data widths from the `using` (or `binary`) format specification
//
//void plot_option_binary_format(char * format_string)
void GpDatafile::PlotOptionBinaryFormat(char * format_string)
{
	int    prev_read_type = DF_DEFAULT_TYPE; /* Defaults when none specified. */
	int    no_fields = 0;
	char * substr = format_string;
	while(*substr != '\0' && *substr != '\"' && *substr != '\'') {
		if(*substr == ' ') {
			substr++;
			continue;
		} /* Ignore spaces. */
		if(*substr == '%') {
			int ignore, field_repeat, j = 0, k = 0, m = 0, breakout;
			substr++;
			ignore = (*substr == '*');
			if(ignore)
				substr++;
			/* Check for field repeat number. */
			field_repeat = isdigit((uchar)*substr) ? strtol(substr, &substr, 10) : 1;
			/* Try finding the word among the valid type names. */
			for(j = 0, breakout = 0; j < (sizeof(df_binary_tables)/sizeof(df_binary_tables[0])); j++) {
				for(k = 0, breakout = 0;
				    k < df_binary_tables[j].group_length;
				    k++) {
					for(m = 0;
					    m < df_binary_tables[j].group[k].no_names;
					    m++) {
						int strl = strlen(df_binary_tables[j].group[k].name[m]);
						// Check for exact match, which includes character
						// after the substring being non-alphanumeric
						if(!strncmp(substr, df_binary_tables[j].group[k].name[m], strl) && strchr("%\'\" ", *(substr + strl)) ) {
							substr += strl; // Advance pointer in array to next text
							if(!ignore) {
								for(int n = 0; n < field_repeat; n++) {
									no_fields++;
									df_set_skip_after(no_fields, 0);
									GpDf.DfSetReadType(no_fields, df_binary_tables[j].group[k].type.read_type);
									prev_read_type = df_binary_tables[j].group[k].type.read_type;
								}
							}
							else {
								if(!df_column_bininfo)
									GpGg.IntError(GpC, NO_CARET, "Failure in binary table initialization");
								df_column_bininfo[no_fields].skip_bytes += field_repeat * df_binary_tables[j].group[k].type.read_size;
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
			if(j == (sizeof(df_binary_tables)/sizeof(df_binary_tables[0])) &&
				(k == df_binary_tables[j-1].group_length) && (m == df_binary_tables[j-1].group[k-1].no_names)) {
				GpGg.IntError(GpC, GpC.CToken, "Unrecognized binary format specification");
			}
		}
		else {
			GpGg.IntError(GpC, GpC.CToken, "Format specifier must begin with '%'");
		}
	}

	/* Any remaining unspecified fields are assumed to be of the same type
	 * as the last specified field.
	 */
	for(; no_fields < df_no_bin_cols; no_fields++) {
		df_set_skip_after(no_fields, 0);
		DfSetSkipBefore(no_fields, 0);
		DfSetReadType(no_fields, (df_data_type)prev_read_type);
	}
	df_no_bin_cols = no_fields;
}

//void df_show_binary(FILE * fp)
void GpDatafile::DfShowBinary(FILE * fp)
{
	int i, num_record;
	df_binary_file_record_struct * bin_record;
	fprintf(fp, "\tDefault binary data file settings (in-file settings may override):\n");
	if(!df_num_bin_records_default) {
		bin_record = &df_bin_record_reset;
		num_record = 1;
	}
	else {
		bin_record = df_bin_record_default;
		num_record = df_num_bin_records_default;
	}
	fprintf(fp, "\n\t  File Type: ");
	if(df_bin_filetype_default >= 0)
		fprintf(fp, "%s", df_bin_filetype_table[df_bin_filetype_default].key);
	else
		fprintf(fp, "none");
	fprintf(fp, "\n\t  File Endianness: %s", df_endian[df_bin_file_endianess_default]);
	fprintf(fp, "\n\t  Default binary format: %s", df_binary_format ? df_binary_format : "none");
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
			bool no_flip = true;
			fprintf(fp, "\n\t    Direction: ");
			if(bin_record[i].cart_dir[0] == -1) {
				fprintf(fp, "flip x");
				no_flip = false;
			}
			if((dimension > 1) && (bin_record[i].cart_dir[1] == -1)) {
				fprintf(fp, "%sflip y", (no_flip ? "" : ", "));
				no_flip = false;
			}
			if((dimension > 2) && (bin_record[i].cart_dir[2] == -1)) {
				fprintf(fp, "%sflip z", (no_flip ? "" : ", "));
				no_flip = false;
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
			if((bin_record[i].cart_trans == DF_TRANSLATE_VIA_ORIGIN) || (bin_record[i].cart_trans == DF_TRANSLATE_VIA_CENTER))
				fprintf(fp, " (%f, %f, %f)", bin_record[i].cart_cen_or_ori[0], bin_record[i].cart_cen_or_ori[1], bin_record[i].cart_cen_or_ori[2]);
			fprintf(fp, "\n\t    2D rotation angle: %f", bin_record[i].cart_alpha);
			fprintf(fp, "\n\t    3D normal vector: (%f, %f, %f)", bin_record[i].cart_p[0], bin_record[i].cart_p[1], bin_record[i].cart_p[2]);
			for(j = 0; j < (sizeof(df_bin_scan_table_3D)/sizeof(df_bin_scan_table_3D[0])); j++) {
				if(!strncmp((char*)bin_record[i].cart_scan, (char*)df_bin_scan_table_3D[j].scan, sizeof(bin_record[0].cart_scan))) {
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
	for(i = 0; i < sizeof(df_binary_details)/sizeof(df_binary_details[0]); i++) {
		int j;
		fprintf(fp, "\t  ");
		for(j = 0; j < df_binary_details[i].no_names; j++) {
			fprintf(fp, "\"%s\" ", df_binary_details[i].name[j]);
		}
		fprintf(fp, "(%d)\n", df_binary_details[i].type.read_size);
	}
	fprintf(fp, "\n\tThe following binary data sizes attempt to be machine independent:\n\n\t  name (size in bytes)\n\n");
	for(i = 0; i < sizeof(df_binary_details_independent)/sizeof(df_binary_details_independent[0]); i++) {
		int j;
		fprintf(fp, "\t  ");
		for(j = 0; j < df_binary_details_independent[i].no_names; j++) {
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
	int i = 0;
	fprintf(fp, "\tThis version of gnuplot understands the following binary file types:\n");
	while(df_bin_filetype_table[i].key)
		fprintf(fp, "\t  %s", df_bin_filetype_table[i++].key);
	fputs("\n", fp);
}

void df_swap_bytes_by_endianess(char * data, int read_order, int read_size)
{
	if((read_order == DF_3210)
#if SUPPORT_MIDDLE_ENDIAN
	    || (read_order == DF_2301)
#endif
	    ) {
		int j = 0;
		int k = read_size - 1;
		for(; j < k; j++, k--) {
			char temp = data[j];
			data[j] = data[k];
			data[k] = temp;
		}
	}
#if SUPPORT_MIDDLE_ENDIAN
	if((read_order == DF_1032) || (read_order == DF_2301)) {
		int j = read_size - 1;
		for(; j > 0; j -= 2) {
			char temp = data[j-1];
			data[j-1] = data[j];
			data[j] = temp;
		}
	}
#endif
}

//static int df_skip_bytes(off_t nbytes)
int GpDatafile::DfSkipBytes(off_t nbytes)
{
#if defined(PIPES)
	char cval;
	if(df_pipe_open || plotted_data_from_stdin) {
		while(nbytes--) {
			if(1 == fread(&cval, 1, 1, data_fp))
				continue;
			if(feof(data_fp)) {
				df_eof = 1;
				return DF_EOF;
			}
			GpGg.IntError(GpC, NO_CARET, read_error_msg);
		}
	}
	else
#endif
	if(fseek(data_fp, nbytes, SEEK_CUR)) {
		if(feof(data_fp)) {
			df_eof = 1;
			return DF_EOF;
		}
		GpGg.IntError(GpC, NO_CARET, read_error_msg);
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
int GpDatafile::DfReadBinary(double v[], int max)
{
	/* For general column structured binary. */
	static int scan_size[3];
	static double delta[3];  /* sampling periods */
	static double o[3];      /* add after rotations */
	static double c[3];      /* subtract before doing rotations */
	static double P[3][3];   /* 3D rotation matrix (perpendicular) */
	static double R[2][2];   /* 2D rotation matrix (rotate) */
	static int read_order;
	static off_t record_skip;
	static bool end_of_scan_line;
	static bool end_of_block;
	static bool translation_required;
	static char * memory_data;

	// For matrix data structure (i.e., gnuplot binary)
	static double first_matrix_column;
	static float * scanned_matrix_row = 0;
	static int first_matrix_row_col_count;
	bool saved_first_matrix_column = false;

	assert(max <= MAXDATACOLS);
	assert(df_max_bininfo_cols > df_no_bin_cols);
	assert(df_no_bin_cols);

	// catch attempt to read past EOF on mixed-input 
	if(df_eof)
		return DF_EOF;
	// Check if we have room for at least df_no_bin_cols columns 
	if(df_max_cols < df_no_bin_cols)
		ExpandDfColumn(df_no_bin_cols);
	/* In binary mode, the number of user specs was increased by the
	 * number of dimensions in the underlying uniformly sampled grid
	 * previously.  Fill in those values.  Also, compute elements of
	 * formula x' = P*R*(x - c) + o */
	if(!df_M_count && !df_N_count && !df_O_count) {
		int i;
		bool D2, D3;
		df_binary_file_record_struct * p_rec = df_bin_record + df_bin_record_count;
		scan_size[0] = scan_size[1] = scan_size[2] = 0;
		D2 = rotation_matrix_2D(R, p_rec->cart_alpha);
		D3 = rotation_matrix_3D(P, p_rec->cart_p);
		translation_required = D2 || D3;
		if(df_matrix_file) {
			/* Dimensions */
			scan_size[0] = p_rec->scan_dim[0];
			scan_size[1] = p_rec->scan_dim[1];
			FPRINTF((stderr, "datafile.c:%d matrix dimensions %d x %d\n", __LINE__, scan_size[1], scan_size[0]));
			df_xpixels = scan_size[1];
			df_ypixels = scan_size[0];
			if(scan_size[0] == 0)
				GpGg.IntError(GpC, NO_CARET, "Scan size of matrix is zero");
			/* To accomplish flipping in this case, multiply the
			 * appropriate column of the rotation matrix by -1.  */
			for(i = 0; i < 2; i++) {
				for(int j = 0; j < 2; j++) {
					R[i][j] *= p_rec->cart_dir[i];
				}
			}
			/* o */
			for(i = 0; i < 3; i++) {
				if(p_rec->cart_trans != DF_TRANSLATE_DEFAULT) {
					o[i] = p_rec->cart_cen_or_ori[i];
				}
				else {
					/* Default is translate by center. */
					if(i < 2)
						o[i] = (df_matrix_corner[1][i] + df_matrix_corner[0][i]) / 2;
					else
						o[i] = 0;
				}
			}
			/* c */
			for(i = 0; i < 3; i++) {
				if(p_rec->cart_trans == DF_TRANSLATE_VIA_ORIGIN) {
					c[i] = (i < 2) ? df_matrix_corner[0][i] : 0;
				}
				else {
					c[i] = (i < 2) ? ((df_matrix_corner[1][i] + df_matrix_corner[0][i]) / 2) : 0;
				}
			}
			first_matrix_row_col_count = 0;
		}
		else { /* general binary */
			for(i = 0; i < 3; i++) {
				int map;
				// How to direct the generated coordinates in regard to scan direction
				if(p_rec->cart_dim[i] || p_rec->scan_dim[i]) {
					if(p_rec->scan_generate_coord)
						use_spec[i].column = p_rec->cart_scan[i];
				}
				/* Dimensions */
				map = DF_SCAN_POINT - p_rec->cart_scan[i];
				if(p_rec->cart_dim[i] > 0)
					scan_size[map] = p_rec->cart_dim[i];
				else if(p_rec->cart_dim[i] < 0)
					scan_size[map] = MAXINT;
				else
					scan_size[map] = p_rec->scan_dim[map];
				// Sample periods 
				delta[map] = (p_rec->cart_delta[i]) ? p_rec->cart_delta[i] : p_rec->scan_delta[map];
				delta[map] *= p_rec->scan_dir[map] * p_rec->cart_dir[i];
				/* o */
				if(p_rec->cart_trans != DF_TRANSLATE_DEFAULT)
					o[i] = p_rec->cart_cen_or_ori[i];
				else if(p_rec->scan_trans != DF_TRANSLATE_DEFAULT)
					o[i] = p_rec->scan_cen_or_ori[map];
				else if(scan_size[map] > 0)
					o[i] = (scan_size[map] - 1)*fabs(delta[map])/2;
				else
					o[i] = 0;
				/* c */
				if(p_rec->cart_trans == DF_TRANSLATE_VIA_ORIGIN || (p_rec->cart_trans == DF_TRANSLATE_DEFAULT && p_rec->scan_trans == DF_TRANSLATE_VIA_ORIGIN)) {
					c[i] = ((scan_size[map] > 0) && (delta[map] < 0)) ? ((scan_size[map] - 1)*delta[map]) : 0;
				}
				else {
					c[i] = (scan_size[map] > 0) ? ((scan_size[map] - 1)*(delta[map]/2)) : 0;
				}
			}
		}
		// Check if c and o are the same
		for(i = 0; i < 3; i++)
			translation_required = translation_required || (c[i] != o[i]);
		// Should data come from memory? 
		memory_data = p_rec->memory_data;
		// byte read order 
		read_order = byte_read_order(df_bin_file_endianess);
		// amount to skip before first record 
		record_skip = p_rec->scan_skip[0];
		end_of_scan_line = false;
		end_of_block = false;
		point_count = -1;
		line_count = 0;
		df_current_index = df_bin_record_count;
		//
		// Craig DeForest Feb 2013 - Fast version of uniform binary matrix.
		// Don't apply this to ascii input or special filetypes.
		// Slurp all data from file or pipe in one shot to minimize fread calls.
		//
		if(!memory_data && !(df_bin_filetype > 0) &&  df_binary_file &&  df_matrix && !df_nonuniform_matrix) {
			int    i;
			ulong  bytes_per_point = 0;
			ulong  bytes_per_line = 0;
			ulong  bytes_per_plane = 0;
			ulong  bytes_total = 0;
			size_t fread_ret;
			// Accumulate total number of bytes in this tuple 
			for(i = 0; i<df_no_bin_cols; i++)
				bytes_per_point += df_column_bininfo[i].skip_bytes + df_column_bininfo[i].column.read_size;
			bytes_per_point += df_column_bininfo[df_no_bin_cols].skip_bytes;
			bytes_per_line  = bytes_per_point * (  (scan_size[0] > 0) ? scan_size[0] : 1 );
			bytes_per_plane = bytes_per_line * ( (scan_size[1] > 0) ? scan_size[1] : 1 );
			bytes_total     = bytes_per_plane * ( (scan_size[2]>0) ? scan_size[2] : 1);
			bytes_total    += record_skip;
			/* Allocate a chunk of memory and stuff it */
			/* EAM FIXME: Is this a leak if the plot errors out? */
			memory_data = (char *)malloc(bytes_total);
			p_rec->memory_data = memory_data;
			FPRINTF((stderr, "Fast matrix code:\n"));
			FPRINTF((stderr, "\t\t skip %d bytes, read %ld bytes as %d x %d array\n", record_skip, bytes_total, scan_size[0], scan_size[1]));
			// Do the actual slurping
			fread_ret = fread(memory_data, 1, bytes_total, data_fp);
			if(fread_ret != bytes_total) {
				int_warn(NO_CARET, "Couldn't slurp %ld bytes (return was %zd)\n", bytes_total, fread_ret);
				df_eof = 1;
				return DF_EOF;
			}
		}
	}
	while(!df_eof) {
		/*{{{  process line */
		int line_okay = 1;
		int output = 0;     /* how many numbers written to v[] */
		int i, fread_ret = 0;
		int m_value, n_value, o_value;
		union io_val {
			char ch;
			uchar uc;
			short sh;
			ushort us;
			int in;
			uint ui;
			long lo;
			ulong ul;
			int64 llo;
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
			end_of_scan_line = false;
			point_count = -1;
			line_count++;
			return DF_FIRST_BLANK;
		}
		if(end_of_block) {
			end_of_block = false;
			line_count = 0;
			return DF_SECOND_BLANK;
		}
		if(record_skip) { // Possibly skip bytes before starting to read record. 
			if(memory_data)
				memory_data += record_skip;
			else if(DfSkipBytes(record_skip))
				return DF_EOF;
			record_skip = 0;
		}
		// Bring in variables as described by the field parameters.
		// If less than than the appropriate number of bytes have been
		// read, issue an error stating not enough columns were found. 
		for(i = 0;; i++) {
			off_t skip_bytes = df_column_bininfo[i].skip_bytes;
			if(skip_bytes) {
				if(memory_data)
					memory_data += skip_bytes;
				else if(DfSkipBytes(skip_bytes))
					return DF_EOF;
			}

			/* Last entry only has skip bytes, no data. */
			if(i == df_no_bin_cols)
				break;

			/* Read in a "column", i.e., a binary value of various types. */
			if(df_pixeldata) {
				io_val.uc = df_libgd_get_pixel(df_M_count, df_N_count, i);
			}
			else if(memory_data)        {
				for(fread_ret = 0; fread_ret < df_column_bininfo[i].column.read_size; fread_ret++)
					(&io_val.ch)[fread_ret] = *memory_data++;
			}
			else {
				fread_ret = fread(&io_val.ch, df_column_bininfo[i].column.read_size, 1, data_fp);
				if(fread_ret != 1) {
					df_eof = 1;
					return DF_EOF;
				}
			}
			if(read_order != 0)
				df_swap_bytes_by_endianess(&io_val.ch, read_order, df_column_bininfo[i].column.read_size);
			switch(df_column_bininfo[i].column.read_type) {
				case DF_CHAR: df_column[i].datum = io_val.ch; break;
				case DF_UCHAR: df_column[i].datum = io_val.uc; break;
				case DF_SHORT: df_column[i].datum = io_val.sh; break;
				case DF_USHORT: df_column[i].datum = io_val.us; break;
				case DF_INT: df_column[i].datum = io_val.in; break;
				case DF_UINT: df_column[i].datum = io_val.ui; break;
				case DF_LONG: df_column[i].datum = io_val.lo; break;
				case DF_ULONG: df_column[i].datum = io_val.ul; break;
				case DF_LONGLONG: df_column[i].datum = (double)io_val.llo; break;
				case DF_ULONGLONG: df_column[i].datum = (double)io_val.ull; break;
				case DF_FLOAT: df_column[i].datum = io_val.fl; break;
				case DF_DOUBLE: df_column[i].datum = io_val.db; break;
				default: GpGg.IntError(GpC, NO_CARET, "Binary data type unknown");
			}
			df_column[i].good = DF_GOOD;
			df_column[i].position = NULL; /* cant get a time */
			// Matrix file data is a special case. After reading in just
			// one binary value, stop and decide on what to do with it.
			if(df_matrix_file)
				break;
		}
		if(df_matrix_file) {
			if(df_nonuniform_matrix) {
				/* Store just first column? */
				if(!df_M_count && !saved_first_matrix_column) {
					first_matrix_column = df_column[i].datum;
					saved_first_matrix_column = true;
					continue;
				}
				/* Read reset of first row? */
				if(!df_M_count && !df_N_count && !df_O_count && first_matrix_row_col_count < scan_size[0]) {
					if(!first_matrix_row_col_count)
						scanned_matrix_row = (float *)gp_realloc(scanned_matrix_row, scan_size[0]*sizeof(float), "gpbinary matrix row");
					scanned_matrix_row[first_matrix_row_col_count] = (float)df_column[i].datum;
					first_matrix_row_col_count++;
					if(first_matrix_row_col_count == scan_size[0]) {
						saved_first_matrix_column = false; // Start of the second row
					}
					continue;
				}
			}
			/* Update all the binary columns.  Matrix binary and
			 * matrix ASCII is a slight abuse of notation.  At the
			 * command line, 1 means first row, 2 means first
			 * column.  There can only be one column of data input
			 * because it is a matrix of data, not columns.  */
			{
				int j;
				df_datum = (int)df_column[i].datum;
				/* Fill backward so that current read value is not
				 * overwritten. */
				for(j = df_no_bin_cols-1; j >= 0; j--) {
					if(j == 0)
						df_column[j].datum = df_nonuniform_matrix ? scanned_matrix_row[df_M_count] : df_M_count;
					else if(j == 1)
						df_column[j].datum = df_nonuniform_matrix ? first_matrix_column : df_N_count;
					else
						df_column[j].datum = df_column[i].datum;
					df_column[j].good = DF_GOOD;
					df_column[j].position = NULL;
				}
			}
		}
		else { /* Not matrix file, general binary. */
			df_datum = point_count + 1;
			if(i != df_no_bin_cols) {
				if(feof(data_fp)) {
					if(i != 0)
						GpGg.IntError(GpC, NO_CARET, "Last point in the binary file did not match the specified `using` columns");
					df_eof = 1;
					return DF_EOF;
				}
				else {
					GpGg.IntError(GpC, NO_CARET, read_error_msg);
				}
			}
		}
		m_value = df_M_count;
		n_value = df_N_count;
		o_value = df_O_count;
		df_M_count++;
		if((scan_size[0] > 0) && (df_M_count >= scan_size[0])) {
			/* This is a new "line". */
			df_M_count = 0;
			df_N_count++;
			end_of_scan_line = true;
			if((scan_size[1] >= 0) && (df_N_count >= scan_size[1])) {
				/* This is a "block". */
				df_N_count = 0;
				df_O_count++;
				if((scan_size[2] >= 0) && (df_O_count >= scan_size[2])) {
					df_O_count = 0;
					end_of_block = true;
					if(++df_bin_record_count >= df_num_bin_records) {
						df_eof = 1;
					}
				}
			}
		}
		/*{{{  ignore points outside range of index */
		/* we try to return end-of-file as soon as we pass upper
		 * index, but for mixed input stream, we must skip garbage */
		if(df_current_index < df_lower_index || df_current_index > df_upper_index || ((df_current_index - df_lower_index) % df_index_step) != 0)
			continue;
		/*}}} */

		/*{{{  reject points by every */
		/* accept only lines with (line_count%everyline) == 0 */
		if(line_count < firstline || line_count > lastline || (line_count - firstline) % everyline != 0)
			continue;
		/* update point_count. ignore point if
		   point_count%everypoint != 0 */
		if(++point_count < firstpoint || point_count > lastpoint || (point_count - firstpoint) % everypoint != 0)
			continue;
		/*}}} */

		/* At this point the binary columns have been read
		 * successfully.  Set df_no_cols to df_no_bin_cols for use
		 * in the interpretation code.  */
		df_no_cols = df_no_bin_cols;

		/*{{{  copy column[] to v[] via use[] */
		{
			int limit = (df_no_use_specs ? df_no_use_specs : MAXDATACOLS);
			if(limit > max)
				limit = max;
			for(output = 0; output < limit; ++output) {
				int column = use_spec[output].column;
				/* if there was no using spec, column is output+1 and at=NULL */
				if(use_spec[output].at) {
					t_value a;
					/* no dummy values to set up prior to... */
					evaluate_inside_using = true;
					GpGg.Ev.EvaluateAt(use_spec[output].at, &a);
					evaluate_inside_using = false;
					if(GpGg.Ev.undefined)
						return DF_UNDEFINED;
					if(a.type == STRING) {
						if(use_spec[output].expected_type == CT_STRING) {
							char * s = (char *)malloc(strlen(a.v.string_val)+3);
							*s = '"';
							strcpy(s+1, a.v.string_val);
							strcat(s, "\"");
							free(df_stringexpression[output]);
							df_tokens[output] = df_stringexpression[output] = s;
						}
						gpfree_string(&a);
					}
					else
						v[output] = a.Real();
				}
				else if(column == DF_SCAN_PLANE) {
					if((df_current_plot->plot_style == IMAGE) || (df_current_plot->plot_style == RGBIMAGE))
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
						v[output] = df_column[0].datum;
				}
				else if(column == DF_SCAN_LINE) {
					v[output] = n_value*delta[1];
				}
				else if(column == DF_SCAN_POINT) {
					v[output] = m_value*delta[0];
				}
				else if(column == -2) {
					v[output] = df_current_index;
				}
				else if(column == -1) {
					v[output] = line_count;
				}
				else if(column == 0) {
					v[output] = df_datum;
				}
				else if(column <= 0) {
					GpGg.IntError(GpC, NO_CARET, "internal error: unknown column type");
					/* July 2010 - We used to have special code to handle time data. */
					/* But time data in a binary file is just one more binary value, */
					/* so let the general case code handle it.                       */
				}
				else if((column <= df_no_cols) && df_column[column - 1].good == DF_GOOD)
					v[output] = df_column[column - 1].datum;
				/* EAM - Oct 2002 Distinguish between DF_MISSING
				 * and DF_BAD.  Previous versions would never
				 * notify caller of either case.  Now missing data
				 * will be noted. Bad data should arguably be
				 * noted also, but that would change existing
				 * default behavior.  */
				else if((column <= df_no_cols) && (df_column[column - 1].good == DF_MISSING))
					return DF_MISSING;
				else {
					/* line bad only if user explicitly asked for this column */
					if(df_no_use_specs)
						line_okay = 0;
					break; /* return or ignore depending on line_okay */
				}
				if(fisnan(v[output])) {
					/* EAM April 2012 - return, continue, or ignore??? */
					FPRINTF((stderr, "NaN input value"));
					if(!df_matrix)
						return DF_UNDEFINED;
				}
			}

			/* Linear translation. */
			if(translation_required) {
				double x, y, z;
				x = v[0] - c[0];
				y = v[1] - c[1];
				v[0] = R[0][0] * x + R[0][1] * y;
				v[1] = R[1][0] * x + R[1][1] * y;
				if(df_plot_mode == MODE_SPLOT) {
					x = v[0];
					y = v[1];
					z = v[2] - c[2];
					v[0] = P[0][0] * x + P[0][1] * y + P[0][2] * z;
					v[1] = P[1][0] * x + P[1][1] * y + P[1][2] * z;
					v[2] = P[2][0] * x + P[2][1] * y + P[2][2] * z;
				}
				v[0] += o[0];
				v[1] += o[1];
				if(df_plot_mode == MODE_SPLOT)
					v[2] += o[2];
			}
		}
		/*}}} */
		if(!line_okay)
			continue;
		for(i = df_no_use_specs; i<df_no_use_specs+df_no_tic_specs; i++) {
			if(use_spec[i].expected_type >= CT_XTICLABEL &&  use_spec[i].at != NULL) {
				t_value a;
				int axis, axcol;
				evaluate_inside_using = true;
				GpGg.Ev.EvaluateAt(use_spec[i].at, &a);
				evaluate_inside_using = false;
				switch(use_spec[i].expected_type) {
					default:
					case CT_XTICLABEL:
					    axis = FIRST_X_AXIS;
					    axcol = 0;
					    break;
					case CT_X2TICLABEL:
					    axis = SECOND_X_AXIS;
					    axcol = 0;
					    break;
					case CT_YTICLABEL:
					    axis = FIRST_Y_AXIS;
					    axcol = 1;
					    break;
					case CT_Y2TICLABEL:
					    axis = SECOND_Y_AXIS;
					    axcol = 1;
					    break;
					case CT_ZTICLABEL:
					    axis = FIRST_Z_AXIS;
					    axcol = 2;
					    break;
					case CT_CBTICLABEL:
					    /* EAM FIXME - Which column to set for cbtic? */
					    axis = COLOR_AXIS;
					    axcol = 2;
					    break;
				}
				if(a.type == STRING) {
					add_tic_user(&GpGg[axis], a.v.string_val, v[axcol], -1);
					gpfree_string(&a);
				}
			}
		}
		// output == df_no_use_specs if using was specified - actually, smaller of df_no_use_specs and max
		assert(df_no_use_specs == 0 || output == df_no_use_specs || output == max);
		return output;
	}
	/*}}} */
	df_eof = 1;
	return DF_EOF;
}

void GpDatafile::DfSetPlotMode(int mode)
{
	df_plot_mode = mode;
}

/* Special pseudofile '+' returns x coord of sample for 2D plots,
 * Special pseudofile '++' returns x and y coordinates of grid for 3D plots.
 */
//static char * df_generate_pseudodata()
char * GpDatafile::DfGeneratePseudoData()
{
	// Pseudofile '+' returns a set of (samples) x coordinates 
	// This code copied from that in second pass through eval_plots() 
	if(df_pseudodata == 1) {
		static double t, /*t_min, t_max,*/ t_step; // @global
		static RealRange t_range; // @global
		if(df_pseudorecord == 0) {
			t_step = 0;
			if((GpGg[SAMPLE_AXIS].range_flags & RANGE_SAMPLED)) {
				t_range = GpGg[SAMPLE_AXIS].Range;
				t_step = GpGg[SAMPLE_AXIS].SAMPLE_INTERVAL;
			}
			else if(GpGg.IsParametric || GpGg.IsPolar) {
				t_range = GpGg[T_AXIS].Range;
			}
			else {
				if(GpGg[FIRST_X_AXIS].Range.upp == -GPVL)
					GpGg[FIRST_X_AXIS].Range.upp = 10;
				if(GpGg[FIRST_X_AXIS].Range.low == GPVL)
					GpGg[FIRST_X_AXIS].Range.low = -10;
				t_range = GpGg.GetX().Range;
				GpGg.GetX().UnlogInterval(t_range, true);
			}
			SETIFZ(t_step, t_range.GetDistance() / (GpGg.Samples1 - 1)); // always true unless explicit sample interval was given 
			SETIFZ(t_step, 1); // prevent infinite loop on zero range 
		}
		t = t_range.low + df_pseudorecord * t_step;
		if((GpGg[SAMPLE_AXIS].range_flags & RANGE_SAMPLED)) {
			// This is the case of an explicit sampling range
			// FIXME: should allow for round-off error in floating point summation!
			if(!t_range.CheckX(t))
				return NULL;
		}
		else {
			// This is the usual case
			if(df_pseudorecord >= GpGg.Samples1)
				return NULL;
			if(!GpGg.IsParametric)
				t = GpGg.DelogValue(GpGg.XAxis, t);
		}
		if(df_current_plot && df_current_plot->sample_var)
			df_current_plot->sample_var->udv_value.SetComplex(t, 0.0);
		df_pseudovalue_0 = t;
		sprintf(df_line, "%g", t);
		++df_pseudorecord;
	}
	// Pseudofile '++' returns a (samples X isosamples) grid of x,y coordinates 
	// This code copied from that in second pass through eval_3dplots 
	if(df_pseudodata == 2) {
		static double /*u_min, u_max,*/ u_step, /*v_min, v_max,*/ v_isostep; // @global
		static RealRange u_range; // @global
		static RealRange v_range; // @global
		static int nusteps, nvsteps; // @global
		double u, v;
		AXIS_INDEX u_axis = FIRST_X_AXIS;
		AXIS_INDEX v_axis = FIRST_Y_AXIS;
		if((df_pseudorecord >= nusteps) && (df_pseudorecord > 0)) {
			df_pseudorecord = 0;
			if(++df_pseudospan >= nvsteps)
				return NULL;
			else
				return ""; // blank record for end of scan line 
		}
		if(df_pseudospan == 0) {
			if(GpGg.Samples1 < 2 || GpGg.Samples2 < 2 || GpGg.iso_samples_1 < 2 || GpGg.iso_samples_2 < 2)
				GpGg.IntError(GpC, NO_CARET, "samples or iso_samples < 2. Must be at least 2.");
			if(GpGg.IsParametric) {
				u_range = GpGg[U_AXIS].Range;
				v_range = GpGg[V_AXIS].Range;
			}
			else {
				GpGg.AxisCheckedExtendEmptyRange(FIRST_X_AXIS, "x range is invalid");
				GpGg.AxisCheckedExtendEmptyRange(FIRST_Y_AXIS, "y range is invalid");
				u_range.low = GpGg.LogValueChecked(u_axis, GpGg[u_axis].Range.low, "x range");
				u_range.upp = GpGg.LogValueChecked(u_axis, GpGg[u_axis].Range.upp, "x range");
				v_range.low = GpGg.LogValueChecked(v_axis, GpGg[v_axis].Range.low, "y range");
				v_range.upp = GpGg.LogValueChecked(v_axis, GpGg[v_axis].Range.upp, "y range");
			}
			if(GpGg.hidden3d) {
				u_step = u_range.GetDistance() / (GpGg.iso_samples_1 - 1);
				nusteps = GpGg.iso_samples_1;
			}
			else {
				u_step = u_range.GetDistance() / (GpGg.Samples1 - 1);
				nusteps = GpGg.Samples1;
			}
			v_isostep = v_range.GetDistance() / (GpGg.iso_samples_2 - 1);
			nvsteps = GpGg.iso_samples_2;
		}
		// Duplicate algorithm from calculate_set_of_isolines()
		u = u_range.low + df_pseudorecord * u_step;
		v = v_range.upp - df_pseudospan * v_isostep;
		if(GpGg.IsParametric) {
			df_pseudovalue_0 = u;
			df_pseudovalue_1 = v;
		}
		else {
			df_pseudovalue_0 = GpGg.DelogValue(u_axis, u);
			df_pseudovalue_1 = GpGg.DelogValue(v_axis, v);
		}
		sprintf(df_line, "%g %g", df_pseudovalue_0, df_pseudovalue_1);
		++df_pseudorecord;
	}
	return df_line;
}
//
// Allocate space for more data columns as needed
//
//void expand_df_column(int new_max)
void GpDatafile::ExpandDfColumn(int new_max)
{
	df_column = (df_column_struct *)gp_realloc(df_column, new_max * sizeof(df_column_struct), "datafile column");
	for(; df_max_cols < new_max; df_max_cols++) {
		df_column[df_max_cols].datum = 0;
		df_column[df_max_cols].header = NULL;
		df_column[df_max_cols].position = NULL;
	}
}
//
// Clear column headers stored for previous plot
//
//void clear_df_column_headers()
void GpDatafile::ClearDfColumnHeaders()
{
	for(int i = 0; i < df_max_cols; i++) {
		free(df_column[i].header);
		df_column[i].header = NULL;
	}
	df_longest_columnhead = 0;
}

/* The main loop in df_readascii wants a string to process. */
/* We generate one from the current array entry containing  */
/* the array index in column 1 and the value in column 2.   */
//static char * df_generate_ascii_array_entry()
char * GpDatafile::DfGenerateAsciiArrayEntry()
{
	df_array_index++;
	if(df_array_index > GpC.P.P_DfArray->udv_value.v.value_array[0].v.int_val)
		return NULL;
	else {
		const t_value * entry = &(GpC.P.P_DfArray->udv_value.v.value_array[df_array_index]);
		if(entry->type == STRING)
			sprintf(df_line, "%d \"%s\"", df_array_index, entry->v.string_val);
		else
			sprintf(df_line, "%d %g", df_array_index, entry->Real());
		return df_line;
	}
}

void GpDatafile::SetSeparator(GpCommand & rC)
{
	rC.CToken++;
	ZFREE(df_separators);
	if(!rC.EndOfCommand()) {
		if(rC.AlmostEq("white$space")) {
			rC.CToken++;
		}
		else if(rC.Eq("comma")) {
			df_separators = gp_strdup(",");
			rC.CToken++;
		}
		else if(rC.Eq("tab") || rC.Eq("\'\\t\'")) {
			df_separators = gp_strdup("\t");
			rC.CToken++;
		}
		else if(!(df_separators = rC.TryToGetString())) {
			GpGg.IntError(rC, rC.CToken, "expected \"<separator_char>\"");
		}
	}
}
