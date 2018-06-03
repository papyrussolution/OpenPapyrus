/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_dbreg_ext_h_
#define	_dbreg_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __dbreg_setup __P((DB *, const char *, const char *, uint32));
int __dbreg_teardown(DB *);
int __dbreg_teardown_int __P((ENV *, FNAME *));
int __dbreg_new_id __P((DB *, DB_TXN *));
int __dbreg_get_id __P((DB *, DB_TXN *, int32 *));
int __dbreg_assign_id __P((DB *, int32, int));
int __dbreg_revoke_id __P((DB *, int, int32));
int __dbreg_revoke_id_int __P((ENV *, FNAME *, int, int, int32));
int __dbreg_close_id __P((DB *, DB_TXN *, uint32));
int __dbreg_close_id_int __P((ENV *, FNAME *, uint32, int));
int __dbreg_failchk(ENV *);
int __dbreg_log_close __P((ENV *, FNAME *, DB_TXN *, uint32));
int __dbreg_log_id __P((DB *, DB_TXN *, int32, int));
int __dbreg_init_recover(ENV *, DB_DISTAB *);
int __dbreg_register_42_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __dbreg_register_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __dbreg_init_print(ENV *, DB_DISTAB *);
int __dbreg_register_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __dbreg_register_42_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __dbreg_stat_print(ENV *, uint32);
void __dbreg_print_fname __P((ENV *, FNAME *));
int __dbreg_add_dbentry __P((ENV *, DB_LOG *, DB *, int32));
int __dbreg_rem_dbentry __P((DB_LOG *, int32));
int __dbreg_log_files(ENV *, uint32);
int __dbreg_log_nofiles(ENV *);
int __dbreg_close_files __P((ENV *, int));
int __dbreg_close_file __P((ENV *, FNAME *));
int __dbreg_mark_restored(ENV *);
int __dbreg_invalidate_files __P((ENV *, int));
int __dbreg_id_to_db __P((ENV *, DB_TXN *, DB **, int32, int));
int __dbreg_id_to_fname __P((DB_LOG *, int32, int, FNAME **));
int __dbreg_fid_to_fname __P((DB_LOG *, uint8 *, int, FNAME **));
int __dbreg_blob_file_to_fname __P((DB_LOG *, db_seq_t, int, FNAME **));
int __dbreg_get_name __P((ENV *, uint8 *, char **, char **));
int __dbreg_do_open __P((ENV *, DB_TXN *, DB_LOG *, uint8 *, char *, DBTYPE, int32, db_pgno_t, void *, uint32, uint32, db_seq_t));
int __dbreg_lazy_id(DB *);

#if defined(__cplusplus)
}
#endif
#endif /* !_dbreg_ext_h_ */
