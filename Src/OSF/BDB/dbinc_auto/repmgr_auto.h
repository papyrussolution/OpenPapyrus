/* Do not edit: automatically built by gen_rec.awk. */

#ifndef	__repmgr_AUTO_H
#define	__repmgr_AUTO_H
#ifdef HAVE_REPLICATION_THREADS
// @v9.5.5 #include "dbinc/log.h"
#define	DB___repmgr_member	200
typedef struct ___repmgr_member_args {
	uint32 type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	uint32	version;
	uint32	prev_status;
	uint32	status;
	DBT	host;
	uint32	port;
} __repmgr_member_args;

extern __DB_IMPORT DB_LOG_RECSPEC __repmgr_member_desc[];
static inline int
__repmgr_member_log(ENV *env, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32 flags,
    uint32 version, uint32 prev_status, uint32 status, const DBT *host, uint32 port)
{
	return (__log_put_record(env, NULL, txnp, ret_lsnp,
	    flags, DB___repmgr_member, 0,
	    sizeof(uint32) + sizeof(uint32) + sizeof(DB_LSN) +
	    sizeof(uint32) + sizeof(uint32) + sizeof(uint32) +
	    LOG_DBT_SIZE(host) + sizeof(uint32),
	    __repmgr_member_desc,
	    version, prev_status, status, host, port));
}

static inline int __repmgr_member_read(ENV *env, 
    void *data, __repmgr_member_args **arg)
{
	*arg = NULL;
	return (__log_read_record(env, 
	    NULL, NULL, data, __repmgr_member_desc, sizeof(__repmgr_member_args), (void**)arg));
}
#endif /* HAVE_REPLICATION_THREADS */
#endif
