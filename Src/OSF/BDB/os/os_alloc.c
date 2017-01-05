/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2011 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/lock.h"
#include "dbinc/mp.h"
#include "dbinc/crypto.h"
#include "dbinc/btree.h"
#include "dbinc/hash.h"
#pragma hdrstop

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
	DB_ENV * dbenv = (env == NULL) ? NULL : env->dbenv;
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
	DB_ENV * dbenv = (env == NULL) ? NULL : env->dbenv;
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
void __os_ufree(ENV * env, void * ptr)
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
void __os_free(ENV * env, void * ptr)
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
