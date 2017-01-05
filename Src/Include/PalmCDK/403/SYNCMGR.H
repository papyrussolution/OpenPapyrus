/*****************************************************************************
 *
 * Copyright (c) 1998-1999 Palm Computing, Inc. or its subsidiaries.  
 * All rights reserved.
 *
 ****************************************************************************/

/////////////////////////////////////////////////////////////////////////////
//	 File:      syncmgr.h  
//
//	 Module:    SYNCMGR.DLL 
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
// mja	7/23/95  initial 
//	vmk	8/12/96	renamed the unused "readOnlyDB" field in CDBList to m_miscFlags
//	jim 8/16/96 removed CSyncLog from CSyncProperties
// vmk	10/21/96	added SYNC API versioning scheme and SyncGetAPIVersion() function.
//	vmk	1/29/97	accurately documented SYNCERR_... error codes
//  jayita  10/10/97  moved conduit API to CondAPI.h to remove 
//                             dependency on data subscription(aka FileLink) for Mac developers
//	dlw	5/24/99 	Added FSFileSpec to CSyncPreference for Mac version of generic conduit
//
/////////////////////////////////////////////////////////////////////////////
#ifndef  __SYNCMGR_PUBLIC_API__
#define  __SYNCMGR_PUBLIC_API__ 

// Maximum length of Viewer's database name,
// *including* the null-terminator
#define SYNC_DB_NAMELEN    (32)
#define DB_NAMELEN			 SYNC_DB_NAMELEN


// Maximum size of Viewer's HotSync log
#define SYNC_MAX_HH_LOG_SIZE		(2*1024)


#define BIG_PATH     256

#ifndef macintosh
	#pragma pack(1)
#else
	#pragma options align= reset
#endif

//
//  Common sync properties structure populated by HotSync.exe and passed
//  into the conduit's OpenConduit() function as a parameter.
//
enum eSyncTypes { eFast, eSlow, eHHtoPC, ePCtoHH, eInstall, eBackup, 
				  eDoNothing, eProfileInstall, eSyncTypeDoNotUse=0xffffffff } ;
enum eFirstSync { eNeither, ePC, eHH, eFirstSyncDoNotUse=0xffffffff } ;
#ifndef macintosh
enum eConnType  { eCable, eModemConnType, eConnTypeDoNotUse=0xffffffff };
#define eModem eModemConnType
#else
enum eConnType  { eCable, eModemConnType, eConnTypeDoNotUse=0xffffffff };
#endif

enum eSyncPref  { eNoPreference, ePermanentPreference, eTemporaryPreference, eSyncPrefDoNotUse=0xffffffff };

class CDbList;

typedef class CDbList* CDbListPtr;


class CSyncProperties
{
public:
	eSyncTypes m_SyncType;               // Fast/Slow
#ifdef macintosh
	union 
	{
		char m_PathName[BIG_PATH];				// Path to prepend for disk file names (Windows only)
		FSSpec m_UserDirFSSpec; 				// location of directory that data files  should be created in (Macintosh only)
	} u;
#else
	char       m_PathName[BIG_PATH];     // Path to prepend for disk file names
#endif
	char       m_LocalName[BIG_PATH];    // Actual local disk file names
	char       m_UserName[BIG_PATH];
  	char*      m_RemoteName[SYNC_DB_NAMELEN]; // Names of remote database file names
	CDbListPtr *m_RemoteDbList;			 // CDbLists of remote databases
	int		   m_nRemoteCount;	         // number of remote database files
	DWORD      m_Creator;                // needed to create remote Db's
	WORD       m_CardNo;                 // needed to create remote Db's
	DWORD      m_DbType;                 // needed to create remote Db's
	DWORD      m_AppInfoSize;            // convenience  
	DWORD      m_SortInfoSize;           // convenience  
	eFirstSync m_FirstDevice;            // First time sync for 1 device
	eConnType  m_Connection;             // Transfer medium
	char       m_Registry[BIG_PATH];	 // Full registry path for the conduit
	HKEY       m_hKey;                   // primary registry key
	DWORD	   m_dwReserved;			 // Reserved - set to NULL	
};
class CSyncPreference
{
public:
#ifdef macintosh
	union 
	{
		char m_PathName[BIG_PATH];				// Path to prepend for disk file names (Windows only)
		FSSpec m_UserDirFSSpec; 				// location of directory that data files  should be created in (Macintosh only)
	} u;
#else
	char       m_PathName[BIG_PATH];     // Path to prepend for disk file names
#endif
	char       m_Registry[BIG_PATH];	 // Full registry path for the conduit
	HKEY       m_hKey;                   // primary registry key
	eSyncPref  m_SyncPref;               // sync pref - perm/temp
	eSyncTypes m_SyncType;				 // sync type - fast/install
	DWORD	   m_dwReserved;			 // Reserved - set to NULL	
};

#ifdef macintosh
#pragma options align = packed
#pragma enumsalwaysint  off
#endif

typedef unsigned long CONDHANDLE;


//
//  Used by the calls to obtain the remote Categories
//
class CDbGenInfo
{
public:
	char	m_FileName[SYNC_DB_NAMELEN];	// Name of remote database file 
	WORD	m_TotalBytes;				// When reading, the caller must fill this in
											// with the size of the buffer pointed to by m_pBytes;
											// When writing, the caller must set both this field
											// and m_BytesRead to the size of the block being written.

	WORD	m_BytesRead;				// *This field is poorly named*
											// When reading, it will
											// be filled in with the actual size of
											// the app or sort block, which
											// may be bigger than the amount of data which is
											// copied to m_pBytes in the event the block is bigger
											// than the buffer (in this case, only the first m_TotalBytes
											// of record data will be copied to caller's buffer by
											// Sync API v2.1 or later, and *NOTHING* will
											// be copied by Sync API before v2.1).
											// When writing, the caller must set this field (in addition to
											// m_TotalBytes) to the size of the block being written.

	BYTE*	m_pBytes;					// pointer to caller's buffer

	DWORD	m_dwReserved;				// Reserved - set to NULL	
};


//
//  Used by all the Record Oriented API's. Houses the DT_Link version
//  of a database's record layout, specifically that of the remote device.
//  Raw bytes will be formatted into this structure by the DTLinkConverter
//  object which resides inside of each Conduit.DLL.
//
class CRawRecordInfo
{
public:
	BYTE	m_FileHandle;		// Supplied by caller
	
	DWORD	m_RecId;				// Supplied by caller when reading or deleting records by record id; supplied by
									// caller as the resource type when deleting a resource; filled in
									// by HH when reading (unique record id for records and resource type for resources).
	
	WORD	m_RecIndex;			// Supplied by caller when reading records or resources by index; supplied by caller
									// as the resource id when deleting a resource; filled in by handheld as the resource
									// id when reading a resource; filled in by HH when reading a record using Sync API v2.1
									// or later. 
	
	BYTE	m_Attribs;			// Filled in by HH when reading, and by caller when writing
	
	short	m_CatId;				// Filled in by HH when reading, and by caller when writing
	
	int	m_ConduitId;		// Ignore
	
	DWORD	m_RecSize;			// When reading, filled in by HH with the actual record/resource size,
									// which might be bigger than buffer size m_TotalBytes (in this
									// case, only the first m_TotalBytes of record data will be copied
									// to caller's buffer by Sync API v2.1 or later, and NOTHING will
									// be copied by Sync API before v2.1).  When writing, filled in by
									// caller with record data size (same as m_TotalBytes).

									// ****NOTE that m_TotalBytes is defined as WORD, meaning that only
									// records and resources under 64K may be read or written using this
									// API (the actual maximum is ~63.8K bytes).
	WORD	m_TotalBytes;		// Supplied by caller: buffer size for reading; record data size for writing
	
	BYTE*	m_pBytes;			// Buffer allocated by caller for reading or writing
	
	DWORD	m_dwReserved;		// Reserved	- set to NULL
};


// These attributes are used in CRawRecordInfo m_Attribs field:
// (*All unused bits are reserved by Sync Manager and PalmOS*)
enum eSyncRecAttrs {
	eRecAttrDeleted		= 0x80,	// indicates that this record has been deleted on the handheld
	eRecAttrDirty		= 0x40,	// indicates that this record was modified
	eRecAttrBusy		= 0x20,	// SYSTEM USE ONLY: indicates that this record is currently in use
								// by some application on the remote, hand-held device.
								// CONDUITS: this attribute must be treated as read-only; do *not* pass
								// eRecAttrBusy when writing records.
	eRecAttrSecret		= 0x10,	// "secret" record - password protected (also known as "private")
	eRecAttrArchived	= 0x08,	// indicates that this record has been marked for archival
	eSyncRecAttrDoNotUse= 0xffffffff
};

//
// values for m_Attribs in CRawRecordInfo ( as used in our sample code )
// the #defines below will be obsoleted, please use the enums (above) in your conduit
#define	DELETE_BIT	eRecAttrDeleted
#define	DIRTY_BIT	eRecAttrDirty
#define	PRIVATE_BIT	eRecAttrSecret
#define	ARCHIVE_BIT	eRecAttrArchived


//
//  Used by all the Preferences Oriented API's. Houses the DT_Link version
//  of a preference record layout, specifically that of the remote device.
//  Raw bytes will be formatted into this structure by the DTLinkConverter
//  object which resides inside of each Conduit.DLL.
//
class CRawPreferenceInfo
{
public:
	WORD		m_version;		// Pref version - Supplied by caller in Write calls and by HH in Read calls
	DWORD		m_creator;		// Pref creator - Supplied by caller
	WORD		m_prefId;		// Pref id - Supplied by caller
	WORD		m_reqBytes;		// (read calls only)Number of preference bytes requested - Supplied by caller (0xFFFF means get the entire preference)
	WORD		m_retBytes;		// (read calls only)Number of preference bytes copied to caller's buffer
	WORD		m_actSize;		// (read calls only)Actual pref size - Supplied by HH
	BOOL		m_backedUp;		// If true, the target pref database is the backed-up database - Supplied by caller
	long		m_nBytes;		// Buffer size in read calls, data size in write calls - Supplied by caller
	BYTE*		m_pBytes;		// Data buffer - Allocated and supplied by caller
	DWORD		m_dwReserved;	// Reserved	- set to NULL
};


enum eDbOpenModes { eDbShowSecret  = 0x0010, 
                    eDbExclusive   = 0x0020,
                    eDbWrite       = 0x0040,
                    eDbRead        = 0x0080,
					eDbOpenModesDoNotUse=0xffffffff
                  };

//
//  Flags which can be set in CreateDB structure or ReadDbList struct
//
enum eDbFlags {
	eRecord				= 0x0000,	// *this is not really a flag* -- see eResource
	eResource			= 0x0001,	// if set, indicates a resource database; if clear, indicates a record database
	eReadOnly			= 0x0002,	// if set, indicates a ROM-based database
	eAppInfoDirty		= 0x0004,	// set if Application Info Block is dirty
	eBackupDB			= 0x0008,	// set if database should be backed up to PC if no app-specific conduit has been supplied
	eOkToInstallNewer	= 0x0010,	// this tells the backup/restore conduit that it's OK for it to install a newer
											// version of this database with a different name if the current database is open.  This
											// mechanism is used to update the Graffiti Shortcuts databsae, for example.
	eResetAfterInstall= 0x0020,	// if set, indicates that the device should be reset after this database is installed.  The
											// actual reset will take place at end of sync.
	eCopyPrevention		= 0x0040, // supported from v3.0 on
	eStream				= 0x0080,	// This database is used for file stream implementation.
	eHidden				= 0x0100,	// This database should generally be hidden from view
															//  used to hide some apps from the main view of the
															//  launcher for example.
															// For data (non-resource) databases, this hides the record
															//	 count within the launcher info screen.
	eLaunchableData		= 0x0200,	// This data database (not applicable for executables)
															//  can be "launched" by passing it's name to it's owner
															//  app ('appl' database with same creator) using
															//  the sysAppLaunchCmdOpenNamedDB action code. 
	eRecyclable			= 0x0400,	// This database (resource or record) is recyclable:
									//  it will be deleted Real Soon Now, generally the next
									//  time the database is closed. 
	eBundle				= 0x0800,	// This database (resource or record) is associated with
									// the application with the same creator. It will be beamed
									// and copied along with the application. 
	eOpenDB				= 0x8000,		// indicates that the databse is open
	eDbFlagsDoNotUse    = 0xffffffff
	};


//
// Miscellaneous flags which can be returned in CDbList's m_miscFlags member
//
enum eMiscDbListFlags {
	// eMiscDbFlagExcludeFromSync: if this miscellaneous flag is set, indicates that
	// the db should be excluded fro HotSync;  this is typically the result of the user
	// disabling synchronization for the owning application on the handheld;  this functionality
	// was defined in DLP v1.1 (beginning with PalmOS v2.0).
	eMiscDbFlagExcludeFromSync	= 0x0080,	// if set, indicates that this database should be excluded from HotSync. (DLP v1.1)

	// eMiscDbFlagRamBased: if this miscellaneous flag is set, indicates that the db is
	// in RAM; otherwise, it is in ROM; this flag is returned from devices with Desktop Link
	// Protocol (DLP) v1.2 or later (beginning with PalmOS v3.0).  
	eMiscDbFlagRamBased			= 0x0040,		// if set, indicates that this database is RAM-based. (DLP v1.2)
	eMiscDbFlagsDoNotUse			= 0xffffffff
	};

// eExcludeFromSync is defined for backward compatibility; the new name is eMiscDbFlagExcludeFromSync
#define eExcludeFromSync	eMiscDbFlagExcludeFromSync

//
//  A single element for a ReadDBList function call.
//
class CDbList
{
public:
#ifdef macintosh
	DWORD		m_CardNum;			// DLW: Macintosh version is defined as int which is a DWORD for PPC
										// Suggest we change this in a future version of HotSync to same
										// structure for both platforms
#else
	int		 m_CardNum;
#endif		
	WORD	 m_DbFlags;         // contains Res/Record/Backup/ReadOnly (see enum eDbFlags)
	DWORD    m_DbType;
	char     m_Name[SYNC_DB_NAMELEN];
	DWORD    m_Creator;
	WORD     m_Version;
	DWORD    m_ModNumber;
	WORD     m_Index;				// not returned for SyncFindDbByName/TypeCreator
										// and SyncReadOpenDbInfo
	long     m_CreateDate;
	long     m_ModDate;
	long     m_BackupDate;
	__int32	 m_miscFlags;		// miscellaneous db list flags (see eMiscDbListFlags)
	long     m_RecCount;			// Unused - Not filled in by SyncManager calls
	long     m_dwReserved;		// Unused - set to null
};




//
//  Used in the CreateDB function call.
//
class CDbCreateDB
{
public:
	BYTE     m_FileHandle;       // Upon return gets filled in by SyncMgr.Dll
	DWORD    m_Creator;          // Supplied by caller, obtained from DbList
	eDbFlags m_Flags;            // Supplied by caller, Res/Rec/RAM
	BYTE     m_CardNo;           // Supplied by caller, target card # 
	char     m_Name[SYNC_DB_NAMELEN]; // Supplied by caller, target DBase Name
	DWORD    m_Type;             // 'DATA' or 'APPL'... 
	WORD     m_Version;
	DWORD		m_dwReserved;		  // Reserved - set to NULL	
};



// Buffer size for handheld username
#define SYNC_REMOTE_USERNAME_BUF_SIZE		(64)
#define REMOTE_USERNAME      SYNC_REMOTE_USERNAME_BUF_SIZE		/*for backward compatibility*/

// Maximum handheld username length presently allowed (not including the null-terminator)
#define SYNC_MAX_USERNAME_LENGTH				(20)


// Buffer size for handheld memory card name
#define SYNC_REMOTE_CARDNAME_BUF_SIZE		(32)
#define REMOTE_CARDNAMELEN   SYNC_REMOTE_CARDNAME_BUF_SIZE		/*for backward compatibility*/


// Buffer size for handheld manufacturer name
#define SYNC_REMOTE_MANUFNAME_BUF_SIZE		(32)
#define REMOTE_MANUFNAMELEN  SYNC_REMOTE_MANUFNAME_BUF_SIZE		/*for backward compatibility*/

// Buffer size for handheld password
#define SYNC_REMOTE_PASSWORD_BUF_SIZE      (64)
#define PASSWORD_LENGTH		SYNC_REMOTE_PASSWORD_BUF_SIZE			/*for backward compatibility*/

//
//
//
class CUserIDInfo
{
public:
	char	m_pName[SYNC_REMOTE_USERNAME_BUF_SIZE];
	int		m_NameLength;
	char	m_Password[SYNC_REMOTE_PASSWORD_BUF_SIZE];
	int		m_PasswdLength;
	long	m_LastSyncDate;         // Date/Time of last synchronization
	DWORD	m_LastSyncPC;  
	DWORD	m_Id;
	DWORD	m_ViewerId;
	DWORD	m_dwReserved;			// Reserved - set to NULL	
};



//
//  A structure element for the SyncReadSingleCardInfo() function call.
//
class CCardInfo
{
public:
	BYTE     m_CardNo;
	WORD     m_CardVersion;
	long     m_CreateDate;
	DWORD    m_RomSize;
	DWORD    m_RamSize;
	DWORD    m_FreeRam;
	BYTE     m_CardNameLen;
	BYTE     m_ManufNameLen;
	char     m_CardName[SYNC_REMOTE_CARDNAME_BUF_SIZE];
	char     m_ManufName[SYNC_REMOTE_MANUFNAME_BUF_SIZE];

	// added in v1.1 and v2.0
	WORD		m_romDbCount;		// number of ROM-based databases
	WORD		m_ramDbCount;		// number of RAM-based databases

	DWORD		m_dwReserved;			// Reserved - set to NULL	
};




//
// Used to obtain remote system information.
//

// Product ID buffer size in number of byts
#define	SYNC_MAX_PROD_ID_SIZE		(255)
class CSystemInfo
{
public:
	DWORD   m_RomSoftVersion;	// Upon return is filled in
	DWORD   m_LocalId;			// Upon return is filled in
	BYTE    m_ProdIdLength;		// Upon return is filled in (actual len) 
	BYTE    m_AllocedLen;		// Supplied by caller: size of buffer for ProductIdText
	BYTE*   m_ProductIdText;	// Allocated by caller: bufer for ProductIdText
	DWORD	m_dwReserved;			// Reserved - set to NULL	
};

//
// Rom Version Macros
//
// 0xMMmfsbbb, where MM is major version, m is minor version
// f is bug fix, s is stage: 3-release,2-beta,1-alpha,0-development,
// bbb is build number for non-releases 
// V1.12b3   would be: 0x01122003
// V2.00a2   would be: 0x02001002
// V1.01     would be: 0x01013000
#define SYNCROMVMAJOR(l)	( (WORD)( (((DWORD)(l)) >> 24) & 0x000000FFL ) )
#define SYNCROMVMINOR(l)	( (WORD)( (((DWORD)(l)) >> 20) & 0x0000000FL ) )


//
//  Used by the 'SyncCallApplication()' API 
//
class CCallAppParams
{
public:
	DWORD  m_CreatorID;
	WORD   m_ActionCode;
	WORD   m_ResultCode;
	WORD   m_ParamSize;
	BYTE*  m_pParams;

};


//
//	Used by the 'SyncCallRemoteModule()' API for Pilot v2.0 and greater
//	vmk	12/5/96
//
class CCallModuleParams
{
public:
	// PASSED VALUES:
	DWORD		m_dwCreatorID;				// target application creator ID
	DWORD		m_dwTypeID;					// target application type ID
	WORD		m_wActionCode;				// app-specific action code
	DWORD		m_dwParamSize;				// parameter block size
	void*		m_pParam;					// pointer to parameter block
	DWORD		m_dwResultBufSize;		// result buffer size
	void*		m_pResultBuf;				// pointer to result buffer

	// RETURNED VALUES
	DWORD		m_dwResultCode;			// result code returned by remote module

	DWORD		m_dwActResultSize;		// the actual result data size returned by
												// remote module; this value may be greater
												// than m_dwResultBufSize upon return -- in
												// which case the caller's buffer wasn't
												// big enough to contain the entire result
												// data block and only m_dwResultBufSize bytes
												// were copied to the result buffer.
	// RESERVED:
	DWORD		m_dwReserved;				// RESERVED -- CALLER/CALLEE: SET TO NULL!!!
};


//
//  Used by ReadPositionXMap 
//
class CPositionInfo 
{
public:
	BYTE   m_FileHandle;            // Open database handle
	WORD   m_FirstPos;              // offset of first position to read 
	WORD   m_MaxEntries;            // total number of record Id's to read 
	WORD   m_NumReadIn;             // updated with number read in 
	WORD   m_TotalBytes;            // total length of 'pBytes'
	BYTE * m_pBytes;                // buffer to contain all record Id's
};



///////////////////////////////////////////////////////////////////////////////
//
// Parameter to SyncFindDbByName, SyncFindDbByTypeCreator, and SyncReadOpenDbInfo
//
///////////////////////////////////////////////////////////////////////////////


// Database info retrieval options

#define SYNC_DB_INFO_OPT_GET_ATTRIBUTES	(0x80)
	// Set to get database attributes (name, creator, type,
	// flags, etc.) -- this is an option to allow find operations
	// to skip returning this data as a performance optimization
	// time, which we would rather avoid if it is not needed

#define SYNC_DB_INFO_OPT_GET_SIZE			(0x40)
	// Set to get record count and data size also -- this is
	// an option because the operation can take a long
	// time, which we would rather avoid if it is not needed
																
#define SYNC_DB_INFO_OPT_GET_MAX_REC_SIZE	(0x20)
	// Set to get max rec/resource size also -- this is
	// an option because the operation can take a long
	// time, which we would rather avoid if it is not needed
	// (SYNC_DB_INFO_OPT_GET_MAX_REC_SIZE is only supported for
	// SyncReadOpenDbInfo)


// Structure for returning database information
typedef struct SyncDatabaseInfoType {
	// PUBLIC FIELDS:

	// Basic database information:
	CDbList	baseInfo;						

	// Extended database information:
	
	// Returned only if SYNC_DB_INFO_OPT_GET_SIZE is set:
	DWORD		dwNumRecords;					// number of records/resources in the database
	DWORD		dwTotalBytes;					// total bytes of storage used by database (including overhead)
	DWORD		dwDataBytes;					// total bytes of storage used for data

	// Returned only for SyncReadOpenDbInfo and only if SYNC_DB_INFO_OPT_GET_SIZE
	// is set:
	DWORD		dwAppBlkSize;					// app info block size in number of bytes
	DWORD		dwSortBlkSize;					// sort info block size in number of bytes

	// Returned only for SyncReadOpenDbInfo and only if SYNC_DB_INFO_OPT_GET_MAX_REC_SIZE
	// is set:
	DWORD		dwMaxRecSize;					// size of the largest record or resource in the
													// database

	DWORD		dwReserved;						// RESERVED -- SET TO NULL!!!


	// FIELDS FOR INTERNAL USE:

	DWORD		dwLocalID;						// LocalID of the database (internal use)
	DWORD		dwOpenRef;						// db open ref of the database if it is currently opened
													// by Sync Manager; zero otherwise; may change after
													// calling SyncReadPositionXMap; (for internal use);

} SyncDatabaseInfoType;


// SyncFindDbByName parameter
typedef struct SyncFindDbByNameParams {
	BYTE		bOptFlags;						// bitfield of SYNC_DB_INFO_OPT_... flags
	DWORD		dwCardNum;						// memory card number
	char*		pcDatabaseName;				// pointer to null-terminated database name
	} SyncFindDbByNameParams;


// SyncFindDbByTypeCreator parameter
typedef struct SyncFindDbByTypeCreatorParams {
	BYTE		bOptFlags;						// bitfield of SYNC_DB_INFO_OPT_... flags
	BYTE		bSrchFlags;						// bitfield of SYNC_DB_SRCH_OPT_... flags
	DWORD		dwType;							// database type (zero = wildcard)
	DWORD		dwCreator;						// database creator (zero = wildcard)
	} SyncFindDbByTypeCreatorParams;

// Search option flags for the SyncFindDbByTypeCreatorParams structure
#define SYNC_DB_SRCH_OPT_NEW_SEARCH		(0x80)	// set to beging a new search; subsequent
																// iterations of the same search should
																// have the flag cleared
#define SYNC_DB_SRCH_OPT_ONLY_LATEST	(0x40)	// set to search for the latest version


// SyncReadOpenDbInfo parameter
typedef struct SyncReadOpenDbInfoParams {
	BYTE		bOptFlags;						// bitfield of SYNC_DB_INFO_OPT_... flags
	BYTE		bDbHandle;						// open database handle (from SyncOpenDB or SyncCreateDB)
	} SyncReadOpenDbInfoParams;


///////////////////////////////////////////////////////////////////////////////
//
// Option flags for SyncCloseDBEx
//
///////////////////////////////////////////////////////////////////////////////
#define SYNC_CLOSE_DB_OPT_UPDATE_BACKUP_DATE	0x80	// Update the backup date after closing
#define SYNC_CLOSE_DB_OPT_UPDATE_MOD_DATE		0x40	// Update the modification date after closing



///////////////////////////////////////////////////////////////////////////////
//
// Error Code Classes
//
///////////////////////////////////////////////////////////////////////////////
#define COND_ERR_CLASS						0x00001000L		// Conduit error class
#define TRANS_ERR_CLASS						0x00002000L		// Communications/Transport error class
#define SYNC_ERR_CLASS						0x00004000L		// Sync Manager DLL error class
#define HSAPP_ERR_CLASS						0x00008000L		// HotSync application error class
#define EXPAPI_ERR_CLASS					0x00002900L		// VFSAPI error class for Expansion mgr calls
#define VFSAPI_ERR_CLASS					0x00002A00L		// VFSAPI error class for vfs calls



///////////////////////////////////////////////////////////////////////////////
//
//  Error Codes returned by the SyncManager calls as a result
//  of communication over the link to the HH device.
//
///////////////////////////////////////////////////////////////////////////////

#define	 SYNC_FATAL_ERR_MASK				0x10000000L
#define   SYNC_FATAL_ERR					(SYNC_FATAL_ERR_MASK + SYNC_ERR_CLASS)

#define   SYNCERR_NONE                 0x0000						// SUCCESS!

#define   SYNCERR_UNKNOWN              (SYNC_ERR_CLASS + 0x01)	// An unknown error occurred (local/remote
																					// error code mapping does not exist)
																				

#define   SYNCERR_MORE                 (SYNC_ERR_CLASS + 0x02)	// NOT USED

#define   SYNCERR_NOT_FOUND				(SYNC_ERR_CLASS + 0x03)	// Requested database, record, resource, etc. not found	
#define   SYNCERR_FILE_NOT_FOUND       SYNCERR_NOT_FOUND			// For backward compatibility

#define   SYNCERR_FILE_NOT_OPEN        (SYNC_ERR_CLASS + 0x04)	// Attempt to open a database failed

#define   SYNCERR_FILE_OPEN            (SYNC_ERR_CLASS + 0x05)	// NOT USED

#define   SYNCERR_RECORD_BUSY          (SYNC_ERR_CLASS + 0x06)	// The requested record is in use by someone
																					// else and will remain so indefinitely

#define   SYNCERR_RECORD_DELETED       (SYNC_ERR_CLASS + 0x07)	// The requested record has either been deleted
																					// or archived

#define   SYNCERR_READ_ONLY            (SYNC_ERR_CLASS + 0x09)	// Caller does not have write access or database is in ROM
#define   SYNCERR_ROM_BASED            SYNCERR_READ_ONLY			// Defined for backward compatility

#define   SYNCERR_COMM_NOT_INIT        (SYNC_ERR_CLASS + 0x0A)	// Communications have not been intialized (this
																					// is an internal error code)

#define   SYNCERR_FILE_ALREADY_EXIST   (SYNC_ERR_CLASS + 0x0B)	// Could not create database because another
																					// one with the same name already exists on remote

#define   SYNCERR_FILE_ALREADY_OPEN    (SYNC_ERR_CLASS + 0x0C)	// The requested database is presently open by
																					// someone else

#define   SYNCERR_NO_FILES_OPEN        (SYNC_ERR_CLASS + 0x0D)	// An operation was requested on a database when
																					// no databases were open

#define   SYNCERR_BAD_OPERATION        (SYNC_ERR_CLASS + 0x0E)	// The requested operation is not supported on
																					// the given database type(record or resource).

#define   SYNCERR_REMOTE_BAD_ARG       (SYNC_ERR_CLASS + 0x0F)	// Invalid argument passed to remote

#define   SYNCERR_BAD_ARG_WRAPPER      (SYNC_ERR_CLASS + 0x10)	// Internal Desktop Link error -- indicates
																					// protocol implementation error

#define   SYNCERR_ARG_MISSING          (SYNC_ERR_CLASS + 0x11)	// Internal Desktop Link error -- indicates
																					// protocol implementation error

#define   SYNCERR_LOCAL_BUFF_TOO_SMALL (SYNC_ERR_CLASS + 0x12)	// The passed buffer is too small for the
																					// reply data

#define   SYNCERR_REMOTE_MEM           (SYNC_ERR_CLASS + 0x13)	// Insufficient memory on remote to receive
																					// or complete the request

#define   SYNCERR_REMOTE_NO_SPACE      (SYNC_ERR_CLASS + 0x14)	// Insufficient memory in remote data store
																					// to complete the request (write record,
																					// resource, etc.)

#define   SYNCERR_REMOTE_SYS           (SYNC_ERR_CLASS + 0x15)	// Generic remote system error (returned when
																					// exact cause is unknown)
																					// NEW v2.0 -- moved from fatal errors vmk 10/15/96
																					
#define   SYNCERR_LOCAL_MEM            (SYNC_ERR_CLASS + 0x16)	// Local (PC) memory allocation error
																					// NEW v2.0

// NEW v2.1
#define   SYNCERR_BAD_ARG					(SYNC_ERR_CLASS + 0x17)	// Invalid parameter to local function, or
																					// parameter is too big

#define   SYNCERR_LIMIT_EXCEEDED			(SYNC_ERR_CLASS + 0x18)	// Data limit exceeded on remote (for example,
																					// when the hotsync error log size limit has
																					// has been exceeded on remote)

#define   SYNCERR_UNKNOWN_REQUEST		(SYNC_ERR_CLASS + 0x19)	// This request (command) is not supported
																					// by remote


// Fatal Errors
#define   SYNCERR_TOO_MANY_OPEN_FILES  (SYNC_FATAL_ERR + 0x403)// Request failed because there are too many
																					// open databases (for efficiency, the current
																					// Desktop Link implementation supports only
																					// one open database at a time)
#define   SYNCERR_TOO_MANY_FILES       SYNCERR_TOO_MANY_OPEN_FILES	// Defined for backward compatibility 

#define   SYNCERR_REMOTE_CANCEL_SYNC   (SYNC_FATAL_ERR + 0x405)// The request to cancel HotSync was initiated
																					// from the remote

#define   SYNCERR_LOST_CONNECTION		(SYNC_FATAL_ERR + TRANS_ERR_CLASS + 0x410)
																					// Connection is lost.  We add TRANS_ERR_CLASS
																					// because existing software checks that
																					// bit to detect connection loss.
																					// NEW v2.0

#define   SYNCERR_LOCAL_CANCEL_SYNC		(SYNC_FATAL_ERR + 0x411)// The request to cancel HotSync was initiated
																					// from the PC
																					// NEW v2.0



///////////////////////////////////////////////////////////////////////////////////////////////
//
// Sync Manager API version numbers (v2.0 and later)
//
///////////////////////////////////////////////////////////////////////////////////////////////

/**********************************************************************************************
 * (v2.0 and later)
 *
 * The SYNC MANAGER API VERSION scheme enables the conduits to identify the level
 * of service provided by the Sync Manager DLL.
 *
 * MAJOR AND MINOR VERSION NUMBERS:
 *
 * Beginning with the current version (v2.0), the Sync Manager API will strive to
 * maintain backward compatibility within a given MAJOR version number group.  As new
 * exported functions are added to the API or critical bugs are fixed, the MINOR version number of
 * the API will be incremented and the documentation of the new functions will identify the
 * API version number where they were first available.  The conduits can (and should) check
 * the Sync Manager API version number using the function SyncGetAPIVersion().
 * So, if a conduit requires a particular Sync Manager function
 * which was added in API version number 2.1, for example, the conduit should call SyncGetAPIVersion
 * to make sure that the MAJOR number is 2 and the MINOR number is 1 or greater.
 *
 * SYNC MANAGER DLL NAMING CONVENTION:
 *
 * The Sync Manager DLL will be named to reflect the MAJOR API version number.  For example, the release
 * version of the Sync Manager DLL which implements a 2.x API will be named Sync20.dll.  The debug
 * version of same will be named Sync20d.dll.  This convention permits a conduit which depends on given
 * MAJOR and MINOR Sync Manager API version numbers to link with a Sync Manager DLL which implements an
 * API with the same MAJOR version number but a higher MINOR version number.
 *
 * vmk	10/21/96
 *
 **********************************************************************************************/

// Major API version numbers
#define	SYNCAPI_VER_MAJOR_2		2

// Minor API version numbers
#define	SYNCAPI_VER_MINOR_0		0
#define	SYNCAPI_VER_MINOR_1		1
#define	SYNCAPI_VER_MINOR_2		2
#define	SYNCAPI_VER_MINOR_3		3



#ifndef SYNC_API
	#define SYNC_API __declspec(dllimport)
#endif

extern "C" {

// NEW

// 1.0 CALLS:
SYNC_API long SyncAddLogEntry          (const char*);
SYNC_API long SyncRegisterConduit      (CONDHANDLE &) ;
SYNC_API long SyncUnRegisterConduit    (CONDHANDLE) ;
SYNC_API long SyncReadUserID           (CUserIDInfo &);
SYNC_API long SyncOpenDB               (const char* pName, int nCardNum, 
                                               BYTE& rHandle, 
                                               BYTE  openMode = (eDbWrite | 
                                                                 eDbRead  | 
                                                                eDbExclusive));
SYNC_API long SyncDeleteDB             (const char* pName, int nCardNum); 
SYNC_API long SyncCreateDB             (CDbCreateDB& rDbStats);
SYNC_API long SyncCloseDB              (BYTE);
SYNC_API long SyncGetDBRecordCount     (BYTE, WORD&);
SYNC_API long SyncPurgeDeletedRecs     (BYTE);
SYNC_API long SyncPurgeAllRecs         (BYTE);
SYNC_API long SyncResetSyncFlags       (BYTE);
SYNC_API long SyncReadDBList           (BYTE, WORD, BOOL,CDbList*,int& rCnt);
SYNC_API long SyncWriteRec             (CRawRecordInfo& rInfo);
SYNC_API long SyncDeleteRec            (CRawRecordInfo& rInfo);
SYNC_API long SyncDeleteResourceRec    (CRawRecordInfo& rInfo);
SYNC_API long SyncDeleteAllResourceRec (BYTE);
SYNC_API long SyncReadRecordById       (CRawRecordInfo& rInfo);
SYNC_API long SyncReadRecordByIndex    (CRawRecordInfo& rInfo);
SYNC_API long SyncReadResRecordByIndex (CRawRecordInfo& rInfo, 
                                                     BOOL bBody=TRUE);
SYNC_API long SyncReadNextModifiedRec  (CRawRecordInfo& rInfo);
SYNC_API long SyncReadDBAppInfoBlock   (BYTE, CDbGenInfo& rInfo);
SYNC_API long SyncWriteDBAppInfoBlock  (BYTE, CDbGenInfo& rInfo);
SYNC_API long SyncWriteResourceRec     (CRawRecordInfo& rInfo);
SYNC_API long SyncRebootSystem         (void);
SYNC_API long SyncReadSystemInfo       (CSystemInfo& rInfo);
SYNC_API long SyncReadSingleCardInfo   (CCardInfo& rInfo );
SYNC_API long SyncReadSysDateTime      (long& rDate);
SYNC_API long SyncWriteSysDateTime     (long  lDate);
SYNC_API long SyncReadDBSortInfoBlock  (BYTE, CDbGenInfo& rInfo);
SYNC_API long SyncWriteDBSortInfoBlock (BYTE, CDbGenInfo& rInfo);
SYNC_API long SyncCallApplication      (CCallAppParams& rOutParams,
                                          CCallAppParams& rInParams);
SYNC_API long SyncChangeCategory       (BYTE fHandle, BYTE from, BYTE to);
SYNC_API long SyncReadPositionXMap     (CPositionInfo& rInfo);

SYNC_API long SyncYieldCycles          (WORD wMaxMiliSecs);

// Sync API v2.0 CALLS ADDED HERE:
SYNC_API long SyncReadNextRecInCategory	(CRawRecordInfo& rInfo);
SYNC_API long SyncReadNextModifiedRecInCategory	(CRawRecordInfo& rInfo);
SYNC_API long SyncReadAppPreference	(CRawPreferenceInfo& rInfo);
SYNC_API long SyncWriteAppPreference	(CRawPreferenceInfo& rInfo);
SYNC_API long SyncResetRecordIndex   (BYTE fHandle);
SYNC_API long SyncPurgeAllRecsInCategory	(BYTE fHandle, short category);
SYNC_API long SyncGetAPIVersion(DWORD* pdwMajor, DWORD* pdwMinor);

// Sync API v2.1 CALLS ADDED HERE:
SYNC_API long SyncCallRemoteModule(CCallModuleParams* pParams);
SYNC_API long SyncReadFeature(DWORD dwFtrCreator, WORD wFtrNum, DWORD* pdwFtrValue);
SYNC_API WORD SyncGetHHOSVersion(WORD* pwRomVMinor);
SYNC_API	WORD SyncHostToHHWord(WORD wValue);
SYNC_API WORD SyncHHToHostWord(WORD wValue);
SYNC_API	DWORD SyncHostToHHDWord(DWORD dwValue);
SYNC_API DWORD SyncHHToHostDWord(DWORD dwValue);

//		Sync API v2.2 PUBLIC API:

// Get the max record size that may be allocated
// on the remote device (if sufficient free space exists).
// (0xFFFFFFFF = any size up to available free space; 0 = unknown)
SYNC_API	long SyncMaxRemoteRecSize(DWORD& rdwMaxRecSize);

// Looks for a database given its name and memory card number;
// returns comprehensive information about the database, if found
SYNC_API long SyncFindDbByName(SyncFindDbByNameParams& rParam,
	SyncDatabaseInfoType& rInfo);

// Looks for a database given its type and creator; can be iterated; wild cards are
// supported; returns comprehensive information about the database, if found
SYNC_API long SyncFindDbByTypeCreator(SyncFindDbByTypeCreatorParams& rParam,
	SyncDatabaseInfoType& rInfo);

// Returns comprehensive information about an open database
SYNC_API long SyncReadOpenDbInfo(SyncReadOpenDbInfoParams& rParam,
	SyncDatabaseInfoType& rInfo);

// Closes a database and optionally updates its backup
// and/or modification date; bOptFlags may be zero or one
// or more SYNC_CLOSE_DB_OPT_... flags bitwise or'ed
// together (***non-zero bOptFlags may only be passed to PalmOS v3.0
// or later; passing non-zero flags to older version will cause the
// close command to fail***)
SYNC_API long SyncCloseDBEx(BYTE dbHandle, BYTE bOptFlags);

// Sync API v2.3 calls added here
SYNC_API long SyncLoopBackTest(DWORD dwSizeSend, BYTE* pDataSend, DWORD* pdwSizeRecv, BYTE* pDataRecv);

}

#ifdef macintosh
typedef  long (*PROGRESSFN) (char*);
#endif

#ifndef macintosh
#pragma pack()
#endif

#endif


