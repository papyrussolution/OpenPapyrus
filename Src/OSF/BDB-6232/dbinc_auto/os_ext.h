/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_os_ext_h_
#define	_os_ext_h_

#if defined(__cplusplus)
// @sobolev extern "C" {
#endif

void __os_abort(const ENV *);
int __os_abspath(const char *);
#if defined(HAVE_REPLICATION_THREADS)
int __os_getaddrinfo(ENV *, const char *, u_int, const char *, const ADDRINFO *, ADDRINFO **);
#endif
#if defined(HAVE_REPLICATION_THREADS)
void __os_freeaddrinfo(ENV *, ADDRINFO *);
#endif
int __os_umalloc(ENV *, size_t, void *);
int __os_urealloc(ENV *, size_t, void *);
void __os_ufree(ENV *, void *);
int __os_strdup(ENV *, const char *, void *);
int __os_calloc(const ENV *, size_t, size_t, void *);
int __os_malloc(const ENV *, size_t, void *);
int __os_realloc(const ENV *, size_t, void *);
void __os_free(const ENV *, void *);
void *__ua_memcpy(void *, const void *, size_t);
void __os_gettime(const ENV *, db_timespec *, int);
int __os_fs_notzero(void);
int __os_support_direct_io(void);
int __os_support_db_register(void);
int __os_support_replication(void);
uint32 __os_cpu_count(void);
char *__os_ctime(const time_t *, char *);
int __os_dirlist(ENV *, const char *, int, char ***, int *);
void __os_dirfree(ENV *, char **, int);
int __os_get_errno_ret_zero(void);
int __os_get_errno(void);
int __os_get_neterr(void);
int __os_get_syserr(void);
void __os_set_errno(int);
char *__os_strerror(int, char *, size_t);
int __os_posix_err(int);
int __os_fileid(ENV *, const char *, int, uint8 *);
int __os_fdlock(ENV *, DB_FH *, off_t, int, int);
int __os_fsync(ENV *, DB_FH *);
int __os_getenv(ENV *, const char *, char **, size_t);
int __os_openhandle(ENV *, const char *, int, int, DB_FH **);
int __os_closehandle(ENV *, DB_FH *);
int __os_attach(ENV *, REGINFO *, REGION *);
int __os_detach(ENV *, REGINFO *, int);
int __os_mapfile(ENV *, char *, DB_FH *, size_t, int, void **);
int __os_unmapfile(ENV *, void *, size_t);
int __os_mkdir(ENV *, const char *, int);
int __os_open(ENV *, const char *, uint32, uint32, int, DB_FH **);
int __os_concat_path(char *, size_t, const char *, const char *);
void __os_id(DB_ENV *, pid_t *, db_threadid_t*);
int __os_rename(ENV *, const char *, const char *, uint32);
int __os_rmdir(ENV *, const char *);
int __os_isroot(void);
char *__db_rpath(const char *);
int __os_io(ENV *, int, DB_FH *, db_pgno_t, uint32, uint32, uint32, uint8 *, size_t *);
int __os_read(ENV *, DB_FH *, void *, size_t, size_t *);
int __os_write(ENV *, DB_FH *, void *, size_t, size_t *);
int __os_physwrite(ENV *, DB_FH *, void *, size_t, size_t *);
int __os_seek(ENV *, DB_FH *, db_pgno_t, uint32, off_t);
void __os_stack(const ENV *);
void __os_stack_top(const ENV *, unsigned, unsigned);
void __os_stack_text(const ENV *, char *, size_t, unsigned, unsigned);
int __os_stack_save(const ENV *, unsigned, void **);
void __os_stack_msgadd(const ENV *, DB_MSGBUF *, unsigned, unsigned, void **);
int __os_exists(ENV *, const char *, int *);
int __os_ioinfo(ENV *, const char *, DB_FH *, uint32 *, uint32 *, uint32 *);
int __os_tmpdir(ENV *, uint32);
int __os_truncate(ENV *, DB_FH *, db_pgno_t, uint32, off_t);
void __os_unique_id(ENV *, uint32 *);
void __os_srandom(u_int);
u_int __os_random(void);
int __os_unlink(ENV *, const char *, int);
void __os_yield(ENV *, u_long, u_long);
#ifdef HAVE_QNX
int __os_qnx_region_open(ENV *, const char *, int, int, DB_FH **);
#endif
#ifdef DB_WINCE
FILE * __ce_freopen(const char *, const char *, FILE *);
#endif
#ifdef DB_WINCE
struct tm * __ce_gmtime(const time_t *);
#endif
#ifdef DB_WINCE
struct tm * localtime(const time_t *);
#endif
#ifdef DB_WINCE
time_t __ce_mktime(struct tm *);
#endif
#ifdef DB_WINCE
int __ce_remove(const char *path);
#endif
int __os_is_winnt(void);
uint32 __os_cpu_count(void);
#ifdef HAVE_REPLICATION_THREADS
int __os_get_neterr(void);
#endif

#if defined(__cplusplus)
// @sobolev }
#endif
#endif /* !_os_ext_h_ */
