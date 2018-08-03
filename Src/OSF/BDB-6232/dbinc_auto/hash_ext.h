/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_hash_ext_h_
#define	_hash_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __ham_quick_delete(DBC *);
int __hamc_init(DBC *);
int __hamc_count(DBC *, db_recno_t *);
int __hamc_cmp(DBC *, DBC *, int *);
int __hamc_dup(DBC *, DBC *);
int  __ham_contract_table(DBC *, DB_COMPACT *);
uint32 __ham_call_hash(DBC *, uint8 *, uint32);
int  __ham_overwrite(DBC *, DBT *, uint32);
int  __ham_lookup(DBC *, const DBT *, uint32, db_lockmode_t, db_pgno_t *);
int __ham_init_dbt(ENV *, DBT *, uint32, void **, uint32 *);
int __hamc_update(DBC *, uint32, db_ham_curadj, int);
int __ham_get_clist(DB *, db_pgno_t, uint32, DBC ***);
int __ham_init_recover(ENV *, DB_DISTAB *);
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
int __ham_init_print(ENV *, DB_DISTAB *);
int __ham_compact_int(DBC *, DBT *, DBT *, uint32, DB_COMPACT *, int *, uint32);
int __ham_compact_bucket(DBC *, DB_COMPACT *, int *);
int __ham_compact_hash(DB *, DB_THREAD_INFO *, DB_TXN *, DB_COMPACT *);
int __ham_pgin(DB *, db_pgno_t, void *, DBT *);
int __ham_pgout(DB *, db_pgno_t, void *, DBT *);
int __ham_mswap(ENV *, void *);
int __ham_add_dup(DBC *, DBT *, uint32, db_pgno_t *);
int __ham_dup_convert(DBC *);
int __ham_make_dup(ENV *, const DBT *, DBT *d, void **, uint32 *);
void __ham_dsearch(DBC *, DBT *, uint32 *, int *, uint32);
uint32 __ham_func2(DB *, const void *, uint32);
uint32 __ham_func3(DB *, const void *, uint32);
uint32 __ham_func4(DB *, const void *, uint32);
uint32 __ham_func5(DB *, const void *, uint32);
uint32 __ham_func6(DB *, const void *, uint32);
uint32 __ham_test(DB *, const void *, uint32);
int __ham_get_meta(DBC *);
int FASTCALL __ham_release_meta(DBC *);
int __ham_dirty_meta(DBC *, uint32);
int __ham_return_meta(DBC *, uint32, DBMETA **);
int __ham_db_create(DB *);
int __ham_db_close(DB *);
int __ham_get_h_ffactor(DB *, uint32 *);
int __ham_set_h_compare(DB *, int (*)(DB *, const DBT *, const DBT *, size_t *));
int __ham_get_h_nelem(DB *, uint32 *);
void __ham_copy_config(const DB *, DB*, uint32);
int __ham_open(DB *, DB_THREAD_INFO *, DB_TXN *, const char * name, db_pgno_t, uint32);
int __ham_metachk(DB *, const char *, HMETA *);
int __ham_new_file(DB *, DB_THREAD_INFO *, DB_TXN *, DB_FH *, const char *);
int __ham_new_subdb(DB *, DB *, DB_THREAD_INFO *, DB_TXN *);
int __ham_item(DBC *, db_lockmode_t, db_pgno_t *);
int __ham_item_reset(DBC *);
int __ham_item_init(DBC *);
int __ham_item_last(DBC *, db_lockmode_t, db_pgno_t *);
int __ham_item_first(DBC *, db_lockmode_t, db_pgno_t *);
int __ham_item_prev(DBC *, db_lockmode_t, db_pgno_t *);
int __ham_item_next(DBC *, db_lockmode_t, db_pgno_t *);
int __ham_insertpair(DBC *, PAGE *p, db_indx_t *indxp, const DBT *, const DBT *, uint32, uint32);
int __ham_getindex(DBC *, PAGE *, const DBT *, uint32, int *, db_indx_t *);
int __ham_verify_sorted_page(DBC *, PAGE *);
int __ham_sort_page_cursor(DBC *, PAGE *);
int __ham_sort_page(DBC *,  PAGE **, PAGE *);
int __ham_del_pair(DBC *, int, PAGE *);
int __ham_replpair(DBC *, DBT *, uint32);
void __ham_onpage_replace(DB *, PAGE *, uint32, int32, uint32,  int, DBT *);
int __ham_merge_pages(DBC *, uint32, uint32, DB_COMPACT *);
int __ham_split_page(DBC *, uint32, uint32);
int __ham_add_el(DBC *, const DBT *, const DBT *, uint32);
int __ham_copypair(DBC *, PAGE *, uint32, PAGE *, db_indx_t *, int);
int __ham_add_ovflpage(DBC *, PAGE **);
int __ham_get_cpage(DBC *, db_lockmode_t);
int __ham_next_cpage(DBC *, db_pgno_t);
int __ham_lock_bucket(DBC *, db_lockmode_t);
void __ham_dpair(DB *, PAGE *, uint32);
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
int __ham_reclaim(DB *, DB_THREAD_INFO *, DB_TXN *txn, uint32);
int __ham_truncate(DBC *, uint32 *);
int __ham_stat(DBC *, void *, uint32);
int __ham_stat_print(DBC *, uint32);
void __ham_print_cursor(DBC *);
int __ham_traverse(DBC *, db_lockmode_t, int (*)(DBC *, PAGE *, void *, int *), void *, int);
int __db_no_hash_am(ENV *);
int __ham_30_hashmeta(DB *, char *, uint8 *);
int __ham_30_sizefix(DB *, DB_FH *, char *, uint8 *);
int __ham_31_hashmeta(DB *, char *, uint32, DB_FH *, PAGE *, int *);
int __ham_31_hash(DB *, char *, uint32, DB_FH *, PAGE *, int *);
int __ham_46_hashmeta(DB *, char *, uint32, DB_FH *, PAGE *, int *);
int __ham_46_hash(DB *, char *, uint32, DB_FH *, PAGE *, int *);
int __ham_60_hashmeta(DB *, char *, uint32, DB_FH *, PAGE *, int *);
int __ham_60_hash(DB *, char *, uint32, DB_FH *, PAGE *, int *);
int __ham_vrfy_meta(DB *, VRFY_DBINFO *, HMETA *, db_pgno_t, uint32);
int __ham_vrfy(DB *, VRFY_DBINFO *, PAGE *, db_pgno_t, uint32);
int __ham_vrfy_structure(DB *, VRFY_DBINFO *, db_pgno_t, uint32);
int __ham_vrfy_hashing(DBC *, uint32, HMETA *, uint32, db_pgno_t, uint32, uint32 (*)(DB *, const void *, uint32));
int __ham_salvage(DB *, VRFY_DBINFO *, db_pgno_t, PAGE *, void *, int (*)(void *, const void *), uint32);
int __ham_meta2pgset(DB *, VRFY_DBINFO *, HMETA *, uint32, DB *);

#if defined(__cplusplus)
}
#endif
#endif /* !_hash_ext_h_ */
