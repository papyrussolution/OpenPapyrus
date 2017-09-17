/*
 * Copyright (C) Roman Arutyunyan
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop
//#include <ngx_http.h>

struct ngx_http_mirror_loc_conf_t {
	ngx_array_t * mirror;
	ngx_flag_t request_body;
};

struct ngx_http_mirror_ctx_t {
	ngx_int_t status;
};

static ngx_int_t ngx_http_mirror_handler(ngx_http_request_t * r);
static void ngx_http_mirror_body_handler(ngx_http_request_t * r);
static ngx_int_t ngx_http_mirror_handler_internal(ngx_http_request_t * r);
static void * ngx_http_mirror_create_loc_conf(ngx_conf_t * cf);
static char * ngx_http_mirror_merge_loc_conf(ngx_conf_t * cf, void * parent, void * child);
static const char * ngx_http_mirror(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static ngx_int_t ngx_http_mirror_init(ngx_conf_t * cf);

static ngx_command_t ngx_http_mirror_commands[] = {
	{ ngx_string("mirror"), NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
	  ngx_http_mirror, NGX_HTTP_LOC_CONF_OFFSET, 0, NULL },
	{ ngx_string("mirror_request_body"), NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
	  ngx_conf_set_flag_slot, NGX_HTTP_LOC_CONF_OFFSET, offsetof(ngx_http_mirror_loc_conf_t, request_body), NULL },
	ngx_null_command
};

static ngx_http_module_t ngx_http_mirror_module_ctx = {
	NULL,                              /* preconfiguration */
	ngx_http_mirror_init,              /* postconfiguration */
	NULL,                              /* create main configuration */
	NULL,                              /* init main configuration */
	NULL,                              /* create server configuration */
	NULL,                              /* merge server configuration */
	ngx_http_mirror_create_loc_conf,   /* create location configuration */
	ngx_http_mirror_merge_loc_conf     /* merge location configuration */
};

ngx_module_t ngx_http_mirror_module = {
	NGX_MODULE_V1,
	&ngx_http_mirror_module_ctx,       /* module context */
	ngx_http_mirror_commands,          /* module directives */
	NGX_HTTP_MODULE,                   /* module type */
	NULL,                              /* init master */
	NULL,                              /* init module */
	NULL,                              /* init process */
	NULL,                              /* init thread */
	NULL,                              /* exit thread */
	NULL,                              /* exit process */
	NULL,                              /* exit master */
	NGX_MODULE_V1_PADDING
};

static ngx_int_t ngx_http_mirror_handler(ngx_http_request_t * r)
{
	ngx_int_t rc;
	ngx_http_mirror_ctx_t  * ctx;
	ngx_http_mirror_loc_conf_t  * mlcf;
	if(r != r->main) {
		return NGX_DECLINED;
	}
	mlcf = (ngx_http_mirror_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_mirror_module);
	if(mlcf->mirror == NULL) {
		return NGX_DECLINED;
	}
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "mirror handler");
	if(mlcf->request_body) {
		ctx = (ngx_http_mirror_ctx_t *)ngx_http_get_module_ctx(r, ngx_http_mirror_module);
		if(ctx) {
			return ctx->status;
		}
		ctx = (ngx_http_mirror_ctx_t *)ngx_pcalloc(r->pool, sizeof(ngx_http_mirror_ctx_t));
		if(ctx == NULL) {
			return NGX_ERROR;
		}
		ctx->status = NGX_DONE;
		ngx_http_set_ctx(r, ctx, ngx_http_mirror_module);
		rc = ngx_http_read_client_request_body(r, ngx_http_mirror_body_handler);
		if(rc >= NGX_HTTP_SPECIAL_RESPONSE) {
			return rc;
		}
		ngx_http_finalize_request(r, NGX_DONE);
		return NGX_DONE;
	}
	return ngx_http_mirror_handler_internal(r);
}

static void ngx_http_mirror_body_handler(ngx_http_request_t * r)
{
	ngx_http_mirror_ctx_t  * ctx = (ngx_http_mirror_ctx_t *)ngx_http_get_module_ctx(r, ngx_http_mirror_module);
	ctx->status = ngx_http_mirror_handler_internal(r);
	r->preserve_body = 1;
	r->write_event_handler = ngx_http_core_run_phases;
	ngx_http_core_run_phases(r);
}

static ngx_int_t ngx_http_mirror_handler_internal(ngx_http_request_t * r)
{
	ngx_uint_t i;
	ngx_http_request_t   * sr;
	ngx_http_mirror_loc_conf_t  * mlcf = (ngx_http_mirror_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_mirror_module);
	ngx_str_t * name = (ngx_str_t*)mlcf->mirror->elts;
	for(i = 0; i < mlcf->mirror->nelts; i++) {
		if(ngx_http_subrequest(r, &name[i], &r->args, &sr, NULL, NGX_HTTP_SUBREQUEST_BACKGROUND) != NGX_OK) {
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
		sr->header_only = 1;
		sr->method = r->method;
		sr->method_name = r->method_name;
	}
	return NGX_DECLINED;
}

static void * ngx_http_mirror_create_loc_conf(ngx_conf_t * cf)
{
	ngx_http_mirror_loc_conf_t  * mlcf = (ngx_http_mirror_loc_conf_t *)ngx_pcalloc(cf->pool, sizeof(ngx_http_mirror_loc_conf_t));
	if(mlcf) {
		mlcf->mirror = (ngx_array_t*)NGX_CONF_UNSET_PTR;
		mlcf->request_body = NGX_CONF_UNSET;
	}
	return mlcf;
}

static char * ngx_http_mirror_merge_loc_conf(ngx_conf_t * cf, void * parent, void * child)
{
	ngx_http_mirror_loc_conf_t * prev = (ngx_http_mirror_loc_conf_t *)parent;
	ngx_http_mirror_loc_conf_t * conf = (ngx_http_mirror_loc_conf_t *)child;
	ngx_conf_merge_ptr_value(conf->mirror, prev->mirror, NULL);
	ngx_conf_merge_value(conf->request_body, prev->request_body, 1);
	return NGX_CONF_OK;
}

static const char * ngx_http_mirror(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	ngx_http_mirror_loc_conf_t * mlcf = (ngx_http_mirror_loc_conf_t *)conf;
	ngx_str_t  * s;
	ngx_str_t  * value = (ngx_str_t*)cf->args->elts;
	if(ngx_strcmp(value[1].data, "off") == 0) {
		if(mlcf->mirror != NGX_CONF_UNSET_PTR) {
			return "is duplicate";
		}
		mlcf->mirror = NULL;
		return NGX_CONF_OK;
	}
	if(mlcf->mirror == NULL) {
		return "is duplicate";
	}
	if(mlcf->mirror == NGX_CONF_UNSET_PTR) {
		mlcf->mirror = ngx_array_create(cf->pool, 4, sizeof(ngx_str_t));
		if(mlcf->mirror == NULL) {
			return NGX_CONF_ERROR;
		}
	}
	s = (ngx_str_t*)ngx_array_push(mlcf->mirror);
	if(s == NULL) {
		return NGX_CONF_ERROR;
	}
	*s = value[1];
	return NGX_CONF_OK;
}

static ngx_int_t ngx_http_mirror_init(ngx_conf_t * cf)
{
	ngx_http_core_main_conf_t  * cmcf = (ngx_http_core_main_conf_t*)ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
	ngx_http_handler_pt * h = (ngx_http_handler_pt*)ngx_array_push(&cmcf->phases[NGX_HTTP_PRECONTENT_PHASE].handlers);
	if(h == NULL) {
		return NGX_ERROR;
	}
	*h = ngx_http_mirror_handler;
	return NGX_OK;
}

