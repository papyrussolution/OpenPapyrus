// DBSESS.CPP
// Copyright (c) A.Sobolev 2003, 2004, 2005, 2006, 2007, 2009, 2010, 2011, 2013, 2015, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

DbThreadLocalArea::DbRegList::DbRegList() : Ht(512)
{
	Init();
}

void DbThreadLocalArea::DbRegList::Init()
{
	Tab.clear();
	OpenCount = 0;
	OpenPeak = 0;
}

uint DbThreadLocalArea::DbRegList::GetMaxEntries() const { return Tab.getCount(); }

int FASTCALL DbThreadLocalArea::DbRegList::AddEntry(void * pTbl, void * pSupplementPtr)
{
	int    h = 0;
	for(uint i = 0; !h && i < Tab.getCount(); i++) {
		if(Tab.at(i) == 0) {
			Tab.at(i) = pTbl;
			OpenCount++;
			SETMAX(OpenPeak, OpenCount);
			h = (int)(i + 1);
		}
	}
	if(!h) {
		void * ptr = pTbl;
		Tab.insert(&ptr);
		OpenCount++;
		SETMAX(OpenPeak, OpenCount);
		h = static_cast<int>(Tab.getCount());
	}
	if(h && pSupplementPtr) {
		Ht.Add(pSupplementPtr, h, 0);
	}
	assert(Tab.getCount() == OpenPeak);
	return h;
}

void DbThreadLocalArea::DbRegList::FreeEntry(int handle, void * pSupplementPtr)
{
	assert(handle > 0 && handle <= static_cast<int>(Tab.getCount()));
	if(handle > 0 && handle <= static_cast<int>(Tab.getCount())) {
		OpenCount--;
		Tab.at(handle-1) = 0;
		if(pSupplementPtr) {
			uint   ht_val = 0;
            Ht.Del(pSupplementPtr, &ht_val);
			assert((int)ht_val == handle);
		}
	}
	assert(Tab.getCount() == OpenPeak);
}

void * FASTCALL DbThreadLocalArea::DbRegList::GetPtr_(int handle) const
{
	assert(handle > 0 && handle <= static_cast<int>(Tab.getCount()));
	return (handle > 0 && handle <= static_cast<int>(Tab.getCount())) ? Tab.at(handle-1) : 0;
}

void * FASTCALL DbThreadLocalArea::DbRegList::GetBySupplementPtr(const void * pSupplementPtr) const
{
	uint   ht_val = 0;
	if(Ht.Search(pSupplementPtr, &ht_val, 0)) {
		assert(ht_val > 0 && ht_val <= static_cast<int>(Tab.getCount()));
		return (ht_val > 0 && ht_val <= static_cast<int>(Tab.getCount())) ? Tab.at(ht_val-1) : 0;
	}
	else
		return 0;
}

DbThreadLocalArea::DbThreadLocalArea() : P_StFileName(0), P_CurDict(0)
{
	ClientID[0] = 0;
	Init();
}

DbThreadLocalArea::~DbThreadLocalArea()
{
	delete P_CurDict;
	delete P_StFileName;
}

void DbThreadLocalArea::Init()
{
	Id = 0;
	State = 0;
	LastBtrErr = 0;
	LastDbErr = 0;
	DbTableReg.Init();
	ZDELETE(P_StFileName);
	ZDELETE(P_CurDict);
}

uint DbThreadLocalArea::GetTabEntriesCount() const { return DbTableReg.GetMaxEntries(); }
int  FASTCALL DbThreadLocalArea::AddTableEntry(DBTable * pTbl) { return DbTableReg.AddEntry(pTbl, 0); }
void FASTCALL DbThreadLocalArea::FreeTableEntry(int handle) { DbTableReg.FreeEntry(handle, 0); }

DBTable * DbThreadLocalArea::GetCloneEntry(BTBLID tblID) const
{
	for(uint i = 1; i <= DbTableReg.GetMaxEntries(); i++) {
		DBTable * p_tbl = static_cast<DBTable *>(DbTableReg.GetPtr_(i));
		if(p_tbl && p_tbl->GetTableID() == tblID)
			return p_tbl;
	}
	return 0;
}

void FASTCALL DbThreadLocalArea::InitErrFileName(const char * pFileName)
{
	if(LastBtrErr && pFileName) {
		SETIFZ(P_StFileName, new char[MAX_PATH]);
		strnzcpy(P_StFileName, pFileName, MAX_PATH);
	}
}

const char * DbThreadLocalArea::GetLastErrFileName() const { return P_StFileName; }

int DbThreadLocalArea::StartTransaction()
{
	int    ok = P_CurDict ? P_CurDict->StartTransaction() : Btrieve::StartTransaction(1);
	if(ok) {
		State |= stTransaction;
	}
	return ok;
}

int DbThreadLocalArea::StartTransaction_DbDepend()
{
	int    ok = 1;
	if(P_CurDict) {
		ok = (P_CurDict->GetCapability() & DbProvider::cDbDependTa) ? P_CurDict->StartTransaction() : -1;
	}
	else
		ok = Btrieve::StartTransaction(1);
	if(ok > 0) {
		State |= stTransaction;
	}
	return ok;
}

int DbThreadLocalArea::CommitWork()
{
	int    ok = P_CurDict ? P_CurDict->CommitWork() : Btrieve::CommitWork();
	if(ok) {
		State &= ~stTransaction;
	}
	return ok;
}

int DbThreadLocalArea::RollbackWork()
{
	int    ok = P_CurDict ? P_CurDict->RollbackWork() : Btrieve::RollbackWork();
	if(ok) {
		State &= ~stTransaction;
	}
	return ok;
}
//
// Регистрация типа SRowId
//
class SRowId : public DataType {
public:
	SRowId();
	int    comp(const void *, const void *) const;
	char * tostr(const void *, long, char *) const;
	int    fromstr(void *, long, const char *) const;
	int    base() const;
	void   tobase(const void * s, void * b) const;
	int    baseto(void * s, const void * b) const;
	void   minval(void *) const;
	void   maxval(void *) const;
};

SRowId::SRowId() : DataType(sizeof(DBRowId))
{
}

int SRowId::comp(const void * i1, const void * i2) const
{
	return memcmp(i1, i2, sizeof(DBRowId));
}

char * SRowId::tostr(const void * pData, long, char * pStr) const
{
	const DBRowId * p_row_id = static_cast<const DBRowId *>(pData);
	if(p_row_id) {
		SString temp_buf;
		p_row_id->ToStr__(temp_buf).CopyTo(pStr, 0);
	}
	else
		pStr[0] = 0;
	return pStr;
}

int SRowId::fromstr(void * pData, long, const char * pStr) const
{
	DBRowId * p_row_id = static_cast<DBRowId *>(pData);
	CALLPTRMEMB(p_row_id, FromStr__(pStr));
	return 1;
}

int  SRowId::base() const { return BTS_STRING; }
void SRowId::tobase(const void * s, void * b) const { tostr(s, 0L, static_cast<char *>(b)); }
int  SRowId::baseto(void * s, const void * b) const  { fromstr(s, 0L, static_cast<const char *>(b)); return 1; }

void SRowId::minval(void * pData) const
{
	DBRowId * p_row_id = static_cast<DBRowId *>(pData);
	CALLPTRMEMB(p_row_id, Z());
}

void SRowId::maxval(void * pData) const
{
	DBRowId * p_row_id = static_cast<DBRowId *>(pData);
	CALLPTRMEMB(p_row_id, SetMaxVal());
}

class SLobType : public DataType {
public:
	explicit SLobType(size_t sz = 32) : DataType(sz)
	{
	}
	int    Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

int SLobType::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	return static_cast<SLob *>(pData)->Serialize(dir, S, pInd, rBuf);
}
//
//
//
DbSession::DbSession() : LastThread(), Id__(1), TlsIdx(-1L), _Oe(0)
{
	memzero(__dbdata__, sizeof(__dbdata__));
	SetConfig(0); // Устанавливаем конфигурацию по умолчанию
	InitProtectData();
	TlsIdx = TlsAlloc();
	InitThread();
	RegisterSType(S_ROWID, &SRowId());
	RegisterSType(S_CLOB,  &SLobType());
	RegisterSType(S_BLOB,  &SLobType());
}

DbSession::~DbSession()
{
	Id__ = -0x01abcdef;
	ReleaseThread();
	TlsFree(TlsIdx);
}

bool DbSession::IsConsistent() const { return (Id__ == 1); }

void DbSession::SetConfig(const Config * pCfg)
{
#define Default_Flags               fDetectExistByOpen
#define Default_NWaitLockTries      20
#define Default_NWaitLockTryTimeout 10
	ENTER_CRITICAL_SECTION
	if(pCfg) {
		SETFLAGBYSAMPLE(Cfg.Flags, fDetectExistByOpen, pCfg->Flags);
		if(pCfg->NWaitLockTries >= 0)
			Cfg.NWaitLockTries = pCfg->NWaitLockTries;
		else if(pCfg->NWaitLockTries == BTR_RECLOCKDISABLE)
			Cfg.NWaitLockTries = BTR_RECLOCKDISABLE;
		else
			Cfg.NWaitLockTries = Default_NWaitLockTries;
		if(pCfg->NWaitLockTryTimeout > 0)
			Cfg.NWaitLockTryTimeout = pCfg->NWaitLockTryTimeout;
		else
			Cfg.NWaitLockTryTimeout = Default_NWaitLockTryTimeout;
	}
	else {
		Cfg.Flags = Default_Flags;
		Cfg.NWaitLockTries = Default_NWaitLockTries;
		Cfg.NWaitLockTryTimeout = Default_NWaitLockTryTimeout;
	}
	LEAVE_CRITICAL_SECTION
}

#if 0 // {
void DbSession::SetFlag(long f, int set)
{
	ENTER_CRITICAL_SECTION
	assert(f == fDetectExistByOpen);
	long   _flags = Flags;
	SETFLAG(_flags, f, set);
	if(_flags != Flags) {
		Flags = _flags;
	}
	LEAVE_CRITICAL_SECTION
}

long DbSession::GetFlag(long f) const
{
	return BIN(Flags & f);
}
#endif // } 0

int DbSession::GetTaState()
{
	const DbThreadLocalArea & r_tla = GetConstTLA();
	return BIN(r_tla.GetState() & DbThreadLocalArea::stTransaction);
}

int DbSession::InitThread()
{
	DbThreadLocalArea * p_tla = new DbThreadLocalArea;
	TlsSetValue(TlsIdx, p_tla);
	p_tla->Id = LastThread.Incr();
	memzero(p_tla->ClientID, 16);
	*(reinterpret_cast<uint16 *>(&p_tla->ClientID) + 6) = static_cast<uint16>(0x5050);
	*(reinterpret_cast<uint16 *>(&p_tla->ClientID) + 7) = static_cast<uint16>(p_tla->Id);
	return 1;
}

void DbSession::ReleaseThread()
{
	DbThreadLocalArea * p_tla = static_cast<DbThreadLocalArea *>(TlsGetValue(TlsIdx));
	if(p_tla) {
		delete p_tla;
		TlsSetValue(TlsIdx, 0);
	}
}
//
// См. примечание к определению функций DB.H
//
DbThreadLocalArea & DbSession::GetTLA() { return *static_cast<DbThreadLocalArea *>(SGetTls(TlsIdx)); }
const DbThreadLocalArea & DbSession::GetConstTLA() const { return *static_cast<DbThreadLocalArea *>(SGetTls(TlsIdx)); }

int FASTCALL DbSession::SetError(int errCode)
{
	BtrError = errCode;
	return 0;
}

int FASTCALL DbSession::SetError(int errCode, const char * pAddedMsg)
{
	BtrError = errCode;
	SetAddedMsgString(pAddedMsg);
	return 0;
}

long DbSession::GetDbPathID() const
{
	const DbThreadLocalArea & r_tla = GetConstTLA();
	return r_tla.P_CurDict ? r_tla.P_CurDict->GetDbPathID() : 0;
}

int DbSession::GetDbPathID(const char * pPath, long * pID)
{
	SString unc_path;
	pathToUNC(pPath, unc_path);
	long   id = 0;
	uint   pos = 0;
	ENTER_CRITICAL_SECTION
	if(DbPathList.lsearch(unc_path.cptr(), &pos, PTR_CMPFUNC(PcharNoCase))) {
		id = static_cast<long>(pos + 1);
	}
	else {
		DbPathList.insert(newStr(unc_path));
		id = DbPathList.getCount();
	}
	ASSIGN_PTR(pID, id);
	LEAVE_CRITICAL_SECTION
	return 1;
}

int DbSession::GetDbPath(long dbPathID, SString & rBuf) const
{
	int    ok = 0;
	uint   pos = (dbPathID > 0) ? (dbPathID-1) : UNDEFPOS;
	if(pos < DbPathList.getCount()) {
		rBuf = DbPathList.at(pos);
		ok = 1;
	}
	else
		rBuf.Z();
	return ok;
}

int DbSession::OpenDictionary2(DbProvider * pDb)
{
	DbThreadLocalArea & r_tla = GetTLA();
	r_tla.P_CurDict = pDb;
	if(r_tla.P_CurDict && !r_tla.P_CurDict->IsValid()) {
		ZDELETE(r_tla.P_CurDict);
	}
	return (r_tla.P_CurDict != 0);
}

void DbSession::CloseDictionary()
{
	DbThreadLocalArea & r_tla = GetTLA();
	ZDELETE(r_tla.P_CurDict);
}

void FASTCALL DbSession::SetAddedMsgString(const char * pStr) { GetTLA().AddedMsgString = pStr; }
const SString & DbSession::GetAddedMsgString() const { return GetConstTLA().AddedMsgString; }

void DbSession::GetProtectData(void * pBuf, int decr) const
{
	memcpy(pBuf, __dbdata__, sizeof(__dbdata__));
	if(decr)
		::decrypt(pBuf, sizeof(__dbdata__));
}

void DbSession::SetProtectData(const void * pBuf)
{
	memcpy(__dbdata__, pBuf, sizeof(__dbdata__));
}

void DbSession::InitProtectData()
{
	uint16 * p = __dbdata__;
	p[ 0] = 0x290a, p[ 1] = 0x874a, p[ 2] = 0xc97a,
	p[ 3] = 0x7729, p[ 4] = 0x60cd, p[ 5] = 0xdbb6,
	p[ 6] = 0x69c7, p[ 7] = 0x4fbe, p[ 8] = 0xf00f,
	p[ 9] = 0xbbbd, p[10] = 0xedcd, p[11] = 0xc94e,
	p[12] = 0xedc6, p[13] = 0xcf4e, p[14] = 0xedc6,
	p[15] = 0xd15e, p[16] = 0xf376, p[17] = 0xcf45,
	p[18] = 0xedb6, p[19] = 0xe16e, p[20] = 0x840e,
	p[21] = 0xa8bc, p[22] = 0x81af, p[23] = 0x45ee,
	p[24] = 0xed6f, p[25] = 0x705d, p[26] = 0xfad6,
	p[27] = 0xe8c5, p[28] = 0xedcf, p[29] = 0x836e,
	p[30] = 0x791e, p[31] = 0xa8be;
}

int btrokornfound()
{
	const int err = BtrError;
	return BIN(oneof3(err, 0, BE_EOF, BE_KEYNFOUND));
}

int btrnfound__()
{
	const int err = BtrError;
	return BIN(oneof2(err, BE_EOF, BE_KEYNFOUND));
}

#pragma warning(disable:4073)
#pragma init_seg(lib)
