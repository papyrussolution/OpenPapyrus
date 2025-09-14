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
/*
 * __bam_pgin --
 *	Convert host-specific page layout from the host-independent format stored on disk.
 */
int __bam_pgin(DB * dbp, db_pgno_t pg, void * pp, DBT * cookie)
{
	PAGE * h;
	DB_PGINFO * pginfo = (DB_PGINFO *)cookie->data;
	if(!F_ISSET(pginfo, DB_AM_SWAP))
		return 0;
	h = (PAGE *)pp;
	return (TYPE(h) == P_BTREEMETA) ?  __bam_mswap(dbp->env, (PAGE *)pp) : __db_byteswap(dbp, pg, (PAGE *)pp, pginfo->db_pagesize, 1);
}
/*
 * __bam_pgout --
 *	Convert host-specific page layout to the host-independent format
 *	stored on disk.
 */
int __bam_pgout(DB * dbp, db_pgno_t pg, void * pp, DBT * cookie)
{
	PAGE * h;
	DB_PGINFO * pginfo = (DB_PGINFO *)cookie->data;
	if(!F_ISSET(pginfo, DB_AM_SWAP))
		return 0;
	h = (PAGE *)pp;
	return (TYPE(h) == P_BTREEMETA) ?  __bam_mswap(dbp->env, (PAGE *)pp) : __db_byteswap(dbp, pg, (PAGE *)pp, pginfo->db_pagesize, 0);
}
/*
 * __bam_mswap --
 *	Swap the bytes on the btree metadata page.
 */
int __bam_mswap(ENV * env, PAGE * pg)
{
	uint8 * p;
	COMPQUIET(env, 0);
	__db_metaswap(pg);
	p = (uint8 *)pg+sizeof(DBMETA);
	p += sizeof(uint32); /*unused*/
	SWAP32(p); /* minkey */
	SWAP32(p); /* re_len */
	SWAP32(p); /* re_pad */
	SWAP32(p); /* root */
	p += 92*sizeof(uint32); /*unused*/
	SWAP32(p); /* crypto_magic */
	return 0;
}

