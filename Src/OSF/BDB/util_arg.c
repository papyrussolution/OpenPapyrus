/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001, 2011 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
// @v9.5.5 #include "dbinc/db_page.h"
// @v9.5.5 #include "dbinc/lock.h"
// @v9.5.5 #include "dbinc/mp.h"
// @v9.5.5 #include "dbinc/crypto.h"
// @v9.5.5 #include "dbinc/btree.h"
// @v9.5.5 #include "dbinc/hash.h"
#pragma hdrstop

#if DB_VERSION_MAJOR < 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR < 5
/*
 * !!!
 * We build this file in old versions of Berkeley DB when we're doing test
 * runs using the test_micro tool.   Without a prototype in place, we get
 * warnings, and there's no simple workaround.
 */
char * strsep(char ** stringp, const char * delim);
#endif

/*
 * __db_util_arg --
 *	Convert a string into an argc/argv pair.
 *
 * PUBLIC: int __db_util_arg __P((char *, char *, int *, char ***));
 */
int __db_util_arg(char * arg0, char * str, int * argcp, char *** argvp)
{
	int n, ret;
	char ** ap, ** argv;
#define MAXARGS 25
	if((ret = __os_malloc(NULL, (MAXARGS+1)*sizeof(char **), &argv)) != 0)
		return ret;
	ap = argv;
	*ap++ = arg0;
	for(n = 1; (*ap = strsep(&str, " \t")) != NULL; )
		if(**ap != '\0') {
			++ap;
			if(++n == MAXARGS)
				break;
		}
	*ap = NULL;
	*argcp = (int)(ap-argv);
	*argvp = argv;
	return 0;
}
