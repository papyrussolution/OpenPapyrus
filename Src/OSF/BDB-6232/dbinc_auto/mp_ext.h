/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_mp_ext_h_
#define	_mp_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __memp_bh_unreachable(ENV *, BH *, DB_LSN *, int);
int __memp_alloc(DB_MPOOL *, REGINFO *, MPOOLFILE *, size_t, roff_t *, void *);
void __memp_free(REGINFO *, void *);
int __memp_backup_open(ENV *, DB_MPOOLFILE *, const char *, const char *, uint32, DB_FH **, void**);
int __memp_backup_mpf(ENV *, DB_MPOOLFILE *, DB_THREAD_INFO *, db_pgno_t, db_pgno_t, DB_FH *, void *,  uint32);
int __memp_backup_close(ENV *, DB_MPOOLFILE *, const char *, DB_FH *, void *HANDLE);
int __memp_failchk(ENV *);
int __memp_bhwrite(DB_MPOOL *, DB_MPOOL_HASH *, MPOOLFILE *, BH *, int);
int __memp_pgread(DB_MPOOLFILE *, BH *, int);
int __memp_pg(DB_MPOOLFILE *, db_pgno_t, void *, int);
int __memp_bhfree(DB_MPOOL *, REGINFO *, MPOOLFILE *, DB_MPOOL_HASH *, BH *, uint32);
void __memp_bh_clear_dirty(ENV*, DB_MPOOL_HASH *, BH *);
int __memp_fget_pp(DB_MPOOLFILE *, db_pgno_t *, DB_TXN *, uint32, void *);
int FASTCALL __memp_fget(DB_MPOOLFILE *, db_pgno_t *, DB_THREAD_INFO *, DB_TXN *, uint32, void *);
int  __memp_find_obsolete_version(ENV *, BH *, DB_MPOOL_HASH *, BH **);
int __memp_fcreate_pp(DB_ENV *, DB_MPOOLFILE **, uint32);
int __memp_fcreate(ENV *, DB_MPOOLFILE **);
int __memp_set_clear_len(DB_MPOOLFILE *, uint32);
int __memp_get_fileid(DB_MPOOLFILE *, uint8 *);
int __memp_set_fileid(DB_MPOOLFILE *, uint8 *);
int __memp_get_flags(DB_MPOOLFILE *, uint32 *);
int __memp_set_flags(DB_MPOOLFILE *, uint32, int);
int __memp_get_ftype(DB_MPOOLFILE *, int *);
int __memp_set_ftype(DB_MPOOLFILE *, int);
int __memp_set_lsn_offset(DB_MPOOLFILE *, int32);
void __memp_set_maxpgno(MPOOLFILE *, uint32, uint32);
int __memp_get_pgcookie(DB_MPOOLFILE *, DBT *);
int __memp_set_pgcookie(DB_MPOOLFILE *, DBT *);
int __memp_get_priority(DB_MPOOLFILE *, DB_CACHE_PRIORITY *);
int __memp_get_last_pgno(DB_MPOOLFILE *, db_pgno_t *);
char * __memp_fn(DB_MPOOLFILE *);
char * __memp_fns(DB_MPOOL *, MPOOLFILE *);
int __memp_fopen_pp(DB_MPOOLFILE *, const char *, uint32, int, size_t);
int __memp_fopen(DB_MPOOLFILE *, MPOOLFILE *, const char *, const char **, uint32, int, size_t);
int __memp_fclose_pp(DB_MPOOLFILE *, uint32);
int __memp_fclose(DB_MPOOLFILE *, uint32);
int __memp_mf_discard(DB_MPOOL *, MPOOLFILE *, int);
int __memp_inmemlist(ENV *, char ***, int *);
void __memp_mf_mark_dead(DB_MPOOL *, MPOOLFILE *, int*);
int __memp_fput_pp(DB_MPOOLFILE *, void *, DB_CACHE_PRIORITY, uint32);
int FASTCALL __memp_fput(DB_MPOOLFILE *, DB_THREAD_INFO *, void *, DB_CACHE_PRIORITY);
int __memp_unpin_buffers(ENV *, DB_THREAD_INFO *);
int FASTCALL __memp_dirty(DB_MPOOLFILE *, void *, DB_THREAD_INFO *, DB_TXN *, DB_CACHE_PRIORITY, uint32);
int __memp_shared(DB_MPOOLFILE *, void *);
int __memp_env_create(DB_ENV *);
void __memp_env_destroy(DB_ENV *);
int __memp_get_cachesize(DB_ENV *, uint32 *, uint32 *, int *);
int __memp_set_cachesize(DB_ENV *, uint32, uint32, int);
int __memp_set_config(DB_ENV *, uint32, int);
int __memp_get_config(DB_ENV *, uint32, int *);
int __memp_get_mp_max_openfd(DB_ENV *, int *);
int __memp_set_mp_max_openfd(DB_ENV *, int);
int __memp_get_mp_max_write(DB_ENV *, int *, db_timeout_t *);
int __memp_set_mp_max_write(DB_ENV *, int, db_timeout_t);
int __memp_get_mp_mmapsize(DB_ENV *, size_t *);
int __memp_set_mp_mmapsize(DB_ENV *, size_t);
int __memp_get_mp_pagesize(DB_ENV *, uint32 *);
int __memp_set_mp_pagesize(DB_ENV *, uint32);
int __memp_get_reg_dir(DB_ENV *, const char **);
int __memp_set_reg_dir(DB_ENV *, const char *);
int __memp_get_mp_tablesize(DB_ENV *, uint32 *);
int __memp_set_mp_tablesize(DB_ENV *, uint32);
int __memp_get_mp_mtxcount(DB_ENV *, uint32 *);
int __memp_set_mp_mtxcount(DB_ENV *, uint32);
int __memp_nameop(ENV *, uint8 *, const char *, const char *, const char *, int);
int __memp_ftruncate(DB_MPOOLFILE *, DB_TXN *, DB_THREAD_INFO *, db_pgno_t, uint32);
int __memp_alloc_freelist(DB_MPOOLFILE *, uint32, db_pgno_t **);
int __memp_free_freelist(DB_MPOOLFILE *);
int __memp_get_freelist( DB_MPOOLFILE *, uint32 *, db_pgno_t **);
int __memp_extend_freelist( DB_MPOOLFILE *, uint32 , db_pgno_t **);
int __memp_set_last_pgno(DB_MPOOLFILE *, db_pgno_t);
int __memp_bh_settxn(DB_MPOOL *, MPOOLFILE *mfp, BH *, void *);
int __memp_skip_curadj(DBC *, db_pgno_t);
int __memp_bh_freeze(DB_MPOOL *, REGINFO *, DB_MPOOL_HASH *, BH *, int *);
int __memp_bh_thaw(DB_MPOOL *, REGINFO *, DB_MPOOL_HASH *, BH *, BH *);
int __memp_open(ENV *, int);
int __memp_region_detach(ENV *, DB_MPOOL *);
int	__memp_init(ENV *, DB_MPOOL *, u_int, uint32, u_int);
uint32 __memp_max_regions(ENV *);
uint32 __memp_region_mutex_count(ENV *);
int __memp_env_refresh(ENV *);
int __memp_region_bhfree(REGINFO *);
int __memp_register_pp(DB_ENV *, int, int (*)(DB_ENV *, db_pgno_t, void *, DBT *), int (*)(DB_ENV *, db_pgno_t, void *, DBT *));
int __memp_register(ENV *, int, int (*)(DB_ENV *, db_pgno_t, void *, DBT *), int (*)(DB_ENV *, db_pgno_t, void *, DBT *));
int __memp_get_bucket(ENV *, MPOOLFILE *, db_pgno_t, REGINFO **, DB_MPOOL_HASH **, uint32 *);
int __memp_resize(DB_MPOOL *, uint32, uint32);
int __memp_get_cache_max(DB_ENV *, uint32 *, uint32 *);
int __memp_set_cache_max(DB_ENV *, uint32, uint32);
int __memp_stat_pp(DB_ENV *, DB_MPOOL_STAT **, DB_MPOOL_FSTAT ***, uint32);
int __memp_stat_print_pp(DB_ENV *, uint32);
int  __memp_stat_print(ENV *, uint32);
void __memp_stat_hash(REGINFO *, MPOOL *, uint32 *);
int __memp_walk_files(ENV *, MPOOL *, int (*)(ENV *, MPOOLFILE *, void *, uint32 *, uint32), void *, uint32 *, uint32);
int __memp_discard_all_mpfs(ENV *, MPOOL *);
int __memp_sync_pp(DB_ENV *, DB_LSN *);
int __memp_sync(ENV *, uint32, DB_LSN *);
int __memp_fsync_pp(DB_MPOOLFILE *);
int __memp_fsync(DB_MPOOLFILE *);
int __mp_xxx_fh(DB_MPOOLFILE *, DB_FH **);
int __memp_sync_int(ENV *, DB_MPOOLFILE *, uint32, uint32, uint32 *, int *);
int __memp_mf_sync(DB_MPOOL *, MPOOLFILE *, int);
int __memp_purge_dead_files(ENV *);
int __memp_trickle_pp(DB_ENV *, int, int *);

#if defined(__cplusplus)
}
#endif
#endif /* !_mp_ext_h_ */
