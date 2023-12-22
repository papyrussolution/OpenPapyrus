/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop
//#include <ngx_event.h>

static ngx_int_t ngx_select_init(ngx_cycle_t * cycle, ngx_msec_t timer);
static void ngx_select_done(ngx_cycle_t * cycle);
static ngx_int_t ngx_select_add_event(ngx_event_t * ev, ngx_int_t event, ngx_uint_t flags);
static ngx_int_t ngx_select_del_event(ngx_event_t * ev, ngx_int_t event, ngx_uint_t flags);
static ngx_int_t ngx_select_process_events(ngx_cycle_t * cycle, ngx_msec_t timer, ngx_uint_t flags);
static const char * ngx_select_init_conf(ngx_cycle_t * cycle, void * conf);

struct NgxWin32SelectModuleBlock {
	fd_set master_read_fd_set;
	fd_set master_write_fd_set;
	fd_set work_read_fd_set;
	fd_set work_write_fd_set;
	ngx_uint_t max_read;
	ngx_uint_t max_write;
	ngx_uint_t nevents;
	ngx_event_t ** event_index;
};

static NgxWin32SelectModuleBlock _ModulBlk;
/*
static fd_set master_read_fd_set;
static fd_set master_write_fd_set;
static fd_set work_read_fd_set;
static fd_set work_write_fd_set;
static ngx_uint_t max_read;
static ngx_uint_t max_write;
static ngx_uint_t nevents;
static ngx_event_t ** event_index;
*/

static ngx_str_t select_name = ngx_string("select");

static ngx_event_module_t ngx_select_module_ctx = {
	&select_name,
	NULL,                 // create configuration 
	ngx_select_init_conf, // init configuration 
	{
		ngx_select_add_event,      // F_Add()           add an event 
		ngx_select_del_event,      // F_Delete()        delete an event 
		ngx_select_add_event,      // enable()          enable an event 
		ngx_select_del_event,      // disable()         disable an event 
		NULL,                      // F_AddConn()       add an connection 
		NULL,                      // F_DelConn()       delete an connection 
		NULL,                      // F_Notify()        trigger a notify 
		ngx_select_process_events, // F_ProcessEvents() process the events 
		ngx_select_init,           // F_Init()          init the events 
		ngx_select_done            // F_Done()          done the events 
	}
};

ngx_module_t ngx_select_module = {
	NGX_MODULE_V1,
	&ngx_select_module_ctx,            /* module context */
	NULL,                              /* module directives */
	NGX_EVENT_MODULE,                  /* module type */
	NULL,                              /* init master */
	NULL,                              /* init module */
	NULL,                              /* init process */
	NULL,                              /* init thread */
	NULL,                              /* exit thread */
	NULL,                              /* exit process */
	NULL,                              /* exit master */
	NGX_MODULE_V1_PADDING
};

static ngx_int_t ngx_select_init(ngx_cycle_t * cycle, ngx_msec_t timer)
{
	if(_ModulBlk.event_index == NULL) {
		FD_ZERO(&_ModulBlk.master_read_fd_set);
		FD_ZERO(&_ModulBlk.master_write_fd_set);
		_ModulBlk.nevents = 0;
	}
	if(ngx_process >= NGX_PROCESS_WORKER || cycle->old_cycle == NULL || cycle->old_cycle->connection_n < cycle->connection_n) {
		ngx_event_t ** pp_index = (ngx_event_t **)ngx_alloc(sizeof(ngx_event_t *) * 2 * cycle->connection_n, cycle->log);
		if(pp_index == NULL) {
			return NGX_ERROR;
		}
		if(_ModulBlk.event_index) {
			memcpy(pp_index, _ModulBlk.event_index, sizeof(ngx_event_t *) * _ModulBlk.nevents);
			SAlloc::F(_ModulBlk.event_index);
		}
		_ModulBlk.event_index = pp_index;
	}
	ngx_io = ngx_os_io;
	ngx_event_actions = ngx_select_module_ctx.actions;
	ngx_event_flags = NGX_USE_LEVEL_EVENT;
	_ModulBlk.max_read = 0;
	_ModulBlk.max_write = 0;
	return NGX_OK;
}

static void ngx_select_done(ngx_cycle_t * cycle)
{
	SAlloc::F(_ModulBlk.event_index);
	_ModulBlk.event_index = NULL;
}

static ngx_int_t ngx_select_add_event(ngx_event_t * ev, ngx_int_t event, ngx_uint_t flags)
{
	ngx_connection_t * c = (ngx_connection_t *)ev->P_Data;
	ngx_log_debug2(NGX_LOG_DEBUG_EVENT, ev->log, 0, "select add event fd:%d ev:%i", c->fd, event);
	if(ev->index != NGX_INVALID_INDEX)
		ngx_log_error(NGX_LOG_ALERT, ev->log, 0, "select event fd:%d ev:%i is already set", c->fd, event);
	else {
		if((event == NGX_READ_EVENT && ev->write) || (event == NGX_WRITE_EVENT && !ev->write)) {
			ngx_log_error(NGX_LOG_ALERT, ev->log, 0, "invalid select %s event fd:%d ev:%i", ev->write ? "write" : "read", c->fd, event);
			return NGX_ERROR;
		}
		if((event == NGX_READ_EVENT && _ModulBlk.max_read >= FD_SETSIZE) || (event == NGX_WRITE_EVENT && _ModulBlk.max_write >= FD_SETSIZE)) {
			ngx_log_error(NGX_LOG_ERR, ev->log, 0, "maximum number of descriptors supported by select() is %d", FD_SETSIZE);
			return NGX_ERROR;
		}
		if(event == NGX_READ_EVENT) {
			FD_SET(c->fd, &_ModulBlk.master_read_fd_set);
			_ModulBlk.max_read++;
		}
		else if(event == NGX_WRITE_EVENT) {
			FD_SET(c->fd, &_ModulBlk.master_write_fd_set);
			_ModulBlk.max_write++;
		}
		ev->active = 1;
		_ModulBlk.event_index[_ModulBlk.nevents] = ev;
		ev->index = _ModulBlk.nevents;
		_ModulBlk.nevents++;
	}
	return NGX_OK;
}

static ngx_int_t ngx_select_del_event(ngx_event_t * ev, ngx_int_t event, ngx_uint_t flags)
{
	ngx_connection_t * c = (ngx_connection_t *)ev->P_Data;
	ev->active = 0;
	if(ev->index != NGX_INVALID_INDEX) {
		ngx_log_debug2(NGX_LOG_DEBUG_EVENT, ev->log, 0, "select del event fd:%d ev:%i", c->fd, event);
		if(event == NGX_READ_EVENT) {
			FD_CLR(c->fd, &_ModulBlk.master_read_fd_set);
			_ModulBlk.max_read--;
		}
		else if(event == NGX_WRITE_EVENT) {
			FD_CLR(c->fd, &_ModulBlk.master_write_fd_set);
			_ModulBlk.max_write--;
		}
		if(ev->index < --_ModulBlk.nevents) {
			ngx_event_t * e = _ModulBlk.event_index[_ModulBlk.nevents];
			_ModulBlk.event_index[ev->index] = e;
			e->index = ev->index;
		}
		ev->index = NGX_INVALID_INDEX;
	}
	return NGX_OK;
}

static void ngx_select_repair_fd_sets(ngx_cycle_t * cycle)
{
	int    n;
	uint   i;
	for(i = 0; i < _ModulBlk.master_read_fd_set.fd_count; i++) {
		ngx_socket_t s = _ModulBlk.master_read_fd_set.fd_array[i];
		socklen_t len = sizeof(int);
		if(getsockopt(s, SOL_SOCKET, SO_TYPE, (char *)&n, &len) == -1) {
			ngx_err_t err = ngx_socket_errno;
			ngx_log_error(NGX_LOG_ALERT, cycle->log, err, "invalid descriptor #%d in read fd_set", s);
			FD_CLR(s, &_ModulBlk.master_read_fd_set);
		}
	}
	for(i = 0; i < _ModulBlk.master_write_fd_set.fd_count; i++) {
		ngx_socket_t s = _ModulBlk.master_write_fd_set.fd_array[i];
		socklen_t len = sizeof(int);
		if(getsockopt(s, SOL_SOCKET, SO_TYPE, (char *)&n, &len) == -1) {
			ngx_err_t err = ngx_socket_errno;
			ngx_log_error(NGX_LOG_ALERT, cycle->log, err, "invalid descriptor #%d in write fd_set", s);
			FD_CLR(s, &_ModulBlk.master_write_fd_set);
		}
	}
}

static ngx_int_t ngx_select_process_events(ngx_cycle_t * cycle, ngx_msec_t timer, ngx_uint_t flags)
{
	int    ready;
	ngx_uint_t i, found;
	struct timeval tv, * tp;
#if (NGX_DEBUG)
	if(cycle->log->Level & NGX_LOG_DEBUG_ALL) {
		for(i = 0; i < _ModulBlk.nevents; i++) {
			ngx_event_t * ev = _ModulBlk.event_index[i];
			ngx_connection_t * c = (ngx_connection_t *)ev->P_Data;
			ngx_log_debug2(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "select event: fd:%d wr:%d", c->fd, ev->write);
		}
	}
#endif
	if(timer == NGX_TIMER_INFINITE) {
		tp = NULL;
	}
	else {
		tv.tv_sec = (long)(timer / 1000);
		tv.tv_usec = (long)((timer % 1000) * 1000);
		tp = &tv;
	}
	ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "select timer: %M", timer);
	_ModulBlk.work_read_fd_set = _ModulBlk.master_read_fd_set;
	_ModulBlk.work_write_fd_set = _ModulBlk.master_write_fd_set;
	if(_ModulBlk.max_read || _ModulBlk.max_write) {
		ready = select(0, &_ModulBlk.work_read_fd_set, &_ModulBlk.work_write_fd_set, NULL, tp);
	}
	else {
		// 
		// Winsock select() requires that at least one descriptor set must be
		// be non-null, and any non-null descriptor set must contain at least
		// one handle to a socket.  Otherwise select() returns WSAEINVAL.
		// 
		ngx_msleep(timer);
		ready = 0;
	}
	{
		const ngx_err_t err = (ready == -1) ? ngx_socket_errno : 0;
		if(flags & NGX_UPDATE_TIME) {
			ngx_time_update();
		}
		ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "select ready %d", ready);
		if(err) {
			ngx_log_error(NGX_LOG_ALERT, cycle->log, err, "select() failed");
			if(err == WSAENOTSOCK) {
				ngx_select_repair_fd_sets(cycle);
			}
			return NGX_ERROR;
		}
		else if(ready == 0) {
			if(timer != NGX_TIMER_INFINITE) {
				return NGX_OK;
			}
			else {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, 0, "select() returned no events without timeout");
				return NGX_ERROR;
			}
		}
		else {
			int    nready = 0;
			for(i = 0; i < _ModulBlk.nevents; i++) {
				ngx_event_t * ev = _ModulBlk.event_index[i];
				ngx_connection_t * c = (ngx_connection_t *)ev->P_Data;
				found = 0;
				if(ev->write) {
					if(FD_ISSET(c->fd, &_ModulBlk.work_write_fd_set)) {
						found = 1;
						ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "select write %d", c->fd);
					}
				}
				else {
					if(FD_ISSET(c->fd, &_ModulBlk.work_read_fd_set)) {
						found = 1;
						ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "select read %d", c->fd);
					}
				}
				if(found) {
					ev->ready = 1;
					ngx_queue_t * queue = ev->accept ? &ngx_posted_accept_events : &ngx_posted_events;
					ngx_post_event(ev, queue);
					nready++;
				}
			}
			if(ready != nready) {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, 0, "select ready != events: %d:%d", ready, nready);
				ngx_select_repair_fd_sets(cycle);
			}
			return NGX_OK;
		}
	}
}

static const char * ngx_select_init_conf(ngx_cycle_t * cycle, void * conf)
{
	ngx_event_conf_t * ecf = (ngx_event_conf_t*)ngx_event_get_conf(cycle->conf_ctx, ngx_event_core_module);
	if(ecf->use != ngx_select_module.ctx_index) {
		return NGX_CONF_OK;
	}
	return NGX_CONF_OK;
}
