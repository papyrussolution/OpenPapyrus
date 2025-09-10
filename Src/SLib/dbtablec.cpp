// DBTABLEC.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2025
// @codepage UTF-8
// Классы и функции DBTable, не зависящие от провайдера DBMS
//
#include <slib-internal.h>
#pragma hdrstop
//
//
//
DBTable * FASTCALL _GetTable(int handle) { return DBS.GetTLA().GetTableEntry(handle); }
//
//
// static members of DBTable {
const char * DBTable::CrTempFileNamePtr = reinterpret_cast<const char *>(0x0003);

void   FASTCALL DBTable::InitErrFileName(const char * pFileName) { DBS.GetTLA().InitErrFileName(pFileName); }
const  char * DBTable::GetLastErrorFileName() { return DBS.GetConstTLA().GetLastErrFileName(); }
void   DBTable::InitErrFileName() { DBS.GetTLA().InitErrFileName(OpenedFileName); }
// } static members of DBTable
//
//
static const S_GUID DBRowId_Ind_I32(GUID{0xE70D094F, 0xA0DD, 0x4391, { 0x84, 0x30, 0xCE, 0x8E, 0x69, 0xD0, 0x4F, 0x54}}); //{E70D094F-A0DD-4391-8430-CE8E69D04F54}
static const S_GUID DBRowId_Ind_I64(GUID{0x1B9D684E, 0x3884, 0x483D, { 0x81, 0x0E, 0x01, 0x59, 0x50, 0xCC, 0x26, 0x31}}); //{1B9D684E-3884-483D-810E-015950CC2631}

DBRowId::DBRowId()
{
	memzero(S, sizeof(S));
}

void DBRowId::SetI32(uint32 id)
{
	static_assert(sizeof(S) >= (sizeof(DBRowId_Ind_I32) + sizeof(id)));
	memcpy(S+0, &id, sizeof(id));
	memzero(S+sizeof(id), sizeof(S)-sizeof(DBRowId_Ind_I32));
	memcpy(S+sizeof(S)-sizeof(DBRowId_Ind_I32), &DBRowId_Ind_I32, sizeof(DBRowId_Ind_I32));
}

void DBRowId::SetI64(int64 id)
{
	static_assert(sizeof(S) >= (sizeof(DBRowId_Ind_I64) + sizeof(id)));
	memcpy(S+0, &id, sizeof(id));
	memzero(S+sizeof(id), sizeof(S)-sizeof(DBRowId_Ind_I64));
	memcpy(S+sizeof(S)-sizeof(DBRowId_Ind_I64), &DBRowId_Ind_I64, sizeof(DBRowId_Ind_I64));
}

bool DBRowId::IsI32() const
{
	return (memcmp(S+sizeof(S)-sizeof(DBRowId_Ind_I32), &DBRowId_Ind_I32, sizeof(DBRowId_Ind_I32)) == 0);
}

bool DBRowId::IsI64() const
{
	return (memcmp(S+sizeof(S)-sizeof(DBRowId_Ind_I64), &DBRowId_Ind_I64, sizeof(DBRowId_Ind_I64)) == 0);
}

uint32 DBRowId::GetI32() const
{
	return IsI32() ? *reinterpret_cast<const uint32 *>(S) : 0U;
}

int64  DBRowId::GetI64() const
{
	return IsI64() ? *reinterpret_cast<const int64 *>(S) : 0ULL;
}

bool DBRowId::IsLarge() const
{
	return !(IsI32() || IsI64());
}

bool DBRowId::SetLarge(const void * pIdent, size_t len)
{
	bool    ok = true;
	if(pIdent && len <= sizeof(S)) {
		memcpy(S, pIdent, len);
		if(len < sizeof(S)) {
			memzero(S+len, sizeof(S)-len);
		}
	}
	else
		ok = false;
	return ok;
}

size_t DBRowId::GetLarge(void * pIdent, size_t bufLen) const
{
	size_t actual_size = 0;
	if(IsLarge()) {
		actual_size = sizeof(S);
		if(pIdent) {
			if(bufLen >= actual_size)
				memcpy(pIdent, S, actual_size);
			else
				actual_size = 0;
		}
	}
	return actual_size;
}

DBRowId::operator RECORDNUMBER() const { return GetI32(); }

DBRowId & FASTCALL DBRowId::operator = (RECORDNUMBER n)
{
	SetI32(n);
	return *this;
}

DBRowId & DBRowId::Z()
{
	memzero(S, sizeof(S));
	return *this;
}

void DBRowId::SetMaxVal()
{
	memset(S, 0xff, sizeof(S)-1);
	S[sizeof(S)-1] = 0;
}

SString & FASTCALL DBRowId::ToStr__(SString & rBuf) const
{
	rBuf.Z();
	if(IsI32())
		rBuf.Cat(GetI32());
	else if(IsI64())
		rBuf.Cat(GetI64());
	else {
		assert(IsLarge());
		rBuf.CatHex(S, sizeof(S));
	}
	return rBuf;
}

int FASTCALL DBRowId::FromStr__(const char * pStr)
{
	int    ok = -1;
	const  size_t len = sstrlen(pStr);
	Z();
	if(len) {
		uint64 int_value = 0ULL;
		bool   is_there_non_dec = false;
		bool   is_there_non_hex = false;
		bool   is_there_non_printable = false;
		for(uint i = 0; i < len; i++) {
			const char c = pStr[i];
			if(!isdec(c)) {
				is_there_non_dec = true;
				if(!ishex(c)) {
					is_there_non_hex = true;
					if(!isasciiprint(c))
						is_there_non_printable = true;
				}
			}
		}
		if(is_there_non_dec) {
			if(is_there_non_hex) {
				ok = SetLarge(pStr, len);
			}
			else {
				SBinaryChunk bc;
				if(bc.FromHex(pStr)) {
					ok = SetLarge(bc.PtrC(), bc.Len());
				}
			}
		}
		else {
			int_value = satou64(pStr);
			if(int_value > MAXUINT) {
				SetI64(int_value);
			}
			else {
				SetI32(static_cast<uint32>(int_value));
			}
		}
		if(ok < 0) {
			/*
			//
			// В строке все символы цифровые (см. выше): трактуем ИД как беззнаковое целое
			//
			strtouint(pStr, &B);
			PTR32(S)[1] = 0;
			ok = 1;
			*/
		}
	}
	return ok;
}
//
//
//
// extern const uint32 SLobSignature[4];
// @v12.2.3 replaced with SlConst::SLobSignature static const uint32 SLobSignature[4] = { 0x2efc, 0xd421, 0x426c, 0xee07 }; 

bool   SLob::IsStructured() const 
{ 
	// @v12.2.3 test (will be removed after @v14.0.0) {
	static const uint32 __SLobSignature[4] = { 0x2efc, 0xd421, 0x426c, 0xee07 }; 
	assert(SMem::Cmp(SlConst::SLobSignature, __SLobSignature, sizeof(SlConst::SLobSignature)) == 0);
	// } @v12.2.3 
	return (memcmp(Buf.H.Signature, SlConst::SLobSignature, sizeof(SlConst::SLobSignature)) == 0); 
}

bool   SLob::IsPtr() const 
{ 
	return (IsStructured() && Buf.H.Flags & hfPtr); 
}

void * SLob::GetRawDataPtr() 
{ 
	// @v12.2.3 @debug return IsStructured() ? ((Buf.H.Flags & hfPtr) ? Buf.H.H : 0) : Buf.B; 
	// @v12.2.3 @debug {
	if(IsStructured()) {
		return (Buf.H.Flags & hfPtr) ? Buf.H.H : 0;
	}
	else {
		return Buf.B;
	}
	// } @v12.2.3 @debug 
}

size_t SLob::GetPtrSize() const { return (IsStructured() && Buf.H.Flags & hfPtr) ? Buf.H.PtrSize : 0; }

int SLob::SetStructured()
{
	if(!IsStructured()) {
		THISZERO();
		memcpy(Buf.H.Signature, SlConst::SLobSignature, sizeof(SlConst::SLobSignature));
		return 1;
	}
	else
		return -1;
}

void FASTCALL SLob::Init(void * pDescriptor)
{
	DestroyPtr();
	THISZERO();
	if(pDescriptor) {
		SetStructured();
		Buf.H.H = pDescriptor;
	}
}

void SLob::Empty()
{
	DestroyPtr();
	THISZERO();
	SetStructured();
}

int SLob::EnsureUnstructured()
{
	int    ok = -1;
	if(IsStructured()) {
		memzero(Buf.H.Signature, sizeof(Buf.H.Signature));
		ok = 1;
	}
	return ok;
}

int FASTCALL SLob::InitPtr(uint32 sz)
{
	int    ok = -1;
	DestroyPtr();
	if(sz) {
		SetStructured();
		Buf.H.H = SAlloc::M(sz);
		if(!Buf.H.H) {
			Buf.H.PtrSize = 0;
			ok = 0;
		}
		else {
			Buf.H.PtrSize = sz;
			Buf.H.Flags |= hfPtr;
			ok = 1;
		}
	}
	return ok;
}

int SLob::DestroyPtr()
{
	int    ok = -1;
	if(IsStructured()) {
		if(Buf.H.Flags & hfPtr) {
			SAlloc::F(Buf.H.H);
			Buf.H.H = 0;
			Buf.H.PtrSize = 0;
			Buf.H.Flags &= ~hfPtr;
			ok = 1;
		}
		THISZERO();
	}
	return ok;
}

int SLob::Serialize(int dir, size_t flatSize, uint8 * pInd, SBuffer & rBuf)
{
	int    ok = 1;
	if(dir > 0) {
		if(IsPtr()) {
			void * p = GetRawDataPtr();
			ulong  sz = Buf.H.PtrSize;
			*pInd = 2;
			rBuf.Write(sz);
			rBuf.Write(p, sz);
		}
		else if(ismemzero(Buf.B, flatSize))
			*pInd = 1;
		else {
			rBuf.Write(Buf.B, flatSize);
			*pInd = 0;
		}
	}
	else if(dir < 0) {
		if(*pInd == 1) {
			DestroyPtr();
			memzero(Buf.B, flatSize);
		}
		else if(*pInd == 2) {
			ulong  sz = 0;
			rBuf.Read(sz);
			InitPtr(sz);
			rBuf.Read(GetRawDataPtr(), sz);
		}
		else {
			DestroyPtr();
			rBuf.Read(Buf.B, flatSize);
		}
	}
	return ok;
}
//
//
//
DBLobBlock::DBLobBlock() : SVector(sizeof(DBLobItem))
{
}

int FASTCALL DBLobBlock::SearchPos(uint fldIdx, uint * pPos) const
{
	int    ok = -1;
	uint   pos = 0;
	const  uint c = getCount();
	if(c) {
		for(uint i = 0; i < c; i++) {
			if(static_cast<DBLobItem *>(at(i))->FldN == fldIdx) {
				pos = i;
				ok = 1;
				break;
			}
		}
		if(ok <= 0)
			ok = 0;
	}
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int DBLobBlock::SetSize(uint fldIdx, size_t sz)
{
	uint   pos = 0;
	int    ok = SearchPos(fldIdx, &pos);
	if(ok > 0)
		static_cast<DBLobItem *>(at(pos))->Size = sz;
	return ok;
}

int DBLobBlock::GetSize(uint fldIdx, size_t * pSz) const
{
	size_t sz = 0;
	uint   pos = 0;
	int    ok = SearchPos(fldIdx, &pos);
	if(ok > 0)
		sz = static_cast<DBLobItem *>(at(pos))->Size;
	ASSIGN_PTR(pSz, sz);
	return ok;
}

int DBLobBlock::SetLocator(uint fldIdx, uint32 loc)
{
	uint   pos = 0;
	int    ok = SearchPos(fldIdx, &pos);
	if(ok > 0)
		static_cast<DBLobItem *>(at(pos))->Loc = loc;
	return ok;
}

int DBLobBlock::GetLocator(uint fldIdx, uint32 * pLoc) const
{
	size_t loc = 0;
	uint   pos = 0;
	int    ok = SearchPos(fldIdx, &pos);
	if(ok > 0)
		loc = static_cast<DBLobItem *>(at(pos))->Loc;
	ASSIGN_PTR(pLoc, loc);
	return ok;
}
//
//
//
DBTable::SelectStmt::SelectStmt(DbProvider * pDb, const Generator_SQL & rSql, int idx, int sp, int sf) : SSqlStmt(pDb, rSql), Idx(idx), Sp(sp), Sf(sf)
{
}

void DBTable::SetStmt(SelectStmt * pStmt)
{
	if(pStmt) {
		if(pStmt != P_Stmt) {
			DELETEANDASSIGN(P_Stmt, pStmt);
		}
	}
	else {
		ZDELETE(P_Stmt);
	}
}

void DBTable::ToggleStmt(bool release)
{
	if(release)
		ZDELETE(P_OppStmt);
	else {
		SelectStmt * p_temp = P_OppStmt;
		P_OppStmt = P_Stmt;
		P_Stmt = p_temp;
	}
}

int (*DBTable::OpenExceptionProc)(const char * pFileName, int btrErr) = 0; // @global

int DBTable::Init(DbProvider * pDbP)
{
	index  = 0;
	P_DBuf = 0;
	bufLen = 0;
	State  = 0;
	FixRecSize = 0;
	P_Db = NZOR(pDbP, CurDict);
	P_Stmt = 0;
	P_OppStmt = 0;
	handle  = 0;
	flags   = 0;
	tableID = 0;
	ownrLvl = 0;
	tableName[0] = 0;
	fileName.Z();
	indexes.setTableRef(offsetof(DBTable, indexes));
	PageSize = 0;
	LastLockedRow.Z();
	return 1;
}

DBTable::DBTable()
{
	Init(0);
}

DBTable::DBTable(const char * pTblName, const char * pFileName, int openMode, DbProvider * pDbP)
{
	Init(pDbP);
	open(pTblName, pFileName, openMode);
}
//
// Protected constructor
//
DBTable::DBTable(const char * pTblName, const char * pFileName, void * pFlds, void * pData, int om, DbProvider * pDbP)
{
	class _DBField {
	public:
		int    hTbl;
		int    hFld;
	};
	Init(pDbP);
	RECORDSIZE s = 0;
	if(open(pTblName, pFileName, om)) {
		if(pFlds) {
			for(int16 i = fields.getCount() - 1; i >= 0; i--) {
				static_cast<_DBField *>(pFlds)[i].hTbl = handle;
				static_cast<_DBField *>(pFlds)[i].hFld = i;
				s += (RECORDSIZE)stsize(fields[i].T);
			}
		}
		if(pData)
			setDataBuf(pData, NZOR(s, fields.CalculateRecSize()));
	}
}

DBTable::~DBTable()
{
	close();
	if(State & sOwnDataBuf)
		ZFREE(P_DBuf);
	ZDELETE(P_Stmt);
	ZDELETE(P_OppStmt);
}

int DBTable::Debug_Output(SString & rBuf) const
{
	int    ok = 1;
	uint   i;
	rBuf.Z();
	CAT_FLD(handle, rBuf).CR();
	CAT_FLD(flags, rBuf).CR();
	CAT_FLD(tableID, rBuf).CR();
	CAT_FLD(ownrLvl, rBuf).CR();
	CAT_FLD(tableName, rBuf).CR();
	CAT_FLD(fileName, rBuf).CR();
	CAT_FLD(fields.getCount(), rBuf).CR();
	for(i = 0; i < fields.getCount(); i++) {
		CAT_FLD(fields.getField(i).Id, rBuf.Tab()).CR();
		CAT_FLD(fields.getField(i).Name, rBuf.Tab()).CR();
		CAT_FLD(fields.getField(i).Offs, rBuf.Tab()).CR();
		CAT_FLD_HEX(fields.getField(i).T, rBuf.Tab()).CR();
	}
	CAT_FLD(indexes.getNumKeys(), rBuf).CR();
	for(i = 0; i < indexes.getNumKeys(); i++) {
		CAT_FLD(indexes[i].getKeyNumber(), rBuf.Tab()).CR();
		CAT_FLD_HEX((long)indexes[i].getFlags(), rBuf.Tab()).CR();
		CAT_FLD(indexes[i].getNumSeg(), rBuf.Tab()).CR();
		for(int j = 0; j < indexes[i].getNumSeg(); j++) {
			CAT_FLD(indexes[i].getFieldID(j), rBuf.Tab_(2)).CR();
			CAT_FLD_HEX((long)indexes[i].getFlags(j), rBuf.Tab_(2)).CR();
		}
	}
	return ok;
}

int DBTable::open(const char * pTblName, const char * pFileName, int openMode)
{
	SString temp_file_name;
	if(handle == 0) {
		char   p[64];
		if(pFileName == DBTable::CrTempFileNamePtr) {
			assert(pTblName != 0);
			pFileName = 0;
			THROW(P_Db && P_Db->CreateTempFile(pTblName, temp_file_name, 0));
			pFileName = temp_file_name;
			flags |= XTF_TEMP;
		}
		if(pTblName) {
			if(!tableID) {
				THROW(P_Db && P_Db->LoadTableSpec(this, pTblName, pFileName, 1));
			}
		}
		else {
			assert(pFileName != 0);
			fileName = pFileName;
		}
		DBS.GetProtectData(p, 1);
		if(P_Db) {
			THROW(P_Db->Implement_Open(this, fileName, openMode, p));
		}
		else {
			THROW(Btr_Open(fileName, openMode, p));
		}
		State |= sOpened_;
		memzero(p, sizeof(p));
		THROW(handle = DBS.GetTLA().AddTableEntry(this));
	}
	CATCH
		handle = 0;
		if(OpenExceptionProc) {
			fileName.SetIfEmpty(NZOR(pFileName, pTblName));
			OpenExceptionProc(fileName, BtrError);
		}
	ENDCATCH
	return handle;
}

int DBTable::close()
{
	if(State & sOpened_) {
		if(P_Db)
			P_Db->Implement_Close(this);
		else {
			Btr_Close();
		}
		State &= ~sOpened_;
	}
	if(handle) {
		tableName[0] = 0;
		fileName.Z();
		fields.Z();
		indexes.Z();
		DBS.GetTLA().FreeTableEntry(handle);
		handle = 0;
	}
	return 1;
}

int    DBTable::IsOpened() const { return (handle != 0); }
void   DBTable::clearDataBuf() { memzero(P_DBuf, bufLen); }
RECORDSIZE DBTable::getBufLen() const { return bufLen; }
DBRowId * DBTable::getCurRowIdPtr() { return &CurRowId; }
uint   DBTable::GetLobCount() const { return LobB.getCount(); }
DBLobBlock * DBTable::getLobBlock() { return &LobB; }
int    DBTable::setLobSize(DBField fld, size_t sz) { return LobB.SetSize((uint)fld.fld, sz); }
int    DBTable::getLobSize(DBField fld, size_t * pSz) const { return LobB.GetSize((uint)fld.fld, pSz); }
RECORDSIZE FASTCALL DBTable::getRecSize() const { return FixRecSize; }
DBTable::SelectStmt * DBTable::GetStmt() { return P_Stmt; }

bool FASTCALL DBTable::getField(uint fldN, DBField * pFld) const
{
	if(fldN < fields.getCount()) {
		DBField fld;
		fld.Id = handle;
		fld.fld = fldN;
		ASSIGN_PTR(pFld, fld);
		return true;
	}
	else
		return false;
}

int DBTable::getFieldByName(const char * pName, DBField * pFld) const
{
	uint   pos = 0;
	const  BNField * f = &fields.getField(pName, &pos);
	if(f) {
		DBField fld;
		fld.Id = handle;
		fld.fld = pos;
		ASSIGN_PTR(pFld, fld);
		return 1;
	}
	else
		return 0;
}

int DBTable::getFieldValue(uint fldN, void * pBuf, size_t * pSize) const
	{ return (P_DBuf && fldN < fields.getCount()) ? fields[fldN].getValue(P_DBuf, pBuf, pSize) : 0; }
int DBTable::setFieldValue(uint fldN, const void * pBuf)
	{ return (P_DBuf && fldN < fields.getCount()) ? fields[fldN].setValue(P_DBuf, pBuf) : 0; }

int DBTable::getFieldValByName(const char * pName, void * pVal, size_t * pSize) const
{
	const  BNField * f = &fields.getField(pName, 0);
	return (P_DBuf && f) ? f->getValue(P_DBuf, pVal, pSize) : 0;
}

int DBTable::setFieldValByName(const char * pName, const void * pVal)
{
	const  BNField * f = &fields.getField(pName, 0);
	return (f && P_DBuf) ? f->setValue(P_DBuf, pVal) : 0;
}

int DBTable::putRecToString(SString & rBuf, int withFieldNames)
{
	rBuf.Z();
	for(uint i = 0; i < fields.getCount(); i++) {
		char   temp_buf[1024];
		const BNField & f = fields[i];
		f.putValueToString(P_DBuf, temp_buf);
		if(withFieldNames)
			rBuf.CatEq(f.Name, temp_buf);
		else
			rBuf.Cat(temp_buf);
		rBuf.Semicol().Space();
	}
	return 1;
}

int DBTable::AllocateOwnBuffer(int size)
{
	int    ok = 1;
	const  RECORDSIZE rec_size = (size < 0) ? fields.CalculateRecSize() : static_cast<RECORDSIZE>(size);
	if(State & sOwnDataBuf) {
		ZFREE(P_DBuf);
	}
	P_DBuf = static_cast<char *>(SAlloc::C(rec_size+1, 1));
	if(P_DBuf)
		bufLen = rec_size;
	else {
		bufLen = 0;
		ok = 0;
	}
	return ok;
}

void FASTCALL DBTable::setDataBuf(void * pBuf, RECORDSIZE aBufLen)
{
	if(State & sOwnDataBuf)
		ZFREE(P_DBuf);
	P_DBuf = pBuf;
	bufLen = aBufLen;
}

void FASTCALL DBTable::setBuffer(SBaseBuffer & rBuf)
{
	if(State & sOwnDataBuf)
		ZFREE(P_DBuf);
	P_DBuf = rBuf.P_Buf;
	bufLen = (RECORDSIZE)rBuf.Size;
}

const SBaseBuffer DBTable::getBuffer() const
{
	SBaseBuffer ret_buf;
	ret_buf.P_Buf = (char *)P_DBuf; // @trick
	ret_buf.Size = bufLen;
	return ret_buf;
}

void FASTCALL DBTable::copyBufFrom(const void * pBuf)
{
	if(pBuf && P_DBuf) {
		memcpy(P_DBuf, pBuf, bufLen);
	}
}

void FASTCALL DBTable::copyBufFrom(const void * pBuf, size_t srcBufSize)
{
	if(pBuf && P_DBuf) {
		const size_t s = (srcBufSize && srcBufSize < bufLen) ? srcBufSize : bufLen;
		memcpy(P_DBuf, pBuf, s);
	}
}

void FASTCALL DBTable::copyBufTo(void * pBuf) const
{
	if(pBuf && P_DBuf)
		memcpy(pBuf, P_DBuf, bufLen);
}

int DBTable::copyBufToKey(int idx, void * pKey) const
{
	int    ok = 1;
	if(!pKey)
		ok = -1;
	else if(idx >= 0 && idx < (int)indexes.getNumKeys()) {
		const BNKey k = indexes.getKey(idx);
		const int ns = k.getNumSeg();
		size_t offs = 0;
		for(int i = 0; i < ns; i++) {
			size_t sz;
			indexes.field(idx, i).getValue(P_DBuf, PTR8(pKey)+offs, &sz);
			offs += sz;
		}
	}
	else
		ok = 0; // @todo(errdef)
	return ok;
}

int FASTCALL DBTable::HasNote(DBField * pLastFld) const
{
	int    ok = -1;
	if(State & sHasNote) {
		ok = 1;
		if(pLastFld) {
			const bool last_note_field_found = getField(fields.getCount()-1, pLastFld);
			if(!last_note_field_found) {
				assert(last_note_field_found);
				ok = 0;
			}
			else {
				int    t_ = GETSTYPE(pLastFld->getField().T);
				if(t_ != S_NOTE) {
					// @v10.5.9 assert(t_ == S_NOTE);
					ok = 0;
				}
			}
		}
	}
	return ok;
}

int FASTCALL DBTable::HasLob(DBField * pLastFld) const
{
	int    ok = -1;
	if(State & sHasLob) {
		ok = 1;
		if(pLastFld) {
			const bool last_lob_field_found = getField(fields.getCount()-1, pLastFld);
			if(!last_lob_field_found) {
				assert(last_lob_field_found);
				ok = 0;
			}
			else {
				int    t_ = GETSTYPE(pLastFld->getField().T);
				if(!oneof2(t_, S_BLOB, S_CLOB)) {
					assert(oneof2(t_, S_BLOB, S_CLOB));
					ok = 0;
				}
			}
		}
	}
	return ok;
}

int DBTable::InitLob()
{
	int    ok = 0;
	State &= ~(sHasLob | sHasNote);
	LobB.clear();
	for(uint i = 0; i < fields.getCount(); i++) {
		int _t = GETSTYPE(fields[i].T);
		if(oneof2(_t, S_BLOB, S_CLOB)) {
			DBLobItem item;
			item.FldN = i;
			item.Size = 0;
			item.Loc = 0;
			LobB.insert(&item);
			State |= sHasLob;
			ok = 1;
		}
		else if(_t == S_NOTE) {
			State |= sHasNote;
		}
		else if(_t == S_AUTOINC)
			State |= sHasAutoinc;
	}
	return ok;
	//SETFLAG(State, sHasLob, (LobB.Init(fields) > 0));
	//return BIN(State & sHasLob);
}

int DBTable::GetLobField(uint n, DBField * pFld) const
{
	int    ok = 0;
	if(pFld) {
		pFld->Id = 0;
		pFld->fld = 0;
	}
	if(n < LobB.getCount()) {
		uint fld_n = ((DBLobItem *)LobB.at(n))->FldN;
		if(fld_n < fields.getCount()) {
			if(pFld) {
				pFld->Id = handle;
				pFld->fld = (int)fld_n;
			}
			ok = 1;
		}
	}
	return ok;
}

void FASTCALL DBTable::destroyLobData(DBField fld)
{
	SLob * p_fld_data = static_cast<SLob *>(fld.getValuePtr());
	p_fld_data->DestroyPtr();
}

int DBTable::readLobData(DBField fld, SBuffer & rBuf) const
{
	int    ok = -1;
	size_t sz;
	if(LobB.GetSize((uint)fld.fld, &sz) > 0) {
		if(sz) {
			SLob * p_fld_data = static_cast<SLob *>(fld.getValuePtr());
			const void * ptr = p_fld_data->GetRawDataPtr();
			if(ptr) {
				rBuf.Write(ptr, sz);
				ok = 1;
			}
		}
	}
	return ok;
}

int DBTable::writeLobData(DBField fld, const void * pBuf, size_t dataSize, int forceCanonical /*=0*/)
{
	int    ok = -1, r;
	size_t sz;
	//
	// Проверка на то, что поле fld действительно является LOB'ом
	//
	THROW(r = LobB.GetSize((uint)fld.fld, &sz));
	if(r > 0) {
		size_t flat_size = fld.getField().size();
		SLob * p_fld_data = static_cast<SLob *>(fld.getValuePtr());
		void * ptr = p_fld_data->GetRawDataPtr();
		STempBuffer temp_buf(0);
		if(ptr == pBuf && (dataSize > flat_size || forceCanonical)) {
			//
			// Если исходный указатель равен указателю на данные LOB, то необходимо
			// предпринять меры по сохранению данных перед тем, как функция InitPtr
			// преобразует SLob в структуру с отвлетвлением.
			//
			THROW(temp_buf.Alloc(dataSize));
			memcpy(temp_buf.vptr(), pBuf, dataSize);
			pBuf = temp_buf.vcptr();
		}
		THROW(p_fld_data->InitPtr((dataSize > flat_size || forceCanonical) ? dataSize : 0));
		THROW(ptr = p_fld_data->GetRawDataPtr());
		if(ptr != pBuf)
			memcpy(ptr, pBuf, dataSize);
		THROW(setLobSize(fld, dataSize));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int DBTable::StoreAndTrimLob()
{
	int    ok = -1;
	if(State & sHasLob) {
		LobB.Storage.Z();
		for(uint i = 0; i < fields.getCount(); i++) {
			const BNField & r_fld = fields[i];
			if(oneof2(GETSTYPE(r_fld.T), S_BLOB, S_CLOB)) {
				uint   lob_pos = 0;
				THROW_DS(LobB.SearchPos(i, &lob_pos));
				DBLobItem * p_lob_item = static_cast<DBLobItem *>(LobB.at(lob_pos));
				uint32 lob_sz = p_lob_item->Size;
				LobB.Storage.Write(lob_sz);
				if(lob_sz) {
					SLob * p_lob = reinterpret_cast<SLob *>(PTR8(P_DBuf)+r_fld.Offs);
					THROW_DS(p_lob->Serialize(+1, stsize(r_fld.T), &p_lob_item->StrgInd, LobB.Storage));
					p_lob->Empty();
					p_lob_item->Size = 0;
					ok = 2;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int DBTable::RestoreLob()
{
	int    ok = -1;
	if(State & sHasLob) {
		assert(LobB.Storage.GetAvailableSize() != 0);
		for(uint i = 0; i < fields.getCount(); i++) {
			const BNField & r_fld = fields[i];
			if(oneof2(GETSTYPE(r_fld.T), S_BLOB, S_CLOB)) {
				uint   lob_pos = 0;
				THROW_DS(LobB.SearchPos(i, &lob_pos));
				DBLobItem * p_lob_item = static_cast<DBLobItem *>(LobB.at(lob_pos));
				uint32 lob_sz = 0;
				THROW(LobB.Storage.Read(lob_sz));
				if(lob_sz) {
					SLob * p_lob = reinterpret_cast<SLob *>(PTR8(P_DBuf)+r_fld.Offs);
					THROW_DS(p_lob->Serialize(-1, stsize(r_fld.T), &p_lob_item->StrgInd, LobB.Storage));
				}
				p_lob_item->Size = lob_sz;
				ok = 1;
			}
		}
	}
	CATCHZOK
	LobB.Storage.Z();
	return ok;
}

int FASTCALL DBTable::insertRecBuf(const void * pData)
{
	OutOfTransactionLogging("insertRec");
	return P_Db ? P_Db->Implement_InsertRec(this, -1, 0, pData) : Btr_Implement_InsertRec(-1, 0, pData);
}

int DBTable::insertRecBuf(const void * pData, int idx, void * pKeyBuf)
{
	OutOfTransactionLogging("insertRec");
	return P_Db ? P_Db->Implement_InsertRec(this, idx, pKeyBuf, pData) : Btr_Implement_InsertRec(idx, pKeyBuf, pData);
}

int FASTCALL DBTable::insertRec()
{
	OutOfTransactionLogging("insertRec");
	if(P_Db)
		return P_Db->Implement_InsertRec(this, -1, 0, 0);
	else
		return Btr_Implement_InsertRec(-1, 0, 0);
}

int FASTCALL DBTable::insertRec(int idx, void * pKeyBuf)
{
	OutOfTransactionLogging("insertRec");
	return P_Db ? P_Db->Implement_InsertRec(this, idx, pKeyBuf, 0) : Btr_Implement_InsertRec(idx, pKeyBuf, 0);
}

void FASTCALL DBTable::OutOfTransactionLogging(const char * pOp) const
{
	if(!(flags & (XTF_TEMP|XTF_DICT|XTF_DISABLEOUTOFTAMSG)) && !(DBS.GetTLA().GetState() & DbThreadLocalArea::stTransaction)) {
		SString msg_buf;
		msg_buf.CatCurDateTime().Tab().Cat(pOp).Space().Cat("executed out of transaction").CatDiv(':', 2).
			Cat(tableName).CatChar('(').Cat(fileName).CatChar(')');
		SLS.LogMessage("dbwarn.log", msg_buf);
	}
}

int FASTCALL DBTable::updateRec()
{
	OutOfTransactionLogging("updateRec");
	return P_Db ? P_Db->Implement_UpdateRec(this, 0, 0) : Btr_Implement_UpdateRec(0, 0);
}

int FASTCALL DBTable::updateRecNCC()
{
	OutOfTransactionLogging("updateRec");
	return P_Db ? P_Db->Implement_UpdateRec(this, 0, 1) : Btr_Implement_UpdateRec(0, 1);
}

int FASTCALL DBTable::updateRecBuf(const void * pDataBuf)
{
	OutOfTransactionLogging("updateRec");
	return P_Db ? P_Db->Implement_UpdateRec(this, pDataBuf, 0) : Btr_Implement_UpdateRec(pDataBuf, 0);
}

int DBTable::deleteRec()
{
	OutOfTransactionLogging("deleteRec");
	return P_Db ? P_Db->Implement_DeleteRec(this) : Btr_Implement_DeleteRec();
}

int DBTable::deleteByQuery(int useTa, DBQ & rQ)
{
	int    ok = 1;
	if(P_Db) {
		ok = P_Db->Implement_DeleteFrom(this, useTa, rQ);
	}
	else {
		DBQuery * q = & selectAll().from(this, 0L).where(rQ);
		q->setDestroyTablesMode(0);
		if(!useTa || Btrieve::StartTransaction(1)) {
			for(int dir = spFirst; ok && q->single_fetch(0, 0, dir); dir = spNext) {
				uint8  key_buf[512];
				DBRowId _dbpos;
				if(!getPosition(&_dbpos))
					ok = 0;
				else if(!getDirectForUpdate(getCurIndex(), key_buf, _dbpos))
					ok = 0;
				else {
					if(deleteRec() == 0) // @sfu
						ok = 0;
				}
			}
			if(q->error)
				ok = 0;
			if(useTa)
				if(ok) {
					if(!Btrieve::CommitWork()) {
						Btrieve::RollbackWork();
						ok = 0;
					}
				}
				else
					Btrieve::RollbackWork();
		}
		else
			ok = 0;
		delete q;
	}
	return ok;
}

int FASTCALL DBTable::getPosition(DBRowId * pPos)
{
	return P_Db ? P_Db->Implement_GetPosition(this, pPos) : Btr_Implement_GetPosition(pPos);
}

int DBTable::getDirect(int idx, void * pKey, const DBRowId & rPos)
{
	BtrDbKey k;
	memcpy(k, &rPos, sizeof(rPos));
#define sf (sfDirect)
	int    ok = P_Db ? P_Db->Implement_Search(this, idx, k, 0, sf) : Btr_Implement_Search(idx, k, 0, sf);
#undef sf
	if(pKey && ok)
		memcpy(pKey, k, indexes.getKeySize((idx >= 0) ? idx : index));
	return ok;
}

int DBTable::rereadForUpdate(int idx, void * pKey)
{
	int    ok = 1;
	if(DBS.GetConfig().NWaitLockTries != BTR_RECLOCKDISABLE) { // @v8.6.5 нет необходимости перечитывать запись, если блокировки не применяются
		uint8  _key[512];
		if(!pKey) {
			MEMSZERO(_key);
			pKey = _key;
		}
		if(idx < 0)
			idx = 0;
		DBRowId _dbpos;
		THROW(getPosition(&_dbpos));
		THROW(getDirectForUpdate(idx, pKey, _dbpos));
	}
	CATCHZOK
	return ok;
}

int DBTable::getDirectForUpdate(int idx, void * pKey, const DBRowId & rPos)
{
	BtrDbKey k;
	memcpy(k, &rPos, sizeof(rPos));
#define sf (sfDirect|sfForUpdate)
	int    ok = P_Db ? P_Db->Implement_Search(this, idx, k, 0, sf) : Btr_Implement_Search(idx, k, 0, sf);
#undef sf
	if(pKey && ok)
		memcpy(pKey, k, indexes.getKeySize((idx >= 0) ? idx : index));
	return ok;
}

int FASTCALL DBTable::searchForUpdate(void * pKey, int srchMode)
	{ return P_Db ? P_Db->Implement_Search(this, index, pKey, srchMode, sfForUpdate) : Btr_Implement_Search(index, pKey, srchMode, sfForUpdate); }
int FASTCALL DBTable::searchForUpdate(int idx, void * pKey, int srchMode)
	{ return P_Db ? P_Db->Implement_Search(this, idx, pKey, srchMode, sfForUpdate) : Btr_Implement_Search(idx, pKey, srchMode, sfForUpdate); }
int FASTCALL DBTable::search(int idx, void * pKey, int srchMode)
	{ return P_Db ? P_Db->Implement_Search(this, idx, pKey, srchMode, 0) : Btr_Implement_Search(idx, pKey, srchMode, 0); }
int FASTCALL DBTable::search(void * pKey, int srchMode)
	{ return P_Db ? P_Db->Implement_Search(this, index, pKey, srchMode, 0) : Btr_Implement_Search(index, pKey, srchMode, 0); }
int DBTable::searchKey(int idx, void * pKey, int srchMode)
	{ return P_Db ? P_Db->Implement_Search(this, idx, pKey, srchMode, sfKeyOnly) : Btr_Implement_Search(idx, pKey, srchMode, sfKeyOnly); }
int DBTable::GetFileStat(long reqItems, DbTableStat * pStat)
	{ return P_Db ? P_Db->GetFileStat(this, reqItems, pStat) : Btr_GetStat(reqItems, pStat); }

int DBTable::SerializeSpec(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = -1;
	SString tbl_name;
	if(dir > 0)
		tbl_name = tableName;
	THROW(pCtx->Serialize(dir, tbl_name, rBuf));
	if(dir < 0)
		tbl_name.CopyTo(tableName, sizeof(tableName));
	THROW(pCtx->Serialize(dir, fileName, rBuf));
	THROW(pCtx->SerializeFieldList(dir, &fields, rBuf));
	THROW(indexes.Serialize(dir, rBuf, pCtx));
	CATCHZOK
	return ok;
}

int DBTable::SerializeRecord(int dir, void * pRec, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = -1;
	if(dir > 0) {
		ok = pCtx->Serialize(tableName, &fields, pRec, rBuf);
	}
	else if(dir < 0) {
		ok = pCtx->Unserialize(tableName, &fields, pRec, rBuf);
	}
	return ok;
}

int DBTable::Helper_SerializeArrayOfRecords(int dir, SVectorBase * pList, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	int32  c = SVectorBase::GetCount(pList); // @persistent
	STempBuffer temp_buf(0);
	if(dir > 0 && c > 0) {
		uint32 dbt_id = 0;
		THROW(pCtx->AddDbtDescr(tableName, &fields, &dbt_id));
	}
	THROW(pCtx->Serialize(dir, c, rBuf));
	for(int32 i = 0; i < c; i++) {
		if(pList) {
			THROW(temp_buf.Alloc(pList->getItemSize()+4096)); // +4096 страховка
		}
		else {
			THROW(temp_buf.Alloc(8192));
		}
		if(dir > 0) {
			memcpy(temp_buf.vptr(), pList->at(i), pList->getItemSize());
		}
		THROW(SerializeRecord(dir, temp_buf.vptr(), rBuf, pCtx));
		if(dir < 0 && pList) {
			THROW(pList->insert(temp_buf.vcptr()));
		}
	}
	CATCHZOK
	return ok;
}


int DBTable::SerializeArrayOfRecords(int dir, SArray * pList, SBuffer & rBuf, SSerializeContext * pCtx)
{
	if(dir < 0 && pList)
		pList->clear();
	return Helper_SerializeArrayOfRecords(dir, pList, rBuf, pCtx);
}

int DBTable::SerializeArrayOfRecords(int dir, SVector * pList, SBuffer & rBuf, SSerializeContext * pCtx)
{
	if(dir < 0 && pList)
		pList->clear();
	return Helper_SerializeArrayOfRecords(dir, pList, rBuf, pCtx);
}
