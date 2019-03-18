/* Do not edit: automatically built by gen_rec.awk. */

#ifndef	__heap_AUTO_H
#define	__heap_AUTO_H
#ifdef HAVE_HEAP
// @v9.5.5 #include "dbinc/log.h"
#define	DB___heap_addrem	151
typedef struct ___heap_addrem_args {
	uint32 type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	uint32	opcode;
	int32	fileid;
	db_pgno_t	pgno;
	uint32	indx;
	uint32	nbytes;
	DBT	hdr;
	DBT	dbt;
	DB_LSN	pagelsn;
} __heap_addrem_args;

extern __DB_IMPORT DB_LOG_RECSPEC __heap_addrem_desc[];
static inline int
__heap_addrem_log(DB *dbp, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32 flags,
    uint32 opcode, db_pgno_t pgno, uint32 indx, uint32 nbytes,
    const DBT *hdr, const DBT *dbt, DB_LSN * pagelsn)
{
	return (__log_put_record((dbp)->env, dbp, txnp, ret_lsnp, flags, DB___heap_addrem, 0,
	    sizeof(uint32) + sizeof(uint32) + sizeof(DB_LSN) + sizeof(uint32) + sizeof(uint32) + sizeof(uint32) +
	    sizeof(uint32) + sizeof(uint32) + LOG_DBT_SIZE(hdr) + LOG_DBT_SIZE(dbt) + sizeof(*pagelsn),
	    __heap_addrem_desc, opcode, pgno, indx, nbytes, hdr, dbt, pagelsn));
}

static inline int __heap_addrem_read(ENV *env, DB **dbpp, void *td, void *data, __heap_addrem_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, dbpp, td, data, __heap_addrem_desc, sizeof(__heap_addrem_args), reinterpret_cast<void **>(arg)));
}
#define	DB___heap_pg_alloc	152
typedef struct ___heap_pg_alloc_args {
	uint32 type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	int32	fileid;
	DB_LSN	meta_lsn;
	db_pgno_t	meta_pgno;
	db_pgno_t	pgno;
	uint32	ptype;
	db_pgno_t	last_pgno;
} __heap_pg_alloc_args;

extern __DB_IMPORT DB_LOG_RECSPEC __heap_pg_alloc_desc[];
static inline int __heap_pg_alloc_log(DB *dbp, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32 flags, DB_LSN * meta_lsn, db_pgno_t meta_pgno, db_pgno_t pgno, uint32 ptype,
    db_pgno_t last_pgno)
{
	return (__log_put_record((dbp)->env, dbp, txnp, ret_lsnp, flags, DB___heap_pg_alloc, 0,
	    sizeof(uint32) + sizeof(uint32) + sizeof(DB_LSN) + sizeof(uint32) + sizeof(*meta_lsn) + sizeof(uint32) +
	    sizeof(uint32) + sizeof(uint32) + sizeof(uint32), __heap_pg_alloc_desc, meta_lsn, meta_pgno, pgno, ptype, last_pgno));
}

static inline int __heap_pg_alloc_read(ENV *env, DB **dbpp, void *td, void *data, __heap_pg_alloc_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, dbpp, td, data, __heap_pg_alloc_desc, sizeof(__heap_pg_alloc_args), reinterpret_cast<void **>(arg)));
}
#define	DB___heap_trunc_meta	153
typedef struct ___heap_trunc_meta_args {
	uint32 type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	int32	fileid;
	db_pgno_t	pgno;
	uint32	last_pgno;
	uint32	key_count;
	uint32	record_count;
	uint32	curregion;
	uint32	nregions;
	DB_LSN	pagelsn;
} __heap_trunc_meta_args;

extern __DB_IMPORT DB_LOG_RECSPEC __heap_trunc_meta_desc[];
static inline int __heap_trunc_meta_log(DB *dbp, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32 flags, db_pgno_t pgno, 
	uint32 last_pgno, uint32 key_count, uint32 record_count, uint32 curregion, uint32 nregions, DB_LSN * pagelsn)
{
	return (__log_put_record((dbp)->env, dbp, txnp, ret_lsnp, flags, DB___heap_trunc_meta, 0,
	    sizeof(uint32) + sizeof(uint32) + sizeof(DB_LSN) + sizeof(uint32) + sizeof(uint32) + sizeof(uint32) +
	    sizeof(uint32) + sizeof(uint32) + sizeof(uint32) + sizeof(uint32) + sizeof(*pagelsn),
	    __heap_trunc_meta_desc, pgno, last_pgno, key_count, record_count, curregion, nregions, pagelsn));
}

static inline int __heap_trunc_meta_read(ENV *env, DB **dbpp, void *td, void *data, __heap_trunc_meta_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, dbpp, td, data, __heap_trunc_meta_desc, sizeof(__heap_trunc_meta_args), reinterpret_cast<void **>(arg)));
}
#define	DB___heap_trunc_page	154
typedef struct ___heap_trunc_page_args {
	uint32 type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	int32	fileid;
	db_pgno_t	pgno;
	DBT	old_data;
	uint32	is_region;
	DB_LSN	pagelsn;
} __heap_trunc_page_args;

extern __DB_IMPORT DB_LOG_RECSPEC __heap_trunc_page_desc[];
static inline int
__heap_trunc_page_log(DB *dbp, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32 flags, db_pgno_t pgno, const DBT *old_data, uint32 is_region, DB_LSN * pagelsn)
{
	return (__log_put_record((dbp)->env, dbp, txnp, ret_lsnp,
	    flags, DB___heap_trunc_page, 0,
	    sizeof(uint32) + sizeof(uint32) + sizeof(DB_LSN) +
	    sizeof(uint32) + sizeof(uint32) + LOG_DBT_SIZE(old_data) +
	    sizeof(uint32) + sizeof(*pagelsn),
	    __heap_trunc_page_desc, pgno, old_data, is_region, pagelsn));
}

static inline int __heap_trunc_page_read(ENV *env, 
    DB **dbpp, void *td, void *data, __heap_trunc_page_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, 
	    dbpp, td, data, __heap_trunc_page_desc, sizeof(__heap_trunc_page_args), reinterpret_cast<void **>(arg)));
}
#endif /* HAVE_HEAP */
#endif
