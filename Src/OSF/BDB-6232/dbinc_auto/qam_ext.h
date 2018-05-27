/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_qam_ext_h_
#define	_qam_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __qam_position(DBC *, db_recno_t *, uint32, int *);
int __qam_pitem(DBC *,  QPAGE *, uint32, db_recno_t, DBT *);
int __qam_append(DBC *, DBT *, DBT *);
int __qamc_dup(DBC *, DBC *);
int __qamc_init(DBC *);
int __qam_truncate(DBC *, uint32 *);
int __qam_delete(DBC *,  DBT *, uint32);
int __qam_init_recover(ENV *, DB_DISTAB *);
int __qam_incfirst_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __qam_mvptr_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __qam_del_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __qam_add_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __qam_delext_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __qam_init_print(ENV *, DB_DISTAB *);
int __qam_mswap(ENV *, PAGE *);
int __qam_pgin_out(ENV *, db_pgno_t, void *, DBT *);
int __qam_fprobe(DBC *, db_pgno_t, void *, qam_probe_mode, DB_CACHE_PRIORITY, uint32);
int __qam_fclose(DB *, db_pgno_t);
int __qam_fremove(DB *, db_pgno_t);
int __qam_sync(DB *);
int __qam_gen_filelist(DB *, DB_THREAD_INFO *, QUEUE_FILELIST **);
int __qam_extent_names(ENV *, char *, char ***);
void __qam_exid(DB *, uint8 *, uint32);
int __qam_nameop(DB *, DB_TXN *, const char *, qam_name_op);
int __qam_lsn_reset(DB *, DB_THREAD_INFO *);
int __qam_backup_extents(DB *, DB_THREAD_INFO *, const char *, uint32);
int __qam_db_create(DB *);
int __qam_db_close(DB *, uint32);
int __qam_get_extentsize(DB *, uint32 *);
int __queue_pageinfo(DB *, db_pgno_t *, db_pgno_t *, int *, int, uint32);
int __db_prqueue(DB *, uint32);
int __qam_remove(DB *, DB_THREAD_INFO *, DB_TXN *, const char *, const char *, uint32);
int __qam_rename(DB *, DB_THREAD_INFO *, DB_TXN *, const char *, const char *, const char *);
void __qam_map_flags(DB *, uint32 *, uint32 *);
int __qam_set_flags(DB *, uint32 *flagsp);
int __qam_open(DB *, DB_THREAD_INFO *, DB_TXN *, const char *, db_pgno_t, int, uint32);
int __qam_set_ext_data(DB*, const char *);
int __qam_metachk(DB *, const char *, QMETA *);
int __qam_new_file(DB *, DB_THREAD_INFO *, DB_TXN *, DB_FH *, const char *);
int __qam_incfirst_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __qam_mvptr_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __qam_del_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __qam_delext_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __qam_add_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __qam_stat(DBC *, void *, uint32);
int __qam_stat_print(DBC *, uint32);
int __db_no_queue_am(ENV *);
int __qam_31_qammeta(DB *, char *, uint8 *);
int __qam_32_qammeta(DB *, char *, uint8 *);
int __qam_vrfy_meta(DB *, VRFY_DBINFO *, QMETA *, db_pgno_t, uint32);
int __qam_meta2pgset(DB *, VRFY_DBINFO *, DB *);
int __qam_vrfy_data(DB *, VRFY_DBINFO *, QPAGE *, db_pgno_t, uint32);
int __qam_vrfy_structure(DB *, VRFY_DBINFO *, uint32);
int __qam_vrfy_walkqueue(DB *, VRFY_DBINFO *, void *, int (*)(void *, const void *), uint32);
int __qam_salvage(DB *, VRFY_DBINFO *, db_pgno_t, PAGE *, void *, int (*)(void *, const void *), uint32);

#if defined(__cplusplus)
}
#endif
#endif /* !_qam_ext_h_ */
