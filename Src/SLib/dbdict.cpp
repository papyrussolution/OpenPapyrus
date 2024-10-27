// DBDICT.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
//
//
//
/*static*/DbDictionary * (*DbDictionary::CreateInstanceProc)(const char * pPath, long options) = 0;

DbTableStat::DbTableStat() : ReqItems(0), RetItems(0), UnsupItem(0), ID(0), OwnerLevel(0), Flags(0), FixRecSize(0), NumRecs(0), FldCount(0), IdxCount(0), PageSize(0)
{
}

DbTableStat & DbTableStat::Z()
{
	ID = 0;
	OwnerLevel = 0;
	Flags = 0;
	ReqItems = 0;
	RetItems = 0;
	UnsupItem = 0;
	FixRecSize = 0;
	NumRecs = 0;
	FldCount = 0;
	IdxCount = 0;
	PageSize = 0;
	TblName.Z();
	Location.Z();
	OwnerName.Z();
	SpaceName.Z();
	FldList.Z();
	IdxList.Z();
	return *this;
}

DbDictionary::DbDictionary() : State(0)
{
}

DbDictionary::~DbDictionary()
{
}

bool DbDictionary::IsValid() const { return !(State & stError); }

/*static*/void DbDictionary::SetCreateInstanceProc(DbDictionary * (*proc)(const char * pPath, long options))
{
	ENTER_CRITICAL_SECTION
	CreateInstanceProc = proc;
	LEAVE_CRITICAL_SECTION
}


/*static*/DbDictionary * DbDictionary::CreateInstance(const char * pPath, long options)
{
	return CreateInstanceProc ? CreateInstanceProc(pPath, options) : new DbDict_Btrieve(pPath);
}
//
//
/*static*/const char * BDictionary::DdfTableFileName = "FILE.DDF";
/*static*/const char * BDictionary::DdfFieldFileName = "FIELD.DDF";
/*static*/const char * BDictionary::DdfIndexFileName = "INDEX.DDF";
//
// Implementation of API functions
//
void DBRemoveTempFiles()
{
	CALLPTRMEMB(CurDict, RemoveTempFiles());
}
//
// Implementation of class BDictionary
//
/*static*/BDictionary * BDictionary::CreateBtrDictInstance(const char * pPath)
{
	return new BDictionary(1, pPath);
}

int BDictionary::Init(const char * pDataPath, const char * pTempPath)
{
	assert(P_Dict);
	DbName.Z();
	DbPathID = 0;
	DataPath = NZOR(pDataPath, Path);
	if(pTempPath && ::access(pTempPath, 0) == 0)
		TempPath = pTempPath;
	if(IsValid()) {
		DBS.GetDbPathID(DataPath, &DbPathID);
		GetProtectData();
	}
	return IsValid();
}

BDictionary::BDictionary(const char * pPath, const char * pDataPath, const char * pTempPath) : DbProvider(DbDictionary::CreateInstance(pPath, 0), 0)
	{ Init(pDataPath, pTempPath); }
BDictionary::BDictionary(int btrDict, const char * pPath) : DbProvider(new DbDict_Btrieve(pPath), 0)
	{ Init(0, 0); }

/*virtual*/int BDictionary::GetDatabaseState(uint * pStateFlags)
{
	int    ok = 1;
	uint   state = 0;
	if(DataPath.NotEmpty()) {
		SDirEntry de;
		SString wc;
		(wc = DataPath).SetLastSlash().CatChar('*').DotCat("^^^");
		for(SDirec sd(wc); sd.Next(&de) > 0;) {
			if(de.IsFile()) {
				state |= dbstContinuous;
				break;
			}
		}
	}
	ASSIGN_PTR(pStateFlags, state);
	return ok;
}

/*virtual*/SString & BDictionary::MakeFileName_(const char * pTblName, SString & rFileName)
{
	SFsPath ps(rFileName);
	if(ps.Dir.IsEmpty()) {
		int    path_from_redirect = 0;
		SString data_path;
		GetDataPath(data_path);
		SString redirect_file = data_path;
		redirect_file.SetLastSlash().Cat(FILE_REDIRECT);
		if(::fileExists(redirect_file)) {
			SFile f(redirect_file, SFile::mRead);
			if(f.IsValid()) {
				SString buf, tbl_name, tbl_path;
				while(!path_from_redirect && f.ReadLine(buf) > 0) {
					uint j = 0;
					buf.Divide('=', tbl_name, tbl_path);
					tbl_name.Strip();
					if(ps.Nam.CmpNC(tbl_name) == 0) {
						tbl_path.TrimRightChr('\x0A').TrimRightChr('\x0D').Strip().SetLastSlash();
						SFsPath rps(tbl_path);
						rps.Merge(&ps, SFsPath::fNam|SFsPath::fExt, rFileName);
						path_from_redirect = 1;
						break;
					}
				}
			}
		}
		if(!path_from_redirect && data_path.NotEmpty())
			(rFileName = data_path).SetLastSlash().Cat(ps.Nam).Dot().Cat(ps.Ext);
	}
	return rFileName;
}

/*virtual*/ int BDictionary::CreateDataFile(const DBTable * pTbl, const char * pFileName, int createMode, const char * pAltCode)
{
	int    ok = 1;
	DBFileSpec * p_h = new DBFileSpec; // разрушается функцией Btrieve::CreateTable
	memzero(p_h, sizeof(*p_h));
	p_h->PageSize = pTbl->PageSize;
	p_h->RecSize = pTbl->fields.getRecSize();
	p_h->Flags   = pTbl->flags;
	for(uint i = 0; i < pTbl->indexes.getNumKeys(); i++) {
		BNKey k = pTbl->indexes.getKey(i);
		for(int j = 0; j < k.getNumSeg(); j++) {
			const BNField & f = pTbl->fields[k.getFieldID(j)];
			int16  offs = static_cast<int16>(f.Offs + 1);
			if(GETSTYPE(f.T) == S_DATETIME) {
				//
				// Для типа S_DATETIME в btrieve-таблице придется создать два сегмента: S_DATE; S_TIME
				//
				{
					DBIdxSpec & r_idx = *new DBIdxSpec;
					MEMSZERO(r_idx);
					r_idx.position  = offs;
					r_idx.length    = sizeof(LDATE);
					r_idx.flags     = k.getFlags(j);
					r_idx.extType   = SLib2BtrType(S_DATE);
					r_idx.keyNumber = k.getKeyNumber();
					r_idx.acsNumber = k.getACSNumber();
					p_h = &((*p_h) + r_idx);
				}
				{
					DBIdxSpec & r_idx = *new DBIdxSpec;
					MEMSZERO(r_idx);
					r_idx.position  = offs + static_cast<int16>(sizeof(LDATE));
					r_idx.length    = sizeof(LTIME);
					r_idx.flags     = k.getFlags(j);
					r_idx.extType   = SLib2BtrType(S_TIME);
					r_idx.keyNumber = k.getKeyNumber();
					r_idx.acsNumber = k.getACSNumber();
					p_h = &((*p_h) + r_idx);
				}
			}
			else {
				DBIdxSpec & r_idx = *new DBIdxSpec;
				MEMSZERO(r_idx);
				r_idx.position  = offs;
				r_idx.length    = static_cast<int16>(stsize(f.T));
				r_idx.flags     = k.getFlags(j);
				r_idx.extType   = SLib2BtrType(GETSTYPE(f.T));
				r_idx.keyNumber = k.getKeyNumber();
				r_idx.acsNumber = k.getACSNumber();
				p_h = &((*p_h) + r_idx);
			}
		}
	}
	SString b(NZOR(pFileName, pTbl->fileName.cptr()));
	MakeFileName_(pTbl->tableName, b);
	if(!Btrieve::CreateTable(b, *p_h, RESET_CRM_TEMP(createMode), pAltCode))
		ok = 0;
	else {
		//
		// В Windows 7 от момента успешного создания файла, до фактического
		// его появления может проийти некоторое время. По этому, формируем
		// задержку для ожидания появления созданного файла.
		//
		while(!IsFileExists_(b)) {
			SDelay(10);
		}
		if(createMode < 0 && IS_CRM_TEMP(createMode)) {
			//
			// Регистрируем имя временного файла в драйвере БД для последующего удаления //
			//
			AddTempFileName(b);
		}
	}
	return ok;
}

/*virtual*/ int BDictionary::IsFileExists_(const char * pFileName)
{
	int    yes = 0;
	if(DBS.GetConfig().Flags & DbSession::fDetectExistByOpen) {
		char   fpb[256];
		uint16 bl = 0;
		SString file_name;
		if(sstrchr(pFileName, ' '))
			file_name.Z().CatQStr(pFileName);
		else
			file_name = pFileName;
		char   temp_buf[512];
		file_name.CopyTo(temp_buf, sizeof(temp_buf));
		int    ret = BTRV(B_OPEN, fpb, 0 /*pPassword*/, &bl, temp_buf, 0, omReadOnly);
		if(!ret) {
			int    cret = BTRV(B_CLOSE, fpb, 0, 0, 0, 0, 0);
			yes = 1;
		}
		else if(oneof2(ret, BE_INVFNAME, BE_FNFOUND))
			yes = 0;
		else
			yes = -1;
	}
	else {
		yes = fileExists(pFileName);
	}
	return yes;
}

/*virtual*/ int BDictionary::DropFile(const char * pFileName)
	{ return (IsFileExists_(pFileName) > 0) ? SFile::Remove(pFileName) : -1; }
/*virtual*/ int BDictionary::GetFileStat(DBTable * pTbl, long reqItems, DbTableStat * pStat)
	{ return pTbl->Btr_GetStat(reqItems, pStat); }

/*virtual*/ int BDictionary::Login(const DbLoginBlock * pBlk, long options)
{
	EXCEPTVAR(DBErrCode);
	int    ok = 1;
	int    b_major = 0, b_minor = 0, b_isnet = 0;
	THROW(Btrieve::GetVersion(&b_major, &b_minor, &b_isnet));
	DBS.SetAddedMsgString("Btrieve 6.15");
	THROW_V(b_major >= 6, SDBERR_INCOMPATDBVER);
	Common_Login(pBlk);
	CATCHZOK
	return ok;
}

/*virtual*/ int BDictionary::Logout()
{
	Btrieve::Reset(0);
	Common_Logout();
	return 1;
}

/*virtual*/ int BDictionary::ProtectTable(long dbTableID, char * pResetOwnrName, char * pSetOwnrName, int clearProtection)
{
	int    ok = 1;
	DBTable tbl;
	if(IsValid()) {
		DbTableStat ts;
		if(GetTableInfo(dbTableID, &ts)) {
			if(!(ts.Flags & XTF_DICT) && ts.OwnerLevel > 0 && ts.OwnerLevel < 5) {
				SString path;
				path = ts.Location;
				if(IsFileExists_(MakeFileName_(ts.TblName, path)) > 0) {
					THROW(tbl.Btr_Open(path, omNormal, pResetOwnrName));
					if(!clearProtection) {
						THROW(tbl.Btr_Decrypt());
						THROW(tbl.Btr_Encrypt(pSetOwnrName, ts.OwnerLevel-1));
					}
					else {
						THROW(tbl.Btr_Decrypt());
					}
					tbl.close();
				}
			}
		}
		else {
			THROW(BTRNFOUND);
		}
	}
	CATCHZOK
	return ok;
}

/*virtual*/ int BDictionary::StartTransaction()
	{ return Btrieve::StartTransaction(1, 0); }
/*virtual*/ int BDictionary::CommitWork()
	{ return Btrieve::CommitWork(); }
/*virtual*/ int BDictionary::RollbackWork()
	{ return Btrieve::RollbackWork(); }
/*virtual*/ int BDictionary::Implement_Open(DBTable * pTbl, const char * pFileName, int openMode, char * pPassword)
	{ return pTbl->Btr_Open(pFileName, openMode, pPassword); }
/*virtual*/ int BDictionary::Implement_Close(DBTable * pTbl)
	{ return pTbl->Btr_Close(); }
/*virtual*/ int BDictionary::Implement_Search(DBTable * pTbl, int idx, void * pKey, int srchMode, long sf)
	{ return pTbl->Btr_Implement_Search(idx, pKey, srchMode, sf); }
/*virtual*/ int BDictionary::Implement_InsertRec(DBTable * pTbl, int idx, void * pKeyBuf, const void * pData)
	{ return pTbl->Btr_Implement_InsertRec(idx, pKeyBuf, pData); }
/*virtual*/ int BDictionary::Implement_UpdateRec(DBTable * pTbl, const void * pDataBuf, int ncc)
	{ return pTbl->Btr_Implement_UpdateRec(pDataBuf, ncc); }
/*virtual*/ int BDictionary::Implement_DeleteRec(DBTable * pTbl)
	{ return pTbl->Btr_Implement_DeleteRec(); }
/*virtual*/ int BDictionary::Implement_BExtInsert(BExtInsert * pBei)
	{ return pBei->getTable()->Btr_Implement_BExtInsert(pBei); }
/*virtual*/ int BDictionary::Implement_GetPosition(DBTable * pTbl, DBRowId * pPos)
	{ return pTbl->Btr_Implement_GetPosition(pPos); }

BDictionary::~BDictionary()
{
	RemoveTempFiles();
}
//
// Процедура создания временной таблицы
//
/*virtual*/ SString & BDictionary::GetTemporaryFileName(SString & rFileNameBuf, long * pStart, int forceInDataPath)
{
	const char * p_path = (TempPath && !forceInDataPath) ? TempPath : DataPath;
	return MakeTempFileName(p_path, "TMP", "BTR", pStart, rFileNameBuf);
}
//
//
//
BRecoverParam::BRecoverParam() : P_DestPath(0), P_BakPath(0), Format(0), Flags(0), OrgNumRecs(0), ActNumRecs(0), ErrCode(0), Tm(0)
{
}

int BRecoverParam::callbackProc(int, const void * lp1, const void * lp2, const void * vp)
{
	return -1;
}

#define BRPF_REPLACE   0x0001
#define BRPF_PROTECT   0x0002
#define BRPF_SKIPDUP   0x0004

int BDictionary::RecoverTable(BTBLID tblID, BRecoverParam * pParam)
{
	int    ok = 1, replace_src = 0;
	int16  k  = tblID;
	DBRowId pos;
	char   acs[265];
	char   buf[MAX_PATH];
	STempBuffer rec_buf(8192);
	//long   s_bak_ext = 0x5F5F5FL;
	static const char * p_bak_ext = "___";
	SString path, dest, spart;
	TablePartsEnum tpe(0);
	DBTable tbl, newtbl;
	pParam->OrgNumRecs = pParam->ActNumRecs = 0;
	if(IsValid()) {
		DbTableStat ts;
		if(GetTableInfo(tblID, &ts)) {
			path = ts.Location;
			if(path.NotEmpty() && IsFileExists_(MakeFileName_(ts.TblName, path)) > 0) {
				RECORDSIZE fix_rec_size = 0;
				SString tbl_name = ts.TblName;
				DBS.GetProtectData(buf, 1);
				THROW(tbl.open(tbl_name, path, omReadOnly));
				tbl.setDataBuf(rec_buf, static_cast<RECORDSIZE>(rec_buf.GetSize()));
				tbl.getNumRecs(&pParam->OrgNumRecs);
				fix_rec_size = tbl.getRecSize();
				if(pParam->P_DestPath && pParam->P_DestPath[0]) {
					(dest = pParam->P_DestPath).Strip().SetLastSlash();
					dest.Cat(ts.Location);
					//
					// Удаляется файл назначения (если таблица состоит из нескольких файлов, то удаляются все файлы)
					//
					for(tpe.Init(dest); tpe.Next(spart) > 0;) {
						if(fileExists(spart)) {
							if(!SFile::Remove(spart)) {
								pParam->callbackProc(BREV_ERRDELPREV, spart.cptr()); // @badcast
								CALLEXCEPT();
							}
						}
					}
				}
				else {
					GetTemporaryFileName(dest, 0, 0);
					replace_src = 1;
				}
				THROW(LoadTableSpec(&newtbl, tbl_name, dest, 0));
				if(CreateDataFile(&newtbl, dest, crmNoReplace, GetRusNCaseACS(acs))) {
					DBField lob_fld;
					int    sp_first = spFirst, sp_next = spNext;
					THROW(newtbl.open(tbl_name, dest));
					if(newtbl.GetLobCount())
						newtbl.GetLobField(newtbl.GetLobCount()-1, &lob_fld);
					else
						lob_fld.Id = 0;
					pParam->callbackProc(BREV_START, path.cptr(), dest.cptr());
					if(tbl.step(sp_first) || BtrError == BE_VLRPAGE) {
						do {
							RECORDSIZE retBufLen = tbl.getRetBufLen();
							THROW(tbl.getPosition(&pos));
							if(lob_fld.Id) {
								newtbl.setDataBuf(tbl.getDataBuf(), fix_rec_size);
								newtbl.setLobSize(lob_fld, (retBufLen > fix_rec_size) ? (retBufLen-fix_rec_size) : 0);
							}
							else
								newtbl.setDataBuf(tbl.getDataBuf(), retBufLen);
							if(newtbl.insertRec()) {
								pParam->ActNumRecs++;
								if(!pParam->callbackProc(BREV_PROGRESS, reinterpret_cast<const void *>(pParam->ActNumRecs), 
									reinterpret_cast<const void *>(pParam->OrgNumRecs), path.cptr())) {
									ok = -1;
									break;
								}
							}
							else {
								if(!pParam->callbackProc(BREV_ERRINS, reinterpret_cast<const void *>((RECORDNUMBER)pos), 
									reinterpret_cast<const void *>(retBufLen), tbl.getDataBufConst())) {
									ok = -1;
									break;
								}
							}
						} while(tbl.step(sp_next) || BtrError == BE_VLRPAGE);
					}
					if(ok >= 0) {
						if(!BTRNFOUND) {
							pParam->callbackProc(BREV_ERRSTEP, reinterpret_cast<const void *>((RECORDNUMBER)pos));
							ok = 0;
						}
						pParam->callbackProc(BREV_FINISH, reinterpret_cast<const void *>(pParam->ActNumRecs), reinterpret_cast<const void *>(pParam->OrgNumRecs));
					}
				}
				else {
					pParam->callbackProc(BREV_ERRCREATE, dest.cptr());
					ok = 0;
				}
			}
		}
		else {
			THROW(BTRNFOUND);
		}
	}
	CATCHZOK
	tbl.setDataBuf(0, 0);
	newtbl.setDataBuf(0, 0);
	if(ok > 0 && replace_src) {
		int    first = 0;
		SString temp_buf;
		tbl.close();
		newtbl.close();
		if(pParam->P_BakPath) {
			for(tpe.Init(path); ok && tpe.Next(spart, &first) > 0;) {
				SFsPath::ReplacePath(temp_buf = spart, pParam->P_BakPath, 1);
				if(!SFile::Rename(spart, temp_buf)) {
					pParam->callbackProc(BREV_ERRRENAME, dest.cptr());
					ok = 0;
				}
			}
			if(ok) {
				for(tpe.Init(dest); tpe.Next(spart, &first) > 0;) {
					const SFsPath sp(spart);
					SFsPath::ReplaceExt(temp_buf = path, sp.Ext, 1);
					SFile::Rename(spart, temp_buf);
				}
			}
		}
		else {
			int    renm = 1;
			STRNSCPY(buf, path);
			SFsPath::ReplaceExt(path, p_bak_ext, 1);
			for(tpe.Init(path); tpe.Next(spart, &first) > 0;) {
				tpe.ReplaceExt(first, spart, temp_buf);
				if(fileExists(spart)) {
					if(!SFile::Remove(spart)) {
						SFile::Remove(temp_buf);
						renm = 0;
					}
				}
				if(renm)
					SFile::Rename(temp_buf, spart);
			}
			for(tpe.Init(dest); tpe.Next(spart, &first) > 0;) {
				if(first)
					temp_buf = buf;
				else {
					SFsPath sp(buf);
					sp.Merge(~SFsPath::fExt, temp_buf);
					sp.Split(spart);
					temp_buf.Dot().Cat(sp.Ext);
				}
				SFile::Rename(spart, temp_buf);
			}
		}
	}
	return ok;
}
//
//
//
TablePartsEnum::TablePartsEnum(const char * pPath)
{
	Init(pPath);
}

int TablePartsEnum::Init(const char * pPath)
{
	int    ok = -1;
	if(pPath) {
		SString path;
		SFsPath sp(pPath);
		sp.Ext = (sp.Ext.Cmp("___", 0) == 0) ? "_??" : "^??";
		sp.Merge(path);
		sp.Merge(0, SFsPath::fNam|SFsPath::fExt, Dir);
		//
		SDirec direc(path);
		MainPart = pPath;
		//
		long   i = 0;
		List.Z().setPointer(0);
		List.Add(++i, MainPart);
		SDirEntry fb;
		while(direc.Next(&fb) > 0) {
			if(!(fb.Attr & 0x10)) {
				fb.GetNameA(Dir, path);
				List.Add(++i, path);
			}
		}
		//
		ok = 1;
	}
	return ok;
}

int TablePartsEnum::Next(SString & rPath, int * pFirst /*=0*/)
{
	int    ok = -1;
	if(List.getPointer() < List.getCount()) {
		rPath = List.Get(List.getPointer()).Txt;
		ASSIGN_PTR(pFirst, List.getPointer() == 0);
		List.incPointer();
		ok = 1;
	}
	return ok;
}

int TablePartsEnum::ReplaceExt(int first, const SString & rIn, SString & rOut)
{
	SString ext;
	SFsPath sp(rIn);
	const bool to_save = (sp.Ext.C(0) != '_');
	ext = sp.Ext;
	sp.Ext.Z();
	if(first)
		ext = to_save ? "___" : "btr";
	else {
		ext.ShiftLeft();
		sp.Ext.CatChar(to_save ? '_' : '^');
	}
	sp.Ext.Cat(ext);
	sp.Merge(rOut);
	return 1;
}
//
//
//
DBTablePartitionList::_InnerEntry::_InnerEntry() : Id(0), Flags(0), P(0)
{
}

DBTablePartitionList::DBTablePartitionList()
	{ Init(0, 0, 0); }
DBTablePartitionList::DBTablePartitionList(const char * pPath, const char * pFileName, long options)
	{ Init(pPath, pFileName, options); }

int DBTablePartitionList::Init(const char * pPath, const char * pFileName, long options)
{
	int    ok = -1;
	InitPathP = 0;
	InitNameP = 0;
	Pool.Z().add("$"); // zero index - is empty string
	List.clear();
	if(pPath) {
		SString temp_buf;
		SString path;
		SString name; // Имя файла без расширения //
		SFsPath sp_p(pPath);
		SFsPath sp_n(pFileName);
		if(sp_n.Nam.NotEmpty())
			name = sp_n.Nam;
		else if(sp_p.Nam.NotEmpty())
			name = sp_p.Nam;
		if(name.NotEmpty()) {
			Entry test_entry;
			sp_p.Merge(SFsPath::fDrv|SFsPath::fDir, path);
			Pool.add(path, &InitPathP);
			Pool.add(name, &InitNameP);

			sp_p.Nam = name;
			sp_p.Ext = "*";
			sp_p.Merge(temp_buf);
			SDirEntry fb;
			long   counter = 0;
			for(SDirec direc(temp_buf); direc.Next(&fb) > 0;) {
				if(!(fb.Attr & 0x10)) {
					fb.GetNameA(path, temp_buf);
					sp_n.Split(temp_buf.Strip());
					_InnerEntry entry;
					if(sp_n.Ext == "^^^") {
						assert(GetConEntry(test_entry) == 0);
						entry.Id = ++counter;
						entry.Flags |= fCon;
						Pool.add(temp_buf, &entry.P);
						List.insert(&entry);
					}
					else if(sp_n.Ext.Len() == 3 && sp_n.Ext.C(0) == '^') {
						entry.Id = ++counter;
						entry.Flags |= fExt;
						Pool.add(temp_buf, &entry.P);
						List.insert(&entry);
					}
					else if(sp_n.Ext.IsEqiAscii("btr")) {
						assert(GetMainEntry(test_entry) == 0);
						entry.Id = ++counter;
						entry.Flags |= fMain;
						Pool.add(temp_buf, &entry.P);
						List.insert(&entry);
						ok = 1;
					}
					else if(sp_n.Ext == "___") {
						assert(GetMainBuEntry(test_entry) == 0);
						entry.Id = ++counter;
						entry.Flags |= (fBu|fZip|fMain);
						Pool.add(temp_buf, &entry.P);
						List.insert(&entry);
					}
					else if(sp_n.Ext.Len() == 3 && sp_n.Ext.C(0) == '_') {
						entry.Id = ++counter;
						entry.Flags |= (fBu|fZip|fExt);
						Pool.add(temp_buf, &entry.P);
						List.insert(&entry);
					}
					else {
						//
						// Неизвестный тип файла, хотя и имеет имя, совпадающее с основным именем файла
						//
						entry.Id = ++counter;
						entry.Flags = 0;
						Pool.add(temp_buf, &entry.P);
						List.insert(&entry);
					}
				}
			}
			ok = 1;
		}
		else
			ok = 0; // Unable identify filename without extention
	}
	return ok;
}

int DBTablePartitionList::GetInitPath(SString & rBuf) const
{
	Pool.getnz(InitPathP, rBuf.Z());
	return BIN(rBuf.NotEmpty());
}

uint DBTablePartitionList::GetCount() const
{
	return List.getCount();
}

int DBTablePartitionList::Get(uint p, Entry & rEntry) const
{
	int    ok = 1;
	if(p < List.getCount()) {
		const _InnerEntry & r_entry = List.at(p);
		rEntry.Id = r_entry.Id;
		rEntry.Flags = r_entry.Flags;
		Pool.getnz(r_entry.P, rEntry.Path);
	}
	else {
		rEntry.Id = 0;
		rEntry.Flags = 0;
		rEntry.Path.Z();
		ok = 0;
	}
	return ok;
}

int DBTablePartitionList::Helper_GetEntry(long andF, long notF, Entry & rEntry) const
{
	int    ok = 0;
	for(uint i = 0; !ok && i < List.getCount(); i++) {
		const _InnerEntry & r_entry = List.at(i);
		if((andF && (r_entry.Flags & andF) == andF) && (notF && !(r_entry.Flags & notF))) {
			rEntry.Id = r_entry.Id;
			rEntry.Flags = r_entry.Flags;
			Pool.getnz(r_entry.P, rEntry.Path);
			ok = 1;
		}
	}
	return ok;
}

int DBTablePartitionList::GetMainEntry(Entry & rEntry) const
	{ return Helper_GetEntry(fMain, fBu, rEntry); }
int DBTablePartitionList::GetMainBuEntry(Entry & rEntry) const
	{ return Helper_GetEntry(fMain|fBu, 0, rEntry); }
int DBTablePartitionList::GetConEntry(Entry & rEntry) const
	{ return Helper_GetEntry(fCon, 0, rEntry); }
