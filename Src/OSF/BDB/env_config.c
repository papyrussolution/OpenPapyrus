/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2011 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop

static int __config_parse(ENV*, char *, int);
/*
 * __env_read_db_config --
 *	Read the DB_CONFIG file.
 *
 * PUBLIC: int __env_read_db_config(ENV *);
 */
int __env_read_db_config(ENV * env)
{
	// Parse the config file. 
	char * p = NULL;
	int    ret = __db_appname(env, DB_APP_NONE, "DB_CONFIG", NULL, &p);
	if(ret == 0) {
		FILE * fp = 0;
		if(p) {
			fp = fopen(p, "r");
			__os_free(env, p);
		}
		if(fp) {
			char   buf[256];
			for(int lc = 1; fgets(buf, sizeof(buf), fp) != NULL; ++lc) {
				if((p = sstrchr(buf, '\n')) == NULL)
					p = buf+sstrlen(buf);
				if(p > buf && p[-1] == '\r')
					--p;
				*p = '\0';
				for(p = buf; *p != '\0' && isspace((int)*p); ++p)
					;
				if(*p != '\0' && *p != '#') {
					if((ret = __config_parse(env, p, lc)) != 0)
						break;
				}
			}
			fclose(fp);
		}
	}
	return ret;
}

#undef  CONFIG_GET_INT
#define CONFIG_GET_INT(s, vp) do {                                      \
		int __ret;                                                      \
		if((__ret = __db_getlong(env->dbenv, NULL, s, 0, INT_MAX, vp)) != 0)    \
			return (__ret);                                         \
} while(0)
#undef  CONFIG_GET_LONG
#define CONFIG_GET_LONG(s, vp) do {                                     \
		int __ret;                                                      \
		if((__ret = __db_getlong(env->dbenv, NULL, s, 0, LONG_MAX, vp)) != 0)   \
			return (__ret);                                         \
} while(0)
#undef  CONFIG_GET_UINT
#define CONFIG_GET_UINT(s, vp) do {                                     \
		int __ret;                                                      \
		if((__ret = __db_getulong(env->dbenv, NULL, s, 0, UINT_MAX, vp)) != 0)  \
			return (__ret);                                         \
} while(0)
#undef  CONFIG_INT
#define CONFIG_INT(s, f) do {                                           \
		if(strcasecmp(s, argv[0]) == 0) {                              \
			long __v;                                               \
			if(nf != 2)                                            \
				goto format;                                    \
			CONFIG_GET_INT(argv[1], &__v);                          \
			return (f(env->dbenv, (int)__v));                       \
		}                                                               \
} while(0)
#undef  CONFIG_GET_UINT32
#define CONFIG_GET_UINT32(s, vp) do { if(__db_getulong(env->dbenv, NULL, s, 0, UINT32_MAX, vp) != 0) return (EINVAL); } while(0)
#undef  CONFIG_UINT32
#define CONFIG_UINT32(s, f) do { \
		if(sstreqi_ascii(s, argv[0])) { \
			ulong __v; \
			if(nf != 2) \
				goto format; \
			CONFIG_GET_UINT32(argv[1], &__v); \
			return (f(env->dbenv, (uint32)__v)); \
		} \
} while(0)

#undef  CONFIG_SLOTS
#define CONFIG_SLOTS    10
/*
 * __config_parse --
 *	Parse a single NAME VALUE pair.
 */
static int __config_parse(ENV * env, char * s, int lc)
{
	DB_SITE * site;
	ulong uv1, uv2;
	uint32 flags;
	long lv1, lv2;
	uint port;
	int i, nf, onoff, ret, t_ret;
	char * argv[CONFIG_SLOTS];
	DB_MEM_CONFIG mem_conf;
	int bad = 0;
	DB_ENV * dbenv = env->dbenv;
	// Split the line by white-space
	if((nf = __config_split(s, argv)) < 2) {
format:
		__db_errx(env, DB_STR_A("1584", "line %d: %s: incorrect name-value pair", "%d %s"), lc, argv[0]);
		return EINVAL;
	}
	if(sstreqi_ascii(argv[0], "set_memory_max")) {
		if(nf != 3)
			goto format;
		CONFIG_GET_UINT32(argv[1], &uv1);
		CONFIG_GET_UINT32(argv[2], &uv2);
		return __env_set_memory_max(dbenv, (uint32)uv1, (uint32)uv2);
	}
	if(sstreqi_ascii(argv[0], "set_memory_init")) {
		if(nf != 3)
			goto format;
		if(sstreqi_ascii(argv[1], "DB_MEM_LOCK"))
			mem_conf = DB_MEM_LOCK;
		else if(sstreqi_ascii(argv[1], "DB_MEM_LOCKER"))
			mem_conf = DB_MEM_LOCKER;
		else if(sstreqi_ascii(argv[1], "DB_MEM_LOCKOBJECT"))
			mem_conf = DB_MEM_LOCKOBJECT;
		else if(sstreqi_ascii(argv[1], "DB_MEM_TRANSACTION"))
			mem_conf = DB_MEM_TRANSACTION;
		else if(sstreqi_ascii(argv[1], "DB_MEM_THREAD"))
			mem_conf = DB_MEM_THREAD;
		else if(sstreqi_ascii(argv[1], "DB_MEM_LOGID"))
			mem_conf = DB_MEM_LOGID;
		else
			goto format;
		CONFIG_GET_UINT32(argv[2], &uv2);
		return __env_set_memory_init(dbenv, mem_conf, (uint32)uv2);
	}
	CONFIG_UINT32("mutex_set_align", __mutex_set_align);
	CONFIG_UINT32("mutex_set_increment", __mutex_set_increment);
	CONFIG_UINT32("mutex_set_init", __mutex_set_init);
	CONFIG_UINT32("mutex_set_max", __mutex_set_max);
	CONFIG_UINT32("mutex_set_tas_spins", __mutex_set_tas_spins);
	if(sstreqi_ascii(argv[0], "rep_set_clockskew")) {
		if(nf != 3)
			goto format;
		CONFIG_GET_UINT32(argv[1], &uv1);
		CONFIG_GET_UINT32(argv[2], &uv2);
		return __rep_set_clockskew(dbenv, (uint32)uv1, (uint32)uv2);
	}
	else if(sstreqi_ascii(argv[0], "rep_set_config")) {
		if(nf != 2 && nf != 3)
			goto format;
		onoff = 1;
		if(nf == 3) {
			if(sstreqi_ascii(argv[2], "off"))
				onoff = 0;
			else if(!sstreqi_ascii(argv[2], "on"))
				goto format;
		}
		if(sstreqi_ascii(argv[1], "db_rep_conf_autoinit"))
			return __rep_set_config(dbenv, DB_REP_CONF_AUTOINIT, onoff);
		else if(sstreqi_ascii(argv[1], "db_rep_conf_autorollback"))
			return __rep_set_config(dbenv, DB_REP_CONF_AUTOROLLBACK, onoff);
		else if(sstreqi_ascii(argv[1], "db_rep_conf_bulk"))
			return __rep_set_config(dbenv, DB_REP_CONF_BULK, onoff);
		else if(sstreqi_ascii(argv[1], "db_rep_conf_delayclient"))
			return __rep_set_config(dbenv, DB_REP_CONF_DELAYCLIENT, onoff);
		else if(sstreqi_ascii(argv[1], "db_rep_conf_inmem"))
			return __rep_set_config(dbenv, DB_REP_CONF_INMEM, onoff);
		else if(sstreqi_ascii(argv[1], "db_rep_conf_lease"))
			return __rep_set_config(dbenv, DB_REP_CONF_LEASE, onoff);
		else if(sstreqi_ascii(argv[1], "db_rep_conf_nowait"))
			return __rep_set_config(dbenv, DB_REP_CONF_NOWAIT, onoff);
		else if(sstreqi_ascii(argv[1], "db_repmgr_conf_2site_strict"))
			return __rep_set_config(dbenv, DB_REPMGR_CONF_2SITE_STRICT, onoff);
		else if(sstreqi_ascii(argv[1], "db_repmgr_conf_elections"))
			return __rep_set_config(dbenv, DB_REPMGR_CONF_ELECTIONS, onoff);
		else 
			goto format;
	}
	else if(sstreqi_ascii(argv[0], "rep_set_limit")) {
		if(nf != 3)
			goto format;
		CONFIG_GET_UINT32(argv[1], &uv1);
		CONFIG_GET_UINT32(argv[2], &uv2);
		return __rep_set_limit(dbenv, (uint32)uv1, (uint32)uv2);
	}
	else if(sstreqi_ascii(argv[0], "rep_set_nsites")) {
		if(nf != 2)
			goto format;
		CONFIG_GET_UINT32(argv[1], &uv1);
		return __rep_set_nsites_pp(dbenv, (uint32)uv1);
	}
	else if(sstreqi_ascii(argv[0], "rep_set_priority")) {
		if(nf != 2)
			goto format;
		CONFIG_GET_UINT32(argv[1], &uv1);
		return __rep_set_priority(dbenv, (uint32)uv1);
	}
	else if(sstreqi_ascii(argv[0], "rep_set_request")) {
		if(nf != 3)
			goto format;
		CONFIG_GET_UINT32(argv[1], &uv1);
		CONFIG_GET_UINT32(argv[2], &uv2);
		return __rep_set_request(dbenv, (uint32)uv1, (uint32)uv2);
	}
	else if(sstreqi_ascii(argv[0], "rep_set_timeout")) {
		if(nf != 3)
			goto format;
		CONFIG_GET_UINT32(argv[2], &uv2);
		if(sstreqi_ascii(argv[1], "db_rep_ack_timeout"))
			return __rep_set_timeout(dbenv, DB_REP_ACK_TIMEOUT, (uint32)uv2);
		else if(sstreqi_ascii(argv[1], "db_rep_checkpoint_delay"))
			return __rep_set_timeout(dbenv, DB_REP_CHECKPOINT_DELAY, (uint32)uv2);
		else if(sstreqi_ascii(argv[1], "db_rep_connection_retry"))
			return __rep_set_timeout(dbenv, DB_REP_CONNECTION_RETRY, (uint32)uv2);
		else if(sstreqi_ascii(argv[1], "db_rep_election_timeout"))
			return __rep_set_timeout(dbenv, DB_REP_ELECTION_TIMEOUT, (uint32)uv2);
		else if(sstreqi_ascii(argv[1], "db_rep_election_retry"))
			return __rep_set_timeout(dbenv, DB_REP_ELECTION_RETRY, (uint32)uv2);
		else if(sstreqi_ascii(argv[1], "db_rep_full_election_timeout"))
			return __rep_set_timeout(dbenv, DB_REP_FULL_ELECTION_TIMEOUT, (uint32)uv2);
		else if(sstreqi_ascii(argv[1], "db_rep_heartbeat_monitor"))
			return __rep_set_timeout(dbenv, DB_REP_HEARTBEAT_MONITOR, (uint32)uv2);
		else if(sstreqi_ascii(argv[1], "db_rep_heartbeat_send"))
			return __rep_set_timeout(dbenv, DB_REP_HEARTBEAT_SEND, (uint32)uv2);
		else if(sstreqi_ascii(argv[1], "db_rep_lease_timeout"))
			return __rep_set_timeout(dbenv, DB_REP_LEASE_TIMEOUT, (uint32)uv2);
		else 
			goto format;
	}
	else if(sstreqi_ascii(argv[0], "repmgr_set_ack_policy")) {
		if(nf != 2)
			goto format;
		if(sstreqi_ascii(argv[1], "db_repmgr_acks_all"))
			return __repmgr_set_ack_policy(dbenv, DB_REPMGR_ACKS_ALL);
		else if(sstreqi_ascii(argv[1], "db_repmgr_acks_all_available"))
			return __repmgr_set_ack_policy(dbenv, DB_REPMGR_ACKS_ALL_AVAILABLE);
		else if(sstreqi_ascii(argv[1], "db_repmgr_acks_all_peers"))
			return __repmgr_set_ack_policy(dbenv, DB_REPMGR_ACKS_ALL_PEERS);
		else if(sstreqi_ascii(argv[1], "db_repmgr_acks_none"))
			return __repmgr_set_ack_policy(dbenv, DB_REPMGR_ACKS_NONE);
		else if(sstreqi_ascii(argv[1], "db_repmgr_acks_one"))
			return __repmgr_set_ack_policy(dbenv, DB_REPMGR_ACKS_ONE);
		else if(sstreqi_ascii(argv[1], "db_repmgr_acks_one_peer"))
			return __repmgr_set_ack_policy(dbenv, DB_REPMGR_ACKS_ONE_PEER);
		else if(sstreqi_ascii(argv[1], "db_repmgr_acks_quorum"))
			return __repmgr_set_ack_policy(dbenv, DB_REPMGR_ACKS_QUORUM);
		else 
			goto format;
	}
	/*
	 * Configure name/value pairs of config information for a site (local or remote).
	 *
	 * repmgr_site host port [which value] ...
	 */
	else if(sstreqi_ascii(argv[0], "repmgr_site")) {
		if(nf < 3 || (nf%2) == 0)
			goto format;
		CONFIG_GET_UINT(argv[2], &uv2);
		port = (uint)uv2;
		if((ret = __repmgr_site(dbenv, argv[1], port, &site, 0)) != 0)
			return ret;
#ifdef HAVE_REPLICATION_THREADS
		for(i = 3; i < nf; i += 2) {
			if(sstreqi_ascii(argv[i], "db_bootstrap_helper"))
				uv1 = DB_BOOTSTRAP_HELPER;
			else if(sstreqi_ascii(argv[i], "db_group_creator"))
				uv1 = DB_GROUP_CREATOR;
			else if(sstreqi_ascii(argv[i], "db_legacy"))
				uv1 = DB_LEGACY;
			else if(sstreqi_ascii(argv[i], "db_local_site"))
				uv1 = DB_LOCAL_SITE;
			else if(sstreqi_ascii(argv[i], "db_repmgr_peer"))
				uv1 = DB_REPMGR_PEER;
			else {
				bad = 1;
				break;
			}
			if(sstreqi_ascii(argv[i+1], "on"))
				uv2 = 1;
			else if(sstreqi_ascii(argv[i+1], "off"))
				uv2 = 0;
			else
				CONFIG_GET_UINT32(argv[i+1], &uv2);
			if((ret = __repmgr_site_config(site, (uint32)uv1, (uint32)uv2)) != 0)
				break;
		}
		if((t_ret = __repmgr_site_close(site)) != 0 && ret == 0)
			ret = t_ret;
		if(bad)
			goto format;
#else
		/* If repmgr not built, __repmgr_site() returns DB_OPNOTSUP. */
		COMPQUIET(i, 0);
		COMPQUIET(t_ret, 0);
		DB_ASSERT(env, 0);
#endif
		return ret;
	}
	else if(sstreqi_ascii(argv[0], "set_cachesize")) {
		if(nf != 4)
			goto format;
		CONFIG_GET_UINT32(argv[1], &uv1);
		CONFIG_GET_UINT32(argv[2], &uv2);
		CONFIG_GET_INT(argv[3], &lv1);
		return __memp_set_cachesize(dbenv, (uint32)uv1, (uint32)uv2, (int)lv1);
	}
	else if(sstreqi_ascii(argv[0], "set_cache_max")) {
		if(nf != 3)
			goto format;
		CONFIG_GET_UINT32(argv[1], &uv1);
		CONFIG_GET_UINT32(argv[2], &uv2);
		return __memp_set_cache_max(dbenv, (uint32)uv1, (uint32)uv2);
	}
	else if(sstreqi_ascii(argv[0], "set_data_dir") || sstreqi_ascii(argv[0], "db_data_dir")) { /* Compatibility. */
		if(nf != 2)
			goto format;
		return __env_set_data_dir(dbenv, argv[1]);
	}
	else if(sstreqi_ascii(argv[0], "add_data_dir")) {
		if(nf != 2)
			goto format;
		return __env_add_data_dir(dbenv, argv[1]);
	}
	else if(sstreqi_ascii(argv[0], "set_create_dir")) {
		if(nf != 2)
			goto format;
		return __env_set_create_dir(dbenv, argv[1]);
	}
	// Compatibility 
	else if(sstreqi_ascii(argv[0], "set_intermediate_dir")) {
		if(nf != 2)
			goto format;
		CONFIG_GET_INT(argv[1], &lv1);
		if(lv1 <= 0)
			goto format;
		env->dir_mode = (int)lv1;
		return 0;
	}
	else if(sstreqi_ascii(argv[0], "set_intermediate_dir_mode")) {
		if(nf != 2)
			goto format;
		return __env_set_intermediate_dir_mode(dbenv, argv[1]);
	}
	else if(sstreqi_ascii(argv[0], "set_flags")) {
		if(nf != 2 && nf != 3)
			goto format;
		onoff = 1;
		if(nf == 3) {
			if(sstreqi_ascii(argv[2], "off"))
				onoff = 0;
			else if(!sstreqi_ascii(argv[2], "on"))
				goto format;
		}
		if(sstreqi_ascii(argv[1], "db_auto_commit"))
			return __env_set_flags(dbenv, DB_AUTO_COMMIT, onoff);
		else if(sstreqi_ascii(argv[1], "db_cdb_alldb"))
			return __env_set_flags(dbenv, DB_CDB_ALLDB, onoff);
		else if(sstreqi_ascii(argv[1], "db_direct_db"))
			return __env_set_flags(dbenv, DB_DIRECT_DB, onoff);
		else if(sstreqi_ascii(argv[1], "db_dsync_db"))
			return __env_set_flags(dbenv, DB_DSYNC_DB, onoff);
		else if(sstreqi_ascii(argv[1], "db_multiversion"))
			return __env_set_flags(dbenv, DB_MULTIVERSION, onoff);
		else if(sstreqi_ascii(argv[1], "db_nolocking"))
			return __env_set_flags(dbenv, DB_NOLOCKING, onoff);
		else if(sstreqi_ascii(argv[1], "db_nommap"))
			return __env_set_flags(dbenv, DB_NOMMAP, onoff);
		else if(sstreqi_ascii(argv[1], "db_nopanic"))
			return __env_set_flags(dbenv, DB_NOPANIC, onoff);
		else if(sstreqi_ascii(argv[1], "db_overwrite"))
			return __env_set_flags(dbenv, DB_OVERWRITE, onoff);
		else if(sstreqi_ascii(argv[1], "db_region_init"))
			return __env_set_flags(dbenv, DB_REGION_INIT, onoff);
		else if(sstreqi_ascii(argv[1], "db_time_notgranted"))
			return __env_set_flags(dbenv, DB_TIME_NOTGRANTED, onoff);
		else if(sstreqi_ascii(argv[1], "db_txn_nosync"))
			return __env_set_flags(dbenv, DB_TXN_NOSYNC, onoff);
		else if(sstreqi_ascii(argv[1], "db_txn_nowait"))
			return __env_set_flags(dbenv, DB_TXN_NOWAIT, onoff);
		else if(sstreqi_ascii(argv[1], "db_txn_snapshot"))
			return __env_set_flags(dbenv, DB_TXN_SNAPSHOT, onoff);
		else if(sstreqi_ascii(argv[1], "db_txn_write_nosync"))
			return __env_set_flags(dbenv, DB_TXN_WRITE_NOSYNC, onoff);
		else if(sstreqi_ascii(argv[1], "db_yieldcpu"))
			return __env_set_flags(dbenv, DB_YIELDCPU, onoff);
		else if(sstreqi_ascii(argv[1], "db_log_inmemory"))
			return __log_set_config(dbenv, DB_LOG_IN_MEMORY, onoff);
		else if(sstreqi_ascii(argv[1], "db_direct_log"))
			return __log_set_config(dbenv, DB_LOG_DIRECT, onoff);
		else if(sstreqi_ascii(argv[1], "db_dsync_log"))
			return __log_set_config(dbenv, DB_LOG_DSYNC, onoff);
		else if(sstreqi_ascii(argv[1], "db_log_autoremove"))
			return __log_set_config(dbenv, DB_LOG_AUTO_REMOVE, onoff);
		else
			goto format;
	}
	else if(sstreqi_ascii(argv[0], "log_set_config")) {
		if(nf != 2 && nf != 3)
			goto format;
		onoff = 1;
		if(nf == 3) {
			if(sstreqi_ascii(argv[2], "off"))
				onoff = 0;
			else if(!sstreqi_ascii(argv[2], "on"))
				goto format;
		}
		if(sstreqi_ascii(argv[1], "db_log_auto_remove"))
			return __log_set_config(dbenv, DB_LOG_AUTO_REMOVE, onoff);
		else if(sstreqi_ascii(argv[1], "db_log_direct"))
			return __log_set_config(dbenv, DB_LOG_DIRECT, onoff);
		else if(sstreqi_ascii(argv[1], "db_log_dsync"))
			return __log_set_config(dbenv, DB_LOG_DSYNC, onoff);
		else if(sstreqi_ascii(argv[1], "db_log_in_memory"))
			return __log_set_config(dbenv, DB_LOG_IN_MEMORY, onoff);
		else if(sstreqi_ascii(argv[1], "db_log_zero"))
			return __log_set_config(dbenv, DB_LOG_ZERO, onoff);
		else
			goto format;
	}
	CONFIG_UINT32("set_data_len", __env_set_data_len);
	CONFIG_UINT32("set_lg_bsize", __log_set_lg_bsize);
	CONFIG_INT("set_lg_filemode", __log_set_lg_filemode);
	CONFIG_UINT32("set_lg_max", __log_set_lg_max);
	CONFIG_UINT32("set_lg_regionmax", __log_set_lg_regionmax);
	if(sstreqi_ascii(argv[0], "set_lg_dir") || sstreqi_ascii(argv[0], "db_log_dir")) {    /* Compatibility. */
		if(nf != 2)
			goto format;
		return __log_set_lg_dir(dbenv, argv[1]);
	}
	else if(sstreqi_ascii(argv[0], "set_lk_detect")) {
		if(nf != 2)
			goto format;
		if(sstreqi_ascii(argv[1], "db_lock_default"))
			flags = DB_LOCK_DEFAULT;
		else if(sstreqi_ascii(argv[1], "db_lock_expire"))
			flags = DB_LOCK_EXPIRE;
		else if(sstreqi_ascii(argv[1], "db_lock_maxlocks"))
			flags = DB_LOCK_MAXLOCKS;
		else if(sstreqi_ascii(argv[1], "db_lock_maxwrite"))
			flags = DB_LOCK_MAXWRITE;
		else if(sstreqi_ascii(argv[1], "db_lock_minlocks"))
			flags = DB_LOCK_MINLOCKS;
		else if(sstreqi_ascii(argv[1], "db_lock_minwrite"))
			flags = DB_LOCK_MINWRITE;
		else if(sstreqi_ascii(argv[1], "db_lock_oldest"))
			flags = DB_LOCK_OLDEST;
		else if(sstreqi_ascii(argv[1], "db_lock_random"))
			flags = DB_LOCK_RANDOM;
		else if(sstreqi_ascii(argv[1], "db_lock_youngest"))
			flags = DB_LOCK_YOUNGEST;
		else
			goto format;
		return __lock_set_lk_detect(dbenv, flags);
	}
	CONFIG_UINT32("set_lk_max_locks", __lock_set_lk_max_locks);
	CONFIG_UINT32("set_lk_max_lockers", __lock_set_lk_max_lockers);
	CONFIG_UINT32("set_lk_max_objects", __lock_set_lk_max_objects);
	CONFIG_UINT32("set_lk_partitions", __lock_set_lk_partitions);
	CONFIG_UINT32("set_lk_tablesize", __lock_set_lk_tablesize);
	if(sstreqi_ascii(argv[0], "set_lock_timeout")) {
		if(nf != 2)
			goto format;
		CONFIG_GET_UINT32(argv[1], &uv1);
		return __lock_set_env_timeout(dbenv, (uint32)uv1, DB_SET_LOCK_TIMEOUT);
	}
	CONFIG_INT("set_mp_max_openfd", __memp_set_mp_max_openfd);
	CONFIG_UINT32("set_mp_mtxcount", __memp_set_mp_mtxcount);
	CONFIG_UINT32("set_mp_pagesize", __memp_set_mp_pagesize);
	if(sstreqi_ascii(argv[0], "set_mp_max_write")) {
		if(nf != 3)
			goto format;
		CONFIG_GET_INT(argv[1], &lv1);
		CONFIG_GET_INT(argv[2], &lv2);
		return __memp_set_mp_max_write(dbenv, (int)lv1, (db_timeout_t)lv2);
	}
	CONFIG_UINT32("set_mp_mmapsize", __memp_set_mp_mmapsize);
	if(sstreqi_ascii(argv[0], "set_open_flags")) {
		if(nf != 2 && nf != 3)
			goto format;
		onoff = 1;
		if(nf == 3) {
			if(sstreqi_ascii(argv[2], "off"))
				onoff = 0;
			else if(!sstreqi_ascii(argv[2], "on"))
				goto format;
		}
		if(sstreqi_ascii(argv[1], "db_init_rep")) {
			SETFLAG(env->open_flags, DB_INIT_REP, (onoff == 1));
			return 0;
		}
		else if(sstreqi_ascii(argv[1], "db_private")) {
			SETFLAG(env->open_flags, DB_PRIVATE, (onoff == 1));
			return 0;
		}
		else if(sstreqi_ascii(argv[1], "db_register")) {
			SETFLAG(env->open_flags, DB_REGISTER, (onoff == 1));
			return 0;
		}
		else if(sstreqi_ascii(argv[1], "db_thread")) {
			SETFLAG(env->open_flags, DB_THREAD, (onoff == 1));
			return 0;
		}
		else
			goto format;
	}
	if(sstreqi_ascii(argv[0], "set_region_init")) {
		if(nf != 2)
			goto format;
		CONFIG_GET_INT(argv[1], &lv1);
		if(lv1 != 0 && lv1 != 1)
			goto format;
		return __env_set_flags(dbenv, DB_REGION_INIT, lv1 == 0 ? 0 : 1);
	}
	if(sstreqi_ascii(argv[0], "set_reg_timeout")) {
		if(nf != 2)
			goto format;
		CONFIG_GET_UINT32(argv[1], &uv1);
		return __env_set_timeout(dbenv, (uint32)uv1, DB_SET_REG_TIMEOUT);
	}
	if(sstreqi_ascii(argv[0], "set_shm_key")) {
		if(nf != 2)
			goto format;
		CONFIG_GET_LONG(argv[1], &lv1);
		return __env_set_shm_key(dbenv, lv1);
	}
	/*
	 * The set_tas_spins method has been replaced by mutex_set_tas_spins.
	 * The set_tas_spins argv[0] remains for DB_CONFIG compatibility.
	 */
	CONFIG_UINT32("set_tas_spins", __mutex_set_tas_spins);
	if(sstreqi_ascii(argv[0], "set_tmp_dir") || sstreqi_ascii(argv[0], "db_tmp_dir")) { /* Compatibility.*/
		if(nf != 2)
			goto format;
		return __env_set_tmp_dir(dbenv, argv[1]);
	}
	CONFIG_UINT32("set_thread_count", __env_set_thread_count);
	CONFIG_UINT32("set_tx_max", __txn_set_tx_max);
	if(sstreqi_ascii(argv[0], "set_txn_timeout")) {
		if(nf != 2)
			goto format;
		CONFIG_GET_UINT32(argv[1], &uv1);
		return __lock_set_env_timeout(dbenv, (uint32)uv1, DB_SET_TXN_TIMEOUT);
	}
	if(sstreqi_ascii(argv[0], "set_verbose")) {
		if(nf != 2 && nf != 3)
			goto format;
		onoff = 1;
		if(nf == 3) {
			if(sstreqi_ascii(argv[2], "off"))
				onoff = 0;
			else if(!sstreqi_ascii(argv[2], "on"))
				goto format;
		}
		if(sstreqi_ascii(argv[1], "db_verb_deadlock"))
			flags = DB_VERB_DEADLOCK;
		else if(sstreqi_ascii(argv[1], "db_verb_fileops"))
			flags = DB_VERB_FILEOPS;
		else if(sstreqi_ascii(argv[1], "db_verb_fileops_all"))
			flags = DB_VERB_FILEOPS_ALL;
		else if(sstreqi_ascii(argv[1], "db_verb_recovery"))
			flags = DB_VERB_RECOVERY;
		else if(sstreqi_ascii(argv[1], "db_verb_register"))
			flags = DB_VERB_REGISTER;
		else if(sstreqi_ascii(argv[1], "db_verb_replication"))
			flags = DB_VERB_REPLICATION;
		else if(sstreqi_ascii(argv[1], "db_verb_rep_elect"))
			flags = DB_VERB_REP_ELECT;
		else if(sstreqi_ascii(argv[1], "db_verb_rep_lease"))
			flags = DB_VERB_REP_LEASE;
		else if(sstreqi_ascii(argv[1], "db_verb_rep_misc"))
			flags = DB_VERB_REP_MISC;
		else if(sstreqi_ascii(argv[1], "db_verb_rep_msgs"))
			flags = DB_VERB_REP_MSGS;
		else if(sstreqi_ascii(argv[1], "db_verb_rep_sync"))
			flags = DB_VERB_REP_SYNC;
		else if(sstreqi_ascii(argv[1], "db_verb_rep_system"))
			flags = DB_VERB_REP_SYSTEM;
		else if(sstreqi_ascii(argv[1], "db_verb_rep_test"))
			flags = DB_VERB_REP_TEST;
		else if(sstreqi_ascii(argv[1], "db_verb_repmgr_connfail"))
			flags = DB_VERB_REPMGR_CONNFAIL;
		else if(sstreqi_ascii(argv[1], "db_verb_repmgr_misc"))
			flags = DB_VERB_REPMGR_MISC;
		else if(sstreqi_ascii(argv[1], "db_verb_waitsfor"))
			flags = DB_VERB_WAITSFOR;
		else
			goto format;
		return __env_set_verbose(dbenv, flags, onoff);
	}
	__db_errx(env, DB_STR_A("1585", "unrecognized name-value pair: %s", "%s"), s);
	return EINVAL;
}
/*
 * __config_split --
 *	Split lines into white-space separated fields, returning the count of
 *	fields.
 *
 * PUBLIC: int __config_split __P((char *, char *[]));
 */
int __config_split(char * input, char * argv[CONFIG_SLOTS])
{
	int count;
	char ** ap;
	for(count = 0, ap = argv; (*ap = strsep(&input, " \t\n")) != NULL;)
		if(**ap != '\0') {
			++count;
			if(++ap == &argv[CONFIG_SLOTS-1]) {
				*ap = NULL;
				break;
			}
		}
	return count;
}
