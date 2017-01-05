/******************************************************************************
 *
 * Copyright (c) 1998-2004 PalmSource, Inc. All rights reserved.
 *
 * File: condmgr.h
 *
 * Release: 
 *
 *****************************************************************************/

/*****************************************************************
 *
 * Conduit Manager API
 *
 ****************************************************************/
#ifndef __CONDUIT_MGR_H_
#define __CONDUIT_MGR_H_


#define ERR_CONDUIT_MGR					-1000
#define ERR_INDEX_OUT_OF_RANGE			(ERR_CONDUIT_MGR - 1)
#define ERR_UNABLE_TO_DELETE			(ERR_CONDUIT_MGR - 2)
#define ERR_NO_CONDUIT					(ERR_CONDUIT_MGR - 3)
#define ERR_NO_MEMORY					(ERR_CONDUIT_MGR - 4)
#define ERR_CREATORID_ALREADY_IN_USE	(ERR_CONDUIT_MGR - 5)
#define ERR_REGISTRY_ACCESS				(ERR_CONDUIT_MGR - 6)
#define ERR_UNABLE_TO_CREATE_CONDUIT	(ERR_CONDUIT_MGR - 7)
#define ERR_UNABLE_TO_SET_CONDUIT_VALUE	(ERR_CONDUIT_MGR - 8)
#define ERR_INVALID_HANDLE				(ERR_CONDUIT_MGR - 9)
#define ERR_BUFFER_TOO_SMALL			(ERR_CONDUIT_MGR - 10)
#define ERR_VALUE_NOT_FOUND				(ERR_CONDUIT_MGR - 11)
#define ERR_INVALID_CREATOR_ID			(ERR_CONDUIT_MGR - 12)
#define ERR_INVALID_POINTER				(ERR_CONDUIT_MGR - 13)
#define ERR_UNABLE_TO_INSTALL_OLD		(ERR_CONDUIT_MGR - 14)
#define ERR_INVALID_CONDUIT_TYPE		(ERR_CONDUIT_MGR - 15)
#define ERR_INVALID_COM_PORT_TYPE		(ERR_CONDUIT_MGR - 16)
#define ERR_NO_LONGER_SUPPORTED			(ERR_CONDUIT_MGR - 17)

// additions for NotifierManager and InstallConduit manager
#define ERR_INVALID_PATH				(ERR_CONDUIT_MGR - 18)
#define ERR_ALREADY_INSTALLED			(ERR_CONDUIT_MGR - 19)
#define ERR_STORAGE_ACCESS				(ERR_CONDUIT_MGR - 20)
#define ERR_NOTIFIER_NOT_FOUND			(ERR_CONDUIT_MGR - 21)
#define ERR_INSTALL_ID_ALREADY_IN_USE	(ERR_CONDUIT_MGR - 22)
#define ERR_INVALID_INSTALL_ID			(ERR_CONDUIT_MGR - 23)

// additions for system conduits
#define ERR_INSUFFICIENT_PRIVILEGES		(ERR_CONDUIT_MGR - 24)

// additions for folder conduits
#define ERR_FOLDER_NOT_FOUND			(ERR_CONDUIT_MGR - 25)
#define ERR_CONDUIT_READ_ONLY			(ERR_CONDUIT_MGR - 26)
#define ERR_AMBIGUOUS_CREATORID			(ERR_CONDUIT_MGR - 27)
#define ERR_COULD_NOT_CREATE_DIRECTORY	(ERR_CONDUIT_MGR - 28)

// more addations for InstallConduit Manager
#define ERR_CONFLICTING_EXTENSION		(ERR_CONDUIT_MGR - 29)

#define CONDUIT_COMPONENT           0
#define CONDUIT_APPLICATION         1
#define CONDUIT_CONDUITS            10

#define DIRECT_COM_PORT             0
#define MODEM_COM_PORT              1
#define MAX_COM_PORT                MODEM_COM_PORT

#define CM_CREATOR_ID_SIZE 8		// Rounded up from 4+1

typedef struct {
    char            szCreatorID[CM_CREATOR_ID_SIZE]; // only need 4 + 1 string terminator, by lets
    int             iReserved;
} CM_CREATORLIST_ITEM_TYPE;

typedef CM_CREATORLIST_ITEM_TYPE *CM_CREATORLIST_TYPE;
typedef struct {
    int             iStructureVersion;
    int             iStructureSize;
    int             iType; // Types CONDUIT_X
    char            szCreatorID[CM_CREATOR_ID_SIZE]; // only need 4 + 1 string terminator, by lets
                                    // make it even.
    DWORD           dwPriority;
    int             iConduitNameOffset;
    int             iDirectoryOffset;
    int             iFileOffset;
    int             iRemoteDBOffset;
    int             iUsernameOffset;
    int             iTitleOffset;
    int             iInfoOffset;
} CmConduitType;

typedef struct {
    DWORD           dwCreatorID;
	DWORD           dwPriority;
    TCHAR           szConduitPath[255];
    TCHAR           szLocalDirectory[255];
    TCHAR           szLocalFile[255];
    TCHAR           szRemoteDB[32];
    TCHAR           szTitle[255];
} CmConduitType2;

typedef struct {
  BOOL bDiscoverableViaFolder;
  BOOL bDiscoverableViaRegistry;
  BOOL bLoadable;
} CmDiscoveryInfoType;

//InstallConduit structure
#ifndef FILEINSTALLTYPE_DEF
#define FILEINSTALLTYPE_DEF

typedef struct
{
	TCHAR		szDir[ 64 ];
	TCHAR		szExt[ 256];
	DWORD		dwMask;
	TCHAR		szModule[ 256 ];
    DWORD       dwCreatorID;
	TCHAR		szName[ 256 ];
} FileInstallType;

#define FILEINSTALLTYPE_SIZE sizeof(FileInstallType)
#endif

#define CM_MIN_CONDUITTYPE_SIZE            sizeof(CmConduitType)
#define CM_CONDUIT_BUFFER_OFFSET           sizeof(CmConduitType) 

#define CONDUIT_VERSION                     100

//	API functions
#define CM_INITIAL_LIB_VERSION 0x0001
#define CM_UPDATE_1            0x0002
#define CM_UPDATE_2            0x0003

WORD WINAPI CmGetLibVersion();

//	Utilities for manipulating Creator ID's
int WINAPI CmConvertCreatorIDToString(DWORD dwID, TCHAR * pString, int *piSize);
int WINAPI CmConvertStringToCreatorID(const TCHAR * pString, DWORD *pdwID);

//	Utility for determining admin rights
//  Usefull for checking if setting system settings will work.
BOOL WINAPI CmIsCurrentUserAdmin();

//	Functions for reading current conduit configuration information
//  user conduits
int WINAPI CmGetConduitCount(void);
int WINAPI CmGetCreatorIDList(CM_CREATORLIST_TYPE pCreatorList, int *piSize);
int WINAPI CmGetConduitCreatorID(int iIndex, char *pCreatorID, int *piSize);
int WINAPI CmGetConduitByCreator(const char *pCreatorID, HANDLE *hStruct);
int WINAPI CmGetConduitByIndex(int iIndex, CmConduitType2& sConduitInfo);
int WINAPI CmGetDiscoveryInfoByIndex(int iIndex, CmDiscoveryInfoType& sDiscoveryInfo);
//  system conduits 
int WINAPI CmGetSystemConduitCount(void);
int WINAPI CmGetSystemCreatorIDList(CM_CREATORLIST_TYPE pCreatorList, int *piSize);
int WINAPI CmGetSystemConduitCreatorID(int iIndex, char *pCreatorID, int *piSize);
int WINAPI CmGetSystemConduitByCreator(const char *pCreatorID, CmConduitType2& sConduitInfo);
int WINAPI CmGetSystemConduitByIndex(int iIndex, CmConduitType2& sConduitInfo);
int WINAPI CmGetSystemDiscoveryInfoByIndex(int iIndex, CmDiscoveryInfoType& sDiscoveryInfo);
	
//	Functions for installing a conduit
//  user conduits
int WINAPI CmInstallConduit(HANDLE hStruct);
int WINAPI CmInstallCreator(const char *pCreator, int iType);
int WINAPI CmInstallConduitByStruct(CmConduitType2& sConduitInfo);
//  system conduits
int WINAPI CmInstallSystemCreator(const char *pCreator, int iType);
int WINAPI CmInstallSystemConduitByStruct(const CmConduitType2& sConduitInfo);

//	Functions for removing a conduit
//  user conduits
int WINAPI CmRemoveConduitByIndex(int iIndex);
int WINAPI CmRemoveConduitByCreatorID(const char *pCreatorID);
//  system conduits
int WINAPI CmRemoveSystemConduitByIndex(int iIndex);
int WINAPI CmRemoveSystemConduitByCreatorID(const char *pCreatorID);

//
// Creator ID based functions for accessing individual data items.
// user conduits
int WINAPI CmSetCreatorPriority(     const char *pCreatorID, DWORD dwPriority);
int WINAPI CmGetCreatorPriority(     const char *pCreatorID, DWORD * pdwPriority);
int WINAPI CmSetCreatorName(         const char *pCreatorID, const TCHAR * pConduitName);
int WINAPI CmGetCreatorName(         const char *pCreatorID, TCHAR * pConduitName, int *piSize);
int WINAPI CmSetCreatorDirectory(    const char *pCreatorID, const TCHAR * pDirectory);
int WINAPI CmGetCreatorDirectory(    const char *pCreatorID, TCHAR * pDirectory, int *piSize);
int WINAPI CmSetCreatorRemote(       const char *pCreatorID, const TCHAR * pRemoteDB);
int WINAPI CmGetCreatorRemote(       const char *pCreatorID, TCHAR * pRemoteDB, int *piSize);
int WINAPI CmSetCreatorTitle(        const char *pCreatorID, const TCHAR * pTitle);
int WINAPI CmGetCreatorTitle(        const char *pCreatorID, TCHAR * pTitle, int *piSize);
int WINAPI CmSetCreatorFile(         const char *pCreatorID, const TCHAR * pFile);
int WINAPI CmGetCreatorFile(         const char *pCreatorID, TCHAR * pFile, int *piSize);
// system conduits
int WINAPI CmSetSystemCreatorPriority(const char *pCreatorID, DWORD dwPriority);
int WINAPI CmGetSystemCreatorPriority(const char *pCreatorID, DWORD * pdwPriority);
int WINAPI CmSetSystemCreatorName(	 const char *pCreatorID, const TCHAR * pConduitName);
int WINAPI CmGetSystemCreatorName(   const char *pCreatorID, TCHAR * pConduitName, int *piSize);
int WINAPI CmSetSystemCreatorDirectory(const char *pCreatorID, const TCHAR * pDirectory);
int WINAPI CmGetSystemCreatorDirectory(const char *pCreatorID, TCHAR * pDirectory, int *piSize);
int WINAPI CmSetSystemCreatorRemote( const char *pCreatorID, const TCHAR * pRemoteDB);
int WINAPI CmGetSystemCreatorRemote( const char *pCreatorID, TCHAR * pRemoteDB, int *piSize);
int WINAPI CmSetSystemCreatorTitle(  const char *pCreatorID, const TCHAR * pTitle);
int WINAPI CmGetSystemCreatorTitle(  const char *pCreatorID, TCHAR * pTitle, int *piSize);
int WINAPI CmSetSystemCreatorFile(   const char *pCreatorID, const TCHAR * pFile);
int WINAPI CmGetSystemCreatorFile(   const char *pCreatorID, TCHAR * pFile, int *piSize);

// Deprecated calls.  These settings are not used.
int WINAPI CmSetCreatorUser(         const char *pCreatorID, const TCHAR * pUsername);
int WINAPI CmGetCreatorUser(         const char *pCreatorID, TCHAR * pUsername, int *piSize);
int WINAPI CmSetCreatorIntegrate(    const char *pCreatorID, DWORD dwIntegrate);
int WINAPI CmGetCreatorIntegrate(    const char *pCreatorID, DWORD * pdwIntegrate);
int WINAPI CmSetCreatorModule(       const char *pCreatorID, const TCHAR * pModule);
int WINAPI CmGetCreatorModule(       const char *pCreatorID, TCHAR * pModule, int *piSize);
int WINAPI CmSetCreatorArgument(     const char *pCreatorID, const TCHAR * pArgument);
int WINAPI CmGetCreatorArgument(     const char *pCreatorID, TCHAR * pArgument, int *piSize);
int WINAPI CmSetCreatorInfo(         const char *pCreatorID, const TCHAR * pInfo);
int WINAPI CmGetCreatorInfo(         const char *pCreatorID, TCHAR * pInfo, int *piSize);
int WINAPI CmGetCreatorType(const char *pCreator);	// Generally for internal use.

// Deprecated calls.  For internal use only.
int WINAPI CmSetPCIdentifier(DWORD dwPCID);
int WINAPI CmSetCorePath(const TCHAR *pPath);
int WINAPI CmSetHotSyncExecPath(const TCHAR *pPath);
int WINAPI CmRestoreHotSyncSettings(BOOL bToDefaults);


// Deprecated calls.  Use the Notifier Manager (Nm) calls
// to access user/system notifiers
int WINAPI CmSetNotifierDll(int iIndex, const TCHAR *pNotifier);
int WINAPI CmGetNotifierDll(int iIndex, TCHAR *pNotifier, int *piSize);

//
//	Functions for accessing other HotSync configuration info.
//
// Port access
// user setting only
int WINAPI CmSetComPort(int iType, const TCHAR *pPort);
int WINAPI CmGetComPort(int iType, TCHAR *pPort, int *piSize);

// Backup conduit
// user setting 
int WINAPI CmSetBackupConduit(const TCHAR *pConduit);
int WINAPI CmGetBackupConduit(TCHAR *pConduit, int *piSize);
// system setting
int WINAPI CmSetSystemBackupConduit(const TCHAR *pConduit);
int WINAPI CmGetSystemBackupConduit(TCHAR *pConduit, int *piSize);

// PC ident
// user setting only
int WINAPI CmGetPCIdentifier(DWORD *pdwPCID);

// Core path
// user setting only
int WINAPI CmGetCorePath(TCHAR *pPath, int *piSize);

// HotSync Path
// user setting (deprecated, but still here for legacy conduit installers)
int WINAPI CmGetHotSyncExecPath(TCHAR *pPath, int *piSize);
// system setting
int WINAPI CmGetSystemHotSyncExecPath(TCHAR *pPath, int *piSize);

// user values 
int WINAPI CmSetCreatorValueDword(const char *pCreatorID, TCHAR * pValue, DWORD dwValue);
int WINAPI CmGetCreatorValueDword(const char *pCreatorID, 
                                  TCHAR * pValue, 
                                  DWORD *dwValue,
                                  DWORD dwDefault);
int WINAPI CmSetCreatorValueString(const char *pCreatorID, TCHAR * pValue, TCHAR * pString);
int WINAPI CmGetCreatorValueString(const char *pCreatorID, 
                                   TCHAR * pValue, 
                                   TCHAR * pString, 
                                   int *piSize,
                                   TCHAR *pDefault);
// system values
int WINAPI CmSetSystemCreatorValueDword(const char *pCreatorID, TCHAR * pValue, DWORD dwValue);
int WINAPI CmGetSystemCreatorValueDword(const char *pCreatorID, 
                                  TCHAR * pValue, 
                                  DWORD *dwValue,
                                  DWORD dwDefault);
int WINAPI CmSetSystemCreatorValueString(const char *pCreatorID, TCHAR * pValue, TCHAR * pString);
int WINAPI CmGetSystemCreatorValueString(const char *pCreatorID, 
                                   TCHAR * pValue, 
                                   TCHAR * pString, 
                                   int *piSize,
                                   TCHAR *pDefault);

//
// Index based functions for accessing individual data items.
//

// general Palm information storage calls
// user settings
long WINAPI PiSetValueDword(const char *pFolder, const char *pKey, DWORD dwValue );
long WINAPI PiGetValueDword(const char *pFolder, const char *pKey, DWORD *pdwValue, DWORD dwDefault );
long WINAPI PiSetValueString(const char *pFolder, const char *pKey, const char *pValue);
long WINAPI PiGetValueString(const char *pFolder, const char *pKey, char *pValue, int *piLen, const char *pDefault);
long WINAPI PiSetHotsyncValueDword(const char *pFolder, const char *pKey, DWORD dwValue);
long WINAPI PiGetHotsyncValueDword(const char *pFolder, const char *pKey, DWORD *pdwValue, DWORD dwDefault);
long WINAPI PiSetHotsyncValueString(const char *pFolder, const char *pKey, const char *pValue);
long WINAPI PiGetHotsyncValueString(const char *pFolder, const char *pKey, char *pValue, int *piLen, const char *pDefault);
// system settings
long WINAPI PiSetSystemValueDword(const char *pFolder, const char *pKey, DWORD dwValue );
long WINAPI PiGetSystemValueDword(const char *pFolder, const char *pKey, DWORD *pdwValue, DWORD dwDefault );
long WINAPI PiSetSystemValueString(const char *pFolder, const char *pKey, const char *pValue);
long WINAPI PiGetSystemValueString(const char *pFolder, const char *pKey, char *pValue, int *piLen, const char *pDefault);
long WINAPI PiSetSystemHotsyncValueDword(const char *pFolder, const char *pKey, DWORD dwValue);
long WINAPI PiGetSystemHotsyncValueDword(const char *pFolder, const char *pKey, DWORD *pdwValue, DWORD dwDefault);
long WINAPI PiSetSystemHotsyncValueString(const char *pFolder, const char *pKey, const char *pValue);
long WINAPI PiGetSystemHotsyncValueString(const char *pFolder, const char *pKey, char *pValue, int *piLen, const char *pDefault);



// Install Conduit API
// User Install Conduits
int  WINAPI ImRegister(const FileInstallType sConduitInfo);
int  WINAPI ImRegisterID(DWORD dwCreatorID);
int  WINAPI ImUnregisterID(DWORD dwCreatorID);
int  WINAPI ImSetDirectory(DWORD dwID, const TCHAR* pDirectory);
int  WINAPI ImSetExtension(DWORD dwID, const TCHAR* pExtension);
int  WINAPI ImSetMask(DWORD dwID, DWORD dwMask);
int  WINAPI ImSetModule(DWORD dwID, const TCHAR* pModule);
int  WINAPI ImSetName(DWORD dwID, const TCHAR* pConduitName);
int  WINAPI ImSetDWord(DWORD dwID, const TCHAR* pValue, DWORD dwValue);
int  WINAPI ImSetString(DWORD dwID, const TCHAR* pValue, TCHAR* pString);
int  WINAPI ImGetDirectory(DWORD dwID, TCHAR* pDirectory, int *piSize);
int  WINAPI ImGetExtension(DWORD dwID, TCHAR* pExtension, int* piSize);
int  WINAPI ImGetMask(DWORD dwID, DWORD* pdwMask);
int  WINAPI ImGetModule(DWORD dwID, TCHAR* pModule, int* piSize);
int  WINAPI ImGetName(DWORD dwID, TCHAR* pConduitName, int* piSize);
int  WINAPI ImGetDWord(DWORD dwID, const TCHAR* pValue, DWORD* pdwValue, DWORD dwDefault);
int  WINAPI ImGetString(DWORD dwID, const TCHAR* pValue, TCHAR *pString, int* piSize, const TCHAR* pDefault);
// System Install Conduits
int  WINAPI ImRegisterSystem(const FileInstallType sConduitInfo);
int  WINAPI ImRegisterSystemID(DWORD dwCreatorID);
int  WINAPI ImUnregisterSystemID(DWORD dwCreatorID);
int  WINAPI ImSetSystemDirectory(DWORD dwID, const TCHAR* pDirectory);
int  WINAPI ImSetSystemExtension(DWORD dwID, const TCHAR* pExtension);
int  WINAPI ImSetSystemMask(DWORD dwID, DWORD dwMask);
int  WINAPI ImSetSystemModule(DWORD dwID, const TCHAR* pModule);
int  WINAPI ImSetSystemName(DWORD dwID, const TCHAR* pConduitName);
int  WINAPI ImSetSystemDWord(DWORD dwID, const TCHAR* pValue, DWORD dwValue);
int  WINAPI ImSetSystemString(DWORD dwID, const TCHAR* pValue, TCHAR* pString);
int  WINAPI ImGetSystemDirectory(DWORD dwID, TCHAR* pDirectory, int *piSize);
int  WINAPI ImGetSystemExtension(DWORD dwID, TCHAR* pExtension, int* piSize);
int  WINAPI ImGetSystemMask(DWORD dwID, DWORD* pdwMask);
int  WINAPI ImGetSystemModule(DWORD dwID, TCHAR* pModule, int* piSize);
int  WINAPI ImGetSystemName(DWORD dwID, TCHAR* pConduitName, int* piSize);
int  WINAPI ImGetSystemDWord(DWORD dwID, const TCHAR* pValue, DWORD* pdwValue, DWORD dwDefault);
int  WINAPI ImGetSystemString(DWORD dwID, const TCHAR* pValue, TCHAR *pString, int* piSize, const TCHAR* pDefault);

// Install  Notifier Manager API
// User Notifiers
int  WINAPI NmRegister(const TCHAR* pNotifierPath);
int  WINAPI NmUnregister(const TCHAR* pNotifierPath);
int  WINAPI NmGetCount(DWORD* pdwCount);
int  WINAPI NmFind(const TCHAR *pNotifier);
int  WINAPI NmGetByIndex(int iIndex, TCHAR *pNotifier, int *piSize);
int  WINAPI NmRenameByIndex(int iIndex, const TCHAR *pNotifier);
// System Notifiers
int  WINAPI NmRegisterSystem(const TCHAR* pNotifierPath);
int  WINAPI NmUnregisterSystem(const TCHAR* pNotifierPath);
int  WINAPI NmGetSystemCount(DWORD* pdwCount);
int  WINAPI NmFindSystem(const TCHAR *pNotifier);
int  WINAPI NmGetSystemByIndex(int iIndex, TCHAR *pNotifier, int *piSize);
int  WINAPI NmRenameSystemByIndex(int iIndex, const TCHAR *pNotifier);

// Public APIs for geting conduit folders
// User Conduits
int WINAPI FmGetCurrentUserConduitFolder(TCHAR* pPath, int* piSize);
int WINAPI FmGetCurrentUserDisabledConduitFolder(TCHAR* pPath, int* piSize);
// System Conduits
int WINAPI FmGetSystemConduitFolder(TCHAR* pPath, int* piSize);
int WINAPI FmGetSystemDisabledConduitFolder(TCHAR* pPath, int* piSize);

// API for accessing folder based conduits
// User Conduits
int WINAPI FmGetCurrentUserConduitCount(void);
int WINAPI FmGetCurrentUserConduitByIndex(int iIndex, CmConduitType2& sConduitInfo);
int WINAPI FmDisableCurrentUserConduitByIndex(int iIndex);
int WINAPI FmDisableCurrentUserConduitByPath(TCHAR* pPath);
int WINAPI FmEnableCurrentUserConduitByPath(TCHAR* pPath);

// System Conduits
int WINAPI FmGetSystemConduitCount(void);
int WINAPI FmGetSystemConduitByIndex(int iIndex, CmConduitType2& sConduitInfo);
int WINAPI FmDisableSystemConduitByIndex(int iIndex);
int WINAPI FmDisableSystemConduitByPath(TCHAR* pPath);
int WINAPI FmEnableSystemConduitByPath(TCHAR* pPath);

#endif // __CONDUIT_MGR_H_
