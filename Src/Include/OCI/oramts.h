#ifndef ORACLE_ORAMTS
# define ORACLE_ORAMTS

#include <windows.h>
#include <oci.h>
#include <oratypes.h>

/*----------------------------------------------------------------------------*/
/* various error codes reoprted by the OraMTS<> functions                     */
/*----------------------------------------------------------------------------*/
/* All error codes here:                                                      */
/*    0       -        - success                                              */
/*    1       - 1000   - Public APIs (kpntsvcget(), kpntsvcrelease(), etc.)   */ 
/*    1001    - 2000   - Dispenser component                                  */
/*    2001    - 3000   - Database identifier object                           */
/*    3001    - 4000   - Server object                                        */
/*    4001    - 5000   - Session object                                       */
/*    5001    - 6000   - RPC-related                                          */
/*    6001    - 7000   - Miscellaneous                                        */
/*----------------------------------------------------------------------------*/
/* Error codes for the connection pool APIs.                                  */
/*----------------------------------------------------------------------------*/
#define  ORAMTSERR_NOERROR            (0   )       /* success code            */
/* ------------ dispenser object related errors ----------------------------- */
#define  ORAMTSERR_NOMTXDISPEN        (1001)       /* no MTXDM.DLL available  */
#define  ORAMTSERR_DSPCREAFAIL        (1002)       /* failure to create dispen*/
#define  ORAMTSERR_DSPMAXSESSN        (1003)       /* exceeded max sessions   */
#define  ORAMTSERR_DSPINVLSVCC        (1004)       /* invalid OCI Svc ctx     */
#define  ORAMTSERR_DSPNODBIDEN        (1005)       /* can't create new dbiden */
/*------------- database identifier related errors ---------------------------*/
#define  ORAMTSERR_NOSERVEROBJ        (2001)       /* unable to alloc a server*/
/* ------------ server object related errors -------------------------------- */
#define  ORAMTSERR_INVALIDSRVR        (3001)       /* invalid server object   */
#define  ORAMTSERR_FAILEDATTCH        (3002)       /* failed attach to Oracle */
#define  ORAMTSERR_FAILEDDETCH        (3003)       /* failed detach from db   */
#define  ORAMTSERR_FAILEDTRANS        (3004)       /* failed to start trans.  */
#define  ORAMTSERR_SETATTRIBUT        (3005)       /* OCI set attrib failed   */       
#define  ORAMTSERR_CONNXBROKEN        (3006)       /* conn to Oracle broken   */
#define  ORAMTSERR_NOTATTACHED        (3007)       /* not attached to Oracle  */
#define  ORAMTSERR_ALDYATTACHD        (3008)       /* alrdy attached to Oracle*/
/* ------------ session object related errors ------------------------------- */
#define  ORAMTSERR_INVALIDSESS        (4001)       /* invalid session object  */
#define  ORAMTSERR_FAILEDLOGON        (4002)       /* failed logon to Oracle  */
#define  ORAMTSERR_FAILEDLOGOF        (4003)       /* failed logoff from db   */
#define  ORAMTSERR_TRANSEXISTS        (4004)       /* no transaction beneath  */
#define  ORAMTSERR_LOGONEXISTS        (4005)       /* already logged on to db */
#define  ORAMTSERR_NOTLOGGEDON        (4006)       /* not logged on to Oracle */
/* ------------ RPC errors -------------------------------------------------- */
#define  ORAMTSERR_RPCINVLCTXT        (5001)       /* RPC context is invalid  */
#define  ORAMTSERR_RPCCOMMUERR        (5002)       /* generic communic. error */
#define  ORAMTSERR_RPCALRDYCON        (5003)       /* endpoint already connect*/
#define  ORAMTSERR_RPCNOTCONNE        (5004)       /* endpoint not connected  */
#define  ORAMTSERR_RPCPROTVIOL        (5005)       /* protocol violation      */
#define  ORAMTSERR_RPCACCPTIMO        (5006)       /* timeout accepting conn. */
#define  ORAMTSERR_RPCILLEGOPC        (5007)       /* invalid RPC opcode      */
#define  ORAMTSERR_RPCBADINCNO        (5008)       /* mismatched incarnation# */
#define  ORAMTSERR_RPCCONNTIMO        (5009)       /* client connect timeout  */
#define  ORAMTSERR_RPCSENDTIMO        (5010)       /* synch. send timeout     */
#define  ORAMTSERR_RPCRECVTIMO        (5011)       /* synch. receive timedout */
#define  ORAMTSERR_RPCCONRESET        (5012)       /* connection reset by peer*/
/* ------------ miscellaneous errors ---------------------------------------- */
#define  ORAMTSERR_INVALIDARGU        (6001)       /* invalid args to function*/
#define  ORAMTSERR_INVALIDOBJE        (6002)       /* an object was invalid   */
#define  ORAMTSERR_ILLEGALOPER        (6003)       /* illegal operation       */
#define  ORAMTSERR_ALLOCMEMORY        (6004)       /* memory allocation error */
#define  ORAMTSERR_ERRORSYNCHR        (6005)       /* synchr. object error    */
#define  ORAMTSERR_NOORAPROXY         (6006)       /* no Oracle Proxy server  */
#define  ORAMTSERR_ALRDYENLIST        (6007)       /* session already enlisted*/
#define  ORAMTSERR_NOTENLISTED        (6008)       /* session is not enlisted */
#define  ORAMTSERR_TYPMANENLIS        (6009)       /* illeg on manuenlst sess */
#define  ORAMTSERR_TYPAUTENLIS        (6010)       /* illeg on autoenlst sess */
#define  ORAMTSERR_TRANSDETACH        (6011)       /* error detaching trans.  */
#define  ORAMTSERR_OCIHNDLALLC        (6012)       /* OCI handle alloc error  */
#define  ORAMTSERR_OCIHNDLRELS        (6013)       /* OCI handle dealloc error*/
#define  ORAMTSERR_TRANSEXPORT        (6014)       /* error exporting trans.  */
#define  ORAMTSERR_OSCREDSFAIL	      (6105)       /* error getting NT creds  */
/*----------------------------------------------------------------------------*/
/* //////////// Connection flags on call OraMTSSvcGet() ///////////////////// */
/*----------------------------------------------------------------------------*/
#define ORAMTS_CFLG_ALLDEFAULT (0x00000000)    /* default flags               */
#define ORAMTS_CFLG_NOIMPLICIT (0x00000001)    /* don't do implicit enlistment*/
#define ORAMTS_CFLG_UNIQUESRVR (0x00000002)    /* need a separate Net8 connect*/
#define ORAMTS_CFLG_SYSDBALOGN (0x00000004)    /* logon as a SYSDBA           */
#define ORAMTS_CFLG_SYSOPRLOGN (0x00000010)    /* logon as a SYSOPER          */
#define ORAMTS_CFLG_PRELIMAUTH (0x00000020)    /* preliminary internal login  */
/*----------------------------------------------------------------------------*/
/* //////////// Enlistment flags on call kpntsvcenlist() //////////////////// */
/*----------------------------------------------------------------------------*/
#define ORAMTS_ENFLG_DEFAULT   (0x00000000)   /* default flags                */
#define ORAMTS_ENFLG_RESUMTX   (0x00000001)   /* resume a detached transact.  */
#define ORAMTS_ENFLG_DETCHTX   (0x00000002)   /* detached from the transact.  */

#ifdef __cplusplus
 extern "C" 
{
#endif /* __cplusplus */

/*----------------------------------------------------------------------------*/
/* /////////////////////// Public functions ///////////////////////////////// */
/*----------------------------------------------------------------------------*/
/* Name      : OraMTSSvcGet()                                                 */
/* Purpose   : Obtain a pooled connection from the OCI connection pool.       */
/* Arguments : text       *lpUname: (IN ) user name for session               */
/*             text       *lpPsswd: (IN ) password for session                */
/*             text       *lpDbnam: (IN ) database alias for connection       */
/*             OCISvcCtx **pOCISvc: (OUT) ptr to OCI svc context handle.      */
/*             OCIEnv    **pOCIEnv: (OUT) ptr to OCI enironment handle.       */
/*             ub4         ConFlgs: (IN ) connection flags                    */
/* Returns   : ORAMTSERR_NOERROR on success.                                  */
/* Notes     : This call returns a pooled OCI connection to the caller. If the*/
/*             caller is a transactional MTS component, currently in a Dtc TM */
/*             controlled transaction, we will attempt to enlist the session  */
/*             in the transaction unless the caller explicitly asks us not to */
/*             to do so via the dwConnFlgs. In all other cases, we will not   */
/*             attempt to enlist.                                             */
/*----------------------------------------------------------------------------*/
DWORD  OraMTSSvcGet( 
                     text       *lpUname,
                     text       *lpPsswd,
                     text       *lpDbnam,              
                     OCISvcCtx **pOCISvc,
                     OCIEnv    **pOCIEnv,
                     ub4         ConFlg
                   );
/*----------------------------------------------------------------------------*/
/* Name      : OraMTSSvcRel()                                                 */
/* Purpose   : Release a pooled OCI connection back to the pool.              */
/* Arguments : OCISvcCtx *OCISvc: (IN ) OCI service context for pooled conn.  */
/* Returns   : ORAMTSERR_NOERROR on success.                                  */
/* Notes     : An OCI service context obtained via a previous call to kpntsvc */
/*             get() from the pool is released back to the pool. Once released*/
/*             back to the pool, the OCI svc., its environment handle, and all*/
/*             children handles are invalid.                                  */
/*----------------------------------------------------------------------------*/
DWORD OraMTSSvcRel(OCISvcCtx *OCISvc);
/*----------------------------------------------------------------------------*/
/* Name      : OraMTSSvcEnlist()                                              */
/* Purpose   : Enlist an OCI connection or service context in a DTC-trans.    */
/* Arguments : OCISvcCtx *OCISvc : (IN ) OCI service context for pooled conn. */
/*             OCIError  *OCIErr : (IN/OUT) OCI error handle for errors. This */
/*             is ignored if the service context represents a pooled conn.    */
/*             void      *lpTrans: (IN ) ptr. to DTC-controlled transaction   */
/*             unsigned   dwFlags: (IN ) enlistment flags                     */
/* Returns   : ORAMTSERR_NOERROR on success.                                  */
/* Notes     : An OCI service context obtained via a previous call to kpntsvc */
/*             get() or allocated otherwise, is enlisted via this call in a   */
/*             DTC-transaction. For pooled connections, the underlying session*/
/*             object must be explicitly enlistable. A subsequent call to the */
/*             same function with a NULL lpTrans will cause deenlistment from */
/*             the transaction. Callers should allocate a connection, then    */
/*             enlist the connection, perform work, deenlist the connection,  */
/*             then release the connection and then attempt to commit or abrt.*/
/*             For nonpooled connections, one must allocate a transaction han */
/*             dle and assosciate the OCI service context with that handle.The*/
/*             transaction handle must be undisturbed till the service context*/
/*             is finally disposed off. Once a nonpooled connection has been  */
/*             enlisted, it can be detached and attached to the underlying Ora*/
/*             cle transaction via the same call using the dwFlags parameter. */
/*             If one needs to detach from the Oracle transaction lpTrans is  */
/*             NULL and dwFlags is ORAMTS_ENFLG_DETCHTX. If one wants to resum*/
/*             the current transaction lpTrans is not NULL and dwFlags is set */
/*             to ORAMTS_ENFLG_RESUMTX.                                       */
/*----------------------------------------------------------------------------*/
DWORD OraMTSSvcEnlist(
                       OCISvcCtx  *OCISvc, 
                       OCIError   *OCIErr, 
                       void       *lpTrans, 
                       unsigned    dwFlags
                     );
/*----------------------------------------------------------------------------*/
/* Name      : OraMTSSvcEnlistEx()                                            */
/* Purpose   : Enlist an OCI connection or service context in a DTC-trans.    */
/* Arguments : OCISvcCtx *OCISvc  : (IN ) OCI service context for pooled conn.*/
/*             OCIError  *OCIErr  : (IN/OUT) OCI error handle for errors. This*/
/*             is ignored if the service context represents a pooled conn.    */
/*             void      *lpTrans : (IN ) ptr. to DTC-controlled transaction  */
/*             unsigned   dwFlags : (IN ) enlistment flags                    */
/*	       char	 *lpDBName: (IN ) Net8 dbalias (connect alias).	      */
/* Returns   : ORAMTSERR_NOERROR on success.                                  */
/* Notes     : An OCI service context obtained via a previous call to OraMTSS-*/
/*             vcGet() or allocated otherwise, is enlisted via this call in a */
/*             DTC-transaction. For pooled connections, the underlying session*/
/*             object must be explicitly enlistable. A subsequent call to the */
/*             same function with a NULL lpTrans will cause deenlistment from */
/*             the transaction. Callers should allocate a connection, then    */
/*             enlist the connection, perform work, deenlist the connection,  */
/*             then release the connection and then attempt to commit or abrt.*/
/*             For nonpooled connections, one must allocate a transaction han */
/*             dle and assosciate the OCI service context with that handle.The*/
/*             transaction handle must be undisturbed till the service context*/
/*             is finally disposed off. Once a nonpooled connection has been  */
/*             enlisted, it can be detached and attached to the underlying Ora*/
/*             cle transaction via the same call using the dwFlags parameter. */
/*             If one needs to detach from the Oracle transaction lpTrans is  */
/*             NULL and dwFlags is ORAMTS_ENFLG_DETCHTX. If one wants to resum*/
/*             the current transaction lpTrans is not NULL and dwFlags is set */
/*             to ORAMTS_ENFLG_RESUMTX. The lpDBName parameter is only used   */
/*             when enlisting non-pooled connections. The parameter is used to*/
/*	       cache information regarding the proxy service for the database */
/*	       to improve performance. This parameter is ignored for pooled   */
/*	       connections and also for de-enlistments.			      */
/*----------------------------------------------------------------------------*/
DWORD OraMTSSvcEnlistEx(
                       		OCISvcCtx  *OCISvc, 
				OCIError   *OCIErr, 
				void       *lpTrans, 
				unsigned    dwFlags,
				char	   *lpDBName
                       );
/*----------------------------------------------------------------------------*/
/* Name      : OraMTSEnlCtxGet()					      */
/* Purpose   : Create an enlistment context for a non-pooled OCI connection.  */
/* Arguments : text*	  pUname :(IN ) database username for connection.     */
/*	       text*	  pPsswd :(IN ) database password for connection.     */
/*	       text*	  pDbnam :(IN ) database connect str for connection.  */
/*	       OCISvcCtx* pOCISvc:(IN ) OCI service context for pooled conn.  */
/*             OCIError*  pOCIErr:(IN ) OCI error handle for errors.	      */
/*	       ub4	  dwFlags:(IN ) enlistment flags. 0 is the only value */
/*				  currently allowed.			      */
/*	       void**	  pCtxt	 :(OUT) enlistment context to be created.     */
/*             is ignored if the service context represents a pooled conn.    */
/* Notes     : This call setus up anenlistment context for a non-pooled OCI   */
/*	       connection. This call must be issued just after the caller has */
/*	       established the OCI connection to the database. Once created   */
/*	       this context can be passed into kpntjoindtctxn() calls. Prior  */
/*	       to  tearing down the OCI connection kpntenlistctxrel() must be */
/*	       called to destroy the enlistment context.		      */
/* Returns   : ORAMTSERR_NOERROR on success.                                  */
/*----------------------------------------------------------------------------*/
DWORD OraMTSEnlCtxGet(
			text*		lpUname,
			text*		lpPsswd,
			text*		lpDbnam,              
			OCISvcCtx*	pOCISvc,
			OCIError*	pOCIErr,
			ub4		dwFlags,
			void**		pCtxt
		     );
/*----------------------------------------------------------------------------*/
/* Name      : OraMTSEnlCtxRel()					      */
/* Purpose   : Destroy the enlistment context for a non-pooled OCI connection.*/
/* Arguments : void* pCtxt : (IN ) enlistment context to destroy.	      */
/* Notes     : Before dropping a non-pooled OCI connection a client must call */
/*	       kpntenlistctxrel() to destroy any enlistment context it may    */
/*	       created for that connection. Since the enlistment context may  */
/*	       maintain OCI handles allocated off the connection's env handle */
/*	       it is imperative that the env handle not be deleted for the    */
/*	       of the assosciated enlistment context.			      */
/* Returns   : ORAMTSERR_NOERROR on success.                                  */
/*----------------------------------------------------------------------------*/
DWORD OraMTSEnlCtxRel(void* pCtxt);
/*----------------------------------------------------------------------------*/
/* Name      : OraMTSJoinTxn()						      */
/* Purpose   : Enlist an non-pooled OCI connection in an MSDTC-transaction.   */
/* Arguments : void* pCtxt : (IN ) enlistment context for the OCI connection  */
/*	       void* pTrans: (IN ) reference to MSDTC transaction object.     */
/* Notes     : This call is used by clients with non-pooled OCI connections to*/
/*	       enlist these connections in MSDTC-coordinated transactions. The*/
/*	       client passes in the opaque reference to the enlistment context*/
/*	       representing the OCI connection along with a reference to an   */
/*	       MSDTC transaction object. If pTrans is NULL the OCI connection */
/*	       is deenlisted from any MSDTC transaction it is currently enlis-*/
/*	       -ted in. It is perfectly legal to enlist a previously enlisted */
/*	       OCI connection in a different MSDTC transaction.		      */
/* Returns   : ORAMTSERR_NOERROR on success.                                  */
/*----------------------------------------------------------------------------*/
DWORD OraMTSJoinTxn(void *pCtxt, void* pTrans);
/*----------------------------------------------------------------------------*/
/* Name      : OraMTSTransTest()					      */
/* Purpose   : Test if we are running inside an MTS initiated transaction.    */
/* Arguments : - None -                                                       */
/* Returns   : TRUE if we are running inside an MTS transaction. FALSE other- */
/*	       wise.							      */
/* Notes     : This can be made by MTS transactional components to check if   */
/*	       component is executing within the context of an MTS transaction*/
/*	       Note, that this can only test MTS initiated transactions. Txns */
/*	       started by directly calling the MSDTC will not be detected.    */
/*----------------------------------------------------------------------------*/
BOOL OraMTSTransTest();
/*----------------------------------------------------------------------------*/
/* Name      : OraMTSOCIErrGet()					      */
/* Purpose   : Retrieve the OCI error (if any) from the last kpntsvc operation*/
/* Arguments : DWORD  *dwErr : (OUT) error code.			      */ 
/*	       LPTSTR lpcEMsg: (OUT) buffer for the error message if any.     */ 
/*	       DWORD  *lpdLen: (IN/OUT)	size of lpcEmsg in; msg bytes out.    */
/* Returns   : TRUE if an OCI error was encountered. FALSE otherwise. If TRUE */
/*	       and lpcEMsg and lpdLen are valid, and we do have a stashed err */
/*	       message, upto lpdLen bytes are copied into lpcEMsg. lpdLen is  */
/*	       set to the actual # of message bytes.			      */
/* Notes     : - None -							      */
/*----------------------------------------------------------------------------*/
BOOL OraMTSOCIErrGet(DWORD *dwErr, LPTSTR lpcEMsg, DWORD *lpdLen);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ORACLE_ORAMTS */
