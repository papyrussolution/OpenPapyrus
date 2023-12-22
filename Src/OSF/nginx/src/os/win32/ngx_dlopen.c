/*
 * Copyright (C) Maxim Dounin
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

char * ngx_dlerror(void)
{
	static uchar errstr[NGX_MAX_ERROR_STR];
	uchar * p = ngx_strerror(ngx_errno, errstr, NGX_MAX_ERROR_STR);
	*p = '\0';
	return (char *)errstr;
}

