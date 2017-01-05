/*****************************************************************************
 *
 * Copyright (c) 1998-1999 Palm Computing, Inc. or its subsidiaries.  
 * All rights reserved.
 *
 ****************************************************************************/
/////////////////////////////////////////////////////////////////////////////
//
//	 File:      hslog.h
//
//	 Module:    HSLOGxx(d).DLL
//
//
//	 Description:  Publicly consumable header file prototyping the 'C' API
//                 and the structures used for their parameters.
//                 When using the Microsoft compiler we asure packed 
//                 structures on single byte boundaries, with the pragma(1).
//
//
/////////////////////////////////////////////////////////////////////////////
//	 REVISION HISTORY:
//   mja  7/23/95  initial 
//
//
//
/////////////////////////////////////////////////////////////////////////////
#ifndef  __HSLOG_H__
#define  __HSLOG_H__

#ifdef _HSLOG
#define HSLOG_API __declspec(dllexport)
#else
#define HSLOG_API __declspec(dllimport)
#endif

enum LogError { slNoError,
				slBadStream,
				slDeleteFileFailed,
				slMoveFileFailed };

enum Activity { slText = -1,
                slDoubleModify,
                slDoubleModifyArchive,
                slReverseDelete,
                slTooManyCategories,
                slCategoryDeleted,
                slDateChanged,
                slCustomLabel,
				slChangeCatFailed,
				slRemoteReadFailed,
                slRemoteAddFailed,
                slRemotePurgeFailed,
                slRemoteChangeFailed,
                slRemoteDeleteFailed,
                slLocalAddFailed,
				slRecCountMismatch,
				slXMapFailed,
				slArchiveFailed,
				slLocalSaveFailed,
				slResetFlagsFailed,
                slSyncStarted,
                slSyncFinished,
                slSyncAborted,
				slWarning,
				slDoubleModifySubsc,
				slFileLinkCompleted,
				slFileLinkDeleted };

extern "C" {

HSLOG_API long    LogInit();
HSLOG_API long    LogUnInit();
HSLOG_API long    LogAddEntry(LPCTSTR pszEntry, Activity act, BOOL bTimeStamp);
HSLOG_API long    LogSaveLog(LPCTSTR pszFile);
HSLOG_API LPCTSTR LogGetWorkFileName();
HSLOG_API long    LogBuildRemoteLog(LPTSTR pszBuffer, DWORD dwLen);
HSLOG_API WORD    LogTestCounters();
HSLOG_API void	  LogCloseLog();
HSLOG_API long    LogAddFormattedEntry(Activity act, BOOL bTimeStamp, const char* dataString, ...);

}

#endif


