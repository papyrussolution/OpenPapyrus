/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2007, 2011 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop

static void __rep_find_entry(ENV*, REP*, int, REP_LEASE_ENTRY**);
/*
 * __rep_update_grant -
 * Update a client's lease grant for this perm record
 *	and send the grant to the master.  Caller must
 *	hold the mtx_clientdb mutex.  Timespec given is in
 *	host local format.
 *
 * PUBLIC: int __rep_update_grant __P((ENV *, db_timespec *));
 */
int __rep_update_grant(ENV*env, db_timespec * ts)
{
	DBT lease_dbt;
	__rep_grant_info_args gi;
	db_timespec mytime;
	uint8 buf[__REP_GRANT_INFO_SIZE];
	int master, ret;
	size_t len;
	DB_REP * db_rep = env->rep_handle;
	REP * rep = db_rep->region;
	DB_LOG * dblp = env->lg_handle;
	LOG * lp = static_cast<LOG *>(dblp->reginfo.primary);
	timespecclear(&mytime);
	//
	// Get current time, and add in the (skewed) lease duration time to send the grant to the master.
	//
	__os_gettime(env, &mytime, 1);
	timespecadd(&mytime, &rep->lease_duration);
	REP_SYSTEM_LOCK(env);
	/*
	 * If we are in an election, we cannot grant the lease.
	 * We need to check under the region mutex.
	 */
	if(IN_ELECTION(rep)) {
		REP_SYSTEM_UNLOCK(env);
		return 0;
	}
	if(timespeccmp(&mytime, &rep->grant_expire, >))
		rep->grant_expire = mytime;
	F_CLR(rep, REP_F_LEASE_EXPIRED);
	REP_SYSTEM_UNLOCK(env);

	/*
	 * Send the LEASE_GRANT message with the current lease grant
	 * no matter if we've actually extended the lease or not.
	 */
	gi.msg_sec = (uint32)ts->tv_sec;
	gi.msg_nsec = (uint32)ts->tv_nsec;
	if((ret = __rep_grant_info_marshal(env, &gi, buf, __REP_GRANT_INFO_SIZE, &len)) != 0)
		return ret;
	DB_INIT_DBT(lease_dbt, buf, len);
	/*
	 * Don't send to the master if this site has zero priority because
	 * our site cannot count toward the data being safe.
	 */
	if((master = rep->master_id) != DB_EID_INVALID && rep->priority > 0)
		__rep_send_message(env, master, REP_LEASE_GRANT, &lp->max_perm_lsn, &lease_dbt, 0, 0);
	return 0;
}

/*
 * __rep_islease_granted -
 * Return 0 if this client has no outstanding lease granted.
 *	Return 1 otherwise.
 *	Caller must hold the REP_SYSTEM (region) mutex, and (rep_elect) relies
 * on us not dropping it.
 *
 * PUBLIC: int __rep_islease_granted(ENV *);
 */
int __rep_islease_granted(ENV*env)
{
	DB_REP * db_rep;
	REP * rep;
	db_timespec mytime;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	/*
	 * Get current time and compare against our granted lease.
	 */
	timespecclear(&mytime);
	__os_gettime(env, &mytime, 1);

	return timespeccmp(&mytime, &rep->grant_expire, <=) ? 1 : 0;
}

/*
 * __rep_lease_table_alloc -
 *	Allocate the lease table on a master.  Called with rep mutex
 * held.  We need to acquire the env region mutex, so we need to
 * make sure we never acquire those mutexes in the opposite order.
 *
 * PUBLIC: int __rep_lease_table_alloc __P((ENV *, uint32));
 */
int __rep_lease_table_alloc(ENV*env, uint32 nsites)
{
	REP_LEASE_ENTRY * le, * table;
	int * lease, ret;
	uint32 i;
	REP * rep = env->rep_handle->region;
	REGINFO * infop = env->reginfo;
	REGENV * renv = (REGENV *)infop->primary;
	MUTEX_LOCK(env, renv->mtx_regenv);
	/*
	 * If we have an old table from some other time, free it and
	 * allocate ourselves a new one that is known to be for
	 * the right number of sites.
	 */
	if(rep->lease_off != INVALID_ROFF) {
		__env_alloc_free(infop, R_ADDR(infop, rep->lease_off));
		rep->lease_off = INVALID_ROFF;
	}
	ret = __env_alloc(infop, (size_t)nsites*sizeof(REP_LEASE_ENTRY), &lease);
	MUTEX_UNLOCK(env, renv->mtx_regenv);
	if(ret)
		return ret;
	else
		rep->lease_off = R_OFFSET(infop, lease);
	table = (REP_LEASE_ENTRY *)R_ADDR(infop, rep->lease_off);
	for(i = 0; i < nsites; i++) {
		le = &table[i];
		le->eid = DB_EID_INVALID;
		timespecclear(&le->start_time);
		timespecclear(&le->end_time);
		ZERO_LSN(le->lease_lsn);
	}
	return 0;
}

/*
 * __rep_lease_grant -
 *	Handle incoming REP_LEASE_GRANT message on a master.
 *
 * PUBLIC: int __rep_lease_grant __P((ENV *, __rep_control_args *, DBT *, int));
 */
int __rep_lease_grant(ENV * env, __rep_control_args * rp, DBT * rec, int eid)
{
	DB_REP * db_rep;
	REP * rep;
	__rep_grant_info_args gi;
	REP_LEASE_ENTRY * le;
	db_timespec msg_time;
	int ret;
	db_rep = env->rep_handle;
	rep = db_rep->region;
	if((ret = __rep_grant_info_unmarshal(env, &gi, (uint8 *)rec->data, rec->size, NULL)) != 0)
		return ret;
	timespecset(&msg_time, gi.msg_sec, gi.msg_nsec);
	le = NULL;
	/*
	 * Get current time, and add in the (skewed) lease duration
	 * time to send the grant to the master.
	 */
	REP_SYSTEM_LOCK(env);
	__rep_find_entry(env, rep, eid, &le);
	/*
	 * We either get back this site's entry, or an empty entry
	 * that we need to initialize.
	 */
	DB_ASSERT(env, le != NULL);
	/*
	 * Update the entry if it is an empty entry or if the new
	 * lease grant is a later start time than the current one.
	 */
	VPRINT(env, (env, DB_VERB_REP_LEASE, "lease_grant: grant msg time %lu %lu",
		(ulong)msg_time.tv_sec, (ulong)msg_time.tv_nsec));
	if(le->eid == DB_EID_INVALID || timespeccmp(&msg_time, &le->start_time, >)) {
		le->eid = eid;
		le->start_time = msg_time;
		le->end_time = le->start_time;
		timespecadd(&le->end_time, &rep->lease_duration);
		VPRINT(env, (env, DB_VERB_REP_LEASE,
			     "lease_grant: eid %d, start %lu %lu, end %lu %lu, duration %lu %lu",
			     le->eid, (ulong)le->start_time.tv_sec, (ulong)le->start_time.tv_nsec,
			     (ulong)le->end_time.tv_sec, (ulong)le->end_time.tv_nsec,
			     (ulong)rep->lease_duration.tv_sec, (ulong)rep->lease_duration.tv_nsec));
	}
	/*
	 * Only update the lease table with a larger LSN value
	 * than the previous entry. This handles the case of a
	 * lagging record with a later start time, which is
	 * sometimes possible when a failed lease check resends
	 * the last permanent record.
	 */
	if(LOG_COMPARE(&rp->lsn, &le->lease_lsn) > 0) {
		le->lease_lsn = rp->lsn;
		VPRINT(env, (env, DB_VERB_REP_LEASE, "lease_grant: eid %d, lease_lsn [%lu][%lu]",
			le->eid, (ulong)le->lease_lsn.file, (ulong)le->lease_lsn.Offset_));
	}
	REP_SYSTEM_UNLOCK(env);
	return 0;
}
/*
 * Find the entry for the given EID.  Or the first empty one.
 */
static void __rep_find_entry(ENV*env, REP * rep, int eid, REP_LEASE_ENTRY ** lep)
{
	REGINFO * infop = env->reginfo;
	REP_LEASE_ENTRY * table = (REP_LEASE_ENTRY *)R_ADDR(infop, rep->lease_off);
	for(uint32 i = 0; i < rep->config_nsites; i++) {
		REP_LEASE_ENTRY * le = &table[i];
		/*
		 * Find either the one that matches the client's
		 * EID or the first empty one.
		 */
		if(le->eid == eid || le->eid == DB_EID_INVALID) {
			*lep = le;
			return;
		}
	}
}
/*
 * __rep_lease_check -
 * Return 0 if this master holds valid leases and can confirm
 *	its mastership.  If leases are expired, an attempt is made
 *	to refresh the leases.  If that fails, then return the
 *	DB_REP_LEASE_EXPIRED error to the user.  No mutexes held.
 *
 * PUBLIC: int __rep_lease_check __P((ENV *, int));
 */
int FASTCALL __rep_lease_check(ENV * env, int refresh)
{
	DB_LSN lease_lsn;
	REP_LEASE_ENTRY * le, * table;
	db_timespec curtime;
	int max_tries, ret;
	int tries = 0;
	uint32 i, min_leases, valid_leases;
	REGINFO * infop = env->reginfo;
	DB_REP * db_rep = env->rep_handle;
	REP * rep = db_rep->region;
	DB_LOG * dblp = env->lg_handle;
	LOG * lp = static_cast<LOG *>(dblp->reginfo.primary);
	LOG_SYSTEM_LOCK(env);
	lease_lsn = lp->max_perm_lsn;
	LOG_SYSTEM_UNLOCK(env);
#ifdef HAVE_STATISTICS
	rep->stat.st_lease_chk++;
#endif
	/*
	 * Set the maximum number of retries to be 2x the lease timeout
	 * so that if a site is waiting to sync, it has a chance to do so.
	 */
	max_tries = (int)(rep->lease_timeout/(LEASE_REFRESH_USEC/2));
	SETMAX(max_tries, LEASE_REFRESH_MIN);
retry:
	REP_SYSTEM_LOCK(env);
	min_leases = rep->config_nsites/2;
	ret = 0;
	__os_gettime(env, &curtime, 1);
	VPRINT(env, (env, DB_VERB_REP_LEASE, "%s %d of %d refresh %d min_leases %lu curtime %lu %lu, maxLSN [%lu][%lu]",
		"lease_check: try ", tries, max_tries, refresh, (ulong)min_leases, (ulong)curtime.tv_sec,
		(ulong)curtime.tv_nsec, (ulong)lease_lsn.file, (ulong)lease_lsn.Offset_));
	table = (REP_LEASE_ENTRY *)R_ADDR(infop, rep->lease_off);
	for(i = 0, valid_leases = 0; i < rep->config_nsites && valid_leases < min_leases; i++) {
		le = &table[i];
		/*
		 * Count this lease as valid if:
		 * - It is a valid entry (has an EID).
		 * - The lease has not expired.
		 * - The LSN is up to date.
		 */
		if(le->eid != DB_EID_INVALID) {
			VPRINT(env, (env, DB_VERB_REP_LEASE, "lease_check: valid %lu eid %d, lease_lsn [%lu][%lu]",
				(ulong)valid_leases, le->eid, (ulong)le->lease_lsn.file, (ulong)le->lease_lsn.Offset_));
			VPRINT(env, (env, DB_VERB_REP_LEASE, "lease_check: endtime %lu %lu", (ulong)le->end_time.tv_sec, (ulong)le->end_time.tv_nsec));
		}
		if(le->eid != DB_EID_INVALID && timespeccmp(&le->end_time, &curtime, >=) && LOG_COMPARE(&le->lease_lsn, &lease_lsn) >= 0)
			valid_leases++;
	}
	REP_SYSTEM_UNLOCK(env);
	//
	// Now see if we have enough.
	//
	VPRINT(env, (env, DB_VERB_REP_LEASE, "valid %lu, min %lu", (ulong)valid_leases, (ulong)min_leases));
	if(valid_leases < min_leases) {
#ifdef HAVE_STATISTICS
		rep->stat.st_lease_chk_misses++;
#endif
		if(!refresh || tries > max_tries)
			ret = DB_REP_LEASE_EXPIRED;
		else {
			/*
			 * If we are successful, we need to recheck the leases
			 * because the lease grant messages may have raced with
			 * the PERM acknowledgement.  Give the grant messages
			 * a chance to arrive and be processed.
			 */
			if(((tries%10) == 5 && (ret = __rep_lease_refresh(env)) == 0) || (tries%10) != 5) {
				/*
				 * If we were successful sending, but
				 * not in racing the message threads,
				 * then yield the processor so that
				 * the message threads get a chance
				 * to run.
				 */
				if(tries > 0)
					__os_yield(env, 0, LEASE_REFRESH_USEC);
				tries++;
#ifdef HAVE_STATISTICS
				rep->stat.st_lease_chk_refresh++;
#endif
				goto retry;
			}
		}
	}
	if(ret == DB_REP_LEASE_EXPIRED)
		RPRINT(env, (env, DB_VERB_REP_LEASE, "lease_check: Expired.  Only %lu valid", (ulong)valid_leases));
	return ret;
}
/*
 * __rep_lease_refresh -
 *	Find the last permanent record and send that out so that it
 *	forces clients to grant their leases.
 *
 *	If there is no permanent record, this function cannot refresh
 *	leases.  That should not happen because the master should write
 *	a checkpoint when it starts, if there is no other perm record.
 */
int __rep_lease_refresh(ENV*env)
{
	DBT rec;
	DB_LOGC * logc;
	DB_LSN lsn;
	int ret, t_ret;
	if((ret = __log_cursor(env, &logc)) != 0)
		return ret;
	memzero(&rec, sizeof(rec));
	memzero(&lsn, sizeof(lsn));
	/*
	 * Use __rep_log_backup to find the last PERM record.
	 */
	if((ret = __rep_log_backup(env, logc, &lsn, REP_REC_PERM)) != 0) {
		// If there is no PERM record, then we get DB_NOTFOUND.
		if(ret == DB_NOTFOUND)
			ret = 0;
		goto err;
	}
	if((ret = __logc_get(logc, &lsn, &rec, DB_CURRENT)) != 0)
		goto err;
	__rep_send_message(env, DB_EID_BROADCAST, REP_LOG, &lsn, &rec, REPCTL_LEASE, 0);
err:
	if((t_ret = __logc_close(logc)) != 0 && ret == 0)
		ret = t_ret;
	return ret;
}

/*
 * __rep_lease_expire -
 *	Proactively expire all leases granted to us.
 * Assume the caller holds the REP_SYSTEM (region) mutex.
 *
 * PUBLIC: int __rep_lease_expire(ENV *);
 */
int __rep_lease_expire(ENV * env)
{
	REP_LEASE_ENTRY * le;
	int ret = 0;
	DB_REP * db_rep = env->rep_handle;
	REP * rep = db_rep->region;
	REGINFO * infop = env->reginfo;
	if(rep->lease_off != INVALID_ROFF) {
		REP_LEASE_ENTRY * table = (REP_LEASE_ENTRY *)R_ADDR(infop, rep->lease_off);
		/*
		 * Expire all leases forcibly.  We are guaranteed that the
		 * start_time for all leases are not in the future.  Therefore,
		 * set the end_time to the start_time.
		 */
		for(uint32 i = 0; i < rep->config_nsites; i++) {
			le = &table[i];
			le->end_time = le->start_time;
		}
	}
	return ret;
}
/*
 * __rep_lease_waittime -
 *	Return the amount of time remaining on a granted lease.
 * Assume the caller holds the REP_SYSTEM (region) mutex.
 *
 * PUBLIC: db_timeout_t __rep_lease_waittime(ENV *);
 */
db_timeout_t __rep_lease_waittime(ENV*env)
{
	db_timespec mytime;
	DB_REP * db_rep = env->rep_handle;
	REP * rep = db_rep->region;
	db_timespec exptime = rep->grant_expire;
	db_timeout_t to = 0;
	/*
	 * If the lease has never been granted, we must wait a full
	 * lease timeout because we could be freshly rebooted after
	 * a crash and a lease could be granted from a previous
	 * incarnation of this client.  However, if the lease has never
	 * been granted, and this client has already waited a full
	 * lease timeout, we know our lease cannot be granted and there
	 * is no need to wait again.
	 */
	RPRINT(env, (env, DB_VERB_REP_LEASE, "wait_time: grant_expire %lu %lu lease_to %lu",
		(ulong)exptime.tv_sec, (ulong)exptime.tv_nsec, (ulong)rep->lease_timeout));
	if(!timespecisset(&exptime)) {
		if(!F_ISSET(rep, REP_F_LEASE_EXPIRED))
			to = rep->lease_timeout;
	}
	else {
		__os_gettime(env, &mytime, 1);
		RPRINT(env, (env, DB_VERB_REP_LEASE, "wait_time: mytime %lu %lu, grant_expire %lu %lu",
			(ulong)mytime.tv_sec, (ulong)mytime.tv_nsec, (ulong)exptime.tv_sec, (ulong)exptime.tv_nsec));
		if(timespeccmp(&mytime, &exptime, <=)) {
			//
			// If the current time is before the grant expiration
			// compute the difference and return remaining grant time.
			//
			timespecsub(&exptime, &mytime);
			DB_TIMESPEC_TO_TIMEOUT(to, &exptime, 1);
		}
	}
	return to;
}
