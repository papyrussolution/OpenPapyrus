// SLIB-WINAPI-HELPER.CPP
// Copyright (c) A.Sobolev 2021
//
#include <slib-internal.h>
#pragma hdrstop

void FASTCALL ZDeleteWinGdiObject(void * pHandle)
{
	if(pHandle) {
		HGDIOBJ * p_obj = static_cast<HGDIOBJ *>(pHandle);
		if(*p_obj) {
			::DeleteObject(*p_obj);
			*p_obj = 0;
		}
	}
}
