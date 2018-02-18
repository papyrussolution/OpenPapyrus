// BDB_TXN.C
// See the file LICENSE for redistribution information.
// Copyright (c) 1996, 2011 Oracle and/or its affiliates.  All rights reserved.
//
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop
#include "dbinc_auto/db_auto.h"
#include "dbinc_auto/crdel_auto.h"
#include "dbinc_auto/db_ext.h"

#define LOG_FLAGS(txn) (DB_LOG_COMMIT|(F_ISSET(txn, TXN_SYNC) ? DB_FLUSH : (F_ISSET(txn, TXN_WRITE_NOSYNC) ? DB_LOG_WRNOSYNC : 0)))
/*
 * __txn_isvalid enumerated types.  We cannot simply use the transaction
 * statuses, because different statuses need to be handled differently
 * depending on the caller.
 */
typedef enum {
	TXN_OP_ABORT,
	TXN_OP_COMMIT,
	TXN_OP_DISCARD,
	TXN_OP_PREPARE
} txnop_t;

static int __txn_abort_pp(DB_TXN *);
static int __txn_applied(ENV*, DB_THREAD_INFO*, DB_COMMIT_INFO*, db_timeout_t);
static void __txn_build_token(DB_TXN*, DB_LSN *);
static int __txn_begin_int(DB_TXN *);
static int __txn_close_cursors(DB_TXN *);
static int __txn_commit_pp(DB_TXN*, uint32);
static int __txn_discard(DB_TXN*, uint32);
static int __txn_dispatch_undo(ENV*, DB_TXN*, DBT*, DB_LSN*, DB_TXNHEAD *);
static int __txn_end(DB_TXN*, int);
static int __txn_isvalid(const DB_TXN*, txnop_t);
static int __txn_undo(DB_TXN *);
static int __txn_set_commit_token(DB_TXN*txn, DB_TXN_TOKEN *);
static void __txn_set_txn_lsnp(DB_TXN*, DB_LSN**, DB_LSN**);
static int __txn_init(ENV*, DB_TXNMGR *);
static int __txn_compare(const void *, const void *);
static int __txn_print_all(ENV*, uint32);
static int __txn_print_stats(ENV*, uint32);
static int __txn_stat(ENV*, DB_TXN_STAT**, uint32);
static char * __txn_status(DB_TXN_ACTIVE *);
static char * __txn_xa_status(DB_TXN_ACTIVE *);
static void __txn_gid(ENV*, DB_MSGBUF*, DB_TXN_ACTIVE *);

static void __clear_fe_watermark(DB_TXN *, DB *);

#define TxnAlloc "Unable to allocate a transaction handle"
/*
 * __txn_begin_pp --
 *	ENV->txn_begin pre/post processing.
 */
int __txn_begin_pp(DB_ENV * dbenv, DB_TXN * parent, DB_TXN ** txnpp, uint32 flags)
{
	DB_THREAD_INFO * ip;
	int rep_check, ret;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->tx_handle, "txn_begin", DB_INIT_TXN);
	if((ret = __db_fchk(env, "txn_begin", flags, DB_IGNORE_LEASE|DB_READ_COMMITTED|DB_READ_UNCOMMITTED|
		DB_TXN_FAMILY|DB_TXN_NOSYNC|DB_TXN_SNAPSHOT|DB_TXN_SYNC|DB_TXN_WAIT|DB_TXN_WRITE_NOSYNC|DB_TXN_NOWAIT|DB_TXN_BULK)) != 0)
		return ret;
	if((ret = __db_fcchk(env, "txn_begin", flags, DB_TXN_WRITE_NOSYNC|DB_TXN_NOSYNC, DB_TXN_SYNC)) != 0)
		return ret;
	if((ret = __db_fcchk(env, "txn_begin", flags, DB_TXN_WRITE_NOSYNC, DB_TXN_NOSYNC)) != 0)
		return ret;
	if(parent && LF_ISSET(DB_TXN_FAMILY)) {
		__db_errx(env, DB_STR("4521", "Family transactions cannot have parents"));
		return EINVAL;
	}
	else if(IS_REAL_TXN(parent) && !F_ISSET(parent, TXN_SNAPSHOT) && LF_ISSET(DB_TXN_SNAPSHOT)) {
		__db_errx(env, DB_STR("4522", "Child transaction snapshot setting must match parent"));
		return EINVAL;
	}
	ENV_ENTER(env, ip);

	/* Replication accounts for top-level transactions. */
	rep_check = IS_ENV_REPLICATED(env) && !IS_REAL_TXN(parent) && !LF_ISSET(DB_TXN_FAMILY);
	if(rep_check && (ret = __op_rep_enter(env, 0, 1)) != 0)
		goto err;
	ret = __txn_begin(env, ip, parent, txnpp, flags);
	/*
	 * We only decrement the count if the operation fails.
	 * Otherwise the count will be decremented when the
	 * txn is resolved by txn_commit, txn_abort, etc.
	 */
	if(ret != 0 && rep_check)
		__op_rep_exit(env);
err:
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __txn_begin --
 *	ENV->txn_begin.
 *
 * This is a wrapper to the actual begin process.  We allocate a DB_TXN
 * structure for the caller and then call into __txn_begin_int code.
 *
 * Internally, we use TXN_DETAIL structures, but the DB_TXN structure
 * provides access to the transaction ID and the offset in the transaction
 * region of the TXN_DETAIL structure.
 */
int __txn_begin(ENV*env, DB_THREAD_INFO * ip, DB_TXN * parent, DB_TXN ** txnpp, uint32 flags)
{
	DB_ENV * dbenv;
	DB_LOCKREGION * region;
	DB_TXN * txn;
	TXN_DETAIL * ptd, * td;
	int ret;
	if(F_ISSET(env, ENV_FORCE_TXN_BULK))
		flags |= DB_TXN_BULK;
	*txnpp = NULL;
	if((ret = __os_calloc(env, 1, sizeof(DB_TXN), &txn)) != 0) {
		__db_errx(env, TxnAlloc);
		return ret;
	}
	dbenv = env->dbenv;
	txn->mgrp = env->tx_handle;
	txn->parent = parent;
	if(parent && F_ISSET(parent, TXN_FAMILY))
		parent = NULL;
	TAILQ_INIT(&txn->kids);
	TAILQ_INIT(&txn->events);
	STAILQ_INIT(&txn->logs);
	TAILQ_INIT(&txn->my_cursors);
	TAILQ_INIT(&txn->femfs);
	txn->flags = TXN_MALLOC;
	txn->thread_info = (ip ? ip : (parent ? parent->thread_info : NULL));
	/*
	 * Set the sync mode for commit.  Any local bits override those
	 * in the environment.  SYNC is the default.
	 */
	if(LF_ISSET(DB_TXN_SYNC))
		F_SET(txn, TXN_SYNC);
	else if(LF_ISSET(DB_TXN_NOSYNC))
		F_SET(txn, TXN_NOSYNC);
	else if(LF_ISSET(DB_TXN_WRITE_NOSYNC))
		F_SET(txn, TXN_WRITE_NOSYNC);
	else if(F_ISSET(dbenv, DB_ENV_TXN_NOSYNC))
		F_SET(txn, TXN_NOSYNC);
	else if(F_ISSET(dbenv, DB_ENV_TXN_WRITE_NOSYNC))
		F_SET(txn, TXN_WRITE_NOSYNC);
	else
		F_SET(txn, TXN_SYNC);
	if(LF_ISSET(DB_TXN_NOWAIT) || (F_ISSET(dbenv, DB_ENV_TXN_NOWAIT) && !LF_ISSET(DB_TXN_WAIT)))
		F_SET(txn, TXN_NOWAIT);
	if(LF_ISSET(DB_READ_COMMITTED))
		F_SET(txn, TXN_READ_COMMITTED);
	if(LF_ISSET(DB_READ_UNCOMMITTED))
		F_SET(txn, TXN_READ_UNCOMMITTED);
	if(LF_ISSET(DB_TXN_FAMILY))
		F_SET(txn, TXN_FAMILY|TXN_INFAMILY|TXN_READONLY);
	if(LF_ISSET(DB_TXN_SNAPSHOT) || F_ISSET(dbenv, DB_ENV_TXN_SNAPSHOT) || (parent && F_ISSET(parent, TXN_SNAPSHOT)))
		F_SET(txn, TXN_SNAPSHOT);
	if(LF_ISSET(DB_IGNORE_LEASE))
		F_SET(txn, TXN_IGNORE_LEASE);
	/*
	 * We set TXN_BULK only for the outermost transaction.  This
	 * is a temporary limitation; in the future we will allow it
	 * for nested transactions as well.  See #17669 for details.
	 *
	 * Also, ignore requests for DB_TXN_BULK if replication is enabled.
	 */
	if(LF_ISSET(DB_TXN_BULK) && parent == NULL && !REP_ON(txn->mgrp->env))
		F_SET(txn, TXN_BULK);
	if((ret = __txn_begin_int(txn)) != 0)
		goto err;
	td = (TXN_DETAIL *)txn->td;
	if(parent) {
		ptd = (TXN_DETAIL *)parent->td;
		TAILQ_INSERT_HEAD(&parent->kids, txn, klinks);
		SH_TAILQ_INSERT_HEAD(&ptd->kids, td, klinks, __txn_detail);
	}
	if(LOCKING_ON(env)) {
		region = (DB_LOCKREGION *)env->lk_handle->reginfo.primary;
		if(parent) {
			ret = __lock_inherit_timeout(env, parent->locker, txn->locker);
			/* No parent locker set yet. */
			if(ret == EINVAL) {
				parent = NULL;
				ret = 0;
			}
			if(ret != 0)
				goto err;
		}
		/*
		 * Parent is NULL if we have no parent
		 * or it has no timeouts set.
		 */
		if(parent == NULL && region->tx_timeout != 0)
			if((ret = __lock_set_timeout(env, txn->locker, region->tx_timeout, DB_SET_TXN_TIMEOUT)) != 0)
				goto err;
	}
	*txnpp = txn;
	PERFMON2(env, txn, begin, txn->txnid, flags);
	return 0;
err:
	__os_free(env, txn);
	return ret;
}
/*
 * __txn_recycle_id --
 *	Find a range of useable transaction ids.
 */
int __txn_recycle_id(ENV*env, int locked)
{
	DB_LSN null_lsn;
	TXN_DETAIL * td;
	uint32 * ids;
	int nids, ret;
	DB_TXNMGR * mgr = env->tx_handle;
	DB_TXNREGION * region = (DB_TXNREGION *)mgr->reginfo.primary;
	if((ret = __os_malloc(env, sizeof(uint32)*region->curtxns, &ids)) != 0) {
		__db_errx(env, DB_STR("4523", "Unable to allocate transaction recycle buffer"));
		return ret;
	}
	nids = 0;
	SH_TAILQ_FOREACH(td, &region->active_txn, links, __txn_detail)
	ids[nids++] = td->txnid;
	region->last_txnid = TXN_MINIMUM-1;
	region->cur_maxid = TXN_MAXIMUM;
	if(nids != 0)
		__db_idspace(ids, nids, &region->last_txnid, &region->cur_maxid);
	__os_free(env, ids);
	/*
	 * Check LOGGING_ON rather than DBENV_LOGGING as we want to emit this record at the end of recovery.
	 */
	if(LOGGING_ON(env)) {
		if(locked)
			TXN_SYSTEM_UNLOCK(env);
		ret = __txn_recycle_log(env, NULL, &null_lsn, 0, region->last_txnid+1, region->cur_maxid);
		/* Make it simple on the caller, if error we hold the lock. */
		if(locked && ret != 0)
			TXN_SYSTEM_LOCK(env);
	}
	return ret;
}
/*
 * __txn_begin_int --
 *	Normal DB version of txn_begin.
 */
static int __txn_begin_int(DB_TXN*txn)
{
	uint32 id;
	int ret;
	DB_TXNMGR * mgr = txn->mgrp;
	ENV * env = mgr->env;
	DB_ENV * dbenv = env->dbenv;
	DB_TXNREGION * region = (DB_TXNREGION *)mgr->reginfo.primary;
	TXN_DETAIL * td = NULL;
	int inserted = 0;
	TXN_SYSTEM_LOCK(env);
	if(!F_ISSET(txn, TXN_COMPENSATE) && F_ISSET(region, TXN_IN_RECOVERY)) {
		__db_errx(env, DB_STR("4524", "operation not permitted during recovery"));
		ret = EINVAL;
		goto err;
	}
	/*
	 * Allocate a new transaction id. Our current valid range can span
	 * the maximum valid value, so check for it and wrap manually.
	 */
	if(region->last_txnid == TXN_MAXIMUM && region->cur_maxid != TXN_MAXIMUM)
		region->last_txnid = TXN_MINIMUM-1;
	/* Allocate a new transaction detail structure. */
	if((ret = __env_alloc(&mgr->reginfo, sizeof(TXN_DETAIL), &td)) != 0) {
		__db_errx(env, DB_STR("4525", "Unable to allocate memory for transaction detail"));
		goto err;
	}
	id = ++region->last_txnid;

#ifdef HAVE_STATISTICS
	STAT_INC(env, txn, nbegins, region->stat.st_nbegins, id);
	STAT_INC(env, txn, nactive, region->stat.st_nactive, id);
	if(region->stat.st_nactive > region->stat.st_maxnactive)
		STAT_SET(env, txn, maxnactive, region->stat.st_maxnactive, region->stat.st_nactive, id);
#endif
	td->txnid = id;
	dbenv->thread_id(dbenv, &td->pid, &td->tid);
	ZERO_LSN(td->last_lsn);
	ZERO_LSN(td->begin_lsn);
	SH_TAILQ_INIT(&td->kids);
	if(txn->parent && !F_ISSET(txn->parent, TXN_FAMILY))
		td->parent = R_OFFSET(&mgr->reginfo, txn->parent->td);
	else
		td->parent = INVALID_ROFF;
	td->name = INVALID_ROFF;
	MAX_LSN(td->read_lsn);
	MAX_LSN(td->visible_lsn);
	td->mvcc_ref = 0;
	td->mvcc_mtx = MUTEX_INVALID;
	td->status = TXN_RUNNING;
	td->flags = F_ISSET(txn, TXN_NOWAIT) ? TXN_DTL_NOWAIT : 0;
	td->nlog_dbs = 0;
	td->nlog_slots = TXN_NSLOTS;
	td->log_dbs = R_OFFSET(&mgr->reginfo, td->slots);

	/* XA specific fields. */
	td->xa_ref = 1;
	td->xa_br_status = TXN_XA_IDLE;

	/* Place transaction on active transaction list. */
	SH_TAILQ_INSERT_HEAD(&region->active_txn, td, links, __txn_detail);
	region->curtxns++;
	/* Increment bulk transaction counter while holding transaction lock. */
	if(F_ISSET(txn, TXN_BULK))
		((DB_TXNREGION *)env->tx_handle->reginfo.primary)->n_bulk_txn++;
	inserted = 1;
	if(region->last_txnid == region->cur_maxid) {
		if((ret = __txn_recycle_id(env, 1)) != 0)
			goto err;
	}
	else
		TXN_SYSTEM_UNLOCK(env);
	txn->txnid = id;
	txn->td  = td;
	/* Allocate a locker for this txn. */
	if(LOCKING_ON(env) && (ret = __lock_getlocker(env->lk_handle, id, 1, &txn->locker)) != 0)
		goto err;
	txn->abort = __txn_abort_pp;
	txn->commit = __txn_commit_pp;
	txn->discard = __txn_discard;
	txn->get_name = __txn_get_name;
	txn->get_priority = __txn_get_priority;
	txn->id = __txn_id;
	txn->prepare = __txn_prepare;
	txn->set_commit_token = __txn_set_commit_token;
	txn->set_txn_lsnp = __txn_set_txn_lsnp;
	txn->set_name = __txn_set_name;
	txn->set_priority = __txn_set_priority;
	txn->set_timeout = __txn_set_timeout;
	/* We can't call __txn_set_priority until txn->td is set. */
	if(LOCKING_ON(env) && (ret = __txn_set_priority(txn,
		txn->parent == NULL ? TXN_PRIORITY_DEFAULT : txn->parent->locker->priority)) != 0)
		goto err;
	else
		td->priority = 0;
	/*
	 * If this is a transaction family, we must link the child to the
	 * maximal grandparent in the lock table for deadlock detection.
	 */
	if(txn->parent) {
		if(LOCKING_ON(env) && (ret = __lock_addfamilylocker(env, txn->parent->txnid, txn->txnid, F_ISSET(txn->parent, TXN_FAMILY))) != 0)
			goto err;
		/*
		 * If the parent is only used to establish compatability, do
		 * not reference it again.
		 */
		if(F_ISSET(txn->parent, TXN_FAMILY)) {
			txn->parent = NULL;
			F_SET(txn, TXN_INFAMILY);
		}
	}
	if(F_ISSET(txn, TXN_MALLOC)) {
		MUTEX_LOCK(env, mgr->mutex);
		TAILQ_INSERT_TAIL(&mgr->txn_chain, txn, links);
		MUTEX_UNLOCK(env, mgr->mutex);
	}
	return 0;
err:
	if(inserted) {
		TXN_SYSTEM_LOCK(env);
		SH_TAILQ_REMOVE(&region->active_txn, td, links, __txn_detail);
		region->curtxns--;
		if(F_ISSET(txn, TXN_BULK))
			((DB_TXNREGION *)env->tx_handle->reginfo.primary)->n_bulk_txn--;
	}
	if(td)
		__env_alloc_free(&mgr->reginfo, td);
	TXN_SYSTEM_UNLOCK(env);
	return ret;
}
/*
 * __txn_continue
 *	Fill in the fields of the local transaction structure given
 *	the detail transaction structure.  Optionally link transactions
 *	to transaction manager list.
 */
int __txn_continue(ENV*env, DB_TXN * txn, TXN_DETAIL * td, DB_THREAD_INFO * ip, int add_to_list)
{
	DB_LOCKREGION * region;
	DB_TXNMGR * mgr;
	int ret = 0;
	/*
	 * This code follows the order of the structure definition so it
	 * is relatively easy to make sure that we are setting everything.
	 */
	mgr = txn->mgrp = env->tx_handle;
	txn->parent = NULL;
	txn->thread_info = ip;
	txn->txnid = td->txnid;
	txn->name = NULL;
	txn->td = td;
	td->xa_ref++;
	/* This never seems to be used: txn->expire */
	txn->txn_list = NULL;
	TAILQ_INIT(&txn->kids);
	TAILQ_INIT(&txn->events);
	STAILQ_INIT(&txn->logs);
	/*
	 * These fields should never persist across different processes as we
	 * require that cursors be opened/closed within the same service routine
	 * and we disallow file level operations in XA transactions.
	 */
	TAILQ_INIT(&txn->my_cursors);
	TAILQ_INIT(&txn->femfs);
	/* Put the transaction onto the transaction manager's list. */
	if(add_to_list) {
		MUTEX_LOCK(env, mgr->mutex);
		TAILQ_INSERT_TAIL(&mgr->txn_chain, txn, links);
		MUTEX_UNLOCK(env, mgr->mutex);
	}
	txn->token_buffer = 0;
	txn->cursors = 0;
	txn->abort = __txn_abort_pp;
	txn->commit = __txn_commit_pp;
	txn->discard = __txn_discard;
	txn->get_name = __txn_get_name;
	txn->get_priority = __txn_get_priority;
	txn->id = __txn_id;
	txn->prepare = __txn_prepare;
	txn->set_commit_token = __txn_set_commit_token;
	txn->set_name = __txn_set_name;
	txn->set_priority = __txn_set_priority;
	txn->set_timeout = __txn_set_timeout;
	txn->set_txn_lsnp = __txn_set_txn_lsnp;

	/* XXX Do we need to explicitly set a SYNC flag here? */
	txn->flags = TXN_MALLOC|(F_ISSET(td, TXN_DTL_NOWAIT) ? TXN_NOWAIT : 0);
	txn->xa_thr_status = TXN_XA_THREAD_NOTA;
	/*
	 * If this is a restored transaction, we need to propagate that fact
	 * to the process-local structure.  However, if it's not a restored
	 * transaction, we need to make sure that we have a locker associated
	 * with this transaction.
	 */
	if(F_ISSET(td, TXN_DTL_RESTORED))
		F_SET(txn, TXN_RESTORED);
	else
	if((ret = __lock_getlocker(env->lk_handle, txn->txnid, 0, &txn->locker)) == 0)
		ret = __txn_set_priority(txn, td->priority);
	if(LOCKING_ON(env)) {
		region = (DB_LOCKREGION *)env->lk_handle->reginfo.primary;
		if(region->tx_timeout != 0 && (ret = __lock_set_timeout(env, txn->locker, region->tx_timeout, DB_SET_TXN_TIMEOUT)) != 0)
			return ret;
		txn->lock_timeout = region->tx_timeout;
	}
	return ret;
}
/*
 * __txn_commit_pp --
 *	Interface routine to TXN->commit.
 */
static int __txn_commit_pp(DB_TXN*txn, uint32 flags)
{
	DB_THREAD_INFO * ip;
	int ret, t_ret;
	ENV * env = txn->mgrp->env;
	int rep_check = IS_ENV_REPLICATED(env) && txn->parent == NULL && IS_REAL_TXN(txn);
	ENV_ENTER(env, ip);
	ret = __txn_commit(txn, flags);
	if(rep_check && (t_ret = __op_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __txn_commit --
 *	Commit a transaction.
 */
int __txn_commit(DB_TXN*txn, uint32 flags)
{
	DBT list_dbt;
	DB_LOCKREQ request;
	DB_TXN * kid;
	REGENV * renv;
	REGINFO * infop;
	DB_LSN token_lsn;
	uint32 id;
	int ret, t_ret;
	ENV * env = txn->mgrp->env;
	TXN_DETAIL * td = (TXN_DETAIL *)txn->td;
	PERFMON2(env, txn, commit, txn->txnid, flags);
	DB_ASSERT(env, txn->xa_thr_status == TXN_XA_THREAD_NOTA || td->xa_ref == 1);
	/*
	 * A common mistake in Berkeley DB programs is to mis-handle deadlock
	 * return.  If the transaction deadlocked, they want abort, not commit.
	 */
	if(F_ISSET(txn, TXN_DEADLOCK)) {
		ret = __db_txn_deadlock_err(env, txn);
		goto err;
	}
	/* Close registered cursors before committing. */
	if((ret = __txn_close_cursors(txn)) != 0)
		goto err;
	if((ret = __txn_isvalid(txn, TXN_OP_COMMIT)) != 0)
		return ret;
	/*
	 * Check for master leases at the beginning.  If we are a master and
	 * cannot have valid leases now, we error and abort this txn.  There
	 * should always be a perm record in the log because the master updates
	 * the LSN history system database in rep_start() (with IGNORE_LEASE
	 * set).
	 */
	if(!txn->parent && IS_REP_MASTER(env) && IS_USING_LEASES(env) && !F_ISSET(txn, TXN_IGNORE_LEASE) && (ret = __rep_lease_check(env, 1)) != 0) {
		DB_ASSERT(env, ret != DB_NOTFOUND);
		goto err;
	}
	infop = env->reginfo;
	renv = (REGENV *)infop->primary;
	/*
	 * No mutex is needed as envid is read-only once it is set.
	 */
	id = renv->envid;
	/*
	 * We clear flags that are incorrect, ignoring any flag errors, and
	 * default to synchronous operations.  By definition, transaction
	 * handles are dead when we return, and this error should never
	 * happen, but we don't want to fail in the field 'cause the app is
	 * specifying the wrong flag for some reason.
	 */
	if(__db_fchk(env, "DB_TXN->commit", flags, DB_TXN_NOSYNC|DB_TXN_SYNC|DB_TXN_WRITE_NOSYNC) != 0)
		flags = DB_TXN_SYNC;
	if(__db_fcchk(env, "DB_TXN->commit", flags, DB_TXN_SYNC, DB_TXN_NOSYNC|DB_TXN_WRITE_NOSYNC) != 0)
		flags = DB_TXN_SYNC;
	if(LF_ISSET(DB_TXN_WRITE_NOSYNC)) {
		F_CLR(txn, TXN_SYNC_FLAGS);
		F_SET(txn, TXN_WRITE_NOSYNC);
	}
	if(LF_ISSET(DB_TXN_NOSYNC)) {
		F_CLR(txn, TXN_SYNC_FLAGS);
		F_SET(txn, TXN_NOSYNC);
	}
	if(LF_ISSET(DB_TXN_SYNC)) {
		F_CLR(txn, TXN_SYNC_FLAGS);
		F_SET(txn, TXN_SYNC);
	}
	DB_ASSERT(env, F_ISSET(txn, TXN_SYNC_FLAGS));

	/*
	 * Commit any unresolved children.  If anyone fails to commit,
	 * then try to abort the rest of the kids and then abort the parent.
	 * Abort should never fail; if it does, we bail out immediately.
	 */
	while((kid = TAILQ_FIRST(&txn->kids)) != NULL)
		if((ret = __txn_commit(kid, flags)) != 0)
			while((kid = TAILQ_FIRST(&txn->kids)) != NULL)
				if((t_ret = __txn_abort(kid)) != 0)
					return __env_panic(env, t_ret);
	/*
	 * If there are any log records, write a log record and sync the log,
	 * else do no log writes.  If the commit is for a child transaction,
	 * we do not need to commit the child synchronously since it may still
	 * abort (if its parent aborts), and otherwise its parent or ultimate
	 * ancestor will write synchronously.
	 */
	ZERO_LSN(token_lsn);
	if(DBENV_LOGGING(env) && (!IS_ZERO_LSN(td->last_lsn) || STAILQ_FIRST(&txn->logs) != NULL)) {
		if(txn->parent == NULL) {
			/*
			 * We are about to free all the read locks for this
			 * transaction below.  Some of those locks might be
			 * handle locks which should not be freed, because
			 * they will be freed when the handle is closed. Check
			 * the events and preprocess any trades now so we don't
			 * release the locks below.
			 */
			if((ret = __txn_doevents(env, txn, TXN_COMMIT, 1)) != 0)
				goto err;
			memzero(&request, sizeof(request));
			if(LOCKING_ON(env)) {
				request.op = DB_LOCK_PUT_READ;
				if(IS_REP_MASTER(env) && !IS_ZERO_LSN(td->last_lsn)) {
					memzero(&list_dbt, sizeof(list_dbt));
					request.obj = &list_dbt;
				}
				ret = __lock_vec(env, txn->locker, 0, &request, 1, 0);
			}
			if(ret == 0 && !IS_ZERO_LSN(td->last_lsn)) {
				ret = __txn_flush_fe_files(txn);
				if(ret == 0)
					ret = __txn_regop_log(env, txn, &td->visible_lsn, LOG_FLAGS(txn), TXN_COMMIT, (int32)time(NULL), id, request.obj);
				if(ret == 0)
					token_lsn = td->last_lsn = td->visible_lsn;
#ifdef DIAGNOSTIC
				if(ret == 0) {
					DB_LSN s_lsn;
					DB_ASSERT(env, __log_current_lsn_int(env, &s_lsn, NULL, NULL) == 0);
					DB_ASSERT(env, LOG_COMPARE(&td->visible_lsn, &s_lsn) <= 0);
					COMPQUIET(s_lsn.file, 0);
				}
#endif
			}
			if(request.obj && request.obj->data)
				__os_free(env, request.obj->data);
			if(ret != 0)
				goto err;
		}
		else {
			/* Log the commit in the parent! */
			if(!IS_ZERO_LSN(td->last_lsn) && (ret = __txn_child_log(env, txn->parent,
				    &((TXN_DETAIL *)txn->parent->td)->last_lsn, 0, txn->txnid, &td->last_lsn)) != 0) {
				goto err;
			}
			if(STAILQ_FIRST(&txn->logs) != NULL) {
				/*
				 * Put the child first so we back it out first.
				 * All records are undone in reverse order.
				 */
				STAILQ_CONCAT(&txn->logs, &txn->parent->logs);
				txn->parent->logs = txn->logs;
				STAILQ_INIT(&txn->logs);
			}
			F_SET(txn->parent, TXN_CHILDCOMMIT);
		}
	}
	if(txn->token_buffer && ret == 0 && DBENV_LOGGING(env))
		__txn_build_token(txn, &token_lsn);
	if(txn->txn_list) {
		__db_txnlist_end(env, (DB_TXNHEAD *)txn->txn_list);
		txn->txn_list = NULL;
	}
	if(ret != 0)
		goto err;
	/*
	 * Check for master leases at the end of only a normal commit.
	 * If we're a child, that is not a perm record.  If we are a
	 * master and cannot get valid leases now, something happened
	 * during the commit.  The only thing to do is panic.
	 */
	if(txn->parent == NULL && IS_REP_MASTER(env) && IS_USING_LEASES(env) && !F_ISSET(txn, TXN_IGNORE_LEASE) &&
	   (ret = __rep_lease_check(env, 1)) != 0)
		return __env_panic(env, ret);
	/*
	 * This is here rather than in __txn_end because __txn_end is
	 * called too late during abort.  So commit and abort each
	 * call it independently.
	 */
	__txn_reset_fe_watermarks(txn);
	/* This is OK because __txn_end can only fail with a panic. */
	return __txn_end(txn, 1);
err:
	/*
	 * If we are prepared, then we "must" be able to commit.  We panic here
	 * because even though the coordinator might be able to retry it is not
	 * clear it would know to do that.  Otherwise  we'll try to abort.  If
	 * that is successful, then we return whatever was in ret (that is, the
	 * reason we failed).  If the abort was unsuccessful, abort probably
	 * returned DB_RUNRECOVERY and we need to propagate that up.
	 */
	if(td->status == TXN_PREPARED)
		ret = __env_panic(env, ret);
	else if((t_ret = __txn_abort(txn)) != 0)
		ret = t_ret;
	return ret;
}
/*
 * __txn_close_cursors
 *	Close a transaction's registered cursors, all its cursors are
 *	guaranteed to be closed.
 */
static int __txn_close_cursors(DB_TXN * txn)
{
	int tret = 0;
	int ret = 0;
	DBC * dbc = NULL;
	if(txn == NULL)
		return 0;
	while((dbc = TAILQ_FIRST(&txn->my_cursors)) != NULL) {
		DB_ASSERT(dbc->env, txn == dbc->txn);

		/*
		 * Unregister the cursor from its transaction, regardless
		 * of return.
		 */
		TAILQ_REMOVE(&(txn->my_cursors), dbc, txn_cursors);
		dbc->txn_cursors.tqe_next = NULL;
		dbc->txn_cursors.tqe_prev = NULL;
		/* Removed from the active queue here. */
		if(F_ISSET(dbc, DBC_ACTIVE))
			ret = __dbc_close(dbc);
		dbc->txn = NULL;
		/* We have to close all cursors anyway, so continue on error. */
		if(ret != 0) {
			__db_err(dbc->env, ret, "__dbc_close");
			if(tret == 0)
				tret = ret;
		}
	}
	txn->my_cursors.tqh_first = NULL;
	txn->my_cursors.tqh_last = NULL;

	return tret;  /* Return the first error if any. */
}
/*
 * __txn_set_commit_token --
 *	Store a pointer to user's commit token buffer, for later use.
 */
static int __txn_set_commit_token(DB_TXN * txn, DB_TXN_TOKEN * tokenp)
{
	ENV * env = txn->mgrp->env;
	ENV_REQUIRES_CONFIG(env, env->lg_handle, "DB_TXN->set_commit_token", DB_INIT_LOG);
	if(txn->parent) {
		__db_errx(env, DB_STR("4526", "commit token unavailable for nested txn"));
		return EINVAL;
	}
	if(IS_REP_CLIENT(env)) {
		__db_errx(env, DB_STR("4527", "may not be called on a replication client"));
		return EINVAL;
	}
	txn->token_buffer = tokenp;

#ifdef DIAGNOSTIC
	/*
	 * Applications may rely on the contents of the token buffer becoming
	 * valid only after a successful commit().  So it is not strictly
	 * necessary to initialize the buffer here.  But in case they get
	 * confused we initialize it here to a recognizably invalid value.
	 */
	memzero(tokenp, DB_TXN_TOKEN_SIZE);
#endif
	return 0;
}
/*
 * __txn_build_token --
 *	Stash a token describing the committing transaction into the buffer
 * previously designated by the user.  Called only in the case where the user
 * has indeed supplied a buffer address.
 */
static void __txn_build_token(DB_TXN*txn, DB_LSN * lsnp)
{
	uint8 * bp = txn->token_buffer->buf;
	ENV * env = txn->mgrp->env;
	REGENV * renv = (REGENV *)env->reginfo->primary;
	// Marshal the information into external form
	uint32 version = REP_COMMIT_TOKEN_FMT_VERSION;
	uint32 gen = REP_ON(env) ? env->rep_handle->region->gen : 0;
	DB_HTONL_COPYOUT(env, bp, version);
	DB_HTONL_COPYOUT(env, bp, gen);
	DB_HTONL_COPYOUT(env, bp, renv->envid);
	DB_HTONL_COPYOUT(env, bp, lsnp->file);
	DB_HTONL_COPYOUT(env, bp, lsnp->Offset_);
}
/*
 * __txn_abort_pp --
 *	Interface routine to TXN->abort.
 */
static int __txn_abort_pp(DB_TXN*txn)
{
	DB_THREAD_INFO * ip;
	int ret, t_ret;
	ENV * env = txn->mgrp->env;
	int rep_check = IS_ENV_REPLICATED(env) && txn->parent == NULL && IS_REAL_TXN(txn);
	ENV_ENTER(env, ip);
	ret = __txn_abort(txn);
	if(rep_check && (t_ret = __op_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;
	ENV_LEAVE(env, ip);
	return ret;
}
//
// Abort a transaction.
//
int __txn_abort(DB_TXN * txn)
{
	int ret = 0;
	if(txn) {
		DB_LOCKREQ request;
		DB_TXN * kid;
		REGENV * renv;
		REGINFO * infop;
		uint32 id;
		ENV * env = txn->mgrp->env;
		TXN_DETAIL * td = (TXN_DETAIL *)txn->td;
		// 
		// Do not abort an XA transaction if another process is still using
		// it, however make sure that it is aborted when the last process tries to abort it.
		// 
		if(txn->xa_thr_status != TXN_XA_THREAD_NOTA &&  td->xa_ref > 1) {
			td->status = TXN_NEED_ABORT;
			return 0;
		}
		PERFMON1(env, txn, abort, txn->txnid);
		/*
		 * Close registered cursors before the abort. Even if the call fails,
		 * all cursors are closed.
		 */
		if((ret = __txn_close_cursors(txn)) != 0)
			return __env_panic(env, ret);
		/* Ensure that abort always fails fatally. */
		if((ret = __txn_isvalid(txn, TXN_OP_ABORT)) != 0)
			return __env_panic(env, ret);
		/*
		 * Clear the watermarks now.  Can't do this in __txn_end because
		 * __db_refresh, called from undo, will free the DB_MPOOLFILEs.
		 */
		__txn_reset_fe_watermarks(txn);
		/*
		 * Try to abort any unresolved children.
		 *
		 * Abort either succeeds or panics the region.  As soon as we
		 * see any failure, we just get out of here and return the panic
		 * up.
		 */
		while((kid = TAILQ_FIRST(&txn->kids)) != NULL)
			if((ret = __txn_abort(kid)) != 0)
				return ret;
		infop = env->reginfo;
		renv = (REGENV *)infop->primary;
		/*
		 * No mutex is needed as envid is read-only once it is set.
		 */
		id = renv->envid;
		/*
		 * Fast path -- no need to do anything fancy if there were no
		 * modifications (e.g., log records) for this transaction.
		 * We still call txn_undo to cleanup the txn_list from our
		 * children.
		 */
		if(IS_ZERO_LSN(td->last_lsn) && STAILQ_FIRST(&txn->logs) == NULL) {
			if(txn->txn_list == NULL)
				goto done;
			else
				goto undo;
		}
		if(LOCKING_ON(env)) {
			/* Allocate a locker for this restored txn if necessary. */
			if(txn->locker == NULL && (ret = __lock_getlocker(env->lk_handle, txn->txnid, 1, &txn->locker)) != 0)
				return __env_panic(env, ret);
			/*
			 * We are about to free all the read locks for this transaction
			 * below.  Some of those locks might be handle locks which
			 * should not be freed, because they will be freed when the
			 * handle is closed.  Check the events and preprocess any
			 * trades now so that we don't release the locks below.
			 */
			if((ret = __txn_doevents(env, txn, TXN_ABORT, 1)) != 0)
				return __env_panic(env, ret);
			/* Turn off timeouts. */
			if((ret = __lock_set_timeout(env, txn->locker, 0, DB_SET_TXN_TIMEOUT)) != 0)
				return __env_panic(env, ret);
			if((ret = __lock_set_timeout(env, txn->locker, 0, DB_SET_LOCK_TIMEOUT)) != 0)
				return __env_panic(env, ret);
			request.op = DB_LOCK_UPGRADE_WRITE;
			request.obj = NULL;
			if((ret = __lock_vec(env, txn->locker, 0, &request, 1, NULL)) != 0)
				return __env_panic(env, ret);
		}
	undo:
		if((ret = __txn_undo(txn)) != 0)
			return __env_panic(env, ret);
		/*
		 * Normally, we do not need to log aborts.  However, if we
		 * are a distributed transaction (i.e., we have a prepare),
		 * then we log the abort so we know that this transaction was actually completed.
		 */
	done:
		if(DBENV_LOGGING(env) && td->status == TXN_PREPARED && (ret = __txn_regop_log(env, txn, &td->last_lsn,
			LOG_FLAGS(txn), TXN_ABORT, (int32)time(NULL), id, NULL)) != 0)
			ret = __env_panic(env, ret);
		else {
			// __txn_end always panics if it errors, so pass the return along
			ret = __txn_end(txn, 0);
		}
	}
	return ret;
}
/*
 * __txn_discard --
 *	Interface routine to TXN->discard.
 */
static int __txn_discard(DB_TXN*txn, uint32 flags)
{
	DB_THREAD_INFO * ip;
	int ret, t_ret;
	ENV * env = txn->mgrp->env;
	int rep_check = IS_ENV_REPLICATED(env) && txn->parent == NULL && IS_REAL_TXN(txn);
	ENV_ENTER(env, ip);
	ret = __txn_discard_int(txn, flags);
	if(rep_check && (t_ret = __op_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __txn_discard --
 *	Free the per-process resources associated with this txn handle.
 */
int __txn_discard_int(DB_TXN*txn, uint32 flags)
{
	DB_TXNMGR * mgr;
	ENV * env;
	int ret;
	COMPQUIET(flags, 0);
	mgr = txn->mgrp;
	env = mgr->env;
	/* Close registered cursors. */
	if((ret = __txn_close_cursors(txn)) != 0)
		return ret;
	if((ret = __txn_isvalid(txn, TXN_OP_DISCARD)) != 0)
		return ret;
	/* Should be no children. */
	DB_ASSERT(env, TAILQ_FIRST(&txn->kids) == NULL);

	/* Free the space. */
	MUTEX_LOCK(env, mgr->mutex);
	mgr->n_discards++;
	if(F_ISSET(txn, TXN_MALLOC)) {
		TAILQ_REMOVE(&mgr->txn_chain, txn, links);
	}
	MUTEX_UNLOCK(env, mgr->mutex);
	if(F_ISSET(txn, TXN_MALLOC) && txn->xa_thr_status != TXN_XA_THREAD_ASSOCIATED)
		__os_free(env, txn);
	return 0;
}
/*
 * __txn_prepare --
 *	Flush the log so a future commit is guaranteed to succeed.
 */
int __txn_prepare(DB_TXN*txn, uint8 * gid)
{
	DBT list_dbt, gid_dbt;
	DB_LOCKREQ request;
	DB_THREAD_INFO * ip;
	DB_TXN * kid;
	uint32 lflags;
	int ret;
	ENV * env = txn->mgrp->env;
	TXN_DETAIL * td = (TXN_DETAIL *)txn->td;
	PERFMON2(env, txn, prepare, txn->txnid, gid);
	DB_ASSERT(env, txn->xa_thr_status == TXN_XA_THREAD_NOTA || td->xa_ref == 1);
	ENV_ENTER(env, ip);
	/* Close registered cursors. */
	if((ret = __txn_close_cursors(txn)) != 0)
		goto err;
	if((ret = __txn_isvalid(txn, TXN_OP_PREPARE)) != 0)
		goto err;
	if(F_ISSET(txn, TXN_DEADLOCK)) {
		ret = __db_txn_deadlock_err(env, txn);
		goto err;
	}
	/* Commit any unresolved children. */
	while((kid = TAILQ_FIRST(&txn->kids)) != NULL)
		if((ret = __txn_commit(kid, DB_TXN_NOSYNC)) != 0)
			goto err;
	/* We must set the global transaction ID here.  */
	memcpy(td->gid, gid, DB_GID_SIZE);
	if((ret = __txn_doevents(env, txn, TXN_PREPARE, 1)) != 0)
		goto err;
	memzero(&request, sizeof(request));
	if(LOCKING_ON(env)) {
		request.op = DB_LOCK_PUT_READ;
		if(!IS_ZERO_LSN(td->last_lsn)) {
			memzero(&list_dbt, sizeof(list_dbt));
			request.obj = &list_dbt;
		}
		if((ret = __lock_vec(env, txn->locker, 0, &request, 1, NULL)) != 0)
			goto err;
	}
	if(DBENV_LOGGING(env)) {
		memzero(&gid_dbt, sizeof(gid));
		gid_dbt.data = gid;
		gid_dbt.size = DB_GID_SIZE;
		lflags = DB_LOG_COMMIT|DB_FLUSH;
		if((ret = __txn_prepare_log(env, txn, &td->last_lsn, lflags, TXN_PREPARE, &gid_dbt, &td->begin_lsn, request.obj)) != 0)
			__db_err(env, ret, DB_STR("4528", "DB_TXN->prepare: log_write failed"));
		if(request.obj && request.obj->data)
			__os_free(env, request.obj->data);
		if(ret != 0)
			goto err;
	}
	MUTEX_LOCK(env, txn->mgrp->mutex);
	td->status = TXN_PREPARED;
	MUTEX_UNLOCK(env, txn->mgrp->mutex);
err:
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __txn_id --
 *	Return the transaction ID.
 */
uint32 __txn_id(DB_TXN*txn)
{
	return txn->txnid;
}
/*
 * __txn_get_name --
 *	Get a descriptive string from a transaction.
 */
int __txn_get_name(DB_TXN*txn, const char ** namep)
{
	*namep = txn->name;
	return 0;
}
/*
 * __txn_set_name --
 *	Set a descriptive string for a transaction.
 */
int __txn_set_name(DB_TXN*txn, const char * name)
{
	DB_THREAD_INFO * ip;
	int ret;
	char * p;
	DB_TXNMGR * mgr = txn->mgrp;
	ENV * env = mgr->env;
	TXN_DETAIL * td = (TXN_DETAIL *)txn->td;
	size_t len = sstrlen(name)+1;
	if((ret = __os_realloc(env, len, &txn->name)) != 0)
		return ret;
	memcpy(txn->name, name, len);
	ENV_ENTER(env, ip);
	TXN_SYSTEM_LOCK(env);
	if(td->name != INVALID_ROFF) {
		__env_alloc_free(&mgr->reginfo, R_ADDR(&mgr->reginfo, td->name));
		td->name = INVALID_ROFF;
	}
	if((ret = __env_alloc(&mgr->reginfo, len, &p)) != 0) {
		TXN_SYSTEM_UNLOCK(env);
		__db_errx(env, DB_STR("4529", "Unable to allocate memory for transaction name"));
		__os_free(env, txn->name);
		txn->name = NULL;
		ENV_LEAVE(env, ip);
		return ret;
	}
	TXN_SYSTEM_UNLOCK(env);
	td->name = R_OFFSET(&mgr->reginfo, p);
	memcpy(p, name, len);
#ifdef DIAGNOSTIC
	/*
	 * If DIAGNOSTIC is set, map the name into the log so users can track
	 * operations through the log.
	 */
	if(DBENV_LOGGING(env))
		__log_printf(env, txn, "transaction %#lx named %s", (ulong)txn->txnid, name);
#endif
	ENV_LEAVE(env, ip);
	return 0;
}
/*
 * __txn_get_priority --
 *	Get a transaction's priority level
 */
int __txn_get_priority(DB_TXN*txn, uint32 * priorityp)
{
	if(txn->locker == NULL)
		return EINVAL;
	else {
		*priorityp = txn->locker->priority;
		return 0;
	}
}
/*
 * __txn_set_priority --
 *	Assign a transaction a priority level
 */
int __txn_set_priority(DB_TXN*txn, uint32 priority)
{
	if(txn->locker == NULL)
		return EINVAL;
	else {
		txn->locker->priority = priority;
		((TXN_DETAIL *)txn->td)->priority = priority;
		return 0;
	}
}
/*
 * __txn_set_timeout --
 *	ENV->set_txn_timeout.
 */
int __txn_set_timeout(DB_TXN*txn, db_timeout_t timeout, uint32 op)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = txn->mgrp->env;
	if(op != DB_SET_TXN_TIMEOUT && op != DB_SET_LOCK_TIMEOUT)
		return __db_ferr(env, "DB_TXN->set_timeout", 0);
	ENV_ENTER(env, ip);
	ret = __lock_set_timeout( env, txn->locker, timeout, op);
	ENV_LEAVE(txn->mgrp->env, ip);
	return ret;
}
/*
 * __txn_isvalid --
 *	Return 0 if the DB_TXN is reasonable, otherwise panic.
 */
static int __txn_isvalid(const DB_TXN*txn, txnop_t op)
{
	TXN_DETAIL * td;
	DB_TXNMGR * mgr = txn->mgrp;
	ENV * env = mgr->env;
	DB_TXNREGION * region = (DB_TXNREGION *)mgr->reginfo.primary;
	/* Check for recovery. */
	if(!F_ISSET(txn, TXN_COMPENSATE) && F_ISSET(region, TXN_IN_RECOVERY)) {
		__db_errx(env, DB_STR("4530", "operation not permitted during recovery"));
		goto err;
	}
	/* Check for live cursors. */
	if(txn->cursors != 0) {
		__db_errx(env, DB_STR("4531", "transaction has active cursors"));
		goto err;
	}
	/* Check transaction's state. */
	td = (TXN_DETAIL *)txn->td;

	/* Handle any operation specific checks. */
	switch(op) {
	    case TXN_OP_DISCARD:
		/*
		 * Since we're just tossing the per-process space; there are
		 * a lot of problems with the transaction that we can tolerate.
		 */
		/* Transaction is already been reused. */
		if(txn->txnid != td->txnid)
			return 0;
		/*
		 * What we've got had better be either a prepared or
		 * restored transaction.
		 */
		if(td->status != TXN_PREPARED && !F_ISSET(td, TXN_DTL_RESTORED)) {
			__db_errx(env, DB_STR("4532", "not a restored transaction"));
			return __env_panic(env, EINVAL);
		}
		return 0;
	    case TXN_OP_PREPARE:
		if(txn->parent) {
			/*
			 * This is not fatal, because you could imagine an
			 * application that simply prepares everybody because
			 * it doesn't distinguish between children and parents.
			 * I'm not arguing this is good, but I could imagine
			 * someone doing it.
			 */
			__db_errx(env, DB_STR("4533", "Prepare disallowed on child transactions"));
			return EINVAL;
		}
		break;
	    case TXN_OP_ABORT:
	    case TXN_OP_COMMIT:
	    default:
		break;
	}
	switch(td->status) {
	    case TXN_PREPARED:
		if(op == TXN_OP_PREPARE) {
			__db_errx(env, DB_STR("4534", "transaction already prepared"));
			/*
			 * Txn_prepare doesn't blow away the user handle, so
			 * in this case, give the user the opportunity to
			 * abort or commit.
			 */
			return EINVAL;
		}
		break;
	    case TXN_RUNNING:
	    case TXN_NEED_ABORT:
		break;
	    case TXN_ABORTED:
	    case TXN_COMMITTED:
	    default:
		__db_errx(env, DB_STR_A("4535", "transaction already %s", "%s"),
			td->status == TXN_COMMITTED ? DB_STR_P("committed") : DB_STR_P("aborted"));
		goto err;
	}
	return 0;

err:    
	/*
	 * If there's a serious problem with the transaction, panic.  TXN
	 * handles are dead by definition when we return, and if you use
	 * a cursor you forgot to close, we have no idea what will happen.
	 */
	return __env_panic(env, EINVAL);
}
/*
 * __txn_end --
 *	Internal transaction end routine.
 */
static int __txn_end(DB_TXN*txn, int is_commit)
{
	DB_LOCKREQ request;
	DB_TXNLOGREC * lr;
	TXN_DETAIL * ptd, * td;
	db_mutex_t mvcc_mtx;
	int ret;
	DB_TXNMGR * mgr = txn->mgrp;
	ENV * env = mgr->env;
	DB_TXNREGION * region = (DB_TXNREGION *)mgr->reginfo.primary;
	int do_closefiles = 0;
	/* Process commit events. */
	if((ret = __txn_doevents(env, txn, is_commit ? TXN_COMMIT : TXN_ABORT, 0)) != 0)
		return __env_panic(env, ret);
	/* End the transaction. */
	td = (TXN_DETAIL *)txn->td;
	if(td->nlog_dbs != 0 && (ret = __txn_dref_fname(env, txn)) != 0 && ret != EIO)
		return __env_panic(env, ret);
	if(td->mvcc_ref != 0 && IS_MAX_LSN(td->visible_lsn)) {
		/*
		 * Some pages were dirtied but nothing was logged.  This can
		 * happen easily if we are aborting, but there are also cases
		 * in the compact code where pages are dirtied unconditionally
		 * and then we find out that there is no work to do.
		 *
		 * We need to make sure that the versions become visible to
		 * future transactions.  We need to set visible_lsn before
		 * setting td->status to ensure safe reads of visible_lsn in
		 * __memp_fget.
		 */
		if((ret = __log_current_lsn_int(env, &td->visible_lsn, NULL, NULL)) != 0)
			return __env_panic(env, ret);
	}
	/*
	 * Release the locks.
	 *
	 * __txn_end cannot return an simple error, we MUST return
	 * success/failure from commit or abort, ignoring any internal
	 * errors.  So, we panic if something goes wrong.  We can't
	 * deadlock here because we're not acquiring any new locks,
	 * so DB_LOCK_DEADLOCK is just as fatal as any other error.
	 */
	if(LOCKING_ON(env)) {
		/* Allocate a locker for this restored txn if necessary. */
		if(txn->locker == NULL && (ret = __lock_getlocker(env->lk_handle, txn->txnid, 1, &txn->locker)) != 0)
			return __env_panic(env, ret);
		request.op = txn->parent == NULL || is_commit == 0 ? DB_LOCK_PUT_ALL : DB_LOCK_INHERIT;
		request.obj = NULL;
		if((ret = __lock_vec(env, txn->locker, 0, &request, 1, NULL)) != 0)
			return __env_panic(env, ret);
	}
	TXN_SYSTEM_LOCK(env);
	td->status = is_commit ? TXN_COMMITTED : TXN_ABORTED;
	SH_TAILQ_REMOVE(&region->active_txn, td, links, __txn_detail);
	region->curtxns--;
	if(F_ISSET(td, TXN_DTL_RESTORED)) {
		region->stat.st_nrestores--;
		do_closefiles = region->stat.st_nrestores == 0;
	}
	if(td->name != INVALID_ROFF) {
		__env_alloc_free(&mgr->reginfo, R_ADDR(&mgr->reginfo, td->name));
		td->name = INVALID_ROFF;
	}
	if(td->nlog_slots != TXN_NSLOTS)
		__env_alloc_free(&mgr->reginfo, R_ADDR(&mgr->reginfo, td->log_dbs));
	if(txn->parent) {
		ptd = (TXN_DETAIL *)txn->parent->td;
		SH_TAILQ_REMOVE(&ptd->kids, td, klinks, __txn_detail);
	}
	else if((mvcc_mtx = td->mvcc_mtx) != MUTEX_INVALID) {
		MUTEX_LOCK(env, mvcc_mtx);
		if(td->mvcc_ref != 0) {
			SH_TAILQ_INSERT_HEAD(&region->mvcc_txn, td, links, __txn_detail);
			/*
			 * The transaction has been added to the list of
			 * committed snapshot transactions with active pages.
			 * It needs to be freed when the last page is evicted.
			 */
			F_SET(td, TXN_DTL_SNAPSHOT);
#ifdef HAVE_STATISTICS
			STAT_INC(env, txn, nsnapshot, region->stat.st_nsnapshot, txn->txnid);
			if(region->stat.st_nsnapshot > region->stat.st_maxnsnapshot)
				STAT_SET(env, txn, maxnsnapshot, region->stat.st_maxnsnapshot, region->stat.st_nsnapshot, txn->txnid);
#endif
			td = NULL;
		}
		MUTEX_UNLOCK(env, mvcc_mtx);
		if(td)
			if((ret = __mutex_free(env, &td->mvcc_mtx)) != 0)
				return __env_panic(env, ret);
	}
	if(td)
		__env_alloc_free(&mgr->reginfo, td);
#ifdef HAVE_STATISTICS
	if(is_commit)
		STAT_INC(env, txn, ncommits, region->stat.st_ncommits, txn->txnid);
	else
		STAT_INC(env, txn, naborts, region->stat.st_naborts, txn->txnid);
	STAT_DEC(env, txn, nactive, region->stat.st_nactive, txn->txnid);
#endif
	/* Increment bulk transaction counter while holding transaction lock. */
	if(F_ISSET(txn, TXN_BULK))
		((DB_TXNREGION *)env->tx_handle->reginfo.primary)->n_bulk_txn--;
	TXN_SYSTEM_UNLOCK(env);
	//
	// The transaction cannot get more locks, remove its locker info, if any.
	//
	if(LOCKING_ON(env) && (ret = __lock_freelocker(env->lk_handle, txn->locker)) != 0)
		return __env_panic(env, ret);
	if(txn->parent)
		TAILQ_REMOVE(&txn->parent->kids, txn, klinks);
	/* Free the space. */
	while((lr = STAILQ_FIRST(&txn->logs)) != NULL) {
		STAILQ_REMOVE(&txn->logs, lr, __txn_logrec, links);
		__os_free(env, lr);
	}
	if(txn->name) {
		__os_free(env, txn->name);
		txn->name = NULL;
	}
	/*
	 * Free the transaction structure if we allocated it and if we are
	 * not in an XA transaction that will be freed when we exit the XA
	 * wrapper routines.
	 */
	if(F_ISSET(txn, TXN_MALLOC) && txn->xa_thr_status != TXN_XA_THREAD_ASSOCIATED) {
		MUTEX_LOCK(env, mgr->mutex);
		TAILQ_REMOVE(&mgr->txn_chain, txn, links);
		MUTEX_UNLOCK(env, mgr->mutex);
		__os_free(env, txn);
	}
	if(do_closefiles) {
		/*
		 * Otherwise, we have resolved the last outstanding prepared
		 * txn and need to invalidate the fileids that were left
		 * open for those txns and then close them.
		 */
		__dbreg_invalidate_files(env, 1);
		__dbreg_close_files(env, 1);
		if(IS_REP_MASTER(env))
			F_CLR(env->rep_handle, DBREP_OPENFILES);
		F_CLR(env->lg_handle, DBLOG_OPENFILES);
		mgr->n_discards = 0;
		__txn_checkpoint(env, 0, 0, DB_CKP_INTERNAL|DB_FORCE);
	}
	return 0;
}

static int __txn_dispatch_undo(ENV*env, DB_TXN * txn, DBT * rdbt, DB_LSN * key_lsn, DB_TXNHEAD * txnlist)
{
	int ret;
	txnlist->td = txn->td;
	ret = __db_dispatch(env, &env->recover_dtab, rdbt, key_lsn, DB_TXN_ABORT, txnlist);
	if(ret == DB_SURPRISE_KID) {
		F_SET(txn, TXN_CHILDCOMMIT);
		ret = 0;
	}
	if(ret == 0 && F_ISSET(txn, TXN_CHILDCOMMIT) && IS_ZERO_LSN(*key_lsn))
		ret = __db_txnlist_lsnget(env, txnlist, key_lsn, 0);
	return ret;
}
/*
 * __txn_undo --
 *	Undo the transaction with id txnid.
 */
static int __txn_undo(DB_TXN*txn)
{
	DBT rdbt;
	DB_LSN key_lsn;
	DB_TXN * ptxn;
	DB_TXNLOGREC * lr;
	int t_ret;
	DB_TXNMGR * mgr = txn->mgrp;
	ENV * env = mgr->env;
	DB_LOGC * logc = NULL;
	DB_TXNHEAD * txnlist = NULL;
	int ret = 0;
	if(!LOGGING_ON(env))
		return 0;
	/*
	 * This is the simplest way to code this, but if the mallocs during
	 * recovery turn out to be a performance issue, we can do the
	 * allocation here and use DB_DBT_USERMEM.
	 */
	memzero(&rdbt, sizeof(rdbt));
	/*
	 * Allocate a txnlist for children and aborted page allocs.
	 * We need to associate the list with the maximal parent
	 * so that aborted pages are recovered when that transaction
	 * is committed or aborted.
	 */
	for(ptxn = txn->parent; ptxn && ptxn->parent; )
		ptxn = ptxn->parent;
	if(ptxn && ptxn->txn_list)
		txnlist = (DB_TXNHEAD *)ptxn->txn_list;
	else if(txn->txn_list)
		txnlist = (DB_TXNHEAD *)txn->txn_list;
	else if((ret = __db_txnlist_init(env, txn->thread_info, 0, 0, NULL, &txnlist)) != 0)
		return ret;
	else if(ptxn)
		ptxn->txn_list = txnlist;
	// 
	// Take log records from the linked list stored in the transaction, then from the log.
	// 	
	STAILQ_FOREACH(lr, &txn->logs, links) {
		rdbt.data = lr->data;
		rdbt.size = 0;
		LSN_NOT_LOGGED(key_lsn);
		ret = __txn_dispatch_undo(env, txn, &rdbt, &key_lsn, txnlist);
		if(ret != 0) {
			__db_err(env, ret, DB_STR("4536", "DB_TXN->abort: in-memory log undo failed"));
			goto err;
		}
	}
	key_lsn = ((TXN_DETAIL *)txn->td)->last_lsn;
	if(!IS_ZERO_LSN(key_lsn) && (ret = __log_cursor(env, &logc)) != 0)
		goto err;
	while(!IS_ZERO_LSN(key_lsn)) {
		//
		// The dispatch routine returns the lsn of the record
		// before the current one in the key_lsn argument.
		// 
		if((ret = __logc_get(logc, &key_lsn, &rdbt, DB_SET)) == 0) {
			ret = __txn_dispatch_undo(env, txn, &rdbt, &key_lsn, txnlist);
		}
		if(ret != 0) {
			__db_err(env, ret, DB_STR_A("4537", "DB_TXN->abort: log undo failed for LSN: %lu %lu", "%lu %lu"), (ulong)key_lsn.file, (ulong)key_lsn.Offset_);
			goto err;
		}
	}
err:
	if(logc && (t_ret = __logc_close(logc)) != 0 && ret == 0)
		ret = t_ret;
	if(ptxn == NULL)
		__db_txnlist_end(env, txnlist);
	return ret;
}
/*
 * __txn_activekids --
 *	Return if this transaction has any active children.
 */
int __txn_activekids(ENV*env, uint32 rectype, DB_TXN * txn)
{
	/*
	 * On a child commit, we know that there are children (i.e., the
	 * committing child at the least.  In that case, skip this check.
	 */
	if(F_ISSET(txn, TXN_COMPENSATE) || rectype == DB___txn_child)
		return 0;
	if(TAILQ_FIRST(&txn->kids) != NULL) {
		__db_errx(env, DB_STR("4538", "Child transaction is active"));
		return EPERM;
	}
	return 0;
}
/*
 * __txn_force_abort --
 *	Force an abort record into the log if the commit record
 *	failed to get to disk.
 */
int __txn_force_abort(ENV*env, uint8 * buffer)
{
	HDR hdr;
	uint32 offset, opcode, sum_len;
	uint8 * bp, * key;
	size_t rec_len;
	int ret;
	DB_CIPHER * db_cipher = env->crypto_handle;
	/*
	 * This routine depends on the layout of HDR and the __txn_regop
	 * record in txn.src.  We are passed the beginning of the commit
	 * record in the log buffer and overwrite the commit with an abort
	 * and recalculate the checksum.
	 */
	size_t hdrsize = CRYPTO_ON(env) ? HDR_CRYPTO_SZ : HDR_NORMAL_SZ;
	HDR * hdrp = (HDR *)buffer;
	memcpy(&hdr.prev, buffer+SSZ(HDR, prev), sizeof(hdr.prev));
	memcpy(&hdr.len, buffer+SSZ(HDR, len), sizeof(hdr.len));
	if(LOG_SWAPPED(env))
		__log_hdrswap(&hdr, CRYPTO_ON(env));
	rec_len = hdr.len-hdrsize;
	offset = sizeof(uint32)+sizeof(uint32)+sizeof(DB_LSN);
	if(CRYPTO_ON(env)) {
		key = db_cipher->mac_key;
		sum_len = DB_MAC_KEY;
		if((ret = db_cipher->decrypt(env, db_cipher->data, &hdrp->iv[0], buffer+hdrsize, rec_len)) != 0)
			return __env_panic(env, ret);
	}
	else {
		key = NULL;
		sum_len = sizeof(uint32);
	}
	bp = buffer+hdrsize+offset;
	opcode = TXN_ABORT;
	LOGCOPY_32(env, bp, &opcode);
	if(CRYPTO_ON(env) && (ret = db_cipher->encrypt(env, db_cipher->data, &hdrp->iv[0], buffer+hdrsize, rec_len)) != 0)
		return __env_panic(env, ret);
#ifdef HAVE_LOG_CHECKSUM
	__db_chksum(&hdr, buffer+hdrsize, rec_len, key, 0);
	if(LOG_SWAPPED(env))
		__log_hdrswap(&hdr, CRYPTO_ON(env));
	memcpy(buffer+SSZA(HDR, chksum), hdr.chksum, sum_len);
#endif
	return 0;
}
/*
 * __txn_preclose --
 *	Before we can close an environment, we need to check if we were in the
 *	middle of taking care of restored transactions.  If so, close the files we opened.
 */
int __txn_preclose(ENV*env)
{
	int do_closefiles, ret;
	DB_TXNMGR * mgr = env->tx_handle;
	DB_TXNREGION * region = (DB_TXNREGION *)mgr->reginfo.primary;
	do_closefiles = 0;
	TXN_SYSTEM_LOCK(env);
	if(region && region->stat.st_nrestores <= mgr->n_discards && mgr->n_discards != 0)
		do_closefiles = 1;
	TXN_SYSTEM_UNLOCK(env);
	if(do_closefiles) {
		//
		// Set the DBLOG_RECOVER flag while closing these files so they
		// do not create additional log records that will confuse future recoveries.
		//
		F_SET(env->lg_handle, DBLOG_RECOVER);
		ret = __dbreg_close_files(env, 0);
		F_CLR(env->lg_handle, DBLOG_RECOVER);
	}
	else
		ret = 0;
	return ret;
}
/*
 * __txn_reset --
 *	Reset the last txnid to its minimum value, and log the reset.
 */
int __txn_reset(ENV*env)
{
	DB_LSN scrap;
	DB_TXNREGION * region = (DB_TXNREGION *)env->tx_handle->reginfo.primary;
	region->last_txnid = TXN_MINIMUM;
	DB_ASSERT(env, LOGGING_ON(env));
	return __txn_recycle_log(env, NULL, &scrap, 0, TXN_MINIMUM, TXN_MAXIMUM);
}
/*
 * txn_set_txn_lsnp --
 *	Set the pointer to the begin_lsn field if that field is zero.
 *	Set the pointer to the last_lsn field.
 */
static void __txn_set_txn_lsnp(DB_TXN*txn, DB_LSN ** blsnp, DB_LSN ** llsnp)
{
	TXN_DETAIL * td = (TXN_DETAIL *)txn->td;
	*llsnp = &td->last_lsn;
	while(txn->parent)
		txn = txn->parent;
	td = (TXN_DETAIL *)txn->td;
	if(IS_ZERO_LSN(td->begin_lsn))
		*blsnp = &td->begin_lsn;
}

int __txn_applied_pp(DB_ENV * dbenv, DB_TXN_TOKEN * token, db_timeout_t timeout, uint32 flags)
{
	DB_THREAD_INFO * ip;
	DB_COMMIT_INFO commit_info;
	uint8 * bp;
	int ret;
	ENV * env = dbenv->env;
	if(flags != 0)
		return __db_ferr(env, "DB_ENV->txn_applied", 0);
	/* Unmarshal the token from its stored form. */
	bp = token->buf;
	DB_NTOHL_COPYIN(env, commit_info.version, bp);
	DB_ASSERT(env, commit_info.version == REP_COMMIT_TOKEN_FMT_VERSION);
	DB_NTOHL_COPYIN(env, commit_info.gen, bp);
	DB_NTOHL_COPYIN(env, commit_info.envid, bp);
	DB_NTOHL_COPYIN(env, commit_info.lsn.file, bp);
	DB_NTOHL_COPYIN(env, commit_info.lsn.Offset_, bp);
	/*
	 * Check for a token representing a transaction that committed without
	 * any log records having been written.  Ideally an application should
	 * be smart enough to avoid trying to use a token from such an "empty"
	 * transaction.  But in some cases it might be difficult for them to
	 * keep track, so we don't really forbid it.
	 */
	if(IS_ZERO_LSN(commit_info.lsn))
		return DB_KEYEMPTY;
	ENV_REQUIRES_CONFIG(env, env->lg_handle, "DB_ENV->txn_applied", DB_INIT_LOG);
	ENV_ENTER(env, ip);
	ret = __txn_applied(env, ip, &commit_info, timeout);
	ENV_LEAVE(env, ip);
	return ret;
}

static int __txn_applied(ENV * env, DB_THREAD_INFO * ip, DB_COMMIT_INFO * commit_info, db_timeout_t timeout)
{
	LOG * lp;
	DB_LSN lsn;
	REGENV * renv;
	/*
	 * The lockout protection scope between __op_handle_enter and
	 * __env_db_rep_exit is handled within __rep_txn_applied, and is not
	 * needed here since the rest of this function only runs in a
	 * non-replication env.
	 */
	if(REP_ON(env))
		return __rep_txn_applied(env, ip, commit_info, timeout);
	if(commit_info->gen != 0) {
		__db_errx(env, DB_STR("4539", "replication commit token in non-replication env"));
		return EINVAL;
	}
	lp = (LOG *)env->lg_handle->reginfo.primary;
	LOG_SYSTEM_LOCK(env);
	lsn = lp->lsn;
	LOG_SYSTEM_UNLOCK(env);
	renv = (REGENV *)env->reginfo->primary;
	if(renv->envid == commit_info->envid && LOG_COMPARE(&commit_info->lsn, &lsn) <= 0)
		return 0;
	return DB_NOTFOUND;
}
/*
 * __txn_checkpoint_pp --
 *	ENV->txn_checkpoint pre/post processing.
 */
int __txn_checkpoint_pp(DB_ENV * dbenv, uint32 kbytes, uint32 minutes, uint32 flags)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->tx_handle, "txn_checkpoint", DB_INIT_TXN);
	/*
	 * On a replication client, all transactions are read-only; therefore,
	 * a checkpoint is a null-op.
	 *
	 * We permit txn_checkpoint, instead of just rendering it illegal,
	 * so that an application can just let a checkpoint thread continue
	 * to operate as it gets promoted or demoted between being a
	 * master and a client.
	 */
	if(IS_REP_CLIENT(env))
		return 0;
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__txn_checkpoint(env, kbytes, minutes, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return ret;
}

int __txn_checkpoint(ENV * env, uint32 kbytes, uint32 minutes, uint32 flags)
{
	DB_LOG * dblp;
	DB_LSN ckp_lsn, last_ckp;
	DB_TXNMGR * mgr;
	DB_TXNREGION * region;
	LOG * lp;
	REGENV * renv;
	REGINFO * infop;
	__time64_t last_ckp_time, now;
	uint32 bytes, id, logflags, mbytes, op;
	int ret = 0;
	/*
	 * A client will only call through here during recovery,
	 * so just sync the Mpool and go home.  We want to be sure
	 * that since queue meta pages are not rolled back that they
	 * are clean in the cache prior to any transaction log
	 * truncation due to syncup.
	 */
	if(IS_REP_CLIENT(env)) {
		if(MPOOL_ON(env) && (ret = __memp_sync(env, DB_SYNC_CHECKPOINT, NULL)) != 0)
			__db_err(env, ret, DB_STR("4518", "txn_checkpoint: failed to flush the buffer cache"));
		else
			ret = 0;
	}
	else {
		dblp = env->lg_handle;
		lp = (LOG *)dblp->reginfo.primary;
		mgr = env->tx_handle;
		region = (DB_TXNREGION *)mgr->reginfo.primary;
		infop = env->reginfo;
		renv = (REGENV *)infop->primary;
		//
		// No mutex is needed as envid is read-only once it is set.
		//
		id = renv->envid;
		MUTEX_LOCK(env, region->mtx_ckp);
		/*
		 * The checkpoint LSN is an LSN such that all transactions begun before
		 * it are complete.  Our first guess (corrected below based on the list
		 * of active transactions) is the last-written LSN.
		 */
		if((ret = __log_current_lsn_int(env, &ckp_lsn, &mbytes, &bytes)) != 0)
			goto err;
		if(!LF_ISSET(DB_FORCE)) {
			/* Don't checkpoint a quiescent database. */
			if(bytes == 0 && mbytes == 0)
				goto err;
			/*
			 * If either kbytes or minutes is non-zero, then only take the
			 * checkpoint if more than "minutes" minutes have passed or if
			 * more than "kbytes" of log data have been written since the
			 * last checkpoint.
			 */
			if(kbytes != 0 && mbytes*1024+bytes/1024 >= (uint32)kbytes)
				goto do_ckp;
			if(minutes != 0) {
				_time64(&now);
				TXN_SYSTEM_LOCK(env);
				last_ckp_time = region->time_ckp;
				TXN_SYSTEM_UNLOCK(env);
				if(now-last_ckp_time >= (__time64_t)(minutes*60))
					goto do_ckp;
			}
			//
			// If we checked time and data and didn't go to checkpoint, we're done.
			//
			if(minutes != 0 || kbytes != 0)
				goto err;
		}
		/*
		 * We must single thread checkpoints otherwise the chk_lsn may get out
		 * of order.  We need to capture the start of the earliest currently
		 * active transaction (chk_lsn) and then flush all buffers.  While
		 * doing this we we could then be overtaken by another checkpoint that
		 * sees a later chk_lsn but competes first.  An archive process could
		 * then remove a log this checkpoint depends on.
		 */
	do_ckp:
		if((ret = __txn_getactive(env, &ckp_lsn)) != 0)
			goto err;
		/*
		 * Checkpoints in replication groups can cause performance problems.
		 *
		 * As on the master, checkpoint on the replica requires the cache be
		 * flushed.  The problem occurs when a client has dirty cache pages
		 * to write when the checkpoint record arrives, and the client's PERM
		 * response is necessary in order to meet the system's durability
		 * guarantees.  In this case, the master will have to wait until the
		 * client completes its cache flush and writes the checkpoint record
		 * before subsequent transactions can be committed.  The delay may
		 * cause transactions to timeout waiting on client response, which
		 * can cause nasty ripple effects in the system's overall throughput.
		 * [#15338]
		 *
		 * First, we send a start-sync record when the checkpoint starts so
		 * clients can start flushing their cache in preparation for the
		 * arrival of the checkpoint record.
		 */
		if(LOGGING_ON(env) && IS_REP_MASTER(env)) {
	#ifdef HAVE_REPLICATION_THREADS
			/*
			 * If repmgr is configured in the shared environment, but no
			 * send() function configured for this process, assume we have a
			 * replication-unaware process that wants to automatically
			 * participate in replication (i.e., sending replication
			 * messages to clients).
			 */
			if(env->rep_handle->send == NULL && F_ISSET(env, ENV_THREAD) && APP_IS_REPMGR(env) && (ret = __repmgr_autostart(env)) != 0)
				goto err;
	#endif
			if(env->rep_handle->send)
				__rep_send_message(env, DB_EID_BROADCAST, REP_START_SYNC, &ckp_lsn, NULL, 0, 0);
		}
		/* Flush the cache. */
		if(MPOOL_ON(env) && (ret = __memp_sync_int(env, NULL, 0, DB_SYNC_CHECKPOINT, NULL, NULL)) != 0) {
			__db_err(env, ret, DB_STR("4519", "txn_checkpoint: failed to flush the buffer cache"));
			goto err;
		}
		/*
		 * The client won't have more dirty pages to flush from its cache than
		 * the master did, but there may be differences between the hardware,
		 * I/O configuration and workload on the master and the client that
		 * can result in the client being unable to finish its cache flush as
		 * fast as the master.  A way to avoid the problem is to pause after
		 * the master completes its checkpoint and before the actual checkpoint
		 * record is logged, giving the replicas additional time to finish.
		 *
		 * !!!
		 * Currently turned off when testing, because it makes the test suite
		 * take a long time to run.
		 */
	#ifndef CONFIG_TEST
		if(LOGGING_ON(env) && IS_REP_MASTER(env) && env->rep_handle->send && !LF_ISSET(DB_CKP_INTERNAL) && env->rep_handle->region->chkpt_delay != 0)
			__os_yield(env, 0, env->rep_handle->region->chkpt_delay);
	#endif
		/*
		 * Because we can't be a replication client here, and because
		 * recovery (somewhat unusually) calls txn_checkpoint and expects
		 * it to write a log message, LOGGING_ON is the correct macro here.
		 */
		if(LOGGING_ON(env)) {
			TXN_SYSTEM_LOCK(env);
			last_ckp = region->last_ckp;
			TXN_SYSTEM_UNLOCK(env);
			/*
			 * Put out records for the open files before we log
			 * the checkpoint.  The records are certain to be at
			 * or after ckp_lsn, but before the checkpoint record
			 * itself, so they're sure to be included if we start
			 * recovery from the ckp_lsn contained in this checkpoint.
			 */
			logflags = DB_LOG_CHKPNT;
			/*
			 * If this is a normal checkpoint, log files as checkpoints.
			 * If we are recovering, only log as DBREG_RCLOSE if
			 * there are no prepared txns.  Otherwise, it should stay as DBREG_CHKPNT.
			 */
			op = DBREG_CHKPNT;
			if(!IS_RECOVERING(env))
				logflags |= DB_FLUSH;
			else if(region->stat.st_nrestores == 0)
				op = DBREG_RCLOSE;
			if((ret = __dbreg_log_files(env, op)) != 0 ||
			   (ret = __txn_ckp_log(env, NULL, &ckp_lsn, logflags, &ckp_lsn, &last_ckp, (int32)time(NULL), id, 0)) != 0) {
				__db_err(env, ret, DB_STR_A("4520", "txn_checkpoint: log failed at LSN [%ld %ld]", "%ld %ld"), (long)ckp_lsn.file, (long)ckp_lsn.Offset_);
				goto err;
			}
			if((ret = __txn_updateckp(env, &ckp_lsn)) != 0)
				goto err;
		}
err:
		MUTEX_UNLOCK(env, region->mtx_ckp);
		if(ret == 0 && lp->db_log_autoremove)
			__log_autoremove(env);
	}
	return ret;
}
/*
 * __txn_getactive --
 *	 Find the oldest active transaction and figure out its "begin" LSN.
 *	 This is the lowest LSN we can checkpoint, since any record written
 *	 after it may be involved in a transaction and may therefore need
 *	 to be undone in the case of an abort.
 *
 *	 We check both the file and offset for 0 since the lsn may be in
 *	 transition.  If it is then we don't care about this txn because it
 *	 must be starting after we set the initial value of lsnp in the caller.
 *	 All txns must initalize their begin_lsn before writing to the log.
 */
int __txn_getactive(ENV*env, DB_LSN * lsnp)
{
	TXN_DETAIL * td;
	DB_TXNMGR * mgr = env->tx_handle;
	DB_TXNREGION * region = (DB_TXNREGION *)mgr->reginfo.primary;
	TXN_SYSTEM_LOCK(env);
	SH_TAILQ_FOREACH(td, &region->active_txn, links, __txn_detail)
	if(td->begin_lsn.file && td->begin_lsn.Offset_ && LOG_COMPARE(&td->begin_lsn, lsnp) < 0)
		*lsnp = td->begin_lsn;
	TXN_SYSTEM_UNLOCK(env);
	return 0;
}
/*
 * __txn_getckp --
 *	Get the LSN of the last transaction checkpoint.
 */
int __txn_getckp(ENV*env, DB_LSN * lsnp)
{
	DB_LSN lsn;
	DB_TXNMGR * mgr = env->tx_handle;
	DB_TXNREGION * region = (DB_TXNREGION *)mgr->reginfo.primary;
	TXN_SYSTEM_LOCK(env);
	lsn = region->last_ckp;
	TXN_SYSTEM_UNLOCK(env);
	if(IS_ZERO_LSN(lsn))
		return DB_NOTFOUND;
	else {
		*lsnp = lsn;
		return 0;
	}
}
/*
 * __txn_updateckp --
 *	Update the last_ckp field in the transaction region.  This happens
 * at the end of a normal checkpoint and also when a replication client
 * receives a checkpoint record.
 */
int __txn_updateckp(ENV*env, DB_LSN * lsnp)
{
	DB_TXNMGR * mgr = env->tx_handle;
	DB_TXNREGION * region = (DB_TXNREGION *)mgr->reginfo.primary;
	/*
	 * We want to make sure last_ckp only moves forward;  since we drop
	 * locks above and in log_put, it's possible for two calls to
	 * __txn_ckp_log to finish in a different order from how they were
	 * called.
	 */
	TXN_SYSTEM_LOCK(env);
	if(LOG_COMPARE(&region->last_ckp, lsnp) < 0) {
		region->last_ckp = *lsnp;
		_time64(&region->time_ckp);
	}
	TXN_SYSTEM_UNLOCK(env);
	return 0;
}
/*
 * __txn_failchk --
 *	Check for transactions started by dead threads of control.
 */
int __txn_failchk(ENV*env)
{
	DB_TXN * ktxn, * txn;
	TXN_DETAIL * ktd, * td;
	db_threadid_t tid;
	int ret;
	char buf[DB_THREADID_STRLEN];
	pid_t pid;
	DB_TXNMGR * mgr = env->tx_handle;
	DB_ENV * dbenv = env->dbenv;
	DB_TXNREGION * region = (DB_TXNREGION *)mgr->reginfo.primary;
retry:
	TXN_SYSTEM_LOCK(env);
	SH_TAILQ_FOREACH(td, &region->active_txn, links, __txn_detail) {
		/*
		 * If this is a child transaction, skip it.
		 * The parent will take care of it.
		 */
		if(td->parent != INVALID_ROFF)
			continue;
		/*
		 * If the txn is prepared, then it does not matter
		 * what the state of the thread is.
		 */
		if(td->status == TXN_PREPARED)
			continue;
		/* If the thread is still alive, it's not a problem. */
		if(dbenv->is_alive(dbenv, td->pid, td->tid, 0))
			continue;
		if(F_ISSET(td, TXN_DTL_INMEMORY)) {
			TXN_SYSTEM_UNLOCK(env);
			return __db_failed(env, DB_STR("4501", "Transaction has in memory logs"), td->pid, td->tid);
		}
		/* Abort the transaction. */
		TXN_SYSTEM_UNLOCK(env);
		if((ret = __os_calloc(env, 1, sizeof(DB_TXN), &txn)) != 0)
			return ret;
		if((ret = __txn_continue(env, txn, td, NULL, 1)) != 0)
			return ret;
		SH_TAILQ_FOREACH(ktd, &td->kids, klinks, __txn_detail) {
			if(F_ISSET(ktd, TXN_DTL_INMEMORY))
				return __db_failed(env, DB_STR("4502", "Transaction has in memory logs"), td->pid, td->tid);
			if((ret = __os_calloc(env, 1, sizeof(DB_TXN), &ktxn)) != 0)
				return ret;
			if((ret = __txn_continue(env, ktxn, ktd, NULL, 1)) != 0)
				return ret;
			ktxn->parent = txn;
			ktxn->mgrp = txn->mgrp;
			TAILQ_INSERT_HEAD(&txn->kids, ktxn, klinks);
		}
		pid = td->pid;
		tid = td->tid;
		dbenv->thread_id_string(dbenv, pid, tid, buf);
		__db_msg(env, DB_STR_A("4503", "Aborting txn %#lx: %s", "%#lx %s"), (ulong)txn->txnid, buf);
		if((ret = __txn_abort(txn)) != 0)
			return __db_failed(env, DB_STR("4504", "Transaction abort failed"), pid, tid);
		goto retry;
	}
	TXN_SYSTEM_UNLOCK(env);
	return 0;
}
/*
 * PUBLIC: int __txn_regop_recover
 * PUBLIC:   (ENV *, DBT *, DB_LSN *, db_recops, void *);
 *
 * These records are only ever written for commits.  Normally, we redo any
 * committed transaction, however if we are doing recovery to a timestamp, then
 * we may treat transactions that committed after the timestamp as aborted.
 */
int __txn_regop_recover(ENV*env, DBT * dbtp, DB_LSN * lsnp, db_recops op, void * info)
{
	__txn_regop_args * argp;
	DB_TXNHEAD * headp;
	int ret;
	uint32 status;
#ifdef DEBUG_RECOVER
	__txn_regop_print(env, dbtp, lsnp, op, info);
#endif
	if((ret = __txn_regop_read(env, dbtp->data, &argp)) != 0)
		return ret;
	headp = (DB_TXNHEAD *)info;
	/*
	 * We are only ever called during FORWARD_ROLL or BACKWARD_ROLL.
	 * We check for the former explicitly and the last two clauses
	 * apply to the BACKWARD_ROLL case.
	 */
	if(op == DB_TXN_FORWARD_ROLL) {
		/*
		 * If this was a 2-phase-commit transaction, then it
		 * might already have been removed from the list, and
		 * that's OK.  Ignore the return code from remove.
		 */
		if((ret = __db_txnlist_remove(env, (DB_TXNHEAD *)info, argp->txnp->txnid)) != DB_NOTFOUND && ret != 0)
			goto err;
	}
	else if((env->dbenv->tx_timestamp != 0 && argp->timestamp > (int32)env->dbenv->tx_timestamp) ||
	        (!IS_ZERO_LSN(headp->trunc_lsn) &&
	         LOG_COMPARE(&headp->trunc_lsn, lsnp) < 0)) {
		/*
		 * We failed either the timestamp check or the trunc_lsn check,
		 * so we treat this as an abort even if it was a commit record.
		 */
		if((ret = __db_txnlist_update(env, (DB_TXNHEAD *)info, argp->txnp->txnid, TXN_ABORT, NULL, &status, 1)) != 0)
			goto err;
		else if(status != TXN_IGNORE && status != TXN_OK)
			goto err;
	}
	else {
		/* This is a normal commit; mark it appropriately. */
		if((ret = __db_txnlist_update(env, (DB_TXNHEAD *)info, argp->txnp->txnid, argp->opcode, lsnp, &status, 0)) == DB_NOTFOUND) {
			if((ret = __db_txnlist_add(env, (DB_TXNHEAD *)info, argp->txnp->txnid, argp->opcode == TXN_ABORT ? TXN_IGNORE : argp->opcode, lsnp)) != 0)
				goto err;
		}
		else if(ret != 0 || (status != TXN_IGNORE && status != TXN_OK))
			goto err;
	}
	if(ret == 0)
		*lsnp = argp->prev_lsn;
	if(0) {
err:
		__db_errx(env, DB_STR_A("4514", "txnid %lx commit record found, already on commit list", "%lx"), (ulong)argp->txnp->txnid);
		ret = EINVAL;
	}
	__os_free(env, argp);
	return ret;
}
/*
 * PUBLIC: int __txn_prepare_recover
 * PUBLIC:   (ENV *, DBT *, DB_LSN *, db_recops, void *);
 *
 * These records are only ever written for prepares.
 */
int __txn_prepare_recover(ENV*env, DBT * dbtp, DB_LSN * lsnp, db_recops op, void * info)
{
	__txn_prepare_args * argp;
	DBT * lock_dbt;
	DB_TXNHEAD * headp;
	DB_LOCKTAB * lt;
	uint32 status;
	int ret;
#ifdef DEBUG_RECOVER
	__txn_prepare_print(env, dbtp, lsnp, op, info);
#endif
	if((ret = __txn_prepare_read(env, dbtp->data, &argp)) != 0)
		return ret;
	if(argp->opcode != TXN_PREPARE && argp->opcode != TXN_ABORT) {
		ret = EINVAL;
		goto err;
	}
	headp = (DB_TXNHEAD *)info;
	/*
	 * The return value here is either a DB_NOTFOUND or it is
	 * the transaction status from the list.  It is not a normal
	 * error return, so we must make sure that in each of the
	 * cases below, we overwrite the ret value so we return
	 * appropriately.
	 */
	ret = __db_txnlist_find(env, (DB_TXNHEAD *)info, argp->txnp->txnid, &status);

	/*
	 * If we are rolling forward, then an aborted prepare
	 * indicates that this may be the last record we'll see for
	 * this transaction ID, so we should remove it from the list.
	 */
	if(op == DB_TXN_FORWARD_ROLL) {
		if((ret = __db_txnlist_remove(env, (DB_TXNHEAD *)info, argp->txnp->txnid)) != 0)
			goto txn_err;
	}
	else if(op == DB_TXN_BACKWARD_ROLL && status == TXN_PREPARE) {
		/*
		 * On the backward pass, we have four possibilities:
		 * 1. The transaction is already committed, no-op.
		 * 2. The transaction is already aborted, no-op.
		 * 3. The prepare failed and was aborted, mark as abort.
		 * 4. The transaction is neither committed nor aborted.
		 *	 Treat this like a commit and roll forward so that
		 *	 the transaction can be resurrected in the region.
		 * We handle cases 3 and 4 here; cases 1 and 2
		 * are the final clause below.
		 */
		if(argp->opcode == TXN_ABORT) {
			if((ret = __db_txnlist_update(env, (DB_TXNHEAD *)info, argp->txnp->txnid, TXN_ABORT, NULL, &status, 0)) != 0 && status != TXN_PREPARE)
				goto txn_err;
			ret = 0;
		}
		/*
		 * This is prepared, but not yet committed transaction.  We
		 * need to add it to the transaction list, so that it gets
		 * rolled forward. We also have to add it to the region's
		 * internal state so it can be properly aborted or committed
		 * after recovery (see txn_recover).
		 */
		else if((ret = __db_txnlist_remove(env, (DB_TXNHEAD *)info, argp->txnp->txnid)) != 0) {
txn_err:
			__db_errx(env, DB_STR_A("4515", "transaction not in list %lx", "%lx"), (ulong)argp->txnp->txnid);
			ret = DB_NOTFOUND;
		}
		else if(IS_ZERO_LSN(headp->trunc_lsn) || LOG_COMPARE(&headp->trunc_lsn, lsnp) >= 0) {
			if((ret = __db_txnlist_add(env, (DB_TXNHEAD *)info, argp->txnp->txnid, TXN_COMMIT, lsnp)) == 0) {
				/* Re-acquire the locks for this transaction. */
				lock_dbt = &argp->locks;
				if(LOCKING_ON(env)) {
					lt = env->lk_handle;
					if((ret = __lock_getlocker(lt, argp->txnp->txnid, 1, &argp->txnp->locker)) != 0)
						goto err;
					if((ret = __lock_get_list(env, argp->txnp->locker, 0, DB_LOCK_WRITE, lock_dbt)) != 0)
						goto err;
				}
				ret = __txn_restore_txn(env, lsnp, argp);
			}
		}
	}
	else
		ret = 0;
	if(ret == 0)
		*lsnp = argp->prev_lsn;
err:
	__os_free(env, argp);
	return ret;
}

/*
 * PUBLIC: int __txn_ckp_recover
 * PUBLIC:(ENV *, DBT *, DB_LSN *, db_recops, void *);
 */
int __txn_ckp_recover(ENV*env, DBT * dbtp, DB_LSN * lsnp, db_recops op, void * info)
{
	__txn_ckp_args * argp;
	int ret;
#ifdef DEBUG_RECOVER
	__txn_ckp_print(env, dbtp, lsnp, op, info);
#endif
	if((ret = __txn_ckp_read(env, dbtp->data, &argp)) != 0)
		return ret;
	if(op == DB_TXN_BACKWARD_ROLL)
		__db_txnlist_ckp(env, (DB_TXNHEAD *)info, lsnp);
	*lsnp = argp->last_ckp;
	__os_free(env, argp);
	return DB_TXN_CKP;
}

/*
 * __txn_child_recover
 *	Recover a commit record for a child transaction.
 */
int __txn_child_recover(ENV*env, DBT * dbtp, DB_LSN * lsnp, db_recops op, void * info)
{
	__txn_child_args * argp;
	uint32 c_stat, p_stat, tmpstat;
	int ret, t_ret;

#ifdef DEBUG_RECOVER
	__txn_child_print(env, dbtp, lsnp, op, info);
#endif
	if((ret = __txn_child_read(env, dbtp->data, &argp)) != 0)
		return ret;
	/*
	 * This is a record in a PARENT's log trail indicating that a
	 * child committed.  If we are aborting, return the childs last
	 * record's LSN.  If we are in recovery, then if the
	 * parent is committing, we set ourselves up to commit, else we do nothing.
	 */
	if(op == DB_TXN_ABORT) {
		*lsnp = argp->c_lsn;
		ret = __db_txnlist_lsnadd(env, (DB_TXNHEAD *)info, &argp->prev_lsn);
		goto out;
	}
	else if(op == DB_TXN_BACKWARD_ROLL) {
		/* Child might exist -- look for it. */
		ret = __db_txnlist_find(env, (DB_TXNHEAD *)info, argp->child, &c_stat);
		t_ret = __db_txnlist_find(env, (DB_TXNHEAD *)info, argp->txnp->txnid, &p_stat);
		if(!oneof2(ret, 0, DB_NOTFOUND))
			goto out;
		if(t_ret != 0 && t_ret != DB_NOTFOUND) {
			ret = t_ret;
			goto out;
		}
		/*
		 * If the parent is in state COMMIT or IGNORE, then we apply
		 * that to the child, else we need to abort the child.
		 */
		if(ret == DB_NOTFOUND  || oneof2(c_stat, TXN_OK, TXN_COMMIT)) {
			if(t_ret == DB_NOTFOUND || (p_stat != TXN_COMMIT  && p_stat != TXN_IGNORE))
				c_stat = TXN_ABORT;
			else
				c_stat = p_stat;
			if(ret == DB_NOTFOUND)
				ret = __db_txnlist_add(env, (DB_TXNHEAD *)info, argp->child, c_stat, 0);
			else
				ret = __db_txnlist_update(env, (DB_TXNHEAD *)info, argp->child, c_stat, NULL, &tmpstat, 0);
		}
		else if(c_stat == TXN_EXPECTED) {
			/*
			 * The open after this create succeeded.  If the
			 * parent succeeded, we don't want to redo; if the
			 * parent aborted, we do want to undo.
			 */
			switch(p_stat) {
			    case TXN_COMMIT:
			    case TXN_IGNORE: c_stat = TXN_IGNORE; break;
			    default: c_stat = TXN_ABORT;
			}
			ret = __db_txnlist_update(env, (DB_TXNHEAD *)info, argp->child, c_stat, NULL, &tmpstat, 0);
		}
		else if(c_stat == TXN_UNEXPECTED) {
			/*
			 * The open after this create failed.  If the parent
			 * is rolling forward, we need to roll forward.  If
			 * the parent failed, then we do not want to abort
			 * (because the file may not be the one in which we
			 * are interested).
			 */
			ret = __db_txnlist_update(env, (DB_TXNHEAD *)info, argp->child, p_stat == TXN_COMMIT ? TXN_COMMIT : TXN_IGNORE, NULL, &tmpstat, 0);
		}
	}
	else if(op == DB_TXN_OPENFILES) {
		/*
		 * If we have a partial subtransaction, then the whole
		 * transaction should be ignored.
		 */
		if((ret = __db_txnlist_find(env, (DB_TXNHEAD *)info, argp->child, &c_stat)) == DB_NOTFOUND)
			ret = __db_txnlist_update(env, (DB_TXNHEAD *)info, argp->txnp->txnid, TXN_IGNORE, NULL, &p_stat, 1);
	}
	else if(DB_REDO(op)) {
		/* Forward Roll */
		if((ret = __db_txnlist_remove(env, (DB_TXNHEAD *)info, argp->child)) != 0)
			__db_errx(env, DB_STR_A("4516", "Transaction not in list %x", "%x"), argp->child);
	}
	if(ret == 0)
		*lsnp = argp->prev_lsn;
out:
	__os_free(env, argp);
	return ret;
}
/*
 * __txn_restore_txn --
 *	Using only during XA recovery.  If we find any transactions that are
 * prepared, but not yet committed, then we need to restore the transaction's
 * state into the shared region, because the TM is going to issue an abort
 * or commit and we need to respond correctly.
 *
 * lsnp is the LSN of the returned LSN
 * argp is the prepare record (in an appropriate structure)
 */
int __txn_restore_txn(ENV*env, DB_LSN * lsnp, __txn_prepare_args * argp)
{
	DB_TXNMGR * mgr;
	DB_TXNREGION * region;
	TXN_DETAIL * td;
	int ret;
	if(argp->gid.size == 0)
		return 0;
	mgr = env->tx_handle;
	region = (DB_TXNREGION *)mgr->reginfo.primary;
	TXN_SYSTEM_LOCK(env);
	/* Allocate a new transaction detail structure. */
	if((ret = __env_alloc(&mgr->reginfo, sizeof(TXN_DETAIL), &td)) != 0) {
		TXN_SYSTEM_UNLOCK(env);
		return ret;
	}
	/* Place transaction on active transaction list. */
	SH_TAILQ_INSERT_HEAD(&region->active_txn, td, links, __txn_detail);
	region->curtxns++;

	td->txnid = argp->txnp->txnid;
	__os_id(env->dbenv, &td->pid, &td->tid);
	td->last_lsn = *lsnp;
	td->begin_lsn = argp->begin_lsn;
	td->parent = INVALID_ROFF;
	td->name = INVALID_ROFF;
	SH_TAILQ_INIT(&td->kids);
	MAX_LSN(td->read_lsn);
	MAX_LSN(td->visible_lsn);
	td->mvcc_ref = 0;
	td->mvcc_mtx = MUTEX_INVALID;
	td->status = TXN_PREPARED;
	td->flags = TXN_DTL_RESTORED;
	memcpy(td->gid, argp->gid.data, argp->gid.size);
	td->nlog_dbs = 0;
	td->nlog_slots = TXN_NSLOTS;
	td->log_dbs = R_OFFSET(&mgr->reginfo, td->slots);
	region->stat.st_nrestores++;
#ifdef HAVE_STATISTICS
	STAT_INC(env, txn, nactive, region->stat.st_nactive, td->txnid);
	if(region->stat.st_nactive > region->stat.st_maxnactive)
		STAT_SET(env, txn, maxnactive, region->stat.st_maxnactive, region->stat.st_nactive, td->txnid);
#endif
	TXN_SYSTEM_UNLOCK(env);
	return 0;
}
/*
 * __txn_recycle_recover --
 *	Recovery function for recycle.
 */
int __txn_recycle_recover(ENV*env, DBT * dbtp, DB_LSN * lsnp, db_recops op, void * info)
{
	__txn_recycle_args * argp;
	int ret;
#ifdef DEBUG_RECOVER
	__txn_child_print(env, dbtp, lsnp, op, info);
#endif
	if((ret = __txn_recycle_read(env, dbtp->data, &argp)) != 0)
		return ret;
	COMPQUIET(lsnp, 0);
	if((ret = __db_txnlist_gen(env, (DB_TXNHEAD *)info, DB_UNDO(op) ? -1 : 1, argp->min, argp->max)) != 0)
		return ret;
	__os_free(env, argp);
	return 0;
}
/*
 * These records are only ever written for commits.  Normally, we redo any
 * committed transaction, however if we are doing recovery to a timestamp, then
 * we may treat transactions that committed after the timestamp as aborted.
 */
int __txn_regop_42_recover(ENV*env, DBT * dbtp, DB_LSN * lsnp, db_recops op, void * info)
{
	__txn_regop_42_args * argp;
	DB_TXNHEAD * headp;
	uint32 status;
	int ret;
#ifdef DEBUG_RECOVER
	__txn_regop_42_print(env, dbtp, lsnp, op, info);
#endif
	if((ret = __txn_regop_42_read(env, dbtp->data, &argp)) != 0)
		return ret;
	headp = (DB_TXNHEAD *)info;
	/*
	 * We are only ever called during FORWARD_ROLL or BACKWARD_ROLL.
	 * We check for the former explicitly and the last two clauses
	 * apply to the BACKWARD_ROLL case.
	 */
	if(op == DB_TXN_FORWARD_ROLL) {
		/*
		 * If this was a 2-phase-commit transaction, then it
		 * might already have been removed from the list, and
		 * that's OK.  Ignore the return code from remove.
		 */
		if((ret = __db_txnlist_remove(env, (DB_TXNHEAD *)info, argp->txnp->txnid)) != DB_NOTFOUND && ret != 0)
			goto err;
	}
	else if((env->dbenv->tx_timestamp != 0 && argp->timestamp > (int32)env->dbenv->tx_timestamp) ||
		(!IS_ZERO_LSN(headp->trunc_lsn) && LOG_COMPARE(&headp->trunc_lsn, lsnp) < 0)) {
		/*
		 * We failed either the timestamp check or the trunc_lsn check,
		 * so we treat this as an abort even if it was a commit record.
		 */
		if((ret = __db_txnlist_update(env, (DB_TXNHEAD *)info, argp->txnp->txnid, TXN_ABORT, NULL, &status, 1)) != 0)
			goto err;
		else if(status != TXN_IGNORE && status != TXN_OK)
			goto err;
	}
	else {
		/* This is a normal commit; mark it appropriately. */
		if((ret = __db_txnlist_update(env, (DB_TXNHEAD *)info, argp->txnp->txnid, argp->opcode, lsnp, &status, 0)) == DB_NOTFOUND) {
			if((ret = __db_txnlist_add(env, (DB_TXNHEAD *)info, argp->txnp->txnid, argp->opcode == TXN_ABORT ? TXN_IGNORE : argp->opcode, lsnp)) != 0)
				goto err;
		}
		else if(ret != 0 || (status != TXN_IGNORE && status != TXN_OK))
			goto err;
	}
	if(ret == 0)
		*lsnp = argp->prev_lsn;
	if(0) {
err:
		__db_errx(env, DB_STR_A("4517", "txnid %lx commit record found, already on commit list", "%lx"), (ulong)argp->txnp->txnid);
		ret = EINVAL;
	}
	__os_free(env, argp);
	return ret;
}

int __txn_ckp_42_recover(ENV*env, DBT * dbtp, DB_LSN * lsnp, db_recops op, void * info)
{
	__txn_ckp_42_args * argp;
	int ret;
#ifdef DEBUG_RECOVER
	__txn_ckp_42_print(env, dbtp, lsnp, op, info);
#endif
	if((ret = __txn_ckp_42_read(env, dbtp->data, &argp)) != 0)
		return ret;
	if(op == DB_TXN_BACKWARD_ROLL)
		__db_txnlist_ckp(env, (DB_TXNHEAD *)info, lsnp);
	*lsnp = argp->last_ckp;
	__os_free(env, argp);
	return DB_TXN_CKP;
}
/*
 * __txn_recover_pp --
 *	ENV->txn_recover pre/post processing.
 */
int __txn_recover_pp(DB_ENV * dbenv, DB_PREPLIST * preplist, long count, long * retp, uint32 flags)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->tx_handle, "txn_recover", DB_INIT_TXN);
	if(F_ISSET((DB_TXNREGION *)env->tx_handle->reginfo.primary, TXN_IN_RECOVERY)) {
		__db_errx(env, DB_STR("4505", "operation not permitted while in recovery"));
		return EINVAL;
	}
	if(flags != DB_FIRST && flags != DB_NEXT)
		return __db_ferr(env, "DB_ENV->txn_recover", 0);
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__txn_recover(env, preplist, count, retp, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __txn_recover --
 *	ENV->txn_recover.
 */
int __txn_recover(ENV*env, DB_PREPLIST * txns, long count, long * retp, uint32 flags)
{
	/*
	 * Public API to retrieve the list of prepared, but not yet committed
	 * transactions.  See __txn_get_prepared for details.  This function
	 * and __db_xa_recover both wrap that one.
	 */
	return __txn_get_prepared(env, NULL, txns, count, retp, flags);
}
/*
 * __txn_get_prepared --
 *      Returns a list of prepared (and for XA, heuristically completed)
 *      transactions (less than or equal to the count parameter).  One of
 *      xids or txns must be set to point to an array of the appropriate type.
 *      The count parameter indicates the number of entries in the xids and/or
 *      txns array. The retp parameter will be set to indicate the number of
 *      entries returned in the xids/txns array.  Flags indicates the operation,
 *      one of DB_FIRST or DB_NEXT.
 */
int __txn_get_prepared(ENV*env, XID * xids, DB_PREPLIST * txns, long count, long * retp, uint32 flags)
{
	DB_LSN min;
	DB_PREPLIST * prepp;
	DB_THREAD_INFO * ip;
	DB_TXNMGR * mgr;
	DB_TXNREGION * region;
	TXN_DETAIL * td;
	XID * xidp;
	long i;
	int restored = 0;
	int ret = 0;
	*retp = 0;
	MAX_LSN(min);
	prepp = txns;
	xidp = xids;
	// 
	// If we are starting a scan, then we traverse the active transaction
	// list once making sure that all transactions are marked as not having
	// been collected.  Then on each pass, we mark the ones we collected
	// so that if we cannot collect them all at once, we can finish up
	// next time with a continue.
	// 
	mgr = env->tx_handle;
	region = (DB_TXNREGION *)mgr->reginfo.primary;
	// 
	// During this pass we need to figure out if we are going to need
	// to open files.  We need to open files if we've never collected
	// before (in which case, none of the COLLECTED bits will be set)
	// and the ones that we are collecting are restored (if they aren't
	// restored, then we never crashed; just the main server did).
	// 
	TXN_SYSTEM_LOCK(env);
	ENV_GET_THREAD_INFO(env, ip);
	// Now begin collecting active transactions. 
	for(td = SH_TAILQ_FIRST(&region->active_txn, __txn_detail); td && *retp < count; td = SH_TAILQ_NEXT(td, links, __txn_detail)) {
		if(td->status != TXN_PREPARED || (flags != DB_FIRST && F_ISSET(td, TXN_DTL_COLLECTED)))
			continue;
		if(F_ISSET(td, TXN_DTL_RESTORED))
			restored = 1;
		if(xids) {
			xidp->formatID = td->format;
			// 
			// XID structure uses longs; use use uint32's as we
			// log them to disk. Cast them to make the conversion explicit.
			// 
			xidp->gtrid_length = (long)td->gtrid;
			xidp->bqual_length = (long)td->bqual;
			memcpy(xidp->data, td->gid, sizeof(td->gid));
			xidp++;
		}
		if(txns) {
			if((ret = __os_calloc(env, 1, sizeof(DB_TXN), &prepp->txn)) != 0) {
				TXN_SYSTEM_UNLOCK(env);
				goto err;
			}
			prepp->txn->td = td;
			memcpy(prepp->gid, td->gid, sizeof(td->gid));
			prepp++;
		}
		if(!IS_ZERO_LSN(td->begin_lsn) && LOG_COMPARE(&td->begin_lsn, &min) < 0)
			min = td->begin_lsn;
		(*retp)++;
		F_SET(td, TXN_DTL_COLLECTED);
	}
	if(flags == DB_FIRST)
		for(; td; td = SH_TAILQ_NEXT(td, links, __txn_detail))
			F_CLR(td, TXN_DTL_COLLECTED);
	TXN_SYSTEM_UNLOCK(env);
	/*
	 * Now link all the transactions into the transaction manager's list.
	 */
	if(txns && *retp != 0) {
		MUTEX_LOCK(env, mgr->mutex);
		for(i = 0; i < *retp; i++) {
			if((ret = __txn_continue(env, txns[i].txn, (TXN_DETAIL *)txns[i].txn->td, ip, 0)) != 0)
				goto err;
			F_SET(txns[i].txn, TXN_MALLOC);
			if(F_ISSET(env->dbenv, DB_ENV_TXN_NOSYNC))
				F_SET(txns[i].txn, TXN_NOSYNC);
			else if(F_ISSET(env->dbenv, DB_ENV_TXN_WRITE_NOSYNC))
				F_SET(txns[i].txn, TXN_WRITE_NOSYNC);
			else
				F_SET(txns[i].txn, TXN_SYNC);
			TAILQ_INSERT_TAIL(&mgr->txn_chain, txns[i].txn, links);
		}
		MUTEX_UNLOCK(env, mgr->mutex);
		// 
		// If we are restoring, update our count of outstanding transactions.
		// 
		if(REP_ON(env)) {
			REP_SYSTEM_LOCK(env);
			env->rep_handle->region->op_cnt += (ulong)*retp;
			REP_SYSTEM_UNLOCK(env);
		}
	}
	// If recovery already opened the files for us, don't do it here. 
	if(restored != 0 && flags == DB_FIRST && !F_ISSET(env->lg_handle, DBLOG_OPENFILES))
		ret = __txn_openfiles(env, ip, &min, 0);
	if(0) {
err:
		TXN_SYSTEM_UNLOCK(env);
	}
	return ret;
}
/*
 * __txn_openfiles --
 *	Call env_openfiles.
 */
int __txn_openfiles(ENV*env, DB_THREAD_INFO * ip, DB_LSN * min, int force)
{
	DBT data;
	DB_LSN open_lsn;
	DB_TXNHEAD * txninfo;
	__txn_ckp_args * ckp_args;
	int ret, t_ret;
	/*
	 * Figure out the last checkpoint before the smallest start_lsn in the region.
	 */
	DB_LOGC * logc = NULL;
	if((ret = __log_cursor(env, &logc)) != 0)
		goto err;
	memzero(&data, sizeof(data));
	if((ret = __txn_getckp(env, &open_lsn)) == 0)
		while(!IS_ZERO_LSN(open_lsn) && (ret = __logc_get(logc, &open_lsn, &data, DB_SET)) == 0 && (force || (min && LOG_COMPARE(min, &open_lsn) < 0))) {
			/* Format the log record. */
			if((ret = __txn_ckp_read(env, data.data, &ckp_args)) != 0) {
				__db_errx(env, DB_STR_A("4506", "Invalid checkpoint record at [%lu][%lu]", "%lu %lu"), (ulong)open_lsn.file, (ulong)open_lsn.Offset_);
				goto err;
			}
			/*
			 * If force is set, then we're forcing ourselves
			 * to go back far enough to open files.
			 * Use ckp_lsn and then break out of the loop.
			 */
			open_lsn = force ? ckp_args->ckp_lsn : ckp_args->last_ckp;
			__os_free(env, ckp_args);
			if(force) {
				if((ret = __logc_get(logc, &open_lsn, &data, DB_SET)) != 0)
					goto err;
				break;
			}
		}
	/*
	 * There are several ways by which we may have gotten here.
	 * - We got a DB_NOTFOUND -- we need to read the first
	 *	log record.
	 * - We found a checkpoint before min.  We're done.
	 * - We found a checkpoint after min who's last_ckp is 0.  We
	 *	need to start at the beginning of the log.
	 * - We are forcing an openfiles and we have our ckp_lsn.
	 */
	if((ret == DB_NOTFOUND || IS_ZERO_LSN(open_lsn)) && (ret = __logc_get(logc, &open_lsn, &data, DB_FIRST)) != 0) {
		__db_errx(env, DB_STR("4507", "No log records"));
		goto err;
	}
	if((ret = __db_txnlist_init(env, ip, 0, 0, NULL, &txninfo)) != 0)
		goto err;
	ret = __env_openfiles(env, logc, txninfo, &data, &open_lsn, NULL, (double)0, 0);
	__db_txnlist_end(env, txninfo);
err:
	if(logc && (t_ret = __logc_close(logc)) != 0 && ret == 0)
		ret = t_ret;
	return ret;
}
//
//
//
/*
 * __txn_open --
 *	Open a transaction region.
 */
int __txn_open(ENV * env)
{
	DB_TXNMGR * mgr;
	DB_TXNREGION * region;
	int ret;
	/* Create/initialize the transaction manager structure. */
	if((ret = __os_calloc(env, 1, sizeof(DB_TXNMGR), &mgr)) != 0)
		return ret;
	TAILQ_INIT(&mgr->txn_chain);
	mgr->env = env;
	/* Join/create the txn region. */
	if((ret = __env_region_share(env, &mgr->reginfo)) != 0)
		goto err;
	/* If we created the region, initialize it. */
	if(F_ISSET(&mgr->reginfo, REGION_CREATE))
		if((ret = __txn_init(env, mgr)) != 0)
			goto err;
	/* Set the local addresses. */
	region = (DB_TXNREGION *)(mgr->reginfo.primary = R_ADDR(&mgr->reginfo, ((REGENV *)env->reginfo->primary)->tx_primary));
	/* If threaded, acquire a mutex to protect the active TXN list. */
	if((ret = __mutex_alloc(env, MTX_TXN_ACTIVE, DB_MUTEX_PROCESS_ONLY, &mgr->mutex)) != 0)
		goto err;
	mgr->reginfo.mtx_alloc = region->mtx_region;
	env->tx_handle = mgr;
	return 0;
err:
	env->tx_handle = NULL;
	if(mgr->reginfo.addr)
		__env_region_detach(env, &mgr->reginfo, 0);
	__mutex_free(env, &mgr->mutex);
	__os_free(env, mgr);
	return ret;
}
/*
 * __txn_init --
 *	Initialize a transaction region in shared memory.
 */
static int __txn_init(ENV * env, DB_TXNMGR * mgr)
{
	DB_LSN last_ckp;
	DB_TXNREGION * region;
	int ret;
	DB_ENV * dbenv = env->dbenv;
	/*
	 * Find the last checkpoint in the log.
	 */
	ZERO_LSN(last_ckp);
	if(LOGGING_ON(env)) {
		/*
		 * The log system has already walked through the last
		 * file.  Get the LSN of a checkpoint it may have found.
		 */
		if((ret = __log_get_cached_ckp_lsn(env, &last_ckp)) != 0)
			return ret;
		/*
		 * If that didn't work, look backwards from the beginning of
		 * the last log file until we find the last checkpoint.
		 */
		if(IS_ZERO_LSN(last_ckp) && (ret = __txn_findlastckp(env, &last_ckp, NULL)) != 0)
			return ret;
	}
	if((ret = __env_alloc(&mgr->reginfo, sizeof(DB_TXNREGION), &mgr->reginfo.primary)) != 0) {
		__db_errx(env, DB_STR("4508", "Unable to allocate memory for the transaction region"));
		return ret;
	}
	((REGENV *)env->reginfo->primary)->tx_primary = R_OFFSET(&mgr->reginfo, mgr->reginfo.primary);
	region = (DB_TXNREGION *)mgr->reginfo.primary;
	memzero(region, sizeof(*region));
	// We share the region so we need the same mutex. 
	region->mtx_region = ((REGENV *)env->reginfo->primary)->mtx_regenv;
	mgr->reginfo.mtx_alloc = region->mtx_region;

	region->maxtxns = dbenv->tx_max;
	region->inittxns = dbenv->tx_init;
	region->last_txnid = TXN_MINIMUM;
	region->cur_maxid = TXN_MAXIMUM;
	if((ret = __mutex_alloc(env, MTX_TXN_CHKPT, 0, &region->mtx_ckp)) != 0)
		return ret;
	region->last_ckp = last_ckp;
	region->time_ckp = time(NULL);
	memzero(&region->stat, sizeof(region->stat));
#ifdef HAVE_STATISTICS
	region->stat.st_maxtxns = region->maxtxns;
	region->stat.st_inittxns = region->inittxns;
#endif
	SH_TAILQ_INIT(&region->active_txn);
	SH_TAILQ_INIT(&region->mvcc_txn);
	return ret;
}
/*
 * __txn_findlastckp --
 *	Find the last checkpoint in the log, walking backwards from the
 *	max_lsn given or the beginning of the last log file.  (The
 *	log system looked through the last log file when it started up.)
 */
int __txn_findlastckp(ENV * pEnv, DB_LSN * pLsnp, DB_LSN * pMaxLsn)
{
	DBT dbt;
	DB_LOGC * logc;
	DB_LSN lsn;
	int ret, t_ret;
	uint32 rectype;
	uint64 _log_get_count = 0;
	ZERO_LSN(*pLsnp);
	if((ret = __log_cursor(pEnv, &logc)) != 0)
		return ret;
	// Get the last LSN
	// @ctr memzero(&dbt, sizeof(dbt));
	if(pMaxLsn) {
		lsn = *pMaxLsn;
		if((ret = __logc_get(logc, &lsn, &dbt, DB_SET)) != 0)
			goto err;
	}
	else {
		if((ret = __logc_get(logc, &lsn, &dbt, DB_LAST)) != 0)
			goto err;
		// 
		// Twiddle the last LSN so it points to the beginning of the
		// last file; we know there's no checkpoint after that, since
		// the log system already looked there.
		//
		lsn.Offset_ = 0;
	}
	// Read backwards, looking for checkpoints
	while((ret = __logc_get(logc, &lsn, &dbt, DB_PREV)) == 0) {
		_log_get_count++;
		if(dbt.size >= sizeof(uint32)) {
			LOGCOPY_32(pEnv, &rectype, dbt.data);
			if(rectype == DB___txn_ckp) {
				*pLsnp = lsn;
				break;
			}
		}
	}
err:    
	if((t_ret = __logc_close(logc)) != 0 && ret == 0)
		ret = t_ret;
	// Not finding a checkpoint is not an error;  there may not exist one in the log.
	return oneof2(ret, 0, DB_NOTFOUND) ? 0 : ret;
}
/*
 * __txn_env_refresh --
 *	Clean up after the transaction system on a close or failed open.
 */
int __txn_env_refresh(ENV * env)
{
	DB_TXN * txn;
	uint32 txnid;
	int t_ret;
	int ret = 0;
	DB_TXNMGR * mgr = env->tx_handle;
	REGINFO * reginfo = &mgr->reginfo;
	/*
	 * This function can only be called once per process (i.e., not
	 * once per thread), so no synchronization is required.
	 *
	 * The caller is probably doing something wrong if close is called with
	 * active transactions.  Try and abort any active transactions that are
	 * not prepared, but it's quite likely the aborts will fail because
	 * recovery won't find open files.  If we can't abort any of the
	 * unprepared transaction, panic, we have to run recovery to get back
	 * to a known state.
	 */
	int aborted = 0;
	if(TAILQ_FIRST(&mgr->txn_chain) != NULL) {
		while((txn = TAILQ_FIRST(&mgr->txn_chain)) != NULL) {
			/* Prepared transactions are OK. */
			txnid = txn->txnid;
			if(((TXN_DETAIL *)txn->td)->status == TXN_PREPARED) {
				if((ret = __txn_discard_int(txn, 0)) != 0) {
					__db_err(env, ret, DB_STR_A("4509", "unable to discard txn %#lx", "%#lx"), (ulong)txnid);
					break;
				}
				continue;
			}
			aborted = 1;
			if((t_ret = __txn_abort(txn)) != 0) {
				__db_err(env, t_ret, DB_STR_A("4510", "unable to abort transaction %#lx", "%#lx"), (ulong)txnid);
				ret = __env_panic(env, t_ret);
				break;
			}
		}
		if(aborted) {
			__db_errx(env, DB_STR("4511", "Error: closing the transaction region with active transactions"));
			SETIFZ(ret, EINVAL);
		}
	}
	/* Discard the per-thread lock. */
	if((t_ret = __mutex_free(env, &mgr->mutex)) != 0 && ret == 0)
		ret = t_ret;
	/* Detach from the region. */
	if(F_ISSET(env, ENV_PRIVATE))
		reginfo->mtx_alloc = MUTEX_INVALID;
	if((t_ret = __env_region_detach(env, reginfo, 0)) != 0 && ret == 0)
		ret = t_ret;
	__os_free(env, mgr);
	env->tx_handle = NULL;
	return ret;
}
/*
 * __txn_region_mutex_count --
 *	Return the number of mutexes the txn region will need.
 */
uint32 __txn_region_mutex_count(ENV*env)
{
	COMPQUIET(env, 0);
	/*
	 * We need  a mutex for DB_TXNMGR structure, two mutexes for the DB_TXNREGION structure.
	 */
	return 1+2;
}
/*
 * __txn_region_mutex_max --
 *	Return the number of additional mutexes the txn region will need.
 */
uint32 __txn_region_mutex_max(ENV*env)
{
	uint32 count;
	DB_ENV * dbenv = env->dbenv;
	if((count = dbenv->tx_max) == 0)
		count = DEF_MAX_TXNS;
	/* We may need a mutex for each MVCC txn. */
	return count > dbenv->tx_init ? count-dbenv->tx_init : 0;
}
/*
 * __txn_region_size --
 *	 Return the amount of space needed for the txn region.
 */
size_t __txn_region_size(ENV*env)
{
	DB_ENV * dbenv = env->dbenv;
	/*
	 * Make the region large enough to hold the primary transaction region
	 * structure, txn_init transaction detail structures, txn_init chunks of
	 * overhead required by the underlying shared region allocator for each
	 * chunk of memory, txn_max transaction names, at an average of 20
	 * bytes each, and 10KB for safety.
	 */
	size_t s = sizeof(DB_TXNREGION)+dbenv->tx_init*(sizeof(TXN_DETAIL)+__env_alloc_overhead()+20)+10*1024;
	return s;
}
/*
 * __txn_region_max --
 *	 Return the additional amount of space needed for the txn region.
 */
size_t __txn_region_max(ENV*env)
{
	size_t s;
	uint32 count;
	DB_ENV * dbenv = env->dbenv;
	if((count = dbenv->tx_max) == 0)
		count = DEF_MAX_TXNS;
	if(count <= dbenv->tx_init)
		return 0;
	s = (count-dbenv->tx_init)*(sizeof(TXN_DETAIL)+__env_alloc_overhead()+20);
	return s;
}
/*
 * __txn_id_set --
 *	Set the current transaction ID and current maximum unused ID (for
 *	testing purposes only).
 */
int __txn_id_set(ENV * env, uint32 cur_txnid, uint32 max_txnid)
{
	DB_TXNMGR * mgr;
	DB_TXNREGION * region;
	int ret;
	ENV_REQUIRES_CONFIG(env, env->tx_handle, "txn_id_set", DB_INIT_TXN);
	mgr = env->tx_handle;
	region = (DB_TXNREGION *)mgr->reginfo.primary;
	region->last_txnid = cur_txnid;
	region->cur_maxid = max_txnid;
	ret = 0;
	if(cur_txnid < TXN_MINIMUM) {
		__db_errx(env, DB_STR_A("4512", "Current ID value %lu below minimum", "%lu"), (ulong)cur_txnid);
		ret = EINVAL;
	}
	if(max_txnid < TXN_MINIMUM) {
		__db_errx(env, DB_STR_A("4513", "Maximum ID value %lu below minimum", "%lu"), (ulong)max_txnid);
		ret = EINVAL;
	}
	return ret;
}
/*
 * __txn_oldest_reader --
 *	 Find the oldest "read LSN" of any active transaction'
 *	 MVCC changes older than this can safely be discarded from the cache.
 */
int __txn_oldest_reader(ENV*env, DB_LSN * lsnp)
{
	DB_LSN old_lsn;
	DB_TXNMGR * mgr;
	DB_TXNREGION * region;
	TXN_DETAIL * td;
	int ret;
	if((mgr = env->tx_handle) == NULL)
		return 0;
	region = (DB_TXNREGION *)mgr->reginfo.primary;
	if((ret = __log_current_lsn_int(env, &old_lsn, NULL, NULL)) != 0)
		return ret;
	TXN_SYSTEM_LOCK(env);
	SH_TAILQ_FOREACH(td, &region->active_txn, links, __txn_detail)
	if(LOG_COMPARE(&td->read_lsn, &old_lsn) < 0)
		old_lsn = td->read_lsn;
	*lsnp = old_lsn;
	TXN_SYSTEM_UNLOCK(env);
	return 0;
}
/*
 * __txn_add_buffer --
 *	Add to the count of buffers created by the given transaction.
 */
int __txn_add_buffer(ENV*env, TXN_DETAIL * td)
{
	DB_ASSERT(env, td != NULL);
	MUTEX_LOCK(env, td->mvcc_mtx);
	DB_ASSERT(env, td->mvcc_ref < UINT32_MAX);
	++td->mvcc_ref;
	MUTEX_UNLOCK(env, td->mvcc_mtx);
	COMPQUIET(env, 0);
	return 0;
}
/*
 * __txn_remove_buffer --
 *	Remove a buffer from a transaction -- free the transaction if necessary.
 */
int __txn_remove_buffer(ENV*env, TXN_DETAIL * td, db_mutex_t hash_mtx)
{
	DB_TXNMGR * mgr;
	DB_TXNREGION * region;
	int need_free, ret;
	DB_ASSERT(env, td != NULL);
	ret = 0;
	mgr = env->tx_handle;
	region = (DB_TXNREGION *)mgr->reginfo.primary;
	MUTEX_LOCK(env, td->mvcc_mtx);
	DB_ASSERT(env, td->mvcc_ref > 0);
	/*
	 * We free the transaction detail here only if this is the last
	 * reference and td is on the list of committed snapshot transactions with active pages.
	 */
	need_free = (--td->mvcc_ref == 0) && F_ISSET(td, TXN_DTL_SNAPSHOT);
	MUTEX_UNLOCK(env, td->mvcc_mtx);
	if(need_free) {
		MUTEX_UNLOCK(env, hash_mtx);
		ret = __mutex_free(env, &td->mvcc_mtx);
		td->mvcc_mtx = MUTEX_INVALID;
		TXN_SYSTEM_LOCK(env);
		SH_TAILQ_REMOVE(&region->mvcc_txn, td, links, __txn_detail);
		STAT_DEC(env, txn, nsnapshot, region->stat.st_nsnapshot, td->txnid);
		__env_alloc_free(&mgr->reginfo, td);
		TXN_SYSTEM_UNLOCK(env);
		MUTEX_READLOCK(env, hash_mtx);
	}
	return ret;
}

#ifdef HAVE_STATISTICS
/*
 * __txn_stat_pp --
 *	DB_ENV->txn_stat pre/post processing.
 */
int __txn_stat_pp(DB_ENV * dbenv, DB_TXN_STAT ** statp, uint32 flags)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->tx_handle, "DB_ENV->txn_stat", DB_INIT_TXN);
	if((ret = __db_fchk(env, "DB_ENV->txn_stat", flags, DB_STAT_CLEAR)) != 0)
		return ret;
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__txn_stat(env, statp, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __txn_stat --
 *	ENV->txn_stat.
 */
static int __txn_stat(ENV * env, DB_TXN_STAT ** statp, uint32 flags)
{
	DB_TXNMGR * mgr;
	DB_TXNREGION * region;
	DB_TXN_STAT * stats;
	TXN_DETAIL * td;
	size_t nbytes;
	uint32 maxtxn, ndx;
	int ret;
	*statp = NULL;
	mgr = env->tx_handle;
	region = (DB_TXNREGION *)mgr->reginfo.primary;
	TXN_SYSTEM_LOCK(env);
	maxtxn = region->curtxns;
	nbytes = sizeof(DB_TXN_STAT)+sizeof(DB_TXN_ACTIVE)*maxtxn;
	if((ret = __os_umalloc(env, nbytes, &stats)) != 0) {
		TXN_SYSTEM_UNLOCK(env);
		return ret;
	}
	memcpy(stats, &region->stat, sizeof(region->stat));
	stats->st_last_txnid = region->last_txnid;
	stats->st_last_ckp = region->last_ckp;
	stats->st_time_ckp = region->time_ckp;
	stats->st_txnarray = (DB_TXN_ACTIVE *)&stats[1];
	for(ndx = 0, td = SH_TAILQ_FIRST(&region->active_txn, __txn_detail); td && ndx < maxtxn; td = SH_TAILQ_NEXT(td, links, __txn_detail), ++ndx) {
		stats->st_txnarray[ndx].txnid = td->txnid;
		if(td->parent == INVALID_ROFF)
			stats->st_txnarray[ndx].parentid = TXN_INVALID;
		else
			stats->st_txnarray[ndx].parentid = ((TXN_DETAIL *)R_ADDR(&mgr->reginfo, td->parent))->txnid;
		stats->st_txnarray[ndx].pid = td->pid;
		stats->st_txnarray[ndx].tid = td->tid;
		stats->st_txnarray[ndx].lsn = td->begin_lsn;
		stats->st_txnarray[ndx].read_lsn = td->read_lsn;
		stats->st_txnarray[ndx].mvcc_ref = td->mvcc_ref;
		stats->st_txnarray[ndx].status = td->status;
		stats->st_txnarray[ndx].xa_status = td->xa_br_status;
		stats->st_txnarray[ndx].priority = td->priority;
		if(td->status == TXN_PREPARED)
			memcpy(stats->st_txnarray[ndx].gid, td->gid, sizeof(td->gid));
		if(td->name != INVALID_ROFF) {
			strncpy(stats->st_txnarray[ndx].name, (const char *)R_ADDR(&mgr->reginfo, td->name), sizeof(stats->st_txnarray[ndx].name)-1);
			stats->st_txnarray[ndx].name[sizeof(stats->st_txnarray[ndx].name)-1] = '\0';
		}
		else
			stats->st_txnarray[ndx].name[0] = '\0';
	}
	__mutex_set_wait_info(env, region->mtx_region, &stats->st_region_wait, &stats->st_region_nowait);
	stats->st_regsize = (roff_t)mgr->reginfo.rp->size;
	if(LF_ISSET(DB_STAT_CLEAR)) {
		if(!LF_ISSET(DB_STAT_SUBSYSTEM))
			__mutex_clear(env, region->mtx_region);
		memzero(&region->stat, sizeof(region->stat));
		region->stat.st_maxtxns = region->maxtxns;
		region->stat.st_inittxns = region->inittxns;
		region->stat.st_maxnactive = region->stat.st_nactive = stats->st_nactive;
		region->stat.st_maxnsnapshot = region->stat.st_nsnapshot = stats->st_nsnapshot;
	}
	TXN_SYSTEM_UNLOCK(env);
	*statp = stats;
	return 0;
}
/*
 * __txn_stat_print_pp --
 *	DB_ENV->txn_stat_print pre/post processing.
 */
int __txn_stat_print_pp(DB_ENV * dbenv, uint32 flags)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->tx_handle, "DB_ENV->txn_stat_print", DB_INIT_TXN);
	if((ret = __db_fchk(env, "DB_ENV->txn_stat_print", flags, DB_STAT_ALL|DB_STAT_ALLOC|DB_STAT_CLEAR)) != 0)
		return ret;
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__txn_stat_print(env, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __txn_stat_print
 *	ENV->txn_stat_print method.
 */
int __txn_stat_print(ENV*env, uint32 flags)
{
	int ret;
	uint32 orig_flags = flags;
	LF_CLR(DB_STAT_CLEAR|DB_STAT_SUBSYSTEM);
	if(flags == 0 || LF_ISSET(DB_STAT_ALL)) {
		ret = __txn_print_stats(env, orig_flags);
		if(flags == 0 || ret != 0)
			return ret;
	}
	if(LF_ISSET(DB_STAT_ALL) && (ret = __txn_print_all(env, orig_flags)) != 0)
		return ret;
	return 0;
}
/*
 * __txn_print_stats --
 *	Display default transaction region statistics.
 */
static int __txn_print_stats(ENV*env, uint32 flags)
{
	DB_MSGBUF mb;
	DB_TXN_ACTIVE * txn;
	DB_TXN_STAT * sp;
	uint32 i;
	int ret;
	char buf[DB_THREADID_STRLEN], time_buf[CTIME_BUFLEN];
	DB_ENV * dbenv = env->dbenv;
	if((ret = __txn_stat(env, &sp, flags)) != 0)
		return ret;
	if(LF_ISSET(DB_STAT_ALL))
		__db_msg(env, "Default transaction region information:");
	__db_msg(env, "%lu/%lu\t%s", (ulong)sp->st_last_ckp.file, (ulong)sp->st_last_ckp.Offset_,
		sp->st_last_ckp.file == 0 ? "No checkpoint LSN" : "File/offset for last checkpoint LSN");
	if(sp->st_time_ckp == 0)
		__db_msg(env, "0\tNo checkpoint timestamp");
	else
		__db_msg(env, "%.24s\tCheckpoint timestamp", __os_ctime(&sp->st_time_ckp, time_buf));
	__db_msg(env, "%#lx\tLast transaction ID allocated", (ulong)sp->st_last_txnid);
	__db_dl(env, "Maximum number of active transactions configured", (ulong)sp->st_maxtxns);
	__db_dl(env, "Initial number of transactions configured", (ulong)sp->st_inittxns);
	__db_dl(env, "Active transactions", (ulong)sp->st_nactive);
	__db_dl(env, "Maximum active transactions", (ulong)sp->st_maxnactive);
	__db_dl(env, "Number of transactions begun", (ulong)sp->st_nbegins);
	__db_dl(env, "Number of transactions aborted", (ulong)sp->st_naborts);
	__db_dl(env, "Number of transactions committed", (ulong)sp->st_ncommits);
	__db_dl(env, "Snapshot transactions", (ulong)sp->st_nsnapshot);
	__db_dl(env, "Maximum snapshot transactions", (ulong)sp->st_maxnsnapshot);
	__db_dl(env, "Number of transactions restored", (ulong)sp->st_nrestores);
	__db_dlbytes(env, "Region size", (ulong)0, (ulong)0, (ulong)sp->st_regsize);
	__db_dl_pct(env, "The number of region locks that required waiting", (ulong)sp->st_region_wait, DB_PCT(sp->st_region_wait, sp->st_region_wait+sp->st_region_nowait), 0);
	qsort(sp->st_txnarray, sp->st_nactive, sizeof(sp->st_txnarray[0]), __txn_compare);
	__db_msg(env, "Active transactions:");
	DB_MSGBUF_INIT(&mb);
	for(i = 0; i < sp->st_nactive; ++i) {
		txn = &sp->st_txnarray[i];
		__db_msgadd(env, &mb, "\t%lx: %s; xa_status %s; pid/thread %s; begin LSN: file/offset %lu/%lu",
			(ulong)txn->txnid, __txn_status(txn), __txn_xa_status(txn), dbenv->thread_id_string(dbenv, txn->pid, txn->tid, buf), (ulong)txn->lsn.file, (ulong)txn->lsn.Offset_);
		if(txn->parentid != 0)
			__db_msgadd(env, &mb, "; parent: %lx", (ulong)txn->parentid);
		if(!IS_MAX_LSN(txn->read_lsn))
			__db_msgadd(env, &mb, "; read LSN: %lu/%lu", (ulong)txn->read_lsn.file, (ulong)txn->read_lsn.Offset_);
		if(txn->mvcc_ref != 0)
			__db_msgadd(env, &mb, "; mvcc refcount: %lu", (ulong)txn->mvcc_ref);
		if(LOCKING_ON(env))
			__db_msgadd(env, &mb, "; priority: %lu", (ulong)txn->priority);
		if(txn->name[0] != '\0')
			__db_msgadd(env, &mb, "; \"%s\"", txn->name);
		if(txn->status == TXN_PREPARE)
			__txn_gid(env, &mb, txn);
		DB_MSGBUF_FLUSH(env, &mb);
	}
	__os_ufree(env, sp);
	return 0;
}
/*
 * __txn_print_all --
 *	Display debugging transaction region statistics.
 */
static int __txn_print_all(ENV*env, uint32 flags)
{
	static const FN fn[] = {
		{ TXN_IN_RECOVERY,      "TXN_IN_RECOVERY" },
		{ 0,                    NULL }
	};
	char time_buf[CTIME_BUFLEN];
	DB_TXNMGR * mgr = env->tx_handle;
	DB_TXNREGION * region = (DB_TXNREGION *)mgr->reginfo.primary;
	TXN_SYSTEM_LOCK(env);
	__db_print_reginfo(env, &mgr->reginfo, "Transaction", flags);
	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "DB_TXNMGR handle information:");
	__mutex_print_debug_single(env, "DB_TXNMGR mutex", mgr->mutex, flags);
	__db_dl(env, "Number of transactions discarded", (ulong)mgr->n_discards);
	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "DB_TXNREGION handle information:");
	__mutex_print_debug_single(env, "DB_TXNREGION region mutex", region->mtx_region, flags);
	STAT_ULONG("Maximum number of active txns", region->maxtxns);
	STAT_HEX("Last transaction ID allocated", region->last_txnid);
	STAT_HEX("Current maximum unused ID", region->cur_maxid);
	__mutex_print_debug_single(env, "checkpoint mutex", region->mtx_ckp, flags);
	STAT_LSN("Last checkpoint LSN", &region->last_ckp);
	__db_msg(env, "%.24s\tLast checkpoint timestamp", region->time_ckp == 0 ? "0" : __os_ctime(&region->time_ckp, time_buf));
	__db_prflags(env, NULL, region->flags, fn, NULL, "\tFlags");
	__db_msg(env, "%s", DB_GLOBAL(db_line));
	TXN_SYSTEM_UNLOCK(env);
	return 0;
}

static char * __txn_status(DB_TXN_ACTIVE * txn)
{
	switch(txn->status) {
	    case TXN_ABORTED: return "aborted";
	    case TXN_COMMITTED: return "committed";
	    case TXN_NEED_ABORT: return "need abort";
	    case TXN_PREPARED: return "prepared";
	    case TXN_RUNNING: return "running";
	    default: break;
	}
	return "unknown state";
}

static char * __txn_xa_status(DB_TXN_ACTIVE * txn)
{
	switch(txn->xa_status) {
	    case TXN_XA_ACTIVE: return "xa active";
	    case TXN_XA_DEADLOCKED: return "xa deadlock";
	    case TXN_XA_IDLE: return "xa idle";
	    case TXN_XA_PREPARED: return "xa prepared";
	    case TXN_XA_ROLLEDBACK: return "xa rollback";
	    default: break;
	}
	return "no xa state";
}

static void __txn_gid(ENV*env, DB_MSGBUF * mbp, DB_TXN_ACTIVE * txn)
{
	uint32 v, * xp;
	uint i;
	int cnt;
	__db_msgadd(env, mbp, "\n\tGID:");
	for(cnt = 0, xp = (uint32 *)txn->gid, i = 0;; ) {
		memcpy(&v, xp++, sizeof(uint32));
		__db_msgadd(env, mbp, "%#lx ", (ulong)v);
		if((i += sizeof(uint32)) >= DB_GID_SIZE)
			break;
		if(++cnt == 4) {
			DB_MSGBUF_FLUSH(env, mbp);
			__db_msgadd(env, mbp, "\t\t");
			cnt = 0;
		}
	}
}

static int __txn_compare(const void * a1, const void * b1)
{
	const DB_TXN_ACTIVE * a = (const DB_TXN_ACTIVE *)a1;
	const DB_TXN_ACTIVE * b = (const DB_TXN_ACTIVE *)b1;
	if(a->txnid > b->txnid)
		return 1;
	else if(a->txnid < b->txnid)
		return -1;
	else
		return 0;
}

#else /* !HAVE_STATISTICS */

int __txn_stat_pp(DB_ENV * dbenv, DB_TXN_STAT ** statp, uint32 flags)
{
	COMPQUIET(statp, 0);
	COMPQUIET(flags, 0);

	return __db_stat_not_built(dbenv->env);
}

int __txn_stat_print_pp(DB_ENV * dbenv, uint32 flags)
{
	COMPQUIET(flags, 0);
	return __db_stat_not_built(dbenv->env);
}
#endif
//
//
//
typedef struct __txn_event TXN_EVENT;

struct __txn_event {
	TXN_EVENT_T op;
	TAILQ_ENTRY(__txn_event) links;
	union {
		struct {
			// Delayed close
			DB * dbp; 
		} c;
		struct {
			// Delayed remove
			char * name;
			uint8 * fileid;
			int inmem;
		} r;
		struct {
			// Lock event
			DB_LOCK_BASE lock;
			DB_LOCKER * locker;
			DB * dbp;
		} t;
	} u;
};

#define TXN_TOP_PARENT(txn) do { while(txn->parent) txn = txn->parent; } while(0)
//
// Creates a close event that can be added to the [so-called] commit list, so
// that we can redo a failed DB handle close once we've aborted the transaction.
//
int __txn_closeevent(ENV * env, DB_TXN * txn, DB * dbp)
{
	int ret;
	TXN_EVENT * e = NULL;
	if((ret = __os_calloc(env, 1, sizeof(TXN_EVENT), &e)) != 0)
		return ret;
	e->u.c.dbp = dbp;
	e->op = TXN_CLOSE;
	TXN_TOP_PARENT(txn);
	TAILQ_INSERT_TAIL(&txn->events, e, links);
	return 0;
}
//
// Creates a remove event that can be added to the commit list.
//
int __txn_remevent(ENV*env, DB_TXN * txn, const char * name, uint8 * fileid, int inmem)
{
	int ret;
	TXN_EVENT * e = NULL;
	if((ret = __os_calloc(env, 1, sizeof(TXN_EVENT), &e)) != 0)
		return ret;
	if((ret = __os_strdup(env, name, &e->u.r.name)) != 0)
		goto err;
	if(fileid) {
		if((ret = __os_calloc(env, 1, DB_FILE_ID_LEN, &e->u.r.fileid)) != 0) {
			__os_free(env, e->u.r.name);
			goto err;
		}
		memcpy(e->u.r.fileid, fileid, DB_FILE_ID_LEN);
	}
	e->u.r.inmem = inmem;
	e->op = TXN_REMOVE;
	TAILQ_INSERT_TAIL(&txn->events, e, links);
	return 0;
err:    
	__os_free(env, e);
	return ret;
}
//
// Remove a remove event because the remove has been superceeded,
// by a create of the same name, for example.
//
void __txn_remrem(ENV*env, DB_TXN * txn, const char * name)
{
	TXN_EVENT * next_e;
	for(TXN_EVENT * e = TAILQ_FIRST(&txn->events); e; e = next_e) {
		next_e = TAILQ_NEXT(e, links);
		if(e->op == TXN_REMOVE && strcmp(name, e->u.r.name) == 0) {
			TAILQ_REMOVE(&txn->events, e, links);
			__os_free(env, e->u.r.name);
			__os_free(env, e->u.r.fileid);
			__os_free(env, e);
		}
	}
}
/*
 * __txn_lockevent --
 *
 * Add a lockevent to the commit-queue.  The lock event indicates a locker
 * trade.
 */
int __txn_lockevent(ENV * env, DB_TXN * txn, DB * dbp, DB_LOCK * lock, DB_LOCKER * locker)
{
	int ret;
	TXN_EVENT * e = 0;
	if(!LOCKING_ON(env))
		return 0;
	if((ret = __os_calloc(env, 1, sizeof(TXN_EVENT), &e)) != 0)
		return ret;
	e->u.t.locker = locker;
	e->u.t.lock = *lock;
	e->u.t.dbp = dbp;
	e->op = TXN_TRADE;
	/* This event goes on the current transaction, not its parent. */
	TAILQ_INSERT_TAIL(&txn->events, e, links);
	dbp->cur_txn = txn;
	return 0;
}
/*
 * __txn_remlock --
 *	Remove a lock event because the locker is going away.  We can remove
 * by lock (using offset) or by locker_id (or by both).
 */
void __txn_remlock(ENV*env, DB_TXN * txn, DB_LOCK * lock, DB_LOCKER * locker)
{
	TXN_EVENT * e, * next_e;
	for(e = TAILQ_FIRST(&txn->events); e; e = next_e) {
		next_e = TAILQ_NEXT(e, links);
		if((e->op != TXN_TRADE && e->op != TXN_TRADED) || (e->u.t.lock.off != lock->off && e->u.t.locker != locker))
			continue;
		TAILQ_REMOVE(&txn->events, e, links);
		__os_free(env, e);
	}
}
/*
 * __txn_doevents --
 * Process the list of events associated with a transaction.  On commit,
 * apply the events; on abort, just toss the entries.
 */
/*
 * Trade a locker associated with a thread for one that is associated
 * only with the handle. Mark the locker so failcheck will know.
 */
#define DO_TRADE do {                                                   \
		memzero(&req, sizeof(req));                                   \
		req.lock = *(DB_LOCK *)&e->u.t.lock;                            \
		req.op = DB_LOCK_TRADE;                                         \
		t_ret = __lock_vec(env, txn->parent ? txn->parent->locker : e->u.t.locker, 0, &req, 1, 0); \
		if(t_ret == 0) {                                               \
			if(txn->parent) {                              \
				e->u.t.dbp->cur_txn = txn->parent;              \
				e->u.t.dbp->cur_locker = txn->parent->locker;   \
			} else {                                                \
				e->op = TXN_TRADED;                             \
				e->u.t.dbp->cur_locker = e->u.t.locker;         \
				F_SET(e->u.t.dbp->cur_locker, DB_LOCKER_HANDLE_LOCKER);                   \
				if(opcode != TXN_PREPARE)                      \
					e->u.t.dbp->cur_txn = NULL;             \
			}                                                       \
		} else if(t_ret == DB_NOTFOUND) \
			t_ret = 0;                                              \
		if(t_ret != 0 && ret == 0)                                     \
			ret = t_ret;                                            \
} while(0)

int __txn_doevents(ENV*env, DB_TXN * txn, int opcode, int preprocess)
{
	DB_LOCKREQ req;
	TXN_EVENT * e, * enext;
	int t_ret;
	int ret = 0;
	/*
	 * This phase only gets called if we have a phase where we
	 * release read locks.  Since not all paths will call this
	 * phase, we have to check for it below as well.  So, when
	 * we do the trade, we update the opcode of the entry so that
	 * we don't try the trade again.
	 */
	if(preprocess) {
		for(e = TAILQ_FIRST(&txn->events); e; e = enext) {
			enext = TAILQ_NEXT(e, links);
			if(e->op != TXN_TRADE || IS_WRITELOCK(e->u.t.lock.mode))
				continue;
			DO_TRADE;
			if(txn->parent) {
				TAILQ_REMOVE(&txn->events, e, links);
				TAILQ_INSERT_HEAD(&txn->parent->events, e, links);
			}
		}
		return ret;
	}
	//
	// Prepare should only cause a preprocess, since the transaction isn't over.
	//
	DB_ASSERT(env, opcode != TXN_PREPARE);
	while((e = TAILQ_FIRST(&txn->events)) != NULL) {
		TAILQ_REMOVE(&txn->events, e, links);
		/*
		 * Most deferred events should only happen on
		 * commits, not aborts or prepares.  The one exception
		 * is a close which gets done on commit and abort, but
		 * not prepare. If we're not doing operations, then we
		 * can just go free resources.
		 */
		if(opcode == TXN_ABORT && e->op != TXN_CLOSE)
			goto dofree;
		switch(e->op) {
		    case TXN_CLOSE:
			if((t_ret = __db_close(e->u.c.dbp, NULL, DB_NOSYNC)) != 0 && ret == 0)
				ret = t_ret;
			break;
		    case TXN_REMOVE:
			if(txn->parent)
				TAILQ_INSERT_TAIL(&txn->parent->events, e, links);
			else if(e->u.r.fileid) {
				if((t_ret = __memp_nameop(env, e->u.r.fileid, NULL, e->u.r.name, NULL, e->u.r.inmem)) != 0 && ret == 0)
					ret = t_ret;
			}
			else if((t_ret = __os_unlink(env, e->u.r.name, 0)) != 0 && ret == 0)
				ret = t_ret;
			break;
		    case TXN_TRADE:
			DO_TRADE;
			if(txn->parent) {
				TAILQ_INSERT_HEAD(&txn->parent->events, e, links);
				continue;
			}
		    /* Fall through */
		    case TXN_TRADED:
			/* Downgrade the lock. */
			if((t_ret = __lock_downgrade(env, (DB_LOCK *)&e->u.t.lock, DB_LOCK_READ, 0)) != 0 && ret == 0)
				ret = t_ret;
			break;
		    default:
			/* This had better never happen. */
			DB_ASSERT(env, 0);
		}
dofree:
		// Free resources here
		switch(e->op) {
		    case TXN_REMOVE:
				if(txn->parent)
					continue;
				__os_free(env, e->u.r.fileid);
				__os_free(env, e->u.r.name);
				break;
		    case TXN_TRADE:
				if(opcode == TXN_ABORT)
					e->u.t.dbp->cur_txn = NULL;
				break;
		    case TXN_CLOSE:
		    case TXN_TRADED:
		    default:
				break;
		}
		__os_free(env, e);
	}
	return ret;
}

int __txn_record_fname(ENV*env, DB_TXN * txn, FNAME * fname)
{
	DB_LOG * dblp;
	DB_TXNMGR * mgr;
	TXN_DETAIL * td;
	roff_t fname_off;
	roff_t * np, * ldbs;
	uint32 i;
	int ret;
	if((td = (TXN_DETAIL *)txn->td) == NULL)
		return 0;
	mgr = env->tx_handle;
	dblp = env->lg_handle;
	fname_off = R_OFFSET(&dblp->reginfo, fname);
	/* See if we already have a ref to this DB handle. */
	ldbs = (roff_t *)R_ADDR(&mgr->reginfo, td->log_dbs);
	for(i = 0, np = ldbs; i < td->nlog_dbs; i++, np++)
		if(*np == fname_off)
			return 0;
	if(td->nlog_slots <= td->nlog_dbs) {
		TXN_SYSTEM_LOCK(env);
		if((ret = __env_alloc(&mgr->reginfo, sizeof(roff_t)*(td->nlog_slots<<1), &np)) != 0) {
			TXN_SYSTEM_UNLOCK(env);
			return ret;
		}
		memcpy(np, ldbs, td->nlog_dbs*sizeof(roff_t));
		if(td->nlog_slots > TXN_NSLOTS)
			__env_alloc_free(&mgr->reginfo, ldbs);
		TXN_SYSTEM_UNLOCK(env);
		td->log_dbs = R_OFFSET(&mgr->reginfo, np);
		ldbs = np;
		td->nlog_slots = td->nlog_slots<<1;
	}
	ldbs[td->nlog_dbs] = fname_off;
	td->nlog_dbs++;
	fname->txn_ref++;
	return 0;
}
/*
 * __txn_dref_fnam --
 *	Either pass the fname to our parent txn or decrement the refcount
 * and close the fileid if it goes to zero.
 */
int __txn_dref_fname(ENV*env, DB_TXN * txn)
{
	uint32 i;
	int ret = 0;
	TXN_DETAIL * td = (TXN_DETAIL *)txn->td;
	if(td->nlog_dbs) {
		DB_TXNMGR * mgr = env->tx_handle;
		DB_LOG * dblp = env->lg_handle;
		TXN_DETAIL * ptd = (txn->parent ? (TXN_DETAIL *)txn->parent->td : NULL);
		roff_t * np = (roff_t *)R_ADDR(&mgr->reginfo, td->log_dbs);
		np += td->nlog_dbs-1;
		for(i = 0; i < td->nlog_dbs; i++, np--) {
			FNAME * fname = (FNAME *)R_ADDR(&dblp->reginfo, *np);
			MUTEX_LOCK(env, fname->mutex);
			if(ptd) {
				ret = __txn_record_fname(env, txn->parent, fname);
				fname->txn_ref--;
				MUTEX_UNLOCK(env, fname->mutex);
			}
			else if(fname->txn_ref == 1) {
				MUTEX_UNLOCK(env, fname->mutex);
				DB_ASSERT(env, fname->txn_ref != 0);
				ret = __dbreg_close_id_int(env, fname, DBREG_CLOSE, 0);
			}
			else {
				fname->txn_ref--;
				MUTEX_UNLOCK(env, fname->mutex);
			}
			if(ret != 0 && ret != EIO)
				break;
		}
	}
	return ret;
}
/*
 * Common removal routine.  This is called only after verifying that
 * the DB_MPOOLFILE is in the list.
 */
static void __clear_fe_watermark(DB_TXN * txn, DB * db)
{
	MPOOLFILE * mpf = db->mpf->mfp;
	mpf->fe_watermark = PGNO_INVALID;
	mpf->fe_txnid = 0U;
	mpf->fe_nlws = 0U;
	TAILQ_REMOVE(&txn->femfs, db, felink);
}
/*
 * __txn_reset_fe_watermarks
 * Reset the file extension state of MPOOLFILEs involved in this transaction.
 */
void __txn_reset_fe_watermarks(DB_TXN * txn)
{
	DB * db;
	if(txn->parent) {
		DB_ASSERT(txn->mgrp->env, TAILQ_FIRST(&txn->femfs) == NULL);
	}
	while((db = TAILQ_FIRST(&txn->femfs)))
		__clear_fe_watermark(txn, db);
}
/*
 * __txn_remove_fe_watermark
 * Remove a watermark from the transaction's list
 */
void __txn_remove_fe_watermark(DB_TXN * txn, DB * db)
{
	if(txn && F_ISSET(txn, TXN_BULK)) {
		DB * db_tmp;
		TAILQ_FOREACH(db_tmp, &txn->femfs, felink) {
			if(db_tmp == db) {
				__clear_fe_watermark(txn, db);
				break;
			}
		}
	}
}
/*
 * __txn_add_fe_watermark
 *
 * Add an entry to the transaction's list of
 * file_extension_watermarks, if warranted.  Also, set the watermark
 * page number in the MPOOLFILE.  The metadata lock associated with
 * the mfp must be held when this function is called.
 */
void __txn_add_fe_watermark(DB_TXN*txn, DB * db, db_pgno_t pgno)
{
	if(txn && F_ISSET(txn, TXN_BULK)) {
		MPOOLFILE * mfp = db->mpf->mfp;
		/* If the watermark is already set, there's nothing to do. */
		if(mfp->fe_watermark != PGNO_INVALID) {
	#ifdef DIAGNOSTIC
			DB_ASSERT(txn->mgrp->env, mfp->fe_txnid == txn->txnid);
	#endif
			return;
		}
		/* We can update MPOOLFILE because the metadata lock is held. */
		mfp->fe_watermark = pgno;
		mfp->fe_txnid = txn->txnid;
		TAILQ_INSERT_TAIL(&txn->femfs, db, felink);
	}
}
/*
 * __txn_flush_fe_files
 * For every extended file in which a log record write was skipped,
 * flush the data pages.  This is called during commit.
 */
int __txn_flush_fe_files(DB_TXN*txn)
{
	DB * db;
	int ret;
	ENV * env = txn->mgrp->env;
	DB_ASSERT(env, txn->mgrp != NULL);
	DB_ASSERT(env, env != NULL);
#ifdef DIAGNOSTIC
	DB_ASSERT(env, txn->parent == NULL);
#endif
	TAILQ_FOREACH(db, &txn->femfs, felink) {
		if(db->mpf->mfp->fe_nlws > 0 && (ret = __memp_sync_int(env, db->mpf, 0, DB_SYNC_FILE, NULL, NULL)))
			return ret;
	}
	return 0;
}
/*
 * __txn_pg_above_fe_watermark --
 *
 * Test whether there is a file extension watermark for the given
 * database, and, if so, whether the given page number is above the
 * watermark.  If this test returns true, then logging of the page's
 * update can be suppressed when the file extension/bulk loading
 * optimization is in force.
 */
int __txn_pg_above_fe_watermark(DB_TXN*txn, MPOOLFILE * mpf, db_pgno_t pgno)
{
	if(txn == NULL || (!F_ISSET(txn, TXN_BULK)) || mpf->fe_watermark == PGNO_INVALID)
		return 0;
	else {
		ENV * env = txn->mgrp->env;
		int skip = 0;
		TXN_SYSTEM_LOCK(env);
		if(((DB_TXNREGION *)env->tx_handle->reginfo.primary)->n_hotbackup > 0)
			skip = 1;
		TXN_SYSTEM_UNLOCK(env);
		if(skip)
			return 0;
		else {
			/*
			 * If the watermark is a valid page number, then the extending
			 * transaction should be the current outermost transaction.
			 */
			DB_ASSERT(txn->mgrp->env, mpf->fe_txnid == txn->txnid);
			return (mpf->fe_watermark <= pgno);
		}
	}
}
/*
 * __txn_env_create --
 *	Transaction specific initialization of the DB_ENV structure.
 */
int __txn_env_create(DB_ENV * dbenv)
{
	/*
	 * !!!
	 * Our caller has not yet had the opportunity to reset the panic
	 * state or turn off mutex locking, and so we can neither check
	 * the panic state or acquire a mutex in the DB_ENV create path.
	 */
	dbenv->tx_max = 0;
	return 0;
}
/*
 * __txn_env_destroy --
 *	Transaction specific destruction of the DB_ENV structure.
 */
void __txn_env_destroy(DB_ENV * dbenv)
{
	COMPQUIET(dbenv, 0);
}

int __txn_get_tx_max(DB_ENV * dbenv, uint32 * tx_maxp)
{
	ENV * env = dbenv->env;
	ENV_NOT_CONFIGURED(env, env->tx_handle, "DB_ENV->get_tx_max", DB_INIT_TXN);
	if(TXN_ON(env)) {
		/* Cannot be set after open, no lock required to read. */
		*tx_maxp = ((DB_TXNREGION *)env->tx_handle->reginfo.primary)->maxtxns;
	}
	else
		*tx_maxp = dbenv->tx_max;
	return 0;
}

int __txn_set_tx_max(DB_ENV * dbenv, uint32 tx_max)
{
	ENV * env = dbenv->env;
	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_tx_max");
	dbenv->tx_max = tx_max;
	return 0;
}

int __txn_get_tx_timestamp(DB_ENV * dbenv, __time64_t * timestamp)
{
	*timestamp = dbenv->tx_timestamp;
	return 0;
}
//
// Set the transaction recovery timestamp.
//
int __txn_set_tx_timestamp(DB_ENV * dbenv, __time64_t * timestamp)
{
	ENV * env = dbenv->env;
	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_tx_timestamp");
	dbenv->tx_timestamp = *timestamp;
	return 0;
}
