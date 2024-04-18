// DBSQL-SQLITE.CPP
// Copyright (c) A.Sobolev 2021, 2022, 2023, 2024
// Модуль интерфейса с DBMS Sqlite3
// @construction
//
#include <slib-internal.h>
#pragma hdrstop
#include <ued.h>
//#include <snet.h>
#include <..\osf\SQLite\sqlite3.h>

#ifndef NDEBUG
	#define DEBUG_LOG(msg) SLS.LogMessage("dbsqlite.log", msg, 0)
#else
	#define DEBUG_LOG(msg)
#endif

/*static*/bool SSqliteDbProvider::IsInitialized = false;

SSqliteDbProvider::SSqliteDbProvider() : DbProvider(DbDictionary::CreateInstance(0, 0), DbProvider::cSQL|DbProvider::cDbDependTa), SqlGen(sqlstSQLite, 0), H(0), Flags(0)
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
		// @todo
		ok = 0;
	}
	return ok;	
}

int SSqliteDbProvider::GetFileStat(const char * pFileName, long reqItems, DbTableStat * pStat)
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
	BNFieldList fld_list;
	struct SqliteTblEntry {
		char   type[128];
		char   name[128];
		char   tbl_name[128];
		int64  rootpage;
		char   sql[128];
	} rec_buf;
	MEMSZERO(rec_buf);
	char   name[128];
	STRNSCPY(name, pFileName);
	strlwr(name);
	fld_list.addField("type", MKSTYPE(S_ZSTRING, 128));
	fld_list.addField("name", MKSTYPE(S_ZSTRING, 128));
	fld_list.addField("tbl_name", MKSTYPE(S_ZSTRING, 128));
	fld_list.addField("rootpage", T_INT64);
	fld_list.addField("sql", MKSTYPE(S_ZSTRING, 128));
	SqlGen.Z().Select(&fld_list).From("sqlite_schema").Sp().Tok(Generator_SQL::tokWhere).Sp().Eq("name", name);
	SSqlStmt stmt(this, (const SString &)SqlGen);
	THROW(stmt.Exec(0, 0));
	THROW(stmt.BindData(+1, 1, fld_list, &rec_buf, 0));
	if(Fetch(stmt, 1, &actual) && actual) {
		THROW(stmt.GetData(0));
		ok = 1;
		if(pStat) {
			//pStat->NumRecs = rec_buf.NumRows;
			//SETFLAG(pStat->Flags, XTF_TEMP, rec_buf.Temp[0] == 'Y');
			//pStat->OwnerName = rec_buf.Owner;
			//pStat->SpaceName = rec_buf.TableSpace; // DbTableStat
			if(reqItems & DbTableStat::iFldList) {
			}
			if(reqItems & DbTableStat::iIdxList) {
			}
		}
	}
	CATCHZOK
	return ok;
}

/*virtual*/int SSqliteDbProvider::Login(const DbLoginBlock * pBlk, long options)
{
	int    ok = 1;
	sqlite3 * h = 0;
	SString path;
	assert(pBlk);
	THROW(pBlk);
	pBlk->GetAttr(DbLoginBlock::attrDbPath, path);
	{
		Logout();
		THROW(ProcessError(sqlite3_open(path, &h)))
		H = h;
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
	
/*virtual*/SString & SSqliteDbProvider::GetTemporaryFileName(SString & rFileNameBuf, long * pStart, int forceInDataPath)
{
	rFileNameBuf.Z();
	return rFileNameBuf;
}
	
/*virtual*/int SSqliteDbProvider::CreateDataFile(const DBTable * pTbl, const char * pFileName, int createMode, const char * pAltCode)
{
	int    ok = 1;
	// @construction {
	const int cm = RESET_CRM_TEMP(createMode);
	THROW(SqlGen.Z().CreateTable(*pTbl, 0, oneof2(cm, crmNoReplace, crmTTSNoReplace), 1));
	{
		SSqlStmt stmt(this, (const SString &)SqlGen);
		THROW(stmt.Exec(1, OCI_DEFAULT));
	}
	uint j;
	for(j = 0; j < pTbl->indexes.getNumKeys(); j++) {
		THROW(SqlGen.Z().CreateIndex(*pTbl, pFileName, j));
		{
			SSqlStmt stmt(this, (const SString &)SqlGen);
			THROW(stmt.Exec(1, OCI_DEFAULT));
		}
	}
	for(j = 0; j < pTbl->fields.getCount(); j++) {
		TYPEID _t = pTbl->fields[j].T;
		if(GETSTYPE(_t) == S_AUTOINC) {
			THROW(SqlGen.Z().CreateSequenceOnField(*pTbl, pFileName, j, 0));
			{
				SSqlStmt stmt(this, (const SString &)SqlGen);
				THROW(stmt.Exec(1, OCI_DEFAULT));
			}
		}
	}
	if(createMode < 0 && IS_CRM_TEMP(createMode)) {
		//
		// Регистрируем имя временного файла в драйвере БД для последующего удаления //
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
	uint   prep_flags = 0; // SQLITE_PREPARE_XXX
	const  char * p_ztail = 0;
	sqlite3_stmt * p_stmt = 0;
	THROW_S_S(!isempty(pText), SLERR_INVPARAM, __FUNCTION__"/pText");
	THROW(ProcessError(sqlite3_prepare_v3(static_cast<sqlite3 *>(H), pText, sstrleni(pText), prep_flags, &p_stmt, &p_ztail)));
	pS->H = p_stmt;
	/*
	OH h = OhAlloc(OCI_HTYPE_STMT);
	pS->H = h;
	THROW(ProcessError(OCIStmtPrepare(h, Err, reinterpret_cast<const OraText *>(pText), sstrlen(pText), OCI_NTV_SYNTAX, OCI_DEFAULT)));
	*/
	CATCHZOK
	return ok;
}

/*virtual*/int SSqliteDbProvider::DestroyStmt(SSqlStmt * pS)
{
	int   ok = 1;
	if(pS && pS->H) {
		int r = sqlite3_finalize(static_cast<sqlite3_stmt *>(pS->H));
		pS->H = 0;
		ok = ProcessError(r);
	}
	else
		ok = -1;
	return ok;
}

/*virtual*/int SSqliteDbProvider::ExecStmt(SSqlStmt & rS, uint count, int mode)
{
	int    ok = ProcessError(sqlite3_step(static_cast<sqlite3_stmt *>(rS.H)));
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
	return 0;
}

/*virtual*/int SSqliteDbProvider::CommitWork()
{
	return 0;
}

/*virtual*/int SSqliteDbProvider::RollbackWork()
{
	return 0;
}

/*virtual*/int SSqliteDbProvider::Implement_Open(DBTable* pTbl, const char* pFileName, int openMode, char* pPassword)
{
	return 0;
}

/*virtual*/int SSqliteDbProvider::Implement_Close(DBTable* pTbl)
{
	return 0;
}
/*virtual*/int SSqliteDbProvider::Implement_Search(DBTable* pTbl, int idx, void* pKey, int srchMode, long sf)
{
	return 0;
}
/*virtual*/int SSqliteDbProvider::Implement_InsertRec(DBTable* pTbl, int idx, void* pKeyBuf, const void* pData)
{
	return 0;
}
/*virtual*/int SSqliteDbProvider::Implement_UpdateRec(DBTable* pTbl, const void* pDataBuf, int ncc)
{
	return 0;
}
/*virtual*/int SSqliteDbProvider::Implement_DeleteRec(DBTable* pTbl)
{
	return 0;
}
/*virtual*/int SSqliteDbProvider::Implement_BExtInsert(BExtInsert* pBei)
{
	return 0;
}
/*virtual*/int SSqliteDbProvider::Implement_GetPosition(DBTable* pTbl, DBRowId* pPos)
{
	return 0;
}
/*virtual*/int SSqliteDbProvider::Implement_DeleteFrom(DBTable* pTbl, int useTa, DBQ& rQ)
{
	return 0;
}

/*virtual*/int SSqliteDbProvider::GetFileStat(DBTable * pTbl, long reqItems, DbTableStat * pStat)
{
	int    ok = 0;
	return ok;
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
	if(dir == -1) { // Входящие для SQL-запроса параметры (например для INSERT)
		for(uint i = 0; i < col_count; i++) {
			SSqlStmt::Bind & r_bind = rS.BL.at(i);
			if(r_bind.Pos < 0) {
				THROW(ProcessBinding(0, row_count, &rS, &r_bind));
				THROW(ProcessBinding(-1, row_count, &rS, &r_bind));
			}
		}
	}
	else if(dir == +1) { // Исходящие из SQL-запроса значения (например для SELECT)
		for(uint i = 0; i < col_count; i++) {
			SSqlStmt::Bind & r_bind = rS.BL.at(i);
			if(r_bind.Pos > 0) {
				THROW(ProcessBinding(0, row_count, &rS, &r_bind));
				THROW(ProcessBinding(dir, row_count, &rS, &r_bind));
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
	else if(pBind->Dim > 1) {
		if(action < 0)
			memcpy(pStmt->GetBindOuterPtr(pBind, count), pBind->P_Data, pBind->NtvSize);
		else if(action == 1)
			memcpy(pBind->P_Data, pStmt->GetBindOuterPtr(pBind, count), pBind->NtvSize);
	}
	return 1;
}
//
// ARG(action IN):
//   0 - инициализация структуры SSqlStmt::Bind
//   1 - извлечение данных из внешнего источника во внутренние буферы
//  -1 - перенос данных из внутренних буферов во внешний источник
//  1000 - разрушение специфичных для SQL-сервера элементов структуры SSqlStmt::Bind
//
/*virtual*/int SSqliteDbProvider::ProcessBinding(int action, uint count, SSqlStmt * pStmt, SSqlStmt::Bind * pBind)
{
	int    ok = 1;
	const  size_t sz = stsize(pBind->Typ);
	uint16 out_typ = 0;
	sqlite3_stmt * h_stmt = StmtHandle(*pStmt);
	void * p_data = pStmt->GetBindOuterPtr(pBind, 0);
	pBind->NtvSize = static_cast<uint16>(sz); // default value
	if(action == 0)
		pBind->Dim = count;
	const int idx = abs(pBind->Pos);
	const int t = GETSTYPE(pBind->Typ);
	const uint s = GETSSIZE(pBind->Typ);
	switch(t) {
		case S_INT:
		case S_AUTOINC:
		case S_INT64:
		case S_UINT64:
			if(action == 0) {
			}
			else if(action < 0) {
				if(s == 8)
					sqlite3_bind_int64(h_stmt, idx, *static_cast<const sqlite3_int64 *>(p_data));
				else if(s == 4) {
					sqlite3_bind_int(h_stmt, idx, *static_cast<const int *>(p_data));
				}
				else if(s == 2) {
					sqlite3_bind_int(h_stmt, idx, *static_cast<const int16 *>(p_data));
				}
			}
			else if(action == 1) {
				if(s == 8) {
					*static_cast<int64 *>(p_data) = sqlite3_column_int64(h_stmt, idx);
				}
				else if(s == 4) {
					*static_cast<int *>(p_data) = sqlite3_column_int(h_stmt, idx);
				}
				else if(s == 2) {
					*static_cast<int16 *>(p_data) = sqlite3_column_int(h_stmt, idx);
				}
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
					*static_cast<double *>(p_data) = sqlite3_column_double(h_stmt, idx);
				}
				else if(s == 4) {
					*static_cast<float *>(p_data) = sqlite3_column_double(h_stmt, idx);
				}
			}
			break;
		case S_DATE:
			if(action == 0) {
			}
			else if(action < 0) {
				LDATE * p_dt = static_cast<LDATE *>(p_data);
				SUniTime_Internal tmi(*p_dt);
				uint64 ued = UED::_SetRaw_Time(UED_META_DATE_DAY, tmi);
				sqlite3_bind_int64(h_stmt, idx, *reinterpret_cast<const sqlite3_int64 *>(&ued));
			}
			else if(action == 1) {
			}
			break;
		case S_TIME:
			if(action == 0) {
			}
			else if(action < 0) {
				LTIME * p_tm = static_cast<LTIME *>(p_data);
				SUniTime_Internal tmi(*p_tm);
				uint64 ued = UED::_SetRaw_Time(UED_META_DAYTIME_MS, tmi);
				sqlite3_bind_int64(h_stmt, idx, *reinterpret_cast<const sqlite3_int64 *>(&ued));
			}
			else if(action == 1) {
			}
			break;
		case S_DATETIME:
			break;
		case S_ZSTRING:
			if(action == 0) {
			}
			else if(action < 0) {
				const int len = sstrlen(static_cast<const char *>(p_data));
				sqlite3_bind_text(h_stmt, idx, static_cast<const char *>(p_data), len, SQLITE_STATIC);
			}
			else if(action == 1) {
				const uchar * p_outer_text = sqlite3_column_text(h_stmt, idx);
				strnzcpy(static_cast<char *>(p_data), p_outer_text, s);
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
				const wchar_t * p_outer_text = static_cast<const wchar_t *>(sqlite3_column_text16(h_stmt, idx));
				strnzcpy(static_cast<wchar_t *>(p_data), p_outer_text, s / sizeof(size_t));
			}
			break;
		case S_DEC:
			break;
		case S_MONEY:
			break;
		case S_LOGICAL:
			break;
		case S_NUMERIC:
			break;
		case S_LSTRING:
			break;
		case S_NOTE:
			break;
		case S_LVAR:
			break;
		case S_UBINARY:
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
		case S_BLOB:
			break;
		case S_CLOB:
			break;
		case S_RAW:
			break;
		case S_ROWID:
			break;
		case S_IPOINT2:
			break;
		case S_FPOINT2:
			break;
		case S_WZSTRING:
			break;
		case S_UUID_:
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
	return ok;
}