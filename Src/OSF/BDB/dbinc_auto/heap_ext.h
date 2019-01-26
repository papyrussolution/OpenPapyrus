/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_heap_ext_h_
#define	_heap_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __heapc_init(DBC *);
int __heap_ditem(DBC *, PAGE *, uint32, uint32);
int __heap_append(DBC *, DBT *, DBT *);
int __heap_pitem(DBC *, PAGE *, uint32, uint32, DBT *, DBT *);
int FASTCALL __heapc_dup(DBC *, DBC *);
int __heapc_gsplit(DBC *, DBT *, void **, uint32 *);
int __heapc_refresh(DBC *);
int __heap_init_recover(ENV *, DB_DISTAB *);
int __heap_addrem_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __heap_pg_alloc_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __heap_trunc_meta_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __heap_trunc_page_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __heap_init_print(ENV *, DB_DISTAB *);
int __heap_pgin(DB *, db_pgno_t, void *, DBT *);
int __heap_pgout(DB *, db_pgno_t, void *, DBT *);
int __heap_mswap(ENV *, PAGE *);
int __heap_db_create(DB *);
int __heap_db_close(DB *);
int __heap_get_heapsize(DB *, uint32 *, uint32 *);
int __heap_set_heapsize(DB *, uint32, uint32, uint32);
int __heap_exist();
int __heap_open(DB *, DB_THREAD_INFO *, DB_TXN *, const char *, db_pgno_t, uint32);
int __heap_metachk(DB *, const char *, HEAPMETA *);
int __heap_read_meta(DB *, DB_THREAD_INFO *, DB_TXN *, db_pgno_t, uint32);
int __heap_new_file(DB *, DB_THREAD_INFO *, DB_TXN *, DB_FH *, const char *);
int __heap_create_region(DBC *, db_pgno_t);
int __heap_addrem_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __heap_pg_alloc_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __heap_trunc_meta_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __heap_trunc_page_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __heap_truncate(DBC *, uint32 *);
int __heap_stat(DBC *, void *, uint32);
int __heap_stat_print(DBC *, uint32);
void __heap_print_cursor(DBC *);
int __heap_stat_callback(DBC *, PAGE *, void *, int *);
int __heap_traverse(DBC *, int (*)(DBC *, PAGE *, void *, int *), void *);
int __db_no_heap_am(ENV *);
int __heap_vrfy_meta(DB *, VRFY_DBINFO *, const HEAPMETA *, db_pgno_t, uint32);
int __heap_vrfy(DB *, VRFY_DBINFO *, PAGE *, db_pgno_t, uint32);
int __heap_vrfy_structure(DB *, VRFY_DBINFO *, uint32);
int __heap_salvage(DB *, VRFY_DBINFO *, db_pgno_t, PAGE *, void *, int (*)(void *, const void *), uint32);
int __heap_meta2pgset(DB *, VRFY_DBINFO *, HEAPMETA *, DB *);

#if defined(__cplusplus)
}
#endif
#endif /* !_heap_ext_h_ */
