/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001, 2011 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop

static int __db_cursor_check_func __P((DBC*, DBC*, uint32 *, db_pgno_t, uint32, void *));
static int __db_cursor_check __P((DB *));
/*
 * __db_truncate_pp
 *	DB->truncate pre/post processing.
 */
int __db_truncate_pp(DB * dbp, DB_TXN * txn, uint32 * countp, uint32 flags)
{
	DB_THREAD_INFO * ip;
	int ret, t_ret;
	ENV * env = dbp->env;
	int handle_check = 0;
	int txn_local = 0;
	STRIP_AUTO_COMMIT(flags);
	/* Check for invalid flags. */
	if(F_ISSET(dbp, DB_AM_SECONDARY)) {
		__db_errx(env, DB_STR("0685", "DB->truncate forbidden on secondary indices"));
		return EINVAL;
	}
	if((ret = __db_fchk(env, "DB->truncate", flags, 0)) != 0)
		return ret;
	ENV_ENTER(env, ip);
	XA_CHECK_TXN(ip, txn);
	/*
	 * Make sure there are no active cursors on this db.  Since we drop
	 * pages we cannot really adjust cursors.
	 */
	if((ret = __db_cursor_check(dbp)) != 0) {
		__db_errx(env, DB_STR("0686", "DB->truncate not permitted with active cursors"));
		goto err;
	}
#ifdef CONFIG_TEST
	if(IS_REP_MASTER(env))
		DB_TEST_WAIT(env, env->test_check);
#endif
	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if(handle_check && (ret = __db_rep_enter(dbp, 1, 0, IS_REAL_TXN(txn))) != 0) {
		handle_check = 0;
		goto err;
	}
	/*
	 * Check for changes to a read-only database.  This must be after the
	 * replication block so that we cannot race master/client state changes.
	 */
	if(DB_IS_READONLY(dbp)) {
		ret = __db_rdonly(env, "DB->truncate");
		goto err;
	}
	/*
	 * Create local transaction as necessary, check for consistent
	 * transaction usage.
	 */
	if(IS_DB_AUTO_COMMIT(dbp, txn)) {
		if((ret = __txn_begin(env, ip, NULL, &txn, 0)) != 0)
			goto err;
		txn_local = 1;
	}
	/* Check for consistent transaction usage. */
	if((ret = __db_check_txn(dbp, txn, DB_LOCK_INVALIDID, 0)) != 0)
		goto err;
	ret = __db_truncate(dbp, ip, txn, countp);
err:
	if(txn_local && (t_ret = __db_txn_auto_resolve(env, txn, 0, ret)) && ret == 0)
		ret = t_ret;
	/* Release replication block. */
	if(handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __db_truncate
 *	DB->truncate.
 */
int __db_truncate(DB * dbp, DB_THREAD_INFO * ip, DB_TXN * txn, uint32 * countp)
{
	DB * sdbp;
	uint32 scount;
	int t_ret;
	ENV * env = dbp->env;
	DBC * dbc = NULL;
	int ret = 0;
	/*
	 * Run through all secondaries and truncate them first.  The count
	 * returned is the count of the primary only.  QUEUE uses normal
	 * processing to truncate so it will update the secondaries normally.
	 */
	if(dbp->type != DB_QUEUE && DB_IS_PRIMARY(dbp)) {
		if((ret = __db_s_first(dbp, &sdbp)) != 0)
			return ret;
		for(; sdbp && !ret; ret = __db_s_next(&sdbp, txn))
			if((ret = __db_truncate(sdbp, ip, txn, &scount)) != 0)
				break;
		if(sdbp)
			__db_s_done(sdbp, txn);
		if(ret != 0)
			return ret;
	}
	DB_TEST_RECOVERY(dbp, DB_TEST_PREDESTROY, ret, 0);
	/* Acquire a cursor. */
	if((ret = __db_cursor(dbp, ip, txn, &dbc, 0)) != 0)
		return ret;
	DEBUG_LWRITE(dbc, txn, "DB->truncate", NULL, NULL, 0);
#ifdef HAVE_PARTITION
	if(DB_IS_PARTITIONED(dbp))
		ret = __part_truncate(dbc, countp);
	else
#endif
	switch(dbp->type) {
	    case DB_BTREE:
	    case DB_RECNO: ret = __bam_truncate(dbc, countp); break;
	    case DB_HASH: ret = __ham_truncate(dbc, countp); break;
	    case DB_HEAP: ret = __heap_truncate(dbc, countp); break;
	    case DB_QUEUE: ret = __qam_truncate(dbc, countp); break;
	    case DB_UNKNOWN:
	    default: ret = __db_unknown_type(env, "DB->truncate", dbp->type); break;
	}
	// Discard the cursor.
	if(dbc && (t_ret = __dbc_close(dbc)) != 0 && !ret)
		ret = t_ret;
	DB_TEST_RECOVERY(dbp, DB_TEST_POSTDESTROY, ret, 0);
	DB_TEST_RECOVERY_LABEL
	return ret;
}

static int __db_cursor_check_func(DBC * dbc, DBC * my_dbc, uint32 * foundp, db_pgno_t pgno, uint32 indx, void * args)
{
	COMPQUIET(my_dbc, 0);
	COMPQUIET(args, 0);
	COMPQUIET(pgno, 0);
	COMPQUIET(indx, 0);
	if(IS_INITIALIZED(dbc)) {
		*foundp = 1;
		return EEXIST;
	}
	return 0;
}
/*
 * __db_cursor_check --
 *	See if there are any active cursors on this db.
 */
static int __db_cursor_check(DB * dbp)
{
	uint32 found;
	int ret = __db_walk_cursors(dbp, NULL, __db_cursor_check_func, &found, 0, 0, 0);
	return ret == EEXIST ? EINVAL : ret;
}
