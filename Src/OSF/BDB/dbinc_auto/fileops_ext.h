/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_fileops_ext_h_
#define	_fileops_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __fop_init_recover __P((ENV *, DB_DISTAB *));
int __fop_create_42_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __fop_create_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __fop_remove_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __fop_write_42_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __fop_write_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __fop_rename_42_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __fop_rename_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __fop_file_remove_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __fop_init_print __P((ENV *, DB_DISTAB *));
int __fop_create __P((ENV *, DB_TXN *, DB_FH **, const char *, const char **, APPNAME, int, uint32));
int __fop_remove __P((ENV *, DB_TXN *, uint8 *, const char *, const char **, APPNAME, uint32));
int __fop_write __P((ENV *, DB_TXN *, const char *, const char *, APPNAME, DB_FH *, uint32, db_pgno_t, uint32, void *, uint32, uint32, uint32));
int __fop_rename __P((ENV *, DB_TXN *, const char *, const char *, const char **, uint8 *, APPNAME, int, uint32));
int __fop_create_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __fop_create_42_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __fop_remove_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __fop_write_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __fop_write_42_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __fop_rename_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __fop_rename_noundo_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __fop_rename_42_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __fop_rename_noundo_46_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __fop_file_remove_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __fop_lock_handle __P((ENV *, DB *, DB_LOCKER *, db_lockmode_t, DB_LOCK *, uint32));
int __fop_file_setup __P((DB *, DB_THREAD_INFO *ip, DB_TXN *, const char *, int, uint32, uint32 *));
int __fop_subdb_setup __P((DB *, DB_THREAD_INFO *, DB_TXN *, const char *, const char *, int, uint32));
int __fop_remove_setup __P((DB *, DB_TXN *, const char *, uint32));
int __fop_read_meta __P((ENV *, const char *, uint8 *, size_t, DB_FH *, int, size_t *));
int __fop_dummy __P((DB *, DB_TXN *, const char *, const char *));
int __fop_dbrename __P((DB *, const char *, const char *));

#if defined(__cplusplus)
}
#endif
#endif /* !_fileops_ext_h_ */
