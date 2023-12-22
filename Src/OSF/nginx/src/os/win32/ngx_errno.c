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
		// @v10.3.11 {
		SString sys_msg_buf;
		len = SSystem::SFormatMessage(err, sys_msg_buf);
		sys_msg_buf.CopyTo(reinterpret_cast<char *>(errstr), size);
		// } @v10.3.11 
		/* @v10.3.11
		len = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, lang, (char *)errstr, size, NULL);
		if(len == 0 && lang && GetLastError() == ERROR_RESOURCE_LANG_NOT_FOUND) {
			// Try to use English messages first and fallback to a language,
			// based on locale: non-English Windows have no English messages
			// at all.  This way allows to use English messages at least on Windows with MUI.
			lang = 0;
			len = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, lang, (char *)errstr, size, NULL);
		}
		if(!len) {
			return ngx_snprintf(errstr, size, "FormatMessage() error:(%d)", GetLastError());
		} */
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

