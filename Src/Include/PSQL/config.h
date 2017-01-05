#ifndef _CONFIG_H_INCLUDED
/*************************************************************************
**
**  Copyright 1998-2000 Pervasive Software Inc. All Rights Reserved
**
*************************************************************************/
/*************************************************************************
*   CONFIG.H
*
*
*   This header prototypes a number of Configuration functions
*   of the Pervasive Distributed Tuning Interface.
*
*     The following functions are found in this file:
*
*       PvGetCategoryList()
*       PvGetCategoryInfo()
*       PvGetSettingList()
*       PvIsSettingAvailable()
*       PvGetBooleanValue()
*       PvGetLongValue()
*       PvGetStringValue()
*       PvGetSelectionValue()
*       PvSetBooleanValue()
*       PvSetLongValue()
*       PvSetStringValue()
*       PvSetSelectionValue()
*       PvGetSettingInfo()
*       PvGetValueLimit()
*       PvCountSelectionItems()
*       PvGetAllPossibleSelections()
*       PvGetSelectionString()
*       PvGetBooleanStrings()
*       PvGetSettingMap()
*       PvGetSettingHelp()
*       PvGetSettingUnits()
*       PvGetStringType()
*		PvGetSettingUnitsSize()    
*		PvGetSettingHelpSize()
*		PvGetSelectionStringSize()   
*		PvGetStringValueSize()
*		PvGetSettingListCount()   
*		PvGetCategoryListCount()   
*
*
*************************************************************************/
/*===========================================================================
 *
 * Configuration API
 *
 *=========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/* Don't pad our structures */
#pragma pack(push, 8)

/*
 * Different types of settings
 */
typedef enum
{
   PVSETTING_BOOLEAN = 0,  /* boolean type */
   PVSETTING_LONG,         /* long integer type */
   PVSETTING_STRING,       /* null terminated String type */
   PVSETTING_SINGLE_SEL,   /* single selection type */
   PVSETTING_MULTI_SEL,    /* multi  Selection type */
} PVSETTINGENUM;


/*
 * Ranks of the setting based on usage
 */
typedef enum
{
   PVRANK_NORMAL  = 0,  /* frequently used setting */
   PVRANK_ADVANCED,     /* rarely used setting for sophisticated users */
} PVRANKENUM;


/*
 * Identifiers indiciating which type of data to set or get.
 *
 * The Get operations can use PVDATA_DEFAULT, PVDATA_CURRENT.
 *
 * The Set operations can use PVDATA_CURRENT, PVDATA_PERSISTENT,
 * or both values combined with a binary or.
 *
 * NOTE: PVDATA_ values should be defined to allow binary operations.
 */
#define PVDATA_DEFAULT              0x1
#define PVDATA_CURRENT              0x2
#define PVDATA_PERSISTENT           0x4

/*
 * Identifiers to indicate client-side or server-side settings
 */
#define PVSETTING_CLIENT            0x0
#define PVSETTING_SERVER            0x1

/*
 * sub-types of PVSTRING_SETTING retrieves by PvGetStringType
 */

#define PVSTRING      0x0
#define PVDIRECTORYSTRING 0x1
#define PVFILESTRING    0x2


/*
 * Special constants
 */
#define P_LOCAL_DB_CONNECTION       0xFFFFFFFF

/* Maximum valid memory or disk size */
#define P_MAX_MEM_DISK_SIZE         0xFFFFFF7F

/* Maximum size limited by available disk space */
#define P_MAX_LIMITED_BY_DISK       0xFFFFFFFE

/* Maximum size limited by available memory */
#define P_MAX_LIMITED_BY_MEMORY     0xFFFFFFFF

/* Maximum size of value name strings */
#define P_MAX_VALUE_NAME_LENGTH     255

/* Maximum size of category name strings */
#define P_MAX_CATEGORY_NAME_LENGTH  255


/*
 * Category information
 */
typedef struct
{
   BTI_CHAR    cName[P_MAX_CATEGORY_NAME_LENGTH];
   BTI_ULONG   numOfSettings;
} PVCATEGORYINFO;


/*
 * Common information related to the value regardless of its value type
 */
typedef struct
{
   PVSETTINGENUM  sType;                          /* setting type */
   BTI_CHAR       sName[P_MAX_VALUE_NAME_LENGTH]; /* setting display name */
   PVRANKENUM     sRank;                          /* setting rank */
   BTI_WORD       sClientServer;                  /* client or server flag */
} PVSETTINGINFO;


/*
 * Name:
 *    PvGetCategoryList()
 *
 * Description:
 *    Retrieve the list of categories
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();  Note that the array size can 
 *    be obtained using PvGetCategoryListCount() call.
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    pNumCategories  [in/out] Address of an unsigned long containing number of items
 *                             in the array allocated to receive category list.
 *                             Receives actual number of of category id values returned
 *    pCategoryList   [out]    List of categoryIDs returned
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_FAIL                 Failed to get category information
 */
BTI_API PvGetCategoryList(
   BTI_LONG          hConnection,
   BTI_ULONG_PTR     pNumCategories,
   BTI_ULONG_PTR     pCategoriesList);


/*
 * Name:
 *    PvGetCategoryInfo()
 *
 * Description:
 *    Retrieve category information
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    categoryID      [in]     Unique identifier for the category
 *    pCatInfo        [out]    Address of PVCATEGORYINFO structure that
 *                             receives category information
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_FAIL                 Failed to get category information
 */
BTI_API PvGetCategoryInfo(
   BTI_LONG          hConnection,
   BTI_ULONG         categoryID,
   PVCATEGORYINFO*   pCatInfo);


/*
 * Name:
 *    PvGetSettingList()
 *
 * Description:
 *    Retrieves a list of settings belonging to the specified category.
 *    If the connection is a remote connection, only server-side
 *    settings for the category are returned. If the connection is a
 *    local connection, both client-side and server-side settings for
 *    this category will be returned. Use PvIsSettingsAvalible to
 *    determine if the setting can be set at this time.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();  Note that the number
 *    of settings can be obtained using PvGetSettingListCount() call.  
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    categoryID      [in]     Unique identifier for the category
 *    pNumSettings    [out]    Address of an unsigned long containing number of 
 *                             items in the allocated array on input, and receives number of
 *                             items in the returned list.
 *    pSettingList    [out]    Pointer to the list of setting IDs returned
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_BUFFER_TOO_SMALL     The array size is too small. In this case the required
 *                             number of members in the array is returned in pNumSettings.
 *    P_E_FAIL                 Failed to get setting list
 */
BTI_API PvGetSettingList(
   BTI_LONG       hConnection,
   BTI_ULONG      categoryID,
   BTI_ULONG_PTR  pNumSettings,
   BTI_ULONG_PTR  pSettingList);


/*
 * Name:
 *    PvIsSettingAvailable()
 *
 * Description:
 *    Query to see if a setting is available for configuring
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    settingID       [in]     Unique identifier for the setting
 *
 * Return Value:
 *    Non-zero value if setting is available, 0 if not available
 */
BTI_API PvIsSettingAvailable(
   BTI_LONG  hConnection,
   BTI_ULONG settingID);


/*
 * Name:
 *    PvGetBooleanValue()
 *
 * Description:
 *    Retrieve the value for a Boolean type setting, from the data source
 *    specified by whichData.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    settingID       [in]     Unique identifier for the setting
 *    pValue          [out]    Address of a Boolean variable that receives
 *                             setting value returned
 *    whichData       [in]     Flag to indicate which value is requested.
 *                               PVDATA_DEFAULT  to return default value
 *                               PVDATA_CURRENT  to return current value
 *                               Others          to return current value
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_DATA_TYPE    The setting requested is not of Boolean type
 *    P_E_FAIL                 Failed to retrieve setting value
 */
BTI_API PvGetBooleanValue(
   BTI_LONG       hConnection,
   BTI_ULONG      settingID,
   BTI_SINT_PTR   pValue,
   BTI_SINT       whichData);


/*
 * Name:
 *    PvGetLongValue()
 *
 * Description:
 *    Retrieve thevalue for an long integer type setting, from the data
 *    source specified by whichData.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    settingID       [in]     Unique identifier for the setting
 *    pValue          [out]    Address of an long integer that receives
 *                             setting value returned.
 *                             Non-zero for TRUE, zero for FALSE.
 *    whichData       [in]     Flag to indicate which value is requested.
 *                               PVDATA_DEFAULT  to return default value
 *                               PVDATA_CURRENT  to return current value
 *                               Others          to return current value
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_DATA_TYPE    The setting requested is not of long type
 *    P_E_FAIL                 Failed to retrieve setting value
 */
BTI_API PvGetLongValue(
   BTI_LONG       hConnection,
   BTI_ULONG      settingID,
   BTI_LONG_PTR   pValue,
   BTI_SINT       whichData);


/*
 * Name:
 *    PvGetStringValue()
 *
 * Description:
 *    Retrieve the value (null terminated string) for a string type setting,
 *    from the data source specified by whichData. Some settings may
 *    return a list of strings separated by ';'.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();  Size needed for the 
 *    buffer can be obtained using PvGetStringValueSize() call.
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    settingID       [in]     Unique identifier for the setting
 *    pBufSize        [in/out] Address of an unsigned long containing size
 *                             of buffer allocated to receive setting value.
 *                             Receives actual size of setting value returned
 *                             The size should include the null terminator.
 *    value           [out]    String value returned
 *    whichData       [in]     Flag to indicate which value is requested.
 *                               PVDATA_DEFAULT  to return default value
 *                               PVDATA_CURRENT  to return current value
 *                               Others          to return current value
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_DATA_TYPE    The setting requested is not of string type
 *    P_E_BUFFER_TOO_SMALL     Allocated buffer is too small for the string,
 *                             returned string is truncated.  In this case
 *                             the required size is retured in pBufSize.
 *    P_E_FAIL                 Failed to retrieve setting value
 */
BTI_API PvGetStringValue(
   BTI_LONG       hConnection,
   BTI_ULONG      settingID,
   BTI_ULONG_PTR  pBufSize,
   BTI_CHAR_PTR   value,
   BTI_SINT       whichData);


/*
 * Name:
 *    PvGetSelectionValue()
 *
 * Description:
 *    Retrieve the value for a selection type setting, from the data source
 *    specified by whichData.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Optionally, number of selection items retrieved by calling
 *       PvCountSelectionItems();
 *
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    settingID       [in]     Unique identifier for the setting
 *    pNumItems       [in/out] Address of an unsigned long that specifies
 *                             the array size on input, and receives number
 *                             of individual selection items returned
 *    pValue          [out]    Array of individual selection indexes
 *    whichData       [in]     Flag to indicate which value is requested.
 *                               PVDATA_DEFAULT  to return default value
 *                               PVDATA_CURRENT  to return current value
 *                               Others          to return current value
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_DATA_TYPE    The setting requested is not of selection type
 *    P_E_BUFFER_TOO_SMALL     The array size is too small. In this case the
 *                             required size is returned in pNumItems.
 *    P_E_FAIL                 Failed to retrieve setting value
 */
BTI_API PvGetSelectionValue(
   BTI_LONG       hConnection,
   BTI_ULONG      settingID,
   BTI_ULONG_PTR  pNumItems,
   BTI_LONG_PTR   pValue,
   BTI_SINT       whichData);


/*
 * Name:
 *    PvSetBooleanValue()
 *
 * Description:
 *    Save new value for a Boolean type setting, to the data target
 *    specified by whichData.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    settingID       [in]     Unique identifier for the setting
 *    newValue        [in]     Boolean value to be set
 *                             Non-zero value for TRUE, zero for FALSE.
 *    whichData       [in]     Flag to indicate which value to be set
 *                             May be the combination of the following values
 *                               PVDATA_CURRENT    to set current value
 *                               PVDATA_PERSISTENT to set persistent value
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_INVALID_DATA_TYPE    The setting to be set is not of Boolean type
 *    P_E_SET_CURRENT_DATA     Cannot set the current setting value
 *    P_E_FAIL                 Failed to save setting value
 */
BTI_API PvSetBooleanValue(
   BTI_LONG  hConnection,
   BTI_ULONG settingID,
   BTI_SINT  newValue,
   BTI_SINT  whichData);


/*
 * Name:
 *    PvSetLongValue()
 *
 * Description:
 *    Save new value for an long integer type setting, to the data target
 *    specified by whichData.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    settingID       [in]     Unique identifier for the setting
 *    newValue        [in]     Integer value to be set
 *    whichData       [in]     Flag to indicate which value to be set
 *                             May be the combination of the following values
 *                               PVDATA_CURRENT    to set current value
 *                               PVDATA_PERSISTENT to set persistent value
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_INVALID_DATA_TYPE    The setting to be set is not of long type
 *    P_E_SET_CURRENT_DATA     Cannot set the current setting value
 *    P_E_OUT_OF_RANGE         The setting value to be set is out of range
 *    P_E_FAIL                 Failed to save setting value
 */
BTI_API PvSetLongValue(
   BTI_LONG  hConnection,
   BTI_ULONG settingID,
   BTI_LONG  newValue,
   BTI_SINT  whichData);


/*
 * Name:
 *    PvSetStringValue()
 *
 * Description:
 *    Save new value to a string type setting, to the data target
 *    specified by whichData. Some settings may take multiple strings
 *    separated by ';'.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    settingID       [in]     Unique identifier for the setting
 *    newValue        [in]     String value to be set
 *    whichData       [in]     Flag to indicate which value to be set
 *                             May be the combination of the following values
 *                               PVDATA_CURRENT    to set current value
 *                               PVDATA_PERSISTENT to set persistent value
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_DATA_TYPE    The setting to be set is not of string type
 *    P_E_SET_CURRENT_DATA     Cannot set the current setting value
 *    P_E_FAIL                 Failed to save setting value
 */
BTI_API PvSetStringValue(
   BTI_LONG     hConnection,
   BTI_ULONG    settingID,
   BTI_CHAR_PTR newValue,
   BTI_SINT     whichData);


/*
 * Name:
 *    PvSetSelectionValue()
 *
 * Description:
 *    Save new value to a selection type setting, to the data target
 *    specified by whichData.  This function is used to work with both
 *    single-seleciton and multi-selection data types.  If more than one
 *    selection items are set for a single-selection item, the first value
 *    is used.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    settingID       [in]     Unique identifier for the setting
 *    numItems        [in]     Number of individual selection items to be set
 *    pNewValue       [in]     Array of individual selection items to be set
 *    whichData       [in]     Flag to indicate which value to be set
 *                             May be the combination of the following values
 *                               PVDATA_CURRENT    to set current value
 *                               PVDATA_PERSISTENT to set persistent value
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_DATA_TYPE    The setting to be set is not of selection type
 *    P_E_INVALID_SELECTION    At least one selection item is invalid
 *    P_E_SET_CURRENT_DATA     Cannot set the current setting value
 *    P_E_FAIL                 Failed to save setting value
 */
BTI_API PvSetSelectionValue(
   BTI_LONG       hConnection,
   BTI_ULONG      settingID,
   BTI_ULONG      numItems,
   BTI_LONG_PTR   pNewValue,
   BTI_SINT       whichValue);


/*
 * Name:
 *    PvGetSettingInfo()
 *
 * Description:
 *    Retrieve setting information common to all types of settings
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    settingID       [in]     Unique identifier for the setting
 *    pSettingInfo    [out]    Address of PVSETTINGINFO structure that
 *                             receives setting information
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_FAIL                 Failed to get setting information
 */
BTI_API PvGetSettingInfo(
   BTI_LONG        hConnection,
   BTI_ULONG       settingID,
   PVSETTINGINFO*  pSettingInfo);


/*
 * Name:
 *    PvGetValueLimit()
 *
 * Description:
 *    Retrieve upper and lower limits for setting of long type
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    settingID       [in]     Unique identifier for the setting
 *    pMaxValue       [out]    Address of a long integer that receives upper
 *                             limit value returned. If NULL is passed in
 *                             here, no value will be returned
 *    pMinValue       [out]    Address of a long integer that receives lower
 *                             limit value returned. If NULL is passed in
 *                             here, no value will be returned
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_INVALID_DATA_TYPE    The setting requested is not of long type
 *    P_E_FAIL                 Failed to get setting limits
 */
BTI_API PvGetValueLimit(
   BTI_LONG       hConnection,
   BTI_ULONG      settingID,
   BTI_LONG_PTR   pMaxValue,
   BTI_LONG_PTR   pMinValue);


/*
 * Name:
 *    PvCountSelectionItems()
 *
 * Description:
 *    Count the number of selection items for a setting of selection type.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    settingID       [in]     Unique identifier for the setting
 *    pNumItems       [out]    Address of an unsigned long that receives
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_DATA_TYPE    The setting requested is not of selection type
 *    P_E_FAIL                 Failed to get all selection choices
 */
BTI_API PvCountSelectionItems(
   BTI_LONG       hConnection,
   BTI_ULONG      settingID,
   BTI_ULONG_PTR  pNumItems);


/*
 * Name:
 *    PvGetAllPossibleSelections()
 *
 * Description:
 *    Retrieve all available selection choices for setting of selection type
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Optionally, number of selection items retrieved by calling
 *       PvCountSelectionItems();
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    settingID       [in]     Unique identifier for the setting
 *    pNumItems       [in/out] Address of an unsigned long that specifies
 *                             the array size on input, and receives total
 *                             number of selection choices on output
 *    pSelectionList  [out]    Array contains all available selection choices
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_DATA_TYPE    The setting requested is not of selection type
 *    P_E_BUFFER_TOO_SMALL     The array size is too small. In this case the
 *                             required size is returned in pNumItems.
 *    P_E_FAIL                 Failed to get all selection choices
 */
BTI_API PvGetAllPossibleSelections(
   BTI_LONG       hConnection,
   BTI_ULONG      settingID,
   BTI_ULONG_PTR  pNumItems,
   BTI_ULONG_PTR  pSelectionList);


/*
 * Name:
 *    PvGetSelectionString()
 *
 * Description:
 *    Retrieve display string for a specific choice of selection type setting
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *    Selection choices retrieved by calling PvGetAllSelections() or
 *                                           PvGetSelectionValue()
 *	  Sizes needed for pBufSize parameter can be obtained by calling PvGetSelectionStringSize() function.
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    settingID       [in]     Unique identifier for the setting
 *    selection       [in]     Selection choice index
 *    pBufSize        [in/out] Address of an unsigned long containing size
 *                             of the buffer allocated to receive the string.
 *                             It receives actual lenght of selection string
 *                             returned.
 *    dispString      [out]    Display string returned
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_DATA_TYPE    The setting requested is not of selection type
 *    P_E_BUFFER_TOO_SMALL     The buffer allocated is too small, returned
 *                             display string is truncated.  In this case the
 *                             required buffer size is returned in pBufSize.
 *    P_E_FAIL                 Failed to get all selection choices
 */
BTI_API PvGetSelectionString(
   BTI_LONG       hConnection,
   BTI_ULONG      settingID,
   BTI_ULONG      selection,
   BTI_ULONG_PTR  pBufSize,
   BTI_CHAR_PTR   dispString);


/*
 * Name:
 *    PvGetBooleanStrings()
 *
 * Description:
 *    Retrieve display string related to Boolean type setting
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    settingID       [in]     Unique identifier for the setting
 *    trueString      [out]    Display string for TRUE  (size >= 16 bytes)
 *    falseString     [out]    Display string for FALSE (size >= 16 bytes)
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_DATA_TYPE    The setting requested is not of Boolean type
 *    P_E_FAIL                 Failed to get display strings
 */
BTI_API PvGetBooleanStrings(
   BTI_LONG   hConnection,
   BTI_ULONG    settingID,
   BTI_LONG_PTR trueStringSize,
   BTI_CHAR_PTR trueString,
   BTI_LONG_PTR falseStringSize,
   BTI_CHAR_PTR falseString);


/*
 * Name:
 *    PvGetSettingMap()
 *
 * Description:
 *    Retrieve option and component for setting (
 *    option& component  for mapping settiing to DBUGetInfo or DBUSetInfo )
 *
 * Parameters:
 *    settingID       [in]     Unique identifier for the setting
 *    pComponentID    [out]    Address of an unsigned short for Component
                
 *    pOptionID       [out]    Address of an unsigned short for Option

 * Return Value:
 *    P_OK                     Successful
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_FAIL                 Failed to get
 */


BTI_API PvGetSettingMap(
   BTI_ULONG    settingID,
   BTI_WORD_PTR pComponentID,
   BTI_WORD_PTR pOptionID);


/*
 * Name:
 *    PvGetSettingHelp()
 *
 * Description:
 *    Retrieve help string related to setting
 *
 * Parameters:
 *    settingID       [in]     Unique identifier for the setting
 *    pBufSize        [in/out] Address of an unsigned long containing size
 *                             of buffer allocated to receive setting value.
 *                             Receives actual size of setting value returned
 *                             The size should include the null terminator.
 *    pHelpString     [out]    String value returned

 * Return Value:
 *    P_OK                     Successful
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_BUFFER_TOO_SMALL     The buffer allocated is too small, returned
 *                             display string is truncated.  In this case the
 *                             required buffer size is returned in pBufSize.
 *    P_E_FAIL                 Failed to get help strings
 */

BTI_API PvGetSettingHelp(
   BTI_ULONG    settingID,
   BTI_ULONG_PTR   pBufSize,
   BTI_CHAR_PTR    pHelpString);

/*
 * Name:
 *    PvGetSettingUnits()
 *
 * Description:
 *    Retrieve default units and suggested factor
 *    Only for Setting of Long type
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *	  PvGetSettingUnitsSize() call will return sizes needed for 
 *	  successful PvGetSettingUnits() call.
 *
 * Parameters:
 *    hConnection   [in]    Connection handle which identifies the server
 *    settingID     [in]    Unique identifier for the setting
 *    pBufSize      [in/out]  Address of an unsigned long containing size
 *                of buffer allocated to receive string of default units.
 *                Receives actual size of string of default units returned
 *                The size should include the null terminator.
 *    pValue    [out]   String of default value returned
 *    pfactor   [out]   Address of an unsigned long for factor
 *    pFBufSize     [in/out]  Address of an unsigned long containing size
 *                of buffer allocated to receive string of "factor" units.
 *                Receives actual size of string of default units returned
 *                The size should include the null terminator.
 *    pFValue   [out]   String of "factor" value returned
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_DATA_TYPE    The setting requested is not of Long type
 *    P_E_BUFFER_TOO_SMALL     The buffer allocated is too small, returned
 *                             display string is truncated.  In this case the
 *                             required buffer size is returned in pBufSize.
 *    P_E_FAIL                 Failed to get help strings
 */

BTI_API PvGetSettingUnits(
   BTI_LONG   hConnection,
   BTI_ULONG    settingID,
   BTI_ULONG_PTR  pBufSize,
   BTI_CHAR_PTR     pValue,
   BTI_ULONG_PTR  pfactor,
   BTI_ULONG_PTR  pFBufSize,
   BTI_CHAR_PTR     pFValue);


/*
 * Name:
 *    PvGetStringType()
 *
 * Description:
 *    Retrieve additional information about PVSETTING_STRING setting
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection   [in]    Connection handle which identifies the server
 *    settingID     [in]    Unique identifier for the setting
 *    pTypeString [out]   subtype of PVSETTING_STRING returned
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_DATA_TYPE    The setting requested is not of Long type
 *    P_E_FAIL                 Failed to get help strings
 */

BTI_API PvGetStringType(
   BTI_LONG     hConnection,
   BTI_ULONG    settingID,
   BTI_ULONG_PTR  pTypeString);

/* The following functions return sizes of different elements
The user can call these prior to calling corresponding functions to
find out how much memory to allocate
*/

/*
 * Name:
 *    PvGetSettingUnitsSize()
 *
 * Description:
 *    Returns the size in bytes of buffer size required to recieve
 *    information in PvGetSettingUnits() call
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection   [in]    Connection handle which identifies the server
 *    settingID     [in]    Unique identifier for the setting
 *	  pBufSize [in, out]  A pointer to a variable that will contain the required size of the buffer
 *    pBufSize [in, out]  A pointer to a variable that will contain the required size of the factor buffer
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_DATA_TYPE    The setting requested is not of Long type 
 */

BTI_API PvGetSettingUnitsSize(
    BTI_LONG        hConnection,
    BTI_ULONG       settingID,
    BTI_ULONG_PTR   pBufSize,        
    BTI_ULONG_PTR   pFBufSize);

/*
 * Name:
 *    PvGetSettingHelpSize()
 *
 * Description:
 *    Retrieve help string related to setting
 *
 * Parameters:
 *    settingID       [in]     Unique identifier for the setting
 *    pBufSize        [in/out] Address of an unsigned long that will contain size
 *                             of buffer needed to receive setting value. 
 *                             The size includes the null terminator. 

 * Return Value:
 *    P_OK                     Successful
 *    P_E_NULL_PTR             Call with NULL pointer 
 */

BTI_API PvGetSettingHelpSize(
    BTI_ULONG       settingID,
    BTI_ULONG_PTR   pBufSize);


/*
 * Name:
 *    PvGetSelectionStringSize()
 *
 * Description:
 *    Retrieve size of buffer needed for successful PvGetSelectionString() call.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    settingID       [in]     Unique identifier for the setting 
 *    pBufSize        [in/out] Address of an unsigned long containing size
 *										needed for buffer in PvGetSelectionString() call 
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_DATA_TYPE    The setting requested is not of selection type
 *    P_E_FAIL                 Failed to get the size.
 */
BTI_API PvGetSelectionStringSize(
   BTI_LONG       hConnection,
   BTI_ULONG      settingID,
   BTI_ULONG      selection,
   BTI_ULONG_PTR  pBufSize);


/*
 * Name:
 *    PvGetStringValueSize()
 *
 * Description:
 *    Retrieve the value (null terminated string) for a string type setting,
 *    from the data source specified by whichData. Some settings may
 *    return a list of strings separated by ';'.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    settingID       [in]     Unique identifier for the setting
 *    pBufSize        [in/out] Address of an unsigned long containing size
 *                             of buffer allocated to receive setting value.
 *                             Receives actual size of setting value returned
 *                             The size should include the null terminator. 
 *    whichData       [in]     Flag to indicate which value is requested.
 *                               PVDATA_DEFAULT  to return default value
 *                               PVDATA_CURRENT  to return current value
 *                               Others          to return current value
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_INVALID_DATA_TYPE    The setting requested is not of string type
 *    P_E_FAIL                 Failed to retrieve setting value
 */
BTI_API PvGetStringValueSize(
   BTI_LONG       hConnection,
   BTI_ULONG      settingID,
   BTI_ULONG_PTR  pBufSize,   
   BTI_SINT       whichData);

/*
 * Name:
 *    PvGetSettingListCount()
 *
 * Description:
 *    Retrieves numbef of settings belonging to the specified category.  This number can 
 *    then be used to allocate an array to pass to PvGetSettingList(). 
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    categoryID      [in]     Unique identifier for the category
 *    pNumSettings    [out]    Address of an unsigned long containing size
 *                             of the array needed for successful PvGetSettingList() call.
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer 
 *    P_E_FAIL                 Failed to get setting list
 */
BTI_API PvGetSettingListCount(
   BTI_LONG       hConnection,
   BTI_ULONG      categoryID,
   BTI_ULONG_PTR  pListCount);

/*
 * Name:
 *    PvGetCategoryListCount()
 *
 * Description:
 *    Retrieve the numbef of of categories available.
 *
 * Precondition:
 *    Connection established by calling PvConnectServer();
 *
 * Parameters:
 *    hConnection     [in]     Connection handle which identifies the server
 *    pNumCategories  [in/out] Address of an unsigned long containing size of
 *                             the array needed to receive category list. 
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_FAIL                 Failed to get category information
 */
BTI_API PvGetCategoryListCount(
   BTI_LONG       hConnection,
   BTI_ULONG_PTR  pListCount);

#pragma pack(pop)
#ifdef __cplusplus
}
#endif

#define _CONFIG_H_INCLUDED
#endif /* _CONFIG_H_INCLUDED */

