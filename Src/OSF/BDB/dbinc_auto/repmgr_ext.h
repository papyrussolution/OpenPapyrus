/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_repmgr_ext_h_
#define	_repmgr_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __repmgr_init_recover(ENV *, DB_DISTAB *);
void __repmgr_handshake_marshal(ENV *, __repmgr_handshake_args *, uint8 *);
int __repmgr_handshake_unmarshal(ENV *, __repmgr_handshake_args *, uint8 *, size_t, uint8 **);
void __repmgr_v3handshake_marshal(ENV *, __repmgr_v3handshake_args *, uint8 *);
int __repmgr_v3handshake_unmarshal(ENV *, __repmgr_v3handshake_args *, uint8 *, size_t, uint8 **);
void __repmgr_v2handshake_marshal(ENV *, __repmgr_v2handshake_args *, uint8 *);
int __repmgr_v2handshake_unmarshal(ENV *, __repmgr_v2handshake_args *, uint8 *, size_t, uint8 **);
void __repmgr_parm_refresh_marshal(ENV *, __repmgr_parm_refresh_args *, uint8 *);
int __repmgr_parm_refresh_unmarshal(ENV *, __repmgr_parm_refresh_args *, uint8 *, size_t, uint8 **);
void __repmgr_permlsn_marshal(ENV *, __repmgr_permlsn_args *, uint8 *);
int __repmgr_permlsn_unmarshal(ENV *, __repmgr_permlsn_args *, uint8 *, size_t, uint8 **);
void __repmgr_version_proposal_marshal(ENV *, __repmgr_version_proposal_args *, uint8 *);
int __repmgr_version_proposal_unmarshal(ENV *, __repmgr_version_proposal_args *, uint8 *, size_t, uint8 **);
void __repmgr_version_confirmation_marshal(ENV *, __repmgr_version_confirmation_args *, uint8 *);
int __repmgr_version_confirmation_unmarshal(ENV *, __repmgr_version_confirmation_args *, uint8 *, size_t, uint8 **);
void __repmgr_msg_hdr_marshal(ENV *, __repmgr_msg_hdr_args *, uint8 *);
int __repmgr_msg_hdr_unmarshal(ENV *, __repmgr_msg_hdr_args *, uint8 *, size_t, uint8 **);
void __repmgr_msg_metadata_marshal(ENV *, __repmgr_msg_metadata_args *, uint8 *);
int __repmgr_msg_metadata_unmarshal(ENV *, __repmgr_msg_metadata_args *, uint8 *, size_t, uint8 **);
int __repmgr_membership_key_marshal(ENV *, __repmgr_membership_key_args *, uint8 *, size_t, size_t *);
int __repmgr_membership_key_unmarshal(ENV *, __repmgr_membership_key_args *, uint8 *, size_t, uint8 **);
void __repmgr_membership_data_marshal(ENV *, __repmgr_membership_data_args *, uint8 *);
int __repmgr_membership_data_unmarshal(ENV *, __repmgr_membership_data_args *, uint8 *, size_t, uint8 **);
void __repmgr_member_metadata_marshal(ENV *, __repmgr_member_metadata_args *, uint8 *);
int __repmgr_member_metadata_unmarshal(ENV *, __repmgr_member_metadata_args *, uint8 *, size_t, uint8 **);
int __repmgr_gm_fwd_marshal(ENV *, __repmgr_gm_fwd_args *, uint8 *, size_t, size_t *);
int __repmgr_gm_fwd_unmarshal(ENV *, __repmgr_gm_fwd_args *, uint8 *, size_t, uint8 **);
void __repmgr_membr_vers_marshal(ENV *, __repmgr_membr_vers_args *, uint8 *);
int __repmgr_membr_vers_unmarshal(ENV *, __repmgr_membr_vers_args *, uint8 *, size_t, uint8 **);
int __repmgr_site_info_marshal(ENV *, __repmgr_site_info_args *, uint8 *, size_t, size_t *);
int __repmgr_site_info_unmarshal(ENV *, __repmgr_site_info_args *, uint8 *, size_t, uint8 **);
void __repmgr_connect_reject_marshal(ENV *, __repmgr_connect_reject_args *, uint8 *);
int __repmgr_connect_reject_unmarshal(ENV *, __repmgr_connect_reject_args *, uint8 *, size_t, uint8 **);
int __repmgr_member_print(ENV *, DBT *, DB_LSN *, db_recops, void *);
int __repmgr_init_print(ENV *, DB_DISTAB *);
int __repmgr_init_election(ENV *, uint32);
int __repmgr_claim_victory(ENV *);
int __repmgr_turn_on_elections(ENV *);
int __repmgr_start(DB_ENV *, int, uint32);
int __repmgr_valid_config(ENV *, uint32);
int __repmgr_autostart(ENV *);
int __repmgr_start_selector(ENV *);
int __repmgr_close(ENV *);
int __repmgr_set_ack_policy(DB_ENV *, int);
int __repmgr_get_ack_policy(DB_ENV *, int *);
int __repmgr_env_create(ENV *, DB_REP *);
void __repmgr_env_destroy(ENV *, DB_REP *);
int __repmgr_stop_threads(ENV *);
int __repmgr_local_site(DB_ENV *, DB_SITE **);
int __repmgr_channel(DB_ENV *, int, DB_CHANNEL **, uint32);
int __repmgr_set_msg_dispatch(DB_ENV *, void (*)(DB_ENV *, DB_CHANNEL *, DBT *, uint32, uint32), uint32);
int __repmgr_send_msg(DB_CHANNEL *, DBT *, uint32, uint32);
int __repmgr_send_request(DB_CHANNEL *, DBT *, uint32, DBT *, db_timeout_t, uint32);
int __repmgr_send_response(DB_CHANNEL *, DBT *, uint32, uint32);
int __repmgr_channel_close(DB_CHANNEL *, uint32);
int __repmgr_channel_timeout(DB_CHANNEL *, db_timeout_t);
int __repmgr_send_request_inval(DB_CHANNEL *, DBT *, uint32, DBT *, db_timeout_t, uint32);
int __repmgr_channel_close_inval(DB_CHANNEL *, uint32);
int __repmgr_channel_timeout_inval(DB_CHANNEL *, db_timeout_t);
int __repmgr_join_group(ENV *);
int __repmgr_site(DB_ENV *, const char *, uint, DB_SITE **, uint32);
int __repmgr_site_by_eid(DB_ENV *, int, DB_SITE **);
int __repmgr_get_site_address(DB_SITE *, const char **, uint *);
int __repmgr_get_eid(DB_SITE *, int *);
int __repmgr_get_config(DB_SITE *, uint32, uint32 *);
int __repmgr_site_config(DB_SITE *, uint32, uint32);
int __repmgr_site_close(DB_SITE *);
void *__repmgr_msg_thread(void *);
int __repmgr_send_err_resp(ENV *, CHANNEL *, int);
int __repmgr_handle_event(ENV *, uint32, void *);
int __repmgr_update_membership(ENV *, DB_THREAD_INFO *, int, uint32);
int __repmgr_set_gm_version(ENV *, DB_THREAD_INFO *, DB_TXN *, uint32);
int __repmgr_setup_gmdb_op(ENV *, DB_THREAD_INFO *, DB_TXN **, uint32);
int __repmgr_cleanup_gmdb_op(ENV *, int);
int __repmgr_hold_master_role(ENV *, REPMGR_CONNECTION *);
int __repmgr_rlse_master_role(ENV *);
void __repmgr_set_sites(ENV *);
int __repmgr_connect(ENV *, repmgr_netaddr_t *, REPMGR_CONNECTION **, int *);
int __repmgr_send(DB_ENV *, const DBT *, const DBT *, const DB_LSN *, int, uint32);
int __repmgr_sync_siteaddr(ENV *);
int __repmgr_send_broadcast(ENV *, uint, const DBT *, const DBT *, uint *, uint *);
int __repmgr_send_one(ENV *, REPMGR_CONNECTION *, uint, const DBT *, const DBT *, db_timeout_t);
int __repmgr_send_many(ENV *, REPMGR_CONNECTION *, REPMGR_IOVECS *, db_timeout_t);
int __repmgr_send_own_msg(ENV *, REPMGR_CONNECTION *, uint32, uint8 *, uint32);
int __repmgr_write_iovecs(ENV *, REPMGR_CONNECTION *, REPMGR_IOVECS *, size_t *);
int __repmgr_bust_connection(ENV *, REPMGR_CONNECTION *);
int __repmgr_disable_connection(ENV *, REPMGR_CONNECTION *);
int __repmgr_cleanup_defunct(ENV *, REPMGR_CONNECTION *);
int __repmgr_close_connection(ENV *, REPMGR_CONNECTION *);
int __repmgr_decr_conn_ref(ENV *, REPMGR_CONNECTION *);
int __repmgr_destroy_conn(ENV *, REPMGR_CONNECTION *);
int __repmgr_pack_netaddr(ENV *, const char *, uint, repmgr_netaddr_t *);
int __repmgr_getaddr(ENV *, const char *, uint, int, ADDRINFO **);
int __repmgr_listen(ENV *);
int __repmgr_net_close(ENV *);
void __repmgr_net_destroy(ENV *, DB_REP *);
int __repmgr_thread_start(ENV *, REPMGR_RUNNABLE *);
int __repmgr_thread_join(REPMGR_RUNNABLE *);
int __repmgr_set_nonblock_conn(REPMGR_CONNECTION *);
int __repmgr_set_nonblocking(socket_t);
int __repmgr_wake_waiters(ENV *, waiter_t *);
int __repmgr_await_cond(ENV *, PREDICATE, void *, db_timeout_t, waiter_t *);
int __repmgr_await_gmdbop(ENV *);
void __repmgr_compute_wait_deadline(ENV*, struct timespec *, db_timeout_t);
int __repmgr_await_drain(ENV *, REPMGR_CONNECTION *, db_timeout_t);
int __repmgr_alloc_cond(cond_var_t *);
int __repmgr_free_cond(cond_var_t *);
void __repmgr_env_create_pf(DB_REP *);
int __repmgr_create_mutex_pf(mgr_mutex_t *);
int __repmgr_destroy_mutex_pf(mgr_mutex_t *);
int __repmgr_init(ENV *);
int __repmgr_deinit(ENV *);
int __repmgr_init_waiters(ENV *, waiter_t *);
int __repmgr_destroy_waiters(ENV *, waiter_t *);
int __repmgr_lock_mutex(mgr_mutex_t *);
int __repmgr_unlock_mutex(mgr_mutex_t *);
int __repmgr_signal(cond_var_t *);
int __repmgr_wake_msngers(ENV*, uint);
int __repmgr_wake_main_thread(ENV*);
int __repmgr_writev(socket_t, db_iovec_t *, int, size_t *);
int __repmgr_readv(socket_t, db_iovec_t *, int, size_t *);
int __repmgr_select_loop(ENV *);
int __repmgr_queue_destroy(ENV *);
int __repmgr_queue_get(ENV *, REPMGR_MESSAGE **, REPMGR_RUNNABLE *);
int __repmgr_queue_put(ENV *, REPMGR_MESSAGE *);
int __repmgr_queue_size(ENV *);
int __repmgr_member_recover(ENV *, DBT *, DB_LSN *, db_recops, void *);
void *__repmgr_select_thread(void *);
int __repmgr_bow_out(ENV *);
int __repmgr_accept(ENV *);
int __repmgr_compute_timeout(ENV *, db_timespec *);
REPMGR_CONNECTION *__repmgr_master_connection(ENV *);
int __repmgr_check_timeouts(ENV *);
int __repmgr_first_try_connections(ENV *);
int __repmgr_send_v1_handshake(ENV *, REPMGR_CONNECTION *, void *, size_t);
int __repmgr_read_from_site(ENV *, REPMGR_CONNECTION *);
int __repmgr_read_conn(REPMGR_CONNECTION *);
int __repmgr_prepare_simple_input(ENV *, REPMGR_CONNECTION *, __repmgr_msg_hdr_args *);
int __repmgr_send_handshake(ENV *, REPMGR_CONNECTION *, void *, size_t, uint32);
int __repmgr_find_version_info(ENV *, REPMGR_CONNECTION *, DBT *);
int __repmgr_write_some(ENV *, REPMGR_CONNECTION *);
int __repmgr_stat_pp(DB_ENV *, DB_REPMGR_STAT **, uint32);
int __repmgr_stat_print_pp(DB_ENV *, uint32);
int __repmgr_stat_print(ENV *, uint32);
int __repmgr_site_list(DB_ENV *, uint *, DB_REPMGR_SITE **);
#ifndef HAVE_REPLICATION_THREADS
	int __repmgr_close(ENV *);
#endif
#ifndef HAVE_REPLICATION_THREADS
	int __repmgr_get_ack_policy(DB_ENV *, int *);
#endif
#ifndef HAVE_REPLICATION_THREADS
	int __repmgr_set_ack_policy(DB_ENV *, int);
#endif
#ifndef HAVE_REPLICATION_THREADS
	int __repmgr_site(DB_ENV *, const char *, uint, DB_SITE **, uint32);
#endif
#ifndef HAVE_REPLICATION_THREADS
	int __repmgr_site_by_eid(DB_ENV *, int, DB_SITE **);
#endif
#ifndef HAVE_REPLICATION_THREADS
	int __repmgr_local_site(DB_ENV *, DB_SITE **);
#endif
#ifndef HAVE_REPLICATION_THREADS
	int __repmgr_site_list(DB_ENV *, uint *, DB_REPMGR_SITE **);
#endif
#ifndef HAVE_REPLICATION_THREADS
	int __repmgr_start(DB_ENV *, int, uint32);
#endif
#ifndef HAVE_REPLICATION_THREADS
	int __repmgr_stat_pp(DB_ENV *, DB_REPMGR_STAT **, uint32);
#endif
#ifndef HAVE_REPLICATION_THREADS
	int __repmgr_stat_print_pp(DB_ENV *, uint32);
#endif
#ifndef HAVE_REPLICATION_THREADS
	int __repmgr_handle_event(ENV *, uint32, void *);
#endif
#ifndef HAVE_REPLICATION_THREADS
	int __repmgr_channel(DB_ENV *, int, DB_CHANNEL **, uint32);
#endif
#ifndef HAVE_REPLICATION_THREADS
	int __repmgr_set_msg_dispatch(DB_ENV *, void (*)(DB_ENV *, DB_CHANNEL *, DBT *, uint32, uint32), uint32);
#endif
#ifndef HAVE_REPLICATION_THREADS
	int __repmgr_init_recover(ENV *, DB_DISTAB *);
#endif
int __repmgr_schedule_connection_attempt(ENV *, uint, int);
void __repmgr_reset_for_reading(REPMGR_CONNECTION *);
int __repmgr_new_connection(ENV *, REPMGR_CONNECTION **, socket_t, int);
int __repmgr_new_site(ENV *, REPMGR_SITE**, const char *, uint);
int __repmgr_create_mutex(ENV *, mgr_mutex_t **);
int __repmgr_destroy_mutex(ENV *, mgr_mutex_t *);
void __repmgr_cleanup_netaddr(ENV *, repmgr_netaddr_t *);
void __repmgr_iovec_init(REPMGR_IOVECS *);
void __repmgr_add_buffer(REPMGR_IOVECS *, void *, size_t);
void __repmgr_add_dbt(REPMGR_IOVECS *, const DBT *);
int __repmgr_update_consumed(REPMGR_IOVECS *, size_t);
int __repmgr_prepare_my_addr(ENV *, DBT *);
int __repmgr_get_nsites(ENV *, uint32 *);
int __repmgr_thread_failure(ENV *, int);
char *__repmgr_format_eid_loc(DB_REP *, REPMGR_CONNECTION *, char *);
char *__repmgr_format_site_loc(REPMGR_SITE *, char *);
char *__repmgr_format_addr_loc(repmgr_netaddr_t *, char *);
int __repmgr_repstart(ENV *, uint32);
int __repmgr_become_master(ENV *);
int __repmgr_each_connection(ENV *, CONNECTION_ACTION, void *, int);
int __repmgr_open(ENV *, void *);
int __repmgr_join(ENV *, void *);
int __repmgr_env_refresh(ENV *env);
int __repmgr_share_netaddrs(ENV *, void *, uint, uint);
int __repmgr_copy_in_added_sites(ENV *);
int __repmgr_init_new_sites(ENV *, uint, uint);
int __repmgr_failchk(ENV *);
int __repmgr_master_is_known(ENV *);
int __repmgr_stable_lsn(ENV *, DB_LSN *);
int __repmgr_send_sync_msg(ENV *, REPMGR_CONNECTION *, uint32, uint8 *, uint32);
int __repmgr_marshal_member_list(ENV *, uint8 **, size_t *);
int __repmgr_refresh_membership(ENV *, uint8 *, size_t);
int __repmgr_reload_gmdb(ENV *);
int __repmgr_gmdb_version_cmp(ENV *, uint32, uint32);
int __repmgr_init_save(ENV *, DBT *);
int __repmgr_init_restore(ENV *, DBT *);
int __repmgr_defer_op(ENV *, uint32);
void __repmgr_fire_conn_err_event(ENV *, REPMGR_CONNECTION *, int);
void __repmgr_print_conn_err(ENV *, repmgr_netaddr_t *, int);
int __repmgr_become_client(ENV *);
REPMGR_SITE *__repmgr_lookup_site(ENV *, const char *, uint);
int __repmgr_find_site(ENV *, const char *, uint, int *);
int __repmgr_set_membership(ENV *, const char *, uint, uint32);
int __repmgr_bcast_parm_refresh(ENV *);
int __repmgr_chg_prio(ENV *, uint32, uint32);
int __repmgr_bcast_own_msg(ENV *, uint32, uint8 *, size_t);

#if defined(__cplusplus)
}
#endif
#endif /* !_repmgr_ext_h_ */
