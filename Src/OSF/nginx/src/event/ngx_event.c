/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop
//#include <ngx_event.h>

#define DEFAULT_CONNECTIONS  512

extern ngx_module_t ngx_kqueue_module;
extern ngx_module_t ngx_eventport_module;
extern ngx_module_t ngx_devpoll_module;
extern ngx_module_t ngx_epoll_module;
extern ngx_module_t ngx_select_module;

ngx_queue_t ngx_posted_accept_events; // @global Очередь событий, связанный с акцептом соединения
ngx_queue_t ngx_posted_events; // @global Основная очередь событий (минус ngx_posted_accept_events)

static ngx_uint_t ngx_timer_resolution; // @global
sig_atomic_t ngx_event_timer_alarm; // @global
static ngx_uint_t ngx_event_max_module; // @global
ngx_uint_t ngx_event_flags; // @global
ngx_event_actions_t ngx_event_actions; // @global
static ngx_atomic_t connection_counter = 1; // @global
ngx_atomic_t * ngx_connection_counter = &connection_counter; // @global
ngx_atomic_t * ngx_accept_mutex_ptr; // @global
ngx_shmtx_t ngx_accept_mutex; // @global
ngx_uint_t ngx_use_accept_mutex; // @global
ngx_uint_t ngx_accept_events; // @global
ngx_uint_t ngx_accept_mutex_held; // @global bool
ngx_msec_t ngx_accept_mutex_delay; // @global
ngx_int_t ngx_accept_disabled; // @global

#if (NGX_STAT_STUB)
	static ngx_atomic_t ngx_stat_accepted0;
	ngx_atomic_t * ngx_stat_accepted = &ngx_stat_accepted0;
	static ngx_atomic_t ngx_stat_handled0;
	ngx_atomic_t * ngx_stat_handled = &ngx_stat_handled0;
	static ngx_atomic_t ngx_stat_requests0;
	ngx_atomic_t * ngx_stat_requests = &ngx_stat_requests0;
	static ngx_atomic_t ngx_stat_active0;
	ngx_atomic_t * ngx_stat_active = &ngx_stat_active0;
	static ngx_atomic_t ngx_stat_reading0;
	ngx_atomic_t * ngx_stat_reading = &ngx_stat_reading0;
	static ngx_atomic_t ngx_stat_writing0;
	ngx_atomic_t * ngx_stat_writing = &ngx_stat_writing0;
	static ngx_atomic_t ngx_stat_waiting0;
	ngx_atomic_t * ngx_stat_waiting = &ngx_stat_waiting0;
#endif

static const char * ngx_event_init_conf(ngx_cycle_t * cycle, void * conf);
static ngx_int_t ngx_event_module_init(ngx_cycle_t * cycle);
static ngx_int_t ngx_event_process_init(ngx_cycle_t * cycle);
static const char * ngx_events_block(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static const char * ngx_event_connections(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static const char * ngx_event_use(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static const char * ngx_event_debug_connection(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static void * ngx_event_core_create_conf(ngx_cycle_t * cycle);
static const char * ngx_event_core_init_conf(ngx_cycle_t * cycle, void * conf);

static ngx_command_t ngx_events_commands[] = {
	{ ngx_string("events"), NGX_MAIN_CONF|NGX_CONF_BLOCK|NGX_CONF_NOARGS, ngx_events_block, 0, 0, NULL },
	ngx_null_command
};

static ngx_core_module_t ngx_events_module_ctx = { ngx_string("events"), NULL, ngx_event_init_conf };

ngx_module_t ngx_events_module = {
	NGX_MODULE_V1,
	&ngx_events_module_ctx,            /* module context */
	ngx_events_commands, /* module directives */
	NGX_CORE_MODULE,                   /* module type */
	NULL,                              /* init master */
	NULL,                              /* init module */
	NULL,                              /* init process */
	NULL,                              /* init thread */
	NULL,                              /* exit thread */
	NULL,                              /* exit process */
	NULL,                              /* exit master */
	NGX_MODULE_V1_PADDING
};

static ngx_str_t event_core_name = ngx_string("event_core");

static ngx_command_t ngx_event_core_commands[] = {
	{ ngx_string("worker_connections"), NGX_EVENT_CONF|NGX_CONF_TAKE1, ngx_event_connections,      0, 0, NULL },
	{ ngx_string("use"),                NGX_EVENT_CONF|NGX_CONF_TAKE1, ngx_event_use,              0, 0, NULL },
	{ ngx_string("multi_accept"),       NGX_EVENT_CONF|NGX_CONF_FLAG,  ngx_conf_set_flag_slot,     0, offsetof(ngx_event_conf_t, multi_accept), NULL },
	{ ngx_string("accept_mutex"),       NGX_EVENT_CONF|NGX_CONF_FLAG,  ngx_conf_set_flag_slot,     0, offsetof(ngx_event_conf_t, accept_mutex), NULL },
	{ ngx_string("accept_mutex_delay"), NGX_EVENT_CONF|NGX_CONF_TAKE1, ngx_conf_set_msec_slot,     0, offsetof(ngx_event_conf_t, accept_mutex_delay), NULL },
	{ ngx_string("debug_connection"),   NGX_EVENT_CONF|NGX_CONF_TAKE1, ngx_event_debug_connection, 0, 0, NULL },
	ngx_null_command
};

static ngx_event_module_t ngx_event_core_module_ctx = {
	&event_core_name,
	ngx_event_core_create_conf, // create configuration 
	ngx_event_core_init_conf,   // init configuration 
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

ngx_module_t ngx_event_core_module = {
	NGX_MODULE_V1,
	&ngx_event_core_module_ctx,        /* module context */
	ngx_event_core_commands,           /* module directives */
	NGX_EVENT_MODULE,                  /* module type */
	NULL,                              /* init master */
	ngx_event_module_init,             /* init module */
	ngx_event_process_init,            /* init process */
	NULL,                              /* init thread */
	NULL,                              /* exit thread */
	NULL,                              /* exit process */
	NULL,                              /* exit master */
	NGX_MODULE_V1_PADDING
};

static void ngx_event_process_posted(ngx_cycle_t * cycle, ngx_queue_t * posted)
{
	while(!ngx_queue_empty(posted)) {
		ngx_queue_t * q = ngx_queue_head(posted);
		ngx_event_t * ev = ngx_queue_data(q, ngx_event_t, queue);
		ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "posted event %p", ev);
		ngx_delete_posted_event(ev);
		ev->F_EvHandler(ev);
	}
}

void FASTCALL ngx_process_events_and_timers(ngx_cycle_t * pCycle)
{
	ngx_uint_t flags;
	ngx_msec_t timer;
	if(ngx_timer_resolution) {
		timer = NGX_TIMER_INFINITE;
		flags = 0;
	}
	else {
		timer = ngx_event_find_timer();
		flags = NGX_UPDATE_TIME;
#if (NGX_WIN32)
		// handle signals from master in case of network inactivity 
		if(timer == NGX_TIMER_INFINITE || timer > 500) {
			timer = 5/*500*/; // @sobolev Задержка уменьшена ради быстрой обработки цикла обслуживания делегированных запросов (see below)
		}
#endif
	}
	if(ngx_use_accept_mutex) {
		if(ngx_accept_disabled > 0) {
			ngx_accept_disabled--;
		}
		else {
			if(ngx_trylock_accept_mutex(pCycle) == NGX_ERROR) {
				return;
			}
			else if(ngx_accept_mutex_held) {
				flags |= NGX_POST_EVENTS;
			}
			else if(timer == NGX_TIMER_INFINITE || timer > ngx_accept_mutex_delay) {
				timer = ngx_accept_mutex_delay;
			}
		}
	}
	{
		ngx_msec_t delta = ngx_current_msec;
		{
			//
			// @sobolev {
			// Предварительная попытка втолкнуть обработчик уже готовых к отправке запросов
			// в основной цикл.
			// На текущий момент не до конца понятно как правильно сформировать ngx_event_t перед ngx_post_event.
			//
			// @v9.8.1 В первом приближении схема работает!
			//
			struct SB {
				static ngx_event_t & Push_NgxEvent()
				{
					ngx_event_t new_ev;
					MEMSZERO(new_ev);
					return PushRecycledObject <ngx_event_t, 1024> (new_ev);
				}
				static NgxReqResult & Push_NgxReqResult(NgxReqResult & rReqRes)
				{
					return PushRecycledObject <NgxReqResult, 1024> (rReqRes);
				}
				static void EventHandle(ngx_event_t * pEv)
				{
					NgxReqResult * p_req_res = (NgxReqResult *)pEv->P_Data;
					if(p_req_res) {
						ngx_http_send_header(p_req_res->P_Req);
						ngx_http_output_filter(p_req_res->P_Req, &p_req_res->Chain);
						ngx_http_finalize_request(p_req_res->P_Req, p_req_res->ReplyCode/*NGX_DONE*/);
						pEv->P_Data = 0;
					}
				}
			};
			NgxReqResult req_res;
			while(NgxPopRequestResult(&req_res) > 0) {
				NgxReqResult & r_req_res = SB::Push_NgxReqResult(req_res);
				ngx_event_t & r_ev = SB::Push_NgxEvent();
				MEMSZERO(r_ev);
				r_ev.P_Data = &r_req_res;
				r_ev.ready = 1;
				r_ev.log = pCycle->log;
				r_ev.F_EvHandler = SB::EventHandle;
				ngx_post_event(&r_ev, &ngx_posted_events);
			}
			// } @sobolev
		}
		// @sobolev (void)ngx_process_events(cycle, timer, flags);
		ngx_event_actions.F_ProcessEvents(pCycle, timer, flags); // @sobolev 
		delta = ngx_current_msec - delta;
		ngx_log_debug1(NGX_LOG_DEBUG_EVENT, pCycle->log, 0, "timer delta: %M", delta);
		ngx_event_process_posted(pCycle, &ngx_posted_accept_events);
		if(ngx_accept_mutex_held) {
			ngx_shmtx_unlock(&ngx_accept_mutex);
		}
		if(delta) {
			ngx_event_expire_timers();
		}
	}
	ngx_event_process_posted(pCycle, &ngx_posted_events);
}

ngx_int_t FASTCALL ngx_handle_read_event(ngx_event_t * rev, ngx_uint_t flags)
{
	ngx_int_t result = NGX_OK;
	if(ngx_event_flags & NGX_USE_CLEAR_EVENT) {
		// kqueue, epoll 
		if(!rev->active && !rev->ready) {
			THROW(ngx_add_event(rev, NGX_READ_EVENT, NGX_CLEAR_EVENT) != NGX_ERROR);
		}
	}
	else if(ngx_event_flags & NGX_USE_LEVEL_EVENT) {
		// select, poll, /dev/poll 
		if(!rev->active && !rev->ready) {
			THROW(ngx_add_event(rev, NGX_READ_EVENT, NGX_LEVEL_EVENT) != NGX_ERROR);
		}
		else if(rev->active && (rev->ready || (flags & NGX_CLOSE_EVENT))) {
			THROW(ngx_del_event(rev, NGX_READ_EVENT, NGX_LEVEL_EVENT | flags) != NGX_ERROR);
		}
	}
	else if(ngx_event_flags & NGX_USE_EVENTPORT_EVENT) {
		// event ports 
		if(!rev->active && !rev->ready) {
			THROW(ngx_add_event(rev, NGX_READ_EVENT, 0) != NGX_ERROR);
		}
		else if(rev->oneshot && !rev->ready) {
			THROW(ngx_del_event(rev, NGX_READ_EVENT, 0) != NGX_ERROR);
		}
	}
	// iocp 
	CATCH
		result = NGX_ERROR;
	ENDCATCH
	return result;
	//return NGX_OK;
}

ngx_int_t FASTCALL ngx_handle_write_event(ngx_event_t * wev, size_t lowat)
{
	ngx_int_t result = NGX_OK;
	if(lowat) {
		ngx_connection_t * c = (ngx_connection_t *)wev->P_Data;
		THROW(ngx_send_lowat(c, lowat) != NGX_ERROR);
	}
	//
	if(ngx_event_flags & NGX_USE_CLEAR_EVENT) {
		// kqueue, epoll 
		if(!wev->active && !wev->ready) {
			THROW(ngx_add_event(wev, NGX_WRITE_EVENT, NGX_CLEAR_EVENT | (lowat ? NGX_LOWAT_EVENT : 0)) != NGX_ERROR);
		}
	}
	else if(ngx_event_flags & NGX_USE_LEVEL_EVENT) {
		// select, poll, /dev/poll 
		if(!wev->active && !wev->ready) {
			THROW(ngx_add_event(wev, NGX_WRITE_EVENT, NGX_LEVEL_EVENT) != NGX_ERROR);
		}
		else if(wev->active && wev->ready) {
			THROW(ngx_del_event(wev, NGX_WRITE_EVENT, NGX_LEVEL_EVENT) != NGX_ERROR);
		}
	}
	else if(ngx_event_flags & NGX_USE_EVENTPORT_EVENT) {
		// event ports 
		if(!wev->active && !wev->ready) {
			THROW(ngx_add_event(wev, NGX_WRITE_EVENT, 0) != NGX_ERROR);
		}
		else if(wev->oneshot && wev->ready) {
			THROW(ngx_del_event(wev, NGX_WRITE_EVENT, 0) != NGX_ERROR);
		}
	}
	// iocp 
	CATCH
		result = NGX_ERROR;
	ENDCATCH
	return result;
	//return NGX_OK;
}

static const char * ngx_event_init_conf(ngx_cycle_t * cycle, void * conf)
{
	if(ngx_get_conf(cycle->conf_ctx, ngx_events_module) == NULL) {
		ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "no \"events\" section in configuration");
		return NGX_CONF_ERROR;
	}
	else
		return NGX_CONF_OK;
}

static ngx_int_t ngx_event_module_init(ngx_cycle_t * cycle)
{
	u_char * shared;
	size_t size, cl;
	ngx_shm_t shm;
	ngx_time_t * tp;
	ngx_core_conf_t * ccf;
	void *** ppp_cf = ngx_get_conf(cycle->conf_ctx, ngx_events_module);
	ngx_event_conf_t * ecf = (ngx_event_conf_t*)(*ppp_cf)[ngx_event_core_module.ctx_index];
	if(!ngx_test_config__ && ngx_process <= NGX_PROCESS_MASTER) {
		ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "using the \"%s\" event method", ecf->name);
	}
	ccf = (ngx_core_conf_t*)ngx_get_conf(cycle->conf_ctx, ngx_core_module);
	ngx_timer_resolution = ccf->timer_resolution;
#if !(NGX_WIN32)
	{
		ngx_int_t limit;
		struct rlimit rlmt;
		if(getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
			ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno, "getrlimit(RLIMIT_NOFILE) failed, ignored");
		}
		else {
			if(ecf->connections > (ngx_uint_t)rlmt.rlim_cur && (ccf->rlimit_nofile == NGX_CONF_UNSET || ecf->connections > (ngx_uint_t)ccf->rlimit_nofile)) {
				limit = (ccf->rlimit_nofile == NGX_CONF_UNSET) ? (ngx_int_t)rlmt.rlim_cur : ccf->rlimit_nofile;
				ngx_log_error(NGX_LOG_WARN, cycle->log, 0, "%ui worker_connections exceed open file resource limit: %i", ecf->connections, limit);
			}
		}
	}
#endif /* !(NGX_WIN32) */
	if(ccf->master == 0) {
		return NGX_OK;
	}
	if(ngx_accept_mutex_ptr) {
		return NGX_OK;
	}
	/* cl should be equal to or greater than cache line size */
	cl = 128;
	size = cl        /* ngx_accept_mutex */
	    + cl         /* ngx_connection_counter */
	    + cl; /* ngx_temp_number */
#if (NGX_STAT_STUB)
	size += cl       /* ngx_stat_accepted */
	    + cl         /* ngx_stat_handled */
	    + cl         /* ngx_stat_requests */
	    + cl         /* ngx_stat_active */
	    + cl         /* ngx_stat_reading */
	    + cl         /* ngx_stat_writing */
	    + cl; /* ngx_stat_waiting */
#endif
	shm.size = size;
	ngx_str_set(&shm.name, "nginx_shared_zone");
	shm.log = cycle->log;
	if(ngx_shm_alloc(&shm) != NGX_OK) {
		return NGX_ERROR;
	}
	shared = shm.addr;
	ngx_accept_mutex_ptr = (ngx_atomic_t*)shared;
	ngx_accept_mutex.spin = (ngx_uint_t)-1;
	if(ngx_shmtx_create(&ngx_accept_mutex, (ngx_shmtx_sh_t*)shared, cycle->lock_file.data) != NGX_OK) {
		return NGX_ERROR;
	}
	ngx_connection_counter = (ngx_atomic_t*)(shared + 1 * cl);
	(void)ngx_atomic_cmp_set(ngx_connection_counter, 0, 1);
	ngx_log_debug2(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "counter: %p, %uA", ngx_connection_counter, *ngx_connection_counter);
	ngx_temp_number = (ngx_atomic_t*)(shared + 2 * cl);
	tp = ngx_timeofday();
	ngx_random_number = (tp->msec << 16) + ngx_pid;
#if (NGX_STAT_STUB)
	ngx_stat_accepted = (ngx_atomic_t*)(shared + 3 * cl);
	ngx_stat_handled = (ngx_atomic_t*)(shared + 4 * cl);
	ngx_stat_requests = (ngx_atomic_t*)(shared + 5 * cl);
	ngx_stat_active = (ngx_atomic_t*)(shared + 6 * cl);
	ngx_stat_reading = (ngx_atomic_t*)(shared + 7 * cl);
	ngx_stat_writing = (ngx_atomic_t*)(shared + 8 * cl);
	ngx_stat_waiting = (ngx_atomic_t*)(shared + 9 * cl);
#endif
	return NGX_OK;
}

#if !(NGX_WIN32)

static void ngx_timer_signal_handler(int signo)
{
	ngx_event_timer_alarm = 1;
#if 1
	ngx_log_debug0(NGX_LOG_DEBUG_EVENT, ngx_cycle->log, 0, "timer signal");
#endif
}

#endif

static ngx_int_t ngx_event_process_init(ngx_cycle_t * cycle)
{
	ngx_uint_t m, i;
	ngx_event_t * rev, * wev;
	ngx_listening_t  * ls;
	ngx_connection_t * c, * next, * old;
	ngx_event_module_t * module;
	ngx_core_conf_t  * ccf = (ngx_core_conf_t*)ngx_get_conf(cycle->conf_ctx, ngx_core_module);
	ngx_event_conf_t * ecf = (ngx_event_conf_t*)ngx_event_get_conf(cycle->conf_ctx, ngx_event_core_module);
	if(ccf->master && ccf->worker_processes > 1 && ecf->accept_mutex) {
		ngx_use_accept_mutex = 1;
		ngx_accept_mutex_held = 0;
		ngx_accept_mutex_delay = ecf->accept_mutex_delay;
	}
	else {
		ngx_use_accept_mutex = 0;
	}
#if (NGX_WIN32)
	// 
	// disable accept mutex on win32 as it may cause deadlock if
	// grabbed by a process which can't accept connections
	// 
	ngx_use_accept_mutex = 0;
#endif
	ngx_queue_init(&ngx_posted_accept_events);
	ngx_queue_init(&ngx_posted_events);
	if(ngx_event_timer_init(cycle->log) == NGX_ERROR) {
		return NGX_ERROR;
	}
	for(m = 0; cycle->modules[m]; m++) {
		if(cycle->modules[m]->type == NGX_EVENT_MODULE) {
			if(cycle->modules[m]->ctx_index == ecf->use) {
				module = (ngx_event_module_t*)cycle->modules[m]->ctx;
				if(module->actions.F_Init(cycle, ngx_timer_resolution) != NGX_OK) {
					exit(2); // fatal 
				}
				break;
			}
		}
	}
#if !(NGX_WIN32)
	if(ngx_timer_resolution && !(ngx_event_flags & NGX_USE_TIMER_EVENT)) {
		struct sigaction sa;
		struct itimerval itv;
		memzero(&sa, sizeof(struct sigaction));
		sa.sa_handler = ngx_timer_signal_handler;
		sigemptyset(&sa.sa_mask);
		if(sigaction(SIGALRM, &sa, NULL) == -1) {
			ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno, "sigaction(SIGALRM) failed");
			return NGX_ERROR;
		}
		itv.it_interval.tv_sec = ngx_timer_resolution / 1000;
		itv.it_interval.tv_usec = (ngx_timer_resolution % 1000) * 1000;
		itv.it_value.tv_sec = ngx_timer_resolution / 1000;
		itv.it_value.tv_usec = (ngx_timer_resolution % 1000 ) * 1000;
		if(setitimer(ITIMER_REAL, &itv, NULL) == -1) {
			ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno, "setitimer() failed");
		}
	}
	if(ngx_event_flags & NGX_USE_FD_EVENT) {
		struct rlimit rlmt;
		if(getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
			ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno, "getrlimit(RLIMIT_NOFILE) failed");
			return NGX_ERROR;
		}
		cycle->files_n = (ngx_uint_t)rlmt.rlim_cur;
		cycle->files = ngx_calloc(sizeof(ngx_connection_t *) * cycle->files_n, cycle->log);
		if(cycle->files == NULL) {
			return NGX_ERROR;
		}
	}
#else
	if(ngx_timer_resolution && !(ngx_event_flags & NGX_USE_TIMER_EVENT)) {
		ngx_log_error(NGX_LOG_WARN, cycle->log, 0, "the \"timer_resolution\" directive is not supported with the configured event method, ignored");
		ngx_timer_resolution = 0;
	}
#endif
	cycle->connections = (ngx_connection_t *)ngx_alloc(sizeof(ngx_connection_t) * cycle->connection_n, cycle->log);
	if(cycle->connections == NULL) {
		return NGX_ERROR;
	}
	c = cycle->connections;
	cycle->read_events = (ngx_event_t*)ngx_alloc(sizeof(ngx_event_t) * cycle->connection_n, cycle->log);
	if(cycle->read_events == NULL) {
		return NGX_ERROR;
	}
	rev = cycle->read_events;
	for(i = 0; i < cycle->connection_n; i++) {
		rev[i].closed = 1;
		rev[i].instance = 1;
	}
	cycle->write_events = (ngx_event_t*)ngx_alloc(sizeof(ngx_event_t) * cycle->connection_n, cycle->log);
	if(cycle->write_events == NULL) {
		return NGX_ERROR;
	}
	wev = cycle->write_events;
	for(i = 0; i < cycle->connection_n; i++) {
		wev[i].closed = 1;
	}
	i = cycle->connection_n;
	next = NULL;
	do {
		i--;
		c[i].data = next;
		c[i].P_EvRd = &cycle->read_events[i];
		c[i].P_EvWr = &cycle->write_events[i];
		c[i].fd = (ngx_socket_t)-1;
		next = &c[i];
	} while(i);
	cycle->free_connections = next;
	cycle->free_connection_n = cycle->connection_n;
	// for each listening socket 
	ls = (ngx_listening_t*)cycle->listening.elts;
	for(i = 0; i < cycle->listening.nelts; i++) {
#if (NGX_HAVE_REUSEPORT)
		if(ls[i].reuseport && ls[i].worker != ngx_worker) {
			continue;
		}
#endif
		c = ngx_get_connection(ls[i].fd, cycle->log);
		if(!c) {
			return NGX_ERROR;
		}
		c->type = ls[i].type;
		c->log = &ls[i].log;
		c->listening = &ls[i];
		ls[i].connection = c;
		rev = c->P_EvRd;
		rev->log = c->log;
		rev->accept = 1;
#if (NGX_HAVE_DEFERRED_ACCEPT)
		rev->deferred_accept = ls[i].deferred_accept;
#endif
		if(!(ngx_event_flags & NGX_USE_IOCP_EVENT)) {
			if(ls[i].previous) {
				// 
				// delete the old accept events that were bound to the old cycle read events array
				// 
				old = ls[i].previous->connection;
				if(ngx_del_event(old->P_EvRd, NGX_READ_EVENT, NGX_CLOSE_EVENT) == NGX_ERROR) {
					return NGX_ERROR;
				}
				old->fd = (ngx_socket_t)-1;
			}
		}
#if (NGX_WIN32)
		if(ngx_event_flags & NGX_USE_IOCP_EVENT) {
			ngx_iocp_conf_t * iocpcf;
			rev->F_EvHandler = ngx_event_acceptex;
			if(ngx_use_accept_mutex) {
				continue;
			}
			if(ngx_add_event(rev, 0, NGX_IOCP_ACCEPT) == NGX_ERROR) {
				return NGX_ERROR;
			}
			ls[i].log.handler = ngx_acceptex_log_error;
			iocpcf = (ngx_iocp_conf_t *)ngx_event_get_conf(cycle->conf_ctx, ngx_iocp_module);
			if(ngx_event_post_acceptex(&ls[i], iocpcf->post_acceptex) == NGX_ERROR) {
				return NGX_ERROR;
			}
		}
		else {
			rev->F_EvHandler = ngx_event_accept;
			if(ngx_use_accept_mutex) {
				continue;
			}
			if(ngx_add_event(rev, NGX_READ_EVENT, 0) == NGX_ERROR) {
				return NGX_ERROR;
			}
		}
#else
		rev->handler = (c->type == SOCK_STREAM) ? ngx_event_accept : ngx_event_recvmsg;
#if (NGX_HAVE_REUSEPORT)
		if(ls[i].reuseport) {
			if(ngx_add_event(rev, NGX_READ_EVENT, 0) == NGX_ERROR) {
				return NGX_ERROR;
			}
			continue;
		}
#endif
		if(ngx_use_accept_mutex) {
			continue;
		}
#if (NGX_HAVE_EPOLLEXCLUSIVE)
		if((ngx_event_flags & NGX_USE_EPOLL_EVENT) && ccf->worker_processes > 1) {
			if(ngx_add_event(rev, NGX_READ_EVENT, NGX_EXCLUSIVE_EVENT) == NGX_ERROR) {
				return NGX_ERROR;
			}
			continue;
		}
#endif
		if(ngx_add_event(rev, NGX_READ_EVENT, 0) == NGX_ERROR) {
			return NGX_ERROR;
		}
#endif
	}
	return NGX_OK;
}

ngx_int_t ngx_send_lowat(ngx_connection_t * c, size_t lowat)
{
	int sndlowat;
#if (NGX_HAVE_LOWAT_EVENT)
	if(ngx_event_flags & NGX_USE_KQUEUE_EVENT) {
		c->write->available = lowat;
		return NGX_OK;
	}
#endif
	if(lowat == 0 || c->sndlowat) {
		return NGX_OK;
	}
	sndlowat = (int)lowat;
	if(setsockopt(c->fd, SOL_SOCKET, SO_SNDLOWAT, (const char *)&sndlowat, sizeof(int)) == -1) {
		ngx_connection_error(c, ngx_socket_errno, "setsockopt(SO_SNDLOWAT) failed");
		return NGX_ERROR;
	}
	c->sndlowat = 1;
	return NGX_OK;
}

static const char * ngx_events_block(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	const char * rv;
	void *** ctx;
	ngx_uint_t i;
	ngx_conf_t pcf;
	if(*(void **)conf) {
		return "is duplicate";
	}
	else {
		// count the number of the event modules and set up their indices 
		ngx_event_max_module = ngx_count_modules(cf->cycle, NGX_EVENT_MODULE);
		ctx = (void***)ngx_pcalloc(cf->pool, sizeof(void *));
		if(!ctx) {
			return NGX_CONF_ERROR;
		}
		*ctx = (void **)ngx_pcalloc(cf->pool, ngx_event_max_module * sizeof(void *));
		if(*ctx == NULL) {
			return NGX_CONF_ERROR;
		}
		*(void **)conf = ctx;
		for(i = 0; cf->cycle->modules[i]; i++) {
			if(cf->cycle->modules[i]->type == NGX_EVENT_MODULE) {
				ngx_event_module_t * m = (ngx_event_module_t*)cf->cycle->modules[i]->ctx;
				if(m->create_conf) {
					(*ctx)[cf->cycle->modules[i]->ctx_index] = m->create_conf(cf->cycle);
					if((*ctx)[cf->cycle->modules[i]->ctx_index] == NULL) {
						return NGX_CONF_ERROR;
					}
				}
			}
		}
		pcf = *cf;
		cf->ctx = ctx;
		cf->module_type = NGX_EVENT_MODULE;
		cf->cmd_type = NGX_EVENT_CONF;
		rv = ngx_conf_parse(cf, NULL);
		*cf = pcf;
		if(rv != NGX_CONF_OK) {
			return rv;
		}
		for(i = 0; cf->cycle->modules[i]; i++) {
			if(cf->cycle->modules[i]->type == NGX_EVENT_MODULE) {
				ngx_event_module_t * m = (ngx_event_module_t*)cf->cycle->modules[i]->ctx;
				if(m->F_InitConf) {
					rv = m->F_InitConf(cf->cycle, (*ctx)[cf->cycle->modules[i]->ctx_index]);
					if(rv != NGX_CONF_OK) {
						return rv;
					}
				}
			}
		}
		return NGX_CONF_OK;
	}
}

static const char * ngx_event_connections(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	ngx_event_conf_t * ecf = (ngx_event_conf_t *)conf;
	if(ecf->connections != NGX_CONF_UNSET_UINT) {
		return "is duplicate";
	}
	else {
		ngx_str_t * value = static_cast<ngx_str_t *>(cf->args->elts);
		ecf->connections = ngx_atoi(value[1].data, value[1].len);
		if(ecf->connections == (ngx_uint_t)NGX_ERROR) {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid number \"%V\"", &value[1]);
			return NGX_CONF_ERROR;
		}
		else {
			cf->cycle->connection_n = ecf->connections;
			return NGX_CONF_OK;
		}
	}
}

static const char * ngx_event_use(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	ngx_event_conf_t  * ecf = (ngx_event_conf_t *)conf;
	if(ecf->use != NGX_CONF_UNSET_UINT) {
		return "is duplicate";
	}
	else {
		ngx_str_t * value = static_cast<ngx_str_t *>(cf->args->elts);
		ngx_event_conf_t * old_ecf = 0;
		if(cf->cycle->old_cycle->conf_ctx)
			old_ecf = (ngx_event_conf_t *)ngx_event_get_conf(cf->cycle->old_cycle->conf_ctx, ngx_event_core_module);
		for(ngx_int_t m = 0; cf->cycle->modules[m]; m++) {
			if(cf->cycle->modules[m]->type == NGX_EVENT_MODULE) {
				ngx_event_module_t * module = (ngx_event_module_t*)cf->cycle->modules[m]->ctx;
				if(module->name->len == value[1].len) {
					if(sstreq(module->name->data, value[1].data)) {
						ecf->use = cf->cycle->modules[m]->ctx_index;
						ecf->name = module->name->data;
						if(ngx_process == NGX_PROCESS_SINGLE && old_ecf && old_ecf->use != ecf->use) {
							ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
								"when the server runs without a master process the \"%V\" event type must be the same as "
								"in previous configuration - \"%s\" and it cannot be changed on the fly, "
								"to change it you need to stop server and start it again",
								&value[1], old_ecf->name);
							return NGX_CONF_ERROR;
						}
						else
							return NGX_CONF_OK;
					}
				}
			}
		}
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid event type \"%V\"", &value[1]);
		return NGX_CONF_ERROR;
	}
}

static const char * ngx_event_debug_connection(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
#if (NGX_DEBUG)
	ngx_event_conf_t * ecf = (ngx_event_conf_t *)conf;
	ngx_int_t rc;
	ngx_str_t * value;
	ngx_url_t u;
	ngx_cidr_t c, * cidr;
	ngx_uint_t i;
	struct sockaddr_in * sin;
#if (NGX_HAVE_INET6)
	struct sockaddr_in6  * sin6;
#endif
	value = static_cast<ngx_str_t *>(cf->args->elts);
#if (NGX_HAVE_UNIX_DOMAIN)
	if(sstreq(value[1].data, "unix:")) {
		cidr = ngx_array_push(&ecf->debug_connection);
		if(cidr == NULL) {
			return NGX_CONF_ERROR;
		}
		cidr->family = AF_UNIX;
		return NGX_CONF_OK;
	}
#endif
	rc = ngx_ptocidr(&value[1], &c);
	if(rc != NGX_ERROR) {
		if(rc == NGX_DONE) {
			ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "low address bits of %V are meaningless", &value[1]);
		}
		cidr = (ngx_cidr_t *)ngx_array_push(&ecf->debug_connection);
		if(cidr == NULL) {
			return NGX_CONF_ERROR;
		}
		*cidr = c;
		return NGX_CONF_OK;
	}
	memzero(&u, sizeof(ngx_url_t));
	u.host = value[1];
	if(ngx_inet_resolve_host(cf->pool, &u) != NGX_OK) {
		if(u.err) {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s in debug_connection \"%V\"", u.err, &u.host);
		}
		return NGX_CONF_ERROR;
	}
	cidr = (ngx_cidr_t *)ngx_array_push_n(&ecf->debug_connection, u.naddrs);
	if(cidr == NULL) {
		return NGX_CONF_ERROR;
	}
	memzero(cidr, u.naddrs * sizeof(ngx_cidr_t));
	for(i = 0; i < u.naddrs; i++) {
		cidr[i].family = u.addrs[i].sockaddr->sa_family;
		switch(cidr[i].family) {
#if (NGX_HAVE_INET6)
			case AF_INET6:
			    sin6 = (struct sockaddr_in6*)u.addrs[i].sockaddr;
			    cidr[i].u.in6.addr = sin6->sin6_addr;
			    memset(cidr[i].u.in6.mask.s6_addr, 0xff, 16);
			    break;
#endif
			default: /* AF_INET */
			    sin = (struct sockaddr_in*)u.addrs[i].sockaddr;
			    cidr[i].u.in.addr = sin->sin_addr.s_addr;
			    cidr[i].u.in.mask = 0xffffffff;
			    break;
		}
	}
#else
	ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "\"debug_connection\" is ignored, you need to rebuild nginx using --with-debug option to enable it");
#endif
	return NGX_CONF_OK;
}

static void * ngx_event_core_create_conf(ngx_cycle_t * cycle)
{
	ngx_event_conf_t * ecf = (ngx_event_conf_t*)ngx_palloc(cycle->pool, sizeof(ngx_event_conf_t));
	if(ecf) {
		ecf->connections = NGX_CONF_UNSET_UINT;
		ecf->use = NGX_CONF_UNSET_UINT;
		ecf->multi_accept = NGX_CONF_UNSET;
		ecf->accept_mutex = NGX_CONF_UNSET;
		ecf->accept_mutex_delay = NGX_CONF_UNSET_MSEC;
		ecf->name = reinterpret_cast<u_char *>(NGX_CONF_UNSET);
#if (NGX_DEBUG)
		if(ngx_array_init(&ecf->debug_connection, cycle->pool, 4, sizeof(ngx_cidr_t)) == NGX_ERROR) {
			return NULL;
		}
#endif
	}
	return ecf;
}

static const char * ngx_event_core_init_conf(ngx_cycle_t * cycle, void * conf)
{
	ngx_event_conf_t  * ecf = (ngx_event_conf_t *)conf;
#if (NGX_HAVE_EPOLL) && !(NGX_TEST_BUILD_EPOLL)
	int fd;
#endif
	ngx_int_t i;
	ngx_module_t * module = 0;
	ngx_event_module_t  * event_module;
#if (NGX_HAVE_EPOLL) && !(NGX_TEST_BUILD_EPOLL)
	fd = epoll_create(100);
	if(fd != -1) {
		(void)close(fd);
		module = &ngx_epoll_module;
	}
	else if(ngx_errno != NGX_ENOSYS) {
		module = &ngx_epoll_module;
	}
#endif
#if (NGX_HAVE_DEVPOLL) && !(NGX_TEST_BUILD_DEVPOLL)
	module = &ngx_devpoll_module;
#endif
#if (NGX_HAVE_KQUEUE)
	module = &ngx_kqueue_module;
#endif
#if (NGX_HAVE_SELECT)
	if(module == NULL) {
		module = &ngx_select_module;
	}
#endif
	if(module == NULL) {
		for(i = 0; cycle->modules[i]; i++) {
			if(cycle->modules[i]->type == NGX_EVENT_MODULE) {
				event_module = (ngx_event_module_t*)cycle->modules[i]->ctx;
				if(!sstreq(event_module->name->data, event_core_name.data)) {
					module = cycle->modules[i];
					break;
				}
			}
		}
	}
	if(module == NULL) {
		ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "no events module found");
		return NGX_CONF_ERROR;
	}
	ngx_conf_init_uint_value(ecf->connections, DEFAULT_CONNECTIONS);
	cycle->connection_n = ecf->connections;
	ngx_conf_init_uint_value(ecf->use, module->ctx_index);
	event_module = (ngx_event_module_t*)module->ctx;
	ngx_conf_init_ptr_value(ecf->name, event_module->name->data);
	ngx_conf_init_value(ecf->multi_accept, 0);
	ngx_conf_init_value(ecf->accept_mutex, 0);
	ngx_conf_init_msec_value(ecf->accept_mutex_delay, 500);
	return NGX_CONF_OK;
}
