/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop
//#include <ngx_event.h>

ngx_os_io_t ngx_io; // @global

static void ngx_drain_connections(ngx_cycle_t * cycle);

ngx_listening_t * ngx_create_listening(ngx_conf_t * cf, struct sockaddr * sockaddr, socklen_t socklen)
{
	size_t len;
	struct sockaddr * sa;
	uchar text[NGX_SOCKADDR_STRLEN];
	ngx_listening_t * ls = (ngx_listening_t *)ngx_array_push(&cf->cycle->listening);
	if(ls) {
		memzero(ls, sizeof(ngx_listening_t));
		sa = (struct sockaddr *)ngx_palloc(cf->pool, socklen);
		if(sa == NULL) {
			return NULL;
		}
		memcpy(sa, sockaddr, socklen);
		ls->sockaddr = sa;
		ls->socklen = socklen;
		len = ngx_sock_ntop(sa, socklen, text, NGX_SOCKADDR_STRLEN, 1);
		ls->addr_text.len = len;
		switch(ls->sockaddr->sa_family) {
#if (NGX_HAVE_INET6)
			case AF_INET6:
				ls->addr_text_max_len = NGX_INET6_ADDRSTRLEN;
				break;
#endif
#if (NGX_HAVE_UNIX_DOMAIN)
			case AF_UNIX:
				ls->addr_text_max_len = NGX_UNIX_ADDRSTRLEN;
				len++;
				break;
#endif
			case AF_INET:
				ls->addr_text_max_len = NGX_INET_ADDRSTRLEN;
				break;
			default:
				ls->addr_text_max_len = NGX_SOCKADDR_STRLEN;
				break;
		}
		ls->addr_text.data = static_cast<uchar *>(ngx_pnalloc(cf->pool, len));
		if(ls->addr_text.data == NULL) {
			return NULL;
		}
		memcpy(ls->addr_text.data, text, len);
		ls->fd = (ngx_socket_t)-1;
		ls->type = SOCK_STREAM;
		ls->backlog = NGX_LISTEN_BACKLOG;
		ls->rcvbuf = -1;
		ls->sndbuf = -1;
#if (NGX_HAVE_SETFIB)
		ls->setfib = -1;
#endif
#if (NGX_HAVE_TCP_FASTOPEN)
		ls->fastopen = -1;
#endif
	}
	return ls;
}

ngx_int_t ngx_clone_listening(ngx_conf_t * cf, ngx_listening_t * ls)
{
#if (NGX_HAVE_REUSEPORT)

	ngx_int_t n;
	ngx_core_conf_t  * ccf;
	ngx_listening_t ols;
	if(!ls->reuseport) {
		return NGX_OK;
	}
	ols = *ls;
	ccf = (ngx_core_conf_t*)ngx_get_conf(cf->cycle->conf_ctx, ngx_core_module);
	for(n = 1; n < ccf->worker_processes; n++) {
		/* create a socket for each worker process */
		ls = ngx_array_push(&cf->cycle->listening);
		if(ls == NULL) {
			return NGX_ERROR;
		}
		*ls = ols;
		ls->worker = n;
	}
#endif
	return NGX_OK;
}

ngx_int_t ngx_set_inherited_sockets(ngx_cycle_t * cycle)
{
	size_t len;
	ngx_uint_t i;
	ngx_listening_t * ls;
	socklen_t olen;
#if (NGX_HAVE_DEFERRED_ACCEPT || NGX_HAVE_TCP_FASTOPEN)
	ngx_err_t err;
#endif
#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
	struct accept_filter_arg af;
#endif
#if (NGX_HAVE_DEFERRED_ACCEPT && defined TCP_DEFER_ACCEPT)
	int timeout;
#endif
#if (NGX_HAVE_REUSEPORT)
	int reuseport;
#endif
	ls = (ngx_listening_t *)cycle->listening.elts;
	for(i = 0; i < cycle->listening.nelts; i++) {
		ls[i].sockaddr = (struct sockaddr *)ngx_palloc(cycle->pool, sizeof(ngx_sockaddr_t));
		if(ls[i].sockaddr == NULL) {
			return NGX_ERROR;
		}
		ls[i].socklen = sizeof(ngx_sockaddr_t);
		if(getsockname(ls[i].fd, ls[i].sockaddr, &ls[i].socklen) == -1) {
			ngx_log_error(NGX_LOG_CRIT, cycle->log, ngx_socket_errno, "getsockname() of the inherited socket #%d failed", ls[i].fd);
			ls[i].ignore = 1;
			continue;
		}
		switch(ls[i].sockaddr->sa_family) {
#if (NGX_HAVE_INET6)
			case AF_INET6:
			    ls[i].addr_text_max_len = NGX_INET6_ADDRSTRLEN;
			    len = NGX_INET6_ADDRSTRLEN + sizeof("[]:65535") - 1;
			    break;
#endif
#if (NGX_HAVE_UNIX_DOMAIN)
			case AF_UNIX:
			    ls[i].addr_text_max_len = NGX_UNIX_ADDRSTRLEN;
			    len = NGX_UNIX_ADDRSTRLEN;
			    break;
#endif
			case AF_INET:
			    ls[i].addr_text_max_len = NGX_INET_ADDRSTRLEN;
			    len = NGX_INET_ADDRSTRLEN + sizeof(":65535") - 1;
			    break;
			default:
			    ngx_log_error(NGX_LOG_CRIT, cycle->log, ngx_socket_errno, "the inherited socket #%d has an unsupported protocol family", ls[i].fd);
			    ls[i].ignore = 1;
			    continue;
		}
		ls[i].addr_text.data = (uchar *)ngx_pnalloc(cycle->pool, len);
		if(ls[i].addr_text.data == NULL) {
			return NGX_ERROR;
		}
		len = ngx_sock_ntop(ls[i].sockaddr, ls[i].socklen, ls[i].addr_text.data, len, 1);
		if(!len) {
			return NGX_ERROR;
		}
		ls[i].addr_text.len = len;
		ls[i].backlog = NGX_LISTEN_BACKLOG;
		olen = sizeof(int);
		if(getsockopt(ls[i].fd, SOL_SOCKET, SO_TYPE, (char *)&ls[i].type, &olen) == -1) {
			ngx_log_error(NGX_LOG_CRIT, cycle->log, ngx_socket_errno, "getsockopt(SO_TYPE) %V failed", &ls[i].addr_text);
			ls[i].ignore = 1;
			continue;
		}
		olen = sizeof(int);
		if(getsockopt(ls[i].fd, SOL_SOCKET, SO_RCVBUF, (char *)&ls[i].rcvbuf, &olen) == -1) {
			ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "getsockopt(SO_RCVBUF) %V failed, ignored", &ls[i].addr_text);
			ls[i].rcvbuf = -1;
		}
		olen = sizeof(int);
		if(getsockopt(ls[i].fd, SOL_SOCKET, SO_SNDBUF, (char *)&ls[i].sndbuf, &olen) == -1) {
			ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "getsockopt(SO_SNDBUF) %V failed, ignored", &ls[i].addr_text);
			ls[i].sndbuf = -1;
		}
#if 0
		/* SO_SETFIB is currently a set only option */
#if (NGX_HAVE_SETFIB)
		olen = sizeof(int);
		if(getsockopt(ls[i].fd, SOL_SOCKET, SO_SETFIB, (void *)&ls[i].setfib, &olen) == -1) {
			ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "getsockopt(SO_SETFIB) %V failed, ignored", &ls[i].addr_text);
			ls[i].setfib = -1;
		}
#endif
#endif
#if (NGX_HAVE_REUSEPORT)
		reuseport = 0;
		olen = sizeof(int);
		if(getsockopt(ls[i].fd, SOL_SOCKET, SO_REUSEPORT, (void *)&reuseport, &olen) == -1) {
			ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "getsockopt(SO_REUSEPORT) %V failed, ignored", &ls[i].addr_text);
		}
		else {
			ls[i].reuseport = reuseport ? 1 : 0;
		}
#endif
		if(ls[i].type != SOCK_STREAM) {
			continue;
		}
#if (NGX_HAVE_TCP_FASTOPEN)
		olen = sizeof(int);
		if(getsockopt(ls[i].fd, IPPROTO_TCP, TCP_FASTOPEN, (void *)&ls[i].fastopen, &olen) == -1) {
			err = ngx_socket_errno;
			if(err != NGX_EOPNOTSUPP && err != NGX_ENOPROTOOPT) {
				ngx_log_error(NGX_LOG_NOTICE, cycle->log, err, "getsockopt(TCP_FASTOPEN) %V failed, ignored", &ls[i].addr_text);
			}
			ls[i].fastopen = -1;
		}
#endif
#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
		memzero(&af, sizeof(struct accept_filter_arg));
		olen = sizeof(struct accept_filter_arg);
		if(getsockopt(ls[i].fd, SOL_SOCKET, SO_ACCEPTFILTER, &af, &olen) == -1) {
			err = ngx_socket_errno;
			if(err == NGX_EINVAL) {
				continue;
			}
			ngx_log_error(NGX_LOG_NOTICE, cycle->log, err, "getsockopt(SO_ACCEPTFILTER) for %V failed, ignored", &ls[i].addr_text);
			continue;
		}
		if(olen < sizeof(struct accept_filter_arg) || af.af_name[0] == '\0') {
			continue;
		}
		ls[i].accept_filter = ngx_palloc(cycle->pool, 16);
		if(ls[i].accept_filter == NULL) {
			return NGX_ERROR;
		}
		(void)ngx_cpystrn((uchar *)ls[i].accept_filter, (uchar *)af.af_name, 16);
#endif
#if (NGX_HAVE_DEFERRED_ACCEPT && defined TCP_DEFER_ACCEPT)
		timeout = 0;
		olen = sizeof(int);
		if(getsockopt(ls[i].fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &timeout, &olen) == -1) {
			err = ngx_socket_errno;
			if(err == NGX_EOPNOTSUPP) {
				continue;
			}
			ngx_log_error(NGX_LOG_NOTICE, cycle->log, err, "getsockopt(TCP_DEFER_ACCEPT) for %V failed, ignored", &ls[i].addr_text);
			continue;
		}
		if(olen < sizeof(int) || timeout == 0) {
			continue;
		}
		ls[i].deferred_accept = 1;
#endif
	}
	return NGX_OK;
}

ngx_int_t ngx_open_listening_sockets(ngx_cycle_t * cycle, const NgxStartUpOptions & rO)
{
	ngx_uint_t i, tries, failed;
	ngx_err_t err;
	ngx_log_t * log;
	ngx_socket_t s;
	ngx_listening_t  * ls;
	int reuseaddr = 1;
#if (NGX_SUPPRESS_WARN)
	failed = 0;
#endif
	log = cycle->log;
	/* @todo configurable try number */
	for(tries = 5; tries; tries--) {
		failed = 0;
		/* for each listening socket */
		ls = (ngx_listening_t *)cycle->listening.elts;
		for(i = 0; i < cycle->listening.nelts; i++) {
			if(ls[i].ignore) {
				continue;
			}
#if (NGX_HAVE_REUSEPORT)
			if(ls[i].add_reuseport) {
				/*
				 * to allow transition from a socket without SO_REUSEPORT
				 * to multiple sockets with SO_REUSEPORT, we have to set
				 * SO_REUSEPORT on the old socket before opening new ones
				 */
				int reuseport = 1;
				if(setsockopt(ls[i].fd, SOL_SOCKET, SO_REUSEPORT, (const void *)&reuseport, sizeof(int)) == -1) {
					ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "setsockopt(SO_REUSEPORT) %V failed, ignored", &ls[i].addr_text);
				}
				ls[i].add_reuseport = 0;
			}
#endif
			if(ls[i].fd != (ngx_socket_t)-1) {
				continue;
			}
			if(ls[i].inherited) {
				/* @todo close on exit */
				/* @todo nonblocking */
				/* @todo deferred accept */
				continue;
			}
			s = ngx_socket(ls[i].sockaddr->sa_family, ls[i].type, 0);
			if(s == (ngx_socket_t)-1) {
				ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno, ngx_socket_n " %V failed", &ls[i].addr_text);
				return NGX_ERROR;
			}
			if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuseaddr, sizeof(int)) == -1) {
				ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno, "setsockopt(SO_REUSEADDR) %V failed", &ls[i].addr_text);
				if(ngx_close_socket(s) == -1) {
					ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno, ngx_close_socket_n " %V failed", &ls[i].addr_text);
				}
				return NGX_ERROR;
			}
#if (NGX_HAVE_REUSEPORT)
			if(ls[i].reuseport && !ngx_test_config) {
				int reuseport = 1;
				if(setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (const void *)&reuseport, sizeof(int)) == -1) {
					ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno, "setsockopt(SO_REUSEPORT) %V failed", &ls[i].addr_text);
					if(ngx_close_socket(s) == -1) {
						ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno, ngx_close_socket_n " %V failed", &ls[i].addr_text);
					}
					return NGX_ERROR;
				}
			}
#endif
#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
			if(ls[i].sockaddr->sa_family == AF_INET6) {
				int ipv6only = ls[i].ipv6only;
				if(setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&ipv6only, sizeof(int)) == -1) {
					ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno, "setsockopt(IPV6_V6ONLY) %V failed, ignored", &ls[i].addr_text);
				}
			}
#endif
			/* @todo close on exit */
			if(!(ngx_event_flags & NGX_USE_IOCP_EVENT)) {
				if(ngx_nonblocking(s) == -1) {
					ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno, ngx_nonblocking_n " %V failed", &ls[i].addr_text);
					if(ngx_close_socket(s) == -1) {
						ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno, ngx_close_socket_n " %V failed", &ls[i].addr_text);
					}
					return NGX_ERROR;
				}
			}
			ngx_log_debug2(NGX_LOG_DEBUG_CORE, log, 0, "bind() %V #%d ", &ls[i].addr_text, s);
			if(bind(s, ls[i].sockaddr, ls[i].socklen) == -1) {
				err = ngx_socket_errno;
				if(err != NGX_EADDRINUSE || !(/*ngx_test_config*/rO.Flags & rO.fTestConf)) {
					ngx_log_error(NGX_LOG_EMERG, log, err, "bind() to %V failed", &ls[i].addr_text);
				}
				if(ngx_close_socket(s) == -1) {
					ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno, ngx_close_socket_n " %V failed", &ls[i].addr_text);
				}
				if(err != NGX_EADDRINUSE) {
					return NGX_ERROR;
				}
				if(!(/*ngx_test_config*/rO.Flags & rO.fTestConf)) {
					failed = 1;
				}
				continue;
			}
#if (NGX_HAVE_UNIX_DOMAIN)
			if(ls[i].sockaddr->sa_family == AF_UNIX) {
				uchar  * name = ls[i].addr_text.data + sizeof("unix:") - 1;
				mode_t mode = (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
				if(chmod((char *)name, mode) == -1) {
					ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno, "chmod() \"%s\" failed", name);
				}
				if(ngx_test_config) {
					if(ngx_delete_file(name) == NGX_FILE_ERROR) {
						ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno, ngx_delete_file_n " %s failed", name);
					}
				}
			}
#endif
			if(ls[i].type != SOCK_STREAM) {
				ls[i].fd = s;
				continue;
			}
			if(listen(s, ls[i].backlog) == -1) {
				err = ngx_socket_errno;
				/*
				 * on OpenVZ after suspend/resume EADDRINUSE
				 * may be returned by listen() instead of bind(), see
				 * https://bugzilla.openvz.org/show_bug.cgi?id=2470
				 */
				if(err != NGX_EADDRINUSE || !(/*ngx_test_config*/rO.Flags & rO.fTestConf)) {
					ngx_log_error(NGX_LOG_EMERG, log, err, "listen() to %V, backlog %d failed", &ls[i].addr_text, ls[i].backlog);
				}
				if(ngx_close_socket(s) == -1) {
					ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno, ngx_close_socket_n " %V failed", &ls[i].addr_text);
				}
				if(err != NGX_EADDRINUSE) {
					return NGX_ERROR;
				}
				if(!(/*ngx_test_config*/rO.Flags & rO.fTestConf)) {
					failed = 1;
				}
				continue;
			}
			ls[i].listen = 1;
			ls[i].fd = s;
		}
		if(!failed) {
			break;
		}
		/* @todo delay configurable */
		ngx_log_error(NGX_LOG_NOTICE, log, 0, "try again to bind() after 500ms");
		ngx_msleep(500);
	}
	if(failed) {
		ngx_log_error(NGX_LOG_EMERG, log, 0, "still could not bind()");
		return NGX_ERROR;
	}
	return NGX_OK;
}

void ngx_configure_listening_sockets(ngx_cycle_t * cycle)
{
	int value;
	ngx_uint_t i;
	ngx_listening_t * ls;
#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
	struct accept_filter_arg af;
#endif
	ls = (ngx_listening_t *)cycle->listening.elts;
	for(i = 0; i < cycle->listening.nelts; i++) {
		ls[i].log = *ls[i].logp;
		if(ls[i].rcvbuf != -1) {
			if(setsockopt(ls[i].fd, SOL_SOCKET, SO_RCVBUF, (const char *)&ls[i].rcvbuf, sizeof(int)) == -1) {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "setsockopt(SO_RCVBUF, %d) %V failed, ignored", ls[i].rcvbuf, &ls[i].addr_text);
			}
		}
		if(ls[i].sndbuf != -1) {
			if(setsockopt(ls[i].fd, SOL_SOCKET, SO_SNDBUF, (const char *)&ls[i].sndbuf, sizeof(int)) == -1) {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "setsockopt(SO_SNDBUF, %d) %V failed, ignored", ls[i].sndbuf, &ls[i].addr_text);
			}
		}
		if(ls[i].keepalive) {
			value = (ls[i].keepalive == 1) ? 1 : 0;
			if(setsockopt(ls[i].fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&value, sizeof(int)) == -1) {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "setsockopt(SO_KEEPALIVE, %d) %V failed, ignored", value, &ls[i].addr_text);
			}
		}
#if (NGX_HAVE_KEEPALIVE_TUNABLE)
		if(ls[i].keepidle) {
			value = ls[i].keepidle;
#if (NGX_KEEPALIVE_FACTOR)
			value *= NGX_KEEPALIVE_FACTOR;
#endif
			if(setsockopt(ls[i].fd, IPPROTO_TCP, TCP_KEEPIDLE, (const void *)&value, sizeof(int)) == -1) {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "setsockopt(TCP_KEEPIDLE, %d) %V failed, ignored", value, &ls[i].addr_text);
			}
		}
		if(ls[i].keepintvl) {
			value = ls[i].keepintvl;
#if (NGX_KEEPALIVE_FACTOR)
			value *= NGX_KEEPALIVE_FACTOR;
#endif
			if(setsockopt(ls[i].fd, IPPROTO_TCP, TCP_KEEPINTVL, (const void *)&value, sizeof(int)) == -1) {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "setsockopt(TCP_KEEPINTVL, %d) %V failed, ignored", value, &ls[i].addr_text);
			}
		}
		if(ls[i].keepcnt) {
			if(setsockopt(ls[i].fd, IPPROTO_TCP, TCP_KEEPCNT, (const void *)&ls[i].keepcnt, sizeof(int)) == -1) {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "setsockopt(TCP_KEEPCNT, %d) %V failed, ignored", ls[i].keepcnt, &ls[i].addr_text);
			}
		}
#endif
#if (NGX_HAVE_SETFIB)
		if(ls[i].setfib != -1) {
			if(setsockopt(ls[i].fd, SOL_SOCKET, SO_SETFIB, (const void *)&ls[i].setfib, sizeof(int)) == -1) {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "setsockopt(SO_SETFIB, %d) %V failed, ignored", ls[i].setfib, &ls[i].addr_text);
			}
		}
#endif
#if (NGX_HAVE_TCP_FASTOPEN)
		if(ls[i].fastopen != -1) {
			if(setsockopt(ls[i].fd, IPPROTO_TCP, TCP_FASTOPEN, (const void *)&ls[i].fastopen, sizeof(int)) == -1) {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "setsockopt(TCP_FASTOPEN, %d) %V failed, ignored", ls[i].fastopen, &ls[i].addr_text);
			}
		}
#endif
#if 0
		if(1) {
			int tcp_nodelay = 1;
			if(setsockopt(ls[i].fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&tcp_nodelay, sizeof(int)) == -1) {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "setsockopt(TCP_NODELAY) %V failed, ignored", &ls[i].addr_text);
			}
		}
#endif
		if(ls[i].listen) {
			/* change backlog via listen() */
			if(listen(ls[i].fd, ls[i].backlog) == -1) {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "listen() to %V, backlog %d failed, ignored", &ls[i].addr_text, ls[i].backlog);
			}
		}
		/*
		 * setting deferred mode should be last operation on socket,
		 * because code may prematurely continue cycle on failure
		 */
#if (NGX_HAVE_DEFERRED_ACCEPT)
#ifdef SO_ACCEPTFILTER
		if(ls[i].delete_deferred) {
			if(setsockopt(ls[i].fd, SOL_SOCKET, SO_ACCEPTFILTER, NULL, 0) == -1) {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "setsockopt(SO_ACCEPTFILTER, NULL) for %V failed, ignored", &ls[i].addr_text);
				if(ls[i].accept_filter) {
					ngx_log_error(NGX_LOG_ALERT, cycle->log, 0, "could not change the accept filter to \"%s\" for %V, ignored", ls[i].accept_filter, &ls[i].addr_text);
				}
				continue;
			}
			ls[i].deferred_accept = 0;
		}
		if(ls[i].add_deferred) {
			memzero(&af, sizeof(struct accept_filter_arg));
			(void)ngx_cpystrn((uchar *)af.af_name, (uchar *)ls[i].accept_filter, 16);
			if(setsockopt(ls[i].fd, SOL_SOCKET, SO_ACCEPTFILTER, &af, sizeof(struct accept_filter_arg)) == -1) {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "setsockopt(SO_ACCEPTFILTER, \"%s\") for %V failed, ignored", ls[i].accept_filter, &ls[i].addr_text);
				continue;
			}
			ls[i].deferred_accept = 1;
		}
#endif
#ifdef TCP_DEFER_ACCEPT
		if(ls[i].add_deferred || ls[i].delete_deferred) {
			if(ls[i].add_deferred) {
				/*
				 * There is no way to find out how long a connection was
				 * in queue (and a connection may bypass deferred queue at all
				 * if syncookies were used), hence we use 1 second timeout
				 * here.
				 */
				value = 1;
			}
			else {
				value = 0;
			}
			if(setsockopt(ls[i].fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &value, sizeof(int)) == -1) {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "setsockopt(TCP_DEFER_ACCEPT, %d) for %V failed, ignored", value, &ls[i].addr_text);
				continue;
			}
		}
		if(ls[i].add_deferred) {
			ls[i].deferred_accept = 1;
		}
#endif
#endif /* NGX_HAVE_DEFERRED_ACCEPT */
#if (NGX_HAVE_IP_RECVDSTADDR)
		if(ls[i].wildcard && ls[i].type == SOCK_DGRAM && ls[i].sockaddr->sa_family == AF_INET) {
			value = 1;
			if(setsockopt(ls[i].fd, IPPROTO_IP, IP_RECVDSTADDR, (const void *)&value, sizeof(int)) == -1) {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "setsockopt(IP_RECVDSTADDR) for %V failed, ignored", &ls[i].addr_text);
			}
		}
#elif (NGX_HAVE_IP_PKTINFO)
		if(ls[i].wildcard && ls[i].type == SOCK_DGRAM && ls[i].sockaddr->sa_family == AF_INET) {
			value = 1;
			if(setsockopt(ls[i].fd, IPPROTO_IP, IP_PKTINFO, (const void *)&value, sizeof(int)) == -1) {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "setsockopt(IP_PKTINFO) for %V failed, ignored", &ls[i].addr_text);
			}
		}
#endif
#if (NGX_HAVE_INET6 && NGX_HAVE_IPV6_RECVPKTINFO)
		if(ls[i].wildcard && ls[i].type == SOCK_DGRAM && ls[i].sockaddr->sa_family == AF_INET6) {
			value = 1;
			if(setsockopt(ls[i].fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, (const void *)&value, sizeof(int)) == -1) {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno, "setsockopt(IPV6_RECVPKTINFO) for %V failed, ignored", &ls[i].addr_text);
			}
		}

#endif
	}
	return;
}

void ngx_close_listening_sockets(ngx_cycle_t * cycle)
{
	if(!(ngx_event_flags & NGX_USE_IOCP_EVENT)) {
		ngx_accept_mutex_held = 0;
		ngx_use_accept_mutex = 0;
		ngx_listening_t * ls = (ngx_listening_t *)cycle->listening.elts;
		for(ngx_uint_t i = 0; i < cycle->listening.nelts; i++) {
			ngx_connection_t * c = ls[i].connection;
			if(c) {
				if(c->P_EvRd->active) {
					if(ngx_event_flags & NGX_USE_EPOLL_EVENT) {
						/*
						 * it seems that Linux-2.6.x OpenVZ sends events
						 * for closed shared listening sockets unless
						 * the events was explicitly deleted
						 */
						ngx_del_event(c->P_EvRd, NGX_READ_EVENT, 0);
					}
					else {
						ngx_del_event(c->P_EvRd, NGX_READ_EVENT, NGX_CLOSE_EVENT);
					}
				}
				ngx_free_connection(c);
				c->fd = (ngx_socket_t)-1;
			}
			ngx_log_debug2(NGX_LOG_DEBUG_CORE, cycle->log, 0, "close listening %V #%d ", &ls[i].addr_text, ls[i].fd);
			if(ngx_close_socket(ls[i].fd) == -1) {
				ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_socket_errno, ngx_close_socket_n " %V failed", &ls[i].addr_text);
			}
#if (NGX_HAVE_UNIX_DOMAIN)
			if(ls[i].sockaddr->sa_family == AF_UNIX && ngx_process <= NGX_PROCESS_MASTER && ngx_new_binary == 0) {
				uchar * name = ls[i].addr_text.data + sizeof("unix:") - 1;
				if(ngx_delete_file(name) == NGX_FILE_ERROR) {
					ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_socket_errno, ngx_delete_file_n " %s failed", name);
				}
			}
#endif
			ls[i].fd = (ngx_socket_t)-1;
		}
		cycle->listening.nelts = 0;
	}
}

ngx_connection_t * ngx_get_connection(ngx_socket_t s, ngx_log_t * log)
{
	ngx_connection_t * c = 0;
	// disable warning: Win32 SOCKET is u_int while UNIX socket is int 
	ngx_log_error(NGX_LOG_INFO, log, 0, __FUNCTION__ " - connection-count: free=%d, reusable=%d, total=%d", 
		ngx_cycle->free_connection_n, ngx_cycle->reusable_connections_n, ngx_cycle->connection_n); // @v11.4.3
	if(ngx_cycle->files && (ngx_uint_t)s >= ngx_cycle->files_n) {
		ngx_log_error(NGX_LOG_ALERT, log, 0, "the new socket has number %d, but only %ui files are available", s, ngx_cycle->files_n);
	}
	else {
		c = ngx_cycle->free_connections;
		if(!c) {
			ngx_drain_connections((ngx_cycle_t*)ngx_cycle);
			c = ngx_cycle->free_connections;
		}
		if(!c) {
			ngx_log_error(NGX_LOG_ALERT, log, 0, "%ui worker_connections are not enough", ngx_cycle->connection_n);
		}
		else {
			ngx_cycle->free_connections = (ngx_connection_t *)c->data;
			ngx_cycle->free_connection_n--;
			if(ngx_cycle->files && ngx_cycle->files[s] == NULL) {
				ngx_cycle->files[s] = c;
			}
			{
				ngx_event_t * rev = c->P_EvRd;
				ngx_event_t * wev = c->P_EvWr;
				memzero(c, sizeof(ngx_connection_t));
				c->P_EvRd = rev;
				c->P_EvWr = wev;
				c->fd = s;
				c->log = log;
				{
					const ngx_uint_t instance = rev->instance;
					memzero(rev, sizeof(ngx_event_t));
					memzero(wev, sizeof(ngx_event_t));
					rev->instance = !instance;
					wev->instance = !instance;
					rev->index = NGX_INVALID_INDEX;
					wev->index = NGX_INVALID_INDEX;
					rev->P_Data = c;
					wev->P_Data = c;
					wev->write = 1;
				}
			}
		}
	}
	return c;
}

void FASTCALL ngx_free_connection(ngx_connection_t * c)
{
	if(c) {
		c->data = ngx_cycle->free_connections;
		ngx_cycle->free_connections = c;
		ngx_cycle->free_connection_n++;
		if(ngx_cycle->files && ngx_cycle->files[c->fd] == c)
			ngx_cycle->files[c->fd] = NULL;
	}
}

void FASTCALL ngx_close_connection(ngx_connection_t * c)
{
	ngx_uint_t log_error, level;
	ngx_socket_t fd;
	if(c->fd == (ngx_socket_t)-1)
		ngx_log_error(NGX_LOG_ALERT, c->log, 0, "connection already closed");
	else {
		if(c->P_EvRd->timer_set) {
			ngx_del_timer(c->P_EvRd);
		}
		if(c->P_EvWr->timer_set) {
			ngx_del_timer(c->P_EvWr);
		}
		if(!c->shared) {
			if(ngx_del_conn) {
				ngx_del_conn(c, NGX_CLOSE_EVENT);
			}
			else {
				if(c->P_EvRd->active || c->P_EvRd->disabled) {
					ngx_del_event(c->P_EvRd, NGX_READ_EVENT, NGX_CLOSE_EVENT);
				}
				if(c->P_EvWr->active || c->P_EvWr->disabled) {
					ngx_del_event(c->P_EvWr, NGX_WRITE_EVENT, NGX_CLOSE_EVENT);
				}
			}
		}
		if(c->P_EvRd->posted) {
			ngx_delete_posted_event(c->P_EvRd);
		}
		if(c->P_EvWr->posted) {
			ngx_delete_posted_event(c->P_EvWr);
		}
		c->P_EvRd->closed = 1;
		c->P_EvWr->closed = 1;
		ngx_reusable_connection(c, 0);
		log_error = c->log_error;
		ngx_free_connection(c);
		fd = c->fd;
		c->fd = (ngx_socket_t)-1;
		if(!c->shared) {
			if(ngx_close_socket(fd) == -1) {
				const ngx_err_t err = ngx_socket_errno;
				if(oneof2(err, NGX_ECONNRESET, NGX_ENOTCONN)) {
					switch(log_error) {
						case NGX_ERROR_INFO: level = NGX_LOG_INFO; break;
						case NGX_ERROR_ERR:  level = NGX_LOG_ERR;  break;
						default: level = NGX_LOG_CRIT;
					}
				}
				else
					level = NGX_LOG_CRIT;
				ngx_log_error(level, c->log, err, ngx_close_socket_n " %d failed", fd);
			}
		}
	}
}

void ngx_reusable_connection(ngx_connection_t * c, ngx_uint_t reusable)
{
	ngx_log_debug1(NGX_LOG_DEBUG_CORE, c->log, 0, "reusable connection: %ui", reusable);
	if(c->reusable) {
		ngx_queue_remove(&c->Queue);
		ngx_cycle->reusable_connections_n--;
#if (NGX_STAT_STUB)
		(void)ngx_atomic_fetch_add(ngx_stat_waiting, -1);
#endif
	}
	c->reusable = reusable;
	if(reusable) {
		/* need cast as ngx_cycle is volatile */
		ngx_queue_insert_head((ngx_queue_t*)&ngx_cycle->reusable_connections_queue, &c->Queue);
		ngx_cycle->reusable_connections_n++;

#if (NGX_STAT_STUB)
		(void)ngx_atomic_fetch_add(ngx_stat_waiting, 1);
#endif
	}
}

static void ngx_drain_connections(ngx_cycle_t * cycle)
{
	ngx_uint_t n = MAX(MIN(32, cycle->reusable_connections_n / 8), 1);
	for(ngx_uint_t i = 0; i < n; i++) {
		if(ngx_queue_empty(&cycle->reusable_connections_queue)) {
			break;
		}
		else {
			ngx_queue_t * q = ngx_queue_last(&cycle->reusable_connections_queue);
			ngx_connection_t * c = ngx_queue_data(q, ngx_connection_t, Queue);
			ngx_log_debug0(NGX_LOG_DEBUG_CORE, c->log, 0, "reusing connection");
			c->close = 1;
			c->P_EvRd->F_EvHandler(c->P_EvRd);
		}
	}
}

void ngx_close_idle_connections(ngx_cycle_t * cycle)
{
	ngx_connection_t * c = cycle->connections;
	for(ngx_uint_t i = 0; i < cycle->connection_n; i++) {
		// THREAD: lock 
		if(c[i].fd != (ngx_socket_t)-1 && c[i].idle) {
			c[i].close = 1;
			c[i].P_EvRd->F_EvHandler(c[i].P_EvRd);
		}
	}
}

ngx_int_t ngx_connection_local_sockaddr(ngx_connection_t * c, ngx_str_t * s, ngx_uint_t port)
{
	socklen_t len;
	ngx_uint_t addr = 0;
	ngx_sockaddr_t sa;
	struct sockaddr_in * sin;
#if (NGX_HAVE_INET6)
	ngx_uint_t i;
	struct sockaddr_in6 * sin6;
#endif
	if(c->local_socklen) {
		switch(c->local_sockaddr->sa_family) {
#if (NGX_HAVE_INET6)
			case AF_INET6:
			    sin6 = (struct sockaddr_in6*)c->local_sockaddr;
			    for(i = 0; addr == 0 && i < 16; i++) {
				    addr |= sin6->sin6_addr.s6_addr[i];
			    }
			    break;
#endif
#if (NGX_HAVE_UNIX_DOMAIN)
			case AF_UNIX:
			    addr = 1;
			    break;
#endif
			default: /* AF_INET */
			    sin = (struct sockaddr_in*)c->local_sockaddr;
			    addr = sin->sin_addr.s_addr;
			    break;
		}
	}
	if(addr == 0) {
		len = sizeof(ngx_sockaddr_t);
		if(getsockname(c->fd, &sa.sockaddr, &len) == -1) {
			ngx_connection_error(c, ngx_socket_errno, "getsockname() failed");
			return NGX_ERROR;
		}
		c->local_sockaddr = (struct sockaddr *)ngx_palloc(c->pool, len);
		if(c->local_sockaddr == NULL) {
			return NGX_ERROR;
		}
		memcpy(c->local_sockaddr, &sa, len);
		c->local_socklen = len;
	}
	if(!s) {
		return NGX_OK;
	}
	s->len = ngx_sock_ntop(c->local_sockaddr, c->local_socklen, s->data, s->len, port);
	return NGX_OK;
}

ngx_int_t ngx_tcp_nodelay(ngx_connection_t * c)
{
	if(c->tcp_nodelay == NGX_TCP_NODELAY_UNSET) {
		ngx_log_debug0(NGX_LOG_DEBUG_CORE, c->log, 0, "tcp_nodelay");
		int tcp_nodelay = 1;
		if(setsockopt(c->fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&tcp_nodelay, sizeof(int)) == -1) {
#if (NGX_SOLARIS)
			if(c->log_error == NGX_ERROR_INFO) {
				/* Solaris returns EINVAL if a socket has been shut down */
				c->log_error = NGX_ERROR_IGNORE_EINVAL;
				ngx_connection_error(c, ngx_socket_errno, "setsockopt(TCP_NODELAY) failed");
				c->log_error = NGX_ERROR_INFO;
				return NGX_ERROR;
			}
#endif
			ngx_connection_error(c, ngx_socket_errno, "setsockopt(TCP_NODELAY) failed");
			return NGX_ERROR;
		}
		c->tcp_nodelay = NGX_TCP_NODELAY_SET;
	}
	return NGX_OK;
}

ngx_int_t ngx_connection_error(ngx_connection_t * c, ngx_err_t err, const char * text)
{
	ngx_uint_t level;
	// Winsock may return NGX_ECONNABORTED instead of NGX_ECONNRESET 
	if((err == NGX_ECONNRESET
#if (NGX_WIN32)
		   || err == NGX_ECONNABORTED
#endif
		    ) && c->log_error == NGX_ERROR_IGNORE_ECONNRESET) {
		return 0;
	}
#if (NGX_SOLARIS)
	if(err == NGX_EINVAL && c->log_error == NGX_ERROR_IGNORE_EINVAL) {
		return 0;
	}
#endif
	if(err == 0 || err == NGX_ECONNRESET
#if (NGX_WIN32)
	   || err == NGX_ECONNABORTED
#else
	   || err == NGX_EPIPE
#endif
	   || oneof7(err, NGX_ENOTCONN, NGX_ETIMEDOUT, NGX_ECONNREFUSED, NGX_ENETDOWN, NGX_ENETUNREACH, NGX_EHOSTDOWN, NGX_EHOSTUNREACH)) {
		switch(c->log_error) {
			case NGX_ERROR_IGNORE_EINVAL:
			case NGX_ERROR_IGNORE_ECONNRESET:
			case NGX_ERROR_INFO:
			    level = NGX_LOG_INFO;
			    break;
			default:
			    level = NGX_LOG_ERR;
		}
	}
	else {
		level = NGX_LOG_ALERT;
	}
	ngx_log_error(level, c->log, err, text);
	return NGX_ERROR;
}
