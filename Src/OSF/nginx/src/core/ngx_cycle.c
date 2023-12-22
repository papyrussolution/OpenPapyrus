/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop
//#include <ngx_event.h>

static void ngx_destroy_cycle_pools(ngx_conf_t * conf);
static ngx_int_t ngx_init_zone_pool(ngx_cycle_t * cycle, ngx_shm_zone_t * shm_zone);
static ngx_int_t ngx_test_lockfile(uchar * file, ngx_log_t * log);
static void ngx_clean_old_cycles(ngx_event_t * ev);
static void ngx_shutdown_timer_handler(ngx_event_t * ev);

volatile ngx_cycle_t * ngx_cycle; // @global
ngx_array_t ngx_old_cycles; // @global
static ngx_pool_t * ngx_temp_pool; // @global
static ngx_event_t ngx_cleaner_event; // @global
static ngx_event_t ngx_shutdown_event; // @global
ngx_uint_t ngx_test_config__; // @global
ngx_uint_t ngx_dump_config__; // @global
//ngx_uint_t ngx_quiet_mode;

/* STUB NAME */
static ngx_connection_t dumb; // @global
/* STUB */

ngx_cycle_t * ngx_init_cycle(ngx_cycle_t * old_cycle, const NgxStartUpOptions & rO)
{
	ngx_cycle_t * cycle = 0;
	//ngx_cycle_t ** old;
	//void * rv;
	//char ** senv;
	//ngx_uint_t i;
	//ngx_uint_t n;
	ngx_conf_t conf;
	//ngx_shm_zone_t * shm_zone;
	//ngx_shm_zone_t * oshm_zone;
	//ngx_list_part_t * part;
	//ngx_list_part_t * opart;
	//ngx_open_file_t * file;
	//ngx_listening_t * ls;
	//ngx_listening_t * nls;
	//ngx_core_conf_t * ccf;
	char hostname[NGX_MAXHOSTNAMELEN];
	ngx_timezone_update();
	// force localtime update with a new timezone 
	ngx_time_t * tp = ngx_timeofday();
	tp->sec = 0;
	ngx_time_update();
	ngx_log_t * p_log = old_cycle->log;
	ngx_pool_t * p_pool = ngx_create_pool(NGX_CYCLE_POOL_SIZE, p_log);
	if(p_pool == NULL) {
		return NULL;
	}
	p_pool->log = p_log;
	cycle = (ngx_cycle_t *)ngx_pcalloc(p_pool, sizeof(ngx_cycle_t));
	if(cycle == NULL) {
		ngx_destroy_pool(p_pool);
		return NULL;
	}
	cycle->pool = p_pool;
	cycle->log = p_log;
	cycle->old_cycle = old_cycle;
	cycle->conf_prefix.len = old_cycle->conf_prefix.len;
	cycle->conf_prefix.data = ngx_pstrdup(p_pool, &old_cycle->conf_prefix);
	if(cycle->conf_prefix.data == NULL) {
		ngx_destroy_pool(p_pool);
		return NULL;
	}
	cycle->prefix.len = old_cycle->prefix.len;
	cycle->prefix.data = ngx_pstrdup(p_pool, &old_cycle->prefix);
	if(cycle->prefix.data == NULL) {
		ngx_destroy_pool(p_pool);
		return NULL;
	}
	cycle->conf_file.len = old_cycle->conf_file.len;
	cycle->conf_file.data = (uchar *)ngx_pnalloc(p_pool, old_cycle->conf_file.len + 1);
	if(cycle->conf_file.data == NULL) {
		ngx_destroy_pool(p_pool);
		return NULL;
	}
	ngx_cpystrn(cycle->conf_file.data, old_cycle->conf_file.data, old_cycle->conf_file.len + 1);
	cycle->conf_param.len = old_cycle->conf_param.len;
	cycle->conf_param.data = ngx_pstrdup(p_pool, &old_cycle->conf_param);
	if(cycle->conf_param.data == NULL) {
		ngx_destroy_pool(p_pool);
		return NULL;
	}
	{
		const ngx_uint_t n = old_cycle->paths.nelts ? old_cycle->paths.nelts : 10;
		if(ngx_array_init(&cycle->paths, p_pool, n, sizeof(ngx_path_t *)) != NGX_OK) {
			ngx_destroy_pool(p_pool);
			return NULL;
		}
		memzero(cycle->paths.elts, n * sizeof(ngx_path_t *));
		if(ngx_array_init(&cycle->config_dump, p_pool, 1, sizeof(ngx_conf_dump_t)) != NGX_OK) {
			ngx_destroy_pool(p_pool);
			return NULL;
		}
	}
	{
		ngx_uint_t n = 20;
		ngx_rbtree_init(&cycle->config_dump_rbtree, &cycle->config_dump_sentinel, ngx_str_rbtree_insert_value);
		if(old_cycle->open_files.part.nelts) {
			n = old_cycle->open_files.part.nelts;
			for(ngx_list_part_t * part = old_cycle->open_files.part.next; part; part = part->next) {
				n += part->nelts;
			}
		}
		else {
			n = 20;
		}
		if(ngx_list_init(&cycle->open_files, p_pool, n, sizeof(ngx_open_file_t)) != NGX_OK) {
			ngx_destroy_pool(p_pool);
			return NULL;
		}
	}
	{
		ngx_uint_t n = 1;
		if(old_cycle->shared_memory.part.nelts) {
			n = old_cycle->shared_memory.part.nelts;
			for(ngx_list_part_t * part = old_cycle->shared_memory.part.next; part; part = part->next) {
				n += part->nelts;
			}
		}
		else {
			n = 1;
		}
		if(ngx_list_init(&cycle->shared_memory, p_pool, n, sizeof(ngx_shm_zone_t)) != NGX_OK) {
			ngx_destroy_pool(p_pool);
			return NULL;
		}
	}
	{
		const ngx_uint_t n = old_cycle->listening.nelts ? old_cycle->listening.nelts : 10;
		if(ngx_array_init(&cycle->listening, p_pool, n, sizeof(ngx_listening_t)) != NGX_OK) {
			ngx_destroy_pool(p_pool);
			return NULL;
		}
		memzero(cycle->listening.elts, n * sizeof(ngx_listening_t));
	}
	{
		ngx_queue_init(&cycle->reusable_connections_queue);
		cycle->conf_ctx = (void ****)ngx_pcalloc(p_pool, ngx_max_module * sizeof(void *));
		if(cycle->conf_ctx == NULL) {
			ngx_destroy_pool(p_pool);
			return NULL;
		}
	}
	if(gethostname(hostname, NGX_MAXHOSTNAMELEN) == -1) {
		ngx_log_error(NGX_LOG_EMERG, p_log, ngx_errno, "gethostname() failed");
		ngx_destroy_pool(p_pool);
		return NULL;
	}
	//
	// on Linux gethostname() silently truncates name that does not fit 
	//
	hostname[NGX_MAXHOSTNAMELEN-1] = '\0';
	cycle->hostname.len = ngx_strlen(hostname);
	cycle->hostname.data = (uchar *)ngx_pnalloc(p_pool, cycle->hostname.len);
	if(cycle->hostname.data == NULL) {
		ngx_destroy_pool(p_pool);
		return NULL;
	}
	ngx_strlow(cycle->hostname.data, (uchar *)hostname, cycle->hostname.len);
	{
		if(ngx_cycle_modules(cycle) != NGX_OK) {
			ngx_destroy_pool(p_pool);
			return NULL;
		}
		for(ngx_uint_t i = 0; cycle->modules[i]; i++) {
			if(cycle->modules[i]->type == NGX_CORE_MODULE) {
				ngx_core_module_t * p_module = (ngx_core_module_t *)cycle->modules[i]->ctx;
				if(p_module->create_conf) {
					void * rv = p_module->create_conf(cycle);
					if(rv == NULL) {
						ngx_destroy_pool(p_pool);
						return NULL;
					}
					cycle->conf_ctx[cycle->modules[i]->index] = (void ***)rv;
				}
			}
		}
	}
	{
		char ** pp_senv = environ;
		memzero(&conf, sizeof(ngx_conf_t));
		// STUB: init array ? 
		conf.args = ngx_array_create(p_pool, 10, sizeof(ngx_str_t));
		if(conf.args == NULL) {
			ngx_destroy_pool(p_pool);
			return NULL;
		}
		conf.temp_pool = ngx_create_pool(NGX_CYCLE_POOL_SIZE, p_log);
		if(conf.temp_pool == NULL) {
			ngx_destroy_pool(p_pool);
			return NULL;
		}
		conf.ctx = cycle->conf_ctx;
		conf.cycle = cycle;
		conf.pool = p_pool;
		conf.log = p_log;
		conf.module_type = NGX_CORE_MODULE;
		conf.cmd_type = NGX_MAIN_CONF;
#if 0
		log->log_level = NGX_LOG_DEBUG_ALL;
#endif
		if(ngx_conf_param(&conf) != NGX_CONF_OK) {
			environ = pp_senv;
			ngx_destroy_cycle_pools(&conf);
			return NULL;
		}
		if(ngx_conf_parse(&conf, &cycle->conf_file) != NGX_CONF_OK) {
			environ = pp_senv;
			ngx_destroy_cycle_pools(&conf);
			return NULL;
		}
		//if(ngx_test_config && !ngx_quiet_mode) {
		if(rO.Flags & rO.fTestConf && !(rO.Flags & rO.fQuietMode)) {
			ngx_log_stderr(0, "the configuration file %s syntax is ok", cycle->conf_file.data);
		}
		{
			for(ngx_uint_t i = 0; cycle->modules[i]; i++) {
				if(cycle->modules[i]->type == NGX_CORE_MODULE) {
					ngx_core_module_t * module = (ngx_core_module_t *)cycle->modules[i]->ctx;
					if(module->F_InitConf) {
						if(module->F_InitConf(cycle, cycle->conf_ctx[cycle->modules[i]->index]) == NGX_CONF_ERROR) {
							environ = pp_senv;
							ngx_destroy_cycle_pools(&conf);
							return NULL;
						}
					}
				}
			}
		}
	}
	if(ngx_process == NGX_PROCESS_SIGNALLER) {
		return cycle;
	}
	{
		ngx_core_conf_t * ccf = (ngx_core_conf_t*)ngx_get_conf(cycle->conf_ctx, ngx_core_module);
		if(/*ngx_test_config*/rO.Flags & rO.fTestConf) {
			if(ngx_create_pidfile(&ccf->pid, p_log, rO) != NGX_OK) {
				goto failed;
			}
		}
		else if(!ngx_is_init_cycle(old_cycle)) {
			// 
			// we do not create the pid file in the first ngx_init_cycle() call
			// because we need to write the demonized process pid
			// 
			ngx_core_conf_t * old_ccf = (ngx_core_conf_t*)ngx_get_conf(old_cycle->conf_ctx, ngx_core_module);
			if(ccf->pid.len != old_ccf->pid.len || !sstreq(ccf->pid.data, old_ccf->pid.data)) {
				// new pid file name 
				if(ngx_create_pidfile(&ccf->pid, p_log, rO) != NGX_OK) {
					goto failed;
				}
				ngx_delete_pidfile(old_cycle);
			}
		}
		if(ngx_test_lockfile(cycle->lock_file.data, p_log) != NGX_OK) {
			goto failed;
		}
		if(ngx_create_paths(cycle, ccf->user) != NGX_OK) {
			goto failed;
		}
		if(ngx_log_open_default(cycle) != NGX_OK) {
			goto failed;
		}
	}
	{
		// open the new files 
		ngx_list_part_t * part = &cycle->open_files.part;
		ngx_open_file_t * file = (ngx_open_file_t *)part->elts;
		for(ngx_uint_t i = 0; /* void */; i++) {
			if(i >= part->nelts) {
				if(part->next == NULL) {
					break;
				}
				part = part->next;
				file = (ngx_open_file_t *)part->elts;
				i = 0;
			}
			if(file[i].name.len) {
				file[i].fd = ngx_open_file(file[i].name.data, NGX_FILE_APPEND, NGX_FILE_CREATE_OR_OPEN, NGX_FILE_DEFAULT_ACCESS);
				ngx_log_debug3(NGX_LOG_DEBUG_CORE, p_log, 0, "log: %p %d \"%s\"", &file[i], file[i].fd, file[i].name.data);
				if(file[i].fd == NGX_INVALID_FILE) {
					ngx_log_error(NGX_LOG_EMERG, p_log, ngx_errno, ngx_open_file_n " \"%s\" failed", file[i].name.data);
					goto failed;
				}
#if !(NGX_WIN32)
				if(fcntl(file[i].fd, F_SETFD, FD_CLOEXEC) == -1) {
					ngx_log_error(NGX_LOG_EMERG, p_log, ngx_errno, "fcntl(FD_CLOEXEC) \"%s\" failed", file[i].name.data);
					goto failed;
				}
#endif
			}
		}
	}
	cycle->log = &cycle->new_log;
	p_pool->log = &cycle->new_log;
	{
		//
		// create shared memory 
		//
		ngx_list_part_t * part = &cycle->shared_memory.part;
		ngx_shm_zone_t * shm_zone = (ngx_shm_zone_t *)part->elts;
		for(ngx_uint_t i = 0; /* void */; i++) {
			if(i >= part->nelts) {
				if(part->next == NULL) {
					break;
				}
				part = part->next;
				shm_zone = (ngx_shm_zone_t *)part->elts;
				i = 0;
			}
			if(shm_zone[i].shm.size == 0) {
				ngx_log_error(NGX_LOG_EMERG, p_log, 0, "zero size shared memory zone \"%V\"", &shm_zone[i].shm.name);
				goto failed;
			}
			shm_zone[i].shm.log = cycle->log;
			{
				ngx_list_part_t * opart = &old_cycle->shared_memory.part;
				ngx_shm_zone_t * oshm_zone = (ngx_shm_zone_t *)opart->elts;
				for(ngx_uint_t n = 0; /* void */; n++) {
					if(n >= opart->nelts) {
						if(opart->next == NULL) {
							break;
						}
						opart = opart->next;
						oshm_zone = (ngx_shm_zone_t *)opart->elts;
						n = 0;
					}
					if(shm_zone[i].shm.name.len == oshm_zone[n].shm.name.len && ngx_strncmp(shm_zone[i].shm.name.data, oshm_zone[n].shm.name.data, shm_zone[i].shm.name.len) == 0) {
						if(shm_zone[i].tag == oshm_zone[n].tag && shm_zone[i].shm.size == oshm_zone[n].shm.size && !shm_zone[i].noreuse) {
							shm_zone[i].shm.addr = oshm_zone[n].shm.addr;
#if (NGX_WIN32)
							shm_zone[i].shm.handle = oshm_zone[n].shm.handle;
#endif
							if(shm_zone[i].F_Init(&shm_zone[i], oshm_zone[n].data) != NGX_OK) {
								goto failed;
							}
							goto shm_zone_found;
						}
						ngx_shm_free(&oshm_zone[n].shm);
						break;
					}
				}
			}
			if(ngx_shm_alloc(&shm_zone[i].shm) != NGX_OK) {
				goto failed;
			}
			if(ngx_init_zone_pool(cycle, &shm_zone[i]) != NGX_OK) {
				goto failed;
			}
			if(shm_zone[i].F_Init(&shm_zone[i], NULL) != NGX_OK) {
				goto failed;
			}
shm_zone_found:
			continue;
		}
	}
	//
	// handle the listening sockets 
	//
	if(old_cycle->listening.nelts) {
		ngx_listening_t * ls = (ngx_listening_t *)old_cycle->listening.elts;
		{
			for(ngx_uint_t i = 0; i < old_cycle->listening.nelts; i++) {
				ls[i].remain = 0;
			}
		}
		ngx_listening_t * nls = (ngx_listening_t *)cycle->listening.elts;
		for(ngx_uint_t n = 0; n < cycle->listening.nelts; n++) {
			for(ngx_uint_t i = 0; i < old_cycle->listening.nelts; i++) {
				if(!ls[i].ignore && !ls[i].remain && ls[i].type == nls[n].type) {
					if(ngx_cmp_sockaddr(nls[n].sockaddr, nls[n].socklen, ls[i].sockaddr, ls[i].socklen, 1) == NGX_OK) {
						nls[n].fd = ls[i].fd;
						nls[n].previous = &ls[i];
						ls[i].remain = 1;
						if(ls[i].backlog != nls[n].backlog) {
							nls[n].listen = 1;
						}
#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
						// 
						// FreeBSD, except the most recent versions, could not remove accept filter
						// 
						nls[n].deferred_accept = ls[i].deferred_accept;
						if(ls[i].accept_filter && nls[n].accept_filter) {
							if(!sstreq(ls[i].accept_filter, nls[n].accept_filter)) {
								nls[n].delete_deferred = 1;
								nls[n].add_deferred = 1;
							}
						}
						else if(ls[i].accept_filter) {
							nls[n].delete_deferred = 1;
						}
						else if(nls[n].accept_filter) {
							nls[n].add_deferred = 1;
						}
#endif
#if (NGX_HAVE_DEFERRED_ACCEPT && defined TCP_DEFER_ACCEPT)
						if(ls[i].deferred_accept && !nls[n].deferred_accept) {
							nls[n].delete_deferred = 1;
						}
						else if(ls[i].deferred_accept != nls[n].deferred_accept) {
							nls[n].add_deferred = 1;
						}
#endif
#if (NGX_HAVE_REUSEPORT)
						if(nls[n].reuseport && !ls[i].reuseport) {
							nls[n].add_reuseport = 1;
						}
#endif
						break;
					}
				}
			}
			if(nls[n].fd == (ngx_socket_t)-1) {
				nls[n].open = 1;
#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
				if(nls[n].accept_filter) {
					nls[n].add_deferred = 1;
				}
#endif
#if (NGX_HAVE_DEFERRED_ACCEPT && defined TCP_DEFER_ACCEPT)
				if(nls[n].deferred_accept) {
					nls[n].add_deferred = 1;
				}
#endif
			}
		}
	}
	else {
		ngx_listening_t * ls = (ngx_listening_t *)cycle->listening.elts;
		for(ngx_uint_t i = 0; i < cycle->listening.nelts; i++) {
			ls[i].open = 1;
#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
			if(ls[i].accept_filter) {
				ls[i].add_deferred = 1;
			}
#endif
#if (NGX_HAVE_DEFERRED_ACCEPT && defined TCP_DEFER_ACCEPT)
			if(ls[i].deferred_accept) {
				ls[i].add_deferred = 1;
			}
#endif
		}
	}
	if(ngx_open_listening_sockets(cycle, rO) != NGX_OK) {
		goto failed;
	}
	if(!/*ngx_test_config*/(rO.Flags & rO.fTestConf)) {
		ngx_configure_listening_sockets(cycle);
	}
	// commit the new cycle configuration 
	if(!ngx_use_stderr) {
		(void)ngx_log_redirect_stderr(cycle);
	}
	p_pool->log = cycle->log;
	if(ngx_init_modules(cycle) != NGX_OK) {
		exit(1); // fatal 
	}
	{
		// close and delete stuff that lefts from an old cycle 
		// free the unnecessary shared memory 
		ngx_list_part_t * opart = &old_cycle->shared_memory.part;
		ngx_shm_zone_t * oshm_zone = (ngx_shm_zone_t *)opart->elts;
		for(ngx_uint_t i = 0; /* void */; i++) {
			if(i >= opart->nelts) {
				if(opart->next == NULL) {
					goto old_shm_zone_done;
				}
				opart = opart->next;
				oshm_zone = (ngx_shm_zone_t *)opart->elts;
				i = 0;
			}
			{
				ngx_list_part_t * part = &cycle->shared_memory.part;
				ngx_shm_zone_t * shm_zone = (ngx_shm_zone_t *)part->elts;
				for(ngx_uint_t n = 0; /* void */; n++) {
					if(n >= part->nelts) {
						if(part->next == NULL) {
							break;
						}
						part = part->next;
						shm_zone = (ngx_shm_zone_t *)part->elts;
						n = 0;
					}
					if(oshm_zone[i].shm.name.len == shm_zone[n].shm.name.len && ngx_strncmp(oshm_zone[i].shm.name.data, shm_zone[n].shm.name.data, oshm_zone[i].shm.name.len) == 0) {
						goto live_shm_zone;
					}
				}
			}
			ngx_shm_free(&oshm_zone[i].shm);
live_shm_zone:
			continue;
		}
	}
old_shm_zone_done:
	{
		//
		// close the unnecessary listening sockets 
		//
		ngx_listening_t * ls = (ngx_listening_t *)old_cycle->listening.elts;
		for(ngx_uint_t i = 0; i < old_cycle->listening.nelts; i++) {
			if(!ls[i].remain && ls[i].fd != (ngx_socket_t)-1) {
				if(ngx_close_socket(ls[i].fd) == -1) {
					ngx_log_error(NGX_LOG_EMERG, p_log, ngx_socket_errno, ngx_close_socket_n " listening socket on %V failed", &ls[i].addr_text);
				}
#if (NGX_HAVE_UNIX_DOMAIN)
				if(ls[i].sockaddr->sa_family == AF_UNIX) {
					uchar * name = ls[i].addr_text.data + sizeof("unix:") - 1;
					ngx_log_error(NGX_LOG_WARN, cycle->log, 0, "deleting socket %s", name);
					if(ngx_delete_file(name) == NGX_FILE_ERROR) {
						ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_socket_errno, ngx_delete_file_n " %s failed", name);
					}
				}
#endif
			}
		}
	}
	{
		//
		// close the unnecessary open files 
		//
		ngx_list_part_t * part = &old_cycle->open_files.part;
		ngx_open_file_t * file = (ngx_open_file_t *)part->elts;
		for(ngx_uint_t i = 0; /* void */; i++) {
			if(i >= part->nelts) {
				if(part->next == NULL) {
					break;
				}
				part = part->next;
				file = (ngx_open_file_t *)part->elts;
				i = 0;
			}
			if(file[i].fd != NGX_INVALID_FILE && file[i].fd != ngx_stderr) {
				if(ngx_close_file(file[i].fd) == NGX_FILE_ERROR) {
					ngx_log_error(NGX_LOG_EMERG, p_log, ngx_errno, ngx_close_file_n " \"%s\" failed", file[i].name.data);
				}
			}
		}
	}
	ngx_destroy_pool(conf.temp_pool);
	if(ngx_process == NGX_PROCESS_MASTER || ngx_is_init_cycle(old_cycle)) {
		ngx_destroy_pool(old_cycle->pool);
		cycle->old_cycle = NULL;
	}
	else {
		if(ngx_temp_pool == NULL) {
			ngx_temp_pool = ngx_create_pool(128, cycle->log);
			if(ngx_temp_pool == NULL) {
				ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "could not create ngx_temp_pool");
				exit(1);
			}
			{
				const ngx_uint_t n = 10;
				if(ngx_array_init(&ngx_old_cycles, ngx_temp_pool, n, sizeof(ngx_cycle_t *)) != NGX_OK) {
					exit(1);
				}
				memzero(ngx_old_cycles.elts, n * sizeof(ngx_cycle_t *));
				ngx_cleaner_event.F_EvHandler = ngx_clean_old_cycles;
				ngx_cleaner_event.log = cycle->log;
				ngx_cleaner_event.P_Data = &dumb;
				dumb.fd = (ngx_socket_t)-1;
			}
		}
		ngx_temp_pool->log = cycle->log;
		{
			ngx_cycle_t ** pp_old = (ngx_cycle_t **)ngx_array_push(&ngx_old_cycles);
			if(pp_old == NULL) {
				exit(1);
			}
			*pp_old = old_cycle;
		}
		if(!ngx_cleaner_event.timer_set) {
			ngx_add_timer(&ngx_cleaner_event, 30000);
			ngx_cleaner_event.timer_set = 1;
		}
	}
	return cycle;
failed:
	if(!ngx_is_init_cycle(old_cycle)) {
		ngx_core_conf_t * old_ccf = (ngx_core_conf_t*)ngx_get_conf(old_cycle->conf_ctx, ngx_core_module);
		if(old_ccf->environment) {
			environ = old_ccf->environment;
		}
	}
	{
		//
		// rollback the new cycle configuration 
		//
		ngx_list_part_t * part = &cycle->open_files.part;
		ngx_open_file_t * file = (ngx_open_file_t *)part->elts;
		for(ngx_uint_t i = 0; /* void */; i++) {
			if(i >= part->nelts) {
				if(part->next == NULL) {
					break;
				}
				part = part->next;
				file = (ngx_open_file_t *)part->elts;
				i = 0;
			}
			if(!oneof2(file[i].fd, NGX_INVALID_FILE, ngx_stderr)) {
				if(ngx_close_file(file[i].fd) == NGX_FILE_ERROR)
					ngx_log_error(NGX_LOG_EMERG, p_log, ngx_errno, ngx_close_file_n " \"%s\" failed", file[i].name.data);
			}
		}
	}
	if(!/*ngx_test_config*/(rO.Flags & rO.fTestConf)) {
		ngx_listening_t * ls = (ngx_listening_t *)cycle->listening.elts;
		for(ngx_uint_t i = 0; i < cycle->listening.nelts; i++) {
			if(ls[i].fd != (ngx_socket_t)-1 && ls[i].open) {
				if(ngx_close_socket(ls[i].fd) == -1)
					ngx_log_error(NGX_LOG_EMERG, p_log, ngx_socket_errno, ngx_close_socket_n " %V failed", &ls[i].addr_text);
			}
		}
	}
	ngx_destroy_cycle_pools(&conf);
	return NULL;
}

static void ngx_destroy_cycle_pools(ngx_conf_t * conf)
{
	ngx_destroy_pool(conf->temp_pool);
	ngx_destroy_pool(conf->pool);
}

static ngx_int_t ngx_init_zone_pool(ngx_cycle_t * cycle, ngx_shm_zone_t * zn)
{
	uchar * file;
	ngx_slab_pool_t  * sp = (ngx_slab_pool_t*)zn->shm.addr;
	if(zn->shm.exists) {
		if(sp == sp->addr) {
			return NGX_OK;
		}
#if (NGX_WIN32)
		/* remap at the required address */
		if(ngx_shm_remap(&zn->shm, (uchar *)sp->addr) != NGX_OK) {
			return NGX_ERROR;
		}
		sp = (ngx_slab_pool_t*)zn->shm.addr;
		if(sp == sp->addr) {
			return NGX_OK;
		}
#endif
		ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "shared zone \"%V\" has no equal addresses: %p vs %p", &zn->shm.name, sp->addr, sp);
		return NGX_ERROR;
	}
	sp->end = zn->shm.addr + zn->shm.size;
	sp->min_shift = 3;
	sp->addr = zn->shm.addr;
#if (NGX_HAVE_ATOMIC_OPS)
	file = NULL;
#else
	file = ngx_pnalloc(cycle->pool, cycle->lock_file.len + zn->shm.name.len);
	if(file == NULL) {
		return NGX_ERROR;
	}
	(void)ngx_sprintf(file, "%V%V%Z", &cycle->lock_file, &zn->shm.name);
#endif
	if(ngx_shmtx_create(&sp->mutex, &sp->lock, file) != NGX_OK) {
		return NGX_ERROR;
	}
	ngx_slab_init(sp);
	return NGX_OK;
}

ngx_int_t ngx_create_pidfile(ngx_str_t * name, ngx_log_t * log, const NgxStartUpOptions & rO)
{
	if(ngx_process <= NGX_PROCESS_MASTER) {
		size_t len;
		ngx_uint_t create;
		uchar pid[NGX_INT64_LEN + 2];
		ngx_file_t file;
		memzero(&file, sizeof(ngx_file_t));
		file.name = *name;
		file.log = log;
		create = (/*ngx_test_config*/rO.Flags & rO.fTestConf) ? NGX_FILE_CREATE_OR_OPEN : NGX_FILE_TRUNCATE;
		file.fd = ngx_open_file(file.name.data, NGX_FILE_RDWR, create, NGX_FILE_DEFAULT_ACCESS);
		if(file.fd == NGX_INVALID_FILE) {
			ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, ngx_open_file_n " \"%s\" failed", file.name.data);
			return NGX_ERROR;
		}
		if(!(/*ngx_test_config*/rO.Flags & rO.fTestConf)) {
			len = ngx_snprintf(pid, NGX_INT64_LEN + 2, "%P%N", ngx_pid) - pid;
			if(ngx_write_file(&file, pid, len, 0) == NGX_ERROR) {
				return NGX_ERROR;
			}
		}
		if(ngx_close_file(file.fd) == NGX_FILE_ERROR) {
			ngx_log_error(NGX_LOG_ALERT, log, ngx_errno, ngx_close_file_n " \"%s\" failed", file.name.data);
		}
	}
	return NGX_OK;
}

void ngx_delete_pidfile(ngx_cycle_t * cycle)
{
	ngx_core_conf_t  * ccf = (ngx_core_conf_t*)ngx_get_conf(cycle->conf_ctx, ngx_core_module);
	uchar * name = ngx_new_binary ? ccf->oldpid.data : ccf->pid.data;
	if(ngx_delete_file(name) == NGX_FILE_ERROR) {
		ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno, ngx_delete_file_n " \"%s\" failed", name);
	}
}

static ngx_int_t ngx_test_lockfile(uchar * file, ngx_log_t * log)
{
#if !(NGX_HAVE_ATOMIC_OPS)
	ngx_fd_t fd = ngx_open_file(file, NGX_FILE_RDWR, NGX_FILE_CREATE_OR_OPEN, NGX_FILE_DEFAULT_ACCESS);
	if(fd == NGX_INVALID_FILE) {
		ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, ngx_open_file_n " \"%s\" failed", file);
		return NGX_ERROR;
	}
	if(ngx_close_file(fd) == NGX_FILE_ERROR) {
		ngx_log_error(NGX_LOG_ALERT, log, ngx_errno, ngx_close_file_n " \"%s\" failed", file);
	}
	if(ngx_delete_file(file) == NGX_FILE_ERROR) {
		ngx_log_error(NGX_LOG_ALERT, log, ngx_errno, ngx_delete_file_n " \"%s\" failed", file);
	}
#endif
	return NGX_OK;
}

void ngx_reopen_files(ngx_cycle_t * cycle, ngx_uid_t user)
{
	ngx_fd_t fd;
	ngx_uint_t i;
	ngx_list_part_t  * part = &cycle->open_files.part;
	ngx_open_file_t  * file = (ngx_open_file_t *)part->elts;
	for(i = 0; /* void */; i++) {
		if(i >= part->nelts) {
			if(part->next == NULL) {
				break;
			}
			part = part->next;
			file = (ngx_open_file_t *)part->elts;
			i = 0;
		}
		if(file[i].name.len == 0) {
			continue;
		}
		if(file[i].flush) {
			file[i].flush(&file[i], cycle->log);
		}
		fd = ngx_open_file(file[i].name.data, NGX_FILE_APPEND, NGX_FILE_CREATE_OR_OPEN, NGX_FILE_DEFAULT_ACCESS);
		ngx_log_debug3(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "reopen file \"%s\", old:%d new:%d", file[i].name.data, file[i].fd, fd);
		if(fd == NGX_INVALID_FILE) {
			ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno, ngx_open_file_n " \"%s\" failed", file[i].name.data);
			continue;
		}
#if !(NGX_WIN32)
		if(user != (ngx_uid_t)NGX_CONF_UNSET_UINT) {
			ngx_file_info_t fi;
			if(ngx_file_info(file[i].name.data, &fi) == NGX_FILE_ERROR) {
				ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno, ngx_file_info_n " \"%s\" failed", file[i].name.data);
				if(ngx_close_file(fd) == NGX_FILE_ERROR) {
					ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno, ngx_close_file_n " \"%s\" failed", file[i].name.data);
				}
				continue;
			}
			if(fi.st_uid != user) {
				if(chown((const char *)file[i].name.data, user, -1) == -1) {
					ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno, "chown(\"%s\", %d) failed", file[i].name.data, user);
					if(ngx_close_file(fd) == NGX_FILE_ERROR) {
						ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno, ngx_close_file_n " \"%s\" failed", file[i].name.data);
					}
					continue;
				}
			}
			if((fi.st_mode & (S_IRUSR|S_IWUSR)) != (S_IRUSR|S_IWUSR)) {
				fi.st_mode |= (S_IRUSR|S_IWUSR);
				if(chmod((const char *)file[i].name.data, fi.st_mode) == -1) {
					ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno, "chmod() \"%s\" failed", file[i].name.data);
					if(ngx_close_file(fd) == NGX_FILE_ERROR) {
						ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno, ngx_close_file_n " \"%s\" failed", file[i].name.data);
					}
					continue;
				}
			}
		}
		if(fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
			ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno, "fcntl(FD_CLOEXEC) \"%s\" failed", file[i].name.data);
			if(ngx_close_file(fd) == NGX_FILE_ERROR) {
				ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno, ngx_close_file_n " \"%s\" failed", file[i].name.data);
			}
			continue;
		}
#endif
		if(ngx_close_file(file[i].fd) == NGX_FILE_ERROR) {
			ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno, ngx_close_file_n " \"%s\" failed", file[i].name.data);
		}
		file[i].fd = fd;
	}
	(void)ngx_log_redirect_stderr(cycle);
}

ngx_shm_zone_t * ngx_shared_memory_add(ngx_conf_t * cf, ngx_str_t * name, size_t size, void * tag)
{
	ngx_uint_t i;
	ngx_list_part_t  * part = &cf->cycle->shared_memory.part;
	ngx_shm_zone_t * shm_zone = (ngx_shm_zone_t *)part->elts;
	for(i = 0; /* void */; i++) {
		if(i >= part->nelts) {
			if(part->next == NULL) {
				break;
			}
			part = part->next;
			shm_zone = (ngx_shm_zone_t *)part->elts;
			i = 0;
		}
		if(name->len == shm_zone[i].shm.name.len && ngx_strncmp(name->data, shm_zone[i].shm.name.data, name->len) == 0) {
			if(tag != shm_zone[i].tag) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "the shared memory zone \"%V\" is already declared for a different use", &shm_zone[i].shm.name);
				return NULL;
			}
			SETIFZ(shm_zone[i].shm.size, size);
			if(size && size != shm_zone[i].shm.size) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "the size %uz of shared memory zone \"%V\" conflicts with already declared size %uz",
					size, &shm_zone[i].shm.name, shm_zone[i].shm.size);
				return NULL;
			}
			return &shm_zone[i];
		}
	}
	shm_zone = (ngx_shm_zone_t *)ngx_list_push(&cf->cycle->shared_memory);
	if(shm_zone) {
		shm_zone->data = NULL;
		shm_zone->shm.log = cf->cycle->log;
		shm_zone->shm.size = size;
		shm_zone->shm.name = *name;
		shm_zone->shm.exists = 0;
		shm_zone->F_Init = NULL;
		shm_zone->tag = tag;
		shm_zone->noreuse = 0;
	}
	return shm_zone;
}

static void ngx_clean_old_cycles(ngx_event_t * ev)
{
	ngx_uint_t i, n, found, live;
	ngx_cycle_t  ** cycle;
	ngx_log_t * log = ngx_cycle->log;
	ngx_temp_pool->log = log;
	ngx_log_debug0(NGX_LOG_DEBUG_CORE, log, 0, "clean old cycles");
	live = 0;
	cycle = (ngx_cycle_t **)ngx_old_cycles.elts;
	for(i = 0; i < ngx_old_cycles.nelts; i++) {
		if(cycle[i]) {
			found = 0;
			for(n = 0; n < cycle[i]->connection_n; n++) {
				if(cycle[i]->connections[n].fd != (ngx_socket_t)-1) {
					found = 1;
					ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0, "live fd:%ui", n);
					break;
				}
			}
			if(found) {
				live = 1;
			}
			else {
				ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0, "clean old cycle: %ui", i);
				ngx_destroy_pool(cycle[i]->pool);
				cycle[i] = NULL;
			}
		}
	}
	ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0, "old cycles status: %ui", live);
	if(live) {
		ngx_add_timer(ev, 30000);
	}
	else {
		ngx_destroy_pool(ngx_temp_pool);
		ngx_temp_pool = NULL;
		ngx_old_cycles.nelts = 0;
	}
}

void ngx_set_shutdown_timer(ngx_cycle_t * cycle)
{
	ngx_core_conf_t  * ccf = (ngx_core_conf_t*)ngx_get_conf(cycle->conf_ctx, ngx_core_module);
	if(ccf->shutdown_timeout) {
		ngx_shutdown_event.F_EvHandler = ngx_shutdown_timer_handler;
		ngx_shutdown_event.P_Data = cycle;
		ngx_shutdown_event.log = cycle->log;
		ngx_shutdown_event.cancelable = 1;
		ngx_add_timer(&ngx_shutdown_event, ccf->shutdown_timeout);
	}
}

static void ngx_shutdown_timer_handler(ngx_event_t * ev)
{
	ngx_cycle_t * cycle = (ngx_cycle_t *)ev->P_Data;
	ngx_connection_t  * c = cycle->connections;
	for(ngx_uint_t i = 0; i < cycle->connection_n; i++) {
		if(c[i].fd == (ngx_socket_t)-1 || c[i].P_EvRd == NULL || c[i].P_EvRd->accept || c[i].P_EvRd->channel || c[i].P_EvRd->resolver) {
			continue;
		}
		ngx_log_debug1(NGX_LOG_DEBUG_CORE, ev->log, 0, "*%uA shutdown timeout", c[i].number);
		c[i].close = 1;
		c[i].error = 1;
		c[i].P_EvRd->F_EvHandler(c[i].P_EvRd);
	}
}
