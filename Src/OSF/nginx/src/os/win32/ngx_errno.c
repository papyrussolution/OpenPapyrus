/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

uchar * ngx_strerror(ngx_err_t err, uchar * errstr, size_t size)
{
	uint len;
	static ulong lang = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
	if(size == 0) {
		return errstr;
	}
	else {
		SString sys_msg_buf;
		len = SSystem::SFormatMessage(err, sys_msg_buf);
		sys_msg_buf.CopyTo(reinterpret_cast<char *>(errstr), size);
		// remove ".\r\n\0" 
		while(errstr[len] == '\0' || errstr[len] == __CR || errstr[len] == LF || errstr[len] == '.') {
			--len;
		}
		return &errstr[++len];
	}
}

ngx_int_t ngx_strerror_init(void)
{
	return NGX_OK;
}

