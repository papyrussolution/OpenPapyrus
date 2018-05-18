/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2017 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop
#include "dbinc/txn.h"
#include "dbinc_auto/xa_ext.h"
/*
 * This file contains all the mapping information that we need to support
 * the DB/XA interface.
 */
/*
 * __db_rmid_to_env
 *	Return the environment associated with a given XA rmid.
 *
 * PUBLIC: int __db_rmid_to_env __P((int, ENV **));
 */
int __db_rmid_to_env(int rmid, ENV ** envp)
{
	ENV * env;
	*envp = NULL;
	if(TAILQ_EMPTY(&DB_GLOBAL(envq)))
		TAILQ_INIT(&DB_GLOBAL(envq));
	/*
	 * When we map an rmid, move that environment to be the first one in
	 * the list of environments, so we acquire the correct environment
	 * in DB->open.
	 */
	for(env = TAILQ_FIRST(&DB_GLOBAL(envq));
	    env != NULL; env = TAILQ_NEXT(env, links)) {
		if(env->xa_rmid == rmid) {
			*envp = env;
			if(env != TAILQ_FIRST(&DB_GLOBAL(envq))) {
				TAILQ_REMOVE(&DB_GLOBAL(envq), env, links);
				TAILQ_INSERT_HEAD(&DB_GLOBAL(envq), env, links);
			}
			return (0);
		}
	}
	return (1);
}
/*
 * __db_xid_to_txn
 *	Return the txn that corresponds to this XID.
 *
 * PUBLIC: int __db_xid_to_txn __P((ENV *, XID *, TXN_DETAIL **));
 */
int __db_xid_to_txn(ENV *env, XID * xid, TXN_DETAIL ** tdp)
{
	DB_TXNMGR * mgr;
	DB_TXNREGION * region;
	uint8 * gid;
	mgr = env->tx_handle;
	region = (DB_TXNREGION *)mgr->reginfo.primary;
	/*
	 * Search the internal active transaction table to find the
	 * matching xid.  If this is a performance hit, then we
	 * can create a hash table, but I doubt it's worth it.
	 */
	TXN_SYSTEM_LOCK(env);
	gid = (uint8*)(xid->data);
	SH_TAILQ_FOREACH(*tdp, &region->active_txn, links, __txn_detail)
	if(memcmp(gid, (*tdp)->gid, sizeof((*tdp)->gid)) == 0)
		break;
	TXN_SYSTEM_UNLOCK(env);

	/*
	 * This returns an error, because TXN_SYSTEM_{UN}LOCK may return
	 * an error.
	 */
	return (0);
}

/*
 * __db_map_rmid
 *	Create a mapping between the specified rmid and environment.
 *
 * PUBLIC: void __db_map_rmid __P((int, ENV *));
 */
void __db_map_rmid(int rmid, ENV * env)
{
	env->xa_rmid = rmid;
	TAILQ_INSERT_HEAD(&DB_GLOBAL(envq), env, links);
}
/*
 * __db_unmap_rmid
 *	Destroy the mapping for the given rmid.
 *
 * PUBLIC: int __db_unmap_rmid __P((int));
 */
int __db_unmap_rmid(int rmid)
{
	ENV * e;
	for(e = TAILQ_FIRST(&DB_GLOBAL(envq)); e->xa_rmid != rmid; e = TAILQ_NEXT(e, links))
		;
	if(e == NULL)
		return (EINVAL);
	TAILQ_REMOVE(&DB_GLOBAL(envq), e, links);
	return (0);
}
/*
 * __db_unmap_xid
 *	Destroy the mapping for the specified XID.
 *
 * PUBLIC: void __db_unmap_xid __P((ENV *, XID *, size_t));
 */
void __db_unmap_xid(ENV *env, XID * xid, size_t off)
{
	TXN_DETAIL * td;
	COMPQUIET(xid, NULL);
	td = (TXN_DETAIL *)R_ADDR(&env->tx_handle->reginfo, off);
	memset(td->gid, 0, sizeof(td->gid));
}
