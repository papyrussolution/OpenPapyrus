// regposerr.c - Oniguruma (regular expression library)
// Copyright (c) 2002-2020  K.Kosako All rights reserved.
//
#include "regint.h"
#pragma hdrstop
/* Can't include regint.h etc.. for conflict of regex_t.
   Define ONIGURUMA_EXPORT here for onigposix.h.
 */
#ifndef ONIGURUMA_EXPORT
#define ONIGURUMA_EXPORT
#endif

#include "onigposix.h"

#undef regex_t
#undef regmatch_t
#undef regoff_t
#undef regcomp
#undef regexec
#undef regfree
#undef regerror
#undef reg_set_encoding
#undef reg_name_to_group_numbers
#undef reg_foreach_name
#undef reg_number_of_names

#if defined(__GNUC__)
	#define ARG_UNUSED  __attribute__ ((unused))
#else
	#define ARG_UNUSED
#endif
//#if defined(_WIN32) && !defined(__GNUC__)
	//#ifndef xsnprintf
		//#define xsnprintf   sprintf_s
	//#endif
	//#ifndef xstrncpy
		//#define xstrncpy(dest, src, size)   strncpy_s(dest, size, src, _TRUNCATE)
	//#endif
//#else
	//#ifndef xsnprintf
		//#define xsnprintf   snprintf
	//#endif
	//#ifndef xstrncpy
		//#define xstrncpy    strncpy
	//#endif
//#endif

static const char * ESTRING[] = {
	NULL,
	"failed to match",                   /* REG_NOMATCH    */
	"Invalid regular expression",        /* REG_BADPAT     */
	"invalid collating element referenced", /* REG_ECOLLATE   */
	"invalid character class type referenced", /* REG_ECTYPE     */
	"bad backslash-escape sequence",     /* REG_EESCAPE    */
	"invalid back reference number",     /* REG_ESUBREG    */
	"imbalanced [ and ]",                /* REG_EBRACK     */
	"imbalanced ( and )",                /* REG_EPAREN     */
	"imbalanced { and }",                /* REG_EBRACE     */
	"invalid repeat range {n,m}",        /* REG_BADBR      */
	"invalid range",                     /* REG_ERANGE     */
	SlTxtOutOfMem,                     /* REG_ESPACE     */
	"? * + not preceded by valid regular expression", /* REG_BADRPT   */

	/* Extended errors */
	"internal error",                    /* REG_EONIG_INTERNAL */
	"invalid wide char value",           /* REG_EONIG_BADWC    */
	"invalid argument"                   /* REG_EONIG_BADARG   */
};

extern size_t onig_posix_regerror(int posix_ecode, const onig_posix_regex_t* reg ARG_UNUSED, char * buf, size_t size)
{
	const char * s;
	char tbuf[35];
	size_t len;
	if(posix_ecode > 0 && posix_ecode < (int)(sizeof(ESTRING) / sizeof(ESTRING[0]))) {
		s = ESTRING[posix_ecode];
	}
	else if(posix_ecode == 0) {
		s = "";
	}
	else {
		/*xsnprintf*/slsprintf_s(tbuf, sizeof(tbuf), "undefined error code (%d)", posix_ecode);
		s = tbuf;
	}
	len = strlen(s) + 1; /* use strlen() because s is ascii encoding. */
	if(buf && size > 0) {
		strnzcpy(buf, s, size);
		//xstrncpy(buf, s, size-1);
		//buf[size-1] = '\0';
	}
	return len;
}

#ifdef USE_BINARY_COMPATIBLE_POSIX_API
	extern size_t regerror(int posix_ecode, const onig_posix_regex_t * reg ARG_UNUSED, char * buf, size_t size)
	{
		return onig_posix_regerror(posix_ecode, reg, buf, size);
	}
#endif
