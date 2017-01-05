/*
 * $Id: datafile.h,v 1.51 2016/03/21 23:13:47 sfeam Exp $
 */

/* GNUPLOT - datafile.h */

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

#ifndef GNUPLOT_DATAFILE_H
	#define GNUPLOT_DATAFILE_H

/* #if... / #include / #define collection: */

//#include "axis.h"
//#include "graph3d.h"
//#include "graphics.h"

/* returns from DF_READLINE in datafile.c */
/* +ve is number of columns read */
enum DF_STATUS {
    DF_BAD = 0,
    DF_GOOD = 1,
    DF_EOF = -1,
    DF_UNDEFINED = -2,
    DF_FIRST_BLANK = -3,
    DF_SECOND_BLANK = -4,
    DF_MISSING = -5,
    DF_FOUND_KEY_TITLE = -6,
    DF_KEY_TITLE_MISSING = -7,
    DF_STRINGDATA = -8,
    DF_COLUMN_HEADERS = -9
};

/* large file support (offsets potentially > 2GB) */
#if defined(HAVE_FSEEKO) && defined(HAVE_OFF_T)
	#define fseek(stream,pos,whence) fseeko(stream,pos,whence)
	#define ftell(stream) ftello(stream)
#elif defined(_MSC_VER)
	// @sobolev #define fseek(stream,pos,whence) _fseeki64(stream,pos,whence)
	// @sobolev #define ftell(stream) _ftelli64(stream)
#elif defined(__MINGW32__)
	#define fseek(stream,pos,whence) fseeko64(stream,pos,whence)
	#define ftell(stream) ftello64(stream)
#endif
//
// Maximum number of columns returned to caller by df_readline		
// Various data structures are dimensioned to hold this many entries.	
// As of June 2013, plot commands never ask for more than 7 columns of	
// data, but fit commands can use more. "fit" is also limited by	
// the number of parameters that can be passed	to a user function, so	
// let's try setting MAXDATACOLS to match.				
// At present this bumps it from 7 to 14.				
//
#define MAXDATACOLS (MAX_NUM_VAR+2)
#define NO_COLUMN_HEADER (-99)  // some value that can never be a real column 
//
// Prototypes of functions exported by datafile.c
//
//int df_open(const char *, int, curve_points *);
//int df_readline(double [], int);
//void df_close();
//void df_init();
//char * df_fgets(FILE *);
//void df_showdata();
int df_2dbinary(curve_points *);
int df_3dmatrix(surface_points *, int);
//void df_set_key_title(curve_points *);
//void df_set_key_title_columnhead(curve_points *);
//char * df_parse_string_field(char *);
//int expect_string(const char column );
void df_reset_after_error();
void f_dollars(GpArgument *x);
void f_column(GpArgument *x);
void f_columnhead(GpArgument *x);
void f_valid(GpArgument *x);
void f_timecolumn(GpArgument *x);
//void f_stringcolumn(GpArgument *x);

struct UseSpecS {
	UseSpecS()
	{
		column = 0;
		expected_type = 0;
		at = 0;
	}
    int    column;
    int    expected_type;
    AtType * at;
};
//
// Details about the records contained in a binary data file
//
enum df_translation_type {
    DF_TRANSLATE_DEFAULT,     /* Gnuplot will position in first quadrant at origin. */
    DF_TRANSLATE_VIA_ORIGIN,
    DF_TRANSLATE_VIA_CENTER
};

enum df_sample_scan_type {
    DF_SCAN_POINT = -3,  /* fastest */
    DF_SCAN_LINE  = -4,
    DF_SCAN_PLANE = -5   /* slowest */
};
//
// To generate a swap, take the bit-wise complement of the lowest two bits
//
enum df_endianess_type {
    DF_LITTLE_ENDIAN,
    DF_PDP_ENDIAN,
    DF_DPD_ENDIAN,
    DF_BIG_ENDIAN,
    DF_ENDIAN_TYPE_LENGTH  /* Must be last */
};

const long long_0x2468 = 0x2468;
#define TEST_BIG_PDP         ( (((char*)&long_0x2468)[0] < 3) ? DF_BIG_ENDIAN : DF_PDP_ENDIAN )
#define THIS_COMPILER_ENDIAN ( (((char*)&long_0x2468)[0] < 5) ? TEST_BIG_PDP : DF_LITTLE_ENDIAN )

#define DF_BIN_FILE_ENDIANESS_RESET THIS_COMPILER_ENDIAN
//
// The various types of numerical types that can be read from a data file
//
enum df_data_type {
	DF_UNDEF = -1, // @sobolev
    DF_CHAR = 0, 
	DF_UCHAR, 
	DF_SHORT, 
	DF_USHORT, 
	DF_INT,
    DF_UINT, 
	DF_LONG,  
	DF_ULONG, 
	DF_FLOAT,  
	DF_DOUBLE,
    DF_LONGLONG, 
	DF_ULONGLONG,
    DF_BAD_TYPE
};

#define DF_DEFAULT_TYPE DF_FLOAT

/* Some macros for making the compiler figure out what function
 * the "machine independent" names should execute to read the
 * appropriately sized variable from a data file.
 */ 
#define SIGNED_TEST(val) ((val)==sizeof(long) ? DF_LONG : \
			 ((val)==sizeof(int64) ? DF_LONGLONG : \
			 ((val)==sizeof(int) ? DF_INT : \
			 ((val)==sizeof(short) ? DF_SHORT : \
			 ((val)==sizeof(char) ? DF_CHAR : DF_BAD_TYPE)))))
#define UNSIGNED_TEST(val) ((val)==sizeof(ulong) ? DF_ULONG : \
			   ((val)==sizeof(uint64) ? DF_ULONGLONG : \
			   ((val)==sizeof(uint) ? DF_UINT : \
			   ((val)==sizeof(ushort) ? DF_USHORT : \
			   ((val)==sizeof(uchar) ? DF_UCHAR : DF_BAD_TYPE)))))
#define FLOAT_TEST(val) ((val)==sizeof(float) ? DF_FLOAT : ((val)==sizeof(double) ? DF_DOUBLE : DF_BAD_TYPE))

enum df_records_type {
    DF_CURRENT_RECORDS,
    DF_DEFAULT_RECORDS
};

struct df_binary_type_struct {
    df_data_type read_type;
    ushort read_size;
};

struct df_column_bininfo_struct {
    long skip_bytes;
    df_binary_type_struct column;
};

/* NOTE TO THOSE WRITING FILE TYPE FUNCTIONS
 *
 * "cart" means Cartesian, i.e., the (x,y,z) [or (r,t,z)] GpCoordinate
 * system of the plot.  "scan" refers to the scanning method of the
 * file in question, i.e., first points, then lines, then planes.
 * The important variables for a file type function to fill in are
 * those beginning with "scan".  There is a tricky set of rules
 * related to the "scan_cart" mapping, the file-specified variables,
 * the default variables, and the command-line variables.  Basically,
 * command line overrides data file which overrides default.  (Yes,
 * like a confusing version of rock, paper, scissors.) So, from the
 * file type function perspective, it is better to leave those
 * variables which are not specifically known from file data or
 * otherwise (e.g., sample periods "scan_delta") unaltered in case
 * the user has issued "set datafile" to define defaults.
 */
struct df_binary_file_record_struct {
    int cart_dim[3];                  // dimension array size, x/y/z 
    int cart_dir[3];                  // 1 scan in positive direction, -1 negative, x/y/z 
    double cart_delta[3];             // spacing between array points, x/y/z 
    df_translation_type cart_trans;   // translate via origin, center or default 
    double cart_cen_or_ori[3];        // vector representing center or origin, x/y/z 
    double cart_alpha;                // 2D rotation angle (rotate) 
    double cart_p[3];                 // 3D rotation normal vector (perpendicular) 

    df_sample_scan_type cart_scan[3]; // how to assign the dimensions read from file when generating coordinates 
    bool scan_generate_coord;     // whether or not Gnuplot should generate coordinates. 
    off_t scan_skip[3];               // skip bytes before the record, line, plane 
	//
    // Not controllable by the user, only by file type functions.
    // These are all points/lines/planes format.
    //
    int scan_dim[3];                  // number of points, lines, planes 
    int scan_dir[3];                  // 1 scan in positive direction wrt Cartesian GpCoordinate system, -1 negative 
    double scan_delta[3];             // sample period along points, lines, planes 
    df_translation_type scan_trans;   // translate via origin, center or default 
    double scan_cen_or_ori[3];        // vector representing center or origin, x/y/z 
    // *** Do not modify outside of datafile.c!!! *** 
    char GPFAR *memory_data;
};

enum df_multivalue_type {
	DF_DELTA,
	DF_FLIP_AXIS,
	DF_FLIP,
	DF_SCAN,
	DF_ORIGIN,
	DF_CENTER,
	DF_ROTATION,
	DF_PERPENDICULAR,
	DF_SKIP
};

#ifndef MAXINT                  /* should there be one already defined ? */
	#define MAXINT INT_MAX        /* from <limits.h> */
#endif
//
// rather than three arrays which all grow dynamically, make one dynamic array of this structure
//
struct df_column_struct {
	double datum;
	DF_STATUS good;
	char * position; /* points to start of this field in current line */
	char * header;  /* points to copy of the header for this column */
};

class GpDatafile {
public:
	static void AvsFiletypeFunction();
	static void F_StringColumn(GpArgument * arg);

	GpDatafile() : blank_data_line(UNDEFINED, -999.0, -999.0, -999.0, -999.0, -999.0, -999.0, -999.0)
	{
		MEMSZERO(df_tokens);
		MEMSZERO(df_matrix_corner);
		df_commentschars = 0;
		missing_val = 0;
		df_separators = 0;
		df_bin_record = 0;
		df_num_bin_records = 0;
		df_bin_record_count = 0;
		df_max_num_bin_records = 0;
		df_max_num_bin_records_default = 0;
		df_num_bin_records_default = 0;
		df_bin_filetype = 0;
		df_bin_filetype_default = 0;
		df_bin_file_endianess_default = DF_BIN_FILE_ENDIANESS_RESET;
		df_bin_file_endianess = DF_BIN_FILE_ENDIANESS_RESET;

		df_no_use_specs = 0;
		df_datum = 0;
		df_line_number = 0;
		df_last_col = 0;
		df_no_bin_cols = 0;
		df_pixeldata = 0;

		df_matrix = false;
		df_nofpe_trap = false;
		df_fortran_constants = false;
		plotted_data_from_stdin = false;
		evaluate_inside_using = false;
		df_warn_on_missing_columnheader = false;
		//
		//
		//
		df_line = NULL;
		max_line_len = 0;
		data_fp = NULL;
#if defined(PIPES)
		df_pipe_open = false;
#endif
#if defined(HAVE_FDOPEN)
		data_fd = -2;        // only used for file redirection 
#endif
		mixed_data_fp = false; // inline data 
		df_filename = NULL;      // name of data file 
		df_eof = 0;
		df_no_tic_specs = 0;
		blank_count = 0;     // how many blank lines recently 
		df_lower_index = 0;  // first mesh required 
		df_upper_index = MAXINT;
		df_index_step = 1;   // 'every' for indices 
		df_current_index = 0;    // current mesh 
		indexname = NULL;
		index_found = false;
		df_longest_columnhead = 0;
		everypoint = 1;
		firstpoint = 0;
		lastpoint = MAXINT;
		everyline = 1;
		firstline = 0;
		lastline = MAXINT;
		point_count = -1;     // point counter - preincrement and test 0
		line_count = 0;       // line counter
		df_skip_at_front = 0; // for ascii file "skip" lines at head of file 
		df_pseudodata = 0;
		df_pseudorecord = 0;
		df_pseudospan = 0;
		df_pseudovalue_0 = 0;
		df_pseudovalue_1 = 0;
		df_datablock = false;
		df_datablock_line = NULL;
		df_array_index = 0;
		df_xpixels = 0;
		df_ypixels = 0;
		df_transpose = false;
		df_format = NULL;
		df_binary_format = NULL;

		df_column = NULL;
		df_max_cols = 0;
		df_no_cols = 0;
		fast_columns = 0;
		//
		MEMSZERO(df_stringexpression);
		df_current_plot = 0;    // used to process histogram labels + key entries 
		column_for_key_title = NO_COLUMN_HEADER;
		df_already_got_headers = false;
		df_key_title = NULL;     // filled in from column header if requested 
		df_read_binary = false;
		df_nonuniform_matrix = false;
		df_matrix_columnheaders = false;
		df_matrix_rowheaders = false;
		df_plot_mode = 0;
		//
		df_column_bininfo = NULL;      /* allocate space as needed */
		df_max_bininfo_cols = 0;     /* space allocated */
		matrix_general_binary_conflict_msg = "Conflict between some matrix binary and general binary keywords";
	}
	void   InitializeUseSpec();
	void   InitializeBinaryVars();
	void   DfInit();
	int    DfOpen(const char * cmd_filename, int max_using, curve_points * plot);
	void   DfClose();
	int    DfReadLine(double v[], int max);
	int    DfReadAscii(double v[], int max);
	int    DfReadBinary(double v[], int max);
	float * DfReadMatrix(int * rows, int * cols);
	char * DfParseStringField(char * field);
	void   DfDetermineMatrixInfo(FILE * fin);
	void   DfSetDatafileBinary();
	void   DfUnsetDatafileBinary();
	void   DfShowData();
	void   DfShowBinary(FILE * fp);
	int    DfTokenise(char * s);
	void   PlotOptionUsing(int max_using);
	void   PlotOptionArray();
	void   PlotOptionMultiValued(df_multivalue_type type, int arg);
	void   PlotOptionBinary(bool set_matrix, bool set_default);
	void   PlotOptionBinaryFormat(char * format_string);
	void   PlotOptionIndex();
	void   PlotOptionEvery();
	void   PlotTicLabelUsing(int axis);
	void   AdjustBinaryUseSpec(curve_points * plot);
	void   DfAddBinaryRecords(int num_records_to_add, df_records_type records_type);
	void   DfInsertScannedUseSpec(int uspec);
	void   ClearBinaryRecords(df_records_type records_type);
	void   ClearDfColumnHeaders();
	void   DfSetKeyTitleColumnHead(curve_points * plot);
	void   DfSetKeyTitle(curve_points * plot);
	void   DfSetPlotMode(int mode);
	int    ExpectString(const char column);
	int    CheckMissing(char * s);
	char * DfGets();
	char * DfFGets(FILE * fin);
	int    DfSkipBytes(off_t nbytes);
	char * DfGenerateAsciiArrayEntry();
	char * DfGeneratePseudoData();
	void   ExpandDfColumn(int new_max);
	void   AddKeyEntry(char * temp_string, int df_datum);
	void   DfSetSkipBefore(int col, int bytes);
	void   DfSetReadType(int col, df_data_type type);
	df_data_type DfGetReadType(int col);
	int    DfGetReadSize(int col);
	void   DfExtendBinaryColumns(int no_cols);
	void   SetSeparator();
    
	const GpCoordinate blank_data_line;

	int    df_no_use_specs; // how many using columns were specified in the current command 
	int    df_datum; // suggested x value if none given 
	char * df_filename;
	int    df_line_number;
	AXIS_INDEX df_axis[MAXDATACOLS];
#ifdef BACKWARDS_COMPATIBLE
	UdftEntry ydata_func; // deprecated "thru" function 
#endif
	char * df_tokens[MAXDATACOLS]; // Returned to caller by df_readline() 
	int    df_last_col; // number of columns in first row of data return to user in STATS_columns 
	char * missing_val; // string representing missing values, ascii datafiles 
	char * df_separators; // input field separators, NULL if whitespace is the separator 
	char * df_commentschars; // comments chars 
	bool   df_matrix; // is this a matrix splot? 
	bool   df_binary; // is this a binary file? 
	bool   df_matrix_file;
	bool   df_binary_file;
	bool   plotted_data_from_stdin; // flag if any 'inline' data are in use, for the current plot 
	// Setting this allows the parser to recognize Fortran D or Q   
	// format constants in the input file. But it slows things down 
	bool   df_fortran_constants;
	// Setting this disables initialization of the floating point exception 
	// handler before every expression evaluation in a using specifier.   	 
	// This can speed data input significantly, but assumes valid input.    
	bool   df_nofpe_trap;
	bool   evaluate_inside_using;
	bool   df_warn_on_missing_columnheader;
	int    df_no_bin_cols; // cols to read 
	int    df_num_bin_records;
	int    df_max_num_bin_records;
	int    df_bin_record_count;
	int    df_max_num_bin_records_default;
	int    df_num_bin_records_default;
	int    df_bin_filetype; // Initially set to default and then possibly altered by command line
	int    df_bin_filetype_default;
	void * df_pixeldata;

	df_binary_file_record_struct * df_bin_record;
	df_endianess_type df_bin_file_endianess_default;
	df_endianess_type df_bin_file_endianess;
	double df_matrix_corner[2][2]; // First argument is corner, second argument is x (0) or y(1)

	UseSpecS use_spec[MAXDATACOLS];
	//
	// Bookkeeping for df_fgets() and df_gets(). Must be initialized before any calls to either function.
	//
	char * df_line;
	size_t max_line_len;
	FILE * data_fp;
	#if defined(PIPES)
	bool df_pipe_open;
	#endif
	#if defined(HAVE_FDOPEN)
	int data_fd;        // only used for file redirection 
	#endif
	bool mixed_data_fp; // inline data 
	int df_eof;
	int df_no_tic_specs;     // ticlabel columns not counted in df_no_use_specs 
	// stuff for implementing index 
	int blank_count;     // how many blank lines recently 
	int df_lower_index;  // first mesh required 
	int df_upper_index;
	int df_index_step;   // 'every' for indices 
	int df_current_index;    // current mesh 
	// stuff for named index support 
	char * indexname;
	bool index_found;
	int df_longest_columnhead;
	// stuff for every point:line 
	int everypoint;
	int firstpoint;
	int lastpoint;
	int everyline;
	int firstline;
	int lastline;
	int point_count;     // point counter - preincrement and test 0
	int line_count;       // line counter
	int df_skip_at_front; // for ascii file "skip" lines at head of file 
	//
	// for pseudo-data (1 if filename = '+'; 2 if filename = '++') 
	//
	int df_pseudodata;
	int df_pseudorecord;
	int df_pseudospan;
	double df_pseudovalue_0;
	double df_pseudovalue_1;
	// for datablocks 
	bool df_datablock;
	char ** df_datablock_line;
	// for arrays
	int df_array_index;
	// track dimensions of input matrix/array/image 
	uint df_xpixels;
	uint df_ypixels;
	bool df_transpose;
	// parsing stuff 
	char * df_format;
	char * df_binary_format;
	//
	df_column_struct * df_column;      // we'll allocate space as needed 
	int df_max_cols;     // space allocated 
	int df_no_cols;          // cols read 
	int fast_columns;        // corey@cac optimization 
	//
	char * df_stringexpression[MAXDATACOLS]; // filled in after evaluate_at()
	curve_points * df_current_plot; // used to process histogram labels + key entries
	int    column_for_key_title;
	bool   df_already_got_headers;
	char * df_key_title; // filled in from column header if requested
	//
	// Binary *read* variables used by df_readbinary().
	// There is a confusing difference between the ascii and binary "matrix" keywords.
	// Ascii matrix data by default is interpreted as having an implicit uniform grid
	// of x and y coords that are not actually present in the data file.
	// The equivalent binary data format is called "binary general".
	// In both of these cases the internal flag df_nonuniform_matrix is false;
	// Binary matrix data contains explicit y values in the first row, and explicit x
	// values in the first column. This is signalled by "binary matrix".
	// In this case the internal flag df_nonuniform_matrix is true.
	//
	// EAM May 2011 - Add a keyword "nonuniform matrix" to indicate ascii matrix data
	// in the same format as "binary matrix", i.e. with explicit x and y coordinates.
	// EAM Jul 2014 - Add keywords "columnheaders" and "rowheaders" to indicate ascii
	// matrix data in the uniform grid format containing labels in row 1 and column 1.
	//
	bool   df_read_binary;
	bool   df_nonuniform_matrix;
	bool   df_matrix_columnheaders;
	bool   df_matrix_rowheaders;
	int    df_plot_mode;
	//
	// Information about binary data structure, to be determined by the
	// using and format options.  This should be one greater than df_no_bin_cols.
	//
	df_column_bininfo_struct * df_column_bininfo;      /* allocate space as needed */
	int df_max_bininfo_cols;     /* space allocated */
	const char * matrix_general_binary_conflict_msg;
};

extern GpDatafile GpDf; // @global
//
// Prototypes of functions exported by datafile.c 
//
void df_show_datasizes(FILE *fp);
void df_show_filetypes(FILE *fp);
#define df_set_skip_after(col,bytes) GpDf.DfSetSkipBefore(col+1,bytes)  /* Number of bytes to skip after a binary column. */

#endif /* GNUPLOT_DATAFILE_H */
