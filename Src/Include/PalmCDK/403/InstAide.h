/*****************************************************************************
 *
 * Copyright (c) 2000 Palm Inc. or its subsidiaries.  
 * All rights reserved.
 *
 ****************************************************************************/
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

int WINAPI PltGetUserCount(void);
int WINAPI PltGetUser(unsigned int iIndex, TCHAR *pUserBuffer, short *psUserBufSize);
int WINAPI PltInstallFile(TCHAR *pUser, TCHAR *pFileSpec);
int WINAPI PltGetFileCount(TCHAR *pUser, TCHAR *pExtension);
int WINAPI PltGetFileName(TCHAR *pUser, unsigned int iIndex, TCHAR *pExtension, TCHAR *pFileName, short *psFileBufSize);
int WINAPI PltGetFileInfo(TCHAR *pUser, unsigned int iIndex, TCHAR *pExtension, TCHAR *pFileName, short *psFileBufSize, DWORD *pdwFileSize);
int WINAPI PltRemoveInstallFile(TCHAR *pUser, TCHAR *pFileName);
int WINAPI PltGetPath(unsigned short sPathType, TCHAR *pPathBuffer, short *psPathBufSize);
int WINAPI PltSetPath(unsigned short sPathType, TCHAR *pPathBuffer);

int WINAPI PltGetInstallFileFilter( TCHAR *pFilter, int *piBufferSize);

int WINAPI PltGetFileTypesCount( void );
int WINAPI PltGetFileTypeExtension(int iIndex, TCHAR *pBuffer, int *piBufferSize);

// HotSync Functions
int WINAPI PltGetInstallConduitCount(void);
int WINAPI PltGetInstallConduitInfo(unsigned int iIndex, FileInstallType *pBuf, int *piBufferSize);
int WINAPI PltIsInstallMaskSet(TCHAR *pUser, DWORD dwMask);
int WINAPI PltResetInstallMask(TCHAR *pUser, DWORD dwMask);
int WINAPI PltGetInstallCreatorInfo(DWORD dwCreatorID, FileInstallType *pBuf, int *piBufferSize);
int WINAPI PltSetInstallRegistry();
int WINAPI PltGetRegistryPath(unsigned int iIndex, TCHAR *pBuffer, int *piBufferSize);  // for Hotsync verbose logging only !!

// functions for Installer
int WINAPI PltGetUserDirectory(TCHAR *pUser, TCHAR *pBuffer, int *piBufferSize);
int WINAPI PltIsUserProfile(TCHAR *pUser);


int WINAPI PlmGetUserNameFromID(DWORD dwID, TCHAR *pUserBuffer, short *psUserBufSize);
int WINAPI PlmGetUserIDFromName( TCHAR *pUser, DWORD *pdwID);

// version ????
int WINAPI PltInstallFileToDest(TCHAR *pUser, TCHAR *pFileSpec, TCHAR *pDestName);
int WINAPI PltGetInstallFileFilterForUser( TCHAR *pUser, TCHAR *pFilter, int *piBufferSize);

// Slot Install functions
int WINAPI PlmSlotGetFileCount (DWORD dwUserId, DWORD dwSlotId, long *plFileCount);
int WINAPI PlmSlotGetFileInfo (DWORD dwUserId, DWORD dwSlotId, unsigned int iIndex,
								TCHAR *pszFileName, long *plFileBufSize, DWORD *pdwFileSize);
int WINAPI PlmSlotInstallFile (DWORD dwUserId, DWORD dwSlotId, const TCHAR *pszFileName);
int WINAPI PlmSlotRemoveInstallFile (DWORD dwUserId, DWORD dwSlotId, const TCHAR *pszFileName);
int WINAPI PlmSlotMoveInstallFile (DWORD dwUserId, DWORD dwSlotIdFrom, 
								   const TCHAR *pszFileName, DWORD dwSlotIdTo);
int WINAPI PlmMoveInstallFileToSlot (DWORD dwUserId, const TCHAR *pszFileName, 
									 DWORD dwSlotIdTo);
int WINAPI PlmMoveInstallFileToHandheld (DWORD dwUserId, DWORD dwSlotIdFrom, 
										 const TCHAR *pszFileName);

				   

// Path defines
#define PILOT_PATH_HOME         1
#define PILOT_PATH_HOTSYNC      2
#define PILOT_PATH_TUTORIAL     3
#define PILOT_PATH_MAX          PILOT_PATH_TUTORIAL

// Error codes
#define ERR_PILOT_NO_USERS                  -500
#define ERR_PILOT_INVALID_USER_INDEX        -501
#define ERR_PILOT_BUFSIZE_TOO_SMALL         -502
#define ERR_PILOT_NO_USERSDAT_FILE          -503
#define ERR_PILOT_COPY_FAILED               -504
#define ERR_IA_INVALID_USER                 -505
#define ERR_PILOT_INVALID_USER              ERR_IA_INVALID_USER
#define ERR_PILOT_INVALID_FILENAME          -506
#define ERR_PILOT_INVALID_FILE_INDEX        -507
#define ERR_PILOT_USERSDAT_ALREADY_EXISTS   -508
#define ERR_PILOT_INVALID_PATH_TYPE         -509
#define ERR_PILOT_INVALID_REGISTRY          -510
#define ERR_PILOT_INVALID_PATH              -511
#define ERR_PILOT_FILE_ALREADY_EXISTS       -512
#define ERR_PILOT_INVALID_SOURCE_FILE       -513
#define ERR_PILOT_INVALID_INDEX             -514
#define ERR_PILOT_INVALID_FILE_TYPE         -515
#define ERR_PILOT_INVALID_BUFFER            -516
#define ERR_PILOT_INVALID_CREATORID         -517

#define ERR_PALM_NO_CORE_PATH               -518
#define ERR_PALM_UNABLE_TO_CREATE_NEW_FILE  -519
#define ERR_PALM_SEMAPHORE_ACCESS           -520
#define ERR_PALM_OTHER_USERSDAT_ACCESS_PROBLEM -521

#define ERR_IA_INVALID_USER_ID              -522

#define ERR_PALM_FILE_DELETE_FAILED         -523
#define	ERR_PALM_FILE_MOVE_FAILED			-524

#define ERR_USER_MANAGER_BASE       -550
#define ERR_PILOT_NO_DIRECTORY      ERR_USER_MANAGER_BASE - 1
#define ERR_PILOT_NO_USER_FILE      ERR_USER_MANAGER_BASE - 2
#define ERR_PILOT_NOT_FOUND         ERR_USER_MANAGER_BASE - 3
#define ERR_PILOT_BAD_FILENAME      ERR_USER_MANAGER_BASE - 4
#define ERR_PILOT_USER_ACCESS       ERR_USER_MANAGER_BASE - 5

#define ERR_IA_INVALID_FILE_TYPE  -701

