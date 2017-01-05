/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_tcl_ext_h_
#define	_tcl_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int bdb_HCommand(Tcl_Interp *, int, Tcl_Obj * const*);
#if DB_DBM_HSEARCH != 0
	int bdb_NdbmOpen(Tcl_Interp *, int, Tcl_Obj * const*, DBM **);
#endif
#if DB_DBM_HSEARCH != 0
	int bdb_DbmCommand(Tcl_Interp *, int, Tcl_Obj * const*, int, DBM *);
#endif
int ndbm_Cmd(ClientData, Tcl_Interp *, int, Tcl_Obj * const*);
void _DbInfoDelete(Tcl_Interp *, DBTCL_INFO *);
int db_Cmd(ClientData, Tcl_Interp *, int, Tcl_Obj * const*);
int tcl_CompactStat(Tcl_Interp *, DBTCL_INFO *);
int tcl_rep_send(DB_ENV *, const DBT *, const DBT *, const DB_LSN *, int, uint32);
int dbc_Cmd(ClientData, Tcl_Interp *, int, Tcl_Obj * const*);
int env_Cmd(ClientData, Tcl_Interp *, int, Tcl_Obj * const*);
int tcl_EnvRemove(Tcl_Interp *, int, Tcl_Obj * const*);
int tcl_EnvIdReset(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_EnvLsnReset(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_EnvVerbose(Tcl_Interp *, DB_ENV *, Tcl_Obj *, Tcl_Obj *);
int tcl_EnvAttr(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_EnvSetFlags(Tcl_Interp *, DB_ENV *, Tcl_Obj *, Tcl_Obj *);
int tcl_EnvTest(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_EnvGetEncryptFlags(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
void tcl_EnvSetErrfile(Tcl_Interp *, DB_ENV *, DBTCL_INFO *, char *);
void tcl_EnvSetMsgfile(Tcl_Interp *, DB_ENV *, DBTCL_INFO *, char *);
int tcl_EnvSetErrpfx(Tcl_Interp *, DB_ENV *, DBTCL_INFO *, char *);
int tcl_EnvStatPrint(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
DBTCL_INFO *_NewInfo(Tcl_Interp *, void *, char *, enum INFOTYPE);
void *_NameToPtr(const char *);
DBTCL_INFO *_PtrToInfo(const void *);
DBTCL_INFO *_NameToInfo(const char *);
void  _SetInfoData(DBTCL_INFO *, void *);
void  _DeleteInfo(DBTCL_INFO *);
int _SetListElem(Tcl_Interp *, Tcl_Obj *, void *, uint32, void *, uint32);
int _SetListElemInt(Tcl_Interp *, Tcl_Obj *, void *, long);
int _SetListElemWideInt(Tcl_Interp *, Tcl_Obj *, void *, int64_t);
int _SetListRecnoElem(Tcl_Interp *, Tcl_Obj *, db_recno_t, uchar *, uint32);
int _SetListHeapElem(Tcl_Interp *, Tcl_Obj *, DB_HEAP_RID, uchar *, uint32);
int _Set3DBTList(Tcl_Interp *, Tcl_Obj *, DBT *, int, DBT *, int, DBT *);
int _SetMultiList(Tcl_Interp *, Tcl_Obj *, DBT *, DBT*, DBTYPE, uint32);
int _GetGlobPrefix(char *, char **);
int _ReturnSetup(Tcl_Interp *, int, int, char *);
int _ErrorSetup(Tcl_Interp *, int, char *);
void _ErrorFunc(const DB_ENV *, const char *, const char *);
#ifdef CONFIG_TEST 
	void _EventFunc(DB_ENV *, uint32, void *);
#endif
int _GetLsn(Tcl_Interp *, Tcl_Obj *, DB_LSN *);
int _GetRid(Tcl_Interp *, Tcl_Obj *, DB_HEAP_RID *);
int _GetUInt32(Tcl_Interp *, Tcl_Obj *, uint32 *);
Tcl_Obj *_GetFlagsList(Tcl_Interp *, uint32, const FN *);
void _debug_check ();
int _CopyObjBytes (Tcl_Interp *, Tcl_Obj *obj, void *, uint32 *, int *);
int tcl_LockDetect(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_LockGet(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_LockStat(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_LockStatPrint(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_LockTimeout(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_LockVec(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_LogArchive(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_LogCompare(Tcl_Interp *, int, Tcl_Obj * const*);
int tcl_LogFile(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_LogFlush(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_LogGet(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_LogPut(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_LogStat(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_LogStatPrint(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int logc_Cmd(ClientData, Tcl_Interp *, int, Tcl_Obj * const*);
int tcl_LogConfig(Tcl_Interp *, DB_ENV *, Tcl_Obj *, Tcl_Obj *);
int tcl_LogGetConfig(Tcl_Interp *, DB_ENV *, Tcl_Obj *);
void _MpInfoDelete(Tcl_Interp *, DBTCL_INFO *);
int tcl_MpSync(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_MpTrickle(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_Mp(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *, DBTCL_INFO *);
int tcl_MpStat(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_MpStatPrint(Tcl_Interp *, int,  Tcl_Obj * const*, DB_ENV *);
int tcl_Mutex(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_MutFree(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_MutGet(Tcl_Interp *, DB_ENV *, int);
int tcl_MutLock(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_MutSet(Tcl_Interp *, Tcl_Obj *, DB_ENV *, int);
int tcl_MutStat(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_MutStatPrint(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_MutUnlock(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_RepConfig(Tcl_Interp *, DB_ENV *, Tcl_Obj *);
int tcl_RepGetTwo(Tcl_Interp *, DB_ENV *, int);
int tcl_RepGetConfig(Tcl_Interp *, DB_ENV *, Tcl_Obj *);
int tcl_RepGetTimeout(Tcl_Interp *, DB_ENV *, Tcl_Obj *);
int tcl_RepGetAckPolicy(Tcl_Interp *, int, Tcl_Obj * const *, DB_ENV *);
int tcl_RepGetLocalSite(Tcl_Interp *, int, Tcl_Obj * const *, DB_ENV *);
int tcl_RepElect(Tcl_Interp *, int, Tcl_Obj * const *, DB_ENV *);
int tcl_RepFlush(Tcl_Interp *, int, Tcl_Obj * const *, DB_ENV *);
int tcl_RepSync(Tcl_Interp *, int, Tcl_Obj * const *, DB_ENV *);
int tcl_RepLease (Tcl_Interp *, int, Tcl_Obj * const *, DB_ENV *);
int tcl_RepInmemFiles (Tcl_Interp *, DB_ENV *);
int tcl_RepLimit(Tcl_Interp *, int, Tcl_Obj * const *, DB_ENV *);
int tcl_RepNSites(Tcl_Interp *, int, Tcl_Obj * const *, DB_ENV *);
int tcl_RepRequest(Tcl_Interp *, int, Tcl_Obj * const *, DB_ENV *);
int tcl_RepNoarchiveTimeout(Tcl_Interp *, DB_ENV *);
int tcl_RepTransport (Tcl_Interp *, int, Tcl_Obj * const *, DB_ENV *, DBTCL_INFO *);
int tcl_RepStart(Tcl_Interp *, int, Tcl_Obj * const *, DB_ENV *);
int tcl_RepProcessMessage(Tcl_Interp *, int, Tcl_Obj * const *, DB_ENV *);
int tcl_RepStat(Tcl_Interp *, int, Tcl_Obj * const *, DB_ENV *);
int tcl_RepStatPrint(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_RepMgr(Tcl_Interp *, int, Tcl_Obj * const *, DB_ENV *);
int tcl_RepMgrSiteList(Tcl_Interp *, int, Tcl_Obj * const *, DB_ENV *);
int tcl_RepMgrStat(Tcl_Interp *, int, Tcl_Obj * const *, DB_ENV *);
int tcl_RepMgrStatPrint(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_RepApplied(Tcl_Interp *, int, Tcl_Obj * const *, DB_ENV *);
int seq_Cmd(ClientData, Tcl_Interp *, int, Tcl_Obj * const*);
void _TxnInfoDelete(Tcl_Interp *, DBTCL_INFO *);
int tcl_TxnCheckpoint(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_Txn(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *, DBTCL_INFO *);
int tcl_CDSGroup(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *, DBTCL_INFO *);
int tcl_TxnStat(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_TxnStatPrint(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_TxnTimeout(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *);
int tcl_TxnRecover(Tcl_Interp *, int, Tcl_Obj * const*, DB_ENV *, DBTCL_INFO *);
int bdb_RandCommand(Tcl_Interp *, int, Tcl_Obj * const*);
int tcl_LockMutex(DB_ENV *, db_mutex_t);
int tcl_UnlockMutex(DB_ENV *, db_mutex_t);

#if defined(__cplusplus)
}
#endif
#endif /* !_tcl_ext_h_ */
