// GNUPLOT - win/wgnuplib.c 
// Copyright 1992, 1993, 1998, 2004   Russell Lang
//
#include <gnuplot.h>
#pragma hdrstop
#define STRICT
#include <windowsx.h>
#include "wgnuplib.h"
#include "wresourc.h"
#include "wcommon.h"

HINSTANCE hdllInstance;
LPCWSTR szParentClass = L"wgnuplot_parent";
LPCWSTR szTextClass = L"wgnuplot_text";
LPCWSTR szToolbarClass = L"wgnuplot_toolbar";
LPCWSTR szSeparatorClass = L"wgnuplot_separator";
LPCWSTR szPauseClass = L"wgnuplot_pause";

/* Window ID */
struct WID {
	BOOL used;
	HWND hwnd;
	void * ptr;
};

struct WID * widptr = NULL;
uint nwid = 0;
HLOCAL hwid = 0;

void * LocalAllocPtr(UINT flags, UINT size)
{
	HLOCAL hlocal = LocalAlloc(flags, size+1);
	return (char *)LocalLock(hlocal);
}

void * LocalReAllocPtr(void * ptr, UINT flags, UINT size)
{
	HLOCAL hlocal = LocalHandle(ptr);
	LocalUnlock(hlocal);
	hlocal = LocalReAlloc(hlocal, size+1, flags);
	return (char *)LocalLock(hlocal);
}

void LocalFreePtr(void * ptr)
{
	HLOCAL hlocal = LocalHandle(ptr);
	LocalUnlock(hlocal);
	LocalFree(hlocal);
	return;
}

/* ascii to int */
/* returns:
 *  A pointer to character past int if successful,
 *  otherwise NULL on failure.
 *  convert int is stored at pval.
 */
LPTSTR GetInt(LPTSTR str, LPINT pval)
{
	int val = 0;
	BOOL negative = FALSE;
	BOOL success = FALSE;
	TCHAR ch;
	if(!str)
		return NULL;
	while(((ch = *str) != 0) && isspace(ch))
		str++;
	if(ch == '-') {
		negative = TRUE;
		str++;
	}
	while(((ch = *str) != 0) && isdigit(ch)) {
		success = TRUE;
		val = val * 10 + (ch - '0');
		str++;
	}
	if(success) {
		if(negative)
			val = -val;
		*pval = val;
		return str;
	}
	return NULL;
}
