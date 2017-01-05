/******************************************************************************
 *
 * Copyright (c) 1999-2004 PalmSource, Inc. All rights reserved.
 *
 * File: Hsapi.h
 *
 * Release: 
 *
 *****************************************************************************/

#define HOTSYNC_STATUS_SYNCING  1l
#define HOTSYNC_STATUS_IDLE     0l

#define HS_API_VERSION_1        0x00010001
#define HS_API_VERSION_2        0x00010002

long WINAPI HsGetApiVersion(DWORD *pdwVersion);
long WINAPI HsGetSyncStatus(DWORD *pdwStatus);
long WINAPI HsCheckApiStatus(void);
long WINAPI HsDisplayCustomDlg(void);
long WINAPI HsDisplaySetupDlg(void);
long WINAPI HsDisplayLog(DWORD dwUserId);
//  FileLink is no longer supported in HotSync version 6.X and newer
long WINAPI HsDisplayFileLink(DWORD dwUserId);
long WINAPI HsResetComm(void);

#define HSCONNECTION_ENABLED        0x00000001
#define HSCONNECTION_DISABLED       0x00000000
enum HSConnectionType { CtSerialPort=0, CtModemPort=1, CtNetwork=2, CtIr=3, CtUSB=4, CtReserved};
long WINAPI HsSetCommStatus( HSConnectionType connType, DWORD dwStatus);
long WINAPI HsGetCommStatus( HSConnectionType connType, DWORD *pdwStatus);

enum HsStatusType { HsCloseApp, HsStartApp, HsRestart, HsReserved};

#define HSFLAG_NONE                             0x00000000
#define HSFLAG_RESTORE_REGISTRY                 0x00000001
#define HSFLAG_RESTORE_REGISTRY_DEFAULT         0x00000002
#define HSFLAG_VERBOSE                          0x00000010
#define HSFLAG_LOG_DEBUG_LEVEL_1                0x00000100
#define HSFLAG_LOG_DEBUG_LEVEL_2                0x00000200
#define HSFLAG_INSPECT_CONDUIT                  0x00001000
#define HSFLAG_DEVICE_SYNC_CHECK                0x00010000

long WINAPI HsSetAppStatus(HsStatusType statusType, DWORD dwStartFlags);

#define ERROR_HSAPI_ERROR_BASE              0x10000000
#define ERROR_HSAPI_HOTSYNC_NOT_FOUND       (ERROR_HSAPI_ERROR_BASE + 1)
#define ERROR_HSAPI_INVALID_CONN_TYPE       (ERROR_HSAPI_ERROR_BASE + 2)
#define ERROR_HSAPI_FAILURE                 (ERROR_HSAPI_ERROR_BASE + 3)
#define ERROR_HSAPI_REG_FAILURE             (ERROR_HSAPI_ERROR_BASE + 4)
#define ERROR_HSAPI_UNKNOWN_STATUS_TYPE     (ERROR_HSAPI_ERROR_BASE + 5)
#define ERROR_HSAPI_UNABLE_TO_CLOSE         (ERROR_HSAPI_ERROR_BASE + 6)
#define ERROR_HSAPI_NO_HOTSYNC_PATH         (ERROR_HSAPI_ERROR_BASE + 7)
#define ERROR_HSAPI_UNABLE_TO_START         (ERROR_HSAPI_ERROR_BASE + 8)
#define ERROR_HSAPI_INVALID_STATUS_FLAG     (ERROR_HSAPI_ERROR_BASE + 9)
#define ERROR_HSAPI_INVALID_POINTER		    (ERROR_HSAPI_ERROR_BASE + 10)

long WINAPI HsRefreshConduitInfo(void);
