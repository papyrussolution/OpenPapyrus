/*-
 * See the file LICENSE for redistribution information.
 * Copyright (c) 1999, 2011 Oracle and/or its affiliates.  All rights reserved.
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop

#ifdef HAVE_STATISTICS
/*
 * __qam_stat --
 *	Gather/print the qam statistics
 *
 * PUBLIC: int __qam_stat __P((DBC *, void *, uint32));
 */
int __qam_stat(DBC*dbc, void * spp, uint32 flags)
{
	DB_MPOOLFILE * mpf;
	DB_QUEUE_STAT * sp;
	PAGE * h;
	QAMDATA * qp, * ep;
	QMETA * meta;
	QUEUE * t;
	db_indx_t indx;
	db_pgno_t first, last, pgno, pg_ext, stop;
	uint32 re_len;
	int ret, t_ret;
	DB * dbp = dbc->dbp;
	DB_LOCK lock;
	LOCK_INIT(lock);
	mpf = dbp->mpf;
	sp = NULL;
	t = (QUEUE *)dbp->q_internal;
	if(spp == NULL)
		return 0;
	/* Allocate and clear the structure. */
	if((ret = __os_umalloc(dbp->env, sizeof(*sp), &sp)) != 0)
		goto err;
	memzero(sp, sizeof(*sp));
	re_len = ((QUEUE *)dbp->q_internal)->re_len;
	/* Determine the last page of the database. */
	if((ret = __db_lget(dbc, 0, t->q_meta, DB_LOCK_READ, 0, &lock)) != 0)
		goto err;
	if((ret = __memp_fget(mpf, &t->q_meta, dbc->thread_info, dbc->txn, 0, &meta)) != 0)
		goto err;
	if(flags == DB_FAST_STAT) {
		sp->qs_nkeys = meta->dbmeta.key_count;
		sp->qs_ndata = meta->dbmeta.record_count;
		goto meta_only;
	}
	first = QAM_RECNO_PAGE(dbp, meta->first_recno);
	last = QAM_RECNO_PAGE(dbp, meta->cur_recno);

	ret = __memp_fput(mpf, dbc->thread_info, meta, dbc->priority);
	if((t_ret = __LPUT(dbc, lock)) != 0 && ret == 0)
		ret = t_ret;
	if(ret)
		goto err;
	pgno = first;
	if(first > last)
		stop = QAM_RECNO_PAGE(dbp, UINT32_MAX);
	else
		stop = last;
	/* Dump each page. */
	pg_ext = ((QUEUE *)dbp->q_internal)->page_ext;
begin:
	/* Walk through the pages and count. */
	for(; pgno <= stop; ++pgno) {
		if((ret =
		            __db_lget(dbc, 0, pgno, DB_LOCK_READ, 0, &lock)) != 0)
			goto err;
		ret = __qam_fget(dbc, &pgno, 0, &h);
		if(ret == ENOENT) {
			pgno += pg_ext-1;
			continue;
		}
		if(ret == DB_PAGE_NOTFOUND) {
			if(pg_ext == 0) {
				if(pgno != stop && first != last)
					goto err;
				ret = 0;
				break;
			}
			pgno += (pg_ext-((pgno-1)%pg_ext))-1;
			continue;
		}
		if(ret)
			goto err;
		++sp->qs_pages;

		ep = (QAMDATA *)((uint8 *)h+dbp->pgsize-re_len);
		for(indx = 0, qp = QAM_GET_RECORD(dbp, h, indx);
		    qp <= ep;
		    ++indx,  qp = QAM_GET_RECORD(dbp, h, indx)) {
			if(F_ISSET(qp, QAM_VALID))
				sp->qs_ndata++;
			else
				sp->qs_pgfree += re_len;
		}
		ret = __qam_fput(dbc, pgno, h, dbc->priority);
		if((t_ret = __LPUT(dbc, lock)) != 0 && ret == 0)
			ret = t_ret;
		if(ret)
			goto err;
	}
	if((ret = __LPUT(dbc, lock)) != 0)
		goto err;
	if(first > last) {
		pgno = 1;
		stop = last;
		first = last;
		goto begin;
	}
	// Get the meta-data page
	if((ret = __db_lget(dbc, 0, t->q_meta, F_ISSET(dbp, DB_AM_RDONLY) ? DB_LOCK_READ : DB_LOCK_WRITE, 0, &lock)) != 0)
		goto err;
	if((ret = __memp_fget(mpf, &t->q_meta, dbc->thread_info, dbc->txn, F_ISSET(dbp, DB_AM_RDONLY) ? 0 : DB_MPOOL_DIRTY, &meta)) != 0)
		goto err;
	if(!F_ISSET(dbp, DB_AM_RDONLY))
		meta->dbmeta.key_count = meta->dbmeta.record_count = sp->qs_ndata;
	sp->qs_nkeys = sp->qs_ndata;
meta_only:
	// Get the metadata fields
	sp->qs_magic = meta->dbmeta.magic;
	sp->qs_version = meta->dbmeta.version;
	sp->qs_metaflags = meta->dbmeta.flags;
	sp->qs_pagesize = meta->dbmeta.pagesize;
	sp->qs_extentsize = meta->page_ext;
	sp->qs_re_len = meta->re_len;
	sp->qs_re_pad = meta->re_pad;
	sp->qs_first_recno = meta->first_recno;
	sp->qs_cur_recno = meta->cur_recno;
	// Discard the meta-data page
	ret = __memp_fput(mpf, dbc->thread_info, meta, dbc->priority);
	if((t_ret = __LPUT(dbc, lock)) != 0 && ret == 0)
		ret = t_ret;
	if(ret)
		goto err;
	*(DB_QUEUE_STAT **)spp = sp;
	if(0) {
err:            
		__os_ufree(dbp->env, sp);
	}
	if((t_ret = __LPUT(dbc, lock)) != 0 && ret == 0)
		ret = t_ret;
	return ret;
}

/*
 * __qam_stat_print --
 *	Display queue statistics.
 *
 * PUBLIC: int __qam_stat_print __P((DBC *, uint32));
 */
int __qam_stat_print(DBC*dbc, uint32 flags)
{
	DB_QUEUE_STAT * sp;
	int ret;
	DB * dbp = dbc->dbp;
	ENV * env = dbp->env;
	if((ret = __qam_stat(dbc, &sp, LF_ISSET(DB_FAST_STAT))) != 0)
		return ret;
	if(LF_ISSET(DB_STAT_ALL)) {
		__db_msg_db_line(env);
		__db_msg(env, "Default Queue database information:");
	}
	__db_msg(env, "%lx\tQueue magic number", (ulong)sp->qs_magic);
	__db_msg(env, "%lu\tQueue version number", (ulong)sp->qs_version);
	__db_dl(env, "Fixed-length record size", (ulong)sp->qs_re_len);
	__db_msg(env, "%#x\tFixed-length record pad", (int)sp->qs_re_pad);
	__db_dl(env, "Underlying database page size", (ulong)sp->qs_pagesize);
	__db_dl(env, "Underlying database extent size", (ulong)sp->qs_extentsize);
	__db_dl(env, "Number of records in the database", (ulong)sp->qs_nkeys);
	__db_dl(env, "Number of data items in the database", (ulong)sp->qs_ndata);
	__db_dl(env, "Number of database pages", (ulong)sp->qs_pages);
	__db_dl_pct(env, "Number of bytes free in database pages", (ulong)sp->qs_pgfree, DB_PCT_PG(sp->qs_pgfree, sp->qs_pages, sp->qs_pagesize), "ff");
	__db_msg(env, "%lu\tFirst undeleted record", (ulong)sp->qs_first_recno);
	__db_msg(env, "%lu\tNext available record number", (ulong)sp->qs_cur_recno);
	__os_ufree(env, sp);
	return 0;
}

#else /* !HAVE_STATISTICS */

int __qam_stat(DBC*dbc, void * spp, uint32 flags)
{
	COMPQUIET(spp, 0);
	COMPQUIET(flags, 0);
	return __db_stat_not_built(dbc->env);
}
#endif
