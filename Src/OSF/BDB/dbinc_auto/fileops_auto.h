/* Do not edit: automatically built by gen_rec.awk. */

#ifndef	__fop_AUTO_H
#define	__fop_AUTO_H
#define	DB___fop_create_42	143
typedef struct ___fop_create_42_args {
	uint32 type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	name;
	uint32	appname;
	uint32	mode;
} __fop_create_42_args;

extern __DB_IMPORT DB_LOG_RECSPEC __fop_create_42_desc[];
static inline int __fop_create_42_read(ENV *env, 
    void *data, __fop_create_42_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, 
	    NULL, NULL, data, __fop_create_42_desc, sizeof(__fop_create_42_args), reinterpret_cast<void **>(arg)));
}
#define	DB___fop_create	143
typedef struct ___fop_create_args {
	uint32 type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	name;
	DBT	dirname;
	uint32	appname;
	uint32	mode;
} __fop_create_args;

extern __DB_IMPORT DB_LOG_RECSPEC __fop_create_desc[];
static inline int
__fop_create_log(ENV *env, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32 flags,
    const DBT *name, const DBT *dirname, uint32 appname, uint32 mode)
{
	return (__log_put_record(env, NULL, txnp, ret_lsnp,
	    flags, DB___fop_create, 0,
	    sizeof(uint32) + sizeof(uint32) + sizeof(DB_LSN) +
	    LOG_DBT_SIZE(name) + LOG_DBT_SIZE(dirname) + sizeof(uint32) +
	    sizeof(uint32),
	    __fop_create_desc,
	    name, dirname, appname, mode));
}

static inline int __fop_create_read(ENV *env, 
    void *data, __fop_create_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, 
	    NULL, NULL, data, __fop_create_desc, sizeof(__fop_create_args), reinterpret_cast<void **>(arg)));
}
#define	DB___fop_remove	144
typedef struct ___fop_remove_args {
	uint32 type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	name;
	DBT	fid;
	uint32	appname;
} __fop_remove_args;

extern __DB_IMPORT DB_LOG_RECSPEC __fop_remove_desc[];
static inline int
__fop_remove_log(ENV *env, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32 flags,
    const DBT *name, const DBT *fid, uint32 appname)
{
	return (__log_put_record(env, NULL, txnp, ret_lsnp,
	    flags, DB___fop_remove, 0,
	    sizeof(uint32) + sizeof(uint32) + sizeof(DB_LSN) +
	    LOG_DBT_SIZE(name) + LOG_DBT_SIZE(fid) + sizeof(uint32),
	    __fop_remove_desc,
	    name, fid, appname));
}

static inline int __fop_remove_read(ENV *env, 
    void *data, __fop_remove_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, 
	    NULL, NULL, data, __fop_remove_desc, sizeof(__fop_remove_args), reinterpret_cast<void **>(arg)));
}
#define	DB___fop_write_42	145
typedef struct ___fop_write_42_args {
	uint32 type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	name;
	uint32	appname;
	uint32	pgsize;
	db_pgno_t	pageno;
	uint32	offset;
	DBT	page;
	uint32	flag;
} __fop_write_42_args;

extern __DB_IMPORT DB_LOG_RECSPEC __fop_write_42_desc[];
static inline int __fop_write_42_read(ENV *env, void *data, __fop_write_42_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, NULL, NULL, data, __fop_write_42_desc, sizeof(__fop_write_42_args), reinterpret_cast<void **>(arg)));
}
#define	DB___fop_write	145
typedef struct ___fop_write_args {
	uint32 type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	name;
	DBT	dirname;
	uint32	appname;
	uint32	pgsize;
	db_pgno_t	pageno;
	uint32	offset;
	DBT	page;
	uint32	flag;
} __fop_write_args;

extern __DB_IMPORT DB_LOG_RECSPEC __fop_write_desc[];
static inline int __fop_write_log(ENV *env, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32 flags,
    const DBT *name, const DBT *dirname, uint32 appname, uint32 pgsize, db_pgno_t pageno, uint32 offset, const DBT *page, uint32 flag)
{
	return (__log_put_record(env, NULL, txnp, ret_lsnp, flags, DB___fop_write, 0,
	    sizeof(uint32) + sizeof(uint32) + sizeof(DB_LSN) + LOG_DBT_SIZE(name) + LOG_DBT_SIZE(dirname) + sizeof(uint32) +
	    sizeof(uint32) + sizeof(uint32) + sizeof(uint32) + LOG_DBT_SIZE(page) + sizeof(uint32),
	    __fop_write_desc, name, dirname, appname, pgsize, pageno, offset, page, flag));
}

static inline int __fop_write_read(ENV *env, void *data, __fop_write_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, NULL, NULL, data, __fop_write_desc, sizeof(__fop_write_args), reinterpret_cast<void **>(arg)));
}
#define	DB___fop_rename_42	146
#define	DB___fop_rename_noundo_46	150
typedef struct ___fop_rename_42_args {
	uint32 type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	oldname;
	DBT	newname;
	DBT	fileid;
	uint32	appname;
} __fop_rename_42_args;

extern __DB_IMPORT DB_LOG_RECSPEC __fop_rename_42_desc[];
static inline int __fop_rename_42_read(ENV *env, void *data, __fop_rename_42_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, NULL, NULL, data, __fop_rename_42_desc, sizeof(__fop_rename_42_args), reinterpret_cast<void **>(arg)));
}
extern __DB_IMPORT DB_LOG_RECSPEC __fop_rename_noundo_46_desc[];
static inline int __fop_rename_noundo_46_read(ENV *env, void *data, __fop_rename_42_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, NULL, NULL, data, __fop_rename_noundo_46_desc, sizeof(__fop_rename_42_args), reinterpret_cast<void **>(arg)));
}
#define	DB___fop_rename	146
#define	DB___fop_rename_noundo	150
typedef struct ___fop_rename_args {
	uint32 type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	oldname;
	DBT	newname;
	DBT	dirname;
	DBT	fileid;
	uint32	appname;
} __fop_rename_args;

extern __DB_IMPORT DB_LOG_RECSPEC __fop_rename_desc[];
static inline int
__fop_rename_log(ENV *env, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32 flags,
    const DBT *oldname, const DBT *newname, const DBT *dirname, const DBT *fileid, uint32 appname)
{
	return (__log_put_record(env, NULL, txnp, ret_lsnp,
	    flags, DB___fop_rename, 0,
	    sizeof(uint32) + sizeof(uint32) + sizeof(DB_LSN) +
	    LOG_DBT_SIZE(oldname) + LOG_DBT_SIZE(newname) + LOG_DBT_SIZE(dirname) +
	    LOG_DBT_SIZE(fileid) + sizeof(uint32),
	    __fop_rename_desc,
	    oldname, newname, dirname, fileid, appname));
}

static inline int __fop_rename_read(ENV *env, void *data, __fop_rename_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, 
	    NULL, NULL, data, __fop_rename_desc, sizeof(__fop_rename_args), reinterpret_cast<void **>(arg)));
}
extern __DB_IMPORT DB_LOG_RECSPEC __fop_rename_noundo_desc[];
static inline int
__fop_rename_noundo_log(ENV *env, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32 flags,
    const DBT *oldname, const DBT *newname, const DBT *dirname, const DBT *fileid, uint32 appname)
{
	return (__log_put_record(env, NULL, txnp, ret_lsnp,
	    flags, DB___fop_rename_noundo, 0,
	    sizeof(uint32) + sizeof(uint32) + sizeof(DB_LSN) +
	    LOG_DBT_SIZE(oldname) + LOG_DBT_SIZE(newname) + LOG_DBT_SIZE(dirname) +
	    LOG_DBT_SIZE(fileid) + sizeof(uint32),
	    __fop_rename_noundo_desc,
	    oldname, newname, dirname, fileid, appname));
}

static inline int __fop_rename_noundo_read(ENV *env, 
    void *data, __fop_rename_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, 
	    NULL, NULL, data, __fop_rename_noundo_desc, sizeof(__fop_rename_args), reinterpret_cast<void **>(arg)));
}
#define	DB___fop_file_remove	141
typedef struct ___fop_file_remove_args {
	uint32 type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	real_fid;
	DBT	tmp_fid;
	DBT	name;
	uint32	appname;
	uint32	child;
} __fop_file_remove_args;

extern __DB_IMPORT DB_LOG_RECSPEC __fop_file_remove_desc[];
static inline int
__fop_file_remove_log(ENV *env, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32 flags,
    const DBT *real_fid, const DBT *tmp_fid, const DBT *name, uint32 appname, uint32 child)
{
	return (__log_put_record(env, NULL, txnp, ret_lsnp,
	    flags, DB___fop_file_remove, 0,
	    sizeof(uint32) + sizeof(uint32) + sizeof(DB_LSN) +
	    LOG_DBT_SIZE(real_fid) + LOG_DBT_SIZE(tmp_fid) + LOG_DBT_SIZE(name) +
	    sizeof(uint32) + sizeof(uint32),
	    __fop_file_remove_desc,
	    real_fid, tmp_fid, name, appname, child));
}

static inline int __fop_file_remove_read(ENV *env, 
    void *data, __fop_file_remove_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, 
	    NULL, NULL, data, __fop_file_remove_desc, sizeof(__fop_file_remove_args), reinterpret_cast<void **>(arg)));
}
#endif
