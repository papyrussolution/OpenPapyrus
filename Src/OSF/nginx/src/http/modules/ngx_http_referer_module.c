
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

#define NGX_HTTP_REFERER_NO_URI_PART  (reinterpret_cast<void *>(4))

typedef struct {
	ngx_hash_combined_t hash;
#if (NGX_PCRE)
	ngx_array_t   * regex;
	ngx_array_t   * server_name_regex;
#endif
	ngx_flag_t no_referer;
	ngx_flag_t blocked_referer;
	ngx_flag_t server_names;
	ngx_hash_keys_arrays_t  * keys;
	ngx_uint_t referer_hash_max_size;
	ngx_uint_t referer_hash_bucket_size;
} ngx_http_referer_conf_t;

static void * ngx_http_referer_create_conf(ngx_conf_t * cf);
static char * ngx_http_referer_merge_conf(ngx_conf_t * cf, void * parent, void * child);
static const char * ngx_http_valid_referers(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static ngx_int_t ngx_http_add_referer(ngx_conf_t * cf, ngx_hash_keys_arrays_t * keys, ngx_str_t * value, ngx_str_t * uri);
static ngx_int_t ngx_http_add_regex_referer(ngx_conf_t * cf, ngx_http_referer_conf_t * rlcf, ngx_str_t * name);
#if (NGX_PCRE)
static ngx_int_t ngx_http_add_regex_server_name(ngx_conf_t * cf, ngx_http_referer_conf_t * rlcf, ngx_http_regex_t * regex);
#endif
static int ngx_libc_cdecl ngx_http_cmp_referer_wildcards(const void * one, const void * two);

static ngx_command_t ngx_http_referer_commands[] = {
	{ ngx_string("valid_referers"), NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
	  ngx_http_valid_referers, NGX_HTTP_LOC_CONF_OFFSET, 0, NULL },
	{ ngx_string("referer_hash_max_size"), NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_num_slot, NGX_HTTP_LOC_CONF_OFFSET, offsetof(ngx_http_referer_conf_t, referer_hash_max_size), NULL },
	{ ngx_string("referer_hash_bucket_size"), NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_num_slot, NGX_HTTP_LOC_CONF_OFFSET, offsetof(ngx_http_referer_conf_t, referer_hash_bucket_size), NULL },
	ngx_null_command
};

static ngx_http_module_t ngx_http_referer_module_ctx = {
	NULL,                              /* preconfiguration */
	NULL,                              /* postconfiguration */
	NULL,                              /* create main configuration */
	NULL,                              /* init main configuration */
	NULL,                              /* create server configuration */
	NULL,                              /* merge server configuration */
	ngx_http_referer_create_conf,      /* create location configuration */
	ngx_http_referer_merge_conf        /* merge location configuration */
};

ngx_module_t ngx_http_referer_module = {
	NGX_MODULE_V1,
	&ngx_http_referer_module_ctx,      /* module context */
	ngx_http_referer_commands,         /* module directives */
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

static ngx_int_t ngx_http_referer_variable(ngx_http_request_t * r, ngx_http_variable_value_t * v,
    uintptr_t data)
{
	uchar  * p, * ref, * last;
	size_t len;
	ngx_str_t * uri;
	ngx_uint_t i, key;
	ngx_http_referer_conf_t * rlcf;
	uchar buf[256];
#if (NGX_PCRE)
	ngx_int_t rc;
	ngx_str_t referer;
#endif
	rlcf = (ngx_http_referer_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_referer_module);
	if(rlcf->hash.hash.buckets == NULL && rlcf->hash.wc_head == NULL && rlcf->hash.wc_tail == NULL
#if (NGX_PCRE)
	    && rlcf->regex == NULL && rlcf->server_name_regex == NULL
#endif
	    ) {
		goto valid;
	}
	if(r->headers_in.referer == NULL) {
		if(rlcf->no_referer) {
			goto valid;
		}
		goto invalid;
	}
	len = r->headers_in.referer->value.len;
	ref = r->headers_in.referer->value.data;
	if(len >= sizeof("http://i.ru") - 1) {
		last = ref + len;
		if(ngx_strncasecmp(ref, (uchar *)"http://", 7) == 0) {
			ref += 7;
			len -= 7;
			goto valid_scheme;
		}
		else if(ngx_strncasecmp(ref, (uchar *)"https://", 8) == 0) {
			ref += 8;
			len -= 8;
			goto valid_scheme;
		}
	}
	if(rlcf->blocked_referer) {
		goto valid;
	}
	goto invalid;
valid_scheme:
	i = 0;
	key = 0;
	for(p = ref; p < last; p++) {
		if(*p == '/' || *p == ':') {
			break;
		}

		if(i == 256) {
			goto invalid;
		}

		buf[i] = stolower_ascii(*p);
		key = ngx_hash(key, buf[i++]);
	}
	uri = (ngx_str_t *)ngx_hash_find_combined(&rlcf->hash, key, buf, p - ref);
	if(uri) {
		goto uri;
	}
#if (NGX_PCRE)
	if(rlcf->server_name_regex) {
		referer.len = p - ref;
		referer.data = buf;
		rc = ngx_regex_exec_array(rlcf->server_name_regex, &referer, r->connection->log);
		if(rc == NGX_OK) {
			goto valid;
		}
		if(rc == NGX_ERROR) {
			return rc;
		}
		/* NGX_DECLINED */
	}

	if(rlcf->regex) {
		referer.len = len;
		referer.data = ref;
		rc = ngx_regex_exec_array(rlcf->regex, &referer, r->connection->log);
		if(rc == NGX_OK) {
			goto valid;
		}
		if(rc == NGX_ERROR) {
			return rc;
		}
		/* NGX_DECLINED */
	}
#endif
invalid:
	*v = ngx_http_variable_true_value;
	return NGX_OK;
uri:
	for(/* void */; p < last; p++) {
		if(*p == '/') {
			break;
		}
	}
	len = last - p;
	if(uri == NGX_HTTP_REFERER_NO_URI_PART) {
		goto valid;
	}
	if(len < uri->len || ngx_strncmp(uri->data, p, uri->len) != 0) {
		goto invalid;
	}
valid:
	*v = ngx_http_variable_null_value;
	return NGX_OK;
}

static void * ngx_http_referer_create_conf(ngx_conf_t * cf)
{
	ngx_http_referer_conf_t  * conf = (ngx_http_referer_conf_t *)ngx_pcalloc(cf->pool, sizeof(ngx_http_referer_conf_t));
	if(!conf) {
		return NULL;
	}
	/*
	 * set by ngx_pcalloc():
	 *
	 *   conf->hash = { NULL };
	 *   conf->server_names = 0;
	 *   conf->keys = NULL;
	 */

#if (NGX_PCRE)
	conf->regex = (ngx_array_t*)NGX_CONF_UNSET_PTR;
	conf->server_name_regex = (ngx_array_t*)NGX_CONF_UNSET_PTR;
#endif

	conf->no_referer = NGX_CONF_UNSET;
	conf->blocked_referer = NGX_CONF_UNSET;
	conf->referer_hash_max_size = NGX_CONF_UNSET_UINT;
	conf->referer_hash_bucket_size = NGX_CONF_UNSET_UINT;

	return conf;
}

static char * ngx_http_referer_merge_conf(ngx_conf_t * cf, void * parent, void * child)
{
	ngx_http_referer_conf_t * prev = (ngx_http_referer_conf_t *)parent;
	ngx_http_referer_conf_t * conf = (ngx_http_referer_conf_t *)child;
	ngx_uint_t n;
	ngx_hash_init_t hash;
	ngx_http_server_name_t  * sn;
	ngx_http_core_srv_conf_t  * cscf;
	if(conf->keys == NULL) {
		conf->hash = prev->hash;

#if (NGX_PCRE)
		ngx_conf_merge_ptr_value(conf->regex, prev->regex, NULL);
		ngx_conf_merge_ptr_value(conf->server_name_regex,
		    prev->server_name_regex, NULL);
#endif
		ngx_conf_merge_value(conf->no_referer, prev->no_referer, 0);
		ngx_conf_merge_value(conf->blocked_referer, prev->blocked_referer, 0);
		ngx_conf_merge_uint_value(conf->referer_hash_max_size,
		    prev->referer_hash_max_size, 2048);
		ngx_conf_merge_uint_value(conf->referer_hash_bucket_size,
		    prev->referer_hash_bucket_size, 64);

		return NGX_CONF_OK;
	}
	if(conf->server_names == 1) {
		cscf = (ngx_http_core_srv_conf_t *)ngx_http_conf_get_module_srv_conf(cf, ngx_http_core_module);
		sn = (ngx_http_server_name_t*)cscf->server_names.elts;
		for(n = 0; n < cscf->server_names.nelts; n++) {
#if (NGX_PCRE)
			if(sn[n].regex) {
				if(ngx_http_add_regex_server_name(cf, conf, sn[n].regex)
				    != NGX_OK) {
					return NGX_CONF_ERROR;
				}

				continue;
			}
#endif

			if(ngx_http_add_referer(cf, conf->keys, &sn[n].name, NULL)
			    != NGX_OK) {
				return NGX_CONF_ERROR;
			}
		}
	}
	if((conf->no_referer == 1 || conf->blocked_referer == 1) && !conf->keys->keys.nelts && !conf->keys->dns_wc_head.nelts && !conf->keys->dns_wc_tail.nelts) {
		ngx_log_error(NGX_LOG_EMERG, cf->log, 0, "the \"none\" or \"blocked\" referers are specified in the \"valid_referers\" directive without any valid referer");
		return NGX_CONF_ERROR;
	}
	ngx_conf_merge_uint_value(conf->referer_hash_max_size, prev->referer_hash_max_size, 2048);
	ngx_conf_merge_uint_value(conf->referer_hash_bucket_size, prev->referer_hash_bucket_size, 64);
	conf->referer_hash_bucket_size = ngx_align(conf->referer_hash_bucket_size, ngx_cacheline_size);

	hash.key = ngx_hash_key_lc;
	hash.max_size = conf->referer_hash_max_size;
	hash.bucket_size = conf->referer_hash_bucket_size;
	hash.name = "referer_hash";
	hash.pool = cf->pool;

	if(conf->keys->keys.nelts) {
		hash.hash = &conf->hash.hash;
		hash.temp_pool = NULL;
		if(ngx_hash_init(&hash, (ngx_hash_key_t*)conf->keys->keys.elts, conf->keys->keys.nelts) != NGX_OK) {
			return NGX_CONF_ERROR;
		}
	}
	if(conf->keys->dns_wc_head.nelts) {
		ngx_qsort(conf->keys->dns_wc_head.elts, (size_t)conf->keys->dns_wc_head.nelts, sizeof(ngx_hash_key_t), ngx_http_cmp_referer_wildcards);
		hash.hash = NULL;
		hash.temp_pool = cf->temp_pool;
		if(ngx_hash_wildcard_init(&hash, (ngx_hash_key_t*)conf->keys->dns_wc_head.elts, conf->keys->dns_wc_head.nelts) != NGX_OK) {
			return NGX_CONF_ERROR;
		}
		conf->hash.wc_head = (ngx_hash_wildcard_t*)hash.hash;
	}
	if(conf->keys->dns_wc_tail.nelts) {
		ngx_qsort(conf->keys->dns_wc_tail.elts, (size_t)conf->keys->dns_wc_tail.nelts, sizeof(ngx_hash_key_t), ngx_http_cmp_referer_wildcards);
		hash.hash = NULL;
		hash.temp_pool = cf->temp_pool;
		if(ngx_hash_wildcard_init(&hash, (ngx_hash_key_t*)conf->keys->dns_wc_tail.elts, conf->keys->dns_wc_tail.nelts) != NGX_OK) {
			return NGX_CONF_ERROR;
		}
		conf->hash.wc_tail = (ngx_hash_wildcard_t*)hash.hash;
	}
#if (NGX_PCRE)
	ngx_conf_merge_ptr_value(conf->regex, prev->regex, NULL);
	ngx_conf_merge_ptr_value(conf->server_name_regex, prev->server_name_regex, NULL);
#endif
	if(conf->no_referer == NGX_CONF_UNSET) {
		conf->no_referer = 0;
	}
	if(conf->blocked_referer == NGX_CONF_UNSET) {
		conf->blocked_referer = 0;
	}
	conf->keys = NULL;
	return NGX_CONF_OK;
}

static const char * ngx_http_valid_referers(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	ngx_http_referer_conf_t  * rlcf = (ngx_http_referer_conf_t *)conf;
	uchar  * p;
	ngx_str_t * value, uri, name;
	ngx_uint_t i;
	ngx_http_variable_t  * var;
	ngx_str_set(&name, "invalid_referer");
	var = ngx_http_add_variable(cf, &name, NGX_HTTP_VAR_CHANGEABLE);
	if(var == NULL) {
		return NGX_CONF_ERROR;
	}
	var->get_handler = ngx_http_referer_variable;
	if(rlcf->keys == NULL) {
		rlcf->keys = (ngx_hash_keys_arrays_t *)ngx_pcalloc(cf->temp_pool, sizeof(ngx_hash_keys_arrays_t));
		if(rlcf->keys == NULL) {
			return NGX_CONF_ERROR;
		}
		rlcf->keys->pool = cf->pool;
		rlcf->keys->temp_pool = cf->pool;
		if(ngx_hash_keys_array_init(rlcf->keys, NGX_HASH_SMALL) != NGX_OK) {
			return NGX_CONF_ERROR;
		}
	}
	value = static_cast<ngx_str_t *>(cf->args->elts);
	for(i = 1; i < cf->args->nelts; i++) {
		if(value[i].len == 0) {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid referer \"%V\"", &value[i]);
			return NGX_CONF_ERROR;
		}
		if(sstreq(value[i].data, "none")) {
			rlcf->no_referer = 1;
			continue;
		}
		if(sstreq(value[i].data, "blocked")) {
			rlcf->blocked_referer = 1;
			continue;
		}
		if(sstreq(value[i].data, "server_names")) {
			rlcf->server_names = 1;
			continue;
		}
		if(value[i].data[0] == '~') {
			if(ngx_http_add_regex_referer(cf, rlcf, &value[i]) != NGX_OK) {
				return NGX_CONF_ERROR;
			}
			continue;
		}
		ngx_str_null(&uri);
		p = (uchar *)ngx_strchr(value[i].data, '/');
		if(p) {
			uri.len = (value[i].data + value[i].len) - p;
			uri.data = p;
			value[i].len = p - value[i].data;
		}
		if(ngx_http_add_referer(cf, rlcf->keys, &value[i], &uri) != NGX_OK) {
			return NGX_CONF_ERROR;
		}
	}
	return NGX_CONF_OK;
}

static ngx_int_t ngx_http_add_referer(ngx_conf_t * cf, ngx_hash_keys_arrays_t * keys, ngx_str_t * value, ngx_str_t * uri)
{
	ngx_int_t rc;
	ngx_str_t * u;
	if(uri == NULL || uri->len == 0) {
		u = (ngx_str_t *)NGX_HTTP_REFERER_NO_URI_PART;
	}
	else {
		u = (ngx_str_t *)ngx_palloc(cf->pool, sizeof(ngx_str_t));
		if(u == NULL) {
			return NGX_ERROR;
		}
		*u = *uri;
	}
	rc = ngx_hash_add_key(keys, value, u, NGX_HASH_WILDCARD_KEY);
	if(rc == NGX_OK) {
		return NGX_OK;
	}
	else {
		if(rc == NGX_DECLINED)
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid hostname or wildcard \"%V\"", value);
		else if(rc == NGX_BUSY)
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "conflicting parameter \"%V\"", value);
		return NGX_ERROR;
	}
}

static ngx_int_t ngx_http_add_regex_referer(ngx_conf_t * cf, ngx_http_referer_conf_t * rlcf, ngx_str_t * name)
{
#if (NGX_PCRE)
	ngx_regex_elt_t * re;
	ngx_regex_compile_t rc;
	uchar errstr[NGX_MAX_CONF_ERRSTR];
	if(name->len == 1) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "empty regex in \"%V\"", name);
		return NGX_ERROR;
	}
	if(rlcf->regex == NGX_CONF_UNSET_PTR) {
		rlcf->regex = ngx_array_create(cf->pool, 2, sizeof(ngx_regex_elt_t));
		if(rlcf->regex == NULL) {
			return NGX_ERROR;
		}
	}
	re = (ngx_regex_elt_t *)ngx_array_push(rlcf->regex);
	if(re == NULL) {
		return NGX_ERROR;
	}
	name->len--;
	name->data++;
	memzero(&rc, sizeof(ngx_regex_compile_t));
	rc.pattern = *name;
	rc.pool = cf->pool;
	rc.options = NGX_REGEX_CASELESS;
	rc.err.len = NGX_MAX_CONF_ERRSTR;
	rc.err.data = errstr;
	if(ngx_regex_compile(&rc) != NGX_OK) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%V", &rc.err);
		return NGX_ERROR;
	}
	re->regex = rc.regex;
	re->name = name->data;
	return NGX_OK;
#else
	ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "the using of the regex \"%V\" requires PCRE library", name);
	return NGX_ERROR;

#endif
}

#if (NGX_PCRE)

static ngx_int_t ngx_http_add_regex_server_name(ngx_conf_t * cf, ngx_http_referer_conf_t * rlcf, ngx_http_regex_t * regex)
{
	ngx_regex_elt_t  * re;
	if(rlcf->server_name_regex == NGX_CONF_UNSET_PTR) {
		rlcf->server_name_regex = ngx_array_create(cf->pool, 2, sizeof(ngx_regex_elt_t));
		if(rlcf->server_name_regex == NULL) {
			return NGX_ERROR;
		}
	}
	re = (ngx_regex_elt_t *)ngx_array_push(rlcf->server_name_regex);
	if(re == NULL) {
		return NGX_ERROR;
	}
	re->regex = regex->regex;
	re->name = regex->name.data;
	return NGX_OK;
}

#endif

static int ngx_libc_cdecl ngx_http_cmp_referer_wildcards(const void * one, const void * two)
{
	ngx_hash_key_t * first = (ngx_hash_key_t*)one;
	ngx_hash_key_t * second = (ngx_hash_key_t*)two;
	return ngx_dns_strcmp(first->key.data, second->key.data);
}
