/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2011 Oracle and/or its affiliates.  All rights reserved.
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop

static int __db_quicksort __P((DB*, DBT*, DBT*, uint32 *, uint32 *, uint32 *, uint32 *, uint32));
/*
 * __db_compare_both --
 *	Use the comparison functions from db to compare akey and bkey, and if
 *	DB_DUPSORT adata and bdata.
 */
int __db_compare_both(DB * db, const DBT * akey, const DBT * adata, const DBT * bkey, const DBT * bdata)
{
	BTREE * t = static_cast<BTREE *>(db->bt_internal);
	int cmp = t->bt_compare(db, akey, bkey);
	if(cmp != 0) return cmp;
	if(!F_ISSET(db, DB_AM_DUPSORT))
		return 0;
	if(adata == 0) return bdata == 0 ? 0 : -1;
	if(bdata == 0) return 1;
#ifdef HAVE_COMPRESSION
	if(DB_IS_COMPRESSED(db))
		return t->compress_dup_compare(db, adata, bdata);
#endif
	return db->dup_compare(db, adata, bdata);
}

#define DB_SORT_SWAP(a, ad, b, bd)                                      \
        do {                                                                    \
		tmp = (a)[0]; (a)[0] = (b)[0]; (b)[0] = tmp;                    \
		tmp = (a)[-1]; (a)[-1] = (b)[-1]; (b)[-1] = tmp;                \
		if(data != NULL) {                                             \
			tmp = (ad)[0]; (ad)[0] = (bd)[0]; (bd)[0] = tmp;        \
			tmp = (ad)[-1]; (ad)[-1] = (bd)[-1]; (bd)[-1] = tmp;    \
		}                                                               \
	} while(0)

#define DB_SORT_LOAD_DBT(a, ad, aptr, adptr)                            \
        do {                                                                    \
		(a).data = (uint8 *)key->data+(aptr)[0];                    \
		(a).size = (aptr)[-1];                                          \
		if(data != NULL) {                                             \
			(ad).data = (uint8 *)data->data+(adptr)[0];         \
			(ad).size = (adptr)[-1];                                \
		}                                                               \
	} while(0)

#define DB_SORT_COMPARE(a, ad, b, bd) (data != NULL ?                   \
                                       __db_compare_both(db, &(a), &(ad), &(b), &(bd)) :               \
                                       __db_compare_both(db, &(a), 0, &(b), 0))

#define DB_SORT_STACKSIZE 32

/*
 * __db_quicksort --
 *	The quicksort implementation for __db_sort_multiple() and
 *	__db_sort_multiple_key().
 */
static int __db_quicksort(DB * db, DBT * key, DBT * data, uint32 * kstart, uint32 * kend, uint32 * dstart, uint32 * dend, uint32 size)
{
	int ret, cmp;
	uint32 tmp, len;
	uint32 * kptr, * dptr, * kl, * dl, * kr, * dr;
	DBT a, ad, b, bd, m, md;
	ENV * env;

	struct DB_SORT_quicksort_stack {
		uint32 * kstart;
		uint32 * kend;
		uint32 * dstart;
		uint32 * dend;
	} stackbuf[DB_SORT_STACKSIZE], * stack;
	uint32 soff, slen;
	ret = 0;
	env = db->env;
	// (replaced by ctr) memzero(&a, sizeof(DBT));
	// (replaced by ctr) memzero(&ad, sizeof(DBT));
	// (replaced by ctr) memzero(&b, sizeof(DBT));
	// (replaced by ctr) memzero(&bd, sizeof(DBT));
	// (replaced by ctr) memzero(&m, sizeof(DBT));
	// (replaced by ctr) memzero(&md, sizeof(DBT));
	// NB end is smaller than start 
	stack = stackbuf;
	soff = 0;
	slen = DB_SORT_STACKSIZE;

start:
	if(kend >= kstart) goto pop;
	/* If there's only one value, it's already sorted */
	len = (uint32)(kstart-kend)/size;
	if(len == 1) goto pop;
	DB_SORT_LOAD_DBT(a, ad, kstart, dstart);
	DB_SORT_LOAD_DBT(b, bd, kend+size, dend+size);
	if(len == 2) {
		/* Special case the sorting of two value sequences */
		if(DB_SORT_COMPARE(a, ad, b, bd) > 0) {
			DB_SORT_SWAP(kstart, dstart, kend+size,
				dend+size);
		}
		goto pop;
	}
	kptr = kstart-(len/2)*size;
	dptr = dstart-(len/2)*size;
	DB_SORT_LOAD_DBT(m, md, kptr, dptr);
	/* Find the median of three */
	if(DB_SORT_COMPARE(a, ad, b, bd) < 0) {
		if(DB_SORT_COMPARE(m, md, a, ad) < 0) {
			/* m < a < b */
			if(len == 3) {
				DB_SORT_SWAP(kstart, dstart, kptr, dptr);
				goto pop;
			}
			DB_SORT_SWAP(kstart, dstart, kend+size, dend+size);
		}
		else if(DB_SORT_COMPARE(m, md, b, bd) < 0) {
			/* a <= m < b */
			if(len == 3) {
				goto pop;
			}
			DB_SORT_SWAP(kptr, dptr, kend+size, dend+size);
		}
		else {
			/* a < b <= m */
			if(len == 3) {
				DB_SORT_SWAP(kptr, dptr, kend+size,
					dend+size);
				goto pop;
			}
			/* Do nothing */
		}
	}
	else {
		if(DB_SORT_COMPARE(a, ad, m, md) < 0) {
			/* b <= a < m */
			DB_SORT_SWAP(kstart, dstart, kend+size, dend+size);
			if(len == 3) {
				DB_SORT_SWAP(kptr, dptr, kend+size, dend+size);
				goto pop;
			}
		}
		else if(DB_SORT_COMPARE(b, bd, m, md) < 0) {
			/* b < m <= a */
			if(len == 3) {
				DB_SORT_SWAP(kstart, dstart, kend+size, dend+size);
				goto pop;
			}
			DB_SORT_SWAP(kptr, dptr, kend+size, dend+size);
		}
		else {
			/* m <= b <= a */
			if(len == 3) {
				DB_SORT_SWAP(kstart, dstart, kptr, dptr);
				DB_SORT_SWAP(kptr, dptr, kend+size, dend+size);
				goto pop;
			}
			/* Do nothing */
		}
	}
	/* partition */
	DB_SORT_LOAD_DBT(b, bd, kend+size, dend+size);
	kl = kstart;
	dl = dstart;
	kr = kend+size;
	dr = dend+size;
	kptr = kstart;
	dptr = dstart;
	while(kptr >= kr) {
		DB_SORT_LOAD_DBT(a, ad, kptr, dptr);
		cmp = DB_SORT_COMPARE(a, ad, b, bd);
		if(cmp < 0) {
			DB_SORT_SWAP(kl, dl, kptr, dptr);
			kl -= size;
			dl -= size;
			kptr -= size;
			dptr -= size;
		}
		else if(cmp > 0) {
			DB_SORT_SWAP(kr, dr, kptr, dptr);
			kr += size;
			dr += size;
		}
		else {
			kptr -= size;
			dptr -= size;
		}
	}
	if(soff == slen) {
		/* Grow the stack */
		slen = slen*2;
		if(stack == stackbuf) {
			ret = __os_malloc(env, slen*
				sizeof(struct DB_SORT_quicksort_stack), &stack);
			if(ret) goto error;
			memcpy(stack, stackbuf, soff*
				sizeof(struct DB_SORT_quicksort_stack));
		}
		else {
			ret = __os_realloc(env, slen*
				sizeof(struct DB_SORT_quicksort_stack), &stack);
			if(ret) goto error;
		}
	}
	/* divide and conquer */
	stack[soff].kstart = kr-size;
	stack[soff].kend = kend;
	stack[soff].dstart = dr-size;
	stack[soff].dend = dend;
	++soff;

	kend = kl;
	dend = dl;

	goto start;

pop:
	if(soff != 0) {
		--soff;
		kstart = stack[soff].kstart;
		kend = stack[soff].kend;
		dstart = stack[soff].dstart;
		dend = stack[soff].dend;
		goto start;
	}
error:
	if(stack != stackbuf)
		__os_free(env, stack);
	return ret;
}

#undef DB_SORT_SWAP
#undef DB_SORT_LOAD_DBT
/*
 * __db_sort_multiple --
 *	If flags == DB_MULTIPLE_KEY, sorts a DB_MULTIPLE_KEY format DBT using
 *	the BTree comparison function and duplicate comparison function.
 *
 *	If flags == DB_MULTIPLE, sorts one or two DB_MULTIPLE format DBTs using
 *	the BTree comparison function and duplicate comparison function. Will
 *	assume key and data specifies pairs of key/data to sort together. If
 *	data is NULL, will just sort key according to the btree comparison
 *	function.
 *
 *	Uses an in-place quicksort algorithm, with median of three for the pivot
 *	point.
 */
int __db_sort_multiple(DB * db, DBT * key, DBT * data, uint32 flags)
{
	uint32 * kend, * dstart, * dend;
	/* @todo sanity checks on the DBTs */
	/* DB_ILLEGAL_METHOD(db, DB_OK_BTREE); */
	uint32 * kstart = (uint32 *)((uint8 *)key->data+key->ulen)-1;
	switch(flags) {
	    case DB_MULTIPLE:
		if(data)
			dstart = (uint32 *)((uint8 *)data->data+data->ulen)-1;
		else
			dstart = kstart;
		// Find the end 
		for(kend = kstart, dend = dstart; *kend != static_cast<uint32>(-1) && *dend != static_cast<uint32>(-1); kend -= 2, dend -= 2)
			;
		return __db_quicksort(db, key, data, kstart, kend, dstart, dend, 2);
	    case DB_MULTIPLE_KEY:
		/* Find the end */
		for(kend = kstart; *kend != static_cast<uint32>(-1); kend -= 4)
			;
		return __db_quicksort(db, key, key, kstart, kend, kstart-2, kend-2, 4);
	    default:
		return __db_ferr(db->env, "DB->sort_multiple", 0);
	}
}
