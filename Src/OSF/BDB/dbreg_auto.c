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
// @v9.5.5 #include "dbinc/db_dispatch.h"
// @v9.5.5 #include "dbinc/db_am.h"
// @v9.5.5 #include "dbinc/txn.h"

DB_LOG_RECSPEC __dbreg_register_desc[] = {
	{LOGREC_DBOP, SSZ(__dbreg_register_args, opcode), "opcode", ""},
	{LOGREC_DBT, SSZ(__dbreg_register_args, name), "name", ""},
	{LOGREC_DBT, SSZ(__dbreg_register_args, uid), "uid", ""},
	{LOGREC_ARG, SSZ(__dbreg_register_args, fileid), "fileid", "%ld"},
	{LOGREC_ARG, SSZ(__dbreg_register_args, ftype), "ftype", "%lx"},
	{LOGREC_ARG, SSZ(__dbreg_register_args, meta_pgno), "meta_pgno", "%lu"},
	{LOGREC_ARG, SSZ(__dbreg_register_args, id), "id", "%lx"},
	{LOGREC_Done, 0, "", ""}
};
/*
 * PUBLIC: int __dbreg_init_recover __P((ENV *, DB_DISTAB *));
 */
int __dbreg_init_recover(ENV * env, DB_DISTAB * dtabp)
{
	int ret;
	if((ret = __db_add_recovery_int(env, dtabp, __dbreg_register_recover, DB___dbreg_register)) != 0)
		return ret;
	return 0;
}
