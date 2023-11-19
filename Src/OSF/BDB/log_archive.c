/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2011 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop

static int __absname(ENV*, char *, char *, char **);
static int __build_data(ENV*, char *, char ***);
static int __cmpfunc(const void *, const void *);
static int __log_archive(ENV*, char **[], uint32);
static int __usermem(ENV*, char ***);
/*
 * __log_archive_pp --
 *	ENV->log_archive pre/post processing.
 */
int __log_archive_pp(DB_ENV * dbenv, char *** listp, uint32 flags)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->lg_handle, "DB_ENV->log_archive", DB_INIT_LOG);
#undef  OKFLAGS
#define OKFLAGS (DB_ARCH_ABS|DB_ARCH_DATA|DB_ARCH_LOG|DB_ARCH_REMOVE)
	if(flags != 0) {
		if((ret = __db_fchk(env, "DB_ENV->log_archive", flags, OKFLAGS)) != 0)
			return ret;
		if((ret = __db_fcchk(env, "DB_ENV->log_archive", flags, DB_ARCH_DATA, DB_ARCH_LOG)) != 0)
			return ret;
		if((ret = __db_fcchk(env, "DB_ENV->log_archive", flags, DB_ARCH_REMOVE, DB_ARCH_ABS|DB_ARCH_DATA|DB_ARCH_LOG)) != 0)
			return ret;
	}
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__log_archive(env, listp, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __log_archive --
 *	ENV->log_archive.  Internal.
 */
static int __log_archive(ENV * env, char *** listp, uint32 flags)
{
	DBT rec;
	DB_LOGC * logc;
	DB_LSN stable_lsn;
	uint array_size, n;
	uint32 fnum;
	int handle_check, t_ret;
	char ** arrayp, * p, * pref;
#ifdef HAVE_GETCWD
	char path[DB_MAXPATHLEN];
#endif
	DB_LOG * dblp = env->lg_handle;
	LOG * lp = static_cast<LOG *>(dblp->reginfo.primary);
	char ** array = NULL;
	char * name = NULL;
	int ret = 0;
	COMPQUIET(fnum, 0);
	if(flags != DB_ARCH_REMOVE)
		*listp = NULL;
	/* There are no log files if logs are in memory. */
	if(lp->db_log_inmemory) {
		LF_CLR(~DB_ARCH_DATA);
		if(flags == 0)
			return 0;
	}
	/*
	 * Check if the user wants the list of log files to remove and we're
	 * at a bad time in replication initialization.
	 */
	handle_check = 0;
	if(!LF_ISSET(DB_ARCH_DATA) && !LF_ISSET(DB_ARCH_LOG)) {
		/*
		 * If we're locked out, just return success.  No files
		 * can be archived right now.  Any other error pass back to the caller.
		 */
		handle_check = IS_ENV_REPLICATED(env);
		if(handle_check && (ret = __archive_rep_enter(env)) != 0)
			return (ret == DB_REP_LOCKOUT) ? 0 : ret;
	}
	/*
	 * Prepend the original absolute pathname if the user wants an
	 * absolute path to the database environment directory.
	 */
#ifdef HAVE_GETCWD
	if(LF_ISSET(DB_ARCH_ABS)) {
		/*
		 * XXX
		 * Can't trust getcwd(3) to set a valid errno, so don't display
		 * one unless we know it's good.  It's likely a permissions
		 * problem: use something bland and useless in the default
		 * return value, so we don't send somebody off in the wrong
		 * direction.
		 */
		__os_set_errno(0);
		if(getcwd(path, sizeof(path)) == NULL) {
			ret = __os_get_errno();
			__db_err(env, ret, DB_STR("2570", "no absolute path for the current directory"));
			goto err;
		}
		pref = path;
	}
	else
#endif
	pref = NULL;
	LF_CLR(DB_ARCH_ABS);
	switch(flags) {
	    case DB_ARCH_DATA:
			ret = __build_data(env, pref, listp);
			goto err;
	    case DB_ARCH_LOG:
			memzero(&rec, sizeof(rec));
			if((ret = __log_cursor(env, &logc)) != 0)
				goto err;
#ifdef UMRW
			ZERO_LSN(stable_lsn);
#endif
			ret = __logc_get(logc, &stable_lsn, &rec, DB_LAST);
			if((t_ret = __logc_close(logc)) != 0 && ret == 0)
				ret = t_ret;
			if(ret)
				goto err;
			fnum = stable_lsn.file;
			break;
	    case DB_ARCH_REMOVE:
			__log_autoremove(env);
			goto err;
	    case 0:
			ret = __log_get_stable_lsn(env, &stable_lsn, 1);
			/*
			 * A return of DB_NOTFOUND means the checkpoint LSN
			 * is before the beginning of the log files we have.
			 * This is not an error; it just means we're done.
			 */
			if(ret) {
				if(ret == DB_NOTFOUND)
					ret = 0;
				goto err;
			}
			/* Remove any log files before the last stable LSN. */
			fnum = stable_lsn.file-1;
			break;
	    default:
			ret = __db_unknown_path(env, "__log_archive");
			goto err;
	}
#define LIST_INCREMENT  64
	/* Get some initial space. */
	array_size = 64;
	if((ret = __os_malloc(env, sizeof(char *)*array_size, &array)) != 0)
		goto err;
	array[0] = NULL;
	/* Build an array of the file names. */
	for(n = 0; fnum > 0; --fnum) {
		if((ret = __log_name(dblp, fnum, &name, NULL, 0)) != 0) {
			__os_free(env, name);
			goto err;
		}
		if(__os_exists(env, name, NULL) != 0) {
			__os_free(env, name);
			name = NULL;
			if(LF_ISSET(DB_ARCH_LOG) && fnum == stable_lsn.file)
				continue;
			break;
		}
		if(n >= array_size-2) {
			array_size += LIST_INCREMENT;
			if((ret = __os_realloc(env, sizeof(char *)*array_size, &array)) != 0)
				goto err;
		}
		if(pref) {
			if((ret = __absname(env, pref, name, &array[n])) != 0)
				goto err;
			__os_free(env, name);
		}
		else if((p = __db_rpath(name)) != NULL) {
			if((ret = __os_strdup(env, p+1, &array[n])) != 0)
				goto err;
			__os_free(env, name);
		}
		else
			array[n] = name;
		name = NULL;
		array[++n] = NULL;
	}
	// If there's nothing to return, we're done
	if(n == 0)
		goto err;
	// Sort the list
	qsort(array, (size_t)n, sizeof(char *), __cmpfunc);
	// Rework the memory. 
	if((ret = __usermem(env, &array)) != 0)
		goto err;
	ASSIGN_PTR(listp, array);
	if(0) {
err:
		if(array) {
			for(arrayp = array; *arrayp != NULL; ++arrayp)
				__os_free(env, *arrayp);
			__os_free(env, array);
		}
		__os_free(env, name);
	}
	if(handle_check && (t_ret = __archive_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;
	return ret;
}
/*
 * __log_get_stable_lsn --
 *	Get the stable lsn based on where checkpoints are.
 *
 * PUBLIC: int __log_get_stable_lsn __P((ENV *, DB_LSN *, int));
 */
int __log_get_stable_lsn(ENV * env, DB_LSN * stable_lsn, int group_wide)
{
	DBT rec;
	DB_LOGC * logc;
	__txn_ckp_args * ckp_args;
	int t_ret;
	LOG * lp = (LOG *)env->lg_handle->reginfo.primary;
	int ret = 0;
	memzero(&rec, sizeof(rec));
	if(!TXN_ON(env)) {
		if((ret = __log_get_cached_ckp_lsn(env, stable_lsn)) != 0)
			goto err;
		/*
		 * No need to check for a return value of DB_NOTFOUND;
		 * __txn_findlastckp returns 0 if no checkpoint record
		 * is found.  Instead of checking the return value, we
		 * check to see if the return LSN has been filled in.
		 */
		if(IS_ZERO_LSN(*stable_lsn) && (ret = __txn_findlastckp(env, stable_lsn, NULL)) != 0)
			goto err;
		/*
		 * If the LSN has not been filled in return DB_NOTFOUND
		 * so that the caller knows it may be done.
		 */
		if(IS_ZERO_LSN(*stable_lsn)) {
			ret = DB_NOTFOUND;
			goto err;
		}
	}
	else if((ret = __txn_getckp(env, stable_lsn)) != 0)
		goto err;
	if((ret = __log_cursor(env, &logc)) != 0)
		goto err;
	/*
	 * Read checkpoint records until we find one that is on disk,
	 * then copy the ckp_lsn to the stable_lsn;
	 */
	while((ret = __logc_get(logc, stable_lsn, &rec, DB_SET)) == 0 && (ret = __txn_ckp_read(env, rec.data, &ckp_args)) == 0) {
		if(stable_lsn->file < lp->s_lsn.file || (stable_lsn->file == lp->s_lsn.file && stable_lsn->Offset_ < lp->s_lsn.Offset_)) {
			*stable_lsn = ckp_args->ckp_lsn;
			__os_free(env, ckp_args);
			break;
		}
		*stable_lsn = ckp_args->last_ckp;
		__os_free(env, ckp_args);
	}
	if((t_ret = __logc_close(logc)) != 0 && ret == 0)
		ret = t_ret;
#ifdef  HAVE_REPLICATION_THREADS
	//
	// If we have RepMgr, get the minimum group-aware LSN.
	//
	if(group_wide && ret == 0 && REP_ON(env) && APP_IS_REPMGR(env) && (t_ret = __repmgr_stable_lsn(env, stable_lsn)) != 0)
		ret = t_ret;
#else
	COMPQUIET(group_wide, 0);
#endif
err:
	return ret;
}
//
// Delete any non-essential log files.
//
 void __log_autoremove(ENV * env)
{
	int ret;
	char ** begin, ** list;
	// 
	// Complain if there's an error, but don't return the error to our
	// caller.  Auto-remove is done when writing a log record, and we
	// don't want to fail a write, which could fail the corresponding
	// committing transaction, for a permissions error.
	// 
	if((ret = __log_archive(env, &list, DB_ARCH_ABS)) != 0) {
		if(ret != DB_NOTFOUND)
			__db_err(env, ret, DB_STR("2571", "log file auto-remove"));
		return;
	}
	// Remove the files
	if(list != NULL) {
		for(begin = list; *list != NULL; ++list)
			__os_unlink(env, *list, 0);
		__os_ufree(env, begin);
	}
}
/*
 * __build_data --
 *	Build a list of datafiles for return.
 */
static int __build_data(ENV * env, char * pref, char *** listp)
{
	DBT rec;
	DB_LOGC * logc;
	DB_LSN lsn;
	__dbreg_register_args * argp;
	uint last, n, nxt;
	uint32 rectype;
	int ret, t_ret;
	char ** array, ** arrayp, ** list, ** lp, * p, * real_name;
	// Get some initial space
	uint array_size = 64;
	if((ret = __os_malloc(env, sizeof(char *)*array_size, &array)) != 0)
		return ret;
	array[0] = NULL;
	memzero(&rec, sizeof(rec));
	if((ret = __log_cursor(env, &logc)) != 0)
		return ret;
	for(n = 0; (ret = __logc_get(logc, &lsn, &rec, DB_PREV)) == 0;) {
		if(rec.size < sizeof(rectype)) {
			ret = EINVAL;
			__db_errx(env, DB_STR("2572", "DB_ENV->log_archive: bad log record"));
			break;
		}
		LOGCOPY_32(env, &rectype, rec.data);
		if(rectype != DB___dbreg_register)
			continue;
		if((ret = __dbreg_register_read(env, rec.data, &argp)) != 0) {
			ret = EINVAL;
			__db_errx(env, DB_STR("2573", "DB_ENV->log_archive: unable to read log record"));
			break;
		}
		if(n >= array_size-2) {
			array_size += LIST_INCREMENT;
			if((ret = __os_realloc(env, sizeof(char *)*array_size, &array)) != 0)
				goto free_continue;
		}
		if((ret = __os_strdup(env, PTRCHRC(argp->name.data), &array[n++])) != 0)
			goto free_continue;
		array[n] = NULL;
		if(argp->ftype == DB_QUEUE) {
			if((ret = __qam_extent_names(env, (char *)argp->name.data, &list)) != 0)
				goto q_err;
			for(lp = list; lp != NULL && *lp != NULL; lp++) {
				if(n >= array_size-2) {
					array_size += LIST_INCREMENT;
					if((ret = __os_realloc(env, sizeof(char *)*array_size, &array)) != 0)
						goto q_err;
				}
				if((ret = __os_strdup(env, *lp, &array[n++])) != 0)
					goto q_err;
				array[n] = NULL;
			}
q_err:
			__os_free(env, list);
		}
free_continue:
		__os_free(env, argp);
		if(ret)
			break;
	}
	if(ret == DB_NOTFOUND)
		ret = 0;
	if((t_ret = __logc_close(logc)) != 0 && ret == 0)
		ret = t_ret;
	if(ret)
		goto err1;
	// If there's nothing to return, we're done
	if(n == 0) {
		ret = 0;
		*listp = NULL;
		goto err1;
	}
	// Sort the list
	qsort(array, (size_t)n, sizeof(char *), __cmpfunc);
	//
	// Build the real pathnames, discarding nonexistent files and duplicates.
	//
	for(last = nxt = 0; nxt < n;) {
		//
		// Discard duplicates.  Last is the next slot we're going
		// to return to the user, nxt is the next slot that we're going to consider.
		//
		if(last != nxt) {
			array[last] = array[nxt];
			array[nxt] = NULL;
		}
		for(++nxt; nxt < n && strcmp(array[last], array[nxt]) == 0; ++nxt) {
			__os_free(env, array[nxt]);
			array[nxt] = NULL;
		}
		/* Get the real name. */
		if((ret = __db_appname(env, DB_APP_DATA, array[last], NULL, &real_name)) != 0)
			goto err2;
		/* If the file doesn't exist, ignore it. */
		if(__os_exists(env, real_name, NULL) != 0) {
			__os_free(env, real_name);
			__os_free(env, array[last]);
			array[last] = NULL;
			continue;
		}
		/* Rework the name as requested by the user. */
		__os_free(env, array[last]);
		array[last] = NULL;
		if(pref) {
			ret = __absname(env, pref, real_name, &array[last]);
			__os_free(env, real_name);
			if(ret)
				goto err2;
		}
		else if((p = __db_rpath(real_name)) != NULL) {
			ret = __os_strdup(env, p+1, &array[last]);
			__os_free(env, real_name);
			if(ret)
				goto err2;
		}
		else
			array[last] = real_name;
		++last;
	}
	/* NULL-terminate the list. */
	array[last] = NULL;
	/* Rework the memory. */
	if((ret = __usermem(env, &array)) != 0)
		goto err1;
	*listp = array;
	return 0;

err2:   /*
	 * XXX
	 * We've possibly inserted NULLs into the array list, so clean up a
	 * bit so that the other error processing works.
	 */
	if(array != NULL)
		for(; nxt < n; ++nxt)
			__os_free(env, array[nxt]);
	// @fallthrough
err1:
	if(array) {
		for(arrayp = array; *arrayp != NULL; ++arrayp)
			__os_free(env, *arrayp);
		__os_free(env, array);
	}
	return ret;
}
/*
 * __absname --
 *	Return an absolute path name for the file.
 */
static int __absname(ENV * env, char * pref, char * name, char ** newnamep)
{
	int ret;
	char * newname;
	size_t l_name = sstrlen(name);
	int isabspath = SFsPath::IsWindowsPathPrefix(name);
	size_t l_pref = isabspath ? 0 : sstrlen(pref);
	/* Malloc space for concatenating the two. */
	if((ret = __os_malloc(env, l_pref+l_name+2, &newname)) != 0)
		return ret;
	*newnamep = newname;
	/* Build the name.  If `name' is an absolute path, ignore any prefix. */
	if(!isabspath) {
		memcpy(newname, pref, l_pref);
		if(sstrchr(PATH_SEPARATOR, newname[l_pref-1]) == NULL)
			newname[l_pref++] = PATH_SEPARATOR[0];
	}
	memcpy(newname+l_pref, name, l_name+1);
	return 0;
}
/*
 * __usermem --
 *	Create a single chunk of memory that holds the returned information.
 *	If the user has their own SAlloc::M routine, use it.
 */
static int __usermem(ENV * env, char *** listp)
{
	size_t len;
	int ret;
	char ** array, ** arrayp, ** orig, * strp;
	/* Find out how much space we need. */
	for(len = 0, orig = *listp; *orig != NULL; ++orig)
		len += sizeof(char *)+sstrlen(*orig)+1;
	len += sizeof(char *);
	/* Allocate it and set up the pointers. */
	if((ret = __os_umalloc(env, len, &array)) != 0)
		return ret;
	strp = (char *)(array+(orig-*listp)+1);
	/* Copy the original information into the new memory. */
	for(orig = *listp, arrayp = array; *orig != NULL; ++orig, ++arrayp) {
		len = sstrlen(*orig);
		memcpy(strp, *orig, len+1);
		*arrayp = strp;
		strp += len+1;
		__os_free(env, *orig);
	}
	/* NULL-terminate the list. */
	*arrayp = NULL;
	__os_free(env, *listp);
	*listp = array;
	return 0;
}

static int __cmpfunc(const void * p1, const void * p2)
{
	return strcmp(*((char * const *)p1), *((char * const *)p2));
}
