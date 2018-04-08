// DBDICT.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2017, 2018
//
#include <db.h>
#pragma hdrstop
// @v9.6.3 #include <idea.h>
//
//
//
//static
DbDictionary * (*DbDictionary::CreateInstanceProc)(const char * pPath, long options) = 0;

DbTableStat::DbTableStat()
{
	Clear();
}

void DbTableStat::Clear()
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
	FldList.reset();
	IdxList.reset();
}

SLAPI DbDictionary::DbDictionary()
{
	State = 0;
}

SLAPI DbDictionary::~DbDictionary()
{
}

int SLAPI DbDictionary::IsValid() const
{
	return BIN(!(State & stError));
}

//static
void DbDictionary::SetCreateInstanceProc(DbDictionary * (*proc)(const char * pPath, long options))
{
	ENTER_CRITICAL_SECTION
	CreateInstanceProc = proc;
	LEAVE_CRITICAL_SECTION
}

//static
DbDictionary * DbDictionary::CreateInstance(const char * pPath, long options)
{
	return CreateInstanceProc ? CreateInstanceProc(pPath, options) : new DbDict_Btrieve(pPath);
}
//
//
// static
const char * BDictionary::DdfTableFileName = "FILE.DDF";
const char * BDictionary::DdfFieldFileName = "FIELD.DDF";
const char * BDictionary::DdfIndexFileName = "INDEX.DDF";
//
// Implementation of API functions
//
void SLAPI DBRemoveTempFiles()
{
	CALLPTRMEMB(CurDict, RemoveTempFiles());
}
//
// Implementation of class BDictionary
//
//static
BDictionary * SLAPI BDictionary::CreateBtrDictInstance(const char * pPath)
{
	return new BDictionary(1, pPath);
}

int SLAPI BDictionary::Init(const char * pDataPath, const char * pTempPath)
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

SLAPI BDictionary::BDictionary(const char * pPath, const char * pDataPath, const char * pTempPath) : DbProvider(DbDictionary::CreateInstance(pPath, 0), 0)
{
	Init(pDataPath, pTempPath);
}

SLAPI BDictionary::BDictionary(int btrDict, const char * pPath) : DbProvider(new DbDict_Btrieve(pPath), 0)
{
	Init(0, 0);
}

//virtual 
int SLAPI BDictionary::GetDatabaseState(uint * pStateFlags)
{
	int    ok = 1;
	uint   state = 0;
	if(DataPath.NotEmpty()) {
		SDirEntry de;
		SString wc;
		(wc = DataPath).SetLastSlash().CatChar('*').Dot().Cat("^^^");
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

//virtual
SString & SLAPI BDictionary::MakeFileName_(const char * pTblName, SString & rFileName)
{
	SPathStruc ps(rFileName);
	if(ps.Dir.Empty()) {
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
						tbl_path.TrimRightChr('\xA').TrimRightChr('\xD').Strip().SetLastSlash();
						SPathStruc rps(tbl_path);
						rps.Merge(&ps, SPathStruc::fNam|SPathStruc::fExt, rFileName);
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

/*virtual*/ int SLAPI BDictionary::CreateDataFile(const DBTable * pTbl, const char * pFileName, int createMode, const char * pAltCode)
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
			int16  offs = (int16)(f.Offs + 1);
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
					r_idx.position  = offs + (int16)sizeof(LDATE);
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
				r_idx.length    = (int16)stsize(f.T);
				r_idx.flags     = k.getFlags(j);
				r_idx.extType   = SLib2BtrType(GETSTYPE(f.T));
				r_idx.keyNumber = k.getKeyNumber();
				r_idx.acsNumber = k.getACSNumber();
				p_h = &((*p_h) + r_idx);
			}
		}
	}
	SString b = NZOR(pFileName, pTbl->fileName);
	MakeFileName_(pTbl->tableName, b);
	if(!Btrieve::CreateTable(b, *p_h, RESET_CRM_TEMP(createMode), pAltCode))
		ok = 0;
	else {
		//
		// В Windows 7 от момента успешного создания файла, до фактического
		// его появления может проийти некоторое время. По этому, формируем
		// задержку для ожидания появления созданного файла.
		//
		while(!IsFileExists_(b)) { // @v7.9.9 fileExists-->BDictionary::IsFileExists
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

/*virtual*/ int SLAPI BDictionary::IsFileExists_(const char * pFileName)
{
	int    yes = 0;
	if(DBS.GetConfig().Flags & DbSession::fDetectExistByOpen) {
		char   fpb[256];
		uint16 bl = 0;
		SString file_name;
		if(strchr(pFileName, ' '))
			file_name.Z().CatQStr(pFileName);
		else
			file_name = pFileName;
		char   temp_buf[512];
		file_name.CopyTo(temp_buf, sizeof(temp_buf));
		int    ret = BTRV(B_OPEN, fpb, 0 /*pPassword*/, &bl, temp_buf, 0, omReadOnly);
		if(ret == 0) {
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

/*virtual*/ int SLAPI BDictionary::DropFile(const char * pFileName)
	{ return (IsFileExists_(pFileName) > 0) ? SFile::Remove(pFileName) : -1; }
/*virtual*/ int SLAPI BDictionary::GetFileStat(DBTable * pTbl, long reqItems, DbTableStat * pStat)
	{ return pTbl->Btr_GetStat(reqItems, pStat); }

/*virtual*/ int SLAPI BDictionary::Login(const DbLoginBlock * pBlk, long options)
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

/*virtual*/ int SLAPI BDictionary::Logout()
{
	Btrieve::Reset(0);
	Common_Logout();
	return 1;
}

/*virtual*/ int SLAPI BDictionary::ProtectTable(long dbTableID, char * pResetOwnrName, char * pSetOwnrName, int clearProtection)
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

/*virtual*/ int SLAPI BDictionary::StartTransaction()
{
	return Btrieve::StartTransaction(1, 0);
}

/*virtual*/ int SLAPI BDictionary::CommitWork()
	{ return Btrieve::CommitWork(); }
/*virtual*/ int SLAPI BDictionary::RollbackWork()
	{ return Btrieve::RollbackWork(); }
/*virtual*/ int SLAPI BDictionary::Implement_Open(DBTable * pTbl, const char * pFileName, int openMode, char * pPassword)
	{ return pTbl->Btr_Open(pFileName, openMode, pPassword); }
/*virtual*/ int SLAPI BDictionary::Implement_Close(DBTable * pTbl)
	{ return pTbl->Btr_Close(); }
/*virtual*/ int SLAPI BDictionary::Implement_Search(DBTable * pTbl, int idx, void * pKey, int srchMode, long sf)
	{ return pTbl->Btr_Implement_Search(idx, pKey, srchMode, sf); }
/*virtual*/ int SLAPI BDictionary::Implement_InsertRec(DBTable * pTbl, int idx, void * pKeyBuf, const void * pData)
	{ return pTbl->Btr_Implement_InsertRec(idx, pKeyBuf, pData); }
/*virtual*/ int SLAPI BDictionary::Implement_UpdateRec(DBTable * pTbl, const void * pDataBuf, int ncc)
	{ return pTbl->Btr_Implement_UpdateRec(pDataBuf, ncc); }
/*virtual*/ int SLAPI BDictionary::Implement_DeleteRec(DBTable * pTbl)
	{ return pTbl->Btr_Implement_DeleteRec(); }
/*virtual*/ int SLAPI BDictionary::Implement_BExtInsert(BExtInsert * pBei)
	{ return pBei->getTable()->Btr_Implement_BExtInsert(pBei); }
/*virtual*/ int SLAPI BDictionary::Implement_GetPosition(DBTable * pTbl, DBRowId * pPos)
	{ return pTbl->Btr_Implement_GetPosition(pPos); }

SLAPI BDictionary::~BDictionary()
{
	RemoveTempFiles();
}
//
// Процедура создания временной таблицы
//
/*virtual*/ SString & SLAPI BDictionary::GetTempFileName(SString & rFileNameBuf, long * pStart, int forceInDataPath)
{
	const char * p_path = (TempPath && !forceInDataPath) ? TempPath : DataPath;
	return MakeTempFileName(p_path, "TMP", "BTR", pStart, rFileNameBuf);
}
//
//
//
SLAPI BRecoverParam::BRecoverParam() : P_DestPath(0), P_BakPath(0), OrgNumRecs(0), ActNumRecs(0), ErrCode(0), Tm(0)
{
}

int SLAPI BRecoverParam::callbackProc(int, long, long, void *)
{
	return -1;
}

#define BRPF_REPLACE   0x0001
#define BRPF_PROTECT   0x0002
#define BRPF_SKIPDUP   0x0004

int SLAPI BDictionary::RecoverTable(BTBLID tblID, BRecoverParam * pParam)
{
	int    ok = 1, replace_src = 0;
	int16  k  = tblID;
	DBRowId pos;
	char   acs[265];
	char   buf[MAXPATH];
	STempBuffer rec_buf(8192);
	long   s_bak_ext = 0x5F5F5FL;
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
				tbl.setDataBuf(rec_buf, (RECORDSIZE)rec_buf.GetSize());
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
								pParam->callbackProc(BREV_ERRDELPREV, (long)(const char*)spart);
								CALLEXCEPT();
							}
						}
					}
				}
				else {
					GetTempFileName(dest, 0, 0);
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
					pParam->callbackProc(BREV_START, (long)path.cptr(), (long)dest.cptr());
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
								if(!pParam->callbackProc(BREV_PROGRESS, ++pParam->ActNumRecs, pParam->OrgNumRecs, (void *)path.cptr())) {
									ok = -1;
									break;
								}
							}
							else {
								if(!pParam->callbackProc(BREV_ERRINS, pos, retBufLen, tbl.getDataBuf())) {
									ok = -1;
									break;
								}
							}
						} while(tbl.step(sp_next) || BtrError == BE_VLRPAGE);
					}
					if(ok >= 0) {
						if(!BTRNFOUND) {
							pParam->callbackProc(BREV_ERRSTEP, pos);
							ok = 0;
						}
						pParam->callbackProc(BREV_FINISH, pParam->ActNumRecs, pParam->OrgNumRecs);
					}
				}
				else {
					pParam->callbackProc(BREV_ERRCREATE, (long)dest.cptr());
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
				SPathStruc::ReplacePath(temp_buf = spart, pParam->P_BakPath, 1);
				if(!SFile::Rename(spart, temp_buf)) {
					pParam->callbackProc(BREV_ERRRENAME, (long)dest.cptr());
					ok = 0;
				}
			}
			if(ok) {
				for(tpe.Init(dest); tpe.Next(spart, &first) > 0;) {
					SPathStruc sp(spart);
					SPathStruc::ReplaceExt(temp_buf = path, sp.Ext, 1);
					SFile::Rename(spart, temp_buf);
				}
			}
		}
		else {
			int    renm = 1;
			STRNSCPY(buf, path);
			SPathStruc::ReplaceExt(path, (const char *)&s_bak_ext, 1);
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
					SPathStruc sp(buf);
					sp.Merge(~SPathStruc::fExt, temp_buf);
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
SLAPI TablePartsEnum::TablePartsEnum(const char * pPath)
{
	Init(pPath);
}

int SLAPI TablePartsEnum::Init(const char * pPath)
{
	int    ok = -1;
	if(pPath) {
		SString path;
		SPathStruc sp(pPath);
		sp.Ext = (sp.Ext.Cmp("___", 0) == 0) ? "_??" : "^??";
		sp.Merge(path);
		sp.Merge(0, SPathStruc::fNam|SPathStruc::fExt, Dir);
		//
		SDirec direc(path);
		MainPart = pPath;
		//
		long   i = 0;
		List.Clear();
		List.setPointer(0);
		List.Add(++i, MainPart);
		SDirEntry fb;
		while(direc.Next(&fb) > 0)
			if(!(fb.Attr & 0x10))
				List.Add(++i, (path = Dir).SetLastSlash().Cat(fb.FileName));
		//
		ok = 1;
	}
	return ok;
}

int SLAPI TablePartsEnum::Next(SString & rPath, int * pFirst /*=0*/)
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

int SLAPI TablePartsEnum::ReplaceExt(int first, const SString & rIn, SString & rOut)
{
	SString ext;
	SPathStruc sp(rIn);
	int    to_save = BIN(sp.Ext.C(0) != '_');
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
DBTablePartitionList::DBTablePartitionList()
{
	Init(0, 0, 0);
}

DBTablePartitionList::DBTablePartitionList(const char * pPath, const char * pFileName, long options)
{
	Init(pPath, pFileName, options);
}

int DBTablePartitionList::Init(const char * pPath, const char * pFileName, long options)
{
	int    ok = -1;
	InitPathP = 0;
	InitNameP = 0;
	Pool.clear();
	Pool.add("$"); // zero index - is empty string
	List.clear();
	if(pPath) {
		SString temp_buf;
		SString path;
		SString name; // Имя файла без расширения //
		SPathStruc sp_p(pPath);
		SPathStruc sp_n(pFileName);
		if(sp_n.Nam.NotEmpty())
			name = sp_n.Nam;
		else if(sp_p.Nam.NotEmpty())
			name = sp_p.Nam;
		if(name.NotEmpty()) {
			Entry test_entry;
			sp_p.Merge(SPathStruc::fDrv|SPathStruc::fDir, path);
			Pool.add(path, &InitPathP);
			Pool.add(name, &InitNameP);

			sp_p.Nam = name;
			sp_p.Ext = "*";
			sp_p.Merge(temp_buf);

			SDirEntry fb;
			long   counter = 0;
			for(SDirec direc(temp_buf); direc.Next(&fb) > 0;) {
				if(!(fb.Attr & 0x10)) {
					sp_n.Split((temp_buf = path).SetLastSlash().Cat(fb.FileName).Strip());
					_InnerEntry entry;
					MEMSZERO(entry);
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
					else if(sp_n.Ext.CmpNC("btr") == 0) {
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
		rEntry.Path = 0;
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
