/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2011 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop

__db_dbt::__db_dbt()
{
	THISZERO();
}

__db_dbt::__db_dbt(uint32 f)
{
	THISZERO();
	flags = f;
}
//
// __dbt_usercopy --
// Take a copy of the user's data, if a callback is supplied.
//
int FASTCALL __dbt_usercopy(ENV * env, DBT * dbt)
{
	int ret;
	if(!dbt || !F_ISSET(dbt, DB_DBT_USERCOPY) || dbt->size == 0 || dbt->data)
		return 0;
	else {
		void * buf = NULL;
		if((ret = __os_umalloc(env, dbt->size, &buf)) != 0 || (ret = env->dbt_usercopy(dbt, 0, buf, dbt->size, DB_USERCOPY_GETDATA)) != 0)
			goto err;
		dbt->data = buf;
		return 0;
err:
		if(buf) {
			__os_ufree(env, buf);
			dbt->data = NULL;
		}
	}
	return ret;
}
/*
 * __dbt_userfree --
 *	Free a copy of the user's data, if necessary.
 */
void __dbt_userfree(ENV * env, DBT * key, DBT * pkey, DBT * data)
{
	if(key && F_ISSET(key, DB_DBT_USERCOPY) && key->data) {
		__os_ufree(env, key->data);
		key->data = NULL;
	}
	if(pkey && F_ISSET(pkey, DB_DBT_USERCOPY) && pkey->data) {
		__os_ufree(env, pkey->data);
		pkey->data = NULL;
	}
	if(data && F_ISSET(data, DB_DBT_USERCOPY) && data->data) {
		__os_ufree(env, data->data);
		data->data = NULL;
	}
}
