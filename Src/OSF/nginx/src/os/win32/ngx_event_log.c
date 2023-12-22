/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

#define NGX_MAX_ERROR_STR   2048

void ngx_cdecl ngx_event_log(ngx_err_t err, const char * fmt, ...)
{
	long types;
	HKEY key;
	HANDLE ev;
	va_list args;
	uchar text[NGX_MAX_ERROR_STR];
	const TCHAR * msgarg[9];
	static uchar netmsg[] = "%SystemRoot%\\System32\\netmsg.dll";
	uchar * last = text + NGX_MAX_ERROR_STR;
	// @v10.3.11 uchar * p = text + GetModuleFileName(NULL, reinterpret_cast<char *>(text), NGX_MAX_ERROR_STR - 50);
	// @v10.3.11 {
	SString module_file_name;
	const bool gmfnr = SSystem::SGetModuleFileName(0, module_file_name);
	module_file_name.CopyTo(reinterpret_cast<char *>(text), SIZEOFARRAY(text));
	uchar * p = text + sstrlen(text);
	// } @v10.3.11
	*p++ = ':';
	ngx_linefeed(p);
	va_start(args, fmt);
	p = ngx_vslprintf(p, last, fmt, args);
	va_end(args);
	if(err) {
		p = ngx_log_errno(p, last, err);
	}
	if(p > last - NGX_LINEFEED_SIZE - 1) {
		p = last - NGX_LINEFEED_SIZE - 1;
	}
	ngx_linefeed(p);
	*p = '\0';
	/*
	 * we do not log errors here since we use
	 * Event Log only to log our own logs open errors
	 */
	if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\nginx"),
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &key, NULL) != 0) {
		return;
	}
	if(RegSetValueEx(key, _T("EventMessageFile"), 0, REG_EXPAND_SZ, netmsg, sizeof(netmsg) - 1) != 0) {
		return;
	}
	types = EVENTLOG_ERROR_TYPE;
	if(RegSetValueEx(key, _T("TypesSupported"), 0, REG_DWORD, (uchar *)&types, sizeof(long)) != 0) {
		return;
	}
	RegCloseKey(key);
	ev = RegisterEventSource(NULL, _T("nginx"));
	msgarg[0] = SUcSwitch(reinterpret_cast<const char *>(text));
	msgarg[1] = NULL;
	msgarg[2] = NULL;
	msgarg[3] = NULL;
	msgarg[4] = NULL;
	msgarg[5] = NULL;
	msgarg[6] = NULL;
	msgarg[7] = NULL;
	msgarg[8] = NULL;
	/*
	 * the 3299 event id in netmsg.dll has the generic message format:
	 *   "%1 %2 %3 %4 %5 %6 %7 %8 %9"
	 */
	ReportEvent(ev, EVENTLOG_ERROR_TYPE, 0, 3299, NULL, 9, 0, msgarg, NULL);
	DeregisterEventSource(ev);
}

