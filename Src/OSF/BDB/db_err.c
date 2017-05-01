/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2011 Oracle and/or its affiliates.  All rights reserved.
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
// @v9.5.5 #include "dbinc/db_am.h"
// @v9.5.5 #include "dbinc/txn.h"

static void __db_msgcall(const DB_ENV*, const char *, va_list);
static void __db_msgfile(const DB_ENV*, const char *, va_list);

/*
 * __db_fchk --
 *	General flags checking routine.
 *
 * PUBLIC: int __db_fchk __P((ENV *, const char *, uint32, uint32));
 */
int __db_fchk(ENV * env, const char * name, uint32 flags, uint32 ok_flags)
{
	return LF_ISSET(~ok_flags) ? __db_ferr(env, name, 0) : 0;
}

/*
 * __db_fcchk --
 *	General combination flags checking routine.
 *
 * PUBLIC: int __db_fcchk
 * PUBLIC:    __P((ENV *, const char *, uint32, uint32, uint32));
 */
int __db_fcchk(ENV * env, const char * name, uint32 flags, uint32 flag1, uint32 flag2)
{
	return LF_ISSET(flag1) && LF_ISSET(flag2) ? __db_ferr(env, name, 1) : 0;
}
/*
 * __db_ferr --
 *	Common flag errors.
 */
int __db_ferr(const ENV * env, const char * name, int iscombo)
{
	if(iscombo)
		__db_errx(env, DB_STR_A("0054", "illegal flag combination specified to %s", "%s"), name);
	else
		__db_errx(env, DB_STR_A("0055", "illegal flag specified to %s", "%s"), name);
	return EINVAL;
}
/*
 * __db_fnl --
 *	Common flag-needs-locking message.
 *
 * PUBLIC: int __db_fnl __P((const ENV *, const char *));
 */
int __db_fnl(const ENV * env, const char * name)
{
	__db_errx(env, DB_STR_A("0056", "%s: DB_READ_COMMITTED, DB_READ_UNCOMMITTED and DB_RMW require locking", "%s"), name);
	return EINVAL;
}
/*
 * __db_pgerr --
 *	Error when unable to retrieve a specified page.
 *
 * PUBLIC: int __db_pgerr __P((DB *, db_pgno_t, int));
 */
int __db_pgerr(DB * dbp, db_pgno_t pgno, int errval)
{
	/*
	 * Three things are certain:
	 * Death, taxes, and lost data.
	 * Guess which has occurred.
	 */
	__db_errx(dbp->env, DB_STR_A("0057", "unable to create/retrieve page %lu", "%lu"), (ulong)pgno);
	return __env_panic(dbp->env, errval);
}
/*
 * __db_pgfmt --
 *	Error when a page has the wrong format.
 *
 * PUBLIC: int __db_pgfmt __P((ENV *, db_pgno_t));
 */
int __db_pgfmt(ENV * env, db_pgno_t pgno)
{
	__db_errx(env, DB_STR_A("0058", "page %lu: illegal page type or format", "%lu"), (ulong)pgno);
	return __env_panic(env, EINVAL);
}

#ifdef DIAGNOSTIC
/*
 * __db_assert --
 *	Error when an assertion fails.  Only checked if #DIAGNOSTIC defined.
 *
 * PUBLIC: #ifdef DIAGNOSTIC
 * PUBLIC: void __db_assert __P((ENV *, const char *, const char *, int));
 * PUBLIC: #endif
 */
void __db_assert(ENV * env, const char * e, const char * file, int line)
{
	if(DB_GLOBAL(j_assert) != NULL)
		DB_GLOBAL(j_assert) (e, file, line);
	else {
		__db_errx(env, DB_STR_A("0059", "assert failure: %s/%d: \"%s\"", "%s %d %s"), file, line, e);
		__os_abort(env);
		/* NOTREACHED */
	}
}
#endif

/*
 * __env_panic_msg --
 *	Just report that someone else paniced.
 *
 * PUBLIC: int __env_panic_msg(ENV *);
 */
int __env_panic_msg(ENV * env)
{
	DB_ENV * dbenv = env->dbenv;
	int ret = DB_RUNRECOVERY;
	__db_errx(env, DB_STR("0060", "PANIC: fatal region error detected; run recovery"));
	if(dbenv->db_paniccall != NULL)                 /* Deprecated */
		dbenv->db_paniccall(dbenv, ret);
	/* Must check for DB_EVENT_REG_PANIC panic first because it is never
	 * set by itself.  If set, it means panic came from DB_REGISTER code
	 * only, otherwise it could be from many possible places in the code.
	 */
	if((env->reginfo != NULL) && (((REGENV *)env->reginfo->primary)->reg_panic))
		DB_EVENT(env, DB_EVENT_REG_PANIC, &ret);
	else
		DB_EVENT(env, DB_EVENT_PANIC, &ret);
	return ret;
}

/*
 * __env_panic --
 *	Lock out the database environment due to unrecoverable error.
 *
 * PUBLIC: int __env_panic __P((ENV *, int));
 */
int __env_panic(ENV * env, int errval)
{
	DB_ENV * dbenv = env->dbenv;
	if(env != NULL) {
		__env_panic_set(env, 1);
		__db_err(env, errval, DB_STR("0061", "PANIC"));
		if(dbenv->db_paniccall != NULL)         /* Deprecated */
			dbenv->db_paniccall(dbenv, errval);
		/* Must check for DB_EVENT_REG_PANIC first because it is never
		 * set by itself.  If set, it means panic came from DB_REGISTER
		 * code only, otherwise it could be from many possible places
		 * in the code.
		 */
		if((env->reginfo != NULL) && (((REGENV *)env->reginfo->primary)->reg_panic))
			DB_EVENT(env, DB_EVENT_REG_PANIC, &errval);
		else
			DB_EVENT(env, DB_EVENT_PANIC, &errval);
	}
#if defined(DIAGNOSTIC) && !defined(CONFIG_TEST)
	/*
	 * We want a stack trace of how this could possibly happen.
	 *
	 * Don't drop core if it's the test suite -- it's reasonable for the
	 * test suite to check to make sure that DB_RUNRECOVERY is returned
	 * under certain conditions.
	 */
	__os_abort(env);
	/* NOTREACHED */
#endif
	/*
	 * Chaos reigns within.
	 * Reflect, repent, and reboot.
	 * Order shall return.
	 */
	return DB_RUNRECOVERY;
}

/*
 * db_strerror --
 *	ANSI C strerror(3) for DB.
 *
 * EXTERN: char *db_strerror(int);
 */
char * db_strerror(int error)
{
	if(error == 0)
		return DB_STR("0062", "Successful return: 0");
	else if(error > 0) {
		char * p = strerror(error);
		return NZOR(p, __db_unknown_error(error));
	}
	else {
		/*
		* !!!
		* The Tcl API requires that some of these return strings be compared
		* against strings stored in application scripts.  So, any of these
		* errors that do not invariably result in a Tcl exception may not be
		* altered.
		*/
		switch(error) {
			case DB_BUFFER_SMALL: return DB_STR("0063", "DB_BUFFER_SMALL: User memory too small for return value");
			case DB_DONOTINDEX: return DB_STR("0064", "DB_DONOTINDEX: Secondary index callback returns null");
			case DB_FOREIGN_CONFLICT: return DB_STR("0065", "DB_FOREIGN_CONFLICT: A foreign database constraint has been violated");
			case DB_HEAP_FULL: return DB_STR("0208", "DB_HEAP_FULL: no free space in db");
			case DB_KEYEMPTY: return DB_STR("0066", "DB_KEYEMPTY: Non-existent key/data pair");
			case DB_KEYEXIST: return DB_STR("0067", "DB_KEYEXIST: Key/data pair already exists");
			case DB_LOCK_DEADLOCK: return DB_STR("0068", "DB_LOCK_DEADLOCK: Locker killed to resolve a deadlock");
			case DB_LOCK_NOTGRANTED: return DB_STR("0069", "DB_LOCK_NOTGRANTED: Lock not granted");
			case DB_LOG_BUFFER_FULL: return DB_STR("0070", "DB_LOG_BUFFER_FULL: In-memory log buffer is full");
			case DB_LOG_VERIFY_BAD: return DB_STR("0071", "DB_LOG_VERIFY_BAD: Log verification failed");
			case DB_NOSERVER: return DB_STR("0072", "DB_NOSERVER: No message dispatch call-back function has been configured");
			case DB_NOTFOUND: return DB_STR("0073", "DB_NOTFOUND: No matching key/data pair found");
			case DB_OLD_VERSION: return DB_STR("0074", "DB_OLDVERSION: Database requires a version upgrade");
			case DB_PAGE_NOTFOUND: return DB_STR("0075", "DB_PAGE_NOTFOUND: Requested page not found");
			case DB_REP_DUPMASTER: return DB_STR("0076", "DB_REP_DUPMASTER: A second master site appeared");
			case DB_REP_HANDLE_DEAD: return DB_STR("0077", "DB_REP_HANDLE_DEAD: Handle is no longer valid");
			case DB_REP_HOLDELECTION: return DB_STR("0078", "DB_REP_HOLDELECTION: Need to hold an election");
			case DB_REP_IGNORE: return DB_STR("0079", "DB_REP_IGNORE: Replication record/operation ignored");
			case DB_REP_ISPERM: return DB_STR("0080", "DB_REP_ISPERM: Permanent record written");
			case DB_REP_JOIN_FAILURE: return DB_STR("0081", "DB_REP_JOIN_FAILURE: Unable to join replication group");
			case DB_REP_LEASE_EXPIRED: return DB_STR("0082", "DB_REP_LEASE_EXPIRED: Replication leases have expired");
			case DB_REP_LOCKOUT: return DB_STR("0083", "DB_REP_LOCKOUT: Waiting for replication recovery to complete");
			case DB_REP_NEWSITE: return DB_STR("0084", "DB_REP_NEWSITE: A new site has entered the system");
			case DB_REP_NOTPERM: return DB_STR("0085", "DB_REP_NOTPERM: Permanent log record not written");
			case DB_REP_UNAVAIL: return DB_STR("0086", "DB_REP_UNAVAIL: Too few remote sites to complete operation");
			case DB_REP_WOULDROLLBACK: return DB_STR("0207", "DB_REP_WOULDROLLBACK: Client data has diverged"); /* Undocumented; C API only. */
			case DB_RUNRECOVERY: return DB_STR("0087", "DB_RUNRECOVERY: Fatal error, run database recovery");
			case DB_SECONDARY_BAD: return DB_STR("0088", "DB_SECONDARY_BAD: Secondary index inconsistent with primary");
			case DB_TIMEOUT: return DB_STR("0089", "DB_TIMEOUT: Operation timed out");
			case DB_VERIFY_BAD: return DB_STR("0090", "DB_VERIFY_BAD: Database verification failed");
			case DB_VERSION_MISMATCH: return DB_STR("0091", "DB_VERSION_MISMATCH: Database environment version mismatch");
			default:
			break;
		}
		return __db_unknown_error(error);
	}
}
/*
 * __db_unknown_error --
 *	Format an unknown error value into a static buffer.
 *
 * PUBLIC: char *__db_unknown_error(int);
 */
char * __db_unknown_error(int error)
{
	/*
	 * !!!
	 * Room for a 64-bit number + slop.  This buffer is only used
	 * if we're given an unknown error number, which should never
	 * happen.
	 *
	 * We're no longer thread-safe if it does happen, but the worst
	 * result is a corrupted error string because there will always
	 * be a trailing nul byte since the error buffer is nul filled
	 * and longer than any error message.
	 */
	snprintf(DB_GLOBAL(error_buf), sizeof(DB_GLOBAL(error_buf)), DB_STR_A("0092", "Unknown error: %d", "%d"), error);
	return DB_GLOBAL(error_buf);
}
/*
 * __db_syserr --
 *	Standard error routine.
 *
 * PUBLIC: void __db_syserr __P((const ENV *, int, const char *, ...))
 * PUBLIC:    __attribute__ ((__format__ (__printf__, 3, 4)));
 */
void __db_syserr(const ENV * env, int error, const char * fmt, ...)
{
	DB_ENV * dbenv = env ? env->dbenv : 0;
	/*
	 * The same as DB->err, except we don't default to writing to stderr
	 * after any output channel has been configured, and we use a system-
	 * specific function to translate errors to strings.
	 */
	DB_REAL_ERR(dbenv, error, DB_ERROR_SYSTEM, 0, fmt);
}
/*
 * __db_err --
 *	Standard error routine.
 *
 * PUBLIC: void __db_err __P((const ENV *, int, const char *, ...))
 * PUBLIC:    __attribute__ ((__format__ (__printf__, 3, 4)));
 */
void __db_err(const ENV * env, int error, const char * fmt, ...)
{
	DB_ENV * dbenv = env ? env->dbenv : 0;
	/*
	 * The same as DB->err, except we don't default to writing to stderr
	 * once an output channel has been configured.
	 */
	DB_REAL_ERR(dbenv, error, DB_ERROR_SET, 0, fmt);
}
/*
 * __db_errx --
 *	Standard error routine.
 *
 * PUBLIC: void __db_errx __P((const ENV *, const char *, ...))
 * PUBLIC:    __attribute__ ((__format__ (__printf__, 2, 3)));
 */
void __db_errx(const ENV * env, const char * fmt, ...)
{
	/*
	 * The same as DB->errx, except we don't default to writing to stderr
	 * once an output channel has been configured.
	 */
	DB_REAL_ERR((env ? env->dbenv : 0), 0, DB_ERROR_NOT_SET, 0, fmt);
}
/*
 * __db_errcall --
 *	Do the error message work for callback functions.
 *
 * PUBLIC: void __db_errcall
 * PUBLIC:    __P((const DB_ENV *, int, db_error_set_t, const char *, va_list));
 */
void __db_errcall(const DB_ENV*dbenv, int error, db_error_set_t error_set, const char * fmt, va_list ap)
{
	char buf[2048];         /* !!!: END OF THE STACK DON'T TRUST SPRINTF. */
	char sysbuf[1024];      /* !!!: END OF THE STACK DON'T TRUST SPRINTF. */
	char * p = buf;
	if(fmt != NULL)
		p += vsnprintf(buf, sizeof(buf), fmt, ap);
	if(error_set != DB_ERROR_NOT_SET)
		p += snprintf(p, sizeof(buf)-(size_t)(p-buf), ": %s", error_set == DB_ERROR_SET ? db_strerror(error) : __os_strerror(error, sysbuf, sizeof(sysbuf)));
	dbenv->db_errcall(dbenv, dbenv->db_errpfx, buf);
}

/*
 * __db_errfile --
 *	Do the error message work for FILE *s.
 *
 * PUBLIC: void __db_errfile
 * PUBLIC:    __P((const DB_ENV *, int, db_error_set_t, const char *, va_list));
 */
void __db_errfile(const DB_ENV*dbenv, int error, db_error_set_t error_set, const char * fmt, va_list ap)
{
	char sysbuf[1024];      /* !!!: END OF THE STACK DON'T TRUST SPRINTF. */
	FILE * fp = (!dbenv || !dbenv->db_errfile) ? stderr : dbenv->db_errfile;
	int need_sep = 0;
	if(dbenv && dbenv->db_errpfx) {
		fprintf(fp, "%s", dbenv->db_errpfx);
		need_sep = 1;
	}
	if(!isempty(fmt)) {
		if(need_sep)
			fprintf(fp, ": ");
		need_sep = 1;
		vfprintf(fp, fmt, ap);
	}
	if(error_set != DB_ERROR_NOT_SET)
		fprintf(fp, "%s%s", need_sep ? ": " : "", error_set == DB_ERROR_SET ? db_strerror(error) : __os_strerror(error, sysbuf, sizeof(sysbuf)));
	fprintf(fp, "\n");
	fflush(fp);
}

/*
 * __db_msgadd --
 *	Aggregate a set of strings into a buffer for the callback API.
 *
 * PUBLIC: void __db_msgadd __P((ENV *, DB_MSGBUF *, const char *, ...))
 * PUBLIC:    __attribute__ ((__format__ (__printf__, 3, 4)));
 */
void __db_msgadd(ENV * env, DB_MSGBUF * mbp, const char * fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	__db_msgadd_ap(env, mbp, fmt, ap);
	va_end(ap);
}
/*
 * __db_msgadd_ap --
 *	Aggregate a set of strings into a buffer for the callback API.
 *
 * PUBLIC: void __db_msgadd_ap
 * PUBLIC:     __P((ENV *, DB_MSGBUF *, const char *, va_list));
 */
void __db_msgadd_ap(ENV*env, DB_MSGBUF * mbp, const char * fmt, va_list ap)
{
	char buf[2048];         /* !!!: END OF THE STACK DON'T TRUST SPRINTF. */
	size_t len = (size_t)vsnprintf(buf, sizeof(buf), fmt, ap);
	/*
	 * There's a heap buffer in the ENV handle we use to aggregate the
	 * message chunks.  We maintain a pointer to the buffer, the next slot
	 * to be filled in in the buffer, and a total buffer length.
	 */
	size_t olen = (size_t)(mbp->cur-mbp->buf);
	if(olen+len >= mbp->len) {
		if(__os_realloc(env, mbp->len+len+256, &mbp->buf))
			return;
		mbp->len += (len+256);
		mbp->cur = mbp->buf+olen;
	}
	memcpy(mbp->cur, buf, len+1);
	mbp->cur += len;
}

/*
 * __db_msg --
 *	Standard DB stat message routine.
 *
 * PUBLIC: void __db_msg __P((const ENV *, const char *, ...))
 * PUBLIC:    __attribute__ ((__format__ (__printf__, 2, 3)));
 */
void __db_msg(const ENV * env, const char * fmt, ...)
{
	DB_ENV * dbenv = env ? env->dbenv : 0;
	DB_REAL_MSG(dbenv, fmt);
}

/*
 * __db_repmsg --
 *	Replication system message routine.
 *
 * PUBLIC: void __db_repmsg __P((const ENV *, const char *, ...))
 * PUBLIC:    __attribute__ ((__format__ (__printf__, 2, 3)));
 */
void __db_repmsg(const ENV * env, const char * fmt, ...)
{
	va_list ap;
	char buf[2048];         /* !!!: END OF THE STACK DON'T TRUST SPRINTF. */
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	__rep_msg(env, buf);
	va_end(ap);
}

/*
 * __db_msgcall --
 *	Do the message work for callback functions.
 */
static void __db_msgcall(const DB_ENV * dbenv, const char * fmt, va_list ap)
{
	char buf[2048];         /* !!!: END OF THE STACK DON'T TRUST SPRINTF. */
	vsnprintf(buf, sizeof(buf), fmt, ap);
	dbenv->db_msgcall(dbenv, buf);
}
/*
 * __db_msgfile --
 *	Do the message work for FILE *s.
 */
static void __db_msgfile(const DB_ENV * dbenv, const char * fmt, va_list ap)
{
	FILE * fp = (dbenv == NULL || dbenv->db_msgfile == NULL) ? stdout : dbenv->db_msgfile;
	vfprintf(fp, fmt, ap);
	fprintf(fp, "\n");
	fflush(fp);
}

/*
 * __db_unknown_flag -- report internal error
 *
 * PUBLIC: int __db_unknown_flag __P((ENV *, char *, uint32));
 */
int __db_unknown_flag(ENV * env, char * routine, uint32 flag)
{
	__db_errx(env, DB_STR_A("0093", "%s: Unknown flag: %#x", "%s %#x"), routine, (uint)flag);
#ifdef DIAGNOSTIC
	__os_abort(env);
	/* NOTREACHED */
#endif
	return EINVAL;
}

/*
 * __db_unknown_type -- report internal database type error
 *
 * PUBLIC: int __db_unknown_type __P((ENV *, char *, DBTYPE));
 */
int __db_unknown_type(ENV * env, char * routine, DBTYPE type)
{
	__db_errx(env, DB_STR_A("0094", "%s: Unexpected database type: %s", "%s %s"), routine, __db_dbtype_to_string(type));
#ifdef DIAGNOSTIC
	__os_abort(env);
	/* NOTREACHED */
#endif
	return EINVAL;
}
/*
 * __db_unknown_path -- report unexpected database code path error.
 *
 * PUBLIC: int __db_unknown_path __P((ENV *, char *));
 */
int __db_unknown_path(ENV * env, char * routine)
{
	__db_errx(env, DB_STR_A("0095", "%s: Unexpected code path error", "%s"), routine);
#ifdef DIAGNOSTIC
	__os_abort(env);
	/* NOTREACHED */
#endif
	return EINVAL;
}

/*
 * __db_check_txn --
 *	Check for common transaction errors.
 *
 * PUBLIC: int __db_check_txn __P((DB *, DB_TXN *, DB_LOCKER *, int));
 */
int __db_check_txn(DB * dbp, DB_TXN * txn, DB_LOCKER * assoc_locker, int read_op)
{
	int related, ret;
	ENV * env = dbp->env;
	/*
	 * If we are in recovery or aborting a transaction, then we
	 * don't need to enforce the rules about dbp's not allowing
	 * transactional operations in non-transactional dbps and
	 * vica-versa.  This happens all the time as the dbp during
	 * an abort may be transactional, but we undo operations
	 * outside a transaction since we're aborting.
	 */
	if(IS_RECOVERING(env) || F_ISSET(dbp, DB_AM_RECOVER))
		return 0;
	/*
	 * Check for common transaction errors:
	 *	an operation on a handle whose open commit hasn't completed.
	 *	a transaction handle in a non-transactional environment
	 *	a transaction handle for a non-transactional database
	 */
	if(!read_op && txn != NULL && F_ISSET(txn, TXN_READONLY)) {
		__db_errx(env, DB_STR("0096", "Read-only transaction cannot be used for an update"));
		return EINVAL;
	}
	else if(txn == NULL || F_ISSET(txn, TXN_PRIVATE)) {
		if(dbp->cur_locker != NULL && dbp->cur_locker->id >= TXN_MINIMUM)
			goto open_err;
		if(!read_op && F_ISSET(dbp, DB_AM_TXN)) {
			__db_errx(env, DB_STR("0097", "Transaction not specified for a transactional database"));
			return EINVAL;
		}
	}
	else if(F_ISSET(txn, TXN_FAMILY)) {
		/*
		 * Family transaction handles can be passed to any method,
		 * since they only determine locker IDs.
		 */
		return 0;
	}
	else {
		if(!TXN_ON(env))
			return __db_not_txn_env(env);
		if(!F_ISSET(dbp, DB_AM_TXN)) {
			__db_errx(env, DB_STR("0098", "Transaction specified for a non-transactional database"));
			return EINVAL;
		}
		if(F_ISSET(txn, TXN_DEADLOCK))
			return __db_txn_deadlock_err(env, txn);
		if(dbp->cur_locker != NULL && dbp->cur_locker->id >= TXN_MINIMUM && dbp->cur_locker->id != txn->txnid) {
			if((ret = __lock_locker_same_family(env, dbp->cur_locker, txn->locker, &related)) != 0)
				return ret;
			if(!related)
				goto open_err;
		}
	}
	/*
	 * If dbp->associate_locker is not NULL, that means we're in
	 * the middle of a DB->associate with DB_CREATE (i.e., a secondary index
	 * creation).
	 *
	 * In addition to the usual transaction rules, we need to lock out
	 * non-transactional updates that aren't part of the associate (and
	 * thus are using some other locker ID).
	 *
	 * Transactional updates should simply block;  from the time we
	 * decide to build the secondary until commit, we'll hold a write
	 * lock on all of its pages, so it should be safe to attempt to update
	 * the secondary in another transaction (presumably by updating the
	 * primary).
	 */
	if(!read_op && dbp->associate_locker && txn && dbp->associate_locker != assoc_locker) {
		__db_errx(env, DB_STR("0099", "Operation forbidden while secondary index is being created"));
		return EINVAL;
	}
	/*
	 * Check the txn and dbp are from the same env.
	 */
	if(txn && env != txn->mgrp->env) {
		__db_errx(env, DB_STR("0100", "Transaction and database from different environments"));
		return EINVAL;
	}
	return 0;
open_err:
	__db_errx(env, DB_STR("0101", "Transaction that opened the DB handle is still active"));
	return EINVAL;
}

/*
 * __db_txn_deadlock_err --
 *	Transaction has allready been deadlocked.
 *
 * PUBLIC: int __db_txn_deadlock_err __P((ENV *, DB_TXN *));
 */
int __db_txn_deadlock_err(ENV * env, DB_TXN * txn)
{
	const char * name = NULL;
	__txn_get_name(txn, &name);
	__db_errx(env, DB_STR_A("0102", "%s%sprevious transaction deadlock return not resolved",
		"%s %s"), name == NULL ? "" : name, name == NULL ? "" : ": ");
	return EINVAL;
}

/*
 * __db_not_txn_env --
 *	DB handle must be in an environment that supports transactions.
 *
 * PUBLIC: int __db_not_txn_env(ENV *);
 */
int __db_not_txn_env(ENV * env)
{
	__db_errx(env, DB_STR("0103", "DB environment not configured for transactions"));
	return EINVAL;
}

/*
 * __db_rec_toobig --
 *	Fixed record length exceeded error message.
 *
 * PUBLIC: int __db_rec_toobig __P((ENV *, uint32, uint32));
 */
int __db_rec_toobig(ENV * env, uint32 data_len, uint32 fixed_rec_len)
{
	__db_errx(env, DB_STR_A("0104", "%lu larger than database's maximum record length %lu", "%lu %lu"), (ulong)data_len, (ulong)fixed_rec_len);
	return EINVAL;
}

/*
 * __db_rec_repl --
 *	Fixed record replacement length error message.
 *
 * PUBLIC: int __db_rec_repl __P((ENV *, uint32, uint32));
 */
int __db_rec_repl(ENV * env, uint32 data_size, uint32 data_dlen)
{
	__db_errx(env, DB_STR_A("0105", "Record length error: replacement length %lu differs from replaced length %lu", "%lu %lu"), (ulong)data_size, (ulong)data_dlen);
	return EINVAL;
}

#if defined(DIAGNOSTIC) || defined(DEBUG_ROP)  || defined(DEBUG_WOP)
/*
 * __dbc_logging --
 *	In DIAGNOSTIC mode, check for bad replication combinations.
 *
 * PUBLIC: int __dbc_logging(DBC *);
 */
int __dbc_logging(DBC * dbc)
{
	ENV * env = dbc->env;
	DB_REP * db_rep = env->rep_handle;
	int ret = LOGGING_ON(env) && !F_ISSET(dbc, DBC_RECOVER) && !IS_REP_CLIENT(env);
	/*
	 * If we're not using replication or running recovery, return.
	 */
	if(db_rep == NULL || F_ISSET(dbc, DBC_RECOVER))
		return ret;
 #ifndef DEBUG_ROP
	/*
	 *  Only check when DEBUG_ROP is not configured.  People often do
	 * non-transactional reads, and debug_rop is going to write
	 * a log record.
	 */
	{
		REP * rep = db_rep->region;
		/*
		 * If we're a client and not running recovery or non durably, error.
		 */
		if(IS_REP_CLIENT(env) && !F_ISSET(dbc->dbp, DB_AM_NOT_DURABLE)) {
			__db_errx(env, DB_STR("0106", "dbc_logging: Client update"));
			goto err;
		}
  #ifndef DEBUG_WOP
		/*
		 * If DEBUG_WOP is enabled, then we'll generate debugging log records
		 * that are non-transactional.  This is OK.
		 */
		if(IS_REP_MASTER(env) && dbc->txn == NULL && !F_ISSET(dbc->dbp, DB_AM_NOT_DURABLE)) {
			__db_errx(env, DB_STR("0107", "Dbc_logging: Master non-txn update"));
			goto err;
		}
  #endif
		if(0) {
err:
			__db_errx(env, DB_STR_A("0108", "Rep: flags 0x%lx msg_th %lu", "%lx %lu"), (ulong)rep->flags, (ulong)rep->msg_th);
			__db_errx(env, DB_STR_A("0109", "Rep: handle %lu, opcnt %lu", "%lu %lu"), (ulong)rep->handle_cnt, (ulong)rep->op_cnt);
			__os_abort(env);
			/* NOTREACHED */
		}
	}
 #endif
	return ret;
}
#endif

/*
 * __db_check_lsn --
 *	Display the log sequence error message.
 *
 * PUBLIC: int __db_check_lsn __P((ENV *, DB_LSN *, DB_LSN *));
 */
int __db_check_lsn(ENV * env, DB_LSN * lsn, DB_LSN * prev)
{
	__db_errx(env, DB_STR_A("0110", "Log sequence error: page LSN %lu %lu; previous LSN %lu %lu",
		"%lu %lu %lu %lu"), (ulong)(lsn)->file, (ulong)(lsn)->offset, (ulong)(prev)->file, (ulong)(prev)->offset);
	return EINVAL;
}

/*
 * __db_rdonly --
 *	Common readonly message.
 * PUBLIC: int __db_rdonly __P((const ENV *, const char *));
 */
int __db_rdonly(const ENV * env, const char * name)
{
	__db_errx(env, DB_STR_A("0111", "%s: attempt to modify a read-only database", "%s"), name);
	return EACCES;
}

/*
 * __db_space_err --
 *	Common out of space message.
 * PUBLIC: int __db_space_err __P((const DB *));
 */
int __db_space_err(const DB * dbp)
{
	__db_errx(dbp->env, DB_STR_A("0112", "%s: file limited to %lu pages", "%s %lu"), dbp->fname, (ulong)dbp->mpf->mfp->maxpgno);
	return ENOSPC;
}

/*
 * __db_failed --
 *	Common failed thread  message.
 *
 * PUBLIC: int __db_failed __P((const ENV *,
 * PUBLIC:      const char *, pid_t, db_threadid_t));
 */
int __db_failed(const ENV * env, const char * msg, pid_t pid, db_threadid_t tid)
{
	char buf[DB_THREADID_STRLEN];
	DB_ENV * dbenv = env->dbenv;
	__db_errx(env, DB_STR_A("0113", "Thread/process %s failed: %s", "%s %s"), dbenv->thread_id_string(dbenv, pid, tid, buf),  msg);
	return DB_RUNRECOVERY;
}
