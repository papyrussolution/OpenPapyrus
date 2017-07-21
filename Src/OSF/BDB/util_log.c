/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000, 2011 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop
/*
 * __db_util_logset --
 *	Log that we're running.
 */
int __db_util_logset(const char * progname, char * fname)
{
	pid_t pid;
	FILE * fp;
	__time64_t now;
	char time_buf[CTIME_BUFLEN];
	if((fp = fopen(fname, "w")) == NULL)
		goto err;
	_time64(&now);
	__os_id(NULL, &pid, 0);
	fprintf(fp, "%s: %lu %s", progname, (ulong)pid, __os_ctime(&now, time_buf));
	if(fclose(fp) == EOF)
		goto err;
	return 0;
err:
	fprintf(stderr, "%s: %s: %s\n", progname, fname, strerror(errno));
	return 1;
}
