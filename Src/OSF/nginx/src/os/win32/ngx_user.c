/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

#if (NGX_CRYPT)

ngx_int_t ngx_libc_crypt(ngx_pool_t * pool, uchar * key, uchar * salt, uchar ** encrypted)
{
	/* STUB: a plain text password */
	*encrypted = key;
	return NGX_OK;
}

#endif /* NGX_CRYPT */
