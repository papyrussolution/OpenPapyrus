/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2004, 2011 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop

#ifdef HAVE_64BIT_TYPES
	#include "dbinc_auto/sequence_ext.h"

 #ifdef HAVE_STATISTICS
static int __seq_print_all __P((DB_SEQUENCE*, uint32));
static int __seq_print_stats __P((DB_SEQUENCE*, uint32));
/*
 * __seq_stat --
 *	Get statistics from the sequence.
 */
int __seq_stat(DB_SEQUENCE * seq, DB_SEQUENCE_STAT ** spp, uint32 flags)
{
	DBT data;
	DB_SEQUENCE_STAT * sp;
	DB_SEQ_RECORD record;
	DB_THREAD_INFO * ip;
	int handle_check, ret, t_ret;
	DB * dbp = seq->seq_dbp;
	ENV * env = dbp->env;
	SEQ_ILLEGAL_BEFORE_OPEN(seq, "DB_SEQUENCE->stat");
	switch(flags) {
	    case DB_STAT_CLEAR:
	    case DB_STAT_ALL:
	    case 0:
		break;
	    default:
		return __db_ferr(env, "DB_SEQUENCE->stat", 0);
	}
	ENV_ENTER(env, ip);
	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if(handle_check && (ret = __db_rep_enter(dbp, 1, 0, 0)) != 0) {
		handle_check = 0;
		goto err;
	}
	/* Allocate and clear the structure. */
	if((ret = __os_umalloc(env, sizeof(*sp), &sp)) != 0)
		goto err;
	memzero(sp, sizeof(*sp));
	if(seq->mtx_seq != MUTEX_INVALID) {
		__mutex_set_wait_info(env, seq->mtx_seq, &sp->st_wait, &sp->st_nowait);
		if(LF_ISSET(DB_STAT_CLEAR))
			__mutex_clear(env, seq->mtx_seq);
	}
	memzero(&data, sizeof(data));
	data.data = &record;
	data.ulen = sizeof(record);
	data.flags = DB_DBT_USERMEM;
retry:
	if((ret = __db_get(dbp, ip, NULL, &seq->seq_key, &data, 0)) != 0) {
		if(ret == DB_BUFFER_SMALL &&
		   data.size > sizeof(seq->seq_record)) {
			if((ret = __os_malloc(env, data.size, &data.data)) != 0)
				goto err;
			data.ulen = data.size;
			goto retry;
		}
		goto err;
	}
	if(data.data != &record)
		memcpy(&record, data.data, sizeof(record));
	sp->st_current = record.seq_value;
	sp->st_value = seq->seq_record.seq_value;
	sp->st_last_value = seq->seq_last_value;
	sp->st_min = seq->seq_record.seq_min;
	sp->st_max = seq->seq_record.seq_max;
	sp->st_cache_size = seq->seq_cache_size;
	sp->st_flags = seq->seq_record.flags;
	*spp = sp;
	if(data.data != &record)
		__os_free(env, data.data);
	/* Release replication block. */
err:
	if(handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __seq_stat_print --
 *	Print statistics from the sequence.
 */
int __seq_stat_print(DB_SEQUENCE * seq, uint32 flags)
{
	DB_THREAD_INFO * ip;
	int handle_check, ret, t_ret;
	DB * dbp = seq->seq_dbp;
	ENV * env = dbp->env;
	SEQ_ILLEGAL_BEFORE_OPEN(seq, "DB_SEQUENCE->stat_print");
	ENV_ENTER(env, ip);
	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if(handle_check && (ret = __db_rep_enter(dbp, 1, 0, 0)) != 0) {
		handle_check = 0;
		goto err;
	}
	if((ret = __seq_print_stats(seq, flags)) != 0)
		goto err;
	if(LF_ISSET(DB_STAT_ALL) && (ret = __seq_print_all(seq, flags)) != 0)
		goto err;
	/* Release replication block. */
err:
	if(handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;
	ENV_LEAVE(env, ip);
	return ret;
}

static const FN __db_seq_flags_fn[] = {
	{ DB_SEQ_DEC,           "decrement" },
	{ DB_SEQ_INC,           "increment" },
	{ DB_SEQ_RANGE_SET,     "range set (internal)" },
	{ DB_SEQ_WRAP,          "wraparound at end" },
	{ 0,                    NULL }
};
/*
 * __db_get_seq_flags_fn --
 *	Return the __db_seq_flags_fn array.
 */
const FN * __db_get_seq_flags_fn()
{
	return __db_seq_flags_fn;
}

/*
 * __seq_print_stats --
 *	Display sequence stat structure.
 */
static int __seq_print_stats(DB_SEQUENCE * seq, uint32 flags)
{
	DB_SEQUENCE_STAT * sp;
	int ret;
	ENV * env = seq->seq_dbp->env;
	if((ret = __seq_stat(seq, &sp, flags)) != 0)
		return ret;
	__db_dl_pct(env, "The number of sequence locks that required waiting", (ulong)sp->st_wait, DB_PCT(sp->st_wait, sp->st_wait+sp->st_nowait), 0);
	STAT_FMT("The current sequence value", INT64_FMT, db_seq_t, sp->st_current);
	STAT_FMT("The cached sequence value", INT64_FMT, db_seq_t, sp->st_value);
	STAT_FMT("The last cached sequence value", INT64_FMT, db_seq_t, sp->st_last_value);
	STAT_FMT("The minimum sequence value", INT64_FMT, db_seq_t, sp->st_min);
	STAT_FMT("The maximum sequence value", INT64_FMT, db_seq_t, sp->st_max);
	STAT_ULONG("The cache size", sp->st_cache_size);
	__db_prflags(env, NULL, sp->st_flags, __db_seq_flags_fn, NULL, "\tSequence flags");
	__os_ufree(seq->seq_dbp->env, sp);
	return 0;
}

/*
 * __seq_print_all --
 *	Display sequence debugging information - none for now.
 *	(The name seems a bit strange, no?)
 */
static int __seq_print_all(DB_SEQUENCE * seq, uint32 flags)
{
	COMPQUIET(seq, 0);
	COMPQUIET(flags, 0);
	return 0;
}

 #else /* !HAVE_STATISTICS */

int __seq_stat(DB_SEQUENCE * seq, DB_SEQUENCE_STAT ** statp, uint32 flags)
{
	COMPQUIET(statp, 0);
	COMPQUIET(flags, 0);
	return __db_stat_not_built(seq->seq_dbp->env);
}

int __seq_stat_print(DB_SEQUENCE * seq, uint32 flags)
{
	COMPQUIET(flags, 0);
	return __db_stat_not_built(seq->seq_dbp->env);
}
/*
 * __db_get_seq_flags_fn --
 *	Return the __db_seq_flags_fn array.
 */
const FN * __db_get_seq_flags_fn()
{
	static const FN __db_seq_flags_fn[] = {
		{ 0,    NULL }
	};
	/*
	 * !!!
	 * The Tcl API uses this interface, stub it off.
	 */
	return __db_seq_flags_fn;
}

 #endif /* !HAVE_STATISTICS */
#endif /* HAVE_64BIT_TYPES */
