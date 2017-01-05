/*****************************************************************************
 *
 * Copyright (c) 1998-1999 Palm Computing, Inc. or its subsidiaries.  
 * All rights reserved.
 *
 ****************************************************************************/
#ifndef __PALM_COMMON_DLL_H_ 
#define __PALM_COMMON_DLL_H_
// @sobolev #include <LANG_DLL.H>
#include <palmcdk\403\LANG_DLL.H> // @sobolev

#define PS_BASE_REG_PATH "Software\\U.S. Robotics\\Pilot Desktop\\"
#define PS_MAX_LENGTH MAX_PATH 

#define PS_ERROR_INVALID_SECTION	-7
#define PS_ERROR_INVALID_NAME		-6
#define PS_ERROR_ACCESSING_STORE	-5
#define PS_ERROR_OUT_OF_BOUNDS		-4
#define PS_ERROR_WRONG_TYPE			-3
#define PS_ERROR_BUFFER_TOO_SMALL	-2
#define PS_ERROR_INVALID_PARAMETER	-1
#define PS_ERROR_SUCCESS			0

// conduit C API

extern "C" {
long APIENTRY 
PsGetSectionCount(long *piCount);

long APIENTRY 
PsGetSectionByIndex(long iIndex, TCHAR *pBuffer, long *piSize);

long APIENTRY 
PsGetNameCount(const TCHAR *pszSection, long *piCount);

long APIENTRY 
PsGetNameByIndex(const TCHAR *pszSection, long iIndex, TCHAR *pBuffer, long *piSize);

long APIENTRY 
PsGetVersion(void);

long APIENTRY 
PsGetString(const TCHAR *pszSection, const TCHAR *pszName, TCHAR *pBuffer, long *piSize,  const TCHAR *pszDefault);

long APIENTRY 
PsGetInteger(const TCHAR *pszSection, const TCHAR *pszName, long *piValue, long iDefault);

long APIENTRY 
PsSetString(const TCHAR *pszSection, const TCHAR *pszName, const TCHAR* pszValue);

long APIENTRY 
PsSetInteger(const TCHAR *pszSection, const TCHAR *pszName, long iValue);

long APIENTRY 
PsDelete(const TCHAR *pszSection, const TCHAR *pszName);

long APIENTRY PsGetSubSectionCount(const TCHAR *pszSection, long *piCount);
long APIENTRY PsGetSubSectionByIndex(const TCHAR *pszSection, long iIndex, TCHAR *pBuffer, long *piSize);



HINSTANCE WINAPI PalmLoadLanguage(LPCTSTR pFileName, 
                           HINSTANCE hAppInst, 
                           DWORD *pdwVersion);
DWORD WINAPI    PalmGetVersion(void);
BOOL WINAPI     PalmFreeLanguage(HINSTANCE hRscInst, 
                      HINSTANCE hAppInst);
DWORD WINAPI PalmGetResourceVersion(HINSTANCE hLangInstance);

#define GENERAL_PALM_DEVICE         1
#define GENERAL_GENERIC_DEVICE      2
#define CDK_TOOLS_PALM              3
#define CDK_TOOLS_GENERIC           4

void WINAPI PalmAboutBox(HWND hParent,
                         const char *pAppName,
                         HICON hIcon,
                         const char *pAboutString,
                         int iBitmapType);

}

#endif