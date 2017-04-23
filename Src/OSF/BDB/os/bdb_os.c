// BDB_OS.C
//
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop

#if 0 // excl file list {
	os\os_alloc.c
	os\os_ctime.c
	os\os_addrinfo.c
	os\os_root.c
	os\os_rpath.c
	os\os_tmpdir.c
	os\os_uid.c
#endif // } 0 excl file list

#ifdef DIAGNOSTIC
static void __os_guard(ENV *);

typedef union {
	size_t size;
	uintmax_t align;
} db_allocinfo_t;
#endif

/*
 * !!!
 * Correct for systems that return NULL when you allocate 0 bytes of memory.
 * There are several places in DB where we allocate the number of bytes held
 * by the key/data item, and it can be 0.  Correct here so that malloc never
 * returns a NULL for that reason (which behavior is permitted by ANSI).  We
 * could make these calls macros on non-Alpha architectures (that's where we
 * saw the problem), but it's probably not worth the autoconf complexity.
 *
 * !!!
 * Correct for systems that don't set errno when malloc and friends fail.
 *
 *	Out of memory.
 *	We wish to hold the whole sky,
 *	But we never will.
 */

/*
 * __os_umalloc --
 *	Allocate memory to be used by the application.
 *
 *	Use, in order of preference, the allocation function specified to the
 *	ENV handle, the allocation function specified as a replacement for
 *	the library malloc, or the library malloc().
 *
 * PUBLIC: int __os_umalloc(ENV *, size_t, void *);
 */
int __os_umalloc(ENV * env, size_t size, void * storep)
{
	int ret;
	DB_ENV * dbenv = env ? env->dbenv : 0;
	/* Never allocate 0 bytes -- some C libraries don't like it. */
	if(size == 0)
		++size;
	if(dbenv == NULL || dbenv->db_malloc == NULL) {
		*(void **)storep = (DB_GLOBAL(j_malloc) != NULL) ? DB_GLOBAL(j_malloc) (size) : malloc(size);
		if(*(void **)storep == NULL) {
			/*
			 *  Correct error return, see __os_malloc.
			 */
			if((ret = __os_get_errno_ret_zero()) == 0) {
				ret = ENOMEM;
				__os_set_errno(ENOMEM);
			}
			__db_err(env, ret, DB_STR_A("0143", "malloc: %lu", "%lu"), (ulong)size);
			return ret;
		}
		return 0;
	}
	if((*(void **)storep = dbenv->db_malloc(size)) == NULL) {
		__db_errx(env, DB_STR("0144", "user-specified malloc function returned NULL"));
		return ENOMEM;
	}
	return 0;
}
/*
 * __os_urealloc --
 *	Allocate memory to be used by the application.
 *
 *	A realloc(3) counterpart to __os_umalloc's malloc(3).
 *
 * PUBLIC: int __os_urealloc(ENV *, size_t, void *);
 */
int __os_urealloc(ENV * env, size_t size, void * storep)
{
	int ret;
	DB_ENV * dbenv = env ? env->dbenv : 0;
	void * ptr = *(void **)storep;
	/* Never allocate 0 bytes -- some C libraries don't like it. */
	if(size == 0)
		++size;
	if(dbenv == NULL || dbenv->db_realloc == NULL) {
		if(ptr == NULL)
			return __os_umalloc(env, size, storep);
		*(void **)storep = (DB_GLOBAL(j_realloc) != NULL) ? DB_GLOBAL(j_realloc) (ptr, size) : realloc(ptr, size);
		if(*(void **)storep == NULL) {
			/*
			 * Correct errno, see __os_realloc.
			 */
			if((ret = __os_get_errno_ret_zero()) == 0) {
				ret = ENOMEM;
				__os_set_errno(ENOMEM);
			}
			__db_err(env, ret, DB_STR_A("0145", "realloc: %lu", "%lu"), (ulong)size);
			return ret;
		}
		return 0;
	}
	if((*(void **)storep = dbenv->db_realloc(ptr, size)) == NULL) {
		__db_errx(env, DB_STR("0146", "User-specified realloc function returned NULL"));
		return ENOMEM;
	}
	return 0;
}
/*
 * __os_ufree --
 *	Free memory used by the application.
 *
 *	A free(3) counterpart to __os_umalloc's malloc(3).
 *
 * PUBLIC: void __os_ufree __P((ENV *, void *));
 */
void FASTCALL __os_ufree(ENV * env, void * ptr)
{
	DB_ENV * dbenv = env ? env->dbenv : NULL;
	if(dbenv && dbenv->db_free)
		dbenv->db_free(ptr);
	else if(DB_GLOBAL(j_free))
		DB_GLOBAL(j_free) (ptr);
	else
		free(ptr);
}
/*
 * __os_strdup --
 *	The strdup(3) function for DB.
 *
 * PUBLIC: int __os_strdup __P((ENV *, const char *, void *));
 */
int __os_strdup(ENV * env, const char * str, void * storep)
{
	size_t size;
	int ret;
	void * p;
	*(void **)storep = NULL;
	size = strlen(str)+1;
	if((ret = __os_malloc(env, size, &p)) != 0)
		return ret;
	memcpy(p, str, size);
	*(void **)storep = p;
	return 0;
}
/*
 * __os_calloc --
 *	The calloc(3) function for DB.
 *
 * PUBLIC: int __os_calloc __P((ENV *, size_t, size_t, void *));
 */
int __os_calloc(ENV * env, size_t num, size_t size, void * storep)
{
	int ret;
	size *= num;
	if((ret = __os_malloc(env, size, storep)) != 0)
		return ret;
	memzero(*(void **)storep, size);
	return 0;
}
/*
 * __os_malloc --
 *	The malloc(3) function for DB.
 *
 * PUBLIC: int __os_malloc(ENV *, size_t, void *);
 */
int __os_malloc(ENV * env, size_t size, void * storep)
{
	int ret;
	void * p;
	*(void **)storep = NULL;
	/* Never allocate 0 bytes -- some C libraries don't like it. */
	if(size == 0)
		++size;
#ifdef DIAGNOSTIC
	/* Add room for size and a guard byte. */
	size += sizeof(db_allocinfo_t)+1;
#endif
	p = (DB_GLOBAL(j_malloc) != NULL) ? DB_GLOBAL(j_malloc) (size) : malloc(size);
	if(p == NULL) {
		/*
		 * Some C libraries don't correctly set errno when malloc(3)
		 * fails.  We'd like to 0 out errno before calling malloc,
		 * but it turns out that setting errno is quite expensive on
		 * Windows/NT in an MT environment.
		 */
		if((ret = __os_get_errno_ret_zero()) == 0) {
			ret = ENOMEM;
			__os_set_errno(ENOMEM);
		}
		__db_err(env, ret, DB_STR_A("0147", "malloc: %lu", "%lu"), (ulong)size);
		return ret;
	}
#ifdef DIAGNOSTIC
	/* Overwrite memory. */
	memset(p, CLEAR_BYTE, size);
	/*
	 * Guard bytes: if #DIAGNOSTIC is defined, we allocate an additional
	 * byte after the memory and set it to a special value that we check
	 * for when the memory is free'd.
	 */
	((uint8 *)p)[size-1] = CLEAR_BYTE;
	((db_allocinfo_t *)p)->size = size;
	p = &((db_allocinfo_t *)p)[1];
#endif
	*(void **)storep = p;
	return 0;
}
/*
 * __os_realloc --
 *	The realloc(3) function for DB.
 *
 * PUBLIC: int __os_realloc(ENV *, size_t, void *);
 */
int __os_realloc(ENV * env, size_t size, void * storep)
{
	int ret;
	void * p;
	void * ptr = *(void **)storep;
	/* Never allocate 0 bytes -- some C libraries don't like it. */
	if(size == 0)
		++size;
	/* If we haven't yet allocated anything yet, simply call malloc. */
	if(ptr == NULL)
		return __os_malloc(env, size, storep);
#ifdef DIAGNOSTIC
	/* Add room for size and a guard byte. */
	size += sizeof(db_allocinfo_t)+1;
	/* Back up to the real beginning */
	ptr = &((db_allocinfo_t *)ptr)[-1];
	{
		size_t s = ((db_allocinfo_t *)ptr)->size;
		if(((uint8 *)ptr)[s-1] != CLEAR_BYTE)
			__os_guard(env);
	}
#endif
	/*
	 * Don't overwrite the original pointer, there are places in DB we
	 * try to continue after realloc fails.
	 */
	p = (DB_GLOBAL(j_realloc) != NULL) ? DB_GLOBAL(j_realloc) (ptr, size) : realloc(ptr, size);
	if(p == NULL) {
		/*
		 * Some C libraries don't correctly set errno when malloc(3)
		 * fails.  We'd like to 0 out errno before calling malloc,
		 * but it turns out that setting errno is quite expensive on
		 * Windows/NT in an MT environment.
		 */
		if((ret = __os_get_errno_ret_zero()) == 0) {
			ret = ENOMEM;
			__os_set_errno(ENOMEM);
		}
		__db_err(env, ret, DB_STR_A("0148", "realloc: %lu", "%lu"), (ulong)size);
		return ret;
	}
#ifdef DIAGNOSTIC
	((uint8 *)p)[size-1] = CLEAR_BYTE;   /* Initialize guard byte. */

	((db_allocinfo_t *)p)->size = size;
	p = &((db_allocinfo_t *)p)[1];
#endif
	*(void **)storep = p;
	return 0;
}
/*
 * __os_free --
 *	The free(3) function for DB.
 *
 * PUBLIC: void __os_free __P((ENV *, void *));
 */
void FASTCALL __os_free(ENV * env, void * ptr)
{
	/*
	 * ANSI C requires free(NULL) work.  Don't depend on the underlying
	 * library.
	 */
	if(ptr) {
#ifdef DIAGNOSTIC
		size_t size;
		/*
		* Check that the guard byte (one past the end of the memory) is
		* still CLEAR_BYTE.
		*/
		ptr = &((db_allocinfo_t *)ptr)[-1];
		size = ((db_allocinfo_t *)ptr)->size;
		if(((uint8 *)ptr)[size-1] != CLEAR_BYTE)
			__os_guard(env);
		/* Overwrite memory. */
		if(size != 0)
			memset(ptr, CLEAR_BYTE, size);
#else
		COMPQUIET(env, NULL);
#endif
		if(DB_GLOBAL(j_free) != NULL)
			DB_GLOBAL(j_free)(ptr);
		else
			free(ptr);
	}
}

#ifdef DIAGNOSTIC
/*
 * __os_guard --
 *	Complain and abort.
 */
static void __os_guard(ENV*env)
{
	__db_errx(env, DB_STR("0149", "Guard byte incorrect during free"));
	__os_abort(env);
	/* NOTREACHED */
}
#endif
/*
 * __ua_memcpy --
 *	Copy memory to memory without relying on any kind of alignment.
 *
 *	There are places in DB that we have unaligned data, for example,
 *	when we've stored a structure in a log record as a DBT, and now
 *	we want to look at it.  Unfortunately, if you have code like:
 *
 *		struct a {
 *			int x;
 *		} *p;
 *
 *		void *func_argument;
 *		int local;
 *
 *		p = (struct a *)func_argument;
 *		memcpy(&local, p->x, sizeof(local));
 *
 *	compilers optimize to use inline instructions requiring alignment,
 *	and records in the log don't have any particular alignment.  (This
 *	isn't a compiler bug, because it's a structure they're allowed to
 *	assume alignment.)
 *
 *	Casting the memcpy arguments to (uint8 *) appears to work most
 *	of the time, but we've seen examples where it wasn't sufficient
 *	and there's nothing in ANSI C that requires that work.
 *
 * PUBLIC: void *__ua_memcpy __P((void *, const void *, size_t));
 */
void * __ua_memcpy(void * dst, const void * src, size_t len)
{
	return (void *)memcpy(dst, src, len);
}
//
//
//
/*
 * __os_ctime --
 *	Format a time-stamp.
 *
 * PUBLIC: char *__os_ctime __P((const __time64_t *, char *));
 */
char * FASTCALL __os_ctime(const __time64_t * tod, char * time_buf)
{
	time_buf[CTIME_BUFLEN-1] = '\0';
	/*
	 * The ctime_r interface is the POSIX standard, thread-safe version of
	 * ctime.  However, it was implemented in three different ways (with
	 * and without a buffer length argument, and where the buffer length
	 * argument was an int vs. a size_t *).  Also, you can't depend on a
	 * return of (char *) from ctime_r, HP-UX 10.XX's version returned an
	 * int.
	 */
#if defined(HAVE_VXWORKS)
	{
		size_t buflen = CTIME_BUFLEN;
		ctime_r(tod, time_buf, &buflen);
	}
#elif defined(HAVE_CTIME_R_3ARG)
	ctime_r(tod, time_buf, CTIME_BUFLEN);
#elif defined(HAVE_CTIME_R)
	ctime_r(tod, time_buf);
#else
	strncpy(time_buf, _ctime64(tod), CTIME_BUFLEN-1);
#endif
	return time_buf;
}
//
//
//
/*
 * __os_getaddrinfo and __os_freeaddrinfo wrap the getaddrinfo and freeaddrinfo
 * calls, as well as the associated platform dependent error handling, mapping
 * the error return to a ANSI C/POSIX error return.
 */

/*
 * __os_getaddrinfo --
 *
 * PUBLIC: #if defined(HAVE_REPLICATION_THREADS)
 * PUBLIC: int __os_getaddrinfo __P((ENV *, const char *, uint,
 * PUBLIC:    const char *, const ADDRINFO *, ADDRINFO **));
 * PUBLIC: #endif
 */
int __os_getaddrinfo(ENV * env, const char * nodename, uint port, const char * servname, const ADDRINFO * hints, ADDRINFO ** res)
{
#ifdef HAVE_GETADDRINFO
	int ret;
	if((ret = getaddrinfo(nodename, servname, hints, res)) == 0)
		return 0;
	__db_errx(env, DB_STR_A("0153", "%s(%u): host lookup failed: %s", "%s %u %s"), nodename == NULL ? "" : nodename, port,
 #ifdef DB_WIN32
		gai_strerrorA(ret));
 #else
		gai_strerror(ret));
 #endif
	return __os_posix_err(ret);
#else
	ADDRINFO * answer;
	struct hostent * hostaddr;
	struct sockaddr_in sin;
	uint32 tmpaddr;
	int ret;

	COMPQUIET(hints, NULL);
	COMPQUIET(servname, NULL);

	/* INADDR_NONE is not defined on Solaris 2.6, 2.7 or 2.8. */
 #ifndef INADDR_NONE
  #define INADDR_NONE     ((ulong)0xffffffff)
 #endif

	/*
	 * Basic implementation of IPv4 component of getaddrinfo.
	 * Limited to the functionality used by repmgr.
	 */
	memzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	if(nodename) {
		if(nodename[0] == '\0')
			sin.sin_addr.s_addr = htonl(INADDR_ANY);
		else if((tmpaddr = inet_addr(CHAR_STAR_CAST nodename)) != INADDR_NONE) {
			sin.sin_addr.s_addr = tmpaddr;
		}
		else {
			hostaddr = gethostbyname(nodename);
			if(hostaddr == NULL) {
 #ifdef DB_WIN32
				ret = __os_get_neterr();
				__db_syserr(env, ret, DB_STR_A("0154", "%s(%u): host lookup failed", "%s %u"), nodename == NULL ? "" : nodename, port);
				return __os_posix_err(ret);
 #else
				/*
				 * Historic UNIX systems used the h_errno
				 * global variable to return gethostbyname
				 * errors.  The only function we currently
				 * use that needs h_errno is gethostbyname,
				 * so we deal with it here.
				 *
				 * hstrerror is not available on Solaris 2.6
				 * (it is in libresolv but is a private,
				 * unexported symbol).
				 */
  #ifdef HAVE_HSTRERROR
				__db_errx(env, DB_STR_A("0155", "%s(%u): host lookup failed: %s", "%s %u %s"), nodename == NULL ? "" : nodename, port, hstrerror(h_errno));
  #else
				__db_errx(env, DB_STR_A("0156", "%s(%u): host lookup failed: %d", "%s %u %d"), nodename == NULL ? "" : nodename, port, h_errno);
  #endif
				switch(h_errno) {
				    case HOST_NOT_FOUND:
				    case NO_DATA: return EHOSTUNREACH;
				    case TRY_AGAIN: return EAGAIN;
				    case NO_RECOVERY:
				    default: return EFAULT;
				}
				/* NOTREACHED */
 #endif
			}
			memcpy(&(sin.sin_addr), hostaddr->h_addr, (size_t)hostaddr->h_length);
		}
	}
	else                                    /* No host specified. */
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons((uint16)port);
	if((ret = __os_calloc(env, 1, sizeof(ADDRINFO), &answer)) != 0)
		return ret;
	if((ret = __os_malloc(env, sizeof(sin), &answer->ai_addr)) != 0) {
		__os_free(env, answer);
		return ret;
	}
	answer->ai_family = AF_INET;
	answer->ai_protocol = IPPROTO_TCP;
	answer->ai_socktype = SOCK_STREAM;
	answer->ai_addrlen = sizeof(sin);
	memcpy(answer->ai_addr, &sin, sizeof(sin));
	*res = answer;

	return 0;
#endif /* HAVE_GETADDRINFO */
}

/*
 * __os_freeaddrinfo --
 *
 * PUBLIC: #if defined(HAVE_REPLICATION_THREADS)
 * PUBLIC: void __os_freeaddrinfo __P((ENV *, ADDRINFO *));
 * PUBLIC: #endif
 */
void __os_freeaddrinfo(ENV * env, ADDRINFO * ai)
{
#ifdef HAVE_GETADDRINFO
	COMPQUIET(env, NULL);
	freeaddrinfo(ai);
#else
	ADDRINFO * next, * tmpaddr;
	for(next = ai; next != NULL; next = tmpaddr) {
		__os_free(env, next->ai_canonname);
		__os_free(env, next->ai_addr);
		tmpaddr = next->ai_next;
		__os_free(env, next);
	}
#endif
}
//
//
//
/*
 * __os_isroot --
 *	Return if user has special permissions.
 *
 * PUBLIC: int __os_isroot();
 */
int __os_isroot()
{
#ifdef HAVE_GETUID
	return (getuid() == 0);
#else
	return (0);
#endif
}
//
//
//
/*
 * __db_rpath --
 *	Return the last path separator in the path or NULL if none found.
 *
 * PUBLIC: char *__db_rpath __P((const char *));
 */
char * __db_rpath(const char * path)
{
	const char * s = path;
	const char * last = NULL;
	if(PATH_SEPARATOR[1] != '\0') {
		for(; s[0] != '\0'; ++s)
			if(strchr(PATH_SEPARATOR, s[0]) != NULL)
				last = s;
	}
	else
		for(; s[0] != '\0'; ++s)
			if(s[0] == PATH_SEPARATOR[0])
				last = s;
	return (char *)last;
}
//
//
//
#ifdef HAVE_SYSTEM_INCLUDE_FILES
	#ifdef macintosh
		#include <TFileSpec.h>
	#endif
#endif
/*
 * __os_tmpdir --
 *	Set the temporary directory path.
 *
 * The order of items in the list structure and the order of checks in
 * the environment are documented.
 *
 * PUBLIC: int __os_tmpdir __P((ENV *, uint32));
 */
int __os_tmpdir(ENV * env, uint32 flags)
{
	int isdir, ret;
	char * tdir, tdir_buf[DB_MAXPATHLEN];
	DB_ENV * dbenv = env->dbenv;
	/* Use the environment if it's permitted and initialized. */
	if(LF_ISSET(DB_USE_ENVIRON) || (LF_ISSET(DB_USE_ENVIRON_ROOT) && __os_isroot())) {
		/* POSIX: TMPDIR */
		tdir = tdir_buf;
		if((ret = __os_getenv(env, "TMPDIR", &tdir, sizeof(tdir_buf))) != 0)
			return ret;
		if(tdir != NULL && tdir[0] != '\0')
			goto found;
		/*
		 * Windows: TEMP, TMP
		 */
		tdir = tdir_buf;
		if((ret = __os_getenv(env, "TEMP", &tdir, sizeof(tdir_buf))) != 0)
			return ret;
		if(tdir != NULL && tdir[0] != '\0')
			goto found;
		tdir = tdir_buf;
		if((ret = __os_getenv(env, "TMP", &tdir, sizeof(tdir_buf))) != 0)
			return ret;
		if(tdir != NULL && tdir[0] != '\0')
			goto found;
		/* Macintosh */
		tdir = tdir_buf;
		if((ret = __os_getenv(env, "TempFolder", &tdir, sizeof(tdir_buf))) != 0)
			return ret;
		if(tdir != NULL && tdir[0] != '\0')
found:                  return __os_strdup(env, tdir, &dbenv->db_tmp_dir);
	}
#ifdef macintosh
	/* Get the path to the temporary folder. */
	{FSSpec spec;
	 if(!Special2FSSpec(kTemporaryFolderType, kOnSystemDisk, 0, &spec))
		 return __os_strdup(env, FSp2FullPath(&spec), &dbenv->db_tmp_dir); }
#endif
#ifdef DB_WIN32
	/* Get the path to the temporary directory. */
	{
		_TCHAR tpath[DB_MAXPATHLEN+1];
		char * path, * eos;
		if(GetTempPath(DB_MAXPATHLEN, tpath) > 2) {
			FROM_TSTRING(env, tpath, path, ret);
			if(ret != 0)
				return ret;
			eos = path+strlen(path)-1;
			if(*eos == '\\' || *eos == '/')
				*eos = '\0';
			if(__os_exists(env, path, &isdir) == 0 && isdir) {
				ret = __os_strdup(env, path, &dbenv->db_tmp_dir);
				FREE_STRING(env, path);
				return ret;
			}
			FREE_STRING(env, path);
		}
	}
#endif

	/*
	 * Step through the static list looking for a possibility.
	 *
	 * We don't use the obvious data structure because some C compilers
	 * (and I use the phrase loosely) don't like static data arrays.
	 */
#define DB_TEMP_DIRECTORY(n) { char * __p = n; if(__os_exists(env, __p, &isdir) == 0 && isdir != 0) return (__os_strdup(env, __p, &dbenv->db_tmp_dir)); }
#ifdef DB_WIN32
	DB_TEMP_DIRECTORY("/temp");
	DB_TEMP_DIRECTORY("C:/temp");
	DB_TEMP_DIRECTORY("C:/tmp");
#else
	DB_TEMP_DIRECTORY("/var/tmp");
	DB_TEMP_DIRECTORY("/usr/tmp");
	DB_TEMP_DIRECTORY("/tmp");
 #if defined(ANDROID) || defined(DB_ANDROID)
	DB_TEMP_DIRECTORY("/cache");
 #endif
#endif

	/*
	 * If we don't have any other place to store temporary files, store
	 * them in the current directory.
	 */
	return __os_strdup(env, "", &dbenv->db_tmp_dir);
}
//
//
//
/*
 * __os_unique_id --
 *	Return a unique 32-bit value.
 *
 * PUBLIC: void __os_unique_id __P((ENV *, uint32 *));
 */
void __os_unique_id(ENV * env, uint32 * idp)
{
	DB_ENV * dbenv = env ? env->dbenv : 0;
	db_timespec v;
	pid_t pid;
	uint32 id;
	*idp = 0;
	/*
	 * Our randomized value is comprised of our process ID, the current
	 * time of day and a stack address, all XOR'd together.
	 */
	__os_id(dbenv, &pid, NULL);
	__os_gettime(env, &v, 1);
	id = (uint32)pid^(uint32)v.tv_sec^(uint32)v.tv_nsec^P_TO_UINT32(&pid);
	/*
	 * We could try and find a reasonable random-number generator, but
	 * that's not all that easy to do.  Seed and use srand()/rand(), if
	 * we can find them.
	 */
	if(DB_GLOBAL(uid_init) == 0) {
		DB_GLOBAL(uid_init) = 1;
		srand((uint)id);
	}
	id ^= (uint)rand();
	*idp = id;
}
