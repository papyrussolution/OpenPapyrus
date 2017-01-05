/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ceutil.h

Abstract:

    Declaration and implementation of Service helper functions

    Contains all registry manipulation functions for the Windows CE Services.
    Note: The Windows CE Services path is always
        "Software\\Microsoft\\Windows CE Services"
    under HKCU or HKLM


Environment:

    WIN32

--*/
#ifndef _INC_CEREG_H
#define _INC_CEREG_H

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

typedef HKEY  HCESVC;
typedef PHKEY PHCESVC;
typedef DWORD DEVICEID;

#define DEVICE_GUEST    (DEVICEID)-1
#define DEVICE_INVALID  (DEVICEID)0

enum {
    CESVC_ROOT_COMMON=0,
    CESVC_ROOT_MACHINE = CESVC_ROOT_COMMON,
    CESVC_ROOT_USER,
    CESVC_DEVICES,
    CESVC_DEVICEX,
    CESVC_DEVICE,
    CESVC_DEVICE_SELECTED,
    CESVC_SERVICES,
    CESVC_SERVICES_USER = CESVC_SERVICES,
    CESVC_SERVICES_COMMON,
    CESVC_SYNC,
    CESVC_SYNC_COMMON,
    CESVC_FILTERS,
    CESVC_SPECIAL_DEFAULTS,
    CESVC_CUSTOM_MENUS,
    CESVC_SYNCX };

#define SVC_FLAG_GUEST            0x0001
#define SVC_FLAG_CURRENT_PROFILE  0x0002
#define SVC_FLAG_PROFILE          0x0004  // specify profile id
#define SVC_FLAG_ALL_PROFILES     0x0008  // all ids
#define SVC_FLAG_COMMON           0x0010  // shared crud
#define SVC_FLAG_ALL              0x001F

typedef struct {
    DWORD   cbSize;
    DWORD   Flags;
    DWORD   ProfileId;
    BOOL    Enabled;
} SVCINFO_GENERIC;

typedef struct {
    DWORD   cbSize;
    DWORD   Flags;
    DWORD   ProfileId;
    BOOL    Enabled;
    LPTSTR  DisplayName;        // sync app name
    LPTSTR  ProgId;
} SVCINFO_SYNC;

//
// Prototypes:
//

//
//  Obsolete APIs 
//
HRESULT __stdcall CeSvcAdd( LPTSTR pszSvcName, LPTSTR pszSvcClass, LPVOID pSvcInfo );
HRESULT __stdcall CeSvcRemove(LPTSTR pszSvcName, LPTSTR pszSvcClass, DWORD dwSvcFlags );
HRESULT __stdcall CeSvcQueryInfo( LPTSTR pszSvcName, LPTSTR pszSvcClass, LPVOID pSvcInfo, DWORD cbBuffer );
HRESULT __stdcall CeSvcDelete( HCESVC hSvc );
//
//  End of obsolete APIs
//


HRESULT __stdcall CeSvcOpen( UINT uSvc, LPTSTR pszPath, BOOL fCreate, PHCESVC phSvc );
HRESULT __stdcall CeSvcOpenEx( HCESVC hSvcRoot, LPTSTR pszPath, BOOL fCreate, PHCESVC phSvc );
HRESULT __stdcall CeSvcClose( HCESVC hSvc );

HRESULT __stdcall CeSvcGetString( HCESVC hSvc, LPCTSTR pszValName, LPTSTR pszVal, DWORD cbVal );
HRESULT __stdcall CeSvcSetString( HCESVC hSvc, LPCTSTR pszValName, LPCTSTR pszVal );
HRESULT __stdcall CeSvcGetDword( HCESVC hSvc, LPCTSTR pszValName, LPDWORD pdwVal );
HRESULT __stdcall CeSvcSetDword( HCESVC hSvc, LPCTSTR pszValName, DWORD dwVal );
HRESULT __stdcall CeSvcGetBinary( HCESVC hSvc, LPCTSTR pszValName, LPBYTE pszVal, LPDWORD pcbVal );
HRESULT __stdcall CeSvcSetBinary( HCESVC hSvc, LPCTSTR pszValName, LPBYTE pszVal, DWORD cbVal );
HRESULT __stdcall CeSvcDeleteVal( HCESVC hSvc, LPCTSTR pszValName );

DEVICEID __stdcall CeGetDeviceId( void );
DEVICEID __stdcall CeGetSelectedDeviceId( void );

HRESULT __stdcall CeSvcEnumProfiles(PHCESVC phSvc, DWORD lProfileIndex, PDWORD plProfile);

#ifdef __cplusplus
}       /* End of extern "C" { */
#endif /* __cplusplus */

#endif  // _INC_CEREG_H

