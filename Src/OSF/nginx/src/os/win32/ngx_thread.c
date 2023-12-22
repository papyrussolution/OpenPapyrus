/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

ngx_err_t ngx_create_thread(ngx_tid_t * tid, ngx_thread_value_t (__stdcall * func)(void * arg), void * arg, ngx_log_t * log)
{
	ulong id;
	*tid = CreateThread(NULL, 0, func, arg, 0, &id);
	if(*tid) {
		ngx_log_error(NGX_LOG_NOTICE, log, 0, "create thread " NGX_TID_T_FMT, id);
		return 0;
	}
	else {
		ngx_err_t err = ngx_errno;
		ngx_log_error(NGX_LOG_ALERT, log, err, "CreateThread() failed");
		return err;
	}
}

