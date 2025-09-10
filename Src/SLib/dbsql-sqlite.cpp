// DBSQL-SQLITE.CPP
// Copyright (c) A.Sobolev 2021, 2022, 2023, 2024, 2025
// Модуль интерфейса с DBMS Sqlite3
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
		LastErr.Code = status;
		LastErr.Descr = sqlite3_errmsg(static_cast<sqlite3 *>(H));
		if(LastErr.Descr.NotEmpty()) {
			DBS.SetError(BE_SQLITE_TEXT, LastErr.Descr);
		}
		ok = 0;
	}
	return ok;	
}

int SSqliteDbProvider::GetFileStat(const char * pFileName/*регистр символов важен!*/, long reqItems, DbTableStat * pStat)
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
		char   sql[256];
	} rec_buf;
	MEMSZERO(rec_buf);
	char   name[128];
	STRNSCPY(name, pFileName); // Поиск чувствителен к регистру!
	SString temp_buf;
	fld_list.addField("type", MKSTYPE(S_ZSTRING, sizeof(rec_buf.type)));
	fld_list.addField("name", MKSTYPE(S_ZSTRING, sizeof(rec_buf.name)));
	fld_list.addField("tbl_name", MKSTYPE(S_ZSTRING, sizeof(rec_buf.tbl_name)));
	fld_list.addField("rootpage", T_INT64);
	fld_list.addField("sql", MKSTYPE(S_ZSTRING, sizeof(rec_buf.sql)));
	SqlGen.Z().Select(&fld_list).From("sqlite_schema").Sp().Tok(Generator_SQL::tokWhere).Sp().Eq("type", "table").Sp().Tok(Generator_SQL::tokAnd).Sp().Eq("name", name);
	SSqlStmt stmt(this, SqlGen);
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
	for(j = 0; j < pTbl->fields.getCount(); j++) {
		TYPEID _t = pTbl->fields[j].T;
		if(GETSTYPE(_t) == S_AUTOINC) {
			THROW(SqlGen.Z().CreateSequenceOnField(*pTbl, pFileName, j, 0));
			{
				SSqlStmt stmt(this, SqlGen);
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
	uint   prep_flags = SQLITE_PREPARE_NORMALIZE; // SQLITE_PREPARE_XXX
	const  char * p_ztail = 0;
	sqlite3_stmt * p_stmt = 0;
	THROW_S_S(!isempty(pText), SLERR_INVPARAM, __FUNCTION__"/pText");
	const int text_len = sstrleni(pText);
	THROW(ProcessError(sqlite3_prepare_v3(static_cast<sqlite3 *>(H), pText, text_len, prep_flags, &p_stmt, &p_ztail))); 
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
	SqlGen.Z().Tok(Generator_SQL::tokBegin).Sp().Tok(Generator_SQL::tokTransaction);
	char * p_err_msg = 0;
	int    ok = ProcessError(sqlite3_exec(static_cast<sqlite3 *>(H), static_cast<SString &>(SqlGen), 0, 0, &p_err_msg));
	return ok;
}

/*virtual*/int SSqliteDbProvider::CommitWork()
{
	SqlGen.Z().Tok(Generator_SQL::tokCommit).Sp().Tok(Generator_SQL::tokTransaction);
	char * p_err_msg = 0;
	int    ok = ProcessError(sqlite3_exec(static_cast<sqlite3 *>(H), static_cast<SString &>(SqlGen), 0, 0, &p_err_msg));
	return ok;
}

/*virtual*/int SSqliteDbProvider::RollbackWork()
{
	SqlGen.Z().Tok(Generator_SQL::tokRollback).Sp().Tok(Generator_SQL::tokTransaction);
	char * p_err_msg = 0;
	int    ok = ProcessError(sqlite3_exec(static_cast<sqlite3 *>(H), static_cast<SString &>(SqlGen), 0, 0, &p_err_msg));
	return ok;
}

/*virtual*/int SSqliteDbProvider::Implement_Open(DBTable* pTbl, const char* pFileName, int openMode, char* pPassword)
{
	int    ok = 1;
	if(pTbl) {
		pTbl->fileName = NZOR(pFileName, pTbl->tableName);
		pTbl->OpenedFileName = pTbl->fileName;
		pTbl->FixRecSize = pTbl->fields.CalculateRecSize();
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

/*virtual*/int SSqliteDbProvider::Implement_Search(DBTable * pTbl, int idx, void* pKey, int srchMode, long sf)
{
	//
	// Изначально код функции скопирован из SOraDbProvider::Implement_Search() с целью быстрой имплементации.
	// Вместо hint'ов здесь мы будем использовать расширение sql от SQLite "INDEXED BY"
	// ------------------------------------------------------------------------------------------------------
	//
	// BNKeyList BNFieldList BNKey Generator_SQL
	//
	// select /*+ index_asc(tbl_name index_name) */ * from
	//
	// В sqlite нет "for update" (вся база блокируется при изменении, ибо база не мультипользовательская)

	int    ok = 1;
	//
	// Если can_continue == 1, то допускается последующий запрос spNext или spPrev
	// Соответственно, stmt сохраняется в pTbl.
	//
	int    can_continue = 0;
	int    new_stmt = 0;
	uint   actual = 0;
	LongArray seg_map; // Карта номеров сегментов индекса, которые должны быть привязаны
	DBTable::SelectStmt * p_stmt = 0;
	const BNKeyList & r_indices = pTbl->indexes;
	THROW(idx < (int)r_indices.getNumKeys());
	if(!oneof2(srchMode, spNext, spPrev)) {
		const char * p_alias = "t";
		SString temp_buf;
		uint8  temp_key[1024];
		void * p_key_data = pKey;
		const  BNKey & r_key = r_indices[idx];
		const  int ns = r_key.getNumSeg();
		SqlGen.Z().Tok(Generator_SQL::tokSelect);
		if(!(sf & DBTable::sfDirect)) {
			/*
			SqlGen.HintBegin().
			HintIndex(*pTbl, p_alias, idx, BIN(oneof3(srchMode, spLt, spLe, spLast))).
			HintEnd();
			*/
		}
		SqlGen.Sp();
		SqlGen.Text(p_alias).Dot().Aster().Com().Text(p_alias).Dot().Tok(Generator_SQL::tokRowId);
		SqlGen.Sp().From(pTbl->fileName, p_alias);
		if(sf & DBTable::sfDirect) {
			DBRowId * p_rowid = static_cast<DBRowId *>(pKey);
			THROW(p_rowid && p_rowid->IsI32());
			p_rowid->ToStr__(temp_buf);
			SqlGen.Sp().Tok(Generator_SQL::tokWhere).Sp().Tok(Generator_SQL::tokRowId)._Symb(_EQ_).QText(temp_buf);
		}
		else {
			{
				if(SqlGen.GetIndexName(*pTbl, idx, temp_buf)) {
					SqlGen.Sp().Tok(Generator_SQL::tokIndexedBy).Sp().Text(temp_buf);
				}
			}
			if(oneof2(srchMode, spFirst, spLast)) {
				memzero(temp_key, sizeof(temp_key));
				r_indices.setBound(idx, 0, BIN(srchMode == spLast), temp_key);
				p_key_data = temp_key;
			}
			SqlGen.Sp().Tok(Generator_SQL::tokWhere).Sp();
			{
				for(int i = 0; i < ns; i++) {
					const BNField & r_fld = r_indices.field(idx, i);
					if(i > 0) { // НЕ первый сегмент
						SqlGen.Tok(Generator_SQL::tokAnd).Sp();
						if(srchMode != spEq)
							SqlGen.LPar();
					}
					if(r_key.getFlags(i) & XIF_ACS) {
						int   _func_tok = Generator_SQL::tokLower;
						SqlGen.Func(_func_tok, r_fld.Name);
					}
					else
						SqlGen.Text(r_fld.Name);
					int   cmps = _EQ_;
					if(srchMode == spEq)
						cmps = _EQ_;
					else if(srchMode == spLt)
						cmps = (i == ns-1) ? _LT_ : _LE_;
					else if(oneof2(srchMode, spLe, spLast))
						cmps = _LE_;
					else if(srchMode == spGt) {
						cmps = (i == ns-1) ? _GT_ : _GE_;
					}
					else if(oneof2(srchMode, spGe, spFirst))
						cmps = _GE_;
					SqlGen._Symb(cmps);
					SqlGen.Param(temp_buf.NumberToLat(i));
					seg_map.add(i);
					if(i > 0 && srchMode != spEq) {
						//
						// При каскадном сравнении ключа второй и последующие сегменты
						// должны удовлетворять условиям неравенства только при равенстве
						// всех предыдущих сегментов.
						//
						// Пример:
						//
						// index {X, Y, Z}
						// X > :A and (Y > :B or (X <> :A)) and (Z > :C or (X <> :A and Y <> :B))
						//
						SqlGen.Sp().Tok(Generator_SQL::tokOr).Sp().LPar();
						for(int j = 0; j < i; j++) {
							const BNField & r_fld2 = r_indices.field(idx, j);
							if(j > 0)
								SqlGen.Tok(Generator_SQL::tokAnd).Sp();
							if(r_key.getFlags(j) & XIF_ACS) {
								int   _func_tok = Generator_SQL::tokLower;
								SqlGen.Func(_func_tok, r_fld2.Name);
							}
							else
								SqlGen.Text(r_fld2.Name);
							SqlGen._Symb(_NE_);
							SqlGen.Param(temp_buf.NumberToLat(j));
							seg_map.add(j);
							SqlGen.Sp();
						}
						SqlGen.RPar().RPar().Sp();
					}
					SqlGen.Sp();
				}
			}
			if(oneof3(srchMode, spLe, spLt, spLast)) { // Обратное направление поиска
				SqlGen.Tok(Generator_SQL::tokOrderBy);
				for(int i = 0; i < ns; i++) {
					const BNField & r_fld = r_indices.field(idx, i);
					if(i)
						SqlGen.Com();
					SqlGen.Sp().Text(r_fld.Name).Sp().Tok(Generator_SQL::tokDesc);
				}
				SqlGen.Sp();
			}
			can_continue = 1;
		}
		if(sf & DBTable::sfForUpdate)
			SqlGen.Tok(Generator_SQL::tokFor).Sp().Tok(Generator_SQL::tokUpdate);
		{
			THROW(p_stmt = new DBTable::SelectStmt(this, SqlGen, idx, srchMode, sf));
			new_stmt = 1;
			THROW(p_stmt->IsValid());
			{
				size_t key_len = 0;
				p_stmt->BL.Dim = 1;
				for(uint i = 0; i < seg_map.getCount(); i++) {
					const int  seg = seg_map.get(i);
					const BNField & r_fld = r_indices.field(idx, seg);
					const size_t seg_offs = r_indices.getSegOffset(idx, seg);
					key_len += stsize(r_fld.T);
					if(r_key.getFlags(seg) & XIF_ACS) {
						strlwr866((char *)(PTR8(p_key_data) + seg_offs));
					}
					SSqlStmt::Bind b;
					b.Pos = -(int16)(i+1);
					b.Typ = r_fld.T;
					b.P_Data = PTR8(p_key_data) + seg_offs;
					uint   lp = 0;
					if(p_stmt->BL.lsearch(&b.Pos, &lp, PTR_CMPFUNC(int16)))
						p_stmt->BL.atFree(lp);
					p_stmt->BL.insert(&b);
				}
				memcpy(p_stmt->Key, pKey, MIN(sizeof(p_stmt->Key), key_len));
				THROW(Binding(*p_stmt, -1));
				THROW(p_stmt->SetData(0));
			}
			THROW(p_stmt->Exec(0, OCI_DEFAULT));
			if(!(sf & DBTable::sfForUpdate)) {
				int rowid_pos = pTbl->fields.getCount()+1;
				THROW(p_stmt->BindRowId(rowid_pos, 1, pTbl->getCurRowIdPtr()));
			}
			THROW(p_stmt->BindData(+1, 1, pTbl->fields, pTbl->getDataBufConst(), pTbl->getLobBlock()));
			THROW(ok = Helper_Fetch(pTbl, p_stmt, &actual));
			if(ok > 0)
				pTbl->copyBufToKey(idx, pKey);
			else {
				ok = 0;
				can_continue = 0;
			}
		}
	}
	else { // oneof2(srchMode, spNext, spPrev)
		p_stmt = pTbl->GetStmt();
		if(p_stmt) {
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
					can_continue = 1;
				else {
					BtrError = BE_EOF;
					ok = 0;
				}
			}
			else
				ok = 0;
		}
		else {
			BtrError = BE_EOF;
			ok = 0;
		}
	}
	CATCH
		ok = 0;
		can_continue = 0;
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

/*virtual*/int SSqliteDbProvider::Implement_InsertRec(DBTable * pTbl, int idx, void* pKeyBuf, const void * pData)
{
	// Для первого приближения функция скопирована из SOraDbProvider::Implement_InsertRec
	int    ok = 1;
	int    subst_no = 0;
	uint   i;
	int    do_process_lob = 0;
	int    map_ret_key = 0;
	BNKey  key;
	uint   ns = 0;
	const  uint fld_count = pTbl->fields.getCount();
	SString temp_buf;
	SString let_buf;
	SSqlStmt  stmt(this);
	if(pData)
		pTbl->copyBufFrom(pData);
	if(pTbl->State & DBTable::sHasLob) {
		int    r = 0;
		THROW(r = pTbl->StoreAndTrimLob());
		if(r > 0)
			do_process_lob = 1;
	}
	SqlGen.Z().Tok(Generator_SQL::tokInsert).Sp().Tok(Generator_SQL::tokInto).Sp().Text(pTbl->fileName).Sp();
	SqlGen.Tok(Generator_SQL::tokValues).Sp().LPar();
	stmt.BL.Dim = 1;
	stmt.BL.P_Lob = pTbl->getLobBlock();
	for(i = 0; i < fld_count; i++) {
		if(i)
			SqlGen.Com();
		SqlGen.Param(temp_buf.NumberToLat(subst_no++));
		const BNField & r_fld = pTbl->fields.getField(i);
		if(GETSTYPE(r_fld.T) == S_AUTOINC) {
			long   val = 0;
			size_t val_sz = 0;
			r_fld.getValue(pTbl->getDataBufConst(), &val, &val_sz);
			assert(val_sz == sizeof(val));
			if(val == 0) {
				// (oracle) THROW(GetAutolongVal(*pTbl, i, &val));
				r_fld.setValue(pTbl->getDataBuf(), &val);
			}
		}
		stmt.BindItem(-subst_no, 1, r_fld.T, PTR8(pTbl->getDataBuf()) + r_fld.Offs);
	}
	SqlGen.RPar();
	SqlGen.Sp().Tok(Generator_SQL::tokReturning).Sp().Tok(Generator_SQL::tokRowId);
	/*
	//
	// temp_buf будет содержать список переменных, в которые должны заносится возвращаемые значения //
	//
	let_buf.NumberToLat(subst_no++);
	temp_buf.Z().Colon().Cat(let_buf);
	stmt.BindRowId(-subst_no, 1, pTbl->getCurRowIdPtr());
	if(pKeyBuf && idx >= 0 && idx < static_cast<int>(pTbl->indexes.getNumKeys())) {
		map_ret_key = 1;
		key = pTbl->indexes[idx];
		ns = static_cast<uint>(key.getNumSeg());
		for(i = 0; i < ns; i++) {
			const BNField & r_fld = pTbl->indexes.field(idx, i);
			SqlGen.Com().Text(r_fld.Name);
			let_buf.NumberToLat(subst_no++);
			temp_buf.CatDiv(',', 0).Colon().Cat(let_buf);
			stmt.BindItem(-subst_no, 1, r_fld.T, PTR8(pKeyBuf)+pTbl->indexes.getSegOffset(idx, i));
		}
	}
	SqlGen.Sp().Tok(Generator_SQL::tokInto).Sp().Text(temp_buf);
	*/
	{
		THROW(stmt.SetSqlText(SqlGen));
		THROW(Binding(stmt, -1));
		THROW(stmt.SetDataDML(0));
		THROW(stmt.Exec(1, OCI_DEFAULT));
		THROW(stmt.GetOutData(0));
		if(do_process_lob) {
			//
			// Если в записи были не пустые значения LOB-полей, то придется перечитать
			// вставленную запись и изменить значения LOB-полей.
			//
			// @todo Надо обновлять только LOB-поля, а не всю запись.
			//
			DBRowId row_id = *pTbl->getCurRowIdPtr();
			THROW(Implement_Search(pTbl, -1, &row_id, spEq, DBTable::sfDirect | DBTable::sfForUpdate));
			THROW(pTbl->RestoreLob());
			THROW(Implement_UpdateRec(pTbl, 0, 0));
		}
	}
	CATCHZOK
	return ok;
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
	// (Эти 2 строчки должны быть перед вызовом pStmt->GetBindOuterPtr поскольку он полагается на поле pBind->Dim) {
	if(action == 0)
		pBind->Dim = count;
	// {
	void * p_data = pStmt->GetBindOuterPtr(pBind, 0);
	pBind->NtvSize = static_cast<uint16>(sz); // default value
	const int idx = abs(pBind->Pos);
	const int t = GETSTYPE(pBind->Typ);
	const uint s = GETSSIZE(pBind->Typ);
	switch(t) {
		case S_INT:
		case S_AUTOINC:
		case S_UINT:
		case S_INT64:
		case S_UINT64:
			if(action == 0) {
			}
			else if(action < 0) {
				if(s == 8) {
					sqlite3_bind_int64(h_stmt, idx, *static_cast<const sqlite3_int64 *>(p_data));
				}
				else if(s == 4) {
					// @todo Нужна специальная обработка для беззнакового значения если установлен старший бит
					sqlite3_bind_int(h_stmt, idx, *static_cast<const int *>(p_data));
				}
				else if(s == 2) {
					// @todo Нужна специальная обработка для беззнакового значения если установлен старший бит
					sqlite3_bind_int(h_stmt, idx, *static_cast<const int16 *>(p_data));
				}
			}
			else if(action == 1) {
				if(s == 8) {
					*static_cast<int64 *>(p_data) = sqlite3_column_int64(h_stmt, idx-1); // @v12.3.11 @fix idx-->(idx-1)
				}
				else if(s == 4) {
					// @todo Нужна специальная обработка для беззнакового значения //
					*static_cast<int *>(p_data) = sqlite3_column_int(h_stmt, idx-1); // @v12.3.11 @fix idx-->(idx-1)
				}
				else if(s == 2) {
					// @todo Нужна специальная обработка для беззнакового значения //
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
					; // Вообще не знаю что в такой ситуации делать :(
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
			ok = -1;
		}
	} while(r == SQLITE_ROW && actual_count < count);
	ASSIGN_PTR(pActualCount, actual_count);
	return ok;
}