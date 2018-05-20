/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_txn_ext_h_
#define	_txn_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __txn_begin_pp(DB_ENV *, DB_TXN *, DB_TXN **, uint32);
int __txn_allocate(ENV *, DB_TXN **);
int __txn_begin(ENV *, DB_THREAD_INFO *, DB_TXN *, DB_TXN **, uint32);
int __txn_recycle_id(ENV *, int);
int __txn_continue(ENV *, DB_TXN *, TXN_DETAIL *, DB_THREAD_INFO *, int);
int __txn_commit(DB_TXN *, uint32);
int __txn_abort(DB_TXN *);
int __txn_discard_int(DB_TXN *, uint32 flags);
int __txn_prepare(DB_TXN *, uint8 *);
uint32 __txn_id(DB_TXN *);
int __txn_get_name(DB_TXN *, const char **);
int __txn_set_name(DB_TXN *, const char *);
int __txn_get_priority(DB_TXN *, uint32 *);
int __txn_set_priority(DB_TXN *, uint32);
int  __txn_set_timeout(DB_TXN *, db_timeout_t, uint32);
int __txn_activekids(ENV *, uint32, DB_TXN *);
int __txn_force_abort(ENV *, uint8 *);
int __txn_preclose(ENV *);
int __txn_reset(ENV *);
int __txn_applied_pp(DB_ENV *, DB_TXN_TOKEN *, db_timeout_t, uint32);
int __txn_init_timeout(DB_TXN *, DB_TXN *);
int __txn_slice_begin(DB_TXN *, DB_TXN **, db_slice_t);
int __txn_multislice(DB_TXN *);
int __txn_init_recover(ENV *, DB_DISTAB *);
int __txn_regop_42_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __txn_regop_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __txn_ckp_42_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __txn_ckp_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __txn_child_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __txn_xa_regop_42_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __txn_prepare_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __txn_recycle_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __txn_init_print(ENV *, DB_DISTAB *);
int __txn_checkpoint_pp(DB_ENV *, uint32, uint32, uint32);
int __txn_checkpoint(ENV *, uint32, uint32, uint32);
int __txn_getactive(ENV *, DB_LSN *);
int __txn_getckp(ENV *, DB_LSN *);
int __txn_updateckp(ENV *, DB_LSN *);
int __txn_failchk(ENV *);
int __txn_env_create(DB_ENV *);
void __txn_env_destroy(DB_ENV *);
int __txn_get_tx_max(DB_ENV *, uint32 *);
int __txn_set_tx_max(DB_ENV *, uint32);
int __txn_get_tx_timestamp(DB_ENV *, time_t *);
int __txn_set_tx_timestamp(DB_ENV *, time_t *);
int __txn_regop_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __txn_prepare_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __txn_ckp_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __txn_child_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __txn_restore_txn(ENV *, DB_LSN *, __txn_prepare_args *);
int __txn_recycle_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __txn_recover_pp(DB_ENV *, DB_PREPLIST *, long, long *, uint32);
int __txn_recover(ENV *, DB_PREPLIST *, long, long *, uint32);
int __txn_get_prepared(ENV *, XID *, DB_PREPLIST *, long, long *, uint32);
int __txn_openfiles(ENV *, DB_THREAD_INFO *, DB_LSN *, int);
int __txn_open(ENV *);
int __txn_region_detach(ENV *, DB_TXNMGR *);
int __txn_findlastckp(ENV *, DB_LSN *, DB_LSN *);
int __txn_env_refresh(ENV *);
uint32 __txn_region_mutex_count(ENV *);
uint32 __txn_region_mutex_max(ENV *);
size_t __txn_region_size(ENV *);
size_t __txn_region_max(ENV *);
int __txn_id_set(ENV *, uint32, uint32);
int __txn_get_readers(ENV *, DB_LSN **, int *);
int __txn_add_buffer(ENV *, TXN_DETAIL *);
int __txn_remove_buffer(ENV *, TXN_DETAIL *, db_mutex_t);
int __txn_stat_pp(DB_ENV *, DB_TXN_STAT **, uint32);
int __txn_stat_print_pp(DB_ENV *, uint32);
int  __txn_stat_print(ENV *, uint32);
int __txn_closeevent(ENV *, DB_TXN *, DB *);
int __txn_remevent(ENV *, DB_TXN *, const char *, uint8 *, int);
void __txn_remrem(ENV *, DB_TXN *, const char *);
int __txn_lockevent(ENV *, DB_TXN *, DB *, DB_LOCK *, DB_LOCKER *);
void __txn_remlock(ENV *, DB_TXN *, DB_LOCK *, DB_LOCKER *);
int __txn_doevents(ENV *, DB_TXN *, int, int);
int __txn_record_fname(ENV *, DB_TXN *, FNAME *);
int __txn_dref_fname(ENV *, DB_TXN *);
void __txn_reset_fe_watermarks(DB_TXN *);
void __txn_remove_fe_watermark(DB_TXN *,DB *);
void __txn_add_fe_watermark(DB_TXN *, DB *, db_pgno_t);
int __txn_flush_fe_files(DB_TXN *);
int __txn_pg_above_fe_watermark(DB_TXN*, MPOOLFILE*, db_pgno_t);

#if defined(__cplusplus)
}
#endif
#endif /* !_txn_ext_h_ */
