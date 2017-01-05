/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_os_ext_h_
#define	_os_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

void __os_abort(ENV *);
int __os_abspath __P((const char *));
#if defined(HAVE_REPLICATION_THREADS)
	int __os_getaddrinfo __P((ENV *, const char *, uint, const char *, const ADDRINFO *, ADDRINFO **));
#endif
#if defined(HAVE_REPLICATION_THREADS)
	void __os_freeaddrinfo __P((ENV *, ADDRINFO *));
#endif
int __os_umalloc(ENV *, size_t, void *);
int __os_urealloc(ENV *, size_t, void *);
void __os_ufree __P((ENV *, void *));
int __os_strdup __P((ENV *, const char *, void *));
int __os_calloc __P((ENV *, size_t, size_t, void *));
int __os_malloc(ENV *, size_t, void *);
int __os_realloc(ENV *, size_t, void *);
void __os_free __P((ENV *, void *));
void *__ua_memcpy __P((void *, const void *, size_t));
void __os_gettime __P((ENV *, db_timespec *, int));
int __os_fs_notzero();
int __os_support_direct_io();
int __os_support_db_register();
int __os_support_replication();
uint32 __os_cpu_count();
char *__os_ctime __P((const __time64_t *, char *));
int __os_dirlist __P((ENV *, const char *, int, char ***, int *));
void __os_dirfree __P((ENV *, char **, int));
int __os_get_errno_ret_zero();
int __os_get_errno();
int __os_get_neterr();
int __os_get_syserr();
void __os_set_errno(int);
char *__os_strerror __P((int, char *, size_t));
int __os_posix_err(int);
int __os_fileid __P((ENV *, const char *, int, uint8 *));
int __os_fdlock __P((ENV *, DB_FH *, off_t, int, int));
int __os_fsync(ENV *, DB_FH *);
int __os_getenv __P((ENV *, const char *, char **, size_t));
int __os_openhandle __P((ENV *, const char *, int, int, DB_FH **));
int __os_closehandle(ENV *, DB_FH *);
int __os_attach __P((ENV *, REGINFO *, REGION *));
int __os_detach __P((ENV *, REGINFO *, int));
int __os_mapfile __P((ENV *, char *, DB_FH *, size_t, int, void **));
int __os_unmapfile __P((ENV *, void *, size_t));
int __os_mkdir __P((ENV *, const char *, int));
int __os_open __P((ENV *, const char *, uint32, uint32, int, DB_FH **));
void __os_id __P((DB_ENV *, pid_t *, db_threadid_t*));
int __os_rename __P((ENV *, const char *, const char *, uint32));
int __os_isroot();
char *__db_rpath __P((const char *));
int __os_io __P((ENV *, int, DB_FH *, db_pgno_t, uint32, uint32, uint32, uint8 *, size_t *));
int __os_read __P((ENV *, DB_FH *, void *, size_t, size_t *));
int __os_write __P((ENV *, DB_FH *, void *, size_t, size_t *));
int __os_physwrite __P((ENV *, DB_FH *, void *, size_t, size_t *));
int __os_seek __P((ENV *, DB_FH *, db_pgno_t, uint32, off_t));
void __os_stack(ENV *);
int __os_exists __P((ENV *, const char *, int *));
int __os_ioinfo __P((ENV *, const char *, DB_FH *, uint32 *, uint32 *, uint32 *));
int __os_tmpdir __P((ENV *, uint32));
int __os_truncate __P((ENV *, DB_FH *, db_pgno_t, uint32));
void __os_unique_id __P((ENV *, uint32 *));
int __os_unlink __P((ENV *, const char *, int));
void __os_yield __P((ENV *, ulong, ulong));
#ifdef HAVE_QNX
	int __os_qnx_region_open __P((ENV *, const char *, int, int, DB_FH **));
#endif
int __os_is_winnt();
uint32 __os_cpu_count();
#ifdef HAVE_REPLICATION_THREADS
	int __os_get_neterr();
#endif

#if defined(__cplusplus)
}
#endif
#endif /* !_os_ext_h_ */
