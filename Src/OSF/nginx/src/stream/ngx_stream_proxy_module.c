/*
 * Copyright (C) Roman Arutyunyan
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop
//#include <ngx_stream.h>

typedef struct {
	ngx_addr_t  * addr;
	ngx_stream_complex_value_t * value;
#if (NGX_HAVE_TRANSPARENT_PROXY)
	ngx_uint_t transparent; /* unsigned  transparent:1; */
#endif
} ngx_stream_upstream_local_t;

typedef struct {
	ngx_msec_t connect_timeout;
	ngx_msec_t timeout;
	ngx_msec_t next_upstream_timeout;
	size_t buffer_size;
	size_t upload_rate;
	size_t download_rate;
	ngx_uint_t responses;
	ngx_uint_t next_upstream_tries;
	ngx_flag_t next_upstream;
	ngx_flag_t proxy_protocol;
	ngx_stream_upstream_local_t   * local;

#if (NGX_STREAM_SSL)
	ngx_flag_t ssl_enable;
	ngx_flag_t ssl_session_reuse;
	ngx_uint_t ssl_protocols;
	ngx_str_t ssl_ciphers;
	ngx_stream_complex_value_t * ssl_name;
	ngx_flag_t ssl_server_name;

	ngx_flag_t ssl_verify;
	ngx_uint_t ssl_verify_depth;
	ngx_str_t ssl_trusted_certificate;
	ngx_str_t ssl_crl;
	ngx_str_t ssl_certificate;
	ngx_str_t ssl_certificate_key;
	ngx_array_t * ssl_passwords;

	ngx_ssl_t   * ssl;
#endif

	ngx_stream_upstream_srv_conf_t  * upstream;
	ngx_stream_complex_value_t * upstream_value;
} ngx_stream_proxy_srv_conf_t;

static void ngx_stream_proxy_handler(ngx_stream_session_t * s);
static ngx_int_t ngx_stream_proxy_eval(ngx_stream_session_t * s, ngx_stream_proxy_srv_conf_t * pscf);
static ngx_int_t ngx_stream_proxy_set_local(ngx_stream_session_t * s, ngx_stream_upstream_t * u, ngx_stream_upstream_local_t * local);
static void ngx_stream_proxy_connect(ngx_stream_session_t * s);
static void ngx_stream_proxy_init_upstream(ngx_stream_session_t * s);
static void ngx_stream_proxy_resolve_handler(ngx_resolver_ctx_t * ctx);
static void ngx_stream_proxy_upstream_handler(ngx_event_t * ev);
static void ngx_stream_proxy_downstream_handler(ngx_event_t * ev);
static void ngx_stream_proxy_process_connection(ngx_event_t * ev, ngx_uint_t from_upstream);
static void ngx_stream_proxy_connect_handler(ngx_event_t * ev);
static ngx_int_t ngx_stream_proxy_test_connect(ngx_connection_t * c);
static void ngx_stream_proxy_process(ngx_stream_session_t * s, ngx_uint_t from_upstream, ngx_uint_t do_write);
static void ngx_stream_proxy_next_upstream(ngx_stream_session_t * s);
static void ngx_stream_proxy_finalize(ngx_stream_session_t * s, ngx_uint_t rc);
static uchar * ngx_stream_proxy_log_error(ngx_log_t * log, uchar * buf, size_t len);

static void * ngx_stream_proxy_create_srv_conf(ngx_conf_t * cf);
static char * ngx_stream_proxy_merge_srv_conf(ngx_conf_t * cf, void * parent, void * child);
static const char * ngx_stream_proxy_pass(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static const char * ngx_stream_proxy_bind(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler

#if (NGX_STREAM_SSL)

static ngx_int_t ngx_stream_proxy_send_proxy_protocol(ngx_stream_session_t * s);
static const char * ngx_stream_proxy_ssl_password_file(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static void ngx_stream_proxy_ssl_init_connection(ngx_stream_session_t * s);
static void ngx_stream_proxy_ssl_handshake(ngx_connection_t * pc);
static ngx_int_t ngx_stream_proxy_ssl_name(ngx_stream_session_t * s);
static ngx_int_t ngx_stream_proxy_set_ssl(ngx_conf_t * cf, ngx_stream_proxy_srv_conf_t * pscf);

static ngx_conf_bitmask_t ngx_stream_proxy_ssl_protocols[] = {
	{ ngx_string("SSLv2"), NGX_SSL_SSLv2 },
	{ ngx_string("SSLv3"), NGX_SSL_SSLv3 },
	{ ngx_string("TLSv1"), NGX_SSL_TLSv1 },
	{ ngx_string("TLSv1.1"), NGX_SSL_TLSv1_1 },
	{ ngx_string("TLSv1.2"), NGX_SSL_TLSv1_2 },
	{ ngx_string("TLSv1.3"), NGX_SSL_TLSv1_3 },
	{ ngx_null_string, 0 }
};

#endif

static ngx_conf_deprecated_t ngx_conf_deprecated_proxy_downstream_buffer = {
	ngx_conf_deprecated, "proxy_downstream_buffer", "proxy_buffer_size"
};

static ngx_conf_deprecated_t ngx_conf_deprecated_proxy_upstream_buffer = {
	ngx_conf_deprecated, "proxy_upstream_buffer", "proxy_buffer_size"
};

static ngx_command_t ngx_stream_proxy_commands[] = {
	{ ngx_string("proxy_pass"), NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_stream_proxy_pass, NGX_STREAM_SRV_CONF_OFFSET, 0, NULL },
	{ ngx_string("proxy_bind"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE12,
	  ngx_stream_proxy_bind, NGX_STREAM_SRV_CONF_OFFSET, 0, NULL },
	{ ngx_string("proxy_connect_timeout"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_msec_slot, NGX_STREAM_SRV_CONF_OFFSET, offsetof(ngx_stream_proxy_srv_conf_t, connect_timeout), NULL },
	{ ngx_string("proxy_timeout"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_msec_slot, NGX_STREAM_SRV_CONF_OFFSET, offsetof(ngx_stream_proxy_srv_conf_t, timeout), NULL },
	{ ngx_string("proxy_buffer_size"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_size_slot, NGX_STREAM_SRV_CONF_OFFSET, offsetof(ngx_stream_proxy_srv_conf_t, buffer_size), NULL },
	{ ngx_string("proxy_downstream_buffer"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_size_slot, NGX_STREAM_SRV_CONF_OFFSET, offsetof(ngx_stream_proxy_srv_conf_t, buffer_size), &ngx_conf_deprecated_proxy_downstream_buffer },
	{ ngx_string("proxy_upstream_buffer"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_size_slot, NGX_STREAM_SRV_CONF_OFFSET, offsetof(ngx_stream_proxy_srv_conf_t, buffer_size), &ngx_conf_deprecated_proxy_upstream_buffer },
	{ ngx_string("proxy_upload_rate"),
	  NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_size_slot,
	  NGX_STREAM_SRV_CONF_OFFSET,
	  offsetof(ngx_stream_proxy_srv_conf_t, upload_rate),
	  NULL },

	{ ngx_string("proxy_download_rate"),
	  NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_size_slot,
	  NGX_STREAM_SRV_CONF_OFFSET,
	  offsetof(ngx_stream_proxy_srv_conf_t, download_rate),
	  NULL },

	{ ngx_string("proxy_responses"),
	  NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_num_slot,
	  NGX_STREAM_SRV_CONF_OFFSET,
	  offsetof(ngx_stream_proxy_srv_conf_t, responses),
	  NULL },

	{ ngx_string("proxy_next_upstream"),
	  NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_FLAG,
	  ngx_conf_set_flag_slot,
	  NGX_STREAM_SRV_CONF_OFFSET,
	  offsetof(ngx_stream_proxy_srv_conf_t, next_upstream),
	  NULL },

	{ ngx_string("proxy_next_upstream_tries"),
	  NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_num_slot,
	  NGX_STREAM_SRV_CONF_OFFSET,
	  offsetof(ngx_stream_proxy_srv_conf_t, next_upstream_tries),
	  NULL },

	{ ngx_string("proxy_next_upstream_timeout"),
	  NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_msec_slot,
	  NGX_STREAM_SRV_CONF_OFFSET,
	  offsetof(ngx_stream_proxy_srv_conf_t, next_upstream_timeout),
	  NULL },

	{ ngx_string("proxy_protocol"),
	  NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_FLAG,
	  ngx_conf_set_flag_slot,
	  NGX_STREAM_SRV_CONF_OFFSET,
	  offsetof(ngx_stream_proxy_srv_conf_t, proxy_protocol),
	  NULL },

#if (NGX_STREAM_SSL)

	{ngx_string("proxy_ssl"),
	 NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_FLAG,
	 ngx_conf_set_flag_slot,
	 NGX_STREAM_SRV_CONF_OFFSET,
	 offsetof(ngx_stream_proxy_srv_conf_t, ssl_enable),
	 NULL },

	{ ngx_string("proxy_ssl_session_reuse"),
	  NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_FLAG,
	  ngx_conf_set_flag_slot,
	  NGX_STREAM_SRV_CONF_OFFSET,
	  offsetof(ngx_stream_proxy_srv_conf_t, ssl_session_reuse),
	  NULL },

	{ ngx_string("proxy_ssl_protocols"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_1MORE,
	  ngx_conf_set_bitmask_slot, NGX_STREAM_SRV_CONF_OFFSET, offsetof(ngx_stream_proxy_srv_conf_t, ssl_protocols), &ngx_stream_proxy_ssl_protocols },
	{ ngx_string("proxy_ssl_ciphers"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_str_slot, NGX_STREAM_SRV_CONF_OFFSET, offsetof(ngx_stream_proxy_srv_conf_t, ssl_ciphers), NULL },
	{ ngx_string("proxy_ssl_name"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_stream_set_complex_value_slot, NGX_STREAM_SRV_CONF_OFFSET, offsetof(ngx_stream_proxy_srv_conf_t, ssl_name), NULL },
	{ ngx_string("proxy_ssl_server_name"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_FLAG,
	  ngx_conf_set_flag_slot, NGX_STREAM_SRV_CONF_OFFSET, offsetof(ngx_stream_proxy_srv_conf_t, ssl_server_name), NULL },
	{ ngx_string("proxy_ssl_verify"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_FLAG,
	  ngx_conf_set_flag_slot, NGX_STREAM_SRV_CONF_OFFSET, offsetof(ngx_stream_proxy_srv_conf_t, ssl_verify), NULL },
	{ ngx_string("proxy_ssl_verify_depth"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_num_slot, NGX_STREAM_SRV_CONF_OFFSET, offsetof(ngx_stream_proxy_srv_conf_t, ssl_verify_depth), NULL },
	{ ngx_string("proxy_ssl_trusted_certificate"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_str_slot, NGX_STREAM_SRV_CONF_OFFSET, offsetof(ngx_stream_proxy_srv_conf_t, ssl_trusted_certificate), NULL },
	{ ngx_string("proxy_ssl_crl"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_str_slot, NGX_STREAM_SRV_CONF_OFFSET, offsetof(ngx_stream_proxy_srv_conf_t, ssl_crl), NULL },
	{ ngx_string("proxy_ssl_certificate"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_str_slot, NGX_STREAM_SRV_CONF_OFFSET, offsetof(ngx_stream_proxy_srv_conf_t, ssl_certificate), NULL },
	{ ngx_string("proxy_ssl_certificate_key"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_str_slot, NGX_STREAM_SRV_CONF_OFFSET, offsetof(ngx_stream_proxy_srv_conf_t, ssl_certificate_key), NULL },
	{ ngx_string("proxy_ssl_password_file"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_stream_proxy_ssl_password_file, NGX_STREAM_SRV_CONF_OFFSET, 0, NULL },
#endif
	ngx_null_command
};

static ngx_stream_module_t ngx_stream_proxy_module_ctx = {
	NULL,                              /* preconfiguration */
	NULL,                              /* postconfiguration */

	NULL,                              /* create main configuration */
	NULL,                              /* init main configuration */

	ngx_stream_proxy_create_srv_conf,  /* create server configuration */
	ngx_stream_proxy_merge_srv_conf    /* merge server configuration */
};

ngx_module_t ngx_stream_proxy_module = {
	NGX_MODULE_V1,
	&ngx_stream_proxy_module_ctx,      /* module context */
	ngx_stream_proxy_commands,         /* module directives */
	NGX_STREAM_MODULE,                 /* module type */
	NULL,                              /* init master */
	NULL,                              /* init module */
	NULL,                              /* init process */
	NULL,                              /* init thread */
	NULL,                              /* exit thread */
	NULL,                              /* exit process */
	NULL,                              /* exit master */
	NGX_MODULE_V1_PADDING
};

static void ngx_stream_proxy_handler(ngx_stream_session_t * s)
{
	uchar  * p;
	ngx_str_t * host;
	ngx_uint_t i;
	ngx_connection_t * c;
	ngx_resolver_ctx_t * ctx, temp;
	ngx_stream_upstream_t  * u;
	ngx_stream_core_srv_conf_t  * cscf;
	ngx_stream_proxy_srv_conf_t * pscf;
	ngx_stream_upstream_srv_conf_t * uscf, ** uscfp;
	ngx_stream_upstream_main_conf_t  * umcf;
	c = s->connection;
	pscf = (ngx_stream_proxy_srv_conf_t *)ngx_stream_get_module_srv_conf(s, ngx_stream_proxy_module);
	ngx_log_debug0(NGX_LOG_DEBUG_STREAM, c->log, 0, "proxy connection handler");
	u = (ngx_stream_upstream_t *)ngx_pcalloc(c->pool, sizeof(ngx_stream_upstream_t));
	if(u == NULL) {
		ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
		return;
	}
	s->upstream = u;
	s->log_handler = ngx_stream_proxy_log_error;
	u->peer.log = c->log;
	u->peer.log_error = NGX_ERROR_ERR;
	if(ngx_stream_proxy_set_local(s, u, pscf->local) != NGX_OK) {
		ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
		return;
	}
	u->peer.type = c->type;
	u->start_sec = ngx_time();
	c->P_EvWr->F_EvHandler = ngx_stream_proxy_downstream_handler;
	c->P_EvRd->F_EvHandler = ngx_stream_proxy_downstream_handler;
	s->upstream_states = ngx_array_create(c->pool, 1, sizeof(ngx_stream_upstream_state_t));
	if(s->upstream_states == NULL) {
		ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
		return;
	}
	if(c->type == SOCK_STREAM) {
		p = (uchar *)ngx_pnalloc(c->pool, pscf->buffer_size);
		if(!p) {
			ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
			return;
		}
		u->downstream_buf.start = p;
		u->downstream_buf.end = p + pscf->buffer_size;
		u->downstream_buf.pos = p;
		u->downstream_buf.last = p;
		if(c->P_EvRd->ready) {
			ngx_post_event(c->P_EvRd, &ngx_posted_events);
		}
	}
	if(pscf->upstream_value) {
		if(ngx_stream_proxy_eval(s, pscf) != NGX_OK) {
			ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
			return;
		}
	}
	if(u->resolved == NULL) {
		uscf = pscf->upstream;
	}
	else {
#if (NGX_STREAM_SSL)
		u->ssl_name = u->resolved->host;
#endif
		host = &u->resolved->host;
		umcf = (ngx_stream_upstream_main_conf_t *)ngx_stream_get_module_main_conf(s, ngx_stream_upstream_module);
		uscfp = (ngx_stream_upstream_srv_conf_t **)umcf->upstreams.elts;
		for(i = 0; i < umcf->upstreams.nelts; i++) {
			uscf = uscfp[i];
			if(uscf->host.len == host->len && ((uscf->port == 0 && u->resolved->no_port) || uscf->port == u->resolved->port)
			    && ngx_strncasecmp(uscf->host.data, host->data, host->len) == 0) {
				goto found;
			}
		}

		if(u->resolved->sockaddr) {
			if(u->resolved->port == 0 && u->resolved->sockaddr->sa_family != AF_UNIX) {
				ngx_log_error(NGX_LOG_ERR, c->log, 0, "no port in upstream \"%V\"", host);
				ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
				return;
			}
			if(ngx_stream_upstream_create_round_robin_peer(s, u->resolved) != NGX_OK) {
				ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
				return;
			}
			ngx_stream_proxy_connect(s);
			return;
		}
		if(u->resolved->port == 0) {
			ngx_log_error(NGX_LOG_ERR, c->log, 0, "no port in upstream \"%V\"", host);
			ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
			return;
		}
		temp.name = *host;
		cscf = (ngx_stream_core_srv_conf_t *)ngx_stream_get_module_srv_conf(s, ngx_stream_core_module);
		ctx = ngx_resolve_start(cscf->resolver, &temp);
		if(!ctx) {
			ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
			return;
		}
		if(ctx == NGX_NO_RESOLVER) {
			ngx_log_error(NGX_LOG_ERR, c->log, 0, "no resolver defined to resolve %V", host);
			ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
			return;
		}
		ctx->name = *host;
		ctx->handler = ngx_stream_proxy_resolve_handler;
		ctx->data = s;
		ctx->timeout = cscf->resolver_timeout;
		u->resolved->ctx = ctx;
		if(ngx_resolve_name(ctx) != NGX_OK) {
			u->resolved->ctx = NULL;
			ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
			return;
		}
		return;
	}
found:
	if(uscf == NULL) {
		ngx_log_error(NGX_LOG_ALERT, c->log, 0, "no upstream configuration");
		ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
		return;
	}
	u->upstream = uscf;
#if (NGX_STREAM_SSL)
	u->ssl_name = uscf->host;
#endif
	if(uscf->peer.init(s, uscf) != NGX_OK) {
		ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
		return;
	}
	u->peer.start_time = ngx_current_msec;
	if(pscf->next_upstream_tries
	    && u->peer.tries > pscf->next_upstream_tries) {
		u->peer.tries = pscf->next_upstream_tries;
	}
	ngx_stream_proxy_connect(s);
}

static ngx_int_t ngx_stream_proxy_eval(ngx_stream_session_t * s, ngx_stream_proxy_srv_conf_t * pscf)
{
	ngx_str_t host;
	ngx_url_t url;
	ngx_stream_upstream_t  * u;
	if(ngx_stream_complex_value(s, pscf->upstream_value, &host) != NGX_OK) {
		return NGX_ERROR;
	}
	memzero(&url, sizeof(ngx_url_t));
	url.url = host;
	url.no_resolve = 1;
	if(ngx_parse_url(s->connection->pool, &url) != NGX_OK) {
		if(url.err) {
			ngx_log_error(NGX_LOG_ERR, s->connection->log, 0, "%s in upstream \"%V\"", url.err, &url.url);
		}
		return NGX_ERROR;
	}
	u = s->upstream;
	u->resolved = (ngx_stream_upstream_resolved_t *)ngx_pcalloc(s->connection->pool, sizeof(ngx_stream_upstream_resolved_t));
	if(u->resolved == NULL) {
		return NGX_ERROR;
	}
	if(url.addrs) {
		u->resolved->sockaddr = url.addrs[0].sockaddr;
		u->resolved->socklen = url.addrs[0].socklen;
		u->resolved->name = url.addrs[0].name;
		u->resolved->naddrs = 1;
	}
	u->resolved->host = url.host;
	u->resolved->port = url.port;
	u->resolved->no_port = url.no_port;
	return NGX_OK;
}

static ngx_int_t ngx_stream_proxy_set_local(ngx_stream_session_t * s, ngx_stream_upstream_t * u,
    ngx_stream_upstream_local_t * local)
{
	ngx_int_t rc;
	ngx_str_t val;
	ngx_addr_t  * addr;

	if(local == NULL) {
		u->peer.local = NULL;
		return NGX_OK;
	}

#if (NGX_HAVE_TRANSPARENT_PROXY)
	u->peer.transparent = local->transparent;
#endif
	if(local->value == NULL) {
		u->peer.local = local->addr;
		return NGX_OK;
	}
	if(ngx_stream_complex_value(s, local->value, &val) != NGX_OK) {
		return NGX_ERROR;
	}
	if(val.len == 0) {
		return NGX_OK;
	}
	addr = (ngx_addr_t *)ngx_palloc(s->connection->pool, sizeof(ngx_addr_t));
	if(addr == NULL) {
		return NGX_ERROR;
	}
	rc = ngx_parse_addr_port(s->connection->pool, addr, val.data, val.len);
	if(rc == NGX_ERROR) {
		return NGX_ERROR;
	}
	if(rc != NGX_OK) {
		ngx_log_error(NGX_LOG_ERR, s->connection->log, 0, "invalid local address \"%V\"", &val);
		return NGX_OK;
	}
	addr->name = val;
	u->peer.local = addr;
	return NGX_OK;
}

static void ngx_stream_proxy_connect(ngx_stream_session_t * s)
{
	ngx_int_t rc;
	ngx_connection_t * pc;
	ngx_stream_upstream_t * u;
	ngx_stream_proxy_srv_conf_t  * pscf;
	ngx_connection_t * c = s->connection;
	c->log->action = "connecting to upstream";
	pscf = (ngx_stream_proxy_srv_conf_t *)ngx_stream_get_module_srv_conf(s, ngx_stream_proxy_module);
	u = s->upstream;
	u->connected = 0;
	u->proxy_protocol = pscf->proxy_protocol;
	if(u->state) {
		u->state->response_time = ngx_current_msec - u->state->response_time;
	}
	u->state = (ngx_stream_upstream_state_t *)ngx_array_push(s->upstream_states);
	if(u->state == NULL) {
		ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
		return;
	}
	memzero(u->state, sizeof(ngx_stream_upstream_state_t));
	u->state->connect_time = (ngx_msec_t)-1;
	u->state->first_byte_time = (ngx_msec_t)-1;
	u->state->response_time = ngx_current_msec;
	rc = ngx_event_connect_peer(&u->peer);
	ngx_log_debug1(NGX_LOG_DEBUG_STREAM, c->log, 0, "proxy connect: %i", rc);
	if(rc == NGX_ERROR) {
		ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
		return;
	}
	u->state->peer = u->peer.name;
	if(rc == NGX_BUSY) {
		ngx_log_error(NGX_LOG_ERR, c->log, 0, "no live upstreams");
		ngx_stream_proxy_finalize(s, NGX_STREAM_BAD_GATEWAY);
		return;
	}

	if(rc == NGX_DECLINED) {
		ngx_stream_proxy_next_upstream(s);
		return;
	}
	/* rc == NGX_OK || rc == NGX_AGAIN || rc == NGX_DONE */
	pc = u->peer.connection;
	pc->data = s;
	pc->log = c->log;
	pc->pool = c->pool;
	pc->P_EvRd->log = c->log;
	pc->P_EvWr->log = c->log;
	if(rc != NGX_AGAIN) {
		ngx_stream_proxy_init_upstream(s);
		return;
	}
	pc->P_EvRd->F_EvHandler = ngx_stream_proxy_connect_handler;
	pc->P_EvWr->F_EvHandler = ngx_stream_proxy_connect_handler;
	ngx_add_timer(pc->P_EvWr, pscf->connect_timeout);
}

static void ngx_stream_proxy_init_upstream(ngx_stream_session_t * s)
{
	uchar   * p;
	ngx_chain_t  * cl;
	ngx_connection_t   * c, * pc;
	ngx_log_handler_pt handler;
	ngx_stream_upstream_t * u;
	ngx_stream_core_srv_conf_t * cscf;
	ngx_stream_proxy_srv_conf_t  * pscf;
	u = s->upstream;
	pc = u->peer.connection;
	cscf = (ngx_stream_core_srv_conf_t *)ngx_stream_get_module_srv_conf(s, ngx_stream_core_module);
	if(pc->type == SOCK_STREAM && cscf->tcp_nodelay && ngx_tcp_nodelay(pc) != NGX_OK) {
		ngx_stream_proxy_next_upstream(s);
		return;
	}
	pscf = (ngx_stream_proxy_srv_conf_t *)ngx_stream_get_module_srv_conf(s, ngx_stream_proxy_module);
#if (NGX_STREAM_SSL)
	if(pc->type == SOCK_STREAM && pscf->ssl) {
		if(u->proxy_protocol) {
			if(ngx_stream_proxy_send_proxy_protocol(s) != NGX_OK) {
				return;
			}
			u->proxy_protocol = 0;
		}
		if(pc->ssl == NULL) {
			ngx_stream_proxy_ssl_init_connection(s);
			return;
		}
	}
#endif
	c = s->connection;
	if(c->log->Level >= NGX_LOG_INFO) {
		ngx_str_t str;
		uchar addr[NGX_SOCKADDR_STRLEN];
		str.len = NGX_SOCKADDR_STRLEN;
		str.data = addr;
		if(ngx_connection_local_sockaddr(pc, &str, 1) == NGX_OK) {
			handler = c->log->handler;
			c->log->handler = NULL;
			ngx_log_error(NGX_LOG_INFO, c->log, 0, "%sproxy %V connected to %V", pc->type == SOCK_DGRAM ? "udp " : "", &str, u->peer.name);
			c->log->handler = handler;
		}
	}
	u->state->connect_time = ngx_current_msec - u->state->response_time;
	if(u->peer.notify) {
		u->peer.notify(&u->peer, u->peer.data, NGX_STREAM_UPSTREAM_NOTIFY_CONNECT);
	}
	c->log->action = "proxying connection";
	if(u->upstream_buf.start == NULL) {
		p = (uchar *)ngx_pnalloc(c->pool, pscf->buffer_size);
		if(!p) {
			ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
			return;
		}
		u->upstream_buf.start = p;
		u->upstream_buf.end = p + pscf->buffer_size;
		u->upstream_buf.pos = p;
		u->upstream_buf.last = p;
	}
	if(c->buffer && c->buffer->pos < c->buffer->last) {
		ngx_log_debug1(NGX_LOG_DEBUG_STREAM, c->log, 0, "stream proxy add preread buffer: %uz", c->buffer->last - c->buffer->pos);
		cl = ngx_chain_get_free_buf(c->pool, &u->P_Free);
		if(cl == NULL) {
			ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
			return;
		}
		*cl->buf = *c->buffer;
		cl->buf->tag = (ngx_buf_tag_t)&ngx_stream_proxy_module;
		cl->buf->flush = 1;
		cl->buf->last_buf = (c->type == SOCK_DGRAM);
		cl->next = u->upstream_out;
		u->upstream_out = cl;
	}
	if(u->proxy_protocol) {
		ngx_log_debug0(NGX_LOG_DEBUG_STREAM, c->log, 0, "stream proxy add PROXY protocol header");
		cl = ngx_chain_get_free_buf(c->pool, &u->P_Free);
		if(cl == NULL) {
			ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
			return;
		}
		p = (uchar *)ngx_pnalloc(c->pool, NGX_PROXY_PROTOCOL_MAX_HEADER);
		if(!p) {
			ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
			return;
		}
		cl->buf->pos = p;
		p = ngx_proxy_protocol_write(c, p, p + NGX_PROXY_PROTOCOL_MAX_HEADER);
		if(!p) {
			ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
			return;
		}
		cl->buf->last = p;
		cl->buf->temporary = 1;
		cl->buf->flush = 0;
		cl->buf->last_buf = 0;
		cl->buf->tag = (ngx_buf_tag_t)&ngx_stream_proxy_module;
		cl->next = u->upstream_out;
		u->upstream_out = cl;
		u->proxy_protocol = 0;
	}
	if(c->type == SOCK_DGRAM && pscf->responses == 0) {
		pc->P_EvRd->ready = 0;
		pc->P_EvRd->eof = 1;
	}
	u->connected = 1;
	pc->P_EvRd->F_EvHandler = ngx_stream_proxy_upstream_handler;
	pc->P_EvWr->F_EvHandler = ngx_stream_proxy_upstream_handler;
	if(pc->P_EvRd->ready || pc->P_EvRd->eof) {
		ngx_post_event(pc->P_EvRd, &ngx_posted_events);
	}
	ngx_stream_proxy_process(s, 0, 1);
}

#if (NGX_STREAM_SSL)

static ngx_int_t ngx_stream_proxy_send_proxy_protocol(ngx_stream_session_t * s)
{
	uchar * p;
	ssize_t n, size;
	ngx_connection_t * pc;
	ngx_stream_upstream_t * u;
	ngx_stream_proxy_srv_conf_t  * pscf;
	uchar buf[NGX_PROXY_PROTOCOL_MAX_HEADER];
	ngx_connection_t * c = s->connection;
	ngx_log_debug0(NGX_LOG_DEBUG_STREAM, c->log, 0, "stream proxy send PROXY protocol header");
	p = ngx_proxy_protocol_write(c, buf, buf + NGX_PROXY_PROTOCOL_MAX_HEADER);
	if(!p) {
		ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
		return NGX_ERROR;
	}
	u = s->upstream;
	pc = u->peer.connection;
	size = p - buf;
	n = pc->send(pc, buf, size);
	if(n == NGX_AGAIN) {
		if(ngx_handle_write_event(pc->P_EvWr, 0) != NGX_OK) {
			ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
			return NGX_ERROR;
		}
		pscf = (ngx_stream_proxy_srv_conf_t *)ngx_stream_get_module_srv_conf(s, ngx_stream_proxy_module);
		ngx_add_timer(pc->P_EvWr, pscf->timeout);
		pc->P_EvWr->F_EvHandler = ngx_stream_proxy_connect_handler;
		return NGX_AGAIN;
	}
	if(n == NGX_ERROR) {
		ngx_stream_proxy_finalize(s, NGX_STREAM_OK);
		return NGX_ERROR;
	}
	if(n != size) {
		/*
		 * PROXY protocol specification:
		 * The sender must always ensure that the header
		 * is sent at once, so that the transport layer
		 * maintains atomicity along the path to the receiver.
		 */
		ngx_log_error(NGX_LOG_ERR, c->log, 0, "could not send PROXY protocol header at once");
		ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
		return NGX_ERROR;
	}
	return NGX_OK;
}

static const char * ngx_stream_proxy_ssl_password_file(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	ngx_stream_proxy_srv_conf_t * pscf = (ngx_stream_proxy_srv_conf_t *)conf;
	if(pscf->ssl_passwords != NGX_CONF_UNSET_PTR) {
		return "is duplicate";
	}
	else {
		ngx_str_t * value = static_cast<ngx_str_t *>(cf->args->elts);
		pscf->ssl_passwords = ngx_ssl_read_password_file(cf, &value[1]);
		if(pscf->ssl_passwords == NULL) {
			return NGX_CONF_ERROR;
		}
		return NGX_CONF_OK;
	}
}

static void ngx_stream_proxy_ssl_init_connection(ngx_stream_session_t * s)
{
	ngx_int_t rc;
	ngx_stream_upstream_t * u = s->upstream;
	ngx_connection_t * pc = u->peer.connection;
	ngx_stream_proxy_srv_conf_t  * pscf = (ngx_stream_proxy_srv_conf_t *)ngx_stream_get_module_srv_conf(s, ngx_stream_proxy_module);
	if(ngx_ssl_create_connection(pscf->ssl, pc, NGX_SSL_BUFFER|NGX_SSL_CLIENT) != NGX_OK) {
		ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
		return;
	}
	if(pscf->ssl_server_name || pscf->ssl_verify) {
		if(ngx_stream_proxy_ssl_name(s) != NGX_OK) {
			ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
			return;
		}
	}
	if(pscf->ssl_session_reuse) {
		if(u->peer.set_session(&u->peer, u->peer.data) != NGX_OK) {
			ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
			return;
		}
	}
	s->connection->log->action = "SSL handshaking to upstream";
	rc = ngx_ssl_handshake(pc);
	if(rc == NGX_AGAIN) {
		if(!pc->P_EvWr->timer_set) {
			ngx_add_timer(pc->P_EvWr, pscf->connect_timeout);
		}
		pc->ssl->handler = ngx_stream_proxy_ssl_handshake;
		return;
	}
	ngx_stream_proxy_ssl_handshake(pc);
}

static void ngx_stream_proxy_ssl_handshake(ngx_connection_t * pc)
{
	long rc;
	ngx_stream_upstream_t * u;
	ngx_stream_session_t  * s = (ngx_stream_session_t *)pc->data;
	ngx_stream_proxy_srv_conf_t  * pscf = (ngx_stream_proxy_srv_conf_t *)ngx_stream_get_module_srv_conf(s, ngx_stream_proxy_module);
	if(pc->ssl->handshaked) {
		if(pscf->ssl_verify) {
			rc = SSL_get_verify_result(pc->ssl->connection);
			if(rc != X509_V_OK) {
				ngx_log_error(NGX_LOG_ERR, pc->log, 0, "upstream SSL certificate verify error: (%l:%s)", rc, X509_verify_cert_error_string(rc));
				goto failed;
			}
			u = s->upstream;
			if(ngx_ssl_check_host(pc, &u->ssl_name) != NGX_OK) {
				ngx_log_error(NGX_LOG_ERR, pc->log, 0, "upstream SSL certificate does not match \"%V\"", &u->ssl_name);
				goto failed;
			}
		}
		if(pscf->ssl_session_reuse) {
			u = s->upstream;
			u->peer.save_session(&u->peer, u->peer.data);
		}
		if(pc->P_EvWr->timer_set) {
			ngx_del_timer(pc->P_EvWr);
		}
		ngx_stream_proxy_init_upstream(s);
		return;
	}
failed:
	ngx_stream_proxy_next_upstream(s);
}

static ngx_int_t ngx_stream_proxy_ssl_name(ngx_stream_session_t * s)
{
	uchar   * p, * last;
	ngx_str_t name;
	ngx_stream_proxy_srv_conf_t  * pscf = (ngx_stream_proxy_srv_conf_t *)ngx_stream_get_module_srv_conf(s, ngx_stream_proxy_module);
	ngx_stream_upstream_t * u = s->upstream;
	if(pscf->ssl_name) {
		if(ngx_stream_complex_value(s, pscf->ssl_name, &name) != NGX_OK) {
			return NGX_ERROR;
		}
	}
	else {
		name = u->ssl_name;
	}
	if(name.len == 0) {
		goto done;
	}
	/*
	 * ssl name here may contain port, strip it for compatibility
	 * with the http module
	 */
	p = name.data;
	last = name.data + name.len;
	if(*p == '[') {
		p = ngx_strlchr(p, last, ']');
		if(!p) {
			p = name.data;
		}
	}
	p = ngx_strlchr(p, last, ':');
	if(p != NULL) {
		name.len = p - name.data;
	}
	if(!pscf->ssl_server_name) {
		goto done;
	}
#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
	/* as per RFC 6066, literal IPv4 and IPv6 addresses are not permitted */
	if(name.len == 0 || *name.data == '[') {
		goto done;
	}
	if(ngx_inet_addr(name.data, name.len) != INADDR_NONE) {
		goto done;
	}
	/*
	 * SSL_set_tlsext_host_name() needs a null-terminated string,
	 * hence we explicitly null-terminate name here
	 */
	p = (uchar *)ngx_pnalloc(s->connection->pool, name.len + 1);
	if(!p) {
		return NGX_ERROR;
	}
	(void)ngx_cpystrn(p, name.data, name.len + 1);
	name.data = p;
	ngx_log_debug1(NGX_LOG_DEBUG_STREAM, s->connection->log, 0, "upstream SSL server name: \"%s\"", name.data);
	if(SSL_set_tlsext_host_name(u->peer.connection->ssl->connection, (char *)name.data) == 0) {
		ngx_ssl_error(NGX_LOG_ERR, s->connection->log, 0, "SSL_set_tlsext_host_name(\"%s\") failed", name.data);
		return NGX_ERROR;
	}
#endif
done:
	u->ssl_name = name;
	return NGX_OK;
}

#endif

static void ngx_stream_proxy_downstream_handler(ngx_event_t * ev)
{
	ngx_stream_proxy_process_connection(ev, ev->write);
}

static void ngx_stream_proxy_resolve_handler(ngx_resolver_ctx_t * ctx)
{
	ngx_stream_proxy_srv_conf_t   * pscf;
	ngx_stream_session_t  * s = (ngx_stream_session_t *)ctx->data;
	ngx_stream_upstream_t * u = s->upstream;
	ngx_stream_upstream_resolved_t  * ur = u->resolved;
	ngx_log_debug0(NGX_LOG_DEBUG_STREAM, s->connection->log, 0, "stream upstream resolve");
	if(ctx->state) {
		ngx_log_error(NGX_LOG_ERR, s->connection->log, 0, "%V could not be resolved (%i: %s)", &ctx->name, ctx->state, ngx_resolver_strerror(ctx->state));
		ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
		return;
	}
	ur->naddrs = ctx->naddrs;
	ur->addrs = ctx->addrs;
#if (NGX_DEBUG)
	{
		uchar text[NGX_SOCKADDR_STRLEN];
		ngx_str_t addr;
		ngx_uint_t i;
		addr.data = text;
		for(i = 0; i < ctx->naddrs; i++) {
			addr.len = ngx_sock_ntop(ur->addrs[i].sockaddr, ur->addrs[i].socklen, text, NGX_SOCKADDR_STRLEN, 0);
			ngx_log_debug1(NGX_LOG_DEBUG_STREAM, s->connection->log, 0, "name was resolved to %V", &addr);
		}
	}
#endif
	if(ngx_stream_upstream_create_round_robin_peer(s, ur) != NGX_OK) {
		ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
		return;
	}
	ngx_resolve_name_done(ctx);
	ur->ctx = NULL;
	u->peer.start_time = ngx_current_msec;
	pscf = (ngx_stream_proxy_srv_conf_t *)ngx_stream_get_module_srv_conf(s, ngx_stream_proxy_module);
	if(pscf->next_upstream_tries && u->peer.tries > pscf->next_upstream_tries) {
		u->peer.tries = pscf->next_upstream_tries;
	}
	ngx_stream_proxy_connect(s);
}

static void ngx_stream_proxy_upstream_handler(ngx_event_t * ev)
{
	ngx_stream_proxy_process_connection(ev, !ev->write);
}

static void ngx_stream_proxy_process_connection(ngx_event_t * ev, ngx_uint_t from_upstream)
{
	ngx_connection_t * pc;
	ngx_stream_proxy_srv_conf_t  * pscf;
	ngx_connection_t * c = (ngx_connection_t *)ev->P_Data;
	ngx_stream_session_t  * s = (ngx_stream_session_t *)c->data;
	ngx_stream_upstream_t * u = s->upstream;
	c = s->connection;
	pc = u->peer.connection;
	pscf = (ngx_stream_proxy_srv_conf_t *)ngx_stream_get_module_srv_conf(s, ngx_stream_proxy_module);
	if(ev->timedout) {
		ev->timedout = 0;
		if(ev->delayed) {
			ev->delayed = 0;
			if(!ev->ready) {
				if(ngx_handle_read_event(ev, 0) != NGX_OK) {
					ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
					return;
				}
				if(u->connected && !c->P_EvRd->delayed && !pc->P_EvRd->delayed) {
					ngx_add_timer(c->P_EvWr, pscf->timeout);
				}
				return;
			}
		}
		else {
			if(s->connection->type == SOCK_DGRAM) {
				if(pscf->responses == NGX_MAX_INT32_VALUE) {
					/*
					 * successfully terminate timed out UDP session
					 * with unspecified number of responses
					 */
					pc->P_EvRd->ready = 0;
					pc->P_EvRd->eof = 1;
					ngx_stream_proxy_process(s, 1, 0);
					return;
				}
				if(u->received == 0) {
					ngx_stream_proxy_next_upstream(s);
					return;
				}
			}
			ngx_connection_error(c, NGX_ETIMEDOUT, "connection timed out");
			ngx_stream_proxy_finalize(s, NGX_STREAM_OK);
			return;
		}
	}
	else if(ev->delayed) {
		ngx_log_debug0(NGX_LOG_DEBUG_STREAM, c->log, 0, "stream connection delayed");
		if(ngx_handle_read_event(ev, 0) != NGX_OK) {
			ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
		}
		return;
	}
	if(from_upstream && !u->connected) {
		return;
	}
	ngx_stream_proxy_process(s, from_upstream, ev->write);
}

static void ngx_stream_proxy_connect_handler(ngx_event_t * ev)
{
	ngx_connection_t * c = (ngx_connection_t *)ev->P_Data;
	ngx_stream_session_t * s = (ngx_stream_session_t *)c->data;
	if(ev->timedout) {
		ngx_log_error(NGX_LOG_ERR, c->log, NGX_ETIMEDOUT, "upstream timed out");
		ngx_stream_proxy_next_upstream(s);
	}
	else {
		ngx_del_timer(c->P_EvWr);
		ngx_log_debug0(NGX_LOG_DEBUG_STREAM, c->log, 0, "stream proxy connect upstream");
		if(ngx_stream_proxy_test_connect(c) != NGX_OK)
			ngx_stream_proxy_next_upstream(s);
		else
			ngx_stream_proxy_init_upstream(s);
	}
}

static ngx_int_t ngx_stream_proxy_test_connect(ngx_connection_t * c)
{
	int err;
	socklen_t len;
#if (NGX_HAVE_KQUEUE)
	if(ngx_event_flags & NGX_USE_KQUEUE_EVENT) {
		err = c->write->kq_errno ? c->write->kq_errno : c->read->kq_errno;
		if(err) {
			(void)ngx_connection_error(c, err, "kevent() reported that connect() failed");
			return NGX_ERROR;
		}
	}
	else
#endif
	{
		err = 0;
		len = sizeof(int);
		/*
		 * BSDs and Linux return 0 and set a pending error in err
		 * Solaris returns -1 and sets errno
		 */
		if(getsockopt(c->fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len) == -1) {
			err = ngx_socket_errno;
		}
		if(err) {
			(void)ngx_connection_error(c, err, "connect() failed");
			return NGX_ERROR;
		}
	}
	return NGX_OK;
}

static void ngx_stream_proxy_process(ngx_stream_session_t * s, ngx_uint_t from_upstream, ngx_uint_t do_write)
{
	nginx_off_t * received, limit;
	size_t size, limit_rate;
	ssize_t n;
	ngx_buf_t * b;
	ngx_int_t rc;
	ngx_uint_t flags;
	ngx_msec_t delay;
	ngx_chain_t  * cl, ** ll, ** out, ** busy;
	ngx_connection_t * src, * dst;
	ngx_log_handler_pt handler;
	ngx_stream_upstream_t * u = s->upstream;
	ngx_connection_t * c = s->connection;
	ngx_connection_t * pc = u->connected ? u->peer.connection : NULL;
	if(c->type == SOCK_DGRAM && (ngx_terminate || ngx_exiting)) {
		// socket is already closed on worker shutdown 
		handler = c->log->handler;
		c->log->handler = NULL;
		ngx_log_error(NGX_LOG_INFO, c->log, 0, "disconnected on shutdown");
		c->log->handler = handler;
		ngx_stream_proxy_finalize(s, NGX_STREAM_OK);
	}
	else {
		ngx_stream_proxy_srv_conf_t * pscf = (ngx_stream_proxy_srv_conf_t *)ngx_stream_get_module_srv_conf(s, ngx_stream_proxy_module);
		if(from_upstream) {
			src = pc;
			dst = c;
			b = &u->upstream_buf;
			limit_rate = pscf->download_rate;
			received = (nginx_off_t *)&u->received;
			out = &u->downstream_out;
			busy = &u->downstream_busy;
		}
		else {
			src = c;
			dst = pc;
			b = &u->downstream_buf;
			limit_rate = pscf->upload_rate;
			received = (nginx_off_t *)&s->received;
			out = &u->upstream_out;
			busy = &u->upstream_busy;
		}
		for(;;) {
			if(do_write && dst) {
				if(*out || *busy || dst->buffered) {
					rc = ngx_stream_top_filter(s, *out, from_upstream);
					if(rc == NGX_ERROR) {
						if(c->type == SOCK_DGRAM && !from_upstream)
							ngx_stream_proxy_next_upstream(s);
						else 
							ngx_stream_proxy_finalize(s, NGX_STREAM_OK);
						return;
					}
					ngx_chain_update_chains(c->pool, &u->P_Free, busy, out, (ngx_buf_tag_t)&ngx_stream_proxy_module);
					if(*busy == NULL) {
						b->pos = b->start;
						b->last = b->start;
					}
				}
			}
			size = b->end - b->last;
			if(size && src->P_EvRd->ready && !src->P_EvRd->delayed && !src->P_EvRd->error) {
				if(limit_rate) {
					limit = (nginx_off_t)limit_rate * (ngx_time() - u->start_sec + 1) - *received;
					if(limit <= 0) {
						src->P_EvRd->delayed = 1;
						delay = (ngx_msec_t)(-limit * 1000 / limit_rate + 1);
						ngx_add_timer(src->P_EvRd, delay);
						break;
					}
					if((nginx_off_t)size > limit) {
						size = (size_t)limit;
					}
				}
				n = src->recv(src, b->last, size);
				if(n == NGX_AGAIN) {
					break;
				}
				if(n == NGX_ERROR) {
					if(c->type == SOCK_DGRAM && u->received == 0) {
						ngx_stream_proxy_next_upstream(s);
						return;
					}
					src->P_EvRd->eof = 1;
					n = 0;
				}
				if(n >= 0) {
					if(limit_rate) {
						delay = (ngx_msec_t)(n * 1000 / limit_rate);
						if(delay > 0) {
							src->P_EvRd->delayed = 1;
							ngx_add_timer(src->P_EvRd, delay);
						}
					}
					if(from_upstream) {
						if(u->state->first_byte_time == (ngx_msec_t)-1) {
							u->state->first_byte_time = ngx_current_msec - u->state->response_time;
						}
					}
					if(c->type == SOCK_DGRAM && ++u->responses == pscf->responses) {
						src->P_EvRd->ready = 0;
						src->P_EvRd->eof = 1;
					}
					for(ll = out; *ll; ll = &(*ll)->next) { /* void */
					}
					cl = ngx_chain_get_free_buf(c->pool, &u->P_Free);
					if(cl == NULL) {
						ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
						return;
					}
					*ll = cl;
					cl->buf->pos = b->last;
					cl->buf->last = b->last + n;
					cl->buf->tag = (ngx_buf_tag_t)&ngx_stream_proxy_module;
					cl->buf->temporary = (n ? 1 : 0);
					cl->buf->last_buf = src->P_EvRd->eof;
					cl->buf->flush = 1;
					*received += n;
					b->last += n;
					do_write = 1;
					continue;
				}
			}
			break;
		}
		if(src->P_EvRd->eof && dst && (dst->P_EvRd->eof || !dst->buffered)) {
			handler = c->log->handler;
			c->log->handler = NULL;
			ngx_log_error(NGX_LOG_INFO, c->log, 0, "%s%s disconnected, bytes from/to client:%O/%O, bytes from/to upstream:%O/%O",
				src->type == SOCK_DGRAM ? "udp " : "", from_upstream ? "upstream" : "client", s->received, c->sent, u->received, pc ? pc->sent : 0);
			c->log->handler = handler;
			ngx_stream_proxy_finalize(s, NGX_STREAM_OK);
		}
		else {
			flags = src->P_EvRd->eof ? NGX_CLOSE_EVENT : 0;
			if(!src->shared && ngx_handle_read_event(src->P_EvRd, flags) != NGX_OK) {
				ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
			}
			else if(dst) {
				if(!dst->shared && ngx_handle_write_event(dst->P_EvWr, 0) != NGX_OK) {
					ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
				}
				else {
					if(!c->P_EvRd->delayed && !pc->P_EvRd->delayed) {
						ngx_add_timer(c->P_EvWr, pscf->timeout);
					}
					else if(c->P_EvWr->timer_set) {
						ngx_del_timer(c->P_EvWr);
					}
				}
			}
		}
	}
}

static void ngx_stream_proxy_next_upstream(ngx_stream_session_t * s)
{
	ngx_msec_t timeout;
	ngx_connection_t   * pc;
	ngx_stream_upstream_t * u;
	ngx_stream_proxy_srv_conf_t  * pscf;
	ngx_log_debug0(NGX_LOG_DEBUG_STREAM, s->connection->log, 0, "stream proxy next upstream");
	u = s->upstream;
	pc = u->peer.connection;
	if(u->upstream_out || u->upstream_busy || (pc && pc->buffered)) {
		ngx_log_error(NGX_LOG_ERR, s->connection->log, 0, "pending buffers on next upstream");
		ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
		return;
	}
	if(u->peer.sockaddr) {
		u->peer.free(&u->peer, u->peer.data, NGX_PEER_FAILED);
		u->peer.sockaddr = NULL;
	}
	pscf = (ngx_stream_proxy_srv_conf_t *)ngx_stream_get_module_srv_conf(s, ngx_stream_proxy_module);
	timeout = pscf->next_upstream_timeout;
	if(u->peer.tries == 0 || !pscf->next_upstream || (timeout && ngx_current_msec - u->peer.start_time >= timeout)) {
		ngx_stream_proxy_finalize(s, NGX_STREAM_BAD_GATEWAY);
		return;
	}
	if(pc) {
		ngx_log_debug1(NGX_LOG_DEBUG_STREAM, s->connection->log, 0, "close proxy upstream connection: %d", pc->fd);
#if (NGX_STREAM_SSL)
		if(pc->ssl) {
			pc->ssl->no_wait_shutdown = 1;
			pc->ssl->no_send_shutdown = 1;
			(void)ngx_ssl_shutdown(pc);
		}
#endif
		u->state->bytes_received = u->received;
		u->state->bytes_sent = pc->sent;
		ngx_close_connection(pc);
		u->peer.connection = NULL;
	}
	ngx_stream_proxy_connect(s);
}

static void ngx_stream_proxy_finalize(ngx_stream_session_t * s, ngx_uint_t rc)
{
	ngx_connection_t  * pc;
	ngx_stream_upstream_t  * u;
	ngx_log_debug1(NGX_LOG_DEBUG_STREAM, s->connection->log, 0, "finalize stream proxy: %i", rc);
	u = s->upstream;
	if(u == NULL) {
		goto noupstream;
	}
	if(u->resolved && u->resolved->ctx) {
		ngx_resolve_name_done(u->resolved->ctx);
		u->resolved->ctx = NULL;
	}
	pc = u->peer.connection;
	if(u->state) {
		u->state->response_time = ngx_current_msec - u->state->response_time;
		if(pc) {
			u->state->bytes_received = u->received;
			u->state->bytes_sent = pc->sent;
		}
	}
	if(u->peer.free && u->peer.sockaddr) {
		u->peer.free(&u->peer, u->peer.data, 0);
		u->peer.sockaddr = NULL;
	}
	if(pc) {
		ngx_log_debug1(NGX_LOG_DEBUG_STREAM, s->connection->log, 0, "close stream proxy upstream connection: %d", pc->fd);
#if (NGX_STREAM_SSL)
		if(pc->ssl) {
			pc->ssl->no_wait_shutdown = 1;
			(void)ngx_ssl_shutdown(pc);
		}
#endif
		ngx_close_connection(pc);
		u->peer.connection = NULL;
	}
noupstream:
	ngx_stream_finalize_session(s, rc);
}

static uchar * ngx_stream_proxy_log_error(ngx_log_t * log, uchar * buf, size_t len)
{
	ngx_connection_t  * pc;
	ngx_stream_session_t * s = (ngx_stream_session_t *)log->data;
	ngx_stream_upstream_t  * u = s->upstream;
	uchar * p = buf;
	if(u->peer.name) {
		p = ngx_snprintf(p, len, ", upstream: \"%V\"", u->peer.name);
		len -= p - buf;
	}
	pc = u->peer.connection;
	p = ngx_snprintf(p, len, ", bytes from/to client:%O/%O, bytes from/to upstream:%O/%O", s->received, s->connection->sent, u->received, pc ? pc->sent : 0);
	return p;
}

static void * ngx_stream_proxy_create_srv_conf(ngx_conf_t * cf)
{
	ngx_stream_proxy_srv_conf_t * conf = (ngx_stream_proxy_srv_conf_t *)ngx_pcalloc(cf->pool, sizeof(ngx_stream_proxy_srv_conf_t));
	if(conf) {
		/*
		 * set by ngx_pcalloc():
		 *
		 *   conf->ssl_protocols = 0;
		 *   conf->ssl_ciphers = { 0, NULL };
		 *   conf->ssl_name = NULL;
		 *   conf->ssl_trusted_certificate = { 0, NULL };
		 *   conf->ssl_crl = { 0, NULL };
		 *   conf->ssl_certificate = { 0, NULL };
		 *   conf->ssl_certificate_key = { 0, NULL };
		 *
		 *   conf->ssl = NULL;
		 *   conf->upstream = NULL;
		 *   conf->upstream_value = NULL;
		 */
		conf->connect_timeout = NGX_CONF_UNSET_MSEC;
		conf->timeout = NGX_CONF_UNSET_MSEC;
		conf->next_upstream_timeout = NGX_CONF_UNSET_MSEC;
		conf->buffer_size = NGX_CONF_UNSET_SIZE;
		conf->upload_rate = NGX_CONF_UNSET_SIZE;
		conf->download_rate = NGX_CONF_UNSET_SIZE;
		conf->responses = NGX_CONF_UNSET_UINT;
		conf->next_upstream_tries = NGX_CONF_UNSET_UINT;
		conf->next_upstream = NGX_CONF_UNSET;
		conf->proxy_protocol = NGX_CONF_UNSET;
		conf->local = (ngx_stream_upstream_local_t *)NGX_CONF_UNSET_PTR;
#if (NGX_STREAM_SSL)
		conf->ssl_enable = NGX_CONF_UNSET;
		conf->ssl_session_reuse = NGX_CONF_UNSET;
		conf->ssl_server_name = NGX_CONF_UNSET;
		conf->ssl_verify = NGX_CONF_UNSET;
		conf->ssl_verify_depth = NGX_CONF_UNSET_UINT;
		conf->ssl_passwords = (ngx_array_t *)NGX_CONF_UNSET_PTR;
#endif
	}
	return conf;
}

static char * ngx_stream_proxy_merge_srv_conf(ngx_conf_t * cf, void * parent, void * child)
{
	ngx_stream_proxy_srv_conf_t * prev = (ngx_stream_proxy_srv_conf_t *)parent;
	ngx_stream_proxy_srv_conf_t * conf = (ngx_stream_proxy_srv_conf_t *)child;
	ngx_conf_merge_msec_value(conf->connect_timeout, prev->connect_timeout, 60000);
	ngx_conf_merge_msec_value(conf->timeout, prev->timeout, 10 * 60000);
	ngx_conf_merge_msec_value(conf->next_upstream_timeout, prev->next_upstream_timeout, 0);
	ngx_conf_merge_size_value(conf->buffer_size, prev->buffer_size, 16384);
	ngx_conf_merge_size_value(conf->upload_rate, prev->upload_rate, 0);
	ngx_conf_merge_size_value(conf->download_rate, prev->download_rate, 0);
	ngx_conf_merge_uint_value(conf->responses, prev->responses, NGX_MAX_INT32_VALUE);
	ngx_conf_merge_uint_value(conf->next_upstream_tries, prev->next_upstream_tries, 0);
	ngx_conf_merge_value(conf->next_upstream, prev->next_upstream, 1);
	ngx_conf_merge_value(conf->proxy_protocol, prev->proxy_protocol, 0);
	ngx_conf_merge_ptr_value(conf->local, prev->local, NULL);
#if (NGX_STREAM_SSL)
	ngx_conf_merge_value(conf->ssl_enable, prev->ssl_enable, 0);
	ngx_conf_merge_value(conf->ssl_session_reuse, prev->ssl_session_reuse, 1);
	ngx_conf_merge_bitmask_value(conf->ssl_protocols, prev->ssl_protocols, (NGX_CONF_BITMASK_SET|NGX_SSL_TLSv1|NGX_SSL_TLSv1_1|NGX_SSL_TLSv1_2));
	ngx_conf_merge_str_value(conf->ssl_ciphers, prev->ssl_ciphers, "DEFAULT");
	SETIFZ(conf->ssl_name, prev->ssl_name);
	ngx_conf_merge_value(conf->ssl_server_name, prev->ssl_server_name, 0);
	ngx_conf_merge_value(conf->ssl_verify, prev->ssl_verify, 0);
	ngx_conf_merge_uint_value(conf->ssl_verify_depth, prev->ssl_verify_depth, 1);
	ngx_conf_merge_str_value(conf->ssl_trusted_certificate, prev->ssl_trusted_certificate, "");
	ngx_conf_merge_str_value(conf->ssl_crl, prev->ssl_crl, "");
	ngx_conf_merge_str_value(conf->ssl_certificate, prev->ssl_certificate, "");
	ngx_conf_merge_str_value(conf->ssl_certificate_key, prev->ssl_certificate_key, "");
	ngx_conf_merge_ptr_value(conf->ssl_passwords, prev->ssl_passwords, NULL);
	if(conf->ssl_enable && ngx_stream_proxy_set_ssl(cf, conf) != NGX_OK) {
		return NGX_CONF_ERROR;
	}
#endif
	return NGX_CONF_OK;
}

#if (NGX_STREAM_SSL)

static ngx_int_t ngx_stream_proxy_set_ssl(ngx_conf_t * cf, ngx_stream_proxy_srv_conf_t * pscf)
{
	ngx_pool_cleanup_t * cln;
	pscf->ssl = (ngx_ssl_t *)ngx_pcalloc(cf->pool, sizeof(ngx_ssl_t));
	if(pscf->ssl == NULL) {
		return NGX_ERROR;
	}
	pscf->ssl->log = cf->log;
	if(ngx_ssl_create(pscf->ssl, pscf->ssl_protocols, NULL) != NGX_OK) {
		return NGX_ERROR;
	}
	cln = ngx_pool_cleanup_add(cf->pool, 0);
	if(cln == NULL) {
		return NGX_ERROR;
	}
	cln->handler = ngx_ssl_cleanup_ctx;
	cln->data = pscf->ssl;
	if(pscf->ssl_certificate.len) {
		if(pscf->ssl_certificate_key.len == 0) {
			ngx_log_error(NGX_LOG_EMERG, cf->log, 0, "no \"proxy_ssl_certificate_key\" is defined for certificate \"%V\"", &pscf->ssl_certificate);
			return NGX_ERROR;
		}
		if(ngx_ssl_certificate(cf, pscf->ssl, &pscf->ssl_certificate, &pscf->ssl_certificate_key, pscf->ssl_passwords) != NGX_OK) {
			return NGX_ERROR;
		}
	}
	if(ngx_ssl_ciphers(cf, pscf->ssl, &pscf->ssl_ciphers, 0) != NGX_OK) {
		return NGX_ERROR;
	}
	if(pscf->ssl_verify) {
		if(pscf->ssl_trusted_certificate.len == 0) {
			ngx_log_error(NGX_LOG_EMERG, cf->log, 0, "no proxy_ssl_trusted_certificate for proxy_ssl_verify");
			return NGX_ERROR;
		}
		if(ngx_ssl_trusted_certificate(cf, pscf->ssl, &pscf->ssl_trusted_certificate, pscf->ssl_verify_depth) != NGX_OK) {
			return NGX_ERROR;
		}
		if(ngx_ssl_crl(cf, pscf->ssl, &pscf->ssl_crl) != NGX_OK) {
			return NGX_ERROR;
		}
	}
	return NGX_OK;
}

#endif

static const char * ngx_stream_proxy_pass(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	ngx_stream_proxy_srv_conf_t * pscf = (ngx_stream_proxy_srv_conf_t *)conf;
	ngx_url_t u;
	ngx_str_t  * value, * url;
	ngx_stream_complex_value_t cv;
	ngx_stream_compile_complex_value_t ccv;
	if(pscf->upstream || pscf->upstream_value) {
		return "is duplicate";
	}
	else {
		ngx_stream_core_srv_conf_t * cscf = (ngx_stream_core_srv_conf_t *)ngx_stream_conf_get_module_srv_conf(cf, ngx_stream_core_module);
		cscf->handler = ngx_stream_proxy_handler;
		value = static_cast<ngx_str_t *>(cf->args->elts);
		url = &value[1];
		memzero(&ccv, sizeof(ngx_stream_compile_complex_value_t));
		ccv.cf = cf;
		ccv.value = url;
		ccv.complex_value = &cv;
		if(ngx_stream_compile_complex_value(&ccv) != NGX_OK) {
			return NGX_CONF_ERROR;
		}
		if(cv.lengths) {
			pscf->upstream_value = (ngx_stream_complex_value_t *)ngx_palloc(cf->pool, sizeof(ngx_stream_complex_value_t));
			if(pscf->upstream_value == NULL) {
				return NGX_CONF_ERROR;
			}
			*pscf->upstream_value = cv;
			return NGX_CONF_OK;
		}
		memzero(&u, sizeof(ngx_url_t));
		u.url = *url;
		u.no_resolve = 1;
		pscf->upstream = ngx_stream_upstream_add(cf, &u, 0);
		if(pscf->upstream == NULL) {
			return NGX_CONF_ERROR;
		}
		return NGX_CONF_OK;
	}
}

static const char * ngx_stream_proxy_bind(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	ngx_stream_proxy_srv_conf_t * pscf = (ngx_stream_proxy_srv_conf_t *)conf;
	ngx_int_t rc;
	ngx_stream_complex_value_t cv;
	ngx_stream_upstream_local_t  * local;
	ngx_stream_compile_complex_value_t ccv;
	if(pscf->local != NGX_CONF_UNSET_PTR) {
		return "is duplicate";
	}
	else {
		ngx_str_t * value = static_cast<ngx_str_t *>(cf->args->elts);
		if(cf->args->nelts == 2 && sstreq(value[1].data, "off")) {
			pscf->local = NULL;
			return NGX_CONF_OK;
		}
		memzero(&ccv, sizeof(ngx_stream_compile_complex_value_t));
		ccv.cf = cf;
		ccv.value = &value[1];
		ccv.complex_value = &cv;
		if(ngx_stream_compile_complex_value(&ccv) != NGX_OK) {
			return NGX_CONF_ERROR;
		}
		local = (ngx_stream_upstream_local_t *)ngx_pcalloc(cf->pool, sizeof(ngx_stream_upstream_local_t));
		if(local == NULL) {
			return NGX_CONF_ERROR;
		}
		pscf->local = local;
		if(cv.lengths) {
			local->value = (ngx_stream_complex_value_t *)ngx_palloc(cf->pool, sizeof(ngx_stream_complex_value_t));
			if(local->value == NULL) {
				return NGX_CONF_ERROR;
			}
			*local->value = cv;
		}
		else {
			local->addr = (ngx_addr_t *)ngx_palloc(cf->pool, sizeof(ngx_addr_t));
			if(local->addr == NULL) {
				return NGX_CONF_ERROR;
			}
			rc = ngx_parse_addr_port(cf->pool, local->addr, value[1].data, value[1].len);
			switch(rc) {
				case NGX_OK:
					local->addr->name = value[1];
					break;
				case NGX_DECLINED:
					ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid address \"%V\"", &value[1]);
				// @fallthrough
				default:
					return NGX_CONF_ERROR;
			}
		}
		if(cf->args->nelts > 2) {
			if(sstreq(value[2].data, "transparent")) {
#if (NGX_HAVE_TRANSPARENT_PROXY)
				local->transparent = 1;
#else
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "transparent proxying is not supported on this platform, ignored");
#endif
			}
			else {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid parameter \"%V\"", &value[2]);
				return NGX_CONF_ERROR;
			}
		}
		return NGX_CONF_OK;
	}
}
