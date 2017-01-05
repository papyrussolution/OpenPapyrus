#ifndef _CATALOG_H_INCLUDED
/*************************************************************************
**
**  Copyright 1998-1999 Pervasive Software Inc. All Rights Reserved
**
*************************************************************************/
/*************************************************************************
*   CATALOG.H
*
*
*   This header prototypes the Database Names and Named Database functions
*   of the Pervasive Distributed Tuning Interface.
*
*     The following functions are found in this file:
*
*       PvGetDbNamesData()
*       PvFreeDbNamesData()
*       PvGetDbName()
*       PvCreateDatabase()
*       PvDropDatabase()
*       PvModifyDatabase()
*       PvCheckDbInfo()
*       PvGetDbServerName()
*       PvGetDbFlags()
*       PvGetDbDictionaryPath()
*       PvGetDbDataPath()
*       PvGetDSN()
*       PvCountDSNs()
*       PvListDSNs()
*       PvGetEngineInformation()
*
*
*************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


// Database flags
 
#define  P_DBFLAG_BOUND       0x00000001  /* bound database */
#define  P_DBFLAG_RI          0x00000002  /* relational integrity */
#define  P_DBFLAG_CREATE_DDF  0x00000004  /* create ddf flag */           
#define  P_DBFLAG_NA          0x80000000  /* not applicable */
#define  P_DBFLAG_DEFAULT  \
            (P_DBFLAG_BOUND | \
             P_DBFLAG_RI)

//catalog constants
#define	BDB_SIZE_DBNAME	20
#define DB_FLAG_SIZE    10

#define DSN_NAME_LEN    20
#define DSN_DESC_LEN    255
#define DSN_DBQ_LEN     255

#define DICT_PATH_LEN   255
#define DATA_PATH_LEN   512
#define SRVR_NAME_LEN   30

// component version 
#define MKDE_CLNT_API_STRUCT_VER	2
#define MKDE_CLNT_MAJOR_VER			7
#define MKDE_CLNT_MINOR_VER			50
#define MKDE_CLNT_PLEVEL			0

#define MKDE_SRVR_API_STRUCT_VER	2
#define MKDE_SRVR_MAJOR_VER			7
#define MKDE_SRVR_MINOR_VER			50
#define MKDE_SRVR_PLEVEL			0


// DSN Open Mode String
#define NORMAL_MODE_STR         "0"
#define ACCELERATED_MODE_STR    "-1"
#define READONLY_MODE_STR       "1"
#define EXCLUSIVE_MODE_STR      "-4"

#define MAX_DSN_NAME_LENGTH     32
#define DSN_OPENMODE_LEN        20
#define DSN_DESC_LEN            255
#define DSN_DBQ_LEN             255

// Server Client information
#define UNKNOWN_ENGINE_CLIENT   0
#define NT_SERVER               1
#define NW_SERVER               2
#define WIN32_CLIENT            3
#define UNIX_SERVER             4
#define CACHE_ENGINE_CLIENT     5


// DSN Open Mode

#define NORMAL_MODE         0
#define ACCELERATED_MODE    1
#define READONLY_MODE       2
#define EXCLUSIVE_MODE      3

/*
 * Name:
 *    PvGetDbNamesData()
 *
 * Description:
 *    Retrieve all the database names for a connected server.  This function
 *    should be called first before calling any other functions to get
 *    information on database names.  The caller should call
 *    PvFreeDbNamesData() to free the resources allocated for database names.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pCount         [out]     Address of an usinged long to receive the
 *                             number of database names on the server
 *
 * See Also:
 *    The following published functions may also be used to get database
 *    names, see Pervasive.SQL Programmer's Reference for more details.
 *       SQLGetCountDatabaseNames()
 *       SQLGetCountRemoteDatabaseNames()
 *       SQLGetDatabaseNames()
 *       SQLGetGetRemoteDatabaseNames()
 *       SQLUnloadDBNames()
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_FAIL                 Failed for any other reason
 */
BTI_API PvGetDbNamesData(
   BTI_LONG       hConnection,
   BTI_ULONG_PTR  pCount);


/*
 * Name:
 *    PvFreeDbNamesData()
 *
 * Description:
 *    Free the resource allocated for database names on a connected server.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Database names data retrieved by calling PvGetDbNamesData();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_DATA_UNAVAILABLE     Data related to database names not available
 *    P_E_FAIL                 Failed for any other reason
 */
BTI_API PvFreeDbNamesData(
   BTI_LONG hConnection);


/*
 * Name:
 *    PvGetDbName()
 *
 * Description:
 *    Get the name of a database on a connected server.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Database names data retrieved by calling PvGetDbNamesData();
 *    Caller has a valid database name sequence number;
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the service
 *    sequence        [in]     The sequence number (1 based) of the database
 *                             name.  Must be within a valid range with upper
 *                             limit defined by PvGetDbNamesData()
 *    pBufSize        [in/out] Address of an unsigned long containing size
 *                             of buffer allocated to receive the database
 *                             name.  The maximum size is defined by DSN_NAME_LEN constant.
 *							   Receives actual size of chars copied.
 *                             The size should include the null terminator.
 *    dbName          [out]    String value returned
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_DATA_UNAVAILABLE     Data related to database names not available
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_BUFFER_TOO_SMALL     Allocated buffer is too small for the string,
 *                             returned string is truncated.  In this case
 *                             the required size is retured in pBufSize.
 *    P_E_INVALID_SEQUENCE     Sequence number is not valid
 *    P_E_FAIL                 Failed for any other reason
 */
BTI_API PvGetDbName(
   BTI_LONG          hConnection,
   BTI_ULONG         sequence,
   BTI_ULONG_PTR     pBufSize,
   BTI_CHAR_PTR      dbName);


/*
 * Name:
 *    PvCreateDatabase()
 *
 * Description:
 *    Create a named database using the specified information for dictionary
 *    and data paths, and the database flag.
 *
 * Precondition:
 *    Connection established by calling PvConnectService();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    dbName         [in]      name of the database
 *    dictPath       [in]      dictionary path
 *    dataPath       [in]      data path.  Set it to NULL to use the default
 *                             data path (same as dictionary path)
 *    dbFlags        [in]      database flags, which can be a combination of
 *                             the P_DBFLAG_ constants
 *
 * Return Value:
 *    P_OK                          Successful
 *    P_E_INVALID_HANDLE            Invalid connection handle
 *    P_E_NULL_PTR                  Call with NULL pointer
 *    P_E_ACCESS_RIGHT              Insufficient access right for the operation
 *    P_E_DUPLICATE_NAME            Named database already exists on the server
 *    P_E_DICTIONARY_ALREADY_EXISTS Dictionary path is used by another database
 *    P_E_SHARED_DDF_EXIST          DDF files already exist at dictionary path
 *    P_E_FAIL                      Failed to create the database
 */
BTI_API PvCreateDatabase(
   BTI_LONG       hConnection,
   BTI_CHAR_PTR   dbName,
   BTI_CHAR_PTR   dictPath,
   BTI_CHAR_PTR   dataPath,
   BTI_ULONG      dbFlags);


/*
 * Name:
 *    PvDropDatabase()
 *
 * Description:
 *    Delete a named database.
 *
 * Precondition:
 *    Connection established by calling PvConnectService();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    dbName         [in]      name of the database
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_ACCESS_RIGHT         Insufficient access right for the operation
 *    P_E_NOT_EXIST            Named database does not exist
 *    P_E_DICT_IN_USE		   Data dictionary is in use and cannot be deleted 
 *    P_E_FAIL                 Failed to create the database
 */
BTI_API PvDropDatabase(
   BTI_LONG       hConnection,
   BTI_CHAR_PTR   dbName,
   BTI_CHAR       option      
   );


/*
 * Name:
 *    PvModifyDatabase()
 *
 * Description:
 *    Modify an existing named database using the specified information for
 *    the new database name, dictionary & data paths, and the database flag.
 *
 * Precondition:
 *    Connection established by calling PvConnectService();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    dbNameExisting [in]      name of the existing database
 *    dbNameNew      [in]      name of the new database.  Set it to NULL if
 *                             the database name is not to be changed.
 *    dictPath       [in]      new dictionary path.  Set it to NULL if the
 *                             dictionary path is not to be changed.
 *    dataPath       [in]      new data path.  Set it to NULL if the data
 *                             path is not to be changed.
 *    dbFlags        [in]      database flags, which can be a combination of
 *                             the P_DBFLAG_ constants.  Set to P_DBFLAG_NA
 *                             if database flags are not to be changed.
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer(for existing DB name)
 *    P_E_ACCESS_RIGHT         Insufficient access right for the operation
 *    P_E_NOT_EXIST            Named database does not exist
 *    P_E_FAIL                 Failed to create the database
 */
BTI_API PvModifyDatabase(
   BTI_LONG       hConnection,
   BTI_CHAR_PTR   dbNameExisting,
   BTI_CHAR_PTR   dbNameNew,
   BTI_CHAR_PTR   dictPath,
   BTI_CHAR_PTR   dataPath,
   BTI_ULONG      dbFlags);


/*
 * Name:
 *    PvCheckDbInfo()
 *
 * Description:
 *    Check database info for a named database.
 *
 * Precondition:
 *    Connection established by calling PvConnectService();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    dbName         [in]      name of an existing database
 *    checkFlags     [in]      flags indicating what to check.
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer(for existing DB name)
 *    P_E_ACCESS_RIGHT         Insufficient access right for the operation
 *    P_E_NOT_EXIST            Named database does not exist
 *    P_E_FAIL                 Failed to create the database
 *
 */
BTI_API PvCheckDbInfo(
   BTI_LONG       hConnection,
   BTI_CHAR_PTR   dbName,
   BTI_ULONG      checkFlags);


/*
 * Name:
 *    PvGetDbServerName()
 *
 * Description:
 *    Retrieve the name of the server where the named database resides.
 *
 * Precondition:
 *    Connection established by calling PvConnectService();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    dbName         [in]      database name
 *    pBufSize       [in/out]  Address of an unsigned long containing size
 *                             of the buffer.  The maximum length for server name is SRVR_NAME_LEN.
 *                             Receives actual size of
 *                             server name returned.
 *    serverName     [out]     Contain server name if successful, empty
 *                             string otherwise
 *    pIsLocal       [out]     Non-zero for local server
 *                             Zero for remote server
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_BUFFER_TOO_SMALL     The buffer is too small for the string.
 *                             In this case the required buffer size is
 *                             returned in pBufSize.
 *    P_E_NOT_EXIST            Named database does not exist
 *    P_E_FAIL                 Failed to retrieve database name
 */
BTI_API PvGetDbServerName(
   BTI_LONG       hConnection,
   BTI_CHAR_PTR   dbName,
   BTI_ULONG_PTR  pBufSize,
   BTI_CHAR_PTR   serverName,
   BTI_SINT_PTR   pIsLocal);


/*
 * Name:
 *    PvGetDbFlags()
 *
 * Description:
 *    Retrieve the database flags associated with a named database
 *
 * Precondition:
 *    Connection established by calling PvConnectService();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    dbName         [in]      database name
 *    pDbFlags       [out]     database flags, which can be a combination of
 *                             the P_DBFLAG_ constants
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_NOT_EXIST            Named database does not exist
 *    P_E_FAIL                 Failed to retrieve database flags
 */
BTI_API PvGetDbFlags(
   BTI_LONG       hConnection,
   BTI_CHAR_PTR   dbName,
   BTI_ULONG_PTR  pDbFlags);


/*
 * Name:
 *    PvGetDbDictionaryPath()
 *
 * Description:
 *    Retrieve the dictionary path (where DDF files reside) of a named
 *    database
 *
 * Precondition:
 *    Connection established by calling PvConnectService();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    dbName         [in]      database name
 *    pBufSize       [in/out]  Address of an unsigned long containing size
 *                             of the buffer.  Note that the maximum size is DICT_PATH_LEN.  
 *                             Receives actual size of
 *                             the path returned.
 *    dictPath       [out]     Contain the dictionary path if successful,
 *                             empty string otherwise
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_BUFFER_TOO_SMALL     The buffer is too small for the string.
 *                             In this case the required buffer size is
 *                             returned in pBufSize.
 *    P_E_NOT_EXIST            Named database does not exist
 *    P_E_FAIL                 Failed to retrieve dictionary path
 *
 */
BTI_API PvGetDbDictionaryPath(
   BTI_LONG       hConnection,
   BTI_CHAR_PTR   dbName,
   BTI_ULONG_PTR  pBufSize,
   BTI_CHAR_PTR   dictPath);


/*
 * Name:
 *    PvGetDbDataPath()
 *
 * Description:
 *    Retrieve the data path (where data files reside) of a named database
 *
 * Precondition:
 *    Connection established by calling PvConnectService();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    dbName         [in]      database name
 *    pBufSize       [in/out]  Address of an unsigned long containing size
 *                             of the buffer.  Note that the maximum size is DATA_PATH_LEN.  
 *                             Receives actual size of database name returned.
 *    dataPath       [out]     Contain the data path if successful, empty
 *                             string otherwise
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_BUFFER_TOO_SMALL     The buffer is too small for the string.
 *                             In this case the required buffer size is
 *                             returned in pBufSize.
 *    P_E_NOT_EXIST            Named database does not exist
 *    P_E_FAIL                 Failed to retrieve data path
 */
BTI_API PvGetDbDataPath(
   BTI_LONG       hConnection,
   BTI_CHAR_PTR   dbName,
   BTI_ULONG_PTR  pBufSize,
   BTI_CHAR_PTR   dataPath);


/*
 * Name:
 *    PvGetDSN()
 *
 * Description:
 *    Retrieve information about DSN
 *
 * Precondition:
 *    Connection established by calling PvConnectService();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    dsnName        [in]      DSN name
 *    pdsnDescSize   [in/out]  Address of an unsigned long containing size
 *                             of the buffer for DSN description.  Note that the maximum size is DSN_DESC_LEN.
 *                 Receives actual size of DSN description(driver name) returned.
 *    dsnDesc    [out]     Contain the description if successful
 *    pdsnDBQSize    [in/out]  Address of an unsigned long containing size
 *                             of the buffer for name of database.  Note that the maximum size is DSN_DBQ_LEN.
 *                 Receives actual size of DSN description returned.
 *    dsnDBQ     [out]     Contain the name of database if successful
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_BUFFER_TOO_SMALL     The buffer is too small for the string.
 *                             In this case the required buffer size is
 *                             returned in pBufSize.
 *    P_E_FAIL                 Failed to retrieve data path
 */

BTI_API PvGetDSN(
   BTI_LONG         hConnection,
   BTI_CHAR_PTR     dsnName,
   BTI_ULONG_PTR    pdsnDescSize,
   BTI_CHAR_PTR     dsnDesc,
   BTI_ULONG_PTR    pdsnDBQSize,
   BTI_CHAR_PTR     dsnDBQ);

/*
 * Name:
 *    PvCountDSNs()
 *
 * Description:
 *    Retrieve count of DSN
 *
 * Precondition:
 *    Connection established by calling PvConnectService();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pdsnCount      [out]     Address of an usinged long to receive the
 *                             number DSN
 *    filtering      [in]      if filtering equal 1 - return only Pervasive DSN,
 *                                                0 - all DSN.  
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_FAIL                 Failed to retrieve data path
 */

BTI_API PvCountDSNs(
   BTI_LONG         hConnection,
   BTI_ULONG_PTR    pdsnCount,
   BTI_CHAR         filtering);


/*
 * Name:
 *    PvListDSNs()
 *
 * Description:
 *    Get list of information of DSN
 *
 * Precondition:
 *    Connection established by calling PvConnectService();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pdsnlistSize   [in/out]  Address of an unsigned long containing size
 *                             of the buffer for list DSN.
 *                 Receives actual size of list DSN returned.
 *    pdsnList     [out]     Contain the name of list database if successful
 *    filtering      [in]      if filtering equal 1 - return only Pervasive DSN,
 *                                                0 - all DSN.  
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_BUFFER_TOO_SMALL     The buffer is too small for the string.
 *                             In this case the required buffer size is
 *                             returned in pdsnlistSize.
 *    P_E_FAIL                 Failed to retrieve data path
 */

BTI_API PvListDSNs(
   BTI_LONG         hConnection,
   BTI_ULONG_PTR    pdsnlistSize,
   BTI_CHAR_PTR     pdsnList,
   BTI_CHAR         filtering);


/*
 * Name
 *   PvGetEngineInformation   
 *
 * Description:
 *    Retrieve the information about engine for given hConnection
 *
 * Precondition:
 *    none
 *
 * Parameters:
 *    hConnection    [in]   Connection handle which identifies the server
 *    pServerClient  [out]    Address of an BTI_CHAR_PTR  
 *                TRUE  - MKDE_SRVR_ENGINE_CID,
 *                FALSE - MKDE_CLNT_ENGINE_CID
 *                              Maybe NULL
 *    pdbuApiVer        [out] version of the structures, maybe NULL
 *    pmajor            [out] Major version, maybe NULL
 *    pminor            [out] Minor version, maybe NULL
 *    pServerClientType [out]   Only for MKDE_SRVR_ENGINE_CID,  for NTSV return NT_SERVER, NWSV - NW_SERVER,
 *                              for MKDE_CLNT_ENGINE_CID return  client type
 *                              maybe NULL
 *
 * Return Value:
 *    P_OK                      Successful
 *    P_E_FAIL                  Failed to retrieve data path
 *    P_E_INVALID_HANDLE        Invalid connection handle
 */


BTI_API PvGetEngineInformation( 
   BTI_LONG         hConnection,
   BTI_CHAR_PTR     pServerClient,          /* TRUE - MKDE_SRVR_ENGINE_CID, FALSE - MKDE_CLNT_ENGINE_CID */ 
   BTI_ULONG_PTR    pdbuApiVer,             /* version of the structures  */
   BTI_ULONG_PTR    pmajor,                 /* Major version  */
   BTI_ULONG_PTR    pminor,                 /* Minor version  */
   BTI_ULONG_PTR    pServerClientType);


//////////////////////////////WARNING////////////////////////////////////////////////////////////
//The following functions are subject to change in the the future releases and are purely for internal use.  //
/////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Name:
 *    PvCreateDSN()
 *
 * Description:
 *    Create New DSN.  Note that this function is subject to change and is 
 *    reserved for internal Pervasive Software use. 
 *
 * Precondition:
 *    Connection established by calling PvConnectService();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pdsnName       [in]      DSN name
 *    pdsnDesc       [in]      Contain the description of DSN
 *    dsnDBQ         [in]      Contain the name of database
 *    openMode       [in]      Contain DSN open mode which can be one of the following constants:
 *                             NORMAL_MODE, ACCELERATED_MODE, READONLY_MODE or EXCLUSIVE_MODE.
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_NAME         Invalid name of DSN 
 *    P_E_DSN_ALREADY_EXIST    DSN already exist
 *    P_E_ACCESS_RIGHT         Insufficient access right for the operation
 *    P_E_INVALID_OPEN_MODE    Invalid Open Mode
 *    P_E_FAIL                 Failed to retrieve data path
 */


BTI_API PvCreateDSN(
    BTI_LONG        hConnection,
    BTI_CHAR_PTR    pdsnName,
    BTI_CHAR_PTR    pdsnDesc,
    BTI_CHAR_PTR    pdsnDBQ,
    BTI_LONG        openMode);


/*
 * Name:
 *    PvDeleteDSN()
 *
 * Description:
 *    Delete DSN.  Note that this function is subject to change and is 
 *    reserved for internal Pervasive Software use. 
 *
 * Precondition:
 *    Connection established by calling PvConnectService();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pdsnName       [in]      DSN name
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_DSN_DOES_NOT_EXIST   DSN  Name doesnt exists
 *    P_E_ACCESS_RIGHT         Insufficient access right for the operation
 *    P_E_FAIL                 Failed to retrieve data path
 */

BTI_API PvDeleteDSN(
    BTI_LONG        hConnection,
    BTI_CHAR_PTR    pdsnName);


/*
 * Name:
 *    PvGetDSNEx()
 *
 * Description:
 *    Retrieve information about DSN. Note that this function is subject to change and is 
 *    reserved for internal Pervasive Software use.
 *
 * Precondition:
 *    Connection established by calling PvConnectService();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    dsnName        [in]      DSN name
 *    pdsnDescSize   [in/out]  Address of an unsigned long containing size
 *                             of the buffer for DSN description.  
 *                             Receives actual size of DSN description(driver name) returned.
 *    dsnDesc        [out]     Contain the description if successful
 *    pdsnDBQSize    [in/out]  Address of an unsigned long containing size
 *                             of the buffer for name of database.  
 *                             Receives actual size of DSN description returned.
 *    dsnDBQ         [out]     Contain the name of database if successful
 *    pOpenMode      [out]     Contain DSN open mode which can be one of the following constants:
 *                             NORMAL_MODE, ACCELERATED_MODE, READONLY_MODE or EXCLUSIVE_MODE.
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_BUFFER_TOO_SMALL     The buffer is too small for the string.
 *                             In this case the required buffer size is
 *                             returned in pBufSize.
 *    P_E_ACCESS_RIGHT         Insufficient access right for the operation
 *    P_E_DSN_DOES_NOT_EXIST   DSN does not exist
 *    P_E_INVALID_OPEN_MODE    Invalid Open Mode
 *    P_E_FAIL                 Failed to retrieve data path
 */

BTI_API PvGetDSNEx( 
   BTI_LONG         hConnection,
   BTI_CHAR_PTR     dsnName,
   BTI_ULONG_PTR    pdsnDescSize,
   BTI_CHAR_PTR     dsnDesc,
   BTI_ULONG_PTR    pdsnDBQSize,
   BTI_CHAR_PTR     dsnDBQ,
   BTI_LONG_PTR     pOpenMode);


/*
 * Name:
 *    PvModifyDSN()
 *
 * Description:
 *    Modify parameters for DSN.  Note that this function is subject to change and is 
 *    reserved for internal Pervasive Software use.
 *
 * Precondition:
 *    Connection established by calling PvConnectService();
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    dsnName        [in]      DSN name
 *    dsnDesc        [in]      Contain new description 
 *    dsnDBQ         [in]      Contain new name of database
 *    openMode       [in]      Contain new DSN open mode which can be one of the following constants:
 *                             NORMAL_MODE, ACCELERATED_MODE, READONLY_MODE or EXCLUSIVE_MODE.
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_DSN_DOES_NOT_EXIST   DSN doesn not exists;
 *    P_E_ACCESS_RIGHT         Insufficient access right for the operation
 *    P_E_INVALID_OPEN_MODE    Invalid Open Mode
 *    P_E_FAIL                 Failed to retrieve data path
 */

BTI_API PvModifyDSN( 
   BTI_LONG         hConnection,
   BTI_CHAR_PTR     pdsnName,
   BTI_CHAR_PTR     pdsnDesc,
   BTI_CHAR_PTR     pdsnDBQ,
   BTI_LONG         openMode);


#ifdef __cplusplus
}
#endif

#define _CATALOG_H_INCLUDED
#endif /* _CATALOG_H_INCLUDED */

