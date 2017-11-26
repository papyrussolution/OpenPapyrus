/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * Copyright 2008-2013 David Hoerl All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY David Hoerl ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL David Hoerl OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * File description:
 *
 *	all include file for users
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// The defines are needed by the C library and OSX Framework Users
// The defines are needed by C++ Library Users

#if defined(__cplusplus)
#include "xlsys.h"
#include "xlstypes.h"
#include <string>
#include "record.h"
#include "globalrec.h"
#include "range.h"
#include "colinfo.h"
#include "row.h"
#include "formula.h"
#include "sheetrec.h"
#include "workbook.h"

#if defined(__FRAMEWORK__)
using namespace xlslib_core;
#endif

#else

#include "xlstypes.h"

// Summary options
typedef enum {
	PROP_AUTHOR = 1,
	PROP_CATEGORY,
	PROP_COMMENTS,
	PROP_COMPANY,
	PROP_CREATINGAPPLICATION,	// [i_a] Cannot see anywhere this is displayed (TODO: remove? use? reserved for future use?)
	PROP_KEYWORDS,
	PROP_MANAGER,
	PROP_REVISION,
	PROP_SUBJECT,
	PROP_TITLE,
	
	PROP_LAST
} property_t;

// Format options
typedef enum
{
  FMT_GENERAL = 0,
  FMT_NUMBER1,					// 0
  FMT_NUMBER2,					// 0.00
  FMT_NUMBER3,					// #,##0
  FMT_NUMBER4,					// #,##0.00
  FMT_CURRENCY1,				// "$"#,##0_);("$"#,##0)
  FMT_CURRENCY2,				// "$"#,##0_);[Red]("$"#,##0)
  FMT_CURRENCY3,				// "$"#,##0.00_);("$"#,##0.00)
  FMT_CURRENCY4,				// "$"#,##0.00_);[Red]("$"#,##0.00)
  FMT_PERCENT1,					// 0%
  FMT_PERCENT2,					// 0.00%
  FMT_SCIENTIFIC1,				// 0.00E+00
  FMT_FRACTION1,				// # ?/?
  FMT_FRACTION2,				// # ??/??
  FMT_DATE1,					// M/D/YY
  FMT_DATE2,					// D-MMM-YY
  FMT_DATE3,					// D-MMM
  FMT_DATE4,					// MMM-YY
  FMT_TIME1,					// h:mm AM/PM
  FMT_TIME2,					// h:mm:ss AM/PM
  FMT_TIME3,					// h:mm
  FMT_TIME4,					// h:mm:ss
  FMT_DATETIME,					// M/D/YY h:mm
  FMT_ACCOUNTING1,				// _(#,##0_);(#,##0)
  FMT_ACCOUNTING2,				// _(#,##0_);[Red](#,##0)
  FMT_ACCOUNTING3,				// _(#,##0.00_);(#,##0.00)
  FMT_ACCOUNTING4,				// _(#,##0.00_);[Red](#,##0.00)
  FMT_CURRENCY5,				// _("$"* #,##0_);_("$"* (#,##0);_("$"* "-"_);_(@_)
  FMT_CURRENCY6,				// _(* #,##0_);_(* (#,##0);_(* "-"_);_(@_)
  FMT_CURRENCY7,				// _("$"* #,##0.00_);_("$"* (#,##0.00);_("$"* "-"??_);_(@_)
  FMT_CURRENCY8,				// _(* #,##0.00_);_(* (#,##0.00);_(* "-"??_);_(@_)
  FMT_TIME5,					// mm:ss
  FMT_TIME6,					// [h]:mm:ss
  FMT_TIME7,					// mm:ss.0
  FMT_SCIENTIFIC2,				// ##0.0E+0
  FMT_TEXT          			// @
} format_number_t;
// good resource for format strings: http://www.mvps.org/dmcritchie/excel/formula.htm
// Good explanation of custom formats: http://www.ozgrid.com/Excel/CustomFormats.htm
// MS examples (need Windows): http://download.microsoft.com/download/excel97win/sample/1.0/WIN98Me/EN-US/Nmbrfrmt.exe
// Google this for MS help: "Create or delete a custom number format"

// Horizontal alignment
typedef enum
{
  HALIGN_GENERAL = 0,
  HALIGN_LEFT,
  HALIGN_CENTER,
  HALIGN_RIGHT,
  HALIGN_FILL,
  HALIGN_JUSTIFY,
  HALIGN_CENTERACCROSS
} halign_option_t;

// Vertical alignment
typedef enum
{
  VALIGN_TOP = 0,
  VALIGN_CENTER,
  VALIGN_BOTTOM,
  VALIGN_JUSTIFY
} valign_option_t;

// Text options
typedef enum
{
  ORI_NONE = 0,      
  ORI_TOPBOTTOMTXT,				// Letters stacked top to bottom but no rotation
  ORI_90NOCLOCKTXT,				// Text rotated 90 degrees counterclockwise
  ORI_90CLOCKTXT				// Text rotated 90 degrees clockwise
} txtori_option_t;

// Text rotation angle (unimplemented, need it? Just ask for it)
// TODO

// Text indention - horizontal alignment must be Left
typedef enum
{
  INDENT_0 = 0,
  INDENT_1,
  INDENT_2,
  INDENT_3,
  INDENT_4,
  INDENT_5,
  INDENT_6,
  INDENT_7,
  INDENT_8,
  INDENT_9,
  INDENT_10,
  INDENT_11,
  INDENT_12,
  INDENT_13,
  INDENT_14,
  INDENT_15,
  INDENT_SHRINK2FIT,
  INDENT_L2R,
  INDENT_R2L
} indent_option_t;

// Foreground, background, and text color options. Change to BIFF8 may cause some of these to be incorrect...
typedef enum
{
  COLOR_BLACK,
  COLOR_DARK_RED,
  COLOR_RED,
  COLOR_PINK,
  COLOR_ROSE,
  COLOR_BROWN,
  COLOR_ORANGE,
  COLOR_LIGHT_ORANGE,
  COLOR_GOLD,
  COLOR_TAN,
  COLOR_OLIVE_GREEN,
  COLOR_DARK_YELLOW,
  COLOR_LIME,
  COLOR_YELLOW,
  COLOR_LIGHT_YELLOW,
  COLOR_DARK_GREEN,
  COLOR_GREEN,
  COLOR_COMBINED08,
  COLOR_BRIGHT_GREEN,
  COLOR_LIGHT_GREEN,
  COLOR_DARK_TEAL,
  COLOR_TEAL,
  COLOR_AQUA,
  COLOR_TURQUOISE,
  COLOR_LIGHT_TURQUOISE,
  COLOR_DARK_BLUE,
  COLOR_BLUE,
  COLOR_LIGHT_BLUE,
  COLOR_SKY_BLUE,
  COLOR_PALEBLUE,
  COLOR_INDIGO,
  COLOR_BLUE_GRAY,
  COLOR_VIOLET,
  COLOR_PLUM,
  COLOR_LAVANDER,
  COLOR_GRAY80,
  COLOR_GRAY50,
  COLOR_GRAY40,
  COLOR_GRAY25,
  COLOR_WHITE,
  COLOR_SYS_WIND_FG,
  COLOR_SYS_WIND_BG        
} color_name_t;

// Cell fill
typedef enum
{
  FILL_NONE = 0,
  FILL_SOLID,
  FILL_ATEN75,
  FILL_ATEN50,
  FILL_ATEN25,
  FILL_ATEN12,
  FILL_ATEN06,
  FILL_HORIZ_LIN,
  FILL_VERTICAL_LIN,
  FILL_DIAG,
  FILL_INV_DIAG,
  FILL_INTER_DIAG,
  FILL_DIAG_THICK_INTER,
  FILL_HORIZ_LINES_THIN,
  FILL_VERTICAL_LINES_THIN,
  FILL_DIAG_THIN,
  FILL_INV_DIAG_THIN,
  FILL_HORIZ_INT_THIN,
  FILL_HORIZ_INTER_THICK   
} fill_option_t;

// Border lines
typedef enum
{
  BORDER_NONE = 0,
  BORDER_THIN,
  BORDER_MEDIUM,
  BORDER_DASHED,
  BORDER_DOTTED,
  BORDER_THICK,
  BORDER_DOUBLE,
  BORDER_HAIR    
} border_style_t;

// Where to put border lines
typedef enum
{
  BORDER_BOTTOM = 0,
  BORDER_TOP,
  BORDER_LEFT,
  BORDER_RIGHT 
} border_side_t;

// Thickness
typedef enum
{
  BOLDNESS_BOLD = 0,
  BOLDNESS_HALF,
  BOLDNESS_NORMAL,
  BOLDNESS_DOUBLE
} boldness_option_t;

// Sub/super script
typedef enum
{
  SCRIPT_NONE = 0,
  SCRIPT_SUPER,
  SCRIPT_SUB
} script_option_t;

// Underlining
typedef enum
{
  UNDERLINE_NONE = 0,
  UNDERLINE_SINGLE,
  UNDERLINE_DOUBLE,
  UNDERLINE_SINGLEACC,
  UNDERLINE_DOUBLEACC

} underline_option_t;

typedef enum
{
	XLERR_NULL  = 0x00, // #NULL!
	XLERR_DIV0  = 0x07, // #DIV/0!
	XLERR_VALUE = 0x0F, // #VALUE!
	XLERR_REF   = 0x17, // #REF!
	XLERR_NAME  = 0x1D, // #NAME?
	XLERR_NUM   = 0x24, // #NUM!
	XLERR_N_A   = 0x2A, // #N/A!
} errcode_t;

typedef enum 
{
    CELL_RELATIVE_A1   = 0xC000,
    CELL_ABSOLUTE_As1  = 0x8000,
    CELL_ABSOLUTE_sA1  = 0x4000,
    CELL_ABSOLUTE_sAs1 = 0,
} cell_addr_mode_t;

typedef enum {
		OP_EXP = 0x01,                                 // ptgExp          01h   control
		OP_TBL = 0x02,                                 // ptgTbl          02h   control
		OP_ADD = 0x03,                                 // ptgAdd          03h   operator
		OP_SUB = 0x04,                                 // ptgSub          04h   operator
		OP_MUL = 0x05,                                 // ptgMul          05h   operator
		OP_DIV = 0x06,                                 // ptgDiv          06h   operator
		OP_POWER = 0x07,                               // ptgPower        07h   operator
		OP_CONCAT = 0x08,                              // ptgConcat       08h   operator
		OP_LT = 0x09,                                  // ptgLT           09h   operator
		OP_LE = 0x0A,                                  // ptgLE           0Ah   operator
		OP_EQ = 0x0B,                                  // ptgEQ           0Bh   operator
		OP_GE = 0x0C,                                  // ptgGE           0Ch   operator
		OP_GT = 0x0D,                                  // ptgGT           0Dh   operator
		OP_NE = 0x0E,                                  // ptgNE           0Eh   operator
		OP_ISECT = 0x0F,                               // ptgIsect        0Fh   operator
		OP_UNION = 0x10,                               // ptgUnion        10h   operator
		OP_RANGE = 0x11,                               // ptgRange        11h   operator
		OP_UPLUS = 0x12,                               // ptgUplus        12h   operator
		OP_UMINUS = 0x13,                              // ptgUminus       13h   operator
		OP_PERCENT = 0x14,                             // ptgPercent      14h   operator
		OP_PAREN = 0x15,                               // ptgParen        15h   control
		OP_MISSARG = 0x16,                             // ptgMissArg      16h   operand
		OP_STR = 0x17,                                 // ptgStr          17h   operand
		OP_ATTR = 0x19,                                // ptgAttr         19h   control
		OP_SHEET = 0x1A,                               // ptgSheet        1Ah   (ptg DELETED)
		OP_ENDSHEET = 0x1B,                            // ptgEndSheet     1Bh   (ptg DELETED)
		OP_ERR = 0x1C,                                 // ptgErr          1Ch   operand
		OP_BOOL = 0x1D,                                // ptgBool         1Dh   operand
		OP_INT = 0x1E,                                 // ptgInt          1Eh   operand
		OP_NUM = 0x1F,                                 // ptgNum          1Fh   operand
		OP_ARRAY = 0x20,                               // ptgArray        20h   operand, reference class
		OP_FUNC = 0x21,                                // ptgFunc         21h   operator
		OP_FUNCVAR = 0x22,                             // ptgFuncVar      22h   operator
		OP_NAME = 0x23,                                // ptgName         23h   operand, reference class
		OP_REF = 0x24,                                 // ptgRef          24h   operand, reference class
		OP_AREA = 0x25,                                // ptgArea         25h   operand, reference class
		OP_MEMAREA = 0x26,                             // ptgMemArea      26h   operand, reference class
		OP_MEMERR = 0x27,                              // ptgMemErr       27h   operand, reference class
		OP_MEMNOMEM = 0x28,                            // ptgMemNoMem     28h   control
		OP_MEMFUNC = 0x29,                             // ptgMemFunc      29h   control
		OP_REFERR = 0x2A,                              // ptgRefErr       2Ah   operand, reference class
		OP_AREAERR = 0x2B,                             // ptgAreaErr      2Bh   operand, reference class
		OP_REFN = 0x2C,                                // ptgRefN         2Ch   operand, reference class
		OP_AREAN = 0x2D,                               // ptgAreaN        2Dh   operand, reference class
		OP_MEMAREAN = 0x2E,                            // ptgMemAreaN     2Eh   control
		OP_MEMNOMEMN = 0x2F,                           // ptgMemNoMemN    2Fh   control
		OP_NAMEX = 0x39,                               // ptgNameX        39h   operand, reference class
		OP_REF3D = 0x3A,                               // ptgRef3d        3Ah   operand, reference class
		OP_AREA3D = 0x3B,                              // ptgArea3d       3Bh   operand, reference class
		OP_REFERR3D = 0x3C,                            // ptgRefErr3d     3Ch   operand, reference class
		OP_AREAERR3D = 0x3D,                           // ptgAreaErr3d    3Dh   operand, reference class
		OP_ARRAYV = 0x40,                              // ptgArrayV       40h   operand, value class
		OP_FUNCV = 0x41,                               // ptgFuncV        41h   operator
		OP_FUNCVARV = 0x42,                            // ptgFuncVarV     42h   operator
		OP_NAMEV = 0x43,                               // ptgNameV        43h   operand, value class
		OP_REFV = 0x44,                                // ptgRefV         44h   operand, value class
		OP_AREAV = 0x45,                               // ptgAreaV        45h   operand, value class
		OP_MEMAREAV = 0x46,                            // ptgMemAreaV     46h   operand, value class
		OP_MEMERRV = 0x47,                             // ptgMemErrV      47h   operand, value class
		OP_MEMNOMEMV = 0x48,                           // ptgMemNoMemV    48h   control
		OP_MEMFUNCV = 0x49,                            // ptgMemFuncV     49h   control
		OP_REFERRV = 0x4A,                             // ptgRefErrV      4Ah   operand, value class
		OP_AREAERRV = 0x4B,                            // ptgAreaErrV     4Bh   operand, value class
		OP_REFNV = 0x4C,                               // ptgRefNV        4Ch   operand, value class
		OP_AREANV = 0x4D,                              // ptgAreaNV       4Dh   operand, value class
		OP_MEMAREANV = 0x4E,                           // ptgMemAreaNV    4Eh   control
		OP_MEMNOMEMNV = 0x4F,                          // ptgMemNoMemNV   4Fh   control
		OP_FUNCCEV = 0x58,                             // ptgFuncCEV      58h   operator
		OP_NAMEXV = 0x59,                              // ptgNameXV       59h   operand, value class
		OP_REF3DV = 0x5A,                              // ptgRef3dV       5Ah   operand, value class
		OP_AREA3DV = 0x5B,                             // ptgArea3dV      5Bh   operand, value class
		OP_REFERR3DV = 0x5C,                           // ptgRefErr3dV    5Ch   operand, value class
		OP_AREAERR3DV = 0x5D,                          // ptgAreaErr3dV   5Dh   operand, value class
		OP_ARRAYA = 0x60,                              // ptgArrayA       60h   operand, array class
		OP_FUNCA = 0x61,                               // ptgFuncA        61h   operator
		OP_FUNCVARA = 0x62,                            // ptgFuncVarA     62h   operator
		OP_NAMEA = 0x63,                               // ptgNameA        63h   operand, array class
		OP_REFA = 0x64,                                // ptgRefA         64h   operand, array class
		OP_AREAA = 0x65,                               // ptgAreaA        65h   operand, array class
		OP_MEMAREAA = 0x66,                            // ptgMemAreaA     66h   operand, array class
		OP_MEMERRA = 0x67,                             // ptgMemErrA      67h   operand, array class
		OP_MEMNOMEMA = 0x68,                           // ptgMemNoMemA    68h   control
		OP_MEMFUNCA = 0x69,                            // ptgMemFuncA     69h   control
		OP_REFERRA = 0x6A,                             // ptgRefErrA      6Ah   operand, array class
		OP_AREAERRA = 0x6B,                            // ptgAreaErrA     6Bh   operand, array class
		OP_REFNA = 0x6C,                               // ptgRefNA        6Ch   operand, array class
		OP_AREANA = 0x6D,                              // ptgAreaNA       6Dh   operand, array class
		OP_MEMAREANA = 0x6E,                           // ptgMemAreaNA    6Eh   control
		OP_MEMNOMEMNA = 0x6F,                          // ptgMemNoMemNA   6Fh   control
		OP_FUNCCEA = 0x78,                             // ptgFuncCEA      78h   operator
		OP_NAMEXA = 0x79,                              // ptgNameXA       79h   operand, array class (NEW ptg)
		OP_REF3DA = 0x7A,                              // ptgRef3dA       7Ah   operand, array class (NEW ptg)
		OP_AREA3DA = 0x7B,                             // ptgArea3dA      7Bh   operand, array class (NEW ptg)
		OP_REFERR3DA = 0x7C,                           // ptgRefErr3dA    7Ch   operand, array class (NEW ptg)
		OP_AREAERR3DA = 0x7D,                          // ptgAreaErr3dA   7Dh   operand, array class (NEW ptg)
} expr_operator_code_t;

typedef enum {
    /*
     * User defined function
     *
     * First argument should be a function reference.
     */
    FUNC_UDF = 255,

    /*
     * Built-in Excel functions and command equivalents
     */
    FUNC_COUNT = 0,
    FUNC_IF,
    FUNC_ISNA,
    FUNC_ISERROR,
    FUNC_SUM,
    FUNC_AVERAGE,
    FUNC_MIN,
    FUNC_MAX,
    FUNC_ROW,
    FUNC_COLUMN,
    FUNC_NA,
    FUNC_NPV,
    FUNC_STDEV,
    FUNC_DOLLAR,
    FUNC_FIXED,
    FUNC_SIN,
    FUNC_COS,
    FUNC_TAN,
    FUNC_ATAN,
    FUNC_PI,
    FUNC_SQRT,
    FUNC_EXP,
    FUNC_LN,
    FUNC_LOG10,
    FUNC_ABS,
    FUNC_INT,
    FUNC_SIGN,
    FUNC_ROUND,
    FUNC_LOOKUP,
    FUNC_INDEX,
    FUNC_REPT,
    FUNC_MID,
    FUNC_LEN,
    FUNC_VALUE,
    FUNC_TRUE,
    FUNC_FALSE,
    FUNC_AND,
    FUNC_OR,
    FUNC_NOT,
    FUNC_MOD,
    FUNC_DCOUNT,
    FUNC_DSUM,
    FUNC_DAVERAGE,
    FUNC_DMIN,
    FUNC_DMAX,
    FUNC_DSTDEV,
    FUNC_VAR,
    FUNC_DVAR,
    FUNC_TEXT,
    FUNC_LINEST,
    FUNC_TREND,
    FUNC_LOGEST,
    FUNC_GROWTH,
    FUNC_GOTO,
    FUNC_HALT,
    FUNC_PV = 56,
    FUNC_FV,
    FUNC_NPER,
    FUNC_PMT,
    FUNC_RATE,
    FUNC_MIRR,
    FUNC_IRR,
    FUNC_RAND,
    FUNC_MATCH,
    FUNC_DATE,
    FUNC_TIME,
    FUNC_DAY,
    FUNC_MONTH,
    FUNC_YEAR,
    FUNC_WEEKDAY,
    FUNC_HOUR,
    FUNC_MINUTE,
    FUNC_SECOND,
    FUNC_NOW,
    FUNC_AREAS,
    FUNC_ROWS,
    FUNC_COLUMNS,
    FUNC_OFFSET,
    FUNC_ABSREF,
    FUNC_RELREF,
    FUNC_ARGUMENT,
    FUNC_SEARCH,
    FUNC_TRANSPOSE,
    FUNC_ERROR,
    FUNC_STEP,
    FUNC_TYPE,
    FUNC_ECHO,
    FUNC_SETNAME,
    FUNC_CALLER,
    FUNC_DEREF,
    FUNC_WINDOWS,
    FUNC_SERIES,
    FUNC_DOCUMENTS,
    FUNC_ACTIVECELL,
    FUNC_SELECTION,
    FUNC_RESULT,
    FUNC_ATAN2,
    FUNC_ASIN,
    FUNC_ACOS,
    FUNC_CHOOSE,
    FUNC_HLOOKUP,
    FUNC_VLOOKUP,
    FUNC_LINKS,
    FUNC_INPUT,
    FUNC_ISREF,
    FUNC_GETFORMULA,
    FUNC_GETNAME,
    FUNC_SETVALUE,
    FUNC_LOG,
    FUNC_EXEC,
    FUNC_CHAR,
    FUNC_LOWER,
    FUNC_UPPER,
    FUNC_PROPER,
    FUNC_LEFT,
    FUNC_RIGHT,
    FUNC_EXACT,
    FUNC_TRIM,
    FUNC_REPLACE,
    FUNC_SUBSTITUTE,
    FUNC_CODE,
    FUNC_NAMES,
    FUNC_DIRECTORY,
    FUNC_FIND,
    FUNC_CELL,
    FUNC_ISERR,
    FUNC_ISTEXT,
    FUNC_ISNUMBER,
    FUNC_ISBLANK,
    FUNC_T,
    FUNC_N,
    FUNC_FOPEN,
    FUNC_FCLOSE,
    FUNC_FSIZE,
    FUNC_FREADLN,
    FUNC_FREAD,
    FUNC_FWRITELN,
    FUNC_FWRITE,
    FUNC_FPOS,
    FUNC_DATEVALUE,
    FUNC_TIMEVALUE,
    FUNC_SLN,
    FUNC_SYD,
    FUNC_DDB,
    FUNC_GETDEF,
    FUNC_REFTEXT,
    FUNC_TEXTREF,
    FUNC_INDIRECT,
    FUNC_REGISTER,
    FUNC_CALL,
    FUNC_ADDBAR,
    FUNC_ADDMENU,
    FUNC_ADDCOMMAND,
    FUNC_ENABLECOMMAND,
    FUNC_CHECKCOMMAND,
    FUNC_RENAMECOMMAND,
    FUNC_SHOWBAR,
    FUNC_DELETEMENU,
    FUNC_DELETECOMMAND,
    FUNC_GETCHARTITEM,
    FUNC_DIALOGBOX,
    FUNC_CLEAN,
    FUNC_MDETERM,
    FUNC_MINVERSE,
    FUNC_MMULT,
    FUNC_FILES,
    FUNC_IPMT,
    FUNC_PPMT,
    FUNC_COUNTA,
    FUNC_CANCELKEY,
    FUNC_INITIATE = 175,
    FUNC_REQUEST,
    FUNC_POKE,
    FUNC_EXECUTE,
    FUNC_TERMINATE,
    FUNC_RESTART,
    FUNC_HELP,
    FUNC_GETBAR,
    FUNC_PRODUCT,
    FUNC_FACT,
    FUNC_GETCELL,
    FUNC_GETWORKSPACE,
    FUNC_GETWINDOW,
    FUNC_GETDOCUMENT,
    FUNC_DPRODUCT,
    FUNC_ISNONTEXT,
    FUNC_GETNOTE,
    FUNC_NOTE,
    FUNC_STDEVP,
    FUNC_VARP,
    FUNC_DSTDEVP,
    FUNC_DVARP,
    FUNC_TRUNC,
    FUNC_ISLOGICAL,
    FUNC_DCOUNTA,
    FUNC_DELETEBAR,
    FUNC_UNREGISTER,
    FUNC_USDOLLAR = 204,
    FUNC_FINDB,
    FUNC_SEARCHB,
    FUNC_REPLACEB,
    FUNC_LEFTB,
    FUNC_RIGHTB,
    FUNC_MIDB,
    FUNC_LENB,
    FUNC_ROUNDUP,
    FUNC_ROUNDDOWN,
    FUNC_ASC,
    FUNC_DBCS,
    FUNC_RANK,
    FUNC_ADDRESS = 219,
    FUNC_DAYS360,
    FUNC_TODAY,
    FUNC_VDB,
    FUNC_MEDIAN = 227,
    FUNC_SUMPRODUCT,
    FUNC_SINH,
    FUNC_COSH,
    FUNC_TANH,
    FUNC_ASINH,
    FUNC_ACOSH,
    FUNC_ATANH,
    FUNC_DGET,
    FUNC_CREATEOBJECT,
    FUNC_VOLATILE,
    FUNC_LASTERROR,
    FUNC_CUSTOMUNDO,
    FUNC_CUSTOMREPEAT,
    FUNC_FORMULACONVERT,
    FUNC_GETLINKINFO,
    FUNC_TEXTBOX,
    FUNC_INFO,
    FUNC_GROUP,
    FUNC_GETOBJECT,
    FUNC_DB,
    FUNC_PAUSE,
    FUNC_RESUME = 251,
    FUNC_FREQUENCY,
    FUNC_ADDTOOLBAR,
    FUNC_DELETETOOLBAR,
    FUNC_RESETTOOLBAR = 256,
    FUNC_EVALUATE,
    FUNC_GETTOOLBAR,
    FUNC_GETTOOL,
    FUNC_SPELLINGCHECK,
    FUNC_ERRORTYPE,
    FUNC_APPTITLE,
    FUNC_WINDOWTITLE,
    FUNC_SAVETOOLBAR,
    FUNC_ENABLETOOL,
    FUNC_PRESSTOOL,
    FUNC_REGISTERID,
    FUNC_GETWORKBOOK,
    FUNC_AVEDEV,
    FUNC_BETADIST,
    FUNC_GAMMALN,
    FUNC_BETAINV,
    FUNC_BINOMDIST,
    FUNC_CHIDIST,
    FUNC_CHIINV,
    FUNC_COMBIN,
    FUNC_CONFIDENCE,
    FUNC_CRITBINOM,
    FUNC_EVEN,
    FUNC_EXPONDIST,
    FUNC_FDIST,
    FUNC_FINV,
    FUNC_FISHER,
    FUNC_FISHERINV,
    FUNC_FLOOR,
    FUNC_GAMMADIST,
    FUNC_GAMMAINV,
    FUNC_CEILING,
    FUNC_HYPGEOMDIST,
    FUNC_LOGNORMDIST,
    FUNC_LOGINV,
    FUNC_NEGBINOMDIST,
    FUNC_NORMDIST,
    FUNC_NORMSDIST,
    FUNC_NORMINV,
    FUNC_NORMSINV,
    FUNC_STANDARDIZE,
    FUNC_ODD,
    FUNC_PERMUT,
    FUNC_POISSON,
    FUNC_TDIST,
    FUNC_WEIBULL,
    FUNC_SUMXMY2,
    FUNC_SUMX2MY2,
    FUNC_SUMX2PY2,
    FUNC_CHITEST,
    FUNC_CORREL,
    FUNC_COVAR,
    FUNC_FORECAST,
    FUNC_FTEST,
    FUNC_INTERCEPT,
    FUNC_PEARSON,
    FUNC_RSQ,
    FUNC_STEYX,
    FUNC_SLOPE,
    FUNC_TTEST,
    FUNC_PROB,
    FUNC_DEVSQ,
    FUNC_GEOMEAN,
    FUNC_HARMEAN,
    FUNC_SUMSQ,
    FUNC_KURT,
    FUNC_SKEW,
    FUNC_ZTEST,
    FUNC_LARGE,
    FUNC_SMALL,
    FUNC_QUARTILE,
    FUNC_PERCENTILE,
    FUNC_PERCENTRANK,
    FUNC_MODE,
    FUNC_TRIMMEAN,
    FUNC_TINV,
    FUNC_MOVIECOMMAND = 334,
    FUNC_GETMOVIE,
    FUNC_CONCATENATE,
    FUNC_POWER,
    FUNC_PIVOTADDDATA,
    FUNC_GETPIVOTTABLE,
    FUNC_GETPIVOTFIELD,
    FUNC_GETPIVOTITEM,
    FUNC_RADIANS,
    FUNC_DEGREES,
    FUNC_SUBTOTAL,
    FUNC_SUMIF,
    FUNC_COUNTIF,
    FUNC_COUNTBLANK,
    FUNC_SCENARIOGET,
    FUNC_OPTIONSLISTSGET,
    FUNC_ISPMT,
    FUNC_DATEDIF,
    FUNC_DATESTRING,
    FUNC_NUMBERSTRING,
    FUNC_ROMAN,
    FUNC_OPENDIALOG,
    FUNC_SAVEDIALOG,
    FUNC_VIEWGET,
    FUNC_GETPIVOTDATA,
    FUNC_HYPERLINK,
    FUNC_PHONETIC,
    FUNC_AVERAGEA,
    FUNC_MAXA,
    FUNC_MINA,
    FUNC_STDEVPA,
    FUNC_VARPA,
    FUNC_STDEVA,
    FUNC_VARA,
    FUNC_BAHTTEXT,
    FUNC_THAIDAYOFWEEK,
    FUNC_THAIDIGIT,
    FUNC_THAIMONTHOFYEAR,
    FUNC_THAINUMSOUND,
    FUNC_THAINUMSTRING,
    FUNC_THAISTRINGLENGTH,
    FUNC_ISTHAIDIGIT,
    FUNC_ROUNDBAHTDOWN,
    FUNC_ROUNDBAHTUP,
    FUNC_THAIYEAR,
    FUNC_RTD,
    FUNC_CUBEVALUE,
    FUNC_CUBEMEMBER,
    FUNC_CUBEMEMBERPROPERTY,
    FUNC_CUBERANKEDMEMBER,
    FUNC_HEX2BIN,
    FUNC_HEX2DEC,
    FUNC_HEX2OCT,
    FUNC_DEC2BIN,
    FUNC_DEC2HEX,
    FUNC_DEC2OCT,
    FUNC_OCT2BIN,
    FUNC_OCT2HEX,
    FUNC_OCT2DEC,
    FUNC_BIN2DEC,
    FUNC_BIN2OCT,
    FUNC_BIN2HEX,
    FUNC_IMSUB,
    FUNC_IMDIV,
    FUNC_IMPOWER,
    FUNC_IMABS,
    FUNC_IMSQRT,
    FUNC_IMLN,
    FUNC_IMLOG2,
    FUNC_IMLOG10,
    FUNC_IMSIN,
    FUNC_IMCOS,
    FUNC_IMEXP,
    FUNC_IMARGUMENT,
    FUNC_IMCONJUGATE,
    FUNC_IMAGINARY,
    FUNC_IMREAL,
    FUNC_COMPLEX,
    FUNC_IMSUM,
    FUNC_IMPRODUCT,
    FUNC_SERIESSUM,
    FUNC_FACTDOUBLE,
    FUNC_SQRTPI,
    FUNC_QUOTIENT,
    FUNC_DELTA,
    FUNC_GESTEP,
    FUNC_ISEVEN,
    FUNC_ISODD,
    FUNC_MROUND,
    FUNC_ERF,
    FUNC_ERFC,
    FUNC_BESSELJ,
    FUNC_BESSELK,
    FUNC_BESSELY,
    FUNC_BESSELI,
    FUNC_XIRR,
    FUNC_XNPV,
    FUNC_PRICEMAT,
    FUNC_YIELDMAT,
    FUNC_INTRATE,
    FUNC_RECEIVED,
    FUNC_DISC,
    FUNC_PRICEDISC,
    FUNC_YIELDDISC,
    FUNC_TBILLEQ,
    FUNC_TBILLPRICE,
    FUNC_TBILLYIELD,
    FUNC_PRICE,
    FUNC_YIELD,
    FUNC_DOLLARDE,
    FUNC_DOLLARFR,
    FUNC_NOMINAL,
    FUNC_EFFECT,
    FUNC_CUMPRINC,
    FUNC_CUMIPMT,
    FUNC_EDATE,
    FUNC_EOMONTH,
    FUNC_YEARFRAC,
    FUNC_COUPDAYBS,
    FUNC_COUPDAYS,
    FUNC_COUPDAYSNC,
    FUNC_COUPNCD,
    FUNC_COUPNUM,
    FUNC_COUPPCD,
    FUNC_DURATION,
    FUNC_MDURATION,
    FUNC_ODDLPRICE,
    FUNC_ODDLYIELD,
    FUNC_ODDFPRICE,
    FUNC_ODDFYIELD,
    FUNC_RANDBETWEEN,
    FUNC_WEEKNUM,
    FUNC_AMORDEGRC,
    FUNC_AMORLINC,
    FUNC_CONVERT,
    FUNC_ACCRINT,
    FUNC_ACCRINTM,
    FUNC_WORKDAY,
    FUNC_NETWORKDAYS,
    FUNC_GCD,
    FUNC_MULTINOMIAL,
    FUNC_LCM,
    FUNC_FVSCHEDULE,
    FUNC_CUBEKPIMEMBER,
    FUNC_CUBESET,
    FUNC_CUBESETCOUNT,
    FUNC_IFERROR,
    FUNC_COUNTIFS,
    FUNC_SUMIFS,
    FUNC_AVERAGEIF,
    FUNC_AVERAGEIFS,
    FUNC_AGGREGATE,
    FUNC_BINOM_DIST,
    FUNC_BINOM_INV,
    FUNC_CONFIDENCE_NORM,
    FUNC_CONFIDENCE_T,
    FUNC_CHISQ_TEST,
    FUNC_F_TEST,
    FUNC_COVARIANCE_P,
    FUNC_COVARIANCE_S,
    FUNC_EXPON_DIST,
    FUNC_GAMMA_DIST,
    FUNC_GAMMA_INV,
    FUNC_MODE_MULT,
    FUNC_MODE_SNGL,
    FUNC_NORM_DIST,
    FUNC_NORM_INV,
    FUNC_PERCENTILE_EXC,
    FUNC_PERCENTILE_INC,
    FUNC_PERCENTRANK_EXC,
    FUNC_PERCENTRANK_INC,
    FUNC_POISSON_DIST,
    FUNC_QUARTILE_EXC,
    FUNC_QUARTILE_INC,
    FUNC_RANK_AVG,
    FUNC_RANK_EQ,
    FUNC_STDEV_S,
    FUNC_STDEV_P,
    FUNC_T_DIST,
    FUNC_T_DIST_2T,
    FUNC_T_DIST_RT,
    FUNC_T_INV,
    FUNC_T_INV_2T,
    FUNC_VAR_S,
    FUNC_VAR_P,
    FUNC_WEIBULL_DIST,
    FUNC_NETWORKDAYS_INTL,
    FUNC_WORKDAY_INTL,
    FUNC_ECMA_CEILING,
    FUNC_ISO_CEILING,
    FUNC_BETA_DIST = 525,
    FUNC_BETA_INV,
    FUNC_CHISQ_DIST,
    FUNC_CHISQ_DIST_RT,
    FUNC_CHISQ_INV,
    FUNC_CHISQ_INV_RT,
    FUNC_F_DIST,
    FUNC_F_DIST_RT,
    FUNC_F_INV,
    FUNC_F_INV_RT,
    FUNC_HYPGEOM_DIST,
    FUNC_LOGNORM_DIST,
    FUNC_LOGNORM_INV,
    FUNC_NEGBINOM_DIST,
    FUNC_NORM_S_DIST,
    FUNC_NORM_S_INV,
    FUNC_T_TEST,
    FUNC_Z_TEST,
    FUNC_ERF_PRECISE,
    FUNC_ERFC_PRECISE,
    FUNC_GAMMALN_PRECISE,
    FUNC_CEILING_PRECISE,
    FUNC_FLOOR_PRECISE,
} expr_function_code_t;

enum {
    DVAL_TYPE_ANY = 0x00,
    DVAL_TYPE_INTEGER = 0x01,
    DVAL_TYPE_DECIMAL = 0x02,
    DVAL_TYPE_LIST = 0x03,
    DVAL_TYPE_DATE = 0x04,
    DVAL_TYPE_TIME = 0x05,
    DVAL_TYPE_LENGTH = 0x06,
    DVAL_TYPE_FORMULA = 0x07
};

enum {
    DVAL_ERROR_STOP = 0x00,
    DVAL_ERROR_WARNING = 0x10,
    DVAL_ERROR_INFO = 0x20,
};

enum {
    DVAL_STRING_LIST_IN_FORMULA = 0x0080,
    DVAL_EMPTY_OK = 0x0100,
    DVAL_STRING_LIST_SUPPRESS_DROPDOWN = 0x0200,
    DVAL_SHOW_PROMPT_IF_SELECTED = 0x040000,
    DVAL_SHOW_ERROR_IF_INVALID   = 0x080000
};

enum {
    DVAL_OP_BETWEEN          = 0x000000,
    DVAL_OP_NOT_BETWEEN      = 0x100000,
    DVAL_OP_EQUAL            = 0x200000,
    DVAL_OP_NOT_EQUAL        = 0x300000,
    DVAL_OP_GREATER_THAN     = 0x400000,
    DVAL_OP_LESS_THAN        = 0x500000,
    DVAL_OP_GREATER_OR_EQUAL = 0x600000,
    DVAL_OP_LESS_OR_EQUAL    = 0x700000,
};

#endif // ifdef __cpluplus

#if !(defined(__cplusplus)) || defined(CPP_BRIDGE_XLS)

#ifdef CPP_BRIDGE_XLS

#define EXTERN_TYPE
using namespace xlslib_core;
using namespace xlslib_strings;

extern "C" {

#else

#define EXTERN_TYPE extern

typedef wchar_t unichar_t;

typedef struct _workbook workbook;
typedef struct _worksheet worksheet;
typedef struct _font_t font_t;
typedef struct _format_t format_t;
typedef struct _cell_t cell_t;
typedef struct _formula_t formula_t;
typedef struct _xf_t xf_t;
typedef struct _range range;

#endif // CPP_BRIDGE_XLS

// Workbook
EXTERN_TYPE workbook *xlsNewWorkbook(void);
EXTERN_TYPE void xlsDeleteWorkbook(workbook *w);

EXTERN_TYPE worksheet *xlsWorkbookSheet(workbook *w, const char *sheetname);
EXTERN_TYPE worksheet *xlsWorkbookSheetW(workbook *w, const unichar_t *sheetname);
EXTERN_TYPE worksheet *xlsWorkbookGetSheet(workbook *w, uint16 sheetnum);
EXTERN_TYPE font_t *xlsWorkbookFont(workbook *w, const char *name);
EXTERN_TYPE format_t *xlsWorkbookFormat(workbook *w, const char *name);
EXTERN_TYPE format_t *xlsWorkbookFormatW(workbook *w, const unichar_t *name);
EXTERN_TYPE xf_t *xlsWorkbookxFormat(workbook *w);
EXTERN_TYPE xf_t *xlsWorkbookxFormatFont(workbook *w, font_t *font);
#ifdef HAVE_ICONV
EXTERN_TYPE	int xlsWorkbookIconvInType(workbook *w, const char *inType);
#endif
EXTERN_TYPE	uint8 xlsWorkbookProperty(workbook *w, property_t prop, const char *s);
EXTERN_TYPE	void xlsWorkBookWindPosition(workbook *w, uint16 horz, uint16 vert);
EXTERN_TYPE	void xlsWorkBookWindSize(workbook *w, uint16 horz, uint16 vert);
EXTERN_TYPE	void xlsWorkBookFirstTab(workbook *w, uint16 firstTab);
EXTERN_TYPE	void xlsWorkBookTabBarWidth(workbook *w, uint16 width);
EXTERN_TYPE	int xlsWorkbookDump(workbook *w, const char *filename);

// Worksheet
EXTERN_TYPE void xlsWorksheetMakeActive(worksheet *w);	// Make this sheet the selected sheet
EXTERN_TYPE cell_t *xlsWorksheetFindCell(worksheet *w, uint32 row, uint32 col);
EXTERN_TYPE void xlsWorksheetMerge(worksheet *w, uint32 first_row, uint32 first_col, uint32 last_row, uint32 last_col);
EXTERN_TYPE void xlsWorksheetColwidth(worksheet *w, uint32 col, uint16 width, xf_t* pxformat);
EXTERN_TYPE void xlsWorksheetRowheight(worksheet *w, uint32 row, uint16 height, xf_t* pxformat);
EXTERN_TYPE range *xlsWorksheetRangegroup(worksheet *w, uint32 row1, uint32 col1, uint32 row2, uint32 col2);
EXTERN_TYPE cell_t *xlsWorksheetLabel(worksheet *w, uint32 row, uint32 col, const char *strlabel, xf_t *pxformat);
EXTERN_TYPE cell_t *xlsWorksheetLabelW(worksheet *w, uint32 row, uint32 col, const unichar_t *strlabel, xf_t *pxformat);
EXTERN_TYPE cell_t *xlsWorksheetBlank(worksheet *w, uint32 row, uint32 col, xf_t *pxformat);
EXTERN_TYPE cell_t *xlsWorksheetNumberDbl(worksheet *w, uint32 row, uint32 col, double numval, xf_t *pxformat);
// 536870911 >= numval >= -536870912
EXTERN_TYPE cell_t *xlsWorksheetNumberInt(worksheet *w, uint32 row, uint32 col, int32 numval, xf_t *pxformat);
EXTERN_TYPE cell_t *xlsWorksheetBoolean(worksheet *w, uint32 row, uint32 col, int boolval, xf_t *pxformat);
EXTERN_TYPE cell_t *xlsWorksheetError(worksheet *w, uint32 row, uint32 col, errcode_t errval, xf_t *pxformat);
EXTERN_TYPE cell_t *xlsWorksheetNote(worksheet *w, uint32 row, uint32 col, const char *remark, const char *author, xf_t *pxformat);
EXTERN_TYPE cell_t *xlsWorksheetNoteW(worksheet *w, uint32 row, uint32 col, const unichar_t *remark, const unichar_t *author, xf_t *pxformat);

EXTERN_TYPE formula_t *xlsWorksheetFormula(worksheet *w);

EXTERN_TYPE void xlsFormulaPushBoolean(formula_t *formula, bool value);
EXTERN_TYPE void xlsFormulaPushMissingArgument(formula_t *formula);
EXTERN_TYPE void xlsFormulaPushError(formula_t *formula, uint8 value);
EXTERN_TYPE void xlsFormulaPushNumberInt(formula_t *formula, int32 value);
EXTERN_TYPE void xlsFormulaPushNumberDbl(formula_t *formula, double value);
EXTERN_TYPE void xlsFormulaPushNumberArray(formula_t *formula, double *values, size_t count);
EXTERN_TYPE void xlsFormulaPushOperator(formula_t *formula, expr_operator_code_t op);
EXTERN_TYPE void xlsFormulaPushCellReference(formula_t *formula, cell_t *cell, cell_addr_mode_t opt);
EXTERN_TYPE void xlsFormulaPushCellAreaReference(formula_t *formula, cell_t *upper_left_cell, 
            cell_t *lower_right_cell, cell_addr_mode_t opt);
EXTERN_TYPE void xlsFormulaPushFunction(formula_t *formula, expr_function_code_t func);
EXTERN_TYPE void xlsFormulaPushFunctionV(formula_t *formula, expr_function_code_t func, size_t arg_count);
EXTERN_TYPE void xlsFormulaPushText(formula_t *formula, const char *text);
EXTERN_TYPE void xlsFormulaPushTextW(formula_t *formula, const unichar_t *text);
EXTERN_TYPE void xlsFormulaPushCharacterArray(formula_t *formula, const char *text, size_t count);
EXTERN_TYPE void xlsFormulaPushCharacterArrayW(formula_t *formula, const unichar_t *text, size_t count);
EXTERN_TYPE void xlsFormulaPushTextArray(formula_t *formula, const char **text, size_t count);
EXTERN_TYPE void xlsFormulaPushTextArrayW(formula_t *formula, const unichar_t **text, size_t count);

EXTERN_TYPE cell_t *xlsWorksheetFormulaCell(worksheet *w, uint32 row, uint32 col, formula_t *stack, xf_t *pxformat);
EXTERN_TYPE void xlsWorksheetValidateCell(worksheet *w, cell_t *cell, uint32 options, 
        const formula_t *cond1, const formula_t *cond2, 
        const char *prompt_title, const char *prompt_text, 
        const char *error_title, const char *error_text);
EXTERN_TYPE void xlsWorksheetValidateCellW(worksheet *w, cell_t *cell, uint32 options, 
        const formula_t *cond1, const formula_t *cond2, 
        const unichar_t *prompt_title, const unichar_t *prompt_text, 
        const unichar_t *error_title, const unichar_t *error_text);
EXTERN_TYPE void xlsWorksheetValidateCellArea(worksheet *w, cell_t *upper_left_cell, cell_t *lower_right_cell, 
        uint32 options, const formula_t *cond1, const formula_t *cond2, 
        const char *prompt_title, const char *prompt_text, 
        const char *error_title, const char *error_text);
EXTERN_TYPE void xlsWorksheetValidateCellAreaW(worksheet *w, cell_t *upper_left_cell, cell_t *lower_right_cell, 
        uint32 options, const formula_t *cond1, const formula_t *cond2, 
        const unichar_t *prompt_title, const unichar_t *prompt_text, 
        const unichar_t *error_title, const unichar_t *error_text);

// define a cell (label, number, etc) - apply proper url (http://blah.blah), possible text mark too minus the '#' (mark can be NULL)
EXTERN_TYPE void xlsWorksheetHyperLink(worksheet *w, cell_t *cell, const char *url, const char *mark);
EXTERN_TYPE void xlsWorksheetHyperLinkW(worksheet *w, cell_t *cell, const unichar_t *url, const unichar_t *mark);

// cell: xfi
EXTERN_TYPE void xlsCellFont(cell_t *c, font_t *fontidx);
EXTERN_TYPE void xlsCellFormat(cell_t *c, format_number_t format);
EXTERN_TYPE void xlsCellFormatP(cell_t *c, format_t *format);
EXTERN_TYPE void xlsCellHalign(cell_t *c, halign_option_t ha_option);
EXTERN_TYPE void xlsCellValign(cell_t *c, valign_option_t va_option);
EXTERN_TYPE void xlsCellIndent(cell_t *c, indent_option_t in_option);
EXTERN_TYPE void xlsCellOrientation(cell_t *c, txtori_option_t ori_option);
EXTERN_TYPE void xlsCellFillfgcolor(cell_t *c, color_name_t color);
EXTERN_TYPE void xlsCellFillbgcolor(cell_t *c, color_name_t color);
EXTERN_TYPE void xlsCellFillstyle(cell_t *c, fill_option_t fill);
EXTERN_TYPE void xlsCellLocked(cell_t *c, bool locked_opt);
EXTERN_TYPE void xlsCellHidden(cell_t *c, bool hidden_opt);
EXTERN_TYPE void xlsCellWrap(cell_t *c, bool wrap_opt);
EXTERN_TYPE void xlsCellBorderstyle(cell_t *c, border_side_t side, border_style_t style);
EXTERN_TYPE void xlsCellBordercolor(cell_t *c, border_side_t side, color_name_t color);
EXTERN_TYPE void xlsCellBordercolorIdx(cell_t *c, border_side_t side, uint8 color);
EXTERN_TYPE void xlsCellFontname(cell_t *c, const char *fntname);
// cell: font
EXTERN_TYPE void xlsCellFontheight(cell_t *c, uint16 fntheight);
EXTERN_TYPE void xlsCellFontbold(cell_t *c, boldness_option_t fntboldness);
EXTERN_TYPE void xlsCellFontunderline(cell_t *c, underline_option_t fntunderline);
EXTERN_TYPE void xlsCellFontscript(cell_t *c, script_option_t fntscript);
EXTERN_TYPE void xlsCellFontcolor(cell_t *c, color_name_t fntcolor);
//EXTERN_TYPE void xlsCellFontattr(cell_t *c, uint16 attr);
EXTERN_TYPE void xlsCellFontitalic(cell_t *c, bool italic);
EXTERN_TYPE void xlsCellFontstrikeout(cell_t *c, bool so);
EXTERN_TYPE void xlsCellFontoutline(cell_t *c, bool ol);
EXTERN_TYPE void xlsCellFontshadow(cell_t *c, bool sh);
EXTERN_TYPE uint32 xlsCellGetRow(cell_t *c);
EXTERN_TYPE uint32 xlsCellGetCol(cell_t *c);
EXTERN_TYPE void xlsRangeCellcolor(range *r, color_name_t color);
EXTERN_TYPE uint16 xlsCellGetXFIndex(cell_t *c);
EXTERN_TYPE void xlsCellSetXF(cell_t *c, xf_t *pxfval);
// xformat
EXTERN_TYPE void xlsXformatSetFont(xf_t *x, font_t* fontidx);
EXTERN_TYPE uint16 xlsXformatGetFontIndex(xf_t *x);
EXTERN_TYPE font_t* xlsXformatGetFont(xf_t *x);
EXTERN_TYPE void xlsXformatSetFormat(xf_t *x, format_number_t formatidx);
EXTERN_TYPE void xlsXformatSetFormatP(xf_t *x, format_t *fmt);
EXTERN_TYPE uint16 xlsXformatGetFormatIndex(xf_t *x);
EXTERN_TYPE format_number_t xlsXformatGetFormat(xf_t *x);
EXTERN_TYPE void xlsXformatSetHAlign(xf_t *x, halign_option_t ha_option);
EXTERN_TYPE uint8 xlsXformatGetHAlign(xf_t *x);
EXTERN_TYPE void xlsXformatSetVAlign(xf_t *x, valign_option_t va_option);
EXTERN_TYPE uint8 xlsXformatGetVAlign(xf_t *x);
EXTERN_TYPE void xlsXformatSetIndent(xf_t *x, indent_option_t in_option);
EXTERN_TYPE uint8 xlsXformatGetIndent(xf_t *x);
EXTERN_TYPE void xlsXformatSetTxtOrientation(xf_t *x, txtori_option_t ori_option);
EXTERN_TYPE uint8 xlsXformatGetTxtOrientation(xf_t *x);
EXTERN_TYPE void xlsXformatSetFillFGColor(xf_t *x, color_name_t color);
EXTERN_TYPE uint16 xlsXformatGetFillFGColorIdx(xf_t *x);
EXTERN_TYPE void xlsXformatSetFillBGColor(xf_t *x, color_name_t color);
EXTERN_TYPE uint16 xlsXformatGetFillBGColorIdx(xf_t *x);
EXTERN_TYPE void xlsXformatSetFillStyle(xf_t *x, fill_option_t fill);
EXTERN_TYPE uint8 xlsXformatGetFillStyle(xf_t *x);
EXTERN_TYPE void xlsXformatSetLocked(xf_t *x, bool locked_opt);
EXTERN_TYPE bool xlsXformatIsLocked(xf_t *x);
EXTERN_TYPE void xlsXformatSetHidden(xf_t *x, bool hidden_opt);
EXTERN_TYPE bool xlsXformatIsHidden(xf_t *x);
EXTERN_TYPE void xlsXformatSetWrap(xf_t *x, bool wrap_opt);
EXTERN_TYPE bool xlsXformatIsWrap(xf_t *x);
EXTERN_TYPE void xlsXformatSetCellMode(xf_t *x, bool cellmode);
EXTERN_TYPE bool xlsXformatIsCell(xf_t *x);
EXTERN_TYPE void xlsXformatSetBorderStyle(xf_t *x, border_side_t side, border_style_t style);
EXTERN_TYPE void xlsXformatSetBorderColor(xf_t *x, border_side_t side, color_name_t color);
EXTERN_TYPE void xlsXformatSetBorderColorIdx(xf_t *x, border_side_t side, uint8 color);
EXTERN_TYPE uint8 xlsXformatGetBorderStyle(xf_t *x, border_side_t side);
EXTERN_TYPE uint16 xlsXformatGetBorderColorIdx(xf_t *x, border_side_t side);
//EXTERN_TYPE uint32 xlsXformatGetSignature(xf_t *x);
// Font
EXTERN_TYPE void xlsFontSetName(font_t *f, const char *name);
EXTERN_TYPE char *xlsFontGetName(font_t *f, char *namebuffer, size_t namebuffersize);
EXTERN_TYPE void xlsFontSetHeight(font_t *f, uint16 fntheight);
EXTERN_TYPE uint16 xlsFontGetHeight(font_t *f);
EXTERN_TYPE void xlsFontSetBoldStyle(font_t *f, boldness_option_t fntboldness);
EXTERN_TYPE uint16 xlsFontGetBoldStyle(font_t *f);
EXTERN_TYPE void xlsFontSetUnderlineStyle(font_t *f, underline_option_t fntunderline);
EXTERN_TYPE uint8 xlsFontGetUnderlineStyle(font_t *f);
EXTERN_TYPE void xlsFontSetScriptStyle(font_t *f, script_option_t fntscript);
EXTERN_TYPE uint16 xlsFontGetScriptStyle(font_t *f);
EXTERN_TYPE void xlsFontSetColor(font_t *f, color_name_t fntcolor);
EXTERN_TYPE uint16 xlsFontGetColorIdx(font_t *f);
EXTERN_TYPE void xlsFontSetItalic(font_t *f, bool italic);
EXTERN_TYPE void xlsFontSetStrikeout(font_t *f, bool so);
#if defined(DEPRECATED)
EXTERN_TYPE void xlsFontSetAttributes(font_t *f, uint16 attr);
#endif
EXTERN_TYPE uint16 xlsFontGetAttributes(font_t *f);
EXTERN_TYPE void xlsFontSetOutline(font_t *f, bool ol);
EXTERN_TYPE void xlsFontSetShadow(font_t *f, bool sh);

#ifdef CPP_BRIDGE_XLS
}
#endif

#endif // !defined(__cplusplus) || define(CPP_BRIDGE_XLS)
