/* Do not edit: automatically built by gen_rec.awk. */

#include "db_config.h"
#include "db_int.h"
// @v9.5.5 #include "dbinc/db_page.h"
// @v9.5.5 #include "dbinc/lock.h"
// @v9.5.5 #include "dbinc/mp.h"
// @v9.5.5 #include "dbinc/crypto.h"
// @v9.5.5 #include "dbinc/btree.h"
// @v9.5.5 #include "dbinc/hash.h"
#pragma hdrstop

#ifdef HAVE_HEAP
 // @v9.5.5 #include "dbinc/db_am.h"
 // @v9.5.5 #include "dbinc/heap.h"
 // @v9.5.5 #include "dbinc/txn.h"
/*
 * PUBLIC: int __heap_addrem_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int __heap_addrem_print(ENV * env, DBT * dbtp, DB_LSN * lsnp, db_recops notused2, void * info)
{
	COMPQUIET(notused2, DB_TXN_PRINT);
	return __log_print_record(env, dbtp, lsnp, "__heap_addrem", __heap_addrem_desc, info);
}
/*
 * PUBLIC: int __heap_pg_alloc_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int __heap_pg_alloc_print(ENV * env, DBT * dbtp, DB_LSN * lsnp, db_recops notused2, void * info)
{
	COMPQUIET(notused2, DB_TXN_PRINT);
	return __log_print_record(env, dbtp, lsnp, "__heap_pg_alloc", __heap_pg_alloc_desc, info);
}
/*
 * PUBLIC: int __heap_trunc_meta_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int __heap_trunc_meta_print(ENV * env, DBT * dbtp, DB_LSN * lsnp, db_recops notused2, void * info)
{
	COMPQUIET(notused2, DB_TXN_PRINT);
	return __log_print_record(env, dbtp, lsnp, "__heap_trunc_meta", __heap_trunc_meta_desc, info);
}
/*
 * PUBLIC: int __heap_trunc_page_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int __heap_trunc_page_print(ENV * env, DBT * dbtp, DB_LSN * lsnp, db_recops notused2, void * info)
{
	COMPQUIET(notused2, DB_TXN_PRINT);
	return __log_print_record(env, dbtp, lsnp, "__heap_trunc_page", __heap_trunc_page_desc, info);
}
/*
 * PUBLIC: int __heap_init_print __P((ENV *, DB_DISTAB *));
 */
int __heap_init_print(ENV * env, DB_DISTAB * dtabp)
{
	int ret;
	if((ret = __db_add_recovery_int(env, dtabp, __heap_addrem_print, DB___heap_addrem)) != 0)
		return ret;
	if((ret = __db_add_recovery_int(env, dtabp, __heap_pg_alloc_print, DB___heap_pg_alloc)) != 0)
		return ret;
	if((ret = __db_add_recovery_int(env, dtabp, __heap_trunc_meta_print, DB___heap_trunc_meta)) != 0)
		return ret;
	if((ret = __db_add_recovery_int(env, dtabp, __heap_trunc_page_print, DB___heap_trunc_page)) != 0)
		return ret;
	return 0;
}
#endif /* HAVE_HEAP */
