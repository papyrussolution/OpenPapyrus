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
 *	Reset the LSNs for every page in the file.
 */
static int __env_lsn_reset(ENV * env, DB_THREAD_INFO * ip, const char * name, int encrypted)
{
	DB * dbp;
	int t_ret, ret;
	// Create the DB object
	if((ret = __db_create_internal(&dbp, env, 0)) != 0)
		return ret;
	// If configured with a password, the databases are encrypted. 
	if(encrypted && (ret = __db_set_flags(dbp, DB_ENCRYPT)) != 0)
		goto err;
	/*
	 * Open the DB file.
	 *
	 * !!!
	 * Note DB_RDWRMASTER flag, we need to open the master database file
	 * for writing in this case.
	 */
	if((ret = __db_open(dbp, ip, NULL, name, NULL, DB_UNKNOWN, DB_RDWRMASTER, 0, PGNO_BASE_MD)) != 0) {
		__db_err_simple_text(env, ret, name);
		goto err;
	}
	ret = __db_lsn_reset(dbp->mpf, ip);
#ifdef HAVE_PARTITION
	if(ret == 0 && DB_IS_PARTITIONED(dbp))
		ret = __part_lsn_reset(dbp, ip);
	else
#endif
	if(ret == 0 && dbp->type == DB_QUEUE)
#ifdef HAVE_QUEUE
		ret = __qam_lsn_reset(dbp, ip);
#else
		ret = __db_no_queue_am(env);
#endif
err:
	if((t_ret = __db_close(dbp, NULL, 0)) != 0 && ret == 0)
		ret = t_ret;
	return ret;
}
/*
 * __env_lsn_reset_pp --
 *	ENV->lsn_reset pre/post processing.
 */
int __env_lsn_reset_pp(DB_ENV * dbenv, const char * name, uint32 flags)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	ENV_ILLEGAL_BEFORE_OPEN(env, "DB_ENV->lsn_reset");
	/*
	 * !!!
	 * The actual argument checking is simple, do it inline, outside of
	 * the replication block.
	 */
	if(flags != 0 && flags != DB_ENCRYPT)
		return __db_ferr(env, "DB_ENV->lsn_reset", 0);
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__env_lsn_reset(env, ip, name, LF_ISSET(DB_ENCRYPT) ? 1 : 0)), 1, ret);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __db_lsn_reset -- reset the lsn for a db mpool handle.
 */
int FASTCALL __db_lsn_reset(DB_MPOOLFILE * mpf, DB_THREAD_INFO * ip)
{
	PAGE * pagep;
	int ret;
	// Reset the LSN on every page of the database file. 
	for(db_pgno_t pgno = 0; (ret = __memp_fget(mpf, &pgno, ip, NULL, DB_MPOOL_DIRTY, &pagep)) == 0; ++pgno) {
		LSN_NOT_LOGGED(pagep->lsn);
		if((ret = __memp_fput(mpf, ip, pagep, DB_PRIORITY_UNCHANGED)) != 0)
			break;
	}
	return (ret == DB_PAGE_NOTFOUND) ? 0 : ret;
}
