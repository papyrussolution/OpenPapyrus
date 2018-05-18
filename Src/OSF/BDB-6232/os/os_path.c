/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2017 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop
/*
 * __os_concat_path --
 *	Concatenate two elements of a path.
 * PUBLIC: int __os_concat_path __P((char *,
 * PUBLIC:     size_t, const char *, const char *));
 */
int __os_concat_path(char * dest, size_t destsize, const char * path, const char * file)
{
	if((size_t)snprintf(dest, destsize, "%s%c%s", path, PATH_SEPARATOR[0], file) >= destsize)
		return (EINVAL);
	return (0);
}
