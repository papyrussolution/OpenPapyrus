/*-
 * See the file LICENSE for redistribution information.
 * Copyright (c) 1996, 2011 Oracle and/or its affiliates.  All rights reserved.
 * $Id$
 */
#ifndef _DB_LOG_VERIFY_H_
#define _DB_LOG_VERIFY_H_

#include "db_config.h"
#include "db_int.h"
//  
// Log verification handle, such a handle is shared among all verification 
// functions during one verification process. 
// 
struct __db_log_verify_info {
	DB_ENV *dbenv;		/* The database environment. */
	DB *txninfo;		/* (txnid, __txn_verify_info) map. */
	DB *ckps;		/* (ckp lrid, __ckpinfo) map. */
	DB *fileregs; 		/* (file-uid, __file_reg_info) map. */
	DB *fnameuid;		/* (fname, fuid), secondary db of fileregs. */
	DB *dbregids; // (dbreg-id, __file_reg_info) map, NOT the sec db for fileregs. 
	DB *pgtxn; 		/* (fileid-pageno, txnid) map. */
	DB *txnpg; 		/* (txnid, fileid-pageno), sec db of pgtxn. */
	DB *lsntime;  // lsn, (time-stamp, logtype(txn_regop or txn_ckp)) map. 
	DB *timelsn; // Secondary db of lsntime, use timestamp as secindex. 
	DB *txnrngs; // Time range database, (uint32, __lv_txnrange) db. 
	DB *txnaborts;	 // Store abort txn (lsn, txnid) map. 
	DB_LSN last_lsn;	/* Lsn of last log record we verified. */
	uint32 ntxn_active, ntxn_abort, ntxn_commit, ntxn_prep;  // The number of active, abort, commit and prepared txns.
	uint32 nckp;		/* The number of checkpoint log records. */
	uint8 target_dbid[DB_FILE_ID_LEN]; // Target database file unique id. Set if only verify log records of a database. 
	uint32 non_txnup_cnt;/* Number of non-txnal log records. */
	uint32 unknown_logrec_cnt;/* Number of unknown log record. */
	uint32 external_logrec_cnt;/* Number of external log record. */
	uint32 lrtypes[256]; // (Log type, number of record) map. typeids are continuous integers, 256 is a big enough number. 
	uint32 aborted_txnid; // The last aborted txnid. 
	DB_LSN aborted_txnlsn; /* Last aborted txn's last log. */
	DB_LSN valid_lsn; /* When reach this log,unset DB_LOG_VERIFY_PARTIAL. */
	char *logtype_names[256]; // The type name string of each type of log.
	const DB_LOG_VERIFY_CONFIG *lv_config;
	DB_THREAD_INFO * ip;
	uint32 flags;	/* The result of the verification. */
};
//
// Transaction information.
//
struct __txn_verify_info {
#define TXN_VERIFY_INFO_FIXSIZE    (4 * sizeof(DB_LSN) + 9 * sizeof(uint32))
#define TXN_VERIFY_INFO_TOTSIZE(s) (TXN_VERIFY_INFO_FIXSIZE + (s).num_recycle * sizeof(DB_LSN) + 	\
	__lv_dbt_arrsz((s).fileups, (s).filenum) + sizeof(int32) * (s).filenum)

	uint32 txnid; 	/* The key, also stored in data here. */
	uint32 ptxnid; 	/* The parent txn id. */
	DB_LSN first_lsn;	/* Lsn of the first log record of this txn. */
	DB_LSN last_lsn; 	/* Last lsn of the txn. */
	DB_LSN prep_lsn; 	/* txn_prepare's lsn.*/
	DB_LSN cur_lsn;		/* The lsn of the latest db op of this txn. */
	uint32 num_recycle; 	/* The number of recycle lsns. */
	uint32 filenum; 	/* The number of files updated. */
#define TXN_STAT_ACTIVE 0
#define TXN_STAT_ABORT 1
#define TXN_STAT_COMMIT 2
#define TXN_STAT_PREPARE 3
	uint32 status;	/* Txn status */
	uint32 nchild_active; // The number of active, abort and commit children
	uint32 nchild_abort;
	uint32 nchild_commit;
	uint32 flags; /* Copied from the DB_TXN::flags member. */
	DB_LSN *recycle_lsns; 	/* The array of txn_recycle records' lsns. */
	DBT *fileups; 	 // The array of file unique ids of files updated by this txn. 
	int32 *dbregid;/* The array of dbreg file ids updated by this txn. */
};
//
// Database file information. 
//
struct __lv_filereg_info {
#define FILE_REG_INFO_FIXSIZE (sizeof(uint32))
#define FILE_REG_INFO_TOTSIZE(s) (FILE_REG_INFO_FIXSIZE + (s).fileid.size + sizeof((s).fileid.size) + sizeof(int32) * (s).regcnt + sstrlen((s).fname) + 1)
	uint32 regcnt;	/* The number of dbregids for this file-uid. */
	int32 *dbregids;
	DBT    fileid; // The file unique id. 
	char * fname;  // Database file name. 
};
//
// Database file dbreg_register information. 
//
struct __lv_filelife {
	int32 dbregid;	/* The primary key. */
	DBTYPE dbtype;		/* The database type. */
	uint32 lifetime;	/* DBREG_OPEN, DBREG_CHKPNT, DBREG_CLOSE */
	db_pgno_t meta_pgno;	/* The meta_pgno; */
	uint8 fileid[DB_FILE_ID_LEN];
	DB_LSN lsn;		/* The lsn of log updating lifetime. */
};
//
// Checkpoint information. 
//
struct __lv_ckp_info {
	int32 timestamp;
	DB_LSN lsn, ckplsn; // Lsn member is the primary key.
};
// 
// General information from log records which have timestamps. 
// We use it to do time range verifications. Such information is 
// acquired when backward-playing the logs before verification. 
// 
struct __lv_timestamp_info {
	DB_LSN lsn;		/* The primary key. */
	int32 timestamp;	/* The secondary key. */
	uint32 logtype; // The log types containing a time stamp, so far only txn_ckp and txn_regop types.
};
// 
// Transaction ranges. Such information is acquired when backward-playing the
// logs before verification. Can be used to find aborted txns.
// 
struct __lv_txnrange {
	uint32 txnid; // Transaction ID, the primary key. The db storing records of this type should allow dup since txnids maybe reused. 
	uint32 ptxnid; // The parent txn id, ptxnid is the parent of txnid during [begin, end]. 
	/* 
	 * The first and last lsn, end is used to sort dup data because it's
	 * seen prior to begin in a backward playback, and [begin, end] intervals won't overlap. 
	 */
	DB_LSN begin, end;
	int32 when_commit; // The time of the commit, 0 if aborted. 
};
//
// Parameter types for __iterate_txninfo function. 
//
struct __add_recycle_params {
	uint32 min, max; // The recycled txnid range. 
	VRFY_TXN_INFO **ti2u; // The array of txn info to update into db.
	uint32 ti2ui, ti2ul;/* The effective length and array length. */
	DB_LSN recycle_lsn;
}; 

struct __ckp_verify_params {
	DB_LSN lsn, ckp_lsn;
	ENV *env;
};
//
// Helper macros.
#define LOGTYPE_NAME(lvh, type) ((lvh->logtype_names[type] == NULL) ? NULL : lvh->logtype_names[type] + 3)
#define NUMCMP(i1, i2) ((i1) > (i2) ? 1 : ((i1) < (i2) ? -1 : 0))

#define INVAL_DBREGID -1
//
// During recovery, DBREG_CHKPNT can be seen as open, and it's followed by a DBREG_RCLOSE or DBREG_CLOSE. 
//
#define IS_DBREG_OPEN(opcode)  oneof4(opcode, DBREG_OPEN, DBREG_PREOPEN, DBREG_REOPEN, DBREG_CHKPNT)
#define IS_DBREG_CLOSE(opcode) oneof2(opcode, DBREG_CLOSE, DBREG_RCLOSE)

#define IS_LOG_VRFY_SUPPORTED(version) ((version) == DB_LOGVERSION)

#endif /* !_DB_LOG_VERIFY_H_*/
