/*****************************************************************************
 *
 * Copyright (c) 1998-1999 Palm Computing, Inc. or its subsidiaries.  
 * All rights reserved.
 *
 ****************************************************************************/
//	 File:      condapi.h  
//
//	 Module:    
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
//	jayita	10/10/97	moved to separate header from syncmgr.h
//  jayita  4/15/98      moved error codes from basemon.h 
//
/////////////////////////////////////////////////////////////////////////////
#ifndef  __SYNCMGR_PUBLIC_CONDUIT_API__
#define  __SYNCMGR_PUBLIC_CONDUIT_API__ 

#ifdef _FILELNK
#include "subscribe.h"
#endif

#ifndef macintosh
#pragma pack(1)
#endif

#ifndef COND_API
#define COND_API __declspec(dllexport)
#endif

// conduit C API

enum ConduitInfoEnum { eConduitName=0, 
                       eMfcVersion, 
                       eDefaultAction,
                       eConduitInfoDoNotUse= 0xffffffff}; 
enum ConduitCfgEnum { eConfig1 = 0,
                      eConduitCfgDoNotUse= 0xffffffff};

#define MFC_VERSION_41      0x00000410
#define MFC_VERSION_50      0x00000500
#define MFC_VERSION_60      0x00000600
#define MFC_NOT_USED        0x10000000



typedef struct ConduitRequestInfoType {
    DWORD dwVersion;
    DWORD dwSize;
    DWORD dwCreatorId;
    DWORD dwUserId;
    TCHAR szUser[64];
} CONDUITREQUESTINFO;

#define CONDUITREQUESTINFO_VERSION_1 0x00000001
#define SZ_CONDUITREQUESTINFO   sizeof(CONDUITREQUESTINFO)

typedef struct CfgConduitInfoType {
    DWORD dwVersion;
    DWORD dwSize;
    DWORD dwCreatorId;
    DWORD dwUserId;
    TCHAR szUser[64];
    char  m_PathName[BIG_PATH];     
    eSyncTypes syncPermanent;
    eSyncTypes syncTemporary;
    eSyncTypes syncNew;
    eSyncPref  syncPref; 
} CFGCONDUITINFO;

#define CFGCONDUITINFO_VERSION_1 0x00000001
#define SZ_CFGCONDUITINFO   sizeof(CFGCONDUITINFO)
extern "C" {

typedef  long (*PROGRESSFN) (char*);

COND_API long OpenConduit(PROGRESSFN, CSyncProperties&);
typedef  long (*POPENCONDUIT) (PROGRESSFN, CSyncProperties&);

COND_API long ConfigureConduit(CSyncPreference&);
typedef  long (*PCONFIGURECONDUIT) (CSyncPreference&);

COND_API long GetConduitName(char*, WORD);
typedef  long (*PGETCONDUITNAME) (char*, WORD);

COND_API DWORD GetConduitVersion();
typedef  DWORD (*PGETCONDUITVERSION) ();

COND_API long GetConduitInfo(ConduitInfoEnum infoType, void *pInArgs, void *pOut, DWORD *pdwOutSize);
typedef  long (*PGETCONDUITINFO)(ConduitInfoEnum infoType, void *pInArgs, void *pOut, DWORD *pdwOutSize);

COND_API long CfgConduit( ConduitCfgEnum cfgType, void *pArgs, DWORD *pdwArgsSize);
typedef  long (*PCFGCONDUIT)( ConduitCfgEnum cfgType, void *pArgs, DWORD *pdwArgsSize);


#ifdef _FILELNK
//  for file link
COND_API long ConfigureSubscription(SubProperties*&);
typedef  long (*PCONFIGURESUBSCRIPTION) (SubProperties*&);

COND_API long SubscriptionSupported();
typedef  long (*PSUBSSUPPORT) ();

COND_API long UpdateTables(SubProperties*);
typedef  long (*PUPDATETABLES) (SubProperties*);

COND_API int ImportData(CSubInfo*& );
typedef  long (*PIMPORTDATA) (CSubInfo*&);
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////

//
//  Error codes for the Conduit DLL's range  0x5000 - 0x5FFF
//
#define CONDERR_NONE                       0x0000
#define CONDERR_FIRST                      0x1000
#define CONDERR_NO_REMOTE_CATEGORIES       CONDERR_FIRST + 1
#define CONDERR_NO_LOCAL_CATEGORIES        CONDERR_FIRST + 2
#define CONDERR_SAVE_REMOTE_CATEGORIES     CONDERR_FIRST + 3
#define CONDERR_BAD_REMOTE_TABLES          CONDERR_FIRST + 4
#define CONDERR_BAD_LOCAL_TABLES           CONDERR_FIRST + 5
#define CONDERR_BAD_LOCAL_BACKUP           CONDERR_FIRST + 6
#define CONDERR_ADD_LOCAL_RECORD           CONDERR_FIRST + 7
#define CONDERR_ADD_REMOTE_RECORD          CONDERR_FIRST + 8
#define CONDERR_CHANGE_REMOTE_RECORD       CONDERR_FIRST + 9
#define CONDERR_RAW_RECORD_ALLOCATE        CONDERR_FIRST + 0x0A
#define CONDERR_REMOTE_CHANGES_NOT_SENT    CONDERR_FIRST + 0x0B
#define CONDERR_LOCAL_MEMORY_ALLOC_FAILED  CONDERR_FIRST + 0x0C
#define CONDERR_CONVERT_TO_REMOTE_CATS     CONDERR_FIRST + 0x0D
#define CONDERR_CONVERT_TO_LOCAL_CATS      CONDERR_FIRST + 0x0E
#define CONDERR_CONVERT_TO_REMOTE_REC      CONDERR_FIRST + 0x0F
#define CONDERR_CONVERT_FROM_REMOTE_REC    CONDERR_FIRST + 0x10
#define CONDERR_REMOTE_RECS_NOT_PURGED     CONDERR_FIRST + 0x11
#define CONDERR_BAD_SYNC_TYPE              CONDERR_FIRST + 0x12
#define CONDERR_ABORT_DB_INSTALL           CONDERR_FIRST + 0x13
#define CONDERR_DATE_MOVED                 CONDERR_FIRST + 0x50
#define CONDERR_SUBSCRIBE_FAILED		   CONDERR_FIRST + 0x60

// Error codes added 01/23/98
#define CONDERR_UNSUPPORTED_CONDUITINFO_ENUM CONDERR_FIRST + 0x70
#define CONDERR_INVALID_PTR                 CONDERR_FIRST + 0x71
#define CONDERR_BUFFER_TOO_SMALL            CONDERR_FIRST + 0x72
#define CONDERR_INVALID_BUFFER_SIZE         CONDERR_FIRST + 0x73
#define CONDERR_INVALID_INARGS_PTR          CONDERR_FIRST + 0x74
#define CONDERR_INVALID_INARGS_STRUCT       CONDERR_FIRST + 0x75
#define CONDERR_CONDUIT_RESOURCE_FAILURE    CONDERR_FIRST + 0x76
#define CONDERR_INVALID_OUTSIZE_PTR         CONDERR_FIRST + 0x77
#define CONDERR_INVALID_ARGSSIZE_PTR        CONDERR_FIRST + 0x78
#define CONDERR_UNSUPPORTED_CFGCONDUIT_ENUM CONDERR_FIRST + 0x79
#define CONDERR_INVALID_ARGSSIZE            CONDERR_FIRST + 0x7A
#define CONDERR_UNSUPPORTED_STRUCT_VERSION  CONDERR_FIRST + 0x7B
#define CONDERR_NOCLIENTINFO_AVAILABLE		CONDERR_FIRST + 0x7C
}

#ifndef macintosh
#pragma pack()
#endif

#endif


