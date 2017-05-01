/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2011 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop
/*
 * log_compare --
 *	Compare two LSN's; return 1, 0, -1 if first is >, == or < second.
 */
int log_compare(const DB_LSN * lsn0, const DB_LSN * lsn1)
{
	return LOG_COMPARE(lsn0, lsn1);
}
/*
 * __log_check_page_lsn --
 *	Panic if the page's lsn in past the end of the current log.
 */
int __log_check_page_lsn(ENV * env, DB * dbp, DB_LSN * lsnp)
{
	int ret;
	LOG * lp = (LOG *)env->lg_handle->reginfo.primary;
	LOG_SYSTEM_LOCK(env);
	ret = LOG_COMPARE(lsnp, &lp->lsn);
	LOG_SYSTEM_UNLOCK(env);
	if(ret < 0)
		return 0;
	__db_errx(env, DB_STR_A("2506", "file %s has LSN %lu/%lu, past end of log at %lu/%lu", "%s %lu %lu %lu %lu"),
		dbp == NULL || dbp->fname == NULL ? DB_STR_P("unknown") : dbp->fname, (ulong)lsnp->file, (ulong)lsnp->offset,
		(ulong)lp->lsn.file, (ulong)lp->lsn.offset);
	__db_errx(env, DB_STR("2507", "Commonly caused by moving a database from one database environment"));
	__db_errx(env, DB_STR("2508", "to another without clearing the database LSNs, or by removing all of"));
	__db_errx(env, DB_STR("2509", "the log files from a database environment"));
	return EINVAL;
}
