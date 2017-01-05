#ifndef _MONITOR_H_INCLUDED
/*************************************************************************
**
**  Copyright 1998-1999 Pervasive Software Inc. All Rights Reserved
**
*************************************************************************/
/*************************************************************************
*   MONITOR.H
*
*
*   This header prototypes the Monitoring functions
*   of the Pervasive Distributed Tuning Interface.
*
*     The following functions are found in this file:
*
*       PvGetOpenFilesData()
*       PvFreeOpenFilesData()
*       PvGetOpenFileName()
*       PvGetFileInfo()
*       PvGetFileHandlesData()
*       PvGetFileHandleInfo()
*       PvGetMkdeClientsData()
*       PvFreeMkdeClientsData()
*       PvGetMkdeClientId()
*       PvDisconnectMkdeClient()
*       PvGetMkdeClientInfo()
*       PvGetMkdeClientHandlesData()
*       PvGetMkdeClientHandleInfo()
*       PvGetMkdeUsage()
*       PvGetMkdeUsageEx()
*       PvGetMkdeCommStat()
*       PvGetMkdeCommStatEx()
*       PvGetMkdeVersion()
*       PvGetSQLConnectionsData()
*       PvFreeSQLConnectionsData()
*       PvGetSQLConnectionInfo()
*       PvDisconnectSQLConnection()
*
*
*************************************************************************/
/*===========================================================================
 *
 * Monitoring & Diagnostics API
 *
 *=========================================================================*/


#define  P_MAX_NAME_SIZE         64    /* maximum size of common names */
#define  P_MAX_PATH_SIZE         1024  /* maximum size of a full path */
#define  P_MAX_COMM_PROTOCOLS    8     /* maximum communication protocols */

#ifdef __cplusplus
extern "C" {
#endif

/* Don't pad our structures */
#pragma pack(push, 8)

/*
 * structure for date-time informaton.
 *
 */
typedef struct tagPVDATETIME
{
   BTI_WORD year;           // Year (current year minus 1900)
   BTI_WORD month;          // Month (0 – 11; January = 0)
   BTI_WORD day;            // Day of month (1 – 31)
   BTI_WORD hour;           // Hours since midnight (0 – 23)
   BTI_WORD minute;         // Minutes after hour (0 – 59)
   BTI_WORD second;         // Seconds after minute (0 – 59)
   BTI_WORD millisecond;    // Milliseconds after minute (0 – 59000). Default to 0 if the smallest time unit is seconds.
} PVDATETIME;


/*
 * Name:
 *    PvGetOpenFilesData()
 *
 * Description:
 *    Retrieve all the information related to the open files. The
 *    information will be cached by DBADMIN for subsequent calls related to
 *    open files.  This function should be called first before calling any
 *    other functions to get open file information.  The caller should call
 *    PvFreeOpenFilesData() to free the cached information when it is
 *    no longer needed.
 *
 *    This function may also be called to refresh the cached information.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pCount         [out]     Address of an usinged long to receive the
 *                             number of open files
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_FAIL                 Failed for any other reason
 */
BTI_API PvGetOpenFilesData(
   BTI_LONG       hConnection,
   BTI_ULONG_PTR  pCount);


/*
 * Name:
 *    PvFreeOpenFilesData()
 *
 * Description:
 *    Free the cached information related to the open files.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Data for open files retrieved by calling PvGetOpenFilesData();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_DATA_UNAVAILABLE     Data related to open files not available
 *    P_E_FAIL                 Failed for any other reason
 */
BTI_API PvFreeOpenFilesData(
   BTI_LONG hConnection);



/*
 * Name:
 *    PvGetOpenFileName()
 *
 * Description:
 *    Get the full path name of an open file.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Data for open files retrieved by calling PvGetOpenFilesData();
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    sequence        [in]     The sequence number (zero based) of the file.
 *                             Must be within a valid range with upper limit
 *                             returned by PvGetOpenFilesData
 *    pBufSize        [in/out] Address of an unsigned long containing size
 *                             of buffer allocated to receive the file name.
 *                             Receives actual size of chars copied.
 *                             The size should include the null terminator.
 *    fileName        [in/out] String value returned
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_DATA_UNAVAILABLE     Data related to open files not available
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_BUFFER_TOO_SMALL     Allocated buffer is too small for the string,
 *                             returned string is truncated.  In this case
 *                             the required size is retured in pBufSize.
 *    P_E_INVALID_SEQUENCE     Sequence number is not valid
 *    P_E_FAIL                 Failed for any other reason
 */
BTI_API PvGetOpenFileName(
   BTI_LONG          hConnection,
   BTI_ULONG         sequence,
   BTI_ULONG_PTR     pBufSize,
   BTI_CHAR_PTR      fileName);


/*
 * Name:
 *    PvGetFileInfo()
 *
 * Description:
 *    Query the information for an open file
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Data for open files retrieved by calling PvGetOpenFilesData();
 *    Caller already has a valid open file name;
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    fileName       [in]      Full path name of the file to be queried
 *    pFileInfo      [out]     Address of a PVFILEINFO struct to receive the
 *                             information on the file
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_DATA_UNAVAILABLE     Data related to open files not available
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_FILE_NOT_OPEN        Specified file is not open currently
 *    P_E_FAIL                 Failed for any other reason
 *
 */
typedef struct tagPVFILEINFO
{
   BTI_BYTE    openMode;            /* open mode */
   BTI_BYTE    locksFlag;           /* TRUE if locked */
   BTI_BYTE    transFlag;           /* TRUE if in transaction mode */
   BTI_BYTE    tTSFlag;             /*  */
   BTI_BYTE    readOnly;            /* TRUE if opened for read-only access */
   BTI_BYTE    continuousOpsFlag;   /*  */
   BTI_BYTE    referentialIntgFlag; /*  */
   BTI_ULONG   aFLIndex;            /*  */
   BTI_ULONG   activeCursors;       /*  */
   BTI_ULONG   pageSize;            /* page size in bytes */
   PVDATETIME  openTimeStamp;       /* time when the file was open */
} PVFILEINFO;

BTI_API PvGetFileInfo(
   BTI_LONG          hConnection,
   BTI_CHAR_PTR      fileName,
   PVFILEINFO*       pFileInfo);


/*
 * Name:
 *    PvGetFileHandlesData()
 *
 * Description:
 *    Retrieve all the file handle information related to an open file. The
 *    information will be cached by DBADMIN for subsequent calls related to
 *    file handles.  This function should be called first for an open file
 *    before calling any other functions to get file handle information.
 *    The cached information for the file handles will be freed when
 *    PvFreeOpenFilesData() is called.
 *
 *    This function may also be called to refresh the cached file handle
 *    information.  DBADMIN only caches open file handles data for one open
 *    file.  If information is requested with a different file name, the
 *    cache is freed and data are retrieved for the new file.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Data for open files retrieved by calling PvGetOpenFilesData();
 *    Caller already has a valid open file name;
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    fileName       [in]      full path name of the file to be queried
 *    pCount         [out]     Address of an usinged long to receive the
 *                             number of handles for the open file
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_DATA_UNAVAILABLE     Data related to open files not available
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_FILE_NOT_OPEN        Specified file is not open currently
 *    P_E_FAIL                 Failed for any other reason
 */
BTI_API PvGetFileHandlesData(
   BTI_LONG          hConnection,
   BTI_CHAR_PTR      fileName,
   BTI_ULONG_PTR     pCount);


/*
 * Name:
 *    PvGetFileHandleInfo()
 *
 * Description:
 *    Query the information for a file handle associated with an open file
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Data for open files retrieved by calling PvGetOpenFilesData();
 *    Data for open file handles retrieved by PvGetFileHandlesData();
 *    Caller already has a valid open file name;
 *    Caller already has a valid file handle sequence;
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    fileName       [in]      full path name of the file to be queried
 *    sequence       [in]      The sequence number (zero based) of the file
 *                             handle.  Must be within a valid range with
 *                             upper limit defined by the number of file
 *                             handles obtained by PvGetFileHandlesData()
 *    pFileHdlInfo   [out]     Address of a PVFILEHDLINFO struct to receive
 *                             the information on the file handle
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_DATA_UNAVAILABLE     Data not available for the file or handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_SEQUENCE     Sequence number is not valid
 *    P_E_FILE_NOT_OPEN        Specified file is not open currently
 *    P_E_FAIL                 Failed for any other reason
 *
 */
typedef struct tagPVFILEHDLINFO
{
   BTI_ULONG   clientIndex;         /*  */
   BTI_BYTE    openMode;            /* open mode */
   BTI_BYTE    locksFlag;           /* TRUE if the file is locked */
   BTI_BYTE    waitFlag;            /* TRUE if in waiting mode */
   BTI_WORD    transState;          /* transaction state */
   BTI_CHAR    userName[P_MAX_NAME_SIZE];    /* user who owns this handle */
} PVFILEHDLINFO;

BTI_API PvGetFileHandleInfo(
   BTI_LONG          hConnection,
   BTI_CHAR_PTR      fileName,
   BTI_ULONG         sequence,
   PVFILEHDLINFO*    pFileHdlInfo);


/*
 * Name:
 *    PvGetMkdeClientsData()
 *
 * Description:
 *    Retrieve all the information related to the active MKDE clients.  The
 *    information will be cached by DBADMIN for subsequent calls related to
 *    MKDE clients.  This function should be called first before calling any
 *    other functions to get MKDE client information.  The caller should call
 *    PvFreeMkdeClientsData() to free the cached information when it is no
 *    longer needed.
 *
 *    This function may also be called to refresh the cached information.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pCount         [out]     Address of an usinged long to receive the
 *                             number of active MKDE clients
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_FAIL                 Failed for any other reason
 */
BTI_API PvGetMkdeClientsData(
   BTI_LONG       hConnection,
   BTI_ULONG_PTR  pCount);


/*
 * Name:
 *    PvFreeMkdeClientsData()
 *
 * Description:
 *    Free the cached information related to the active MKDE clients.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Data for actvie clients retrieved by calling PvGetMkdeClientsData();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_DATA_UNAVAILABLE     Data related to active clients not available
 *    P_E_FAIL                 Failed for any other reason
 */
BTI_API PvFreeMkdeClientsData(
   BTI_LONG hConnection);



/*
 * Name:
 *    PvGetMkdeClientId()
 *
 * Description:
 *    Get the client ID of an active MKDE client
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Data for actvie clients retrieved by calling PvGetMkdeClientsData();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    sequence       [in]      The sequence number (zero based) of the MKDE
 *                             client.  Must be within a valid range with
 *                             upper limit returned by PvGetMkdeClientsData
 *    pClientId      [out]     Address of the PVCLIENTID structure to hold
 *                             the returned client ID information
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_DATA_UNAVAILABLE     Data related to active clients not available
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_SEQUENCE     Sequence number is not valid
 *    P_E_FAIL                 Failed for any other reason
 *
 */
typedef struct tagPVCLIENTID
{
   BTI_CHAR    clientID[12];
   BTI_WORD    serviceAgentID;
   BTI_WORD    taskNumber;
} PVCLIENTID;

BTI_API PvGetMkdeClientId(
   BTI_LONG          hConnection,
   BTI_ULONG         sequence,
   PVCLIENTID*       pClientId);


/*
 * Name:
 *    PvDisconnectMkdeClient()
 *
 * Description:
 *    Disconnect an active MKDE client
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Data for actvie clients retrieved by calling PvGetMkdeClientsData();
 *    Caller already has a valid active MKDE client ID;
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pClientId      [in]      Address of the PVCLIENTID structure to
 *                             identify the MKDE client
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_DATA_UNAVAILABLE     Data related to active clients not available
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_CLIENT       Invalid client ID
 *    P_E_FAIL                 Failed for any other reason
 */
BTI_API PvDisconnectMkdeClient(
   BTI_LONG          hConnection,
   PVCLIENTID*       pClientId);


/*
 * Name:
 *    PvGetMkdeClientInfo()
 *
 * Description:
 *    Query the information for an active MKDE client
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Data for actvie clients retrieved by calling PvGetMkdeClientsData();
 *    Caller already has a valid active MKDE client ID;
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pClientId      [in]      Address of the PVCLIENTID structure to
 *                             identify the MKDE client
 *    pClientInfo    [out]     Address of a PVMKDECLIENTINFO struct to
 *                             receive the information for the MKDE client
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_DATA_UNAVAILABLE     Data related to active clients not available
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_CLIENT       Invalid client ID
 *    P_E_FAIL                 Failed for any other reason
 *
 */
typedef struct tagPVMKDECLIENTINFO
{
   BTI_WORD    clientSite;       /* CLIENT_SITE_XXX constant */
   BTI_WORD    clientPlatform;   /* PLATFORM_XXX constant */
   BTI_CHAR    netAddress[80];   /* Net address, machine name, (any protocol) */
   BTI_WORD    numCursors;
   BTI_ULONG   recordsDeleted;
   BTI_ULONG   recordsUpdated;
   BTI_ULONG   recordsInserted;
   BTI_ULONG   recordsRead;
   BTI_ULONG   diskAccesses;
   BTI_ULONG   cacheAccesses;
   BTI_ULONG   currentLocks;
   BTI_WORD    transState;       /* 0 == none, 19 == excl, 1019 == concurr */
   BTI_WORD    transLevel;       /* future */
   BTI_ULONG   btrvID;
   BTI_ULONG   connectionNumber;
   BTI_CHAR    userName[P_MAX_NAME_SIZE];   /* user login name or "NA" */
} PVMKDECLIENTINFO;

BTI_API PvGetMkdeClientInfo(
   BTI_LONG          hConnection,
   PVCLIENTID*       pClientId,
   PVMKDECLIENTINFO* pClientInfo);


/*
 * Name:
 *    PvGetMkdeClientHandlesData()
 *
 * Description:
 *    Retrieve all the MKDE client handle information related to an active
 *    MKDE client.  The information will be cached by DBADMIN for subsequent
 *    calls related to MKDE client handles.  This function should be called
 *    first for an MKDE client before calling any other functions to get
 *    client information.  The cached information for the file handles will
 *    be freed when PvFreeMkdeClientsData() is called.
 *
 *    This function may also be called to refresh the cached client handle
 *    information.  DBADMIN only caches MKDE client handles data for one
 *    active MKDE client.  If information is requested with a different
 *    client ID, the cache is freed and data are retrieved for the new client
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Data for actvie clients retrieved by calling PvGetMkdeClientsData();
 *    Caller already has a valid active MKDE client ID;
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pClientId      [in]      Address of the PVCLIENTID structure to
 *                             identify the MKDE client
 *    pCount         [out]     Address of an usinged long to receive the
 *                             number of handles for the open file
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_DATA_UNAVAILABLE     Data related to MKDE clients available
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_FILE_NOT_OPEN        Specified file is not open currently
 *    P_E_FAIL                 Failed for any other reason
 */
BTI_API PvGetMkdeClientHandlesData(
   BTI_LONG       hConnection,
   PVCLIENTID*    pClientId,
   BTI_ULONG_PTR  pCount);


/*
 * Name:
 *    PvGetMkdeClientHandleInfo()
 *
 * Description:
 *    Query the information for a MKDE client handle associated with an
 *    active MKDE client
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Data for actvie MKDE clients retrieved by PvGetMkdeClientsData();
 *    Data for MKDE client handles retrieved by PvGetMkdeClientHandlesData();
 *    Caller already has a valid active MKDE client ID;
 *    Caller already has a valid handle sequence for the active MKDE client;
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pClientId      [in]      Address of the PVCLIENTID structure to
 *                             identify the MKDE client
 *    sequence       [in]      The sequence number (zero based) of the client
 *                             handle.  Must be within a valid range with
 *                             upper limit defined by the number of handles
 *                             obtained by PvGetMkdeClientHandlesData()
 *    pClientHdlInfo [out]     Address of a PVMKDECLIENTHDLINFO struct to
 *                             receive the information on the client handle
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_DATA_UNAVAILABLE     Data not available for MKDE client or handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_SEQUENCE     Sequence number is not valid
 *    P_E_INVALID_CLIENT       Invalid client ID
 *    P_E_FAIL                 Failed for any other reason
 *
 */
typedef struct tagPVMKDECLIENTHDLINFO
{
   BTI_BYTE openMode;
   BTI_BYTE locksFlag;
   BTI_BYTE waitFlag;
   BTI_WORD transState;  /* 0 == none, 19 == excl, 1019 = concurr */
   BTI_CHAR fileName[P_MAX_PATH_SIZE];
} PVMKDECLIENTHDLINFO;

BTI_API PvGetMkdeClientHandleInfo(
   BTI_LONG             hConnection,
   PVCLIENTID*          pClientId,
   BTI_ULONG            sequence,
   PVMKDECLIENTHDLINFO* pClientHdlInfo);


/*
 * Name:
 *    PvGetMkdeUsage()
 *
 * Description:
 *    Retrieves the resource usage information from the Microkernel database
 *    engine, including current, peak, and maximum settings for licenses,
 *    files, handles, transactions, clients, threads, and locks.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pMkdeUsage     [out]     Address of a PVMKDEUSAGE struct to receive
 *                             the MKDE resource usage information
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_FAIL                 Failed for any other reason
 *
 */
typedef struct tagPVMKDEUSAGE
{
   BTI_ULONG   currentLicensesInUse;
   BTI_ULONG   peakLicensesInUse;
   BTI_ULONG   maxLicenses;
   BTI_WORD    curFilesInUse;
   BTI_WORD    peakFilesInUse;
   BTI_WORD    maxFiles;
   BTI_WORD    curHandlesInUse;
   BTI_WORD    peakHandlesInUse;
   BTI_WORD    maxHandles;
   BTI_WORD    curTransInUse;
   BTI_WORD    peakTransInUse;
   BTI_WORD    maxTrans;
   BTI_WORD    curClients;
   BTI_WORD    peakClients;
   BTI_WORD    maxClients;
   BTI_WORD    curThreads;
   BTI_WORD    peakThreads;
   BTI_WORD    maxThreads;
   BTI_WORD    curLocksInUse;
   BTI_WORD    peakLocksInUse;
}  PVMKDEUSAGE;

BTI_API PvGetMkdeUsage(
   BTI_LONG       hConnection,
   PVMKDEUSAGE*   pMkdeUsage);

/*
 * Name:
 *    PvGetMkdeUsageEx()
 *
 * Description:
 *    Retrieves the resource usage information from the Microkernel database
 *    engine, including current, peak, and maximum settings for licenses,
 *    files, handles, transactions, clients, threads, and locks.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pMkdeUsage     [out]     Address of a PVMKDEUSAGE struct to receive
 *                             the MKDE resource usage information
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_FAIL                 Failed for any other reason
 *
 */
typedef struct tagPVMKDEUSAGEEX
{
   BTI_ULONG   currentLicensesInUse;
   BTI_ULONG   peakLicensesInUse;
   BTI_LONG    maxLicenses;
   BTI_ULONG   curFilesInUse;
   BTI_ULONG   peakFilesInUse;
   BTI_LONG    maxFiles;
   BTI_ULONG   curHandlesInUse;
   BTI_ULONG   peakHandlesInUse;
   BTI_LONG    maxHandles;
   BTI_ULONG   curTransInUse;
   BTI_ULONG   peakTransInUse;
   BTI_LONG    maxTrans;
   BTI_ULONG   curClients;
   BTI_ULONG   peakClients;
   BTI_LONG    maxClients;
   BTI_ULONG   curThreads;
   BTI_ULONG   peakThreads;
   BTI_LONG    maxThreads;
   BTI_ULONG   curLocksInUse;
   BTI_ULONG   peakLocksInUse;
}  PVMKDEUSAGEEX;

BTI_API PvGetMkdeUsageEx(
   BTI_LONG       hConnection,
   PVMKDEUSAGEEX*   pMkdeUsage);


/*
 * Name:
 *    PvGetMkdeCommStat()
 *
 * Description:
 *    Retrieve all the MKDE communication statistics data.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pCommStat      [out]     Address of a PVCOMMSTAT struct to receive
 *                             the MKDE communication statistics
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_COMPONENT_NOT_LOADED Communication component not loaded
 *    P_E_FAIL                 Failed for any other reason
 *
 */
typedef struct tagPVCOMMPROTOCOLSTAT
{
   BTI_ULONG   protocolId;
   BTI_ULONG   totalRequestsProcessed;
   BTI_ULONG   currentRemoteSessions;
   BTI_ULONG   peakRemoteSessions;
} PVCOMMPROTOCOLSTAT;

typedef struct tagPVCOMMSTAT
{
   BTI_ULONG            totalEngineRequestsProcessed;
   BTI_ULONG            currentQueuedRequests;
   BTI_ULONG            peakQueuedRequests;
   BTI_ULONG            maxQueuedRequests;
   BTI_ULONG            currentRemoteSessions;
   BTI_ULONG            peakRemoteSessions;
   BTI_ULONG            maxRemoteSessions;
   BTI_ULONG            currentActiveThreads;
   BTI_ULONG            peakActiveThreads;
   BTI_ULONG            maxActiveThreads;
   BTI_ULONG            numProtocols;
   PVCOMMPROTOCOLSTAT   protocolStat[P_MAX_COMM_PROTOCOLS];
} PVCOMMSTAT;

BTI_API PvGetMkdeCommStat(
   BTI_LONG       hConnection,
   PVCOMMSTAT*    pCommStat);



/*
 * Name:
 *    PvGetMkdeCommStatEx()
 *
 * Description:
 *    Retrieve all the MKDE communication statistics data.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pCommStat      [out]     Address of a PVCOMMSTATEX struct to receive
 *                             the MKDE communication statistics
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_COMPONENT_NOT_LOADED Communication component not loaded
 *    P_E_FAIL                 Failed for any other reason
 *
 */

typedef struct tagPVCOMMSTATEX
{
   BTI_ULONG            totalEngineRequestsProcessed;
   BTI_ULONG            currentQueuedRequests;
   BTI_ULONG            peakQueuedRequests;
   BTI_ULONG            maxQueuedRequests;
   BTI_ULONG            currentRemoteSessions;
   BTI_ULONG            peakRemoteSessions;
   BTI_ULONG            maxRemoteSessions;
   BTI_ULONG            currentActiveThreads;
   BTI_ULONG            peakActiveThreads;
   BTI_ULONG            maxActiveThreads;
   BTI_ULONG            numProtocols;
   BTI_ULONG            totalTimeouts;
   BTI_ULONG            totalRecoveries;
   PVCOMMPROTOCOLSTAT   protocolStat[P_MAX_COMM_PROTOCOLS];
} PVCOMMSTATEX;

BTI_API PvGetMkdeCommStatEx(
   BTI_LONG       hConnection,
   PVCOMMSTATEX*    pCommStat);



/*
 * Name:
 *    PvGetMkdeVersion()
 *
 * Description:
 *    Retrieve MKDE version info.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pVersion      [out]     Address of a PVVERSION struct to receive
 *                             the MKDE version info.
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_COMPONENT_NOT_LOADED Communication component not loaded
 *    P_E_FAIL                 Failed for any other reason
 *
 */

#define TARGET_SIZE 5

typedef struct tagPVVERSION
{
   BTI_LONG major;                  /* Major version  */
   BTI_LONG minor;                  /* Minor version  */
   BTI_LONG build;                  /* build number */
   BTI_CHAR target[TARGET_SIZE];    /* OS target      */
} PVVERSION;

BTI_API PvGetMkdeVersion(
   BTI_LONG       hConnection,
   PVVERSION*    pVersion);



/*
 * Name:
 *    PvGetSQLConnectionsData()
 *
 * Description:
 *    Retrieve all the information related to connections with the SQL Connection
 *    Manager. The information will be cached by DBADMIN for subsequent calls related to
 *    SQL connections.  This function should be called first before calling any
 *    other functions to get SQL connection information.  The caller should call
 *    PvFreeSQLConnectionsData() to free the cached information when it is
 *    no longer needed.
 *
 *    This function may also be called to refresh the cached information.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pCount         [out]     Address of an usinged long to receive the
 *                             number of SQL connections
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_FAIL                 Failed for any other reason
 */
BTI_API PvGetSQLConnectionsData(
   BTI_LONG       hConnection,
   BTI_ULONG_PTR  pCount);


/*
 * Name:
 *    PvFreeSQLConnectionsData()
 *
 * Description:
 *    Free the cached information related to SQL connections.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Data for open files retrieved by calling PvGetSQLConnectionsData();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_DATA_UNAVAILABLE     Data related to open files not available
 *    P_E_FAIL                 Failed for any other reason
 */
BTI_API PvFreeSQLConnectionsData(
   BTI_LONG hConnection);

/*
 * Name:
 *    PvGetSQLConnectionInfo()
 *
 * Description:
 *    Query the information for a SQL connection
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Data for SQL connections retrieved by calling PvGetSQLConnectionsData();
 *    Caller already has a valid SQL connection sequence;
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    sequence       [in]      The sequence number (zero based) of the SQL
 *                             connection.  Must be within a valid range with
 *                             upper limit defined by the number of SQL
 *                             connections obtained by PvGetSQLConnectionsData()
 *    pSQLConnInfo   [out]     Address of a PVSQLCONNINFO struct to receive
 *                             the information on the SQL connection
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_DATA_UNAVAILABLE     Data not available for the SQL connection
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_SEQUENCE     Sequence number is not valid
 *    P_E_CONN_NOT_OPEN        Specified SQL connection no longer exists
 *    P_E_FAIL                 Failed for any other reason
 *
 */

#define MAXLEN_HOSTNAME      64
#define SQL_MAX_DSN_LENGTH   32
#define MAXLEN_LAM_APP_DESC  255
#define MAXLEN_LAM_IP        40
#define MAXLEN_LAM_USER_NAME 64

typedef struct tagPVSQLCONNINFO
{
    BTI_CHAR    szHostName[MAXLEN_HOSTNAME+1];
    BTI_CHAR    szDSN[SQL_MAX_DSN_LENGTH+1];
    BTI_CHAR    szAppDesc[MAXLEN_LAM_APP_DESC+1];
    BTI_CHAR    szIP[MAXLEN_LAM_IP+1];
    BTI_CHAR    szUserName[MAXLEN_LAM_USER_NAME+1];
    BTI_WORD    ui16Status;     /* true - active; false - idle */
    BTI_ULONG   ui32CurStatusTime;
    BTI_ULONG   ui32ConnectTime;
    BTI_ULONG   ui32ProcessId;
    BTI_ULONG   ui32ThreadId;
} PVSQLCONNINFO;

/* Note the above structure maps to a LAM_USER structure */

BTI_API PvGetSQLConnectionInfo(
   BTI_LONG          hConnection,
   BTI_ULONG         sequence,
   PVSQLCONNINFO*    pSQLConnInfo);




/*
 * Name:
 *    PvDisconnectSQLConnection()
 *
 * Description:
 *    Disconnect an active SQL Connection
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Data for active SQL connections retrieved by calling PvGetSQLConnectionsData();
 *    Caller already has a valid processID and threadID for a SQL connection
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pSQLConnId     [in]      Address of the PVSQLCONNID structure to
 *                             identify the SQL connection
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_DATA_UNAVAILABLE     Data related to SQL connections not available
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_CLIENT       Invalid connection ID
 *    P_E_FAIL                 Failed for any other reason
 */

typedef struct tagPVSQLCONNID
{
    BTI_ULONG   ui32ProcessId;
    BTI_ULONG   ui32ThreadId;
} PVSQLCONNID;

BTI_API PvDisconnectSQLConnection(
   BTI_LONG          hConnection,
   PVSQLCONNID*       pSQLConnId);

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#define _MONITOR_H_INCLUDED
#endif /* _MONITOR_H_INCLUDED */


