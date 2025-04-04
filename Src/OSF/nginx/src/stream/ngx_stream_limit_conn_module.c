/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop
//#include <ngx_stream.h>

struct ngx_stream_limit_conn_node_t {
	uchar color;
	uchar len;
	ushort conn;
	uchar data[1];
};

struct ngx_stream_limit_conn_cleanup_t {
	ngx_shm_zone_t * shm_zone;
	ngx_rbtree_node_t * node;
};

struct ngx_stream_limit_conn_ctx_t {
	ngx_rbtree_t * rbtree;
	ngx_stream_complex_value_t key;
};

struct ngx_stream_limit_conn_limit_t {
	ngx_shm_zone_t * shm_zone;
	ngx_uint_t conn;
};

struct ngx_stream_limit_conn_conf_t {
	ngx_array_t limits;
	ngx_uint_t log_level;
};

static ngx_rbtree_node_t * ngx_stream_limit_conn_lookup(ngx_rbtree_t * rbtree, ngx_str_t * key, uint32_t hash);
static void ngx_stream_limit_conn_cleanup(void * data);
static ngx_inline void ngx_stream_limit_conn_cleanup_all(ngx_pool_t * pool);

static void * ngx_stream_limit_conn_create_conf(ngx_conf_t * cf);
static char * ngx_stream_limit_conn_merge_conf(ngx_conf_t * cf, void * parent, void * child);
static const char * ngx_stream_limit_conn_zone(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static const char * ngx_stream_limit_conn(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static ngx_int_t ngx_stream_limit_conn_init(ngx_conf_t * cf);

static ngx_conf_enum_t ngx_stream_limit_conn_log_levels[] = {
	{ ngx_string("info"), NGX_LOG_INFO },
	{ ngx_string("notice"), NGX_LOG_NOTICE },
	{ ngx_string("warn"), NGX_LOG_WARN },
	{ ngx_string("error"), NGX_LOG_ERR },
	{ ngx_null_string, 0 }
};

static ngx_command_t ngx_stream_limit_conn_commands[] = {
	{ ngx_string("limit_conn_zone"), NGX_STREAM_MAIN_CONF|NGX_CONF_TAKE2,
	  ngx_stream_limit_conn_zone, 0, 0, NULL },
	{ ngx_string("limit_conn"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE2,
	  ngx_stream_limit_conn, NGX_STREAM_SRV_CONF_OFFSET, 0, NULL },
	{ ngx_string("limit_conn_log_level"), NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_enum_slot, NGX_STREAM_SRV_CONF_OFFSET, offsetof(ngx_stream_limit_conn_conf_t, log_level), &ngx_stream_limit_conn_log_levels },
	ngx_null_command
};

static ngx_stream_module_t ngx_stream_limit_conn_module_ctx = {
	NULL,                              /* preconfiguration */
	ngx_stream_limit_conn_init,        /* postconfiguration */

	NULL,                              /* create main configuration */
	NULL,                              /* init main configuration */

	ngx_stream_limit_conn_create_conf, /* create server configuration */
	ngx_stream_limit_conn_merge_conf   /* merge server configuration */
};

ngx_module_t ngx_stream_limit_conn_module = {
	NGX_MODULE_V1,
	&ngx_stream_limit_conn_module_ctx, /* module context */
	ngx_stream_limit_conn_commands,    /* module directives */
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

static ngx_int_t ngx_stream_limit_conn_handler(ngx_stream_session_t * s)
{
	size_t n;
	uint32_t hash;
	ngx_str_t key;
	ngx_uint_t i;
	ngx_slab_pool_t  * shpool;
	ngx_rbtree_node_t  * node;
	ngx_pool_cleanup_t * cln;
	ngx_stream_limit_conn_ctx_t * ctx;
	ngx_stream_limit_conn_node_t * lc;
	ngx_stream_limit_conn_cleanup_t * lccln;
	ngx_stream_limit_conn_conf_t * lccf = (ngx_stream_limit_conn_conf_t *)ngx_stream_get_module_srv_conf(s, ngx_stream_limit_conn_module);
	ngx_stream_limit_conn_limit_t * limits = (ngx_stream_limit_conn_limit_t *)lccf->limits.elts;
	for(i = 0; i < lccf->limits.nelts; i++) {
		ctx = (ngx_stream_limit_conn_ctx_t *)limits[i].shm_zone->data;
		if(ngx_stream_complex_value(s, &ctx->key, &key) != NGX_OK) {
			return NGX_ERROR;
		}
		if(key.len == 0) {
			continue;
		}
		if(key.len > 255) {
			ngx_log_error(NGX_LOG_ERR, s->connection->log, 0, "the value of the \"%V\" key is more than 255 bytes: \"%V\"", &ctx->key.value, &key);
			continue;
		}
		hash = ngx_crc32_short(key.data, key.len);
		shpool = (ngx_slab_pool_t*)limits[i].shm_zone->shm.addr;
		ngx_shmtx_lock(&shpool->mutex);
		node = ngx_stream_limit_conn_lookup(ctx->rbtree, &key, hash);
		if(!node) {
			n = offsetof(ngx_rbtree_node_t, color) + offsetof(ngx_stream_limit_conn_node_t, data) + key.len;
			node = (ngx_rbtree_node_t *)ngx_slab_alloc_locked(shpool, n);
			if(!node) {
				ngx_shmtx_unlock(&shpool->mutex);
				ngx_stream_limit_conn_cleanup_all(s->connection->pool);
				return NGX_STREAM_SERVICE_UNAVAILABLE;
			}
			lc = (ngx_stream_limit_conn_node_t*)&node->color;
			node->key = hash;
			lc->len = (uchar)key.len;
			lc->conn = 1;
			memcpy(lc->data, key.data, key.len);
			ngx_rbtree_insert(ctx->rbtree, node);
		}
		else {
			lc = (ngx_stream_limit_conn_node_t*)&node->color;
			if((ngx_uint_t)lc->conn >= limits[i].conn) {
				ngx_shmtx_unlock(&shpool->mutex);
				ngx_log_error(lccf->log_level, s->connection->log, 0, "limiting connections by zone \"%V\"", &limits[i].shm_zone->shm.name);
				ngx_stream_limit_conn_cleanup_all(s->connection->pool);
				return NGX_STREAM_SERVICE_UNAVAILABLE;
			}
			lc->conn++;
		}
		ngx_log_debug2(NGX_LOG_DEBUG_STREAM, s->connection->log, 0, "limit conn: %08Xi %d", node->key, lc->conn);
		ngx_shmtx_unlock(&shpool->mutex);
		cln = ngx_pool_cleanup_add(s->connection->pool, sizeof(ngx_stream_limit_conn_cleanup_t));
		if(cln == NULL) {
			return NGX_ERROR;
		}
		cln->handler = ngx_stream_limit_conn_cleanup;
		lccln = (ngx_stream_limit_conn_cleanup_t *)cln->data;
		lccln->shm_zone = limits[i].shm_zone;
		lccln->node = node;
	}
	return NGX_DECLINED;
}

static void ngx_stream_limit_conn_rbtree_insert_value(ngx_rbtree_node_t * temp, ngx_rbtree_node_t * node, ngx_rbtree_node_t * sentinel)
{
	ngx_rbtree_node_t ** p;
	ngx_stream_limit_conn_node_t * lcn, * lcnt;
	for(;;) {
		if(node->key < temp->key) {
			p = &temp->left;
		}
		else if(node->key > temp->key) {
			p = &temp->right;
		}
		else { /* node->key == temp->key */
			lcn = (ngx_stream_limit_conn_node_t*)&node->color;
			lcnt = (ngx_stream_limit_conn_node_t*)&temp->color;

			p = (ngx_memn2cmp(lcn->data, lcnt->data, lcn->len, lcnt->len) < 0)
			    ? &temp->left : &temp->right;
		}

		if(*p == sentinel) {
			break;
		}

		temp = *p;
	}

	*p = node;
	node->parent = temp;
	node->left = sentinel;
	node->right = sentinel;
	ngx_rbt_red(node);
}

static ngx_rbtree_node_t * ngx_stream_limit_conn_lookup(ngx_rbtree_t * rbtree, ngx_str_t * key,
    uint32_t hash)
{
	ngx_int_t rc;
	ngx_rbtree_node_t   * node, * sentinel;
	ngx_stream_limit_conn_node_t  * lcn;

	node = rbtree->root;
	sentinel = rbtree->sentinel;

	while(node != sentinel) {
		if(hash < node->key) {
			node = node->left;
			continue;
		}

		if(hash > node->key) {
			node = node->right;
			continue;
		}

		/* hash == node->key */

		lcn = (ngx_stream_limit_conn_node_t*)&node->color;

		rc = ngx_memn2cmp(key->data, lcn->data, key->len, (size_t)lcn->len);

		if(rc == 0) {
			return node;
		}

		node = (rc < 0) ? node->left : node->right;
	}

	return NULL;
}

static void ngx_stream_limit_conn_cleanup(void * data)
{
	ngx_stream_limit_conn_cleanup_t * lccln = (ngx_stream_limit_conn_cleanup_t *)data;
	ngx_slab_pool_t * shpool;
	ngx_rbtree_node_t   * node;
	ngx_stream_limit_conn_ctx_t * ctx;
	ngx_stream_limit_conn_node_t  * lc;
	ctx = (ngx_stream_limit_conn_ctx_t *)lccln->shm_zone->data;
	shpool = (ngx_slab_pool_t*)lccln->shm_zone->shm.addr;
	node = lccln->node;
	lc = (ngx_stream_limit_conn_node_t*)&node->color;
	ngx_shmtx_lock(&shpool->mutex);
	ngx_log_debug2(NGX_LOG_DEBUG_STREAM, lccln->shm_zone->shm.log, 0, "limit conn cleanup: %08Xi %d", node->key, lc->conn);
	lc->conn--;
	if(lc->conn == 0) {
		ngx_rbtree_delete(ctx->rbtree, node);
		ngx_slab_free_locked(shpool, node);
	}
	ngx_shmtx_unlock(&shpool->mutex);
}

static ngx_inline void ngx_stream_limit_conn_cleanup_all(ngx_pool_t * pool)
{
	ngx_pool_cleanup_t  * cln = pool->cleanup;
	while(cln && cln->handler == ngx_stream_limit_conn_cleanup) {
		ngx_stream_limit_conn_cleanup(cln->data);
		cln = cln->next;
	}

	pool->cleanup = cln;
}

static ngx_int_t ngx_stream_limit_conn_init_zone(ngx_shm_zone_t * shm_zone, void * data)
{
	ngx_stream_limit_conn_ctx_t  * octx = (ngx_stream_limit_conn_ctx_t *)data;
	size_t len;
	ngx_slab_pool_t    * shpool;
	ngx_rbtree_node_t  * sentinel;
	ngx_stream_limit_conn_ctx_t  * ctx = (ngx_stream_limit_conn_ctx_t *)shm_zone->data;
	if(octx) {
		if(ctx->key.value.len != octx->key.value.len || ngx_strncmp(ctx->key.value.data, octx->key.value.data, ctx->key.value.len) != 0) {
			ngx_log_error(NGX_LOG_EMERG, shm_zone->shm.log, 0,
			    "limit_conn_zone \"%V\" uses the \"%V\" key while previously it used the \"%V\" key",
			    &shm_zone->shm.name, &ctx->key.value, &octx->key.value);
			return NGX_ERROR;
		}
		ctx->rbtree = octx->rbtree;
		return NGX_OK;
	}
	shpool = (ngx_slab_pool_t*)shm_zone->shm.addr;
	if(shm_zone->shm.exists) {
		ctx->rbtree = (ngx_rbtree_t *)shpool->data;
		return NGX_OK;
	}
	ctx->rbtree = (ngx_rbtree_t *)ngx_slab_alloc(shpool, sizeof(ngx_rbtree_t));
	if(ctx->rbtree == NULL) {
		return NGX_ERROR;
	}
	shpool->data = ctx->rbtree;
	sentinel = (ngx_rbtree_node_t *)ngx_slab_alloc(shpool, sizeof(ngx_rbtree_node_t));
	if(sentinel == NULL) {
		return NGX_ERROR;
	}
	ngx_rbtree_init(ctx->rbtree, sentinel, ngx_stream_limit_conn_rbtree_insert_value);
	len = sizeof(" in limit_conn_zone \"\"") + shm_zone->shm.name.len;
	shpool->log_ctx = (uchar *)ngx_slab_alloc(shpool, len);
	if(shpool->log_ctx == NULL) {
		return NGX_ERROR;
	}
	ngx_sprintf(shpool->log_ctx, " in limit_conn_zone \"%V\"%Z", &shm_zone->shm.name);
	return NGX_OK;
}

static void * ngx_stream_limit_conn_create_conf(ngx_conf_t * cf)
{
	ngx_stream_limit_conn_conf_t * conf = (ngx_stream_limit_conn_conf_t *)ngx_pcalloc(cf->pool, sizeof(ngx_stream_limit_conn_conf_t));
	if(conf) {
		/*
		 * set by ngx_pcalloc():
		 *
		 *   conf->limits.elts = NULL;
		 */
		conf->log_level = NGX_CONF_UNSET_UINT;
	}
	return conf;
}

static char * ngx_stream_limit_conn_merge_conf(ngx_conf_t * cf, void * parent, void * child)
{
	ngx_stream_limit_conn_conf_t * prev = (ngx_stream_limit_conn_conf_t *)parent;
	ngx_stream_limit_conn_conf_t * conf = (ngx_stream_limit_conn_conf_t *)child;
	if(conf->limits.elts == NULL) {
		conf->limits = prev->limits;
	}
	ngx_conf_merge_uint_value(conf->log_level, prev->log_level, NGX_LOG_ERR);
	return NGX_CONF_OK;
}

static const char * ngx_stream_limit_conn_zone(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	uchar  * p;
	ssize_t size;
	ngx_str_t name, s;
	ngx_uint_t i;
	ngx_shm_zone_t  * shm_zone;
	ngx_stream_compile_complex_value_t ccv;
	ngx_str_t * value = static_cast<ngx_str_t *>(cf->args->elts);
	ngx_stream_limit_conn_ctx_t * ctx = (ngx_stream_limit_conn_ctx_t *)ngx_pcalloc(cf->pool, sizeof(ngx_stream_limit_conn_ctx_t));
	if(!ctx) {
		return NGX_CONF_ERROR;
	}
	memzero(&ccv, sizeof(ngx_stream_compile_complex_value_t));
	ccv.cf = cf;
	ccv.value = &value[1];
	ccv.complex_value = &ctx->key;
	if(ngx_stream_compile_complex_value(&ccv) != NGX_OK) {
		return NGX_CONF_ERROR;
	}
	size = 0;
	name.len = 0;
	for(i = 2; i < cf->args->nelts; i++) {
		if(ngx_strncmp(value[i].data, "zone=", 5) == 0) {
			name.data = value[i].data + 5;
			p = (uchar *)ngx_strchr(name.data, ':');
			if(!p) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid zone size \"%V\"", &value[i]);
				return NGX_CONF_ERROR;
			}
			name.len = p - name.data;
			s.data = p + 1;
			s.len = value[i].data + value[i].len - s.data;
			size = ngx_parse_size(&s);
			if(size == NGX_ERROR) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid zone size \"%V\"", &value[i]);
				return NGX_CONF_ERROR;
			}
			if(size < (ssize_t)(8 * ngx_pagesize)) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "zone \"%V\" is too small", &value[i]);
				return NGX_CONF_ERROR;
			}
			continue;
		}
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid parameter \"%V\"", &value[i]);
		return NGX_CONF_ERROR;
	}
	if(name.len == 0) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "\"%V\" must have \"zone\" parameter", &cmd->Name);
		return NGX_CONF_ERROR;
	}
	shm_zone = ngx_shared_memory_add(cf, &name, size, &ngx_stream_limit_conn_module);
	if(shm_zone == NULL) {
		return NGX_CONF_ERROR;
	}
	if(shm_zone->data) {
		ctx = (ngx_stream_limit_conn_ctx_t *)shm_zone->data;
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%V \"%V\" is already bound to key \"%V\"", &cmd->Name, &name, &ctx->key.value);
		return NGX_CONF_ERROR;
	}
	shm_zone->F_Init = ngx_stream_limit_conn_init_zone;
	shm_zone->data = ctx;
	return NGX_CONF_OK;
}

static const char * ngx_stream_limit_conn(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	ngx_stream_limit_conn_conf_t * lccf = (ngx_stream_limit_conn_conf_t *)conf;
	ngx_stream_limit_conn_limit_t  * limit, * limits;
	ngx_int_t n;
	ngx_uint_t i;
	ngx_str_t * value = static_cast<ngx_str_t *>(cf->args->elts);
	ngx_shm_zone_t * shm_zone = ngx_shared_memory_add(cf, &value[1], 0, &ngx_stream_limit_conn_module);
	if(shm_zone == NULL) {
		return NGX_CONF_ERROR;
	}
	limits = (ngx_stream_limit_conn_limit_t *)lccf->limits.elts;
	if(limits == NULL) {
		if(ngx_array_init(&lccf->limits, cf->pool, 1, sizeof(ngx_stream_limit_conn_limit_t)) != NGX_OK) {
			return NGX_CONF_ERROR;
		}
	}
	for(i = 0; i < lccf->limits.nelts; i++) {
		if(shm_zone == limits[i].shm_zone) {
			return "is duplicate";
		}
	}
	n = ngx_atoi(value[2].data, value[2].len);
	if(n <= 0) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid number of connections \"%V\"", &value[2]);
		return NGX_CONF_ERROR;
	}
	if(n > 65535) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "connection limit must be less 65536");
		return NGX_CONF_ERROR;
	}
	limit = (ngx_stream_limit_conn_limit_t *)ngx_array_push(&lccf->limits);
	if(limit == NULL) {
		return NGX_CONF_ERROR;
	}
	limit->conn = n;
	limit->shm_zone = shm_zone;
	return NGX_CONF_OK;
}

static ngx_int_t ngx_stream_limit_conn_init(ngx_conf_t * cf)
{
	ngx_stream_core_main_conf_t  * cmcf = (ngx_stream_core_main_conf_t *)ngx_stream_conf_get_module_main_conf(cf, ngx_stream_core_module);
	ngx_stream_handler_pt * h = (ngx_stream_handler_pt *)ngx_array_push(&cmcf->phases[NGX_STREAM_PREACCESS_PHASE].handlers);
	if(!h) {
		return NGX_ERROR;
	}
	*h = ngx_stream_limit_conn_handler;
	return NGX_OK;
}
