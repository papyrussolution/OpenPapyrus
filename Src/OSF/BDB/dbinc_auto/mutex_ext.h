/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_mutex_ext_h_
#define	_mutex_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __mutex_alloc(ENV *, int, uint32, db_mutex_t *);
int __mutex_alloc_int(ENV *, int, int, uint32, db_mutex_t *);
int __mutex_free(ENV *, db_mutex_t *);
int __mutex_free_int(ENV *, int, db_mutex_t *);
int __mutex_refresh(ENV *, db_mutex_t);
int __mut_failchk(ENV *);
int __db_fcntl_mutex_init(ENV *, db_mutex_t, uint32);
int __db_fcntl_mutex_lock(ENV *, db_mutex_t, db_timeout_t);
int __db_fcntl_mutex_trylock(ENV *, db_mutex_t);
int __db_fcntl_mutex_unlock(ENV *, db_mutex_t);
int __db_fcntl_mutex_destroy(ENV *, db_mutex_t);
int __mutex_alloc_pp(DB_ENV *, uint32, db_mutex_t *);
int __mutex_free_pp(DB_ENV *, db_mutex_t);
int __mutex_lock_pp(DB_ENV *, db_mutex_t);
int __mutex_unlock_pp(DB_ENV *, db_mutex_t);
int __mutex_get_align(DB_ENV *, uint32 *);
int __mutex_set_align(DB_ENV *, uint32);
int __mutex_get_increment(DB_ENV *, uint32 *);
int __mutex_set_increment(DB_ENV *, uint32);
int __mutex_get_init(DB_ENV *, uint32 *);
int __mutex_set_init(DB_ENV *, uint32);
int __mutex_get_max(DB_ENV *, uint32 *);
int __mutex_set_max(DB_ENV *, uint32);
int __mutex_get_tas_spins(DB_ENV *, uint32 *);
int __mutex_set_tas_spins(DB_ENV *, uint32);
#if !defined(HAVE_ATOMIC_SUPPORT) && defined(HAVE_MUTEX_SUPPORT)
atomic_value_t __atomic_inc(ENV *, db_atomic_t *);
#endif
#if !defined(HAVE_ATOMIC_SUPPORT) && defined(HAVE_MUTEX_SUPPORT)
atomic_value_t __atomic_dec(ENV *, db_atomic_t *);
#endif
#if !defined(HAVE_ATOMIC_SUPPORT) && defined(HAVE_MUTEX_SUPPORT)
int atomic_compare_exchange(ENV *, db_atomic_t *, atomic_value_t, atomic_value_t);
#endif
int __db_pthread_mutex_init(ENV *, db_mutex_t, uint32);
#ifndef HAVE_MUTEX_HYBRID
int __db_pthread_mutex_lock(ENV *, db_mutex_t, db_timeout_t);
#endif
#if defined(HAVE_SHARED_LATCHES)
int __db_pthread_mutex_readlock(ENV *, db_mutex_t);
#endif
#ifdef HAVE_MUTEX_HYBRID
int __db_hybrid_mutex_suspend(ENV *, db_mutex_t, db_timespec *, int);
#endif
int __db_pthread_mutex_unlock(ENV *, db_mutex_t);
int __db_pthread_mutex_destroy(ENV *, db_mutex_t);
int __mutex_open(ENV *, int);
int __mutex_env_refresh(ENV *);
void __mutex_resource_return(ENV *, REGINFO *);
int __mutex_stat_pp(DB_ENV *, DB_MUTEX_STAT **, uint32);
int __mutex_stat_print_pp(DB_ENV *, uint32);
int __mutex_stat_print(ENV *, uint32);
void __mutex_print_debug_single(ENV *, const char *, db_mutex_t, uint32);
void __mutex_print_debug_stats(ENV *, DB_MSGBUF *, db_mutex_t, uint32);
void __mutex_set_wait_info(ENV *, db_mutex_t, uintmax_t *, uintmax_t *);
void __mutex_clear(ENV *, db_mutex_t);
int __db_tas_mutex_init(ENV *, db_mutex_t, uint32);
int __db_tas_mutex_lock(ENV *, db_mutex_t, db_timeout_t);
int __db_tas_mutex_trylock(ENV *, db_mutex_t);
#if defined(HAVE_SHARED_LATCHES)
int __db_tas_mutex_readlock(ENV *, db_mutex_t);
#endif
#if defined(HAVE_SHARED_LATCHES)
int __db_tas_mutex_tryreadlock(ENV *, db_mutex_t);
#endif
int __db_tas_mutex_unlock(ENV *, db_mutex_t);
int __db_tas_mutex_destroy(ENV *, db_mutex_t);
int __db_win32_mutex_init(ENV *, db_mutex_t, uint32);
int __db_win32_mutex_lock(ENV *, db_mutex_t, db_timeout_t);
int __db_win32_mutex_trylock(ENV *, db_mutex_t);
#if defined(HAVE_SHARED_LATCHES)
int __db_win32_mutex_readlock(ENV *, db_mutex_t);
#endif
#if defined(HAVE_SHARED_LATCHES)
int __db_win32_mutex_tryreadlock(ENV *, db_mutex_t);
#endif
int __db_win32_mutex_unlock(ENV *, db_mutex_t);
int __db_win32_mutex_destroy(ENV *, db_mutex_t);

#if defined(__cplusplus)
}
#endif
#endif /* !_mutex_ext_h_ */
