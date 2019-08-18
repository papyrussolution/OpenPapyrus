/*-
 * See the file LICENSE for redistribution information.
 * Copyright (c) 1996, 2011 Oracle and/or its affiliates.  All rights reserved.
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop

static int __lock_sort_cmp(const void *, const void *);
/*
 * Lock list routines.
 *	The list is composed of a 32-bit count of locks followed by
 * each lock.  A lock is represented by a 16-bit page-count, a lock
 * object and a page list.  A lock object consists of a 16-bit size
 * and the object itself.   In a pseudo BNF notation, you get:
 *
 * LIST = COUNT32 LOCK*
 * LOCK = COUNT16 LOCKOBJ PAGELIST
 * LOCKOBJ = COUNT16 OBJ
 * PAGELIST = COUNT32*
 *
 * (Recall that X* means "0 or more X's")
 *
 * In most cases, the OBJ is a struct __db_ilock and the page list is
 * a series of (32-bit) page numbers that should get written into the
 * pgno field of the __db_ilock.  So, the actual number of pages locked
 * is the number of items in the PAGELIST plus 1. If this is an application-
 * specific lock, then we cannot interpret obj and the pagelist must
 * be empty.
 *
 * Consider a lock list for: File A, pages 1&2, File B pages 3-5, Applock
 * This would be represented as:
 *	5 1 [fid=A;page=1] 2 2 [fid=B;page=3] 4 5 0 APPLOCK
 *   ------------------ -------------------- ---------
 *    LOCK for file A    LOCK for file B     application-specific lock
 */

#define MAX_PGNOS       0xffff

/*
 * These macros are bigger than one might expect because some compilers say a
 * cast does not return an lvalue, so constructs like *(uint32 *)dp = count;
 * generate warnings.
 */
#define RET_SIZE(size, count)  ((size)+sizeof(uint32)+(count)*2*sizeof(uint16))
#define PUT_COUNT(dp, count)    do { uint32 __c = (count); LOGCOPY_32(env, dp, &__c); dp = (uint8 *)dp+sizeof(uint32); } while(0)
#define PUT_PCOUNT(dp, count)   do { uint16 __c = (count); LOGCOPY_16(env, dp, &__c); dp = (uint8 *)dp+sizeof(uint16); } while(0)
#define PUT_SIZE(dp, size)      do { uint16 __s = (size); LOGCOPY_16(env, dp, &__s); dp = (uint8 *)dp+sizeof(uint16); } while(0)
#define PUT_PGNO(dp, pgno)      do { db_pgno_t __pg = (pgno); LOGCOPY_32(env, dp, &__pg); dp = (uint8 *)dp+sizeof(db_pgno_t); } while(0)
#define COPY_OBJ(dp, obj)       do { memcpy(dp, (obj)->data, (obj)->size); dp = (uint8 *)dp+DB_ALIGN((obj)->size, sizeof(uint32)); } while(0)
#define GET_COUNT(dp, count)    do { LOGCOPY_32(env, &count, dp); dp = (uint8 *)dp+sizeof(uint32); } while(0)
#define GET_PCOUNT(dp, count)   do { LOGCOPY_16(env, &count, dp); dp = (uint8 *)dp+sizeof(uint16);} while(0)
#define GET_SIZE(dp, size)      do { LOGCOPY_16(env, &size, dp); dp = (uint8 *)dp+sizeof(uint16); } while(0)
#define GET_PGNO(dp, pgno)      do { LOGCOPY_32(env, &pgno, dp); dp = (uint8 *)dp+sizeof(db_pgno_t); } while(0)
/*
 * __lock_fix_list --
 *
 * PUBLIC: int __lock_fix_list __P((ENV *, DBT *, uint32));
 */
int __lock_fix_list(ENV * env, DBT * list_dbt, uint32 nlocks)
{
	DBT * obj;
	DB_LOCK_ILOCK * lock, * plock;
	uint32 i, j, nfid, npgno, size;
	uint8 * data, * dp;
	int ret;
	if((size = list_dbt->size) == 0)
		return 0;
	obj = (DBT *)list_dbt->data;
	/*
	 * If necessary sort the list of locks so that locks on the same fileid
	 * are together.  We do not sort 1 or 2 locks because by definition if
	 * there are locks on the same fileid they will be together.  The sort
	 * will also move any locks that do not look like page locks to the end
	 * of the list so we can stop looking for locks we can combine when we
	 * hit one.
	 */
	switch(nlocks) {
	    case 1:
		size = RET_SIZE(obj->size, 1);
		if((ret = __os_malloc(env, size, &data)) != 0)
			return ret;
		dp = data;
		PUT_COUNT(dp, 1);
		PUT_PCOUNT(dp, 0);
		PUT_SIZE(dp, obj->size);
		COPY_OBJ(dp, obj);
		break;
	    default:
		/* Sort so that all locks with same fileid are together. */
		qsort(list_dbt->data, nlocks, sizeof(DBT), __lock_sort_cmp);
	    // @fallthrough
	    case 2:
		nfid = npgno = 0;
		i = 0;
		if(obj->size != sizeof(DB_LOCK_ILOCK))
			goto not_ilock;
		nfid = 1;
		plock = (DB_LOCK_ILOCK *)obj->data;

		/* We use ulen to keep track of the number of pages. */
		j = 0;
		obj[0].ulen = 0;
		for(i = 1; i < nlocks; i++) {
			if(obj[i].size != sizeof(DB_LOCK_ILOCK))
				break;
			lock = (DB_LOCK_ILOCK *)obj[i].data;
			if(obj[j].ulen < MAX_PGNOS && lock->type == plock->type && memcmp(lock->fileid, plock->fileid, DB_FILE_ID_LEN) == 0) {
				obj[j].ulen++;
				npgno++;
			}
			else {
				nfid++;
				plock = lock;
				j = i;
				obj[j].ulen = 0;
			}
		}
not_ilock:
		size = nfid*sizeof(DB_LOCK_ILOCK);
		size += npgno*sizeof(db_pgno_t);
		/* Add the number of nonstandard locks and get their size. */
		nfid += nlocks-i;
		for(; i < nlocks; i++) {
			size += obj[i].size;
			obj[i].ulen = 0;
		}
		size = RET_SIZE(size, nfid);
		if((ret = __os_malloc(env, size, &data)) != 0)
			return ret;
		dp = data;
		PUT_COUNT(dp, nfid);

		for(i = 0; i < nlocks; i = j) {
			PUT_PCOUNT(dp, obj[i].ulen);
			PUT_SIZE(dp, obj[i].size);
			COPY_OBJ(dp, &obj[i]);
			lock = (DB_LOCK_ILOCK *)obj[i].data;
			for(j = i+1; j <= i+obj[i].ulen; j++) {
				lock = (DB_LOCK_ILOCK *)obj[j].data;
				PUT_PGNO(dp, lock->pgno);
			}
		}
	}
	__os_free(env, list_dbt->data);
	list_dbt->data = data;
	list_dbt->size = size;
	return 0;
}
/*
 * PUBLIC: int __lock_get_list __P((ENV *, DB_LOCKER *, uint32,
 * PUBLIC:	      db_lockmode_t, DBT *));
 */
int __lock_get_list(ENV * env, DB_LOCKER * locker, uint32 flags, db_lockmode_t lock_mode, DBT * list)
{
	DBT obj_dbt;
	DB_LOCK ret_lock;
	DB_LOCKREGION * region;
	DB_LOCKTAB * lt;
	DB_LOCK_ILOCK * lock;
	db_pgno_t save_pgno;
	uint16 npgno, size;
	uint32 i, nlocks;
	int ret;
	void * data, * dp;
	if(list->size == 0)
		return 0;
	ret = 0;
	data = NULL;

	lt = env->lk_handle;
	dp = list->data;
	/*
	 * There is no assurance log records will be aligned.  If not, then
	 * copy the data to an aligned region so the rest of the code does
	 * not have to worry about it.
	 */
	if((uintptr_t)dp != DB_ALIGN((uintptr_t)dp, sizeof(uint32))) {
		if((ret = __os_malloc(env, list->size, &data)) != 0)
			return ret;
		memcpy(data, list->data, list->size);
		dp = data;
	}
	region = (DB_LOCKREGION *)lt->reginfo.primary;
	LOCK_SYSTEM_LOCK(lt, region);
	GET_COUNT(dp, nlocks);

	for(i = 0; i < nlocks; i++) {
		GET_PCOUNT(dp, npgno);
		GET_SIZE(dp, size);
		lock = (DB_LOCK_ILOCK *)dp;
		save_pgno = lock->pgno;
		obj_dbt.data = dp;
		obj_dbt.size = size;
		dp = ((uint8 *)dp)+DB_ALIGN(size, sizeof(uint32));
		do {
			if((ret = __lock_get_internal(lt, locker, flags, &obj_dbt, lock_mode, 0, &ret_lock)) != 0) {
				lock->pgno = save_pgno;
				goto err;
			}
			if(npgno != 0)
				GET_PGNO(dp, lock->pgno);
		} while(npgno-- != 0);
		lock->pgno = save_pgno;
	}
err:
	LOCK_SYSTEM_UNLOCK(lt, region);
	__os_free(env, data);
	return ret;
}

#define UINT32_CMP(A, B)        ((A) == (B) ? 0 : ((A) > (B) ? 1 : -1))
static int __lock_sort_cmp(const void * a, const void * b)
{
	DB_LOCK_ILOCK * l1, * l2;
	const DBT * d1 = (const DBT *)a;
	const DBT * d2 = (const DBT *)b;
	/* Force all non-standard locks to sort at end. */
	if(d1->size != sizeof(DB_LOCK_ILOCK)) {
		if(d2->size != sizeof(DB_LOCK_ILOCK))
			return UINT32_CMP(d1->size, d2->size);
		else
			return 1;
	}
	else if(d2->size != sizeof(DB_LOCK_ILOCK))
		return -1;
	l1 = (DB_LOCK_ILOCK *)d1->data;
	l2 = (DB_LOCK_ILOCK *)d2->data;
	if(l1->type != l2->type)
		return UINT32_CMP(l1->type, l2->type);
	return memcmp(l1->fileid, l2->fileid, DB_FILE_ID_LEN);
}
/*
 * PUBLIC: void __lock_list_print __P((ENV *, DB_MSGBUF *, DBT *));
 */
void __lock_list_print(ENV * env, DB_MSGBUF * mbp, DBT * list)
{
	DB_LOCK_ILOCK * lock;
	db_pgno_t pgno;
	uint16 npgno, size;
	uint32 i, nlocks;
	uint8 * fidp;
	char * fname, * dname, * p, namebuf[26];
	void * dp;
	if(list->size == 0)
		return;
	dp = list->data;

	GET_COUNT(dp, nlocks);

	for(i = 0; i < nlocks; i++) {
		GET_PCOUNT(dp, npgno);
		GET_SIZE(dp, size);
		lock = (DB_LOCK_ILOCK *)dp;
		fidp = lock->fileid;
		__dbreg_get_name(env, fidp, &fname, &dname);
		__db_msgadd(env, mbp, "\t");
		if(fname == NULL && dname == NULL)
			__db_msgadd(env, mbp, "(%lx %lx %lx %lx %lx)", (ulong)fidp[0], (ulong)fidp[1], (ulong)fidp[2], (ulong)fidp[3], (ulong)fidp[4]);
		else {
			if(fname != NULL && dname != NULL) {
				snprintf(namebuf, sizeof(namebuf), "%14s.%-10s", fname, dname);
				p = namebuf;
			}
			else if(fname != NULL)
				p = fname;
			else
				p = dname;
			__db_msgadd(env, mbp, "%-25s", p);
		}
		dp = ((uint8 *)dp)+DB_ALIGN(size, sizeof(uint32));
		LOGCOPY_32(env, &pgno, &lock->pgno);
		do {
			__db_msgadd(env, mbp, " %d", pgno);
			if(npgno != 0)
				GET_PGNO(dp, pgno);
		} while(npgno-- != 0);
		__db_msgadd(env, mbp, "\n");
	}
}
