// DBPRVDR.CPP
// Copyright (c) A.Sobolev 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

/*enum SqlServerType {
	sqlstNone    = 0, // Неопределенное значение
	sqlstGeneric = 1, // Общий
	sqlstORA,         // Oracle
	sqlstMSS,         // Ms SQL Server
	sqlstFB,          // FireBird
	sqlstPg,          // PostgreSQL
	sqlstMySQL,       // @v10.9.0 MySQL
	sqlstSQLite       // @v10.9.0 SQLite
};*/

static const SIntToSymbTabEntry SqlServerTypeSymbList[] = {
	{ sqlstNone, "" },
	{ sqlstGeneric, "generic" },
	{ sqlstORA, "oracle" },
	{ sqlstMSS, "mssql" },
	{ sqlstFB, "firebird" },
	{ sqlstPg, "pg" },
	{ sqlstMySQL, "mysql" },
	{ sqlstSQLite, "sqlite" },
};

int GetSqlServerTypeSymb(SqlServerType t, SString & rBuf)
{
	return SIntToSymbTab_GetSymb(SqlServerTypeSymbList, SIZEOFARRAY(SqlServerTypeSymbList), t, rBuf);
}

SqlServerType GetSqlServerTypeBySymb(const char * pSymb)
{
	return static_cast<SqlServerType>(SIntToSymbTab_GetId(SqlServerTypeSymbList, SIZEOFARRAY(SqlServerTypeSymbList), pSymb));
}

DbLoginBlock::DbLoginBlock() : End(0)
{
	SBaseBuffer::Init();
}

DbLoginBlock::DbLoginBlock(const DbLoginBlock & rS) : End(0)
{
	SBaseBuffer::Init();
	Copy(rS);
}

DbLoginBlock::~DbLoginBlock()
{
	SBaseBuffer::Destroy();
}

DbLoginBlock & FASTCALL DbLoginBlock::operator = (const DbLoginBlock & rS)
{
	Copy(rS);
	return *this;
}

DbLoginBlock & DbLoginBlock::Z()
{
	End = 0;
	Items.clear();
	return *this;
}

int FASTCALL DbLoginBlock::Copy(const DbLoginBlock & rS)
{
	if(SBaseBuffer::Copy(rS)) {
		Items = rS.Items;
		End = rS.End;
		return 1;
	}
	else
		return 0;
}

#define ATTR_PW_LEN 128

int DbLoginBlock::GetAttr(int attr, SString & rVal) const
{
	int    ok = -1;
	long   offs = 0;
	rVal.Z();
	if(Items.Search(attr, &offs, 0)) {
		const void * p_val_buf = (P_Buf+offs+sizeof(uint32));
		uint32 len = *reinterpret_cast<const uint32 *>(P_Buf+offs);
		char   temp_buf[ATTR_PW_LEN*2];
		if(attr == attrPassword) {
			assert(len == ATTR_PW_LEN);
			memcpy(temp_buf, p_val_buf, len);
			IdeaDecrypt(0, temp_buf, len);
			rVal = temp_buf;
		}
		else if(attr == attrDbUuid) {
			assert(len == sizeof(S_GUID));
			const S_GUID * p_uuid = static_cast<const S_GUID *>(p_val_buf);
			THROW(p_uuid->ToStr(S_GUID::fmtIDL, rVal));
		}
		else
			rVal = static_cast<const char *>(p_val_buf);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int DbLoginBlock::SetAttr(int attr, const char * pVal)
{
	int    ok = 1;
	Items.Remove(attr);
	if(pVal) {
		const void * p_val_buf = pVal;
		uint32 len = 0;
		char   temp_buf[ATTR_PW_LEN*2];
		S_GUID uuid;
		if(attr == attrPassword) {
			len = ATTR_PW_LEN;
			// @v11.1.1 IdeaRandMem(temp_buf, len);
			SObfuscateBuffer(temp_buf, len); // @v11.1.1
			strnzcpy(temp_buf, pVal, len);
			IdeaEncrypt(0, temp_buf, len);
			p_val_buf = temp_buf;
		}
		else if(attr == attrDbUuid) {
			len = sizeof(S_GUID);
			THROW(uuid.FromStr(pVal));
			p_val_buf = &uuid;
		}
		else {
			len = (uint32)(sstrlen(pVal)+1);
			p_val_buf = pVal;
		}
		THROW(Alloc(End + len + sizeof(len)));
		THROW(Items.Add(attr, (long)End, 0));
		memcpy(P_Buf+End, &len, sizeof(len));
		memcpy(P_Buf+End+sizeof(len), p_val_buf, len);
		End += (len+sizeof(len));
	}
	CATCHZOK
	return ok;
}

DbLoginBlockArray::DbLoginBlockArray() : TSCollection <DbLoginBlock>(), LastId(0), SelId(0)
{
}

void DbLoginBlockArray::Clear()
{
	freeAll();
}

int DbLoginBlockArray::Add(long id, const DbLoginBlock * pBlk, int replaceDup)
{
	int    ok = 1;
	uint   i;
	if(id) {
		SETMAX(LastId, id);
	}
	else
		id = ++LastId;
	SString temp_buf, symb;
	LongArray to_remove_list;
	DbLoginBlock * p_blk = new DbLoginBlock;
	THROW_S(p_blk, SLERR_NOMEM);
	*p_blk = *pBlk;
	p_blk->SetAttr(DbLoginBlock::attrID, temp_buf.Z().Cat(id));
	//
	// Проверяем уникальность атрибутов ID и DbSymb. Эти атрибуты являются ключевыми
	// при поиске, поэтому они не должны дублироваться у разных элементов массива.
	//

	//
	// Атрибут символа базы данных обязательно должен присутствовать в блоке, иначе
	// мы не можем этот блок вставить в массив.
	//
	THROW(p_blk->GetAttr(DbLoginBlock::attrDbSymb, temp_buf) > 0 && temp_buf.NotEmptyS()); // @todo(err)
	for(i = 0; ok == 1 && i < getCount(); i++) {
		if(at(i)->GetAttr(DbLoginBlock::attrDbSymb, symb) > 0) {
			if(temp_buf.CmpNC(symb) == 0) {
				if(replaceDup)
					to_remove_list.addUnique(static_cast<long>(i));
				else
					ok = 2;
			}
		}
		if(at(i)->GetAttr(DbLoginBlock::attrID, symb) > 0) {
			if(id == symb.ToLong())
				if(replaceDup)
					to_remove_list.addUnique(static_cast<long>(i));
				else
					ok = 3;
		}
	}
	if(ok == 1) {
		//
		// Учитывая порядок формирования списка to_remove_list
		// он отсортирован в возрастающем порядке. Следовательно,
		// мы должны удалять элементы, начиная с последнего индекса
		// и до первого включительно.
		//
		i = to_remove_list.getCount();
		if(i) do {
			atFree(to_remove_list.get(--i));
		} while(i);
		THROW(insert(p_blk));
		p_blk = 0; // Дабы не удалить вставленный блок в конце функции
	}
	CATCHZOK
	delete p_blk;
	return ok;
}

uint DbLoginBlockArray::GetCount() const
{
	return SCollection::getCount();
}

int DbLoginBlockArray::GetByID(long id, DbLoginBlock * pBlk) const
{
	int    ok = 0;
	SString symb;
	CALLPTRMEMB(pBlk, Z());
	//
	if(id > 0 && id <= static_cast<long>(getCount())) {
		const DbLoginBlock * p_blk = at(id-1);
		ASSIGN_PTR(pBlk, *p_blk);
		ok = (int)id;
	}
	/*
	for(uint i = 0; !ok && i < getCount(); i++) {
		const DbLoginBlock * p_blk = at(i);
		if(p_blk->GetAttr(DbLoginBlock::attrID, symb) > 0 && symb.ToLong() == id) {
			ASSIGN_PTR(pBlk, *p_blk);
			ok = (int)(i+1);
		}
	}
	*/
	return ok;
}

int DbLoginBlockArray::GetBySymb(const char * pSymb, DbLoginBlock * pBlk) const
{
	int    ok = 0;
	SString symb;
	CALLPTRMEMB(pBlk, Z());
	for(uint i = 0; !ok && i < getCount(); i++) {
		const DbLoginBlock * p_blk = at(i);
		if(p_blk->GetAttr(DbLoginBlock::attrDbSymb, symb) > 0 && symb.CmpNC(pSymb) == 0) {
			ASSIGN_PTR(pBlk, *p_blk);
			ok = static_cast<int>(i+1);
		}
	}
	if(!ok)
		SLS.SetError(SLERR_INVDBSYMB, pSymb);
	return ok;
}

int DbLoginBlockArray::GetByPos(uint pos, DbLoginBlock * pBlk) const
{
	int    ok = 0;
	CALLPTRMEMB(pBlk, Z());
	if(pos < getCount()) {
		ASSIGN_PTR(pBlk, *at(pos));
		ok = (pos+1);
	}
	return ok;
}

int DbLoginBlockArray::GetAttr(const char * pDbSymb, int attr, SString & rVal) const
{
	int    ok = 0;
	rVal.Z();
	SString symb;
	for(uint i = 0; i < getCount(); i++) {
		const DbLoginBlock * p_blk = at(i);
		if(p_blk->GetAttr(DbLoginBlock::attrDbSymb, symb) > 0 && symb.CmpNC(pDbSymb) == 0) {
			ok = p_blk->GetAttr(attr, rVal);
			break;
		}
	}
	return ok;
}

int DbLoginBlockArray::GetAttr(long id, int attr, SString & rVal) const
{
	int    ok = 0;
	rVal.Z();
	//
	if(id > 0 && id <= static_cast<long>(getCount())) {
		const DbLoginBlock * p_blk = at(id-1);
		ok = p_blk->GetAttr(attr, rVal);
	}
	/*
	SString symb;
	for(uint i = 0; i < getCount(); i++) {
		const DbLoginBlock * p_blk = at(i);
		if(p_blk->GetAttr(DbLoginBlock::attrID, symb) > 0 && symb.ToLong() == id) {
			ok = p_blk->GetAttr(attr, rVal);
			break;
		}
	}
	*/
	return ok;
}

long DbLoginBlockArray::SetSelection(long id)
{
	if(id >= 0 && id <= static_cast<long>(getCount()))
		SelId = id;
	return SelId;
}

int DbLoginBlockArray::MakeList(StrAssocArray * pList, long options, const LongArray * pDbesIdxList) const
{
	int    ok = -1;
	SString temp_buf;
	for(uint i = 0; i < getCount(); i++) {
		if(!pDbesIdxList || pDbesIdxList->lsearch(i+1)) {
			const DbLoginBlock * p_blk = at(i);
			p_blk->GetAttr(DbLoginBlock::attrID, temp_buf);
			const long id = static_cast<long>(i+1); // temp_buf.ToLong();
			temp_buf.Z();
			if(options & loUseFriendlyName)
				p_blk->GetAttr(DbLoginBlock::attrDbFriendlyName, temp_buf);
			if(!temp_buf.NotEmptyS() && options & loUseDbSymb)
				p_blk->GetAttr(DbLoginBlock::attrDbSymb, temp_buf);
			if(!temp_buf.NotEmptyS() && options & loUseDbPath)
				p_blk->GetAttr(DbLoginBlock::attrDbPath, temp_buf);
			if(temp_buf.NotEmptyS() && id != 0) {
				CALLPTRMEMB(pList, Add(id, temp_buf));
				ok = 1;
			}
		}
	}
	return ok;
}
//
//
//
DbProvider::DbProvider(DbDictionary * pDict, long capability) : P_Dict(pDict), Capability(capability), State(0), DbPathID(0)
{
}

DbProvider::~DbProvider()
{
	delete P_Dict;
}

//virtual default-implementation
int DbProvider::GetDatabaseState(uint * pStateFlags)
{
	ASSIGN_PTR(pStateFlags, 0);
	return -1;
}

bool DbProvider::IsValid() const { return !(State & stError); }

int DbProvider::SetDbName(const char * pName, const char * pSymb)
{
	DbName = pName;
	DbSymb = pSymb;
	return 1;
}

int DbProvider::GetDbName(SString & rBuf) const
{
	Lb.GetAttr(DbLoginBlock::attrDbFriendlyName, rBuf);
	return rBuf.NotEmpty() ? 1 : -1;
}

bool DbProvider::GetDbSymb(SString & rBuf) const
{
	Lb.GetAttr(DbLoginBlock::attrDbSymb, rBuf);
	return rBuf.NotEmpty();
}

int DbProvider::GetDataPath(SString & rBuf) const
{
	Lb.GetAttr(DbLoginBlock::attrDbPath, rBuf);
	return rBuf.NotEmptyS();
}

int DbProvider::GetSysPath(SString & rBuf) const
{
	Lb.GetAttr(DbLoginBlock::attrDictPath, rBuf);
	return rBuf.NotEmptyS();
}

int DbProvider::GetDbUUID(S_GUID * pUuid) const
{
	int    ok = 0;
	SString temp_buf;
	Lb.GetAttr(DbLoginBlock::attrDbUuid, temp_buf);
	if(pUuid)
		ok = pUuid->FromStr(temp_buf);
	return ok;
}

int DbProvider::AddTempFileName(const char * pFileName)
{
	return isempty(pFileName) ? -1 : TempFileList.add(pFileName);
}

int DbProvider::DelTempFileName(const char * pFileName)
{
	int    ok = -1;
	SString file_name;
	for(uint i = 0; TempFileList.get(&i, file_name);) {
		if(file_name.CmpNC(pFileName) == 0)
			ok = 1;
	}
	if(ok > 0) {
		StringSet temp_list;
		for(uint i = 0; TempFileList.get(&i, file_name);) {
			if(file_name.CmpNC(pFileName) != 0)
				temp_list.add(file_name);
		}
		TempFileList = temp_list;
	}
	return ok;

}

void DbProvider::RemoveTempFiles()
{
	SString file_name;
	StringSet temp_list;
	for(uint i = 0; TempFileList.get(&i, file_name);) {
		int    opened = 0;
		const  int ec = DBS.GetTLA().GetTabEntriesCount();
		for(int j = 1; !opened && j <= ec; j++) {
			DBTable * p_table = _GetTable(j);
			if(p_table && file_name.CmpNC(p_table->GetName()) == 0)
				opened = 1;
		}
		if(opened || DropFile(file_name) != 0)
			temp_list.add(file_name);
	}
	TempFileList = temp_list;
}

int DbProvider::LoadTableSpec(DBTable * pTbl, const char * pTblName, const char * pFileName, int createIfNExists)
{
	int    ok = 1;
	THROW(P_Dict->LoadTableSpec(pTbl, pTblName));
	{
		SString tbl_loc;
		if(pFileName)
			tbl_loc = pFileName;
		else if(Capability & cSQL)
			tbl_loc = pTblName;
		else
			tbl_loc = pTbl->GetName();
		pTbl->fileName = MakeFileName_(pTblName, tbl_loc);
	}
	if(createIfNExists && !IsFileExists_(pTbl->GetName())) {
		char   acs[265];
		THROW(CreateDataFile(pTbl, pTbl->GetName(), crmNoReplace, GetRusNCaseACS(acs)));
	}
	CATCH
		pTbl->tableID = 0;
		ok = 0;
	ENDCATCH
	return ok;
}

int DbProvider::CreateTableAndFileBySpec(DBTable ** ppTblSpec)
{
	int    ok = 1;
	assert(ppTblSpec);
	assert(*ppTblSpec);
	DBTable * p_tbl = *ppTblSpec;
	DBTable * p_new_tbl = 0;
	SString file_name;
	SString tbl_name(p_tbl->GetTableName());
	DbTableStat ts;
	P_Dict->DropTableSpec(p_tbl->GetTableName(), &ts);
	THROW(P_Dict->CreateTableSpec(p_tbl));
	file_name = p_tbl->GetName();
	ZDELETE(p_tbl);
	DropFile(file_name);
	THROW(p_new_tbl = new DBTable(tbl_name, file_name, omNormal, this));
	p_new_tbl->AllocateOwnBuffer(-1);
	CATCH
		ZDELETE(p_new_tbl);
		ok = 0;
	ENDCATCH
	*ppTblSpec = p_new_tbl;
	return ok;
}

int DbProvider::CreateTempFile(const char * pTblName, SString & rFileNameBuf, int forceInDataPath)
{
	int    ok = 0;
	DBTable crtbl;
	long   _start = SLS.GetTLA().Rg.GetUniformInt(100000);
	do {
		GetTemporaryFileName(rFileNameBuf, &_start, forceInDataPath);
	} while(IsFileExists_(rFileNameBuf) > 0);
	if(LoadTableSpec(&crtbl, pTblName, rFileNameBuf, 0)) {
		char   acs[265];
		if(CreateDataFile(&crtbl, rFileNameBuf, SET_CRM_TEMP(crmNoReplace), GetRusNCaseACS(acs))) {
			ok = 1;
		}
	}
	return ok;
}

int DbProvider::RenewFile(DBTable & rTbl, int createMode, const char * pAltCode)
{
	int    ok = 1;
	char   acst[512];
	SString tbl_fname = rTbl.GetName();
	SString tbl_name(rTbl.GetTableName());
	rTbl.close();
	THROW(DropFile(tbl_fname));
	THROW(LoadTableSpec(&rTbl, tbl_name, tbl_fname, 0));
	THROW(CreateDataFile(&rTbl, tbl_fname, createMode, NZOR(pAltCode, GetRusNCaseACS(acst))));
	THROW(rTbl.open(tbl_name));
	CATCHZOK
	return ok;
}

int DbProvider::DropTable(const char * pTblName, int inDictOnly)
{
	DbTableStat ts;
	int    ok = P_Dict->DropTableSpec(pTblName, &ts);
	if(ok) {
		if(!inDictOnly)
			DropFile(ts.Location);
	}
	return ok;
}
//
// Data protection
//
#define PASZ  4096U
#define PAOFS   34U

static char * cryptPassword(char * p)
{
	p[2] = 'e'; p[3] = '$'; p[9] = 0;   p[1] = '8'; p[4] = 'g';
	p[7] = '+'; p[0] = '-'; p[6] = '+'; p[5] = 'p'; p[8] = '+';
	return p;
}

static char * protectFileName(char * p, const char * pDataPath)
{
	char   n[16];
	n[2] = '['; n[2] = 'P'; n[0] = 'D'; n[3] = '\x24'; n[3] = 'A'; n[4] = 0; n[1] = 'B';
	if(pDataPath && pDataPath[0]) {
		size_t len = sstrlen(strcpy(p, pDataPath));
		if(p[len-1] != '\\') {
			p[len] = '\\';
			p[++len] = 0;
		}
		strcpy(p + len, n);
	}
	else
		strcpy(p, n);
	return p;
}

int DbProvider::GetProtectData(FILE * f, uint16 * pBuf)
{
	int    ok = 1;
	char   cpw[16];
	STempBuffer temp_buf(PASZ);
	if(!temp_buf.IsValid())
		ok = 0;
	else if(fread(temp_buf, temp_buf.GetSize(), 1, f) == 1) {
		IdeaDecrypt(cryptPassword(cpw), temp_buf, temp_buf.GetSize());
		memcpy(pBuf, temp_buf + PAOFS, 32);
		memset(temp_buf, ' ', temp_buf.GetSize());
	}
	else
		ok = 0;
	return ok;
}

int DbProvider::SetupProtectData(const char * pOldPw, const char * pNewPw)
{
	int    ok = 1, exists = 1;
	FILE * f = 0;
	uint16 buf[32];
	char   cpw[16];
	char   file_name[MAX_PATH];
	char * p_temp = 0;
	char   mode[6];
	protectFileName(file_name, DataPath);
	if(fileExists(file_name)) {
		mode[0] = 'r';
		mode[1] = '+';
		mode[2] = 'b';
		mode[3] = 0;
	}
	else {
		exists = 0;
		mode[0] = 'w';
		mode[1] = '+';
		mode[2] = 'b';
		mode[3] = 0;
	}
	THROW(f = fopen(file_name, mode));
	if(!exists)
		DBS.GetProtectData(buf, 1);
	else {
		THROW(GetProtectData(f, buf));
		::decrypt((char *)buf, sizeof(buf));
	}
	THROW(stricmp((char *)buf, pOldPw) == 0);
	p_temp = static_cast<char *>(SAlloc::M(PASZ));
	// @v11.1.1 IdeaRandMem(p_temp, PASZ);
	// @v11.1.1 IdeaRandMem(buf, sizeof(buf));
	SObfuscateBuffer(p_temp, PASZ); // @v11.1.1
	SObfuscateBuffer(buf, sizeof(buf)); // @v11.1.1
	strcpy((char *)buf, pNewPw);
	::encrypt((char *)buf, sizeof(buf));
	memcpy(p_temp + PAOFS, buf, sizeof(buf));
	IdeaEncrypt(cryptPassword(cpw), p_temp, PASZ);
	rewind(f);
	THROW(fwrite(p_temp, PASZ, 1, f) == 1);
	CATCHZOK
	memset(buf, ' ', sizeof(buf));
	memset(p_temp, ' ', PASZ);
	memset(file_name, ' ', sizeof(file_name));
	ZDELETE(p_temp);
	SFile::ZClose(&f);
	return ok;
}

int DbProvider::GetProtectData()
{
	int    ok = 0;
	uint16 buf[32];
	char   file_name[MAX_PATH];
	protectFileName(file_name, DataPath);
	if(fileExists(file_name)) {
		FILE * f = fopen(file_name, "rb");
		if(f) {
			if(GetProtectData(f, buf)) {
				DBS.SetProtectData(buf);
				ok = 1;
			}
			SFile::ZClose(&f);
		}
	}
	else
		ok = -1;
	return ok;
}
//
//
//
int DbProvider::CreateTableSpec(DBTable * pTbl) { return P_Dict->CreateTableSpec(pTbl); }
int DbProvider::GetTableID(const char * pTblName, long * pID, DbTableStat * pStat) { return P_Dict->GetTableID(pTblName, pID, pStat); }
int DbProvider::GetTableInfo(long tblID, DbTableStat * pStat) { return P_Dict->GetTableInfo(tblID, pStat); }
int DbProvider::GetListOfTables(long options, StrAssocArray * pList) { return P_Dict->GetListOfTables(options, pList); }

int DbProvider::GetUniqueTableName(const char * pPrefix, DBTable * pTbl)
{
	SString tbl_name;
	long   num = 0;
	long   tbl_id = 0;
	do {
		(tbl_name = pPrefix).Cat(++num);
	} while(GetTableID(tbl_name, &tbl_id, 0));
	tbl_name.CopyTo(pTbl->tableName, sizeof(pTbl->tableName));
	return 1;
}

int DbProvider::RecoverTable(BTBLID tblID, BRecoverParam * pParam)
{
	return 0;
}
//
//
//
void DbProvider::Common_Login(const DbLoginBlock * pBlk)
{
	if(pBlk)
		Lb.Copy(*pBlk);
	State |= stLoggedIn;
}

void DbProvider::Common_Logout()
{
	State &= ~stLoggedIn;
}

int DbProvider::Login(const DbLoginBlock * pBlk, long options)
{
	Common_Login(pBlk);
	return 1;
}

int DbProvider::Logout()
{
	Common_Logout();
	return 1;
}

int DbProvider::PostProcessAfterUndump(DBTable * pTbl)
{
	return -1;
}

int DbProvider::Implement_GetPosition(DBTable * pTbl, DBRowId * pPos)
{
	ASSIGN_PTR(pPos, *pTbl->getCurRowIdPtr());
	return 1;
}

int DbProvider::CreateStmt(SSqlStmt * pS, const char * pText, long flags) { return 0; }
int DbProvider::DestroyStmt(SSqlStmt * pS) { return 0; }
int DbProvider::Binding(SSqlStmt & rS, int dir) { return 0; }
int DbProvider::ProcessBinding(int action, uint count, SSqlStmt * pStmt, SSqlStmt::Bind * pBind) { return 0; }
int DbProvider::ExecStmt(SSqlStmt & rS, uint count, int mode) { return 0; }
int DbProvider::Describe(SSqlStmt & rS, SdRecord &) { return 0; }
int DbProvider::Fetch(SSqlStmt & rS, uint count, uint * pActualCount) { return 0; }
//
//
//
int DbProvider::Implement_DeleteFrom(DBTable * pTbl, int useTa, DBQ & rQ)
{
	int    ok = 1;
	DBQuery * q = & selectAll().from(pTbl, 0L).where(rQ);
	q->setDestroyTablesMode(0);
	if(!useTa || DBS.GetTLA().StartTransaction()) {
		if(q->single_fetch(0, 0, spFirst)) do {
			uint8  key_buf[512];
			DBRowId _dbpos;
			if(!pTbl->getPosition(&_dbpos))
				ok = 0;
			else if(!pTbl->getDirectForUpdate(pTbl->getCurIndex(), key_buf, _dbpos))
				ok = 0;
			else if(pTbl->deleteRec() == 0) // @sfu
				ok = 0;
		} while(ok && q->single_fetch(0, 0, spNext));
		if(q->error)
			ok = 0;
		if(useTa)
			if(ok) {
				if(!DBS.GetTLA().CommitWork()) {
					DBS.GetTLA().RollbackWork();
					ok = 0;
				}
			}
			else
				DBS.GetTLA().RollbackWork();
	}
	else
		ok = 0;
	delete q;
	return ok;
}
