/*-
 * See the file LICENSE for redistribution information.
 * Copyright (c) 2005, 2011 Oracle and/or its affiliates.  All rights reserved.
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop
/*
 * The functions in this module implement a simple wire protocol for
 * transmitting messages of various types.  Every message consists of a 9-byte
 * header followed by a body (though the body could be 0-length).  The header is
 * the marshaled form of the "msg_hdr" structure defined in repmgr.src.  The
 * interpretation of header fields depends on message type, and is defined in
 * repmgr.h.  But as a general principle, in all cases there is enough
 * information in the header for us to know the total size of the body, and the
 * total amount of memory we need to allocate for storing and processing the
 * message.
 */

/*
 * In sending a message, we first try to send it in-line, in the sending thread,
 * and without first copying the message, by using scatter/gather I/O, using
 * iovecs to point to the various pieces of the message.  If that all works
 * without blocking, that's optimal.
 *   If we find that, for a particular connection, we can't send without
 * blocking, then we must copy the message for sending later in the select()
 * thread.  In the course of doing that, we might as well "flatten" the message,
 * forming one single buffer, to simplify life.  Not only that, once we've gone
 * to the trouble of doing that, other sites to which we also want to send the
 * message (in the case of a broadcast), may as well take advantage of the
 * simplified structure also.
 *   The sending_msg structure below holds it all.  Note that this structure,
 * and the "flat_msg" structure, are allocated separately, because (1) the
 * flat_msg version is usually not needed; and (2) when a flat_msg is needed, it
 * will need to live longer than the wrapping sending_msg structure.
 *   Note that, for the broadcast case, where we're going to use this
 * repeatedly, the iovecs is a template that must be copied, since in normal use
 * the iovecs pointers and lengths get adjusted after every partial write.
 */
struct sending_msg {
	REPMGR_IOVECS * iovecs;
	REPMGR_FLAT * fmsg;
};

/* Context for a thread waiting for client acks for PERM message. */
struct repmgr_permanence {
	DB_LSN lsn; /* LSN whose ack this thread is waiting for. */
	uint threshold; /* Number of client acks needed. */
	int policy; /* Ack policy to be used for this txn. */
};

#ifdef  CONFIG_TEST
static uint fake_port __P((ENV*, uint));
#endif
static int final_cleanup __P((ENV*, REPMGR_CONNECTION*, void *));
static int flatten __P((ENV*, struct sending_msg *));
static int is_permanent __P((ENV*, void *));
static int __repmgr_finish_connect __P((ENV*, socket_t s, REPMGR_CONNECTION**));
static int __repmgr_propose_version __P((ENV*, REPMGR_CONNECTION *));
static int __repmgr_start_connect __P((ENV*, socket_t*, ADDRINFO*, int *));
static void setup_sending_msg __P((ENV*, struct sending_msg *, uint8 *, uint, const DBT*, const DBT *));
static int __repmgr_send_internal __P((ENV*, REPMGR_CONNECTION*, struct sending_msg *, db_timeout_t));
static int enqueue_msg __P((ENV*, REPMGR_CONNECTION*, struct sending_msg *, size_t));
static REPMGR_SITE * __repmgr_available_site __P((ENV*, int));
static REPMGR_SITE * __repmgr_find_available_peer(ENV *);

/*
 * Connects to the given network address, using blocking operations.  Any thread
 * synchronization is the responsibility of the caller.
 *
 * PUBLIC: int __repmgr_connect __P((ENV *,
 * PUBLIC:     repmgr_netaddr_t *, REPMGR_CONNECTION **, int *));
 */
int __repmgr_connect(ENV*env, repmgr_netaddr_t * netaddr, REPMGR_CONNECTION ** connp, int * errp)
{
	REPMGR_CONNECTION * conn;
	ADDRINFO * ai0, * ai;
	socket_t sock;
	int err, ret;
	uint port;

	COMPQUIET(err, 0);
#ifdef  CONFIG_TEST
	port = fake_port(env, netaddr->port);
#else
	port = netaddr->port;
#endif
	if((ret = __repmgr_getaddr(env, netaddr->host, port, 0, &ai0)) != 0)
		return ret;
	/*
	 * Try each address on the list, until success.  Note that if several
	 * addresses on the list produce retryable error, we can only pass back
	 * to our caller the last one.
	 */
	for(ai = ai0; ai != NULL; ai = ai->ai_next) {
		switch((ret = __repmgr_start_connect(env, &sock, ai, &err))) {
		    case 0:
			if((ret = __repmgr_finish_connect(env, sock, &conn)) == 0)
				*connp = conn;
			else
				closesocket(sock);
			goto out;
		    case DB_REP_UNAVAIL:
			continue;
		    default:
			goto out;
		}
	}
out:
	__os_freeaddrinfo(env, ai0);
	if(ret == DB_REP_UNAVAIL) {
		__repmgr_print_conn_err(env, netaddr, err);
		*errp = err;
	}
	return ret;
}

static int __repmgr_start_connect(ENV * env, socket_t * socket_result, ADDRINFO * ai, int * err)
{
	socket_t s;
	int ret;
	if((s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == SOCKET_ERROR) {
		ret = net_errno;
		__db_err(env, ret, "create socket");
		return ret;
	}
	if(connect(s, ai->ai_addr, (socklen_t)ai->ai_addrlen) != 0) {
		*err = net_errno;
		closesocket(s);
		return DB_REP_UNAVAIL;
	}
	RPRINT(env, (env, DB_VERB_REPMGR_MISC, "connection established"));
	*socket_result = s;
	return 0;
}

static int __repmgr_finish_connect(ENV*env, socket_t s, REPMGR_CONNECTION ** connp)
{
	REPMGR_CONNECTION * conn;
	int ret;
	if((ret = __repmgr_new_connection(env, &conn, s, CONN_CONNECTED)) != 0)
		return ret;
	if((ret = __repmgr_propose_version(env, conn)) == 0)
		*connp = conn;
	else
		__repmgr_destroy_conn(env, conn);
	return ret;
}

static int __repmgr_propose_version(ENV * env, REPMGR_CONNECTION * conn)
{
	DB_REP * db_rep;
	__repmgr_version_proposal_args versions;
	repmgr_netaddr_t * my_addr;
	size_t hostname_len, rec_length;
	uint8 * buf, * p;
	int ret;
	db_rep = env->rep_handle;
	my_addr = &SITE_FROM_EID(db_rep->self_eid)->net_addr;
	/*
	 * In repmgr wire protocol version 1, a handshake message had a rec part
	 * that looked like this:
	 *
	 *  +-----------------+----+
	 *  |  host name ...  | \0 |
	 *  +-----------------+----+
	 *
	 * To ensure its own sanity, the old repmgr would write a NUL into the
	 * last byte of a received message, and then use normal C library string
	 * operations (e.g., sstrlen, strcpy).
	 *
	 * Now, a version proposal has a rec part that looks like this:
	 *
	 *  +-----------------+----+------------------+------+
	 *  |  host name ...  | \0 |  extra info ...  |  \0  |
	 *  +-----------------+----+------------------+------+
	 *
	 * The "extra info" contains the version parameters, in marshaled form.
	 */
	hostname_len = sstrlen(my_addr->host);
	rec_length = hostname_len+1+__REPMGR_VERSION_PROPOSAL_SIZE+1;
	if((ret = __os_malloc(env, rec_length, &buf)) != 0)
		goto out;
	p = buf;
	strcpy((char *)p, my_addr->host);
	p += hostname_len+1;
	versions.min = DB_REPMGR_MIN_VERSION;
	versions.max = DB_REPMGR_VERSION;
	__repmgr_version_proposal_marshal(env, &versions, p);
	ret = __repmgr_send_v1_handshake(env, conn, buf, rec_length);
	__os_free(env, buf);
out:
	return ret;
}
/*
 * __repmgr_send --
 *	The send function for DB_ENV->rep_set_transport.
 *
 * PUBLIC: int __repmgr_send __P((DB_ENV *, const DBT *, const DBT *,
 * PUBLIC:     const DB_LSN *, int, uint32));
 */
int __repmgr_send(DB_ENV * dbenv, const DBT * control, const DBT * rec, const DB_LSN * lsnp, int eid, uint32 flags)
{
	DB_REP * db_rep;
	REP * rep;
	ENV * env;
	REPMGR_CONNECTION * conn;
	REPMGR_SITE * site;
	struct repmgr_permanence perm;
	db_timeout_t maxblock;
	uint32 available, nclients, needed, quorum;
	uint npeers_sent;
	uint nsites_sent;
	int policy, ret, t_ret;
	env = dbenv->env;
	db_rep = env->rep_handle;
	rep = db_rep->region;
	ret = 0;
	COMPQUIET(quorum, 0);
	LOCK_MUTEX(db_rep->mutex);
	/*
	 * If we're already "finished", we can't send anything.  This covers the
	 * case where a bulk buffer is flushed at env close, or perhaps an
	 * unexpected __repmgr_thread_failure.
	 */
	if(db_rep->finished) {
		ret = DB_REP_UNAVAIL;
		goto out;
	}
	/*
	 * Check whether we need to refresh our site address information with
	 * more recent updates from shared memory.
	 */
	if(rep->siteinfo_seq > db_rep->siteinfo_seq && (ret = __repmgr_sync_siteaddr(env)) != 0)
		goto out;
	if(eid == DB_EID_BROADCAST) {
		if((ret = __repmgr_send_broadcast(env, REPMGR_REP_MESSAGE, control, rec, &nsites_sent, &npeers_sent)) != 0)
			goto out;
	}
	else {
		DB_ASSERT(env, IS_KNOWN_REMOTE_SITE(eid));
		/*
		 * Since repmgr's simple c2c implementation doesn't truly manage
		 * staged synchronization it doesn't work well with master
		 * leases.  So, disable it during the time when a new master may
		 * be trying to establish its first set of lease grants.
		 */
		if(IS_USING_LEASES(env) && !rep->stat.st_startup_complete)
			LF_CLR(DB_REP_ANYWHERE);
		/*
		 * If this is a request that can be sent anywhere, then see if
		 * we can send it to our peer (to save load on the master), but
		 * not if it's a rerequest, 'cuz that likely means we tried this
		 * already and failed.
		 */
		if((flags&(DB_REP_ANYWHERE|DB_REP_REREQUEST)) == DB_REP_ANYWHERE && (site = __repmgr_find_available_peer(env)) != NULL) {
			VPRINT(env, (env, DB_VERB_REPMGR_MISC, "sending request to peer"));
		}
		else if((site = __repmgr_available_site(env, eid)) == NULL) {
			RPRINT(env, (env, DB_VERB_REPMGR_MISC, "ignoring message sent to unavailable site"));
			ret = DB_REP_UNAVAIL;
			goto out;
		}
		conn = site->ref.conn;
		/*
		 * In case the connection is clogged up and we have to wait for
		 * space on the output queue, how long shall we wait?  We could
		 * of course create a new timeout configuration type, so that
		 * the application could set it directly.  But that would start
		 * to overwhelm the user with too many choices to think about.
		 * We already have an ACK timeout, which is the user's estimate
		 * of how long it should take to send a message to the client,
		 * have it be processed, and return a message back to us.  We
		 * multiply that by the queue size, because that's how many
		 * messages have to be swallowed up by the client before we're
		 * able to start sending again (at least to a rough
		 * approximation).
		 */
		maxblock = OUT_QUEUE_LIMIT* (rep->ack_timeout == 0 ? DB_REPMGR_DEFAULT_ACK_TIMEOUT : rep->ack_timeout);
		if((ret = __repmgr_send_one(env, conn, REPMGR_REP_MESSAGE, control, rec, maxblock)) == DB_REP_UNAVAIL &&
		   (t_ret = __repmgr_bust_connection(env, conn)) != 0)
			ret = t_ret;
		if(ret)
			goto out;
		nsites_sent = 1;
		npeers_sent = F_ISSET(site, SITE_ELECTABLE) ? 1 : 0;
	}
	/*
	 * Right now, nsites and npeers represent the (maximum) number of sites
	 * we've attempted to begin sending the message to.  Of course we
	 * haven't really received any ack's yet.  But since we've only sent to
	 * nsites/npeers other sites, that's the maximum number of ack's we
	 * could possibly expect.  If even that number fails to satisfy our PERM
	 * policy, there's no point waiting for something that will never
	 * happen.
	 */
	if(LF_ISSET(DB_REP_PERMANENT)) {
		/* Adjust so as not to count the local site. */
		nclients = db_rep->region->config_nsites-1;
		/*
		 * When doing membership DB changes, avoid some impossible
		 * situations.
		 */
		policy = rep->perm_policy;
		switch(db_rep->active_gmdb_update) {
		    case gmdb_primary:
			if(policy == DB_REPMGR_ACKS_ALL || policy == DB_REPMGR_ACKS_ALL_PEERS)
				policy = DB_REPMGR_ACKS_ALL_AVAILABLE;
			else if(policy == DB_REPMGR_ACKS_QUORUM && nclients == 1)
				nclients = 0;
			else if((policy == DB_REPMGR_ACKS_ONE || policy == DB_REPMGR_ACKS_ONE_PEER) && nclients == 1) {
				nclients = 0;
				policy = DB_REPMGR_ACKS_QUORUM;
			}
			break;
		    case gmdb_secondary:
			policy = DB_REPMGR_ACKS_NONE;
			break;
		    case gmdb_none:
			break;
		}
		switch(policy) {
		    case DB_REPMGR_ACKS_NONE:
			needed = 0;
			COMPQUIET(available, 0);
			break;

		    case DB_REPMGR_ACKS_ONE:
			needed = 1;
			available = nsites_sent;
			break;

		    case DB_REPMGR_ACKS_ALL:
			/* Number of sites in the group besides myself. */
			needed = nclients;
			available = nsites_sent;
			break;

		    case DB_REPMGR_ACKS_ONE_PEER:
			needed = 1;
			available = npeers_sent;
			break;

		    case DB_REPMGR_ACKS_ALL_PEERS:
			/*
			 * Too hard to figure out "needed", since we're not
			 * keeping track of how many peers we have; so just skip
			 * the optimization in this case.
			 */
			needed = 1;
			available = npeers_sent;
			break;

		    case DB_REPMGR_ACKS_QUORUM:
		    case DB_REPMGR_ACKS_ALL_AVAILABLE:
			/*
			 * The minimum number of acks necessary to ensure that
			 * the transaction is durable if an election is held.
			 *
			 * Unless instructed otherwise, our special handling for
			 * 2-site groups means that a client that loses contact
			 * with the master elects itself master (even though
			 * that doesn't constitute a majority).  In order to
			 * provide the expected guarantee implied by the
			 * definition of "quorum" we have to fudge the ack
			 * calculation in this case: specifically, we need to
			 * make sure that the client has received it in order
			 * for us to consider it "perm".  Thus, if nclients is
			 * 1, needed should be 1.
			 *
			 * While we're at it, if nclients is 0 (a nascent
			 * "group" consisting of nothing but a master), surely
			 * the number of acks we need should be 0.
			 *
			 * Note that turning the usual strict behavior back on
			 * in a 2-site group results in "0" as the number of
			 * clients needed to ack a txn in order for it to have
			 * arrived at a quorum.  This is the correct result,
			 * strange as it may seem!  This may well mean that in a
			 * 2-site group the QUORUM policy is rarely the right
			 * choice.
			 *
			 * When a GMDB update adds the second site, force
			 * "strict" behavior: in that case nsites is 2, but the
			 * new site is not yet allowed to contribute an ack.
			 */
			if(nclients > 1 || FLD_ISSET(db_rep->region->config, REP_C_2SITE_STRICT) ||
			   db_rep->active_gmdb_update == gmdb_primary)
				needed = nclients/2;
			else
				needed = nclients;
			if(policy == DB_REPMGR_ACKS_ALL_AVAILABLE) {
				quorum = needed;
				needed = available = nsites_sent;
			}
			else {
				available = npeers_sent;
				quorum = 0;
			}
			break;

		    default:
			COMPQUIET(available, 0);
			COMPQUIET(needed, 0);
			__db_unknown_path(env, "__repmgr_send");
			break;
		}
		if(needed == 0)
			goto out;
		if(available < needed) {
			ret = DB_REP_UNAVAIL;
			goto out;
		}
		/* In ALL_PEERS case, display of "needed" might be confusing. */
		VPRINT(env, (env, DB_VERB_REPMGR_MISC, "will await acknowledgement: need %u", needed));
		perm.lsn = *lsnp;
		perm.threshold = needed;
		perm.policy = policy;
		ret = __repmgr_await_cond(env, is_permanent, &perm, rep->ack_timeout, &db_rep->ack_waiters);
		/*
		 * If using ACKS_ALL_AVAILABLE and all possible sites acked,
		 * only return success if we have a quorum minimum available
		 * to ensure data integrity.
		 */
		if(ret == 0 && policy == DB_REPMGR_ACKS_ALL_AVAILABLE && available < quorum)
			ret = DB_REP_UNAVAIL;
	}
out:
	UNLOCK_MUTEX(db_rep->mutex);
	if(LF_ISSET(DB_REP_PERMANENT)) {
		if(ret) {
			switch(db_rep->active_gmdb_update) {
			    case gmdb_none:
				/*
				 * Fire perm-failed event to the application as
				 * usual; no other bookkeeping needed here.
				 */
				STAT(db_rep->region->mstat.st_perm_failed++);
				DB_EVENT(env, DB_EVENT_REP_PERM_FAILED, 0);
				break;
			    case gmdb_primary:
				/*
				 * Since this is a membership DB operation,
				 * refrain from bothering the application about
				 * it (with an event that it wouldn't be
				 * expecting), and make a note of the failure so
				 * we can resolve it later.
				 */
				db_rep->limbo_failure = *lsnp;
			    // @fallthrough
			    case gmdb_secondary:
				/* Merely refrain from firing event. */
				RPRINT(env, (env, DB_VERB_REPMGR_MISC, "GMDB perm failure %d at [%lu][%lu]",
					(int)db_rep->active_gmdb_update, (ulong)lsnp->file, (ulong)lsnp->Offset_));
				break;
			}
		}
		else if(db_rep->limbo_resolution_needed) {
			/*
			 * A previous membership DB operation failed, leaving us
			 * "in limbo", but now some perm operation has completed
			 * successfully.  Since the ack of any txn implies ack
			 * of all txns that occur before it (in LSN order), we
			 * now know that the previous failure can be resolved.
			 * We can't do it here in this thread, so put a request
			 * on the message processing queue to have it handled later.
			 */
			db_rep->durable_lsn = *lsnp;
			RPRINT(env, (env, DB_VERB_REPMGR_MISC, "perm success [%lu][%lu] with limbo resolution needed",
				(ulong)lsnp->file, (ulong)lsnp->Offset_));
			db_rep->limbo_resolution_needed = FALSE;
			/* Don't trump ret, even if it's zero. */
			LOCK_MUTEX(db_rep->mutex);
			if((t_ret = __repmgr_defer_op(env, REPMGR_RESOLVE_LIMBO)) != 0)
				__db_err(env, t_ret, "repmgr_defer_op");
			UNLOCK_MUTEX(db_rep->mutex);
		}
	}
	return ret;
}

static REPMGR_SITE * __repmgr_available_site(ENV * env, int eid)
{
	DB_REP * db_rep = env->rep_handle;
	REPMGR_SITE * site = SITE_FROM_EID(eid);
	if(IS_SITE_HANDSHAKEN(site))
		return site;
	return NULL;
}
/*
 * Synchronize our list of sites with new information that has been added to the
 * list in the shared region.
 *
 * PUBLIC: int __repmgr_sync_siteaddr(ENV *);
 */
int __repmgr_sync_siteaddr(ENV*env)
{
	DB_REP * db_rep;
	REP * rep;
	uint added;
	int ret = 0;
	db_rep = env->rep_handle;
	rep = db_rep->region;
	MUTEX_LOCK(env, rep->mtx_repmgr);
	if(!IS_VALID_EID(db_rep->self_eid))
		db_rep->self_eid = rep->self_eid;
	added = db_rep->site_cnt;
	if((ret = __repmgr_copy_in_added_sites(env)) == 0)
		ret = __repmgr_init_new_sites(env, added, db_rep->site_cnt);
	MUTEX_UNLOCK(env, rep->mtx_repmgr);
	return ret;
}
/*
 * Sends message to all sites with which we currently have an active
 * connection.  Sets result parameters according to how many sites we attempted
 * to begin sending to, even if we did nothing more than queue it for later
 * delivery.
 *
 * !!!
 * Caller must hold env->mutex.
 * PUBLIC: int __repmgr_send_broadcast __P((ENV *, uint,
 * PUBLIC:    const DBT *, const DBT *, uint *, uint *));
 */
int __repmgr_send_broadcast(ENV * env, uint type, const DBT * control, const DBT * rec, uint * nsitesp, uint * npeersp)
{
	DB_REP * db_rep;
	REP * rep;
	struct sending_msg msg;
	REPMGR_CONNECTION * conn;
	REPMGR_SITE * site;
	REPMGR_IOVECS iovecs;
	uint8 msg_hdr_buf[__REPMGR_MSG_HDR_SIZE];
	uint eid, nsites, npeers;
	int full_member, ret;

	static const uint version_max_msg_type[] = {
		0,
		REPMGR_MAX_V1_MSG_TYPE,
		REPMGR_MAX_V2_MSG_TYPE,
		REPMGR_MAX_V3_MSG_TYPE,
		REPMGR_MAX_V4_MSG_TYPE
	};

	db_rep = env->rep_handle;
	rep = db_rep->region;

	/*
	 * Sending a broadcast is quick, because we allow no blocking.  So it
	 * shouldn't much matter.  But just in case, take the timestamp before
	 * sending, so that if anything we err on the side of keeping clients
	 * placated (i.e., possibly sending a heartbeat slightly more frequently
	 * than necessary).
	 */
	__os_gettime(env, &db_rep->last_bcast, 1);

	msg.iovecs = &iovecs;
	setup_sending_msg(env, &msg, msg_hdr_buf, type, control, rec);
	nsites = npeers = 0;

	/* Send to (only the main connection with) every site. */
	FOR_EACH_REMOTE_SITE_INDEX(eid) {
		if((site = __repmgr_available_site(env, (int)eid)) == NULL)
			continue;
		/*
		 * Exclude non-member sites, unless we're the master, since it's
		 * useful to keep letting a removed site see updates so that it
		 * learns of its own removal, and will know to rejoin at its
		 * next reboot.
		 */
		if(site->membership == SITE_PRESENT)
			full_member = TRUE;
		else {
			full_member = FALSE;
			if(rep->master_id != db_rep->self_eid)
				continue;
		}
		conn = site->ref.conn;

		DB_ASSERT(env, IS_KNOWN_REMOTE_SITE(conn->eid) &&
			conn->version > 0 &&
			conn->version <= DB_REPMGR_VERSION);
		/*
		 * Skip if the type of message we're sending is beyond the range
		 * of known message types for this connection's version.
		 *
		 * !!!
		 * Don't be misled by the apparent generality of this simple
		 * test.  It works currently, because the only kinds of messages
		 * that we broadcast are REP_MESSAGE and HEARTBEAT.  But in the
		 * future other kinds of messages might require more intricate
		 * per-connection-version customization (for example,
		 * per-version message format conversion, addition of new
		 * fields, etc.).
		 */
		if(type > version_max_msg_type[conn->version])
			continue;
		/*
		 * Broadcast messages are either application threads committing
		 * transactions, or replication status message that we can
		 * afford to lose.  So don't allow blocking for them (pass
		 * maxblock argument as 0).
		 */
		if((ret = __repmgr_send_internal(env, conn, &msg, 0)) == 0) {
			if(full_member) {
				/*
				 * Since the purpose of the counting is to
				 * manage waiting for acks, only count sites we
				 * send to from which we can reasonably expect
				 * to get an ack.  When a site is not a fully
				 * "present" member of the group we can't accept
				 * an ack from it.
				 */
				nsites++;
				if(F_ISSET(site, SITE_ELECTABLE))
					npeers++;
			}
		}
		else if(ret == DB_TIMEOUT) {
			/*
			 * Couldn't send because of a full output queue.
			 * Incrementing counters would be wrong, but it's
			 * otherwise OK in the sense that the connection isn't
			 * definitively known to be broken, and rep protocol
			 * always allows us to drop a message if we have to.
			 */
			ret = 0;
		}
		else if(ret == DB_REP_UNAVAIL) {
			if((ret = __repmgr_bust_connection(env, conn)) != 0)
				return ret;
		}
		else
			return ret;
	}

	*nsitesp = nsites;
	*npeersp = npeers;
	return 0;
}
/*
 * __repmgr_send_one --
 *	Send a message to a site, or if you can't just yet, make a copy of it
 * and arrange to have it sent later.  'rec' may be NULL, in which case we send
 * a zero length and no data.
 *
 * !!!
 * Note that the mutex should be held through this call.
 * It doubles as a synchronizer to make sure that two threads don't
 * intersperse writes that are part of two single messages.
 *
 * PUBLIC: int __repmgr_send_one __P((ENV *, REPMGR_CONNECTION *,
 * PUBLIC:    uint, const DBT *, const DBT *, db_timeout_t));
 */
int __repmgr_send_one(ENV * env, REPMGR_CONNECTION * conn, uint msg_type, const DBT * control, const DBT * rec, db_timeout_t maxblock)
{
	struct sending_msg msg;
	REPMGR_IOVECS iovecs;
	uint8 hdr_buf[__REPMGR_MSG_HDR_SIZE];
	int ret;
	msg.iovecs = &iovecs;
	setup_sending_msg(env, &msg, hdr_buf, msg_type, control, rec);
	if((ret = __repmgr_send_internal(env, conn, &msg, maxblock)) == DB_TIMEOUT && maxblock == 0)
		ret = 0;
	return ret;
}
/*
 * PUBLIC: int __repmgr_send_many __P((ENV *,
 * PUBLIC:     REPMGR_CONNECTION *, REPMGR_IOVECS *, db_timeout_t));
 */
int __repmgr_send_many(ENV * env, REPMGR_CONNECTION * conn, REPMGR_IOVECS * iovecs, db_timeout_t maxblock)
{
	struct sending_msg msg;
	int ret;
	if(conn->state == CONN_DEFUNCT)
		return DB_REP_UNAVAIL;
	msg.iovecs = iovecs;
	msg.fmsg = NULL;
	if((ret = __repmgr_send_internal(env, conn, &msg, maxblock)) == DB_TIMEOUT && maxblock == 0)
		ret = 0;
	if(ret != 0 && ret != DB_TIMEOUT)
		__repmgr_disable_connection(env, conn);
	return ret;
}
/*
 * PUBLIC: int __repmgr_send_own_msg __P((ENV *,
 * PUBLIC:     REPMGR_CONNECTION *, uint32, uint8 *, uint32));
 */
int __repmgr_send_own_msg(ENV * env, REPMGR_CONNECTION * conn, uint32 type, uint8 * buf, uint32 len)
{
	REPMGR_IOVECS iovecs;
	struct sending_msg msg;
	__repmgr_msg_hdr_args msg_hdr;
	uint8 hdr_buf[__REPMGR_MSG_HDR_SIZE];
	if(conn->version < OWN_MIN_VERSION)
		return 0;
	msg_hdr.type = REPMGR_OWN_MSG;
	REPMGR_OWN_BUF_SIZE(msg_hdr) = len;
	REPMGR_OWN_MSG_TYPE(msg_hdr) = type;
	__repmgr_msg_hdr_marshal(env, &msg_hdr, hdr_buf);
	__repmgr_iovec_init(&iovecs);
	__repmgr_add_buffer(&iovecs, hdr_buf, __REPMGR_MSG_HDR_SIZE);
	if(len > 0)
		__repmgr_add_buffer(&iovecs, buf, len);
	msg.iovecs = &iovecs;
	msg.fmsg = NULL;
	return __repmgr_send_internal(env, conn, &msg, 0);
}
/*
 * Attempts a "best effort" to send a message on the given site.  If there is an
 * excessive backlog of message already queued on the connection, what shall we
 * do?  If the caller doesn't mind blocking, we'll wait (a limited amount of
 * time) for the queue to drain.  Otherwise we'll simply drop the message.  This
 * is always allowed by the replication protocol.  But in the case of a
 * multi-message response to a request like PAGE_REQ, LOG_REQ or ALL_REQ we
 * almost always get a flood of messages that instantly fills our queue, so
 * blocking improves performance (by avoiding the need for the client to
 * re-request).
 */
static int __repmgr_send_internal(ENV * env, REPMGR_CONNECTION * conn, struct sending_msg * msg, db_timeout_t maxblock)
{
	SITE_STRING_BUFFER buffer;
	int ret;
	size_t total_written;
	DB_REP * db_rep = env->rep_handle;
	DB_ASSERT(env, conn->state != CONN_DEFUNCT);
	if(!STAILQ_EMPTY(&conn->outbound_queue)) {
		/*
		 * Output to this site is currently owned by the select()
		 * thread, so we can't try sending in-line here.  We can only
		 * queue the msg for later.
		 */
		VPRINT(env, (env, DB_VERB_REPMGR_MISC, "msg to %s to be queued", __repmgr_format_eid_loc(db_rep, conn, buffer)));
		if(conn->out_queue_length >= OUT_QUEUE_LIMIT && maxblock > 0 && conn->state != CONN_CONGESTED) {
			VPRINT(env, (env, DB_VERB_REPMGR_MISC, "block thread, awaiting output queue space"));
			conn->ref_count++;
			ret = __repmgr_await_drain(env, conn, maxblock);
			conn->ref_count--;
			VPRINT(env, (env, DB_VERB_REPMGR_MISC, "drain returned %d (%d,%d)", ret, db_rep->finished, conn->out_queue_length));
			if(db_rep->finished)
				return DB_TIMEOUT;
			if(ret)
				return ret;
			if(STAILQ_EMPTY(&conn->outbound_queue))
				goto empty;
		}
		if(conn->out_queue_length < OUT_QUEUE_LIMIT)
			return enqueue_msg(env, conn, msg, 0);
		else {
			RPRINT(env, (env, DB_VERB_REPMGR_MISC, "queue limit exceeded"));
			STAT(env->rep_handle->region->mstat.st_msgs_dropped++);
			return DB_TIMEOUT;
		}
	}
empty:
	if((ret = __repmgr_write_iovecs(env, conn, msg->iovecs, &total_written)) == 0)
		return 0;
	switch(ret) {
	    case WOULDBLOCK:
#if defined(DB_REPMGR_EAGAIN) && DB_REPMGR_EAGAIN != WOULDBLOCK
	    case DB_REPMGR_EAGAIN:
#endif
		break;
	    default:
#ifdef EBADF
		DB_ASSERT(env, ret != EBADF);
#endif
		__repmgr_fire_conn_err_event(env, conn, ret);
		STAT(env->rep_handle->region->mstat.st_connection_drop++);
		return DB_REP_UNAVAIL;
	}
	VPRINT(env, (env, DB_VERB_REPMGR_MISC, "wrote only %lu bytes to %s",
		(ulong)total_written, __repmgr_format_eid_loc(db_rep, conn, buffer)));
	/*
	 * We can't send any more without blocking: queue (a pointer to) a
	 * "flattened" copy of the message, so that the select() thread will
	 * finish sending it later.
	 */
	if((ret = enqueue_msg(env, conn, msg, total_written)) != 0)
		return ret;
	STAT(env->rep_handle->region->mstat.st_msgs_queued++);

	/*
	 * Wake the main select thread so that it can discover that it has
	 * received ownership of this connection.  Note that we didn't have to
	 * do this in the previous case (above), because the non-empty queue
	 * implies that the select() thread is already managing ownership of
	 * this connection.
	 */
	return __repmgr_wake_main_thread(env);
}
/*
 * PUBLIC: int __repmgr_write_iovecs __P((ENV *, REPMGR_CONNECTION *,
 * PUBLIC:     REPMGR_IOVECS *, size_t *));
 */
int __repmgr_write_iovecs(ENV * env, REPMGR_CONNECTION * conn, REPMGR_IOVECS * iovecs, size_t * writtenp)
{
	REPMGR_IOVECS iovec_buf, * v;
	size_t nw, sz, total_written;
	int ret;
	/*
	 * Send as much data to the site as we can, without blocking.  Keep
	 * writing as long as we're making some progress.
	 *
	 * Make a scratch copy of iovecs for our use, since we destroy it in the
	 * process of adjusting pointers after each partial I/O.  The minimal
	 * REPMGR_IOVECS struct template is usually enough.  But for app
	 * messages that need more than 3 segments we allocate a separate
	 * buffer.
	 */
	if(iovecs->count <= MIN_IOVEC) {
		v = &iovec_buf;
		sz = sizeof(iovec_buf);
	}
	else {
		sz = (size_t)REPMGR_IOVECS_ALLOC_SZ((uint)iovecs->count);
		if((ret = __os_malloc(env, sz, &v)) != 0)
			return ret;
	}
	memcpy(v, iovecs, sz);
	total_written = 0;
	while((ret = __repmgr_writev(conn->fd, &v->vectors[v->offset], v->count-v->offset, &nw)) == 0) {
		total_written += nw;
		if(__repmgr_update_consumed(v, nw))  /* all written */
			break;
	}
	*writtenp = total_written;
	if(v != &iovec_buf)
		__os_free(env, v);
	return ret;
}
/*
 * Count up how many sites have ack'ed the given LSN.  Returns TRUE if enough
 * sites have ack'ed; FALSE otherwise.
 *
 * !!!
 * Caller must hold the mutex.
 */
static int is_permanent(ENV * env, void * context)
{
	REPMGR_SITE * site;
	uint eid, nsites, npeers;
	int is_perm, has_missing_peer;
	DB_REP * db_rep = env->rep_handle;
	struct repmgr_permanence * perm = (struct repmgr_permanence *)context;
	int policy = perm->policy;
	if(policy == DB_REPMGR_ACKS_NONE)
		return TRUE;
	nsites = npeers = 0;
	has_missing_peer = FALSE;
	FOR_EACH_REMOTE_SITE_INDEX(eid) {
		site = SITE_FROM_EID(eid);
		if(site->membership != SITE_PRESENT)
			continue;
		if(!F_ISSET(site, SITE_HAS_PRIO)) {
			/*
			 * Never connected to this site: since we can't know
			 * whether it's a peer, assume the worst.
			 */
			has_missing_peer = TRUE;
			continue;
		}
		if(LOG_COMPARE(&site->max_ack, &perm->lsn) >= 0) {
			nsites++;
			if(F_ISSET(site, SITE_ELECTABLE))
				npeers++;
		}
		else {
			/* This site hasn't ack'ed the message. */
			if(F_ISSET(site, SITE_ELECTABLE))
				has_missing_peer = TRUE;
		}
	}
	VPRINT(env, (env, DB_VERB_REPMGR_MISC, "checking perm result, %lu, %lu, %d", (ulong)nsites, (ulong)npeers, has_missing_peer));
	switch(policy) {
	    case DB_REPMGR_ACKS_ALL:
	    case DB_REPMGR_ACKS_ALL_AVAILABLE:
	    case DB_REPMGR_ACKS_ONE:
		is_perm = (nsites >= perm->threshold);
		break;
	    case DB_REPMGR_ACKS_ONE_PEER:
	    case DB_REPMGR_ACKS_QUORUM:
		is_perm = (npeers >= perm->threshold);
		break;
	    case DB_REPMGR_ACKS_ALL_PEERS:
		is_perm = !has_missing_peer;
		break;
	    default:
		is_perm = FALSE;
		__db_unknown_path(env, "is_permanent");
	}
	return is_perm;
}
/*
 * Abandons a connection, to recover from an error.  Takes necessary recovery
 * action.  Note that we don't actually close and clean up the connection here;
 * that happens later, in the select() thread main loop.  See further
 * explanation at function __repmgr_disable_connection().
 *
 * PUBLIC: int __repmgr_bust_connection __P((ENV *, REPMGR_CONNECTION *));
 *
 * !!!
 * Caller holds mutex.
 */
int __repmgr_bust_connection(ENV * env, REPMGR_CONNECTION * conn)
{
	REPMGR_SITE * site;
	uint32 flags;
	int subordinate_conn;
	DB_REP * db_rep = env->rep_handle;
	REP * rep = db_rep->region;
	int ret = 0;
	int eid = conn->eid;
	if((ret = __repmgr_disable_connection(env, conn)) != 0)
		return ret;
	/*
	 * Take any/all appropriate recovery steps, depending on the nature of
	 * the connection, whom it was with, and our current role.
	 */
	if(conn->type == REP_CONNECTION && IS_KNOWN_REMOTE_SITE(eid)) {
		site = SITE_FROM_EID(eid);
		subordinate_conn = (conn != site->ref.conn);
		if(!subordinate_conn &&
		   (ret = __repmgr_schedule_connection_attempt(env,
			    (uint)eid, FALSE)) != 0)
			return ret;
		/*
		 * If the failed connection was the one between us and the
		 * master, assume that the master may have failed, and call for
		 * an election.  But only do this for the connection to the main
		 * master process, not a subordinate one.  And only do it if
		 * we're our site's main process, not a subordinate one.  And
		 * skip it if the application has configured us not to do
		 * elections.
		 */
		if(!IS_SUBORDINATE(db_rep) &&
		   !subordinate_conn && eid == rep->master_id) {
			/*
			 * Even if we're not doing elections, defer the event
			 * notification to later execution in the election
			 * thread.  We don't want to fire an event in the select
			 * thread, and certainly not while holding the mutex.
			 */
			flags = ELECT_F_EVENT_NOTIFY;
			if(FLD_ISSET(db_rep->region->config, REP_C_ELECTIONS))
				LF_SET(ELECT_F_IMMED|ELECT_F_FAST);
			else
				RPRINT(env, (env, DB_VERB_REPMGR_MISC,
					     "Master failure, but no elections"));
			if((ret = __repmgr_init_election(env, flags)) != 0)
				return ret;
		}
		/*
		 * If we're the master site, and we lose a main connection to a
		 * client (whether we're the main replication process or a
		 * subordinate process), then the client is going to have
		 * trouble receiving live log records from us.  So, set the
		 * temporary log archive block timer, to give the client a
		 * fighting chance to restart/recover/reconnect.  (We don't care
		 * about the client's subordinate connections to us -- i.e.,
		 * connections with a subordinate process at the client site --
		 * because those sites can only be reading, not applying updates
		 * from us.)
		 */
		if(!subordinate_conn && rep->master_id == db_rep->self_eid) {
			RPRINT(env, (env, DB_VERB_REPMGR_MISC, "Repmgr: bust connection.  Block archive"));
			MASTER_UPDATE(env, (REGENV *)env->reginfo->primary);
		}
	}
	return 0;
}

/*
 * Remove a connection from the possibility of any further activity, making sure
 * it ends up on the main connections list, so that it will be cleaned up at the
 * next opportunity in the select() thread.
 *
 * Various threads write onto TCP/IP sockets, and an I/O error could occur at
 * any time.  However, only the dedicated "select()" thread may close the socket
 * file descriptor, because under POSIX we have to drop our mutex and then call
 * select() as two distinct (non-atomic) operations.
 *
 * To simplify matters, there is a single place in the select thread where we
 * close and clean up after any defunct connection.  Even if the I/O error
 * happens in the select thread we follow this convention.
 *
 * When an error occurs, we disable the connection (mark it defunct so that no
 * one else will try to use it, and so that the select thread will find it and
 * clean it up), and then usually take some additional recovery action: schedule
 * a connection retry for later, and possibly call for an election if it was a
 * connection to the master.  (This happens in the function
 * __repmgr_bust_connection.)  But sometimes we don't want to do the recovery
 * part; just the disabling part.
 *
 * PUBLIC: int __repmgr_disable_connection __P((ENV *, REPMGR_CONNECTION *));
 */
int __repmgr_disable_connection(ENV * env, REPMGR_CONNECTION * conn)
{
	DB_REP * db_rep;
	REPMGR_SITE * site;
	REPMGR_RESPONSE * resp;
	uint32 i;
	int eid, ret, t_ret;
	db_rep = env->rep_handle;
	ret = 0;
	conn->state = CONN_DEFUNCT;
	if(conn->type == REP_CONNECTION) {
		eid = conn->eid;
		if(IS_VALID_EID(eid)) {
			site = SITE_FROM_EID(eid);
			if(conn != site->ref.conn)
				/* It's a subordinate connection. */
				TAILQ_REMOVE(&site->sub_conns, conn, entries);
			TAILQ_INSERT_TAIL(&db_rep->connections, conn, entries);
			conn->ref_count++;
		}
		conn->eid = -1;
	}
	else if(conn->type == APP_CONNECTION) {
		for(i = 0; i < conn->aresp; i++) {
			resp = &conn->responses[i];
			if(F_ISSET(resp, RESP_IN_USE) && F_ISSET(resp, RESP_THREAD_WAITING)) {
				F_SET(resp, RESP_COMPLETE);
				resp->ret = DB_REP_UNAVAIL;
			}
		}
		ret = __repmgr_wake_waiters(env, &conn->response_waiters);
	}
	if((t_ret = __repmgr_signal(&conn->drained)) != 0 && ret == 0)
		ret = t_ret;
	if((t_ret = __repmgr_wake_main_thread(env)) != 0 && ret == 0)
		ret = t_ret;
	return ret;
}
/*
 * PUBLIC: int __repmgr_cleanup_defunct __P((ENV *, REPMGR_CONNECTION *));
 *
 * Caller should hold mutex, since we remove connection from main list.
 */
int __repmgr_cleanup_defunct(ENV * env, REPMGR_CONNECTION * conn)
{
	int t_ret;
	DB_REP * db_rep = env->rep_handle;
	int ret = __repmgr_close_connection(env, conn);
	TAILQ_REMOVE(&db_rep->connections, conn, entries);
	if((t_ret = __repmgr_decr_conn_ref(env, conn)) != 0 && ret == 0)
		ret = t_ret;
	return ret;
}
/*
 * PUBLIC: int __repmgr_close_connection __P((ENV *, REPMGR_CONNECTION *));
 */
int __repmgr_close_connection(ENV*env, REPMGR_CONNECTION * conn)
{
	int ret = 0;
#ifdef DB_WIN32
	int t_ret;
#endif
	if(conn->fd != INVALID_SOCKET && closesocket(conn->fd) == SOCKET_ERROR) {
		ret = net_errno;
		__db_err(env, ret, DB_STR("3582", "closing socket"));
	}
	conn->fd = INVALID_SOCKET;
#ifdef DB_WIN32
	if(conn->event_object != WSA_INVALID_EVENT && !WSACloseEvent(conn->event_object)) {
		t_ret = net_errno;
		__db_err(env, t_ret, DB_STR("3583", "releasing WSA event object"));
		if(!ret)
			ret = t_ret;
	}
	conn->event_object = WSA_INVALID_EVENT;
#endif
	return ret;
}
/*
 * Decrements a connection's ref count; destroys the connection when the ref
 * count reaches zero.
 *
 * PUBLIC: int __repmgr_decr_conn_ref __P((ENV *, REPMGR_CONNECTION *));
 */
int __repmgr_decr_conn_ref(ENV * env, REPMGR_CONNECTION * conn)
{
	DB_ASSERT(env, conn->ref_count > 0);
	return --conn->ref_count > 0 ? 0 : __repmgr_destroy_conn(env, conn);
}
/*
 * Destroys a conn struct, by freeing all memory and associated resources.
 * (This is a destructor, so it always must run to completion, and of course the
 * passed-in object no longer exists upon return.)
 *
 * PUBLIC: int __repmgr_destroy_conn __P((ENV *, REPMGR_CONNECTION *));
 *
 * Caller is responsible for holding mutex if necessary; we make no assumption
 * here, since we operate only on the given connection, in isolation.  (However,
 * note that if this conn has messages on its outbound queue, those are shared
 * objects, and we decrement the ref count.  So in that case the mutex will need
 * to be held.)
 */
int __repmgr_destroy_conn(ENV * env, REPMGR_CONNECTION * conn)
{
	QUEUED_OUTPUT * out;
	REPMGR_FLAT * msg;
	REPMGR_RESPONSE * resp;
	DBT * dbt;
	int t_ret;
	int ret = 0;
	DB_ASSERT(env, conn->ref_count == 0);
	/*
	 * Deallocate any input and output buffers we may have.
	 */
	if(conn->reading_phase == DATA_PHASE) {
		switch(conn->msg_type) {
		    case REPMGR_OWN_MSG:
			if(conn->input.rep_message == NULL)
				break;
		    // @fallthrough
		    case REPMGR_APP_MESSAGE:
		    case REPMGR_HEARTBEAT:
		    case REPMGR_REP_MESSAGE:
			__os_free(env, conn->input.rep_message);
			break;

		    case REPMGR_APP_RESPONSE:
			/*
			 * DATA_PHASE of an APP_RESPONSE is another way of
			 * saying there must be a cur_resp, and it must be
			 * READING.
			 */
			DB_ASSERT(env, conn->cur_resp < conn->aresp && conn->responses != NULL);
			resp = &conn->responses[conn->cur_resp];
			DB_ASSERT(env, F_ISSET(resp, RESP_READING));
			if(F_ISSET(resp, RESP_DUMMY_BUF))
				__os_free(env, resp->dbt.data);
			break;

		    case REPMGR_PERMLSN:
		    case REPMGR_HANDSHAKE:
				dbt = (DBT *)&conn->input.repmgr_msg.cntrl;
				if(dbt->size > 0)
					__os_free(env, dbt->data);
				dbt = (DBT *)&conn->input.repmgr_msg.rec;
				if(dbt->size > 0)
					__os_free(env, dbt->data);
				break;
		    case REPMGR_RESP_ERROR:
		    /*
		 * This type doesn't use a DATA_PHASE, so this should be
		 * impossible.
		     */
		    default:
			ret = __db_unknown_path(env, "destroy_conn");
		}
	}
	if(conn->type == APP_CONNECTION && conn->responses != NULL)
		__os_free(env, conn->responses);
	if((t_ret = __repmgr_destroy_waiters(env,
		    &conn->response_waiters)) != 0 && ret == 0)
		ret = t_ret;
	while(!STAILQ_EMPTY(&conn->outbound_queue)) {
		out = STAILQ_FIRST(&conn->outbound_queue);
		STAILQ_REMOVE_HEAD(&conn->outbound_queue, entries);
		msg = out->msg;
		if(--msg->ref_count <= 0)
			__os_free(env, msg);
		__os_free(env, out);
	}
	if((t_ret = __repmgr_free_cond(&conn->drained)) != 0 && ret == 0)
		ret = t_ret;
	__os_free(env, conn);
	return ret;
}

static int enqueue_msg(ENV * env, REPMGR_CONNECTION * conn, struct sending_msg * msg, size_t offset)
{
	QUEUED_OUTPUT * q_element;
	int ret;
	if(msg->fmsg == NULL && ((ret = flatten(env, msg)) != 0))
		return ret;
	if((ret = __os_malloc(env, sizeof(QUEUED_OUTPUT), &q_element)) != 0)
		return ret;
	q_element->msg = msg->fmsg;
	msg->fmsg->ref_count++; /* encapsulation would be sweeter */
	q_element->offset = offset;
	/* Put it on the connection's outbound queue. */
	STAILQ_INSERT_TAIL(&conn->outbound_queue, q_element, entries);
	conn->out_queue_length++;
	return 0;
}
/*
 * Either "control" or "rec" (or both) may be NULL, in which case we treat it
 * like a zero-length DBT.
 */
static void setup_sending_msg(ENV * env, struct sending_msg * msg, uint8 * hdr_buf, uint type, const DBT * control, const DBT * rec)
{
	__repmgr_msg_hdr_args msg_hdr;
	/*
	 * Since we know that the msg hdr is a fixed size, we can add its buffer
	 * to the iovecs before actually marshaling the content.  But the
	 * add_buffer and add_dbt calls have to be in the right order.
	 */
	__repmgr_iovec_init(msg->iovecs);
	__repmgr_add_buffer(msg->iovecs, hdr_buf, __REPMGR_MSG_HDR_SIZE);
	msg_hdr.type = type;
	if((REP_MSG_CONTROL_SIZE(msg_hdr) = (control == NULL ? 0 : control->size)) > 0)
		__repmgr_add_dbt(msg->iovecs, control);
	if((REP_MSG_REC_SIZE(msg_hdr) = (rec == NULL ? 0 : rec->size)) > 0)
		__repmgr_add_dbt(msg->iovecs, rec);
	__repmgr_msg_hdr_marshal(env, &msg_hdr, hdr_buf);
	msg->fmsg = NULL;
}
/*
 * Convert a message stored as iovec pointers to various pieces, into flattened
 * form, by copying all the pieces, and then make the iovec just point to the
 * new simplified form.
 */
static int flatten(ENV*env, struct sending_msg * msg)
{
	uint8 * p;
	size_t msg_size;
	int i, ret;
	DB_ASSERT(env, msg->fmsg == NULL);
	msg_size = msg->iovecs->total_bytes;
	if((ret = __os_malloc(env, sizeof(*msg->fmsg)+msg_size, &msg->fmsg)) != 0)
		return ret;
	msg->fmsg->length = msg_size;
	msg->fmsg->ref_count = 0;
	p = &msg->fmsg->data[0];
	for(i = 0; i < msg->iovecs->count; i++) {
		memcpy(p, msg->iovecs->vectors[i].iov_base, msg->iovecs->vectors[i].iov_len);
		p = &p[msg->iovecs->vectors[i].iov_len];
	}
	__repmgr_iovec_init(msg->iovecs);
	__repmgr_add_buffer(msg->iovecs, &msg->fmsg->data[0], msg_size);
	return 0;
}
/*
 * Scan the list of remote sites, returning the first one that is a peer,
 * is not the current master, and is available.
 */
static REPMGR_SITE * __repmgr_find_available_peer(ENV*env)
{
	DB_REP * db_rep;
	REP * rep;
	REPMGR_SITE * site;
	uint i;
	db_rep = env->rep_handle;
	rep = db_rep->region;
	FOR_EACH_REMOTE_SITE_INDEX(i) {
		site = &db_rep->sites[i];
		if(FLD_ISSET(site->config, DB_REPMGR_PEER) && EID_FROM_SITE(site) != rep->master_id && IS_SITE_AVAILABLE(site))
			return site;
	}
	return NULL;
}
/*
 * Copy host/port values into the given netaddr struct.  Allocates memory for
 * the copy of the host name, which becomes the responsibility of the caller.
 *
 * PUBLIC: int __repmgr_pack_netaddr __P((ENV *, const char *,
 * PUBLIC:     uint, repmgr_netaddr_t *));
 */
int __repmgr_pack_netaddr(ENV * env, const char * host, uint port, repmgr_netaddr_t * addr)
{
	int ret;
	DB_ASSERT(env, host != NULL);
	if((ret = __os_strdup(env, host, &addr->host)) != 0)
		return ret;
	addr->port = (uint16)port;
	return 0;
}
/*
 * PUBLIC: int __repmgr_getaddr __P((ENV *,
 * PUBLIC:     const char *, uint, int, ADDRINFO **));
 */
int __repmgr_getaddr(ENV * env, const char * host, uint port, int flags /* Matches struct addrinfo declaration. */, ADDRINFO ** result)
{
	ADDRINFO * answer, hints;
	char buffer[10]; /* 2**16 fits in 5 digits. */
	/*
	 * Ports are really 16-bit unsigned values, but it's too painful to
	 * push that type through the API.
	 */
	memzero(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = flags;
	snprintf(buffer, sizeof(buffer), "%u", port);
	/*
	 * Although it's generally bad to discard error information, the return
	 * code from __os_getaddrinfo is undependable.  Our callers at least
	 * would like to be able to distinguish errors in getaddrinfo (which we
	 * want to consider to be re-tryable), from other failure (e.g., EINVAL,
	 * above).
	 */
	if(__os_getaddrinfo(env, host, port, buffer, &hints, &answer) != 0)
		return DB_REP_UNAVAIL;
	*result = answer;

	return 0;
}
/*
 * Initialize a socket for listening.  Sets a file descriptor for the socket,
 * ready for an accept() call in a thread that we're happy to let block.
 *
 * PUBLIC:  int __repmgr_listen(ENV *);
 */
int __repmgr_listen(ENV*env)
{
	ADDRINFO * ai;
	repmgr_netaddr_t * addrp;
	const char * why;
	int sockopt, ret;
	socket_t s;
	DB_REP * db_rep = env->rep_handle;
	// Use OOB value as sentinel to show no socket open
	s = INVALID_SOCKET;
	addrp = &SITE_FROM_EID(db_rep->self_eid)->net_addr;
	if((ret = __repmgr_getaddr(env, addrp->host, addrp->port, AI_PASSIVE, &ai)) != 0)
		return ret;
	/*
	 * Given the assert is correct, we execute the loop at least once, which
	 * means 'why' will have been set by the time it's needed.  But of
	 * course lint doesn't know about DB_ASSERT.
	 */
	COMPQUIET(why, "");
	DB_ASSERT(env, ai != NULL);
	for(; ai != NULL; ai = ai->ai_next) {
		if((s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == INVALID_SOCKET) {
			why = DB_STR("3584", "can't create listen socket");
			continue;
		}
		/*
		 * When testing, it's common to kill and restart regularly.  On
		 * some systems, this causes bind to fail with "address in use"
		 * errors unless this option is set.
		 */
		sockopt = 1;
		if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (sockopt_t)&sockopt, sizeof(sockopt)) != 0) {
			why = DB_STR("3585", "can't set REUSEADDR socket option");
			break;
		}
		if(bind(s, ai->ai_addr, (socklen_t)ai->ai_addrlen) != 0) {
			why = DB_STR("3586", "can't bind socket to listening address");
			closesocket(s);
			s = INVALID_SOCKET;
			continue;
		}
		if(listen(s, 5) != 0) {
			why = DB_STR("3587", "listen()");
			break;
		}
		if((ret = __repmgr_set_nonblocking(s)) != 0) {
			__db_err(env, ret, DB_STR("3588", "can't unblock listen socket"));
			goto clean;
		}
		db_rep->listen_fd = s;
		goto out;
	}
	ret = net_errno;
	__db_err_simple_text(env, ret, why);
clean:
	if(s != INVALID_SOCKET)
		closesocket(s);
out:
	__os_freeaddrinfo(env, ai);
	return ret;
}
/*
 * PUBLIC: int __repmgr_net_close(ENV *);
 */
int __repmgr_net_close(ENV * env)
{
	DB_REP * db_rep = env->rep_handle;
	REP * rep = db_rep->region;
	int ret = __repmgr_each_connection(env, final_cleanup, NULL, FALSE);
	if(db_rep->listen_fd != INVALID_SOCKET) {
		if(closesocket(db_rep->listen_fd) == SOCKET_ERROR && ret == 0)
			ret = net_errno;
		db_rep->listen_fd = INVALID_SOCKET;
		rep->listener = 0;
	}
	return ret;
}
/*
	Called only from env->close(), so we know we're single threaded.
*/
static int final_cleanup(ENV * env, REPMGR_CONNECTION * conn, void * unused)
{
	DB_REP * db_rep;
	REPMGR_SITE * site;
	int ret, t_ret;
	COMPQUIET(unused, 0);
	db_rep = env->rep_handle;
	ret = __repmgr_close_connection(env, conn);
	/* Remove the connection from whatever list it's on, if any. */
	if(conn->type == REP_CONNECTION && IS_VALID_EID(conn->eid)) {
		site = SITE_FROM_EID(conn->eid);
		if(site->state == SITE_CONNECTED && conn == site->ref.conn) {
			/* Not on any list, so no need to do anything. */
		}
		else
			TAILQ_REMOVE(&site->sub_conns, conn, entries);
		t_ret = __repmgr_destroy_conn(env, conn);
	}
	else {
		TAILQ_REMOVE(&db_rep->connections, conn, entries);
		t_ret = __repmgr_decr_conn_ref(env, conn);
	}
	if(t_ret != 0 && ret == 0)
		ret = t_ret;
	return ret;
}
/*
 * PUBLIC: void __repmgr_net_destroy __P((ENV *, DB_REP *));
 */
void __repmgr_net_destroy(ENV*env, DB_REP * db_rep)
{
	REPMGR_RETRY * retry;
	REPMGR_SITE * site;
	uint i;
	if(db_rep->sites == NULL)
		return;
	while(!TAILQ_EMPTY(&db_rep->retries)) {
		retry = TAILQ_FIRST(&db_rep->retries);
		TAILQ_REMOVE(&db_rep->retries, retry, entries);
		__os_free(env, retry);
	}
	DB_ASSERT(env, TAILQ_EMPTY(&db_rep->connections));
	for(i = 0; i < db_rep->site_cnt; i++) {
		site = &db_rep->sites[i];
		DB_ASSERT(env, TAILQ_EMPTY(&site->sub_conns));
		__repmgr_cleanup_netaddr(env, &site->net_addr);
	}
	__os_free(env, db_rep->sites);
	db_rep->sites = NULL;
}

#ifdef  CONFIG_TEST
/*
 * Substitute a fake target port instead of the port actually configured, for
 * certain types of testing, if desired.
 *
 * When a DB_TEST_FAKE_PORT environment variable is present, it names a TCP/IP
 * port on which a "port arbiter" service may be running.  If it is indeed
 * running, we should send it a request to ask it what "fake" port to use in
 * place of the given "real" port.  (The "real" port is the port normally
 * configured, and present in the membership database.)  The arbiter is not
 * always running for all tests, so if it's not present it simply means we
 * should not substitute a fake port.  Also, even if it is running, in some
 * tests we don't want to substitute a fake port: in that case, the arbiter's
 * response could name the same port as the "real" port we sent it.
 *
 * !!! This is only used for testing.
 */
static uint fake_port(ENV * env, uint port)
{
 #define MIN_PORT        1
 #define MAX_PORT        65535
	ADDRINFO * ai0, * ai;
	db_iovec_t iovec;
	char * arbiter, buf[100], * end, * p;
	socket_t s;
	long result;
	size_t count;
	int ret;
	uint arbiter_port;
	if((arbiter = getenv("DB_TEST_FAKE_PORT")) == NULL)
		return port;
	if(__db_getlong(env->dbenv, "repmgr_net.c:fake_port", arbiter, MIN_PORT, MAX_PORT, &result) != 0)
		return port;
	arbiter_port = (uint)result;
	/*
	 * Send a message of the form "{config,Port}" onto a connection to
	 * arbiter_port.
	 */
	if((ret = __repmgr_getaddr(env, "localhost", arbiter_port, 0, &ai0)) != 0) {
		__db_err(env, ret, "fake_port:getaddr");
		return port;
	}
	s = INVALID_SOCKET;
	for(ai = ai0; ai != NULL; ai = ai->ai_next) {
		if((s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == SOCKET_ERROR) {
			ret = net_errno;
			s = INVALID_SOCKET;
			__db_err(env, ret, "fake_port:socket");
			goto err;
		}
		/*
		 * Note that port substitution is used in only a small number of
		 * tests.  When there is no "port arbiter" running, it's not an
		 * error; it just means we should use the normal configured port
		 * as is.
		 */
		if(connect(s, ai->ai_addr, (socklen_t)ai->ai_addrlen) != 0) {
			ret = net_errno;
			closesocket(s);
			s = INVALID_SOCKET;
		}
	}
	if(ret)
		goto err;
	snprintf(buf, sizeof(buf), "{config,%u}\r\n", port);
	iovec.iov_base = buf;
	iovec.iov_len = (ulong)sstrlen(buf);
	while((ret = __repmgr_writev(s, &iovec, 1, &count)) == 0) {
		iovec.iov_base = (uint8 *)iovec.iov_base+count;
		if((iovec.iov_len -= (ulong)count) == 0)
			break;
	}
	if(ret) {
		__db_err(env, ret, "fake_port:writev");
		goto err;
	}
	/* The response should be a line telling us what port to use. */
	iovec.iov_base = buf;
	iovec.iov_len = sizeof(buf);
	p = buf;
	while((ret = __repmgr_readv(s, &iovec, 1, &count)) == 0) {
		if(!count) {
			__db_errx(env, "fake_port: premature EOF");
			goto err;
		}
		/* Keep reading until we get a line end. */
		for(p = iovec.iov_base, end = &p[count]; p < end; p++)
			if(*p == '\r' || *p == '\n')
				break;
		if(p < end) {
			*p = '\0';
			break;
		}
		iovec.iov_base = (uint8 *)iovec.iov_base+count;
		iovec.iov_len -= (ulong)count;
		DB_ASSERT(env, iovec.iov_len > 0);
	}
	if(ret)
		goto err;
	if(__db_getlong(env->dbenv, "repmgr_net.c:fake_port", buf, MIN_PORT, MAX_PORT, &result) == 0)
		port = (uint)result;
err:
	/*
	 * Note that we always return some port value, even if an error happens.
	 * Since this is just test code: if an error prevented proper fake port
	 * substitution, it should result in a test failure.
	 */
	if(s != INVALID_SOCKET)
		closesocket(s);
	__os_freeaddrinfo(env, ai0);
	return port;
}
#endif
