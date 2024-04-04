/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * Copyright 2010-2013 Ger Hobbelt All Rights Reserved.
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
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "xlslib-internal.h"
#pragma hdrstop

#ifdef __BCPLUSPLUS__
#include <memory.h>
// string.h needed for memcpy(). RLN 111215
// This may be applicable to other compilers as well.
#endif

using namespace xlslib_core;
using namespace xlslib_strings;

uint16 xlslib_core::NumberOfArgsForExcelFunction(expr_function_code_t func)
{
	uint16 result = A_UNKNOWN;
	switch(func) {
		case FUNC_UDF: result = A_UNKNOWN; break;
		case FUNC_IF: result = A_2_OR_3; break;
		case FUNC_COUNT: result = A_1_OR_MORE; break;
		case FUNC_ISNA: result = A_1; break;
		case FUNC_ISERROR: result = A_1; break;
		case FUNC_SUM: result = A_1_OR_MORE; break;
		case FUNC_AVERAGE: result = A_1_OR_MORE; break;
		case FUNC_MIN: result = A_1_OR_MORE; break;
		case FUNC_MAX: result = A_1_OR_MORE; break;
		case FUNC_ROW: result = A_0_OR_1; break;
		case FUNC_COLUMN: result = A_0_OR_1; break;
		case FUNC_NA: result = A_0; break;
		case FUNC_NPV: result = A_2_OR_MORE; break;
		case FUNC_STDEV: result = A_1_OR_MORE; break;
		case FUNC_DOLLAR: result = A_1_OR_2; break;
		case FUNC_FIXED: result = A_1_TO_3; break;
		case FUNC_SIN: result = A_1; break;
		case FUNC_COS: result = A_1; break;
		case FUNC_TAN: result = A_1; break;
		case FUNC_ATAN: result = A_1; break;
		case FUNC_PI: result = A_0; break;
		case FUNC_SQRT: result = A_1; break;
		case FUNC_EXP: result = A_1; break;
		case FUNC_LN: result = A_1; break;
		case FUNC_LOG10: result = A_1; break;
		case FUNC_ABS: result = A_1; break;
		case FUNC_INT: result = A_1; break;
		case FUNC_SIGN: result = A_1; break;
		case FUNC_ROUND: result = A_2; break;
		case FUNC_LOOKUP: result = A_2_OR_3; break;
		case FUNC_INDEX: result = A_2_TO_4; break;
		case FUNC_REPT: result = A_2; break;
		case FUNC_MID: result = A_3; break;
		case FUNC_LEN: result = A_1; break;
		case FUNC_VALUE: result = A_1; break;
		case FUNC_TRUE: result = A_0; break;
		case FUNC_FALSE: result = A_0; break;
		case FUNC_AND: result = A_1_OR_MORE; break;
		case FUNC_OR: result = A_1_OR_MORE; break;
		case FUNC_NOT: result = A_1; break;
		case FUNC_MOD: result = A_2; break;
		case FUNC_DCOUNT: result = A_3; break;
		case FUNC_DSUM: result = A_3; break;
		case FUNC_DAVERAGE: result = A_3; break;
		case FUNC_DMIN: result = A_3; break;
		case FUNC_DMAX: result = A_3; break;
		case FUNC_DSTDEV: result = A_3; break;
		case FUNC_VAR: result = A_1_OR_MORE; break;
		case FUNC_DVAR: result = A_3; break;
		case FUNC_TEXT: result = A_2; break;
		case FUNC_LINEST: result = A_1_TO_4; break;
		case FUNC_TREND: result = A_1_TO_4; break;
		case FUNC_LOGEST: result = A_1_TO_4; break;
		case FUNC_GROWTH: result = A_1_TO_4; break;
		case FUNC_GOTO: result = A_1 | A_MACRO; break;
		case FUNC_HALT: result = A_0_OR_1 | A_MACRO; break;
		case FUNC_PV: result = A_3_TO_5; break;
		case FUNC_FV: result = A_3_TO_5; break;
		case FUNC_NPER: result = A_3_TO_5; break;
		case FUNC_PMT: result = A_3_TO_5; break;
		case FUNC_RATE: result = A_3_TO_6; break;
		case FUNC_MIRR: result = A_3; break;
		case FUNC_IRR: result = A_1_OR_2; break;
		case FUNC_RAND: result = A_0; break;
		case FUNC_MATCH: result = A_2_OR_3; break;
		case FUNC_DATE: result = A_3; break;
		case FUNC_TIME: result = A_3; break;
		case FUNC_DAY: result = A_1; break;
		case FUNC_MONTH: result = A_1; break;
		case FUNC_YEAR: result = A_1; break;
		case FUNC_WEEKDAY: result = A_1_OR_2; break;
		case FUNC_HOUR: result = A_1; break;
		case FUNC_MINUTE: result = A_1; break;
		case FUNC_SECOND: result = A_1; break;
		case FUNC_NOW: result = A_0; break;
		case FUNC_AREAS: result = A_1; break;
		case FUNC_ROWS: result = A_1; break;
		case FUNC_COLUMNS: result = A_1; break;
		case FUNC_OFFSET: result = A_3_TO_5; break;
		case FUNC_ABSREF: result = A_2 | A_MACRO; break;
		case FUNC_RELREF: result = A_2 | A_MACRO; break;
		case FUNC_ARGUMENT: result = A_0_TO_3 | A_MACRO; break;
		case FUNC_SEARCH: result = A_2_OR_3; break;
		case FUNC_TRANSPOSE: result = A_1; break;
		case FUNC_ERROR: result = A_0_TO_2 | A_MACRO; break;
		case FUNC_STEP: result = A_0 | A_MACRO; break;
		case FUNC_TYPE: result = A_1; break;
		case FUNC_ECHO: result = A_0_OR_1 | A_MACRO; break;
		case FUNC_SETNAME: result = A_1_OR_2 | A_MACRO; break;
		case FUNC_CALLER: result = A_0 | A_MACRO; break;
		case FUNC_DEREF: result = A_1 | A_MACRO; break;
		case FUNC_WINDOWS: result = A_0_TO_2 | A_MACRO; break;
		case FUNC_SERIES: result = A_4_OR_5; break;
		case FUNC_DOCUMENTS: result = A_0_TO_2 | A_MACRO; break;
		case FUNC_ACTIVECELL: result = A_0 | A_MACRO; break;
		case FUNC_SELECTION: result = A_0 | A_MACRO; break;
		case FUNC_RESULT: result = A_0_OR_1 | A_MACRO; break;
		case FUNC_ATAN2: result = A_2; break;
		case FUNC_ASIN: result = A_1; break;
		case FUNC_ACOS: result = A_1; break;
		case FUNC_CHOOSE: result = A_2_OR_MORE; break;
		case FUNC_HLOOKUP: result = A_3_OR_4; break;
		case FUNC_VLOOKUP: result = A_3_OR_4; break;
		case FUNC_LINKS: result = A_0_TO_2 | A_MACRO; break;
		case FUNC_INPUT: result = A_1_TO_7 | A_MACRO; break;
		case FUNC_ISREF: result = A_1; break;
		case FUNC_GETFORMULA: result = A_1 | A_MACRO; break;
		case FUNC_GETNAME: result = A_1_OR_2 | A_MACRO; break;
		case FUNC_SETVALUE: result = A_2 | A_MACRO; break;
		case FUNC_LOG: result = A_1_OR_2; break;
		case FUNC_EXEC: result = A_1_TO_4 | A_MACRO; break;
		case FUNC_CHAR: result = A_1; break;
		case FUNC_LOWER: result = A_1; break;
		case FUNC_UPPER: result = A_1; break;
		case FUNC_PROPER: result = A_1; break;
		case FUNC_LEFT: result = A_1_OR_2; break;
		case FUNC_RIGHT: result = A_1_OR_2; break;
		case FUNC_EXACT: result = A_2; break;
		case FUNC_TRIM: result = A_1; break;
		case FUNC_REPLACE: result = A_4; break;
		case FUNC_SUBSTITUTE: result = A_3_OR_4; break;
		case FUNC_CODE: result = A_1; break;
		case FUNC_NAMES: result = A_0_TO_3 | A_MACRO; break;
		case FUNC_DIRECTORY: result = A_0_OR_1 | A_MACRO; break;
		case FUNC_FIND: result = A_2_OR_3; break;
		case FUNC_CELL: result = A_1_OR_2; break;
		case FUNC_ISERR: result = A_1; break;
		case FUNC_ISTEXT: result = A_1; break;
		case FUNC_ISNUMBER: result = A_1; break;
		case FUNC_ISBLANK: result = A_1; break;
		case FUNC_T: result = A_1; break;
		case FUNC_N: result = A_1; break;
		case FUNC_FOPEN: result = A_1_OR_2 | A_MACRO; break;
		case FUNC_FCLOSE: result = A_1 | A_MACRO; break;
		case FUNC_FSIZE: result = A_1 | A_MACRO; break;
		case FUNC_FREADLN: result = A_1 | A_MACRO; break;
		case FUNC_FREAD: result = A_2 | A_MACRO; break;
		case FUNC_FWRITELN: result = A_2 | A_MACRO; break;
		case FUNC_FWRITE: result = A_2 | A_MACRO; break;
		case FUNC_FPOS: result = A_1_OR_2 | A_MACRO; break;
		case FUNC_DATEVALUE: result = A_1; break;
		case FUNC_TIMEVALUE: result = A_1; break;
		case FUNC_SLN: result = A_3; break;
		case FUNC_SYD: result = A_4; break;
		case FUNC_DDB: result = A_4_OR_5; break;
		case FUNC_GETDEF: result = A_1_TO_3 | A_MACRO; break;
		case FUNC_REFTEXT: result = A_1_OR_2 | A_MACRO; break;
		case FUNC_TEXTREF: result = A_1_OR_2 | A_MACRO; break;
		case FUNC_INDIRECT: result = A_1_OR_2; break;
		case FUNC_REGISTER: result = A_1_OR_MORE | A_MACRO; break;
		case FUNC_CALL: result = A_1_OR_MORE | A_MACRO; break;
		case FUNC_ADDBAR: result = A_0_OR_1 | A_MACRO; break;
		case FUNC_ADDMENU: result = A_2_TO_4 | A_MACRO; break;
		case FUNC_ADDCOMMAND: result = A_3_TO_5 | A_MACRO; break;
		case FUNC_ENABLECOMMAND: result = A_4_OR_5 | A_MACRO; break;
		case FUNC_CHECKCOMMAND: result = A_4_OR_5 | A_MACRO; break;
		case FUNC_RENAMECOMMAND: result = A_4_OR_5 | A_MACRO; break;
		case FUNC_SHOWBAR: result = A_0_OR_1 | A_MACRO; break;
		case FUNC_DELETEMENU: result = A_2_OR_3 | A_MACRO; break;
		case FUNC_DELETECOMMAND: result = A_3_OR_4 | A_MACRO; break;
		case FUNC_GETCHARTITEM: result = A_1_TO_3 | A_MACRO; break;
		case FUNC_DIALOGBOX: result = A_1 | A_MACRO; break;
		case FUNC_CLEAN: result = A_1; break;
		case FUNC_MDETERM: result = A_1; break;
		case FUNC_MINVERSE: result = A_1; break;
		case FUNC_MMULT: result = A_2; break;
		case FUNC_FILES: result = A_0_TO_2 | A_MACRO; break;
		case FUNC_IPMT: result = A_4_TO_6; break;
		case FUNC_PPMT: result = A_4_TO_6; break;
		case FUNC_COUNTA: result = A_1_OR_MORE; break;
		case FUNC_CANCELKEY: result = A_0_TO_2 | A_MACRO; break;
		case FUNC_INITIATE: result = A_2 | A_MACRO; break;
		case FUNC_REQUEST: result = A_2 | A_MACRO; break;
		case FUNC_POKE: result = A_3 | A_MACRO; break;
		case FUNC_EXECUTE: result = A_2 | A_MACRO; break;
		case FUNC_TERMINATE: result = A_1 | A_MACRO; break;
		case FUNC_RESTART: result = A_0_OR_1 | A_MACRO; break;
		case FUNC_HELP: result = A_0_OR_1 | A_MACRO; break;
		case FUNC_GETBAR: result = A_0_TO_4 | A_MACRO; break;
		case FUNC_PRODUCT: result = A_1_OR_MORE; break;
		case FUNC_FACT: result = A_1; break;
		case FUNC_GETCELL: result = A_1_OR_2 | A_MACRO; break;
		case FUNC_GETWORKSPACE: result = A_1 | A_MACRO; break;
		case FUNC_GETWINDOW: result = A_1_OR_2 | A_MACRO; break;
		case FUNC_GETDOCUMENT: result = A_1_OR_2 | A_MACRO; break;
		case FUNC_DPRODUCT: result = A_3; break;
		case FUNC_ISNONTEXT: result = A_1; break;
		case FUNC_GETNOTE: result = A_0_TO_3 | A_MACRO; break;
		case FUNC_NOTE: result = A_0_TO_4 | A_MACRO; break;
		case FUNC_STDEVP: result = A_1_OR_MORE; break;
		case FUNC_VARP: result = A_1_OR_MORE; break;
		case FUNC_DSTDEVP: result = A_3; break;
		case FUNC_DVARP: result = A_3; break;
		case FUNC_TRUNC: result = A_1_OR_2; break;
		case FUNC_ISLOGICAL: result = A_1; break;
		case FUNC_DCOUNTA: result = A_3; break;
		case FUNC_DELETEBAR: result = A_1 | A_MACRO; break;
		case FUNC_UNREGISTER: result = A_1 | A_MACRO; break;
		case FUNC_USDOLLAR: result = A_1_OR_2; break;
		case FUNC_FINDB: result = A_2_OR_3; break;
		case FUNC_SEARCHB: result = A_2_OR_3; break;
		case FUNC_REPLACEB: result = A_4; break;
		case FUNC_LEFTB: result = A_1_OR_2; break;
		case FUNC_RIGHTB: result = A_1_OR_2; break;
		case FUNC_MIDB: result = A_3; break;
		case FUNC_LENB: result = A_1; break;
		case FUNC_ROUNDUP: result = A_2; break;
		case FUNC_ROUNDDOWN: result = A_2; break;
		case FUNC_ASC: result = A_1; break;
		case FUNC_DBCS: result = A_1; break;
		case FUNC_RANK: result = A_2_OR_3; break;
		case FUNC_ADDRESS: result = A_2_TO_5; break;
		case FUNC_DAYS360: result = A_2_OR_3; break;
		case FUNC_TODAY: result = A_0; break;
		case FUNC_VDB: result = A_5_TO_7; break;
		case FUNC_MEDIAN: result = A_1_OR_MORE; break;
		case FUNC_SUMPRODUCT: result = A_1_OR_MORE; break;
		case FUNC_SINH: result = A_1; break;
		case FUNC_COSH: result = A_1; break;
		case FUNC_TANH: result = A_1; break;
		case FUNC_ASINH: result = A_1; break;
		case FUNC_ACOSH: result = A_1; break;
		case FUNC_ATANH: result = A_1; break;
		case FUNC_DGET: result = A_3; break;
		case FUNC_CREATEOBJECT: result = A_2_OR_MORE | A_MACRO; break;
		case FUNC_VOLATILE: result = A_0_OR_1 | A_MACRO; break;
		case FUNC_LASTERROR: result = A_0 | A_MACRO; break;
		case FUNC_CUSTOMUNDO: result = A_0_TO_2 | A_MACRO; break;
		case FUNC_CUSTOMREPEAT: result = A_0_TO_3 | A_MACRO; break;
		case FUNC_FORMULACONVERT: result = A_2_TO_5 | A_MACRO; break;
		case FUNC_GETLINKINFO: result = A_2_TO_4 | A_MACRO; break;
		case FUNC_TEXTBOX: result = A_1_TO_4 | A_MACRO; break;
		case FUNC_INFO: result = A_1; break;
		case FUNC_GROUP: result = A_0 | A_MACRO; break;
		case FUNC_GETOBJECT: result = A_1_TO_5 | A_MACRO; break;
		case FUNC_DB: result = A_4_OR_5; break;
		case FUNC_PAUSE: result = A_0_OR_1 | A_MACRO; break;
		case FUNC_RESUME: result = A_0_OR_1 | A_MACRO; break;
		case FUNC_FREQUENCY: result = A_2; break;
		case FUNC_ADDTOOLBAR: result = A_0_TO_2 | A_MACRO; break;
		case FUNC_DELETETOOLBAR: result = A_1 | A_MACRO; break;
		case FUNC_RESETTOOLBAR: result = A_1 | A_MACRO; break;
		case FUNC_EVALUATE: result = A_1 | A_MACRO; break;
		case FUNC_GETTOOLBAR: result = A_1_OR_2 | A_MACRO; break;
		case FUNC_GETTOOL: result = A_1_TO_3 | A_MACRO; break;
		case FUNC_SPELLINGCHECK: result = A_1_TO_3 | A_MACRO; break;
		case FUNC_ERRORTYPE: result = A_1; break;
		case FUNC_APPTITLE: result = A_0_OR_1 | A_MACRO; break;
		case FUNC_WINDOWTITLE: result = A_0_OR_1 | A_MACRO; break;
		case FUNC_SAVETOOLBAR: result = A_0_TO_2 | A_MACRO; break;
		case FUNC_ENABLETOOL: result = A_3 | A_MACRO; break;
		case FUNC_PRESSTOOL: result = A_3 | A_MACRO; break;
		case FUNC_REGISTERID: result = A_2_OR_3 | A_MACRO; break;
		case FUNC_GETWORKBOOK: result = A_1_OR_2 | A_MACRO; break;
		case FUNC_AVEDEV: result = A_1_OR_MORE; break;
		case FUNC_BETADIST: result = A_3_TO_5; break;
		case FUNC_GAMMALN: result = A_1; break;
		case FUNC_BETAINV: result = A_3_TO_5; break;
		case FUNC_BINOMDIST: result = A_4; break;
		case FUNC_CHIDIST: result = A_2; break;
		case FUNC_CHIINV: result = A_2; break;
		case FUNC_COMBIN: result = A_2; break;
		case FUNC_CONFIDENCE: result = A_3; break;
		case FUNC_CRITBINOM: result = A_3; break;
		case FUNC_EVEN: result = A_1; break;
		case FUNC_EXPONDIST: result = A_3; break;
		case FUNC_FDIST: result = A_3; break;
		case FUNC_FINV: result = A_3; break;
		case FUNC_FISHER: result = A_1; break;
		case FUNC_FISHERINV: result = A_1; break;
		case FUNC_FLOOR: result = A_2; break;
		case FUNC_GAMMADIST: result = A_4; break;
		case FUNC_GAMMAINV: result = A_3; break;
		case FUNC_CEILING: result = A_2; break;
		case FUNC_HYPGEOMDIST: result = A_4; break;
		case FUNC_LOGNORMDIST: result = A_3; break;
		case FUNC_LOGINV: result = A_3; break;
		case FUNC_NEGBINOMDIST: result = A_3; break;
		case FUNC_NORMDIST: result = A_4; break;
		case FUNC_NORMSDIST: result = A_1; break;
		case FUNC_NORMINV: result = A_3; break;
		case FUNC_NORMSINV: result = A_1; break;
		case FUNC_STANDARDIZE: result = A_3; break;
		case FUNC_ODD: result = A_1; break;
		case FUNC_PERMUT: result = A_2; break;
		case FUNC_POISSON: result = A_3; break;
		case FUNC_TDIST: result = A_3; break;
		case FUNC_WEIBULL: result = A_4; break;
		case FUNC_SUMXMY2: result = A_2; break;
		case FUNC_SUMX2MY2: result = A_2; break;
		case FUNC_SUMX2PY2: result = A_2; break;
		case FUNC_CHITEST: result = A_2; break;
		case FUNC_CORREL: result = A_2; break;
		case FUNC_COVAR: result = A_2; break;
		case FUNC_FORECAST: result = A_3; break;
		case FUNC_FTEST: result = A_2; break;
		case FUNC_INTERCEPT: result = A_2; break;
		case FUNC_PEARSON: result = A_2; break;
		case FUNC_RSQ: result = A_2; break;
		case FUNC_STEYX: result = A_2; break;
		case FUNC_SLOPE: result = A_2; break;
		case FUNC_TTEST: result = A_4; break;
		case FUNC_PROB: result = A_3_OR_4; break;
		case FUNC_DEVSQ: result = A_1_OR_MORE; break;
		case FUNC_GEOMEAN: result = A_1_OR_MORE; break;
		case FUNC_HARMEAN: result = A_1_OR_MORE; break;
		case FUNC_SUMSQ: result = A_1_OR_MORE; break;
		case FUNC_KURT: result = A_1_OR_MORE; break;
		case FUNC_SKEW: result = A_1_OR_MORE; break;
		case FUNC_ZTEST: result = A_2_OR_3; break;
		case FUNC_LARGE: result = A_2; break;
		case FUNC_SMALL: result = A_2; break;
		case FUNC_QUARTILE: result = A_2; break;
		case FUNC_PERCENTILE: result = A_2; break;
		case FUNC_PERCENTRANK: result = A_2_OR_3; break;
		case FUNC_MODE: result = A_1_OR_MORE; break;
		case FUNC_TRIMMEAN: result = A_2; break;
		case FUNC_TINV: result = A_2; break;
		case FUNC_MOVIECOMMAND: result = A_3_OR_4 | A_MACRO; break;
		case FUNC_GETMOVIE: result = A_3_OR_4 | A_MACRO; break;
		case FUNC_CONCATENATE: result = A_1_OR_MORE; break;
		case FUNC_POWER: result = A_2; break;
		case FUNC_PIVOTADDDATA: result = A_2_TO_9 | A_MACRO; break;
		case FUNC_GETPIVOTTABLE: result = A_1_OR_2 | A_MACRO; break;
		case FUNC_GETPIVOTFIELD: result = A_1_TO_3 | A_MACRO; break;
		case FUNC_GETPIVOTITEM: result = A_1_TO_4 | A_MACRO; break;
		case FUNC_RADIANS: result = A_1; break;
		case FUNC_DEGREES: result = A_1; break;
		case FUNC_SUBTOTAL: result = A_2_OR_MORE; break;
		case FUNC_SUMIF: result = A_2_OR_3; break;
		case FUNC_COUNTIF: result = A_2; break;
		case FUNC_COUNTBLANK: result = A_1; break;
		case FUNC_SCENARIOGET: result = A_1_OR_2 | A_MACRO; break;
		case FUNC_OPTIONSLISTSGET: result = A_1 | A_MACRO; break;
		case FUNC_ISPMT: result = A_4; break;
		case FUNC_DATEDIF: result = A_3; break;
		case FUNC_DATESTRING: result = A_1; break;
		case FUNC_NUMBERSTRING: result = A_2; break;
		case FUNC_ROMAN: result = A_1_OR_2; break;
		case FUNC_OPENDIALOG: result = A_0_TO_4 | A_MACRO; break;
		case FUNC_SAVEDIALOG: result = A_0_TO_5 | A_MACRO; break;
		case FUNC_VIEWGET: result = A_1_OR_2 | A_MACRO; break;
		case FUNC_GETPIVOTDATA: result = A_2_OR_MORE; break;
		case FUNC_HYPERLINK: result = A_1_OR_2; break;
		case FUNC_PHONETIC: result = A_1; break;
		case FUNC_AVERAGEA: result = A_1_OR_MORE; break;
		case FUNC_MAXA: result = A_1_OR_MORE; break;
		case FUNC_MINA: result = A_1_OR_MORE; break;
		case FUNC_STDEVPA: result = A_1_OR_MORE; break;
		case FUNC_VARPA: result = A_1_OR_MORE; break;
		case FUNC_STDEVA: result = A_1_OR_MORE; break;
		case FUNC_VARA: result = A_1_OR_MORE; break;
		case FUNC_BAHTTEXT: result = A_1; break;
		case FUNC_THAIDAYOFWEEK: result = A_1; break;
		case FUNC_THAIDIGIT: result = A_1; break;
		case FUNC_THAIMONTHOFYEAR: result = A_1; break;
		case FUNC_THAINUMSOUND: result = A_1; break;
		case FUNC_THAINUMSTRING: result = A_1; break;
		case FUNC_THAISTRINGLENGTH: result = A_1; break;
		case FUNC_ISTHAIDIGIT: result = A_1; break;
		case FUNC_ROUNDBAHTDOWN: result = A_1; break;
		case FUNC_ROUNDBAHTUP: result = A_1; break;
		case FUNC_THAIYEAR: result = A_1; break;
		case FUNC_RTD: result = A_3_OR_MORE; break;
		case FUNC_CUBEVALUE: result = A_1_OR_MORE | A_MACRO; break;
		case FUNC_CUBEMEMBER: result = A_2_OR_3 | A_MACRO; break;
		case FUNC_CUBEMEMBERPROPERTY: result = A_3 | A_MACRO; break;
		case FUNC_CUBERANKEDMEMBER: result = A_3_OR_4 | A_MACRO; break;
		case FUNC_HEX2BIN: result = A_1_OR_2; break;
		case FUNC_HEX2DEC: result = A_1; break;
		case FUNC_HEX2OCT: result = A_1_OR_2; break;
		case FUNC_DEC2BIN: result = A_1_OR_2; break;
		case FUNC_DEC2HEX: result = A_1_OR_2; break;
		case FUNC_DEC2OCT: result = A_1_OR_2; break;
		case FUNC_OCT2BIN: result = A_1_OR_2; break;
		case FUNC_OCT2HEX: result = A_1_OR_2; break;
		case FUNC_OCT2DEC: result = A_1; break;
		case FUNC_BIN2DEC: result = A_1; break;
		case FUNC_BIN2OCT: result = A_1_OR_2; break;
		case FUNC_BIN2HEX: result = A_1_OR_2; break;
		case FUNC_IMSUB: result = A_2; break;
		case FUNC_IMDIV: result = A_2; break;
		case FUNC_IMPOWER: result = A_2; break;
		case FUNC_IMABS: result = A_1; break;
		case FUNC_IMSQRT: result = A_1; break;
		case FUNC_IMLN: result = A_1; break;
		case FUNC_IMLOG2: result = A_1; break;
		case FUNC_IMLOG10: result = A_1; break;
		case FUNC_IMSIN: result = A_1; break;
		case FUNC_IMCOS: result = A_1; break;
		case FUNC_IMEXP: result = A_1; break;
		case FUNC_IMARGUMENT: result = A_1; break;
		case FUNC_IMCONJUGATE: result = A_1; break;
		case FUNC_IMAGINARY: result = A_1; break;
		case FUNC_IMREAL: result = A_1; break;
		case FUNC_COMPLEX: result = A_2_OR_3; break;
		case FUNC_IMSUM: result = A_1_OR_MORE; break;
		case FUNC_IMPRODUCT: result = A_1_OR_MORE; break;
		case FUNC_SERIESSUM: result = A_4; break;
		case FUNC_FACTDOUBLE: result = A_1; break;
		case FUNC_SQRTPI: result = A_1; break;
		case FUNC_QUOTIENT: result = A_2; break;
		case FUNC_DELTA: result = A_1_OR_2; break;
		case FUNC_GESTEP: result = A_1_OR_2; break;
		case FUNC_ISEVEN: result = A_1; break;
		case FUNC_ISODD: result = A_1; break;
		case FUNC_MROUND: result = A_2; break;
		case FUNC_ERF: result = A_1_OR_2; break;
		case FUNC_ERFC: result = A_1; break;
		case FUNC_BESSELJ: result = A_2; break;
		case FUNC_BESSELK: result = A_2; break;
		case FUNC_BESSELY: result = A_2; break;
		case FUNC_BESSELI: result = A_2; break;
		case FUNC_XIRR: result = A_2_OR_3; break;
		case FUNC_XNPV: result = A_3; break;
		case FUNC_PRICEMAT: result = A_5_OR_6; break;
		case FUNC_YIELDMAT: result = A_5_OR_6; break;
		case FUNC_INTRATE: result = A_4_OR_5; break;
		case FUNC_RECEIVED: result = A_4_OR_5; break;
		case FUNC_DISC: result = A_4_OR_5; break;
		case FUNC_PRICEDISC: result = A_4_OR_5; break;
		case FUNC_YIELDDISC: result = A_4_OR_5; break;
		case FUNC_TBILLEQ: result = A_3; break;
		case FUNC_TBILLPRICE: result = A_3; break;
		case FUNC_TBILLYIELD: result = A_3; break;
		case FUNC_PRICE: result = A_6_OR_7; break;
		case FUNC_YIELD: result = A_6_OR_7; break;
		case FUNC_DOLLARDE: result = A_2; break;
		case FUNC_DOLLARFR: result = A_2; break;
		case FUNC_NOMINAL: result = A_2; break;
		case FUNC_EFFECT: result = A_2; break;
		case FUNC_CUMPRINC: result = A_6; break;
		case FUNC_CUMIPMT: result = A_6; break;
		case FUNC_EDATE: result = A_2; break;
		case FUNC_EOMONTH: result = A_2; break;
		case FUNC_YEARFRAC: result = A_2_OR_3; break;
		case FUNC_COUPDAYBS: result = A_3_OR_4; break;
		case FUNC_COUPDAYS: result = A_3_OR_4; break;
		case FUNC_COUPDAYSNC: result = A_3_OR_4; break;
		case FUNC_COUPNCD: result = A_3_OR_4; break;
		case FUNC_COUPNUM: result = A_3_OR_4; break;
		case FUNC_COUPPCD: result = A_3_OR_4; break;
		case FUNC_DURATION: result = A_5_OR_6; break;
		case FUNC_MDURATION: result = A_5_OR_6; break;
		case FUNC_ODDLPRICE: result = A_7_OR_8; break;
		case FUNC_ODDLYIELD: result = A_7_OR_8; break;
		case FUNC_ODDFPRICE: result = A_8_OR_9; break;
		case FUNC_ODDFYIELD: result = A_8_OR_9; break;
		case FUNC_RANDBETWEEN: result = A_2; break;
		case FUNC_WEEKNUM: result = A_1_OR_2; break;
		case FUNC_AMORDEGRC: result = A_6_OR_7; break;
		case FUNC_AMORLINC: result = A_6_OR_7; break;
		case FUNC_CONVERT: result = A_3; break;
		case FUNC_ACCRINT: result = A_6_TO_8; break;
		case FUNC_ACCRINTM: result = A_4_OR_5; break;
		case FUNC_WORKDAY: result = A_2_OR_3; break;
		case FUNC_NETWORKDAYS: result = A_2_OR_3; break;
		case FUNC_GCD: result = A_1_OR_MORE; break;
		case FUNC_MULTINOMIAL: result = A_1_OR_MORE; break;
		case FUNC_LCM: result = A_1_OR_MORE; break;
		case FUNC_FVSCHEDULE: result = A_2; break;
		case FUNC_CUBEKPIMEMBER: result = A_3_OR_4 | A_MACRO; break;
		case FUNC_CUBESET: result = A_2_TO_5 | A_MACRO; break;
		case FUNC_CUBESETCOUNT: result = A_1; break;
		case FUNC_IFERROR: result = A_2; break;
		case FUNC_COUNTIFS: result = A_2_OR_MORE; break;
		case FUNC_SUMIFS: result = A_3_OR_MORE; break;
		case FUNC_AVERAGEIF: result = A_2_OR_3; break;
		case FUNC_AVERAGEIFS: result = A_3_OR_MORE; break;
		case FUNC_AGGREGATE: result = A_UNKNOWN; break;
		case FUNC_BINOM_DIST: result = A_UNKNOWN; break;
		case FUNC_BINOM_INV: result = A_UNKNOWN; break;
		case FUNC_CONFIDENCE_NORM: result = A_UNKNOWN; break;
		case FUNC_CONFIDENCE_T: result = A_UNKNOWN; break;
		case FUNC_CHISQ_TEST: result = A_UNKNOWN; break;
		case FUNC_F_TEST: result = A_UNKNOWN; break;
		case FUNC_COVARIANCE_P: result = A_UNKNOWN; break;
		case FUNC_COVARIANCE_S: result = A_UNKNOWN; break;
		case FUNC_EXPON_DIST: result = A_UNKNOWN; break;
		case FUNC_GAMMA_DIST: result = A_UNKNOWN; break;
		case FUNC_GAMMA_INV: result = A_UNKNOWN; break;
		case FUNC_MODE_MULT: result = A_UNKNOWN; break;
		case FUNC_MODE_SNGL: result = A_UNKNOWN; break;
		case FUNC_NORM_DIST: result = A_UNKNOWN; break;
		case FUNC_NORM_INV: result = A_UNKNOWN; break;
		case FUNC_PERCENTILE_EXC: result = A_UNKNOWN; break;
		case FUNC_PERCENTILE_INC: result = A_UNKNOWN; break;
		case FUNC_PERCENTRANK_EXC: result = A_UNKNOWN; break;
		case FUNC_PERCENTRANK_INC: result = A_UNKNOWN; break;
		case FUNC_POISSON_DIST: result = A_UNKNOWN; break;
		case FUNC_QUARTILE_EXC: result = A_UNKNOWN; break;
		case FUNC_QUARTILE_INC: result = A_UNKNOWN; break;
		case FUNC_RANK_AVG: result = A_UNKNOWN; break;
		case FUNC_RANK_EQ: result = A_UNKNOWN; break;
		case FUNC_STDEV_S: result = A_UNKNOWN; break;
		case FUNC_STDEV_P: result = A_UNKNOWN; break;
		case FUNC_T_DIST: result = A_UNKNOWN; break;
		case FUNC_T_DIST_2T: result = A_UNKNOWN; break;
		case FUNC_T_DIST_RT: result = A_UNKNOWN; break;
		case FUNC_T_INV: result = A_UNKNOWN; break;
		case FUNC_T_INV_2T: result = A_UNKNOWN; break;
		case FUNC_VAR_S: result = A_UNKNOWN; break;
		case FUNC_VAR_P: result = A_UNKNOWN; break;
		case FUNC_WEIBULL_DIST: result = A_UNKNOWN; break;
		case FUNC_NETWORKDAYS_INTL: result = A_UNKNOWN; break;
		case FUNC_WORKDAY_INTL: result = A_UNKNOWN; break;
		case FUNC_ECMA_CEILING: result = A_UNKNOWN; break;
		case FUNC_ISO_CEILING: result = A_UNKNOWN; break;
		case FUNC_BETA_DIST: result = A_UNKNOWN; break;
		case FUNC_BETA_INV: result = A_UNKNOWN; break;
		case FUNC_CHISQ_DIST: result = A_UNKNOWN; break;
		case FUNC_CHISQ_DIST_RT: result = A_UNKNOWN; break;
		case FUNC_CHISQ_INV: result = A_UNKNOWN; break;
		case FUNC_CHISQ_INV_RT: result = A_UNKNOWN; break;
		case FUNC_F_DIST: result = A_UNKNOWN; break;
		case FUNC_F_DIST_RT: result = A_UNKNOWN; break;
		case FUNC_F_INV: result = A_UNKNOWN; break;
		case FUNC_F_INV_RT: result = A_UNKNOWN; break;
		case FUNC_HYPGEOM_DIST: result = A_UNKNOWN; break;
		case FUNC_LOGNORM_DIST: result = A_UNKNOWN; break;
		case FUNC_LOGNORM_INV: result = A_UNKNOWN; break;
		case FUNC_NEGBINOM_DIST: result = A_UNKNOWN; break;
		case FUNC_NORM_S_DIST: result = A_UNKNOWN; break;
		case FUNC_NORM_S_INV: result = A_UNKNOWN; break;
		case FUNC_T_TEST: result = A_UNKNOWN; break;
		case FUNC_Z_TEST: result = A_UNKNOWN; break;
		case FUNC_ERF_PRECISE: result = A_UNKNOWN; break;
		case FUNC_ERFC_PRECISE: result = A_UNKNOWN; break;
		case FUNC_GAMMALN_PRECISE: result = A_UNKNOWN; break;
		case FUNC_CEILING_PRECISE: result = A_UNKNOWN; break;
		case FUNC_FLOOR_PRECISE: result = A_UNKNOWN; break;
		default: result = 0xFFFFU; break;
	}
	return result;
}

formula_t::formula_t(CGlobalRecords& glbl, worksheet * ws) :
	m_GlobalRecords(glbl),
	m_Worksheet(ws)
{
	data_storage = new CDataStorage();

	main_data = data_storage->MakeCUnit();
	aux_data = data_storage->MakeCUnit();

	main_data->Inflate(10);
	aux_data->Inflate(10);
}

formula_t::~formula_t()
{
	delete data_storage;
}

int8 formula_t::PushBoolean(bool value)
{
	int8 errcode = NO_ERRORS;

	errcode |= main_data->AddValue8(OP_BOOL);
	errcode |= main_data->AddValue8((uint8) !!value);

	return errcode;
}

int8 formula_t::PushMissingArgument()
{
	int8 errcode = NO_ERRORS;

	errcode |= main_data->AddValue8(OP_MISSARG);

	return errcode;
}

int8 formula_t::PushError(uint8 value)
{
	int8 errcode = NO_ERRORS;

	errcode |= main_data->AddValue8(OP_ERR);
	errcode |= main_data->AddValue8(value);

	return errcode;
}

int8 formula_t::PushInteger(int32 value)
{
	int8 errcode = NO_ERRORS;

	if(value >= 0 && value <= 65535) {
		errcode |= main_data->AddValue8(OP_INT);
		errcode |= main_data->AddValue16((uint16)value);
	}
	else {
		errcode |= main_data->AddValue8(OP_NUM);
		errcode |= main_data->AddValue64FP(value);
	}

	return errcode;
}

int8 formula_t::PushFloatingPoint(double value)
{
	int8 errcode = NO_ERRORS;

	errcode |= main_data->AddValue8(OP_NUM);
	errcode |= main_data->AddValue64FP(value);

	return errcode;
}

int8 formula_t::PushOperator(expr_operator_code_t op)
{
	int8 errcode = NO_ERRORS;

	errcode |= main_data->AddValue8(op);

	return errcode;
}

int8 formula_t::PushCellReference(const cell_t& cell, cell_addr_mode_t opt)
{
	int8 ret;
	uint32 idx;

	idx = cell.GetWorksheet() ? cell.GetWorksheet()->GetIndex() : invalidIndex;
	ret = PushReference(cell.GetRow(), cell.GetCol(), idx, opt);
	return ret;
}

int8 formula_t::PushReference(uint32 row, uint32 col, uint32 idx, cell_addr_mode_t opt)
{
	int8 errcode = NO_ERRORS;

	if(m_Worksheet == NULL || idx == invalidIndex || idx == m_Worksheet->GetIndex()) {
		errcode |= main_data->AddValue8(OP_REFV);
		col &= 0x3FFF;
	}
	else {
		errcode |= main_data->AddValue8(OP_REF3DV);
		errcode |= main_data->AddValue16(static_cast<uint16>(idx));
		col &= 0x00FF;
	}
	errcode |= main_data->AddValue16(static_cast<uint16>(row));

	XL_ASSERT((opt & ~0xC000) == 0);
	col |= opt & 0xC000;
	errcode |= main_data->AddValue16(static_cast<uint16>(col));

	return errcode;
}

int8 formula_t::PushCellAreaReference(const cell_t& upper_left_cell, const cell_t& lower_right_cell, cell_addr_mode_t opt)
{
	int8 ret;
	uint32 ul_idx, lr_idx;

	ul_idx = upper_left_cell.GetWorksheet() ? upper_left_cell.GetWorksheet()->GetIndex() : invalidIndex;
	lr_idx = lower_right_cell.GetWorksheet() ? lower_right_cell.GetWorksheet()->GetIndex() : invalidIndex;

	ret = PushAreaReference(upper_left_cell.GetRow(), upper_left_cell.GetCol(), ul_idx, lower_right_cell.GetRow(), lower_right_cell.GetCol(), lr_idx, opt);
	return ret;
}

int8 formula_t::PushAreaReference(uint32 ul_row, uint32 ul_col, uint32 ul_idx, uint32 lr_row, uint32 lr_col, uint32 lr_idx, cell_addr_mode_t opt)
{
	int8 errcode = NO_ERRORS;
	(void)lr_idx;   // prevent warning

	if(m_Worksheet == NULL || ul_idx == invalidIndex || ul_idx == m_Worksheet->GetIndex()) {
		errcode |= main_data->AddValue8(OP_AREAA); // OP_AREA. OP_AREAV, OP_AREAA
		ul_col &= 0x3FFF;
		lr_col &= 0x3FFF;
	}
	else {
		errcode |= main_data->AddValue8(OP_AREA3DA); // OP_AREA. OP_AREAV, OP_AREAA
		errcode |= main_data->AddValue16(static_cast<uint16>(ul_idx));
		ul_col &= 0x00FF;
		lr_col &= 0x00FF;
	}

	// BIFF8 format!
	errcode |= main_data->AddValue16(static_cast<uint16>(ul_row));
	errcode |= main_data->AddValue16(static_cast<uint16>(lr_row));

	XL_ASSERT((opt & ~0xC000) == 0);
	ul_col |= opt & 0xC000;
	errcode |= main_data->AddValue16(static_cast<uint16>(ul_col));

	XL_ASSERT((opt & ~0xC000) == 0);
	lr_col |= opt & 0xC000;
	errcode |= main_data->AddValue16(static_cast<uint16>(lr_col));

	return errcode;
}

int8 formula_t::PushFunction(expr_function_code_t func) {
	uint16 argcntmask = NumberOfArgsForExcelFunction(func);
	int8 errcode = NO_ERRORS;
	if(argcntmask == A_0 || argcntmask == A_1 || argcntmask == A_2 ||
	    argcntmask == A_3 || argcntmask == A_4 || argcntmask == A_5 ||
	    argcntmask == A_UNKNOWN) {
		errcode |= main_data->AddValue8(OP_FUNC | CELLOP_AS_VALUE);
		errcode |= main_data->AddValue16(func);
	}
	else {
		errcode = GENERAL_ERROR;
	}
	return errcode;
}

int8 formula_t::PushFunction(expr_function_code_t func, size_t argcount) {
	uint16 argcntmask = NumberOfArgsForExcelFunction(func);
	int8 errcode = NO_ERRORS;
	if(argcntmask == A_UNKNOWN || (argcntmask & ~(1U << argcount))) {
		errcode |= main_data->AddValue8(OP_FUNCVAR | CELLOP_AS_VALUE);
		errcode |= main_data->AddValue8((uint8)argcount & 0x7F); // no prompt for user: 0x80 not set
		errcode |= main_data->AddValue16((uint16)func & 0x7FFF);
	}
	else {
		errcode = GENERAL_ERROR;
	}
	return errcode;
}

int8 formula_t::PushText(const std::string& v) {
	u16string value;
	m_GlobalRecords.char2str16(v, value);

	return PushText(value);
}

int8 formula_t::PushText(const ustring& v) {
	u16string value;
	m_GlobalRecords.wide2str16(v, value);

	return PushText(value);
}

#if !defined(__FRAMEWORK__)
int8 formula_t::PushText(const u16string& value) {
	int8 errcode = NO_ERRORS;

	errcode |= main_data->AddValue8(OP_STR);
	// TODO: clip string to 255 chars max!
	errcode |= main_data->AddUnicodeString(value, CUnit::LEN1_FLAGS_UNICODE);

	return errcode;
}

#endif

int8 formula_t::PushTextArray(const std::vector<std::string>& vec) {
	int8 errcode = NO_ERRORS;
	errcode |= main_data->AddValue8(OP_ARRAYA);
	errcode |= main_data->AddFixedDataArray(0, 7);
	errcode |= aux_data->AddValue8(1);
	errcode |= aux_data->AddValue16((uint16)vec.size());
	for(unsigned int i = 0; i<vec.size(); i++) {
		errcode |= aux_data->AddValue8(0x01);
		std::string str = vec[i];
		u16string value;
		m_GlobalRecords.char2str16(str, value);

		errcode |= aux_data->AddUnicodeString(value, CUnit::LEN1_FLAGS_UNICODE);
	}
	return errcode;
}

int8 formula_t::PushTextArray(const std::vector<ustring>& vec) {
	int8 errcode = NO_ERRORS;
	errcode |= main_data->AddValue8(OP_ARRAYA);
	errcode |= main_data->AddFixedDataArray(0, 7);
	errcode |= aux_data->AddValue8(1);
	errcode |= aux_data->AddValue16((uint16)vec.size());
	for(unsigned int i = 0; i<vec.size(); i++) {
		errcode |= aux_data->AddValue8(0x01);
		ustring str = vec[i];
		u16string value;
		m_GlobalRecords.wide2str16(str, value);

		errcode |= aux_data->AddUnicodeString(value, CUnit::LEN1_FLAGS_UNICODE);
	}
	return errcode;
}

int8 formula_t::PushFloatingPointArray(const std::vector<double>& vec) {
	int8 errcode = NO_ERRORS;
	errcode |= main_data->AddValue8(OP_ARRAYA);
	errcode |= aux_data->AddValue8(1);
	errcode |= aux_data->AddValue16(static_cast<uint16>(vec.size()));
	for(unsigned int i = 0; i<vec.size(); i++) {
		errcode |= aux_data->AddValue8(0x02);
		errcode |= aux_data->AddValue64FP(vec[i]);
	}
	return errcode;
}

void formula_t::DumpData(CUnit &dst) const
{
	dst.Append(*main_data);
	dst.Append(*aux_data);
}

size_t formula_t::GetSize(void) const
{
	return main_data->GetDataSize() + aux_data->GetDataSize();
}

void formula_t::GetResultEstimate(estimated_formula_result_t &dst) const
{
	dst.SetCalcOnLoad();
	dst.SetErrorCode(XLERR_VALUE);
}
