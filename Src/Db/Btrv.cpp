// BTRV.CPP
// Copyright (c) A. Sobolev 1994-1999, 2001, 2003, 2009, 2010, 2013, 2014, 2016, 2017, 2018, 2019, 2020
//
#include <slib-internal.h>
#pragma hdrstop

BtrCallProc   _BtrCall;
BtrCallProcID _BtrCallID;

static int __stdcall _BtrCall_Stub(int, char *, char *, uint16 * pDataBufferLenght, char *, int, int)
	{ return BE_BTRNINIT; }
static int __stdcall _BtrCallID_Stub(int, char *, char *, uint16 * pDataBufferLenght, char *, int, int, void *)
	{ return BE_BTRNINIT; }

static class BtrCallInit {
public:
	BtrCallInit()
	{
		if(!_BtrCall || !_BtrCallID) {
			ENTER_CRITICAL_SECTION
			if(!_BtrCall || !_BtrCallID) {
				HMODULE _btrv_dll_handle = ::LoadLibrary(_T("wbtrv32.dll"));
				if(!_BtrCall) {
					_BtrCall = reinterpret_cast<BtrCallProc>(GetProcAddress(_btrv_dll_handle, "BTRCALL"));
					SETIFZ(_BtrCall, _BtrCall_Stub);
				}
				if(!_BtrCallID) {
					_BtrCallID = reinterpret_cast<BtrCallProcID>(GetProcAddress(_btrv_dll_handle, "BTRCALLID"));
					SETIFZ(_BtrCallID, _BtrCallID_Stub);
				}
			}
			LEAVE_CRITICAL_SECTION
		}
	}
} BtrCallInitInstance;

#if 0 // {

int BTRCALL(int OP, char * POS_BLK, char * DATA_BUF, int16 * DATA_LEN, char * KEY_BUF, int KEY_LEN, int KEY_NUM)
{
#ifndef _MT // {
	static BtrCallProc _dll_btrcall = 0;
	if(!_dll_btrcall) {
		ENTER_CRITICAL_SECTION
		if(!_dll_btrcall) {
			HMODULE _btrv_dll_handle = LoadLibrary(_T("wbtrv32.dll"));
			_dll_btrcall = (BtrCallProc)GetProcAddress(_btrv_dll_handle, "BTRCALL");
		}
		LEAVE_CRITICAL_SECTION
	}
	if(_dll_btrcall)
		return (_dll_btrcall)(OP, POS_BLK, DATA_BUF, DATA_LEN, KEY_BUF, KEY_LEN, KEY_NUM);
	else
		return BE_BTRNINIT;
#else
	static BtrCallProcID _dll_btrcall = 0;
	if(!_dll_btrcall) {
		ENTER_CRITICAL_SECTION
		if(!_dll_btrcall) {
			HMODULE _btrv_dll_handle = LoadLibrary(_T("wbtrv32.dll"));
			_dll_btrcall = (BtrCallProcID)GetProcAddress(_btrv_dll_handle, "BTRCALLID");
		}
		LEAVE_CRITICAL_SECTION
	}
	if(_dll_btrcall)
		return (_dll_btrcall)(OP, POS_BLK, DATA_BUF, DATA_LEN, KEY_BUF, KEY_LEN, KEY_NUM, &DBS.GetTLA().ClientID);
	else
		return BE_BTRNINIT;
#endif  // } _MT
}

#endif // } 0

static int FASTCALL BRet(int r)
{
	if(r == 0) {
		return 1;
	}
	else {
		DbThreadLocalArea & r_tla = DBS.GetTLA();
		r_tla.LastDbErr = SDBERR_BTRIEVE;
		r_tla.LastBtrErr = r;
		return 0;
	}
}
//
//
//
const PageSzInfo Btrieve::LimitPgInfo[NUMPGSIZES] =
#if (BTRIEVE_VER >= 0x0600)
	{{512, 8}, {1024, 23}, {1536, 24}, {2048, 54}, {2560, 54}, {3072, 54}, {3584, 54}, {4096, 119}};
#else
	{{512, 8}, {1024, 24}, {1536, 24}, {2048, 24}, {2560, 24}, {3072, 24}, {3584, 24}, {4096, 24}};
#endif

/*static*/int FASTCALL Btrieve::StartTransaction(int concurrent, int lock)
{
	int    op = B_BEGTRANSACTION + lock;
	if(concurrent)
		op += 1000;
	return BRet(BTRV(op, 0, 0, 0, 0, WBTRVTAIL_ZZ));
}

/*static*/int Btrieve::RollbackWork()
{
	return BRet(BTRV(B_ABORTTRANSACTION, 0, 0, 0, 0, WBTRVTAIL_ZZ));
}

/*static*/int Btrieve::CommitWork()
{
	return BRet(BTRV(B_ENDTRANSACTION, 0, 0, 0, 0, WBTRVTAIL_ZZ));
}

/*static*/int Btrieve::AddContinuous(const char * pFileName /* "volume:\path[,volume:\path]*" */)
{
	int    index = 0;
	const  size_t fnlen = sstrlen(pFileName);
	uint16 bl = static_cast<uint16>(fnlen + 1);
	DBS.SetAddedMsgString(pFileName);
	STempBuffer temp_buf(fnlen+1);
	strnzcpy(temp_buf, pFileName, temp_buf.GetSize());
	return BRet(BTRV(B_CONTINUOUSOPR, 0, temp_buf, &bl, 0, WBTRVTAIL_Z));
}

/*static*/int Btrieve::RemoveContinuous(const char * pFileName /* if fname == 0 then remove all files */)
{
	int    index;
	uint16 bl;
	if(pFileName) {
		index = 2;
		bl = static_cast<uint16>(sstrlen(pFileName) + 1);
	}
	else {
		index = 1;
		bl = 0;
	}
	DBS.SetAddedMsgString(pFileName);
	STempBuffer temp_buf(8192);
	strnzcpy(temp_buf, pFileName, temp_buf.GetSize());
	return BRet(BTRV(B_CONTINUOUSOPR, 0, temp_buf, &bl, 0, WBTRVTAIL_Z));
}

/*static*/int Btrieve::GetVersion(int * pMajor, int * pMinor, int * pIsNet)
{
	struct {
		int16 major, minor;
		union {
			int8 isNet;
			int  dummy;
		};
	} buf[2];
	uint16 bl = sizeof(buf);
	int    ok = BRet(BTRV(B_VERSION, 0, reinterpret_cast<char *>(&buf), &bl, 0, WBTRVTAIL_ZZ));
	if(ok) {
		*pMajor = buf[0].major;
		*pMinor = buf[0].minor;
		*pIsNet = (int)buf[0].isNet;
	}
	return ok;
}

/*static*/int Btrieve::Reset(int station)
{
	char   buf[256];
	int    index;
	if(station) {
		*reinterpret_cast<int16 *>(buf) = static_cast<uint16>(station);
		index = -1;
	}
	else
		index = 0;
	return BRet(BTRV(B_RESET, buf, buf, reinterpret_cast<uint16 *>(buf), buf, WBTRVTAIL_Z));
}

/*static*/int Btrieve::CreateTable(const char * pFileName, DBFileSpec & rTblDesc, int createMode, const char * pAltCode)
{
	int    ok = 0;
	char   fpb[256];
	char * p_buf = 0;
	int16  buf_size = sizeof(DBFileSpec);
	int    index = createMode; // name 'index' used by macro WBTRVTAIL
	int    is_alt = 0;
	int    num_dup = 0;
	int    num_seg = 0;
	DBIdxSpec * p_is = reinterpret_cast<DBIdxSpec *>(&rTblDesc);
	for(int i = 0; i < rTblDesc.NumKeys; i++) {
		do {
			p_is++;
			num_seg++;
			if(p_is->flags & XIF_ACS)
				is_alt = 1;
			buf_size += sizeof(DBIdxSpec);
		} while(p_is->flags & XIF_SEG);
		if((p_is->flags & XIF_DUP) && !(p_is->flags & XIF_REPDUP))
			num_dup++;
	}
	//CHECK(is_alt == 0 || pAltCode != 0);
	//
	// Calculate optimal page size
	//
	// Parameter num_dup equal to number of declared duplicatable keys
	// minus number declared repeated duplicatable keys
	//
	//calcOptPageSize(&rTblDesc, num_dup, num_seg);
	if(!rTblDesc.PageSize) {
		if(rTblDesc.RecSize > 4076) {
			rTblDesc.PageSize = 4096;
			rTblDesc.Flags |= XTF_COMPRESS;
		}
		else {
			if(rTblDesc.Flags & XTF_EXTRADUP)
				num_dup += rTblDesc.ExtraDup;
			int    real_size = 2 + (num_dup * 8) + rTblDesc.RecSize;
			if(rTblDesc.Flags & XTF_VLR)
				real_size += 4;
			if(rTblDesc.Flags & XTF_TRUNCATE)
				real_size += ((rTblDesc.Flags & XTF_VAT) ? 4 : 2);
			if(rTblDesc.Flags & XTF_VAT)
				real_size += 4;
			int    num_pg = 8;
			int    min_delta = 4096;
			for(int i = 1; i <= NUMPGSIZES; i++) {
				if(num_seg <= Btrieve::LimitPgInfo[i-1].maxKeySegs) {
					int    delta = ((i * 512) - 8) % real_size;
					if(delta < min_delta) {
						num_pg = i;
						min_delta = delta;
					}
				}
			}
			rTblDesc.PageSize = num_pg * 512;
		}
	}
	rTblDesc.NumSeg = 0;
	if(is_alt) {
		p_buf = static_cast<char *>(catmem(&rTblDesc, buf_size, pAltCode, 265));
		buf_size += 265;
	}
	else
		p_buf = reinterpret_cast<char *>(&rTblDesc);
	if(p_buf) {
		int    be = 0;
		const  size_t fn_len = sstrlen(pFileName);
		STempBuffer fn_buf(fn_len+32);
		if(sstrchr(pFileName, ' ')) {
			fn_buf[0] = '\"';
			memcpy(fn_buf+1, pFileName, fn_len);
			fn_buf[fn_len+1] = '\"';
			fn_buf[fn_len+2] = 0;
		}
		else {
			memcpy(fn_buf, pFileName, fn_len);
			fn_buf[fn_len] = 0;
		}
		do {
			be = BTRV(B_CREATE, fpb, p_buf, reinterpret_cast<uint16 *>(&buf_size), fn_buf, WBTRVTAIL);
		} while(oneof2(be, BE_INVKEYLEN, BE_INVRECLEN) && (reinterpret_cast<DBFileSpec *>(p_buf)->PageSize += 512) <= 8192);
		ok = BRet(be);
		DBTable::InitErrFileName(pFileName);
		SAlloc::F(p_buf); // @v9.0.7 delete-->free
	}
	else
		ok = BRet(BE_NOMEM);
	return ok;
}
//
//
//
DbDict_Btrieve::DbDict_Btrieve(const char * pPath) : DbDictionary()
{
	MEMSZERO(flq);
	MEMSZERO(ilq);
	ushort pw[5];
	SString base_path, buf;
	// DDF password = "..SC...."
	pw[0] = pw[2] = pw[3] = 0x2E2E;
	pw[1] = 0x4353;
	pw[4] = 0;
	char * p_pw_arg = reinterpret_cast<char *>(&pw);
	(base_path = pPath).SetLastSlash();
	if(!xfile.Btr_Open((buf = base_path).Cat(BDictionary::DdfTableFileName), omNormal, p_pw_arg))
		State |= stError;
	if(IsValid()) {
		xfile.setDataBuf(&fileBuf, sizeof(fileBuf));
		if(!xfield.Btr_Open((buf = base_path).Cat(BDictionary::DdfFieldFileName), omNormal, p_pw_arg))
			State |= stError;
	}
	if(IsValid()) {
		xfield.setDataBuf(&fieldBuf, sizeof(fieldBuf));
		if(!xindex.Btr_Open((buf = base_path).Cat(BDictionary::DdfIndexFileName), omNormal, p_pw_arg))
			State |= stError;
	}
	if(IsValid()) {
		xindex.setDataBuf(&indexBuf, sizeof(indexBuf));
		xfile.clearDataBuf();
		xfield.clearDataBuf();
		xindex.clearDataBuf();
	}
}

DbDict_Btrieve::~DbDict_Btrieve()
{
	xfile.close();
	xfield.close();
	xindex.close();
}

int DbDict_Btrieve::LoadTableSpec(DBTable * pTbl, const char * pTblName)
{
	int    ok = 1;
	DBTable * p_clone = 0;
	long   tbl_id = 0;
	DbTableStat tbl_stat;
	THROW(GetTableID(pTblName, &tbl_id, &tbl_stat));
	pTbl->tableID = static_cast<BTBLID>(tbl_id);
	STRNSCPY(pTbl->tableName, pTblName);
	pTbl->fileName = tbl_stat.Location;
	pTbl->flags = tbl_stat.Flags;
	pTbl->PageSize = static_cast<uint16>(tbl_stat.PageSize);
	p_clone = DBS.GetTLA().GetCloneEntry(static_cast<BTBLID>(tbl_id));
	if(p_clone) {
		pTbl->fields.copy(&p_clone->fields);
		pTbl->indexes.copy(&p_clone->indexes);
	}
	else {
		pTbl->fields.reset();
		pTbl->indexes.reset();
		THROW(getFieldList(static_cast<BTBLID>(tbl_id), &pTbl->fields));
		THROW(getIndexList(static_cast<BTBLID>(tbl_id), &pTbl->indexes));
		//
		// Функция getIndexList инициализирует сегменты
		// индексов через внутренние идентификаторы полей. Здесь
		// мы конвертируем эти ссылки так, чтобы индексы ссылась
		// на поля по номеру поля в списке fields
		//
		for(uint i = 0, nk = pTbl->indexes.getNumKeys(); i < nk; i++) {
			BNKey key = pTbl->indexes[i];
			for(uint j = 0, ns = key.getNumSeg(); j < ns; j++) {
				uint   pos = 0;
				pTbl->fields.getFieldPosition(key.getFieldID(j), &pos);
				key.setFieldID(j, pos);
			}
		}
	}
	pTbl->indexes.setTableRef(offsetof(DBTable, indexes));
	//
	pTbl->InitLob();
	CATCH
		pTbl->tableID = 0;
		ok = 0;
	ENDCATCH
	return ok;
}

static BTBLID getUniqueKey(DBTable * tbl, BTBLID idx)
{
	int16  k = SHRT_MAX;
	if(tbl->searchKey(idx, &k, spLast)) {
		if((k + 1) < SHRT_MAX)
			return (k + 1);
		else
			for(int16 i = 1; i < SHRT_MAX; i++) {
				if(tbl->searchKey(idx, &(k = i), spEq) == 0)
					return BTRNFOUND ? i : 0;
			}
	}
	else if(BtrError == BE_EOF)
		return 1;
	return 0;
}

int DbDict_Btrieve::CreateTableSpec(DBTable * pTbl)
{
	// EXCEPTVAR(BtrError);
	int    ok = 1, ta = 0;
	uint   i;
	union {
		int _t, j;
	};
	long   tbl_id = 0;
	BNFieldList & fl = pTbl->fields;
	BNKeyList   & kl = pTbl->indexes;
	BNKey  key;
	BExtInsert * p_bei = 0;
	THROW(IsValid() && GetTableID(pTbl->tableName, &tbl_id, 0) == 0);
	THROW(Btrieve::StartTransaction());
	ta = 1;
	tbl_id = IsValid() ? getUniqueKey(&xfile, 0) : 0;
	THROW(tbl_id);
	fileBuf.XfId = pTbl->tableID = static_cast<BTBLID>(tbl_id);
	memcpy(memset(fileBuf.XfName, ' ', sizeof(fileBuf.XfName)), pTbl->tableName, sstrlen(pTbl->tableName));
	memcpy(memset(fileBuf.XfLoc, ' ', sizeof(fileBuf.XfLoc)), pTbl->fileName, sstrlen(pTbl->fileName));
	fileBuf.XfFlags   = pTbl->flags;
	fileBuf.XfBTFlags = pTbl->flags;
	fileBuf.XfOwnrLvl = pTbl->ownrLvl;
	memzero(fileBuf.reserv, sizeof(fileBuf.reserv));
	THROW(xfile.insertRec());
	THROW(p_bei = new BExtInsert(&xfield));
	THROW(tbl_id = getUniqueKey(&xfield, 0));
	BTBLID fld_id = static_cast<BTBLID>(tbl_id);
	for(i = 0; i < fl.getCount(); i++) {
		const BNField & f = fl.getField(i);
		const size_t fname_len = sstrlen(f.Name);
		fl.setFieldId(i, fld_id);
		if(GETSTYPE(f.T) == S_DATETIME) {
			{
				MEMSZERO(fieldBuf);
				fieldBuf.XeId   = fld_id++;
				fieldBuf.XeFile = pTbl->tableID;
				memset(fieldBuf.XeName, ' ', sizeof(fieldBuf.XeName));
				memcpy(fieldBuf.XeName, f.Name, fname_len);
				fieldBuf.XeName[fname_len] = '$';
				fieldBuf.XeName[fname_len+1] = 'd';
				fieldBuf.XeDataType = SLib2BtrType(S_DATE);
				fieldBuf.XeOffset = static_cast<int16>(f.Offs);
				fieldBuf.XeSize = sizeof(LDATE);
				THROW(p_bei->insert(&fieldBuf));
			}
			{
				MEMSZERO(fieldBuf);
				fieldBuf.XeId   = fld_id++;
				fieldBuf.XeFile = pTbl->tableID;
				memset(fieldBuf.XeName, ' ', sizeof(fieldBuf.XeName));
				memcpy(fieldBuf.XeName, f.Name, fname_len);
				fieldBuf.XeName[fname_len] = '$';
				fieldBuf.XeName[fname_len+1] = 't';
				fieldBuf.XeDataType = SLib2BtrType(S_TIME);
				fieldBuf.XeOffset = static_cast<int16>(f.Offs+sizeof(LDATE)); // @v10.3.4 @fix sizeof(S_DATE)-->sizeof(LDATE)
				fieldBuf.XeSize = sizeof(LTIME);
				THROW(p_bei->insert(&fieldBuf));
			}
		}
		else {
			MEMSZERO(fieldBuf);
			fieldBuf.XeId   = fld_id++;
			fieldBuf.XeFile = pTbl->tableID;
			memset(fieldBuf.XeName, ' ', sizeof(fieldBuf.XeName));
			memcpy(fieldBuf.XeName, f.Name, fname_len);
			_t = GETSTYPE(f.T);
			fieldBuf.XeDataType = SLib2BtrType(_t);
			fieldBuf.XeOffset = static_cast<int16>(f.Offs);
			fieldBuf.XeSize = static_cast<int16>(stsize(f.T));
			fieldBuf.XeDec = oneof3(_t, S_DEC, S_MONEY, S_NUMERIC) ? static_cast<int8>(GETSPRECD(f.T)) : 0;
			THROW(p_bei->insert(&fieldBuf));
		}
	}
	THROW(p_bei->flash());
	ZDELETE(p_bei);
	THROW(p_bei = new BExtInsert(&xindex));
	for(i = 0; i < kl.getNumKeys(); i++) {
		for(key = kl.getKey(i), j = 0; j < key.getNumSeg(); j++) {
			indexBuf.XiFile   = pTbl->tableID;
			const int fld_id = key.getFieldID(j);
			const BNField * p_fld = &fl.getField(fld_id);
			THROW(p_fld);
			indexBuf.XiField  = static_cast<int16>(p_fld->Id);
			indexBuf.XiNumber = key.getKeyNumber();
			indexBuf.XiPart   = j;
			indexBuf.XiFlags  = key.getFlags(j);
			THROW(p_bei->insert(&indexBuf));
		}
	}
	THROW(p_bei->flash());
	THROW(Btrieve::CommitWork());
	ta = 0;
	CATCH
		if(ta)
			Btrieve::RollbackWork();
		ok = 0;
	ENDCATCH
	delete p_bei;
	return ok;
}

int DbDict_Btrieve::DropTableSpec(const char * pTblName, DbTableStat * pStat)
{
	int    ok = 1;
	long   tbl_id = 0;
	int    i, t = 0;
	DbTableStat stat;
	Btrieve::StartTransaction(1);
	t = 1;
	if(GetTableID(pTblName, &tbl_id, &stat)) {
		THROW(!(stat.Flags & XTF_DICT));
		THROW(xfile.deleteRec());
		i = tbl_id;
		if(xfield.search(1, &i, spEq))
			do {
				THROW(xfield.deleteRec());
			} while(xfield.search(&i, spNext) && i == tbl_id);
		i = tbl_id;
		if(xindex.search(0, &i, spEq))
			do {
				THROW(xindex.deleteRec());
			} while(xindex.search(&i, spNext) && i == tbl_id);
	}
	else
		ok = -1;
	Btrieve::CommitWork();
	t = 0;
	CATCH
		if(t)
			Btrieve::RollbackWork();
		ok = 0;
	ENDCATCH
	ASSIGN_PTR(pStat, stat);
	return ok;
}

int DbDict_Btrieve::ExtractStat(const XFile & rRec, DbTableStat * pStat) const
{
	if(pStat) {
		pStat->Clear();
		pStat->ID = rRec.XfId;
		pStat->OwnerLevel = rRec.XfOwnrLvl;
		pStat->Flags = rRec.XfFlags;
		const char * p = sstrchr(rRec.XfLoc, ' ');
		if(p)
			pStat->Location.Z().CatN(rRec.XfLoc, p-rRec.XfLoc);
		else
			pStat->Location.CopyFromN(rRec.XfLoc, sizeof(rRec.XfLoc));
		p = sstrchr(rRec.XfName, ' ');
		if(p)
			pStat->TblName.Z().CatN(rRec.XfName, p-rRec.XfName);
		else
			pStat->TblName.CopyFromN(rRec.XfName, sizeof(rRec.XfName));
		pStat->Location.Strip();
		pStat->TblName.Strip();
		pStat->RetItems = (DbTableStat::iID|DbTableStat::iOwnerLevel|DbTableStat::iFlags|DbTableStat::iName|DbTableStat::iLocation);
	}
	return 1;
}

int DbDict_Btrieve::GetTableID(const char * pTblName, long * pID, DbTableStat * pStat)
{
	int    ok = 0;
	char   key[BTRMAXKEYLEN];
	if(IsValid()) {
		size_t len = sstrlen(STRNSCPY(key, pTblName));
		memset(key + len, ' ', sizeof(fileBuf.XfName) - len);
		if(xfile.search(1, key, spEq)) {
			*pID = fileBuf.XfId;
			ExtractStat(fileBuf, pStat);
			ok = 1;
		}
		else
			BtrError = BE_TABLEISNOTDEFINED;
	}
	return ok;
}

int DbDict_Btrieve::GetTableInfo(long tblID, DbTableStat * pStat)
{
	int    ok = 1;
	BTBLID tbl_id = static_cast<BTBLID>(tblID);
	if(IsValid() && xfile.search(0, &tbl_id, spEq))
		ExtractStat(fileBuf, pStat);
	else
		ok = 0;
	return ok;
}

int DbDict_Btrieve::GetListOfTables(long options, StrAssocArray * pList)
{
	int    ok = -1;
	char   key[256];
	if(IsValid()) {
		SString temp_buf;
		if(xfile.search(1, key, spFirst))
			do {
				if(pList) {
					char * p = sstrchr(fileBuf.XfName, ' ');
					if(p) {
						p[0] = 0;
						temp_buf = fileBuf.XfName;
					}
					else
						temp_buf.CopyFromN(fileBuf.XfName, sizeof(fileBuf.XfName));
					pList->Add(fileBuf.XfId, temp_buf);
				}
				ok = 1;
			} while(xfile.search(key, spNext));
	}
	return ok;
}

#define EG_sign 0x4745U
#define UC_sign 0x4355U

void DbDict_Btrieve::makeFldListQuery(BTBLID tblID, int numRecs)
{
	if(flq.h.bufLen != sizeof(flq)) {
		flq.h.bufLen     = sizeof(flq);
		flq.h.maxSkip    = 0;
		flq.h.numTerms   = 2;
		flq.t1.fldType   = SLib2BtrType(S_INT);
		flq.t1.fldLen    = 2;
		flq.t1.fldOfs    = offsetof(XField, XeFile);
		flq.t1.cmp       = BExtQuery::SlToBeqOp(_EQ_);
		flq.t1.link      = BExtQuery::SlToBeqLink(_AND___);
		flq.t2.fldType   = SLib2BtrType(S_INT);
		flq.t2.fldLen    = 1;
		flq.t2.fldOfs    = offsetof(XField, XeDataType);
		flq.t2.cmp       = BExtQuery::SlToBeqOp(_NE_);
		flq.t2.link      = BExtQuery::SlToBeqLink(_END___);
		flq.minus_one    = -1;
		flq.th.numFlds   = 2;
		flq.ti[0].fldLen = sizeof(int16);
		flq.ti[0].fldOfs = 0;
		flq.ti[1].fldLen = sizeof(BFIELDNAME)+6;
		flq.ti[1].fldOfs = offsetof(XField, XeName);
	}
	flq.h.signature = UC_sign;
	flq.tblID       = tblID;
	flq.th.numRecs  = numRecs;
}

void DbDict_Btrieve::makeIdxListQuery(BTBLID tblID, int numRecs)
{
	if(ilq.h.bufLen != sizeof(ilq)) {
		ilq.h.bufLen     = sizeof(ilq);
		ilq.h.maxSkip    = 0;
		ilq.h.numTerms   = 1;
		ilq.t.fldType    = SLib2BtrType(S_INT);
		ilq.t.fldLen     = 2;
		ilq.t.fldOfs     = offsetof(XIndex, XiFile);
		ilq.t.cmp        = BExtQuery::SlToBeqOp(_EQ_);
		ilq.t.link       = BExtQuery::SlToBeqLink(_END___);
		ilq.th.numFlds   = 1;
		ilq.ti[0].fldLen = sizeof(XIndex);
		ilq.ti[0].fldOfs = 0;
	}
	ilq.h.signature = UC_sign;
	ilq.tblID       = tblID;
	ilq.th.numRecs  = numRecs;
}

int DbDict_Btrieve::getFieldList(BTBLID tblID, BNFieldList * fields)
{
	EXCEPTVAR(BtrError);
	struct _XFR {
		uint16 reclen;
		RECORDNUMBER recpos;
		int16  id;
		BFIELDNAME   name;
		char   typ;
		int16  ofs;
		int16  siz;
		char   dec;
	};
	const int  numRecs = 32;
	const uint bsize = sizeof(BExtResultHeader) + numRecs * (sizeof(_XFR) + sizeof(BExtResultItem));
	char   s[MAXFIELDNAME];
	int16  key = tblID;
	int    r;
	uint   i, count;
	char * q = 0;
	memzero(s, sizeof(s));
	THROW(IsValid() && tblID);
	THROW_V(xfield.search(1, &key, spEq), BE_FNFOUND);
	makeFldListQuery(tblID, numRecs);
	THROW_V(q = static_cast<char *>(SAlloc::M(bsize)), BE_NOMEM);
	xfield.setDataBuf(q, bsize);
	do {
		memcpy(q, &flq, sizeof(flq));
		THROW(xfield.getExtended(&key, spNext) || BTRNFOUND || (BtrError == BE_REJECTLIMIT));
		count = *reinterpret_cast<const uint16 *>(q);
		for(i = 0; i < count; i++) {
   	    	_XFR * d = reinterpret_cast<_XFR *>(q + sizeof(BExtResultHeader) + i * sizeof(_XFR));
			d->name[sizeof(d->name)-1] = 0;
			strip(d->name);
			fields->addField(/*s*/d->name, MKSTYPED(Btr2SLibType(d->typ), d->siz, d->dec), d->id);
		}
		flq.h.signature = EG_sign;
	} while(count == numRecs);
	r = 1;
	CATCH
		r = 0;
	ENDCATCH
	xfield.setDataBuf(&fieldBuf, sizeof(fieldBuf));
	SAlloc::F(q);
	return r;
}

int DbDict_Btrieve::getIndexList(BTBLID tblID, BNKeyList * pKeyList)
{
	EXCEPTVAR(BtrError);
	const  int  num_recs = 32;
	const  uint bsize = sizeof(BExtResultHeader) + num_recs * (sizeof(XIndex) + sizeof(BExtResultItem));
	BNKey  k;
	int16  key = tblID;
	int    r, i, count;
	char * q = 0;
	THROW(IsValid() && tblID);
	THROW_V(xindex.search(0, &key, spEq), BE_FNFOUND);
	makeIdxListQuery(tblID, num_recs);
	THROW_V(q = static_cast<char *>(SAlloc::M(bsize)), BE_NOMEM);
	xindex.setDataBuf(q, bsize);
	do {
		memcpy(q, &ilq, sizeof(ilq));
		THROW(xindex.getExtended(&key, spNext) || BTRNFOUND || (BtrError == BE_REJECTLIMIT));
		count = *reinterpret_cast<const uint16 *>(q);
		for(i = 0; i < count; i++) {
   	    	const XIndex * d = reinterpret_cast<const XIndex *>(q + sizeof(BExtResultHeader) + i * (sizeof(BExtResultItem) + sizeof(XIndex)) + sizeof(BExtResultItem));
			k.addSegment(d->XiField, d->XiFlags);
			if(!(d->XiFlags & XIF_SEG)) {
				k.setKeyParams(d->XiNumber, 0);
				pKeyList->addKey(k);
			}
		}
		ilq.h.signature = EG_sign;
	} while(count == num_recs);
	r = 1;
	CATCH
		r = 0;
	ENDCATCH
	xindex.setDataBuf(&indexBuf, sizeof(indexBuf));
	SAlloc::F(q);
	return r;
}
//
// XRelate (DDF struct)
//
struct XRelate { // @unused
	int16  XrPID;
	int16  XrPIndex;
	int16  XrFID;
	int16  XrFIndex;
	int8   XrName[MAXFKEYNAME];
	int8   XrUpdateRule;
	int8   XrDeleteRule;
	int8   XrReserved[30];
};
