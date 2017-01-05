#ifndef _CONNECT_H_INCLUDED
/*************************************************************************
**
**  Copyright 1998-1999 Pervasive Software Inc. All Rights Reserved
**
*************************************************************************/
/*************************************************************************
*   CONNECT.H
*
*
*   This header prototypes the server connection functions
*   of the Pervasive Distributed Tuning Interface.
*
*     The following functions are found in this file:
*
*       PvConnectServer()
*       PvDisconnect
*       PvGetServerName()
*       PvStart()
*       PvStop()
*
*
*************************************************************************/
/*===========================================================================
 *
 * Connection API
 *
 *=========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Name:
 *    PvConnectServer()
 *
 * Description:
 *    Attempts to connect to the target server that has Pervasive database
 *    server installed.  If connection is established successfully, a
 *    connection handle is returned for subsequent references.
 *
 *    The implementation should perform necessary initializations when called
 *    the first time.
 *
 *    Multiple simultaneous connections are allowed.
 *
 * Parameters:
 *    serverName      [in]       Server name to connect to
 *    userName        [in]       User name
 *    password        [in]       User password
 *    phConnection    [in/out]   Address of a unsigned long integer that
 *                               receives the connection handle if
 *                               connection is successful
 *
 * Return Value:
 *    P_OK           Successful
 *    P_E_NULL_PTR   Call with NULL pointer
 *    P_E_FAIL       Failed to connect to the named server
 */
BTI_API PvConnectServer(
   BTI_CHAR_PTR   serverName,
   BTI_CHAR_PTR   userName,
   BTI_CHAR_PTR   password,
   BTI_LONG_PTR   phConnection);




/*
 * Name:
 *    PvDisconnect
 *
 * Description:
 *    Attempts to disconnect the connection identified by the handle.
 *
 * Parameters:
 *    hConnection    [in]     Connection handle to be disconnected
 *
 * Return Value:
 *    P_OK                    Successful
 *    P_E_INVALID_HANDLE      Invalid connection handle
 *    P_E_FAIL                Failed to disconnect from the server
 */
BTI_API PvDisconnect(
   BTI_LONG hConnection);


/*
 * Name:
 *    PvGetServerName()
 *
 * Description:
 *    Retrieve name of the connected server indicated by connection handle.
 *
 * Parameters:
 *    hConnection    [in]      Connection handle which identifies the server
 *    pBufSize       [in/out]  Address of an unsigned long containing size
 *                             of the buffer allocated to receive server name
 *                             Receives actual size of server name returned.
 *    serverName     [in/out]  Returned server name if successful, empty
 *                             string otherwise
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_INVALID_HANDLE       Invalid connection handle
 *    P_E_NULL_PTR             Call with NULL pointer
 *    P_E_BUFFER_TOO_SMALL     The buffer is too small for the string.
 *                             In this case the required buffer size is
 *                             returned in pBufSize.
 *    P_E_FAIL                 Failed to retrieve server name
 */
BTI_API PvGetServerName(
   BTI_LONG       hConnection,
   BTI_ULONG_PTR  pBufSize,
   BTI_CHAR_PTR   serverName);



/*
 * Name:
 *    PvStart()
 *
 * Parameters:
 *    reserved    [in]
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_FAIL                 Failed
 */
BTI_API PvStart(BTI_LONG reserved);


/*
 * Name:
 *    PvStop()
 *
 * Parameters:
 *    phConnection    [in]    Address of connection handle
 *
 * Return Value:
 *    P_OK                     Successful
 *    P_E_FAIL                 Failed
 */
BTI_API PvStop(BTI_LONG_PTR preserved);

#ifdef __cplusplus
}
#endif

#define _CONNECT_H_INCLUDED
#endif /* _CONNECT_H_INCLUDED */

