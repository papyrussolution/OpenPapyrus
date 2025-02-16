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

#ifdef HAVE_PARTITION

static int __part_rr(DB*, DB_THREAD_INFO*, DB_TXN*, const char *, const char *, const char *, uint32);
static int __partc_close(DBC*, db_pgno_t, int *);
static int __partc_del(DBC*, uint32);
static int __partc_destroy(DBC *);
static int __partc_get_pp(DBC*, DBT*, DBT*, uint32);
static int __partc_put(DBC*, DBT*, DBT*, uint32, db_pgno_t *);
static int __partc_writelock(DBC *);
static int __partition_chk_meta(DB*, DB_THREAD_INFO*, DB_TXN*, uint32);
static int __partition_setup_keys(DBC*, DB_PARTITION*, const DBMETA*, uint32);
static int __part_key_cmp(const void *, const void *);
static inline void __part_search(DB*, DB_PARTITION*, DBT*, uint32 *);

static const char * Alloc_err = DB_STR_A("0644", "Partition open failed to allocate %d bytes", "%d");
/*
 * Allocate a partition cursor and copy flags to the partition cursor.
 * Not passed:
 *	DBC_PARTITIONED -- the subcursors are not.
 *	DBC_OWN_LID -- the arg dbc owns the lock id.
 *	DBC_WRITECURSOR DBC_WRITER -- CDS locking happens on
 *				the whole DB, not the partition.
 */
 #define GET_PART_CURSOR(dbc, new_dbc, part_id) do {                          \
	DB * __part_dbp = part->handles[part_id];                                 \
	if((ret = __db_cursor_int(__part_dbp, (dbc)->thread_info, (dbc)->txn, __part_dbp->type, PGNO_INVALID, 0, (dbc)->locker, &new_dbc)) != 0) \
		goto err;                                                    \
	(new_dbc)->flags = (dbc)->flags&~(DBC_PARTITIONED|DBC_OWN_LID|DBC_WRITECURSOR|DBC_WRITER);       \
} while(0)
/*
 * Search for the correct partition.(DB*, const DBT*, const DBT *)
 */
static inline void __part_search(DB * dbp, DB_PARTITION * part, DBT * key, uint32 * part_idp)
{
	db_indx_t base, indx, limit;
	int cmp;
	int (*func)(DB*, const DBT*, const DBT *);
	DB_ASSERT(dbp->env, part->nparts != 0);
	COMPQUIET(cmp, 0);
	COMPQUIET(indx, 0);
	func = static_cast<BTREE *>(dbp->bt_internal)->bt_compare;
	DB_BINARY_SEARCH_FOR(base, limit, part->nparts, O_INDX) {
		DB_BINARY_SEARCH_INCR(indx, base, limit, O_INDX);
		cmp = func(dbp, key, &part->keys[indx]);
		if(cmp == 0)
			break;
		if(cmp > 0)
			DB_BINARY_SEARCH_SHIFT_BASE(indx, base, limit, O_INDX);
	}
	if(cmp == 0)
		*part_idp = indx;
	else if((*part_idp = base) != 0)
		(*part_idp)--;
}
/*
 * __partition_init --
 *	Initialize the partition structure.
 * Called when the meta data page is read in during database open or
 * when partition keys or a callback are set.
 *
 * PUBLIC: int __partition_init __P((DB *, uint32));
 */
int __partition_init(DB * dbp, uint32 flags)
{
	DB_PARTITION * part;
	int ret;
	if((part = (DB_PARTITION *)dbp->p_internal) != NULL) {
		if((LF_ISSET(DBMETA_PART_RANGE) && F_ISSET(part, PART_CALLBACK)) || (LF_ISSET(DBMETA_PART_CALLBACK) && F_ISSET(part, PART_RANGE))) {
			__db_errx(dbp->env, DB_STR("0645", "Cannot specify callback and range keys."));
			return EINVAL;
		}
	}
	else if((ret = __os_calloc(dbp->env, 1, sizeof(*part), &part)) != 0)
		return ret;
	if(LF_ISSET(DBMETA_PART_RANGE))
		F_SET(part, PART_RANGE);
	if(LF_ISSET(DBMETA_PART_CALLBACK))
		F_SET(part, PART_CALLBACK);
	dbp->p_internal = part;
	/* Set up AM-specific methods that do not require an open. */
	dbp->db_am_rename = __part_rename;
	dbp->db_am_remove = __part_remove;
	return 0;
}
/*
 * __partition_set --
 *	Set the partitioning keys or callback function.
 * This routine must be called prior to creating the database.
 * PUBLIC: int __partition_set __P((DB *, uint32, DBT *,
 * PUBLIC:	uint32 (*callback)(DB *, DBT *key)));
 */
int __partition_set(DB * dbp, uint32 parts, DBT * keys, uint32 (* callback)(DB *, DBT * key))
{
	DB_PARTITION * part;
	ENV * env;
	int ret;
	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_partition");
	env = dbp->dbenv->env;
	if(parts < 2) {
		__db_errx(env, DB_STR("0646", "Must specify at least 2 partitions."));
		return EINVAL;
	}
	if(keys == NULL && callback == NULL) {
		__db_errx(env, DB_STR("0647", "Must specify either keys or a callback."));
		return EINVAL;
	}
	if(keys && callback) {
bad:
		__db_errx(env, DB_STR("0648", "May not specify both keys and a callback."));
		return EINVAL;
	}
	if((ret = __partition_init(dbp, keys ? DBMETA_PART_RANGE : DBMETA_PART_CALLBACK)) != 0)
		return ret;
	part = (DB_PARTITION *)dbp->p_internal;
	if((part->keys && callback) || (part->callback && keys))
		goto bad;
	part->nparts = parts;
	part->keys = keys;
	part->callback = callback;
	return 0;
}
/*
 * __partition_set_dirs --
 *	Set the directories for creating the partition databases.
 * They must be in the environment.
 * PUBLIC: int __partition_set_dirs __P((DB *, const char **));
 */
int __partition_set_dirs(DB * dbp, const char ** dirp)
{
	DB_ENV * dbenv;
	DB_PARTITION * part;
	ENV * env;
	uint32 ndirs, slen;
	int i, ret;
	const char ** dir;
	char * cp, ** part_dirs, ** pd;
	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_partition_dirs");
	dbenv = dbp->dbenv;
	env = dbp->env;
	ndirs = 1;
	slen = 0;
	for(dir = dirp; *dir; dir++) {
		if(F_ISSET(env, ENV_DBLOCAL))
			slen += (uint32)sstrlen(*dir)+1;
		ndirs++;
	}
	slen += sizeof(char *)*ndirs;
	if((ret = __os_malloc(env, slen,  &part_dirs)) != 0)
		return EINVAL;
	memzero(part_dirs, slen);
	cp = (char *)part_dirs+(sizeof(char *)*ndirs);
	pd = part_dirs;
	for(dir = dirp; *dir; dir++, pd++) {
		if(F_ISSET(env, ENV_DBLOCAL)) {
			strcpy(cp, *dir);
			*pd = cp;
			cp += sstrlen(*dir)+1;
			continue;
		}
		for(i = 0; i < dbenv->data_next; i++)
			if(strcmp(*dir, dbenv->db_data_dir[i]) == 0)
				break;
		if(i == dbenv->data_next) {
			__db_errx(dbp->env, DB_STR_A("0649", "Directory not in environment list %s", "%s"), *dir);
			__os_free(env, part_dirs);
			return EINVAL;
		}
		*pd = dbenv->db_data_dir[i];
	}
	if((part = (DB_PARTITION *)dbp->p_internal) == NULL) {
		if((ret = __partition_init(dbp, 0)) != 0)
			return ret;
		part = (DB_PARTITION *)dbp->p_internal;
	}
	part->dirs = (const char **)part_dirs;
	return 0;
}
/*
 * __partition_open --
 *	Open/create a partitioned database.
 * PUBLIC: int __partition_open __P((DB *, DB_THREAD_INFO *,
 * PUBLIC:	 DB_TXN *, const char *, DBTYPE, uint32, int, int));
 */
int __partition_open(DB * dbp, DB_THREAD_INFO * ip, DB_TXN * txn, const char * fname, DBTYPE type, uint32 flags, int mode, int do_open)
{
	DB * part_db;
	DBC * dbc;
	uint32 part_id;
	int ret;
	char * sp;
	const char ** dirp, * np;
	DB_PARTITION * part = (DB_PARTITION *)dbp->p_internal;
	ENV * env = dbp->dbenv->env;
	char * name = NULL;
	if((ret = __partition_chk_meta(dbp, ip, txn, flags)) != 0 && do_open)
		goto err;
	if((ret = __os_calloc(env, part->nparts, sizeof(*part->handles), &part->handles)) != 0) {
		__db_errx(env, Alloc_err, part->nparts*sizeof(*part->handles));
		goto err;
	}
	DB_ASSERT(env, fname != NULL);
	if((ret = __os_malloc(env, sstrlen(fname)+PART_LEN+1, &name)) != 0) {
		__db_errx(env, Alloc_err, sstrlen(fname)+PART_LEN+1);
		goto err;
	}
	sp = name;
	np = __db_rpath(fname);
	if(!np)
		np = fname;
	else {
		np++;
		strncpy(name, fname, (size_t)(np-fname));
		sp = name+(np-fname);
	}
	if(F_ISSET(dbp, DB_AM_RECOVER))
		goto done;
	dirp = part->dirs;
	for(part_id = 0; part_id < part->nparts; part_id++) {
		if((ret = __db_create_internal(&part->handles[part_id], dbp->env, 0)) != 0)
			goto err;
		part_db = part->handles[part_id];
		part_db->flags = F_ISSET(dbp, ~(DB_AM_CREATED|DB_AM_CREATED_MSTR|DB_AM_OPEN_CALLED));
		part_db->adj_fileid = dbp->adj_fileid;
		part_db->pgsize = dbp->pgsize;
		part_db->priority = dbp->priority;
		part_db->db_append_recno = dbp->db_append_recno;
		part_db->db_feedback = dbp->db_feedback;
		part_db->dup_compare = dbp->dup_compare;
		part_db->app_private = dbp->app_private;
		part_db->api_internal = dbp->api_internal;
		if(dbp->type == DB_BTREE)
			__bam_copy_config(dbp, part_db, part->nparts);
 #ifdef HAVE_HASH
		if(dbp->type == DB_HASH)
			__ham_copy_config(dbp, part_db, part->nparts);
 #endif

		sprintf(sp, PART_NAME, np, part_id);
		if(do_open) {
			/*
			 * Cycle through the directory names passed in,
			 * if any.
			 */
			if(dirp && (part_db->dirname = *dirp++) == NULL) {
				part_db->dirname = *(dirp = part->dirs);
				dirp++;
			}
			if((ret = __db_open(part_db, ip, txn, name, NULL, type, flags, mode, PGNO_BASE_MD)) != 0)
				goto err;
		}
		else if((ret = __os_strdup(env, name, &part_db->fname)) != 0)
			goto err;
	}
	/* Get rid of the cursor used to open the database its the wrong type */
done:
	while((dbc = TAILQ_FIRST(&dbp->free_queue)) != NULL)
		if((ret = __dbc_destroy(dbc)) != 0)
			break;
	if(0) {
err:
		__partition_close(dbp, txn, 0);
	}
	__os_free(env, name);
	return ret;
}
/*
 * __partition_chk_meta --
 * Check for a consistent meta data page and parameters when opening a
 * partitioned database.
 */
static int __partition_chk_meta(DB * dbp, DB_THREAD_INFO * ip, DB_TXN * txn, uint32 flags)
{
	DB_PARTITION * part;
	DB_LOCK metalock;
	DB_MPOOLFILE * mpf;
	ENV * env;
	db_pgno_t base_pgno;
	int ret, t_ret;
	DBC * dbc = NULL;
	DBMETA * meta = NULL;
	LOCK_INIT(metalock);
	part = (DB_PARTITION *)dbp->p_internal;
	mpf = dbp->mpf;
	env = dbp->env;
	ret = 0;
	// Get a cursor on the main db
	dbp->p_internal = NULL;
	if((ret = __db_cursor(dbp, ip, txn, &dbc, 0)) != 0)
		goto err;
	// Get the metadata page
	base_pgno = PGNO_BASE_MD;
	if((ret = __db_lget(dbc, 0, base_pgno, DB_LOCK_READ, 0, &metalock)) != 0)
		goto err;
	if((ret = __memp_fget(mpf, &base_pgno, ip, dbc->txn, 0, &meta)) != 0)
		goto err;
	if(meta->magic != DB_HASHMAGIC && (meta->magic != DB_BTREEMAGIC || F_ISSET(meta, BTM_RECNO))) {
		__db_errx(env, DB_STR("0650", "Partitioning may only specified on BTREE and HASH databases."));
		ret = EINVAL;
		goto err;
	}
	if(!FLD_ISSET(meta->metaflags, DBMETA_PART_RANGE|DBMETA_PART_CALLBACK)) {
		__db_errx(env, DB_STR("0651", "Partitioning specified on a non-partitioned database."));
		ret = EINVAL;
		goto err;
	}
	if((F_ISSET(part, PART_RANGE) && FLD_ISSET(meta->metaflags, DBMETA_PART_CALLBACK)) || 
		(F_ISSET(part, PART_CALLBACK) && FLD_ISSET(meta->metaflags, DBMETA_PART_RANGE))) {
		__db_errx(env, DB_STR("0652", "Incompatible partitioning specified."));
		ret = EINVAL;
		goto err;
	}
	if(FLD_ISSET(meta->metaflags, DBMETA_PART_CALLBACK) && !part->callback && !IS_RECOVERING(env) &&
	   !F_ISSET(dbp, DB_AM_RECOVER) && !LF_ISSET(DB_RDWRMASTER)) {
		__db_errx(env, DB_STR("0653", "Partition callback not specified."));
		ret = EINVAL;
		goto err;
	}
	if(F_ISSET(dbp, DB_AM_RECNUM)) {
		__db_errx(env, DB_STR("0654", "Record numbers are not supported in partitioned databases."));
		ret = EINVAL;
		goto err;
	}
	if(part->nparts == 0) {
		if(LF_ISSET(DB_CREATE) && meta->nparts == 0) {
			__db_errx(env, DB_STR("0655", "Zero paritions specified."));
			ret = EINVAL;
			goto err;
		}
		else
			part->nparts = meta->nparts;
	}
	else if(meta->nparts != 0 && part->nparts != meta->nparts) {
		__db_errx(env, DB_STR("0656", "Number of partitions does not match."));
		ret = EINVAL;
		goto err;
	}
	if(meta->magic == DB_HASHMAGIC) {
		if(!F_ISSET(part, PART_CALLBACK)) {
			__db_errx(env, DB_STR("0657", "Hash database must specify a partition callback."));
			ret = EINVAL;
		}
	}
	else if(meta->magic != DB_BTREEMAGIC) {
		__db_errx(env, DB_STR("0658", "Partitioning only supported on BTREE nad HASH."));
		ret = EINVAL;
	}
	else
		ret = __partition_setup_keys(dbc, part, meta, flags);
err: // Put the metadata page back
	if((t_ret = __memp_fput(mpf, ip, meta, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;
	if((t_ret = __LPUT(dbc, metalock)) != 0 && ret == 0)
		ret = t_ret;
	if(dbc && (t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;
	dbp->p_internal = part;
	return ret;
}
/*
 * Support for sorting keys.  Keys must be sorted using the btree
 * compare function so if we call qsort in __partiton_setup_keys
 * we use this structure to pass the DBP and compare function.
 */
struct key_sort {
	DB * dbp;
	DBT * key;
	int (*compare)(DB*, const DBT*, const DBT *);
};

static int __part_key_cmp(const void * a, const void * b)
{
	const struct key_sort * ka = (const struct key_sort *)a;
	const struct key_sort * kb = (const struct key_sort *)b;
	return ka->compare(ka->dbp, ka->key, kb->key);
}
/*
 * __partition_setup_keys --
 *	Get the partition keys into memory, or put them to disk if we
 * are creating a partitioned database.
 */
static int __partition_setup_keys(DBC * dbc, DB_PARTITION * part, const DBMETA * meta, uint32 flags)
{
	BTREE * t;
	DBT data, key, * keys, * kp;
	uint32 ds, i, j;
	uint8 * dd;
	struct key_sort * ks = 0;
	int have_keys = 0;
	int ret;
	int (*compare)(DB*, const DBT*, const DBT *);
	void * dp;
	COMPQUIET(dd, 0);
	COMPQUIET(ds, 0);
	DB * dbp = dbc->dbp;
	ENV * env = dbp->env;
	// Need to just read the main database. 
	dbp->p_internal = NULL;
	// First verify that things what we expect. 
	if((ret = __dbc_get(dbc, &key, &data, DB_FIRST)) != 0) {
		if(ret != DB_NOTFOUND)
			goto err;
		if(F_ISSET(part, PART_CALLBACK)) {
			ret = 0;
			goto done;
		}
		if(!LF_ISSET(DB_CREATE) && !F_ISSET(dbp, DB_AM_RECOVER) && !LF_ISSET(DB_RDWRMASTER)) {
			__db_errx(env, DB_STR("0659", "No range keys found."));
			ret = EINVAL;
			goto err;
		}
	}
	else {
		if(F_ISSET(part, PART_CALLBACK)) {
			__db_errx(env, DB_STR("0660", "Keys found and callback set."));
			ret = EINVAL;
			goto err;
		}
		if(key.size != 0) {
			__db_errx(env, DB_STR("0661", "Partition key 0 is not empty."));
			ret = EINVAL;
			goto err;
		}
		have_keys = 1;
	}
	if(LF_ISSET(DB_CREATE) && have_keys == 0) {
		/* Insert the keys into the master database. */
		for(i = 0; i < part->nparts-1; i++) {
			if((ret = __db_put(dbp, dbc->thread_info, dbc->txn, &part->keys[i], &data, 0)) != 0)
				goto err;
		}
		/*
		 * Insert the "0" pointer.  All records less than the first
		 * given key go into this partition.  We must use the default
		 * compare to insert this key, otherwise it might not be first.
		 */
		t = static_cast<BTREE *>(dbc->dbp->bt_internal);
		compare = t->bt_compare;
		t->bt_compare = __bam_defcmp;
		memzero(&key, sizeof(key));
		ret = __db_put(dbp, dbc->thread_info, dbc->txn, &key, &data, 0);
		t->bt_compare = compare;
		if(ret)
			goto err;
	}
done:
	if(F_ISSET(part, PART_RANGE)) {
		/*
		 * Allocate one page to hold the keys plus space at the
		 * end of the buffer to put an array of DBTs.  If there
		 * is not enough space __dbc_get will return how much
		 * is needed and we realloc.
		 */
		if((ret = __os_malloc(env, meta->pagesize+(sizeof(DBT)*part->nparts), &part->data)) != 0) {
			__db_errx(env, Alloc_err, meta->pagesize);
			goto err;
		}
		memzero(&key, sizeof(key));
		memzero(&data, sizeof(data));
		data.data = part->data;
		data.ulen = meta->pagesize;
		data.flags = DB_DBT_USERMEM;
again:
		if((ret = __dbc_get(dbc, &key, &data, DB_FIRST|DB_MULTIPLE_KEY)) == DB_BUFFER_SMALL) {
			if((ret = __os_realloc(env, data.size+(sizeof(DBT)*part->nparts), &part->data)) != 0)
				goto err;
			data.data = part->data;
			data.ulen = data.size;
			goto again;
		}
		if(!ret) {
			/*
			 * They passed in keys, they must match.
			 */
			keys = NULL;
			compare = NULL;
			if(have_keys == 1 && (keys = part->keys) != NULL) {
				t = static_cast<BTREE *>(dbc->dbp->bt_internal);
				compare = t->bt_compare;
				if((ret = __os_malloc(env, (part->nparts-1)*sizeof(struct key_sort), &ks)) != 0)
					goto err;
				for(j = 0; j < part->nparts-1; j++) {
					ks[j].dbp = dbc->dbp;
					ks[j].compare = compare;
					ks[j].key = &keys[j];
				}
				qsort(ks, (size_t)part->nparts-1, sizeof(struct key_sort), __part_key_cmp);
			}
			DB_MULTIPLE_INIT(dp, &data);
			part->keys = (DBT *)((uint8 *)part->data+data.size);
			j = 0;
			for(kp = part->keys; kp < &part->keys[part->nparts]; kp++, j++) {
				DB_MULTIPLE_KEY_NEXT(dp, &data, kp->data, kp->size, dd, ds);
				if(dp == NULL) {
					ret = DB_NOTFOUND;
					break;
				}
				if(keys && j != 0 && compare(dbc->dbp, ks[j-1].key, kp) != 0) {
					if(kp->data == NULL && F_ISSET(dbp, DB_AM_RECOVER))
						goto err;
					__db_errx(env, DB_STR_A("0662", "Partition key %d does not match", "%d"), j);
					ret = EINVAL;
					goto err;
				}
			}
		}
	}
	if(ret == DB_NOTFOUND && F_ISSET(dbp, DB_AM_RECOVER))
		ret = 0;
err:
	dbp->p_internal = part;
	__os_free(env, ks);
	return ret;
}
/*
 * __partition_get_callback --
 *	Get the partition callback function.
 * PUBLIC: int __partition_get_callback __P((DB *,
 * PUBLIC:	 uint32 *, uint32 (**callback)(DB *, DBT *key)));
 */
int __partition_get_callback(DB * dbp, uint32 * parts, uint32 (**callback)(DB*, DBT*key))
{
	DB_PARTITION * part = (DB_PARTITION *)dbp->p_internal;
	// Only return populated results if partitioned using callbacks.
	if(part && !F_ISSET(part, PART_CALLBACK))
		part = NULL;
	ASSIGN_PTR(parts, (part ? part->nparts : 0));
	ASSIGN_PTR(callback, (part ? part->callback : NULL));
	return 0;
}
/*
 * __partition_get_keys --
 *	Get partition keys.
 * PUBLIC: int __partition_get_keys __P((DB *, uint32 *, DBT **));
 */
int __partition_get_keys(DB * dbp, uint32 * parts, DBT ** keys)
{
	DB_PARTITION * part = (DB_PARTITION *)dbp->p_internal;
	// Only return populated results if partitioned using ranges
	if(part && !F_ISSET(part, PART_RANGE))
		part = NULL;
	if(parts)
		*parts = (part ? part->nparts : 0);
	if(keys)
		*keys = (part ? &part->keys[1] : NULL);
	return 0;
}
/*
 * __partition_get_dirs --
 *	Get partition dirs.
 * PUBLIC: int __partition_get_dirs __P((DB *, const char ***));
 */
int __partition_get_dirs(DB * dbp, const char *** dirpp)
{
	DB_PARTITION * part;
	uint32 i;
	int ret;
	ENV * env = dbp->env;
	if((part = (DB_PARTITION *)dbp->p_internal) == NULL) {
		*dirpp = NULL;
		return 0;
	}
	if(!F_ISSET(dbp, DB_AM_OPEN_CALLED)) {
		*dirpp = part->dirs;
		return 0;
	}
	/*
	 * We build a list once when asked.  The original directory list,
	 * if any, was discarded at open time.
	 */
	if((*dirpp = part->dirs) != NULL)
		return 0;
	if((ret = __os_calloc(env, sizeof(char *), part->nparts+1, (char **)&part->dirs)) != 0)
		return ret;
	for(i = 0; i < part->nparts; i++)
		part->dirs[i] = part->handles[i]->dirname;
	*dirpp = part->dirs;
	return 0;
}
/*
 * __partc_init --
 *	Initialize the access private portion of a cursor
 *
 * PUBLIC: int __partc_init(DBC *);
 */
int __partc_init(DBC * dbc)
{
	int ret;
	ENV * env = dbc->env;
	// Allocate/initialize the internal structure
	if(!dbc->internal && (ret = __os_calloc(env, 1, sizeof(PART_CURSOR), &dbc->internal)) != 0)
		return ret;
	// Initialize methods
	dbc->close = dbc->c_close = __dbc_close_pp;
	dbc->cmp = __dbc_cmp_pp;
	dbc->count = dbc->c_count = __dbc_count_pp;
	dbc->del = dbc->c_del = __dbc_del_pp;
	dbc->dup = dbc->c_dup = __dbc_dup_pp;
	dbc->get = dbc->c_get = __partc_get_pp;
	dbc->pget = dbc->c_pget = __dbc_pget_pp;
	dbc->put = dbc->c_put = __dbc_put_pp;
	dbc->am_bulk = NULL;
	dbc->am_close = __partc_close;
	dbc->am_del = __partc_del;
	dbc->am_destroy = __partc_destroy;
	dbc->am_get = NULL;
	dbc->am_put = __partc_put;
	dbc->am_writelock = __partc_writelock;
	// We avoid swapping partition cursors since we swap the sub cursors 
	F_SET(dbc, DBC_PARTITIONED);
	return 0;
}
/*
 * __partc_get_pp --
 *	cursor get opeartion on a partitioned database.
 */
static int __partc_get_pp(DBC * dbc, DBT * key, DBT * data, uint32 flags)
{
	DB * dbp;
	DB_THREAD_INFO * ip;
	ENV * env;
	int ignore_lease, ret;
	dbp = dbc->dbp;
	env = dbp->env;
	ignore_lease = LF_ISSET(DB_IGNORE_LEASE) ? 1 : 0;
	LF_CLR(DB_IGNORE_LEASE);
	if((ret = __dbc_get_arg(dbc, key, data, flags)) != 0)
		return ret;
	ENV_ENTER(env, ip);
	DEBUG_LREAD(dbc, dbc->txn, "DBcursor->get", flags == DB_SET || flags == DB_SET_RANGE ? key : NULL, NULL, flags);
	ret = __partc_get(dbc, key, data, flags);
	/*
	 * Check for master leases.
	 */
	if(ret == 0 && IS_REP_MASTER(env) && IS_USING_LEASES(env) && !ignore_lease)
		ret = __rep_lease_check(env, 1);
	ENV_LEAVE(env, ip);
	__dbt_userfree(env, key, NULL, data);
	return ret;
}
/*
 * __partiton_get --
 *	cursor get opeartion on a partitioned database.
 *
 * PUBLIC: int __partc_get __P((DBC*, DBT *, DBT *, uint32));
 */
int __partc_get(DBC * dbc, DBT * key, DBT * data, uint32 flags)
{
	DBC * new_dbc = 0;
	int ret;
	DB * dbp = dbc->dbp;
	PART_CURSOR * cp = (PART_CURSOR *)dbc->internal;
	DBC * orig_dbc = cp->sub_cursor;
	DB_PARTITION * part = (DB_PARTITION *)dbp->p_internal;
	int retry = 0;
	int search = 0;
	uint32 part_id = cp->part_id;
	uint32 multi = flags&~DB_OPFLAGS_MASK;
	switch(flags&DB_OPFLAGS_MASK) {
	    case DB_CURRENT:
		break;
	    case DB_FIRST:
		part_id = 0;
		retry = 1;
		break;
	    case DB_GET_BOTH:
	    case DB_GET_BOTHC:
	    case DB_GET_BOTH_RANGE:
		search = 1;
		break;
	    case DB_SET_RANGE:
		search = 1;
		retry = 1;
		break;
	    case DB_LAST:
		part_id = part->nparts-1;
		retry = 1;
		break;
	    case DB_NEXT:
	    case DB_NEXT_NODUP:
		part_id = (orig_dbc == NULL) ? 0 : cp->part_id;
		retry = 1;
		break;
	    case DB_NEXT_DUP:
		break;
	    case DB_PREV:
	    case DB_PREV_NODUP:
		part_id = (orig_dbc == NULL) ? (part->nparts-1) : cp->part_id;
		retry = 1;
		break;
	    case DB_PREV_DUP:
		break;
	    case DB_SET:
		search = 1;
		break;
	    default:
		return __db_unknown_flag(dbp->env, "__partc_get", flags);
	}
	/*
	 * If we need to find the partition to start on, then
	 * do a binary search of the in memory partition table.
	 */
	if(search == 1 && F_ISSET(part, PART_CALLBACK))
		part_id = part->callback(dbp, key)%part->nparts;
	else if(search == 1)
		__part_search(dbp, part, key, &part_id);
	/* Get a new cursor if necessary */
	if(orig_dbc == NULL || cp->part_id != part_id) {
		GET_PART_CURSOR(dbc, new_dbc, part_id);
	}
	else
		new_dbc = orig_dbc;
	while((ret = __dbc_get(new_dbc, key, data, flags)) == DB_NOTFOUND && retry == 1) {
		switch(flags&DB_OPFLAGS_MASK) {
		    case DB_FIRST:
		    case DB_NEXT:
		    case DB_NEXT_NODUP:
		    case DB_SET_RANGE:
			if(++part_id < part->nparts) {
				flags = DB_FIRST|multi;
				break;
			}
			goto err;
		    case DB_LAST:
		    case DB_PREV:
		    case DB_PREV_NODUP:
			if(part_id-- > 0) {
				flags = DB_LAST|multi;
				break;
			}
			goto err;
		    default:
			goto err;
		}
		if(new_dbc != orig_dbc && (ret = __dbc_close(new_dbc)) != 0)
			goto err;
		GET_PART_CURSOR(dbc, new_dbc, part_id);
	}
	if(ret)
		goto err;
	/* Success: swap original and new cursors. */
	if(new_dbc != orig_dbc) {
		if(orig_dbc) {
			cp->sub_cursor = NULL;
			if((ret = __dbc_close(orig_dbc)) != 0)
				goto err;
		}
		cp->sub_cursor = new_dbc;
		cp->part_id = part_id;
	}
	return 0;
err:
	if(new_dbc != orig_dbc)
		__dbc_close(new_dbc);
	return ret;
}
/*
 * __partc_put --
 *	cursor put opeartion on a partitioned cursor.
 *
 */
static int __partc_put(DBC * dbc, DBT * key, DBT * data, uint32 flags, db_pgno_t * pgnop)
{
	DBC * new_dbc;
	int ret;
	DB * dbp = dbc->dbp;
	PART_CURSOR * cp = (PART_CURSOR *)dbc->internal;
	uint32 part_id = cp->part_id;
	DB_PARTITION * part = (DB_PARTITION *)dbp->p_internal;
	*pgnop = PGNO_INVALID;
	switch(flags) {
	    case DB_KEYFIRST:
	    case DB_KEYLAST:
	    case DB_NODUPDATA:
	    case DB_NOOVERWRITE:
	    case DB_OVERWRITE_DUP:
		if(F_ISSET(part, PART_CALLBACK)) {
			part_id = part->callback(dbp, key)%part->nparts;
			break;
		}
		__part_search(dbp, part, key, &part_id);
		break;
	    default:
		break;
	}
	if((new_dbc = cp->sub_cursor) == NULL || cp->part_id != part_id) {
		if((ret = __db_cursor_int(part->handles[part_id], dbc->thread_info, dbc->txn, part->handles[part_id]->type,
			PGNO_INVALID, 0, dbc->locker, &new_dbc)) != 0)
			goto err;
	}
	if(F_ISSET(dbc, DBC_WRITER|DBC_WRITECURSOR))
		F_SET(new_dbc, DBC_WRITER);
	if((ret = __dbc_put(new_dbc, key, data, flags)) != 0)
		goto err;
	if(new_dbc != cp->sub_cursor) {
		if(cp->sub_cursor) {
			if((ret = __dbc_close(cp->sub_cursor)) != 0)
				goto err;
			cp->sub_cursor = NULL;
		}
		cp->sub_cursor = new_dbc;
		cp->part_id = part_id;
	}
	return 0;
err:
	if(cp->sub_cursor != new_dbc)
		__dbc_close(new_dbc);
	return ret;
}
/*
 * __partc_del
 *	Delete interface to partitioned cursors.
 *
 */
static int __partc_del(DBC * dbc, uint32 flags)
{
	PART_CURSOR * cp = (PART_CURSOR *)dbc->internal;
	if(F_ISSET(dbc, DBC_WRITER|DBC_WRITECURSOR))
		F_SET(cp->sub_cursor, DBC_WRITER);
	return __dbc_del(cp->sub_cursor, flags);
}
/*
 * __partc_writelock
 *	Writelock interface to partitioned cursors.
 *
 */
static int __partc_writelock(DBC * dbc)
{
	PART_CURSOR * cp = (PART_CURSOR *)dbc->internal;
	return cp->sub_cursor->am_writelock(cp->sub_cursor);
}
/*
 * __partc_close
 *	Close interface to partitioned cursors.
 *
 */
static int __partc_close(DBC * dbc, db_pgno_t root_pgno, int * rmroot)
{
	COMPQUIET(root_pgno, 0);
	COMPQUIET(rmroot, 0);
	int ret = 0;
	PART_CURSOR * cp = (PART_CURSOR *)dbc->internal;
	if(cp->sub_cursor) {
		ret = __dbc_close(cp->sub_cursor);
		cp->sub_cursor = NULL;
	}
	return ret;
}
/*
 * __partc_destroy --
 *	Destroy a single cursor.
 */
static int __partc_destroy(DBC * dbc)
{
	PART_CURSOR * cp = (PART_CURSOR *)dbc->internal;
	ENV * env = dbc->env;
	// Discard the structure. Don't recurse.
	__os_free(env, cp);
	return 0;
}
/*
 * __partiton_close
 *	Close a partitioned database.
 *
 * PUBLIC: int __partition_close __P((DB *, DB_TXN *, uint32));
 */
int __partition_close(DB * dbp, DB_TXN * txn, uint32 flags)
{
	DB ** pdbp;
	DB_PARTITION * part;
	ENV * env;
	uint32 i;
	int ret, t_ret;
	if((part = (DB_PARTITION *)dbp->p_internal) == NULL)
		return 0;
	env = dbp->env;
	ret = 0;
	if((pdbp = part->handles) != NULL) {
		for(i = 0; i < part->nparts; i++, pdbp++)
			if(*pdbp && (t_ret = __db_close(*pdbp, txn, flags)) != 0 && ret == 0)
				ret = t_ret;
		__os_free(env, part->handles);
	}
	__os_free(env, (char **)part->dirs);
	__os_free(env, (char **)part->data);
	__os_free(env, part);
	dbp->p_internal = NULL;
	return ret;
}
/*
 * __partiton_sync
 *	Sync a partitioned database.
 *
 * PUBLIC: int __partition_sync __P((DB *));
 */
int __partition_sync(DB * dbp)
{
	DB ** pdbp;
	uint32 i;
	int t_ret;
	int ret = 0;
	DB_PARTITION * part = (DB_PARTITION *)dbp->p_internal;
	if((pdbp = part->handles) != NULL) {
		for(i = 0; i < part->nparts; i++, pdbp++)
			if(*pdbp && F_ISSET(*pdbp, DB_AM_OPEN_CALLED) && (t_ret = __memp_fsync((*pdbp)->mpf)) != 0 && ret == 0)
				ret = t_ret;
	}
	if((t_ret = __memp_fsync(dbp->mpf)) != 0 && ret == 0)
		ret = t_ret;
	return ret;
}
/*
 * __partiton_stat
 *	Stat a partitioned database.
 *
 * PUBLIC: int __partition_stat __P((DBC *, void *, uint32));
 */
int __partition_stat(DBC * dbc, void * spp, uint32 flags)
{
	DB_BTREE_STAT * bsp;
 #ifdef HAVE_HASH
	DB_HASH_STAT * hsp;
 #endif
	DBC * new_dbc;
	uint32 i;
	int ret;
	DB * dbp = dbc->dbp;
	DB_PARTITION * part = (DB_PARTITION *)dbp->p_internal;
	ENV * env = dbp->env;
	DB_BTREE_STAT * fsp = NULL;
 #ifdef HAVE_HASH
	DB_HASH_STAT * hfsp = NULL;
 #endif
	DB ** pdbp = part->handles;
	for(i = 0; i < part->nparts; i++, pdbp++) {
		if((ret = __db_cursor_int(*pdbp, dbc->thread_info, dbc->txn, (*pdbp)->type, PGNO_INVALID, 0, dbc->locker, &new_dbc)) != 0)
			goto err;
		switch(new_dbc->dbtype) {
		    case DB_BTREE:
			if((ret = __bam_stat(new_dbc, &bsp, flags)) != 0)
				goto err;
			if(fsp == NULL) {
				fsp = bsp;
				*(DB_BTREE_STAT **)spp = fsp;
			}
			else {
				fsp->bt_nkeys += bsp->bt_nkeys;
				fsp->bt_ndata += bsp->bt_ndata;
				fsp->bt_pagecnt += bsp->bt_pagecnt;
				SETMAX(fsp->bt_levels, bsp->bt_levels);
				fsp->bt_int_pg += bsp->bt_int_pg;
				fsp->bt_leaf_pg += bsp->bt_leaf_pg;
				fsp->bt_dup_pg += bsp->bt_dup_pg;
				fsp->bt_over_pg += bsp->bt_over_pg;
				fsp->bt_free += bsp->bt_free;
				fsp->bt_int_pgfree += bsp->bt_int_pgfree;
				fsp->bt_leaf_pgfree += bsp->bt_leaf_pgfree;
				fsp->bt_dup_pgfree += bsp->bt_dup_pgfree;
				fsp->bt_over_pgfree += bsp->bt_over_pgfree;
				__os_ufree(env, bsp);
			}
			break;
 #ifdef HAVE_HASH
		    case DB_HASH:
			if((ret = __ham_stat(new_dbc, &hsp, flags)) != 0)
				goto err;
			if(hfsp == NULL) {
				hfsp = hsp;
				*(DB_HASH_STAT **)spp = hfsp;
			}
			else {
				hfsp->hash_nkeys += hsp->hash_nkeys;
				hfsp->hash_ndata += hsp->hash_ndata;
				hfsp->hash_pagecnt += hsp->hash_pagecnt;
				hfsp->hash_ffactor += hsp->hash_ffactor;
				hfsp->hash_buckets += hsp->hash_buckets;
				hfsp->hash_free += hsp->hash_free;
				hfsp->hash_bfree += hsp->hash_bfree;
				hfsp->hash_bigpages += hsp->hash_bigpages;
				hfsp->hash_big_bfree += hsp->hash_big_bfree;
				hfsp->hash_overflows += hsp->hash_overflows;
				hfsp->hash_ovfl_free += hsp->hash_ovfl_free;
				hfsp->hash_dup += hsp->hash_dup;
				hfsp->hash_dup_free += hsp->hash_dup_free;
				__os_ufree(env, hsp);
			}
			break;
 #endif
		    default:
			break;
		}
		if((ret = __dbc_close(new_dbc)) != 0)
			goto err;
	}
	return 0;
err:
	__os_ufree(env, fsp);
	*(DB_BTREE_STAT **)spp = NULL;
	return ret;
}
/*
 * __part_truncate --
 *	Truncate a database.
 */
int __part_truncate(DBC * dbc, uint32 * countp)
{
	DBC * new_dbc;
	uint32 count, i;
	int t_ret;
	DB * dbp = dbc->dbp;
	DB_PARTITION * part = (DB_PARTITION *)dbp->p_internal;
	DB ** pdbp = part->handles;
	int ret = 0;
	ASSIGN_PTR(countp, 0);
	for(i = 0; ret == 0 && i < part->nparts; i++, pdbp++) {
		if((ret = __db_cursor_int(*pdbp, dbc->thread_info, dbc->txn, (*pdbp)->type, PGNO_INVALID, 0, dbc->locker, &new_dbc)) != 0)
			break;
		switch(dbp->type) {
		    case DB_BTREE:
		    case DB_RECNO:
			ret = __bam_truncate(new_dbc, &count);
			break;
		    case DB_HASH:
 #ifdef HAVE_HASH
			ret = __ham_truncate(new_dbc, &count);
			break;
 #endif
		    case DB_QUEUE:
		    case DB_UNKNOWN:
		    default:
			ret = __db_unknown_type(dbp->env, "DB->truncate", dbp->type);
			count = 0;
			break;
		}
		if((t_ret = __dbc_close(new_dbc)) != 0 && ret == 0)
			ret = t_ret;
		if(countp)
			countp += count;
	}
	return ret;
}
/*
 * __part_compact -- compact a partitioned database.
 *
 * PUBLIC: int __part_compact __P((DB *, DB_THREAD_INFO *, DB_TXN *,
 * PUBLIC:     DBT *, DBT *, DB_COMPACT *, uint32, DBT *));
 */
int __part_compact(DB * dbp, DB_THREAD_INFO * ip, DB_TXN * txn, DBT * start, DBT * stop, DB_COMPACT * c_data, uint32 flags, DBT * end)
{
	DB_PARTITION * part = (DB_PARTITION *)dbp->p_internal;
	DB ** pdbp = part->handles;
	int ret = 0;
	for(uint32 i = 0; ret == 0 && i < part->nparts; i++, pdbp++) {
		switch(dbp->type) {
		    case DB_HASH:
		    case DB_BTREE:
		    case DB_RECNO:
				ret = __db_compact_int(*pdbp, ip, txn, start, stop, c_data, flags, end);
				break;
		    default:
				ret = __dbh_am_chk(dbp, DB_OK_BTREE);
				break;
		}
	}
	return ret;
}
/*
 * __part_lsn_reset --
 *	reset the lsns on each partition.
 *
 * PUBLIC: int __part_lsn_reset __P((DB *, DB_THREAD_INFO *));
 */
int __part_lsn_reset(DB * dbp, DB_THREAD_INFO * ip)
{
	DB_PARTITION * part = (DB_PARTITION *)dbp->p_internal;
	DB ** pdbp = part->handles;
	int ret = 0;
	for(uint32 i = 0; ret == 0 && i < part->nparts; i++, pdbp++)
		ret = __db_lsn_reset((*pdbp)->mpf, ip);
	return ret;
}
/*
 * __part_fileid_reset --
 *	reset the fileid on each partition.
 *
 * PUBLIC: int __part_fileid_reset
 * PUBLIC:	 __P((ENV *, DB_THREAD_INFO *, const char *, uint32, int));
 */
int __part_fileid_reset(ENV * env, DB_THREAD_INFO * ip, const char * fname, uint32 nparts, int encrypted)
{
	int ret;
	uint32 part_id;
	char * name, * sp;
	const char * np;
	if((ret = __os_malloc(env, sstrlen(fname)+PART_LEN+1, &name)) != 0) {
		__db_errx(env, Alloc_err, sstrlen(fname)+PART_LEN+1);
		return ret;
	}
	sp = name;
	np = __db_rpath(fname);
	if(!np)
		np = fname;
	else {
		np++;
		strncpy(name, fname, (size_t)(np-fname));
		sp = name+(np-fname);
	}
	for(part_id = 0; ret == 0 && part_id < nparts; part_id++) {
		sprintf(sp, PART_NAME, np, part_id);
		ret = __env_fileid_reset(env, ip, sp, encrypted);
	}
	__os_free(env, name);
	return ret;
}
/*
 * __part_key_range --
 *	Return proportion of keys relative to given key.
 *
 * PUBLIC: int __part_key_range __P((DBC *, DBT *, DB_KEY_RANGE *, uint32));
 */
int __part_key_range(DBC * dbc, DBT * dbt, DB_KEY_RANGE * kp, uint32 flags)
{
	BTREE_CURSOR * cp;
	DBC * new_dbc;
	DB_PARTITION * part;
	PAGE * h;
	uint32 id, part_id;
	uint32 elems, empty, less_elems, my_elems, greater_elems;
	uint32 levels, max_levels, my_levels;
	db_pgno_t root_pgno;
	int ret;
	double total_elems;
	COMPQUIET(flags, 0);
	part = (DB_PARTITION *)dbc->dbp->p_internal;
	/*
	 * First we find the key range for the partition that contains the
	 * key.  Then we scale based on estimates of the other partitions.
	 */
	if(F_ISSET(part, PART_CALLBACK))
		part_id = part->callback(dbc->dbp, dbt)%part->nparts;
	else
		__part_search(dbc->dbp, part, dbt, &part_id);
	GET_PART_CURSOR(dbc, new_dbc, part_id);
	if((ret = __bam_key_range(new_dbc, dbt, kp, flags)) != 0)
		goto err;
	cp = (BTREE_CURSOR *)new_dbc->internal;
	root_pgno = BAM_ROOT_PGNO(new_dbc);
	if((ret = __memp_fget(new_dbc->dbp->mpf, &root_pgno, new_dbc->thread_info, new_dbc->txn, 0, &h)) != 0)
		goto c_err;
	my_elems = NUM_ENT(h);
	my_levels = LEVEL(h);
	max_levels = my_levels;
	if((ret = __memp_fput(new_dbc->dbp->mpf, new_dbc->thread_info, h, new_dbc->priority)) != 0)
		goto c_err;
	if((ret = __dbc_close(new_dbc)) != 0)
		goto err;
	/*
	 * We have the range within one subtree.  Now estimate
	 * what part of the whole range that subtree is.  Figure
	 * out how many levels each part has and how many entries
	 * in the level below the root.
	 */
	empty = less_elems = greater_elems = 0;
	for(id = 0; id < part->nparts; id++) {
		if(id == part_id)
			empty = 0;
		else {
			GET_PART_CURSOR(dbc, new_dbc, id);
			cp = (BTREE_CURSOR *)new_dbc->internal;
			if((ret = __memp_fget(new_dbc->dbp->mpf, &cp->root, new_dbc->thread_info, new_dbc->txn, 0, &h)) != 0)
				goto c_err;
			elems = NUM_ENT(h);
			levels = LEVEL(h);
			if(levels == 1)
				elems /= 2;
			if((ret = __memp_fput(new_dbc->dbp->mpf, new_dbc->thread_info, h, new_dbc->priority)) != 0)
				goto c_err;
			if((ret = __dbc_close(new_dbc)) != 0)
				goto err;
			// If the tree is empty, ignore it. 
			if(elems == 0)
				empty++;
			else {
				// 
				// If a tree has fewer levels than the max just count
				// it as a single element in the higher level.
				// 
				if(id < part_id) {
					if(levels > max_levels) {
						max_levels = levels;
						less_elems = id+elems-empty;
					}
					else if(levels < max_levels)
						less_elems++;
					else
						less_elems += elems;
				}
				else {
					if(levels > max_levels) {
						max_levels = levels;
						greater_elems = (id-part_id)+elems-empty;
					}
					else if(levels < max_levels)
						greater_elems++;
					else
						greater_elems += elems;
				}
			}
		}
	}
	if(my_levels < max_levels) {
		/*
		 * The subtree containing the key is not the tallest one.
		 * Reduce its share by the number of records at the highest
		 * level.  Scale the greater and lesser components up
		 * by  the number of records on either side of this
		 * subtree.
		 */
		total_elems = 1+greater_elems+less_elems;
		kp->equal /= total_elems;
		kp->less /= total_elems;
		kp->less += less_elems/total_elems;
		kp->greater /= total_elems;
		kp->greater += greater_elems/total_elems;
	}
	else if(my_levels == max_levels) {
		/*
		 * The key is in one of the tallest subtrees.  We will
		 * scale the values by the ratio of the records at the
		 * top of this stubtree to the number of records at the
		 * highest level.
		 */
		total_elems = greater_elems+less_elems;
		if(total_elems != 0) {
			/*
			 * First scale down by the fraction of elements
			 * in this subtree.
			 */
			total_elems += my_elems;
			kp->equal *= my_elems;
			kp->equal /= total_elems;
			kp->less *= my_elems;
			kp->less /= total_elems;
			kp->greater *= my_elems;
			kp->greater /= total_elems;
			/*
			 * Proportially add weight from the subtrees to the
			 * left and right of this one.
			 */
			kp->less += less_elems/total_elems;
			kp->greater += greater_elems/total_elems;
		}
	}
	if(0) {
c_err:
		__dbc_close(new_dbc);
	}
err:
	return ret;
}
/*
 * __part_remove --
 *	Remove method for a partitioned database.
 *
 * PUBLIC: int __part_remove __P((DB *, DB_THREAD_INFO *,
 * PUBLIC:      DB_TXN *, const char *, const char *, uint32));
 */
int __part_remove(DB * dbp, DB_THREAD_INFO * ip, DB_TXN * txn, const char * name, const char * subdb, uint32 flags)
{
	return __part_rr(dbp, ip, txn, name, subdb, NULL, flags);
}
/*
 * __part_rename --
 *	Rename method for a partitioned database.
 *
 * PUBLIC: int __part_rename __P((DB *, DB_THREAD_INFO *,
 * PUBLIC:         DB_TXN *, const char *, const char *, const char *));
 */
int __part_rename(DB * dbp, DB_THREAD_INFO * ip, DB_TXN * txn, const char * name, const char * subdb, const char * newname)
{
	return __part_rr(dbp, ip, txn, name, subdb, newname, 0);
}
/*
 * __part_rr --
 *	Remove/Rename method for a partitioned database.
 */
static int __part_rr(DB * dbp, DB_THREAD_INFO * ip, DB_TXN * txn, const char * name, const char * subdb, const char * newname, uint32 flags)
{
	DB ** pdbp, * ptmpdbp, * tmpdbp;
	DB_PARTITION * part;
	uint32 i;
	int t_ret;
	char * np;
	ENV * env = dbp->env;
	int ret = 0;
	if(subdb && name) {
		__db_errx(env, DB_STR("0663", "A partitioned database can not be in a multiple databases file"));
		return EINVAL;
	}
	ENV_GET_THREAD_INFO(env, ip);
	/*
	 * Since rename no longer opens the database, we have
	 * to do it here.
	 */
	if((ret = __db_create_internal(&tmpdbp, env, 0)) != 0)
		return ret;
	/*
	 * We need to make sure we don't self-deadlock, so give
	 * this dbp the same locker as the incoming one.
	 */
	tmpdbp->locker = dbp->locker;
	if((ret = __db_open(tmpdbp, ip, txn, name, NULL, dbp->type, DB_RDWRMASTER|DB_RDONLY, 0, PGNO_BASE_MD)) != 0)
		goto err;
	part = (DB_PARTITION *)tmpdbp->p_internal;
	pdbp = part->handles;
	COMPQUIET(np, 0);
	if(newname && (ret = __os_malloc(env, sstrlen(newname)+PART_LEN+1, &np)) != 0) {
		__db_errx(env, Alloc_err, sstrlen(newname)+PART_LEN+1);
		goto err;
	}
	for(i = 0; i < part->nparts; i++, pdbp++) {
		if((ret = __db_create_internal(&ptmpdbp, env, 0)) != 0)
			break;
		ptmpdbp->locker = (*pdbp)->locker;
		if(newname == NULL)
			ret = __db_remove_int(ptmpdbp, ip, txn, (*pdbp)->fname, NULL, flags);
		else {
			DB_ASSERT(env, np != NULL);
			sprintf(np, PART_NAME, newname, i);
			ret = __db_rename_int(ptmpdbp, ip, txn, (*pdbp)->fname, NULL, np, flags);
		}
		ptmpdbp->locker = NULL;
		__db_close(ptmpdbp, NULL, DB_NOSYNC);
		if(ret)
			break;
	}
	if(newname)
		__os_free(env, np);
	if(!F_ISSET(dbp, DB_AM_OPEN_CALLED)) {
err:            /*
		 * Since we copied the locker ID from the dbp, we'd better not
		 * free it here.
		 */
		tmpdbp->locker = NULL;
		/* We need to remove the lock event we associated with this. */
		if(txn)
			__txn_remlock(env, txn, &tmpdbp->handle_lock, DB_LOCK_INVALIDID);
		if((t_ret = __db_close(tmpdbp, txn, DB_NOSYNC)) != 0 && ret == 0)
			ret = t_ret;
	}
	return ret;
}
 #ifdef HAVE_VERIFY
/*
 * __part_verify --
 *	Verify a partitioned database.
 *
 * PUBLIC: int __part_verify __P((DB *, VRFY_DBINFO *, const char *,
 * PUBLIC:     void *, int (*)(void *, const void *), uint32));
 */
int __part_verify(DB * dbp, VRFY_DBINFO * vdp, const char * fname, void * handle, int (*callback)__P((void *, const void *)), uint32 flags)
{
	DB ** pdbp;
	DB_PARTITION * part;
	DBT * key;
	uint32 i;
	int ret, t_ret;
	ENV * env = dbp->env;
	BINTERNAL * lp = 0;
	BINTERNAL * rp = 0;
	DBC * dbc = NULL;
	DB_THREAD_INFO * ip = vdp->thread_info;
	if(dbp->type == DB_BTREE) {
		if((ret = __bam_open(dbp, ip, NULL, fname, PGNO_BASE_MD, flags)) != 0)
			goto err;
	}
  #ifdef HAVE_HASH
	else if((ret = __ham_open(dbp, ip, NULL, fname, PGNO_BASE_MD, flags)) != 0)
		goto err;
  #endif
	/*
	 * Initalize partition db handles and get the names. Set DB_RDWRMASTER
	 * because we may not have the partition callback, but we can still
	 * look at the structure of the tree.
	 */
	if((ret = __partition_open(dbp, ip, NULL, fname, dbp->type, flags|DB_RDWRMASTER, 0, 0)) != 0)
		goto err;
	part = (DB_PARTITION *)dbp->p_internal;
	if(LF_ISSET(DB_SALVAGE)) {
		/* If we are being aggressive we don't want to dump the keys. */
		if(LF_ISSET(DB_AGGRESSIVE))
			dbp->p_internal = NULL;
		ret = __db_prheader(dbp, NULL, 0, 0, handle, callback, vdp, PGNO_BASE_MD);
		dbp->p_internal = part;
		if(ret)
			goto err;
	}
	if((ret = __db_cursor(dbp, ip, NULL, &dbc, 0)) != 0)
		goto err;
	pdbp = part->handles;
	for(i = 0; i < part->nparts; i++, pdbp++) {
		if(!F_ISSET(part, PART_RANGE) || part->keys == NULL)
			goto vrfy;
		__os_free(env, lp);
		lp = rp;
		rp = NULL;
		if(i+1 <  part->nparts) {
			key = &part->keys[i+1];
			if((ret = __os_malloc(env, BINTERNAL_SIZE(key->size), &rp)) != 0)
				goto err;
			rp->len = key->size;
			memcpy(rp->data, key->data, key->size);
			B_TSET(rp->type, B_KEYDATA);
		}
vrfy:
		if((t_ret = __db_verify(*pdbp, ip, (*pdbp)->fname, NULL, handle, callback, lp, rp, flags|DB_VERIFY_PARTITION)) != 0 && ret == 0)
			ret = t_ret;
	}
err:
	__os_free(env, lp);
	__os_free(env, rp);
	return ret;
}
 #endif

 #ifdef CONFIG_TEST
/*
 * __part_testdocopy -- copy all partitions for testing purposes.
 *
 * PUBLIC: int __part_testdocopy __P((DB *, const char *));
 */
int __part_testdocopy(DB * dbp, const char * name)
{
	DB ** pdbp;
	DB_PARTITION * part;
	uint32 i;
	int ret;
	if((ret = __db_testdocopy(dbp->env, name)) != 0)
		return ret;
	part = dbp->p_internal;
	pdbp = part->handles;
	for(i = 0; i < part->nparts; i++, pdbp++)
		if((ret = __db_testdocopy(dbp->env, (*pdbp)->fname)) != 0)
			return ret;
	return 0;
}
 #endif
#else
/*
 * __db_nopartition --
 *	Error when a Berkeley DB build doesn't include partitioning.
 *
 * PUBLIC: int __db_no_partition(ENV *);
 */
int __db_no_partition(ENV * env)
{
	__db_errx(env, DB_STR("0664", "library build did not include support for the database partitioning"));
	return DB_OPNOTSUP;
}
/*
 * __partition_set --
 *	Set the partitioning keys or callback function.
 * This routine must be called prior to creating the database.
 * PUBLIC: int __partition_set __P((DB *, uint32, DBT *,
 * PUBLIC:	uint32 (*callback)(DB *, DBT *key)));
 */
int __partition_set(DB * dbp, uint32 parts, DBT * keys, uint32 (* callback)(DB *, DBT * key))
{
	COMPQUIET(parts, 0);
	COMPQUIET(keys, 0);
	COMPQUIET(callback, 0);
	return __db_no_partition(dbp->env);
}
/*
 * __partition_get_callback --
 *	Set the partition callback function.  This routine must be called
 * prior to opening a partition database that requires a function.
 * PUBLIC: int __partition_get_callback __P((DB *,
 * PUBLIC:	 uint32 *, uint32 (**callback)(DB *, DBT *key)));
 */
int __partition_get_callback(DB * dbp, uint32 * parts, uint32 (**callback)(DB*, DBT*key))
{
	COMPQUIET(parts, 0);
	COMPQUIET(callback, 0);
	return __db_no_partition(dbp->env);
}
/*
 * __partition_get_dirs --
 *	Get partition dirs.
 * PUBLIC: int __partition_get_dirs __P((DB *, const char ***));
 */
int __partition_get_dirs(DB * dbp, const char *** dirpp)
{
	COMPQUIET(dirpp, 0);
	return __db_no_partition(dbp->env);
}
/*
 * __partition_get_keys --
 *	Get partition keys.
 * PUBLIC: int __partition_get_keys __P((DB *, uint32 *, DBT **));
 */
int __partition_get_keys(DB * dbp, uint32 * parts, DBT ** keys)
{
	COMPQUIET(parts, 0);
	COMPQUIET(keys, 0);
	return __db_no_partition(dbp->env);
}
/*
 * __partition_init --
 *	Initialize the partition structure.
 * Called when the meta data page is read in during database open or
 * when partition keys or a callback are set.
 *
 * PUBLIC: int __partition_init __P((DB *, uint32));
 */
int __partition_init(DB * dbp, uint32 flags)
{
	COMPQUIET(flags, 0);
	return __db_no_partition(dbp->env);
}
/*
 * __part_fileid_reset --
 *	reset the fileid on each partition.
 *
 * PUBLIC: int __part_fileid_reset
 * PUBLIC:	 __P((ENV *, DB_THREAD_INFO *, const char *, uint32, int));
 */
int __part_fileid_reset(ENV * env, DB_THREAD_INFO * ip, const char * fname, uint32 nparts, int encrypted)
{
	COMPQUIET(ip, 0);
	COMPQUIET(fname, 0);
	COMPQUIET(nparts, 0);
	COMPQUIET(encrypted, 0);
	return __db_no_partition(env);
}
/*
 * __partition_set_dirs --
 *	Set the directories for creating the partition databases.
 * They must be in the environment.
 * PUBLIC: int __partition_set_dirs __P((DB *, const char **));
 */
int __partition_set_dirs(DB * dbp, const char ** dirp)
{
	COMPQUIET(dirp, 0);
	return __db_no_partition(dbp->env);
}
#endif
