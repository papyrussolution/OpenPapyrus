/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_hash_ext_h_
#define	_hash_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __ham_quick_delete(DBC *);
int __hamc_init(DBC *);
int __hamc_count __P((DBC *, db_recno_t *));
int __hamc_cmp __P((DBC *, DBC *, int *));
int __hamc_dup __P((DBC *, DBC *));
int  __ham_contract_table __P((DBC *, DB_COMPACT *));
uint32 __ham_call_hash __P((DBC *, uint8 *, uint32));
int  __ham_overwrite __P((DBC *, DBT *, uint32));
int  __ham_lookup __P((DBC *, const DBT *, uint32, db_lockmode_t, db_pgno_t *));
int __ham_init_dbt __P((ENV *, DBT *, uint32, void **, uint32 *));
int __hamc_update __P((DBC *, uint32, db_ham_curadj, int));
int __ham_get_clist __P((DB *, db_pgno_t, uint32, DBC ***));
int __ham_init_recover __P((ENV *, DB_DISTAB *));
int __ham_insdel_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_insdel_42_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_newpage_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_splitdata_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_replace_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_replace_42_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_copypage_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_metagroup_42_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_metagroup_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_groupalloc_42_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_groupalloc_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_changeslot_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_contract_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_curadj_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_chgpg_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_init_print __P((ENV *, DB_DISTAB *));
int __ham_compact_int __P((DBC *, DBT *, DBT *, uint32, DB_COMPACT *, int *, uint32));
int __ham_compact_bucket __P((DBC *, DB_COMPACT *, int *));
int __ham_compact_hash __P((DB *, DB_THREAD_INFO *, DB_TXN *, DB_COMPACT *));
int __ham_pgin __P((DB *, db_pgno_t, void *, DBT *));
int __ham_pgout __P((DB *, db_pgno_t, void *, DBT *));
int __ham_mswap __P((ENV *, void *));
int __ham_add_dup __P((DBC *, DBT *, uint32, db_pgno_t *));
int __ham_dup_convert(DBC *);
int __ham_make_dup __P((ENV *, const DBT *, DBT *d, void **, uint32 *));
void __ham_dsearch __P((DBC *, DBT *, uint32 *, int *, uint32));
uint32 __ham_func2 __P((DB *, const void *, uint32));
uint32 __ham_func3 __P((DB *, const void *, uint32));
uint32 __ham_func4 __P((DB *, const void *, uint32));
uint32 __ham_func5 __P((DB *, const void *, uint32));
uint32 __ham_test __P((DB *, const void *, uint32));
int __ham_get_meta(DBC *);
int __ham_release_meta(DBC *);
int __ham_dirty_meta __P((DBC *, uint32));
int __ham_return_meta __P((DBC *, uint32, DBMETA **));
int __ham_db_create __P((DB *));
int __ham_db_close __P((DB *));
int __ham_get_h_ffactor __P((DB *, uint32 *));
int __ham_set_h_compare __P((DB *, int (*)(DB *, const DBT *, const DBT *)));
int __ham_get_h_nelem __P((DB *, uint32 *));
void __ham_copy_config __P((DB *, DB*, uint32));
int __ham_open __P((DB *, DB_THREAD_INFO *, DB_TXN *, const char * name, db_pgno_t, uint32));
int __ham_metachk __P((DB *, const char *, HMETA *));
int __ham_new_file __P((DB *, DB_THREAD_INFO *, DB_TXN *, DB_FH *, const char *));
int __ham_new_subdb __P((DB *, DB *, DB_THREAD_INFO *, DB_TXN *));
int __ham_item __P((DBC *, db_lockmode_t, db_pgno_t *));
int __ham_item_reset(DBC *);
int __ham_item_init(DBC *);
int __ham_item_last __P((DBC *, db_lockmode_t, db_pgno_t *));
int __ham_item_first __P((DBC *, db_lockmode_t, db_pgno_t *));
int __ham_item_prev __P((DBC *, db_lockmode_t, db_pgno_t *));
int __ham_item_next __P((DBC *, db_lockmode_t, db_pgno_t *));
int __ham_insertpair __P((DBC *, PAGE *p, db_indx_t *indxp, const DBT *, const DBT *, uint32, uint32));
int __ham_getindex __P((DBC *, PAGE *, const DBT *, uint32, int *, db_indx_t *));
int __ham_verify_sorted_page __P((DBC *, PAGE *));
int __ham_sort_page_cursor __P((DBC *, PAGE *));
int __ham_sort_page __P((DBC *,  PAGE **, PAGE *));
int __ham_del_pair __P((DBC *, int, PAGE *));
int __ham_replpair __P((DBC *, DBT *, uint32));
void __ham_onpage_replace __P((DB *, PAGE *, uint32, int32, uint32,  int, DBT *));
int __ham_merge_pages __P((DBC *, uint32, uint32, DB_COMPACT *));
int __ham_split_page __P((DBC *, uint32, uint32));
int __ham_add_el __P((DBC *, const DBT *, const DBT *, uint32));
int __ham_copypair __P((DBC *, PAGE *, uint32, PAGE *, db_indx_t *, int));
int __ham_add_ovflpage __P((DBC *, PAGE **));
int __ham_get_cpage __P((DBC *, db_lockmode_t));
int __ham_next_cpage __P((DBC *, db_pgno_t));
int __ham_lock_bucket __P((DBC *, db_lockmode_t));
void __ham_dpair __P((DB *, PAGE *, uint32));
int __ham_insdel_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_insdel_42_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_newpage_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_replace_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_replace_42_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_splitdata_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_copypage_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_metagroup_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_contract_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_groupalloc_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_changeslot_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_curadj_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_chgpg_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_metagroup_42_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_groupalloc_42_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __ham_reclaim __P((DB *, DB_THREAD_INFO *, DB_TXN *txn, uint32));
int __ham_truncate __P((DBC *, uint32 *));
int __ham_stat __P((DBC *, void *, uint32));
int __ham_stat_print __P((DBC *, uint32));
void __ham_print_cursor(DBC *);
int __ham_traverse __P((DBC *, db_lockmode_t, int (*)(DBC *, PAGE *, void *, int *), void *, int));
int __db_no_hash_am(ENV *);
int __ham_30_hashmeta __P((DB *, char *, uint8 *));
int __ham_30_sizefix __P((DB *, DB_FH *, char *, uint8 *));
int __ham_31_hashmeta __P((DB *, char *, uint32, DB_FH *, PAGE *, int *));
int __ham_31_hash __P((DB *, char *, uint32, DB_FH *, PAGE *, int *));
int __ham_46_hashmeta __P((DB *, char *, uint32, DB_FH *, PAGE *, int *));
int __ham_46_hash __P((DB *, char *, uint32, DB_FH *, PAGE *, int *));
int __ham_vrfy_meta __P((DB *, VRFY_DBINFO *, HMETA *, db_pgno_t, uint32));
int __ham_vrfy __P((DB *, VRFY_DBINFO *, PAGE *, db_pgno_t, uint32));
int __ham_vrfy_structure __P((DB *, VRFY_DBINFO *, db_pgno_t, uint32));
int __ham_vrfy_hashing __P((DBC *, uint32, HMETA *, uint32, db_pgno_t, uint32, uint32 (*) __P((DB *, const void *, uint32))));
int __ham_salvage __P((DB *, VRFY_DBINFO *, db_pgno_t, PAGE *, void *, int (*)(void *, const void *), uint32));
int __ham_meta2pgset __P((DB *, VRFY_DBINFO *, HMETA *, uint32, DB *));

#if defined(__cplusplus)
}
#endif
#endif /* !_hash_ext_h_ */
