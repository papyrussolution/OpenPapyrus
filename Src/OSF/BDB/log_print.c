/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2011 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
// @v9.5.5 #include "dbinc/db_page.h"
// @v9.5.5 #include "dbinc/lock.h"
// @v9.5.5 #include "dbinc/mp.h"
// @v9.5.5 #include "dbinc/crypto.h"
// @v9.5.5 #include "dbinc/btree.h"
// @v9.5.5 #include "dbinc/hash.h"
#pragma hdrstop
// @v9.5.5 #include "dbinc/db_am.h"

static int __log_print_dbregister __P((ENV*, DBT*, DB_LOG *));

/*
 * PUBLIC: int __log_print_record  __P((ENV *,
 * PUBLIC:      DBT *, DB_LSN *, char *, DB_LOG_RECSPEC *, void *));
 */
int __log_print_record(ENV * env, DBT * recbuf, DB_LSN * lsnp, char * name, DB_LOG_RECSPEC * spec, void * info)
{
	DB * dbp;
	DBT dbt;
	DB_LOG_RECSPEC * sp, * np;
	DB_LOG * dblp;
	DB_LSN prev_lsn;
	DB_MSGBUF msgbuf;
	LOG * lp;
	PAGE * hdrstart, * hdrtmp;
	int32 inttmp;
	uint32 hdrsize, op, uinttmp;
	uint32 type, txnid;
	uint8 * bp, * datatmp;
	int has_data, ret, downrev;
	struct tm * lt;
	__time64_t timeval;
	char time_buf[CTIME_BUFLEN], * s;
	const char * hdrname;

	COMPQUIET(hdrstart, NULL);
	COMPQUIET(hdrname, NULL);
	COMPQUIET(hdrsize, 0);
	COMPQUIET(has_data, 0);
	COMPQUIET(op, 0);

	bp = (uint8 *)recbuf->data;
	dblp = (DB_LOG *)info;
	dbp = NULL;
	lp = (LOG *)env->lg_handle->reginfo.primary;
	downrev = lp->persist.version < DB_LOGVERSION_50;
	DB_MSGBUF_INIT(&msgbuf);

	/*
	 * The first three fields are always the same in every arg
	 * struct so we know their offsets.
	 */
	/* type */
	LOGCOPY_32(env, &type, bp);
	bp += sizeof(uint32);

	/* txnp */
	LOGCOPY_32(env, &txnid, bp);
	bp += sizeof(txnid);

	/* Previous LSN */
	LOGCOPY_TOLSN(env, &prev_lsn, bp);
	bp += sizeof(DB_LSN);
	__db_msgadd(env, &msgbuf,
		"[%lu][%lu]%s%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
		(ulong)lsnp->file, (ulong)lsnp->offset,
		name, (type&DB_debug_FLAG) ? "_debug" : "",
		(ulong)type,
		(ulong)txnid,
		(ulong)prev_lsn.file, (ulong)prev_lsn.offset);

	for(sp = spec; sp->type != LOGREC_Done; sp++) {
		switch(sp->type) {
		    case LOGREC_OP:
			LOGCOPY_32(env, &op, bp);
			__db_msgadd(env, &msgbuf,  "\t%s: ", sp->name);
			__db_msgadd(env, &msgbuf,  sp->fmt, OP_MODE_GET(op));
			__db_msgadd(env, &msgbuf,  " ptype: %s\n",
				__db_pagetype_to_string(OP_PAGE_GET(op)));
			bp += sizeof(uinttmp);
			break;
		    case LOGREC_DB:
			LOGCOPY_32(env, &inttmp, bp);
			__db_msgadd(env, &msgbuf,  "\t%s: %lu\n",
				sp->name, (ulong)inttmp);
			bp += sizeof(inttmp);
			if(dblp != NULL && inttmp < dblp->dbentry_cnt)
				dbp = dblp->dbentry[inttmp].dbp;
			break;

		    case LOGREC_DBOP:
			/* Special op for dbreg_register records. */
			if(dblp != NULL && (ret =
			                            __log_print_dbregister(env, recbuf, dblp)) != 0)
				return ret;
			LOGCOPY_32(env, &uinttmp, bp);
			switch(FLD_ISSET(uinttmp, DBREG_OP_MASK)) {
			    case DBREG_CHKPNT:
				s = "CHKPNT";
				break;
			    case DBREG_CLOSE:
				s = "CLOSE";
				break;
			    case DBREG_OPEN:
				s = "OPEN";
				break;
			    case DBREG_PREOPEN:
				s = "PREOPEN";
				break;
			    case DBREG_RCLOSE:
				s = "RCLOSE";
				break;
			    case DBREG_REOPEN:
				s = "REOPEN";
				break;
			    default:
				s = "UNKNOWN";
				break;
			}
			__db_msgadd(env, &msgbuf,  "\t%s: %s %lx\n", sp->name,
				s, (ulong)(uinttmp&~DBREG_OP_MASK));
			bp += sizeof(uinttmp);
			break;
		    case LOGREC_ARG:
			LOGCOPY_32(env, &uinttmp, bp);
			__db_msgadd(env, &msgbuf,  "\t%s: ", sp->name);
			__db_msgadd(env, &msgbuf,  sp->fmt, uinttmp);
			__db_msgadd(env, &msgbuf,  "\n");
			bp += sizeof(uinttmp);
			break;
		    case LOGREC_TIME:
			/* __time64_t is long but we only store 32 bits. */
			LOGCOPY_32(env, &uinttmp, bp);
			timeval = uinttmp;
			lt = _localtime64(&timeval);
			__db_msgadd(env, &msgbuf,
				"\t%s: %ld (%.24s, 20%02lu%02lu%02lu%02lu%02lu.%02lu)\n",
				sp->name, (long)timeval,
				__os_ctime(&timeval, time_buf),
				(ulong)lt->tm_year-100, (ulong)lt->tm_mon+1,
				(ulong)lt->tm_mday, (ulong)lt->tm_hour,
				(ulong)lt->tm_min, (ulong)lt->tm_sec);
			bp += sizeof(uinttmp);
			break;
		    case LOGREC_PGDBT:
		    case LOGREC_PGDDBT:
		    case LOGREC_PGLIST:
		    case LOGREC_LOCKS:
		    case LOGREC_HDR:
		    case LOGREC_DATA:
		    case LOGREC_DBT:
			LOGCOPY_32(env, &uinttmp, bp);
			bp += sizeof(uint32);
			switch(sp->type) {
			    case LOGREC_HDR:
				if(uinttmp == 0)
					break;
				has_data = 0;
				for(np = sp+1; np->type != LOGREC_Done; np++)
					if(np->type == LOGREC_DATA) {
						has_data = 1;
						break;
					}
				hdrstart = (PAGE *)bp;
				hdrsize = uinttmp;
				hdrname = sp->name;
				if(has_data == 1)
					break;
			    /* FALLTHROUGH */
			    case LOGREC_DATA:
				if(downrev ? LOG_SWAPPED(env) : (dbp != NULL  && F_ISSET(dbp, DB_AM_SWAP)))
					__db_recordswap(op, hdrsize, hdrstart, (has_data && uinttmp != 0) ? bp : NULL, 1);
				__db_msgadd(env, &msgbuf,  "\t%s: ", hdrname);
				__db_prbytes(env, &msgbuf,
					(uint8 *)hdrstart, hdrsize);
				if(has_data == 0 || uinttmp == 0)
					break;
			    /* FALLTHROUGH */
			    default:
				__db_msgadd(env, &msgbuf,  "\t%s: ", sp->name);
pr_data:
				__db_prbytes(env, &msgbuf, bp, uinttmp);
				has_data = 0;
				break;
			    case LOGREC_PGDBT:
				has_data = 0;
				for(np = sp+1; np->type != LOGREC_Done; np++)
					if(np->type == LOGREC_PGDDBT) {
						has_data = 1;
						break;
					}
				hdrstart = (PAGE *)bp;
				hdrsize = uinttmp;
				if(has_data == 1)
					break;
			    /* FALLTHROUGH */
			    case LOGREC_PGDDBT:
				DB_ASSERT(env, hdrstart != NULL);
				if(dbp != NULL && (downrev ? LOG_SWAPPED(env) : F_ISSET(dbp, DB_AM_SWAP))) {
					dbt.data = bp;
					dbt.size = uinttmp;
					if((ret = __db_pageswap(env, dbp, hdrstart, hdrsize, has_data == 0 ? NULL : &dbt, 1)) != 0)
						return ret;
				}
				if(downrev)
					goto pr_data;
				if(ALIGNP_INC(hdrstart, sizeof(uint32)) != hdrstart) {
					if((ret = __os_malloc(env, hdrsize, &hdrtmp)) != 0)
						return ret;
					memcpy(hdrtmp, hdrstart, hdrsize);
				}
				else
					hdrtmp = hdrstart;
				if(has_data == 1 && ALIGNP_INC(bp,
					   sizeof(uint32)) != bp) {
					if((ret = __os_malloc(env,
						    uinttmp, &datatmp)) != 0)
						return ret;
					memcpy(datatmp, bp, uinttmp);
				}
				else if(has_data == 1)
					datatmp = bp;
				else
					datatmp = NULL;
				if((ret = __db_prpage_int(env, &msgbuf,
					    dbp, "\t", hdrtmp,
					    uinttmp, datatmp, DB_PR_PAGE)) != 0)
					return ret;
				has_data = 0;
				if(hdrtmp != hdrstart)
					__os_free(env, hdrtmp);
				if(datatmp != bp && datatmp != NULL)
					__os_free(env, datatmp);
				break;
			    case LOGREC_PGLIST:
				dbt.data = bp;
				dbt.size = uinttmp;
				__db_pglist_print(env, &msgbuf, &dbt);
				break;
			    case LOGREC_LOCKS:
				dbt.data = bp;
				dbt.size = uinttmp;
				__lock_list_print(env, &msgbuf, &dbt);
				break;
			}
			bp += uinttmp;
			break;

		    case LOGREC_POINTER:
			LOGCOPY_TOLSN(env, &prev_lsn, bp);
			__db_msgadd(env, &msgbuf,
				"\t%s: [%lu][%lu]\n", sp->name,
				(ulong)prev_lsn.file, (ulong)prev_lsn.offset);
			bp += sizeof(DB_LSN);
			break;
		    case LOGREC_Done:
			DB_ASSERT(env, sp->type != LOGREC_Done);
		}
	}
	if(msgbuf.buf != NULL)
		DB_MSGBUF_FLUSH(env, &msgbuf);
	else
		__db_msg(env, "%s", "");
	return 0;
}

/*
 * __log_print_dbregister --
 *	So that we can properly swap and print information from databases
 * we generate dummy DB handles here.  These are real handles that are never
 * opened but their fileid, meta_pgno and some flags are set properly.
 * This code uses parallel structures to those in the dbregister code.
 * The DB_LOG handle passed in must NOT be the real environment handle
 * since this would confuse actual running transactions if printing is
 * done while the environment is active.
 */
static int __log_print_dbregister(ENV * env, DBT * recbuf, DB_LOG * dblp)
{
	__dbreg_register_args * argp;
	DB * dbp;
	DB_ENTRY * dbe;
	int ret;
	if((ret = __dbreg_register_read(env, recbuf->data, &argp)) != 0)
		return ret;
	if(dblp->dbentry_cnt <= argp->fileid &&
	   (ret = __dbreg_add_dbentry(env, dblp, NULL, argp->fileid)) != 0)
		goto err;
	dbe = &dblp->dbentry[argp->fileid];
	dbp = dbe->dbp;

	switch(FLD_ISSET(argp->opcode, DBREG_OP_MASK)) {
	    case DBREG_CHKPNT:
	    case DBREG_OPEN:
	    case DBREG_REOPEN:
		if(dbp != NULL) {
			if(memcmp(dbp->fileid, argp->uid.data, DB_FILE_ID_LEN) == 0 && dbp->meta_pgno == argp->meta_pgno)
				goto done;
			if((__db_close(dbp, NULL, DB_NOSYNC)) != 0)
				goto err;
			dbe->dbp = dbp = NULL;
		}
		if((ret = __db_create_internal(&dbp, env, 0)) != 0)
			goto err;
		memcpy(dbp->fileid, argp->uid.data, DB_FILE_ID_LEN);
		dbp->meta_pgno = argp->meta_pgno;
		F_SET(dbp, DB_AM_RECOVER);
		/*
		 * We need to swap bytes if we are on a BIGEND machine XOR
		 *	we have a BIGEND database.
		 */
		if((F_ISSET(env, ENV_LITTLEENDIAN) == 0)^
		   (FLD_ISSET(argp->opcode, DBREG_BIGEND) != 0))
			F_SET(dbp, DB_AM_SWAP);
		if(FLD_ISSET(argp->opcode, DBREG_CHKSUM))
			F_SET(dbp, DB_AM_CHKSUM);
		if(FLD_ISSET(argp->opcode, DBREG_ENCRYPT))
			F_SET(dbp, DB_AM_ENCRYPT);
		dbe->dbp = dbp;
		break;
	    case DBREG_CLOSE:
	    case DBREG_RCLOSE:
		if(dbp == NULL)
			goto err;
		if((__db_close(dbp, NULL, DB_NOSYNC)) != 0)
			goto err;
		dbe->dbp = dbp = NULL;
		break;
	    case DBREG_PREOPEN:
		break;
	    default:
		DB_ASSERT(env, argp->opcode != argp->opcode);
	}
done:
err:
	__os_free(env, argp);
	return ret;
}
