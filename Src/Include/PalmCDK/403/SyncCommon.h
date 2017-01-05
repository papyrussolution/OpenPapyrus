/******************************************************************************
 *
 * Copyright (c) 1998-2004 PalmSource, Inc. All rights reserved.
 *
 * File: SyncCommon.h
 *
 * Release: 
 *
 *****************************************************************************/

/** @file SyncCommon.h  
 * This header file contains the macros, enums, defines, classes, structures, and API used for accessing
 * general information about a PalmOS device. Use the Sync Manager with your conduit to exchange data with Palm OS® handhelds that 
 * are connected to the desktop computer. Handhelds can be connected to the desktop computer with a cradle 
 * and a serial cable, modem, or TCP/IP network connection. The Sync Manager handles the actual handheld 
 * communications and provides functions to receive data from and send data to databases on the handheld. 
 *
 * @note When using the Microsoft compiler we assure packed 
 * structures on single byte boundaries, with the pragma(1).
 *
 **/

#ifndef  __SYNCCOMMON_PUBLIC_API__
#define  __SYNCCOMMON_PUBLIC_API__ 

#include "HSDataTypes.h"

#if TARGET_OS_MAC
	#pragma options align= packed
	#pragma enumsalwaysint  off
#else	
	#pragma pack(1)
#endif

///////////////////////////////////////////////////////////////////////////////////////////////
//
// Sync Manager API version numbers (v2.0 and later)
//
///////////////////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @defgroup SyncAPIVersioning Sync API Versioning
 * @par
 * (v2.0 and later)
 * @par
 * The SYNC MANAGER API VERSION scheme enables the conduits to identify the level
 * of service provided by the Sync Manager DLL.
 * @par
 * MAJOR AND MINOR VERSION NUMBERS:
 * @par
 * Beginning with the current version (v2.0), the Sync Manager API will strive to
 * maintain backward compatibility within a given MAJOR version number group.  As new
 * exported functions are added to the API or critical bugs are fixed, the MINOR version number of
 * the API will be incremented and the documentation of the new functions will identify the
 * API version number where they were first available.  The conduits can (and should) check
 * the Sync Manager API version number using the function SyncGetAPIVersion().
 * So, if a conduit requires a particular Sync Manager function
 * which was added in API version number 2.1, for example, the conduit should call SyncGetAPIVersion
 * to make sure that the MAJOR number is 2 and the MINOR number is 1 or greater.
 * @par
 * SYNC MANAGER DLL NAMING CONVENTION:
 * @par
 * The Sync Manager DLL will be named to reflect the MAJOR API version number.  For example, the release
 * version of the Sync Manager DLL which implements a 2.x API will be named Sync20.dll.  The debug
 * version of same will be named Sync20d.dll.  This convention permits a conduit which depends on given
 * MAJOR and MINOR Sync Manager API version numbers to link with a Sync Manager DLL which implements an
 * API with the same MAJOR version number but a higher MINOR version number.
 * @par
 * @revisions vmk	10/21/96
 *
 **********************************************************************************************/
/*@{*/
// Major API version numbers
#define	SYNCAPI_VER_MAJOR_2		2

// Minor API version numbers
#define	SYNCAPI_VER_MINOR_0		0
#define	SYNCAPI_VER_MINOR_1		1
#define	SYNCAPI_VER_MINOR_2		2
#define	SYNCAPI_VER_MINOR_3		3
#define	SYNCAPI_VER_MINOR_4		4
#define	SYNCAPI_VER_MINOR_5		5
/*@}*/


#define SYNC_DB_NAMELEN			(32)			/**< Maximum length of Viewer's database name, including the null-terminator */
#define DB_NAMELEN				SYNC_DB_NAMELEN	/**< Maximum length of Viewer's database name, including the null-terminator */
#define SYNC_MAX_HH_LOG_SIZE	(2*1024)		/**< Maximum size of Viewer's HotSync log */
#define BIG_PATH				256



//
//  Common sync properties structure populated by HotSync.exe and passed
//  into the conduit's OpenConduit() function as a parameter.
//

/** 
 *
 * @brief The synchronization type constants are used to tell your conduit which type of synchronization operation to perform. 
 * You also use one of these values in an object of the CSyncPreference class to inform HotSync Manager of your conduit's mode, when the 
 * application has called the conduit via the ConfigureConduit(), CfgConduit(), or GetConduitInfo() methods.
 **/
enum eSyncTypes { eFast,						/**< Perform a fast synchronization: only records that have been added, 
													 archived, deleted, or modified are exchanged between the handheld 
													 and the desktop computer.	*/
				  eSlow,						/**< Perform a slow synchronization: every record is read from the handheld 
													 and compared to the corresponding record on the desktop computer (the 
													 data in the .dat file and the data in the .bak file). Used when the handheld 
													 has been synchronized with multiple computers.	*/
				  eHHtoPC,						/**< Perform a restore from the handheld: overwrite the desktop database with the 
													 database from the handheld.	*/
				  ePCtoHH,						/**< Perform a restore from the desktop computer: overwrite the database on the 
													 handheld with the database on the desktop computer.	*/
				  eInstall,						/**< Install new applications from the desktop computer to the handheld.	*/
				  eBackup,						/**< Perform a backup of the databases on the handheld to the desktop computer. */
				  eDoNothing,					/**< The conduit does not exchange data between the handheld and the desktop 
													 computer; however, the conduit is loaded and can set flags or log messages.	*/
				  eProfileInstall,				/**< Perform a user profile download. A user profile is a special user account that 
													 you can set up on the desktop computer that downloads data to a handheld, without 
													 assigning a user ID.	*/
				  eSyncTypeDoNotUse=0xFFFF		/**< An invalid value used to get all compilers to see the enum as a 4 byte value. */
				  };  

/**
 *
 * @brief These enum values are used to determine whether the handheld has previously been synchronized with the desktop computer. 
 *
 **/
enum eFirstSync { eNeither,						/**< The handheld has been synchronized before with this desktop computer: the handheld
													 user ID matches a user ID on the desktop computer.	*/
				  ePC,							/**< The handheld has been synchronized before, but has never been synchronized on the 
													 desktop computer: the handheld has a user ID that does not match a user ID on the 
													 desktop computer.	*/
				  eHH,							/**< The handheld has not been synchronized before and thus does not have a user ID. 
													 This might indicate a handheld that was recently reset or synchronized with a 
													 user profile. 	*/
				  eFirstSyncDoNotUse=0xFFFF /**< An invalid value used to get all compilers to see the enum as a 4 byte value. */
				   } ; 
/**
 *
 * @brief These enum values are used to determine how the handheld is connected to the desktop computer. 
 *
 **/
#if TARGET_OS_MAC
enum eConnType  { eCable,						/**< The handheld is connected with a fast connection, either with a direct cable or 
													 through a local area network.	*/
				  eModemConnType,				/**< The handheld is connected with a slow connection via a modem handheld.	*/
				  eConnTypeDoNotUse=0xFFFF		/**< An invalid value used to get all compilers to see the enum as a 4 byte value. */
				  };
#else
enum eConnType  { eCable,						/**< The handheld is connected with a fast connection, either with a direct cable or 
													 through a local area network.	*/
				  eModemConnType,				/**< The handheld is connected with a slow connection via a modem handheld.	*/
				  eConnTypeDoNotUse=0xFFFF 		/**< An invalid value used to get all compilers to see the enum as a 4 byte value. */
				  };
#define eModem eModemConnType
#endif


/**
 *
 * @brief These enum values are used to specify whether the user preferences apply temporarily or permanently when configuration settings in a conduit. 
 *
 **/
enum eSyncPref  { eNoPreference,				/**< Not specified.	*/
				  ePermanentPreference,			/**< The preference is permanent and applies to all future synchronization operations.	*/
				  eTemporaryPreference,			/**< The preference is temporary and only apply to the next synchronization operation. */
				  eSyncPrefDoNotUse=0xFFFF		/**< An invalid value used to get all compilers to see the enum as a 4 byte value. */
				  };

/**
 *
 * @brief This class is used to contain information about a database on the handheld. It is used with methods including SyncReadDBList, 
 * SyncReadOpenDbInfo, SyncFindDbByName, and SyncFindDbByTypeCreator. 
 *
 **/
class CDbList
{
public:
#if TARGET_OS_MAC
	UInt32		m_CardNum;				/**< A UInt32 value that contains the number of the memory card on which 
											the database is stored. The first card in the system is card number 0, 
											and subsequent card numbers are incremented by 1. */
#else
	SInt32		 m_CardNum;				/**< A SInt32 value that contains the number of the memory card on which 
											the database is stored. The first card in the system is card number 0, 
											and subsequent card numbers are incremented by 1.*/
#endif		
	UInt16		m_DbFlags;				/**< A UIn16 value containing a combination of eDbFlags values.*/
	UInt32		m_DbType;				/**< A UInt32 value containing the  the database type, which is a 4-byte 
											type identifier value for the database.*/
	char		m_Name[SYNC_DB_NAMELEN];/**< A array of SYNC_DB_NAMELEN chars that contains the name of the database.*/
	UInt32		m_Creator;				/**< A UInt32 value that contains the creator ID of the database.*/
	UInt16		m_Version;				/**< A UInt16 value that contains the version of the database.*/
	UInt32		m_ModNumber;			/**< A UInt32 value that contains the database modification number.*/
	UInt16		m_Index;				/**< A UInt16 value that contains the index of the database in the 
											list of databases on the handheld. Note: this value is not set for 
											the SyncReadOpenDbInfo, SyncFindDbByName, or SyncFindDbByTypeCreator functions.*/
	SInt32		m_CreateDate;			/**< A SInt32 value that contains the date on which the database was created. This is a time_t value.*/
	SInt32		m_ModDate;				/**< A SInt32 value that contains the date of the most recent database modification. Note: versions 
											1.x of the Palm OS software did not update the modification date. This is a time_t value.*/
	SInt32		m_BackupDate;			/**< A SInt32 value that contains the date of the most recent database backup. This is a time_t value.*/
	SInt32		m_miscFlags;			/**< A SInt32 value that contains miscellaneous flags for the database. You can combine values 
											from the miscellaneous database constants, as described in Miscellaneous Database Flag 
											(eMiscDbListFlags) Constants.*/
	SInt32		m_RecCount;				/**< A SInt32 value that is currently unused.*/
	SInt32		m_dwReserved;			/**< Reserved for future use. You must set this field to NULL (0) before 
											calling the function. */
};

/**
 * @brief A pointer to a CDbList object.
 **/
typedef class CDbList* CDbListPtr; 

/**
 *
 * @brief This class contains information about the properties of the current conduit's synchronization operations. 
 * This class is used by conduits to determine the general sync mode of the HotSync process and contains information
 * about the databases this conduit is configured to handle. 
 *
 **/
class CSyncProperties
{
public:
	eSyncTypes m_SyncType;						/**< The current synchronization type. Use one of the eSyncTypes enums. */
#if TARGET_OS_MAC
	union 
	{
		char	m_PathName[BIG_PATH];			/**< Path to prepend for disk file names (Windows only) */
		FSSpec	m_UserDirFSSpec; 				/**< location of directory that data files  should be created in (Macintosh only) */
	} u;
#else
	char		m_PathName[BIG_PATH];     		/**< A null-terminated string specifying the path to prepend to file names on the desktop computer. */
#endif
	char		m_LocalName[BIG_PATH];    		/**< A null-terminated string specifying the file name on the desktop computer.*/
	char		m_UserName[BIG_PATH];			/**< A null-terminated string specifying the HotSync name of the currently synchronizing user. */
#if TARGET_OS_MAC
	char**		m_RemoteName;             		/**< An array of null-terminated char strings containing the names of the databases on the handheld 
													associated with this conduit. The m_RemoteDbList member contains more detailed information and 
													may be used instead. */
#else
  	char*		m_RemoteName[SYNC_DB_NAMELEN]; 	/**< An array of the names of the databases on the handheld. Do not rely on data in this array 
													if your conduit is associated more than 32 databases with the same creator ID on the 
													handheld; use m_RemoteDbList instead. */
#endif
	CDbListPtr*	m_RemoteDbList;			 		/**< An array of CDbList classes containing information on the remote databases the conduit
												    should synchronize. The number of items in the array is specifying by the m_nRemoteCount member.*/
	SInt32		m_nRemoteCount;	         		/**< The number of entries in the m_RemoteDbList array. */
	UInt32		m_Creator;                		/**< A UInt32 value specifying the creator id associated with this conduit. */
	UInt16		m_CardNo;                 		/**< A UInt16 value specifying the card number of the handheld memory. */
	UInt32		m_DbType;                 		/**< A UInt32 value specifying the type of database of the remote database. */
	UInt32		m_AppInfoSize;            		/**< A UInt32 value specifying the size of the remote application information block of the default remote database. */
	UInt32		m_SortInfoSize;           		/**< A UInt32 value specifying the size of the remote sort information block of the default remote database. */
	eFirstSync	m_FirstDevice;            		/**< A eFirstSync enum specifying whether this is the first time sync for the handheld device.*/
	eConnType	m_Connection;             		/**< A eConnType enum specifying the type of transfer medium of this synchronization session. */
  	char		m_Registry[BIG_PATH];	 		/**< Deprecated. An array of char containing a null-terminated string specifying the full registry path for the conduit. */
#if TARGET_OS_MAC
  	UInt32		m_hKey;                   		/**< Deprecated. ?? on mac */
#else
  	HKEY		m_hKey;							/**< Deprecated. primary registry key. */
#endif
	UInt32		m_dwReserved;			 		/**< Reserved - set to NULL.	*/
};

/**
 *
 * @brief This class is used with the conduit entry point ConfigureConduit(). It is used to transfer information to and from the conduit to the
 * HotSync Manager application.
 *
 **/
class CSyncPreference
{
public:
#if TARGET_OS_MAC
	union 
	{
		char	m_PathName[BIG_PATH];			/**< A null-terminated string containing the path to prepend for disk file names (Windows only) */
		FSSpec	m_UserDirFSSpec; 				/**< A FSSpect containing the location of directory that data files  should be created 
													 in (Macintosh only) */
	} u;
#else
	char		m_PathName[BIG_PATH];     		/**< A null-terminated string containing the path to prepend for disk file names */
#endif
   	char		m_Registry[BIG_PATH];	 		/**< Deprecated. A null terminated string specifying the full registry path for the conduit. */
#if TARGET_OS_MAC
   	UInt32		m_hKey;                   		/**< Deprecated. ?? on mac. */
#else
   	HKEY		m_hKey;							/**< Deprecated. primary registry key. */
#endif
	eSyncPref	m_SyncPref;               		/**< A eSyncPref value used by the conduit to inform HotSync Manager whether the user's selection
													 in the "Configure conduit" dialog is a permanent or temporary setting. */
	eSyncTypes	m_SyncType;				 		/**< A eSyncTypes value for in/out transfer of information between HotSync Manager and the conduit.
													 On [in] the value contains the next sync setting of the conduit. On [out], this value
													 contains the user's selection. */
	UInt32		m_dwReserved;			 		/**< Reserved - set to NULL	*/
};

typedef UInt32 CONDHANDLE; /**< A handle used with SyncRegisterConduit() and SyncUnRegisterConduit() */ 


/**
 *
 * @brief This enum is used to specify the different ways in which a handheld database can be openned.
 *
 **/
enum eDbOpenModes { 
		eDbShowSecret  = 0x0010,	/**< This enum specifies that all records of the database should be accessable. */
        eDbExclusive   = 0x0020,	/**< This enum specifies that the caller should be the only one to access the 
										database at the current time. */
        eDbWrite       = 0x0040,	/**< This enum specifies that the database should be openned for modifications. */
        eDbRead        = 0x0080,	/**< This enum specifies that the database should be openned for reading. */
		eDbOpenModesDoNotUse=0xFFFF /**< An invalid value used to get all compilers to see the enum as a 4 byte value. */
        };

/**
 *
 *  @brief This enum is used to specify flags which can be set in CreateDB structure or ReadDbList structure.
 *
 **/
enum eDbFlags {
	eRecord				= 0x0000,	/**< This default value is used with eResource is not set. It is used to specify the database
										 is a Record database. */
	eResource			= 0x0001,	/**< This value is used to indicate a resource database. */
	eReadOnly			= 0x0002,	/**< This value is used to indicate a ROM-based database. */
	eAppInfoDirty		= 0x0004,	/**< This value is used to indicate if the Application Info Block is dirty (set). */
	eBackupDB			= 0x0008,	/**< This value is used to indicate if the database should be backed up to desktop if no 
										app-specific conduit has been supplied. */
	eOkToInstallNewer	= 0x0010,	/**< This value is used to tell the backup/restore conduit that it's OK for it to install a newer
										version of this database with a different name if the current database is open.  This
										mechanism is used to update the Graffiti Shortcuts databsae, for example. */
	eResetAfterInstall	= 0x0020,	/**< This value is used to indicate that the device should be reset after this database is installed.  The
										actual reset will take place at end of sync. */
	eCopyPrevention		= 0x0040,	/**< This value is used to indicate that the database is not to be copied or beamed to other handhelds. 
										Supported from v3.0 on. */
	eStream				= 0x0080,	/**< This database is used for file stream implementation. */
	eHidden				= 0x0100,	/**< This database should generally be hidden from view. This is used to hide some apps from the main view 
										of the launcher. For data (non-resource) databases, this hides the record count within the launcher info 
										screen. */
	eLaunchableData		= 0x0200,	/**< This data database (not applicable for executables). This database can be "launched" by passing it's 
										name to it's owner app ('appl' database with same creator) using the sysAppLaunchCmdOpenNamedDB action code. */
	eRecyclable			= 0x0400,	/**< This database (resource or record) is recyclable: it will be deleted Real Soon Now, 
										generally the next time the database is closed. */
	eBundle				= 0x0800,	/**< This database (resource or record) is associated with the application with the same creator. It will be 
										beamed and copied along with the application. */
	eSchema				= 0x1000,	/**< This value is used to indicate that the database is a schema database (i.e. a DB-style database). */
	eSecure				= 0x2000,	/**< This value is used to indicate that the database is a secure database. */
	eOpenDB				= 0x8000,	/**< This value is used to indicate that the database is currently open. */
	eDbFlagsDoNotUse    = 0xFFFF	/**< An invalid value used to get all compilers to see the enum as a 4 byte value. */
	};

/**
 * This enum is used to specify miscellaneous flags which can be returned in CDbList m_miscFlags member.
 **/
enum eMiscDbListFlags {
	eMiscDbFlagExcludeFromSync	= 0x0080,		/**< If this flag is set, it indicates that the database should be not be
													included in the HotSync synchronization session. This is typically the 
													result of the user disabling synchronization for the owning application 
													on the handheld. This functionality was defined in DLP v1.1 (beginning with PalmOS v2.0).
													This constant replaces the older constant eExcludeFromSync. */
	eMiscDbFlagRamBased			= 0x0040,		/**< If this flag is set, it indicates that the database is in RAM. If not set, the database 
													is in ROM. Available with Palm OS software versions 3.0 or later. */
	eMiscDbFlagsDoNotUse		= 0xFFFF		/**< An invalid value used to get all compilers to see the enum as a 4 byte value. */
	};

/**
 *
 * This enum is used to describe the modification attributes of a record. These values are used in CRawRecordInfo m_Attribs field. All unused 
 * bits are reserved by Sync Manager and PalmOS.
 *
 **/
enum eSyncRecAttrs {
	eRecAttrDeleted		= 0x80,		/**< Indicates that this record has been deleted on the handheld. */
	eRecAttrDirty		= 0x40,		/**< Indicates that this record was modified. */
	eRecAttrBusy		= 0x20,		/**< SYSTEM USE ONLY: indicates that this record is currently in use
										by some application on the remote, hand-held device.
										CONDUITS: this attribute must be treated as read-only; do *not* pass
										eRecAttrBusy when writing records. */
	eRecAttrSecret		= 0x10,		/**< Indicates "secret" record - password protected (also known as "private") */
	eRecAttrArchived	= 0x08,		/**< Indicates that this record has been marked for archival */
	eSyncRecAttrDoNotUse= 0xFFFF	/**< An invalid value used to get all compilers to see the enum as a 4 byte value. */
};
 
#define eExcludeFromSync	eMiscDbFlagExcludeFromSync /**< eExcludeFromSync is defined for backward compatibility; the new name is eMiscDbFlagExcludeFromSync. */


// Record attributes for classic databases
#define	dmRecAttrDelete				0x80
#define	dmRecAttrDirty				0x40
#define	dmRecAttrBusy				0x20
#define	dmRecAttrSecret				0x10

// All record atributes (for error-checking)
#define	dmAllRecAttrs				( dmRecAttrDelete 	|	\
									  dmRecAttrDirty 	|	\
									  dmRecAttrBusy 	|	\
									  dmRecAttrSecret )

#define	dmSysOnlyRecAttrs			( dmRecAttrBusy )


/**
 *
 * @brief Specifying this constant as a write offset causes an append. For example, 
 * in SynDmWriteRecord() or SyncDbWriteColumnValue() specifying this constant causes 
 * the API to write new data at the end of any  existing data. This is equivalent to 
 * specifying an offset equal to the data  size, but does not require a preceding 
 * call to retrieve the size when it's not known in advance.
 *
 **/
#define kOffsetEndOfData			(0xFFFFFFFF)


// Database attributes
/**
 * @defgroup DmDatabaseHeaderAttributes Dm Database Header Attribute Values
 * These values are used to determine detailed information about a database.
 **/
#define	dmHdrAttrResDB				0x0001		/**< This value specifies the database is a contains resource records. */
#define	dmHdrAttrReadOnly			0x0002		/**< This value specifies the database is read only, which meand the 
													database is probably ROM-based.*/
#define	dmHdrAttrAppInfoDirty		0x0004		/**< This value specifies the database has a modified appInfo block.*/	
#define	dmHdrAttrBackup				0x0008		/**< This value specifies the database should be backed-up 
													during synchronization sessions.*/
#define	dmHdrAttrOKToInstallNewer 	0x0010		/**< This value specifies the database allows newer installs 
													over the existing application.*/
#define	dmHdrAttrResetAfterInstall	0x0020		/**< This value specifies the handheld device should be soft-reset after the
													installation of this database.*/
#define	dmHdrAttrCopyPrevention		0x0040		/**< This value specifies the database is copy protected( and cannot be beamed).*/
#define	dmHdrAttrStream				0x0080		/**< This database is used for file stream implementation. */
#define	dmHdrAttrHidden				0x0100		/**< This value specifies the database should be hidden from the user's view.*/
#define	dmHdrAttrLaunchableData		0x0200		/**< This data database (not applicable for executables). This database can be "launched" by passing it's 
													 name to it's owner app ('appl' database with same creator) using the sysAppLaunchCmdOpenNamedDB action code. */
#define	dmHdrAttrRecyclable			0x0400		/**< This database (resource or record) is recyclable: it will be deleted Real Soon Now, 
													 generally the next time the database is closed. */
#define	dmHdrAttrBundle				0x0800		/**< This database (resource or record) is associated with the application with the same creator. It will be 
													 beamed and copied along with the application. */
#define	dmHdrAttrSchema				0x1000		/**< This value is used to indicate that the database is a schema database (i.e. a DB-style database). */
#define	dmHdrAttrSecure				0x2000		/**< This value is used to indicate that the database is a secure database. */
#define	dmHdrAttrFixedUp			0x4000		/**< This value specifies the database is Fixedup.*/
#define	dmHdrAttrOpen				0x8000		/**< This value is used to indicate that the database is currently open. */
#define dmHdrAttrClassic            0x0000      /**< A zero in the secure and schema bit fields indicates a classic database */

// overloaded header attributes...

// Set if database is a non-schema rec/res db as opposed to a classic (used/created by a 68K
// emulated app) database. This attribute is otherwise only ever used for schema databases.
#define	dmHdrAttrExtended			(dmHdrAttrSecure)

// All database atributes (for error-checking)
#define	dmAllHdrAttrs				(	dmHdrAttrResDB |				\
										dmHdrAttrReadOnly |				\
										dmHdrAttrAppInfoDirty |			\
										dmHdrAttrBackup |				\
										dmHdrAttrOKToInstallNewer |		\
										dmHdrAttrResetAfterInstall |	\
										dmHdrAttrCopyPrevention |		\
										dmHdrAttrStream |				\
										dmHdrAttrHidden |				\
										dmHdrAttrLaunchableData |		\
										dmHdrAttrRecyclable |			\
										dmHdrAttrBundle |				\
										dmHdrAttrSchema |				\
										dmHdrAttrSecure |				\
										dmHdrAttrFixedUp |				\
										dmHdrAttrOpen	)
													
// Database attributes which only the system is allowed to change (for error-checking)
#define	dmSysOnlyHdrAttrs			(	dmHdrAttrResDB |		\
										dmHdrAttrSchema |		\
										dmHdrAttrSecure |		\
										dmHdrAttrFixedUp |		\
										dmHdrAttrOpen	)




#define SYNC_REMOTE_USERNAME_BUF_SIZE		(64)				/**< This value specifies the buffer size for handheld username. */
#define REMOTE_USERNAME      SYNC_REMOTE_USERNAME_BUF_SIZE		/*for backward compatibility*/

 
#define SYNC_MAX_USERNAME_LENGTH			(20)				/**< This value specifies the maximum handheld username length presently 
																	allowed (not including the null-terminator). */

#define SYNC_REMOTE_CARDNAME_BUF_SIZE		(32)				/**< This value specifies the buffer size for handheld memory card name. */
#define REMOTE_CARDNAMELEN   SYNC_REMOTE_CARDNAME_BUF_SIZE		/*for backward compatibility*/


#define SYNC_REMOTE_MANUFNAME_BUF_SIZE		(32)				/**< This value specifies the buffer size for handheld manufacturer name. */
#define REMOTE_MANUFNAMELEN  SYNC_REMOTE_MANUFNAME_BUF_SIZE		/*for backward compatibility*/

 
#define SYNC_REMOTE_PASSWORD_BUF_SIZE      (64)					/**< This value specifies the buffer size for handheld password. */
#define PASSWORD_LENGTH		SYNC_REMOTE_PASSWORD_BUF_SIZE			/*for backward compatibility*/

/**
 *
 * @brief This class is used to retrieve information about the user on the handheld. You use 
 *		this class with the SyncReadUserID() method. 
 *
 **/
class CUserIDInfo
{
public:
	char	m_pName[SYNC_REMOTE_USERNAME_BUF_SIZE];		/**< A char array of length SYNC_REMOTE_USERNAME_BUF_SIZE used to receive
																the user name of the handheld. */
	SInt32	m_NameLength;								/**< A SInt32 value that receives the actual length of the user name stored 
																in the m_pName field. */
	char	m_Password[SYNC_REMOTE_PASSWORD_BUF_SIZE];	/**< A char array of length SYNC_REMOTE_PASSWORD_BUF_SIZE used to receive
																the user's encrypted password, in binary format. */
	SInt32	m_PasswdLength;								/**< A SInt32 value that receives the actual length of the user's encrypted password. */
	SInt32	m_LastSyncDate;								/**< A SInt32 value that receives date of the most recent synchronization for the handheld. 
															This is a time_t value. Note: this field is set to 0 when you call the SyncReadUserID 
															function to read information from a handheld running a version of the Palm OS software 
															earlier than version 3.0. For versions 3.0 and later, this value is set correctly. */
	UInt32	m_LastSyncPC;								/**< A UInt32 value that receives the HotSync ID of the desktop computer with which the 
															handheld was most recently synchronized. This value is created when HotSync Manager 
															is installed, and is stored in the conduit configuration entries. */
	UInt32	m_Id;										/**< A UInt32 value that receives the user ID of the handheld. */
	UInt32	m_ViewerId;									/**< A UInt32 value that receives the ID of the handheld. Not currently used. */
	UInt32	m_dwReserved;								/**< Reserved for future use. You must set this field to NULL (0) before 
															 calling the function. */
};



/**
 *
 * @brief This class is used with the SyncReadSingleCarInfo() to obtain information about a 
 *		memory card on the handheld. 
 *
 **/
class CCardInfo
{
public:
	HSByte		m_CardNo;			/**< A HSByte value used to specify which card to read. */
	UInt16		m_CardVersion;		/**< A UInt16 value which receives the version of the card. */
	SInt32		m_CreateDate;		/**< A SInt32 value which receives the creation date of the card. This is a time_t value. */
	UInt32		m_RomSize;			/**< A UInt32 value which receives the amount of ROM on the card. */
	UInt32		m_RamSize;			/**< A UInt32 value which receives the total amount of RAM on the card. */
	UInt32		m_FreeRam;			/**< A UInt32 value which receives the amount of available RAM on the card. */
	HSByte		m_CardNameLen;		/**< A HSByte value which receives the number of characters in the card name. */
	HSByte		m_ManufNameLen;		/**< A HSByte value which receives the number of character in the manufacturer's name. */
	char		m_CardName[SYNC_REMOTE_CARDNAME_BUF_SIZE];		/**< An array of chars used to contain card name string. */
	char		m_ManufName[SYNC_REMOTE_MANUFNAME_BUF_SIZE];	/**< An array of chars used to contain manufacturer's name string. */

	// added in v1.1 and v2.0
	UInt16		m_romDbCount;		/**< A UInt16 value which receives the number of ROM-based databases on the card. */ 
	UInt16		m_ramDbCount;		/**< A UInt16 value which receives the number of RAM-based databases on the card*/

	UInt32		m_dwReserved;		/**< Reserved for future use. You must set this field to NULL (0) before calling the function. */
};


#define	SYNC_MAX_PROD_ID_SIZE		(255) /**< Product ID buffer size in number of bytes */

/**
 * 
 * @brief This class is used to obtain handheld system information. It 
 *		is used with the SyncReadSystemInfo() method.
 *
 **/
class CSystemInfo
{
public:
	UInt32   m_RomSoftVersion;		/**< A UInt32 value that receives the ROM version of the handheld. */
	UInt32   m_LocalId;				/**< A UInt32 value that receives the localization ID for the handheld. */
	HSByte   m_ProdIdLength;		/**< A HSByte value that receives the actual length of the product ID 
										that is stored into the m_ProductIdText field. */
	HSByte   m_AllocedLen;			/**< A HSByte value that is set to the size of buffer for ProductIdText by the caller. */
	HSByte*  m_ProductIdText;		/**< An array of bytes that is used to receive product ID. The caller must allocate 
										this buffer before calling a function with this object. This buffer must be at least 
										SYNC_MAX_PROD_ID_SIZE bytes long. The SyncReadSystemInfo function stores the handheld 
										information into this buffer. */
	UInt32	 m_dwReserved;			/**< Reserved for future use. You must set this field to NULL (0) before 
															 calling the function. */
};

/**
 * @defgroup RomVersionMacros Rom Version Macros
 *
 * 0xMMmfsbbb, where MM is major version, m is minor version
 * f is bug fix, s is stage: 3-release,2-beta,1-alpha,0-development,
 * bbb is build number for non-releases 
 * @par
 * V1.12b3   would be: 0x01122003
 * @par 
 *  V2.00a2   would be: 0x02001002
 * @par
 *  V1.01     would be: 0x01013000
 **/
/*@{*/
#define SYNCROMVMAJOR(l)	( (UInt16)( (((UInt32)(l)) >> 24) & 0x000000FFL ) )
#define SYNCROMVMINOR(l)	( (UInt16)( (((UInt32)(l)) >> 20) & 0x0000000FL ) )
/*@}*/

/**
 *
 * @brief This class is used by the SyncCallDeviceApplication() method to pass information to and from
 *		a handheld application/module. 
 *
 * @note Used by the 'SyncCallDeviceApplication()' API for PalmOS v6.0 and greater.
 **/
class CCallApplicationParams
{
public:
	// PASSED VALUES:
    char        m_dbName[SYNC_DB_NAMELEN];  /**< A TCHAR pointer specifying the name of the target application */
	UInt32		m_dwCreatorID;		/**< A UInt32 value specifying the creator ID of the target application. */
	UInt32		m_dwTypeID;			/**< A UInt32 value specifying the type ID of the target application. */
    UInt16      m_wAttributes;      /**< A UInt16 combination of dmHdrAttr bits that specify whether the target app is a classic,
                                        extended or schema database. All other dmHdrAttr bits are ignored. */
	UInt16		m_wActionCode;		/**< A UInt16 value specifying the application-specific action code to perform. */
	UInt32		m_dwParamSize;		/**< A UInt32 value specifying the number of bytes in the m_pParam array. */
	void*		m_pParam;			/**< A void pointer to parameter block. */
	UInt32		m_dwResultBufSize;	/**< A UInt32 value specifying the total number of bytes in the m_pResultBuf array. */
	void*		m_pResultBuf;		/**< A void pointer to the result buffer. */

	// RETURNED VALUES
	UInt32		m_dwResultCode;		/**< A UInt32 value to receive the result code returned by remote application. */

	UInt32		m_dwActResultSize;	/**< A UInt32 value specifying the actual size of the result data. This value will 
										be greater than the m_dwResultBufSize value if your buffer was not large enough 
										to accommodate all of the results data. In this case, only dwResultBufSize bytes 
										of data are copied into the buffer.  */
	// RESERVED:
	UInt32		m_dwReserved;		/**< Reserved for future use. You must set this field to NULL (0) before 
															 calling the function. */
};

#define dmDBNameLength			(32)	/**< The maximum number of ASCII bytes in a database name including NULL terminator. */

/**
 *
 * @brief This structure is used to receive header information about databases.
 *
 **/
typedef struct DBDatabaseInfo {
	//
	// section 1
	UInt32				type;					/**< A UInt32 value specifying the type identifier value for the database. */
	UInt32				creator;				/**< A UInt32 value specifying the creator ID for the database. */
	UInt16				attributes;				/**< A UInt16 value specifying the database attributes, such as ... */
	UInt16				flags;					/**< A UInt16 value specifying the miscellaneous flags for the database. */
	UInt16 				version;				/**< A UInt16 value specifying the developer defined version of the database. */
	UInt32				createDate;				/**< A UInt32 value specifying the date when the database was created. */
	UInt32				modifyDate;				/**< A UInt32 value specifying the date when the database was last modified.*/
	UInt32				backupDate;				/**< A UInt32 value specifying the date of the most recent database backup.*/
	UInt32				modifyNumber;			/**< A UInt32 value specifying the database modification number. This number is incremented 
													every time a row in the database is added, modified, or deleted on the handheld.*/
	char				name[dmDBNameLength];	/**< A char array of dmDBNameLength bytes used to contain the null-terminated 
													name of the database. */
	//
	// section 2
	UInt32				rowCount;				/**< A UInt32 value specifying the number of rows in the database.*/
	UInt32				totalBytes;				/**< A UInt32 value specifying the total bytes of storage used by database, including overhead.*/
	UInt32				dataBytes;				/**< A UInt32 value specifying the total bytes of storage used for data.*/
	//
	// section 3
	UInt16				encoding;				/**< A UInt16 value specifying the type of character encoding of text data in the database.*/
	UInt32				tableCount;				/**< A UInt32 value specifying the number of tables in the database.*/
	char				dispName[dmDBNameLength];/**< A char array of dmDBNameLength used to contain the null-terminated display name 
													of the database. */
	// section 4
	UInt32				appBlkSize;				/**< A UInt32 value that contains the block size, in bytes, of the application information 
													block. This field is only filled in for the SyncDmReadOpenDbInfo/SyncDmFindDatabase calls when the 
													SYNC_DB_INFO_OPT_GET_SIZE option is set. */
	UInt32				sortBlkSize;			/**< A UInt32 value that contains the block size, in bytes, of the sort information block. 
													This field is only filled in for the SyncDmReadOpenDbInfo/SyncDmFindDatabase calls when the
													SYNC_DB_INFO_OPT_GET_SIZE option is set. */

	UInt32				maxRecSize;				/**< A UInt32 value that contains the size of the largest record or resource in the database. This 
													field is only filled in for the SyncDmReadOpenDbInfo/SyncDmFindDatabase call when the
													SYNC_DB_INFO_OPT_GET_MAX_REC_SIZE options is set. */
} DBDatabaseInfo;

#define SIZEOF_DB_DATABASE_INFO			(sizeof(DBDatabaseInfo))

/**
 * @defgroup DBDatabaseInfoFlags Bit Values For DBDatabaseInfo Structure 'flags' Field
 * These bit values are used for the 'flags' field in the DBDatabaseInfo structure. 
 **/
/*@{*/
#define dbFlagExcludeFromSync	((UInt16)0x0080)		/**< This flag is used to indicate that this database should be excluded from 
															the HotSync synchronization process. */

#define dbFlagRamBased			((UInt16)0x0040)		/**< This flag is used to indicate that this database is RAM-based. */
/*@}*/

/**
 * @defgroup DatabaseInfoRetrievalOptions Bit Values for Database Info Retrieval Options
 * These bit values are used with the bOptFlags field in structures SyncFindDbByNameParams,
 * SyncFindDbByTypeCreatorParams & SyncReadOpenDbInfoParams in the methods SyncFindDbByName, 
 * SyncFindDbByTypeCreator and SyncReadOpenDbInfo to determine which information for the 
 * databases should be retrieved. 
 **/
/*@{*/
#define SYNC_DB_INFO_OPT_GET_ATTRIBUTES			(0x80)	/**< Set this flag to indicate that database search operations are to retrieve database 
															attribute information. This is an option to allow find operations to skip returning 
															more data as a performance optimization. */

#define SYNC_DB_INFO_OPT_GET_SIZE				(0x40)	/**< Set this flag to indicate that database search operations are to retrieve 
															the record (or resource) size information. You can omit this flag to optimize 
															performance. This flag is used to get record count and data size. */
																
#define SYNC_DB_INFO_OPT_GET_MAX_REC_SIZE		(0x20)	/**< Set this flag to indicate that database search operations are to retrieve the
															maximum record (or resource) size information. You can omit this to optimize 
															performance. Note: this flag applies only to the SyncReadOpenDbInfo function. */
/*@}*/


/**
 * @defgroup SyncFindDatabaseByTypeCreatorSearchFlags Bit Values used in the 
 * SyncFindDbByTypeCreatorParams structure to specify how the seach should be performed. 
 **/
/*@{*/
#define SYNC_DB_SRCH_OPT_NEW_SEARCH				(0x80)	/**< This flag is used to indicate that a new search is being started. 
															Subsequent iterations of the same search should have the flag cleared. */

#define SYNC_DB_SRCH_OPT_ONLY_LATEST			(0x40)	/**< This flag is used to indicate that the search should be based on the latest version. */
/*@}*/

/**
 * @defgroup SyncDbCloseDatabasebOptFlags Bit Values For SyncCloseDBEx()
 * These bit values are used for the bOptFlags parameter of the SyncCloseDBEx() method.
 */
/*@{*/
#define SYNC_CLOSE_DB_OPT_UPDATE_BACKUP_DATE	0x80	/**< This flag is used to indicate that the database's 
															backup date is to be updated after it is closed. */
#define SYNC_CLOSE_DB_OPT_UPDATE_MOD_DATE		0x40	/**< This flag is used to indicate that the database's 
															modification date is to be updated after it is closed. */
/*@}*/




/**
 * 
 * @defgroup ErrorCodeClasses Error Code Classes
 *
 **/
/*@{*/
#define COND_ERR_CLASS						0x00001000L		/**< Error Code Classification: Conduit error codes. */
#define TRANS_ERR_CLASS						0x00002000L		/**< Error Code Classification:  Communications/Transport error class. */
#define SYNC_ERR_CLASS						0x00004000L		/**< Error Code Classification:  Sync Manager DLL error class. */
#define HSAPP_ERR_CLASS						0x00008000L		/**< Error Code Classification:  HotSync application error class. */
#define EXPAPI_ERR_CLASS					0x00002900L		/**< Error Code Classification:  VFSAPI error class for Expansion mgr calls. */
#define VFSAPI_ERR_CLASS					0x00002A00L		/**< Error Code Classification:  VFSAPI error class for vfs calls. */
/*@}*/


/**
 *
 * @defgroup SyncErrorCodes  Error Codes For Sync API Methods
 * These error codes are returned by the Sync API calls as a result of attempted
 * communication with the handheld device.
 *
 **/
/*@{*/
#define	SYNC_FATAL_ERR_MASK				0x10000000L
#define SYNC_FATAL_ERR					(SYNC_FATAL_ERR_MASK + SYNC_ERR_CLASS)

#define SYNCERR_NONE					0x0000					/**< The function call succeeded, without error.*/
#define SYNCERR_UNKNOWN					(SYNC_ERR_CLASS + 0x01)	/**< An unknown error occurred (local/remote
																	error code mapping does not exist). */
#define SYNCERR_MORE					(SYNC_ERR_CLASS + 0x02)	/**< There is data left unread.  */
#define SYNCERR_NOT_FOUND				(SYNC_ERR_CLASS + 0x03)	/**< The requested database, record, resource, etc.
																	could not be found. This error code replaces 
																	the earlier SYNCERR_FILE_NOT_FOUND error code.
																	Note: This result code is returned when 
																	iterating through a database to indicate that 
																	there are no more records to retrieve. */
#define SYNCERR_FILE_NOT_FOUND			(SYNCERR_NOT_FOUND)		/**< For backward compatibility. */
#define SYNCERR_FILE_NOT_OPEN			(SYNC_ERR_CLASS + 0x04)	/**< The attempt to open the database failed. */
#define SYNCERR_FILE_OPEN				(SYNC_ERR_CLASS + 0x05)	/**< NOT USED. */
#define SYNCERR_RECORD_BUSY				(SYNC_ERR_CLASS + 0x06)	/**< The requested record is in use by someone else 
																	and will remain so indefinitely. */
#define SYNCERR_RECORD_DELETED			(SYNC_ERR_CLASS + 0x07)	/**< The requested record has either been deleted or archived. */
#define SYNCERR_READ_ONLY				(SYNC_ERR_CLASS + 0x09)	/**< Your function does not have write access to 
																	the database, or the database is in ROM. This 
																	error code replaces the earlier 
																	SYNCERR_ROM_BASED error code. */
#define SYNCERR_ROM_BASED				(SYNCERR_READ_ONLY)		/**< Defined for backward compatility. */
#define SYNCERR_COMM_NOT_INIT			(SYNC_ERR_CLASS + 0x0A)	/**< An internal error code that indicates communications have 
																	not been initialized. */
#define SYNCERR_FILE_ALREADY_EXIST		(SYNC_ERR_CLASS + 0x0B)	/**< The database could not be created because 
																	another database with the same name already 
																	exists on the handheld. */
#define SYNCERR_FILE_ALREADY_OPEN		(SYNC_ERR_CLASS + 0x0C)	/**< The requested database is already opened. */
#define SYNCERR_NO_FILES_OPEN			(SYNC_ERR_CLASS + 0x0D)	/**< An operation was requested on a database, and there 
																	are not any open databases. */
#define SYNCERR_BAD_OPERATION			(SYNC_ERR_CLASS + 0x0E)	/**< The requested operation is not supported on 
																	the given database type (record or resource). */
#define SYNCERR_REMOTE_BAD_ARG			(SYNC_ERR_CLASS + 0x0F)	/**< An invalid argument has been passed to the handheld. */
#define SYNCERR_BAD_ARG_WRAPPER			(SYNC_ERR_CLASS + 0x10)	/**< An internal HotSync Manager synchronization API error 
																	that indicates a protocol implementation error. */
#define SYNCERR_ARG_MISSING				(SYNC_ERR_CLASS + 0x11)	/**< An invalid parameter has been passed to a function, or the parameter is too large. */
#define SYNCERR_LOCAL_BUFF_TOO_SMALL	(SYNC_ERR_CLASS + 0x12)	/**< The passed buffer is too small for the reply data. */
#define SYNCERR_REMOTE_MEM				(SYNC_ERR_CLASS + 0x13)	/**< There is insufficient memory on the handheld to receive or complete the request.*/
#define SYNCERR_REMOTE_NO_SPACE			(SYNC_ERR_CLASS + 0x14)	/**< There is insufficient memory in the data store on 
																	the handheld to complete the request. This generally 
																	occurs when attempting to write a record or resource 
																	to a handheld database. */
#define SYNCERR_REMOTE_SYS				(SYNC_ERR_CLASS + 0x15)	/**< A generic system error on the handheld. This is returned 
																	when the exact cause is unknown. */
#define SYNCERR_LOCAL_MEM				(SYNC_ERR_CLASS + 0x16)	/**< A memory allocation error occurred on the desktop computer. */
// NEW v2.1
#define SYNCERR_BAD_ARG					(SYNC_ERR_CLASS + 0x17)	/**< An invalid parameter has been passed to a function, or the parameter is too large. */
#define SYNCERR_LIMIT_EXCEEDED			(SYNC_ERR_CLASS + 0x18)	/**< A data limit has been exceeded on the handheld. For example, this happens 
																	when the HotSync error log size limit has been exceeded on the handheld. */
#define SYNCERR_UNKNOWN_REQUEST			(SYNC_ERR_CLASS + 0x19)	/**< This request (command) is not supported by the handheld. */
// v2.4
#define SYNCERR_ACCESS_DENIED			(SYNC_ERR_CLASS + 0x1A)	/**< Accesc denied by the Authorization Mgr on the device or
																	db could not be unencrypted. */
#define SYNCERR_DEVICE_NOT_CONNECTED	(SYNC_ERR_CLASS + 0x1B)	/**< Function cannot complete without the device. */ 
#define SYNCERR_INVALID_IMAGE			(SYNC_ERR_CLASS + 0x1C)	/**< The image being installed is not a valid PalmOS database. */
#define SYNCERR_PATH_NOT_FOUND			(SYNC_ERR_CLASS + 0x1D)	/**< The directory or file path is not valid. */
//
#define SYNCERR_DISK_FULL				(SYNC_ERR_CLASS + 0x1F)	/**< A write to the local (desktop) disk failed because
																	the disk is full. */
// Fatal Errors
#define SYNCERR_TOO_MANY_OPEN_FILES		(SYNC_FATAL_ERR + 0x403)					/**< Request failed because there are too many open databases 
																						(for efficiency, the Synchronization implementation supports only 
																						one open database at a time). This error code replaces the earlier 
																						SYNCERR_TOO_MANY_FILES error code. */
#define SYNCERR_TOO_MANY_FILES			SYNCERR_TOO_MANY_OPEN_FILES					/**< Defined for backward compatibility. */
#define SYNCERR_REMOTE_CANCEL_SYNC		(SYNC_FATAL_ERR + 0x405)					/**< The HotSync operation was cancelled by the handheld user. */
#define SYNCERR_LOST_CONNECTION			(SYNC_FATAL_ERR + TRANS_ERR_CLASS + 0x410)	/**< The connection with the handheld was lost. */
#define SYNCERR_LOCAL_CANCEL_SYNC		(SYNC_FATAL_ERR + 0x411)					/**< The request to cancel HotSync was initiated from the desktop. */
/*@}*/


// section 1 is used to receive header information about both db and dm style databases 
// from method SyncReadDatabaseList
//
// section 2 includes additional information about schema databases retrieval by methods
// SyncDbFindxxx 
//
// section 3 includes additional information about schema databases retrieval by method
// SyncDbReadOpenDatabaseInfo
//
// section 4 includes additional information about non-schema databases retrieval by methods
// SyncDmFindDatabase and SyncDmReadOpenDatabaseInfo

#ifdef __cplusplus
extern "C" {
#endif


// 1.0 CALLS:
HS_EXTERN_API( SInt32 ) 
/**
 *
 * This method is used to add an entry to the log on the handheld. 
 *
 * @param pText	[in] The null-terminated char array containing the log string. 
 *
 * @retval SInt32 The return value is a SInt32 value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_COMM_NOT_INIT 
 * @retval SYNCERR_LOST_CONNECTION 
 * @retval SYNCERR_REMOTE_SYS 
 * @retval SYNCERR_REMOTE_MEM 
 * @retval SYNCERR_REMOTE_BAD_ARG 
 * @retval SYNCERR_LIMIT_EXCEEDED 
 * @note You can use this function to add an entry to the message log on the handheld. Since the log has limited space,
 * keep your entries as short as possible. To include a new line in your log entry, use a single line-feed character (character code 0x0A).
 * Note that HotSync Manager automatically logs the general success or failure status of your conduit; thus, you need not add an entry for this purpose. 
 *
 **/
SyncAddLogEntry          (const char* pText);


HS_EXTERN_API( SInt32 ) 
/**
 *
 * This method is used to register a conduit that is about to start synchronization operations and returns a conduit handle for 
 * use with other Sync Manager functions. 
 *
 * @param rHandle  [out] A reference to a CONDHANDLE to receive the address of the handle to your conduit. 
 *
 * @retval SInt32 The return value is a SInt32 value specifying success or error.
 * @retval 0 If successful, this method returns 0, which means that you can proceed with synchronization. 
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval -1 The conduit could not be registered, because the previous conduit did not unregister itself 
 *		and you cannot proceed with synchronization. 
 * @retval SYNCERR_COMM_NOT_INIT 
 * @retval SYNCERR_LOST_CONNECTION 
 * @retval SYNCERR_REMOTE_CANCEL_SYNC 
 * @retval SYNCERR_LOCAL_CANCEL_SYNC 
 *
 * @note You must call the SyncRegisterConduit function to register your conduit before making any other Sync API calls.
 * If your call to SyncRegisterConduit succeeds, you must call the SyncUnRegisterConduit function after you finish synchronizing. 
 *
 * @sa SyncUnRegisterConduit().
 *
 **/
SyncRegisterConduit(CONDHANDLE &rHandle) ;

HS_EXTERN_API( SInt32 ) 
/**
 *
 * This method is used to unregister a conduit, that was successfully registered with a previous call to the 
 * SyncRegisterConduit() method, and clean up any system resources that the Sync system allocated for the conduit. 
 *
 * @param handle [in] A CONDHANDLE containing the handle that was returned from a previous call to the SyncRegisterConduit() method. 
 *
 * @retval SInt32 The return value is a SInt32 value specifying success or error.
 * @retval 0 If successful, this method returns 0, which means that your conduit was successfully unregistered. 
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval -1 If handle is not a valid conduit handle. 
 * @retval SYNCERR_COMM_NOT_INIT 
 *
 * @sa SyncRegisterConduit(). 
 **/
SyncUnRegisterConduit(CONDHANDLE handle);

HS_EXTERN_API( SInt32 ) 
/**
 *
 * This method is used to retrieve information about the user of the handheld, including the user name, 
 * last synchronization date, and encrypted password. 
 *
 * @param rInfo  [out] A reference to an object of the CUserIDInfo class. Upon successful return, the fields of this object are 
 *		filled with information about the user of the handheld. If the m_NameLength field of rInfo is 0 upon return, 
 *		then user information has not yet been established on the handheld. 
 *
 * @note User information is written to a new or reset handheld after completion of a synchronization. If this has not yet 
 *	occurred on the handheld, the user information is empty, and the m_NameLength field of rInfo is set to 0. 
 * @note When you call SyncReadUserId to read information from a handheld that is running a version of the Palm OS earlier 
 *	than version 3.0, the LastSyncDate field of the CUserIdInfo structure is set to 0. The LastSyncDate field is correctly 
 *	set to the most recent synchronization date for version 3.0 and later. 
 * 
 * @retval SInt32 The return value is a SInt32 value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_COMM_NOT_INIT 
 * @retval SYNCERR_LOST_CONNECTION 
 * @retval SYNCERR_REMOTE_SYS 
 * @retval SYNCERR_REMOTE_MEM 
 *
 * @sa SyncReadSystemInfo() 
 **/
SyncReadUserID(CUserIDInfo &rInfo);

HS_EXTERN_API( SInt32 ) 
/**
 *
 * This method is used to send a request to soft-reset the handheld at the end of synchronization operations. 
 *
 * @retval SInt32 The return value is a SInt32 value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_COMM_NOT_INIT 
 * @retval SYNCERR_LOST_CONNECTION 
 * @retval SYNCERR_REMOTE_SYS 
 *
 **/
SyncRebootSystem (void);

HS_EXTERN_API( SInt32 ) 
/**
 *
 * This method is used to retrieve the Palm OS software version and product information for the handheld. To use this function, 
 * allocate the m_ProductIdText buffer in an object of the CSystemInfo class and fill in the m_AllocedLen member of the object. 
 * You must fill in m_AllocedLen with the size of the buffer that you allocated and assigned to m_ProductIdText. This method 
 * stores the handheld system information into the buffer. The retrieved product ID is a binary (nonASCII) sequence of bytes. 
 * Currently, all handhelds return the same four bytes: 
 * @par
 *  0x00, 0x01, 0x00, 0x00 
 * @par
 * This function also fills in the m_RomSoftVersion, m_LocalId, and m_ProdIdLength fields. 
 *
 * @param rInfo				[in/out] An object of the CSystemInfo class, which contains information about the handheld. For 
 *								this function, you must first allocate the m_ProductIdText buffer and fill in the following fields 
 *								in the object: 
 * @param m_ProductIdText	[out] A BYTE array containing the data buffer in which returned data is stored. You must 
 *								allocate this buffer before calling the function. Allocate SYNC_MAX_PROD_ID_SIZE bytes for this buffer. 
 * @param m_AllocedLen		[in] A BYTE containing the number of bytes that you allocated in the m_ProductIdText buffer. 
 * @param m_RomSoftVersion  [out] A UInt32 value used to receive the ROM version of the handheld. Note: you can use the 
 *								SYNCROMVMAJOR(val) and SYNCROMVMINOR(val) macros to decode this value into the major and minor 
 *								version number values. 
 * @param m_LocalId			[out] A UInt32 value used to receive the localization ID for the handheld. Currently, all systems 
 *								return the value 0x00010000L. 
 * @param m_ProdIdLength	[out] A BYTE value used to receive the actual number of bytes stored into the m_ProductIdText buffer. 
 * @param m_dwReserved		[] A UInt32 value that should be set to 0.
 *
 * @retval SInt32 The return value is a SInt32 value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_COMM_NOT_INIT 
 * @retval SYNCERR_LOST_CONNECTION 
 * @retval SYNCERR_REMOTE_SYS 
 *
 * @sa SyncGetHHOSVersion(). 
 *
 **/
SyncReadSystemInfo       (CSystemInfo& rInfo);


HS_EXTERN_API( SInt32 ) 
/**
 *
 * This method is used to retrieve information about a memory card on the handheld. 
 *
 * @param rInfo			[in/out] An object of the CCardInfo class. Use the following fields with this function: 
 * @param m_CardNo		[in] A BYTE value containing the number of the card for which you want information. The 
 *							first card in the system is card number 0, and subsequent card numbers are incremented by 1.  
 * @param m_CardVersion [out] A UInt16 to receive the card format version. 
 * @param m_CreateDate  [out] A SInt32 value to receive the card creation date as a time_t value. This is 0 or -1 if the 
 *							date is not available. 
 * @param m_RomSize		[out] A UInt32 value to receive the total ROM size of the card, in bytes.
 * @param m_RamSize     [out] A UInt32 value to receive the total RAM size of the card, including storage and dynamic heaps, in bytes. 
 * @param m_FreeRam		[out] A UInt32 value to receive the amount of unused RAM on the card, in bytes; this value is different for 
 *							different versions of the operating systems, as described in the notes section. 
 * @param m_CardNameLen [out] A BYTE value containing the length of the card name string. 
 * @param m_ManufNameLen [out] A BYTE value containing the length of the card manufacturer name string. 
 * @param m_CardName	[out] An char array of size SYNC_REMOTE_CARDNAME_BUF_SIZE that is used to receive the card name string. 
 *							Note that this string is not null-terminated. 
 * @param m_ManufName   [out] An char array of size SYNC_REMOTE_MANUFNAME_BUF_SIZE that is used to receive the 
 *							manufacturer name string. Note that this string is not null-terminated. 
 * @param m_romDbCount  [out] A UInt16 value used to receive the number of ROM-based databases on the card. 
 * @param m_ramDbCount  [out] A UInt16 value used to receive the number of RAM-based databases on the card. 
 * @param m_dwReserved	[]	Reserved for future use. You must set this to NULL (0) before calling this function. 
 *
 * @retval SInt32 The return value is a SInt32 value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_COMM_NOT_INIT 
 * @retval SYNCERR_LOST_CONNECTION 
 * @retval SYNCERR_REMOTE_SYS 
 * @retval SYNCERR_REMOTE_MEM 
 * @retval SYNCERR_NOT_FOUND 
 * @retval SYNCERR_NOT_FOUND is returned if the specified card number is outside of the range of available cards. 
 *
 * @note You can use this function to determine the total number of ROM and RAM-based databases prior to calling the SyncReadDBList function. 
 * The value returned in the m_FreeRam field depends on the version of the Palm OS software running on the handheld: 
 * @par
 * • if the handheld is running version 3.0 or later, the value of m_FreeRam includes unused RAM in storage heaps only,
 *  which is the amount of memory available for records or resources.
 * @par 
 * • if the handheld is running an earlier version of the Palm OS software, the value of m_FreeRam is the sum of 
 * unused RAM in both storage and dynamic heaps. However, you can only store data in the storage heaps.
 *
 * @sa SyncReadDBList(). 
 *
 **/
SyncReadSingleCardInfo   (CCardInfo& rInfo );

HS_EXTERN_API( SInt32 ) 
/**
 *
 * This method is used to retrieve the current date and time, according to the system clock on the handheld. 
 *
 * @param rDate		[out] A SInt32 value used to receive the system date and time on the handheld, as a time_t value. 
 *						If an error occurs, the value of rDate is either -1 or 0. 
 *
 * @retval SInt32 The return value is a SInt32 value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_COMM_NOT_INIT 
 * @retval SYNCERR_LOST_CONNECTION 
 * @retval SYNCERR_REMOTE_SYS 
 *
 * @sa SyncWriteSysDateTime(). 
 **/
SyncReadSysDateTime      (SInt32& rDate);


HS_EXTERN_API( SInt32 ) 
/**
 *
 * This method is used to set the system date and time on the handheld. In general, conduits should avoid changing the system 
 * date and time. The SyncWriteSysDateTime function does not notify applications on the handheld that it has changed the time. 
 * Some applications, such as the built-in datebook, need to know when the system time changes so that they can adjust their 
 * alarm settings. To work around this problem, you need to call the SyncRebootSystem method, which will cause a soft-reset 
 * of the handheld after the HotSync operation completes. All applications on the handheld are notified of the reset and can 
 * make any necessary adjustments. 
 * 
 * @param lDate	[in] A time_t value that specifies the system date and time value. This value must be in the format 
 *					returned by the time function. 
 *
 * @retval SInt32 The return value is a SInt32 value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_COMM_NOT_INIT 
 * @retval SYNCERR_LOST_CONNECTION 
 * @retval SYNCERR_REMOTE_SYS 
 * @retval SYNCERR_REMOTE_BAD_ARG If you call this function in a version of the Sync Manager API earlier 
 *			than version 2.2, the SYNCERR_REMOTE_BAD_ARG error code is returned. 
 *
 * @note IMPORTANT: Although this function is available in all versions of the Sync Manager API, it does not 
 * work properly in any version earlier than version 2.2. 
 *
 * @sa SyncReadSysDateTime(). 
 *
 **/
SyncWriteSysDateTime     (SInt32  lDate);

HS_EXTERN_API( SInt32 ) 
/**
 *
 * This method is used to Causes HotSync Manager to update its progress display indicator. Call the SyncYieldCycles method 
 * periodically to keep HotSync Manager's progress display indicator current. If you neglect to call this function frequently 
 * enough, the user interface will appear to be frozen. SyncYieldCycles does not cause HotSync Manager to check whether 
 * the user has clicked the Cancel button, nor does it send keep-alive messages to the handheld to maintain connection 
 * with it during long periods in which your conduit is not accessing it.
 * 
 * @param wMaxMiliSecs [in] A UInt16 value containing the maximum number of milliseconds for HotSync Manager to spend 
 *						servicing events. This value is currently ignored; you should supply a value of 1.  
 *
 *
 * @retval SInt32 The return value is a SInt32 value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_COMM_NOT_INIT 
 *
 **/
SyncYieldCycles          (UInt16 wMaxMiliSecs);

// Sync API v2.0 CALLS ADDED HERE:

HS_EXTERN_API( SInt32 ) 
/**
 *
 * This method is used to Retrieves the version of the Sync Manager API that is installed on the desktop computer. 
 * You can use this information to determine which of the Sync Manager functions you can use on the desktop computer. 
 * @par
 * For information about Sync Manager API versions and their relationship to HotSync Manager versions, see "HotSync 
 * Manager and Sync Manager API Versions." 
 *
 * @param pdwMajor  [out] A pointer to a UInt32 value used to receive the major version number of the API on the desktop 
 *						computer. Specify NULL to ignore this value. 
 * @param pdwMinor  [out] A pointer to a UInt32 value used to receive the minor version number of the API on the desktop 
 *						computer. Specify NULL to ignore this value. 
 *
 * @retval SInt32 The return value is a SInt32 value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 *
 **/
SyncGetAPIVersion(UInt32* pdwMajor, UInt32* pdwMinor);

// Sync API v2.1 CALLS ADDED HERE:

HS_EXTERN_API( SInt32 ) 
/**
 *
 * This method is used to Retrieves a 32-bit feature value from the Feature Manager on the handheld. 
 * Features are stored in volatile storage that is erased and re-initialized during system reset. 
 * The Palm OS software and applications can register features using their own creator ID. The contents 
 * of features are completely application-specific. 
 *
 * @param dwFtrCreator	[in] A UInt32 value that contains the ID of the feature creator.  
 * @param wFtrNum		[in] A UInt16 value that contains the feature number.  
 * @param pdwFtrValue	[out] A pointer to a UInt32 value that retrieves value of the specified feature. 
 *
 * @retval SInt32 The return value is a SInt32 value specifying success or error.
 * @retval 0 If successful, this method returns 0, which means that the feature was retrieved. 
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_COMM_NOT_INIT 
 * @retval SYNCERR_LOST_CONNECTION 
 * @retval SYNCERR_REMOTE_SYS 
 * @retval SYNCERR_REMOTE_MEM 
 * @retval SYNCERR_UNKNOWN_REQUEST 
 * @retval SYNCERR_NOT_FOUND is returned if the requested feature could not be found, which indicates that it is not registered. 
 *
 **/
SyncReadFeature(UInt32 dwFtrCreator, UInt16 wFtrNum, UInt32* pdwFtrValue);


HS_EXTERN_API( UInt16 ) 
/**
 *
 * This method is used to retrieve the version number of the operating system on the handheld. 
 * You can use this information to determine which functions are available on the handheld. 
 *
 * @param pwRomVMinor	[out] A pointer to a UInt16 that receives the minor version number of the 
 *							operating system. You can pass NULL to ignore this value. 
 *
 * @retval UInt16 The return value is a UInt16 value specifying success or error.
 * @retval !=0 If successful, returns the major version number of the operating system on the handheld. 
 * @retval 0 If unsuccessful, generally indicates a lost connection. 
 *
 * @sa You can also determine the operating system version numbers by calling the SyncReadSystemInfo() and 
 * then using the SYNCROMVMINOR and SYNCROMVMINOR macros on the m_RomSoftVersion field of the CSystemInfo structure. 
 **/
SyncGetHHOSVersion(UInt16* pwRomVMinor);


HS_EXTERN_API( UInt16 ) 
/**
 *
 * This method is used to convert a Desktop's repesentation of UInt16 into the handheld's representation a UInt16.
 * This function performs byte swapping as required and returns the converted value as the function result. 
 *
 * @param wValue	[in] A UInt16 value in the desktop form. 
 *
 * @returns The UInt16 result of the conversion. This is the representation of the value on the handheld. 
 *
 * @sa .SyncHHToHostDWord(), SyncHHToHostWord(), and SyncHostToHHDWord(). 
 *
 **/
SyncHostToHHWord(UInt16 wValue);



HS_EXTERN_API( UInt16 ) 
/**
 *
 * This method is used to convert a handheld's repesentation of UInt16 into the desktop's representation a UInt16.
 * This function performs byte swapping as required and returns the converted value as the function result. 
 *
 * @param wValue	[in] The UInt16 value in the handheld form. 
 *
 * @returns The UInt16 result of the conversion. This is the representation of the value on the desktop computer. 
 *
 * @sa SyncHHToHostDWord(), SyncHostToHHDWord(), and SyncHostToHHWord(). 
 *
 **/
SyncHHToHostWord(UInt16 wValue);


HS_EXTERN_API( UInt32 ) 
/**
 *
 * This method is used to convert a desktop's representation of the specified UInt32 value into the handheld's representation of UInt32.
 * This function performs byte swapping as required and returns the converted value as the function result. 
 *
 * @param dwValue	[in] The UInt32 value in the desktop form. 
 *
 * @returns The UInt32 result of the conversion. This is the representation of the value on the handheld. 
 *
 * @sa SyncHHToHostDWord(), SyncHHToHostWord(), and SyncHostToHHWord(). 
 *
 **/
SyncHostToHHDWord(UInt32 dwValue);

HS_EXTERN_API( UInt32 ) 
/**
 *
 * This method is used to convert a handheld's representation of the specified UInt32 value into the desktop's representation of UInt32.
 *  This function performs byte swapping as required and returns the converted value as the function result. 
 *
 * @param dwValue	[in] The UInt32 value in the handheld form. 
 *
 * @returns The UInt32 result of the conversion. This is the representation of the value on the desktop computer. 
 *
 * @sa SyncHHToHostWord(), SyncHostToHHDWord(), and SyncHostToHHWord(). 
 *
 **/
SyncHHToHostDWord(UInt32 dwValue);


// Sync API v2.3 CALLS ADDED HERE:

HS_EXTERN_API( SInt32 ) 
/**
 * This method is used to test communication with the handheld.
 *
 * @param dwSizeSend	[in] A UInt32 value specifying the number of bytes in the pDataSend HSByte buffer.
 * @param pDataSend		[in] A pointer to a HSByte buffer containing the data to send.
 * @param pdwSizeRecv	[in/out] A pointer to a UInt32 value. [out] to receive the number of bytes received. 
 * @param pDataRecv		[out] A pointer to a HSByte buffer to receive the read data.
 *
 * @retval SInt32 The return value is a SInt32 value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 *
 **/
SyncLoopBackTest(UInt32 dwSizeSend, HSByte* pDataSend, UInt32* pdwSizeRecv, HSByte* pDataRecv);

// Sync API v2.4 CALLS ADDED HERE:

#ifndef __FORCE_PRE_SYNCVER_2_4_COMPATIBILITY__

/** 
 *
 * @brief This enum is used to specify whether the current desktop is trusted or not.
 *
 **/
enum eDesktopTrustStatus {
		eDesktopNotTrusted,  /**< The current desktop is not allowed to open secure databases. */
        eDesktopTrusted,	 /**< The current desktop is allowed to open secure databases. */
        eDesktopTrustNotVerified /**< The current desktop has not been verified for access to secure databases. */
		};

HS_EXTERN_API (HSError)
/**
 *
 * This method is used to retrieve the security level for the ongoing sync session.
 *
 *  @param trustStatus [out] A pointer to eDesktopTrustStatus value that receives the
 *		trust status of the desktop.
 *
 * @retval HSError The return value is a HSError value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_NONE - success
 * @retval SYNCERR_DEVICE_NOT_CONNECTED - this function can only be called during a sync operation.
 * @retval SYNCERR_UNKNOWN_REQUEST - Not supported on the device.
 *
 **/
SyncGetDesktopTrustStatus(eDesktopTrustStatus* pTrustStatus);

HS_EXTERN_API (HSError)
/**
 *
 * This method is used to determine whether the desktop backup file for a
 * specified handheld database is out of date.  The backup file is out of date
 * if the specified database is present on the handheld, has its backup bit set
 * and one or more of the following is true: 
 * (1) The backup file does not exist
 * (2) The backup file exists but is not a backup file for the specified
 *     database, either because it is not the correct type or because it does not
 *     have the correct name, creator id or attributes
 * (3) The handheld and desktop creation dates differ
 * (4) The handheld and desktop modification dates differ
 * (5) The handheld last backup date is zero
 *
 * @param hhInfo [in/out]  A reference to a DBDatabaseInfo structure. On input contains 
 *		the name, creator, type and attributes of the database to test. The attributes must
 *      specify whether to consider a classic, extended or schema database.  All
 *      other attribute bits are ignored.  On output contains values for section
 *      1 of the DBDatabaseInfo structure for the corresponding handheld
 *      database (if it exists). Note that the *output* type and attributes are
 *      used to generate the backup file name (if applicable). 
 * Windows:
 * @param filePath [in/out] A pointer to a TCHAR buffer. On input contains the desktop 
 *		file or directory path for the relevant backup image.  If filePath
 *      specifies a directory and the handheld database exists, the automatically generated 
 *		filename for the backup image is appended on output.  If filePath does not exist 
 *		it is assumed to specify a file name.
 *
 * Macintosh:
 * @param inParentRef  [in] Directory that contains the backup image.
 * @param ioFileName [in,out] If the length field is non-zero then use this as the
 *       name of the backup file. If zero on input then a * 
utomatically generate the 
 *		 file name and return the name in the parameter. 
 * @param dtInfo [out] A pointer to a DBDatabaseInfo structure. It may be set to Null. 
 *		On output contains values for section 1 of the DBDatabaseInfo structure for the 
 *		database image contained in the file specified by filePath (if it exists).
 * @param pDbExists [out] A pointer to a HSBool value which receives true if the specified handheld
 *                          database exists. May be set to NULL.
 * @param pFileExists [out] A pointer to a HSBool value which receives true if the specified backup image
 *                          file exists. May be set to NULL.
 * @param pBackupNeeded [out] A pointer to a HSBool value which receives true if the specified backup file
 *                          is out of date relative to the specified handheld
 *                          database (see above). May be set to NULL.
 *
 *
 * @retval HSError The return value is a HSError value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_NONE Success.
 * @retval SYNCERR_BAD_ARG Either hhInfo.name, hhInfo.creator or filePath
 *                                is invalid
 * @retval SYNC_ERR_TOO_MANY_OPEN_FILES Insufficient file handles available on the
 *                                desktop 
 * @retval SYNCERR_DEVICE_NOT_CONNECTED This function can only be called during a sync
 *                                operation.
 * @retval SYNCERR_UNKNOWN An unknown system error occurred.
 * @retval SYNCERR_UNKNOWN_REQUEST Not supported on the device.
 *
 **/
#if _WINDOWS
SyncIsDatabaseBackupNeeded(DBDatabaseInfo& hhInfo, TCHAR* filePath,
                           DBDatabaseInfo* pDtInfo, HSBool* pDbExists,
                           HSBool* pFileExists, HSBool* pBackupNeeded);
                           
#endif

#if TARGET_OS_MAC
SyncIsDatabaseBackupNeeded(DBDatabaseInfo& hhInfo, const FSRef *inParentRef,
         HFSUniStr255& ioFileName,
                           DBDatabaseInfo* pDtInfo, HSBool* pDbExists,
                           HSBool* pFileExists, HSBool* pBackupNeeded);
                           
#endif

HS_EXTERN_API (HSError)
/**
 *
 * This method is used to generate the unique backup file name derived from the input database creator,
 * type, name and attributes.
 *
 * @param info [in] A reference to DBDatabaseInfo structure. It contains the name, creator, type and
 *			attributes of the relevant database.  The attributes must specify whether to
 *			consider a classic record, classic resource, extended record,
 *			extended resource or schema database.  All other attribute bits are
 *			ignored.
 * Windows:
 * @param fileName [out] A TCHAR buffer of size _MAX_FNAME or larger.  On output
 *          contains the generated file name.
 *
 * Macintosh:
 * @param oFileName [out] A pointer to a HFSUniStr255 record. On output contains the generated file name.
 *
 * @retval HSError The return value is a HSError value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_NONE success.
 * @retval SYNCERR_BAD_ARG Either fileName, info.creator, info.type, info.dbname, info.attributes are invalid.
 *
 **/
#if _WINDOWS
SyncGenerateBackupFileName(DBDatabaseInfo& info, TCHAR* fileName);
#endif

#if TARGET_OS_MAC
SyncGenerateBackupFileName(DBDatabaseInfo& info, HFSUniStr255* oFileName);
#endif

HS_EXTERN_API (HSError)
/**
 *
 * This method is used to retrieve the database header information from the specified backup image file.
 *
 * @param info [out] A reference to a DBDatabaseInfo structure. On output contains values for section 1 of
 *		the DBDatabaseInfo structure for the database image contained in the file specified by filePath.
 *
 * Windows:
 * @param filePath [in] An array of TCHAR values that contain the path and name of the relevant backup
 *                          image file.
 * Macintosh:
 * @param inParentRef [in] A const pointer to a FSRef specifying the directory containing the file.
 * @param inFileName [in] A const HFSUniStr255 value containing the name of the relevant backup image file.
 *
 * @retval HSError The return value is a HSError value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_NONE success.
 * @retval SYNCERR_BAD_ARG filePath is invalid.
 * @retval SYNCERR_PATH_NOT_FOUND filePath does not exist.
 * @retval SYNC_ERR_TOO_MANY_OPEN_FILES Insufficient file handles available on the desktop
 * @retval SYNCERR_INVALID_IMAGE filePath does not contain a valid PalmOS database image.
 *
 **/
#if _WINDOWS
SyncReadBackupImageInfo(TCHAR* filePath, DBDatabaseInfo& info);
#endif
    
#if TARGET_OS_MAC
SyncReadBackupImageInfo(const FSRef* inParentRef, ConstHFSUniStr255Param inFileName,
      DBDatabaseInfo& info);
#endif

HS_EXTERN_API (HSError)
/**
 *
 * This method is used to install a database from the image on the desktop
 * specified by filePath.
 *
 * @param filePath          [in]     A TCHAR pointer to a null terminated string
 * containing the source file location.
 * @param pDbInfo           [out]    Nullable.  Pointer to a DBDatabaseInfo
 * structure that contains the relevant database information.  Only valid when
 * SYNCERR_NONE is returned.
 *
 * @retval HSError The return value is a HSError value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_NONE success.
 * @retval SYNCERR_INVALID_IMAGE The image being installed is not a valid PalmOS db.
 * @retval SYNCERR_FILE_ALREADY_OPEN The operation failed because the database
 * was already open on the device 
 * @retval SYNCERR_REMOTE_NO_SPACE Insufficient memory on the device.
 * @retval SYNCERR_ACCESS_DENIED The image could not be unencrypted successfully.
 * @retval SYNCERR_NOT_FOUND Desktop file (image) not found or could not be read.
 * @retval SYNCERR_DEVICE_NOT_CONNECTED this function can only be called during
 * a sync operation.
 * @retval SYNCERR_UNKNOWN_REQUEST Not supported on the device.
 *
 **/
#if _WINDOWS
SyncInstallDatabase(TCHAR* filePath, DBDatabaseInfo* pDbInfo = 0);
#endif
#if TARGET_OS_MAC
SyncInstallDatabase(const FSRef *inFile, DBDatabaseInfo* pDbInfo = 0 );
#endif

HS_EXTERN_API (HSError)
/**
 *
 * This method is used to install the desktop database image specified by
 * filePath, then back it up to the location specified by backupPath.  This is
 * significantly faster than performing the two operations separately.  The
 * backup is not performed unless the database backup bit is set.  The method
 * return value and output parameters indicate the status or failure of each
 * operation.
 *
 * @param filePath          [in]     A TCHAR pointer to a null terminated string
 * that specifies the database to install.
 * @param backupPath        [in/out] Nullable.  A TCHAR pointer to a null
 * terminated string that specifies where to place the backup.  If backupPath is
 * null, the default backup path is used.  If backupPath is a directory, the
 * backup file name is generated and appended to backupPath.
 * @param pIsInstalled      [out]    Nullable.  Pointer to a boolean indicating
 * whether the database was successfully installed to the handheld.  A return
 * value of SYNCERR_NONE implies *pIsInstalled is true.  When an error is
 * returned and *pIsInstalled is false, the error occurred during installation;
 * this implies that *pDbInfo has not been updated.
 * @param pDbInfo           [out]    Nullable.  Pointer to a DBDatabaseInfo
 * structure that contains the relevant database information.  *pDbInfo is only
 * valid when *pIsInstalled is true.  When SYNCERR_NONE is returned and
 * (pDbInfo->attributes & dmHdrAttrBackup) is non-zero, the database was
 * successfully backed up.  When an error is returned and *pIsInstalled is true,
 * the error occurred during backup.
 *
 * @retval HSError The return value is a HSError value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_NONE success.backupPath != 0
 * @retval SYNCERR_INVALID_IMAGE The image being installed is not a valid PalmOS
 * db. 
 * @retval SYNCERR_FILE_ALREADY_OPEN The operation failed because the database
 * was already open on the handheld.
 * @retval SYNCERR_REMOTE_NO_SPACE Insufficient memory on the device.
 * @retval SYNCERR_ACCESS_DENIED The image could not be unencrypted successfully.
 * @retval SYNCERR_NOT_FOUND The desktop file (image) not found or could not be
 * read.
 * @retval SYNCERR_DEVICE_NOT_CONNECTED This function can only be called during
 * a sync operation. 
 * @retval SYNCERR_UNKNOWN_REQUEST Not supported on the device.
 *
 **/
#if _WINDOWS
SyncInstallAndBackupDatabase(TCHAR*             filePath,
                             TCHAR*             backupPath,
                             HSBool*            pIsInstalled,
                             DBDatabaseInfo*    pDbInfo);
#endif
    
#if TARGET_OS_MAC
SyncInstallAndBackupDatabase(	const FSRef* inFile,
								const FSRef* inBackupDirectory,
								HSBool* outIsInstalled,
								DBDatabaseInfo*	outDbInfo );
#endif

HS_EXTERN_API (HSError)
/**
 *
 * This method is used to backup the security data. The AZM stores all the relevant information 
 * to support protected dbs such as HEKs, rules, tokens, etc. in (1 or more) protected dbs called vaults.
 * These must be Backed up at every sync session. The vault should be easily
 * identified on the desktop so that they can be restored first. (1)give them a
 * unique file extension or (2) Put them in a separate folder.
 * To successfully restore a secure db to a new device, the vault db(s) must be 
 * restored first. 
 *
 *
 * @param filePath [in] A TCHAR pointer to a null terminated string specifying the destination directory 
 *		for the vault dbs.  Filenames will be generated by the system.
 *
 * @retval HSError The return value is a HSError value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_NONE success.
 * @retval SYNCERR_PATH_NOT_FOUND The directory location pointed to by filePath does not exist.
 * @retval SYNCERR_ACCESS_DENIED access denied by the AubackupPath != 0thorization Mgr on the device
 *                         (e.g) cannot access a protected db at a non-trusted desktop.
 * @retval SYNCERR_LOCAL_MEM Desktop memory error.
 * @retval SYNCERR_UNKNOWN_REQUEST Not supported on the device.
 *
 **/
#if _WINDOWS
SyncBackupSecurityData(TCHAR* filePath);
#endif

#if TARGET_OS_MAC
SyncBackupSecurityData(const FSRef *inParentDirectory);
#endif

HS_EXTERN_API (HSError)
/**
 * This method is used to restore security data from the specified directory in the required order.
 * The directory must contain security data backed up via a prior call to
 * SyncBackupSecurityData.  The directory may contain other files as long as
 * they do not use the file extension used for security data files.
 *
 * @param dir [in] A TCHAR pointer to a null terminated string specifying the source file location.
 *
 * @retval HSError The return value is a HSError value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_NONE Success.
 * @retval SYNCERR_NOT_FOUND The input directory either does not exist or
 *		does not contain the required security data files.
 * @retval SYNCERR_UNKNOWN An unknown error has occurred
 * @retval SYNCERR_UNKNOWN_REQUEST Not supported on the handheld
 * @retval SYNCERR_ACCESS_DENIED Access denied by the handheld Authorization
 *                                Mgr
 *
 **/
#if _WINDOWS
SyncRestoreSecurityData(TCHAR* dir);
#endif
#if TARGET_OS_MAC
SyncRestoreSecurityData(const FSRef *inParentDirectory);
#endif

#endif // __FORCE_PRE_SYNCVER_2_4_COMPATIBILITY__

// Sync API v2.5 CALLS ADDED HERE:

HS_EXTERN_API (HSError)
/**
 *
 * This method is used to create or update a backup image file for the handheld
 * database specified by dbInfo.  If filePath is a directory, the backup file
 * name is generated from the database creator, type, name and attributes.  The
 * backup is performed if filePath is writeable and EITHER (1) the corresponding
 * call to SyncIsDatabaseBackupNeeded returns true OR (2) bAlwaysBackup is true.
 *
 * @param dbInfo [in/out] On input, a reference to a DBDatabaseInfo structure
 * containing the name, creator, type and attributes of the database to backup.
 * The attributes must specify whether to backup a classic, extended or schema
 * database. All other attribute bits are ignored. On output contains values for
 * section 1 of the DBDatabaseInfo structure for the corresponding handheld
 * database (if it exists). Note that the *output* type and attributes are used
 * to generate the backup file name (if applicable).  
 * @param filePath [in] A TCHAR pointer to a null terminated string containing
 * destination directory or file 
 * @param bAlwaysBackup [in] A BOOL value specifying to backup the specified
 * file even when its modification date has not changed (TRUE).
 *
 * @retval HSError The return value is a HSError value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_NONE  success.
 * @retval SYNCERR_BAD_ARG  creator, type, dbname or filePath are invalid.
 * @retval SYNCERR_NOT_FOUND The specified database is not present on the device.
 * @retval SYNCERR_ACCESS_DENIED Access to the specified database was denied by the
 *                         device Authorization Mgr, for example, because the
 *                         database is protected and the desktop is not trusted.
 * @retval SYNCERR_PATH_NOT_FOUND Neither filePath nor its parent directory exist.
 * @retval SYNC_ERR_TOO_MANY_OPEN_FILES - Insufficient file handles available on the
 *                                desktop.
 * @retval SYNCERR_READ_ONLY filePath is read only.
 * @retval SYNCERR_LOCAL_MEM The function was unable to allocate desktop memory.
 * @retval SYNCERR_DEVICE_NOT_CONNECTED This function can only be called during a sync
 *                                operation.
 * @retval SYNCERR_DISK_FULL The disk drive specified in filePath does not have enough
 *                     free space.
 * @retval SYNCERR_UNKNOWN_REQUEST Not supported on the device.
 *
 **/
#if _WINDOWS
SyncBackupDatabase(DBDatabaseInfo& dbInfo, TCHAR* filePath, HSBool bAlwaysBackup);
#endif

#if TARGET_OS_MAC
SyncBackupDatabase(DBDatabaseInfo&  dbInfo, const FSRef *inParentRef,
					ConstHFSUniStr255Param inFileName, HSBool inAlwaysBackup );
#endif
    
HS_EXTERN_API( SInt32 ) 
/**
 *
 * This method calls a handheld application (executable) and returns 
 * data and status information to your conduit from that application.
 * This API is similar to SyncCallRemoteModule(), but allows the
 * target application to be uniquely identified by creator, name and
 * database type (classic, schema or extended).
 * @par
 * Note that almost all conduits can accomplish their tasks without needing to
 * use this function, which is provided as a "back door" function. PalmSource,
 * Inc. recommends not using this function unless absolutely essential.  
 * @par
 * You can use the parameter block to send arbitrary data to the
 * application. The application can store variable-sized information into the
 * parameter block, which you can examine when the call completes.  
 * @par
 * Note that the format of the data and the action codes are completely
 * application specific. The handheld application that you call must have the
 * same structure as a Palm OS application; however, the application
 * can have a proprietary type ID so that it does not show up in the
 * launcher.
 *
 * @param pParams			[in/out] An object of the CCallApplicationParams class, which contains information for the 
 *								called application, and returns information to you from the called application, in the following
 *                              fields:  
 * @param m_dbName          [in] A TCHAR pointer specifying the name of the target application
 * @param m_dwCreatorId		[in] A UInt32 value containing the creator ID of the target application. 
 * @param m_wAttributes     [in] A UInt16 combination of dmHdrAttr bits that specify whether the target app is a classic,
 *                              extended or schema database. All other dmHdrAttr bits are ignored.
 * @param m_dwTypeID		[in] A UInt32 value containing the type ID of the target application.  m_dwTypeID is used as a cross
 *                              check and may be set to zero for "don't care".
 * @param m_wActionCode		[in] A UInt16 value containing the action code selector. This value is specific to the 
 *								module that you are calling. 
 * @param m_dwParamSize		[in] A UInt32 value specifying the parameter block size. 
 * @param m_pParam			[in] A void pointer to the parameter block. 
 * @param m_dwResultBufSize [in] A UInt32 value specifying the size of the results buffer, in bytes. 
 * @param m_pResultBuf		[out] An array of bytes of length m_dwResultBufSize that receives the results of the method. The caller must
 *								allocate this buffer.
 * @param m_dwResultCode	[out] A UInt32 value that receives the result code returned by the handheld module. 
 * @param m_dwActResultSize [out] A UInt32 value that receives the actual data size returned by the handheld module. 

 *
 * @retval SInt32 The return value is a SInt32 value specifying success or error.
 * @retval 0 If successful, this method returns 0.
 * @retval <0 If unsuccessful, this method returns negative error code value.
 * @retval SYNCERR_COMM_NOT_INIT 
 * @retval SYNCERR_LOST_CONNECTION 
 * @retval SYNCERR_REMOTE_SYS 
 * @retval SYNCERR_REMOTE_MEM 
 * @retval SYNCERR_UNKNOWN_REQUEST is returned if the handheld module was not found, or if the 
 *				handheld module did not handle the action code. 
 * @retval SYNC_LOCAL_BUFF_TOO_SMALL is returned if your results buffer was not large enough to contain 
 *				the results data. If this is the case, then upon return the value of m_dwActResultSize 
 *				will be greater than the value of m_dwResultBufSize, and only m_dwResultBufSize bytes were 
 *				copied to the results buffer. 
 *
 **/
SyncCallDeviceApplication(CCallApplicationParams* pParams);


#ifdef __cplusplus
}
#endif

#if TARGET_OS_MAC
	#pragma options align= reset
	#pragma enumsalwaysint reset
	typedef  long (*PROGRESSFN) (char*);
#else
	#pragma pack()
#endif

#endif // __SYNCCOMMON_PUBLIC_API__
