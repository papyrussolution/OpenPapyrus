/*****************************************************************************
 *
 * Copyright (c) 1998-2000 Palm, Inc. or its subsidiaries.  
 * All rights reserved.
 *
 ****************************************************************************/

#ifndef _USER_DATA_HEADER_
#define _USER_DATA_HEADER_

enum UmUserSyncAction {	Synchronize = 0,
						PCToHH,
						HHToPC,
						DoNothing,
						Custom };

long WINAPI UmGetLibVersion (DWORD* pdwMajor, DWORD* pdwMinor);

short WINAPI UmGetUserCount(void);
long WINAPI UmGetUserID(short sIndex, DWORD *pdwUserID);
long WINAPI UmGetUserName(DWORD dwUserID, char *pUserNameBuffer, short *psUserNameBufSize);
long WINAPI UmGetUserDirectory(DWORD dwUserID, TCHAR *pUserDirBuffer, short *psUserDirBufSize);
long WINAPI UmGetUserPassword(DWORD dwUserID, TCHAR *pUserPasswordBuffer, short *psUserPasswordBufSize);

long WINAPI UmIsUserInstalled (DWORD dwUserID);
long WINAPI UmIsUserProfile(DWORD dwUserID);

long WINAPI UmSetUserName (DWORD dwUserID, const char *pUserName);
long WINAPI UmSetUserDirectory (DWORD dwUserID, const TCHAR *pUserDir);

long WINAPI UmSetUserInstall(DWORD dwUserID, BOOL bUserInstall);

long WINAPI UmSetUserPermSyncPreferences(DWORD dwUserID, DWORD dwCreatorID, long usaAction);
long WINAPI UmSetUserTempSyncPreferences(DWORD dwUserID, DWORD dwCreatorID, long usaAction);

long WINAPI UmGetUserPermSyncPreferences(DWORD dwUserID, DWORD dwCreatorID, long *pUsaAction);
long WINAPI UmGetUserTempSyncPreferences(DWORD dwUserID, DWORD dwCreatorID, long *pUsaAction);

long WINAPI UmRemoveUserTempSyncPreferences(DWORD dwUserID, DWORD dwCreatorID);
long WINAPI UmDeleteUserPermSyncPreferences(DWORD dwUserID, DWORD dwCreatorID);
long WINAPI UmDeleteUserTempSyncPreferences(DWORD dwUserID, DWORD dwCreatorID);
long WINAPI UmAddUser(const char *pUser, BOOL bProfileUser);
long WINAPI UmDeleteUser (DWORD dwUserID);

long WINAPI UmGetRootDirectory(TCHAR *pRootDirBuffer, short *psRootDirBufSize);

// install conduit user calls
long WINAPI UmIsInstallMaskSet(DWORD dwUserID, DWORD dwMask);
long WINAPI UmClearInstallMask(DWORD dwUserID, DWORD dwMask);
long WINAPI UmSetInstallMask(DWORD dwUserID, DWORD dwMask);

long APIENTRY UmGetString(DWORD dwUserID, const TCHAR* pszSection, const TCHAR* pszKey, TCHAR* pBuf, long* lSize, const TCHAR* pszDefault);
long APIENTRY UmSetString(DWORD dwUserID, const TCHAR* pszSection, const TCHAR* pszKey, const TCHAR* pszValue);
long APIENTRY UmGetInteger (DWORD dwUserID, const TCHAR* pszSection, const TCHAR* pszKey, long* pBuf, long lDefault);
long APIENTRY UmSetInteger (DWORD dwUserID, const TCHAR* pszSection, const TCHAR* pszKey, long lValue);
long APIENTRY UmDeleteKey (DWORD dwUserID, const TCHAR* pszSection, const TCHAR* pszKey);
long APIENTRY UmGetIDFromPath(const TCHAR* pszPath, DWORD* pdwUserID);
long APIENTRY UmGetIDFromName(const TCHAR* pszName, DWORD* pdwUserID);

// Expansion slot info calls
long APIENTRY UmSlotGetExpMgrVersion (DWORD dwUserId, DWORD *pdwExpMgrVersion);

long APIENTRY UmSlotGetSlotCount (DWORD dwUserId, WORD *pwNumSlots);
long APIENTRY UmSlotGetInfo (DWORD dwUserId, DWORD *pdwSlotIdList, WORD *pwNumEntries);
long APIENTRY UmSlotGetMediaType (DWORD dwUserId, DWORD dwSlotId, DWORD *pdwSlotMediaType);
long APIENTRY UmSlotGetDisplayName (DWORD dwUserId, DWORD dwSlotId, 
									TCHAR *pszSlotDisplayName, long *plSize);
long APIENTRY UmSlotGetInstallDirectory (DWORD dwUserId, DWORD dwSlotId, 
										 TCHAR *pszSlotInstallDir, long *plSize);

// Error codes
#define ERR_UM_NO_USERSDAT_FILE					-500
#define ERR_UM_NO_USERS							-501
#define ERR_UM_USERSDAT_ALREADY_EXISTS			-502 // not used
#define ERR_UM_INVALID_USER_INDEX				-503 // commented out
#define ERR_UM_BUFSIZE_TOO_SMALL				-504
#define ERR_UM_INVALID_USER						-505
#define ERR_UM_INVALID_REGISTRY					-506 // not used	
#define ERR_UM_INVALID_INDEX					-507 // not used
#define ERR_UM_INVALID_BUFFER					-508 // not used
#define ERR_UM_INVALID_USER_NAME				-509
#define ERR_UM_INVALID_USER_DIR					-510
#define ERR_UM_NO_DIRECTORY						-511 // not used
#define ERR_UM_NO_USER_FOUND					-512 // not used
#define ERR_UM_NOT_FOUND						-513
#define ERR_UM_BAD_FILENAME						-514 // not used
#define ERR_UM_USER_ACCESS						-515 // not used
#define ERR_UM_SAVE_ERR							-516
#define ERR_UM_BASE								-517 // don't understand
#define	ERR_UM_OTHER_USERSDAT_ACCESS_PROBLEM	-518
#define ERR_UM_SEMAPHORE_ACCESS					-519
#define ERR_UM_UNABLE_TO_CREATE_NEW_FILE		-520 // commented out
#define ERR_UM_NO_CORE_PATH						-521
#define ERR_UM_INVALID_POINTER					-522
#define ERR_UM_USER_ALREADY_EXISTS				-523
#define ERR_UM_USER_DIR_ALREADY_IN_USE			-524



#define	ERR_UM_CANNOT_WRITE_TO_STORE			-525
#define ERR_UM_SYNC_PATH_TOO_BIG				-526
#define ERR_UM_STRING_TOO_BIG					-527
#define ERR_UM_FUNCTION_NOT_SUPPORTED			-528
#define ERR_UM_DEV_CFG_DATA_NOT_AVAILABLE		-529

#endif	// _USER_DATA_HEADER_