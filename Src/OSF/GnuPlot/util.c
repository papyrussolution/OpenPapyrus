/* GNUPLOT - util.c */

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
#include <gnuplot.h>
#pragma hdrstop
#if defined(HAVE_DIRENT_H)
	#include <sys/types.h>
	#include <dirent.h>
#elif defined(_WIN32)
	#include <windows.h>
#endif
#if defined(__MSC__) || defined (__WATCOMC__)
	#include <io.h>
	#include <direct.h>
#endif
#include "version.h"
//
//
//
const char gnuplot_version[] = "5.1";
const char gnuplot_patchlevel[] = "0";
#ifdef DEVELOPMENT_VERSION
#include "timestamp.h"
#else
const char gnuplot_date[] = "2015-01-20 ";
#endif
const char gnuplot_copyright[] = "Copyright (C) 1986-1993, 1998, 2004, 2007-2015";

const char faq_location[] = FAQ_LOCATION;

char * compile_options = (char *)0;      /* Will be loaded at runtime */

const char bug_email[] = "gnuplot-beta@lists.sourceforge.net";
const char help_email[] = "gnuplot-beta@lists.sourceforge.net";
//
// Exported (set-table) variables
//
// true if command just typed; becomes false whenever we
// send some other output to screen.  If false, the command line will be echoed to the screen before the ^ error message.
//bool   screen_ok;
char * decimalsign = NULL; /* decimal sign */
char   degree_sign[8] = "°"; /* degree sign.  Defaults to UTF-8 but will be changed to match encoding */
char * numeric_locale = NULL; /* Holds the name of the current LC_NUMERIC as set by "set decimal locale" */
char * current_locale = NULL; /* Holds the name of the current LC_TIME as set by "set locale" */
const  char * current_prompt = NULL; /* to be set by read_line() */
//
// internal prototypes
//
static void mant_exp(double, double, bool, double *, int *, const char *);
static void parse_sq(char*);
static bool utf8_getmore(ulong * wch, const char ** str, int nbytes);
static char * utf8_strchrn(const char * s, int N);
//
// GpGg.Gp__C.Eq() compares string value of token number t_num with str[], and returns true if they are identical.
//
//int GpGg.Gp__C.Eq(int t_num, const char * str)
int GpCommand::Eq(int t_num, const char * str) const
{
	if(t_num < 0 || t_num >= NumTokens || !P_Token[t_num].is_token)
		return false;
	else {
		int    i;
		for(i = 0; i < P_Token[t_num].length; i++) {
			if(P_InputLine[P_Token[t_num].start_index + i] != str[i])
				return false;
		}
		// now return true if at end of str[], false if not
		return (str[i] == NUL);
	}
}

int GpCommand::Eq(const char * str) const
{
	return Eq(CToken, str);
	/*
	if(CToken < 0 || CToken >= NumTokens) // safer to test here than to trust all callers
		return false;
	else if(!P_Token[CToken].is_token)
		return false; // must be a value--can't be equal
	else {
		int    i;
		for(i = 0; i < P_Token[CToken].length; i++) {
			if(P_InputLine[P_Token[CToken].start_index + i] != str[i])
				return false;
		}
		// now return true if at end of str[], false if not
		return (str[i] == NUL);
	}
	*/
}

//int isstring(int t_num)
int GpCommand::IsString(int t_num) const
{
	const LexicalUnit & r_tok = P_Token[t_num];
	return (r_tok.is_token && (P_InputLine[r_tok.start_index] == '\'' || P_InputLine[r_tok.start_index] == '"'));
}
//
// Test for the existence of a variable without triggering errors.
// Return values:
//   0	variable does not exist or is not defined
//   >0	type of variable: INTGR, CMPLX, STRING
//
//int type_udv(int t_num)
int GpCommand::TypeDdv(int t_num) 
{
	UdvtEntry ** pp_udv = &GpGg.Ev.first_udv;
	while(*pp_udv) {
		if(Eq(t_num, (*pp_udv)->udv_name))
			return ((*pp_udv)->udv_value.type == NOTDEFINED) ? 0 : (*pp_udv)->udv_value.type;
		else
			pp_udv = &((*pp_udv)->next_udv);
	}
	return 0;
}

//int isanumber(int t_num)
int GpCommand::IsANumber(int t_num) const
{
	return (!P_Token[t_num].is_token);
}

//int isletter(int t_num)
int GpCommand::IsLetter(int t_num) const
{
	uchar c = P_InputLine[P_Token[t_num].start_index];
	return (P_Token[t_num].is_token && (isalpha(c) || (c == '_') || ALLOWED_8BITVAR(c)));
}
/*
 * is_definition() returns true if the next tokens are of the form
 *   identifier =
 *              -or-
 *   identifier ( identifer {,identifier} ) =
 */
//int is_definition(int t_num)
int GpCommand::IsDefinition(/*int t_num*/) const
{
	int t_num = CToken;
	if(IsLetter(t_num) && Eq(t_num + 1, "=")) // variable? 
		return 1;
	else if(IsLetter(t_num) && Eq(t_num + 1, "(") && IsLetter(t_num + 2)) { // function? look for dummy variables 
		t_num += 3; // point past first dummy 
		while(Eq(t_num, ",")) {
			if(!IsLetter(++t_num))
				return 0;
			t_num += 1;
		}
		return (Eq(t_num, ")") && Eq(t_num + 1, "="));
	}
	else // neither 
		return 0; 
}
//
//   copies the string in token number t_num into str, appending
//   a null.  No more than max chars are copied (including \0).
//
void GpCommand::CopyStr(char * str, int t_num, int max)
{
	if(t_num >= NumTokens) {
		*str = NUL;
	}
	else {
		int    i = 0;
		int    start = P_Token[t_num].start_index;
		int    count = P_Token[t_num].length;
		if(count >= max) {
			count = max - 1;
			FPRINTF((stderr, "str buffer overflow in GpCommand::CopyStr"));
		}
		do {
			str[i++] = P_InputLine[start++];
		} while(i != count);
		str[i] = NUL;
	}
}
/*
 * capture() copies into str[] the part of GpGg.Gp__C.P_InputLine[] which lies between
 * the begining of token[start] and end of token[end].
 */
void GpCommand::Capture(char * str, int start, int end, int max)
{
	int    i;
	int    e = P_Token[end].start_index + P_Token[end].length;
	if((e - P_Token[start].start_index) >= max) {
		e = P_Token[start].start_index + max - 1;
		FPRINTF((stderr, "str buffer overflow in capture"));
	}
	for(i = P_Token[start].start_index; i < e && P_InputLine[i] != NUL; i++)
		*str++ = P_InputLine[i];
	*str = NUL;
}
//
// m_capture() is similar to capture(), but it mallocs storage for the string.
//
//void m_capture(char ** str, int start, int end)
void GpCommand::MCapture(char ** str, int start, int end)
{
	char * s;
	int e = P_Token[end].start_index + P_Token[end].length;
	*str = (char *)gp_realloc(*str, (e - P_Token[start].start_index + 1), "string");
	s = *str;
	for(int i = P_Token[start].start_index; i < e && P_InputLine[i] != NUL; i++)
		*s++ = P_InputLine[i];
	*s = NUL;
}

/*
 * m_quote_capture() is similar to m_capture(), but it removes
 * quotes from either end of the string.
 */
//void m_quote_capture(char ** str, int start, int end)
void GpCommand::MQuoteCapture(char ** str, int start, int end)
{
	int    i;
	char * s;
	int    e = P_Token[end].start_index + P_Token[end].length - 1;
	*str = (char *)gp_realloc(*str, (e - P_Token[start].start_index + 1), "string");
	s = *str;
	for(i = P_Token[start].start_index + 1; i < e && P_InputLine[i] != NUL; i++)
		*s++ = P_InputLine[i];
	*s = NUL;
	if(P_InputLine[P_Token[start].start_index] == '"')
		parse_esc(*str);
	else
		parse_sq(*str);
}
//
// Wrapper for isstring + m_quote_capture or const_string_express
//
char * GpCommand::TryToGetString()
{
	char * newstring = NULL;
	const int save_token = CToken;
	if(!EndOfCommand()) {
		t_value a;
		P.ConstStringExpress(&a);
		if(a.type == STRING)
			newstring = a.v.string_val;
		else
			CToken = save_token;
	}
	return newstring;
}

int GpCommand::TryToGetString(SString & rBuf)
{
	rBuf = 0;
	int    ok = 0;
	const  int save_token = CToken;
	if(!EndOfCommand()) {
		t_value a;
		P.ConstStringExpress(&a);
		if(a.type == STRING) {
			rBuf = a.v.string_val;
			ok = 1;
		}
		else
			CToken = save_token;
	}
	return ok;
}
//
// Our own version of strdup()
// Make copy of string into malloc'd memory
// As with all conforming str*() functions,
// it is the caller's responsibility to pass valid parameters!
//
char * gp_strdup(const char * s)
{
	char * d;
	if(!s)
		return NULL;
#ifndef HAVE_STRDUP
	d = (char *)malloc(strlen(s) + 1);
	if(d)
		memcpy(d, s, strlen(s) + 1);
#else
	d = _strdup(s);
#endif
	return d;
}
//
// Allocate a new string and initialize it by concatenating two existing strings.
//
char * gp_stradd(const char * a, const char * b)
{
	char * p_new = (char *)malloc(strlen(a)+strlen(b)+1);
	strcpy(p_new, a);
	strcat(p_new, b);
	return p_new;
}

/* HBB 20020405: moved these functions here from axis.c, where they no
 * longer truly belong. */
/*{{{  mant_exp - split into mantissa and/or exponent */
/* HBB 20010121: added code that attempts to fix rounding-induced
 * off-by-one errors in 10^%T and similar output formats */
static void mant_exp(double log10_base, double x,
    bool scientific,        /* round to power of 3 */
    double * m,                  /* results */
    int * p, const char * format)         /* format string for fixup */
{
	int sign = 1;
	double l10;
	int power;
	double mantissa;
	/*{{{  check 0 */
	if(x == 0) {
		ASSIGN_PTR(m, 0);
		ASSIGN_PTR(p, 0);
	}
	else {
		/*}}} */
		/*{{{  check -ve */
		if(x < 0) {
			sign = (-1);
			x = (-x);
		}
		/*}}} */
		l10 = log10(x) / log10_base;
		power = (int)floor(l10);
		mantissa = pow(10.0, log10_base * (l10 - power));

		/* round power to an integer multiple of 3, to get what's
		* sometimes called 'scientific' or 'engineering' notation. Also
		* useful for handling metric unit prefixes like 'kilo' or 'micro'
		* */
		if(scientific) {
			/* Scientific mode makes no sense whatsoever if the base of
			* the logarithmic axis is anything but 10.0 */
			assert(log10_base == 1.0);

			/* HBB FIXED 20040701: negative modulo positive may yield
			* negative result.  But we always want an effectively
			* positive modulus --> adjust input by one step */
			switch(power % 3) {
				case -1:
					power -= 3;
				case 2:
					mantissa *= 100;
					break;
				case -2:
					power -= 3;
				case 1:
					mantissa *= 10;
					break;
				case 0:
					break;
				default:
					GpGg.IntErrorNoCaret("Internal error in scientific number formatting");
			}
			power -= (power % 3);
		}

		/* HBB 20010121: new code for decimal mantissa fixups.  Looks at
		* format string to see how many decimals will be put there.  Iff
		* the number is so close to an exact power of 10 that it will be
		* rounded up to 10.0e??? by an sprintf() with that many digits of
		* precision, increase the power by 1 to get a mantissa in the
		* region of 1.0.  If this handling is not wanted, pass NULL as
		* the format string */
		/* HBB 20040521: extended to also work for bases other than 10.0 */
		if(format) {
			double actual_base = (scientific ? 1000 : pow(10.0, log10_base));
			int precision = 0;
			double tolerance;
			format = strchr(format, '.');
			if(format != NULL)
				/* a decimal point was found in the format, so use that
				* precision. */
				precision = strtol(format + 1, NULL, 10);

			/* See if mantissa would be right on the border.  The
			* condition to watch out for is that the mantissa is within
			* one printing precision of the next power of the logarithm
			* base.  So add the 0.5*10^-precision to the mantissa, and
			* see if it's now larger than the base of the scale */
			tolerance = pow(10.0, -precision) / 2;
			if(mantissa + tolerance >= actual_base) {
				mantissa /= actual_base;
				power += (scientific ? 3 : 1);
			}
		}
		ASSIGN_PTR(m, (sign * mantissa));
		ASSIGN_PTR(p, power);
	}
}

/*}}} */

/*{{{  gprintf */
/* extended s(n)printf */
/* HBB 20010121: added code to maintain consistency between mantissa
 * and exponent across sprintf() calls.  The problem: format string
 * '%t*10^%T' will display 9.99 as '10.0*10^0', but 10.01 as
 * '1.0*10^1'.  This causes problems for people using the %T part,
 * only, with logscaled axes, in combination with the occasional
 * round-off error. */
/* EAM Nov 2012:
 * Unbelievably, the count parameter has been silently ignored or
 * improperly applied ever since this routine was introduced back
 * in version 3.7.  Now fixed to prevent buffer overflow.
 */
void gprintf(char * outstring, size_t count, const char * format, double log10_base, double x)
{
	char tempdest[MAX_LINE_LEN + 1];
	char temp[MAX_LINE_LEN + 1];
	char * t;
	bool seen_mantissa = false; /* remember if mantissa was already output */
	double stored_power_base = 0; /* base for the last mantissa output*/
	int stored_power = 0;   /* power matching the mantissa output earlier */
	bool got_hash = false;
	char * dest  = &tempdest[0];
	char * limit = &tempdest[MAX_LINE_LEN];
	static double log10_of_1024; /* to avoid excess precision comparison in check of connection %b -- %B */
	log10_of_1024 = log10(1024.0);
#define remaining_space (size_t)(limit-dest)
	*dest = '\0';
	set_numeric_locale();
	/* Oct 2013 - default format is now expected to be "%h" */
	if(((term->flags & TERM_IS_LATEX)) && !strcmp(format, DEF_FORMAT))
		format = DEF_FORMAT_LATEX;
	for(;; ) {
		/*{{{  copy to dest until % */
		while(*format != '%')
			if(!(*dest++ = *format++) || (remaining_space == 0)) {
				goto done;
			}
		/*}}} */

		/*{{{  check for %% */
		if(format[1] == '%') {
			*dest++ = '%';
			format += 2;
			continue;
		}
		/*}}} */

		/*{{{  copy format part to temp, excluding conversion character */
		t = temp;
		*t++ = '%';
		if(format[1] == '#') {
			*t++ = '#';
			format++;
			got_hash = true;
		}
		/* dont put isdigit first since side effect in macro is bad */
		while(*++format == '.' || isdigit((uchar)*format) || oneof4(*format, '-', '+', ' ', '\''))
			*t++ = *format;
		/*}}} */

		/*{{{  convert conversion character */
		switch(*format) {
			/*{{{  x and o */
			case 'x':
			case 'X':
			case 'o':
			case 'O':
			    if(fabs(x) >= (double)INT_MAX) {
				    t[0] = 'l';
				    t[1] = 'l';
				    t[2] = *format;
				    t[3] = '\0';
				    snprintf(dest, remaining_space, temp, (int64)x);
			    }
			    else {
				    t[0] = *format;
				    t[1] = '\0';
				    snprintf(dest, remaining_space, temp, (int)x);
			    }
			    break;
			/*}}} */
			/*{{{  e, f and g */
			case 'e':
			case 'E':
			case 'f':
			case 'F':
			case 'g':
			case 'G':
			    t[0] = *format;
			    t[1] = 0;
			    snprintf(dest, remaining_space, temp, x);
			    break;
			case 'h':
			case 'H':
			    /* g/G with enhanced formating (if applicable) */
			    t[0] = (*format == 'h') ? 'g' : 'G';
			    t[1] = 0;
			    if(!(term->flags & (TERM_ENHANCED_TEXT|TERM_IS_LATEX))) { // Not enhanced, not latex, just print it 
				    snprintf(dest, remaining_space, temp, x); 
			    }
			    else if(table_mode) { // Tabular output should contain no markup 
				    snprintf(dest, remaining_space, temp, x); 
			    }
			    else {
				    /* in enhanced mode -- convert E/e to x10^{foo} or *10^{foo} */
#define LOCAL_BUFFER_SIZE 256
				    char tmp[LOCAL_BUFFER_SIZE];
				    char tmp2[LOCAL_BUFFER_SIZE];
				    int i, j;
				    bool bracket_flag = false;
				    snprintf(tmp, 240, temp, x); /* magic number alert: why 240? */
				    for(i = j = 0; tmp[i] && (i < LOCAL_BUFFER_SIZE); i++) {
					    if(tmp[i]=='E' || tmp[i]=='e') {
						    if((term->flags & TERM_IS_LATEX)) {
							    if(*format == 'h') {
								    strcpy(&tmp2[j], "\\times");
								    j += 6;
							    }
							    else {
								    strcpy(&tmp2[j], "\\cdot");
								    j += 5;
							    }
						    }
						    else switch(encoding) {
								    case S_ENC_UTF8:
									strcpy(&tmp2[j], "\xc3\x97"); /* UTF character
								                                        '×' */
									j += 2;
									break;
								    case S_ENC_CP1252:
									tmp2[j++] = (*format=='h') ? 0xd7 : 0xb7;
									break;
								    case S_ENC_ISO8859_1:
								    case S_ENC_ISO8859_2:
								    case S_ENC_ISO8859_9:
								    case S_ENC_ISO8859_15:
									tmp2[j++] = (*format=='h') ? 0xd7 : '*';
									break;
								    default:
									tmp2[j++] = (*format=='h') ? 'x' : '*';
									break;
							    }

						    strcpy(&tmp2[j], "10^{");
						    j += 4;
						    bracket_flag = true;

						    /* Skip + and leading 0 in exponent */
						    i++; /* skip E */
						    if(tmp[i] == '+')
							    i++;
						    else if(tmp[i] == '-') /* copy sign */
							    tmp2[j++] = tmp[i++];
						    while(tmp[i] == '0')
							    i++;
						    i--; /* undo following loop increment */
					    }
					    else {
						    tmp2[j++] = tmp[i];
					    }
				    }
				    if(bracket_flag)
					    tmp2[j++] = '}';
				    tmp2[j] = '\0';
				    strncpy(dest, tmp2, remaining_space);
#undef LOCAL_BUFFER_SIZE
			    }

			    break;

			/*}}} */
			/*{{{  l --- mantissa to current log base */
			case 'l':
		    {
			    double mantissa;
			    t[0] = 'f';
			    t[1] = 0;
			    stored_power_base = log10_base;
			    mant_exp(stored_power_base, x, false, &mantissa, &stored_power, temp);
			    seen_mantissa = true;
			    snprintf(dest, remaining_space, temp, mantissa);
			    break;
		    }
			/*}}} */
			/*{{{  t --- base-10 mantissa */
			case 't':
		    {
			    double mantissa;
			    t[0] = 'f';
			    t[1] = 0;
			    stored_power_base = 1.0;
			    mant_exp(stored_power_base, x, false, &mantissa, &stored_power, temp);
			    seen_mantissa = true;
			    snprintf(dest, remaining_space, temp, mantissa);
			    break;
		    }
			/*}}} */
			/*{{{  s --- base-1000 / 'scientific' mantissa */
			case 's':
		    {
			    double mantissa;
			    t[0] = 'f';
			    t[1] = 0;
			    stored_power_base = 1.0;
			    mant_exp(stored_power_base, x, true, &mantissa, &stored_power, temp);
			    seen_mantissa = true;
			    snprintf(dest, remaining_space, temp, mantissa);
			    break;
		    }
			/*}}} */
			/*{{{  b --- base-1024 mantissa */
			case 'b':
		    {
			    double mantissa;
			    t[0] = 'f';
			    t[1] = 0;
			    stored_power_base = log10_of_1024;
			    mant_exp(stored_power_base, x, false, &mantissa, &stored_power, temp);
			    seen_mantissa = true;
			    snprintf(dest, remaining_space, temp, mantissa);
			    break;
		    }
			/*}}} */
			/*{{{  L --- power to current log base */
			case 'L':
		    {
			    int power;
			    t[0] = 'd';
			    t[1] = 0;
			    if(seen_mantissa) {
				    if(stored_power_base == log10_base) {
					    power = stored_power;
				    }
				    else {
					    GpGg.IntErrorNoCaret("Format character mismatch: %%L is only valid with %%l");
				    }
			    }
			    else {
				    stored_power_base = log10_base;
				    mant_exp(log10_base, x, false, NULL, &power, "%.0f");
			    }
			    snprintf(dest, remaining_space, temp, power);
			    break;
		    }
			/*}}} */
			/*{{{  T --- power of ten */
			case 'T':
		    {
			    int power;
			    t[0] = 'd';
			    t[1] = 0;
			    if(seen_mantissa) {
				    if(stored_power_base == 1.0) {
					    power = stored_power;
				    }
				    else {
					    GpGg.IntErrorNoCaret("Format character mismatch: %%T is only valid with %%t");
				    }
			    }
			    else {
				    mant_exp(1.0, x, false, NULL, &power, "%.0f");
			    }
			    snprintf(dest, remaining_space, temp, power);
			    break;
		    }
			/*}}} */
			/*{{{  S --- power of 1000 / 'scientific' */
			case 'S':
		    {
			    int power;
			    t[0] = 'd';
			    t[1] = 0;
			    if(seen_mantissa) {
				    if(stored_power_base == 1.0) {
					    power = stored_power;
				    }
				    else {
					    GpGg.IntErrorNoCaret("Format character mismatch: %%S is only valid with %%s");
				    }
			    }
			    else {
				    mant_exp(1.0, x, true, NULL, &power, "%.0f");
			    }
			    snprintf(dest, remaining_space, temp, power);
			    break;
		    }
			/*}}} */
			/*{{{  c --- ISO decimal unit prefix letters */
			case 'c':
		    {
			    int power;
			    t[0] = 'c';
			    t[1] = 0;
			    if(seen_mantissa) {
				    if(stored_power_base == 1.0) {
					    power = stored_power;
				    }
				    else {
					    GpGg.IntErrorNoCaret("Format character mismatch: %%c is only valid with %%s");
				    }
			    }
			    else {
				    mant_exp(1.0, x, true, NULL, &power, "%.0f");
			    }
			    if(power >= -24 && power <= 24) {
				    /* name  power   name  power
				       -------------------------
				       yocto  -24    yotta  24
				       zepto  -21    zetta  21
				       atto   -18    Exa    18
				       femto  -15    Peta   15
				       pico   -12    Tera   12
				       nano    -9    Giga    9
				       micro   -6    Mega    6
				       milli   -3    kilo    3   */
				    /* -18 -> 0, 0 -> 6, +18 -> 12, ... */
				    /* HBB 20010121: avoid division of -ve ints! */
				    power = (power + 24) / 3;
				    snprintf(dest, remaining_space, temp, "yzafpnum kMGTPEZY"[power]);
			    }
			    else {
				    /* please extend the range ! */
				    /* fall back to simple exponential */
				    snprintf(dest, remaining_space, "e%+02d", power);
			    }
			    break;
		    }
			/*}}} */
			/*{{{  B --- IEC 60027-2 A.2 / ISO/IEC 80000 binary unit prefix letters */
			case 'B':
		    {
			    int power;
			    t[0] = 'c';
			    t[1] = 'i';
			    t[2] = '\0';
			    if(seen_mantissa) {
				    if(stored_power_base == log10_of_1024) {
					    power = stored_power;
				    }
				    else {
					    GpGg.IntErrorNoCaret("Format character mismatch: %%B is only valid with %%b");
				    }
			    }
			    else {
				    mant_exp(log10_of_1024, x, false, NULL, &power, "%.0f");
			    }
			    if(power > 0 && power <= 8) {
				    /* name  power
				       -----------
				       Yobi   8
				       Zebi   7
				       Exbi   9
				       Pebi   5
				       Tebi   4
				       Gibi   3
				       Mebi   2
				       kibi   1   */
				    snprintf(dest, remaining_space, temp, " kMGTPEZY"[power]);
			    }
			    else if(power > 8) {
				    /* for the larger values, print x2^{10}Gi for example */
				    snprintf(dest, remaining_space, "x2^{%d}Yi", power-8);
			    }
			    else if(power < 0) {
				    snprintf(dest, remaining_space, "x2^{%d}", power*10);
			    }
			    else {
				    snprintf(dest, remaining_space, "  ");
			    }

			    break;
		    }
			/*}}} */
			/*{{{  P --- multiple of pi */
			case 'P':
		    {
			    t[0] = 'f';
			    t[1] = 0;
			    snprintf(dest, remaining_space, temp, x / M_PI);
			    break;
		    }
			/*}}} */
			default:
			    reset_numeric_locale();
			    GpGg.IntErrorNoCaret("Bad format character");
		} /* switch */
		  /*}}} */

		if(got_hash && (format != strpbrk(format, "oeEfFgG"))) {
			reset_numeric_locale();
			GpGg.IntErrorNoCaret("Bad format character");
		}

		/* change decimal '.' to the actual entry in decimalsign */
		if(decimalsign != NULL) {
			char * dotpos1 = dest;
			char * dotpos2;
			size_t newlength = strlen(decimalsign);
			// dot is the default decimalsign we will be replacing
			int dot = *get_decimal_locale();
			// replace every dot by the contents of decimalsign
			while((dotpos2 = strchr(dotpos1, dot)) != NULL) {
				if(newlength == 1) { /* The normal case */
					*dotpos2 = *decimalsign;
					dotpos1++;
				}
				else {  /* Some multi-byte decimal marker */
					size_t taillength = strlen(dotpos2);
					dotpos1 = dotpos2 + newlength;
					if((dotpos1 + taillength) > limit)
						GpGg.IntErrorNoCaret("format too long due to decimalsign string");
					/* move tail end of string out of the way */
					memmove(dotpos1, dotpos2 + 1, taillength);
					/* insert decimalsign */
					memcpy(dotpos2, decimalsign, newlength);
				}
			}
		}
		/* this was at the end of every single case, before: */
		dest += strlen(dest);
		++format;
	} /* for ever */
done:
#if(0)
	/* Oct 2013 - Not safe because it fails to recognize LaTeX macros.	*/
	/* For LaTeX terminals, if the user has not already provided a          */
	/* format in math mode, wrap whatever we got by default in $...$        */
	if(((term->flags & TERM_IS_LATEX)) && !strchr(tempdest, '$')) {
		*(outstring++) = '$';
		strcat(tempdest, "$");
		count -= 2;
	}
#endif

	/* Copy as much as fits */
	strnzcpy(outstring, tempdest, count);
	reset_numeric_locale();
}

/*}}} */

/* some macros for the error and warning functions below
 * may turn this into a utility function later
 */
//#define PRINT_MESSAGE_TO_STDERR do { fprintf(stderr, "\n%s%s\n", current_prompt ? current_prompt : "", GpGg.Gp__C.P_InputLine); } while(0)
void GpCommand::PrintMessageToStderr()
{
	fprintf(stderr, "\n%s%s\n", current_prompt ? current_prompt : "", P_InputLine);
}

void PrintSpacesUnderPrompt()
{
	if(current_prompt) {
		for(const char * p = current_prompt; *p != '\0'; p++)
			fputc(' ', stderr);
	}
}

/*
#define PRINT_SPACES_UPTO_TOKEN						\
	do { \
		for(int i = 0; i < GpGg.Gp__C.P_Token[t_num].start_index; i++)			   \
			fputc((GpGg.Gp__C.P_InputLine[i] == '\t') ? '\t' : ' ', stderr);  \
	} while(0)
*/

void GpCommand::PrintSpacesUpToToken(int tokNum)
{
	for(int i = 0; i < P_Token[/*t_num*/tokNum].start_index; i++)
		fputc((P_InputLine[i] == '\t') ? '\t' : ' ', stderr);
}

//#define PRINT_CARET fputs("^\n", stderr);

void GpGadgets::PrintFileAndLine(GpCommand & rC)
{
	if(!IsInteractive) {
		if(lf_head && lf_head->name) 
			fprintf(stderr, "\"%s\", line %d: ", lf_head->name, rC.InlineNum);
		else 
			fprintf(stderr, "line %d: ", rC.InlineNum);			   
	}
}
//
// os_error() is just like int_error() except that it calls perror().
// 
void os_error(int t_num, const char * str, ...)
{
	va_list args;
#ifdef VMS
	static status[2] = { 1, 0 };            /* 1 is count of error msgs */
#endif /* VMS */
	// reprint line if screen has been written to 
	if(t_num == DATAFILE) {
		GpDf.DfShowData();
	}
	else if(t_num != NO_CARET) {    /* put caret under error */
		if(!GpGg.screen_ok)
			GpGg.Gp__C.PrintMessageToStderr();
		PrintSpacesUnderPrompt();
		GpGg.Gp__C.PrintSpacesUpToToken(t_num);
		fputs("^\n", stderr); //PRINT_CARET;
	}
	PrintSpacesUnderPrompt();
	VA_START(args, str);
#if defined(HAVE_VFPRINTF) || _LIBC
	vfprintf(stderr, str, args);
#else
	_doprnt(str, args, stderr);
#endif
	va_end(args);
	putc('\n', stderr);
	PrintSpacesUnderPrompt();
	GpGg.PrintFileAndLine(GpGg.Gp__C);
#ifdef VMS
	status[1] = vaxc$errno;
	sys$putmsg(status);
#else
	perror("system error");
#endif
	putc('\n', stderr);
	GpGg.Ev.FillGpValString("GPVAL_ERRMSG", strerror(errno));
	GpGg.CommonErrorExit();
}

void GpGadgets::IntErrorNoCaret(const char * pStr, ...)
{
	va_list arg_list;
	va_start(arg_list, pStr);
	IntError(NO_CARET, pStr, arg_list);
	va_end(arg_list);
}

void GpGadgets::IntErrorCurToken(const char * pStr, ...)
{
	va_list arg_list;
	va_start(arg_list, pStr);
	IntError(Gp__C.CToken, pStr, arg_list);
	va_end(arg_list);
}

void GpGadgets::IntError(int t_num, const char * pStr, ...)
{
	va_list args;
	char error_message[128] = {'\0'};
	// reprint line if screen has been written to 
	if(t_num == DATAFILE) {
		GpDf.DfShowData();
	}
	else if(t_num != NO_CARET) { // put caret under error 
		if(!screen_ok)
			Gp__C.PrintMessageToStderr();
		PrintSpacesUnderPrompt();
		Gp__C.PrintSpacesUpToToken(t_num);
		fputs("^\n", stderr); //PRINT_CARET;
	}
	PrintSpacesUnderPrompt();
	PrintFileAndLine(Gp__C);
	VA_START(args, pStr);
#if defined(HAVE_VFPRINTF) || _LIBC
	vsnprintf(error_message, sizeof(error_message), pStr, args);
	fprintf(stderr, "%.120s", error_message);
#else
	_doprnt(pStr, args, stderr);
#endif
	va_end(args);
	fputs("\n\n", stderr);
	Ev.FillGpValString("GPVAL_ERRMSG", error_message);
	CommonErrorExit();
}

void GpGadgets::CommonErrorExit()
{
	// We are bailing out of nested context without ever reaching 
	// the normal cleanup code. Reset any flags before bailing.   
	df_reset_after_error();
	eval_reset_after_error();
	Gp__C.ClauseResetAfterError();
	Gp__C.P.ParseResetAfterError();
	Gp__C.P.IsScanningRangeInProgress = false;
	inside_zoom = false;
	// Load error state variables 
	Ev.UpdateGpValVariables(2);
	bail_to_command_line();
}
//
// Warn without bailing out to command line. Not a user error 
//
//void IntWarn(int t_num, const char * str, ...)
void GpGadgets::IntWarn(int t_num, const char * str, ...)
{
	va_list args;
	// reprint line if screen has been written to 
	if(t_num == DATAFILE) {
		GpDf.DfShowData();
	}
	else if(t_num != NO_CARET) { /* put caret under error */
		if(!screen_ok)
			Gp__C.PrintMessageToStderr();
		PrintSpacesUnderPrompt();
		Gp__C.PrintSpacesUpToToken(t_num);
		fputs("^\n", stderr); //PRINT_CARET;
	}
	PrintSpacesUnderPrompt();
	PrintFileAndLine(Gp__C);
	fputs("warning: ", stderr);
	VA_START(args, str);
#if defined(HAVE_VFPRINTF) || _LIBC
	vfprintf(stderr, str, args);
#else
	_doprnt(str, args, stderr);
#endif
	va_end(args);
	putc('\n', stderr);
}

/*}}} */

/* Squash spaces in the given string (DFK) */
/* That is, reduce all multiple white-space chars to single spaces */
/* Done in place. Currently used only by help_command() */
void squash_spaces(char * s)
{
	char * r = s;   /* reading point */
	char * w = s;   /* writing point */
	bool space = false; /* true if we've already copied a space */

	for(w = r = s; *r != NUL; r++) {
		if(isspace((uchar) *r)) {
			/* white space; only copy if we haven't just copied a space */
			if(!space) {
				space = true;
				*w++ = ' ';
			}       /* else ignore multiple spaces */
		}
		else {
			/* non-space character; copy it and clear flag */
			*w++ = *r;
			space = false;
		}
	}
	*w = NUL;               /* null terminate string */
}

/* postprocess single quoted strings: replace "''" by "'"
 */
void parse_sq(char * instr)
{
	char * s = instr, * t = instr;

	/* the string will always get shorter, so we can do the
	 * conversion in situ
	 */

	while(*s != NUL) {
		if(*s == '\'' && *(s+1) == '\'')
			s++;
		*t++ = *s++;
	}
	*t = NUL;
}

void parse_esc(char * instr)
{
	char * s = instr, * t = instr;
	// the string will always get shorter, so we can do the conversion in situ
	while(*s != NUL) {
		if(*s == '\\') {
			s++;
			if(*s == '\\') {
				*t++ = '\\';
				s++;
			}
			else if(*s == 'n') {
				*t++ = '\n';
				s++;
			}
			else if(*s == 'r') {
				*t++ = '\r';
				s++;
			}
			else if(*s == 't') {
				*t++ = '\t';
				s++;
			}
			else if(*s == '\"') {
				*t++ = '\"';
				s++;
			}
			else if(*s >= '0' && *s <= '7') {
				int i, n;
				char * octal = (*s == '0' ? "%4o%n" : "%3o%n");
				if(sscanf(s, octal, &i, &n) > 0) {
					*t++ = i;
					s += n;
				}
				else {
					// int_error("illegal octal number ", GpGg.Gp__C.CToken);
					*t++ = '\\';
					*t++ = *s++;
				}
			}
		}
		else if(GpDf.df_separators && *s == '\"' && *(s+1) == '\"') {
			// EAM Mar 2003 - For parsing CSV strings with quoted quotes 
			*t++ = *s++; s++;
		}
		else {
			*t++ = *s++;
		}
	}
	*t = NUL;
}

/* FIXME HH 20020915: This function does nothing if dirent.h and windows.h
 * not available. */
bool existdir(const char * name)
{
#if defined(HAVE_DIRENT_H ) || defined(_WIN32)
	DIR * dp;
	if((dp = opendir(name)) == NULL)
		return false;
	closedir(dp);
	return true;
#elif defined(VMS)
	return false;
#else
	IntWarn(NO_CARET, "Test on directory existence not supported\n\t('%s!')", name);
	return false;
#endif
}

bool existfile(const char * name)
{
#ifdef __MSC__
	return (_access(name, 0) == 0);
#else
	return (access(name, F_OK) == 0);
#endif
}

char * getusername()
{
	char * username = getenv("USER");
	SETIFZ(username, getenv("USERNAME"));
	return gp_strdup(username);
}

bool contains8bit(const char * s)
{
	while(*s) {
		if((*s++ & 0x80))
			return true;
	}
	return false;
}

#define INVALID_UTF8 0xfffful

/* Read from second byte to end of UTF-8 sequence.
   used by utf8toulong() */
static bool utf8_getmore(ulong * wch, const char ** str, int nbytes)
{
	const ulong minvalue[] = {0x80, 0x800, 0x10000, 0x200000, 0x4000000};
	for(int i = 0; i < nbytes; i++) {
		const uchar c = (uchar)**str;
		if((c & 0xc0) != 0x80) {
			*wch = INVALID_UTF8;
			return false;
		}
		*wch = (*wch << 6) | (c & 0x3f);
		(*str)++;
	}
	// check for overlong UTF-8 sequences
	if(*wch < minvalue[nbytes-1]) {
		*wch = INVALID_UTF8;
		return false;
	}
	else
		return true;
}
//
// Convert UTF-8 multibyte sequence from string to ulong character.
// Returns true on success.
//
bool utf8toulong(ulong * wch, const char ** str)
{
	const uchar c = (uchar) *(*str)++;
	if((c & 0x80) == 0) {
		*wch = (ulong)c;
		return true;
	}
	else if((c & 0xe0) == 0xc0) {
		*wch = c & 0x1f;
		return utf8_getmore(wch, str, 1);
	}
	else if((c & 0xf0) == 0xe0) {
		*wch = c & 0x0f;
		return utf8_getmore(wch, str, 2);
	}
	else if((c & 0xf8) == 0xf0) {
		*wch = c & 0x07;
		return utf8_getmore(wch, str, 3);
	}
	else if((c & 0xfc) == 0xf8) {
		*wch = c & 0x03;
		return utf8_getmore(wch, str, 4);
	}
	else if((c & 0xfe) == 0xfc) {
		*wch = c & 0x01;
		return utf8_getmore(wch, str, 5);
	}
	else {
		*wch = INVALID_UTF8;
		return false;
	}
}
/*
 * Returns number of (possibly multi-byte) characters in a UTF-8 string
 */
size_t strlen_utf8(const char * s)
{
	int i = 0, j = 0;
	while(s[i]) {
		if((s[i] & 0xc0) != 0x80) j++;
		i++;
	}
	return j;
}

size_t gp_strlen(const char * s)
{
	if(encoding == S_ENC_UTF8)
		return strlen_utf8(s);
	else
		return strlen(s);
}

/*
 * Returns a pointer to the Nth character of s
 * or a pointer to the trailing \0 if N is too large
 */
static char * utf8_strchrn(const char * s, int N)
{
	int i = 0, j = 0;
	if(N <= 0)
		return (char*)s;
	while(s[i]) {
		if((s[i] & 0xc0) != 0x80) {
			if(j++ == N) 
				return (char*)&s[i];
		}
		i++;
	}
	return (char*)&s[i];
}

char * gp_strchrn(const char * s, int N)
{
	if(encoding == S_ENC_UTF8)
		return utf8_strchrn(s, N);
	else
		return (char*)&s[N];
}

/* true if strings a and b are identical save for leading or trailing whitespace */
bool streq(const char * a, const char * b)
{
	int enda, endb;
	while(isspace((uchar)*a))
		a++;
	while(isspace((uchar)*b))
		b++;
	enda = (*a) ? strlen(a) - 1 : 0;
	endb = (*b) ? strlen(b) - 1 : 0;
	while(isspace((uchar)a[enda]))
		enda--;
	while(isspace((uchar)b[endb]))
		endb--;
	return (enda == endb) ? !strncmp(a, b, ++enda) : false;
}

/* append string src to dest
   re-allocates memory if necessary, (re-)determines the length of the
   destination string only if len==0
 */
size_t strappend(char ** dest, size_t * size, size_t len, const char * src)
{
	size_t destlen = (len != 0) ? len : strlen(*dest);
	size_t srclen = strlen(src);
	if((destlen + srclen + 1) > *size) {
		*size *= 2;
		*dest = (char*)gp_realloc(*dest, *size, "strappend");
	}
	memcpy(*dest + destlen, src, srclen + 1);
	return destlen + srclen;
}

/* convert a t_value to a string */
char * value_to_str(t_value * val, bool need_quotes)
{
	static int i = 0;
	static char * s[4] = {NULL, NULL, NULL, NULL};
	static size_t c[4] = {0, 0, 0, 0};
	static const int minbufsize = 54;
	int j = i;
	i = (i + 1) % 4;
	if(s[j] == NULL) {
		s[j] = (char*)malloc(minbufsize);
		c[j] = minbufsize;
	}
	switch(val->type) {
		case INTGR:
		    sprintf(s[j], "%d", val->v.int_val);
		    break;
		case CMPLX:
		    if(fisnan(val->v.cmplx_val.real))
			    sprintf(s[j], "NaN");
		    else if(val->v.cmplx_val.imag != 0.0)
			    sprintf(s[j], "{%s, %s}", num_to_str(val->v.cmplx_val.real), num_to_str(val->v.cmplx_val.imag));
		    else
			    return num_to_str(val->v.cmplx_val.real);
		    break;
		case STRING:
		    if(val->v.string_val) {
			    if(!need_quotes) {
				    return val->v.string_val;
			    }
			    else {
				    char * cstr = conv_text(val->v.string_val);
				    size_t reqsize = strlen(cstr) + 3;
				    if(reqsize > c[j]) {
					    /* Don't leave c[j[ non-zero if realloc fails */
					    s[j] = (char*)gp_realloc(s[j], reqsize + 20, NULL);
					    if(s[j] != NULL) {
						    c[j] = reqsize + 20;
					    }
					    else {
						    c[j] = 0;
						    GpGg.IntErrorNoCaret("out of memory");
					    }
				    }
				    sprintf(s[j], "\"%s\"", cstr);
			    }
		    }
		    else {
			    s[j][0] = NUL;
		    }
		    break;
		case DATABLOCK:
	    {
		    char ** dataline = val->v.data_array;
		    int nlines = 0;
		    if(dataline) {
			    while(*dataline++)
				    nlines++;
		    }
		    sprintf(s[j], "<%d line data block>", nlines);
		    break;
	    }
		case ARRAY:
	    {
		    sprintf(s[j], "<%d element array>", val->v.value_array->v.int_val);
		    break;
	    }
		case NOTDEFINED:
	    {
		    sprintf(s[j], "<undefined>");
		    break;
	    }
		default:
		    GpGg.IntErrorNoCaret("unknown type in value_to_str()");
	}
	return s[j];
}

/* Helper for value_to_str(): convert a single number to decimal
 * format. Rotates through 4 buffers 's[j]', and returns pointers to
 * them, to avoid execution ordering problems if this function is
 * called more than once between sequence points. */
char * num_to_str(double r)
{
	static int i = 0;
	static char s[4][25];
	int j = i++;
	if(i > 3)
		i = 0;
	sprintf(s[j], "%.15g", r);
	if(!strchr(s[j], '.') && !strchr(s[j], 'e') && !strchr(s[j], 'E'))
		strcat(s[j], ".0");
	return s[j];
}
//
// ALLOC
//
#ifndef GP_FARMALLOC
	#ifdef FARALLOC
		#define GP_FARMALLOC(size) farmalloc((size))
		#define GP_FARREALLOC(p, size) farrealloc((p), (size))
	#else
		#ifdef MALLOC_ZERO_RETURNS_ZERO
			#define GP_FARMALLOC(size) malloc((size_t)((size==0) ? 1 : size))
		#else
			#define GP_FARMALLOC(size) malloc((size_t)(size))
		#endif
		#define GP_FARREALLOC(p, size) realloc((p), (size_t)(size))
	#endif
#endif
//
// malloc:
// allocate memory
// This is a protected version of malloc. It causes an int_error
// if there is not enough memory. If message is NULL, we allow NULL return.
// Otherwise, we handle the error, using the message to create the int_error string.
// Note cp/sp_extend uses realloc, so it depends on this using malloc().
// 
void * unused_gp_alloc(size_t size, const char * message)
{
	char * p = (char *)GP_FARMALLOC(size); /* try again */
	if(p == NULL) {
		if(message) {
			GpGg.IntErrorNoCaret("out of memory for %s", message);
			// NOTREACHED
		}
	}
	return (p);
}
//
// note gp_realloc assumes that failed realloc calls leave the original mem
// block allocated. If this is not the case with any C compiler, a substitue
// realloc function has to be used.
//
void * gp_realloc(void * p, size_t size, const char * message)
{
	void * res = 0; // the new allocation 
	// realloc(NULL,x) is meant to do malloc(x), but doesn't always 
	if(!p)
		res = malloc(size);
	else {
		res = (char *)realloc(p, size);
		if(!res) {
			if(message)
				GpGg.IntErrorNoCaret("out of memory for %s", message);
		}
	}
	return (res);
}

#ifdef FARALLOC
	void gpfree(generic * p)
	{
	#ifdef _Windows
		HGLOBAL hGlobal = GlobalHandle(p);
		GlobalUnlock(hGlobal);
		GlobalFree(hGlobal);
	#else
		farfree(p);
	#endif
	}
#endif

