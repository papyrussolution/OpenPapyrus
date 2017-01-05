/*
 * $Id: util.h,v 1.47 2016/02/29 23:12:43 sfeam Exp $
 */

/* GNUPLOT - util.h */

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

#ifndef GNUPLOT_UTIL_H
# define GNUPLOT_UTIL_H

//#include "gp_types.h"
#include "stdfn.h"		/* for size_t */

#define NO_CARET (-1) // special token number meaning 'do not draw the "caret"', for int_error and friends:
#define DATAFILE (-2) // token number meaning 'the error was in the datafile, not the command line'

extern bool screen_ok; // true if command just typed; becomes false whenever we send some other output to screen.  
	// If false, the command line will be echoed to the screen before the ^ error message.
extern char * decimalsign;       // decimal sign 
extern char * numeric_locale;    // LC_NUMERIC 
extern char * current_locale;	 // LC_TIME 
extern char   degree_sign[8];    // degree sign 
extern const  char * current_prompt; // needed by is_error() and friends

/* Functions exported by util.c: */

// Command parsing helpers: 
//int    GpC.Eq(int, const char *);
//int    almost_equals(int, const char *);
//int    isstring(int);
int    isanumber(int);
int    isletter(int);
int    is_definition(int);
void   copy_str(char *, int, int);
size_t token_len(int);
//void   capture(char *, int, int, int);
//void   m_capture(char **, int, int);
//void   m_quote_capture(char **, int, int);
//char * GpC.TryToGetString();
void   parse_esc(char *);
int    type_udv(int);
char * gp_stradd(const char *, const char *);
#define isstringvalue(_tok) (GpC.IsString(_tok) || type_udv(_tok)==STRING)
// HBB 20010726: IMHO this one belongs into alloc.c: 
char * gp_strdup(const char *);
// HBB 20020405: moved this here, from axis.[ch] 
void   gprintf(char *, size_t, char *, double, double);

// Error message handling 
#if defined(VA_START) && defined(STDC_HEADERS)
	#if defined(__GNUC__)
		void os_error(int, const char *, ...)) __attribute__((noreturn);
		void int_error(int, const char *, ...)) __attribute__((noreturn);
		void common_error_exit() __attribute__((noreturn);
	#elif defined(_MSC_VER)
		__declspec(noreturn) void os_error(int, const char *, ...);
		__declspec(noreturn) void int_error(int, const char *, ...);
		__declspec(noreturn) void common_error_exit();
	#else
	    void os_error(int, const char *, ...);
		void int_error(int, const char *, ...);
		void common_error_exit();
	#endif
	void int_warn(int, const char *, ...);
#else
	void os_error();
	void int_error();
	void int_warn();
	void common_error_exit();
#endif
//
// FIXME HBB 20010726: should be moved to where help_comamnd() is, and
// made static. Currently, that's command.c, but it should probably move to help.c, instead.
//
void squash_spaces(char *);
bool existdir(const char *);
bool existfile(const char *);
char * getusername();
bool contains8bit(const char *s);
bool utf8toulong(ulong * wch, const char ** str);
size_t strlen_utf8(const char *s);
size_t gp_strlen(const char *s);
char * gp_strchrn(const char *s, int N);
bool streq(const char *a, const char *b);
size_t strappend(char **dest, size_t *size, size_t len, const char *src);
char *num_to_str(double r);
char *value_to_str(t_value *val, bool need_quotes);
// To disallow 8-bit characters in variable names, set this to
// #define ALLOWED_8BITVAR(c) false 
#define ALLOWED_8BITVAR(c) ((c)&0x80)

#endif /* GNUPLOT_UTIL_H */
