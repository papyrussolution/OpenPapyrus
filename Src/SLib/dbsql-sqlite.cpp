// DBSQL-SQLITE.CPP
// Copyright (c) A.Sobolev 2021, 2022, 2023, 2024, 2025
// ������ ���������� � DBMS Sqlite3
// @construction
//
#include <slib-internal.h>
#pragma hdrstop
#include <..\osf\SQLite\sqlite3.h>
#include <ued.h>

#ifndef NDEBUG
	#define DEBUG_LOG(msg) SLS.LogMessage("dbsqlite.log", msg, 0)
#else
	#define DEBUG_LOG(msg)
#endif

/*static*/bool SSqliteDbProvider::IsInitialized = false;

SSqliteDbProvider::SSqliteDbProvider() : DbProvider(sqlstSQLite, DbDictionary::CreateInstance(0, 0), 
	DbProvider::cSQL|DbProvider::cDbDependTa|DbProvider::cDirectSelectDataMapping), SqlGen(sqlstSQLite, 0), H(0), Flags(0)
{
	if(!IsInitialized) {
		SString temp_buf;
		SLS.QueryPath("temp", temp_buf);
		if(temp_buf.NotEmpty() && SFile::IsDir(temp_buf)) {
			sqlite3_temp_directory = (char *)sqlite3_malloc(temp_buf.Len()+1);
			if(sqlite3_temp_directory) {
				strcpy(sqlite3_temp_directory, temp_buf);
			}
		}
		sqlite3_initialize();
		IsInitialized = true;
	}
}

SSqliteDbProvider::~SSqliteDbProvider()
{
	Logout();
}

int FASTCALL SSqliteDbProvider::ProcessError(int status)
{
	int    ok = 1;
	if(!oneof3(status, SQLITE_OK, SQLITE_ROW, SQLITE_DONE)) {
		LastErr.Code = status;
		LastErr.Descr = sqlite3_errmsg(static_cast<sqlite3 *>(H));
		if(LastErr.Descr.NotEmpty()) {
			DBS.SetError(BE_SQLITE_TEXT, LastErr.Descr);
		}
		ok = 0;
	}
	return ok;	
}

int SSqliteDbProvider::Helper_GetFileStat(int tableType, const char * pFileName/*������� �������� �����!*/, long reqItems, DbTableStat * pStat)
{
	/*
		CREATE TABLE sqlite_schema(
			type text,
			name text,
			tbl_name text,
			rootpage integer,
			sql text
		);
	*/
	int    ok = 0;
	uint   actual = 0;
	const  char * p_sys_table_name = 0;
	if(tableType == 0)
		p_sys_table_name = "sqlite_schema";
	else if(tableType == 1) 
		p_sys_table_name = "sqlite_temp_master";
	if(p_sys_table_name) {
		BNFieldList2 fld_list;
		struct SqliteTblEntry {
			char   type[128];
			char   name[128];
			char   tbl_name[128];
			int64  rootpage;
			char   sql[256];
		} rec_buf;
		MEMSZERO(rec_buf);
		char   name[128];
		STRNSCPY(name, pFileName); // ����� ������������ � ��������!
		SString temp_buf;
		fld_list.addField("type", MKSTYPE(S_ZSTRING, sizeof(rec_buf.type)));
		fld_list.addField("name", MKSTYPE(S_ZSTRING, sizeof(rec_buf.name)));
		fld_list.addField("tbl_name", MKSTYPE(S_ZSTRING, sizeof(rec_buf.tbl_name)));
		fld_list.addField("rootpage", T_INT64);
		fld_list.addField("sql", MKSTYPE(S_ZSTRING, sizeof(rec_buf.sql)));
		SqlGen.Z().Select(&fld_list).From(p_sys_table_name).Sp().Tok(Generator_SQL::tokWhere).Sp().Eq("type", "table").Sp().Tok(Generator_SQL::tokAnd).Sp().Eq("name", name);
		SSqlStmt stmt(this, SqlGen);
		THROW(stmt.Exec(0, 0));
		THROW(stmt.BindData(+1, 1, fld_list, &rec_buf, 0));
		if(Fetch(stmt, 1, &actual) && actual) {
			THROW(stmt.GetData(0));
			ok = 1;
			if(pStat) {
				if(tableType == 1) {
					pStat->Flags |= XTF_TEMP;
				}
				if(reqItems & DbTableStat::iFldList) {
				}
				if(reqItems & DbTableStat::iIdxList) {
				}
				if(reqItems & DbTableStat::iNumRecs) {
					int64 rec_count = 0;
					SqlGen.Z().Select("count(*)").From(pFileName);
					SSqlStmt stmt_count(this, SqlGen);
					THROW(stmt_count.Exec(1, 0));
					THROW(stmt_count.BindItem(+1, 1, T_INT64, &rec_count));
					THROW(Binding(stmt_count, +1));
					if(Fetch(stmt_count, 1, &actual) && actual) {
						THROW(stmt_count.GetData(0));
						pStat->NumRecs = rec_count;
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SSqliteDbProvider::GetFileStat(const char * pFileName/*������� �������� �����!*/, long reqItems, DbTableStat * pStat)
{
	int    ok = 0;
	if(!isempty(pFileName)) {
		int    r0 = Helper_GetFileStat(0, pFileName, reqItems, pStat);
		if(r0)
			ok = 1;
		else {
			int    r1 = Helper_GetFileStat(1, pFileName, reqItems, pStat);
			if(r1)
				ok = 1;
		}
	}
	return ok;		
}

/*static*/int SSqliteDbProvider::CbTrace(uint flag, void * pCtx, void * p, void * x) // Callback-������� ��� ���������� ����������� ������ sqlite
{
	int    result = 0;
	if(pCtx) {
		SSqliteDbProvider * p_this = static_cast<SSqliteDbProvider *>(pCtx);
		SString msg_buf;
		msg_buf.Cat(getcurdatetime_(), DATF_ISO8601, 0);
		switch(flag) {
			case SQLITE_TRACE_STMT:
				msg_buf.Space().Cat("STMT").CatDiv(':', 2).Cat(sqlite3_expanded_sql((sqlite3_stmt*)p));
				//printf("STMT: %s\n", sqlite3_expanded_sql((sqlite3_stmt*)p));
				result = 1;
				break;
			case SQLITE_TRACE_PROFILE:
				//msg_buf.Space().Cat("PROFILE");
				
				//printf("PROFILE: %s took %llu ns\n", sqlite3_sql((sqlite3_stmt*)p), *(sqlite3_uint64*)x);
				break;
			case SQLITE_TRACE_ROW:
				//msg_buf.Space().Cat("ROW");
				
				//printf("ROW returned\n");
				break;
			case SQLITE_TRACE_CLOSE:
				msg_buf.Space().Cat("CLOSE");
				result = 1;
				//printf("DB closed\n");
				break;
		}
		if(result > 0)
			SLS.LogMessage(p_this->DebugLogFileName, msg_buf, SKILOBYTE(8));
	}
	return result;
}

/*static*/void SSqliteDbProvider::CbDebugLog(void * pUserData, int errCode, const char * pMsg)
{
	/*if(errCode == SQLITE_LOCKED || errCode == SQLITE_BUSY) {
		printf("LOCK DETECTED[%d]: %s\n", errCode, pMsg);
	}*/
	if(pUserData) {
		SString msg_buf;
		msg_buf.Cat(getcurdatetime_(), DATF_ISO8601, 0).Space().Cat(errCode).Space().Cat(pMsg);
		SSqliteDbProvider * p_this = static_cast<SSqliteDbProvider *>(pUserData);
		SLS.LogMessage(p_this->DebugLogFileName, msg_buf, SKILOBYTE(8));
	}
}

/*virtual*/int SSqliteDbProvider::DbLogin(const DbLoginBlock * pBlk, long options)
{
	int    ok = 1;
	sqlite3 * h = 0;
	SString path;
	SString log_path;
	assert(pBlk);
	THROW(pBlk);
	pBlk->GetAttr(DbLoginBlock::attrDbPath, path);
	{
		Logout();
		SLS.QueryPath("log", log_path);
		int    open_flags = 0;
		if(options & DbProvider::openfReadOnly)
			open_flags = SQLITE_OPEN_READONLY;
		else
			open_flags = SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE;
		sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
		if(SFile::IsDir(log_path)) {
			(DebugLogFileName = log_path).SetLastSlash().Cat("sqlite-debug").DotCat("log");
			sqlite3_config(SQLITE_CONFIG_LOG, CbDebugLog, this);
		}
		THROW(ProcessError(sqlite3_open_v2(path, &h, open_flags, 0)));
		H = h;
		if(options & DbProvider::openfMainThread) {
			sqlite3_exec(h, "PRAGMA journal_mode=WAL", 0, 0, 0);
			sqlite3_exec(h, "PRAGMA synchronous=NORMAL", 0, 0, 0);
		}
		{
			DbName.Z();
			DbPathID = 0;
			DataPath = path;
			DBS.GetDbPathID(DataPath, &DbPathID);
		}
		if(SFile::IsDir(log_path)) {
			/*
				#define SQLITE_TRACE_STMT       0x01
				#define SQLITE_TRACE_PROFILE    0x02
				#define SQLITE_TRACE_ROW        0x04
				#define SQLITE_TRACE_CLOSE      0x08
			*/
			(TraceFileName = log_path).SetLastSlash().Cat("sqlite-trace").DotCat("log");
			sqlite3_trace_v2(h, SQLITE_TRACE_STMT|SQLITE_TRACE_PROFILE|SQLITE_TRACE_CLOSE, CbTrace, this);
		}
	}
	CATCHZOK
	return ok;
}

/*virtual*/int SSqliteDbProvider::GetDatabaseState(uint * pStateFlags)
{
	int    ok = 0;
	return ok;
}

/*virtual*/SString & SSqliteDbProvider::MakeFileName_(const char * pTblName, SString & rBuf)
{
	return rBuf.SetIfEmpty(pTblName);
}
	
/*virtual*/int SSqliteDbProvider::IsFileExists_(const char * pFileName)
{
	return BIN(GetFileStat(pFileName, 0, 0) > 0);
}
	
/*virtual*/SString & SSqliteDbProvider::GetTemporaryFileName(SString & rFileNameBuf, long * pStart, bool forceInDataPath)
{
	/*
	uint64 t = SLS.GetProfileTime();
	if(t == 0)
		t = SLS.GetProfileTime();
	*/
	uint  _clk = clock();
	rFileNameBuf.Z().Cat("tt").Cat(_clk);
	return rFileNameBuf;
}
	
/*virtual*/int SSqliteDbProvider::CreateDataFile(const DBTable * pTbl, const char * pFileName, int createMode, const char * pAltCode)
{
	int    ok = 1;
	// @construction {
	const bool is_temp_table = IS_CRM_TEMP(createMode);
	const int  cm = RESET_CRM_TEMP(createMode);
	uint   ctf = Generator_SQL::ctfIndent;
	if(oneof2(cm, crmNoReplace, crmTTSNoReplace))
		ctf |= Generator_SQL::ctfIfNotExists;
	if(is_temp_table)
		ctf |= Generator_SQL::ctfTemporary;
	THROW(SqlGen.Z().CreateTable(*pTbl, 0, ctf));
	{
		SSqlStmt stmt(this, SqlGen);
		THROW(stmt.Exec(1, OCI_DEFAULT));
	}
	uint j;
	for(j = 0; j < pTbl->indexes.getNumKeys(); j++) {
		THROW(SqlGen.Z().CreateIndex(*pTbl, pFileName, j));
		{
			SSqlStmt stmt(this, SqlGen);
			THROW(stmt.Exec(1, OCI_DEFAULT));
		}
	}
	/* ��� sqlite sequence'� �� �����
	for(j = 0; j < pTbl->fields.getCount(); j++) {
		TYPEID _t = pTbl->fields[j].T;
		if(GETSTYPE(_t) == S_AUTOINC) {
			THROW(SqlGen.Z().CreateSequenceOnField(*pTbl, pFileName, j, 0));
			{
				SSqlStmt stmt(this, SqlGen);
				THROW(stmt.Exec(1, OCI_DEFAULT));
			}
		}
	}*/
	if(createMode < 0 && IS_CRM_TEMP(createMode)) {
		//
		// ������������ ��� ���������� ����� � �������� �� ��� ������������ �������� //
		//
		AddTempFileName(pFileName);
	}
	// } @construction 
	CATCHZOK
	return ok;
}
	
/*virtual*/int SSqliteDbProvider::DropFile(const char * pFileName)
{
	int    ok = 0;
	return ok;
}
	
/*virtual*/int SSqliteDbProvider::PostProcessAfterUndump(DBTable * pTbl)
{
	int    ok = 1;
	return ok;
}

/*virtual*/int SSqliteDbProvider::CreateStmt(SSqlStmt * pS, const char * pText, long flags)
{
	int    ok = 1;
	uint   prep_flags = SQLITE_PREPARE_NORMALIZE; // SQLITE_PREPARE_XXX
	const  char * p_ztail = 0;
	sqlite3_stmt * p_stmt = 0;
	THROW_S_S(!isempty(pText), SLERR_INVPARAM, __FUNCTION__"/pText");
	const int text_len = sstrleni(pText);
	THROW(ProcessError(sqlite3_prepare_v3(static_cast<sqlite3 *>(H), pText, text_len, prep_flags, &p_stmt, &p_ztail))); 
	pS->H = p_stmt;
	CATCHZOK
	return ok;
}

/*virtual*/int SSqliteDbProvider::DestroyStmt(SSqlStmt * pS)
{
	int   ok = 1;
	if(pS && pS->H) {
		int r = sqlite3_finalize(static_cast<sqlite3_stmt *>(pS->H));
		pS->H = 0;
		pS->Typ = Generator_SQL::typUndef;
		ok = ProcessError(r);
	}
	else
		ok = -1;
	return ok;
}

/*virtual*/int SSqliteDbProvider::ExecStmt(SSqlStmt & rS, uint count, int mode)
{
	int    ok = 1;
	if(rS.Typ == Generator_SQL::typSelect) {
		;
	}
	else {
		ok = ProcessError(sqlite3_step(static_cast<sqlite3_stmt *>(rS.H)));
	}
#ifndef NDEBUG // {
	if(!ok) {
		SString log_buf;
		//log_buf.Cat("EXEC").CatDiv(':', 2).Cat(LastErrMsg);
		DEBUG_LOG(log_buf);
	}
#endif // } !NDEBUG
	return ok;
}

/*virtual*/int SSqliteDbProvider::Logout()
{
	int    ok = 1;
	if(H) {
		ok = ProcessError(sqlite3_close(static_cast<sqlite3 *>(H)));
		H = 0;
	}
	else
		ok = -1;
	return ok;
}

/*virtual*/int SSqliteDbProvider::StartTransaction()
{
	int    ok = -1;
	if(!(State & stTransaction)) {
		SqlGen.Z().Tok(Generator_SQL::tokBegin).Sp().Tok(Generator_SQL::tokTransaction);
		char * p_err_msg = 0;
		ok = ProcessError(sqlite3_exec(static_cast<sqlite3 *>(H), static_cast<SString &>(SqlGen), 0, 0, &p_err_msg));
		if(ok) {
			State |= stTransaction;
		}
	}
	else
		ok = -1;
	return ok;
}

/*virtual*/int SSqliteDbProvider::CommitWork()
{
	int    ok = -1;
	int    rbr = 0; // rollback-result
	if(State & stTransaction) {
		SqlGen.Z().Tok(Generator_SQL::tokCommit)/*.Sp().Tok(Generator_SQL::tokTransaction)*/;
		char * p_err_msg = 0;
		ok = ProcessError(sqlite3_exec(static_cast<sqlite3 *>(H), static_cast<SString &>(SqlGen), 0, 0, &p_err_msg));
		if(!ok) {
			rbr = RollbackWork();
		}
		State &= ~stTransaction;
	}
	else
		ok = -1;
	return ok;
}

/*virtual*/int SSqliteDbProvider::RollbackWork()
{
	int    ok = -1;
	if(State & stTransaction) {
		SqlGen.Z().Tok(Generator_SQL::tokRollback)/*.Sp().Tok(Generator_SQL::tokTransaction)*/;
		char * p_err_msg = 0;
		ok = ProcessError(sqlite3_exec(static_cast<sqlite3 *>(H), static_cast<SString &>(SqlGen), 0, 0, &p_err_msg));
		State &= ~stTransaction;
	}
	else
		ok = -1;
	return ok;
}

/*virtual*/int SSqliteDbProvider::Implement_Open(DBTable* pTbl, const char* pFileName, int openMode, char* pPassword)
{
	int    ok = 1;
	if(pTbl) {
		pTbl->fileName = NZOR(pFileName, pTbl->tableName);
		pTbl->OpenedFileName = pTbl->fileName;
		pTbl->FixRecSize = pTbl->fields.CalculateFixedRecSize();
	}
	else
		ok = 0;
	return ok;
}

/*virtual*/int SSqliteDbProvider::Implement_Close(DBTable * pTbl)
{
	return 1;
}

int SSqliteDbProvider::Helper_Fetch(DBTable * pTbl, DBTable::SelectStmt * pStmt, uint * pActual)
{
	int    ok = 1;
	uint   actual = 0;
	THROW(ok = Fetch(*pStmt, 1, &actual));
	if(ok > 0) {
		THROW(pStmt->GetData(0));
		BtrError = 0;
		ok = 1;
	}
	else {
		BtrError = BE_EOF;
		ok = -1;
	}
	CATCHZOK
	ASSIGN_PTR(pActual, actual);
	return ok;
}

int SSqliteDbProvider::Helper_MakeSearchQuery(DBTable * pTbl, int idx, void * pKey, int srchMode, long sf, SearchQueryBlock & rBlk)
{
	rBlk.Z();
	int    ok = 1;
	SString temp_buf;
	const  BNKeyList & r_indices = pTbl->indexes;
	const  BNKey & r_key = r_indices[idx];
	const  int  ns = r_key.getNumSeg();
	const  char * p_alias = "t";
	rBlk.SrchMode = (srchMode == spNext) ? spGt : ((srchMode == spPrev) ? spLt : srchMode);
	rBlk.P_KeyData = pKey;
	rBlk.SqlG.Z().Tok(Generator_SQL::tokSelect);
	rBlk.SqlG.Sp();
	rBlk.SqlG.Text(p_alias).Dot().Aster().Com().Text(p_alias).Dot().Tok(Generator_SQL::tokRowId);
	rBlk.SqlG.Sp().From(pTbl->fileName, p_alias);
	if(sf & DBTable::sfDirect) {
		DBRowId * p_rowid = static_cast<DBRowId *>(pKey);
		THROW(p_rowid && (p_rowid->IsI32() || p_rowid->IsI64())); // @todo @err
		p_rowid->ToStr__(temp_buf);
		rBlk.SqlG.Sp().Tok(Generator_SQL::tokWhere).Sp().Tok(Generator_SQL::tokRowId)._Symb(_EQ_).QText(temp_buf);
	}
	else {
		{
			if(rBlk.SqlG.GetIndexName(*pTbl, idx, temp_buf)) {
				rBlk.SqlG.Sp().Tok(Generator_SQL::tokIndexedBy).Sp().Text(temp_buf);
			}
		}
		if(oneof2(rBlk.SrchMode, spFirst, spLast)) {
			r_indices.setBound(idx, 0, BIN(rBlk.SrchMode == spLast), rBlk.TempKey);
			rBlk.P_KeyData = rBlk.TempKey;
		}
		rBlk.SqlG.Sp().Tok(Generator_SQL::tokWhere).Sp();
		{
			for(int i = 0; i < ns; i++) {
				const BNField & r_fld = r_indices.field(idx, i);
				if(i > 0) { // �� ������ �������
					rBlk.SqlG.Tok(Generator_SQL::tokAnd).Sp();
					if(rBlk.SrchMode != spEq)
						rBlk.SqlG.LPar();
				}
				if(r_key.getFlags(i) & XIF_ACS) {
					int   _func_tok = Generator_SQL::tokLower;
					rBlk.SqlG.Func(_func_tok, r_fld.Name);
				}
				else
					rBlk.SqlG.Text(r_fld.Name);
				int   cmps = _EQ_;
				if(rBlk.SrchMode == spEq)
					cmps = _EQ_;
				else if(rBlk.SrchMode == spLt)
					cmps = (i == ns-1) ? _LT_ : _LE_;
				else if(oneof2(rBlk.SrchMode, spLe, spLast))
					cmps = _LE_;
				else if(rBlk.SrchMode == spGt) {
					cmps = (i == ns-1) ? _GT_ : _GE_;
				}
				else if(oneof2(rBlk.SrchMode, spGe, spFirst))
					cmps = _GE_;
				rBlk.SqlG._Symb(cmps);
				rBlk.SqlG.Param(temp_buf.NumberToLat(i));
				rBlk.SegMap.add(i);
				if(i > 0 && rBlk.SrchMode != spEq) {
					//
					// ��� ��������� ��������� ����� ������ � ����������� ��������
					// ������ ������������� �������� ����������� ������ ��� ���������
					// ���� ���������� ���������.
					// ������:
					//   index {X, Y, Z}
					//   X > :A and (Y > :B or (X <> :A)) and (Z > :C or (X <> :A and Y <> :B))
					//
					rBlk.SqlG.Sp().Tok(Generator_SQL::tokOr).Sp().LPar();
					for(int j = 0; j < i; j++) {
						const BNField & r_fld2 = r_indices.field(idx, j);
						if(j > 0)
							rBlk.SqlG.Tok(Generator_SQL::tokAnd).Sp();
						if(r_key.getFlags(j) & XIF_ACS) {
							rBlk.SqlG.Func(Generator_SQL::tokLower, r_fld2.Name);
						}
						else
							rBlk.SqlG.Text(r_fld2.Name);
						rBlk.SqlG._Symb(_NE_);
						rBlk.SqlG.Param(temp_buf.NumberToLat(j));
						// @v12.4.4 @fix (���� ������� ��� �������� � ������ ��� j < i) seg_map.add(j);
						rBlk.SqlG.Sp();
					}
					rBlk.SqlG.RPar().RPar().Sp();
				}
				rBlk.SqlG.Sp();
			}
		}
		if(oneof3(rBlk.SrchMode, spLe, spLt, spLast)) { // �������� ����������� ������
			rBlk.SqlG.Tok(Generator_SQL::tokOrderBy);
			for(int i = 0; i < ns; i++) {
				const BNField & r_fld = r_indices.field(idx, i);
				if(i)
					rBlk.SqlG.Com();
				rBlk.SqlG.Sp().Text(r_fld.Name).Sp().Tok(Generator_SQL::tokDesc);
			}
			rBlk.SqlG.Sp();
		}
		rBlk.Flags |= SearchQueryBlock::fCanContinue;
	}
	CATCHZOK
	return ok;
}

/*virtual*/int SSqliteDbProvider::Implement_Search(DBTable * pTbl, int idx, void * pKey, int srchMode, long sf)
{
	//
	// ���������� ��� ������� ���������� �� SOraDbProvider::Implement_Search() � ����� ������� �������������.
	// ������ hint'�� ����� �� ����� ������������ ���������� sql �� SQLite "INDEXED BY"
	// ------------------------------------------------------------------------------------------------------
	//
	// BNKeyList BNFieldList2 BNKey Generator_SQL
	//
	// select /*+ index_asc(tbl_name index_name) */ * from
	//
	// � sqlite ��� "for update" (��� ���� ����������� ��� ���������, ��� ���� �� ����������������������)

	int    ok = 1;
	int    can_continue = false; // ���� can_continue == true, �� ����������� ����������� ������ spNext ��� spPrev. ��������������, stmt ����������� � pTbl.
	bool   new_stmt = false;
	uint   actual = 0;
	DBTable::SelectStmt * p_stmt = 0;
	const BNKeyList & r_indices = pTbl->indexes;
	const char * p_alias = "t";
	SString temp_buf;
	const  BNKey & r_key = r_indices[idx];
	const  bool is_key_uniq = !(r_key.getFlags(idx) & (XIF_DUP|XIF_REPDUP));
	//
	// ��������� ���� ������������ ��� ��� ����: ���� �������� ������ �� ��������� ��������� ������, �� pTbl->GetStmt() == 0,
	// ��, ������ �����, ��� ��������, ��� ���������� ����������� �������� ������. � ���� ������ �� ������ ������ ������,
	// �������� �� ��������� ���������� �������� �����, �� �������� ���� �������. ����� ��� �� ������, �� ���� ���� �����:
	// ���� ���� ������������, �� �� ����� �� ����� �� ����� ������ �� �������-���������� �������� �������� ��������.
	// ��� �� ����� � ��� ���� ��������� - rowid ��������� ���������� ������ (��� ��, ������� ������������� ��������� ���������� �������� �����).
	// ����� �������, � ���� �������� �� ��������� �� ���������� �����:
	//   ������ ��������� ���, ����� ��������� ������ ������ ���� �������� �� ������� ��������� (� �� <= ��� >=) ���������� �������� �����,
	//   �����, ���������� ���� ������ �� ������ ���� ���� ����� ��������� � ����� ����������� ������� �������� ����� ��, � �������
	//   rowid ����� ������ ��������. ���� �� ����� ����� ������ - ��������� - ������ ��� �������� ��� ������.
	//   ����� ���� ����� ������ ����������� (���, �� ����������) - ����� �� ����� ��������� "������������" �������������� ��������
	//   � ������� ��������� ����� (���� �����, �������, ����) � ����� ���� ���, ����� ������ ���������� - �� ����, ������, ���� ������,
	//   ����� �������� �����������.
	//
	bool   do_grop_next_rec = false; // @v12.4.6 
	//
	bool   do_make_query = true;
	//
	THROW(idx < (int)r_indices.getNumKeys());
	if(oneof2(srchMode, spNext, spPrev)) {
		assert(!(sf & DBTable::sfDirect)); // � ������ spNext/spPrev ���� sfDirect - �������������
		p_stmt = pTbl->GetStmt();
		if(p_stmt) {
			do_make_query = false;
			THROW(ok = Helper_Fetch(pTbl, p_stmt, &actual));
			if(ok > 0) {
				int    r = 1;
				pTbl->copyBufToKey(idx, pKey);
				if(oneof5(p_stmt->Sp, spGt, spGe, spLt, spLe, spEq)) {
					int kc = r_indices.compareKey(p_stmt->Idx, pKey, p_stmt->Key);
					if(kc == 0) {
						if(oneof2(p_stmt->Sp, spGt, spLt))
							r = 0;
					}
					else if(kc < 0) {
						if(oneof3(p_stmt->Sp, spGt, spGe, spEq))
							r = 0;
					}
					else if(kc > 0) {
						if(oneof3(p_stmt->Sp, spLt, spLe, spEq))
							r = 0;
					}
				}
				if(r)
					can_continue = true;
				else {
					BtrError = BE_EOF;
					ok = 0;
				}
			}
			else
				ok = 0;
		}
		else {
			// @v12.4.5 @construction {
			// ��� ��� � ��� ���� ���� pKey � ��� �� ������� ����������� � ���������� ������ - ������������� ������ � ���� � ��� �����������,
			// � ������� ��� ������. ������������ ��������� ������ - ���� ���� �� ����������, �� ��� ����������� �� ����� ������ ������
			// ����� ���������� �� ���������. �� ��� �������� �� ����� ������ ����� ����, ��� ����� �������.
			do_make_query = true; // ��! ���������! 
			// �������� ������������ ������ ����� ������ �� ��������� �������.
			// } @v12.4.5 @construction 
			/* @v12.4.5
			BtrError = BE_EOF;
			ok = 0;
			*/
		}
	}
	if(do_make_query) {
		SearchQueryBlock sqb;
		THROW(Helper_MakeSearchQuery(pTbl, idx, pKey, srchMode, sf, sqb));
		can_continue = LOGIC(sqb.Flags & SearchQueryBlock::fCanContinue);
		{
			THROW(p_stmt = new DBTable::SelectStmt(this, sqb.SqlG, idx, sqb.SrchMode, sf));
			new_stmt = true;
			THROW(p_stmt->IsValid());
			{
				size_t key_len = 0;
				p_stmt->BL.Dim = 1;
				for(uint i = 0; i < sqb.SegMap.getCount(); i++) {
					const int  seg = sqb.SegMap.get(i);
					const BNField & r_fld = r_indices.field(idx, seg);
					const size_t seg_offs = r_indices.getSegOffset(idx, seg);
					key_len += stsize(r_fld.T);
					if(r_key.getFlags(seg) & XIF_ACS) {
						strlwr866((char *)(PTR8(sqb.P_KeyData) + seg_offs));
					}
					SSqlStmt::Bind b;
					b.Pos = -(int16)(i+1);
					b.Typ = r_fld.T;
					b.P_Data = PTR8(sqb.P_KeyData) + seg_offs;
					uint   lp = 0;
					if(p_stmt->BL.lsearch(&b.Pos, &lp, PTR_CMPFUNC(int16)))
						p_stmt->BL.atFree(lp);
					p_stmt->BL.insert(&b);
				}
				memcpy(p_stmt->Key, pKey, smin(sizeof(p_stmt->Key), key_len));
				THROW(Binding(*p_stmt, -1));
				THROW(p_stmt->SetData(0));
			}
			THROW(p_stmt->Exec(0, OCI_DEFAULT));
			/*if(!(sf & DBTable::sfForUpdate))*/ { // rowid ���������� ������������� (see above)
				int rowid_pos = pTbl->fields.getCount()+1;
				THROW(p_stmt->BindRowId(rowid_pos, 1, pTbl->getCurRowIdPtr()));
			}
			THROW(p_stmt->BindData(+1, 1, pTbl->fields, pTbl->getDataBufConst(), pTbl->getLobBlock()));
			THROW(ok = Helper_Fetch(pTbl, p_stmt, &actual));
			if(ok > 0)
				pTbl->copyBufToKey(idx, pKey);
			else {
				ok = 0;
				can_continue = false;
			}
		}
	}
	CATCH
		ok = 0;
		can_continue = false;
	ENDCATCH
	if(can_continue) {
		pTbl->SetStmt(p_stmt);
	}
	else {
		pTbl->SetStmt(0);
		if(new_stmt)
			delete p_stmt;
	}
	return ok;
}

/*virtual*/int SSqliteDbProvider::Implement_UpdateRec(DBTable* pTbl, const void* pDataBuf, int ncc)
{
	int    ok = 1;
	SString temp_buf;
	if(pDataBuf)
		pTbl->CopyBufFrom(pDataBuf);
	SqlGen.Z().Tok(Generator_SQL::tokUpdate).Sp().Text(pTbl->fileName).Sp().Tok(Generator_SQL::tokSet).Sp();
	{
		const uint fld_count = pTbl->fields.getCount();
		for(uint i = 0; i < fld_count; i++) {
			if(i)
				SqlGen.Com();
			SqlGen.Text(pTbl->fields[i].Name)._Symb(_EQ_).Param(temp_buf.NumberToLat(i));
		}
	}
	//THROW(pTbl->getCurRowIdPtr()->IsI32());
	pTbl->getCurRowIdPtr()->ToStr__(temp_buf);
	SqlGen.Sp().Tok(Generator_SQL::tokWhere).Sp().Tok(Generator_SQL::tokRowId)._Symb(_EQ_).Text(temp_buf);
	{
		SSqlStmt stmt(this, SqlGen);
		THROW(stmt.IsValid());
		THROW(stmt.BindData(-1, 1, pTbl->fields, pTbl->getDataBufConst(), pTbl->getLobBlock()));
		THROW(stmt.SetDataDML(0));
		THROW(stmt.Exec(1, /*OCI_COMMIT_ON_SUCCESS*/OCI_DEFAULT)); // @debug(OCI_COMMIT_ON_SUCCESS)
	}
	CATCHZOK
	return ok;
}

/*virtual*/int SSqliteDbProvider::Implement_DeleteRec(DBTable * pTbl)
{
	int    ok = 1;
	const  uint fld_count = pTbl->fields.getCount();
	SString temp_buf;
	SqlGen.Z().Tok(Generator_SQL::tokDelete).Sp().From(pTbl->fileName, 0).Sp();
	//THROW(pTbl->getCurRowIdPtr()->IsI32());
	pTbl->getCurRowIdPtr()->ToStr__(temp_buf);
	SqlGen.Sp().Tok(Generator_SQL::tokWhere).Sp().Tok(Generator_SQL::tokRowId)._Symb(_EQ_).Text(temp_buf);
	{
		SSqlStmt   stmt(this, SqlGen);
		THROW(stmt.Exec(1, OCI_DEFAULT));
	}
	CATCHZOK
	return ok;
}

/*virtual*/int SSqliteDbProvider::Implement_InsertRec(DBTable * pTbl, int idx, void * pKeyBuf, const void * pData)
{
	int    ok = 1;
	int    in_subst_no = 0;
	int    out_subst_no = 0;
	uint   i;
	int    do_process_lob = 0;
	int    map_ret_key = 0;
	BNKey  key;
	uint   ns = 0;
	const  uint fld_count = pTbl->fields.getCount();
	SString temp_buf;
	SString let_buf;
	SSqlStmt stmt(this);
	if(pData) {
		pTbl->CopyBufFrom(pData);
		if(pTbl->State & DBTable::sHasLob) {
			const RECORDSIZE frs = pTbl->FixRecSize;
			const SLob * p_src_lob = reinterpret_cast<const SLob *>(PTR8C(pData) + frs);
			//SLob * p_dest_lob = reinterpret_cast<const SLob *>(PTR8C(pData) + frs);
			/*
			size_t s = (frs && frs < bufLen) ? frs : bufLen;
			memcpy(P_DBuf, pBuf, s);
			DBField last_fld;
			THROW(getField(fields.getCount()-1, &last_fld));
			if(srcBufSize)
				THROW(writeLobData(last_fld, PTR8C(pBuf)+frs, (srcBufSize > frs) ? (srcBufSize-frs) : 0));
			*/
		}
		// @construction @20250910 ����� � ����������� ������ � LOB'���

		//pTbl->CopyBufLobFrom(const void * pBuf, size_t srcBufSize);
	}
	/*if(pTbl->State & DBTable::sHasLob) {
		int    r = 0;
		THROW(r = pTbl->StoreAndTrimLob());
		if(r > 0)
			do_process_lob = 1;
	}*/
	SqlGen.Z().Tok(Generator_SQL::tokInsert).Sp().Tok(Generator_SQL::tokInto).Sp().Text(pTbl->fileName).Sp();
	SqlGen.Tok(Generator_SQL::tokValues).Sp().LPar();
	stmt.BL.Dim = 1;
	stmt.BL.P_Lob = pTbl->getLobBlock();
	{
		bool is_first_fld_enum_item = true;
		for(i = 0; i < fld_count; i++) {
			const BNField & r_fld = pTbl->fields.getField(i);
			bool  do_skip_this_fld = false;
			if(GETSTYPE(r_fld.T) == S_AUTOINC) {
				long   val = 0;
				size_t val_sz = 0;
				r_fld.getValue(pTbl->getDataBufConst(), &val, &val_sz);
				assert(val_sz == sizeof(val));
				if(val == 0) {
					// (������, �� ���� ��� ������ - ���� ����������� null-�������� ����) do_skip_this_fld = true;
				}
			}
			if(!do_skip_this_fld) {
				if(!is_first_fld_enum_item)
					SqlGen.Com();
				SqlGen.Param(temp_buf.NumberToLat(in_subst_no++));
				stmt.BindItem(-in_subst_no, 1, r_fld.T, PTR8(pTbl->getDataBuf()) + r_fld.Offs);
				is_first_fld_enum_item = false;
			}
		}
	}
	SqlGen.RPar();
	SqlGen.Sp().Tok(Generator_SQL::tokReturning).Sp().Tok(Generator_SQL::tokRowId);
	{
		//
		// temp_buf ����� ��������� ������ ����������, � ������� ������ ��������� ������������ �������� //
		//
		let_buf.NumberToLat(out_subst_no++);
		temp_buf.Z().Colon().Cat(let_buf);
		stmt.BindRowId(out_subst_no, 1, pTbl->getCurRowIdPtr());
		if(pKeyBuf && idx >= 0 && idx < static_cast<int>(pTbl->indexes.getNumKeys())) {
			map_ret_key = 1;
			key = pTbl->indexes[idx];
			ns = static_cast<uint>(key.getNumSeg());
			for(i = 0; i < ns; i++) {
				const BNField & r_fld = pTbl->indexes.field(idx, i);
				SqlGen.Com().Text(r_fld.Name);
				let_buf.NumberToLat(out_subst_no++);
				temp_buf.CatDiv(',', 0).Colon().Cat(let_buf);
				stmt.BindItem(out_subst_no, 1, r_fld.T, PTR8(pKeyBuf)+pTbl->indexes.getSegOffset(idx, i));
			}
		}
	}
	//
	{
		THROW(stmt.SetSqlText(SqlGen));
		THROW(Binding(stmt, -1));
		THROW(Binding(stmt, +1)); // @v12.4.1
		THROW(stmt.SetDataDML(0));
		THROW(stmt.Exec(1, OCI_DEFAULT));
		// @v12.4.1 THROW(stmt.GetOutData(0));
		THROW(stmt.GetData(0)); // @v12.4.1
		/*if(do_process_lob) {
			//
			// ���� � ������ ���� �� ������ �������� LOB-�����, �� �������� ����������
			// ����������� ������ � �������� �������� LOB-�����.
			//
			// @todo ���� ��������� ������ LOB-����, � �� ��� ������.
			//
			DBRowId row_id = *pTbl->getCurRowIdPtr();
			THROW(Implement_Search(pTbl, -1, &row_id, spEq, DBTable::sfDirect|DBTable::sfForUpdate));
			THROW(pTbl->RestoreLob());
			THROW(Implement_UpdateRec(pTbl, 0, 0));
		}*/
	}
	CATCHZOK
	return ok;
}

int SSqliteDbProvider::ResetStatement(SSqlStmt & rS)
{
	return ProcessError(sqlite3_reset(SSqliteDbProvider::StmtHandle(rS)));
}

/*virtual*/int SSqliteDbProvider::Implement_BExtInsert(BExtInsert * pBei)
{
	// �����: ��� �� SOraDbProvider::Implement_BExtInsert. ������ ���� ��� ����������.
	int    ok = -1;
#if 1 // @construction {
	//
	// ����� �� �������� ���������� ����������� ������ ������� pBei->P_Tbl ������������ ��������� ����� rec_buf.
	//
	SBaseBuffer rec_buf;
	rec_buf.Init();
	const  uint num_recs = pBei->GetCount();
	if(num_recs) {
		uint   i;
		int    in_subst_no = 0;
		int    out_subst_no = 0;
		DBTable * p_tbl = pBei->getTable();
		const  uint fld_count = p_tbl->fields.getCount();
		SString temp_buf;
		SSqlStmt stmt(this);
		SqlGen.Z().Tok(Generator_SQL::tokInsert).Sp().Tok(Generator_SQL::tokInto).Sp().Text(p_tbl->fileName).Sp();
		SqlGen.Tok(Generator_SQL::tokValues).Sp().LPar();
		//
			stmt.BL.Dim = 1;
			stmt.BL.P_Lob = p_tbl->getLobBlock();
			{
				bool is_first_fld_enum_item = true;
				for(i = 0; i < fld_count; i++) {
					const BNField & r_fld = p_tbl->fields.getField(i);
					bool  do_skip_this_fld = false;
					if(GETSTYPE(r_fld.T) == S_AUTOINC) {
						long   val = 0;
						size_t val_sz = 0;
						r_fld.getValue(p_tbl->getDataBufConst(), &val, &val_sz);
						assert(val_sz == sizeof(val));
						if(val == 0) {
							// (������, �� ���� ��� ������ - ���� ����������� null-�������� ����) do_skip_this_fld = true;
						}
					}
					if(!do_skip_this_fld) {
						if(!is_first_fld_enum_item)
							SqlGen.Com();
						SqlGen.Param(temp_buf.NumberToLat(in_subst_no++));
						stmt.BindItem(-in_subst_no, 1, r_fld.T, PTR8(p_tbl->getDataBuf()) + r_fld.Offs);
						is_first_fld_enum_item = false;
					}
				}
			}
			SqlGen.RPar();
			SqlGen.Sp().Tok(Generator_SQL::tokReturning).Sp().Tok(Generator_SQL::tokRowId);
		//
		{
			THROW(stmt.SetSqlText(SqlGen));
			THROW(rec_buf.Alloc(p_tbl->getBufLen()));
			for(i = 0; i < num_recs; i++) {
				SBaseBuffer b = pBei->Get(i);
				assert(b.Size <= rec_buf.Size);
				memcpy(rec_buf.P_Buf, b.P_Buf, b.Size);
				//
				THROW(stmt.BindData(-1, 1, p_tbl->fields, rec_buf.P_Buf, p_tbl->getLobBlock()));
				/*
				if(p_tbl->State & DBTable::sHasAutoinc) {
					for(uint j = 0; j < fld_count; j++) {
						const BNField & r_fld = p_tbl->fields[j];
						if(GETSTYPE(r_fld.T) == S_AUTOINC) {
							long val = 0;
							size_t val_sz = 0;
							r_fld.getValue(rec_buf.P_Buf, &val, &val_sz);
							assert(val_sz == sizeof(val));
							if(val == 0) {
								THROW(GetAutolongVal(*p_tbl, j, &val));
								r_fld.setValue(rec_buf.P_Buf, &val);
							}
						}
					}
				}
				*/
				THROW(stmt.SetDataDML(0));
				THROW(stmt.Exec(1, OCI_DEFAULT));
				THROW(ResetStatement(stmt));
			}
			ok = 1;
		}
	}
	CATCHZOK
	rec_buf.Destroy();
#endif // } @construction
	return ok;
}

/*virtual*/int SSqliteDbProvider::Implement_GetPosition(DBTable * pTbl, DBRowId * pPos)
{
	ASSIGN_PTR(pPos, *pTbl->getCurRowIdPtr());
	return 1;
}

/*virtual*/int SSqliteDbProvider::Implement_DeleteFrom(DBTable * pTbl, int useTa, DBQ & rQ)
{
	int    ok = 1;
	int    ta = 0;
	if(useTa) {
		THROW(StartTransaction());
		ta = 1;
	}
	SqlGen.Z().Tok(Generator_SQL::tokDelete).Sp().From(pTbl->fileName, 0);
	if(&rQ && rQ.tree) {
		SqlGen.Sp().Tok(Generator_SQL::tokWhere).Sp();
		rQ.tree->CreateSqlExpr(SqlGen, -1);
	}
	{
		SSqlStmt stmt(this, SqlGen);
		THROW(stmt.Exec(1, OCI_DEFAULT));
	}
	if(ta) {
		THROW(CommitWork());
		ta = 0;
	}
	CATCH
		if(ta) {
			RollbackWork();
			ta = 0;
		}
		ok = 0;
	ENDCATCH
	return ok;
}

/*virtual*/int SSqliteDbProvider::GetFileStat(DBTable * pTbl, long reqItems, DbTableStat * pStat)
{
	SString file_name;
	return GetFileStat(MakeFileName_(pTbl->tableName, file_name), reqItems, pStat);
}

/*virtual*/int SSqliteDbProvider::ProtectTable(long dbTableID, char * pResetOwnrName, char * pSetOwnrName, int clearProtection)
{
	int    ok = 0;
	return ok;
}

/*virtual*/int SSqliteDbProvider::RecoverTable(BTBLID tblID, BRecoverParam * pParam)
{
	int    ok = 0;
	return ok;
}

/*static*/sqlite3_stmt * FASTCALL SSqliteDbProvider::StmtHandle(const SSqlStmt & rS)
{
	return static_cast<sqlite3_stmt *>(rS.H);
}

/*virtual*/int SSqliteDbProvider::Binding(SSqlStmt & rS, int dir)
{
	int    ok = 1;
	bool   bind_result = true;
	const  uint row_count = rS.BL.Dim;
	const  uint col_count = rS.BL.getCount();
	sqlite3_stmt * h_stmt = StmtHandle(rS);
	assert(checkirange(row_count, 1U, 1024U));
	THROW(rS.SetupBindingSubstBuffer(dir, row_count));
	if(dir == -1) { // �������� ��� SQL-������� ��������� (�������� ��� INSERT)
		for(uint i = 0; i < col_count; i++) {
			SSqlStmt::Bind & r_bind = rS.BL.at(i);
			if(r_bind.Pos < 0) {
				THROW(ProcessBinding(0, row_count, &rS, &r_bind));
				// @v12.4.2 THROW(ProcessBinding(-1, row_count, &rS, &r_bind));
			}
		}
	}
	else if(dir == +1) { // ��������� �� SQL-������� �������� (�������� ��� SELECT)
		for(uint i = 0; i < col_count; i++) {
			SSqlStmt::Bind & r_bind = rS.BL.at(i);
			if(r_bind.Pos > 0) {
				THROW(ProcessBinding(0, row_count, &rS, &r_bind));
				// @v12.4.2 THROW(ProcessBinding(dir, row_count, &rS, &r_bind));
			}
		}
	}
	CATCH
		ok = 0;
		//DEBUG_LOG(LastErrMsg);
	ENDCATCH
	return ok;
}

int SSqliteDbProvider::ProcessBinding_SimpleType(int action, uint count, SSqlStmt * pStmt, SSqlStmt::Bind * pBind, uint ntvType)
{
	if(action == 0) {
		pBind->NtvTyp = ntvType;
		if(count > 1)
			pStmt->AllocBindSubst(count, pBind->NtvSize, pBind);
	}
	else if(pBind->Dim_ > 1) {
		if(action < 0)
			memcpy(pStmt->GetBindOuterPtr(pBind, count), pBind->P_Data, pBind->NtvSize);
		else if(action == 1)
			memcpy(pBind->P_Data, pStmt->GetBindOuterPtr(pBind, count), pBind->NtvSize);
	}
	return 1;
}
//
// ARG(action IN):
//   0 - ������������� ��������� SSqlStmt::Bind
//   1 - ���������� ������ �� �������� ��������� �� ���������� ������
//  -1 - ������� ������ �� ���������� ������� �� ������� ��������
//  1000 - ���������� ����������� ��� SQL-������� ��������� ��������� SSqlStmt::Bind
//
/*virtual*/int SSqliteDbProvider::ProcessBinding(int action, uint count, SSqlStmt * pStmt, SSqlStmt::Bind * pBind)
{
	int    ok = 1;
	const  size_t sz = stsize(pBind->Typ);
	uint16 out_typ = 0;
	sqlite3_stmt * h_stmt = StmtHandle(*pStmt);
	void * p_data = 0;
	// (��� 2 ������� ������ ���� ����� ������� pStmt->GetBindOuterPtr ��������� �� ���������� �� ���� pBind->Dim) {
	if(action == 0)
		pBind->Dim_ = count;
	else if(oneof3(action, -2, -1, +1)) {
		p_data = pStmt->GetBindOuterPtr(pBind, count);
	}
	pBind->NtvSize = static_cast<uint16>(sz); // default value
	const int  idx = abs(pBind->Pos);
	const int  t = GETSTYPE(pBind->Typ);
	const uint s = GETSSIZE(pBind->Typ);
	bool  is_autoinc = false;
	switch(t) {
		case S_AUTOINC:
			is_autoinc = true;
			// @fallthrough
		case S_INT:
		case S_UINT:
		case S_INT64:
		case S_UINT64:
			if(action == 0) {
			}
			else if(action < 0) {
				if(s == 8) {
					if(is_autoinc && pStmt->Typ == Generator_SQL::typInsert && *static_cast<const sqlite3_int64 *>(p_data) == 0) {
						; // �� ����������� ������ ��� ��������������
					}
					else {
						sqlite3_bind_int64(h_stmt, idx, *static_cast<const sqlite3_int64 *>(p_data));
					}
				}
				else if(s == 4) {
					if(is_autoinc && pStmt->Typ == Generator_SQL::typInsert && *static_cast<const int *>(p_data) == 0) {
						; // �� ����������� ������ ��� ��������������
					}
					else {
						// @todo ����� ����������� ��������� ��� ������������ �������� ���� ���������� ������� ���
						sqlite3_bind_int(h_stmt, idx, *static_cast<const int *>(p_data));
					}
				}
				else if(s == 2) {
					if(is_autoinc && pStmt->Typ == Generator_SQL::typInsert && *static_cast<const int16 *>(p_data) == 0) {
						; // �� ����������� ������ ��� ��������������
					}
					else {
						// @todo ����� ����������� ��������� ��� ������������ �������� ���� ���������� ������� ���
						sqlite3_bind_int(h_stmt, idx, *static_cast<const int16 *>(p_data));
					}
				}
			}
			else if(action == 1) {
				if(s == 8) {
					*static_cast<int64 *>(p_data) = sqlite3_column_int64(h_stmt, idx-1); // @v12.3.11 @fix idx-->(idx-1)
				}
				else if(s == 4) {
					// @todo ����� ����������� ��������� ��� ������������ �������� //
					*static_cast<int *>(p_data) = sqlite3_column_int(h_stmt, idx-1); // @v12.3.11 @fix idx-->(idx-1)
				}
				else if(s == 2) {
					// @todo ����� ����������� ��������� ��� ������������ �������� //
					*static_cast<int16 *>(p_data) = sqlite3_column_int(h_stmt, idx-1); // @v12.3.11 @fix idx-->(idx-1)
				}
			}
			break;
		case S_ROWID:
			if(action == 0) {
			}
			else if(action < 0) {
				const DBRowId * p_row_id = static_cast<const DBRowId *>(p_data);
				if(p_row_id->IsI64()) {
					int64 i64val = p_row_id->GetI64();
					sqlite3_bind_int64(h_stmt, idx, i64val);
				}
				else if(p_row_id->IsI32()) {
					uint32 i32val = p_row_id->GetI32();
					sqlite3_bind_int(h_stmt, idx, i32val);
				}
				else {
					; // ������ �� ���� ��� � ����� �������� ������ :(
				}
			}
			else if(action == 1) {
				DBRowId * p_row_id = static_cast<DBRowId *>(p_data);
				int64 i64val = sqlite3_column_int64(h_stmt, idx-1); // @v12.3.11 @fix idx-->(idx-1)
				p_row_id->SetI64(i64val);
			}
			break;
		case S_FLOAT:
			if(action == 0) {
			}
			else if(action < 0) {
				assert(oneof2(s, 4, 8));
				if(s == 8) {
					sqlite3_bind_double(h_stmt, idx, *static_cast<const double *>(p_data));
				}
				else if(s == 4) {
					sqlite3_bind_double(h_stmt, idx, static_cast<double>(*static_cast<const float *>(p_data)));
				}
			}
			else if(action == 1) {
				assert(oneof2(s, 4, 8));
				if(s == 8) {
					*static_cast<double *>(p_data) = sqlite3_column_double(h_stmt, idx-1); // @v12.3.12 @fix idx-->(idx-1)
				}
				else if(s == 4) {
					*static_cast<float *>(p_data) = static_cast<float>(sqlite3_column_double(h_stmt, idx-1)); // @v12.3.11 @fix idx-->(idx-1)
				}
			}
			break;
		case S_DEC:
		case S_MONEY:
			if(action == 0) {
			}
			else {
				const int16 dec_len = static_cast<int16>(GETSSIZED(pBind->Typ));
				const int16 dec_prc = static_cast<int16>(GETSPRECD(pBind->Typ));
				if(action < 0) {
					double val = dectobin(static_cast<const char *>(p_data), dec_len, dec_prc);
					sqlite3_bind_double(h_stmt, idx, val);
				}
				else if(action == 1) {
					double val = sqlite3_column_double(h_stmt, idx-1);
					dectodec(val, static_cast<char *>(p_data), dec_len, dec_prc);
				}
			}
			break;
		case S_DATE:
			if(action == 0) {
			}
			else if(action < 0) {
				LDATE * p_dt = static_cast<LDATE *>(p_data);
				uint32 sqlt_val = p_dt ? p_dt->v : 0;
				sqlite3_bind_int(h_stmt, idx, *reinterpret_cast<const int *>(&sqlt_val));
			}
			else if(action == 1) {
				const int sqlt_val = sqlite3_column_int(h_stmt, idx-1);
				if(p_data)
					static_cast<LDATE *>(p_data)->v = static_cast<uint32>(sqlt_val);
			}
			break;
		case S_TIME:
			if(action == 0) {
			}
			else if(action < 0) {
				LTIME * p_tm = static_cast<LTIME *>(p_data);
				uint32 sqlt_val = p_tm ? p_tm->v : 0;
				sqlite3_bind_int(h_stmt, idx, *reinterpret_cast<const int *>(&sqlt_val));
			}
			else if(action == 1) {
				const int sqlt_val = sqlite3_column_int(h_stmt, idx-1);
				if(p_data)
					static_cast<LTIME *>(p_data)->v = static_cast<uint32>(sqlt_val);
			}
			break;
		case S_DATETIME:
			if(action == 0) {
			}
			else if(action < 0) {
				LDATETIME * p_dtm = static_cast<LDATETIME *>(p_data);
				SUniTime_Internal ut;
				if(p_dtm) {
					ut.SetDate(p_dtm->d);
					ut.SetTime(p_dtm->t);
				}
				uint64 sqlt_val = UED::_SetRaw_Time(UED_META_TIME_MSEC, ut);
				sqlite3_bind_int64(h_stmt, idx, *reinterpret_cast<const sqlite_int64 *>(&sqlt_val));
			}
			else if(action == 1) {
				const int64 sqlt_val = sqlite3_column_int64(h_stmt, idx-1);
				if(p_data) {
					SUniTime_Internal ut;
					if(!UED::_GetRaw_Time(sqlt_val, ut))
						static_cast<LDATETIME *>(p_data)->Z();
					else {
						ut.GetDate(&static_cast<LDATETIME *>(p_data)->d);
						ut.GetTime(&static_cast<LDATETIME *>(p_data)->t);
					}
				}
			}
			break;
		case S_ZSTRING:
			if(action == 0) {
			}
			else if(action < 0) {
				SString & r_temp_buf = SLS.AcquireRvlStr();
				(r_temp_buf = static_cast<const char *>(p_data)).Transf(CTRANSF_INNER_TO_UTF8);
				sqlite3_bind_text(h_stmt, idx, r_temp_buf.cptr(), r_temp_buf.Len(), SQLITE_TRANSIENT);
				//const int len = sstrlen(static_cast<const char *>(p_data));
				//sqlite3_bind_text(h_stmt, idx, static_cast<const char *>(p_data), len, SQLITE_STATIC);
			}
			else if(action == 1) {
				const int csz = sqlite3_column_bytes(h_stmt, idx-1);
				const uchar * p_outer_text = sqlite3_column_text(h_stmt, idx-1); // @v12.3.11 @fix idx-->(idx-1)
				SString & r_temp_buf = SLS.AcquireRvlStr();
				(r_temp_buf = reinterpret_cast<const char *>(p_outer_text)).Transf(CTRANSF_UTF8_TO_INNER);
				strnzcpy(static_cast<char *>(p_data), r_temp_buf, s);
			}
			break;
		case S_WCHAR:
			if(action == 0) {
			}
			else if(action < 0) {
				/*
					sqlite documentation:
					In those routines that have a fourth argument, its value is the number of bytes in the parameter. 
					To be clear: the value is the number of bytes in the value, not the number of characters. 				
				*/
				const int len = sstrlen(static_cast<const wchar_t *>(p_data)) * sizeof(size_t);
				sqlite3_bind_text16(h_stmt, idx, static_cast<const char *>(p_data), len, SQLITE_STATIC);
			}
			else if(action == 1) {
				const int csz = sqlite3_column_bytes(h_stmt, idx-1);
				const wchar_t * p_outer_text = static_cast<const wchar_t *>(sqlite3_column_text16(h_stmt, idx-1)); // @v12.3.11 @fix idx-->(idx-1)
				strnzcpy(static_cast<wchar_t *>(p_data), p_outer_text, s / sizeof(size_t));
			}
			break;
		case S_UUID_:
			if(action == 0) {
			}
			else if(action < 0) {
				S_GUID temp_guid;
				if(p_data) {
					temp_guid = *static_cast<const S_GUID *>(p_data);
				}
				sqlite3_bind_blob(h_stmt, idx, &temp_guid, sizeof(S_GUID), SQLITE_TRANSIENT);
			}
			else if(action == 1) {
				const int csz = sqlite3_column_bytes(h_stmt, idx-1);
				const void * p_sqlt_data = sqlite3_column_blob(h_stmt, idx-1);
				if(p_sqlt_data && csz == sizeof(S_GUID))
					*static_cast<S_GUID *>(p_data) = *static_cast<const S_GUID *>(p_sqlt_data);
				else
					static_cast<S_GUID *>(p_data)->Z();
			}
			break;
		case S_RAW:
			if(action == 0) {
			}
			else if(action < 0) {
				if(p_data) {
					sqlite3_bind_blob(h_stmt, idx, p_data, s, SQLITE_STATIC);
				}
			}
			else if(action == 1) {
				if(p_data) {
					const int csz = sqlite3_column_bytes(h_stmt, idx-1);
					const void * p_sqlt_data = sqlite3_column_blob(h_stmt, idx-1);
					if(p_sqlt_data) {
						memcpy(p_data, p_sqlt_data, smin(static_cast<uint>(csz), s));
					}
					else {
						memzero(p_data, s);
					}
				}
			}
			break;
		case S_BLOB:
			if(action == 0) {
			}
			else if(action < 0) {
				if(p_data) {
					DBLobBlock * p_lb = pStmt->GetBindingLob();
					size_t lob_sz = 0;
					if(p_lb) {
						p_lb->GetSize(labs(pBind->Pos)-1, &lob_sz);
						SLob * p_lob = static_cast<SLob *>(p_data);
						const void * p_lob_data = p_lob->GetRawDataPtrC();
						sqlite3_bind_blob(h_stmt, idx, p_lob_data, lob_sz, SQLITE_STATIC);
					}
				}
			}
			else if(action == 1) {
				if(p_data) {
					const int csz = sqlite3_column_bytes(h_stmt, idx-1);
					const void * p_sqlt_data = sqlite3_column_blob(h_stmt, idx-1);
					if(csz > 0) {
						DBLobBlock * p_lb = pStmt->GetBindingLob();
						if(p_lb) {
							p_lb->SetSize(labs(pBind->Pos)-1, static_cast<size_t>(csz));
						}
						SLob * p_lob = static_cast<SLob *>(p_data);
						p_lob->InitPtr(csz);
						void * p_lob_data = p_lob->GetRawDataPtr();
						memcpy(p_lob_data, p_sqlt_data, csz);
					}
				}
			}
			break;
		case S_CLOB:
		case S_NOTE:
			if(action == 0) {
			}
			else if(action < 0) {
				if(p_data) {
					SLob * p_lob = static_cast<SLob *>(p_data);
					const size_t lob_size = p_lob->GetPtrSize();
					const void * p_lob_data = p_lob->GetRawDataPtrC();
					if(p_lob_data) {
						SString & r_temp_buf = SLS.AcquireRvlStr();
						r_temp_buf.CatN(static_cast<const char *>(p_lob_data), lob_size).Transf(CTRANSF_INNER_TO_UTF8);
						sqlite3_bind_text(h_stmt, idx, r_temp_buf.cptr(), r_temp_buf.Len(), SQLITE_TRANSIENT);
					}
				}
			}
			else if(action == 1) {
				if(p_data) {
					const int csz = sqlite3_column_bytes(h_stmt, idx-1);
					const uchar * p_outer_text = sqlite3_column_text(h_stmt, idx-1); // @v12.3.11 @fix idx-->(idx-1)
					SString & r_temp_buf = SLS.AcquireRvlStr();
					(r_temp_buf = reinterpret_cast<const char *>(p_outer_text)).Transf(CTRANSF_UTF8_TO_INNER);
					strnzcpy(static_cast<char *>(p_data), r_temp_buf, s);
					{
						SLob * p_lob = static_cast<SLob *>(p_data);
						p_lob->InitPtr(csz);
						void * p_lob_data = p_lob->GetRawDataPtr();
						strnzcpy(static_cast<char *>(p_lob_data), r_temp_buf, csz);
					}
				}
			}
			break;
		case S_LOGICAL:
			break;
		case S_NUMERIC:
			break;
		case S_LSTRING:
			break;
		case S_LVAR:
			break;
		case S_BIT:
			break;
		case S_STS:
			break;
		case S_INTRANGE:
			break;
		case S_REALRANGE:
			break;
		case S_DATERANGE:
			break;
		case S_ARRAY:
			break;
		case S_STRUCT:
			break;
		case S_VARIANT:
			break;
		case S_IPOINT2:
			break;
		case S_FPOINT2:
			break;
		case S_WZSTRING:
			break;
		case S_COLOR_RGBA:
			break;
	}
	return ok;
}

/*virtual*/int SSqliteDbProvider::Describe(SSqlStmt & rS, SdRecord &)
{
	int    ok = 0;
	return ok;
}

/*virtual*/int SSqliteDbProvider::Fetch(SSqlStmt & rS, uint count, uint * pActualCount)
{
	int    ok = 0;
	uint   actual_count = 0;
	int    r = 0;
	do {
		r = sqlite3_step(StmtHandle(rS));
		if(r == SQLITE_ROW) {
			actual_count++;	
			ok = 1;
		}
		else if(r == SQLITE_DONE) {
			if(!ok) // ���� �� ����� ���� ������� ���� ���� ������ �� count, �� ��������� ������� 1.
				ok = -1;
		}
	} while(r == SQLITE_ROW && actual_count < count);
	ASSIGN_PTR(pActualCount, actual_count);
	return ok;
}