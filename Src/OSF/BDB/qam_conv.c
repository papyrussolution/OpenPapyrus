/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999, 2011 Oracle and/or its affiliates.  All rights reserved.
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
#include "dbinc/db_swap.h"
#include "dbinc/db_am.h"
#include "dbinc/qam.h"
/*
 * __qam_mswap --
 *	Swap the bytes on the queue metadata page.
 *
 * PUBLIC: int __qam_mswap __P((ENV *, PAGE *));
 */
int __qam_mswap(ENV*env, PAGE * pg)
{
	uint8 * p;
	COMPQUIET(env, NULL);
	__db_metaswap(pg);
	p = (uint8 *)pg+sizeof(DBMETA);
	SWAP32(p);              /* first_recno */
	SWAP32(p);              /* cur_recno */
	SWAP32(p);              /* re_len */
	SWAP32(p);              /* re_pad */
	SWAP32(p);              /* rec_page */
	SWAP32(p);              /* page_ext */
	p += 91*sizeof(uint32);   /* unused */
	SWAP32(p);              /* crypto_magic */
	return 0;
}
/*
 * __qam_pgin_out --
 *	Convert host-specific page layout to/from the host-independent format
 *	stored on disk.
 *  We only need to fix up a few fields in the header
 *
 * PUBLIC: int __qam_pgin_out __P((ENV *, db_pgno_t, void *, DBT *));
 */
int __qam_pgin_out(ENV*env, db_pgno_t pg, void * pp, DBT * cookie)
{
	DB_PGINFO * pginfo;
	QPAGE * h;
	COMPQUIET(pg, 0);
	pginfo = (DB_PGINFO *)cookie->data;
	if(!F_ISSET(pginfo, DB_AM_SWAP))
		return 0;
	h = (QPAGE *)pp;
	if(h->type == P_QAMMETA)
		return __qam_mswap(env, (PAGE *)pp);
	M_32_SWAP(h->lsn.file);
	M_32_SWAP(h->lsn.offset);
	M_32_SWAP(h->pgno);
	return 0;
}
