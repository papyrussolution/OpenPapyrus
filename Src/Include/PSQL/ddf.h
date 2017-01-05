
/***************************************************************************
**
**  Copyright 1998-2000 Pervasive Software Inc. All Rights Reserved
**
***************************************************************************/
/***************************************************************************
   ddf.h - DDF Catalog API
	This file contains function headers
	for catalog API
***************************************************************************/

#ifndef _DDF_H_
#define _DDF_H_


#ifdef __cplusplus
 extern "C" {
#endif

/* Don't pad our structures */
#pragma pack(push, 8)


//====================
//DATA STRUCTURES
//====================


#define TABLENAME_LEN  20
#define COLUMNNAME_LEN  20
#define INDEXNAME_LEN  20
#define PATH_LEN  64
#define ERROR_LEN  255
#define ISR_LEN 16


//error codes
typedef enum 
{
	PCM_Success  = 0,
	PCM_errFailed  = 1,
	PCM_errMemoryAllocation  = 2,
	PCM_errDictionaryNotFound  = 3,
	PCM_errDictionaryAlreadyOpen  = 4,
	PCM_errDictionaryNotOpen  = 5,
	PCM_errInvalidDictionaryHandle  = 6,
	PCM_errTableNotFound  = 7,
	PCM_errInvalidTableName  = 8,
	PCM_errInvalidColumnName  = 9,
	PCM_errInvalidDataType  = 10 ,
	PCM_errDuplicateColumnName  = 11,
	PCM_errInvalidDataSize  = 12,
	PCM_errInvalidColumnOrder  = 13,
	PCM_errInvalidIndexName  = 14,
	PCM_errColumnNotFound  = 15,
	PCM_errTooManySegments  = 16,
	PCM_errStringTooShort = 17,
	PCM_errDictionaryAlreadyExists = 18,
	PCM_errDirectoryError = 19,
	PCM_errSessionSecurityError = 20,
	PCM_errDuplicateTable = 21,
	PCM_errDuplicateIndex = 22
}
PRESULT;

//TABLEINFO flag value can take the following values or their combination:
#define B_FLAG_TRUE_NULLABLE 64 // table is true nullable.
	//When the table is created a one byte null indicator is added before each column that is nullable.

//COLUMNINFO flag value can take the following values or their combination:
#define B_FLAG_CASE_SENSITIVE  1 // Case Sensitive - column values are case sensitive on comparisons and as part of index segments 
#define B_FLAG_NULLABLE  4 // Nullable - if the table is created as true nullable then a one byte null indicator column is added before the column value to indicate whether the column value is null.

//COLUMNINFO dataType can take the following values:
#define B_TYPE_STRING 0
#define B_TYPE_INTEGER 1
#define B_TYPE_FLOAT 2
#define B_TYPE_DATE 3
#define B_TYPE_TIME 4
#define B_TYPE_DECIMAL 5
#define B_TYPE_MONEY 6
#define B_TYPE_LOGICAL 7
#define B_TYPE_NUMERIC 8
#define B_TYPE_BFLOAT 9
#define B_TYPE_LSTRING 10
#define B_TYPE_ZSTRING 11
#define B_TYPE_NOTE 12
#define B_TYPE_LVAR 13
#define B_TYPE_BINARY 14
#define B_TYPE_AUTOINC 15
#define B_TYPE_BIT 16
#define B_TYPE_NUMERSTS 17
#define B_TYPE_NUMERSA 18
#define B_TYPE_CURRENCY 19
#define B_TYPE_TIMESTAMP 20
#define B_TYPE_BLOB 21
#define B_TYPE_GDECIMAL 22
#define B_TYPE_WSTRING 25
#define B_TYPE_WZSTRING 26
#define B_TYPE_DATETIME 30

//INDEXINFO flags can be a combination of these values
#define B_FLAG_DUPLICATES 1 // Duplicates Allowed
#define B_FLAG_MODIFIABLE 2 // Modifiable
#define B_FLAG_SORT_DESCENDING 64 // Sort Descending


//structure used to pass table names
typedef struct TABLEMAPtag
{
	char tableName[TABLENAME_LEN + 1];
}TABLEMAP;

//structure used to pass table properties
typedef struct TABLEINFOtag
{
	char		tableName [TABLENAME_LEN + 1];   	// dictionary tableName
	char		dataLocation [PATH_LEN + 1];  	// actual [path\name]of data file
	WORD	 flags;											//flags of the table
	BOOL	 overwrite;									 //indicates whether data file will be overwritten if one already exists.
}TABLEINFO;


//structure used to pass column information
typedef struct COLUMNMAPtag
{
	WORD  index;											 //ordinal of the column position
	char	 name[COLUMNNAME_LEN + 1]; 		   //column name
	WORD  dataType;                  					  //data type
	WORD  size;                								 //field length
	WORD  decimal;               						  //decimal places
	WORD  flags;											//field flags
	char     isrName[ISR_LEN + 1];								  //international sorting rule name
}COLUMNMAP;

//structure used to pass index information
typedef struct INDEXMAPtag
{
	WORD	index;                 									// Btrieve index number
	WORD	segment;               								// segment index number
	char	   columnName[COLUMNNAME_LEN + 1];   // index into associated field
	char	   indexName[INDEXNAME_LEN + 1];  		// index name
	WORD	flags;                 									// index attributes
}INDEXMAP;

/*
===========================================
Memory Management
=============================================
The following rules apply:

-For [in] only parameters, the client (caller) is responsible 
 for allocating and freeing the memory.

-For [out] only parameters, the server (callee) is responsible 
 for allocating the memory, and the client (caller) 
 is responsible for freeing the memory, but in the case of this component
 the server also provides the deallocation functions
 (see PvFreeTable, PvFreeTableNames)

-For [in, out] parameters, the client (caller) allocates the memory 
 and is also responsible for freeing the memory.  
 However, the component (callee) has the option of 
 reallocating memory if it needs to.

==============================================*/


//====================
// FUNCTION PROTOTYPES
//====================

////////////////////////////////////////////////////////////////////////////////////////////////
/*
*PvOpenDictionary
*
*Opens an existing dictionary.  This function should be called before any other.  
*Given an absolute path to the dictionary or data source names
*it returns a dictionary handle, 
*that will be used for any subsequent calls to any functions.  
*Note that multiple dictionaries can be opened at one time. 
*PvCloseDictionary should be called to free the resources.
*
*Parameters:
*	[in] path 					Absolute path to the dictionary files.
*	[in, out] dictHandle 		Handle to be used in subsequent calls
*	[in] user 					User name needed to open the dictionary, can be set to NULL
*	[in] password 			Used in conjuction with user name to open the dictionary files, can also be NULL
*
*Return Values:
*	PCM_Success
*	PCM_errFailed
*	PCM_errMemoryAllocation
*	PCM_errDictionaryNotFound
*	PCM_errDictionaryAlreadyOpen
*/
//////////////////////////////////////////////////////////////////////////////////////////////

PRESULT WINAPI PvOpenDictionary
	(
	LPCSTR path, 
	WORD* dictHandle, 
	LPCSTR user, 
	LPCSTR password
	);

////////////////////////////////////////////////////////////////////////////////////////////////
/*
*PvCreateDictionary
*
*Given an absolute path to the dictionary it creates a new set of dictionary files, 
*and returns a handle to the newly create dictionary
*that will be used for any subsequent calls to any functions. 
*if the given directory does not exist and attempt will be made to 
*create the directory.  If the attempt fails PCM_errDirectoryError is returned. 
*PvCloseDictionary should be called to free the resources.
*
*Parameters:
*	[in] path 					Absolute path to the dictionary files.
*	[in, out] dictHandle 		Handle to be used in subsequent calls
*	[in] user 					User name needed to open the dictionary, can be set to NULL
*	[in] password 			Used in conjuction with user name to open the dictionary files, can also be NULL
*
*Return Values:
*	PCM_Success
*	PCM_errFailed
*	PCM_errMemoryAllocation
*	PCM_errPathNotFound
*   PCM_errDirectoryError
*
*Postcondition:
*	PvCloseDictionary needs to be called to free all resources.
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////

PRESULT WINAPI PvCreateDictionary
	(
	LPCSTR path, 
	WORD* dictHandle,
	LPCSTR user, 
	LPCSTR password
	);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*PvCloseDictionary
*
*Given a handle of an opened dictionary closes the open dictionary.  
*Since multiple dictionaries can be open,
*this function needs to be called for every opened or newly created dictionary.
*
*Preconditions:
*	Dictionary had to be opened or created using PvOpenDictionary or PvCreateDictionary functions.
*	
*Parameters:
*	[in] dictHandle   handle of an open or newly created dictionary returned by PvOpenDictionary
*									or PvCreateDictionary.
*
*Return Value:
*	PCM_Success
*	PCM_errFailed
*	PCM_errMemoryAllocation
*	PCM_errDictionaryNotOpen
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////

PRESULT WINAPI PvCloseDictionary
	(
	WORD dictHandle
	); 

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*PvGetTableNames
*
*Returns table names of all the tables in the open data dictionary.  
*Dictionary handle specifies which dictionary information is returned.
*
*Precondition:
*	Dictionary was successfully opened using PvOpenDictionary().
*
*Parameters:
*	[in] dictHandle  handle of an open dictionary returned by OpenDictionary
*	[out] tableList		array of TABLEMAP structures that contains table names
*	[in, out] tableCount  number of table names returned in tableList
*
*Return Values:
*	PCM_Success
*	PCM_errFailed
*	PCM_errMemoryAllocation
*	PCM_errInvalidDictionaryHandle
*
*Postcondition:
*	tableList array will need to be released using PvFreeTableNames
*	(see memory management)
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////

PRESULT WINAPI PvGetTableNames
	(
	WORD dictHandle,  
	TABLEMAP** tableList,
	WORD* tableCount
	); 


////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*PvFreeTableNames
*
*Frees memory allocated in PvGetTableNames call
*
*Precondition:
*	Memory was succesfully allocated in PvGetTableNames call
*
*Parameters:	
*	[in, out] tableList		array of TABLEMAP structures
*
*Return Values:
*	PCM_Success
*	PCM_errFailed
*
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////

PRESULT WINAPI PvFreeTableNames
	(	
	TABLEMAP* tableList	
	); 

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*PvGetTable
*
*Returns table attributes for a given table.
*Dictionary handle specifies which dictionary contains the requested table information.
*
*Precondition:
*	Dictionary was successfully opened using PvOpenDictionary()
*
*Parameters:
*	[in] dictHandle  handle of an open dictionary returned by PvOpenDictionary
*	[in] tableName  number of table names returned in tableList
*	[out] tableProps		structure containing table information
*	[out] columnList	array of columns defined in the table
*	[out] columnCount number of columns in columnList
*	[out] indexList	array of segments defined in the table
*	[out] indexCount	number of segment in the indexList array
*
*Return Values:
*	PCM_Success
*	PCM_errFailed
*	PCM_errMemoryAllocation
*	PCM_errInvalidDictionaryHandle
*	PCM_errTableNotFound
*
*Postcondition:
*	tableProps, indexList and columnList will need to be released using CoTaskMemFree
*	(see memory management)
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////

PRESULT WINAPI PvGetTable
	(
	WORD dictHandle, 
	LPCSTR tableName, 
	TABLEINFO** tableProps,
	COLUMNMAP** columnList,
	WORD* columnCount,
	INDEXMAP** indexList,
	WORD* indexCount
	); 

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*PvFreeTable
*
*Frees memory allocated in PvGetTable call
*
*Precondition:
*	Memory was succesfully allocated in PvGetTable call
*
*Parameters:	
*	[in, out] tableProps		pointer to TABLEINFO structure
*	[in, out] columnList    pointer to an array of COLUMNMAP structures
*	[in, out] indexList       pointer to an array of INDEXMAP strucures
*
*Return Values:
*	PCM_Success
*	PCM_errFailed
*
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////
PRESULT WINAPI PvFreeTable
	(	
	TABLEINFO* tableProps,
	COLUMNMAP* columnList,	
	INDEXMAP* indexList	
	); 

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*PvAddTable
*
*Creates a new table in the existing dictionary and a data file at the location specified in table properties.
*This function has to be provided with table information, columns and indexes.  
*indexCount and indexList are optional parameters because
*indexes are not required to create a table.  
*This function will fail if a table with the same name is already present in the specified dictionary.
*
*Precondition:
*	Dictionary was successfully opened using PvOpenDictionary().
*	Table Properties are set up correctly and an array of at least one column is passed in.
*
*Parameters:
*	[in] dictHandle  handle of an open dictionary returned by PvOpenDictionary
*	[in] tableProps  properties of the table to be created
*	[in] columnCount		number of columns in the following columnList array.
*	[in] columnList		array of column definitions
*	[in] indexCount  number of indexes in the following indexList array.
*	[in] indexList		array of index definitions
*
*Return Values:
*	PCM_Success
*	PCM_errFailed
*	PCM_errInvalidDictionaryHandle
*	PCM_errMemoryAllocation
*	PCM_errInvalidColumnName
*	PCM_errInvalidDataType
*	PCM_errDuplicateColumnName
*	PCM_errInvalidDataSize
*	PCM_errInvalidColumnOrder
*	PCM_errInvalidIndexName
*	PCM_errColumnNotFound
*
*Postcondition:
*	User will need to allocate and release COLUMNMAP and INDEXMAP arrays and TABLEINFO structure
*	used to describe the table
*	(see memory management)
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////

PRESULT WINAPI PvAddTable
	(
	WORD dictHandle, 
	TABLEINFO* tableProps, 
	COLUMNMAP* columnList,
	WORD columnCount, 
	INDEXMAP* indexList,
	WORD indexCount
	);

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*PvDropTable
*
*Drops the specified table from the open dictionary specified by dictionary handle.
*
*Precondition:
*	Dictionary was successfully opened using PvOpenDictionary().
*	A table specified by tableName exists in dictionary specified by dictHandle.
*
*Parameters:
*	[in] dictHandle  handle of an open dictionary returned by PvOpenDictionary
*	[in] tableName  name of the table where the columns will be created
*   [in] keepData  indicates whether the data file will be delete or not
                            if 0 the data file associated with the table will be deleted
                            if not 0 the data file not be deleted
*
*Return Values:
*	PCM_Success
*	PCM_errFailed
*	PCM_errInvalidDictionaryHandle
*	PCM_errTableNotFound
*
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////

PRESULT WINAPI PvDropTable
	(
	WORD dictHandle, 
	LPCSTR tableName,
	WORD keepData
	);

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*PvDropIndexByName
*
*Drops the index from the ddf file and the data file, given its name.
*
*Precondition:
*	Dictionary was successfully opened using PvOpenDictionary().
*	A table specified by tableName exists in dictionary specified by dictHandle.
*	index with the given name exists in the table.
*
*Parameters:
*	[in] dictHandle  handle of an open dictionary returned by PvOpenDictionary
*	[in] tableName	 specifes the table on which the operation will be performed
*	[in] indexName  name of the index name to be dropped
*
*Return Values:
*	PCM_Success
*	PCM_errFailed
*	PCM_errInvalidDictionaryHandle
*	PCM_errTableNotFound
*
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////
PRESULT WINAPI PvDropIndexByName
	(
	WORD dictHandle, 
	LPCSTR tableName, 
	LPCSTR indexName
	);

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*PvDropIndex
*
*Drops the index from the ddf file and the data file, given the index number.
*
*Precondition:
*	Dictionary was successfully opened using PvOpenDictionary().
*	A table specified by tableName exists in dictionary specified by dictHandle.
*	index with the given number exists in the table.
*
*Parameters:
*	[in] dictHandle  handle of an open dictionary returned by PvOpenDictionary
*	[in] tableName	 specifes the table on which the operation will be performed
*	[in] indexNimber  number of the index to be dropped.
*   [in] renumber indicates whether the remaining indexes should be renumbered.
*
*Return Values:
*	PCM_Success
*	PCM_errFailed
*	PCM_errInvalidDictionaryHandle
*	PCM_errTableNotFound
*	PCM_errInvalidIndex
*
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////
PRESULT WINAPI PvDropIndex
	(
	WORD dictHandle, 
	LPCSTR tableName, 
	WORD indexNumber, 
	BOOL renumber
	);


////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*PvAddIndex
*
*Adds indexes specified in indexList to the existing table and to the underlying data file.
*
*Precondition:
*	Dictionary was successfully opened using PvOpenDictionary() or created using PvCreateDictionary().
*	Specified table exists and the columns used to define index segments exist in the table.
*
*Parameters:
*	[in] dictHandle  handle of an open dictionary returned by PvOpenDictionary
*	[in] tableName	name of the table where the indexes will be added
*	[in] indexList		array of column definitions
*	[in] indexCount  number of indexes in the following indexList array.
*
*Return Values:
*	PCM_Success
*	PCM_errFailed
*	PCM_errInvalidDictionaryHandle
*	PCM_errTableNotFound
*	PCM_errMemoryAllocation
*	PCM_errInvalidColumnName
*	PCM_errInvalidIndexName
*	PCM_errColumnNotFound
*
*Postcondition:
*	User will need to allocate and release INDEXMAP array
*	used to describe the indexes
*	(see memory management module.)
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////

PRESULT WINAPI PvAddIndex
	(
	WORD dictHandle,
	LPCSTR tableName,
	INDEXMAP* indexList,
	WORD indexCount
	);

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*PvGetError
*
*Returns an error description string, describing last error.  The errorDesc string is allocated
*by the caller.  The maximum size of the error description is specified in ERROR_LEN. 
*
*Parameters:
*	[in, out]errorDesc  string that will contain the error description
*	[in, out]size				size of errorDesc.  If the size is not big enough to contain the
*										error description.  An error returned and the required size is specified in size.
*
*Return Values:
*	PCM_Success
*	PCM_errStringTooShort
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////
PRESULT WINAPI PvGetError
	(
	LPSTR errorDesc, 
	WORD* size
	);

#pragma pack(pop)

#ifdef __cplusplus
 }
#endif

#endif

