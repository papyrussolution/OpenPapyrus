/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_common_ext_h_
#define	_common_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

void __clock_set_expires(ENV *, db_timespec *, db_timeout_t);
int __clock_expired(ENV *, db_timespec *, db_timespec *);
int __crypto_region_init(ENV *);
int __db_isbigendian();
int __db_byteorder(ENV *, int);
uint32 __db_compress_count_int(uint64);
int __db_compress_int(uint8 *, uint64);
uint32 __db_decompress_count_int(const uint8 *);
int __db_decompress_int(const uint8 *, uint64 *);
int __db_decompress_int32(const uint8 *, uint32 *);
int FASTCALL __db_fchk(ENV *, const char *, uint32, uint32);
int FASTCALL __db_fcchk(ENV *, const char *, uint32, uint32, uint32);
int FASTCALL __db_ferr(const ENV *, const char *, int);
int FASTCALL __db_fnl(const ENV *, const char *);
int FASTCALL __db_pgerr(DB *, db_pgno_t, int);
int FASTCALL __db_pgfmt(ENV *, db_pgno_t);
#ifdef DIAGNOSTIC
void __db_assert(ENV *, const char *, const char *, int);
#endif
int FASTCALL __env_panic_msg(ENV *);
int FASTCALL __env_panic(ENV *, int);
char * __db_unknown_error(int);
void __db_syserr(const ENV *, int, const char *, ...) __attribute__ ((__format__ (__printf__, 3, 4)));
void __db_err(const ENV *, int, const char *, ...) __attribute__ ((__format__ (__printf__, 3, 4)));
void __db_errx(const ENV *, const char *, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
void __db_errcall(const DB_ENV *, int, db_error_set_t, const char *, va_list);
void __db_errfile(const DB_ENV *, int, db_error_set_t, const char *, va_list);
void __db_msgadd(ENV *, DB_MSGBUF *, const char *, ...) __attribute__ ((__format__ (__printf__, 3, 4)));
void __db_msgadd_ap(ENV *, DB_MSGBUF *, const char *, va_list);
void __db_msg(const ENV *, const char *, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
void __db_repmsg(const ENV *, const char *, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
int FASTCALL __db_unknown_flag(ENV *, char *, uint32);
int FASTCALL __db_unknown_type(ENV *, char *, DBTYPE);
int FASTCALL __db_unknown_path(ENV *, char *);
int FASTCALL __db_check_txn(DB *, DB_TXN *, DB_LOCKER *, int);
int __db_txn_deadlock_err(ENV *, DB_TXN *);
int FASTCALL __db_not_txn_env(ENV *);
int __db_rec_toobig(ENV *, uint32, uint32);
int __db_rec_repl(ENV *, uint32, uint32);
int __dbc_logging(DBC *);
int __db_check_lsn(ENV *, DB_LSN *, DB_LSN *);
int __db_rdonly(const ENV *, const char *);
int __db_space_err(const DB *);
int __db_failed(const ENV *, const char *, pid_t, db_threadid_t);
int __db_getlong(DB_ENV *, const char *, char *, long, long, long *);
int __db_getulong(DB_ENV *, const char *, char *, ulong, ulong, ulong *);
void __db_idspace(uint32 *, int, uint32 *, uint32 *);
uint32 FASTCALL __db_log2(uint32);
uint32 __db_tablesize(uint32);
void __db_hashinit(void *, uint32);
int    FASTCALL __dbt_usercopy(ENV *, DBT *);
void   FASTCALL __dbt_userfree(ENV *, DBT *, DBT *, DBT *);
int    __db_mkpath(ENV *, const char *);
uint32 __db_openflags(int);
int __db_util_arg(char *, char *, int *, char ***);
int __db_util_cache(DB *, uint32 *, int *);
int __db_util_logset(const char *, char *);
void __db_util_siginit();
int __db_util_interrupted();
void __db_util_sigresend();
int __db_zero_fill(ENV *, DB_FH *);
int __db_zero_extend(ENV *, DB_FH *, db_pgno_t, db_pgno_t, uint32);

#if defined(__cplusplus)
}
#endif
#endif /* !_common_ext_h_ */
