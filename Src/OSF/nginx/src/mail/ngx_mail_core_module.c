/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop
//#include <ngx_event.h>
//#include <ngx_mail.h>

static void * ngx_mail_core_create_main_conf(ngx_conf_t * cf);
static void * ngx_mail_core_create_srv_conf(ngx_conf_t * cf);
static char * ngx_mail_core_merge_srv_conf(ngx_conf_t * cf, void * parent, void * child);
static const char * ngx_mail_core_server(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static const char * ngx_mail_core_listen(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static const char * ngx_mail_core_protocol(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static const char * ngx_mail_core_error_log(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static const char * ngx_mail_core_resolver(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler

static ngx_command_t ngx_mail_core_commands[] = {
	{ ngx_string("server"), NGX_MAIL_MAIN_CONF|NGX_CONF_BLOCK|NGX_CONF_NOARGS,
	  ngx_mail_core_server, 0, 0, NULL },
	{ ngx_string("listen"), NGX_MAIL_SRV_CONF|NGX_CONF_1MORE,
	  ngx_mail_core_listen, NGX_MAIL_SRV_CONF_OFFSET, 0, NULL },
	{ ngx_string("protocol"), NGX_MAIL_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_mail_core_protocol, NGX_MAIL_SRV_CONF_OFFSET, 0, NULL },
	{ ngx_string("timeout"), NGX_MAIL_MAIN_CONF|NGX_MAIL_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_msec_slot, NGX_MAIL_SRV_CONF_OFFSET, offsetof(ngx_mail_core_srv_conf_t, timeout), NULL },
	{ ngx_string("server_name"), NGX_MAIL_MAIN_CONF|NGX_MAIL_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_str_slot, NGX_MAIL_SRV_CONF_OFFSET, offsetof(ngx_mail_core_srv_conf_t, server_name), NULL },
	{ ngx_string("error_log"), NGX_MAIL_MAIN_CONF|NGX_MAIL_SRV_CONF|NGX_CONF_1MORE,
	  ngx_mail_core_error_log, NGX_MAIL_SRV_CONF_OFFSET, 0, NULL },
	{ ngx_string("resolver"), NGX_MAIL_MAIN_CONF|NGX_MAIL_SRV_CONF|NGX_CONF_1MORE,
	  ngx_mail_core_resolver, NGX_MAIL_SRV_CONF_OFFSET, 0, NULL },
	{ ngx_string("resolver_timeout"), NGX_MAIL_MAIN_CONF|NGX_MAIL_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_msec_slot, NGX_MAIL_SRV_CONF_OFFSET, offsetof(ngx_mail_core_srv_conf_t, resolver_timeout), NULL },
	ngx_null_command
};

static ngx_mail_module_t ngx_mail_core_module_ctx = {
	NULL,                              /* protocol */
	ngx_mail_core_create_main_conf,    /* create main configuration */
	NULL,                              /* init main configuration */
	ngx_mail_core_create_srv_conf,     /* create server configuration */
	ngx_mail_core_merge_srv_conf       /* merge server configuration */
};

ngx_module_t ngx_mail_core_module = {
	NGX_MODULE_V1,
	&ngx_mail_core_module_ctx,         /* module context */
	ngx_mail_core_commands,            /* module directives */
	NGX_MAIL_MODULE,                   /* module type */
	NULL,                              /* init master */
	NULL,                              /* init module */
	NULL,                              /* init process */
	NULL,                              /* init thread */
	NULL,                              /* exit thread */
	NULL,                              /* exit process */
	NULL,                              /* exit master */
	NGX_MODULE_V1_PADDING
};

static void * ngx_mail_core_create_main_conf(ngx_conf_t * cf)
{
	ngx_mail_core_main_conf_t  * cmcf = (ngx_mail_core_main_conf_t *)ngx_pcalloc(cf->pool, sizeof(ngx_mail_core_main_conf_t));
	if(cmcf) {
		if(ngx_array_init(&cmcf->servers, cf->pool, 4, sizeof(ngx_mail_core_srv_conf_t *)) != NGX_OK) {
			return NULL;
		}
		if(ngx_array_init(&cmcf->listen, cf->pool, 4, sizeof(ngx_mail_listen_t)) != NGX_OK) {
			return NULL;
		}
	}
	return cmcf;
}

static void * ngx_mail_core_create_srv_conf(ngx_conf_t * cf)
{
	ngx_mail_core_srv_conf_t * cscf = (ngx_mail_core_srv_conf_t*)ngx_pcalloc(cf->pool, sizeof(ngx_mail_core_srv_conf_t));
	if(cscf) {
		/*
		* set by ngx_pcalloc():
		*
		*     cscf->protocol = NULL;
		*     cscf->error_log = NULL;
		*/
		cscf->timeout = NGX_CONF_UNSET_MSEC;
		cscf->resolver_timeout = NGX_CONF_UNSET_MSEC;
		cscf->resolver = (ngx_resolver_t *)NGX_CONF_UNSET_PTR;
		cscf->file_name = cf->conf_file->file.name.data;
		cscf->line = cf->conf_file->line;
	}
	return cscf;
}

static char * ngx_mail_core_merge_srv_conf(ngx_conf_t * cf, void * parent, void * child)
{
	ngx_mail_core_srv_conf_t * prev = (ngx_mail_core_srv_conf_t*)parent;
	ngx_mail_core_srv_conf_t * conf = (ngx_mail_core_srv_conf_t*)child;
	ngx_conf_merge_msec_value(conf->timeout, prev->timeout, 60000);
	ngx_conf_merge_msec_value(conf->resolver_timeout, prev->resolver_timeout, 30000);
	ngx_conf_merge_str_value(conf->server_name, prev->server_name, "");
	if(conf->server_name.len == 0) {
		conf->server_name = cf->cycle->hostname;
	}
	if(conf->protocol == NULL) {
		ngx_log_error(NGX_LOG_EMERG, cf->log, 0, "unknown mail protocol for server in %s:%ui", conf->file_name, conf->line);
		return NGX_CONF_ERROR;
	}
	if(conf->error_log == NULL) {
		if(prev->error_log) {
			conf->error_log = prev->error_log;
		}
		else {
			conf->error_log = &cf->cycle->new_log;
		}
	}
	ngx_conf_merge_ptr_value(conf->resolver, prev->resolver, NULL);
	return NGX_CONF_OK;
}

static const char * ngx_mail_core_server(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	char   * rv;
	void   * mconf;
	ngx_uint_t m;
	ngx_conf_t pcf;
	ngx_mail_module_t   * module;
	ngx_mail_conf_ctx_t * ctx, * mail_ctx;
	ngx_mail_core_srv_conf_t * cscf, ** cscfp;
	ngx_mail_core_main_conf_t  * cmcf;
	ctx = (ngx_mail_conf_ctx_t *)ngx_pcalloc(cf->pool, sizeof(ngx_mail_conf_ctx_t));
	if(!ctx) {
		return NGX_CONF_ERROR;
	}
	mail_ctx = (ngx_mail_conf_ctx_t *)cf->ctx;
	ctx->main_conf = mail_ctx->main_conf;
	/* the server{}'s srv_conf */
	ctx->srv_conf = (void **)ngx_pcalloc(cf->pool, sizeof(void *) * ngx_mail_max_module);
	if(ctx->srv_conf == NULL) {
		return NGX_CONF_ERROR;
	}
	for(m = 0; cf->cycle->modules[m]; m++) {
		if(cf->cycle->modules[m]->type != NGX_MAIL_MODULE) {
			continue;
		}
		module = (ngx_mail_module_t *)cf->cycle->modules[m]->ctx;
		if(module->create_srv_conf) {
			mconf = module->create_srv_conf(cf);
			if(mconf == NULL) {
				return NGX_CONF_ERROR;
			}

			ctx->srv_conf[cf->cycle->modules[m]->ctx_index] = mconf;
		}
	}

	/* the server configuration context */

	cscf = (ngx_mail_core_srv_conf_t*)ctx->srv_conf[ngx_mail_core_module.ctx_index];
	cscf->ctx = ctx;
	cmcf = (ngx_mail_core_main_conf_t *)ctx->main_conf[ngx_mail_core_module.ctx_index];
	cscfp = (ngx_mail_core_srv_conf_t**)ngx_array_push(&cmcf->servers);
	if(cscfp == NULL) {
		return NGX_CONF_ERROR;
	}
	*cscfp = cscf;
	/* parse inside server{} */
	pcf = *cf;
	cf->ctx = ctx;
	cf->cmd_type = NGX_MAIL_SRV_CONF;
	rv = ngx_conf_parse(cf, NULL);
	*cf = pcf;
	if(rv == NGX_CONF_OK && !cscf->listen) {
		ngx_log_error(NGX_LOG_EMERG, cf->log, 0, "no \"listen\" is defined for server in %s:%ui", cscf->file_name, cscf->line);
		return NGX_CONF_ERROR;
	}
	return rv;
}

static const char * ngx_mail_core_listen(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	ngx_mail_core_srv_conf_t  * cscf = (ngx_mail_core_srv_conf_t*)conf;
	ngx_str_t  * value, size;
	ngx_url_t u;
	ngx_uint_t i, m;
	ngx_mail_listen_t   * ls;
	ngx_mail_module_t   * module;
	ngx_mail_core_main_conf_t  * cmcf;
	cscf->listen = 1;
	value = static_cast<ngx_str_t *>(cf->args->elts);
	memzero(&u, sizeof(ngx_url_t));
	u.url = value[1];
	u.listen = 1;
	if(ngx_parse_url(cf->pool, &u) != NGX_OK) {
		if(u.err) {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s in \"%V\" of the \"listen\" directive", u.err, &u.url);
		}
		return NGX_CONF_ERROR;
	}
	cmcf = (ngx_mail_core_main_conf_t *)ngx_mail_conf_get_module_main_conf(cf, ngx_mail_core_module);
	ls = (ngx_mail_listen_t *)cmcf->listen.elts;
	for(i = 0; i < cmcf->listen.nelts; i++) {
		if(ngx_cmp_sockaddr(&ls[i].sockaddr.sockaddr, ls[i].socklen, (struct sockaddr *)&u.sockaddr, u.socklen, 1) != NGX_OK) {
			continue;
		}
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "duplicate \"%V\" address and port pair", &u.url);
		return NGX_CONF_ERROR;
	}
	ls = (ngx_mail_listen_t *)ngx_array_push(&cmcf->listen);
	if(ls == NULL) {
		return NGX_CONF_ERROR;
	}
	memzero(ls, sizeof(ngx_mail_listen_t));
	memcpy(&ls->sockaddr.sockaddr, &u.sockaddr, u.socklen);
	ls->socklen = u.socklen;
	ls->backlog = NGX_LISTEN_BACKLOG;
	ls->rcvbuf = -1;
	ls->sndbuf = -1;
	ls->wildcard = u.wildcard;
	ls->ctx = (ngx_mail_conf_ctx_t *)cf->ctx;

#if (NGX_HAVE_INET6)
	ls->ipv6only = 1;
#endif

	if(cscf->protocol == NULL) {
		for(m = 0; cf->cycle->modules[m]; m++) {
			if(cf->cycle->modules[m]->type != NGX_MAIL_MODULE) {
				continue;
			}
			module = (ngx_mail_module_t *)cf->cycle->modules[m]->ctx;
			if(module->protocol == NULL) {
				continue;
			}
			for(i = 0; module->protocol->port[i]; i++) {
				if(module->protocol->port[i] == u.port) {
					cscf->protocol = module->protocol;
					break;
				}
			}
		}
	}
	for(i = 2; i < cf->args->nelts; i++) {
		if(sstreq(value[i].data, "bind")) {
			ls->bind = 1;
			continue;
		}
		if(ngx_strncmp(value[i].data, "backlog=", 8) == 0) {
			ls->backlog = ngx_atoi(value[i].data + 8, value[i].len - 8);
			ls->bind = 1;
			if(ls->backlog == NGX_ERROR || ls->backlog == 0) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid backlog \"%V\"", &value[i]);
				return NGX_CONF_ERROR;
			}
			continue;
		}
		if(ngx_strncmp(value[i].data, "rcvbuf=", 7) == 0) {
			size.len = value[i].len - 7;
			size.data = value[i].data + 7;

			ls->rcvbuf = ngx_parse_size(&size);
			ls->bind = 1;

			if(ls->rcvbuf == NGX_ERROR) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
				    "invalid rcvbuf \"%V\"", &value[i]);
				return NGX_CONF_ERROR;
			}

			continue;
		}

		if(ngx_strncmp(value[i].data, "sndbuf=", 7) == 0) {
			size.len = value[i].len - 7;
			size.data = value[i].data + 7;

			ls->sndbuf = ngx_parse_size(&size);
			ls->bind = 1;

			if(ls->sndbuf == NGX_ERROR) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
				    "invalid sndbuf \"%V\"", &value[i]);
				return NGX_CONF_ERROR;
			}

			continue;
		}

		if(ngx_strncmp(value[i].data, "ipv6only=o", 10) == 0) {
#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
			size_t len;
			uchar buf[NGX_SOCKADDR_STRLEN];
			if(ls->sockaddr.sockaddr.sa_family == AF_INET6) {
				if(sstreq(&value[i].data[10], "n")) {
					ls->ipv6only = 1;
				}
				else if(sstreq(&value[i].data[10], "ff")) {
					ls->ipv6only = 0;
				}
				else {
					ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid ipv6only flags \"%s\"", &value[i].data[9]);
					return NGX_CONF_ERROR;
				}
				ls->bind = 1;
			}
			else {
				len = ngx_sock_ntop(&ls->sockaddr.sockaddr, ls->socklen, buf, NGX_SOCKADDR_STRLEN, 1);
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "ipv6only is not supported on addr \"%*s\", ignored", len, buf);
			}
			continue;
#else
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
			    "bind ipv6only is not supported "
			    "on this platform");
			return NGX_CONF_ERROR;
#endif
		}
		if(sstreq(value[i].data, "ssl")) {
#if (NGX_MAIL_SSL)
			ls->ssl = 1;
			continue;
#else
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
			    "the \"ssl\" parameter requires "
			    "ngx_mail_ssl_module");
			return NGX_CONF_ERROR;
#endif
		}
		if(ngx_strncmp(value[i].data, "so_keepalive=", 13) == 0) {
			if(sstreq(&value[i].data[13], "on")) {
				ls->so_keepalive = 1;
			}
			else if(sstreq(&value[i].data[13], "off")) {
				ls->so_keepalive = 2;
			}
			else {
#if (NGX_HAVE_KEEPALIVE_TUNABLE)
				uchar   * p, * end;
				ngx_str_t s;

				end = value[i].data + value[i].len;
				s.data = value[i].data + 13;

				p = ngx_strlchr(s.data, end, ':');
				if(!p) {
					p = end;
				}

				if(p > s.data) {
					s.len = p - s.data;

					ls->tcp_keepidle = ngx_parse_time(&s, 1);
					if(ls->tcp_keepidle == (time_t)NGX_ERROR) {
						goto invalid_so_keepalive;
					}
				}

				s.data = (p < end) ? (p + 1) : end;

				p = ngx_strlchr(s.data, end, ':');
				if(!p) {
					p = end;
				}

				if(p > s.data) {
					s.len = p - s.data;

					ls->tcp_keepintvl = ngx_parse_time(&s, 1);
					if(ls->tcp_keepintvl == (time_t)NGX_ERROR) {
						goto invalid_so_keepalive;
					}
				}

				s.data = (p < end) ? (p + 1) : end;

				if(s.data < end) {
					s.len = end - s.data;

					ls->tcp_keepcnt = ngx_atoi(s.data, s.len);
					if(ls->tcp_keepcnt == NGX_ERROR) {
						goto invalid_so_keepalive;
					}
				}

				if(ls->tcp_keepidle == 0 && ls->tcp_keepintvl == 0
				    && ls->tcp_keepcnt == 0) {
					goto invalid_so_keepalive;
				}

				ls->so_keepalive = 1;

#else

				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
				    "the \"so_keepalive\" parameter accepts "
				    "only \"on\" or \"off\" on this platform");
				return NGX_CONF_ERROR;

#endif
			}

			ls->bind = 1;

			continue;

#if (NGX_HAVE_KEEPALIVE_TUNABLE)
invalid_so_keepalive:
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid so_keepalive value: \"%s\"", &value[i].data[13]);
			return NGX_CONF_ERROR;
#endif
		}
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "the invalid \"%V\" parameter", &value[i]);
		return NGX_CONF_ERROR;
	}
	return NGX_CONF_OK;
}

static const char * ngx_mail_core_protocol(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	ngx_mail_core_srv_conf_t  * cscf = (ngx_mail_core_srv_conf_t*)conf;
	ngx_str_t   * value;
	ngx_uint_t m;
	ngx_mail_module_t  * module;
	value = static_cast<ngx_str_t *>(cf->args->elts);
	for(m = 0; cf->cycle->modules[m]; m++) {
		if(cf->cycle->modules[m]->type != NGX_MAIL_MODULE) {
			continue;
		}
		module = (ngx_mail_module_t *)cf->cycle->modules[m]->ctx;
		if(module->protocol && sstreq(module->protocol->name.data, value[1].data)) {
			cscf->protocol = module->protocol;
			return NGX_CONF_OK;
		}
	}
	ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "unknown protocol \"%V\"", &value[1]);
	return NGX_CONF_ERROR;
}

static const char * ngx_mail_core_error_log(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	ngx_mail_core_srv_conf_t  * cscf = (ngx_mail_core_srv_conf_t*)conf;
	return ngx_log_set_log(cf, &cscf->error_log);
}

static const char * ngx_mail_core_resolver(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	ngx_mail_core_srv_conf_t  * cscf = (ngx_mail_core_srv_conf_t*)conf;
	ngx_str_t  * value = static_cast<ngx_str_t *>(cf->args->elts);
	if(cscf->resolver != NGX_CONF_UNSET_PTR) {
		return "is duplicate";
	}
	if(sstreq(value[1].data, "off")) {
		cscf->resolver = NULL;
		return NGX_CONF_OK;
	}
	cscf->resolver = ngx_resolver_create(cf, &value[1], cf->args->nelts - 1);
	if(cscf->resolver == NULL) {
		return NGX_CONF_ERROR;
	}
	return NGX_CONF_OK;
}

const char * ngx_mail_capabilities(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	char * p = static_cast<char *>(conf);
	ngx_str_t  * c;
	ngx_uint_t i;
	ngx_array_t * a = (ngx_array_t*)(p + cmd->offset);
	ngx_str_t  * value = static_cast<ngx_str_t *>(cf->args->elts);
	for(i = 1; i < cf->args->nelts; i++) {
		c = (ngx_str_t *)ngx_array_push(a);
		if(!c) {
			return NGX_CONF_ERROR;
		}
		*c = value[i];
	}
	return NGX_CONF_OK;
}

