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
	if(status != SQLITE_OK) {
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
	rBuf.Z();
	return rBuf;
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
	THROW(SqlGen.Z().CreateTable(*pTbl, 0));
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
	//TSVector <MYSQL_BIND> bind_list;
	assert(checkirange(row_count, 1U, 1024U));
	THROW(rS.SetupBindingSubstBuffer(dir, row_count));
	if(dir == -1) { // Входящие для SQL-запроса параметры (например для INSERT)
		for(uint i = 0; i < col_count; i++) {
			SSqlStmt::Bind & r_bind = rS.BL.at(i);
			if(r_bind.Pos < 0) {
				//OCIBind * p_bd = 0;
				//THROW(ProcessBinding(0, row_count, &rS, &r_bind));
				{
					void * p_data = rS.GetBindOuterPtr(&r_bind, 0);
					/*
						int sqlite3_bind_blob(sqlite3_stmt*, int, const void*, int n, void(*)(void*));
						int sqlite3_bind_blob64(sqlite3_stmt*, int, const void*, sqlite3_uint64, void(*)(void*));
						int sqlite3_bind_double(sqlite3_stmt*, int, double);
						int sqlite3_bind_int(sqlite3_stmt*, int, int);
						int sqlite3_bind_int64(sqlite3_stmt*, int, sqlite3_int64);
						int sqlite3_bind_null(sqlite3_stmt*, int);
						int sqlite3_bind_text(sqlite3_stmt*,int,const char*,int,void(*)(void*));
						int sqlite3_bind_text16(sqlite3_stmt*, int, const void*, int, void(*)(void*));
						int sqlite3_bind_text64(sqlite3_stmt*, int, const char*, sqlite3_uint64, void(*)(void*), unsigned char encoding);
						int sqlite3_bind_value(sqlite3_stmt*, int, const sqlite3_value*);
						int sqlite3_bind_pointer(sqlite3_stmt*, int, void*, const char*,void(*)(void*));
						int sqlite3_bind_zeroblob(sqlite3_stmt*, int, int n);
						int sqlite3_bind_zeroblob64(sqlite3_stmt*, int, sqlite3_uint64);
					*/
					const int t = GETSTYPE(r_bind.Typ);
					const uint s = GETSSIZE(r_bind.Typ);
					switch(t) {
						case S_INT:
							if(s == 8)
								sqlite3_bind_int64(h_stmt, i, *static_cast<const sqlite3_int64 *>(p_data));
							else {
								sqlite3_bind_int(h_stmt, i, *static_cast<const int *>(p_data));
							}
							break;
						case S_FLOAT:
							if(s == 8) {
								sqlite3_bind_double(h_stmt, i, *static_cast<const double *>(p_data));
							}
							else if(s == 4) {
								sqlite3_bind_double(h_stmt, i, static_cast<double>(*static_cast<const float *>(p_data)));
							}
							else {
								assert(0);
							}
							break;
						case S_DATE:
							{
								LDATE * p_dt = static_cast<LDATE *>(p_data);
								SUniTime_Internal tmi(*p_dt);
								uint64 ued = UED::_SetRaw_Time(UED_META_DATE_DAY, tmi);
								sqlite3_bind_int64(h_stmt, i, *reinterpret_cast<const sqlite3_int64 *>(&ued));
							}
							break;
						case S_TIME:
							{
								LDATE * p_dt = static_cast<LDATE *>(p_data);
								SUniTime_Internal tmi(*p_dt);
								uint64 ued = UED::_SetRaw_Time(UED_META_DATE_DAY, tmi);
								sqlite3_bind_int64(h_stmt, i, *reinterpret_cast<const sqlite3_int64 *>(&ued));
							}
							break;
						case S_ZSTRING:
							{
							}
							break;
						case S_NOTE:
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
						case S_LVAR:
							break;
						case S_UBINARY:
							break;
						case S_AUTOINC:
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
						case S_DATETIME:
							break;
						case S_ARRAY:
							break;
						case S_STRUCT:
							break;
						case S_VARIANT:
							break;
						case S_WCHAR:
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
						case S_INT64:
							break;
						case S_UINT64:
							break;
						case S_COLOR_RGBA:
							break;
					}
					//MYSQL_BIND bind_item;
					//MEMSZERO(bind_item);
					//bind_item.buffer_type = static_cast<enum enum_field_types>(r_bind.NtvTyp);
					//bind_item.buffer = p_data;
					//bind_item.buffer_length = r_bind.NtvSize;
					//bind_list.insert(&bind_item);
					#if 0 // {
					{
						uint16 * p_ind = r_bind.IndPos ? reinterpret_cast<uint16 *>(rS.BS.P_Buf + r_bind.IndPos) : 0;
						THROW(ProcessError(OCIBindByPos(h_stmt, &p_bd, Err, -r_bind.Pos, p_data, r_bind.NtvSize, r_bind.NtvTyp,
							p_ind, 0/*alenp*/, 0/*rcodep*/, 0/*maxarr_len*/, 0/*curelep*/, OCI_DEFAULT)));
						r_bind.H = reinterpret_cast<uint32>(p_bd);
					}
					if(row_count > 1) {
						THROW(ProcessError(OCIBindArrayOfStruct(p_bd, Err, r_bind.ItemSize, sizeof(uint16), 0, 0)));
					}
					#endif // } 0
				}
			}
		}
		//if(bind_list.getCount())
			//bind_result = mysql_stmt_bind_param(static_cast<MYSQL_STMT *>(rS.H), static_cast<MYSQL_BIND *>(bind_list.dataPtr()));
	}
	else if(dir == +1) { // Исходящие из SQL-запроса значения (например для SELECT)
		for(uint i = 0; i < col_count; i++) {
			SSqlStmt::Bind & r_bind = rS.BL.at(i);
			if(r_bind.Pos > 0) {
				//OCIDefine * p_bd = 0;
				THROW(ProcessBinding(0, row_count, &rS, &r_bind));
				{
					void * p_data = rS.GetBindOuterPtr(&r_bind, 0);
					//MYSQL_BIND bind_item;
					//MEMSZERO(bind_item);
					//bind_item.buffer_type = static_cast<enum_field_types>(r_bind.NtvTyp);
					//bind_item.buffer_length = r_bind.NtvSize;
					//bind_item.buffer = p_data;
					//bind_item.is_null
					//bind_item.length
					//bind_item.error
					//bind_list.insert(&bind_item);
					#if 0 // {
					{
						uint16 * p_ind = r_bind.IndPos ? reinterpret_cast<uint16 *>(rS.BS.P_Buf + r_bind.IndPos) : 0;
						uint16 * p_fsl = r_bind.FslPos ? reinterpret_cast<uint16 *>(rS.BS.P_Buf + r_bind.FslPos) : 0;
						THROW(ProcessError(OCIDefineByPos(h_stmt, &p_bd, Err, r_bind.Pos, p_data, r_bind.NtvSize, r_bind.NtvTyp, p_ind, p_fsl, 0, OCI_DEFAULT)));
						r_bind.H = (uint32)p_bd;
					}
					if(row_count > 1) {
						THROW(ProcessError(OCIDefineArrayOfStruct(p_bd, Err, r_bind.ItemSize, sizeof(uint16), sizeof(uint16), 0)));
					}
					#endif // } 0
				}
			}
		}
		//if(bind_list.getCount())
			//mysql_stmt_bind_result(static_cast<MYSQL_STMT *>(rS.H), &bind_list.at(0));
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
				else {
					sqlite3_bind_int(h_stmt, idx, *static_cast<const int *>(p_data));
				}
			}
			else { // action > 0
			}
			break;
		case S_FLOAT:
			if(action == 0) {
			}
			else if(action < 0) {
				if(s == 8) {
					sqlite3_bind_double(h_stmt, idx, *static_cast<const double *>(p_data));
				}
				else if(s == 4) {
					sqlite3_bind_double(h_stmt, idx, static_cast<double>(*static_cast<const float *>(p_data)));
				}
				else {
					assert(0);
				}
			}
			else { // action > 0
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
			else { // action > 0
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
			else { // action > 0
			}
			break;
		case S_DATETIME:
			break;
		case S_ZSTRING:
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
		case S_WCHAR:
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
	//MYSQL_BIND bind_item;
	//MEMSZERO(bind_item);
	//bind_item.buffer_type = static_cast<enum enum_field_types>(r_bind.NtvTyp);
	//bind_item.buffer = p_data;
	//bind_item.buffer_length = r_bind.NtvSize;
	//bind_list.insert(&bind_item);
	#if 0 // {
	{
		uint16 * p_ind = r_bind.IndPos ? reinterpret_cast<uint16 *>(rS.BS.P_Buf + r_bind.IndPos) : 0;
		THROW(ProcessError(OCIBindByPos(h_stmt, &p_bd, Err, -r_bind.Pos, p_data, r_bind.NtvSize, r_bind.NtvTyp,
			p_ind, 0/*alenp*/, 0/*rcodep*/, 0/*maxarr_len*/, 0/*curelep*/, OCI_DEFAULT)));
		r_bind.H = reinterpret_cast<uint32>(p_bd);
	}
	if(row_count > 1) {
		THROW(ProcessError(OCIBindArrayOfStruct(p_bd, Err, r_bind.ItemSize, sizeof(uint16), 0, 0)));
	}
	#endif // } 0
#if 0 // {
	const  size_t sz = stsize(pBind->Typ);
	uint16 out_typ = 0;
	pBind->NtvSize = static_cast<uint16>(sz); // default value
	if(action == 0)
		pBind->Dim = count;
	const int t = GETSTYPE(pBind->Typ);
	switch(t) {
		case S_CHAR: ProcessBinding_SimpleType(action, count, pStmt, pBind, datatypeInt); break;
		case S_INT:
		case S_AUTOINC: ProcessBinding_SimpleType(action, count, pStmt, pBind, datatypeInt); break;
		case S_UINT: ProcessBinding_SimpleType(action, count, pStmt, pBind, datatypeInt/*signed*/); break;
		case S_INT64: ProcessBinding_SimpleType(action, count, pStmt, pBind, datatypeInt); break;
		case S_FLOAT: ProcessBinding_SimpleType(action, count, pStmt, pBind, datatypeReal); break;
		case S_DATE:
			/*
				static bool   UED::_GetRaw_Time(uint64 ued, SUniTime_Internal & rT);
				static uint64 UED::_SetRaw_Time(uint64 meta, const SUniTime_Internal & rT);
			*/
			if(action == 0) {
				pBind->SetNtvTypeAndSize(datatypeInt, sizeof(uint64));
				pStmt->AllocBindSubst(count, pBind->NtvSize, pBind);
			}
			else if(action < 0) {
				LDATE * p_dt = static_cast<LDATE *>(pBind->P_Data);
				SUniTime_Internal tmi(*p_dt);
				uint64 ued = UED::_SetRaw_Time(UED_META_DATE_DAY, tmi);
				uint64 * p_dbs_data = static_cast<uint64 *>(pStmt->GetBindOuterPtr(pBind, count));
				*p_dbs_data = ued;
			}
			else if(action == 1) {
				const uint64 * p_dbs_data = static_cast<uint64 *>(pStmt->GetBindOuterPtr(pBind, count));
				SUniTime_Internal tmi;
				UED::_GetRaw_Time(*p_dbs_data, tmi);
				LDATE * p_dt = static_cast<LDATE *>(pBind->P_Data);
				tmi.GetDate(p_dt);
			}
			else if(action == 1000)
				ProcessBinding_FreeDescr(count, pStmt, pBind);
			break;
		case S_TIME:
			if(action == 0) {
				pBind->SetNtvTypeAndSize(MYSQL_TYPE_TIME, sizeof(MYSQL_TIME));
				pStmt->AllocBindSubst(count, pBind->NtvSize, pBind);
			}
			else if(action < 0) {
				MYSQL_TIME * p_ocidt = static_cast<MYSQL_TIME *>(pStmt->GetBindOuterPtr(pBind, count));
				LTIME * p_dt = static_cast<LTIME *>(pBind->P_Data);
				memzero(p_ocidt, sizeof(*p_ocidt));
				p_ocidt->hour = p_dt->hour();
				p_ocidt->minute = p_dt->minut();
				p_ocidt->second = p_dt->sec();
				p_ocidt->second_part = 0;
				p_ocidt->time_type = MYSQL_TIMESTAMP_TIME;
			}
			else if(action == 1) {
				const MYSQL_TIME * p_ocidt = static_cast<const MYSQL_TIME *>(pStmt->GetBindOuterPtr(pBind, count));
				*static_cast<LTIME *>(pBind->P_Data) = encodetime(p_ocidt->hour, p_ocidt->minute, p_ocidt->second, 0/*p_ocidt->second_part*/);
			}
			else if(action == 1000)
				ProcessBinding_FreeDescr(count, pStmt, pBind);
			break;
		case S_DATETIME:
			if(action == 0) {
				pBind->SetNtvTypeAndSize(MYSQL_TYPE_DATETIME, sizeof(MYSQL_TIME));
				pStmt->AllocBindSubst(count, pBind->NtvSize, pBind);
			}
			else if(action < 0) {
				MYSQL_TIME * p_ocidt = static_cast<MYSQL_TIME *>(pStmt->GetBindOuterPtr(pBind, count));
				LDATETIME * p_dt = static_cast<LDATETIME *>(pBind->P_Data);
				memzero(p_ocidt, sizeof(*p_ocidt));
				p_ocidt->year = p_dt->d.year();
				p_ocidt->month = p_dt->d.month();
				p_ocidt->day = p_dt->d.day();
				p_ocidt->hour = p_dt->t.hour();
				p_ocidt->minute = p_dt->t.minut();
				p_ocidt->second = p_dt->t.sec();
				p_ocidt->second_part = 0;
				p_ocidt->time_type = MYSQL_TIMESTAMP_DATETIME;
			}
			else if(action == 1) {
				const MYSQL_TIME * p_ocidt = static_cast<const MYSQL_TIME *>(pStmt->GetBindOuterPtr(pBind, count));
				static_cast<LDATETIME *>(pBind->P_Data)->d = encodedate(p_ocidt->day, p_ocidt->month, p_ocidt->year);
				static_cast<LDATETIME *>(pBind->P_Data)->t = encodetime(p_ocidt->hour, p_ocidt->minute, p_ocidt->second, 0/*p_ocidt->second_part*/);
			}
			else if(action == 1000)
				ProcessBinding_FreeDescr(count, pStmt, pBind);
			break;
		case S_NOTE:
		case S_ZSTRING:
			if(action == 0) {
				pBind->SetNtvTypeAndSize(MYSQL_TYPE_STRING, static_cast<uint16>(sz));
				pStmt->AllocBindSubst(count, pBind->NtvSize, pBind);
			}
			else if(action < 0) {
				char * p_outer = static_cast<char *>(pStmt->GetBindOuterPtr(pBind, count));
				//
				// 1. Необходимо защититься от ситуации, когда в конце буфера отсутствует '\0'
				// 2. Необходимо конвертировать OEM кодировку (используется в btrieve данных и
				//    в проекте в целом) в CHAR кодировку, которая используется для хранения строк
				//    в SQL-базах.
				// 3. Пустая строка для критериев запроса извлечения данных должна быть представлена единственным пробелом.
				//
				if(PTR8(pBind->P_Data)[0] == 0) {
					if(action == -1) {
						p_outer[0] = ' ';
						p_outer[1] = 0;
					}
					else
						p_outer[0] = 0;
				}
				else {
					//
					// Особый случай: все элементы заполнены символами 255.
					// Это - максимальное значение, используемое в сравнениях.
					// Его не следует преобразовывать функцией SOemToChar
					//
					int    is_max = 0;
					if(PTR8C(pBind->P_Data)[0] == 255) {
						is_max = 1;
						for(uint k = 1; k < (sz-1); k++)
							if(PTR8C(pBind->P_Data)[k] != 255) {
								is_max = 0;
								break;
							}
					}
					strnzcpy(p_outer, static_cast<const char *>(pBind->P_Data), sz);
					if(!is_max)
						SOemToChar(p_outer);
				}
				/*
				const size_t len = sstrlen(p_outer);
				if(len < sz-1) {
					memset(p_outer + len, ' ', sz - len);
					p_outer[sz-1] = 0;
				}
				*/
			}
			else if(action == 1) {
				const int16 * p_ind = static_cast<const int16 *>(pStmt->GetIndPtr(pBind, count));
				if(p_ind && *p_ind == -1) {
					PTR8(pBind->P_Data)[0] = 0;
				}
				else {
					CharToOemA(static_cast<char *>(pStmt->GetBindOuterPtr(pBind, count)), static_cast<char *>(pBind->P_Data)); // @unicodeproblem
					trimright(static_cast<char *>(pBind->P_Data));
				}
			}
			break;
#if 0 // {
		case S_BLOB:
		case S_CLOB:
			if(action == 0)
				ProcessBinding_AllocDescr(count, pStmt, pBind, MYSQL_TYPE_BLOB, OCI_DTYPE_LOB);
			else if(action < 0) {
				OD ocilob = *static_cast<const OD *>(pStmt->GetBindOuterPtr(pBind, count));
				DBLobBlock * p_lob = pStmt->GetBindingLob();
				size_t lob_sz = 0;
				uint32 lob_loc = 0; // @x64crit
				if(p_lob) {
					p_lob->GetSize(labs(pBind->Pos)-1, &lob_sz);
					p_lob->GetLocator(labs(pBind->Pos)-1, &lob_loc); // @x64crit
					SETIFZ(lob_loc, (uint32)OdAlloc(OCI_DTYPE_LOB).H); // @x64crit
					ProcessError(OCILobAssign(Env, Err, (const OCILobLocator *)(lob_loc), reinterpret_cast<OCILobLocator **>(&ocilob.H)));
				}
				LobWrite(ocilob, pBind->Typ, static_cast<SLob *>(pBind->P_Data), lob_sz);
			}
			else if(action == 1) {
				OD ocilob = *static_cast<const OD *>(pStmt->GetBindOuterPtr(pBind, count));
				DBLobBlock * p_lob = pStmt->GetBindingLob();
				size_t lob_sz = 0;
				uint32 lob_loc = 0;
				LobRead(ocilob, pBind->Typ, static_cast<SLob *>(pBind->P_Data), &lob_sz);
				if(p_lob) {
					SETIFZ(lob_loc, (uint32)OdAlloc(OCI_DTYPE_LOB).H);
					ProcessError(OCILobAssign(Env, Err, ocilob, (OCILobLocator **)&lob_loc));
					p_lob->SetSize(labs(pBind->Pos)-1, lob_sz);
					p_lob->SetLocator(labs(pBind->Pos)-1, lob_loc);
				}
			}
			else if(action == 1000)
				ProcessBinding_FreeDescr(count, pStmt, pBind);
			break;
		case S_DEC:
		case S_MONEY:
			if(action == 0) {
				pBind->SetNtvTypeAndSize(SQLT_FLT, sizeof(double));
				pStmt->AllocBindSubst(count, pBind->NtvSize, pBind);
			}
			else {
				const int16 dec_len = static_cast<int16>(GETSSIZED(pBind->Typ));
				const int16 dec_prc = static_cast<int16>(GETSPRECD(pBind->Typ));
				if(action < 0)
					*static_cast<double *>(pStmt->GetBindOuterPtr(pBind, count)) = dectobin(static_cast<const char *>(pBind->P_Data), dec_len, dec_prc);
				else if(action == 1)
					dectodec(*static_cast<double *>(pStmt->GetBindOuterPtr(pBind, count)), static_cast<char *>(pBind->P_Data), dec_len, dec_prc);
			}
			break;
		case S_RAW:
			if(action == 0) {
				pBind->SetNtvTypeAndSize(SQLT_BIN, static_cast<uint16>(sz));
				pStmt->AllocBindSubst(count, (sz * 2), pBind);
			}
			else if(action < 0) {
				uint16 * p_outer = static_cast<uint16 *>(pStmt->GetBindOuterPtr(pBind, count));
				memcpy(p_outer, pBind->P_Data, sz);
				/*
				for(uint i = 0; i < sz; i++)
					p_outer[i] = byte2hex(PTR8(pBind->P_Data)[i]);
				*/
			}
			else if(action == 1) {
				const uint16 * p_outer = static_cast<const uint16 *>(pStmt->GetBindOuterPtr(pBind, count));
				memcpy(pBind->P_Data, p_outer, sz);
				/*
				for(uint i = 0; i < sz; i++)
					PTR8(pBind->P_Data)[i] = hex2byte(p_outer[i]);
				*/
			}
			break;
		case S_ROWID:
			if(action == 0)
				ProcessBinding_AllocDescr(count, pStmt, pBind, SQLT_RDD, OCI_DTYPE_ROWID);
			else if(action < 0) {
				//assert(0);
			}
			else if(action == 1) {
				OD ocirid = *static_cast<const OD *>(pStmt->GetBindOuterPtr(pBind, count));
				uint16 len = sizeof(DBRowId);
				OCIRowidToChar(ocirid, static_cast<OraText *>(pBind->P_Data), &len, Err);
			}
			else if(action == 1000)
				ProcessBinding_FreeDescr(count, pStmt, pBind);
			break;
		//case S_LSTRING:
		//case S_WCHAR:
		//case S_LOGICAL:
		//case S_NUMERIC:
#endif // } 0
	}
	//
	// Распределяем пространство для переменных индикаторов и FSL.
	// При сигнале SSqlStmt::Bind::fCalcOnly ничего не делаем
	// поскольку функция SSqlStmt::SetupBindingSubstBuffer должна была
	// предусмотреть необходимость в пространстве для специальных значений.
	//
	if(action == 0 && !(pBind->Flags & SSqlStmt::Bind::fCalcOnly)) {
		pStmt->AllocIndSubst(count, pBind);
		if(pBind->Pos > 0)
			pStmt->AllocFslSubst(count, pBind);
	}
#endif // } 0
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