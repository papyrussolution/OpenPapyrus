/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

static void ngx_http_wait_request_handler(ngx_event_t * ev);
static void ngx_http_process_request_line(ngx_event_t * rev);
static void ngx_http_process_request_headers(ngx_event_t * rev);
static ssize_t ngx_http_read_request_header(ngx_http_request_t * r);
static ngx_int_t ngx_http_alloc_large_header_buffer(ngx_http_request_t * r, ngx_uint_t request_line);
static ngx_int_t ngx_http_process_header_line(ngx_http_request_t * r, ngx_table_elt_t * h, ngx_uint_t offset);
static ngx_int_t ngx_http_process_unique_header_line(ngx_http_request_t * r, ngx_table_elt_t * h, ngx_uint_t offset);
static ngx_int_t ngx_http_process_multi_header_lines(ngx_http_request_t * r, ngx_table_elt_t * h, ngx_uint_t offset);
static ngx_int_t ngx_http_process_host(ngx_http_request_t * r, ngx_table_elt_t * h, ngx_uint_t offset);
static ngx_int_t ngx_http_process_connection(ngx_http_request_t * r, ngx_table_elt_t * h, ngx_uint_t offset);
static ngx_int_t ngx_http_process_user_agent(ngx_http_request_t * r, ngx_table_elt_t * h, ngx_uint_t offset);
static ngx_int_t ngx_http_validate_host(ngx_str_t * host, ngx_pool_t * pool, ngx_uint_t alloc);
static ngx_int_t ngx_http_set_virtual_server(ngx_http_request_t * r, ngx_str_t * host);
static ngx_int_t ngx_http_find_virtual_server(ngx_connection_t * c, ngx_http_virtual_names_t * virtual_names, ngx_str_t * host, ngx_http_request_t * r, ngx_http_core_srv_conf_t ** cscfp);
static void ngx_http_request_handler(ngx_event_t * ev);
static void ngx_http_terminate_request(ngx_http_request_t * r, ngx_int_t rc);
static void ngx_http_terminate_handler(ngx_http_request_t * r);
static void FASTCALL ngx_http_finalize_connection(ngx_http_request_t * r);
static ngx_int_t ngx_http_set_write_handler(ngx_http_request_t * r);
static void ngx_http_writer(ngx_http_request_t * r);
static void ngx_http_request_finalizer(ngx_http_request_t * r);
static void ngx_http_set_keepalive(ngx_http_request_t * r);
static void ngx_http_keepalive_handler(ngx_event_t * ev);
static void ngx_http_set_lingering_close(ngx_http_request_t * r);
static void ngx_http_lingering_close_handler(ngx_event_t * ev);
static ngx_int_t ngx_http_post_action(ngx_http_request_t * r);
static void FASTCALL ngx_http_close_request(ngx_http_request_t * r, ngx_int_t error);
static void ngx_http_log_request(ngx_http_request_t * r);
static uchar * ngx_http_log_error(ngx_log_t * log, uchar * buf, size_t len);
static uchar * ngx_http_log_error_handler(ngx_http_request_t * r, ngx_http_request_t * sr, uchar * buf, size_t len);

#if (NGX_HTTP_SSL)
static void ngx_http_ssl_handshake(ngx_event_t * rev);
static void ngx_http_ssl_handshake_handler(ngx_connection_t * c);
#endif

static const char * ngx_http_client_errors[] = {
	"client sent invalid method", // NGX_HTTP_PARSE_INVALID_METHOD 
	"client sent invalid request", // NGX_HTTP_PARSE_INVALID_REQUEST 
	"client sent invalid version", // NGX_HTTP_PARSE_INVALID_VERSION 
	"client sent invalid method in HTTP/0.9 request" // NGX_HTTP_PARSE_INVALID_09_METHOD 
};

ngx_http_header_t ngx_http_headers_in[] = {
	{ ngx_string("Host"), offsetof(ngx_http_headers_in_t, host), ngx_http_process_host },
	{ ngx_string("Connection"), offsetof(ngx_http_headers_in_t, connection), ngx_http_process_connection },
	{ ngx_string("If-Modified-Since"), offsetof(ngx_http_headers_in_t, if_modified_since), ngx_http_process_unique_header_line },
	{ ngx_string("If-Unmodified-Since"), offsetof(ngx_http_headers_in_t, if_unmodified_since), ngx_http_process_unique_header_line },
	{ ngx_string("If-Match"), offsetof(ngx_http_headers_in_t, if_match), ngx_http_process_unique_header_line },
	{ ngx_string("If-None-Match"), offsetof(ngx_http_headers_in_t, if_none_match), ngx_http_process_unique_header_line },
	{ ngx_string("User-Agent"), offsetof(ngx_http_headers_in_t, user_agent), ngx_http_process_user_agent },
	{ ngx_string("Referer"), offsetof(ngx_http_headers_in_t, referer), ngx_http_process_header_line },
	{ ngx_string("Content-Length"), offsetof(ngx_http_headers_in_t, content_length), ngx_http_process_unique_header_line },
	{ ngx_string("Content-Range"), offsetof(ngx_http_headers_in_t, content_range), ngx_http_process_unique_header_line },
	{ ngx_string("Content-Type"), offsetof(ngx_http_headers_in_t, content_type), ngx_http_process_header_line },
	{ ngx_string("Range"), offsetof(ngx_http_headers_in_t, range), ngx_http_process_header_line },
	{ ngx_string("If-Range"), offsetof(ngx_http_headers_in_t, if_range), ngx_http_process_unique_header_line },
	{ ngx_string("Transfer-Encoding"), offsetof(ngx_http_headers_in_t, transfer_encoding), ngx_http_process_header_line },
	{ ngx_string("Expect"), offsetof(ngx_http_headers_in_t, expect), ngx_http_process_unique_header_line },
	{ ngx_string("Upgrade"), offsetof(ngx_http_headers_in_t, upgrade), ngx_http_process_header_line },
#if (NGX_HTTP_GZIP)
	{ngx_string("Accept-Encoding"), offsetof(ngx_http_headers_in_t, accept_encoding), ngx_http_process_header_line },
	{ ngx_string("Via"), offsetof(ngx_http_headers_in_t, via), ngx_http_process_header_line },
#endif
	{ ngx_string("Authorization"), offsetof(ngx_http_headers_in_t, authorization), ngx_http_process_unique_header_line },
	{ ngx_string("Keep-Alive"), offsetof(ngx_http_headers_in_t, keep_alive), ngx_http_process_header_line },
#if (NGX_HTTP_X_FORWARDED_FOR)
	{ngx_string("X-Forwarded-For"), offsetof(ngx_http_headers_in_t, x_forwarded_for), ngx_http_process_multi_header_lines },
#endif
#if (NGX_HTTP_REALIP)
	{ngx_string("X-Real-IP"), offsetof(ngx_http_headers_in_t, x_real_ip), ngx_http_process_header_line },
#endif
#if (NGX_HTTP_HEADERS)
	{ngx_string("Accept"), offsetof(ngx_http_headers_in_t, accept), ngx_http_process_header_line },
	{ ngx_string("Accept-Language"), offsetof(ngx_http_headers_in_t, accept_language), ngx_http_process_header_line },
#endif
#if (NGX_HTTP_DAV)
	{ ngx_string("Depth"), offsetof(ngx_http_headers_in_t, depth), ngx_http_process_header_line },
	{ ngx_string("Destination"), offsetof(ngx_http_headers_in_t, destination), ngx_http_process_header_line },
	{ ngx_string("Overwrite"), offsetof(ngx_http_headers_in_t, overwrite), ngx_http_process_header_line },
	{ ngx_string("Date"), offsetof(ngx_http_headers_in_t, date), ngx_http_process_header_line },
#endif
	{ ngx_string("Cookie"), offsetof(ngx_http_headers_in_t, cookies), ngx_http_process_multi_header_lines },
	{ ngx_null_string, 0, NULL }
};

void ngx_http_init_connection(ngx_connection_t * c)
{
	ngx_uint_t i;
	struct sockaddr_in * sin;
	ngx_http_port_t * port;
	ngx_http_in_addr_t * addr;
	ngx_http_log_ctx_t * ctx;
	ngx_http_connection_t * hc = (ngx_http_connection_t *)ngx_pcalloc(c->pool, sizeof(ngx_http_connection_t));
	THROW(hc);
	c->data = hc;
	// find the server configuration for the address:port 
	port = (ngx_http_port_t *)c->listening->servers;
	if(port->naddrs > 1) {
		// 
		// there are several addresses on this port and one of them
		// is an "*:port" wildcard so getsockname() in ngx_http_server_addr()
		// is required to determine a server address
		// 
		THROW(ngx_connection_local_sockaddr(c, NULL, 0) == NGX_OK);
		switch(c->local_sockaddr->sa_family) {
#if (NGX_HAVE_INET6)
			case AF_INET6:
				{
					struct sockaddr_in6 * sin6 = (struct sockaddr_in6*)c->local_sockaddr;
					ngx_http_in6_addr_t * addr6 = static_cast<ngx_http_in6_addr_t *>(port->addrs);
					// the last address is "*" 
					for(i = 0; i < port->naddrs - 1; i++) {
						if(memcmp(&addr6[i].addr6, &sin6->sin6_addr, 16) == 0) {
							break;
						}
					}
					hc->addr_conf = &addr6[i].conf;
				}
			    break;
#endif
			default: // AF_INET 
			    sin = (struct sockaddr_in*)c->local_sockaddr;
			    addr = static_cast<ngx_http_in_addr_t *>(port->addrs);
			    // the last address is "*" 
			    for(i = 0; i < port->naddrs - 1; i++) {
				    if(addr[i].addr == sin->sin_addr.s_addr) {
					    break;
				    }
			    }
			    hc->addr_conf = &addr[i].conf;
			    break;
		}
	}
	else {
		switch(c->local_sockaddr->sa_family) {
#if (NGX_HAVE_INET6)
			case AF_INET6:
				{
					ngx_http_in6_addr_t * addr6 = static_cast<ngx_http_in6_addr_t *>(port->addrs);
					hc->addr_conf = &addr6[0].conf;
				}
			    break;
#endif
			default: // AF_INET 
			    addr = static_cast<ngx_http_in_addr_t *>(port->addrs);
			    hc->addr_conf = &addr[0].conf;
			    break;
		}
	}
	// the default server configuration for the address:port 
	hc->conf_ctx = hc->addr_conf->default_server->ctx;
	THROW(ctx = (ngx_http_log_ctx_t *)ngx_palloc(c->pool, sizeof(ngx_http_log_ctx_t)));
	ctx->connection = c;
	ctx->request = NULL;
	ctx->current_request = NULL;
	c->log->connection = c->number;
	c->log->handler = ngx_http_log_error;
	c->log->data = ctx;
	c->log->action = "waiting for request";
	c->log_error = NGX_ERROR_INFO;
	{
		ngx_event_t * rev = c->P_EvRd;
		rev->F_EvHandler = ngx_http_wait_request_handler;
		c->P_EvWr->F_EvHandler = ngx_http_empty_handler;
#if (NGX_HTTP_V2)
		if(hc->addr_conf->http2)
			rev->F_EvHandler = ngx_http_v2_init;
#endif
#if (NGX_HTTP_SSL)
		{
			ngx_http_ssl_srv_conf_t  * sscf = (ngx_http_ssl_srv_conf_t *)ngx_http_get_module_srv_conf(hc->conf_ctx, ngx_http_ssl_module);
			if(sscf->enable || hc->addr_conf->ssl) {
				c->log->action = "SSL handshaking";
				if(hc->addr_conf->ssl && sscf->ssl.ctx == NULL) {
					ngx_log_error(NGX_LOG_ERR, c->log, 0, "no \"ssl_certificate\" is defined in server listening on SSL port");
					CALLEXCEPT();
				}
				hc->ssl = 1;
				rev->F_EvHandler = ngx_http_ssl_handshake;
			}
		}
#endif
		if(hc->addr_conf->proxy_protocol) {
			hc->proxy_protocol = 1;
			c->log->action = "reading PROXY protocol";
		}
		if(rev->ready) {
			// the deferred accept(), iocp 
			if(ngx_use_accept_mutex) {
				ngx_post_event(rev, &ngx_posted_events);
			}
			else {
				rev->F_EvHandler(rev);
			}
		}
		else {
			ngx_add_timer(rev, c->listening->post_accept_timeout);
			ngx_reusable_connection(c, 1);
			THROW(ngx_handle_read_event(rev, 0) == NGX_OK);
		}
	}
	CATCH
		ngx_http_close_connection(c);
	ENDCATCH
}

static void ngx_http_wait_request_handler(ngx_event_t * rev)
{
	uchar * p;
	ssize_t n;
	ngx_connection_t * c = (ngx_connection_t *)rev->P_Data;
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http wait request handler");
	if(rev->timedout) {
		ngx_log_error(NGX_LOG_INFO, c->log, NGX_ETIMEDOUT, "client timed out");
		CALLEXCEPT();
	}
	THROW(!c->close);
	{
		ngx_http_connection_t * hc = (ngx_http_connection_t *)c->data;
		ngx_http_core_srv_conf_t  * cscf = (ngx_http_core_srv_conf_t *)ngx_http_get_module_srv_conf(hc->conf_ctx, ngx_http_core_module);
		size_t size = cscf->client_header_buffer_size;
		ngx_buf_t * b = c->buffer;
		if(!b) {
			THROW(b = ngx_create_temp_buf(c->pool, size));
			c->buffer = b;
		}
		else if(b->start == NULL) {
			b->start = (uchar *)ngx_palloc(c->pool, size);
			THROW(b->start);
			b->pos = b->start;
			b->last = b->start;
			b->end = b->last + size;
		}
		n = c->recv(c, b->last, size);
		if(n == NGX_AGAIN) {
			if(!rev->timer_set) {
				ngx_add_timer(rev, c->listening->post_accept_timeout);
				ngx_reusable_connection(c, 1);
			}
			THROW(ngx_handle_read_event(rev, 0) == NGX_OK);
			// 
			// We are trying to not hold c->buffer's memory for an idle connection.
			// 
			if(ngx_pfree(c->pool, b->start) == NGX_OK)
				b->start = NULL;
		}
		THROW(n != NGX_ERROR);
		if(n == 0) {
			ngx_log_error(NGX_LOG_INFO, c->log, 0, "client closed connection");
			CALLEXCEPT();
		}
		{
			b->last += n;
			if(hc->proxy_protocol) {
				hc->proxy_protocol = 0;
				THROW(p = ngx_proxy_protocol_read(c, b->pos, b->last));
				b->pos = p;
				if(b->pos == b->last) {
					c->log->action = "waiting for request";
					b->pos = b->start;
					b->last = b->start;
					ngx_post_event(rev, &ngx_posted_events);
					return;
				}
			}
			c->log->action = "reading client request line";
			ngx_reusable_connection(c, 0);
			THROW(c->data = ngx_http_create_request(c));
			rev->F_EvHandler = ngx_http_process_request_line;
			ngx_http_process_request_line(rev);
		}
	}
	CATCH
		ngx_http_close_connection(c);
	ENDCATCH
}

ngx_http_request_t * ngx_http_create_request(ngx_connection_t * c)
{
	ngx_pool_t * pool;
	ngx_time_t * tp;
	ngx_http_request_t  * r;
	ngx_http_log_ctx_t  * ctx;
	ngx_http_connection_t * hc;
	ngx_http_core_srv_conf_t * cscf;
	ngx_http_core_loc_conf_t * clcf;
	ngx_http_core_main_conf_t  * cmcf;
	c->requests++;
	hc = (ngx_http_connection_t *)c->data;
	cscf = (ngx_http_core_srv_conf_t *)ngx_http_get_module_srv_conf(hc->conf_ctx, ngx_http_core_module);
	pool = ngx_create_pool(cscf->request_pool_size, c->log);
	if(pool == NULL) {
		return NULL;
	}
	r = (ngx_http_request_t *)ngx_pcalloc(pool, sizeof(ngx_http_request_t));
	if(r == NULL) {
		ngx_destroy_pool(pool);
		return NULL;
	}
	r->pool = pool;
	r->http_connection = hc;
	r->signature = NGX_HTTP_MODULE;
	r->connection = c;
	r->main_conf = hc->conf_ctx->main_conf;
	r->srv_conf = hc->conf_ctx->srv_conf;
	r->loc_conf = hc->conf_ctx->loc_conf;
	r->read_event_handler = ngx_http_block_reading;
	clcf = (ngx_http_core_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_core_module);
	//ngx_set_connection_log(r->connection, clcf->error_log);
	r->connection->SetLog(clcf->error_log);
	r->header_in = hc->busy ? hc->busy->buf : c->buffer;
	if(ngx_list_init(&r->headers_out.headers, r->pool, 20, sizeof(ngx_table_elt_t)) != NGX_OK) {
		ngx_destroy_pool(r->pool);
		return NULL;
	}
	if(ngx_list_init(&r->headers_out.trailers, r->pool, 4, sizeof(ngx_table_elt_t)) != NGX_OK) {
		ngx_destroy_pool(r->pool);
		return NULL;
	}
	r->ctx = (void **)ngx_pcalloc(r->pool, sizeof(void *) * ngx_http_max_module);
	if(r->ctx == NULL) {
		ngx_destroy_pool(r->pool);
		return NULL;
	}
	cmcf = (ngx_http_core_main_conf_t *)ngx_http_get_module_main_conf(r, ngx_http_core_module);
	r->variables = (ngx_http_variable_value_t *)ngx_pcalloc(r->pool, cmcf->variables.nelts * sizeof(ngx_http_variable_value_t));
	if(r->variables == NULL) {
		ngx_destroy_pool(r->pool);
		return NULL;
	}
#if (NGX_HTTP_SSL)
	if(c->ssl) {
		r->main_filter_need_in_memory = 1;
	}
#endif
	r->main = r;
	r->count = 1;
	tp = ngx_timeofday();
	r->start_sec = tp->sec;
	r->start_msec = tp->msec;
	r->method = NGX_HTTP_UNKNOWN;
	r->http_version = NGX_HTTP_VERSION_10;
	r->headers_in.content_length_n = -1;
	r->headers_in.keep_alive_n = -1;
	r->headers_out.content_length_n = -1;
	r->headers_out.last_modified_time = -1;
	r->uri_changes = NGX_HTTP_MAX_URI_CHANGES + 1;
	r->subrequests = NGX_HTTP_MAX_SUBREQUESTS + 1;
	r->http_state = NGX_HTTP_READING_REQUEST_STATE;
	ctx = (ngx_http_log_ctx_t *)c->log->data;
	ctx->request = r;
	ctx->current_request = r;
	r->log_handler = ngx_http_log_error_handler;
#if (NGX_STAT_STUB)
	(void)ngx_atomic_fetch_add(ngx_stat_reading, 1);
	r->stat_reading = 1;
	(void)ngx_atomic_fetch_add(ngx_stat_requests, 1);
#endif
	return r;
}

#if (NGX_HTTP_SSL)

static void ngx_http_ssl_handshake(ngx_event_t * rev)
{
	uchar  * p, buf[NGX_PROXY_PROTOCOL_MAX_HEADER + 1];
	ngx_int_t rc;
	ngx_http_core_loc_conf_t  * clcf;
	ngx_connection_t * c = (ngx_connection_t *)rev->P_Data;
	ngx_http_connection_t * hc = (ngx_http_connection_t *)c->data;
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, rev->log, 0, "http check ssl handshake");
	if(rev->timedout) {
		ngx_log_error(NGX_LOG_INFO, c->log, NGX_ETIMEDOUT, "client timed out");
		ngx_http_close_connection(c);
	}
	else if(c->close)
		ngx_http_close_connection(c);
	else {
		size_t size = hc->proxy_protocol ? sizeof(buf) : 1;
		ssize_t n = recv(c->fd, (char *)buf, size, MSG_PEEK);
		ngx_err_t err = ngx_socket_errno;
		ngx_log_debug1(NGX_LOG_DEBUG_HTTP, rev->log, 0, "http recv(): %z", n);
		if(n == -1) {
			if(err == NGX_EAGAIN) {
				rev->ready = 0;
				if(!rev->timer_set) {
					ngx_add_timer(rev, c->listening->post_accept_timeout);
					ngx_reusable_connection(c, 1);
				}
				if(ngx_handle_read_event(rev, 0) != NGX_OK) {
					ngx_http_close_connection(c);
				}
			}
			else {
				ngx_connection_error(c, err, "recv() failed");
				ngx_http_close_connection(c);
			}
		}
		else {
			if(hc->proxy_protocol) {
				hc->proxy_protocol = 0;
				p = ngx_proxy_protocol_read(c, buf, buf + n);
				if(!p) {
					ngx_http_close_connection(c);
					return;
				}
				else {
					size = p - buf;
					if(c->recv(c, buf, size) != (ssize_t)size) {
						ngx_http_close_connection(c);
						return;
					}
					else {
						c->log->action = "SSL handshaking";
						if(n == (ssize_t)size) {
							ngx_post_event(rev, &ngx_posted_events);
							return;
						}
						else {
							n = 1;
							buf[0] = *p;
						}
					}
				}
			}
			if(n == 1) {
				if(buf[0] & 0x80 /* SSLv2 */ || buf[0] == 0x16 /* SSLv3/TLSv1 */) {
					ngx_log_debug1(NGX_LOG_DEBUG_HTTP, rev->log, 0, "https ssl handshake: 0x%02Xd", buf[0]);
					clcf = (ngx_http_core_loc_conf_t *)ngx_http_get_module_loc_conf(hc->conf_ctx, ngx_http_core_module);
					if(clcf->tcp_nodelay && ngx_tcp_nodelay(c) != NGX_OK)
						ngx_http_close_connection(c);
					else {
						ngx_http_ssl_srv_conf_t * sscf = (ngx_http_ssl_srv_conf_t *)ngx_http_get_module_srv_conf(hc->conf_ctx, ngx_http_ssl_module);
						if(ngx_ssl_create_connection(&sscf->ssl, c, NGX_SSL_BUFFER) != NGX_OK)
							ngx_http_close_connection(c);
						else {
							rc = ngx_ssl_handshake(c);
							if(rc == NGX_AGAIN) {
								if(!rev->timer_set) {
									ngx_add_timer(rev, c->listening->post_accept_timeout);
								}
								ngx_reusable_connection(c, 0);
								c->ssl->handler = ngx_http_ssl_handshake_handler;
							}
							else
								ngx_http_ssl_handshake_handler(c);
						}
					}
				}
				else {
					ngx_log_debug0(NGX_LOG_DEBUG_HTTP, rev->log, 0, "plain http");
					c->log->action = "waiting for request";
					rev->F_EvHandler = ngx_http_wait_request_handler;
					ngx_http_wait_request_handler(rev);
				}
			}
			else {
				ngx_log_error(NGX_LOG_INFO, c->log, 0, "client closed connection");
				ngx_http_close_connection(c);
			}
		}
	}
}

static void ngx_http_ssl_handshake_handler(ngx_connection_t * c)
{
	if(c->ssl->handshaked) {
		/*
		 * The majority of browsers do not send the "close notify" alert.
		 * Among them are MSIE, old Mozilla, Netscape 4, Konqueror,
		 * and Links.  And what is more, MSIE ignores the server's alert.
		 *
		 * Opera and recent Mozilla send the alert.
		 */
		c->ssl->no_wait_shutdown = 1;
#if (NGX_HTTP_V2 && (defined TLSEXT_TYPE_application_layer_protocol_negotiation || defined TLSEXT_TYPE_next_proto_neg))
		{
			unsigned int len;
			const uchar  * data;
			ngx_http_connection_t  * hc = (ngx_http_connection_t *)c->data;
			if(hc->addr_conf->http2) {
#ifdef TLSEXT_TYPE_application_layer_protocol_negotiation
				SSL_get0_alpn_selected(c->ssl->connection, &data, &len);
	#ifdef TLSEXT_TYPE_next_proto_neg
				if(!len) {
					SSL_get0_next_proto_negotiated(c->ssl->connection, &data, &len);
				}
	#endif
#else /* TLSEXT_TYPE_next_proto_neg */
				SSL_get0_next_proto_negotiated(c->ssl->connection, &data, &len);
#endif

				if(len == 2 && data[0] == 'h' && data[1] == '2') {
					ngx_http_v2_init(c->P_EvRd);
					return;
				}
			}
		}
#endif
		c->log->action = "waiting for request";
		c->P_EvRd->F_EvHandler = ngx_http_wait_request_handler;
		/* STUB: epoll edge */ c->P_EvWr->F_EvHandler = ngx_http_empty_handler;
		ngx_reusable_connection(c, 1);
		ngx_http_wait_request_handler(c->P_EvRd);
		return;
	}
	if(c->P_EvRd->timedout) {
		ngx_log_error(NGX_LOG_INFO, c->log, NGX_ETIMEDOUT, "client timed out");
	}
	ngx_http_close_connection(c);
}

#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME

int ngx_http_ssl_servername(ngx_ssl_conn_t * ssl_conn, int * ad, void * arg)
{
	ngx_str_t host;
	ngx_connection_t   * c;
	ngx_http_connection_t   * hc;
	ngx_http_ssl_srv_conf_t * sscf;
	ngx_http_core_loc_conf_t  * clcf;
	ngx_http_core_srv_conf_t  * cscf;
	const char  * servername = SSL_get_servername(ssl_conn, TLSEXT_NAMETYPE_host_name);
	if(servername == NULL) {
		return SSL_TLSEXT_ERR_NOACK;
	}
	c = (ngx_connection_t *)ngx_ssl_get_connection(ssl_conn);
	if(c->ssl->renegotiation) {
		return SSL_TLSEXT_ERR_NOACK;
	}
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0, "SSL server name: \"%s\"", servername);
	host.len = ngx_strlen(servername);
	if(host.len == 0) {
		return SSL_TLSEXT_ERR_NOACK;
	}
	host.data = (uchar *)servername;
	if(ngx_http_validate_host(&host, c->pool, 1) != NGX_OK) {
		return SSL_TLSEXT_ERR_NOACK;
	}
	hc = (ngx_http_connection_t *)c->data;
	if(ngx_http_find_virtual_server(c, hc->addr_conf->virtual_names, &host, NULL, &cscf) != NGX_OK) {
		return SSL_TLSEXT_ERR_NOACK;
	}
	hc->ssl_servername = (ngx_str_t *)ngx_palloc(c->pool, sizeof(ngx_str_t));
	if(hc->ssl_servername == NULL) {
		return SSL_TLSEXT_ERR_NOACK;
	}
	*hc->ssl_servername = host;
	hc->conf_ctx = cscf->ctx;
	clcf = (ngx_http_core_loc_conf_t *)ngx_http_get_module_loc_conf(hc->conf_ctx, ngx_http_core_module);
	//ngx_set_connection_log(c, clcf->error_log);
	c->SetLog(clcf->error_log);
	sscf = (ngx_http_ssl_srv_conf_t *)ngx_http_get_module_srv_conf(hc->conf_ctx, ngx_http_ssl_module);
	c->ssl->buffer_size = sscf->buffer_size;
	if(sscf->ssl.ctx) {
		SSL_set_SSL_CTX(ssl_conn, sscf->ssl.ctx);
		/*
		 * SSL_set_SSL_CTX() only changes certs as of 1.0.0d
		 * adjust other things we care about
		 */
		SSL_set_verify(ssl_conn, SSL_CTX_get_verify_mode(sscf->ssl.ctx), SSL_CTX_get_verify_callback(sscf->ssl.ctx));
		SSL_set_verify_depth(ssl_conn, SSL_CTX_get_verify_depth(sscf->ssl.ctx));
#ifdef SSL_CTRL_CLEAR_OPTIONS
		/* only in 0.9.8m+ */
		SSL_clear_options(ssl_conn, SSL_get_options(ssl_conn) & ~SSL_CTX_get_options(sscf->ssl.ctx));
#endif
		SSL_set_options(ssl_conn, SSL_CTX_get_options(sscf->ssl.ctx));
	}
	return SSL_TLSEXT_ERR_OK;
}

#endif

#endif

static void ngx_http_process_request_line(ngx_event_t * rev)
{
	ssize_t n;
	ngx_int_t rc, rv;
	ngx_str_t host;
	ngx_connection_t  * c = (ngx_connection_t *)rev->P_Data;
	ngx_http_request_t  * r = (ngx_http_request_t *)c->data;
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, rev->log, 0, "http process request line");
	if(rev->timedout) {
		ngx_log_error(NGX_LOG_INFO, c->log, NGX_ETIMEDOUT, "client timed out");
		c->timedout = 1;
		ngx_http_close_request(r, NGX_HTTP_REQUEST_TIME_OUT);
		return;
	}
	rc = NGX_AGAIN;
	for(;;) {
		if(rc == NGX_AGAIN) {
			n = ngx_http_read_request_header(r);
			if(n == NGX_AGAIN || n == NGX_ERROR) {
				return;
			}
		}
		rc = ngx_http_parse_request_line(r, r->header_in);
		if(rc == NGX_OK) {
			/* the request line has been parsed successfully */
			r->request_line.len = r->request_end - r->request_start;
			r->request_line.data = r->request_start;
			r->request_length = r->header_in->pos - r->request_start;
			ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0, "http request line: \"%V\"", &r->request_line);
			r->method_name.len = r->method_end - r->request_start + 1;
			r->method_name.data = r->request_line.data;
			if(r->http_protocol.data) {
				r->http_protocol.len = r->request_end - r->http_protocol.data;
			}
			if(ngx_http_process_request_uri(r) != NGX_OK) {
				return;
			}
			if(r->host_start && r->host_end) {
				host.len = r->host_end - r->host_start;
				host.data = r->host_start;
				rc = ngx_http_validate_host(&host, r->pool, 0);
				if(rc == NGX_DECLINED) {
					ngx_log_error(NGX_LOG_INFO, c->log, 0, "client sent invalid host in request line");
					ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
					return;
				}
				if(rc == NGX_ERROR) {
					ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
					return;
				}
				if(ngx_http_set_virtual_server(r, &host) == NGX_ERROR) {
					return;
				}
				r->headers_in.server = host;
			}
			if(r->http_version < NGX_HTTP_VERSION_10) {
				if(r->headers_in.server.len == 0 && ngx_http_set_virtual_server(r, &r->headers_in.server) == NGX_ERROR) {
					return;
				}
				ngx_http_process_request(r);
				return;
			}
			if(ngx_list_init(&r->headers_in.headers, r->pool, 20, sizeof(ngx_table_elt_t)) != NGX_OK) {
				ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
				return;
			}
			c->log->action = "reading client request headers";
			rev->F_EvHandler = ngx_http_process_request_headers;
			ngx_http_process_request_headers(rev);
			return;
		}
		if(rc != NGX_AGAIN) {
			// there was error while a request line parsing 
			ngx_log_error(NGX_LOG_INFO, c->log, 0, ngx_http_client_errors[rc - NGX_HTTP_CLIENT_ERROR]);
			if(rc == NGX_HTTP_PARSE_INVALID_VERSION)
				ngx_http_finalize_request(r, NGX_HTTP_VERSION_NOT_SUPPORTED);
			else
				ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
			return;
		}
		/* NGX_AGAIN: a request line parsing is still incomplete */
		if(r->header_in->pos == r->header_in->end) {
			rv = ngx_http_alloc_large_header_buffer(r, 1);
			if(rv == NGX_ERROR) {
				ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
				return;
			}
			if(rv == NGX_DECLINED) {
				r->request_line.len = r->header_in->end - r->request_start;
				r->request_line.data = r->request_start;
				ngx_log_error(NGX_LOG_INFO, c->log, 0, "client sent too long URI");
				ngx_http_finalize_request(r, NGX_HTTP_REQUEST_URI_TOO_LARGE);
				return;
			}
		}
	}
}

ngx_int_t ngx_http_process_request_uri(ngx_http_request_t * r)
{
	ngx_http_core_srv_conf_t * cscf;
	if(r->args_start) {
		r->uri.len = r->args_start - 1 - r->uri_start;
	}
	else {
		r->uri.len = r->uri_end - r->uri_start;
	}
	if(r->complex_uri || r->quoted_uri) {
		r->uri.data = (uchar *)ngx_pnalloc(r->pool, r->uri.len + 1);
		if(r->uri.data == NULL) {
			ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
			return NGX_ERROR;
		}
		cscf = (ngx_http_core_srv_conf_t *)ngx_http_get_module_srv_conf(r, ngx_http_core_module);
		if(ngx_http_parse_complex_uri(r, cscf->merge_slashes) != NGX_OK) {
			r->uri.len = 0;
			ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "client sent invalid request");
			ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
			return NGX_ERROR;
		}
	}
	else {
		r->uri.data = r->uri_start;
	}
	r->unparsed_uri.len = r->uri_end - r->uri_start;
	r->unparsed_uri.data = r->uri_start;
	r->valid_unparsed_uri = r->space_in_uri ? 0 : 1;
	if(r->uri_ext) {
		if(r->args_start) {
			r->exten.len = r->args_start - 1 - r->uri_ext;
		}
		else {
			r->exten.len = r->uri_end - r->uri_ext;
		}
		r->exten.data = r->uri_ext;
	}
	if(r->args_start && r->uri_end > r->args_start) {
		r->args.len = r->uri_end - r->args_start;
		r->args.data = r->args_start;
	}
#if (NGX_WIN32)
	{
		uchar  * p = r->uri.data;
		uchar  * last = r->uri.data + r->uri.len;
		while(p < last) {
			if(*p++ == ':') {
				/*
				 * this check covers "::$data", "::$index_allocation" and
				 * ":$i30:$index_allocation"
				 */
				if(p < last && *p == '$') {
					ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "client sent unsafe win32 URI");
					ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
					return NGX_ERROR;
				}
			}
		}
		p = r->uri.data + r->uri.len - 1;
		while(p > r->uri.data) {
			if(*p == ' ') {
				p--;
				continue;
			}
			if(*p == '.') {
				p--;
				continue;
			}
			break;
		}
		if(p != r->uri.data + r->uri.len - 1) {
			r->uri.len = p + 1 - r->uri.data;
			ngx_http_set_exten(r);
		}
	}
#endif
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http uri: \"%V\"", &r->uri);
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http args: \"%V\"", &r->args);
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http exten: \"%V\"", &r->exten);
	return NGX_OK;
}

static void ngx_http_process_request_headers(ngx_event_t * rev)
{
	uchar * p;
	size_t len;
	ssize_t n;
	ngx_int_t rv;
	ngx_table_elt_t  * h;
	ngx_http_header_t   * hh;
	ngx_http_core_srv_conf_t * cscf;
	ngx_connection_t * c = (ngx_connection_t *)rev->P_Data;
	ngx_http_request_t * r = (ngx_http_request_t *)c->data;
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, rev->log, 0, "http process request header line");
	if(rev->timedout) {
		ngx_log_error(NGX_LOG_INFO, c->log, NGX_ETIMEDOUT, "client timed out");
		c->timedout = 1;
		ngx_http_close_request(r, NGX_HTTP_REQUEST_TIME_OUT);
	}
	else {
		ngx_http_core_main_conf_t  * cmcf = (ngx_http_core_main_conf_t *)ngx_http_get_module_main_conf(r, ngx_http_core_module);
		ngx_int_t rc = NGX_AGAIN;
		for(;;) {
			if(rc == NGX_AGAIN) {
				if(r->header_in->pos == r->header_in->end) {
					rv = ngx_http_alloc_large_header_buffer(r, 0);
					if(rv == NGX_ERROR) {
						ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
						return;
					}
					else if(rv == NGX_DECLINED) {
						p = r->header_name_start;
						r->lingering_close = 1;
						if(!p) {
							ngx_log_error(NGX_LOG_INFO, c->log, 0, "client sent too large request");
							ngx_http_finalize_request(r, NGX_HTTP_REQUEST_HEADER_TOO_LARGE);
						}
						else {
							len = r->header_in->end - p;
							if(len > NGX_MAX_ERROR_STR - 300) {
								len = NGX_MAX_ERROR_STR - 300;
							}
							ngx_log_error(NGX_LOG_INFO, c->log, 0, "client sent too long header line: \"%*s...\"", len, r->header_name_start);
							ngx_http_finalize_request(r, NGX_HTTP_REQUEST_HEADER_TOO_LARGE);
						}
						return;
					}
				}
				n = ngx_http_read_request_header(r);
				if(n == NGX_AGAIN || n == NGX_ERROR) {
					return;
				}
			}
			// the host header could change the server configuration context 
			cscf = (ngx_http_core_srv_conf_t *)ngx_http_get_module_srv_conf(r, ngx_http_core_module);
			rc = ngx_http_parse_header_line(r, r->header_in, cscf->underscores_in_headers);
			if(rc == NGX_OK) {
				r->request_length += r->header_in->pos - r->header_name_start;
				if(r->invalid_header && cscf->ignore_invalid_headers) { // there was error while a header line parsing 
					ngx_log_error(NGX_LOG_INFO, c->log, 0, "client sent invalid header line: \"%*s\"", r->header_end - r->header_name_start, r->header_name_start);
					continue;
				}
				// a header line has been parsed successfully 
				h = (ngx_table_elt_t *)ngx_list_push(&r->headers_in.headers);
				if(!h) {
					ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
					return;
				}
				h->hash = r->header_hash;
				h->key.len = r->header_name_end - r->header_name_start;
				h->key.data = r->header_name_start;
				h->key.data[h->key.len] = '\0';
				h->value.len = r->header_end - r->header_start;
				h->value.data = r->header_start;
				h->value.data[h->value.len] = '\0';
				h->lowcase_key = (uchar *)ngx_pnalloc(r->pool, h->key.len);
				if(h->lowcase_key == NULL) {
					ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
					return;
				}
				if(h->key.len == r->lowcase_index) {
					memcpy(h->lowcase_key, r->lowcase_header, h->key.len);
				}
				else {
					ngx_strlow(h->lowcase_key, h->key.data, h->key.len);
				}
				hh = (ngx_http_header_t *)ngx_hash_find(&cmcf->headers_in_hash, h->hash, h->lowcase_key, h->key.len);
				if(hh && hh->handler(r, h, hh->offset) != NGX_OK) {
					return;
				}
				ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http header: \"%V: %V\"", &h->key, &h->value);
				continue;
			}
			if(rc == NGX_HTTP_PARSE_HEADER_DONE) { // a whole header has been parsed successfully 
				ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http header done");
				r->request_length += r->header_in->pos - r->header_name_start;
				r->http_state = NGX_HTTP_PROCESS_REQUEST_STATE;
				rc = ngx_http_process_request_header(r);
				if(rc != NGX_OK) {
					return;
				}
				ngx_http_process_request(r);
				return;
			}
			if(rc == NGX_AGAIN) { // a header line parsing is still not complete 
				continue;
			}
			/* rc == NGX_HTTP_PARSE_INVALID_HEADER */
			ngx_log_error(NGX_LOG_INFO, c->log, 0, "client sent invalid header line");
			ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
			return;
		}
	}
}

static ssize_t ngx_http_read_request_header(ngx_http_request_t * r)
{
	ngx_http_core_srv_conf_t  * cscf;
	ngx_connection_t * c = r->connection;
	ngx_event_t * rev = c->P_EvRd;
	ssize_t n = r->header_in->last - r->header_in->pos;
	if(n > 0) {
		return n;
	}
	if(rev->ready) {
		n = c->recv(c, r->header_in->last, r->header_in->end - r->header_in->last);
	}
	else {
		n = NGX_AGAIN;
	}
	if(n == NGX_AGAIN) {
		if(!rev->timer_set) {
			cscf = (ngx_http_core_srv_conf_t *)ngx_http_get_module_srv_conf(r, ngx_http_core_module);
			ngx_add_timer(rev, cscf->client_header_timeout);
		}
		if(ngx_handle_read_event(rev, 0) != NGX_OK) {
			ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
			return NGX_ERROR;
		}
		return NGX_AGAIN;
	}
	if(n == 0) {
		ngx_log_error(NGX_LOG_INFO, c->log, 0, "client prematurely closed connection");
	}
	if(n == 0 || n == NGX_ERROR) {
		c->error = 1;
		c->log->action = "reading client request headers";
		ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
		return NGX_ERROR;
	}
	r->header_in->last += n;
	return n;
}

static ngx_int_t ngx_http_alloc_large_header_buffer(ngx_http_request_t * r, ngx_uint_t request_line)
{
	uchar  * old;
	uchar * p_new;
	ngx_buf_t * b;
	ngx_chain_t * cl;
	ngx_http_connection_t   * hc;
	ngx_http_core_srv_conf_t  * cscf;
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http alloc large header buffer");
	if(request_line && r->state == 0) {
		/* the client fills up the buffer with "\r\n" */
		r->header_in->pos = r->header_in->start;
		r->header_in->last = r->header_in->start;
		return NGX_OK;
	}
	old = request_line ? r->request_start : r->header_name_start;
	cscf = (ngx_http_core_srv_conf_t *)ngx_http_get_module_srv_conf(r, ngx_http_core_module);
	if(r->state != 0 && (size_t)(r->header_in->pos - old) >= cscf->large_client_header_buffers.size) {
		return NGX_DECLINED;
	}
	hc = r->http_connection;
	if(hc->free) {
		cl = hc->free;
		hc->free = cl->next;
		b = cl->buf;
		ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http large header free: %p %uz", b->pos, b->end - b->last);
	}
	else if(hc->nbusy < cscf->large_client_header_buffers.num) {
		b = ngx_create_temp_buf(r->connection->pool, cscf->large_client_header_buffers.size);
		if(!b) {
			return NGX_ERROR;
		}
		cl = ngx_alloc_chain_link(r->connection->pool);
		if(cl == NULL) {
			return NGX_ERROR;
		}
		cl->buf = b;
		ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http large header alloc: %p %uz", b->pos, b->end - b->last);
	}
	else {
		return NGX_DECLINED;
	}
	cl->next = hc->busy;
	hc->busy = cl;
	hc->nbusy++;
	if(r->state == 0) {
		/*
		 * r->state == 0 means that a header line was parsed successfully
		 * and we do not need to copy incomplete header line and
		 * to relocate the parser header pointers
		 */
		r->header_in = b;
		return NGX_OK;
	}
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http large header copy: %uz", r->header_in->pos - old);
	p_new = b->start;
	memcpy(p_new, old, r->header_in->pos - old);
	b->pos = p_new + (r->header_in->pos - old);
	b->last = p_new + (r->header_in->pos - old);
	if(request_line) {
		r->request_start = p_new;
		if(r->request_end) {
			r->request_end = p_new + (r->request_end - old);
		}
		r->method_end = p_new + (r->method_end - old);
		r->uri_start = p_new + (r->uri_start - old);
		r->uri_end = p_new + (r->uri_end - old);
		if(r->schema_start) {
			r->schema_start = p_new + (r->schema_start - old);
			r->schema_end = p_new + (r->schema_end - old);
		}
		if(r->host_start) {
			r->host_start = p_new + (r->host_start - old);
			if(r->host_end) {
				r->host_end = p_new + (r->host_end - old);
			}
		}
		if(r->port_start) {
			r->port_start = p_new + (r->port_start - old);
			r->port_end = p_new + (r->port_end - old);
		}
		if(r->uri_ext) {
			r->uri_ext = p_new + (r->uri_ext - old);
		}
		if(r->args_start) {
			r->args_start = p_new + (r->args_start - old);
		}
		if(r->http_protocol.data) {
			r->http_protocol.data = p_new + (r->http_protocol.data - old);
		}
	}
	else {
		r->header_name_start = p_new;
		r->header_name_end = p_new + (r->header_name_end - old);
		r->header_start = p_new + (r->header_start - old);
		r->header_end = p_new + (r->header_end - old);
	}
	r->header_in = b;
	return NGX_OK;
}

static ngx_int_t ngx_http_process_header_line(ngx_http_request_t * r, ngx_table_elt_t * h, ngx_uint_t offset)
{
	ngx_table_elt_t  ** ph = (ngx_table_elt_t**)((char *)&r->headers_in + offset);
	if(*ph == NULL) {
		*ph = h;
	}
	return NGX_OK;
}

static ngx_int_t ngx_http_process_unique_header_line(ngx_http_request_t * r, ngx_table_elt_t * h, ngx_uint_t offset)
{
	ngx_table_elt_t  ** ph = (ngx_table_elt_t**)((char *)&r->headers_in + offset);
	if(*ph == NULL) {
		*ph = h;
		return NGX_OK;
	}
	ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "client sent duplicate header line: \"%V: %V\", previous value: \"%V: %V\"",
	    &h->key, &h->value, &(*ph)->key, &(*ph)->value);
	ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
	return NGX_ERROR;
}

static ngx_int_t ngx_http_process_host(ngx_http_request_t * r, ngx_table_elt_t * h, ngx_uint_t offset)
{
	ngx_int_t rc;
	ngx_str_t host;
	if(r->headers_in.host == NULL) {
		r->headers_in.host = h;
	}
	host = h->value;
	rc = ngx_http_validate_host(&host, r->pool, 0);
	if(rc == NGX_DECLINED) {
		ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "client sent invalid host header");
		ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
		return NGX_ERROR;
	}
	if(rc == NGX_ERROR) {
		ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
		return NGX_ERROR;
	}
	if(r->headers_in.server.len) {
		return NGX_OK;
	}
	if(ngx_http_set_virtual_server(r, &host) == NGX_ERROR) {
		return NGX_ERROR;
	}
	r->headers_in.server = host;
	return NGX_OK;
}

static ngx_int_t ngx_http_process_connection(ngx_http_request_t * r, ngx_table_elt_t * h, ngx_uint_t offset)
{
	if(ngx_strcasestrn(h->value.data, "close", 5 - 1)) {
		r->headers_in.connection_type = NGX_HTTP_CONNECTION_CLOSE;
	}
	else if(ngx_strcasestrn(h->value.data, "keep-alive", 10 - 1)) {
		r->headers_in.connection_type = NGX_HTTP_CONNECTION_KEEP_ALIVE;
	}
	return NGX_OK;
}

static ngx_int_t ngx_http_process_user_agent(ngx_http_request_t * r, ngx_table_elt_t * h, ngx_uint_t offset)
{
	uchar  * user_agent, * msie;
	if(r->headers_in.user_agent) {
		return NGX_OK;
	}
	r->headers_in.user_agent = h;
	/* check some widespread browsers while the header is in CPU cache */
	user_agent = h->value.data;
	msie = ngx_strstrn(user_agent, "MSIE ", 5 - 1);
	if(msie && msie + 7 < user_agent + h->value.len) {
		r->headers_in.msie = 1;
		if(msie[6] == '.') {
			switch(msie[5]) {
				case '4':
				case '5':
				    r->headers_in.msie6 = 1;
				    break;
				case '6':
				    if(ngx_strstrn(msie + 8, "SV1", 3 - 1) == NULL) {
					    r->headers_in.msie6 = 1;
				    }
				    break;
			}
		}
#if 0
		/* MSIE ignores the SSL "close notify" alert */
		if(c->ssl) {
			c->ssl->no_send_shutdown = 1;
		}
#endif
	}
	if(ngx_strstrn(user_agent, "Opera", 5 - 1)) {
		r->headers_in.opera = 1;
		r->headers_in.msie = 0;
		r->headers_in.msie6 = 0;
	}
	if(!r->headers_in.msie && !r->headers_in.opera) {
		if(ngx_strstrn(user_agent, "Gecko/", 6 - 1)) {
			r->headers_in.gecko = 1;
		}
		else if(ngx_strstrn(user_agent, "Chrome/", 7 - 1)) {
			r->headers_in.chrome = 1;
		}
		else if(ngx_strstrn(user_agent, "Safari/", 7 - 1) && ngx_strstrn(user_agent, "Mac OS X", 8 - 1)) {
			r->headers_in.safari = 1;
		}
		else if(ngx_strstrn(user_agent, "Konqueror", 9 - 1)) {
			r->headers_in.konqueror = 1;
		}
	}
	return NGX_OK;
}

static ngx_int_t ngx_http_process_multi_header_lines(ngx_http_request_t * r, ngx_table_elt_t * h, ngx_uint_t offset)
{
	ngx_table_elt_t  ** ph;
	ngx_array_t  * headers = (ngx_array_t*)((char *)&r->headers_in + offset);
	if(headers->elts == NULL) {
		if(ngx_array_init(headers, r->pool, 1, sizeof(ngx_table_elt_t *)) != NGX_OK) {
			ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
			return NGX_ERROR;
		}
	}
	ph = (ngx_table_elt_t **)ngx_array_push(headers);
	if(ph == NULL) {
		ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
		return NGX_ERROR;
	}
	*ph = h;
	return NGX_OK;
}

ngx_int_t ngx_http_process_request_header(ngx_http_request_t * r)
{
	if(r->headers_in.server.len == 0 && ngx_http_set_virtual_server(r, &r->headers_in.server) == NGX_ERROR) {
		return NGX_ERROR;
	}
	if(r->headers_in.host == NULL && r->http_version > NGX_HTTP_VERSION_10) {
		ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "client sent HTTP/1.1 request without \"Host\" header");
		ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
		return NGX_ERROR;
	}
	if(r->headers_in.content_length) {
		r->headers_in.content_length_n = ngx_atoof(r->headers_in.content_length->value.data, r->headers_in.content_length->value.len);
		if(r->headers_in.content_length_n == NGX_ERROR) {
			ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "client sent invalid \"Content-Length\" header");
			ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
			return NGX_ERROR;
		}
	}
	if(r->method == NGX_HTTP_TRACE) {
		ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "client sent TRACE method");
		ngx_http_finalize_request(r, NGX_HTTP_NOT_ALLOWED);
		return NGX_ERROR;
	}

	if(r->headers_in.transfer_encoding) {
		if(r->headers_in.transfer_encoding->value.len == 7 && ngx_strncasecmp(r->headers_in.transfer_encoding->value.data, (uchar *)"chunked", 7) == 0) {
			r->headers_in.content_length = NULL;
			r->headers_in.content_length_n = -1;
			r->headers_in.chunked = 1;
		}
		else if(r->headers_in.transfer_encoding->value.len != 8 || ngx_strncasecmp(r->headers_in.transfer_encoding->value.data, (uchar *)"identity", 8) != 0) {
			ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "client sent unknown \"Transfer-Encoding\": \"%V\"", &r->headers_in.transfer_encoding->value);
			ngx_http_finalize_request(r, NGX_HTTP_NOT_IMPLEMENTED);
			return NGX_ERROR;
		}
	}
	if(r->headers_in.connection_type == NGX_HTTP_CONNECTION_KEEP_ALIVE) {
		if(r->headers_in.keep_alive) {
			r->headers_in.keep_alive_n = ngx_atotm(r->headers_in.keep_alive->value.data, r->headers_in.keep_alive->value.len);
		}
	}
	return NGX_OK;
}

void ngx_http_process_request(ngx_http_request_t * r)
{
	ngx_connection_t  * c = r->connection;
#if (NGX_HTTP_SSL)
	if(r->http_connection->ssl) {
		long rc;
		X509 * cert;
		ngx_http_ssl_srv_conf_t  * sscf;
		if(c->ssl == NULL) {
			ngx_log_error(NGX_LOG_INFO, c->log, 0, "client sent plain HTTP request to HTTPS port");
			ngx_http_finalize_request(r, NGX_HTTP_TO_HTTPS);
			return;
		}
		sscf = (ngx_http_ssl_srv_conf_t *)ngx_http_get_module_srv_conf(r, ngx_http_ssl_module);
		if(sscf->verify) {
			rc = SSL_get_verify_result(c->ssl->connection);
			if(rc != X509_V_OK && (sscf->verify != 3 || !ngx_ssl_verify_error_optional(rc))) {
				ngx_log_error(NGX_LOG_INFO, c->log, 0, "client SSL certificate verify error: (%l:%s)", rc, X509_verify_cert_error_string(rc));
				ngx_ssl_remove_cached_session(sscf->ssl.ctx, (SSL_get0_session(c->ssl->connection)));
				ngx_http_finalize_request(r, NGX_HTTPS_CERT_ERROR);
				return;
			}
			if(sscf->verify == 1) {
				cert = SSL_get_peer_certificate(c->ssl->connection);
				if(cert == NULL) {
					ngx_log_error(NGX_LOG_INFO, c->log, 0, "client sent no required SSL certificate");
					ngx_ssl_remove_cached_session(sscf->ssl.ctx, (SSL_get0_session(c->ssl->connection)));
					ngx_http_finalize_request(r, NGX_HTTPS_NO_CERT);
					return;
				}
				X509_free(cert);
			}
		}
	}
#endif
	if(c->P_EvRd->timer_set) {
		ngx_del_timer(c->P_EvRd);
	}
#if (NGX_STAT_STUB)
	(void)ngx_atomic_fetch_add(ngx_stat_reading, -1);
	r->stat_reading = 0;
	(void)ngx_atomic_fetch_add(ngx_stat_writing, 1);
	r->stat_writing = 1;
#endif
	c->P_EvRd->F_EvHandler = ngx_http_request_handler;
	c->P_EvWr->F_EvHandler = ngx_http_request_handler;
	r->read_event_handler = ngx_http_block_reading;
	ngx_http_handler(r);
	ngx_http_run_posted_requests(c);
}

static ngx_int_t ngx_http_validate_host(ngx_str_t * host, ngx_pool_t * pool, ngx_uint_t alloc)
{
	uchar ch;
	size_t i;
	enum {
		sw_usual = 0,
		sw_literal,
		sw_rest
	} state;
	size_t dot_pos = host->len;
	size_t host_len = host->len;
	uchar  * h = host->data;
	state = sw_usual;
	for(i = 0; i < host->len; i++) {
		ch = h[i];
		switch(ch) {
			case '.':
			    if(dot_pos == i - 1) {
				    return NGX_DECLINED;
			    }
			    dot_pos = i;
			    break;
			case ':':
			    if(state == sw_usual) {
				    host_len = i;
				    state = sw_rest;
			    }
			    break;
			case '[':
			    if(i == 0) {
				    state = sw_literal;
			    }
			    break;
			case ']':
			    if(state == sw_literal) {
				    host_len = i + 1;
				    state = sw_rest;
			    }
			    break;
			case '\0':
			    return NGX_DECLINED;
			default:
			    if(ngx_path_separator(ch)) {
				    return NGX_DECLINED;
			    }
			    if(ch >= 'A' && ch <= 'Z') {
				    alloc = 1;
			    }
			    break;
		}
	}
	if(dot_pos == host_len - 1) {
		host_len--;
	}
	if(host_len == 0) {
		return NGX_DECLINED;
	}
	if(alloc) {
		host->data = (uchar *)ngx_pnalloc(pool, host_len);
		if(host->data == NULL) {
			return NGX_ERROR;
		}
		ngx_strlow(host->data, h, host_len);
	}
	host->len = host_len;
	return NGX_OK;
}

static ngx_int_t ngx_http_set_virtual_server(ngx_http_request_t * r, ngx_str_t * host)
{
	ngx_int_t rc;
	ngx_http_connection_t   * hc;
	ngx_http_core_loc_conf_t  * clcf;
	ngx_http_core_srv_conf_t  * cscf;
#if (NGX_SUPPRESS_WARN)
	cscf = NULL;
#endif
	hc = r->http_connection;
#if (NGX_HTTP_SSL && defined SSL_CTRL_SET_TLSEXT_HOSTNAME)
	if(hc->ssl_servername) {
		if(hc->ssl_servername->len == host->len && ngx_strncmp(hc->ssl_servername->data, host->data, host->len) == 0) {
#if (NGX_PCRE)
			if(hc->ssl_servername_regex && ngx_http_regex_exec(r, hc->ssl_servername_regex, hc->ssl_servername) != NGX_OK) {
				ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
				return NGX_ERROR;
			}
#endif
			return NGX_OK;
		}
	}
#endif
	rc = ngx_http_find_virtual_server(r->connection, hc->addr_conf->virtual_names, host, r, &cscf);
	if(rc == NGX_ERROR) {
		ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
		return NGX_ERROR;
	}
#if (NGX_HTTP_SSL && defined SSL_CTRL_SET_TLSEXT_HOSTNAME)
	if(hc->ssl_servername) {
		ngx_http_ssl_srv_conf_t  * sscf;
		if(rc == NGX_DECLINED) {
			cscf = hc->addr_conf->default_server;
			rc = NGX_OK;
		}
		sscf = (ngx_http_ssl_srv_conf_t *)ngx_http_get_module_srv_conf(cscf->ctx, ngx_http_ssl_module);
		if(sscf->verify) {
			ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "client attempted to request the server name different from the one that was negotiated");
			ngx_http_finalize_request(r, NGX_HTTP_MISDIRECTED_REQUEST);
			return NGX_ERROR;
		}
	}
#endif
	if(rc == NGX_DECLINED) {
		return NGX_OK;
	}
	r->srv_conf = cscf->ctx->srv_conf;
	r->loc_conf = cscf->ctx->loc_conf;
	clcf = (ngx_http_core_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_core_module);
	//ngx_set_connection_log(r->connection, clcf->error_log);
	r->connection->SetLog(clcf->error_log);
	return NGX_OK;
}

static ngx_int_t ngx_http_find_virtual_server(ngx_connection_t * c,
    ngx_http_virtual_names_t * virtual_names, ngx_str_t * host,
    ngx_http_request_t * r, ngx_http_core_srv_conf_t ** cscfp)
{
	ngx_http_core_srv_conf_t  * cscf;
	if(virtual_names == NULL) {
		return NGX_DECLINED;
	}
	cscf = (ngx_http_core_srv_conf_t *)ngx_hash_find_combined(&virtual_names->names, ngx_hash_key(host->data, host->len), host->data, host->len);
	if(cscf) {
		*cscfp = cscf;
		return NGX_OK;
	}
#if (NGX_PCRE)
	if(host->len && virtual_names->nregex) {
		ngx_int_t n;
		ngx_uint_t i;
		ngx_http_server_name_t  * sn = virtual_names->regex;
#if (NGX_HTTP_SSL && defined SSL_CTRL_SET_TLSEXT_HOSTNAME)
		if(r == NULL) {
			ngx_http_connection_t  * hc;
			for(i = 0; i < virtual_names->nregex; i++) {
				n = ngx_regex_exec(sn[i].regex->regex, host, NULL, 0);
				if(n == NGX_REGEX_NO_MATCHED) {
					continue;
				}
				if(n >= 0) {
					hc = (ngx_http_connection_t *)c->data;
					hc->ssl_servername_regex = sn[i].regex;
					*cscfp = sn[i].server;
					return NGX_OK;
				}
				ngx_log_error(NGX_LOG_ALERT, c->log, 0, ngx_regex_exec_n " failed: %i on \"%V\" using \"%V\"", n, host, &sn[i].regex->name);
				return NGX_ERROR;
			}
			return NGX_DECLINED;
		}
#endif /* NGX_HTTP_SSL && defined SSL_CTRL_SET_TLSEXT_HOSTNAME */
		for(i = 0; i < virtual_names->nregex; i++) {
			n = ngx_http_regex_exec(r, sn[i].regex, host);
			if(n == NGX_DECLINED) {
				continue;
			}
			if(n == NGX_OK) {
				*cscfp = sn[i].server;
				return NGX_OK;
			}
			return NGX_ERROR;
		}
	}
#endif /* NGX_PCRE */
	return NGX_DECLINED;
}

static void ngx_http_request_handler(ngx_event_t * ev)
{
	ngx_connection_t * c = (ngx_connection_t *)ev->P_Data;
	ngx_http_request_t * r = (ngx_http_request_t *)c->data;
	ngx_http_set_log_request(c->log, r);
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0, "http run request: \"%V?%V\"", &r->uri, &r->args);
	if(ev->delayed && ev->timedout) {
		ev->delayed = 0;
		ev->timedout = 0;
	}
	if(ev->write) {
		r->write_event_handler(r);
	}
	else {
		r->read_event_handler(r);
	}
	ngx_http_run_posted_requests(c);
}

void ngx_http_run_posted_requests(ngx_connection_t * c)
{
	ngx_http_request_t  * r;
	ngx_http_posted_request_t  * pr;
	for(;;) {
		if(c->destroyed) {
			return;
		}
		r = (ngx_http_request_t *)c->data;
		pr = r->main->posted_requests;
		if(pr == NULL) {
			return;
		}
		r->main->posted_requests = pr->next;
		r = pr->request;
		ngx_http_set_log_request(c->log, r);
		ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0, "http posted request: \"%V?%V\"", &r->uri, &r->args);
		r->write_event_handler(r);
	}
}

ngx_int_t ngx_http_post_request(ngx_http_request_t * r, ngx_http_posted_request_t * pr)
{
	ngx_http_posted_request_t  ** p;
	if(pr == NULL) {
		pr = (ngx_http_posted_request_t *)ngx_palloc(r->pool, sizeof(ngx_http_posted_request_t));
		if(pr == NULL) {
			return NGX_ERROR;
		}
	}
	pr->request = r;
	pr->next = NULL;
	for(p = &r->main->posted_requests; *p; p = &(*p)->next) { /* void */
	}
	*p = pr;
	return NGX_OK;
}

void FASTCALL ngx_http_finalize_request(ngx_http_request_t * pReq, ngx_int_t rc) // rc - ��������� ������ F_HttpHandler
{
	ngx_connection_t * c = pReq->connection;
	ngx_log_debug5(NGX_LOG_DEBUG_HTTP, c->log, 0, "http finalize request: %i, \"%V?%V\" a:%d, c:%d", rc, &pReq->uri, &pReq->args, pReq == c->data, pReq->main->count);
	if(rc == NGX_DELEGATED) { // @sobolev
	}
	else if(rc == NGX_DONE) {
		ngx_http_finalize_connection(pReq);
	}
	else if(rc == NGX_DECLINED) {
		pReq->F_HttpContentHandler = 0;
		pReq->write_event_handler = ngx_http_core_run_phases;
		ngx_http_core_run_phases(pReq);
	}
	else {
		if(rc == NGX_OK && pReq->filter_finalize) {
			c->error = 1;
		}
		if(pReq != pReq->main && pReq->post_subrequest) {
			rc = pReq->post_subrequest->handler(pReq, pReq->post_subrequest->data, rc);
		}
		if(oneof3(rc, NGX_ERROR, NGX_HTTP_REQUEST_TIME_OUT, NGX_HTTP_CLIENT_CLOSED_REQUEST) || c->error) {
			if(ngx_http_post_action(pReq) != NGX_OK)
				ngx_http_terminate_request(pReq, rc);
		}
		else if(rc >= NGX_HTTP_SPECIAL_RESPONSE || oneof2(rc, NGX_HTTP_CREATED, NGX_HTTP_NO_CONTENT)) {
			if(rc == NGX_HTTP_CLOSE)
				ngx_http_terminate_request(pReq, rc);
			else {
				if(pReq == pReq->main) {
					if(c->P_EvRd->timer_set) {
						ngx_del_timer(c->P_EvRd);
					}
					if(c->P_EvWr->timer_set) {
						ngx_del_timer(c->P_EvWr);
					}
				}
				c->P_EvRd->F_EvHandler = ngx_http_request_handler;
				c->P_EvWr->F_EvHandler = ngx_http_request_handler;
				ngx_http_finalize_request(pReq, ngx_http_special_response_handler(pReq, rc)); // @recursion
			}
		}
		else if(pReq != pReq->main) {
			ngx_http_core_loc_conf_t * clcf = (ngx_http_core_loc_conf_t *)ngx_http_get_module_loc_conf(pReq, ngx_http_core_module);
			if(pReq->background) {
				if(!pReq->logged) {
					if(clcf->log_subrequest) {
						ngx_http_log_request(pReq);
					}
					pReq->logged = 1;
				}
				else {
					ngx_log_error(NGX_LOG_ALERT, c->log, 0, "subrequest: \"%V?%V\" logged again", &pReq->uri, &pReq->args);
				}
				pReq->done = 1;
				ngx_http_finalize_connection(pReq);
			}
			else if(pReq->buffered || pReq->postponed) {
				if(ngx_http_set_write_handler(pReq) != NGX_OK)
					ngx_http_terminate_request(pReq, 0);
			}
			else {
				ngx_http_request_t * pr = pReq->parent;
				if(pReq == c->data) {
					pReq->main->count--;
					if(!pReq->logged) {
						if(clcf->log_subrequest) {
							ngx_http_log_request(pReq);
						}
						pReq->logged = 1;
					}
					else {
						ngx_log_error(NGX_LOG_ALERT, c->log, 0, "subrequest: \"%V?%V\" logged again", &pReq->uri, &pReq->args);
					}
					pReq->done = 1;
					if(pr->postponed && pr->postponed->request == pReq) {
						pr->postponed = pr->postponed->next;
					}
					c->data = pr;
				}
				else {
					ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0, "http finalize non-active request: \"%V?%V\"", &pReq->uri, &pReq->args);
					pReq->write_event_handler = ngx_http_request_finalizer;
					if(pReq->waited)
						pReq->done = 1;
				}
				if(ngx_http_post_request(pr, NULL) != NGX_OK) {
					pReq->main->count++;
					ngx_http_terminate_request(pReq, 0);
				}
				else
					ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0, "http wake parent request: \"%V?%V\"", &pr->uri, &pr->args);
			}
		}
		else if(pReq->buffered || c->buffered || pReq->postponed) {
			if(ngx_http_set_write_handler(pReq) != NGX_OK)
				ngx_http_terminate_request(pReq, 0);
		}
		else if(pReq != c->data) {
			ngx_log_error(NGX_LOG_ALERT, c->log, 0, "http finalize non-active request: \"%V?%V\"", &pReq->uri, &pReq->args);
		}
		else {
			pReq->done = 1;
			pReq->read_event_handler = ngx_http_block_reading;
			pReq->write_event_handler = ngx_http_request_empty_handler;
			if(!pReq->post_action) {
				pReq->request_complete = 1;
			}
			if(ngx_http_post_action(pReq) != NGX_OK) {
				if(c->P_EvRd->timer_set) {
					ngx_del_timer(c->P_EvRd);
				}
				if(c->P_EvWr->timer_set) {
					c->P_EvWr->delayed = 0;
					ngx_del_timer(c->P_EvWr);
				}
				if(c->P_EvRd->eof)
					ngx_http_close_request(pReq, 0);
				else
					ngx_http_finalize_connection(pReq);
			}
		}
	}
}

static void ngx_http_terminate_request(ngx_http_request_t * r, ngx_int_t rc)
{
	ngx_http_cleanup_t  * cln;
	ngx_http_ephemeral_t  * e;
	ngx_http_request_t  * mr = r->main;
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http terminate request count:%d", mr->count);
	if(rc > 0 && (mr->headers_out.status == 0 || mr->connection->sent == 0)) {
		mr->headers_out.status = rc;
	}
	cln = mr->cleanup;
	mr->cleanup = NULL;
	while(cln) {
		if(cln->handler) {
			cln->handler(cln->data);
		}
		cln = cln->next;
	}
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http terminate cleanup count:%d blk:%d", mr->count, mr->blocked);
	if(mr->write_event_handler) {
		if(mr->blocked) {
			r->connection->error = 1;
			r->write_event_handler = ngx_http_request_finalizer;
			return;
		}
		e = (ngx_http_ephemeral_t *)ngx_http_ephemeral(mr);
		mr->posted_requests = NULL;
		mr->write_event_handler = ngx_http_terminate_handler;
		(void)ngx_http_post_request(mr, &e->terminal_posted_request);
		return;
	}
	ngx_http_close_request(mr, rc);
}

static void ngx_http_terminate_handler(ngx_http_request_t * r)
{
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http terminate handler count:%d", r->count);
	r->count = 1;
	ngx_http_close_request(r, 0);
}

static void FASTCALL ngx_http_finalize_connection(ngx_http_request_t * pReq)
{
#if (NGX_HTTP_V2)
	if(pReq->stream) {
		ngx_http_close_request(pReq, 0);
		return;
	}
#endif
	{
		ngx_http_core_loc_conf_t * clcf = (ngx_http_core_loc_conf_t *)ngx_http_get_module_loc_conf(pReq, ngx_http_core_module);
		if(pReq->main->count != 1) {
			if(pReq->discard_body) {
				pReq->read_event_handler = ngx_http_discarded_request_body_handler;
				ngx_add_timer(pReq->connection->P_EvRd, clcf->lingering_timeout);
				SETIFZ(pReq->lingering_time, (ngx_time() + (time_t)(clcf->lingering_time / 1000)));
			}
			ngx_http_close_request(pReq, 0);
		}
		else {
			pReq = pReq->main;
			if(pReq->reading_body) {
				pReq->keepalive = 0;
				pReq->lingering_close = 1;
			}
			if(!ngx_terminate && !ngx_exiting && pReq->keepalive && clcf->keepalive_timeout > 0) {
				ngx_http_set_keepalive(pReq);
			}
			else if(clcf->lingering_close == NGX_HTTP_LINGERING_ALWAYS || (clcf->lingering_close == NGX_HTTP_LINGERING_ON && 
				(pReq->lingering_close || pReq->header_in->pos < pReq->header_in->last || pReq->connection->P_EvRd->ready))) {
				ngx_http_set_lingering_close(pReq);
			}
			else
				ngx_http_close_request(pReq, 0);
		}
	}
}

static ngx_int_t ngx_http_set_write_handler(ngx_http_request_t * r)
{
	ngx_event_t * wev;
	ngx_http_core_loc_conf_t  * clcf;
	r->http_state = NGX_HTTP_WRITING_REQUEST_STATE;
	r->read_event_handler = r->discard_body ? ngx_http_discarded_request_body_handler : ngx_http_test_reading;
	r->write_event_handler = ngx_http_writer;
	wev = r->connection->P_EvWr;
	if(wev->ready && wev->delayed) {
		return NGX_OK;
	}
	clcf = (ngx_http_core_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_core_module);
	if(!wev->delayed) {
		ngx_add_timer(wev, clcf->send_timeout);
	}
	if(ngx_handle_write_event(wev, clcf->send_lowat) != NGX_OK) {
		ngx_http_close_request(r, 0);
		return NGX_ERROR;
	}
	return NGX_OK;
}

static void ngx_http_writer(ngx_http_request_t * pReq)
{
	ngx_http_core_loc_conf_t  * clcf;
	ngx_connection_t * c = pReq->connection;
	ngx_event_t * wev = c->P_EvWr;
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, wev->log, 0, "http writer handler: \"%V?%V\"", &pReq->uri, &pReq->args);
	clcf = (ngx_http_core_loc_conf_t *)ngx_http_get_module_loc_conf(pReq->main, ngx_http_core_module);
	if(wev->timedout) {
		ngx_log_error(NGX_LOG_INFO, c->log, NGX_ETIMEDOUT, "client timed out");
		c->timedout = 1;
		ngx_http_finalize_request(pReq, NGX_HTTP_REQUEST_TIME_OUT);
	}
	else if(wev->delayed || pReq->aio) {
		ngx_log_debug0(NGX_LOG_DEBUG_HTTP, wev->log, 0, "http writer delayed");
		if(!wev->delayed) {
			ngx_add_timer(wev, clcf->send_timeout);
		}
		if(ngx_handle_write_event(wev, clcf->send_lowat) != NGX_OK) {
			ngx_http_close_request(pReq, 0);
		}
	}
	else {
		ngx_int_t rc = ngx_http_output_filter(pReq, NULL);
		ngx_log_debug3(NGX_LOG_DEBUG_HTTP, c->log, 0, "http writer output filter: %i, \"%V?%V\"", rc, &pReq->uri, &pReq->args);
		if(rc == NGX_ERROR) {
			ngx_http_finalize_request(pReq, rc);
		}
		else if(pReq->buffered || pReq->postponed || (pReq == pReq->main && c->buffered)) {
			if(!wev->delayed) {
				ngx_add_timer(wev, clcf->send_timeout);
			}
			if(ngx_handle_write_event(wev, clcf->send_lowat) != NGX_OK) {
				ngx_http_close_request(pReq, 0);
			}
		}
		else {
			ngx_log_debug2(NGX_LOG_DEBUG_HTTP, wev->log, 0, "http writer done: \"%V?%V\"", &pReq->uri, &pReq->args);
			pReq->write_event_handler = ngx_http_request_empty_handler;
			ngx_http_finalize_request(pReq, rc);
		}
	}
}

static void ngx_http_request_finalizer(ngx_http_request_t * r)
{
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http finalizer done: \"%V?%V\"", &r->uri, &r->args);
	ngx_http_finalize_request(r, 0);
}

void ngx_http_block_reading(ngx_http_request_t * r)
{
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http reading blocked");
	// aio does not call this handler 
	if((ngx_event_flags & NGX_USE_LEVEL_EVENT) && r->connection->P_EvRd->active) {
		if(ngx_del_event(r->connection->P_EvRd, NGX_READ_EVENT, 0) != NGX_OK) {
			ngx_http_close_request(r, 0);
		}
	}
}

void ngx_http_test_reading(ngx_http_request_t * r)
{
	int n;
	char buf[1];
	ngx_err_t err;
	ngx_connection_t * c = r->connection;
	ngx_event_t * rev = c->P_EvRd;
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http test reading");
#if (NGX_HTTP_V2)
	if(r->stream) {
		if(c->error) {
			err = 0;
			goto closed;
		}
		return;
	}
#endif
#if (NGX_HAVE_KQUEUE)
	if(ngx_event_flags & NGX_USE_KQUEUE_EVENT) {
		if(!rev->pending_eof) {
			return;
		}
		rev->eof = 1;
		c->error = 1;
		err = rev->kq_errno;
		goto closed;
	}
#endif
#if (NGX_HAVE_EPOLLRDHUP)
	if((ngx_event_flags & NGX_USE_EPOLL_EVENT) && ngx_use_epoll_rdhup) {
		socklen_t len;
		if(!rev->pending_eof) {
			return;
		}
		rev->eof = 1;
		c->error = 1;
		err = 0;
		len = sizeof(ngx_err_t);
		/*
		 * BSDs and Linux return 0 and set a pending error in err
		 * Solaris returns -1 and sets errno
		 */
		if(getsockopt(c->fd, SOL_SOCKET, SO_ERROR, (void *)&err, &len) == -1) {
			err = ngx_socket_errno;
		}
		goto closed;
	}
#endif
	n = recv(c->fd, buf, 1, MSG_PEEK);
	if(n == 0) {
		rev->eof = 1;
		c->error = 1;
		err = 0;
		goto closed;
	}
	else if(n == -1) {
		err = ngx_socket_errno;
		if(err != NGX_EAGAIN) {
			rev->eof = 1;
			c->error = 1;
			goto closed;
		}
	}
	/* aio does not call this handler */
	if((ngx_event_flags & NGX_USE_LEVEL_EVENT) && rev->active) {
		if(ngx_del_event(rev, NGX_READ_EVENT, 0) != NGX_OK) {
			ngx_http_close_request(r, 0);
		}
	}
	return;
closed:
	if(err) {
		rev->error = 1;
	}
	ngx_log_error(NGX_LOG_INFO, c->log, err, "client prematurely closed connection");
	ngx_http_finalize_request(r, NGX_HTTP_CLIENT_CLOSED_REQUEST);
}

static void ngx_http_set_keepalive(ngx_http_request_t * pReq)
{
	int tcp_nodelay;
	ngx_buf_t * b, * f;
	ngx_chain_t * cl, * ln;
	ngx_event_t * wev;
	ngx_http_connection_t * hc;
	ngx_connection_t * c = pReq->connection;
	ngx_event_t * rev = c->P_EvRd;
	ngx_http_core_loc_conf_t * clcf = (ngx_http_core_loc_conf_t *)ngx_http_get_module_loc_conf(pReq, ngx_http_core_module);
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "set http keepalive handler");
	if(pReq->discard_body) {
		pReq->write_event_handler = ngx_http_request_empty_handler;
		pReq->lingering_time = ngx_time() + (time_t)(clcf->lingering_time / 1000);
		ngx_add_timer(rev, clcf->lingering_timeout);
	}
	else {
		c->log->action = "closing request";
		hc = pReq->http_connection;
		b = pReq->header_in;
		if(b->pos < b->last) {
			// the pipelined request 
			if(b != c->buffer) {
				// 
				// If the large header buffers were allocated while the previous
				// request processing then we do not use c->buffer for
				// the pipelined request (see ngx_http_create_request()).
				// 
				// Now we would move the large header buffers to the free list.
				// 
				for(cl = hc->busy; cl; /* void */) {
					ln = cl;
					cl = cl->next;
					if(ln->buf == b) {
						ngx_free_chain(c->pool, ln);
					}
					else {
						f = ln->buf;
						f->pos = f->start;
						f->last = f->start;
						ln->next = hc->free;
						hc->free = ln;
					}
				}
				cl = ngx_alloc_chain_link(c->pool);
				if(cl == NULL) {
					// @sobolev ����� THROW �� ������� ������������ - ���������� �� connection, � request.
					ngx_http_close_request(pReq, 0);
					return;
				}
				cl->buf = b;
				cl->next = NULL;
				hc->busy = cl;
				hc->nbusy = 1;
			}
		}
		// guard against recursive call from ngx_http_finalize_connection() 
		pReq->keepalive = 0;
		ngx_http_free_request(pReq, 0);
		c->data = hc;
		THROW(ngx_handle_read_event(rev, 0) == NGX_OK);
		wev = c->P_EvWr;
		wev->F_EvHandler = ngx_http_empty_handler;
		if(b->pos < b->last) {
			ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "pipelined request");
			c->log->action = "reading client pipelined request line";
			THROW(pReq = ngx_http_create_request(c));
			pReq->pipeline = 1;
			c->data = pReq;
			c->sent = 0;
			c->destroyed = 0;
			if(rev->timer_set) {
				ngx_del_timer(rev);
			}
			rev->F_EvHandler = ngx_http_process_request_line;
			ngx_post_event(rev, &ngx_posted_events);
		}
		else {
			// 
			// To keep a memory footprint as small as possible for an idle keepalive
			// connection we try to free c->buffer's memory if it was allocated outside
			// the c->pool.  The large header buffers are always allocated outside the c->pool and are freed too.
			// 
			b = c->buffer;
			if(ngx_pfree(c->pool, b->start) == NGX_OK) {
				b->pos = NULL; // the special note for ngx_http_keepalive_handler() that c->buffer's memory was freed
			}
			else {
				b->pos = b->start;
				b->last = b->start;
			}
			ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0, "hc free: %p", hc->free);
			if(hc->free) {
				for(cl = hc->free; cl; /* void */) {
					ln = cl;
					cl = cl->next;
					ngx_pfree(c->pool, ln->buf->start);
					ngx_free_chain(c->pool, ln);
				}
				hc->free = NULL;
			}
			ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0, "hc busy: %p %i", hc->busy, hc->nbusy);
			if(hc->busy) {
				for(cl = hc->busy; cl; /* void */) {
					ln = cl;
					cl = cl->next;
					ngx_pfree(c->pool, ln->buf->start);
					ngx_free_chain(c->pool, ln);
				}
				hc->busy = NULL;
				hc->nbusy = 0;
			}
	#if (NGX_HTTP_SSL)
			if(c->ssl) {
				ngx_ssl_free_buffer(c);
			}
	#endif
			rev->F_EvHandler = ngx_http_keepalive_handler;
			if(wev->active && (ngx_event_flags & NGX_USE_LEVEL_EVENT)) {
				THROW(ngx_del_event(wev, NGX_WRITE_EVENT, 0) == NGX_OK);
			}
			c->log->action = "keepalive";
			if(c->tcp_nopush == NGX_TCP_NOPUSH_SET) {
				if(ngx_tcp_push(c->fd) == -1) {
					ngx_connection_error(c, ngx_socket_errno, ngx_tcp_push_n " failed");
					CALLEXCEPT();
				}
				c->tcp_nopush = NGX_TCP_NOPUSH_UNSET;
				tcp_nodelay = ngx_tcp_nodelay_and_tcp_nopush ? 1 : 0;
			}
			else {
				tcp_nodelay = 1;
			}
			THROW(!tcp_nodelay || !clcf->tcp_nodelay || ngx_tcp_nodelay(c) == NGX_OK);
#if 0
			// if ngx_http_request_t was freed then we need some other place 
			r->http_state = NGX_HTTP_KEEPALIVE_STATE;
#endif
			c->idle = 1;
			ngx_reusable_connection(c, 1);
			ngx_add_timer(rev, clcf->keepalive_timeout);
			if(rev->ready) {
				ngx_post_event(rev, &ngx_posted_events);
			}
		}
	}
	CATCH
		ngx_http_close_connection(c);
	ENDCATCH
}

static void ngx_http_keepalive_handler(ngx_event_t * rev)
{
	size_t size;
	ssize_t n;
	ngx_buf_t  * b;
	ngx_connection_t  * c = (ngx_connection_t *)rev->P_Data;
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http keepalive handler");
	THROW(!rev->timedout && !c->close);
#if (NGX_HAVE_KQUEUE)
	if(ngx_event_flags & NGX_USE_KQUEUE_EVENT) {
		if(rev->pending_eof) {
			c->log->handler = NULL;
			ngx_log_error(NGX_LOG_INFO, c->log, rev->kq_errno, "kevent() reported that client %V closed keepalive connection", &c->addr_text);
#if (NGX_HTTP_SSL)
			if(c->ssl) {
				c->ssl->no_send_shutdown = 1;
			}
#endif
			CALLEXCEPT();
		}
	}
#endif
	b = c->buffer;
	size = b->end - b->start;
	if(b->pos == NULL) {
		// 
		// The c->buffer's memory was freed by ngx_http_set_keepalive().
		// However, the c->buffer->start and c->buffer->end were not changed to keep the buffer size.
		// 
		THROW(b->pos = (uchar *)ngx_palloc(c->pool, size));
		b->start = b->pos;
		b->last = b->pos;
		b->end = b->pos + size;
	}
	// 
	// MSIE closes a keepalive connection with RST flag so we ignore ECONNRESET here.
	// 
	c->log_error = NGX_ERROR_IGNORE_ECONNRESET;
	ngx_set_socket_errno(0);
	n = c->recv(c, b->last, size);
	c->log_error = NGX_ERROR_INFO;
	if(n == NGX_AGAIN) {
		THROW(ngx_handle_read_event(rev, 0) == NGX_OK);
		// 
		// Like ngx_http_set_keepalive() we are trying to not hold
		// c->buffer's memory for a keepalive connection.
		// 
		if(ngx_pfree(c->pool, b->start) == NGX_OK)
			b->pos = NULL; // the special note that c->buffer's memory was freed
	}
	else {
		THROW(n != NGX_ERROR);
		c->log->handler = NULL;
		if(n == 0) {
			ngx_log_error(NGX_LOG_INFO, c->log, ngx_socket_errno, "client %V closed keepalive connection", &c->addr_text);
			CALLEXCEPT();
		}
		b->last += n;
		c->log->handler = ngx_http_log_error;
		c->log->action = "reading client request line";
		c->idle = 0;
		ngx_reusable_connection(c, 0);
		THROW(c->data = ngx_http_create_request(c));
		c->sent = 0;
		c->destroyed = 0;
		ngx_del_timer(rev);
		rev->F_EvHandler = ngx_http_process_request_line;
		ngx_http_process_request_line(rev);
	}
	CATCH
		ngx_http_close_connection(c);
	ENDCATCH
}

static void ngx_http_set_lingering_close(ngx_http_request_t * r)
{
	ngx_event_t * wev;
	ngx_connection_t   * c = r->connection;
	ngx_http_core_loc_conf_t  * clcf = (ngx_http_core_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_core_module);
	ngx_event_t * rev = c->P_EvRd;
	rev->F_EvHandler = ngx_http_lingering_close_handler;
	r->lingering_time = ngx_time() + (time_t)(clcf->lingering_time / 1000);
	ngx_add_timer(rev, clcf->lingering_timeout);
	if(ngx_handle_read_event(rev, 0) != NGX_OK) {
		ngx_http_close_request(r, 0);
		return;
	}
	wev = c->P_EvWr;
	wev->F_EvHandler = ngx_http_empty_handler;
	if(wev->active && (ngx_event_flags & NGX_USE_LEVEL_EVENT)) {
		if(ngx_del_event(wev, NGX_WRITE_EVENT, 0) != NGX_OK) {
			ngx_http_close_request(r, 0);
			return;
		}
	}
	if(ngx_shutdown_socket(c->fd, NGX_WRITE_SHUTDOWN) == -1) {
		ngx_connection_error(c, ngx_socket_errno, ngx_shutdown_socket_n " failed");
		ngx_http_close_request(r, 0);
		return;
	}
	if(rev->ready) {
		ngx_http_lingering_close_handler(rev);
	}
}

static void ngx_http_lingering_close_handler(ngx_event_t * rev)
{
	ssize_t n;
	uchar buffer[NGX_HTTP_LINGERING_BUFFER_SIZE];
	ngx_connection_t   * c = (ngx_connection_t *)rev->P_Data;
	ngx_http_request_t * r = (ngx_http_request_t *)c->data;
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http lingering close handler");
	if(rev->timedout) {
		ngx_http_close_request(r, 0);
	}
	else {
		ngx_msec_t timer = (ngx_msec_t)r->lingering_time - (ngx_msec_t)ngx_time();
		if((ngx_msec_int_t)timer <= 0) {
			ngx_http_close_request(r, 0);
			return;
		}
		do {
			n = c->recv(c, buffer, NGX_HTTP_LINGERING_BUFFER_SIZE);
			ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0, "lingering read: %z", n);
			if(n == NGX_ERROR || n == 0) {
				ngx_http_close_request(r, 0);
				return;
			}
		} while(rev->ready);
		if(ngx_handle_read_event(rev, 0) != NGX_OK) {
			ngx_http_close_request(r, 0);
		}
		else {
			ngx_http_core_loc_conf_t  * clcf = (ngx_http_core_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_core_module);
			timer *= 1000;
			if(timer > clcf->lingering_timeout) {
				timer = clcf->lingering_timeout;
			}
			ngx_add_timer(rev, timer);
		}
	}
}

void ngx_http_empty_handler(ngx_event_t * wev)
{
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, wev->log, 0, "http empty handler");
}

void ngx_http_request_empty_handler(ngx_http_request_t * r)
{
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http request empty handler");
}

ngx_int_t ngx_http_send_special(ngx_http_request_t * r, ngx_uint_t flags)
{
	ngx_buf_t  * b = static_cast<ngx_buf_t *>(ngx_calloc_buf(r->pool));
	if(!b) {
		return NGX_ERROR;
	}
	if(flags & NGX_HTTP_LAST) {
		if(r == r->main && !r->post_action) {
			b->last_buf = 1;
		}
		else {
			b->sync = 1;
			b->last_in_chain = 1;
		}
	}
	if(flags & NGX_HTTP_FLUSH) {
		b->flush = 1;
	}
	{
		ngx_chain_t out(b, 0);
		//out.buf = b;
		//out.next = NULL;
		return ngx_http_output_filter(r, &out);
	}
}

static ngx_int_t ngx_http_post_action(ngx_http_request_t * pReq)
{
	ngx_http_core_loc_conf_t * clcf = (ngx_http_core_loc_conf_t *)ngx_http_get_module_loc_conf(pReq, ngx_http_core_module);
	if(!clcf->post_action.data || (pReq->post_action && pReq->uri_changes == 0)) {
		return NGX_DECLINED;
	}
	else {
		ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pReq->connection->log, 0, "post action: \"%V\"", &clcf->post_action);
		pReq->main->count--;
		pReq->http_version = NGX_HTTP_VERSION_9;
		pReq->header_only = 1;
		pReq->post_action = 1;
		pReq->read_event_handler = ngx_http_block_reading;
		if(clcf->post_action.data[0] == '/') {
			ngx_http_internal_redirect(pReq, &clcf->post_action, NULL);
		}
		else {
			ngx_http_named_location(pReq, &clcf->post_action);
		}
		return NGX_OK;
	}
}

static void FASTCALL ngx_http_close_request(ngx_http_request_t * pReq, ngx_int_t rc)
{
	ngx_connection_t * c;
	pReq = pReq->main;
	c = pReq->connection;
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0, "http request count:%d blk:%d", pReq->count, pReq->blocked);
	if(pReq->count == 0)
		ngx_log_error(NGX_LOG_ALERT, c->log, 0, "http request count is zero");
	pReq->count--;
	if(!pReq->count && !pReq->blocked) {
#if (NGX_HTTP_V2)
		if(pReq->stream) {
			ngx_http_v2_close_stream(pReq->stream, rc);
			return;
		}
#endif
		ngx_http_free_request(pReq, rc);
		ngx_http_close_connection(c);
	}
}

void ngx_http_free_request(ngx_http_request_t * pReq, ngx_int_t rc)
{
	ngx_pool_t * pool;
	struct linger linger;
	ngx_http_log_ctx_t * ctx;
	ngx_log_t * log = pReq->connection->log;
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, log, 0, "http close request");
	if(pReq->pool == NULL)
		ngx_log_error(NGX_LOG_ALERT, log, 0, "http request already closed");
	else {
		ngx_http_cleanup_t * cln = pReq->cleanup;
		pReq->cleanup = NULL;
		while(cln) {
			if(cln->handler)
				cln->handler(cln->data);
			cln = cln->next;
		}
#if (NGX_STAT_STUB)
		if(pReq->stat_reading) {
			(void)ngx_atomic_fetch_add(ngx_stat_reading, -1);
		}
		if(pReq->stat_writing) {
			(void)ngx_atomic_fetch_add(ngx_stat_writing, -1);
		}
#endif
		if(rc > 0 && (pReq->headers_out.status == 0 || pReq->connection->sent == 0)) {
			pReq->headers_out.status = rc;
		}
		log->action = "logging request";
		ngx_http_log_request(pReq);
		log->action = "closing request";
		if(pReq->connection->timedout) {
			ngx_http_core_loc_conf_t  * clcf = (ngx_http_core_loc_conf_t *)ngx_http_get_module_loc_conf(pReq, ngx_http_core_module);
			if(clcf->reset_timedout_connection) {
				linger.l_onoff = 1;
				linger.l_linger = 0;
				if(setsockopt(pReq->connection->fd, SOL_SOCKET, SO_LINGER, (const char *)&linger, sizeof(struct linger)) == -1) {
					ngx_log_error(NGX_LOG_ALERT, log, ngx_socket_errno, "setsockopt(SO_LINGER) failed");
				}
			}
		}
		// the various request strings were allocated from r->pool 
		ctx = (ngx_http_log_ctx_t *)log->data;
		ctx->request = NULL;
		pReq->request_line.len = 0;
		pReq->connection->destroyed = 1;
		// 
		// Setting r->pool to NULL will increase probability to catch double close
		// of request since the request object is allocated from its own pool.
		// 
		pool = pReq->pool;
		pReq->pool = NULL;
		ngx_destroy_pool(pool);
	}
}

static void ngx_http_log_request(ngx_http_request_t * r)
{
	ngx_http_core_main_conf_t  * cmcf = (ngx_http_core_main_conf_t *)ngx_http_get_module_main_conf(r, ngx_http_core_module);
	ngx_http_handler_pt * log_handler = (ngx_http_handler_pt *)cmcf->phases[NGX_HTTP_LOG_PHASE].handlers.elts;
	ngx_uint_t n = cmcf->phases[NGX_HTTP_LOG_PHASE].handlers.nelts;
	for(ngx_uint_t i = 0; i < n; i++) {
		log_handler[i](r);
	}
}

void ngx_http_close_connection(ngx_connection_t * c)
{
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0, "close http connection: %d", c->fd);
#if (NGX_HTTP_SSL)
	if(c->ssl && ngx_ssl_shutdown(c) == NGX_AGAIN) {
		c->ssl->handler = ngx_http_close_connection;
		return;
	}
#endif
#if (NGX_STAT_STUB)
	(void)ngx_atomic_fetch_add(ngx_stat_active, -1);
#endif
	c->destroyed = 1;
	{
		ngx_pool_t * p_pool = c->pool;
		ngx_close_connection(c);
		ngx_destroy_pool(p_pool);
	}
}

static uchar * ngx_http_log_error(ngx_log_t * log, uchar * buf, size_t len)
{
	uchar * p;
	ngx_http_request_t  * r;
	ngx_http_log_ctx_t  * ctx;
	if(log->action) {
		p = ngx_snprintf(buf, len, " while %s", log->action);
		len -= p - buf;
		buf = p;
	}
	ctx = (ngx_http_log_ctx_t *)log->data;
	p = ngx_snprintf(buf, len, ", client: %V", &ctx->connection->addr_text);
	len -= p - buf;
	r = ctx->request;
	if(r) {
		return r->log_handler(r, ctx->current_request, p, len);
	}
	else {
		p = ngx_snprintf(p, len, ", server: %V", &ctx->connection->listening->addr_text);
	}
	return p;
}

static uchar * ngx_http_log_error_handler(ngx_http_request_t * r, ngx_http_request_t * sr, uchar * buf, size_t len)
{
	const char * uri_separator;
	ngx_http_upstream_t * u;
	ngx_http_core_srv_conf_t  * cscf = (ngx_http_core_srv_conf_t *)ngx_http_get_module_srv_conf(r, ngx_http_core_module);
	uchar * p = ngx_snprintf(buf, len, ", server: %V", &cscf->server_name);
	len -= p - buf;
	buf = p;
	if(r->request_line.data == NULL && r->request_start) {
		for(p = r->request_start; p < r->header_in->last; p++) {
			if(*p == __CR || *p == LF) {
				break;
			}
		}
		r->request_line.len = p - r->request_start;
		r->request_line.data = r->request_start;
	}
	if(r->request_line.len) {
		p = ngx_snprintf(buf, len, ", request: \"%V\"", &r->request_line);
		len -= p - buf;
		buf = p;
	}
	if(r != sr) {
		p = ngx_snprintf(buf, len, ", subrequest: \"%V\"", &sr->uri);
		len -= p - buf;
		buf = p;
	}
	u = sr->upstream;
	if(u && u->peer.name) {
		uri_separator = "";
#if (NGX_HAVE_UNIX_DOMAIN)
		if(u->peer.sockaddr && u->peer.sockaddr->sa_family == AF_UNIX) {
			uri_separator = ":";
		}
#endif
		p = ngx_snprintf(buf, len, ", upstream: \"%V%V%s%V\"", &u->schema, u->peer.name, uri_separator, &u->uri);
		len -= p - buf;
		buf = p;
	}
	if(r->headers_in.host) {
		p = ngx_snprintf(buf, len, ", host: \"%V\"", &r->headers_in.host->value);
		len -= p - buf;
		buf = p;
	}
	if(r->headers_in.referer) {
		p = ngx_snprintf(buf, len, ", referrer: \"%V\"", &r->headers_in.referer->value);
		buf = p;
	}
	return buf;
}
//
//
//
int ngx_http_request_t::SetContentType(SFileFormat fmt, SCodepage cp)
{
	int    ok = 1;
	SString temp_buf;
	if(SFileFormat::GetMime(fmt, temp_buf)) {
		if(cp != cpUndef) {
			SString cp_buf;
			SCodepageIdent cpi(cp);
			if(cpi.ToStr(SCodepageIdent::fmtXML, cp_buf)) {
				temp_buf.CatDiv(';', 2).CatEq("charset", cp_buf);
			}
		}
		SStrDupToNgxStr(pool, &temp_buf, &headers_out.content_type);
	}
	else 
		ok = 0;
	//pReq->headers_out.content_type.len = sizeof("text/html; charset=UTF-8") - 1;
	//pReq->headers_out.content_type.data = (uchar *)"text/html; charset=UTF-8";
	return ok;
}

int ngx_http_request_t::GetArg(const char * pName, SString & rValue) const
{
	rValue.Z();
	int    ok = 0;
	if(args.len) {
		SString temp_buf, key, val;
		temp_buf.CatN((const char *)args.data, args.len);
		StringSet ss("&");
		ss.setBuf(temp_buf);
		for(uint ssp = 0; !ok && ss.get(&ssp, temp_buf);) {
			if(temp_buf.Divide('=', key, val) > 0) {
				if(key.IsEqiAscii(pName)) {
					rValue = val;
					ok = 1;
				}
			}
		}
	}
	return ok;
}
