/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *  must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *  may be used to endorse or promote products derived from this software
 *  without specific prior written permission.
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop
/*
 * Get next token from string *stringp, where tokens are possibly-empty
 * strings separated by characters from delim.
 *
 * Writes NULs into the string at *stringp to end tokens.
 * delim need not remain constant from call to call.
 * On return, *stringp points past the last NUL written (if there might
 * be further tokens), or is NULL (if there are definitely no more tokens).
 *
 * If *stringp is NULL, strsep returns NULL.
 *
 * PUBLIC: #ifndef HAVE_STRSEP
 * PUBLIC: char *strsep __P((char **, const char *));
 * PUBLIC: #endif
 */
char * strsep(char ** stringp, const char * delim)
{
	char * s;
	const char * spanp;
	int c, sc;
	char * tok;
	if((s = *stringp) == NULL)
		return NULL;
	for(tok = s;;) {
		c = *s++;
		spanp = delim;
		do {
			if((sc = *spanp++) == c) {
				if(c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*stringp = s;
				return tok;
			}
		} while(sc != 0);
	}
	/* NOTREACHED */
}
