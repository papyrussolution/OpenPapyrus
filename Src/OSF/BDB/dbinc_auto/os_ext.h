/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_os_ext_h_
#define	_os_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

void __os_abort(ENV *);
// @v11.7.10 (replaced with SPathStruc::IsWindowsPathPrefix) int __os_abspath_Removed(const char *);
#if defined(HAVE_REPLICATION_THREADS)
	int __os_getaddrinfo(ENV *, const char *, uint, const char *, const ADDRINFO *, ADDRINFO **);
#endif
#if defined(HAVE_REPLICATION_THREADS)
	void __os_freeaddrinfo(ENV *, ADDRINFO *);
#endif
int FASTCALL __os_umalloc(ENV *, size_t, void *);
int __os_urealloc(ENV *, size_t, void *);
void FASTCALL __os_ufree(ENV *, void *);
int __os_strdup(ENV *, const char *, void *);
int FASTCALL __os_calloc(ENV *, size_t, size_t, void *);
int FASTCALL __os_malloc(ENV *, size_t, void *);
int FASTCALL __os_realloc(ENV *, size_t, void *);
void FASTCALL __os_free(ENV *, void *);
void *__ua_memcpy(void *, const void *, size_t);
void __os_gettime(ENV *, db_timespec *, int);
int __os_fs_notzero();
int __os_support_direct_io();
int __os_support_db_register();
int __os_support_replication();
uint32 __os_cpu_count();
char * FASTCALL __os_ctime(const __time64_t *, char *);
int __os_dirlist(ENV *, const char *, int, char ***, int *);
void __os_dirfree(ENV *, char **, int);
int __os_get_errno_ret_zero();
int __os_get_errno();
int __os_get_neterr();
int __os_get_syserr();
void FASTCALL __os_set_errno(int);
char *__os_strerror(int, char *, size_t);
int __os_posix_err(int);
int __os_fileid(ENV *, const char *, int, uint8 *);
int __os_fdlock(ENV *, DB_FH *, off_t, int, int);
int __os_fsync(ENV *, DB_FH *);
int __os_getenv(ENV *, const char *, char **, size_t);
int __os_openhandle(ENV *, const char *, int, int, DB_FH **);
int FASTCALL __os_closehandle(ENV *, DB_FH *);
int __os_attach(ENV *, REGINFO *, REGION *);
int __os_detach(ENV *, REGINFO *, int);
int __os_mapfile(ENV *, const char *, DB_FH *, size_t, int, void **);
int __os_unmapfile(ENV *, void *, size_t);
int __os_mkdir(ENV *, const char *, int);
int __os_open(ENV *, const char *, uint32, uint32, int, DB_FH **);
void __os_id(DB_ENV *, pid_t *, db_threadid_t*);
int __os_rename(ENV *, const char *, const char *, uint32);
int __os_isroot();
char *__db_rpath(const char *);
int __os_io(ENV *, int, DB_FH *, db_pgno_t, uint32, uint32, uint32, uint8 *, size_t *);
int __os_read(ENV *, DB_FH *, void *, size_t, size_t *);
int __os_write(ENV *, DB_FH *, const void *, size_t, size_t *);
int __os_physwrite(ENV *, DB_FH *, const void *, size_t, size_t *);
int __os_seek(ENV *, DB_FH *, db_pgno_t, uint32, off_t);
void __os_stack(ENV *);
int __os_exists(ENV *, const char *, int *);
int __os_ioinfo(ENV *, const char *, DB_FH *, uint32 *, uint32 *, uint32 *);
int __os_tmpdir(ENV *, uint32);
int __os_truncate(ENV *, DB_FH *, db_pgno_t, uint32);
void __os_unique_id(ENV *, uint32 *);
int __os_unlink(ENV *, const char *, int);
void __os_yield(ENV *, ulong, ulong);
#ifdef HAVE_QNX
	int __os_qnx_region_open(ENV *, const char *, int, int, DB_FH **);
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
