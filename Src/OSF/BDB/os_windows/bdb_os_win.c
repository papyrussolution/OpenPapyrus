// BSB_OS_WIN.C
//
#include "db_config.h"
#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/lock.h"
#include "dbinc/mp.h"
#include "dbinc/crypto.h"
#include "dbinc/btree.h"
#include "dbinc/hash.h"
#pragma hdrstop

#if 0 // excl file list {
	os_windows\os_abs.c
	os_windows\os_mkdir.c
	os_windows\os_open.c
	os_windows\os_clock.c
	os_windows\os_config.c
	os_windows\os_cpu.c
	os_windows\os_dir.c
	os_windows\os_errno.c
	os_windows\os_fid.c
	os_windows\os_flock.c
	os_windows\os_fsync.c
	os_windows\os_getenv.c
	os_windows\os_handle.c
	os_windows\os_map.c
	os_windows\os_pid.c
	os_windows\os_rw.c
	os_windows\os_seek.c
	os_windows\os_stat.c
	os_windows\os_truncate.c
	os_windows\os_unlink.c
	os_windows\os_yield.c
#endif // } 0 excl file list
//
// __os_abspath --
// Return if a path is an absolute path.
//
int __os_abspath(const char * path)
{
	/*
	 * !!!
	 * Check for drive specifications, e.g., "C:".  In addition, the path
	 * separator used by the win32 DB (PATH_SEPARATOR) is \; look for both
	 * / and \ since these are user-input paths.
	 */
	const size_t len = sstrlen(path);
	if(!len)
		return 0;
	if(len >= 3 && isalpha(path[0]) && path[1] == ':')
		path += 2;
	return (path[0] == '/' || path[0] == '\\');
}
//
// Descr: Create a directory.
//
int __os_mkdir(ENV * env, const char * name, int mode)
{
	DB_ENV * dbenv = env ? env->dbenv : 0;
	_TCHAR * tname;
	int ret;
	if(dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS|DB_VERB_FILEOPS_ALL))
		__db_msg(env, DB_STR_A("0013", "fileops: mkdir %s", "%s"), name);
	/* Make the directory, with paranoid permissions. */
	TO_TSTRING(env, name, tname, ret);
	if(ret != 0)
		return ret;
	RETRY_CHK(!CreateDirectory(tname, NULL), ret);
	FREE_STRING(env, tname);
	if(ret != 0)
		return __os_posix_err(ret);
	return ret;
}
//
// Descr: Open a file descriptor (including page size and log size information).
//
int __os_open(ENV * env, const char * name, uint32 page_size, uint32 flags, int mode, DB_FH ** fhpp)
{
	DB_ENV * dbenv = env ? env->dbenv : 0;
	DB_FH * fhp;
#ifndef DB_WINCE
	DWORD cluster_size, sector_size, free_clusters, total_clusters;
	_TCHAR * drive, dbuf[4]; /* <letter><colon><slash><nul> */
#endif
	int access, attr, createflag, nrepeat, ret, share;
	_TCHAR * tname = 0;
	*fhpp = NULL;
	if(dbenv && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS|DB_VERB_FILEOPS_ALL))
		__db_msg(env, DB_STR_A("0025", "fileops: open %s", "%s"), name);
#undef  OKFLAGS
#define OKFLAGS (DB_OSO_ABSMODE|DB_OSO_CREATE|DB_OSO_DIRECT|DB_OSO_DSYNC|DB_OSO_EXCL|DB_OSO_RDONLY|DB_OSO_REGION|DB_OSO_SEQ|DB_OSO_TEMP|DB_OSO_TRUNC)
	if((ret = __db_fchk(env, "__os_open", flags, OKFLAGS)) != 0)
		return ret;
	TO_TSTRING(env, name, tname, ret);
	if(ret != 0)
		goto err;
	/*
	 * Allocate the file handle and copy the file name.  We generally only
	 * use the name for verbose or error messages, but on systems where we
	 * can't unlink temporary files immediately, we use the name to unlink
	 * the temporary file when the file handle is closed.
	 *
	 * Lock the ENV handle and insert the new file handle on the list.
	 */
	if((ret = __os_calloc(env, 1, sizeof(DB_FH), &fhp)) != 0)
		return ret;
	if((ret = __os_strdup(env, name, &fhp->name)) != 0)
		goto err;
	if(env != NULL) {
		MUTEX_LOCK(env, env->mtx_env);
		TAILQ_INSERT_TAIL(&env->fdlist, fhp, q);
		MUTEX_UNLOCK(env, env->mtx_env);
		F_SET(fhp, DB_FH_ENVLINK);
	}
	/*
	 * Otherwise, use the Windows/32 CreateFile interface so that we can
	 * play magic games with files to get data flush effects similar to
	 * the POSIX O_DSYNC flag.
	 *
	 * !!!
	 * We currently ignore the 'mode' argument.  It would be possible
	 * to construct a set of security attributes that we could pass to
	 * CreateFile that would accurately represents the mode.  In worst
	 * case, this would require looking up user and all group names and
	 * creating an entry for each.  Alternatively, we could call the
	 * _chmod (partial emulation) function after file creation, although
	 * this leaves us with an obvious race.  However, these efforts are
	 * largely meaningless on FAT, the most common file system, which
	 * only has a "readable" and "writable" flag, applying to all users.
	 */
	access = GENERIC_READ;
	if(!LF_ISSET(DB_OSO_RDONLY))
		access |= GENERIC_WRITE;
#ifdef DB_WINCE
	/*
	 * WinCE translates these flags into share flags for
	 * CreateFileForMapping.
	 * Also WinCE does not support the FILE_SHARE_DELETE flag.
	 */
	if(LF_ISSET(DB_OSO_REGION))
		share = GENERIC_READ|GENERIC_WRITE;
	else
		share = FILE_SHARE_READ|FILE_SHARE_WRITE;
#else
	share = FILE_SHARE_READ|FILE_SHARE_WRITE;
	if(__os_is_winnt())
		share |= FILE_SHARE_DELETE;
#endif
	attr = FILE_ATTRIBUTE_NORMAL;
	/*
	 * Reproduce POSIX 1003.1 semantics: if O_CREATE and O_EXCL are both
	 * specified, fail, returning EEXIST, unless we create the file.
	 */
	if(LF_ISSET(DB_OSO_CREATE) && LF_ISSET(DB_OSO_EXCL))
		createflag = CREATE_NEW;        /* create only if !exist*/
	else if(!LF_ISSET(DB_OSO_CREATE) && LF_ISSET(DB_OSO_TRUNC))
		createflag = TRUNCATE_EXISTING;  /* truncate, fail if !exist */
	else if(LF_ISSET(DB_OSO_TRUNC))
		createflag = CREATE_ALWAYS;     /* create and truncate */
	else if(LF_ISSET(DB_OSO_CREATE))
		createflag = OPEN_ALWAYS;       /* open or create */
	else
		createflag = OPEN_EXISTING;     /* open only if existing */
	if(LF_ISSET(DB_OSO_DSYNC)) {
		F_SET(fhp, DB_FH_NOSYNC);
		attr |= FILE_FLAG_WRITE_THROUGH;
	}
#ifndef DB_WINCE
	if(LF_ISSET(DB_OSO_SEQ))
		attr |= FILE_FLAG_SEQUENTIAL_SCAN;
	else
		attr |= FILE_FLAG_RANDOM_ACCESS;
#endif
	if(LF_ISSET(DB_OSO_TEMP))
		attr |= FILE_FLAG_DELETE_ON_CLOSE;
	/*
	 * We can turn filesystem buffering off if the page size is a
	 * multiple of the disk's sector size. To find the sector size,
	 * we call GetDiskFreeSpace, which expects a drive name like "d:\\"
	 * or NULL for the current disk (i.e., a relative path).
	 *
	 * WinCE only has GetDiskFreeSpaceEx which does not
	 * return the sector size.
	 */
#ifndef DB_WINCE
	if(LF_ISSET(DB_OSO_DIRECT) && page_size != 0 && name[0] != '\0') {
		if(name[1] == ':') {
			drive = dbuf;
			_sntprintf(dbuf, sizeof(dbuf), _T("%c:\\"), tname[0]);
		}
		else
			drive = NULL;
		/*
		 * We ignore all results except sectorsize, but some versions
		 * of Windows require that the parameters are non-NULL.
		 */
		if(GetDiskFreeSpace(drive, &cluster_size, &sector_size, &free_clusters, &total_clusters) && page_size%sector_size == 0)
			attr |= FILE_FLAG_NO_BUFFERING;
	}
#endif
	fhp->handle = fhp->trunc_handle = INVALID_HANDLE_VALUE;
	for(nrepeat = 1;; ++nrepeat) {
		if(fhp->handle == INVALID_HANDLE_VALUE) {
#ifdef DB_WINCE
			if(LF_ISSET(DB_OSO_REGION))
				fhp->handle = CreateFileForMapping(tname, access, share, NULL, createflag, attr, 0);
			else
#endif
				fhp->handle = ::CreateFile(tname, access, share, NULL, createflag, attr, 0);
		}
#ifdef HAVE_FTRUNCATE
		/*
		 * Older versions of WinCE may not support truncate, if so, the
		 * HAVE_FTRUNCATE macro should be #undef'ed, and we
		 * don't need to open this second handle.
		 *
		 * WinCE dose not support opening a second handle on the same
		 * file via CreateFileForMapping, but this dose not matter
		 * since we are not truncating region files but database files.
		 *
		 * But some older versions of WinCE even
		 * dose not allow a second handle opened via CreateFile. If
		 * this is the case, users will need to #undef the
		 * HAVE_FTRUNCATE macro in build_wince/db_config.h.
		 */
		/*
		 * Windows does not provide truncate directly.  There is no
		 * safe way to use a handle for truncate concurrently with
		 * reads or writes.  To deal with this, we open a second handle
		 * used just for truncating.
		 */
		if(fhp->handle != INVALID_HANDLE_VALUE && !LF_ISSET(DB_OSO_RDONLY|DB_OSO_TEMP) && fhp->trunc_handle == INVALID_HANDLE_VALUE
 #ifdef DB_WINCE
		   /* Do not open trunc handle for region files. */
		   && (!LF_ISSET(DB_OSO_REGION))
 #endif
		   )
			fhp->trunc_handle = CreateFile(tname, access, share, NULL, OPEN_EXISTING, attr, 0);
#endif

#ifndef HAVE_FTRUNCATE
		if(fhp->handle == INVALID_HANDLE_VALUE)
#else
		if(fhp->handle == INVALID_HANDLE_VALUE || (!LF_ISSET(DB_OSO_RDONLY|DB_OSO_TEMP) && fhp->trunc_handle == INVALID_HANDLE_VALUE
 #ifdef DB_WINCE
		    /* Do not open trunc handle for region files. */
		    && (!LF_ISSET(DB_OSO_REGION))
 #endif
		   ))
#endif
		{
			/*
			 * If it's a "temporary" error, we retry up to 3 times,
			 * waiting up to 12 seconds.  While it's not a problem
			 * if we can't open a database, an inability to open a
			 * log file is cause for serious dismay.
			 */
			ret = __os_posix_err(__os_get_syserr());
			if((ret != ENFILE && ret != EMFILE && ret != ENOSPC) || nrepeat > 3)
				goto err;
			__os_yield(env, nrepeat*2, 0);
		}
		else
			break;
	}
	FREE_STRING(env, tname);
	if(LF_ISSET(DB_OSO_REGION))
		F_SET(fhp, DB_FH_REGION);
	F_SET(fhp, DB_FH_OPENED);
	*fhpp = fhp;
	return 0;
err:
	FREE_STRING(env, tname);
	if(fhp != NULL)
		__os_closehandle(env, fhp);
	return ret;
}
//
// __os_gettime --
// Return the current time-of-day clock in seconds and nanoseconds.
//
void __os_gettime(ENV * env, db_timespec * tp, int monotonic)
{
	if(monotonic) {
		/*
		 * The elapsed time is stored as a DWORD value, so time wraps
		 * around to zero if the system runs for 49.7 days.  Initialize
		 * a base value with 50 days worth of seconds, and add 50 more
		 * days every time the counter wraps.  That ensures we always
		 * move forward.
		 *
		 * It's possible this code could race, but the danger is we
		 * would increment base_seconds more than once per wrap and
		 * eventually overflow, which is a pretty remote possibility.
		 */
#define TIMER_WRAP_SECONDS      (50*24*60*60)
		static DWORD last_ticks;
		static __time64_t base_seconds;
		DWORD ticks = GetTickCount();
		if(ticks < last_ticks)
			base_seconds += TIMER_WRAP_SECONDS;
		last_ticks = ticks;
		tp->tv_sec = base_seconds+(uint32)(ticks/1000);
		tp->tv_nsec = (uint32)((ticks%1000)*NS_PER_MS);
	}
	else {
#ifdef DB_WINCE
		FILETIME ft;
		LARGE_INTEGER large_int;
		LONGLONG ns_since_epoch, utc1970;
		SYSTEMTIME st;
		GetSystemTime(&st);
		SystemTimeToFileTime(&st, &ft);
		/*
		 * A FILETIME expresses time as 100 nanosecond chunks from
		 * Jan 1, 1601; convert to a timespec where the time is
		 * is expressed in seconds and nanoseconds from Jan 1, 1970.
		 *
		 * UTC_1970 is the number of 100-nano-second chunks from
		 * 1601 to 1970.
		 */
 #define NS100_PER_SEC   (NS_PER_SEC/100)
 #define UTC_1970        (((LONGLONG)27111902<<32)+(LONGLONG)3577643008)
		memcpy(&large_int, &ft, sizeof(large_int));
		utc1970 = UTC_1970;
		ns_since_epoch = (large_int.QuadPart-utc1970);
		tp->tv_sec = (__time64_t)(ns_since_epoch/NS100_PER_SEC);
		tp->tv_nsec = (long)(ns_since_epoch%NS100_PER_SEC);
#else
		struct _timeb now;
		_ftime(&now);
		tp->tv_sec = now.time;
		tp->tv_nsec = now.millitm*NS_PER_MS;
#endif
	}
}
/*
 * __os_is_winnt --
 *	Return 1 if Windows/NT, otherwise 0.
 *
 * PUBLIC: int __os_is_winnt();
 */
int __os_is_winnt()
{
#ifdef DB_WINCE
	return (1);
#else
	static int __os_type = -1;
	/*
	 * The value of __os_type is computed only once, and cached to
	 * avoid the overhead of repeated calls to GetVersion().
	 */
	if(__os_type == -1) {
		if((GetVersion() & 0x80000000) == 0)
			__os_type = 1;
		else
			__os_type = 0;
	}
	return (__os_type);
#endif
}
/*
 * __os_fs_notzero --
 *	Return 1 if allocated filesystem blocks are not zeroed.
 */
int __os_fs_notzero()
{
#ifdef DB_WINCE
	return (1);
#else
	static int __os_notzero = -1;
	OSVERSIONINFO osvi;

	/*
	 * Windows/NT zero-fills pages that were never explicitly written to
	 * the file.  Note however that this is *NOT* documented.  In fact, the
	 * Win32 documentation makes it clear that there are no guarantees that
	 * uninitialized bytes will be zeroed:
	 *
	 *   If the file is extended, the contents of the file between the old
	 *   EOF position and the new position are not defined.
	 *
	 * Experiments confirm that NT/2K/XP all zero fill for both NTFS and
	 * FAT32.  Cygwin also relies on this behavior.  This is the relevant
	 * comment from Cygwin:
	 *
	 *    Oops, this is the bug case - Win95 uses whatever is on the disk
	 *    instead of some known (safe) value, so we must seek back and fill
	 *    in the gap with zeros. - DJ
	 *    Note: this bug doesn't happen on NT4, even though the
	 *    documentation for WriteFile() says that it *may* happen on any OS.
	 *
	 * We're making a bet, here, but we made it a long time ago and haven't
	 * yet seen any evidence that it was wrong.
	 *
	 * Windows 95/98 and On-Time give random garbage, and that breaks
	 * Berkeley DB.
	 *
	 * The value of __os_notzero is computed only once, and cached to
	 * avoid the overhead of repeated calls to GetVersion().
	 */
	if(__os_notzero == -1) {
		if(__os_is_winnt()) {
			osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			GetVersionEx(&osvi);
			if(_tcscmp(osvi.szCSDVersion, _T("RTTarget-32")) == 0)
				__os_notzero = 1;	/* On-Time */
			else
				__os_notzero = 0;	/* Windows/NT */
		} else
			__os_notzero = 1;		/* Not Windows/NT */
	}
	return (__os_notzero);
#endif
}
/*
 * __os_support_direct_io --
 *	Check to see if we support direct I/O.
 */
int __os_support_direct_io()
{
	return (1);
}
/*
 * __os_support_db_register --
 *	Return 1 if the system supports DB_REGISTER.
 */
int __os_support_db_register()
{
#ifdef DB_WINCE
	return (0);
#else
	return (__os_is_winnt());
#endif
}
/*
 * __os_support_replication --
 *	Return 1 if the system supports replication.
 */
int __os_support_replication()
{
#ifdef DB_WINCE
	return (0);
#else
	return (__os_is_winnt());
#endif
}
//
// __os_cpu_count --
// Return the number of CPUs.
//
// PUBLIC: uint32 __os_cpu_count();
//
uint32 __os_cpu_count()
{
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	return ((uint32)SystemInfo.dwNumberOfProcessors);
}
/*
 * __os_dirlist --
 *	Return a list of the files in a directory.
 */
int __os_dirlist(ENV * env, const char * dir, int returndir, char *** namesp, int * cntp)
{
	HANDLE dirhandle;
	WIN32_FIND_DATA fdata;
	int arraysz, cnt, ret;
	char ** names, * onename;
	_TCHAR tfilespec[DB_MAXPATHLEN+1];
	_TCHAR * tdir;

	*namesp = NULL;
	*cntp = 0;

	TO_TSTRING(env, dir, tdir, ret);
	if(ret != 0)
		return ret;
	_sntprintf(tfilespec, DB_MAXPATHLEN, _T("%s%hc*"), tdir, PATH_SEPARATOR[0]);
	/*
	 * On WinCE, FindFirstFile will return INVALID_HANDLE_VALUE when
	 * the searched directory is empty, and set last error to
	 * ERROR_NO_MORE_FILES, on Windows it will return "." instead.
	 */
	if((dirhandle = FindFirstFile(tfilespec, &fdata)) == INVALID_HANDLE_VALUE) {
		if(GetLastError() == ERROR_NO_MORE_FILES)
			return 0;
		return __os_posix_err(__os_get_syserr());
	}
	names = NULL;
	arraysz = cnt = ret = 0;
	for(;; ) {
		if(returndir || (fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) == 0) {
			if(fdata.cFileName[0] == _T('.') && (fdata.cFileName[1] == _T('\0') || (fdata.cFileName[1] == _T('.') && fdata.cFileName[2] == _T('\0'))))
				goto next;
			if(cnt >= arraysz) {
				arraysz += 100;
				if((ret = __os_realloc(env, arraysz*sizeof(names[0]), &names)) != 0)
					goto err;
			}
			/*
			 * FROM_TSTRING doesn't necessarily allocate new
			 * memory, so we must do that explicitly.
			 * Unfortunately, when compiled with UNICODE, we'll
			 * copy twice.
			 */
			FROM_TSTRING(env, fdata.cFileName, onename, ret);
			if(ret != 0)
				goto err;
			ret = __os_strdup(env, onename, &names[cnt]);
			FREE_STRING(env, onename);
			if(ret != 0)
				goto err;
			cnt++;
		}
next:
		if(!FindNextFile(dirhandle, &fdata)) {
			if(GetLastError() == ERROR_NO_MORE_FILES)
				break;
			else {
				ret = __os_posix_err(__os_get_syserr());
				goto err;
			}
		}
	}
err:
	if(!FindClose(dirhandle) && ret == 0)
		ret = __os_posix_err(__os_get_syserr());
	if(ret == 0) {
		*namesp = names;
		*cntp = cnt;
	}
	else if(names != NULL)
		__os_dirfree(env, names, cnt);
	FREE_STRING(env, tdir);
	return ret;
}
/*
 * __os_dirfree --
 *	Free the list of files.
 */
void __os_dirfree(ENV*env, char ** names, int cnt)
{
	while(cnt > 0)
		__os_free(env, names[--cnt]);
	__os_free(env, names);
}
/*
 * __os_get_errno_ret_zero --
 *	Return the last system error, including an error of zero.
 */
int __os_get_errno_ret_zero()
{
	/* This routine must be able to return the same value repeatedly. */
	return errno;
}

/*
 * We've seen cases where system calls failed but errno was never set.  For
 * that reason, __os_get_errno() and __os_get_syserr set errno to EAGAIN if
 * it's not already set, to work around the problem.  For obvious reasons,
 * we can only call this function if we know an error has occurred, that
 * is, we can't test the return for a non-zero value after the get call.
 *
 * __os_get_errno --
 *	Return the last ANSI C "errno" value or EAGAIN if the last error
 *	is zero.
 */
int __os_get_errno()
{
	/* This routine must be able to return the same value repeatedly. */
	if(errno == 0)
		__os_set_errno(EAGAIN);
	return errno;
}

#ifdef HAVE_REPLICATION_THREADS
/*
 * __os_get_neterr --
 *	Return the last networking error or EAGAIN if the last error is zero.
 *
 * PUBLIC: #ifdef HAVE_REPLICATION_THREADS
 * PUBLIC: int __os_get_neterr();
 * PUBLIC: #endif
 */
int __os_get_neterr()
{
	/* This routine must be able to return the same value repeatedly. */
	int err = WSAGetLastError();
	if(err == 0)
		WSASetLastError(err = ERROR_RETRY);
	return err;
}

#endif
/*
 * __os_get_syserr --
 *	Return the last system error or EAGAIN if the last error is zero.
 */
int __os_get_syserr()
{
	/* This routine must be able to return the same value repeatedly. */
	int err = GetLastError();
	if(err == 0)
		SetLastError(err = ERROR_RETRY);
	return err;
}
/*
 * __os_set_errno --
 *	Set the value of errno.
 */
void __os_set_errno(int evalue)
{
	/*
	 * This routine is called by the compatibility interfaces (DB 1.85,
	 * dbm and hsearch).  Force values > 0, that is, not one of DB 2.X
	 * and later's public error returns.  If something bad has happened,
	 * default to EFAULT -- a nasty return.  Otherwise, default to EINVAL.
	 * As the compatibility APIs aren't included on Windows, the Windows
	 * version of this routine doesn't need this behavior.
	 */
	errno = evalue >= 0 ? evalue : (evalue == DB_RUNRECOVERY ? EFAULT : EINVAL);
}
/*
 * __os_strerror --
 *	Return a string associated with the system error.
 */
char * __os_strerror(int error, char * buf, size_t len)
{
#ifdef DB_WINCE
 #define MAX_TMPBUF_LEN 512
	_TCHAR tbuf[MAX_TMPBUF_LEN];
	size_t maxlen;
	DB_ASSERT(NULL, error != 0);
	memzero(tbuf, sizeof(_TCHAR)*MAX_TMPBUF_LEN);
	maxlen = (len > MAX_TMPBUF_LEN ? MAX_TMPBUF_LEN : len);
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, (DWORD)error, 0, tbuf, maxlen-1, NULL);
	if(WideCharToMultiByte(CP_UTF8, 0, tbuf, -1, buf, len, 0, NULL) == 0)
		strncpy(buf, DB_STR("0035", "Error message translation failed."), len);
#else
	DB_ASSERT(NULL, error != 0);
	/*
	 * Explicitly call FormatMessageA, since we want to receive a char
	 * string back, not a tchar string.
	 */
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, (DWORD)error, 0, buf, (DWORD)(len-1), NULL);
	buf[len-1] = '\0';
#endif
	return buf;
}
/*
 * __os_posix_err --
 *	Convert a system error to a POSIX error.
 */
int __os_posix_err(int error)
{
	/* Handle calls on successful returns. */
	if(error == 0)
		return 0;
	/*
	 * Translate the Windows error codes we care about.
	 */
	switch(error) {
	    case ERROR_INVALID_PARAMETER: return EINVAL;
	    case ERROR_FILE_NOT_FOUND:
	    case ERROR_INVALID_DRIVE:
	    case ERROR_PATH_NOT_FOUND: return ENOENT;
	    case ERROR_NO_MORE_FILES:
	    case ERROR_TOO_MANY_OPEN_FILES: return EMFILE;
	    case ERROR_ACCESS_DENIED: return EPERM;
	    case ERROR_INVALID_HANDLE: return EBADF;
	    case ERROR_NOT_ENOUGH_MEMORY: return ENOMEM;
	    case ERROR_DISK_FULL: return ENOSPC;
	    case ERROR_ARENA_TRASHED:
	    case ERROR_BAD_COMMAND:
	    case ERROR_BAD_ENVIRONMENT:
	    case ERROR_BAD_FORMAT:
	    case ERROR_GEN_FAILURE:
	    case ERROR_INVALID_ACCESS:
	    case ERROR_INVALID_BLOCK:
	    case ERROR_INVALID_DATA:
	    case ERROR_READ_FAULT:
	    case ERROR_WRITE_FAULT: return EFAULT;
	    case ERROR_ALREADY_EXISTS:
	    case ERROR_FILE_EXISTS: return EEXIST;
	    case ERROR_NOT_SAME_DEVICE: return EXDEV;
	    case ERROR_WRITE_PROTECT: return EACCES;
	    case ERROR_LOCK_FAILED:
	    case ERROR_LOCK_VIOLATION:
	    case ERROR_NOT_READY:
	    case ERROR_SHARING_VIOLATION: return EBUSY;
	    case ERROR_RETRY: return EINTR;
	}
	/*
	 * Translate the Windows socket error codes.
	 */
	switch(error) {
	    case WSAEADDRINUSE:
#ifdef EADDRINUSE
		return EADDRINUSE;
#else
		break;
#endif
	    case WSAEADDRNOTAVAIL:
#ifdef EADDRNOTAVAIL
		return EADDRNOTAVAIL;
#else
		break;
#endif
	    case WSAEAFNOSUPPORT:
#ifdef EAFNOSUPPORT
		return EAFNOSUPPORT;
#else
		break;
#endif
	    case WSAEALREADY:
#ifdef EALREADY
		return EALREADY;
#else
		break;
#endif
	    case WSAEBADF: return EBADF;
	    case WSAECONNABORTED:
#ifdef ECONNABORTED
		return ECONNABORTED;
#else
		break;
#endif
	    case WSAECONNREFUSED:
#ifdef ECONNREFUSED
		return ECONNREFUSED;
#else
		break;
#endif
	    case WSAECONNRESET:
#ifdef ECONNRESET
		return ECONNRESET;
#else
		break;
#endif
	    case WSAEDESTADDRREQ:
#ifdef EDESTADDRREQ
		return EDESTADDRREQ;
#else
		break;
#endif
	    case WSAEFAULT: return EFAULT;
	    case WSAEHOSTDOWN:
#ifdef EHOSTDOWN
		return EHOSTDOWN;
#else
		break;
#endif
	    case WSAEHOSTUNREACH:
#ifdef EHOSTUNREACH
		return EHOSTUNREACH;
#else
		break;
#endif
	    case WSAEINPROGRESS:
#ifdef EINPROGRESS
		return EINPROGRESS;
#else
		break;
#endif
	    case WSAEINTR: return EINTR;
	    case WSAEINVAL: return EINVAL;
	    case WSAEISCONN:
#ifdef EISCONN
		return EISCONN;
#else
		break;
#endif
	    case WSAELOOP:
#ifdef ELOOP
		return ELOOP;
#else
		break;
#endif
	    case WSAEMFILE: return EMFILE;
	    case WSAEMSGSIZE:
#ifdef EMSGSIZE
		return EMSGSIZE;
#else
		break;
#endif
	    case WSAENAMETOOLONG: return ENAMETOOLONG;
	    case WSAENETDOWN:
#ifdef ENETDOWN
		return ENETDOWN;
#else
		break;
#endif
	    case WSAENETRESET:
#ifdef ENETRESET
		return ENETRESET;
#else
		break;
#endif
	    case WSAENETUNREACH:
#ifdef ENETUNREACH
		return ENETUNREACH;
#else
		break;
#endif
	    case WSAENOBUFS:
#ifdef ENOBUFS
		return ENOBUFS;
#else
		break;
#endif
	    case WSAENOPROTOOPT:
#ifdef ENOPROTOOPT
		return ENOPROTOOPT;
#else
		break;
#endif
	    case WSAENOTCONN:
#ifdef ENOTCONN
		return ENOTCONN;
#else
		break;
#endif
	    case WSANOTINITIALISED: return EAGAIN;
	    case WSAENOTSOCK:
#ifdef ENOTSOCK
		return ENOTSOCK;
#else
		break;
#endif
	    case WSAEOPNOTSUPP: return DB_OPNOTSUP;
	    case WSAEPFNOSUPPORT:
#ifdef EPFNOSUPPORT
		return EPFNOSUPPORT;
#else
		break;
#endif
	    case WSAEPROTONOSUPPORT:
#ifdef EPROTONOSUPPORT
		return EPROTONOSUPPORT;
#else
		break;
#endif
	    case WSAEPROTOTYPE:
#ifdef EPROTOTYPE
		return EPROTOTYPE;
#else
		break;
#endif
	    case WSAESHUTDOWN:
#ifdef ESHUTDOWN
		return ESHUTDOWN;
#else
		break;
#endif
	    case WSAESOCKTNOSUPPORT:
#ifdef ESOCKTNOSUPPORT
		return ESOCKTNOSUPPORT;
#else
		break;
#endif
	    case WSAETIMEDOUT:
#ifdef ETIMEDOUT
		return ETIMEDOUT;
#else
		break;
#endif
	    case WSAETOOMANYREFS:
#ifdef ETOOMANYREFS
		return ETOOMANYREFS;
#else
		break;
#endif
	    case WSAEWOULDBLOCK:
#ifdef EWOULDBLOCK
		return EWOULDBLOCK;
#else
		return EAGAIN;
#endif
	    case WSAHOST_NOT_FOUND:
#ifdef EHOSTUNREACH
		return EHOSTUNREACH;
#else
		break;
#endif
	    case WSASYSNOTREADY: return EAGAIN;
	    case WSATRY_AGAIN: return EAGAIN;
	    case WSAVERNOTSUPPORTED: return DB_OPNOTSUP;
	    case WSAEACCES: return EACCES;
	}
	/*
	 * EFAULT is the default if we don't have a translation.
	 */
	return EFAULT;
}
/*
 * __os_fileid --
 *	Return a unique identifier for a file.
 */
int __os_fileid(ENV * env, const char * fname, int unique_okay, uint8 * fidp)
{
	pid_t pid;
	size_t i;
	uint32 tmp;
	uint8 * p;
	int ret;
	/*
	 * The documentation for GetFileInformationByHandle() states that the
	 * inode-type numbers are not constant between processes.  Actually,
	 * they are, they're the NTFS MFT indexes.  So, this works on NTFS,
	 * but perhaps not on other platforms, and perhaps not over a network.
	 * Can't think of a better solution right now.
	 */
	DB_FH * fhp;
	BY_HANDLE_FILE_INFORMATION fi;
	BOOL retval = FALSE;
	DB_ASSERT(env, fname != NULL);
	/* Clear the buffer. */
	memzero(fidp, DB_FILE_ID_LEN);
	/*
	 * First we open the file, because we're not given a handle to it.
	 * If we can't open it, we're in trouble.
	 */
	if((ret = __os_open(env, fname, 0, DB_OSO_RDONLY, DB_MODE_400, &fhp)) != 0)
		return ret;
	/* File open, get its info */
	if((retval = GetFileInformationByHandle(fhp->handle, &fi)) == FALSE)
		ret = __os_get_syserr();
	__os_closehandle(env, fhp);
	if(retval == FALSE)
		return __os_posix_err(ret);
	/*
	 * We want the three 32-bit words which tell us the volume ID and
	 * the file ID.  We make a crude attempt to copy the bytes over to
	 * the callers buffer.
	 *
	 * We don't worry about byte sexing or the actual variable sizes.
	 *
	 * When this routine is called from the DB access methods, it's only
	 * called once -- whatever ID is generated when a database is created
	 * is stored in the database file's metadata, and that is what is
	 * saved in the mpool region's information to uniquely identify the
	 * file.
	 *
	 * When called from the mpool layer this routine will be called each
	 * time a new thread of control wants to share the file, which makes
	 * things tougher.  As far as byte sexing goes, since the mpool region
	 * lives on a single host, there's no issue of that -- the entire
	 * region is byte sex dependent.  As far as variable sizes go, we make
	 * the simplifying assumption that 32-bit and 64-bit processes will
	 * get the same 32-bit values if we truncate any returned 64-bit value
	 * to a 32-bit value.
	 */
	tmp = (uint32)fi.nFileIndexLow;
	for(p = (uint8 *)&tmp, i = sizeof(uint32); i > 0; --i)
		*fidp++ = *p++;
	tmp = (uint32)fi.nFileIndexHigh;
	for(p = (uint8 *)&tmp, i = sizeof(uint32); i > 0; --i)
		*fidp++ = *p++;
	if(unique_okay) {
		/* Add in 32-bits of (hopefully) unique number. */
		__os_unique_id(env, &tmp);
		for(p = (uint8 *)&tmp, i = sizeof(uint32); i > 0; --i)
			*fidp++ = *p++;
		/*
		 * Initialize/increment the serial number we use to help
		 * avoid fileid collisions.  Note we don't bother with
		 * locking; it's unpleasant to do from down in here, and
		 * if we race on this no real harm will be done, since the
		 * finished fileid has so many other components.
		 *
		 * We use the bottom 32-bits of the process ID, hoping they
		 * are more random than the top 32-bits (should we be on a
		 * machine with 64-bit process IDs).
		 *
		 * We increment by 100000 on each call as a simple way of
		 * randomizing; simply incrementing seems potentially less
		 * useful if pids are also simply incremented, since this
		 * is process-local and we may be one of a set of processes
		 * starting up.  100000 pushes us out of pid space on most
		 * 32-bit platforms, and has few interesting properties in
		 * base 2.
		 */
		if(DB_GLOBAL(fid_serial) == 0) {
			__os_id(env->dbenv, &pid, NULL);
			DB_GLOBAL(fid_serial) = (uint32)pid;
		}
		else
			DB_GLOBAL(fid_serial) += 100000;
	}
	else {
		tmp = (uint32)fi.dwVolumeSerialNumber;
		for(p = (uint8 *)&tmp, i = sizeof(uint32); i > 0; --i)
			*fidp++ = *p++;
	}
	return 0;
}
/*
 * __os_fdlock --
 *	Acquire/release a lock on a byte in a file.
 */
int __os_fdlock(ENV * env, DB_FH * fhp, off_t offset, int acquire, int nowait)
{
#ifdef DB_WINCE
	/*
	 * This functionality is not supported by WinCE, so just fail.
	 *
	 * Should only happen if an app attempts to open an environment
	 * with the DB_REGISTER flag.
	 */
	__db_errx(env, DB_STR("0019", "fdlock API not implemented for WinCE, DB_REGISTER environment flag not supported."));
	return EFAULT;
#else
	DWORD low, high;
	OVERLAPPED over;
	int ret;
	DB_ENV * dbenv = (env == NULL) ? NULL : env->dbenv;
	DB_ASSERT(env, F_ISSET(fhp, DB_FH_OPENED) && fhp->handle != INVALID_HANDLE_VALUE);
	if(dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS_ALL))
		__db_msg(env, DB_STR_A("0020", "fileops: flock %s %s offset %lu", "%s %s %lu"),
			fhp->name, acquire ? DB_STR_P("acquire") : DB_STR_P("release"), (ulong)offset);
	/*
	 * Windows file locking interferes with read/write operations, so we
	 * map the ranges to an area past the end of the file.
	 */
	DB_ASSERT(env, offset < (u_int64_t)INT64_MAX);
	offset = UINT64_MAX-offset;
	low = (DWORD)offset;
	high = (DWORD)(offset>>32);
	if(acquire) {
		if(nowait)
			RETRY_CHK_EINTR_ONLY(!LockFile(fhp->handle, low, high, 1, 0), ret);
		else if(__os_is_winnt()) {
			memzero(&over, sizeof(over));
			over.Offset = low;
			over.OffsetHigh = high;
			RETRY_CHK_EINTR_ONLY(!LockFileEx(fhp->handle, LOCKFILE_EXCLUSIVE_LOCK, 0, 1, 0, &over), ret);
		}
		else {
			/* Windows 9x/ME doesn't support a blocking call. */
			for(;; ) {
				RETRY_CHK_EINTR_ONLY(!LockFile(fhp->handle, low, high, 1, 0), ret);
				if(__os_posix_err(ret) != EAGAIN)
					break;
				__os_yield(env, 1, 0);
			}
		}
	}
	else
		RETRY_CHK_EINTR_ONLY(!UnlockFile(fhp->handle, low, high, 1, 0), ret);
	return __os_posix_err(ret);
#endif
}
/*
 * __os_fsync --
 *	Flush a file descriptor.
 */
int __os_fsync(ENV * env, DB_FH * fhp)
{
	int ret;
	DB_ENV * dbenv = (env == NULL) ? NULL : env->dbenv;
	/*
	 * Do nothing if the file descriptor has been marked as not requiring
	 * any sync to disk.
	 */
	if(F_ISSET(fhp, DB_FH_NOSYNC))
		return 0;
	if(dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS_ALL))
		__db_msg(env, DB_STR_A("0023", "fileops: flush %s", "%s"), fhp->name);
	RETRY_CHK((!FlushFileBuffers(fhp->handle)), ret);
	if(ret != 0) {
		__db_syserr(env, ret, DB_STR("0024", "FlushFileBuffers"));
		ret = __os_posix_err(ret);
	}
	return ret;
}
//
// Descr: Retrieve an environment variable.
//
int __os_getenv(ENV * env, const char * name, char ** bpp, size_t buflen)
{
#ifdef DB_WINCE
	COMPQUIET(name, NULL);
	/* WinCE does not have a getenv implementation. */
	return 0;
#else
	_TCHAR * tname, tbuf[1024];
	int ret;
	char * p;
	/*
	 * If there's a value and the buffer is large enough:
	 *	copy value into the pointer, return 0
	 * If there's a value and the buffer is too short:
	 *	set pointer to NULL, return EINVAL
	 * If there's no value:
	 *	set pointer to NULL, return 0
	 */
	if((p = getenv(name)) != NULL) {
		if(strlen(p) < buflen) {
			strcpy(*bpp, p);
			return 0;
		}
		else
			goto small_buf;
	}
	TO_TSTRING(env, name, tname, ret);
	if(ret != 0)
		return ret;
	/*
	 * The declared size of the tbuf buffer limits the maximum environment
	 * variable size in Berkeley DB on Windows.  If that's too small, or if
	 * we need to get rid of large allocations on the BDB stack, we should
	 * malloc the tbuf memory.
	 */
	ret = GetEnvironmentVariable(tname, tbuf, sizeof(tbuf));
	FREE_STRING(env, tname);
	/*
	 * If GetEnvironmentVariable succeeds, the return value is the number
	 * of characters stored in the buffer pointed to by lpBuffer, not
	 * including the terminating null character.  If the buffer is not
	 * large enough to hold the data, the return value is the buffer size,
	 * in characters, required to hold the string and its terminating null
	 * character.  If GetEnvironmentVariable fails, the return value is
	 * zero.  If the specified environment variable was not found in the
	 * environment block, GetLastError returns ERROR_ENVVAR_NOT_FOUND.
	 */
	if(ret == 0) {
		if((ret = __os_get_syserr()) == ERROR_ENVVAR_NOT_FOUND) {
			*bpp = NULL;
			return 0;
		}
		else {
			__db_syserr(env, ret, DB_STR("0026", "GetEnvironmentVariable"));
			return __os_posix_err(ret);
		}
	}
	if(ret > (int)sizeof(tbuf))
		goto small_buf;
	FROM_TSTRING(env, tbuf, p, ret);
	if(ret != 0)
		return ret;
	if(strlen(p) < buflen)
		strcpy(*bpp, p);
	else
		*bpp = NULL;
	FREE_STRING(env, p);
	if(*bpp == NULL)
		goto small_buf;
	return 0;
small_buf:
	*bpp = NULL;
	__db_errx(env, DB_STR_A("0027", "%s: buffer too small to hold environment variable %s", "%s %s"), name, p);
	return EINVAL;
#endif
}
/*
 * __os_openhandle --
 *	Open a file, using POSIX 1003.1 open flags.
 */
int __os_openhandle(ENV * env, const char * name, int flags, int mode, DB_FH ** fhpp)
{
#ifdef DB_WINCE
	/*
	 * __os_openhandle API is not implemented on WinCE.
	 * It is not currently called from within the Berkeley DB library,
	 * so don't log the failure via the __db_err mechanism.
	 */
	return EFAULT;
#else
	DB_FH * fhp;
	int ret, nrepeat, retries;
	/*
	 * Allocate the file handle and copy the file name.  We generally only
	 * use the name for verbose or error messages, but on systems where we
	 * can't unlink temporary files immediately, we use the name to unlink
	 * the temporary file when the file handle is closed.
	 *
	 * Lock the ENV handle and insert the new file handle on the list.
	 */
	if((ret = __os_calloc(env, 1, sizeof(DB_FH), &fhp)) != 0)
		return ret;
	if((ret = __os_strdup(env, name, &fhp->name)) != 0)
		goto err;
	if(env != NULL) {
		MUTEX_LOCK(env, env->mtx_env);
		TAILQ_INSERT_TAIL(&env->fdlist, fhp, q);
		MUTEX_UNLOCK(env, env->mtx_env);
		F_SET(fhp, DB_FH_ENVLINK);
	}
	retries = 0;
	for(nrepeat = 1; nrepeat < 4; ++nrepeat) {
		fhp->fd = _open(name, flags, mode);
		if(fhp->fd != -1) {
			ret = 0;
			break;
		}
		switch(ret = __os_posix_err(__os_get_syserr())) {
		    case EMFILE:
		    case ENFILE:
		    case ENOSPC:
			/*
			 * If it's a "temporary" error, we retry up to 3 times,
			 * waiting up to 12 seconds.  While it's not a problem
			 * if we can't open a database, an inability to open a
			 * log file is cause for serious dismay.
			 */
			__os_yield(env, nrepeat*2, 0);
			break;
		    case EAGAIN:
		    case EBUSY:
		    case EINTR:
			/*
			 * If an EAGAIN, EBUSY or EINTR, retry immediately for
			 * DB_RETRY times.
			 */
			if(++retries < DB_RETRY)
				--nrepeat;
			break;
		    default:
			/* Open is silent on error. */
			goto err;
		}
	}
	if(ret == 0) {
		F_SET(fhp, DB_FH_OPENED);
		*fhpp = fhp;
		return 0;
	}
err:    __os_closehandle(env, fhp);
	return ret;
#endif
}
/*
 * __os_closehandle --
 *	Close a file.
 */
int __os_closehandle(ENV * env, DB_FH * fhp)
{
	DB_ENV * dbenv;
	int t_ret;
	int ret = 0;
	if(env != NULL) {
		dbenv = env->dbenv;
		if(fhp->name != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS|DB_VERB_FILEOPS_ALL))
			__db_msg(env, DB_STR_A("0031", "fileops: %s: close", "%s"), fhp->name);
		if(F_ISSET(fhp, DB_FH_ENVLINK)) {
			/*
			 * Lock the ENV handle and remove this file
			 * handle from the list.
			 */
			MUTEX_LOCK(env, env->mtx_env);
			TAILQ_REMOVE(&env->fdlist, fhp, q);
			MUTEX_UNLOCK(env, env->mtx_env);
		}
	}
	/* Discard any underlying system file reference. */
	if(F_ISSET(fhp, DB_FH_OPENED)) {
		if(fhp->handle != INVALID_HANDLE_VALUE)
			RETRY_CHK((!CloseHandle(fhp->handle)), ret);
		else
#ifdef DB_WINCE
			ret = EFAULT;
#else
			RETRY_CHK((_close(fhp->fd)), ret);
#endif
		if(fhp->trunc_handle != INVALID_HANDLE_VALUE) {
			RETRY_CHK((!CloseHandle(fhp->trunc_handle)), t_ret);
			if(t_ret != 0 && ret == 0)
				ret = t_ret;
		}
		if(ret != 0) {
			__db_syserr(env, ret, DB_STR("0032", "CloseHandle"));
			ret = __os_posix_err(ret);
		}
	}
	/* Unlink the file if we haven't already done so. */
	if(F_ISSET(fhp, DB_FH_UNLINK))
		__os_unlink(env, fhp->name, 0);
	__os_free(env, fhp->name);
	__os_free(env, fhp);
	return ret;
}
//
// Map
//
static int __os_map(ENV*, char *, REGINFO*, DB_FH*, size_t, int, int, int, void **);
static int __os_unique_name(_TCHAR*, HANDLE, _TCHAR*, size_t);
//
// Descr: Create/join a shared memory region.
//
int __os_attach(ENV * env, REGINFO * infop, REGION * rp)
{
	int ret;
	int is_sparse;
#ifndef DB_WINCE
	DWORD dw;
#endif
	infop->fhp = NULL;
	/*
	 * On Windows/9X, files that are opened by multiple processes do not
	 * share data correctly.  For this reason, we require that DB_PRIVATE
	 * be specified on that platform.
	 */
	if(!F_ISSET(env, ENV_PRIVATE) && __os_is_winnt() == 0) {
		__db_err(env, EINVAL, DB_STR("0006", "Windows 9X systems must specify DB_PRIVATE"));
		return EINVAL;
	}
	/*
	 * Try to open/create the file.  We DO NOT need to ensure that multiple
	 * threads/processes attempting to simultaneously create the region are
	 * properly ordered, our caller has already taken care of that.
	 */
	if((ret = __os_open(env, infop->name, 0, DB_OSO_REGION|(F_ISSET(infop, REGION_CREATE_OK) ? DB_OSO_CREATE : 0), env->db_mode, &infop->fhp)) != 0) {
		__db_err(env, ret, "%s", infop->name);
		return ret;
	}
	is_sparse = 0;
#ifndef DB_WINCE
	/*
	 * Sparse file only works for NTFS filesystem. If we failed to set it,
	 * just ignore the error and use the normal method.
	 */
	if(!F_ISSET(env, ENV_SYSTEM_MEM) && (DeviceIoControl(infop->fhp->handle, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &dw, NULL)))
		is_sparse = 1;
#endif

	/*
	 * Map the file in.  If we're creating an in-system-memory region,
	 * specify a segment ID (which is never used again) so that the
	 * calling code writes out the REGENV_REF structure to the primary
	 * environment file.
	 */
	ret = __os_map(env, infop->name, infop, infop->fhp, rp->max, 1, F_ISSET(env, ENV_SYSTEM_MEM), 0, &infop->addr);
	if(ret == 0 && F_ISSET(env, ENV_SYSTEM_MEM))
		rp->segid = 1;
	if(ret != 0) {
		__os_closehandle(env, infop->fhp);
		infop->fhp = NULL;
		return ret;
	}
	/*
	 * If we are using sparse file, we don't need to keep the file handle
	 * for writing or extending.
	 */
	if(is_sparse && infop->fhp != NULL) {
		ret = __os_closehandle(env, infop->fhp);
		infop->fhp = NULL;
	}
	return ret;
}
//
// Descr: Detach from a shared memory region.
//
int __os_detach(ENV * env, REGINFO * infop, int destroy)
{
	int ret, t_ret;
	DB_ENV * dbenv = env->dbenv;
	if(infop->wnt_handle != NULL) {
		CloseHandle(infop->wnt_handle);
		infop->wnt_handle = NULL;
	}
	if(infop->fhp != NULL) {
		ret = __os_closehandle(env, infop->fhp);
		infop->fhp = NULL;
		if(ret != 0)
			return ret;
	}
	ret = !UnmapViewOfFile(infop->addr) ? __os_get_syserr() : 0;
	if(ret != 0) {
		__db_syserr(env, ret, DB_STR("0007", "UnmapViewOfFile"));
		ret = __os_posix_err(ret);
	}
	if(!F_ISSET(env, ENV_SYSTEM_MEM) && destroy && (t_ret = __os_unlink(env, infop->name, 1)) != 0 && ret == 0)
		ret = t_ret;
	return ret;
}
/*
 * __os_mapfile --
 *	Map in a shared memory file.
 */
int __os_mapfile(ENV*env, char * path, DB_FH * fhp, size_t len, int is_rdonly, void ** addr)
{
#ifdef DB_WINCE
	/*
	 * Windows CE has special requirements for file mapping to work.
	 * * The input handle needs to be opened using CreateFileForMapping
	 * * Concurrent access via a non mapped file is not supported.
	 * So we disable support for memory mapping files on Windows CE. It is
	 * currently only used as an optimization in mpool for small read only
	 * databases.
	 */
	return EFAULT;
#else
	DB_ENV * dbenv = env ? env->dbenv : 0;
	if(dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS|DB_VERB_FILEOPS_ALL))
		__db_msg(env, DB_STR_A("0008", "fileops: mmap %s", "%s"), path);
	return __os_map(env, path, NULL, fhp, len, 0, 0, is_rdonly, addr);
#endif
}
/*
 * __os_unmapfile --
 *	Unmap the shared memory file.
 */
int __os_unmapfile(ENV*env, void * addr, size_t len)
{
	DB_ENV * dbenv = env ? env->dbenv : 0;
	if(dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS|DB_VERB_FILEOPS_ALL))
		__db_msg(env, DB_STR("0009", "fileops: munmap"));
	return !UnmapViewOfFile(addr) ? __os_posix_err(__os_get_syserr()) : 0;
}
/*
 * __os_unique_name --
 *	Create a unique identifying name from a pathname (may be absolute or
 *	relative) and/or a file descriptor.
 *
 *	The name returned must be unique (different files map to different
 *	names), and repeatable (same files, map to same names).  It's not
 *	so easy to do by name.  Should handle not only:
 *
 *		foo.bar == ./foo.bar == c:/whatever_path/foo.bar
 *
 *	but also understand that:
 *
 *		foo.bar == Foo.Bar	(FAT file system)
 *		foo.bar != Foo.Bar	(NTFS)
 *
 *	The best solution is to use the file index, found in the file
 *	information structure (similar to UNIX inode #).
 *
 *	When a file is deleted, its file index may be reused,
 *	but if the unique name has not gone from its namespace,
 *	we may get a conflict.  So to ensure some tie in to the
 *	original pathname, we also use the creation time and the
 *	file basename.  This is not a perfect system, but it
 *	should work for all but anamolous test cases.
 *
 */
static int __os_unique_name(_TCHAR * orig_path, HANDLE hfile, _TCHAR * result_path, size_t result_path_len)
{
	BY_HANDLE_FILE_INFORMATION fileinfo;
	/*
	 * In Windows, pathname components are delimited by '/' or '\', and
	 * if neither is present, we need to strip off leading drive letter
	 * (e.g. c:foo.txt).
	 */
	_TCHAR * basename = _tcsrchr(orig_path, '/');
	_TCHAR * p = _tcsrchr(orig_path, '\\');
	if(!basename || (p && p > basename))
		basename = p;
	if(!basename)
		basename = _tcsrchr(orig_path, ':');
	if(!basename)
		basename = orig_path;
	else
		basename++;
	if(!GetFileInformationByHandle(hfile, &fileinfo))
		return __os_posix_err(__os_get_syserr());
	_sntprintf(result_path, result_path_len,
		_T("__db_shmem.%8.8lx.%8.8lx.%8.8lx.%8.8lx.%8.8lx.%s"),
		fileinfo.dwVolumeSerialNumber,
		fileinfo.nFileIndexHigh,
		fileinfo.nFileIndexLow,
		fileinfo.ftCreationTime.dwHighDateTime,
		fileinfo.ftCreationTime.dwHighDateTime,
		basename);
	return 0;
}
//
// The mmap(2) function for Windows.
//
static int __os_map(ENV * env, char * path, REGINFO * infop, DB_FH * fhp, size_t len, int is_region, int is_system, int is_rdonly, void ** addr)
{
	int    ret = 0;
	HANDLE hMemory;
	int    use_pagefile;
	_TCHAR * tpath, shmem_name[DB_MAXPATHLEN];
	void * pMemory;
	unsigned __int64 len64;
	if(infop)
		infop->wnt_handle = NULL;
	/*
	 * On 64 bit systems, len is already a 64 bit value.
	 * On 32 bit systems len is a 32 bit value.
	 * Always convert to a 64 bit value, so that the high order
	 * DWORD can be simply extracted on 64 bit platforms.
	 */
	len64 = len;
	use_pagefile = is_region && is_system;
	/*
	 * If creating a region in system space, get a matching name in the
	 * paging file namespace.
	 */
	if(use_pagefile) {
#ifdef DB_WINCE
		__db_errx(env, DB_STR("0010", "Unable to memory map regions using system memory on WinCE."));
		return EFAULT;
#endif
		TO_TSTRING(env, path, tpath, ret);
		if(ret != 0)
			return ret;
		ret = __os_unique_name(tpath, fhp->handle, shmem_name, sizeof(shmem_name));
		FREE_STRING(env, tpath);
		if(ret != 0)
			return ret;
	}
	/*
	 * XXX
	 * DB: We have not implemented copy-on-write here.
	 *
	 * If this is an region in system memory, we try to open it using the
	 * OpenFileMapping() first, and only call CreateFileMapping() if we're
	 * really creating the section.  There are two reasons:
	 *
	 * 1) We only create the mapping if we have newly created the region.
	 *    This avoids a long-running problem caused by Windows reference
	 *    counting, where regions that are closed by all processes are
	 *    deleted.  It turns out that just checking for a zeroed region
	 *    is not good enough. See [#4882] and [#7127] for the details.
	 *
	 * 2) CreateFileMapping seems to mess up making the commit charge to
	 *    the process. It thinks, incorrectly, that when we want to join a
	 *    previously existing section, that it should make a commit charge
	 *    for the whole section.  In fact, there is no new committed memory
	 *    whatever.  The call can fail if there is insufficient memory free
	 *    to handle the erroneous commit charge.  So, we find that the
	 *    bogus commit is not made if we call OpenFileMapping.
	 */
	hMemory = NULL;
	if(use_pagefile) {
#ifndef DB_WINCE
		hMemory = OpenFileMapping(is_rdonly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS, 0, shmem_name);
		if(hMemory == NULL && F_ISSET(infop, REGION_CREATE_OK))
			hMemory = CreateFileMapping((HANDLE)-1, 0, is_rdonly ? PAGE_READONLY : PAGE_READWRITE, (DWORD)(len64>>32), (DWORD)len64, shmem_name);
#endif
	}
	else {
		hMemory = CreateFileMapping(fhp->handle, 0, is_rdonly ? PAGE_READONLY : PAGE_READWRITE, (DWORD)(len64>>32), (DWORD)len64, NULL);
#ifdef DB_WINCE
		/*
		 * WinCE automatically closes the handle passed in.
		 * Ensure DB does not attempt to close the handle again.
		 */
		fhp->handle = INVALID_HANDLE_VALUE;
		F_CLR(fhp, DB_FH_OPENED);
#endif
	}
	if(hMemory == NULL) {
		ret = __os_get_syserr();
		__db_syserr(env, ret, DB_STR("0011", "OpenFileMapping"));
		return __env_panic(env, __os_posix_err(ret));
	}
	pMemory = MapViewOfFile(hMemory, (is_rdonly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS), 0, 0, len);
	if(pMemory == NULL) {
		ret = __os_get_syserr();
		__db_syserr(env, ret, DB_STR("0012", "MapViewOfFile"));
		return __env_panic(env, __os_posix_err(ret));
	}
	/*
	 * XXX
	 * It turns out that the kernel object underlying the named section
	 * is reference counted, but that the call to MapViewOfFile() above
	 * does NOT increment the reference count! So, if we close the handle
	 * here, the kernel deletes the object from the kernel namespace.
	 * When a second process comes along to join the region, the kernel
	 * happily creates a new object with the same name, but completely
	 * different identity. The two processes then have distinct isolated
	 * mapped sections, not at all what was wanted. Not closing the handle
	 * here fixes this problem.  We carry the handle around in the region
	 * structure so we can close it when unmap is called.
	 */
	if(use_pagefile && infop != NULL)
		infop->wnt_handle = hMemory;
	else
		CloseHandle(hMemory);
	*addr = pMemory;
	return ret;
}
//
// Descr: Return the current process ID.
// PUBLIC: void __os_id(DB_ENV *, pid_t *, db_threadid_t*);
//
void __os_id(DB_ENV * dbenv, pid_t * pidp, db_threadid_t * tidp)
{
	/*
	 * We can't depend on dbenv not being NULL, this routine is called
	 * from places where there's no DB_ENV handle.
	 *
	 * We cache the pid in the ENV handle, getting the process ID is a
	 * fairly slow call on lots of systems.
	 */
	if(pidp != NULL) {
		if(dbenv == NULL) {
#if defined(HAVE_VXWORKS)
			*pidp = taskIdSelf();
#else
			*pidp = getpid();
#endif
		}
		else
			*pidp = dbenv->env->pid_cache;
	}
	if(tidp != NULL) {
#if defined(DB_WIN32)
		*tidp = GetCurrentThreadId();
#elif defined(HAVE_MUTEX_UI_THREADS)
		*tidp = thr_self();
#elif defined(HAVE_PTHREAD_SELF)
		*tidp = pthread_self();
#else
		// Default to just getpid.
		*tidp = 0;
#endif
	}
}
//
// Descr: Do an I/O.
//
int __os_io(ENV * env, int op, DB_FH * fhp, db_pgno_t pgno, uint32 pgsize, uint32 relative, uint32 io_len, uint8 * buf, size_t * niop)
{
	int ret;
#ifndef DB_WINCE
	if(__os_is_winnt()) {
		DB_ENV * dbenv;
		DWORD nbytes;
		OVERLAPPED over;
		ULONG64 off;
		dbenv = env == NULL ? NULL : env->dbenv;
		if((off = relative) == 0)
			off = (ULONG64)pgsize*pgno;
		over.Offset = (DWORD)(off&0xffffffff);
		over.OffsetHigh = (DWORD)(off>>32);
		over.hEvent = 0; /* we don't want asynchronous notifications */
		if(dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS_ALL))
			__db_msg(env, DB_STR_A("0014", "fileops: %s %s: %lu bytes at offset %lu", "%s %s %lu %lu"), op == DB_IO_READ ?
				DB_STR_P("read") : DB_STR_P("write"), fhp->name, (ulong)io_len, (ulong)off);
		LAST_PANIC_CHECK_BEFORE_IO(env);
		switch(op) {
		    case DB_IO_READ:
 #if defined(HAVE_STATISTICS)
			++fhp->read_count;
 #endif
			if(!ReadFile(fhp->handle, buf, (DWORD)io_len, &nbytes, &over))
				goto slow;
			break;
		    case DB_IO_WRITE:
 #ifdef HAVE_FILESYSTEM_NOTZERO
			if(__os_fs_notzero())
				goto slow;
 #endif
 #if defined(HAVE_STATISTICS)
			++fhp->write_count;
 #endif
			if(!WriteFile(fhp->handle, buf, (DWORD)io_len, &nbytes, &over))
				goto slow;
			break;
		}
		if(nbytes == io_len) {
			*niop = (size_t)nbytes;
			return 0;
		}
	}
slow:
#endif
	MUTEX_LOCK(env, fhp->mtx_fh);
	if((ret = __os_seek(env, fhp, pgno, pgsize, relative)) != 0)
		goto err;
	switch(op) {
	    case DB_IO_READ:
		ret = __os_read(env, fhp, buf, io_len, niop);
		break;
	    case DB_IO_WRITE:
		ret = __os_write(env, fhp, buf, io_len, niop);
		break;
	}
err:
	MUTEX_UNLOCK(env, fhp->mtx_fh);
	return ret;
}
//
// Descr: Read from a file handle.
//
int __os_read(ENV * env, DB_FH * fhp, void * addr, size_t len, size_t * nrp)
{
	DB_ENV * dbenv = env ? env->dbenv : 0;
	DWORD count;
	size_t offset, nr;
	uint8 * taddr;
	int    ret = 0;
#if defined(HAVE_STATISTICS)
	++fhp->read_count;
#endif
	if(dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS_ALL))
		__db_msg(env, DB_STR_A("0015", "fileops: read %s: %lu bytes", "%s %lu"), fhp->name, (ulong)len);
	for(taddr = (uint8 *)addr, offset = 0; offset < len; taddr += nr, offset += nr) {
		LAST_PANIC_CHECK_BEFORE_IO(env);
		RETRY_CHK((!ReadFile(fhp->handle, taddr, (DWORD)(len-offset), &count, NULL)), ret);
		if(count == 0 || ret != 0)
			break;
		nr = (size_t)count;
	}
	*nrp = taddr-(uint8 *)addr;
	if(ret != 0) {
		__db_syserr(env, ret, DB_STR_A("0016", "read: 0x%lx, %lu", "%lx %lu"), P_TO_ULONG(taddr), (ulong)len-offset);
		ret = __os_posix_err(ret);
	}
	return ret;
}
//
// Descr: Write to a file handle.
//
int __os_write(ENV * env, DB_FH * fhp, void * addr, size_t len, size_t * nwp)
{
	int ret;
#ifdef HAVE_FILESYSTEM_NOTZERO
	/* Zero-fill as necessary. */
	if(__os_fs_notzero() && (ret = __db_zero_fill(env, fhp)) != 0)
		return ret;
#endif
	return __os_physwrite(env, fhp, addr, len, nwp);
}
//
// Descr: Physical write to a file handle.
//
int __os_physwrite(ENV * env, DB_FH * fhp, void * addr, size_t len, size_t * nwp)
{
	DB_ENV * dbenv = env ? env->dbenv : 0;
	DWORD  count;
	size_t offset, nw;
	uint8 * taddr;
	int    ret = 0;
#if defined(HAVE_STATISTICS)
	++fhp->write_count;
#endif
	if(dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS_ALL))
		__db_msg(env, DB_STR_A("0017", "fileops: write %s: %lu bytes", "%s %lu"), fhp->name, (ulong)len);
	for(taddr = (uint8 *)addr, offset = 0; offset < len; taddr += nw, offset += nw) {
		LAST_PANIC_CHECK_BEFORE_IO(env);
		RETRY_CHK((!WriteFile(fhp->handle, taddr, (DWORD)(len-offset), &count, NULL)), ret);
		if(ret != 0)
			break;
		nw = (size_t)count;
	}
	*nwp = len;
	if(ret != 0) {
		__db_syserr(env, ret, DB_STR_A("0018", "write: %#lx, %lu", "%#lx %lu"), P_TO_ULONG(taddr), (ulong)len-offset);
		ret = __os_posix_err(ret);
		DB_EVENT(env, DB_EVENT_WRITE_FAILED, NULL);
	}
	return ret;
}
//
// Descr: Seek to a page/byte offset in the file.
//
int __os_seek(ENV * env, DB_FH * fhp, db_pgno_t pgno, uint32 pgsize, off_t relative)
{
	/* Yes, this really is how Microsoft designed their API. */
	union {
		__int64 bigint;
		struct {
			ulong low;
			long high;
		};
	} offbytes;
	DB_ENV * dbenv = env ? env->dbenv : 0;
	off_t offset;
	int ret;
#if defined(HAVE_STATISTICS)
	++fhp->seek_count;
#endif
	offset = (off_t)pgsize*pgno+relative;
	if(dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS_ALL))
		__db_msg(env, DB_STR_A("0038", "fileops: seek %s to %lu", "%s %lu"), fhp->name, (ulong)offset);
	offbytes.bigint = offset;
	ret = (SetFilePointer(fhp->handle, offbytes.low, &offbytes.high, FILE_BEGIN) == (DWORD)-1) ? __os_get_syserr() : 0;
	if(ret == 0) {
		fhp->pgsize = pgsize;
		fhp->pgno = pgno;
		fhp->offset = relative;
	}
	else {
		__db_syserr(env, ret, DB_STR_A("0039", "seek: %lu: (%lu * %lu) + %lu", "%lu %lu %lu %lu"),
			(ulong)offset, (ulong)pgno, (ulong)pgsize, (ulong)relative);
		ret = __os_posix_err(ret);
	}
	return ret;
}
//
// Raw data reads must be done in multiples of the disk sector size. Currently
// the sector size is either 512 bytes or 4096 bytes. So we set the
// MAX_SECTOR_SIZE to 4096.
//
#define MAX_SECTOR_SIZE 4096
//
// Find the cluster size of the file system that would contain the given path.
// If the value can't be determined, an error is returned.
//
int __os_get_cluster_size(const char * path, uint32 * psize)
{

#if(WINVER < 0x500) || defined(DB_WINCE)
	/*
	 * WinCE and versions of Windows earlier than Windows NT don't have
	 * the APIs required to retrieve the cluster size.
	 */
	*psize = DB_DEF_IOSIZE;
	return 0;
#else
	BYTE clustershift, sectorshift, * pcluster;
	char buffer[MAX_SECTOR_SIZE];
	DWORD flags, infolen, length, mcl, name_size;
	HANDLE vhandle;
	int ret;
	NTFS_VOLUME_DATA_BUFFER ntfsinfo;
	size_t name_len;
	TCHAR * env_path, name_buffer[MAX_PATH+1], root_path[MAX_PATH+1];
	WORD * psector;
	if(path == NULL || psize == NULL) {
		return EINVAL;
	}
	name_size = MAX_PATH+1;
	*psize = 0;

	TO_TSTRING(NULL, path, env_path, ret);
	if(ret != 0)
		return ret;
	/* Retrieve the volume root path where the input path resides. */
	if(!GetVolumePathName(env_path, root_path, name_size)) {
		FREE_STRING(NULL, env_path);
		return __os_posix_err(__os_get_syserr());
	}
	FREE_STRING(NULL, env_path);
	/* Get the volume GUID name from the root path. */
	if(!GetVolumeNameForVolumeMountPoint(
		   root_path, name_buffer, name_size))
		return __os_posix_err(__os_get_syserr());
	/* Delete the last trail "\" in the GUID name. */
	name_len = _tcsclen(name_buffer);
	if(name_len > 0)
		name_buffer[name_len-1] = _T('\0');
	/* Create a handle to the volume. */
	vhandle = CreateFile(name_buffer, FILE_READ_ATTRIBUTES|FILE_READ_DATA,
		FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	/* If open failed, return error */
	if(vhandle == INVALID_HANDLE_VALUE)
		return __os_posix_err(__os_get_syserr());
	/* Get the volume information through the root path. */
	if(!GetVolumeInformation(root_path, NULL, name_size, NULL, &mcl,
		   &flags, name_buffer, name_size)) {
		ret = __os_posix_err(__os_get_syserr());
		CloseHandle(vhandle);
		return ret;
	}
	ret = 0;
	if(_tcscmp(name_buffer, _T("NTFS")) == 0) {
		/*
		 * If this is NTFS file system, use FSCTL_GET_NTFS_VOLUME_DATA
		 * to get the cluster size.
		 */
		if(DeviceIoControl(
			   vhandle,             /* volume handle */
			   FSCTL_GET_NTFS_VOLUME_DATA, /* Control Code */
			   NULL,                /* not use */
			   0,                   /* not use */
			   &ntfsinfo,           /* output buffer */
			   sizeof(NTFS_VOLUME_DATA_BUFFER), /* output buffer length */
			   &infolen,            /* number of returned bytes */
			   NULL))               /* ignore here */
			*psize = ntfsinfo.BytesPerCluster;
		else
			ret = __os_posix_err(__os_get_syserr());
	}
	else if(_tcscmp(name_buffer, _T("exFAT")) == 0) {
		/*
		 * If this is exFAT file system, read the information of sector
		 * and cluster from the BPB on sector 0
		 * +6C H: BYTE SectorSizeShift
		 * +6D H: BYTE ClusterShift
		 */
		if(ReadFile(vhandle, buffer, MAX_SECTOR_SIZE, &length, NULL)) {
			sectorshift = *(BYTE *)(&buffer[0x6C]);
			clustershift = *(BYTE *)(&buffer[0x6D]);
			*psize = 1<<sectorshift;
			*psize = (*psize)<<clustershift;
		}
		else
			ret = __os_posix_err(__os_get_syserr());
	}
	else if(_tcscmp(name_buffer, _T("FAT")) == 0 || _tcscmp(name_buffer, _T("FAT32")) == 0) {
		/*
		 * If this is FAT or FAT32 file system, read the information of
		 * sector and cluster from the BPB on sector 0.
		 * +0B H: WORD Bytes per Sector.
		 * +0D H: BYTE Sectors Per Cluster.
		 */
		if(ReadFile(vhandle, buffer, MAX_SECTOR_SIZE, &length, NULL)) {
			psector = (WORD *)(&buffer[0x0B]);
			pcluster = (BYTE *)(&buffer[0x0D]);
			*psize = (*psector)*(*pcluster);
		}
		else
			ret = __os_posix_err(__os_get_syserr());
	}
	CloseHandle(vhandle);
	return ret;
#endif
}
/*
 * __os_exists --
 *	Return if the file exists.
 */
int __os_exists(ENV * env, const char * path, int * isdirp)
{
	DB_ENV * dbenv;
	DWORD attrs;
	_TCHAR * tpath;
	int ret;
	dbenv = env == NULL ? NULL : env->dbenv;
	TO_TSTRING(env, path, tpath, ret);
	if(ret != 0)
		return ret;
	if(dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS|DB_VERB_FILEOPS_ALL))
		__db_msg(env, DB_STR_A("0033", "fileops: stat %s", "%s"), path);
	RETRY_CHK(((attrs = GetFileAttributes(tpath)) == (DWORD)-1 ? 1 : 0), ret);
	if(ret == 0) {
		if(isdirp != NULL)
			*isdirp = (attrs&FILE_ATTRIBUTE_DIRECTORY);
	}
	else
		ret = __os_posix_err(ret);
	FREE_STRING(env, tpath);
	return ret;
}

/*
 * __os_ioinfo --
 *	Return file size and I/O size; abstracted to make it easier
 *	to replace.
 */
int __os_ioinfo(ENV * env, const char * path, DB_FH * fhp, uint32 * mbytesp, uint32 * bytesp, uint32 * iosizep)
{
	int ret;
	BY_HANDLE_FILE_INFORMATION bhfi;
	unsigned __int64 filesize;
	uint32 io_sz;
	RETRY_CHK((!GetFileInformationByHandle(fhp->handle, &bhfi)), ret);
	if(ret != 0) {
		__db_syserr(env, ret, DB_STR("0034", "GetFileInformationByHandle"));
		return __os_posix_err(ret);
	}
	filesize = ((unsigned __int64)bhfi.nFileSizeHigh<<32)+bhfi.nFileSizeLow;
	/* Return the size of the file. */
	if(mbytesp != NULL)
		*mbytesp = (uint32)(filesize/MEGABYTE);
	if(bytesp != NULL)
		*bytesp = (uint32)(filesize%MEGABYTE);
	if(iosizep != NULL) {
		/*
		 * Attempt to retrieve a file system cluster size, if the
		 * call succeeds, and the value returned is reasonable,
		 * use it as the default page size. Otherwise use a
		 * reasonable default value.
		 */
		if(__os_get_cluster_size(path, &io_sz) != 0 || io_sz < 1025)
			*iosizep = DB_DEF_IOSIZE;
		else
			*iosizep = io_sz;
	}
	return 0;
}
//
// Descr: Truncate the file.
//
int __os_truncate(ENV * env, DB_FH * fhp, db_pgno_t pgno, uint32 pgsize)
{
	/* Yes, this really is how Microsoft have designed their API */
	union {
		__int64 bigint;
		struct {
			ulong low;
			long high;
		};
	} off;
	DB_ENV * dbenv = env ? env->dbenv : 0;
	off_t offset = (off_t)pgsize*pgno;
	int ret = 0;
	if(dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS|DB_VERB_FILEOPS_ALL))
		__db_msg(env, DB_STR_A("0021", "fileops: truncate %s to %lu", "%s %lu"), fhp->name, (ulong)offset);
#ifdef HAVE_FILESYSTEM_NOTZERO
	/*
	 * If the filesystem doesn't zero fill, it isn't safe to extend the
	 * file, or we end up with junk blocks.  Just return in that case.
	 */
	if(__os_fs_notzero()) {
		off_t stat_offset;
		uint32 mbytes, bytes;
		/* Stat the file. */
		if((ret = __os_ioinfo(env, NULL, fhp, &mbytes, &bytes, NULL)) != 0)
			return ret;
		stat_offset = (off_t)mbytes*MEGABYTE+bytes;
		if(offset > stat_offset)
			return 0;
	}
#endif
	LAST_PANIC_CHECK_BEFORE_IO(env);
	/*
	 * Windows doesn't provide truncate directly.  Instead, it has
	 * SetEndOfFile, which truncates to the current position.  To
	 * deal with that, we open a duplicate file handle for truncating.
	 *
	 * We want to retry the truncate call, which involves a SetFilePointer
	 * and a SetEndOfFile, but there are several complications:
	 *
	 * 1) since the Windows API deals in 32-bit values, it's possible that
	 *    the return from SetFilePointer (the low 32-bits) is
	 *    INVALID_SET_FILE_POINTER even when the call has succeeded.  So we
	 *    have to also check whether GetLastError() returns NO_ERROR.
	 *
	 * 2) when it returns, SetFilePointer overwrites the high bits of the
	 *    offset, so if we need to retry, we have to reset the offset each
	 *    time.
	 *
	 * We can't switch to SetFilePointerEx, which knows about 64-bit
	 * offsets, because it isn't supported on Win9x/ME.
	 */
	RETRY_CHK((off.bigint = (__int64)pgsize*pgno,
		(SetFilePointer(fhp->trunc_handle, off.low, &off.high, FILE_BEGIN) == INVALID_SET_FILE_POINTER &&
		GetLastError() != NO_ERROR) || !SetEndOfFile(fhp->trunc_handle)), ret);
	if(ret != 0) {
		__db_syserr(env, ret, DB_STR_A("0022", "SetFilePointer: %lu", "%lu"), pgno*pgsize);
		ret = __os_posix_err(ret);
	}
	return ret;
}
//
// Descr: Remove a file
//
int __os_unlink(ENV * env, const char * path, int overwrite_test)
{
	DB_ENV * dbenv = env ? env->dbenv : 0;
	HANDLE h;
	_TCHAR * tpath, * orig_tpath, buf[DB_MAXPATHLEN];
	uint32 id;
	int ret, t_ret;
	if(dbenv && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS|DB_VERB_FILEOPS_ALL))
		__db_msg(env, DB_STR_A("0028", "fileops: unlink %s", "%s"), path);
	/* Optionally overwrite the contents of the file to enhance security. */
	if(dbenv && overwrite_test && F_ISSET(dbenv, DB_ENV_OVERWRITE))
		__db_file_multi_write(env, path);
	TO_TSTRING(env, path, tpath, ret);
	if(ret != 0)
		return ret;
	orig_tpath = tpath;
	LAST_PANIC_CHECK_BEFORE_IO(env);
	/*
	 * Windows NT and its descendants allow removal of open files, but the
	 * DeleteFile Win32 system call isn't equivalent to a POSIX unlink.
	 * Firstly, it only succeeds if FILE_SHARE_DELETE is set when the file
	 * is opened.  Secondly, it leaves the file in a "zombie" state, where
	 * it can't be opened again, but a new file with the same name can't be
	 * created either.
	 *
	 * Since we depend on being able to recreate files (during recovery,
	 * say), we have to first rename the file, and then delete it.  It
	 * still hangs around, but with a name we don't care about.  The rename
	 * will fail if the file doesn't exist, which isn't a problem, but if
	 * it fails for some other reason, we need to know about it or a
	 * subsequent open may fail for no apparent reason.
	 */
	if(__os_is_winnt()) {
		__os_unique_id(env, &id);
		_sntprintf(buf, DB_MAXPATHLEN, _T("%s.del.%010u"), tpath, id);
		if(MoveFile(tpath, buf))
			tpath = buf;
		else {
			ret = __os_get_syserr();
			if(__os_posix_err(ret) != ENOENT)
				/*
				 * System doesn't always return ENOENT when
				 * file is missing. So we need a double check
				 * here. Set the return value to ENOENT when
				 * file doesn't exist.
				 */
				if(__os_exists(env, path, NULL) == 0)
					__db_err(env, ret, DB_STR_A("0029", "MoveFile: rename %s to temporary file", "%s"), path);
				else
					ret = ENOENT;
		}
		/*
		 * Try removing the file using the delete-on-close flag.  This
		 * plays nicer with files that are still open than DeleteFile.
		 */
		h = CreateFile(tpath, 0, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, 0);
		if(h != INVALID_HANDLE_VALUE) {
			CloseHandle(h);
			if(GetFileAttributes(tpath) == INVALID_FILE_ATTRIBUTES)
				goto skipdel;
		}
	}
	RETRY_CHK((!DeleteFile(tpath)), ret);
skipdel:
	FREE_STRING(env, orig_tpath);
	/*
	 * XXX
	 * We shouldn't be testing for an errno of ENOENT here, but ENOENT
	 * signals that a file is missing, and we attempt to unlink things
	 * (such as v. 2.x environment regions, in ENV->remove) that we
	 * are expecting not to be there.  Reporting errors in these cases
	 * is annoying.
	 */
	if((ret != 0) && (t_ret = __os_posix_err(ret)) != ENOENT) {
		/* Double check if the file exists. */
		if(__os_exists(env, path, NULL) == 0) {
			__db_syserr(env, ret, DB_STR_A("0030", "DeleteFile: %s", "%s"), path);
			ret = t_ret;
		}
		else
			ret = ENOENT;
	}
	return ret;
}
//
// Descr: Yield the processor, optionally pausing until running again.
//
void __os_yield(ENV * env, ulong secs, ulong usecs /* Seconds and microseconds. */)
{
	COMPQUIET(env, NULL);
	/* Don't require the values be normalized. */
	for(; usecs >= US_PER_SEC; usecs -= US_PER_SEC)
		++secs;
	/*
	 * Yield the processor so other processes or threads can run.
	 *
	 * Sheer raving paranoia -- don't sleep for 0 time, in case some
	 * implementation doesn't yield the processor in that case.
	 */
	Sleep(secs*MS_PER_SEC+(usecs/US_PER_MS)+1);
}
