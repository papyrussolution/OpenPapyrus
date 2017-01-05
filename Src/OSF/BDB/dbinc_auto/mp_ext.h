/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_mp_ext_h_
#define	_mp_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __memp_alloc __P((DB_MPOOL *, REGINFO *, MPOOLFILE *, size_t, roff_t *, void *));
void __memp_free __P((REGINFO *, void *));
int __memp_bhwrite __P((DB_MPOOL *, DB_MPOOL_HASH *, MPOOLFILE *, BH *, int));
int __memp_pgread __P((DB_MPOOLFILE *, BH *, int));
int __memp_pg __P((DB_MPOOLFILE *, db_pgno_t, void *, int));
int __memp_bhfree __P((DB_MPOOL *, REGINFO *, MPOOLFILE *, DB_MPOOL_HASH *, BH *, uint32));
int __memp_fget_pp __P((DB_MPOOLFILE *, db_pgno_t *, DB_TXN *, uint32, void *));
int __memp_fget __P((DB_MPOOLFILE *, db_pgno_t *, DB_THREAD_INFO *, DB_TXN *, uint32, void *));
int __memp_fcreate_pp __P((DB_ENV *, DB_MPOOLFILE **, uint32));
int __memp_fcreate __P((ENV *, DB_MPOOLFILE **));
int __memp_set_clear_len __P((DB_MPOOLFILE *, uint32));
int __memp_get_fileid __P((DB_MPOOLFILE *, uint8 *));
int __memp_set_fileid __P((DB_MPOOLFILE *, uint8 *));
int __memp_get_flags __P((DB_MPOOLFILE *, uint32 *));
int __memp_set_flags __P((DB_MPOOLFILE *, uint32, int));
int __memp_get_ftype __P((DB_MPOOLFILE *, int *));
int __memp_set_ftype __P((DB_MPOOLFILE *, int));
int __memp_set_lsn_offset __P((DB_MPOOLFILE *, int32));
int __memp_get_pgcookie __P((DB_MPOOLFILE *, DBT *));
int __memp_set_pgcookie __P((DB_MPOOLFILE *, DBT *));
int __memp_get_priority __P((DB_MPOOLFILE *, DB_CACHE_PRIORITY *));
int __memp_get_last_pgno __P((DB_MPOOLFILE *, db_pgno_t *));
char * __memp_fn __P((DB_MPOOLFILE *));
char * __memp_fns __P((DB_MPOOL *, MPOOLFILE *));
int __memp_fopen_pp __P((DB_MPOOLFILE *, const char *, uint32, int, size_t));
int __memp_fopen __P((DB_MPOOLFILE *, MPOOLFILE *, const char *, const char **, uint32, int, size_t));
int __memp_fclose_pp __P((DB_MPOOLFILE *, uint32));
int __memp_fclose __P((DB_MPOOLFILE *, uint32));
int __memp_mf_discard __P((DB_MPOOL *, MPOOLFILE *, int));
int __memp_inmemlist __P((ENV *, char ***, int *));
int __memp_fput_pp __P((DB_MPOOLFILE *, void *, DB_CACHE_PRIORITY, uint32));
int __memp_fput __P((DB_MPOOLFILE *, DB_THREAD_INFO *, void *, DB_CACHE_PRIORITY));
int __memp_unpin_buffers __P((ENV *, DB_THREAD_INFO *));
int __memp_dirty __P((DB_MPOOLFILE *, void *, DB_THREAD_INFO *, DB_TXN *, DB_CACHE_PRIORITY, uint32));
int __memp_shared __P((DB_MPOOLFILE *, void *));
int __memp_env_create(DB_ENV *);
void __memp_env_destroy(DB_ENV *);
int __memp_get_cachesize __P((DB_ENV *, uint32 *, uint32 *, int *));
int __memp_set_cachesize __P((DB_ENV *, uint32, uint32, int));
int __memp_set_config __P((DB_ENV *, uint32, int));
int __memp_get_config __P((DB_ENV *, uint32, int *));
int __memp_get_mp_max_openfd __P((DB_ENV *, int *));
int __memp_set_mp_max_openfd __P((DB_ENV *, int));
int __memp_get_mp_max_write __P((DB_ENV *, int *, db_timeout_t *));
int __memp_set_mp_max_write __P((DB_ENV *, int, db_timeout_t));
int __memp_get_mp_mmapsize __P((DB_ENV *, size_t *));
int __memp_set_mp_mmapsize __P((DB_ENV *, size_t));
int __memp_get_mp_pagesize(DB_ENV *, uint32 *);
int __memp_set_mp_pagesize(DB_ENV *, uint32);
int __memp_get_mp_tablesize(DB_ENV *, uint32 *);
int __memp_set_mp_tablesize(DB_ENV *, uint32);
int __memp_get_mp_mtxcount(DB_ENV *, uint32 *);
int __memp_set_mp_mtxcount(DB_ENV *, uint32);
int __memp_nameop __P((ENV *, uint8 *, const char *, const char *, const char *, int));
int __memp_ftruncate __P((DB_MPOOLFILE *, DB_TXN *, DB_THREAD_INFO *, db_pgno_t, uint32));
int __memp_alloc_freelist __P((DB_MPOOLFILE *, uint32, db_pgno_t **));
int __memp_free_freelist __P((DB_MPOOLFILE *));
int __memp_get_freelist __P(( DB_MPOOLFILE *, uint32 *, db_pgno_t **));
int __memp_extend_freelist __P(( DB_MPOOLFILE *, uint32 , db_pgno_t **));
int __memp_set_last_pgno __P((DB_MPOOLFILE *, db_pgno_t));
int __memp_bh_settxn __P((DB_MPOOL *, MPOOLFILE *mfp, BH *, void *));
int __memp_skip_curadj __P((DBC *, db_pgno_t));
int __memp_bh_freeze __P((DB_MPOOL *, REGINFO *, DB_MPOOL_HASH *, BH *, int *));
int __memp_bh_thaw __P((DB_MPOOL *, REGINFO *, DB_MPOOL_HASH *, BH *, BH *));
int __memp_open __P((ENV *, int));
int	__memp_init __P((ENV *, DB_MPOOL *, uint, uint32, uint));
uint32 __memp_max_regions(ENV *);
uint32 __memp_region_mutex_count(ENV *);
int __memp_env_refresh(ENV *);
int __memp_register_pp __P((DB_ENV *, int, int (*)(DB_ENV *, db_pgno_t, void *, DBT *), int (*)(DB_ENV *, db_pgno_t, void *, DBT *)));
int __memp_register __P((ENV *, int, int (*)(DB_ENV *, db_pgno_t, void *, DBT *), int (*)(DB_ENV *, db_pgno_t, void *, DBT *)));
int __memp_get_bucket __P((ENV *, MPOOLFILE *, db_pgno_t, REGINFO **, DB_MPOOL_HASH **, uint32 *));
int __memp_resize __P((DB_MPOOL *, uint32, uint32));
int __memp_get_cache_max __P((DB_ENV *, uint32 *, uint32 *));
int __memp_set_cache_max __P((DB_ENV *, uint32, uint32));
int __memp_stat_pp __P((DB_ENV *, DB_MPOOL_STAT **, DB_MPOOL_FSTAT ***, uint32));
int __memp_stat_print_pp(DB_ENV *, uint32);
int  __memp_stat_print __P((ENV *, uint32));
void __memp_stat_hash __P((REGINFO *, MPOOL *, uint32 *));
int __memp_walk_files __P((ENV *, MPOOL *, int (*) __P((ENV *, MPOOLFILE *, void *, uint32 *, uint32)), void *, uint32 *, uint32));
int __memp_discard_all_mpfs __P((ENV *, MPOOL *));
int __memp_sync_pp __P((DB_ENV *, DB_LSN *));
int __memp_sync __P((ENV *, uint32, DB_LSN *));
int __memp_fsync_pp __P((DB_MPOOLFILE *));
int __memp_fsync __P((DB_MPOOLFILE *));
int __mp_xxx_fh __P((DB_MPOOLFILE *, DB_FH **));
int __memp_sync_int __P((ENV *, DB_MPOOLFILE *, uint32, uint32, uint32 *, int *));
int __memp_mf_sync __P((DB_MPOOL *, MPOOLFILE *, int));
int __memp_trickle_pp __P((DB_ENV *, int, int *));

#if defined(__cplusplus)
}
#endif
#endif /* !_mp_ext_h_ */
