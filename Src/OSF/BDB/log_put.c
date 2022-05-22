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
#include "dbinc_auto/db_ext.h"

static int __log_encrypt_record(ENV*, DBT*, HDR*, uint32);
static int __log_file(ENV*, const DB_LSN*, char *, size_t);
static int __log_fill(DB_LOG*, DB_LSN*, void *, uint32);
static int __log_flush_commit(ENV*, const DB_LSN*, uint32);
static int __log_newfh(DB_LOG*, int);
static int __log_put_next(ENV*, DB_LSN*, const DBT*, HDR*, DB_LSN *);
static int __log_put_record_int(ENV*, DB*, DB_TXN*, DB_LSN*, uint32, uint32, uint32, uint32, DB_LOG_RECSPEC*, va_list);
static int __log_putr(DB_LOG*, DB_LSN*, const DBT*, uint32, HDR *);
static int __log_write(DB_LOG*, void *, uint32);
/*
 * __log_put_pp --
 *	ENV->log_put pre/post processing.
 */
int __log_put_pp(DB_ENV * dbenv, DB_LSN * lsnp, const DBT * udbt, uint32 flags)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->lg_handle, "DB_ENV->log_put", DB_INIT_LOG);
	// Validate arguments: check for allowed flags
	if((ret = __db_fchk(env, "DB_ENV->log_put", flags, DB_LOG_CHKPNT|DB_LOG_COMMIT| DB_FLUSH|DB_LOG_NOCOPY|DB_LOG_WRNOSYNC)) != 0)
		return ret;
	/* DB_LOG_WRNOSYNC and DB_FLUSH are mutually exclusive. */
	if(LF_ISSET(DB_LOG_WRNOSYNC) && LF_ISSET(DB_FLUSH))
		return __db_ferr(env, "DB_ENV->log_put", 1);
	/* Replication clients should never write log records. */
	if(IS_REP_CLIENT(env)) {
		__db_errx(env, DB_STR("2511", "DB_ENV->log_put is illegal on replication clients"));
		return EINVAL;
	}
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__log_put(env, lsnp, udbt, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __log_put --
 *	ENV->log_put.
 *
 * PUBLIC: int __log_put __P((ENV *, DB_LSN *, const DBT *, uint32));
 */
int __log_put(ENV * env, DB_LSN * lsnp, const DBT * udbt, uint32 flags)
{
	DB_LSN lsn, old_lsn;
	HDR    hdr;
	int    ret;
	uint8 * key;
	DB_LOG * dblp = env->lg_handle;
	LOG * lp = static_cast<LOG *>(dblp->reginfo.primary);
	DB_CIPHER * db_cipher = env->crypto_handle;
	DB_REP * db_rep = env->rep_handle;
	REP  * rep = db_rep ? db_rep->region : NULL;
	DBT    t = *udbt;
	DBT  * dbt = &t;
	int    lock_held = 0;
	int    need_free = 0;
	ZERO_LSN(old_lsn);
	hdr.len = hdr.prev = 0;
	/*
	 * In general, if we are not a rep application, but are sharing a master
	 * rep env, we should not be writing log records.  However, we can allow
	 * a non-replication-aware process to join a pre-existing repmgr
	 * environment, if env handle meets repmgr's DB_THREAD requirement.
	 */
	if(IS_REP_MASTER(env) && db_rep->send == NULL) {
#ifdef HAVE_REPLICATION_THREADS
		if(F_ISSET(env, ENV_THREAD) && APP_IS_REPMGR(env)) {
			if((ret = __repmgr_autostart(env)) != 0)
				return ret;
		}
		else
#endif
		{
#if !defined(DEBUG_ROP) && !defined(DEBUG_WOP)
			__db_errx(env, DB_STR("2512", "Non-replication DB_ENV handle attempting to modify a replicated environment"));
			return EINVAL;
#endif
		}
	}
	DB_ASSERT(env, !IS_REP_CLIENT(env));
	/*
	 * If we are coming from the logging code, we use an internal flag,
	 * DB_LOG_NOCOPY, because we know we can overwrite/encrypt the log
	 * record in place.  Otherwise, if a user called log_put then we
	 * must copy it to new memory so that we know we can write it.
	 *
	 * We also must copy it to new memory if we are a replication master
	 * so that we retain an unencrypted copy of the log record to send
	 * to clients.
	 */
	if(!LF_ISSET(DB_LOG_NOCOPY) || IS_REP_MASTER(env)) {
		if(CRYPTO_ON(env))
			t.size += db_cipher->adj_size(udbt->size);
		if((ret = __os_calloc(env, 1, t.size, &t.data)) != 0)
			goto err;
		need_free = 1;
		memcpy(t.data, udbt->data, udbt->size);
	}
	if((ret = __log_encrypt_record(env, dbt, &hdr, udbt->size)) != 0)
		goto err;
	if(CRYPTO_ON(env))
		key = db_cipher->mac_key;
	else
		key = NULL;
#ifdef HAVE_LOG_CHECKSUM
	__db_chksum(&hdr, (uint8 *)dbt->data, dbt->size, key, hdr.chksum);
#endif
	LOG_SYSTEM_LOCK(env);
	lock_held = 1;
	if((ret = __log_put_next(env, &lsn, dbt, &hdr, &old_lsn)) != 0)
		goto panic_check;
	/*
	 * Assign the return LSN before dropping the region lock.  Necessary
	 * in case the lsn is a begin_lsn from a TXN_DETAIL structure passed in
	 * by the logging routines.  We use atomic 32-bit operations because
	 * during commit this will be a TXN_DETAIL visible_lsn field, and MVCC
	 * relies on reading the fields atomically.
	 */
	lsnp->file = lsn.file;
	lsnp->Offset_ = lsn.Offset_;
#ifdef HAVE_REPLICATION
	if(IS_REP_MASTER(env)) {
		__rep_newfile_args nf_args;
		DBT newfiledbt;
		REP_BULK bulk;
		size_t len;
		uint32 ctlflags;
		uint8 buf[__REP_NEWFILE_SIZE];
		/*
		 * Replication masters need to drop the lock to send messages,
		 * but want to drop and reacquire it a minimal number of times.
		 */
		ctlflags = LF_ISSET(DB_LOG_COMMIT|DB_LOG_CHKPNT) ? REPCTL_PERM : 0;
		LOG_SYSTEM_UNLOCK(env);
		lock_held = 0;
		if(LF_ISSET(DB_FLUSH))
			ctlflags |= REPCTL_FLUSH;
		/*
		 * If we changed files and we're in a replicated environment,
		 * we need to inform our clients now that we've dropped the
		 * region lock.
		 *
		 * Note that a failed NEWFILE send is a dropped message that
		 * our client can handle, so we can ignore it.  It's possible
		 * that the record we already put is a commit, so we don't just
		 * want to return failure.
		 */
		if(!IS_ZERO_LSN(old_lsn)) {
			memzero(&newfiledbt, sizeof(newfiledbt));
			nf_args.version = lp->persist.version;
			__rep_newfile_marshal(env, &nf_args, buf, __REP_NEWFILE_SIZE, &len);
			DB_INIT_DBT(newfiledbt, buf, len);
			__rep_send_message(env, DB_EID_BROADCAST, REP_NEWFILE, &old_lsn, &newfiledbt, 0, 0);
		}
		/*
		 * If we're doing bulk processing put it in the bulk buffer.
		 */
		ret = 0;
		if(FLD_ISSET(rep->config, REP_C_BULK)) {
			/*
			 * Bulk could have been turned on by another process.
			 * If so, set the address into the bulk region now.
			 */
			SETIFZ(db_rep->bulk, (uint8 *)R_ADDR(&dblp->reginfo, lp->bulk_buf));
			memzero(&bulk, sizeof(bulk));
			bulk.addr = db_rep->bulk;
			bulk.offp = &lp->bulk_off;
			bulk.len = lp->bulk_len;
			bulk.lsn = lsn;
			bulk.type = REP_BULK_LOG;
			bulk.eid = DB_EID_BROADCAST;
			bulk.flagsp = &lp->bulk_flags;
			ret = __rep_bulk_message(env, &bulk, NULL, &lsn, udbt, ctlflags);
		}
		if(!FLD_ISSET(rep->config, REP_C_BULK) || ret == DB_REP_BULKOVF) {
			/*
			 * Then send the log record itself on to our clients.
			 */
			/*
			 * !!!
			 * In the crypto case, we MUST send the udbt, not the
			 * now-encrypted dbt.  Clients have no way to decrypt
			 * without the header.
			 */
			ret = __rep_send_message(env, DB_EID_BROADCAST, REP_LOG, &lsn, udbt, ctlflags, 0);
		}
		if(FLD_ISSET(ctlflags, REPCTL_PERM)) {
			LOG_SYSTEM_LOCK(env);
 #ifdef HAVE_STATISTICS
			if(IS_USING_LEASES(env))
				rep->stat.st_lease_sends++;
 #endif
			/*
			 * Keep track of our last PERM lsn.  Set this on a
			 * master under the log lock.  When using leases, if
			 * we set max_perm_lsn too early (before the send)
			 * then we hit a lot of false invalid lease checks
			 * which all try to refresh and hurt performance.
			 */
			if(LOG_COMPARE(&lp->max_perm_lsn, &lsn) < 0)
				lp->max_perm_lsn = lsn;
			LOG_SYSTEM_UNLOCK(env);
		}
		/*
		 * If the send fails and we're a commit or checkpoint,
		 * there's nothing we can do;  the record's in the log.
		 * Flush it, even if we're running with TXN_NOSYNC,
		 * on the grounds that it should be in durable
		 * form somewhere.
		 */
		if(ret != 0 && FLD_ISSET(ctlflags, REPCTL_PERM))
			LF_SET(DB_FLUSH);
		/*
		 * We ignore send failures so reset 'ret' to 0 here.
		 * We needed to check special return values from
		 * bulk transfer and errors from either bulk or normal
		 * message sending need flushing on perm records.  But
		 * otherwise we need to ignore it and reset it now.
		 */
		ret = 0;
	}
#endif
	/*
	 * If needed, do a flush.  Note that failures at this point
	 * are only permissible if we know we haven't written a commit
	 * record;  __log_flush_commit is responsible for enforcing this.
	 *
	 * If a flush is not needed, see if WRITE_NOSYNC was set and we
	 * need to write out the log buffer.
	 */
	if(LF_ISSET(DB_FLUSH|DB_LOG_WRNOSYNC)) {
		if(!lock_held) {
			LOG_SYSTEM_LOCK(env);
			lock_held = 1;
		}
		if((ret = __log_flush_commit(env, &lsn, flags)) != 0)
			goto panic_check;
	}
	/*
	 * If flushed a checkpoint record, reset the "bytes since the last
	 * checkpoint" counters.
	 */
	if(LF_ISSET(DB_LOG_CHKPNT))
		lp->stat.st_wc_bytes = lp->stat.st_wc_mbytes = 0;
	/* Increment count of records added to the log. */
	STAT(++lp->stat.st_record);
	if(0) {
panic_check:    /*
		 * Writing log records cannot fail if we're a replication
		 * master.  The reason is that once we send the record to
		 * replication clients, the transaction can no longer
		 * abort, otherwise the master would be out of sync with
		 * the rest of the replication group.  Panic the system.
		 */
		if(ret != 0 && IS_REP_MASTER(env))
			ret = __env_panic(env, ret);
	}
err:    if(lock_held)
		LOG_SYSTEM_UNLOCK(env);
	if(need_free)
		__os_free(env, dbt->data);
	/*
	 * If auto-remove is set and we switched files, remove unnecessary
	 * log files.
	 */
	if(ret == 0 && !IS_ZERO_LSN(old_lsn) && lp->db_log_autoremove)
		__log_autoremove(env);
	return ret;
}

/*
 * __log_current_lsn_int --
 *	internal operations of __log_current_lsn
 *
 * PUBLIC: int __log_current_lsn_int
 * PUBLIC:     __P((ENV *, DB_LSN *, uint32 *, uint32 *));
 */
int __log_current_lsn_int(ENV * env, DB_LSN * lsnp, uint32 * mbytesp, uint32 * bytesp)
{
	DB_LOG * dblp = env->lg_handle;
	LOG * lp = static_cast<LOG *>(dblp->reginfo.primary);
	LOG_SYSTEM_LOCK(env);
	/*
	 * We need the LSN of the last entry in the log.
	 *
	 * Typically, it's easy to get the last written LSN, you simply look
	 * at the current log pointer and back up the number of bytes of the
	 * last log record.  However, if the last thing we did was write the
	 * log header of a new log file, then, this doesn't work, so we return
	 * the first log record that will be written in this new file.
	 */
	*lsnp = lp->lsn;
	if(lp->lsn.Offset_ > lp->len)
		lsnp->Offset_ -= lp->len;
	/*
	 * Since we're holding the log region lock, return the bytes put into
	 * the log since the last checkpoint, transaction checkpoint needs it.
	 *
	 * We add the current buffer offset so as to count bytes that have not
	 * yet been written, but are sitting in the log buffer.
	 */
	if(mbytesp != NULL) {
		*mbytesp = lp->stat.st_wc_mbytes;
		*bytesp = (uint32)(lp->stat.st_wc_bytes+lp->b_off);
	}
	LOG_SYSTEM_UNLOCK(env);
	return 0;
}
/*
 * __log_current_lsn --
 *	Return the current LSN.
 *
 * PUBLIC: int __log_current_lsn
 * PUBLIC:     __P((ENV *, DB_LSN *, uint32 *, uint32 *));
 */
int __log_current_lsn(ENV * env, DB_LSN * lsnp, uint32 * mbytesp, uint32 * bytesp)
{
	DB_THREAD_INFO * ip;
	int ret = 0;
	ENV_ENTER(env, ip);
	ret = __log_current_lsn_int(env, lsnp, mbytesp, bytesp);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __log_put_next --
 *	Put the given record as the next in the log, wherever that may
 * turn out to be.
 */
static int __log_put_next(ENV * env, DB_LSN * lsn, const DBT * dbt, HDR * hdr, DB_LSN * old_lsnp)
{
	int ret;
	DB_LOG * dblp = env->lg_handle;
	LOG * lp = static_cast<LOG *>(dblp->reginfo.primary);
	/*
	 * Save a copy of lp->lsn before we might decide to switch log
	 * files and change it.  If we do switch log files, and we're
	 * doing replication, we'll need to tell our clients about the
	 * switch, and they need to receive a NEWFILE message
	 * with this "would-be" LSN in order to know they're not
	 * missing any log records.
	 */
	DB_LSN old_lsn = lp->lsn;
	int newfile = 0;
	int adv_file = 0;
	/*
	 * If our current log is at an older version and we want to write
	 * a record then we need to advance the log.
	 */
	if(lp->persist.version != DB_LOGVERSION) {
		__log_set_version(env, DB_LOGVERSION);
		adv_file = 1;
	}
	/*
	 * If this information won't fit in the file, or if we're a
	 * replication client environment and have been told to do so,
	 * swap files.
	 */
	if(adv_file || lp->lsn.Offset_ == 0 || (lp->lsn.Offset_+hdr->size+dbt->size) > lp->log_size) {
		if(hdr->size+sizeof(LOGP)+dbt->size > lp->log_size) {
			__db_errx(env, DB_STR_A("2513", "DB_ENV->log_put: record larger than maximum file size (%lu > %lu)", "%lu %lu"),
				(ulong)hdr->size+sizeof(LOGP)+dbt->size, (ulong)lp->log_size);
			return EINVAL;
		}
		if((ret = __log_newfile(dblp, NULL, 0, 0)) != 0)
			return ret;
		/*
		 * Flag that we switched files, in case we're a master
		 * and need to send this information to our clients.
		 * We postpone doing the actual send until we can
		 * safely release the log region lock and are doing so
		 * anyway.
		 */
		newfile = 1;
	}
	/* If we switched log files, let our caller know where. */
	if(newfile)
		*old_lsnp = old_lsn;
	/* Actually put the record. */
	return __log_putr(dblp, lsn, dbt, lp->lsn.Offset_-lp->len, hdr);
}
/*
 * __log_flush_commit --
 *	Flush a record.
 */
static int __log_flush_commit(ENV * env, const DB_LSN * lsnp, uint32 flags)
{
	HDR hdr;
	int t_ret;
	size_t nr, nw;
	uint8 * buffer;
	DB_LOG * dblp = env->lg_handle;
	LOG * lp = static_cast<LOG *>(dblp->reginfo.primary);
	DB_LSN flush_lsn = *lsnp;
	int ret = 0;
	/*
	 * DB_FLUSH:
	 *	Flush a record for which the DB_FLUSH flag to log_put was set.
	 *
	 * DB_LOG_WRNOSYNC:
	 *	If there's anything in the current log buffer, write it out.
	 */
	if(LF_ISSET(DB_FLUSH))
		ret = __log_flush_int(dblp, &flush_lsn, 1);
	else if(!lp->db_log_inmemory && lp->b_off != 0)
		if((ret = __log_write(dblp, dblp->bufp, (uint32)lp->b_off)) == 0)
			lp->b_off = 0;
	/*
	 * If a flush supporting a transaction commit fails, we must abort the
	 * transaction.  (If we aren't doing a commit, return the failure; if
	 * if the commit we care about made it to disk successfully, we just
	 * ignore the failure, because there's no way to undo the commit.)
	 */
	if(ret == 0 || !LF_ISSET(DB_LOG_COMMIT))
		return ret;
	if(LF_ISSET(DB_FLUSH) ? flush_lsn.file != lp->s_lsn.file || flush_lsn.Offset_ < lp->s_lsn.Offset_ : flush_lsn.file != lp->lsn.file || flush_lsn.Offset_ < lp->w_off)
		return 0;
	if(IS_REP_MASTER(env)) {
		__db_err(env, ret, DB_STR("2514", "Write failed on MASTER commit."));
		return __env_panic(env, ret);
	}
	/*
	 * Else, make sure that the commit record does not get out after we
	 * abort the transaction.  Do this by overwriting the commit record
	 * in the buffer.  (Note that other commits in this buffer will wait
	 * until a successful write happens, we do not wake them.)  We point
	 * at the right part of the buffer and write an abort record over the
	 * commit.  We must then try and flush the buffer again, since the
	 * interesting part of the buffer may have actually made it out to
	 * disk before there was a failure, we can't know for sure.
	 */
	if(flush_lsn.Offset_ > lp->w_off) {
		if((t_ret = __txn_force_abort(env, dblp->bufp+flush_lsn.Offset_-lp->w_off)) != 0)
			return __env_panic(env, t_ret);
	}
	else {
		/*
		 * The buffer was written, but its not on disk, we
		 * must read it back and force things from a commit
		 * state to an abort state.  Lots of things could fail
		 * here and we will be left with a commit record but
		 * a panic return.
		 */
		if((t_ret = __os_seek(env, dblp->lfhp, 0, 0, flush_lsn.Offset_)) != 0 || (t_ret = __os_read(env, dblp->lfhp, &hdr, HDR_NORMAL_SZ, &nr)) != 0 || nr != HDR_NORMAL_SZ)
			return __env_panic(env, t_ret == 0 ? EIO : t_ret);
		if(LOG_SWAPPED(env))
			__log_hdrswap(&hdr, CRYPTO_ON(env));
		if((t_ret = __os_malloc(env, hdr.len, &buffer)) != 0 || (t_ret = __os_seek(env,
			    dblp->lfhp, 0, 0, flush_lsn.Offset_)) != 0 || (t_ret = __os_read(env, dblp->lfhp, buffer,
			    hdr.len, &nr)) != 0 || nr != hdr.len || (t_ret = __txn_force_abort(env, buffer)) != 0 || (t_ret = __os_seek(env,
			    dblp->lfhp, 0, 0, flush_lsn.Offset_)) != 0 || (t_ret = __os_write(env, dblp->lfhp, buffer, nr, &nw)) != 0 || nw != nr)
			return __env_panic(env, t_ret == 0 ? EIO : t_ret);
		__os_free(env, buffer);
	}
	/*
	 * Try to flush the log again, if the disk just bounced then we
	 * want to be sure it does not go away again before we write the
	 * abort record.
	 */
	__log_flush_int(dblp, &flush_lsn, 0);
	return ret;
}
/*
 * __log_newfile --
 *	Initialize and switch to a new log file.  (Note that this is
 * called both when no log yet exists and when we fill a log file.)
 *
 * PUBLIC: int __log_newfile __P((DB_LOG *, DB_LSN *, uint32, uint32));
 */
int __log_newfile(DB_LOG * dblp, DB_LSN * lsnp, uint32 logfile, uint32 version)
{
	DBT t;
	DB_CIPHER * db_cipher;
	DB_LSN lsn;
	HDR hdr;
	LOGP * tpersist;
	int need_free, ret;
	uint32 lastoff;
	size_t tsize;
	ENV * env = dblp->env;
	LOG * lp = static_cast<LOG *>(dblp->reginfo.primary);
	/*
	 * If we're not specifying a specific log file number and we're
	 * not at the beginning of a file already, start a new one.
	 */
	if(logfile == 0 && lp->lsn.Offset_) {
		/*
		 * Flush the log so this file is out and can be closed.  We
		 * cannot release the region lock here because we need to
		 * protect the end of the file while we switch.  In
		 * particular, a thread with a smaller record than ours
		 * could detect that there is space in the log. Even
		 * blocking that event by declaring the file full would
		 * require all threads to wait here so that the lsn.file
		 * can be moved ahead after the flush completes.  This
		 * probably can be changed if we had an lsn for the
		 * previous file and one for the current, but it does not
		 * seem like this would get much more throughput, if any.
		 */
		if((ret = __log_flush_int(dblp, NULL, 0)) != 0)
			return ret;
		/*
		 * Save the last known offset from the previous file, we'll
		 * need it to initialize the persistent header information.
		 */
		lastoff = lp->lsn.Offset_;
		/* Point the current LSN to the new file. */
		++lp->lsn.file;
		lp->lsn.Offset_ = 0;
		/* Reset the file write offset. */
		lp->w_off = 0;
	}
	else
		lastoff = 0;
	/*
	 * Replication may require we reset the log file name space entirely.
	 * In that case we also force a file switch so that replication can
	 * clean up old files.
	 */
	if(logfile != 0) {
		lp->lsn.file = logfile;
		lp->lsn.Offset_ = 0;
		lp->w_off = 0;
		if(lp->db_log_inmemory) {
			lsn = lp->lsn;
			__log_zero(env, &lsn);
		}
		else {
			lp->s_lsn = lp->lsn;
			if((ret = __log_newfh(dblp, 1)) != 0)
				return ret;
		}
	}
	DB_ASSERT(env, lp->db_log_inmemory || lp->b_off == 0);
	if(lp->db_log_inmemory &&
	   (ret = __log_inmem_newfile(dblp, lp->lsn.file)) != 0)
		return ret;
	/*
	 * Insert persistent information as the first record in every file.
	 * Note that the previous length is wrong for the very first record
	 * of the log, but that's okay, we check for it during retrieval.
	 */
	memzero(&t, sizeof(t));
	memzero(&hdr, sizeof(HDR));
	need_free = 0;
	tsize = sizeof(LOGP);
	db_cipher = env->crypto_handle;
	if(CRYPTO_ON(env))
		tsize += db_cipher->adj_size(tsize);
	if((ret = __os_calloc(env, 1, tsize, &tpersist)) != 0)
		return ret;
	need_free = 1;
	/*
	 * If we're told what version to make this file, then we
	 * need to be at that version.  Update here.
	 */
	if(version != 0) {
		__log_set_version(env, version);
		if((ret = __env_init_rec(env, version)) != 0)
			goto err;
	}
	lp->persist.log_size = lp->log_size = lp->log_nsize;
	memcpy(tpersist, &lp->persist, sizeof(LOGP));
	DB_SET_DBT(t, tpersist, tsize);
	if(LOG_SWAPPED(env))
		__log_persistswap(tpersist);
	if((ret = __log_encrypt_record(env, &t, &hdr, (uint32)tsize)) != 0)
		goto err;
#ifdef HAVE_LOG_CHECKSUM
	if(lp->persist.version != DB_LOGVERSION)
		__db_chksum(NULL, (uint8 *)t.data, t.size, (CRYPTO_ON(env)) ? db_cipher->mac_key : NULL, hdr.chksum);
	else
		__db_chksum(&hdr, (uint8 *)t.data, t.size, (CRYPTO_ON(env)) ? db_cipher->mac_key : NULL, hdr.chksum);
#endif
	if((ret = __log_putr(dblp, &lsn, &t, lastoff == 0 ? 0 : lastoff-lp->len, &hdr)) != 0)
		goto err;
	ASSIGN_PTR(lsnp, lp->lsn); // Update the LSN information returned to the caller
err:
	if(need_free)
		__os_free(env, tpersist);
	return ret;
}
/*
 * __log_putr --
 *	Actually put a record into the log.
 */
static int __log_putr(DB_LOG * dblp, DB_LSN * lsn, const DBT * dbt, uint32 prev, HDR * h)
{
	DB_LSN f_lsn;
	HDR tmp, * hdr;
	int ret, t_ret;
	db_size_t b_off;
	size_t nr;
	uint32 w_off;
	ENV * env = dblp->env;
	LOG * lp = static_cast<LOG *>(dblp->reginfo.primary);
	/*
	 * If we weren't given a header, use a local one.
	 */
	DB_CIPHER * db_cipher = env->crypto_handle;
	if(!h) {
		hdr = &tmp;
		memzero(hdr, sizeof(HDR));
		hdr->size = CRYPTO_ON(env) ? HDR_CRYPTO_SZ : HDR_NORMAL_SZ;
	}
	else
		hdr = h;
	/* Save our position in case we fail. */
	b_off = lp->b_off;
	w_off = lp->w_off;
	f_lsn = lp->f_lsn;
	/*
	 * Initialize the header.  If we just switched files, lsn.offset will
	 * be 0, and what we really want is the offset of the previous record
	 * in the previous file.  Fortunately, prev holds the value we want.
	 */
	hdr->prev = prev;
	hdr->len = (uint32)hdr->size+dbt->size;
	/*
	 * If we were passed in a nonzero checksum, our caller calculated
	 * the checksum before acquiring the log mutex, as an optimization.
	 *
	 * If our caller calculated a real checksum of 0, we'll needlessly
	 * recalculate it.  C'est la vie;  there's no out-of-bounds value
	 * here.
	 */
	if(hdr->chksum[0] == 0) {
#ifdef HAVE_LOG_CHECKSUM
		if(lp->persist.version != DB_LOGVERSION)
			__db_chksum(NULL, (uint8 *)dbt->data, dbt->size, (CRYPTO_ON(env)) ? db_cipher->mac_key : NULL, hdr->chksum);
		else
			__db_chksum(hdr, (uint8 *)dbt->data, dbt->size, (CRYPTO_ON(env)) ? db_cipher->mac_key : NULL, hdr->chksum);
#endif
	}
	else if(lp->persist.version == DB_LOGVERSION) {
		/*
		 * We need to correct for prev and len since they are not
		 * set before here.
		 */
		LOG_HDR_SUM(CRYPTO_ON(env), hdr, hdr->chksum);
	}
	if(lp->db_log_inmemory && (ret = __log_inmem_chkspace(dblp, (uint32)hdr->size+dbt->size)) != 0)
		goto err;
	/*
	 * The offset into the log file at this point is the LSN where
	 * we're about to put this record, and is the LSN the caller wants.
	 */
	*lsn = lp->lsn;
	nr = hdr->size;
	if(LOG_SWAPPED(env))
		__log_hdrswap(hdr, CRYPTO_ON(env));
	/* nr can't overflow a 32 bit value - header size is internal. */
	ret = __log_fill(dblp, lsn, hdr, (uint32)nr);
	if(LOG_SWAPPED(env))
		__log_hdrswap(hdr, CRYPTO_ON(env));
	if(ret != 0)
		goto err;
	if((ret = __log_fill(dblp, lsn, dbt->data, dbt->size)) != 0)
		goto err;
	lp->len = (uint32)(hdr->size+dbt->size);
	lp->lsn.Offset_ += lp->len;
	return 0;
err:
	/*
	 * If we wrote more than one buffer before failing, get the
	 * first one back.  The extra buffers will fail the checksums
	 * and be ignored.
	 */
	if(w_off+lp->buffer_size < lp->w_off) {
		DB_ASSERT(env, !lp->db_log_inmemory);
		if((t_ret = __os_seek(env, dblp->lfhp, 0, 0, w_off)) != 0 || (t_ret = __os_read(env, dblp->lfhp, dblp->bufp, b_off, &nr)) != 0)
			return __env_panic(env, t_ret);
		if(nr != b_off) {
			__db_errx(env, DB_STR("2515", "Short read while restoring log"));
			return __env_panic(env, EIO);
		}
	}
	/* Reset to where we started. */
	lp->w_off = w_off;
	lp->b_off = b_off;
	lp->f_lsn = f_lsn;
	return ret;
}
//
// ENV->log_flush pre/post processing.
//
int __log_flush_pp(DB_ENV * dbenv, const DB_LSN * lsn)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->lg_handle, "DB_ENV->log_flush", DB_INIT_LOG);
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__log_flush(env, lsn)), 0, ret);
	ENV_LEAVE(env, ip);
	return ret;
}

/*
 * See if we need to wait.  s_lsn is not locked so some care is needed.
 * The sync point can only move forward.  The lsnp->file cannot be
 * greater than the s_lsn.file.  If the file we want is in the past
 * we are done.  If the file numbers are the same check the offset.
 * This all assumes we can read an 32-bit quantity in one state or
 * the other, not in transition.
 */
#define ALREADY_FLUSHED(lp, lsnp) (((lp)->s_lsn.file > (lsnp)->file) || ((lp)->s_lsn.file == (lsnp)->file && (lp)->s_lsn.Offset_ > (lsnp)->Offset_))
/*
 * __log_flush --
 *	ENV->log_flush
 *
 * PUBLIC: int __log_flush __P((ENV *, const DB_LSN *));
 */
int __log_flush(ENV * env, const DB_LSN * lsn)
{
	int ret;
	DB_LOG * dblp = env->lg_handle;
	LOG * lp = static_cast<LOG *>(dblp->reginfo.primary);
	if(lsn != NULL && ALREADY_FLUSHED(lp, lsn))
		return 0;
	LOG_SYSTEM_LOCK(env);
	ret = __log_flush_int(dblp, lsn, 1);
	LOG_SYSTEM_UNLOCK(env);
	return ret;
}
//
// __log_flush_int --
// Write all records less than or equal to the specified LSN; internal version.
//
int __log_flush_int(DB_LOG * dblp, const DB_LSN * lsnp, int release)
{
	struct __db_commit * commit;
	DB_LSN flush_lsn, f_lsn;
	size_t b_off;
	uint32 w_off;
	int do_flush, first;
	ENV * env = dblp->env;
	LOG * lp = static_cast<LOG *>(dblp->reginfo.primary);
	uint32 ncommit = 0;
	int ret = 0;
	if(lp->db_log_inmemory) {
		lp->s_lsn = lp->lsn;
		STAT(++lp->stat.st_scount);
		return 0;
	}
	/*
	 * If no LSN specified, flush the entire log by setting the flush LSN
	 * to the last LSN written in the log.  Otherwise, check that the LSN
	 * isn't a non-existent record for the log.
	 */
	if(lsnp == NULL) {
		flush_lsn.file = lp->lsn.file;
		flush_lsn.Offset_ = lp->lsn.Offset_-lp->len;
	}
	else if(lsnp->file > lp->lsn.file || (lsnp->file == lp->lsn.file && lsnp->Offset_ > (lp->lsn.Offset_-lp->len))) {
		__db_errx(env, DB_STR_A("2516", "DB_ENV->log_flush: LSN of %lu/%lu past current end-of-log of %lu/%lu", "%lu %lu %lu %lu"), (ulong)lsnp->file,
			(ulong)lsnp->Offset_, (ulong)lp->lsn.file, (ulong)lp->lsn.Offset_);
		__db_errx(env, DB_STR("2517", "Database environment corrupt; the wrong log files may have been removed or incompatible database files imported from another environment"));
		return __env_panic(env, DB_RUNRECOVERY);
	}
	else {
		if(ALREADY_FLUSHED(lp, lsnp))
			return 0;
		flush_lsn = *lsnp;
	}
	//
	// If a flush is in progress and we're allowed to do so, drop
	// the region lock and block waiting for the next flush.
	//
	if(release && lp->in_flush != 0) {
		if((commit = SH_TAILQ_FIRST(&lp->free_commits, __db_commit)) == NULL) {
			if((ret = __env_alloc(&dblp->reginfo, sizeof(struct __db_commit), &commit)) != 0)
				goto flush;
			memzero(commit, sizeof(*commit));
			if((ret = __mutex_alloc(env, MTX_TXN_COMMIT, DB_MUTEX_SELF_BLOCK, &commit->mtx_txnwait)) != 0) {
				__env_alloc_free(&dblp->reginfo, commit);
				return ret;
			}
			MUTEX_LOCK(env, commit->mtx_txnwait);
		}
		else
			SH_TAILQ_REMOVE(&lp->free_commits, commit, links, __db_commit);
		lp->ncommit++;
		//
		// Flushes may be requested out of LSN order;  be sure we only move lp->t_lsn forward.
		//
		if(LOG_COMPARE(&lp->t_lsn, &flush_lsn) < 0)
			lp->t_lsn = flush_lsn;
		commit->lsn = flush_lsn;
		SH_TAILQ_INSERT_HEAD(&lp->commits, commit, links, __db_commit);
		LOG_SYSTEM_UNLOCK(env);
		/* Wait here for the in-progress flush to finish. */
		MUTEX_LOCK(env, commit->mtx_txnwait);
		LOG_SYSTEM_LOCK(env);
		lp->ncommit--;
		/*
		 * Grab the flag before freeing the struct to see if
		 * we need to flush the log to commit.  If so,
		 * use the maximal lsn for any committing thread.
		 */
		do_flush = F_ISSET(commit, DB_COMMIT_FLUSH);
		F_CLR(commit, DB_COMMIT_FLUSH);
		SH_TAILQ_INSERT_HEAD(&lp->free_commits, commit, links, __db_commit);
		if(do_flush) {
			lp->in_flush--;
			flush_lsn = lp->t_lsn;
		}
		else
			return 0;
	}
	/*
	 * Protect flushing with its own mutex so we can release
	 * the region lock except during file switches.
	 */
flush:
	MUTEX_LOCK(env, lp->mtx_flush);
	/*
	 * If the LSN is less than or equal to the last-sync'd LSN, we're done.
	 * Note, the last-sync LSN saved in s_lsn is the LSN of the first byte
	 * after the byte we absolutely know was written to disk, so the test
	 * is <, not <=.
	 */
	if(flush_lsn.file < lp->s_lsn.file || (flush_lsn.file == lp->s_lsn.file && flush_lsn.Offset_ < lp->s_lsn.Offset_)) {
		MUTEX_UNLOCK(env, lp->mtx_flush);
		goto done;
	}
	/*
	 * We may need to write the current buffer.  We have to write the
	 * current buffer if the flush LSN is greater than or equal to the
	 * buffer's starting LSN.
	 *
	 * Otherwise, it's still possible that this thread may never have
	 * written to this log file.  Acquire a file descriptor if we don't
	 * already have one.
	 */
	if(lp->b_off != 0 && LOG_COMPARE(&flush_lsn, &lp->f_lsn) >= 0) {
		if((ret = __log_write(dblp, dblp->bufp, (uint32)lp->b_off)) != 0) {
			MUTEX_UNLOCK(env, lp->mtx_flush);
			goto done;
		}
		lp->b_off = 0;
	}
	else if(dblp->lfhp == NULL || dblp->lfname != lp->lsn.file)
		if((ret = __log_newfh(dblp, 0)) != 0) {
			MUTEX_UNLOCK(env, lp->mtx_flush);
			goto done;
		}
	// 
	// We are going to flush, release the region.
	// First get the current state of the buffer since
	// another write may come in, but we may not flush it.
	// 
	b_off = lp->b_off;
	w_off = lp->w_off;
	f_lsn = lp->f_lsn;
	lp->in_flush++;
	if(release)
		LOG_SYSTEM_UNLOCK(env);
	//
	// Sync all writes to disk
	//
	if(!lp->nosync) {
		if((ret = __os_fsync(env, dblp->lfhp)) != 0) {
			MUTEX_UNLOCK(env, lp->mtx_flush);
			if(release)
				LOG_SYSTEM_LOCK(env);
			lp->in_flush--;
			goto done;
		}
		STAT(++lp->stat.st_scount); 
	}
	//
	// Set the last-synced LSN.
	// This value must be set to the LSN past the last complete record that has been flushed.  
	// This is at least the first lsn, f_lsn.  If the buffer is empty, b_off == 0, then
	// we can move up to write point since the first lsn is not set for the new buffer.
	// 
	lp->s_lsn = f_lsn;
	if(b_off == 0)
		lp->s_lsn.Offset_ = w_off;
	MUTEX_UNLOCK(env, lp->mtx_flush);
	if(release)
		LOG_SYSTEM_LOCK(env);
	lp->in_flush--;
	// 
	// How many flush calls (usually commits) did this call actually sync?
	// At least one, if it got here.
	// 
	ncommit = 1;
done:
	if(lp->ncommit != 0) {
		first = 1;
		SH_TAILQ_FOREACH(commit, &lp->commits, links, __db_commit)
		if(LOG_COMPARE(&lp->s_lsn, &commit->lsn) > 0) {
			MUTEX_UNLOCK(env, commit->mtx_txnwait);
			SH_TAILQ_REMOVE(&lp->commits, commit, links, __db_commit);
			ncommit++;
		}
		else if(first == 1) {
			F_SET(commit, DB_COMMIT_FLUSH);
			MUTEX_UNLOCK(env, commit->mtx_txnwait);
			SH_TAILQ_REMOVE(&lp->commits, commit, links, __db_commit);
			/*
			 * This thread will wake and flush.
			 * If another thread commits and flushes
			 * first we will waste a trip trough the
			 * mutex.
			 */
			lp->in_flush++;
			first = 0;
		}
	}
#ifdef HAVE_STATISTICS
	if(lp->stat.st_maxcommitperflush < ncommit)
		lp->stat.st_maxcommitperflush = ncommit;
	if(lp->stat.st_mincommitperflush > ncommit || lp->stat.st_mincommitperflush == 0)
		lp->stat.st_mincommitperflush = ncommit;
#endif
	return ret;
}
/*
 * __log_fill --
 *	Write information into the log.
 */
static int __log_fill(DB_LOG * dblp, DB_LSN * lsn, void * addr, uint32 len)
{
	uint32 nrec;
	size_t nw, remain;
	int ret;
	LOG * lp = static_cast<LOG *>(dblp->reginfo.primary);
	uint32 bsize = lp->buffer_size;
	if(lp->db_log_inmemory) {
		__log_inmem_copyin(dblp, lp->b_off, addr, len);
		lp->b_off = (lp->b_off+len)%lp->buffer_size;
		return 0;
	}
	while(len > 0) {                        /* Copy out the data. */
		/*
		 * If we're beginning a new buffer, note the user LSN to which
		 * the first byte of the buffer belongs.  We have to know this
		 * when flushing the buffer so that we know if the in-memory
		 * buffer needs to be flushed.
		 */
		if(lp->b_off == 0)
			lp->f_lsn = *lsn;
		/*
		 * If we're on a buffer boundary and the data is big enough,
		 * copy as many records as we can directly from the data.
		 */
		if(lp->b_off == 0 && len >= bsize) {
			nrec = len/bsize;
			if((ret = __log_write(dblp, addr, nrec*bsize)) != 0)
				return ret;
			addr = (uint8 *)addr+nrec*bsize;
			len -= nrec*bsize;
			STAT(++lp->stat.st_wcount_fill);
			continue;
		}
		/* Figure out how many bytes we can copy this time. */
		remain = bsize-lp->b_off;
		nw = remain > len ? len : remain;
		memcpy(dblp->bufp+lp->b_off, addr, nw);
		addr = (uint8 *)addr+nw;
		len -= (uint32)nw;
		lp->b_off += (uint32)nw;
		/* If we fill the buffer, flush it. */
		if(lp->b_off == bsize) {
			if((ret = __log_write(dblp, dblp->bufp, bsize)) != 0)
				return ret;
			lp->b_off = 0;
			STAT(++lp->stat.st_wcount_fill);
		}
	}
	return 0;
}
/*
 * __log_write --
 *	Write the log buffer to disk.
 */
static int __log_write(DB_LOG * dblp, void * addr, uint32 len)
{
	size_t nw;
	int ret;
	ENV * env = dblp->env;
	LOG * lp = static_cast<LOG *>(dblp->reginfo.primary);
	DB_ASSERT(env, !lp->db_log_inmemory);
	/*
	 * If we haven't opened the log file yet or the current one has
	 * changed, acquire a new log file.  We are creating the file if we're
	 * about to write to the start of it, in other words, if the write
	 * offset is zero.
	 */
	if(dblp->lfhp == NULL || dblp->lfname != lp->lsn.file || dblp->lf_timestamp != lp->timestamp)
		if((ret = __log_newfh(dblp, lp->w_off == 0)) != 0)
			return ret;
	/*
	 * If we're writing the first block in a log file on a filesystem that
	 * guarantees unwritten blocks are zero-filled, we set the size of the
	 * file in advance.  This increases sync performance on some systems,
	 * because they don't need to update metadata on every sync.
	 *
	 * Ignore any error -- we may have run out of disk space, but that's no
	 * reason to quit.
	 */
#ifdef HAVE_FILESYSTEM_NOTZERO
	if(lp->w_off == 0 && !__os_fs_notzero()) {
#else
	if(lp->w_off == 0) {
#endif
		__db_file_extend(env, dblp->lfhp, lp->log_size);
		if(F_ISSET(dblp, DBLOG_ZERO))
			__db_zero_extend(env, dblp->lfhp, 0, lp->log_size/lp->buffer_size, lp->buffer_size);
	}
	/*
	 * Seek to the offset in the file (someone may have written it
	 * since we last did).
	 */
	if((ret = __os_io(env, DB_IO_WRITE, dblp->lfhp, 0, 0, lp->w_off, len, (uint8 *)addr, &nw)) != 0)
		return ret;
	/* Reset the buffer offset and update the seek offset. */
	lp->w_off += len;
	/* Update written statistics. */
	if((lp->stat.st_wc_bytes += len) >= MEGABYTE) {
		lp->stat.st_wc_bytes -= MEGABYTE;
		++lp->stat.st_wc_mbytes;
	}
#ifdef HAVE_STATISTICS
	if((lp->stat.st_w_bytes += len) >= MEGABYTE) {
		lp->stat.st_w_bytes -= MEGABYTE;
		++lp->stat.st_w_mbytes;
	}
	++lp->stat.st_wcount;
#endif
	return 0;
}
/*
 * __log_file_pp --
 *	ENV->log_file pre/post processing.
 *
 * PUBLIC: int __log_file_pp __P((DB_ENV *, const DB_LSN *, char *, size_t));
 */
int __log_file_pp(DB_ENV * dbenv, const DB_LSN * lsn, char * namep, size_t len)
{
	DB_THREAD_INFO * ip;
	int ret, set;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->lg_handle, "DB_ENV->log_file", DB_INIT_LOG);
	if((ret = __log_get_config(dbenv, DB_LOG_IN_MEMORY, &set)) != 0)
		return ret;
	if(set) {
		__db_errx(env, DB_STR("2518", "DB_ENV->log_file is illegal with in-memory logs"));
		return EINVAL;
	}
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__log_file(env, lsn, namep, len)), 0, ret);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __log_file --
 *	ENV->log_file.
 */
static int __log_file(ENV * env, const DB_LSN * lsn, char * namep, size_t len)
{
	int ret;
	char * name;
	DB_LOG * dblp = env->lg_handle;
	LOG_SYSTEM_LOCK(env);
	ret = __log_name(dblp, lsn->file, &name, NULL, 0);
	LOG_SYSTEM_UNLOCK(env);
	if(ret != 0)
		return ret;
	/* Check to make sure there's enough room and copy the name. */
	if(len < sstrlen(name)+1) {
		*namep = '\0';
		__db_errx(env, DB_STR("2519", "DB_ENV->log_file: name buffer is too short"));
		return EINVAL;
	}
	strcpy(namep, name);
	__os_free(env, name);
	return 0;
}
/*
 * __log_newfh --
 *	Acquire a file handle for the current log file.
 */
static int __log_newfh(DB_LOG * dblp, int create)
{
	uint32 flags;
	int ret;
	logfile_validity status;
	ENV * env = dblp->env;
	LOG * lp = static_cast<LOG *>(dblp->reginfo.primary);
	// Close any previous file descriptor
	__os_closehandle(env, dblp->lfhp);
	dblp->lfhp = NULL;
	flags = DB_OSO_SEQ|(create ? DB_OSO_CREATE : 0)|(F_ISSET(dblp, DBLOG_DIRECT) ? DB_OSO_DIRECT : 0)|(F_ISSET(dblp, DBLOG_DSYNC) ? DB_OSO_DSYNC : 0);
	// Get the path of the new file and open it
	dblp->lfname = lp->lsn.file;
	if((ret = __log_valid(dblp, dblp->lfname, 0, &dblp->lfhp, flags, &status, NULL)) != 0)
		__db_err(env, ret, "DB_ENV->log_newfh: %lu", (ulong)lp->lsn.file);
	else if(status != DB_LV_NORMAL && status != DB_LV_INCOMPLETE && status != DB_LV_OLD_READABLE)
		ret = DB_NOTFOUND;
	return ret;
}
/*
 * __log_name --
 *	Return the log name for a particular file, and optionally open it.
 */
int __log_name(DB_LOG * dblp, uint32 filenumber, char ** namep, DB_FH ** fhpp, uint32 flags)
{
	int mode, ret;
	char * oname;
	char old_name[sizeof(LFPREFIX)+5+20], new_name[sizeof(LFPREFIX)+10+20];
	ENV * env = dblp->env;
	LOG * lp = static_cast<LOG *>(dblp->reginfo.primary);
	DB_ASSERT(env, !lp->db_log_inmemory);
	/*
	 * !!!
	 * The semantics of this routine are bizarre.
	 *
	 * The reason for all of this is that we need a place where we can
	 * intercept requests for log files, and, if appropriate, check for
	 * both the old_name-style and new_name-style log file names.  The trick is
	 * that all callers of this routine that are opening the log file
	 * read-only want to use an old_name-style file name if they can't find
	 * a match using a new_name-style name.  The only down-side is that some
	 * callers may check for the old_name-style when they really don't need
	 * to, but that shouldn't mess up anything, and we only check for
	 * the old_name-style name when we've already failed to find a new_name-style
	 * one.
	 *
	 * Create a new_name-style file name, and if we're not going to open the
	 * file, return regardless.
	 */
	snprintf(new_name, sizeof(new_name), LFNAME, filenumber);
	if((ret = __db_appname(env, DB_APP_LOG, new_name, NULL, namep)) != 0 || fhpp == NULL)
		return ret;
	/* The application may have specified an absolute file mode. */
	if(lp->filemode == 0)
		mode = env->db_mode;
	else {
		LF_SET(DB_OSO_ABSMODE);
		mode = lp->filemode;
	}
	/* Open the new_name-style file -- if we succeed, we're done. */
	dblp->lf_timestamp = lp->timestamp;
	if((ret = __os_open(env, *namep, 0, flags, mode, fhpp)) == 0)
		return 0;
	/*
	 * If the open failed for reason other than the file
	 * not being there, complain loudly, the wrong user
	 * probably started up the application.
	 */
	if(ret != ENOENT) {
		__db_err(env, ret, DB_STR_A("2520", "%s: log file unreadable", "%s"), *namep);
		return __env_panic(env, ret);
	}
	/*
	 * The open failed... if the DB_RDONLY flag isn't set, we're done,
	 * the caller isn't interested in old_name-style files.
	 */
	if(!LF_ISSET(DB_OSO_RDONLY)) {
		__db_err(env, ret, DB_STR_A("2521", "%s: log file open failed", "%s"), *namep);
		return __env_panic(env, ret);
	}
	/* Create an old_name-style file name. */
	snprintf(old_name, sizeof(old_name), LFNAME_V1, filenumber);
	if((ret = __db_appname(env, DB_APP_LOG, old_name, NULL, &oname)) != 0)
		goto err;
	/*
	 * Open the old_name-style file -- if we succeed, we're done.  Free the
	 * space allocated for the new_name-style name and return the old_name-style
	 * name to the caller.
	 */
	if((ret = __os_open(env, oname, 0, flags, mode, fhpp)) == 0) {
		__os_free(env, *namep);
		*namep = oname;
		return 0;
	}
	/*
	 * Couldn't find either style of name -- return the new_name-style name
	 * for the caller's error message.  If it's an old_name-style name that's
	 * actually missing we're going to confuse the user with the error
	 * message, but that implies that not only were we looking for an
	 * old_name-style name, but we expected it to exist and we weren't just
	 * looking for any log file.  That's not a likely error.
	 */
err:
	__os_free(env, oname);
	return ret;
}
/*
 * __log_rep_put --
 *	Short-circuit way for replication clients to put records into the
 * log.  Replication clients' logs need to be laid out exactly as their masters'
 * are, so we let replication take responsibility for when the log gets
 * flushed, when log switches files, etc.  This is just a thin PUBLIC wrapper
 * for __log_putr with a slightly prettier interface.
 *
 * Note that the REP->mtx_clientdb should be held when this is called.
 * Note that we acquire the log region mutex while holding mtx_clientdb.
 */
int __log_rep_put(ENV * env, DB_LSN * lsnp, const DBT * rec, uint32 flags)
{
	DBT * dbt, t;
	DB_CIPHER * db_cipher;
	HDR hdr;
	int need_free, ret;
	DB_LOG * dblp = env->lg_handle;
	LOG * lp = static_cast<LOG *>(dblp->reginfo.primary);
	LOG_SYSTEM_LOCK(env);
	memzero(&hdr, sizeof(HDR));
	t = *rec;
	dbt = &t;
	need_free = 0;
	db_cipher = env->crypto_handle;
	if(CRYPTO_ON(env))
		t.size += db_cipher->adj_size(rec->size);
	if((ret = __os_calloc(env, 1, t.size, &t.data)) != 0)
		goto err;
	need_free = 1;
	memcpy(t.data, rec->data, rec->size);
	if((ret = __log_encrypt_record(env, dbt, &hdr, rec->size)) != 0)
		goto err;
#ifdef HAVE_LOG_CHECKSUM
	__db_chksum(&hdr, (uint8 *)t.data, t.size, (CRYPTO_ON(env)) ? db_cipher->mac_key : NULL, hdr.chksum);
#endif
	DB_ASSERT(env, LOG_COMPARE(lsnp, &lp->lsn) == 0);
	ret = __log_putr(dblp, lsnp, dbt, lp->lsn.Offset_-lp->len, &hdr);
err:
	/*
	 * !!! Assume caller holds REP->mtx_clientdb to modify ready_lsn.
	 */
	lp->ready_lsn = lp->lsn;
	if(LF_ISSET(DB_LOG_CHKPNT))
		lp->stat.st_wc_bytes = lp->stat.st_wc_mbytes = 0;
	/* Increment count of records added to the log. */
	STAT(++lp->stat.st_record);
	LOG_SYSTEM_UNLOCK(env);
	if(need_free)
		__os_free(env, t.data);
	return ret;
}

static int __log_encrypt_record(ENV * env, DBT * dbt, HDR * hdr, uint32 orig)
{
	DB_CIPHER * db_cipher;
	int ret;
	if(CRYPTO_ON(env)) {
		db_cipher = env->crypto_handle;
		hdr->size = HDR_CRYPTO_SZ;
		hdr->orig_size = orig;
		if((ret = db_cipher->encrypt(env, db_cipher->data, hdr->iv, (uint8 *)dbt->data, dbt->size)) != 0)
			return ret;
	}
	else {
		hdr->size = HDR_NORMAL_SZ;
	}
	return 0;
}
/*
 * __log_put_record_pp --
 *	DB_ENV->log_put_record pre/post processing.
 */
int __log_put_record_pp(DB_ENV * dbenv, DB * dbp, DB_TXN * txnp, DB_LSN * ret_lsnp, uint32 flags, uint32 rectype,
	uint32 has_data, uint32 size, DB_LOG_RECSPEC * spec, ...)
{
	DB_THREAD_INFO * ip;
	va_list argp;
	int ret;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->lg_handle, "DB_ENV->log_put_record", DB_INIT_LOG);
	// Validate arguments: check for allowed flags
	if((ret = __db_fchk(env, "DB_ENV->log_put_record", flags, DB_LOG_CHKPNT|DB_LOG_COMMIT|DB_FLUSH|DB_LOG_NOCOPY|DB_LOG_WRNOSYNC)) != 0)
		return ret;
	/* DB_LOG_WRNOSYNC and DB_FLUSH are mutually exclusive. */
	if(LF_ISSET(DB_LOG_WRNOSYNC) && LF_ISSET(DB_FLUSH))
		return __db_ferr(env, "DB_ENV->log_put_record", 1);
	/* Replication clients should never write log records. */
	if(IS_REP_CLIENT(env)) {
		__db_errx(env, DB_STR("2522", "DB_ENV->log_put is illegal on replication clients"));
		return EINVAL;
	}
	ENV_ENTER(env, ip);
	va_start(argp, spec);
	REPLICATION_WRAP(env, (__log_put_record_int(env, dbp, txnp, ret_lsnp, flags, rectype, has_data, size, spec, argp)), 0, ret);
	va_end(argp);
	ENV_LEAVE(env, ip);
	return ret;
}

int __log_put_record(ENV * env, DB * dbp, DB_TXN * txnp, DB_LSN * ret_lsnp, uint32 flags, uint32 rectype,
	uint32 has_data, uint32 size, DB_LOG_RECSPEC * spec, ...)
{
	va_list argp;
	int ret;
	va_start(argp, spec);
	ret = __log_put_record_int(env, dbp, txnp, ret_lsnp, flags, rectype, has_data, size, spec, argp);
	va_end(argp);
	return ret;
}

static int __log_put_record_int(ENV * env, DB * dbp, DB_TXN * txnp, DB_LSN * ret_lsnp, uint32 flags,
	uint32 rectype, uint32 has_data, uint32 size, DB_LOG_RECSPEC * spec, va_list argp)
{
	DBT * data, * dbt, * header, logrec;
	DB_LOG_RECSPEC * sp;
	DB_LSN * lsnp, lsn, null_lsn, * pagelsn, * rlsnp;
	DB_TXNLOGREC * lr;
	LOG * lp;
	PAGE * pghdrstart;
	uint32 hdrsize, op, zero, uinttmp, txn_num;
	uint npad;
	uint8 * bp;
	int is_durable, ret;
	void * hdrstart;
	COMPQUIET(lr, 0);
	COMPQUIET(hdrsize, 0);
	COMPQUIET(op, 0);
	COMPQUIET(hdrstart, 0);
	COMPQUIET(pghdrstart, 0);
	COMPQUIET(header, 0);
	/*
	 * rlsnp will be stored into while holding the log system lock.
	 * If this is a commit record then ret_lsnp will be the address of
	 * the transaction detail visible_lsn field.  If not then this
	 * may be the lsn of a page and we do not want to set it if
	 * the log_put fails after writing the record (due to an I/O error).
	 */
	rlsnp = LF_ISSET(DB_LOG_COMMIT) ? ret_lsnp : &lsn;
	npad = 0;
	ret = 0;
	data = NULL;
	if(LF_ISSET(DB_LOG_NOT_DURABLE) || (dbp != NULL && F_ISSET(dbp, DB_AM_NOT_DURABLE))) {
		if(!txnp)
			return 0;
		is_durable = 0;
	}
	else
		is_durable = 1;
	if(!txnp) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.Offset_ = 0;
	}
	else {
		if(TAILQ_FIRST(&txnp->kids) != NULL && (ret = __txn_activekids(env, rectype, txnp)) != 0)
			return ret;
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}
	if(dbp) {
		DB_ASSERT(env, dbp->log_filename != NULL);
		if(dbp->log_filename->id == DB_LOGFILEID_INVALID && (ret = __dbreg_lazy_id(dbp)) != 0)
			return ret;
	}
	logrec.size = size;
	if(CRYPTO_ON(env)) {
		npad = env->crypto_handle->adj_size(logrec.size);
		logrec.size += npad;
	}
	if(is_durable || !txnp) {
		if((ret = __os_malloc(env, logrec.size, &logrec.data)) != 0)
			return ret;
	}
	else {
		if((ret = __os_malloc(env, logrec.size+sizeof(DB_TXNLOGREC), &lr)) != 0)
			return ret;
#ifdef DIAGNOSTIC
		if((ret = __os_malloc(env, logrec.size, &logrec.data)) != 0) {
			__os_free(env, lr);
			return ret;
		}
#else
		logrec.data = lr->data;
#endif
	}
	if(npad > 0)
		memzero((uint8 *)logrec.data+logrec.size-npad, npad);
	bp = (uint8 *)logrec.data;
	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);
	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);
	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);
	zero = 0;
	lp = (LOG *)env->lg_handle->reginfo.primary;
	for(sp = spec; sp->type != LOGREC_Done; sp++) {
		switch(sp->type) {
		    case LOGREC_DB:
			/* This is not in the varargs. */
			uinttmp = (uint32)dbp->log_filename->id;
			LOGCOPY_32(env, bp, &uinttmp);
			bp += sizeof(uinttmp);
			break;

		    case LOGREC_ARG:
		    case LOGREC_TIME:
		    case LOGREC_DBOP:
			uinttmp = va_arg(argp, uint32);
			LOGCOPY_32(env, bp, &uinttmp);
			bp += sizeof(uinttmp);
			break;
		    case LOGREC_OP:
			op = va_arg(argp, uint32);
			LOGCOPY_32(env, bp, &op);
			bp += sizeof(uinttmp);
			break;
		    case LOGREC_DBT:
		    case LOGREC_PGLIST:
		    case LOGREC_LOCKS:
		    case LOGREC_HDR:
		    case LOGREC_DATA:
			dbt = va_arg(argp, DBT *);
			if(dbt == NULL) {
				LOGCOPY_32(env, bp, &zero);
				bp += sizeof(uint32);
			}
			else {
				LOGCOPY_32(env, bp, &dbt->size);
				bp += sizeof(dbt->size);
				memcpy(bp, dbt->data, dbt->size);
			}
			/* Process fields that need to be byte swapped. */
			if(dbp != NULL && F_ISSET(dbp, DB_AM_SWAP)) {
				if(sp->type == LOGREC_HDR && dbt != NULL && has_data == 0)
					__db_recordswap(op, dbt->size, bp, NULL, 0);
				else if(sp->type == LOGREC_HDR) {
					hdrstart = bp;
					hdrsize = dbt ? dbt->size : 0;
				}
				else if(sp->type == LOGREC_DATA) {
					__db_recordswap(op, hdrsize, hdrstart, bp, 0);
					has_data = 0;
				}
			}
			if(dbt != NULL)
				bp += dbt->size;
			break;
		    /*
		 * Page header and data -- we assume that the header
		 * is listed first and the data follows sometime later.
		 * There should be only one header/data pair per record.
		     */
		    case LOGREC_PGDBT:
			header = va_arg(argp, DBT *);
			if(header == NULL) {
				LOGCOPY_32(env, bp, &zero);
				bp += sizeof(uint32);
			}
			else {
				LOGCOPY_32(env, bp, &header->size);
				bp += sizeof(header->size);
				pghdrstart = (PAGE *)bp;
				memcpy(bp, header->data, header->size);
				if(has_data == 0 && F_ISSET(dbp, DB_AM_SWAP) && (ret = __db_pageswap(env, dbp, pghdrstart, (size_t)header->size, NULL, 0)) != 0)
					return ret;
				bp += header->size;
			}
			break;

		    case LOGREC_PGDDBT:
			data = va_arg(argp, DBT *);
			if(data == NULL) {
				zero = 0;
				LOGCOPY_32(env, bp, &zero);
				bp += sizeof(uint32);
			}
			else {
				if(F_ISSET(dbp, DB_AM_SWAP) && (ret = __db_pageswap(env, dbp, pghdrstart, (size_t)header->size, (DBT *)data, 0)) != 0)
					return ret;
				LOGCOPY_32(env, bp, &data->size);
				bp += sizeof(data->size);
				memcpy(bp, data->data, data->size);
				if(F_ISSET(dbp, DB_AM_SWAP) && F_ISSET(data, DB_DBT_APPMALLOC))
					__os_free(env, data->data);
				bp += data->size;
			}
			break;
		    case LOGREC_POINTER:
			pagelsn = va_arg(argp, DB_LSN *);
			if(pagelsn) {
				if(txnp) {
					if(LOG_COMPARE(pagelsn, &lp->lsn) >= 0 && (ret = __log_check_page_lsn(env, dbp, pagelsn)) != 0)
						return ret;
				}
				LOGCOPY_FROMLSN(env, bp, pagelsn);
			}
			else
				memzero(bp, sizeof(*pagelsn));
			bp += sizeof(*pagelsn);
			break;

		    default:
			DB_ASSERT(env, sp->type != sp->type);
		}
	}
	DB_ASSERT(env, (uint32)(bp-(uint8 *)logrec.data) <= logrec.size);
	if(is_durable || !txnp) {
		if((ret = __log_put(env, rlsnp, (DBT *)&logrec, flags|DB_LOG_NOCOPY)) == 0) {
			if(txnp)
				*lsnp = *rlsnp;
			*ret_lsnp = *rlsnp;
		}
	}
	else {
		ret = 0;
#ifdef DIAGNOSTIC
		/*
		 * Set the debug bit if we are going to log non-durable
		 * transactions so they will be ignored by recovery.
		 */
		memcpy(lr->data, logrec.data, logrec.size);
		rectype |= DB_debug_FLAG;
		LOGCOPY_32(env, logrec.data, &rectype);
		if(!IS_REP_CLIENT(env) && !lp->db_log_inmemory)
			ret = __log_put(env, rlsnp, (DBT *)&logrec, flags|DB_LOG_NOCOPY);
#endif
		STAILQ_INSERT_HEAD(&txnp->logs, lr, links);
		F_SET((TXN_DETAIL *)txnp->td, TXN_DTL_INMEMORY);
		LSN_NOT_LOGGED(*ret_lsnp);
	}
#ifdef LOG_DIAGNOSTIC
	if(ret != 0)
		__db_addrem_print(env, (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT, 0);
#endif

#ifdef DIAGNOSTIC
	__os_free(env, logrec.data);
#else
	if(is_durable || !txnp)
		__os_free(env, logrec.data);
#endif
	return ret;
}
