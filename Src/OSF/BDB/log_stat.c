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

#ifdef HAVE_STATISTICS
static int __log_print_all __P((ENV*, uint32));
static int __log_print_stats __P((ENV*, uint32));
static int __log_stat __P((ENV*, DB_LOG_STAT**, uint32));
/*
 * __log_stat_pp --
 *	DB_ENV->log_stat pre/post processing.
 *
 * PUBLIC: int __log_stat_pp __P((DB_ENV *, DB_LOG_STAT **, uint32));
 */
int __log_stat_pp(DB_ENV * dbenv, DB_LOG_STAT ** statp, uint32 flags)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->lg_handle, "DB_ENV->log_stat", DB_INIT_LOG);
	if((ret = __db_fchk(env, "DB_ENV->log_stat", flags, DB_STAT_CLEAR)) != 0)
		return ret;
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__log_stat(env, statp, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __log_stat --
 *	DB_ENV->log_stat.
 */
static int __log_stat(ENV * env, DB_LOG_STAT ** statp, uint32 flags)
{
	DB_LOG * dblp;
	DB_LOG_STAT * stats;
	LOG * lp;
	int ret;
	*statp = NULL;
	dblp = env->lg_handle;
	lp = static_cast<LOG *>(dblp->reginfo.primary);
	if((ret = __os_umalloc(env, sizeof(DB_LOG_STAT), &stats)) != 0)
		return ret;
	/* Copy out the global statistics. */
	LOG_SYSTEM_LOCK(env);
	*stats = lp->stat;
	if(LF_ISSET(DB_STAT_CLEAR))
		memzero(&lp->stat, sizeof(lp->stat));
	stats->st_magic = lp->persist.magic;
	stats->st_version = lp->persist.version;
	stats->st_mode = lp->filemode;
	stats->st_lg_bsize = lp->buffer_size;
	stats->st_lg_size = lp->log_nsize;
	__mutex_set_wait_info(env, lp->mtx_region, &stats->st_region_wait, &stats->st_region_nowait);
	if(LF_ISSET(DB_STAT_CLEAR|DB_STAT_SUBSYSTEM) == DB_STAT_CLEAR)
		__mutex_clear(env, lp->mtx_region);
	stats->st_regsize = dblp->reginfo.rp->size;
	stats->st_cur_file = lp->lsn.file;
	stats->st_cur_offset = lp->lsn.Offset_;
	stats->st_disk_file = lp->s_lsn.file;
	stats->st_disk_offset = lp->s_lsn.Offset_;
	LOG_SYSTEM_UNLOCK(env);
	*statp = stats;
	return 0;
}
/*
 * __log_stat_print_pp --
 *	DB_ENV->log_stat_print pre/post processing.
 *
 * PUBLIC: int __log_stat_print_pp(DB_ENV *, uint32);
 */
int __log_stat_print_pp(DB_ENV * dbenv, uint32 flags)
{
	DB_THREAD_INFO * ip;
	ENV * env;
	int ret;
	env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->lg_handle, "DB_ENV->log_stat_print", DB_INIT_LOG);
	if((ret = __db_fchk(env, "DB_ENV->log_stat_print", flags, DB_STAT_ALL|DB_STAT_ALLOC|DB_STAT_CLEAR)) != 0)
		return ret;
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__log_stat_print(env, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __log_stat_print --
 *	DB_ENV->log_stat_print method.
 *
 * PUBLIC: int __log_stat_print __P((ENV *, uint32));
 */
int __log_stat_print(ENV * env, uint32 flags)
{
	int ret;
	uint32 orig_flags = flags;
	LF_CLR(DB_STAT_CLEAR|DB_STAT_SUBSYSTEM);
	if(flags == 0 || LF_ISSET(DB_STAT_ALL)) {
		ret = __log_print_stats(env, orig_flags);
		if(flags == 0 || ret != 0)
			return ret;
	}
	if(LF_ISSET(DB_STAT_ALL) && (ret = __log_print_all(env, orig_flags)) != 0)
		return ret;
	return 0;
}
/*
 * __log_print_stats --
 *	Display default log region statistics.
 */
static int __log_print_stats(ENV * env, uint32 flags)
{
	DB_LOG_STAT * sp;
	int ret;
	if((ret = __log_stat(env, &sp, flags)) != 0)
		return ret;
	if(LF_ISSET(DB_STAT_ALL))
		__db_msg(env, "Default logging region information:");
	STAT_HEX("Log magic number", sp->st_magic);
	STAT_ULONG("Log version number", sp->st_version);
	__db_dlbytes(env, "Log record cache size", (ulong)0, (ulong)0, (ulong)sp->st_lg_bsize);
	__db_msg(env, "%#o\tLog file mode", sp->st_mode);
	if(sp->st_lg_size%MEGABYTE == 0)
		__db_msg(env, "%luMb\tCurrent log file size", (ulong)sp->st_lg_size/MEGABYTE);
	else if(sp->st_lg_size%1024 == 0)
		__db_msg(env, "%luKb\tCurrent log file size", (ulong)sp->st_lg_size/1024);
	else
		__db_msg(env, "%lu\tCurrent log file size", (ulong)sp->st_lg_size);
	__db_dl(env, "Initial fileid allocation", (ulong)sp->st_fileid_init);
	__db_dl(env, "Current fileids in use", (ulong)sp->st_nfileid);
	__db_dl(env, "Maximum fileids used", (ulong)sp->st_maxnfileid);
	__db_dl(env, "Records entered into the log", (ulong)sp->st_record);
	__db_dlbytes(env, "Log bytes written", (ulong)0, (ulong)sp->st_w_mbytes, (ulong)sp->st_w_bytes);
	__db_dlbytes(env, "Log bytes written since last checkpoint", (ulong)0, (ulong)sp->st_wc_mbytes, (ulong)sp->st_wc_bytes);
	__db_dl(env, "Total log file I/O writes", (ulong)sp->st_wcount);
	__db_dl(env, "Total log file I/O writes due to overflow", (ulong)sp->st_wcount_fill);
	__db_dl(env, "Total log file flushes", (ulong)sp->st_scount);
	__db_dl(env, "Total log file I/O reads", (ulong)sp->st_rcount);
	STAT_ULONG("Current log file number", sp->st_cur_file);
	STAT_ULONG("Current log file offset", sp->st_cur_offset);
	STAT_ULONG("On-disk log file number", sp->st_disk_file);
	STAT_ULONG("On-disk log file offset", sp->st_disk_offset);

	__db_dl(env, "Maximum commits in a log flush", (ulong)sp->st_maxcommitperflush);
	__db_dl(env, "Minimum commits in a log flush", (ulong)sp->st_mincommitperflush);
	__db_dlbytes(env, "Region size", (ulong)0, (ulong)0, (ulong)sp->st_regsize);
	__db_dl_pct(env, "The number of region locks that required waiting", (ulong)sp->st_region_wait, DB_PCT(sp->st_region_wait, sp->st_region_wait+sp->st_region_nowait), 0);
	__os_ufree(env, sp);
	return 0;
}
/*
 * __log_print_all --
 *	Display debugging log region statistics.
 */
static int __log_print_all(ENV * env, uint32 flags)
{
	static const FN fn[] = {
		{ DBLOG_RECOVER,        "DBLOG_RECOVER" },
		{ DBLOG_FORCE_OPEN,     "DBLOG_FORCE_OPEN" },
		{ DBLOG_AUTOREMOVE,     "DBLOG_AUTOREMOVE"},
		{ DBLOG_DIRECT,         "DBLOG_DIRECT"},
		{ DBLOG_DSYNC,          "DBLOG_DSYNC"},
		{ DBLOG_FORCE_OPEN,     "DBLOG_FORCE_OPEN"},
		{ DBLOG_INMEMORY,       "DBLOG_INMEMORY"},
		{ DBLOG_OPENFILES,      "DBLOG_OPENFILES"},
		{ DBLOG_RECOVER,        "DBLOG_RECOVER"},
		{ DBLOG_ZERO,           "DBLOG_ZERO"},
		{ 0,                    NULL }
	};
	DB_LOG * dblp = env->lg_handle;
	LOG * lp = static_cast<LOG *>(dblp->reginfo.primary);
	LOG_SYSTEM_LOCK(env);
	__db_print_reginfo(env, &dblp->reginfo, "Log", flags);
	__db_msg_db_line(env);
	__db_msg(env, "DB_LOG handle information:");
	__mutex_print_debug_single(env, "DB_LOG handle mutex", dblp->mtx_dbreg, flags);
	STAT_ULONG("Log file name", dblp->lfname);
	__db_print_fh(env, "Log file handle", dblp->lfhp, flags);
	__db_prflags(env, NULL, dblp->flags, fn, NULL, "\tFlags");

	__db_msg_db_line(env);
	__db_msg(env, "LOG handle information:");
	__mutex_print_debug_single(env, "LOG region mutex", lp->mtx_region, flags);
	__mutex_print_debug_single(env, "File name list mutex", lp->mtx_filelist, flags);
	STAT_HEX("persist.magic", lp->persist.magic);
	STAT_ULONG("persist.version", lp->persist.version);
	__db_dlbytes(env, "persist.log_size", (ulong)0, (ulong)0, lp->persist.log_size);
	STAT_FMT("log file permissions mode", "%#lo", ulong, lp->filemode);
	STAT_LSN("current file offset LSN", &lp->lsn);
	STAT_LSN("first buffer byte LSN", &lp->lsn);
	STAT_ULONG("current buffer offset", lp->b_off);
	STAT_ULONG("current file write offset", lp->w_off);
	STAT_ULONG("length of last record", lp->len);
	STAT_LONG("log flush in progress", lp->in_flush);
	__mutex_print_debug_single(env, "Log flush mutex", lp->mtx_flush, flags);
	STAT_LSN("last sync LSN", &lp->s_lsn);
	/*
	 * Don't display the replication fields here, they're displayed as part
	 * of the replication statistics.
	 */
	STAT_LSN("cached checkpoint LSN", &lp->cached_ckp_lsn);
	__db_dlbytes(env, "log buffer size", (ulong)0, (ulong)0, lp->buffer_size);
	__db_dlbytes(env, "log file size", (ulong)0, (ulong)0, lp->log_size);
	__db_dlbytes(env, "next log file size", (ulong)0, (ulong)0, lp->log_nsize);
	STAT_ULONG("transactions waiting to commit", lp->ncommit);
	STAT_LSN("LSN of first commit", &lp->t_lsn);
	LOG_SYSTEM_UNLOCK(env);
	return 0;
}

#else /* !HAVE_STATISTICS */

int __log_stat_pp(DB_ENV * dbenv, DB_LOG_STAT ** statp, uint32 flags)
{
	COMPQUIET(statp, 0);
	COMPQUIET(flags, 0);
	return __db_stat_not_built(dbenv->env);
}

int __log_stat_print_pp(DB_ENV * dbenv, uint32 flags)
{
	COMPQUIET(flags, 0);
	return __db_stat_not_built(dbenv->env);
}
#endif
