/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

#define NGX_HTTP_AUTH_BUF_SIZE  2048

struct ngx_http_auth_basic_ctx_t {
	ngx_str_t passwd;
};

struct ngx_http_auth_basic_loc_conf_t {
	ngx_http_complex_value_t  * realm;
	ngx_http_complex_value_t user_file;
};

static ngx_int_t ngx_http_auth_basic_handler(ngx_http_request_t * r);
static ngx_int_t ngx_http_auth_basic_crypt_handler(ngx_http_request_t * r, ngx_http_auth_basic_ctx_t * ctx, ngx_str_t * passwd, ngx_str_t * realm);
static ngx_int_t ngx_http_auth_basic_set_realm(ngx_http_request_t * r, ngx_str_t * realm);
static void ngx_http_auth_basic_close(ngx_file_t * file);
static void * ngx_http_auth_basic_create_loc_conf(ngx_conf_t * cf);
static char * ngx_http_auth_basic_merge_loc_conf(ngx_conf_t * cf, void * parent, void * child);
static ngx_int_t ngx_http_auth_basic_init(ngx_conf_t * cf);
static const char * ngx_http_auth_basic_user_file(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler

static ngx_command_t ngx_http_auth_basic_commands[] = {
	{ ngx_string("auth_basic"), NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_TAKE1,
	  ngx_http_set_complex_value_slot, NGX_HTTP_LOC_CONF_OFFSET, offsetof(ngx_http_auth_basic_loc_conf_t, realm), NULL },
	{ ngx_string("auth_basic_user_file"), NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_TAKE1,
	  ngx_http_auth_basic_user_file, NGX_HTTP_LOC_CONF_OFFSET, offsetof(ngx_http_auth_basic_loc_conf_t, user_file), NULL },
	ngx_null_command
};

static ngx_http_module_t ngx_http_auth_basic_module_ctx = {
	NULL,                              /* preconfiguration */
	ngx_http_auth_basic_init,          /* postconfiguration */

	NULL,                              /* create main configuration */
	NULL,                              /* init main configuration */

	NULL,                              /* create server configuration */
	NULL,                              /* merge server configuration */

	ngx_http_auth_basic_create_loc_conf, /* create location configuration */
	ngx_http_auth_basic_merge_loc_conf /* merge location configuration */
};

ngx_module_t ngx_http_auth_basic_module = {
	NGX_MODULE_V1,
	&ngx_http_auth_basic_module_ctx,   /* module context */
	ngx_http_auth_basic_commands,      /* module directives */
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

static ngx_int_t ngx_http_auth_basic_handler(ngx_http_request_t * r)
{
	nginx_off_t offset;
	ssize_t n;
	ngx_fd_t fd;
	ngx_int_t rc;
	ngx_err_t err;
	ngx_str_t pwd, realm, user_file;
	ngx_uint_t i, level, login, left, passwd;
	ngx_file_t file;
	ngx_http_auth_basic_ctx_t  * ctx;
	ngx_http_auth_basic_loc_conf_t  * alcf;
	uchar buf[NGX_HTTP_AUTH_BUF_SIZE];
	enum {
		sw_login,
		sw_passwd,
		sw_skip
	} state;
	alcf = (ngx_http_auth_basic_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_auth_basic_module);
	if(alcf->realm == NULL || alcf->user_file.value.data == NULL) {
		return NGX_DECLINED;
	}
	if(ngx_http_complex_value(r, alcf->realm, &realm) != NGX_OK) {
		return NGX_ERROR;
	}
	if(realm.len == 3 && ngx_strncmp(realm.data, "off", 3) == 0) {
		return NGX_DECLINED;
	}
	ctx = (ngx_http_auth_basic_ctx_t *)ngx_http_get_module_ctx(r, ngx_http_auth_basic_module);
	if(ctx) {
		return ngx_http_auth_basic_crypt_handler(r, ctx, &ctx->passwd, &realm);
	}
	rc = ngx_http_auth_basic_user(r);
	if(rc == NGX_DECLINED) {
		ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "no user/password was provided for basic authentication");
		return ngx_http_auth_basic_set_realm(r, &realm);
	}
	if(rc == NGX_ERROR) {
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	if(ngx_http_complex_value(r, &alcf->user_file, &user_file) != NGX_OK) {
		return NGX_ERROR;
	}
	fd = ngx_open_file(user_file.data, NGX_FILE_RDONLY, NGX_FILE_OPEN, 0);
	if(fd == NGX_INVALID_FILE) {
		err = ngx_errno;
		if(err == NGX_ENOENT) {
			level = NGX_LOG_ERR;
			rc = NGX_HTTP_FORBIDDEN;
		}
		else {
			level = NGX_LOG_CRIT;
			rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
		ngx_log_error(level, r->connection->log, err, ngx_open_file_n " \"%s\" failed", user_file.data);
		return rc;
	}
	memzero(&file, sizeof(ngx_file_t));
	file.fd = fd;
	file.name = user_file;
	file.log = r->connection->log;
	state = sw_login;
	passwd = 0;
	login = 0;
	left = 0;
	offset = 0;
	for(;;) {
		i = left;
		n = ngx_read_file(&file, buf + left, NGX_HTTP_AUTH_BUF_SIZE - left, offset);
		if(n == NGX_ERROR) {
			ngx_http_auth_basic_close(&file);
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
		if(n == 0) {
			break;
		}
		for(i = left; i < left + n; i++) {
			switch(state) {
				case sw_login:
				    if(login == 0) {
					    if(buf[i] == '#' || buf[i] == __CR) {
						    state = sw_skip;
						    break;
					    }
					    if(buf[i] == LF) {
						    break;
					    }
				    }
				    if(buf[i] != r->headers_in.user.data[login]) {
					    state = sw_skip;
					    break;
				    }
				    if(login == r->headers_in.user.len) {
					    state = sw_passwd;
					    passwd = i + 1;
				    }
				    login++;
				    break;
				case sw_passwd:
				    if(buf[i] == LF || buf[i] == __CR || buf[i] == ':') {
					    buf[i] = '\0';
					    ngx_http_auth_basic_close(&file);
					    pwd.len = i - passwd;
					    pwd.data = &buf[passwd];
					    return ngx_http_auth_basic_crypt_handler(r, NULL, &pwd, &realm);
				    }
				    break;
				case sw_skip:
				    if(buf[i] == LF) {
					    state = sw_login;
					    login = 0;
				    }
				    break;
			}
		}
		if(state == sw_passwd) {
			left = left + n - passwd;
			memmove(buf, &buf[passwd], left);
			passwd = 0;
		}
		else {
			left = 0;
		}
		offset += n;
	}
	ngx_http_auth_basic_close(&file);
	if(state == sw_passwd) {
		pwd.len = i - passwd;
		pwd.data = (uchar *)ngx_pnalloc(r->pool, pwd.len + 1);
		if(pwd.data == NULL) {
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
		ngx_cpystrn(pwd.data, &buf[passwd], pwd.len + 1);
		return ngx_http_auth_basic_crypt_handler(r, NULL, &pwd, &realm);
	}
	ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "user \"%V\" was not found in \"%V\"", &r->headers_in.user, &user_file);
	return ngx_http_auth_basic_set_realm(r, &realm);
}

static ngx_int_t ngx_http_auth_basic_crypt_handler(ngx_http_request_t * r, ngx_http_auth_basic_ctx_t * ctx, ngx_str_t * passwd, ngx_str_t * realm)
{
	uchar * encrypted;
	ngx_int_t rc = ngx_crypt(r->pool, r->headers_in.passwd.data, passwd->data, &encrypted);
	ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "rc: %i user: \"%V\" salt: \"%s\"", rc, &r->headers_in.user, passwd->data);
	if(rc == NGX_OK) {
		if(sstreq(encrypted, passwd->data)) {
			return NGX_OK;
		}
		ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "encrypted: \"%s\"", encrypted);
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "user \"%V\": password mismatch", &r->headers_in.user);
		return ngx_http_auth_basic_set_realm(r, realm);
	}
	if(rc == NGX_ERROR) {
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	/* rc == NGX_AGAIN */
	if(!ctx) {
		ctx = (ngx_http_auth_basic_ctx_t *)ngx_palloc(r->pool, sizeof(ngx_http_auth_basic_ctx_t));
		if(!ctx) {
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
		ngx_http_set_ctx(r, ctx, ngx_http_auth_basic_module);
		ctx->passwd.len = passwd->len;
		passwd->len++;
		ctx->passwd.data = ngx_pstrdup(r->pool, passwd);
		if(ctx->passwd.data == NULL) {
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
	}
	/* @todo add mutex event */
	return rc;
}

static ngx_int_t ngx_http_auth_basic_set_realm(ngx_http_request_t * r, ngx_str_t * realm)
{
	size_t len;
	uchar  * basic, * p;
	r->headers_out.www_authenticate = (ngx_table_elt_t *)ngx_list_push(&r->headers_out.headers);
	if(r->headers_out.www_authenticate == NULL) {
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	len = sizeof("Basic realm=\"\"") - 1 + realm->len;
	basic = static_cast<uchar *>(ngx_pnalloc(r->pool, len));
	if(basic == NULL) {
		r->headers_out.www_authenticate->hash = 0;
		r->headers_out.www_authenticate = NULL;
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	p = ngx_cpymem(basic, "Basic realm=\"", sizeof("Basic realm=\"") - 1);
	p = ngx_cpymem(p, realm->data, realm->len);
	*p = '"';
	r->headers_out.www_authenticate->hash = 1;
	ngx_str_set(&r->headers_out.www_authenticate->key, "WWW-Authenticate");
	r->headers_out.www_authenticate->value.data = basic;
	r->headers_out.www_authenticate->value.len = len;
	return NGX_HTTP_UNAUTHORIZED;
}

static void ngx_http_auth_basic_close(ngx_file_t * file)
{
	if(ngx_close_file(file->fd) == NGX_FILE_ERROR) {
		ngx_log_error(NGX_LOG_ALERT, file->log, ngx_errno, ngx_close_file_n " \"%s\" failed", file->name.data);
	}
}

static void * ngx_http_auth_basic_create_loc_conf(ngx_conf_t * cf)
{
	ngx_http_auth_basic_loc_conf_t * conf = (ngx_http_auth_basic_loc_conf_t *)ngx_pcalloc(cf->pool, sizeof(ngx_http_auth_basic_loc_conf_t));
	return conf;
}

static char * ngx_http_auth_basic_merge_loc_conf(ngx_conf_t * cf, void * parent, void * child)
{
	const ngx_http_auth_basic_loc_conf_t * prev = (const ngx_http_auth_basic_loc_conf_t *)parent;
	ngx_http_auth_basic_loc_conf_t * conf = (ngx_http_auth_basic_loc_conf_t *)child;
	SETIFZ(conf->realm, prev->realm);
	if(conf->user_file.value.data == NULL) {
		conf->user_file = prev->user_file;
	}
	return NGX_CONF_OK;
}

static ngx_int_t ngx_http_auth_basic_init(ngx_conf_t * cf)
{
	ngx_http_core_main_conf_t  * cmcf = (ngx_http_core_main_conf_t *)ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
	ngx_http_handler_pt * h = (ngx_http_handler_pt *)ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
	if(!h) {
		return NGX_ERROR;
	}
	*h = ngx_http_auth_basic_handler;
	return NGX_OK;
}

static const char * ngx_http_auth_basic_user_file(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	ngx_http_auth_basic_loc_conf_t * alcf = (ngx_http_auth_basic_loc_conf_t *)conf;
	if(alcf->user_file.value.data) {
		return "is duplicate";
	}
	else {
		ngx_str_t * value = static_cast<ngx_str_t *>(cf->args->elts);
		ngx_http_compile_complex_value_t ccv;
		memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));
		ccv.cf = cf;
		ccv.value = &value[1];
		ccv.complex_value = &alcf->user_file;
		ccv.zero = 1;
		ccv.conf_prefix = 1;
		return (ngx_http_compile_complex_value(&ccv) != NGX_OK) ? NGX_CONF_ERROR : NGX_CONF_OK;
	}
}
