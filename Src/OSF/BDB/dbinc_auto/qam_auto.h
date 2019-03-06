/* Do not edit: automatically built by gen_rec.awk. */

#ifndef	__qam_AUTO_H
#define	__qam_AUTO_H
#ifdef HAVE_QUEUE
// @v9.5.5 #include "dbinc/log.h"
#define	DB___qam_incfirst	84
typedef struct ___qam_incfirst_args {
	uint32 type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	int32	fileid;
	db_recno_t	recno;
	db_pgno_t	meta_pgno;
} __qam_incfirst_args;

extern __DB_IMPORT DB_LOG_RECSPEC __qam_incfirst_desc[];
static inline int
__qam_incfirst_log(DB *dbp, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32 flags, db_recno_t recno, db_pgno_t meta_pgno)
{
	return (__log_put_record((dbp)->env, dbp, txnp, ret_lsnp,
	    flags, DB___qam_incfirst, 0,
	    sizeof(uint32) + sizeof(uint32) + sizeof(DB_LSN) +
	    sizeof(uint32) + sizeof(uint32) + sizeof(uint32),
	    __qam_incfirst_desc, recno, meta_pgno));
}

static inline int __qam_incfirst_read(ENV *env, 
    DB **dbpp, void *td, void *data, __qam_incfirst_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, 
	    dbpp, td, data, __qam_incfirst_desc, sizeof(__qam_incfirst_args), reinterpret_cast<void **>(arg)));
}
#define	DB___qam_mvptr	85
typedef struct ___qam_mvptr_args {
	uint32 type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	uint32	opcode;
	int32	fileid;
	db_recno_t	old_first;
	db_recno_t	new_first;
	db_recno_t	old_cur;
	db_recno_t	new_cur;
	DB_LSN	metalsn;
	db_pgno_t	meta_pgno;
} __qam_mvptr_args;

extern __DB_IMPORT DB_LOG_RECSPEC __qam_mvptr_desc[];
static inline int
__qam_mvptr_log(DB *dbp, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32 flags,
    uint32 opcode, db_recno_t old_first, db_recno_t new_first, db_recno_t old_cur,
    db_recno_t new_cur, DB_LSN * metalsn, db_pgno_t meta_pgno)
{
	return (__log_put_record((dbp)->env, dbp, txnp, ret_lsnp,
	    flags, DB___qam_mvptr, 0,
	    sizeof(uint32) + sizeof(uint32) + sizeof(DB_LSN) +
	    sizeof(uint32) + sizeof(uint32) + sizeof(uint32) +
	    sizeof(uint32) + sizeof(uint32) + sizeof(uint32) +
	    sizeof(*metalsn) + sizeof(uint32),
	    __qam_mvptr_desc,
	    opcode, old_first, new_first, old_cur, new_cur, metalsn, meta_pgno));
}

static inline int __qam_mvptr_read(ENV *env, 
    DB **dbpp, void *td, void *data, __qam_mvptr_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, 
	    dbpp, td, data, __qam_mvptr_desc, sizeof(__qam_mvptr_args), reinterpret_cast<void **>(arg)));
}
#define	DB___qam_del	79
typedef struct ___qam_del_args {
	uint32 type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	int32	fileid;
	DB_LSN	lsn;
	db_pgno_t	pgno;
	uint32	indx;
	db_recno_t	recno;
} __qam_del_args;

extern __DB_IMPORT DB_LOG_RECSPEC __qam_del_desc[];
static inline int
__qam_del_log(DB *dbp, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32 flags, DB_LSN * lsn, db_pgno_t pgno, uint32 indx, db_recno_t recno)
{
	return (__log_put_record((dbp)->env, dbp, txnp, ret_lsnp,
	    flags, DB___qam_del, 0,
	    sizeof(uint32) + sizeof(uint32) + sizeof(DB_LSN) +
	    sizeof(uint32) + sizeof(*lsn) + sizeof(uint32) +
	    sizeof(uint32) + sizeof(uint32),
	    __qam_del_desc, lsn, pgno, indx, recno));
}

static inline int __qam_del_read(ENV *env, 
    DB **dbpp, void *td, void *data, __qam_del_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, 
	    dbpp, td, data, __qam_del_desc, sizeof(__qam_del_args), reinterpret_cast<void **>(arg)));
}
#define	DB___qam_add	80
typedef struct ___qam_add_args {
	uint32 type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	int32	fileid;
	DB_LSN	lsn;
	db_pgno_t	pgno;
	uint32	indx;
	db_recno_t	recno;
	DBT	data;
	uint32	vflag;
	DBT	olddata;
} __qam_add_args;

extern __DB_IMPORT DB_LOG_RECSPEC __qam_add_desc[];
static inline int
__qam_add_log(DB *dbp, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32 flags, DB_LSN * lsn, db_pgno_t pgno, uint32 indx, db_recno_t recno,
    const DBT *data, uint32 vflag, const DBT *olddata)
{
	return (__log_put_record((dbp)->env, dbp, txnp, ret_lsnp,
	    flags, DB___qam_add, 0,
	    sizeof(uint32) + sizeof(uint32) + sizeof(DB_LSN) +
	    sizeof(uint32) + sizeof(*lsn) + sizeof(uint32) +
	    sizeof(uint32) + sizeof(uint32) + LOG_DBT_SIZE(data) +
	    sizeof(uint32) + LOG_DBT_SIZE(olddata),
	    __qam_add_desc, lsn, pgno, indx, recno, data, vflag, olddata));
}

static inline int __qam_add_read(ENV *env, 
    DB **dbpp, void *td, void *data, __qam_add_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, 
	    dbpp, td, data, __qam_add_desc, sizeof(__qam_add_args), reinterpret_cast<void **>(arg)));
}
#define	DB___qam_delext	83
typedef struct ___qam_delext_args {
	uint32 type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	int32	fileid;
	DB_LSN	lsn;
	db_pgno_t	pgno;
	uint32	indx;
	db_recno_t	recno;
	DBT	data;
} __qam_delext_args;

extern __DB_IMPORT DB_LOG_RECSPEC __qam_delext_desc[];
static inline int
__qam_delext_log(DB *dbp, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32 flags, DB_LSN * lsn, db_pgno_t pgno, uint32 indx, db_recno_t recno,
    const DBT *data)
{
	return (__log_put_record((dbp)->env, dbp, txnp, ret_lsnp,
	    flags, DB___qam_delext, 0,
	    sizeof(uint32) + sizeof(uint32) + sizeof(DB_LSN) +
	    sizeof(uint32) + sizeof(*lsn) + sizeof(uint32) +
	    sizeof(uint32) + sizeof(uint32) + LOG_DBT_SIZE(data),
	    __qam_delext_desc, lsn, pgno, indx, recno, data));
}

static inline int __qam_delext_read(ENV *env, 
    DB **dbpp, void *td, void *data, __qam_delext_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, 
	    dbpp, td, data, __qam_delext_desc, sizeof(__qam_delext_args), reinterpret_cast<void **>(arg)));
}
#endif /* HAVE_QUEUE */
#endif
