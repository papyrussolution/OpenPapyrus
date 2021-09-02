/* DO NOT EDIT: automatically built by dist/s_windows. 
 * See the file LICENSE for redistribution information.
 * Copyright (c) 1996, 2011 Oracle and/or its affiliates.  All rights reserved.
 * $Id$
 */
#ifndef _DB_INT_H_
#define	_DB_INT_H_

/*******************************************************
 * Berkeley DB ANSI/POSIX include files.
 *******************************************************/
#ifdef HAVE_SYSTEM_INCLUDE_FILES
	#include <sys/types.h>
	#ifdef DIAG_MVCC
		#include <sys/mman.h>
	#endif
	#include <sys/stat.h>
	#if defined(HAVE_REPLICATION_THREADS)
		#ifdef HAVE_SYS_SELECT_H
			#include <sys/select.h>
		#endif
		#ifdef HAVE_VXWORKS
			#include <selectLib.h>
		#endif
	#endif
	#if TIME_WITH_SYS_TIME
		#include <sys/time.h>
		#include <time.h>
	#else
		#if HAVE_SYS_TIME_H
			#include <sys/time.h>
		#else
			#include <time.h>
		#endif
	#endif
	#ifdef HAVE_VXWORKS
		#include <net/uio.h>
	#else
		#include <sys/uio.h>
	#endif
	#if defined(HAVE_REPLICATION_THREADS)
		#ifdef HAVE_SYS_SOCKET_H
			#include <sys/socket.h>
		#endif
		#include <netinet/in.h>
		#include <netdb.h>
		#include <arpa/inet.h>
	#endif
	#if defined(STDC_HEADERS) || defined(__cplusplus)
		#include <stdarg.h>
	#else
		#include <varargs.h>
	#endif
	//#include <ctype.h>
	//#include <errno.h>
	#include <fcntl.h>
	//#include <limits.h>
	#include <signal.h>
	#include <stddef.h>
	//#include <string.h>
	#include <unistd.h>
#endif /* !HAVE_SYSTEM_INCLUDE_FILES */
#ifdef DB_WIN32
	#include "dbinc/win_db.h"
#endif
#ifdef HAVE_DBM
	#undef	DB_DBM_HSEARCH
	#define	DB_DBM_HSEARCH 1
#endif
// @sobolev #include "db.h"
#include <BerkeleyDB.h> // @sobolev
#include "clib_port.h"
#include "dbinc/queue.h"
#include "dbinc/shqueue.h"
#include "dbinc/perfmon.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * The Windows compiler needs to be told about structures that are available
 * outside a dll.
 */
#if defined(DB_WIN32) && defined(_MSC_VER) && !defined(DB_CREATE_DLL) && !defined(_LIB)
	#define	__DB_IMPORT __declspec(dllimport)
#else
	#define	__DB_IMPORT
#endif

/*******************************************************
 * Forward structure declarations.
 *******************************************************/
struct __db_commit_info; 
struct __db_reginfo_t;	
struct __db_txnhead;	
struct __db_txnlist;	
struct __vrfy_childinfo;
struct __vrfy_dbinfo;   
struct __vrfy_pageinfo; 
typedef struct __db_commit_info DB_COMMIT_INFO;
typedef struct __db_reginfo_t REGINFO;
typedef struct __db_txnhead DB_TXNHEAD;
typedef struct __db_txnlist DB_TXNLIST;
typedef struct __vrfy_childinfo VRFY_CHILDINFO;
typedef struct __vrfy_dbinfo VRFY_DBINFO;
typedef struct __vrfy_pageinfo VRFY_PAGEINFO;

struct __db_log_verify_info;
struct __txn_verify_info;
struct __lv_filereg_info;
struct __lv_ckp_info;
struct __lv_timestamp_info;
typedef struct __db_log_verify_info DB_LOG_VRFY_INFO;
typedef struct __txn_verify_info VRFY_TXN_INFO;
typedef struct __lv_filereg_info VRFY_FILEREG_INFO;
typedef struct __lv_filelife VRFY_FILELIFE;
typedef struct __lv_ckp_info VRFY_CKP_INFO;
typedef struct __lv_timestamp_info VRFY_TIMESTAMP_INFO;

/*
 * TXNINFO_HANDLER --
 *	Callback function pointer type for __iterate_txninfo.
 */
typedef int (*TXNINFO_HANDLER)(DB_LOG_VRFY_INFO *, VRFY_TXN_INFO *, void *);
typedef SH_TAILQ_HEAD(__hash_head) DB_HASHTAB;

/*******************************************************
 * General purpose constants and macros.
 *******************************************************/
/* @sobolev
#undef	FALSE
#define	FALSE		0
#undef	TRUE
#define	TRUE		(!FALSE)
*/
#define	NS_PER_MS	1000000		/* Nanoseconds in a millisecond */
#define	NS_PER_US	1000		/* Nanoseconds in a microsecond */
#define	NS_PER_SEC	1000000000	/* Nanoseconds in a second */
#define	US_PER_MS	1000		/* Microseconds in a millisecond */
#define	US_PER_SEC	1000000		/* Microseconds in a second */
#define	MS_PER_SEC	1000		/* Milliseconds in a second */

#define	RECNO_OOB	0		/* Illegal record number. */

/*
 * Define a macro which has no runtime effect, yet avoids triggering empty
 * statement compiler warnings. Use it as the text of conditionally-null macros.
 */
#define	NOP_STATEMENT	do { } while(0)

// Test for a power-of-two (tests true for zero, which doesn't matter here). 
// @sobolev (replaced with IsPowerOfTwo) #define POWER_OF_TWO(x)	(((x) & ((x) - 1)) == 0)

/* Test for valid page sizes. */
#define	DB_MIN_PGSIZE	0x000200	/* Minimum page size (512). */
#define	DB_MAX_PGSIZE	0x010000	/* Maximum page size (65536). */
#define	IS_VALID_PAGESIZE(x)  (/*POWER_OF_TWO*/IsPowerOfTwo(x) && (x) >= DB_MIN_PGSIZE && ((x) <= DB_MAX_PGSIZE))
/* Minimum number of pages cached, by default. */
#define	DB_MINPAGECACHE	16
/*
 * If we are unable to determine the underlying filesystem block size, use
 * 8K on the grounds that most OS's use less than 8K for a VM page size.
 */
#define	DB_DEF_IOSIZE	(8 * 1024)
/* Align an integer to a specific boundary. */
#undef	DB_ALIGN
#define	DB_ALIGN(v, bound) (((v) + (bound) - 1) & ~(((uintmax_t)(bound)) - 1))
/* Increment a pointer to a specific boundary. */
#undef	ALIGNP_INC
#define	ALIGNP_INC(p, bound) (void *)(((uintptr_t)(p) + (bound) - 1) & ~(((uintptr_t)(bound)) - 1))

/*
 * DB_ALIGN8 adjusts structure alignments to make sure shared structures have
 * fixed size and filed offset on both 32bit and 64bit platforms when
 * HAVE_MIXED_SIZE_ADDRESSING is defined.
 */
#ifdef HAVE_MIXED_SIZE_ADDRESSING
	#define DB_ALIGN8 __declspec(align(8))
#else
	#define DB_ALIGN8
#endif
/*
 * Berkeley DB uses the va_copy macro from C99, not all compilers include
 * it, so add a dumb implementation compatible with pre C99 implementations.
 */
#ifndef	va_copy
	#define	va_copy(d, s)	((d) = (s))
#endif
//
// Print an address as a ulong (a ulong is the largest type we can print
// portably).  Most 64-bit systems have made longs 64-bits, so this should work.
//
#define	P_TO_ULONG(p)	((ulong)(uintptr_t)(p))
/*
 * Convert a pointer to an integral value.
 *
 * The static_cast<uint16>(uintptr_t) cast avoids warnings: the (uintptr_t) cast
 * converts the value to an integral type, and the (uint16) cast converts
 * it to a small integral type so we don't get complaints when we assign the
 * final result to an integral type smaller than uintptr_t.
 */
#define	P_TO_UINT32(p)	((uint32)(uintptr_t)(p))
#define	P_TO_UINT16(p)	(static_cast<uint16>((uintptr_t)(p)))
#define	P_TO_ROFF(p)	((roff_t)(uintptr_t)(p))
// The converse of P_TO_ROFF() above. 
#define	ROFF_TO_P(roff)	((void *)(uintptr_t)(roff))
/*
 * There are several on-page structures that are declared to have a number of
 * fields followed by a variable length array of items.  The structure size
 * without including the variable length array or the address of the first of
 * those elements can be found using SSZ.
 *
 * This macro can also be used to find the offset of a structure element in a
 * structure.  This is used in various places to copy structure elements from
 * unaligned memory references, e.g., pointers into a packed page.
 *
 * There are two versions because compilers object if you take the address of an array.
 */
#undef	SSZ
#define	SSZ(name, field)  P_TO_UINT16(&(((name *)0)->field))
#undef	SSZA
#define	SSZA(name, field) P_TO_UINT16(&(((name *)0)->field[0]))

/* Structure used to print flag values. */
typedef struct __fn {
	uint32 mask;			/* Flag value. */
	const char *name;		/* Flag name. */
} FN;

/* Set, clear and test flags. */
#define	FLD_CLR(fld, f)        (fld) &= ~(f)
#define	FLD_ISSET(fld, f)      ((fld) & (f))
#define	FLD_SET(fld, f)        (fld) |= (f)
#define	F_CLR(p, f)            (p)->flags &= ~(f)
#define	F_ISSET(p, f)          ((p)->flags & (f))
#define	F_SET(p, f)            (p)->flags |= (f)
#define	LF_CLR(f)              ((flags) &= ~(f))
#define	LF_ISSET(f)            ((flags) & (f))
#define	LF_SET(f)              ((flags) |= (f))
// 
// Calculate a percentage.  The values can overflow 32-bit integer arithmetic so we use floating point.
// 
// When calculating a bytes-vs-page size percentage, we're getting the inverse
// of the percentage in all cases, that is, we want 100 minus the percentage we calculate.
//
#define	DB_PCT(v, total)              ((int)((total) == 0 ? 0 : ((double)(v) * 100) / (total)))
#define	DB_PCT_PG(v, total, pgsize)   ((int)((total) == 0 ? 0 : 100 - ((double)(v) * 100) / (((double)total) * (pgsize))))
/*
 * Statistics update shared memory and so are expensive -- don't update the
 * values unless we're going to display the results.
 * When performance monitoring is enabled, the changed value can be published
 * (via DTrace or SystemTap) along with another associated value or two.
 */
#undef	STAT
#ifdef	HAVE_STATISTICS
#define	STAT(x)	x
#define	STAT_ADJUST(env, cat, subcat, val, amount, id)            do { (val) += (amount); STAT_PERFMON2((env), cat, subcat, (val), (id)); } while(0)
#define	STAT_ADJUST_VERB(env, cat, subcat, val, amount, id1, id2) do { (val) += (amount); STAT_PERFMON3((env), cat, subcat, (val), (id1), (id2)); } while(0)
#define	STAT_INC(env, cat, subcat, val, id)            STAT_ADJUST(env, cat, subcat, (val), 1, (id))
#define	STAT_INC_VERB(env, cat, subcat, val, id1, id2) STAT_ADJUST_VERB((env), cat, subcat, (val), 1, (id1), (id2))
// 
// STAT_DEC() subtracts one rather than adding (-1) with STAT_ADJUST(); the
// latter might generate a compilation warning for an unsigned value.
// 
#define	STAT_DEC(env, cat, subcat, val, id) do { (val)--; STAT_PERFMON2((env), cat, subcat, (val), (id)); } while(0)
/* N.B.: Add a verbose version of STAT_DEC() when needed. */

#define	STAT_SET(env, cat, subcat, val, newval, id)            do { (val) = (newval); STAT_PERFMON2((env), cat, subcat, (val), (id)); } while(0)
#define	STAT_SET_VERB(env, cat, subcat, val, newval, id1, id2) do { (val) = (newval); STAT_PERFMON3((env), cat, subcat, (val), (id1), (id2)); } while(0)
#else
#define	STAT(x)							NOP_STATEMENT
#define	STAT_ADJUST(env, cat, subcat, val, amt, id)		NOP_STATEMENT
#define	STAT_ADJUST_VERB(env, cat, subcat, val, amt, id1, id2)	NOP_STATEMENT
#define	STAT_INC(env, cat, subcat, val, id)			NOP_STATEMENT
#define	STAT_INC_VERB(env, cat, subcat, val, id1, id2)		NOP_STATEMENT
#define	STAT_DEC(env, cat, subcat, val, id)			NOP_STATEMENT
#define	STAT_SET(env, cat, subcat, val, newval, id)		NOP_STATEMENT
#define	STAT_SET_VERB(env, cat, subcat, val, newval, id1, id2)	NOP_STATEMENT
#endif

/*
 * These macros are used when an error condition is first noticed. They allow
 * one to be notified (via e.g. DTrace, SystemTap, ...) when an error occurs
 * deep inside DB, rather than when it is returned back through the API.
 *
 * The second actual argument to these is the second part of the error or
 * warning event name. They work when 'errcode' is a symbolic name e.g.
 * EINVAL or DB_LOCK_DEALOCK, not a variable.  Noticing system call failures
 * would be handled by tracing on syscall exit; when e.g., it returns < 0.
 */
#define	ERR_ORIGIN(env, errcode)          (PERFMON0(env, error, errcode), errcode)
#define	ERR_ORIGIN_MSG(env, errcode, msg) (PERFMON1(env, error, errcode, msg), errcode)
#define	WARNING_ORIGIN(env, errcode)      (PERFMON0(env, warning, errcode), errcode)

/*
 * Structure used for callback message aggregation.
 *
 * Display values in XXX_stat_print calls.
 */
typedef struct __db_msgbuf {
	char *buf;			/* Heap allocated buffer. */
	char *cur;			/* Current end of message. */
	size_t len;			/* Allocated length of buffer. */
} DB_MSGBUF;
#define	DB_MSGBUF_INIT(a) do {						\
	(a)->buf = (a)->cur = NULL;					\
	(a)->len = 0;							\
} while(0)
#define	DB_MSGBUF_FLUSH(env, a) do {					\
	if((a)->buf != NULL) {						\
		if((a)->cur != (a)->buf)				\
			__db_msg(env, "%s", (a)->buf);			\
		__os_free(env, (a)->buf);				\
		DB_MSGBUF_INIT(a);					\
	}								\
} while(0)
#define	DB_MSGBUF_REP_FLUSH(env, a, diag_msg, regular_msg) do {		\
	if((a)->buf != NULL) {						\
		if((a)->cur != (a)->buf && diag_msg)			\
			__db_repmsg(env, "%s", (a)->buf);		\
		if(regular_msg)					\
			DB_MSGBUF_FLUSH(env, a);			\
		else {							\
			__os_free(env, (a)->buf);			\
			DB_MSGBUF_INIT(a);				\
		}							\
	}								\
} while(0)
#define	STAT_FMT(msg, fmt, type, v) do {				\
	DB_MSGBUF __mb;							\
	DB_MSGBUF_INIT(&__mb);						\
	__db_msgadd(env, &__mb, fmt, static_cast<type>(v));			\
	__db_msgadd(env, &__mb, "\t%s", msg);				\
	DB_MSGBUF_FLUSH(env, &__mb);					\
} while(0)
#define	STAT_HEX(msg, v)     __db_msg(env, "%#lx\t%s", static_cast<ulong>(v), msg)
#define	STAT_ISSET(msg, p)   __db_msg(env, "%sSet\t%s", (p) == NULL ? "!" : " ", msg)
#define	STAT_LONG(msg, v)    __db_msg(env, "%ld\t%s", static_cast<long>(v), msg)
#define	STAT_LSN(msg, lsnp)  __db_msg(env, "%lu/%lu\t%s", static_cast<ulong>((lsnp)->file), static_cast<ulong>((lsnp)->Offset_), msg)
#define	STAT_POINTER(msg, v) __db_msg(env, "%#lx\t%s", P_TO_ULONG(v), msg)
#define	STAT_STRING(msg, p) do {					\
	const char *__p = p;	/* p may be a function call. */		\
	__db_msg(env, "%s\t%s", __p ? __p : "!Set", msg);	\
} while(0)
#define	STAT_ULONG(msg, v) __db_msg(env, "%lu\t%s", static_cast<ulong>(v), msg)
/*
 * The following macros are used to control how error and message strings are
 * output by Berkeley DB. There are essentially three different controls
 * available:
 *  - Default behavior is to output error strings with its unique identifier.
 *  - If HAVE_STRIPPED_MESSAGES is enabled, a unique identifier along with any
 *  parameters to the error string will be output.
 *  - If HAVE_LOCALIZATION is defined, and the '_()' macro is implemented, a
 *  gettext or ICU style translation will be done.
 *
 * Each new string that will be output should be wrapped in a DB_STR* macro.
 * There are three versions of this macro for different scenarions:
 *  - DB_STR for strings that need an identifier, and don't have any argument.
 *  - DB_STR_A for strings that need an identifier, and have argument(s).
 *  - DB_STR_P for strings that don't need an identifier, and don't have
 *  arguments.
 *
 * Error message IDs are automatically assigned by dist/s_message_id script.
 */
#ifdef HAVE_LOCALIZATION
	#define _(msg)	msg	/* Replace with localization function. */
#else
	#define _(msg)	msg
#endif
#ifdef HAVE_STRIPPED_MESSAGES
	#define DB_STR_C(msg, fmt)	fmt
#else
	#define DB_STR_C(msg, fmt)	_(msg)
#endif
#define DB_MSGID(id)		"BDB" id
#define DB_STR(id, msg)		DB_MSGID(id) " " DB_STR_C(msg, "")
#define DB_STR_A(id, msg, fmt)	DB_MSGID(id) " " DB_STR_C(msg, fmt)
#define DB_STR_P(msg)		_(msg)

/*
 * There are quite a few places in Berkeley DB where we want to initialize
 * a DBT from a string or other random pointer type, using a length typed
 * to size_t in most cases.  This macro avoids a lot of casting.  The macro
 * comes in two flavors because we often want to clear the DBT first.
 */
#define	DB_SET_DBT(dbt, d, s)  do { (dbt).data = (void *)(d); (dbt).size = static_cast<uint32>(s); } while(0)
#define DB_INIT_DBT(dbt, d, s) do { memzero(&(dbt), sizeof(dbt)); DB_SET_DBT(dbt, d, s); } while(0)

/*******************************************************
 * API return values
 *******************************************************/
/*
 * Return values that are OK for each different call.  Most calls have a
 * standard 'return of 0 is only OK value', but some, like db->get have
 * DB_NOTFOUND as a return value, but it really isn't an error.
 */
#define	DB_RETOK_STD(ret)     ((ret) == 0)
#define	DB_RETOK_DBCDEL(ret)  oneof3((ret), 0, DB_KEYEMPTY, DB_NOTFOUND)
#define	DB_RETOK_DBCGET(ret)  oneof3((ret), 0, DB_KEYEMPTY, DB_NOTFOUND)
#define	DB_RETOK_DBCPUT(ret)  oneof3((ret), 0, DB_KEYEXIST, DB_NOTFOUND)
#define	DB_RETOK_DBDEL(ret)   DB_RETOK_DBCDEL(ret)
#define	DB_RETOK_DBGET(ret)   DB_RETOK_DBCGET(ret)
#define	DB_RETOK_DBPUT(ret)   oneof2((ret), 0, DB_KEYEXIST)
#define	DB_RETOK_EXISTS(ret)  DB_RETOK_DBCGET(ret)
#define	DB_RETOK_LGGET(ret)   oneof2((ret), 0, DB_NOTFOUND)
#define	DB_RETOK_MPGET(ret)   oneof2((ret), 0, DB_PAGE_NOTFOUND)
#define	DB_RETOK_REPPMSG(ret) oneof7((ret), 0, DB_REP_IGNORE, DB_REP_ISPERM, DB_REP_NEWMASTER, DB_REP_NEWSITE, DB_REP_NOTPERM, DB_REP_WOULDROLLBACK)
#define	DB_RETOK_REPMGR_LOCALSITE(ret)  oneof2((ret), 0, DB_NOTFOUND)
#define	DB_RETOK_REPMGR_START(ret)      oneof2((ret), 0, DB_REP_IGNORE)
#define	DB_RETOK_TXNAPPLIED(ret)        oneof4((ret), 0, DB_NOTFOUND, DB_TIMEOUT, DB_KEYEMPTY)

/* Find a reasonable operation-not-supported error. */
#ifdef	EOPNOTSUPP
	#define	DB_OPNOTSUP	EOPNOTSUPP
#else
	#ifdef	ENOTSUP
		#define	DB_OPNOTSUP	ENOTSUP
	#else
		#define	DB_OPNOTSUP	EINVAL
	#endif
#endif

/*******************************************************
 * Files.
 *******************************************************/
/*
 * We use 1024 as the maximum path length.  It's too hard to figure out what
 * the real path length is, as it was traditionally stored in <sys/param.h>,
 * and that file isn't always available.
 */
#define	DB_MAXPATHLEN   1024
#define	PATH_DOT        "."    /* Current working directory. */
#define	PATH_SEPARATOR  "\\/:" /* Path separator character(s). */

/*******************************************************
 * Environment.
 *******************************************************/
/* Type passed to __db_appname(). */
typedef enum {
	DB_APP_NONE=0,			/* No type (region). */
	DB_APP_DATA,			/* Data file. */
	DB_APP_LOG,			/* Log file. */
	DB_APP_TMP,			/* Temporary file. */
	DB_APP_RECOVER			/* We are in recovery. */
} APPNAME;

/*
 * A set of macros to check if various functionality has been configured.
 *
 * ALIVE_ON	The is_alive function is configured.
 * CDB_LOCKING	CDB product locking.
 * CRYPTO_ON	Security has been configured.
 * LOCKING_ON	Locking has been configured.
 * LOGGING_ON	Logging has been configured.
 * MUTEX_ON	Mutexes have been configured.
 * MPOOL_ON	Memory pool has been configured.
 * REP_ON	Replication has been configured.
 * TXN_ON	Transactions have been configured.
 *
 * REP_ON is more complex than most: if the BDB library was compiled without
 * replication support, ENV->rep_handle will be NULL; if the BDB library has
 * replication support, but it was not configured, the region reference will
 * be NULL.
 */
#define	ALIVE_ON(env)		((env)->dbenv->is_alive != NULL)
#define	CDB_LOCKING(env)	F_ISSET(env, ENV_CDB)
#define	CRYPTO_ON(env)		((env)->crypto_handle != NULL)
#define	LOCKING_ON(env)		((env)->lk_handle != NULL)
#define	LOGGING_ON(env)		((env)->lg_handle != NULL)
#define	MPOOL_ON(env)		((env)->mp_handle != NULL)
#define	MUTEX_ON(env)		((env)->mutex_handle != NULL)
#define	REP_ON(env)         ((env)->rep_handle != NULL && (env)->rep_handle->region != NULL)
#define	TXN_ON(env)         ((env)->tx_handle != NULL)

/*
 * STD_LOCKING	Standard locking, that is, locking was configured and CDB
 *		was not.  We do not do locking in off-page duplicate trees,
 *		so we check for that in the cursor first.
 */
#define	STD_LOCKING(dbc)   (!F_ISSET(dbc, DBC_OPD) && !CDB_LOCKING((dbc)->env) && LOCKING_ON((dbc)->env))
/*
 * IS_RECOVERING: The system is running recovery.
 */
#define	IS_RECOVERING(env) (LOGGING_ON(env) && F_ISSET((env)->lg_handle, DBLOG_RECOVER))

/* Initialization methods are often illegal before/after open is called. */
#define	ENV_ILLEGAL_AFTER_OPEN(env, name)  if(F_ISSET((env), ENV_OPEN_CALLED))  return (__db_mi_open(env, name, 1));
#define	ENV_ILLEGAL_BEFORE_OPEN(env, name) if(!F_ISSET((env), ENV_OPEN_CALLED)) return (__db_mi_open(env, name, 0));

/* We're not actually user hostile, honest. */
#define	ENV_REQUIRES_CONFIG(env, handle, i, flags)    if(handle == NULL) return (__env_not_config(env, i, flags));
#define	ENV_REQUIRES_CONFIG_XX(env, handle, i, flags) if((env)->handle->region == NULL) return (__env_not_config(env, i, flags));
#define	ENV_NOT_CONFIGURED(env, handle, i, flags)     if(F_ISSET((env), ENV_OPEN_CALLED)) ENV_REQUIRES_CONFIG(env, handle, i, flags)

#define	ENV_ENTER_RET(env, ip, ret) do {				\
	ret = 0; \
	PANIC_CHECK_RET(env, ret);					\
 	if(!ret) {							\
		if((env)->thr_hashtab == NULL)				\
			ip = NULL;					\
		else 							\
			ret = __env_set_state(env, &(ip), THREAD_ACTIVE);\
	}								\
} while(0)

#define	ENV_ENTER(env, ip) do {						\
	int __ret;							\
	ip = NULL;							\
	ENV_ENTER_RET(env, ip, __ret);					\
	if(__ret != 0)							\
		return (__ret);						\
} while(0)

#define	FAILCHK_THREAD(env, ip) do { if((ip) != NULL) (ip)->dbth_state = THREAD_FAILCHK; } while(0)
#define	ENV_GET_THREAD_INFO(env, ip) ENV_ENTER(env, ip)

#ifdef DIAGNOSTIC
	#define	ENV_LEAVE(env, ip) do {						\
		if((ip) != NULL) {						\
			DB_ASSERT(env, ((ip)->dbth_state == THREAD_ACTIVE || (ip)->dbth_state == THREAD_FAILCHK)); \
			(ip)->dbth_state = THREAD_OUT;				\
		}								\
	} while(0)
#else
	#define	ENV_LEAVE(env, ip) do { if((ip) != NULL) (ip)->dbth_state = THREAD_OUT; } while(0)
#endif
#ifdef DIAGNOSTIC
	#define	CHECK_THREAD(env) do { if((env)->thr_hashtab != NULL) __env_set_state(env, NULL, THREAD_VERIFY); } while(0)
	#ifdef HAVE_STATISTICS
		#define	CHECK_MTX_THREAD(env, mtx) do {					\
			if(mtx->alloc_id != MTX_MUTEX_REGION && mtx->alloc_id != MTX_ENV_REGION && mtx->alloc_id != MTX_APPLICATION) \
				CHECK_THREAD(env);					\
		} while(0)
	#else
		#define	CHECK_MTX_THREAD(env, mtx)	NOP_STATEMENT
	#endif
#else
	#define	CHECK_THREAD(env)		NOP_STATEMENT
	#define	CHECK_MTX_THREAD(env, mtx)	NOP_STATEMENT
#endif

typedef enum {
	THREAD_SLOT_NOT_IN_USE=0,
	THREAD_OUT,
	THREAD_ACTIVE,
	THREAD_BLOCKED,
	THREAD_BLOCKED_DEAD,
	THREAD_FAILCHK,
	THREAD_VERIFY
} DB_THREAD_STATE;

typedef struct __pin_list {
	roff_t b_ref;		/* offset to buffer. */
	int region;		/* region containing buffer. */
} PIN_LIST;
#define	PINMAX 4

struct __db_thread_info { /* SHARED */
	pid_t		dbth_pid;
	db_threadid_t	dbth_tid;
	DB_THREAD_STATE	dbth_state;
	SH_TAILQ_ENTRY	dbth_links;
	/*
	 * The next field contains the (process local) reference to the XA
	 * transaction currently associated with this thread of control.
	 */
	SH_TAILQ_HEAD(__dbth_xatxn) dbth_xatxn;
	uint32	dbth_xa_status;
	/*
	 * The following fields track which buffers this thread of
	 * control has pinned in the mpool buffer cache.
	 */
	uint16	dbth_pincount;	/* Number of pins for this thread. */
	uint16	dbth_pinmax;	/* Number of slots allocated. */
	roff_t		dbth_pinlist;	/* List of pins. */
	PIN_LIST	dbth_pinarray[PINMAX];	/* Initial array of slots. */
#ifdef DIAGNOSTIC
	roff_t		dbth_locker;	/* Current locker for this thread. */
	uint32	dbth_check_off;	/* Count of number of LOCK_OFF calls. */
#endif
};
#ifdef DIAGNOSTIC
#define LOCK_CHECK_OFF(ip) if((ip) != NULL) (ip)->dbth_check_off++
#define LOCK_CHECK_ON(ip)  if((ip) != NULL)  (ip)->dbth_check_off--

#define LOCK_CHECK(dbc, pgno, mode, type)				\
	DB_ASSERT((dbc)->dbp->env, (dbc)->locker == NULL ||		\
	     __db_haslock((dbc)->dbp->env,				\
	    (dbc)->locker, (dbc)->dbp->mpf, pgno, mode, type) == 0)
#else
	#define LOCK_CHECK_OFF(ip)	NOP_STATEMENT
	#define LOCK_CHECK_ON(ip)	NOP_STATEMENT
	#define LOCK_CHECK(dbc, pgno, mode)	NOP_STATEMENT
#endif

typedef struct __env_thread_info {
	uint32 thr_count;
	uint32 thr_init;
	uint32 thr_max;
	uint32 thr_nbucket;
	roff_t thr_hashoff;
} THREAD_INFO;

#define	DB_EVENT(env, e, einfo) do { DB_ENV *__dbenv = (env)->dbenv; if(__dbenv->db_event_func) __dbenv->db_event_func(__dbenv, e, einfo); } while(0)

typedef struct __flag_map {
	uint32 inflag;
	uint32 outflag;
} FLAG_MAP;

/*
 * Internal database environment structure.
 *
 * This is the private database environment handle.  The public environment
 * handle is the DB_ENV structure.   The library owns this structure, the user
 * owns the DB_ENV structure.  The reason there are two structures is because
 * the user's configuration outlives any particular DB_ENV->open call, and
 * separate structures allows us to easily discard internal information without
 * discarding the user's configuration.
 */
TAILQ_HEAD(__dblist, __db);

struct __env {
	DB_ENV * dbenv; // Linked DB_ENV structure 
	// 
	// The ENV structure can be used concurrently, so field access is protected.
	// 
	db_mutex_t mtx_env; // ENV structure mutex 
	/*
	 * Some fields are included in the ENV structure rather than in the
	 * DB_ENV structure because they are only set as arguments to the
	 * DB_ENV->open method.  In other words, because of the historic API,
	 * not for any rational reason.
	 *
	 * Arguments to DB_ENV->open.
	 */
	char * db_home;		/* Database home */
	uint32 open_flags;		/* Flags */
	int	   db_mode;		/* Default open permissions */
	pid_t  pid_cache;		/* Cached process ID */
	DB_FH * lockfhp;		/* fcntl(2) locking file handle */
	DB_LOCKER * env_lref;		/* Locker in non-threaded handles */
	DB_DISTAB recover_dtab;	/* Dispatch table for recover funcs */
	int    dir_mode;			/* Intermediate directory perms. */
#define ENV_DEF_DATA_LEN		100
	uint32 data_len;		/* Data length in __db_prbytes. */
	/* Thread tracking */
	uint32 thr_nbucket;	/* Number of hash buckets */
	DB_HASHTAB * thr_hashtab;	/* Hash table of DB_THREAD_INFO */
	/*
	 * List of open DB handles for this ENV, used for cursor
	 * adjustment.  Must be protected for multi-threaded support.
	 */
	db_mutex_t mtx_dblist;
	int	   db_ref;		/* DB handle reference count */
	struct __dblist dblist;
	/*
	 * List of open file handles for this ENV.  Must be protected
	 * for multi-threaded support.
	 */
	TAILQ_HEAD(__fdlist, __fh_t) fdlist;
	db_mutex_t mtx_mt;      /* Mersenne Twister mutex */
	int    mti;             /* Mersenne Twister index */
	ulong * mt;            /* Mersenne Twister state vector */
	DB_CIPHER	*crypto_handle;	/* Crypto handle */
	DB_LOCKTAB	*lk_handle;	/* Lock handle */
	DB_LOG		*lg_handle;	/* Log handle */
	DB_MPOOL	*mp_handle;	/* Mpool handle */
	DB_MUTEXMGR	*mutex_handle;	/* Mutex handle */
	DB_REP		*rep_handle;	/* Replication handle */
	DB_TXNMGR	*tx_handle;	/* Txn handle */
	/*
	 * XA support.
	 */
	int		 xa_rmid;	/* XA Resource Manager ID */
	int		 xa_ref;	/* XA Reference count */
	TAILQ_ENTRY(__env) links;	/* XA environments */
	/* Application callback to copy data to/from a custom data source */
#define	DB_USERCOPY_GETDATA	0x0001
#define	DB_USERCOPY_SETDATA	0x0002
	int (*dbt_usercopy)(DBT *, uint32, void *, uint32, uint32);
	int (*log_verify_wrap)(ENV *, const char *, uint32, const char *, const char *, __time64_t, __time64_t, uint32,  uint32, uint32, uint32, int, int);
	REGINFO	*reginfo;		/* REGINFO structure reference */
#define	DB_TEST_ELECTINIT	 1	/* after __rep_elect_init */
#define	DB_TEST_ELECTVOTE1	 2	/* after sending VOTE1 */
#define	DB_TEST_NO_PAGES	 3	/* before sending PAGE */
#define	DB_TEST_POSTDESTROY	 4	/* after destroy op */
#define	DB_TEST_POSTLOG		 5	/* after logging all pages */
#define	DB_TEST_POSTLOGMETA	 6	/* after logging meta in btree */
#define	DB_TEST_POSTOPEN	 7	/* after __os_open */
#define	DB_TEST_POSTSYNC	 8	/* after syncing the log */
#define	DB_TEST_PREDESTROY	 9	/* before destroy op */
#define	DB_TEST_PREOPEN		 10	/* before __os_open */
#define	DB_TEST_REPMGR_PERM	 11	/* repmgr perm/archiving tests */
#define	DB_TEST_SUBDB_LOCKS	 12	/* subdb locking tests */
	int	test_abort;		/* Abort value for testing */
	int	test_check;		/* Checkpoint value for testing */
	int	test_copy;		/* Copy value for testing */

#define	ENV_CDB			0x00000001 /* DB_INIT_CDB */
#define	ENV_DBLOCAL		0x00000002 /* Environment for a private DB */
#define	ENV_LITTLEENDIAN	0x00000004 /* Little endian system. */
#define	ENV_LOCKDOWN		0x00000008 /* DB_LOCKDOWN set */
#define	ENV_NO_OUTPUT_SET	0x00000010 /* No output channel set */
#define	ENV_OPEN_CALLED		0x00000020 /* DB_ENV->open called */
#define	ENV_PRIVATE		0x00000040 /* DB_PRIVATE set */
#define	ENV_RECOVER_FATAL	0x00000080 /* Doing fatal recovery in env */
#define	ENV_REF_COUNTED		0x00000100 /* Region references this handle */
#define	ENV_SYSTEM_MEM		0x00000200 /* DB_SYSTEM_MEM set */
#define	ENV_THREAD		0x00000400 /* DB_THREAD set */
#define ENV_FORCE_TXN_BULK	0x00000800 /* Txns use bulk mode-for testing */
	uint32 flags;
};

/*******************************************************
 * Database Access Methods.
 *******************************************************/
/*
 * DB_IS_THREADED --
 *	The database handle is free-threaded (was opened with DB_THREAD).
 */
#define	DB_IS_THREADED(dbp)               ((dbp)->mutex != MUTEX_INVALID)
/* Initialization methods are often illegal before/after open is called. */
#define	DB_ILLEGAL_AFTER_OPEN(dbp, name)  if(F_ISSET((dbp), DB_AM_OPEN_CALLED))  return (__db_mi_open((dbp)->env, name, 1));
#define	DB_ILLEGAL_BEFORE_OPEN(dbp, name) if(!F_ISSET((dbp), DB_AM_OPEN_CALLED)) return (__db_mi_open((dbp)->env, name, 0));
/* Some initialization methods are illegal if environment isn't local. */
#define	DB_ILLEGAL_IN_ENV(dbp, name)  if(!F_ISSET((dbp)->env, ENV_DBLOCAL)) return (__db_mi_env((dbp)->env, name));
#define	DB_ILLEGAL_METHOD(dbp, flags) { int __ret; if((__ret = __dbh_am_chk(dbp, flags)) != 0) return (__ret); }

/*
 * Common DBC->internal fields.  Each access method adds additional fields
 * to this list, but the initial fields are common.
 */
#define	__DBC_INTERNAL							\
	DBC	 * opd;			/* Off-page duplicate cursor. */\
	DBC	 * pdbc;			/* Pointer to parent cursor. */ \
	void * page;			/* Referenced page. */		\
	uint32 part;			/* Partition number. */		\
	db_pgno_t root;			/* Tree root. */		\
	db_pgno_t pgno;			/* Referenced page number. */	\
	db_indx_t indx;			/* Referenced key item index. */\
	/* Streaming -- cache last position. */				\
	db_pgno_t stream_start_pgno;	/* Last start pgno. */		\
	uint32 stream_off;		/* Current offset. */		\
	db_pgno_t stream_curr_pgno;	/* Current overflow page. */	\
	DB_LOCK		lock;		/* Cursor lock. */		\
	db_lockmode_t	lock_mode;	/* Lock mode. */

struct __dbc_internal {
	__DBC_INTERNAL
};
//
// Actions that __db_master_update can take. 
//
typedef enum { 
	MU_REMOVE, 
	MU_RENAME, 
	MU_OPEN, 
	MU_MOVE 
} mu_action;
/*
 * Access-method-common macro for determining whether a cursor
 * has been initialized.
 */
#ifdef HAVE_PARTITION
#define	IS_INITIALIZED(dbc)	(DB_IS_PARTITIONED((dbc)->dbp) ?	\
		((PART_CURSOR *)(dbc)->internal)->sub_cursor && ((PART_CURSOR *)(dbc)->internal)->sub_cursor->internal->pgno != PGNO_INVALID : (dbc)->internal->pgno != PGNO_INVALID)
#else
#define	IS_INITIALIZED(dbc)	((dbc)->internal->pgno != PGNO_INVALID)
#endif

/* Free the callback-allocated buffer, if necessary, hanging off of a DBT. */
#define	FREE_IF_NEEDED(env, dbt)					\
	if(F_ISSET((dbt), DB_DBT_APPMALLOC)) {				\
		__os_ufree((env), (dbt)->data);				\
		F_CLR((dbt), DB_DBT_APPMALLOC);				\
	}

/*
 * Use memory belonging to object "owner" to return the results of
 * any no-DBT-flag get ops on cursor "dbc".
 */
#define	SET_RET_MEM(dbc, owner)				\
	do {						\
		(dbc)->rskey = &(owner)->my_rskey;	\
		(dbc)->rkey = &(owner)->my_rkey;	\
		(dbc)->rdata = &(owner)->my_rdata;	\
	} while(0)

/* Use the return-data memory src is currently set to use in dest as well. */
#define	COPY_RET_MEM(src, dest)				\
	do {						\
		(dest)->rskey = (src)->rskey;		\
		(dest)->rkey = (src)->rkey;		\
		(dest)->rdata = (src)->rdata;		\
	} while(0)

/* Reset the returned-memory pointers to their defaults. */
#define	RESET_RET_MEM(dbc)				\
	do {						\
		(dbc)->rskey = &(dbc)->my_rskey;	\
		(dbc)->rkey = &(dbc)->my_rkey;		\
		(dbc)->rdata = &(dbc)->my_rdata;	\
	} while(0)

#define	COMPACT_TRUNCATE(c_data) do { if(c_data->compact_truncate > 1) c_data->compact_truncate--; } while(0)

/*******************************************************
 * Mpool.
 *******************************************************/
/*
 * File types for DB access methods.  Negative numbers are reserved to DB.
 */
#define	DB_FTYPE_SET		-1		/* Call pgin/pgout functions. */
#define	DB_FTYPE_NOTSET		 0		/* Don't call... */
#define	DB_LSN_OFF_NOTSET	-1		/* Not yet set. */
#define	DB_CLEARLEN_NOTSET	UINT32_MAX	/* Not yet set. */
//
// Structure used as the DB pgin/pgout pgcookie.
//
typedef struct __dbpginfo {
	uint32 db_pagesize; // Underlying page size
	uint32 flags; // Some DB_AM flags needed
	DBTYPE type;  // DB type 
} DB_PGINFO;
//
// Log.
//
// Initialize an LSN to 'zero'
//
//#define ZERO_LSN(LSN)           do { (LSN).file = 0; (LSN).Offset_ = 0; } while(0)
//#define IS_ZERO_LSN(LSN)        ((LSN).file == 0 && (LSN).Offset_ == 0)
#define ZERO_LSN(LSN)           (LSN).Clear()
#define IS_ZERO_LSN(LSN)        (LSN).IsZero()
#define	IS_INIT_LSN(LSN)        ((LSN).file == 1 && (LSN).Offset_ == 0)
#define	INIT_LSN(LSN)           do { (LSN).file = 1; (LSN).Offset_ = 0; } while(0)
#define	MAX_LSN(LSN)            do { (LSN).file = UINT32_MAX; (LSN).Offset_ = UINT32_MAX; } while(0)
#define	IS_MAX_LSN(LSN)         ((LSN).file == UINT32_MAX && (LSN).Offset_ == UINT32_MAX)
//
// If logging is turned off, smash the lsn
//
//#define LSN_NOT_LOGGED(LSN)     do { (LSN).file = 0; (LSN).Offset_ = 1; } while(0)
#define LSN_NOT_LOGGED(LSN)     (LSN).NotLogged()
#define	IS_NOT_LOGGED_LSN(LSN)  ((LSN).file == 0 && (LSN).Offset_ == 1)
//
// LOG_COMPARE -- compare two LSNs.
//
inline int FASTCALL _BdbLogCompare(const DB_LSN * pLsn0, const DB_LSN * pLsn1)
{
	return (pLsn0->file != pLsn1->file ? (pLsn0->file < pLsn1->file ? -1 : 1) : (pLsn0->Offset_ != pLsn1->Offset_ ? (pLsn0->Offset_ < pLsn1->Offset_ ? -1 : 1) : 0));
}
#define LOG_COMPARE(lsn0, lsn1) _BdbLogCompare(lsn0, lsn1)
//#define LOG_COMPARE(lsn0, lsn1) ((lsn0)->file != (lsn1)->file ? ((lsn0)->file < (lsn1)->file ? -1 : 1) : ((lsn0)->Offset_ != (lsn1)->Offset_ ? ((lsn0)->Offset_ < (lsn1)->Offset_ ? -1 : 1) : 0))
//
// Txn.
//
#define	DB_NONBLOCK(C)         ((C)->txn != NULL && F_ISSET((C)->txn, TXN_NOWAIT))
#define	NOWAIT_FLAG(txn)       ((txn) != NULL && F_ISSET((txn), TXN_NOWAIT) ? DB_LOCK_NOWAIT : 0)
#define	IS_REAL_TXN(txn)       ((txn) != NULL && !F_ISSET(txn, TXN_FAMILY))
#define	IS_SUBTRANSACTION(txn) ((txn) != NULL && (txn)->parent != NULL)
//
// Checks for existence of an XA transaction in access method interfaces. 
//
#define	XA_CHECK_TXN(ip, txn) 						\
	if((ip) != NULL && (txn) == NULL) {				\
		(txn) = SH_TAILQ_FIRST(&(ip)->dbth_xatxn, __db_txn);	\
		DB_ASSERT(env, txn == NULL || txn->xa_thr_status == TXN_XA_THREAD_ASSOCIATED);	\
	}

/* Ensure that there is no XA transaction active. */
#define	XA_NO_TXN(ip, retval) {						\
	retval = 0;							\
	if((ip) != NULL) {						\
		DB_TXN *__txn = SH_TAILQ_FIRST(&(ip)->dbth_xatxn, __db_txn);	\
		if(__txn && __txn->xa_thr_status == TXN_XA_THREAD_ASSOCIATED)	\
			retval = EINVAL;				\
	}								\
}
//
// Crypto.
//
#define	DB_IV_BYTES 16 // Bytes per IV
#define	DB_MAC_KEY  20 // Bytes per MAC checksum
//
// Compression
//
#define	CMP_INT_SPARE_VAL	0xFC // Smallest byte value that the integer compression algorithm doesn't use

#if defined(__cplusplus)
}
#endif
//
// Remaining general DB includes.
//
#include "dbinc/globals.h"
#include "dbinc/clock.h"
#include "dbinc/debug.h"
#include "dbinc/region.h"
#include "dbinc/log.h" // @v9.5.5
#include "dbinc_auto/env_ext.h"
#include "dbinc/atomic.h"
#include "dbinc/mutex.h"
#ifdef HAVE_REPLICATION_THREADS
	#include "dbinc/repmgr.h"
#endif
#include "dbinc/rep.h"
#include "dbinc/os.h"
#include "dbinc_auto/clib_ext.h"
#include "dbinc_auto/common_ext.h"
#include "dbinc/db_page.h" // @v9.5.5
#include "dbinc/lock.h" // @v9.5.5
#include "dbinc/mp.h" // @v9.5.5
#include "dbinc/crypto.h" // @v9.5.5
#include "dbinc/btree.h" // @v9.5.5
#include "dbinc/hash.h" // @v9.5.5
#include "dbinc/heap.h" // @v9.5.5
//
//#include "dbinc/partition.h" // @v9.5.5
//
#if defined(__cplusplus)
extern "C" {
#endif

typedef struct __db_partition {
	uint32	nparts;		/* number of partitions. */
	DBT		*keys;		/* array of range keys. */
	void		*data;		/* the partion info. */
	const char	**dirs;		/* locations for partitions. */
	DB		**handles;	/* array of partition handles. */
	uint32	(*callback) (DB *, DBT *);
#define	PART_CALLBACK	0x01
#define	PART_RANGE	0x02
	uint32	flags;
} DB_PARTITION;
// 
// Internal part of a partitoned cursor.
// 
typedef struct __part_internal {
	__DBC_INTERNAL
	uint32	part_id;
	DBC		*sub_cursor;
} PART_CURSOR;

#ifdef HAVE_PARTITION
	#define	PART_NAME	"__dbp.%s.%03d"
	#define	PART_LEN	(sstrlen("__dbp..")+3)
	#define	PART_PREFIX	"__dbp."
	#define	DB_IS_PARTITIONED(dbp) (dbp->p_internal != NULL && ((DB_PARTITION *)dbp->p_internal)->handles != NULL)
	#define	DBC_PART_REFRESH(dbc)	(F_SET(dbc, DBC_PARTITIONED))
#else
	#define	DBC_PART_REFRESH(dbc)
	#define	DB_IS_PARTITIONED(dbp)	(0)
#endif

#if defined(__cplusplus)
}
#endif
//
#include "dbinc/db_swap.h" // @v9.5.5
#include "dbinc/hmac.h" // @v9.5.5
#include "dbinc/fop.h" // @v9.5.5
#include "dbinc/qam.h" // @v9.5.5
//
//#include "dbinc/db_dispatch.h" // @v9.5.5
//
#if defined(__cplusplus)
extern "C" {
#endif
// 
// Declarations and typedefs for the list of transaction IDs used during
// recovery.  This is a generic list used to pass along whatever information we need during recovery.
// 
typedef enum {
	TXNLIST_DELETE,
	TXNLIST_LSN,
	TXNLIST_TXNID
} db_txnlist_type;

#define	DB_TXNLIST_MASK(hp, n)  (n % hp->nslots)

LIST_HEAD(__db_headlink, __db_txnlist); // @sobolev

struct __db_txnhead {
	void *td;		/* If abort, the detail for the txn. */
	DB_THREAD_INFO *thread_info;	/* Thread information. */
	uint32 maxid;	/* Maximum transaction id. */
	DB_LSN maxlsn;		/* Maximum commit lsn. */
	DB_LSN ckplsn;		/* LSN of last retained checkpoint. */
	DB_LSN trunc_lsn;	/* Lsn to which we are going to truncate; make sure we abort anyone after this. */
	uint32 generation;	/* Current generation number. */
	uint32 gen_alloc;	/* Number of generations allocated. */
	struct {
		uint32 generation;
		uint32 txn_min;
		uint32 txn_max;
	} *gen_array;		/* Array of txnids associated with a gen. */
	uint nslots;
	// @sobolev LIST_HEAD(__db_headlink, __db_txnlist) head[1];
	struct __db_headlink head[1]; // @sobolev
};

#define	DB_LSN_STACK_SIZE 4
struct __db_txnlist {
	db_txnlist_type type;
	LIST_ENTRY(__db_txnlist) links;
	union {
		struct {
			uint32 txnid;
			uint32 generation;
			uint32 status;
		} t;
		struct {
			uint32 stack_size;
			uint32 stack_indx;
			DB_LSN *lsn_stack;
		} l;
	} u;
};

#if defined(__cplusplus)
}
#endif
//
#include "dbinc/db_am.h" // @v9.5.5
#include "dbinc/txn.h" // @v9.5.5
//
//#include "dbinc/log_verify.h" // @v9.6.2
//
// Log verification handle, such a handle is shared among all verification 
// functions during one verification process. 
// 
struct __db_log_verify_info {
	DB_ENV *dbenv;		/* The database environment. */
	DB *txninfo;		/* (txnid, __txn_verify_info) map. */
	DB *ckps;		/* (ckp lrid, __ckpinfo) map. */
	DB *fileregs; 		/* (file-uid, __file_reg_info) map. */
	DB *fnameuid;		/* (fname, fuid), secondary db of fileregs. */
	DB *dbregids; // (dbreg-id, __file_reg_info) map, NOT the sec db for fileregs. 
	DB *pgtxn; 		/* (fileid-pageno, txnid) map. */
	DB *txnpg; 		/* (txnid, fileid-pageno), sec db of pgtxn. */
	DB *lsntime;  // lsn, (time-stamp, logtype(txn_regop or txn_ckp)) map. 
	DB *timelsn; // Secondary db of lsntime, use timestamp as secindex. 
	DB *txnrngs; // Time range database, (uint32, __lv_txnrange) db. 
	DB *txnaborts;	 // Store abort txn (lsn, txnid) map. 
	DB_LSN last_lsn;	/* Lsn of last log record we verified. */
	uint32 ntxn_active, ntxn_abort, ntxn_commit, ntxn_prep;  // The number of active, abort, commit and prepared txns.
	uint32 nckp;		/* The number of checkpoint log records. */
	uint8 target_dbid[DB_FILE_ID_LEN]; // Target database file unique id. Set if only verify log records of a database. 
	uint32 non_txnup_cnt;/* Number of non-txnal log records. */
	uint32 unknown_logrec_cnt;/* Number of unknown log record. */
	uint32 external_logrec_cnt;/* Number of external log record. */
	uint32 lrtypes[256]; // (Log type, number of record) map. typeids are continuous integers, 256 is a big enough number. 
	uint32 aborted_txnid; // The last aborted txnid. 
	DB_LSN aborted_txnlsn; /* Last aborted txn's last log. */
	DB_LSN valid_lsn; /* When reach this log,unset DB_LOG_VERIFY_PARTIAL. */
	char *logtype_names[256]; // The type name string of each type of log.
	const DB_LOG_VERIFY_CONFIG *lv_config;
	DB_THREAD_INFO * ip;
	uint32 flags;	/* The result of the verification. */
};
//
// Transaction information.
//
struct __txn_verify_info {
#define TXN_VERIFY_INFO_FIXSIZE    (4 * sizeof(DB_LSN) + 9 * sizeof(uint32))
#define TXN_VERIFY_INFO_TOTSIZE(s) (TXN_VERIFY_INFO_FIXSIZE + (s).num_recycle * sizeof(DB_LSN) + 	\
	__lv_dbt_arrsz((s).fileups, (s).filenum) + sizeof(int32) * (s).filenum)

	uint32 txnid; 	/* The key, also stored in data here. */
	uint32 ptxnid; 	/* The parent txn id. */
	DB_LSN first_lsn;	/* Lsn of the first log record of this txn. */
	DB_LSN last_lsn; 	/* Last lsn of the txn. */
	DB_LSN prep_lsn; 	/* txn_prepare's lsn.*/
	DB_LSN cur_lsn;		/* The lsn of the latest db op of this txn. */
	uint32 num_recycle; 	/* The number of recycle lsns. */
	uint32 filenum; 	/* The number of files updated. */
#define TXN_STAT_ACTIVE 0
#define TXN_STAT_ABORT 1
#define TXN_STAT_COMMIT 2
#define TXN_STAT_PREPARE 3
	uint32 status;	/* Txn status */
	uint32 nchild_active; // The number of active, abort and commit children
	uint32 nchild_abort;
	uint32 nchild_commit;
	uint32 flags; /* Copied from the DB_TXN::flags member. */
	DB_LSN *recycle_lsns; 	/* The array of txn_recycle records' lsns. */
	DBT *fileups; 	 // The array of file unique ids of files updated by this txn. 
	int32 *dbregid;/* The array of dbreg file ids updated by this txn. */
};
//
// Database file information. 
//
struct __lv_filereg_info {
#define FILE_REG_INFO_FIXSIZE (sizeof(uint32))
#define FILE_REG_INFO_TOTSIZE(s) (FILE_REG_INFO_FIXSIZE + (s).fileid.size + sizeof((s).fileid.size) + sizeof(int32) * (s).regcnt + sstrlen((s).fname) + 1)
	uint32 regcnt;	/* The number of dbregids for this file-uid. */
	int32 *dbregids;
	DBT    fileid; // The file unique id. 
	char * fname;  // Database file name. 
};
//
// Database file dbreg_register information. 
//
struct __lv_filelife {
	int32 dbregid;	/* The primary key. */
	DBTYPE dbtype;		/* The database type. */
	uint32 lifetime;	/* DBREG_OPEN, DBREG_CHKPNT, DBREG_CLOSE */
	db_pgno_t meta_pgno;	/* The meta_pgno; */
	uint8 fileid[DB_FILE_ID_LEN];
	DB_LSN lsn;		/* The lsn of log updating lifetime. */
};
//
// Checkpoint information. 
//
struct __lv_ckp_info {
	int32 timestamp;
	DB_LSN lsn, ckplsn; // Lsn member is the primary key.
};
// 
// General information from log records which have timestamps. 
// We use it to do time range verifications. Such information is 
// acquired when backward-playing the logs before verification. 
// 
struct __lv_timestamp_info {
	DB_LSN lsn;		/* The primary key. */
	int32 timestamp;	/* The secondary key. */
	uint32 logtype; // The log types containing a time stamp, so far only txn_ckp and txn_regop types.
};
// 
// Transaction ranges. Such information is acquired when backward-playing the
// logs before verification. Can be used to find aborted txns.
// 
struct __lv_txnrange {
	uint32 txnid; // Transaction ID, the primary key. The db storing records of this type should allow dup since txnids maybe reused. 
	uint32 ptxnid; // The parent txn id, ptxnid is the parent of txnid during [begin, end]. 
	/* 
	 * The first and last lsn, end is used to sort dup data because it's
	 * seen prior to begin in a backward playback, and [begin, end] intervals won't overlap. 
	 */
	DB_LSN begin, end;
	int32 when_commit; // The time of the commit, 0 if aborted. 
};
//
// Parameter types for __iterate_txninfo function. 
//
struct __add_recycle_params {
	uint32 min, max; // The recycled txnid range. 
	VRFY_TXN_INFO **ti2u; // The array of txn info to update into db.
	uint32 ti2ui, ti2ul;/* The effective length and array length. */
	DB_LSN recycle_lsn;
}; 

struct __ckp_verify_params {
	DB_LSN lsn, ckp_lsn;
	ENV *env;
};
//
// Helper macros.
#define LOGTYPE_NAME(lvh, type) ((lvh->logtype_names[type] == NULL) ? NULL : lvh->logtype_names[type] + 3)
#define NUMCMP(i1, i2) ((i1) > (i2) ? 1 : ((i1) < (i2) ? -1 : 0))

#define INVAL_DBREGID -1
//
// During recovery, DBREG_CHKPNT can be seen as open, and it's followed by a DBREG_RCLOSE or DBREG_CLOSE. 
//
#define IS_DBREG_OPEN(opcode)  oneof4(opcode, DBREG_OPEN, DBREG_PREOPEN, DBREG_REOPEN, DBREG_CHKPNT)
#define IS_DBREG_CLOSE(opcode) oneof2(opcode, DBREG_CLOSE, DBREG_RCLOSE)

#define IS_LOG_VRFY_SUPPORTED(version) ((version) == DB_LOGVERSION)
//
//#include "dbinc/db_verify.h" // @v9.5.5
//
// Structures and macros for the storage and retrieval of all information
// needed for inter-page verification of a database.
//
/*
 * EPRINT is the macro for error printing.  Takes as an arg the arg set
 * for DB->err.
 */
#define	EPRINT(x) do { if(!LF_ISSET(DB_SALVAGE)) __db_errx x; } while(0)

/* Complain about a totally zeroed page where we don't expect one. */
#define	ZEROPG_ERR_PRINT(dbenv, pgno, str) do {				\
	EPRINT(((dbenv), DB_STR_A("0501", "Page %lu: %s is of inappropriate type %lu", "%lu %s %lu"), (ulong)(pgno), str, (ulong)P_INVALID)); \
	EPRINT(((dbenv), DB_STR_A("0502", "Page %lu: totally zeroed page", "%lu"), (ulong)(pgno))); \
} while(0)
/*
 * Note that 0 is, in general, a valid pgno, despite equaling PGNO_INVALID;
 * we have to test it separately where it's not appropriate.
 */
#define	IS_VALID_PGNO(x)	((x) <= vdp->last_pgno)
/*
 * VRFY_DBINFO is the fundamental structure;  it either represents the database
 * of subdatabases, or the sole database if there are no subdatabases.
 */
struct __vrfy_dbinfo {
	DB_THREAD_INFO *thread_info;
	DBTYPE		type; /* Info about this database in particular. */
	LIST_HEAD(__subdbs, __vrfy_childinfo) subdbs; /* List of subdatabase meta pages, if any. */
	DB_TXN *txn; /* Transaction handle for CDS group. */
	DB *pgdbp; /* File-global info--stores VRFY_PAGEINFOs for each page. */
	DB *cdbp; /* Child database--stores VRFY_CHILDINFOs of each page. */
	LIST_HEAD(__activepips, __vrfy_pageinfo) activepips; /* Page info structures currently in use. */
	/*
	 * DB we use to keep track of which pages are linked somehow
	 * during verification.  0 is the default, "unseen";  1 is seen.
	 */
	DB *pgset;
	/*
	 * This is a database we use during salvaging to keep track of which
	 * overflow and dup pages we need to come back to at the end and print
	 * with key "UNKNOWN".  Pages which print with a good key get set
	 * to SALVAGE_IGNORE;  others get set, as appropriate, to SALVAGE_LDUP,
	 * SALVAGE_LRECNODUP, SALVAGE_OVERFLOW for normal db overflow pages,
	 * and SALVAGE_BTREE, SALVAGE_LRECNO, and SALVAGE_HASH for subdb pages.
	 */
#define	SALVAGE_INVALID		0
#define	SALVAGE_IGNORE		1
#define	SALVAGE_LDUP		2
#define	SALVAGE_IBTREE		3
#define	SALVAGE_OVERFLOW	4
#define	SALVAGE_LBTREE		5
#define	SALVAGE_HASH		6
#define	SALVAGE_LRECNO		7
#define	SALVAGE_LRECNODUP	8
	DB * salvage_pages;
	db_pgno_t	last_pgno;
	db_pgno_t	meta_last_pgno;
	db_pgno_t	pgs_remaining;	/* For dbp->db_feedback(). */
	/*
	 * These are used during __bam_vrfy_subtree to keep track, while
	 * walking up and down the Btree structure, of the prev- and next-page
	 * chain of leaf pages and verify that it's intact.  Also, make sure
	 * that this chain contains pages of only one type.
	 */
	db_pgno_t	prev_pgno;
	db_pgno_t	next_pgno;
	uint8	leaf_type;
	/* Queue needs these to verify data pages in the first pass. */
	uint32	re_pad;		/* Record pad character. */
	uint32	re_len;		/* Record length. */
	uint32	rec_page;
	uint32	page_ext;
	uint32       first_recno;
	uint32       last_recno;
	int		nextents;
	db_pgno_t	*extents;
#define	SALVAGE_PRINTABLE	0x01	/* Output printable chars literally. */
#define	SALVAGE_PRINTHEADER	0x02	/* Print the unknown-key header. */
#define	SALVAGE_PRINTFOOTER	0x04	/* Print the unknown-key footer. */
#define	SALVAGE_HASSUBDBS	0x08	/* There are subdatabases to salvage. */
#define	VRFY_LEAFCHAIN_BROKEN	0x10	/* Lost one or more Btree leaf pgs. */
#define	VRFY_QMETA_SET		0x20    /* We've seen a QUEUE meta page and set things up for it. */
	uint32	flags;
}; /* VRFY_DBINFO */

/*
 * The amount of state information we need per-page is small enough that
 * it's not worth the trouble to define separate structures for each
 * possible type of page, and since we're doing verification with these we
 * have to be open to the possibility that page N will be of a completely
 * unexpected type anyway.  So we define one structure here with all the
 * info we need for inter-page verification.
 */
struct __vrfy_pageinfo {
	uint8	type;
	uint8	bt_level;
	uint8	unused1;
	uint8	unused2;
	db_pgno_t	pgno;
	db_pgno_t	prev_pgno;
	db_pgno_t	next_pgno;
	/* meta pages */
	db_pgno_t	root;
	db_pgno_t	free;		/* Free list head. */
	db_indx_t	entries;	/* Actual number of entries. */
	uint16	unused;
	db_recno_t	rec_cnt;	/* Record count. */
	uint32	re_pad;		/* Record pad character. */
	uint32	re_len;		/* Record length. */
	uint32	bt_minkey;
	uint32	h_ffactor;
	uint32	h_nelem;

	/* overflow pages */
	/*
	 * Note that refcount is the refcount for an overflow page; pi_refcount
	 * is this structure's own refcount!
	 */
	uint32	refcount;
	uint32	olen;

#define	VRFY_DUPS_UNSORTED	0x0001	/* Have to flag the negative! */
#define	VRFY_HAS_CHKSUM		0x0002
#define	VRFY_HAS_DUPS		0x0004
#define	VRFY_HAS_DUPSORT	0x0008	/* Has the flag set. */
#define	VRFY_HAS_PART_RANGE	0x0010	/* Has the flag set. */
#define	VRFY_HAS_PART_CALLBACK	0x0020	/* Has the flag set. */
#define	VRFY_HAS_RECNUMS	0x0040
#define	VRFY_HAS_SUBDBS		0x0080
#define	VRFY_INCOMPLETE		0x0100	/* Meta or item order checks incomp. */
#define	VRFY_IS_ALLZEROES	0x0200	/* Hash page we haven't touched? */
#define	VRFY_IS_FIXEDLEN	0x0400
#define	VRFY_IS_RECNO		0x0800
#define	VRFY_IS_RRECNO		0x1000
#define	VRFY_OVFL_LEAFSEEN	0x2000
#define	VRFY_HAS_COMPRESS	0x4000
	uint32	flags;
	LIST_ENTRY(__vrfy_pageinfo) links;
	uint32	pi_refcount;
}; /* VRFY_PAGEINFO */

struct __vrfy_childinfo {
	/* The following fields are set by the caller of __db_vrfy_childput. */
	db_pgno_t	pgno;
#define	V_DUPLICATE	1		/* off-page dup metadata */
#define	V_OVERFLOW	2		/* overflow page */
#define	V_RECNO		3		/* btree internal or leaf page */
	uint32	type;
	db_recno_t	nrecs;		/* record count on a btree subtree */
	uint32	tlen;		/* ovfl. item total size */
	/* The following field is maintained by __db_vrfy_childput. */
	uint32	refcnt;		/* # of times parent points to child. */
	LIST_ENTRY(__vrfy_childinfo) links;
}; /* VRFY_CHILDINFO */

/*******************************************************
 * Remaining Log.
 * These need to be defined after the general includes
 * because they need rep.h from above.
 *******************************************************/
/*
 * Test if the environment is currently logging changes.  If we're in recovery
 * or we're a replication client, we don't need to log changes because they're
 * already in the log, even though we have a fully functional log system.
 */
#define	DBENV_LOGGING(env) (LOGGING_ON(env) && !IS_REP_CLIENT(env) && (!IS_RECOVERING(env)))
/*
 * Test if we need to log a change.  By default, we don't log operations without
 * associated transactions, unless DIAGNOSTIC, DEBUG_ROP or DEBUG_WOP are on.
 * This is because we want to get log records for read/write operations, and, if
 * we are trying to debug something, more information is always better.
 *
 * The DBC_RECOVER flag is set when we're in abort, as well as during recovery;
 * thus DBC_LOGGING may be false for a particular dbc even when DBENV_LOGGING
 * is true.
 *
 * We explicitly use LOGGING_ON/IS_REP_CLIENT here because we don't want to pull
 * in the log headers, which IS_RECOVERING (and thus DBENV_LOGGING) rely on, and
 * because DBC_RECOVER should be set anytime IS_RECOVERING would be true.
 *
 * If we're not in recovery (master - doing an abort or a client applying
 * a txn), then a client's only path through here is on an internal
 * operation, and a master's only path through here is a transactional
 * operation.  Detect if either is not the case.
 */
#if defined(DIAGNOSTIC) || defined(DEBUG_ROP) || defined(DEBUG_WOP)
	#define	DBC_LOGGING(dbc) __dbc_logging(dbc)
#else
	#define	DBC_LOGGING(dbc) ((dbc)->txn != NULL && LOGGING_ON((dbc)->env) && !F_ISSET((dbc), DBC_RECOVER) && !IS_REP_CLIENT((dbc)->env))
#endif
//
//
//
#define THROW_BDBI(expr) {if((expr) != 0){goto __scatch;}}
//
// Joins use a join cursor that is similar to a regular DB cursor except
// that it only supports c_get and c_close functionality.  Also, it does
// not support the full range of flags for get.
//
typedef struct __join_cursor {
	uint8 *j_exhausted;	/* Array of flags; is cursor i exhausted? */
	DBC	**j_curslist;	/* Array of cursors in the join: constant. */
	DBC	**j_fdupcurs;	/* Cursors w/ first instances of current dup. */
	DBC	**j_workcurs;	/* Scratch cursor copies to muck with. */
	DB	 *j_primary;	/* Primary dbp. */
	DBT	  j_key;	// Used to do lookups. 
	DBT	  j_rdata;	// Memory used for data return. 
	uint32 j_ncurs;	// How many cursors do we have? 
#define	JOIN_RETRY	0x01 // Error on primary get; re-return same key.
	uint32 flags;
} JOIN_CURSOR;
//
#endif /* !_DB_INT_H_ */
