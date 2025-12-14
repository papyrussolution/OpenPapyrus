// DBSQL-MYSQL.CPP
// Copyright (c) A.Sobolev 2020, 2021, 2022, 2023, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

#if 1 // @construction {

#include <mariadb-20251208/mysql.h>

#ifndef NDEBUG
	#define DEBUG_LOG(msg) SLS.LogMessage("dbmysql.log", msg, 0)
#else
	#define DEBUG_LOG(msg)
#endif

SMySqlDbProvider::SMySqlDbProvider() :
	DbProvider(sqlstMySQL, cpUTF8, DbDictionary::CreateInstance(0, 0), DbProvider::cSQL|DbProvider::cDbDependTa), H(0), SqlGen(sqlstMySQL, 0), Flags(0)
{
}

SMySqlDbProvider::~SMySqlDbProvider()
{
	Logout();
}

int FASTCALL SMySqlDbProvider::ProcessError(int status)
{
	int    ok = 1;
	if(status) {
		LastErr.Code = status;
		LastErr.Descr = mysql_error(static_cast<MYSQL *>(H));
		if(LastErr.Descr.NotEmpty()) {
			DBS.SetError(BE_MYSQL_TEXT, LastErr.Descr);
		}
		ok = 0;
	}
	return ok;	
}

int SMySqlDbProvider::DropDatabase(const char * pDbName) // @v12.4.8
{
	//DROP DATABASE IF EXISTS `%s`
	int    ok = 1;
	if(isempty(pDbName)) {
		ok = -1;
	}
	else {
		SqlGen.Z().Tok(Generator_SQL::tokDrop).Sp().Tok(Generator_SQL::tokDatabase).Sp().Tok(Generator_SQL::tokIfExists).Sp().QbText(pDbName);
		SSqlStmt stmt(this, SqlGen);
		THROW(stmt.Exec(0, 0));
	}
	CATCHZOK
	return ok;
}

int SMySqlDbProvider::CreateDatabase(const char * pDbName)
{
	int    ok = 1;
	if(isempty(pDbName)) {
		ok = -1;
	}
	else {
		SqlGen.Z().Tok(Generator_SQL::tokCreate).Sp().Tok(Generator_SQL::tokDatabase).Sp().QbText(pDbName).
			Sp().Tok(Generator_SQL::tokCharacter).Sp().Tok(Generator_SQL::tokSet).Sp().Text("utf8mb4");
		SSqlStmt stmt(this, SqlGen);
		THROW(stmt.Exec(0, 0));
	}
	CATCHZOK
	return ok;
}

int SMySqlDbProvider::UseDatabase(const char * pDbName) // @v12.4.8
{
	int    ok = 1;
	if(isempty(pDbName)) {
		ok = -1;
	}
	else {
		MYSQL * p_h = static_cast<MYSQL *>(H);
		THROW(ProcessError(mysql_select_db(p_h, pDbName)));
		CurrentDatabase = pDbName;
		//SqlGen.Z().Tok(Generator_SQL::tokUse).Sp().QbText(pDbName);
		//SSqlStmt stmt(this, SqlGen);
		//THROW(stmt.Exec(0, 0));
	}
	CATCHZOK
	return ok;
}

/*virtual*/int SMySqlDbProvider::DbLogin(const DbLoginBlock * pBlk, long options)
{
	int    ok = 0;
	H = mysql_init(0);
	if(H) {
		MYSQL * p_h = static_cast<MYSQL *>(H);
		SString temp_buf;
		SString url_buf;
		SString db_name;
		pBlk->GetAttr(DbLoginBlock::attrDbName, db_name);
		pBlk->GetAttr(DbLoginBlock::attrServerUrl, url_buf);
		{
			int   port = 0;
			SString host;
			SString sid;
			SString user;
			SString pw;
			SString dbsymb;
			if(url_buf.NotEmpty()) {
				InetUrl url(url_buf);
				url.GetComponent(InetUrl::cHost, 1, host);
				url.GetComponent(InetUrl::cPort, 1, temp_buf);
				port = temp_buf.ToLong();
				SETIFZ(port, url.GetPort());
				SETIFZ(port, InetUrl::GetDefProtocolPort(InetUrl::prot_p_MYSQL));
			}
			else {
				StringSet ss(':', db_name);
				uint   ssp = 0;
				SString port_s;
				if(ss.get(&ssp, host)) {
					if(ss.get(&ssp, port_s)) {
						if(ss.get(&ssp, sid)) {
							port = port_s.ToULong();
						}
						else {
							sid = port_s;
							port = InetUrl::GetDefProtocolPort(InetUrl::prot_p_MYSQL);
						}
					}
					else {
						sid = host;
						host = "localhost";
						port = InetUrl::GetDefProtocolPort(InetUrl::prot_p_MYSQL);
					}
				}
			}
			if(db_name.IsEmpty()) {
				pBlk->GetAttr(DbLoginBlock::attrDbSymb, db_name);
			}
			pBlk->GetAttr(DbLoginBlock::attrUserName, user);
			pBlk->GetAttr(DbLoginBlock::attrPassword, pw);
			if(mysql_real_connect(p_h, host, user, pw, db_name, port, 0/*unix-socket*/, 0/*client_flags*/)) {
				CurrentDatabase = db_name;
				ok = 1;
			}
		}
	}
	return ok;
}

/*virtual*/int SMySqlDbProvider::Logout()
{
	int    ok = 0;
	if(H) {
		mysql_close(static_cast<MYSQL *>(H));
		H = 0;
		CurrentDatabase.Z();
		ok = 1;
	}
	return ok;
}

/*virtual*/int SMySqlDbProvider::ExecStmt(SSqlStmt & rS, uint count, int mode)
{
	int    ok = ProcessError(mysql_stmt_execute(static_cast<MYSQL_STMT *>(rS.H)));
#ifndef NDEBUG // {
	if(!ok) {
		SString log_buf;
		//log_buf.Cat("EXEC").CatDiv(':', 2).Cat(LastErrMsg);
		DEBUG_LOG(log_buf);
	}
#endif // } !NDEBUG
	return ok;
}

/*virtual*/int SMySqlDbProvider::PostProcess_LoadTableSpec(DBTable * pTbl) // @v12.4.8
{
	// @construction
	int    ok = -1;
	if(pTbl) {
		BNFieldList2 & r_fld_list = pTbl->GetFieldsNonConst();
		BNKeyList & r_indices = pTbl->GetIndicesNonConst();
		if(r_fld_list.getCount()) {
			bool   is_autoinc_in_idx = false;
			uint   autoinc_fld_id = _FFFFU;
			int    max_fld_id = 0;
			int    max_key_number = 0;
			for(uint ii = 0; ii < r_indices.getNumKeys(); ii++) {
				SETMAX(max_key_number, r_indices.getKey(ii).getKeyNumber());
			}
			for(uint fi = 0; fi < r_fld_list.getCount(); fi++) {
				const BNField & r_fld = r_fld_list.GetFieldByPosition(fi);
				SETMAX(max_fld_id, r_fld.Id);
				if(GETSTYPE(r_fld.T) == S_AUTOINC && GETSSIZE(r_fld.T) >= 4) {
					autoinc_fld_id = r_fld.Id;
				}
			}
			if(autoinc_fld_id != _FFFFU) {
				if(r_indices.GetKeyPosListByField(autoinc_fld_id, 0)) {
					is_autoinc_in_idx = true;
				}
				else {
					// 
				}
			}
			else {
				if(!r_fld_list.addField(SlConst::P_SurrogateRowIdFieldName, MKSTYPE(S_AUTOINC, 8), PrimaryAutoincFldId))
					ok = 0;
				else {
					/* Индекс создавать не надо - mysql автоматом создает индексы для autoinc-полей
					BNKey new_key;
					new_key.addSegment(new_fld_id, XIF_EXT);
					new_key.setKeyParams(max_key_number+1, 0);
					r_indices.addKey(new_key);
					*/
					ok = 1;
				}
			}
		}
	}
	return ok;
}

const BNField * SMySqlDbProvider::GetRowIdField(const DBTable * pTbl, uint * pFldPos) const // @v12.4.11
{
	const  BNField * p_result = 0;
	uint   fld_pos = 0;
	if(pTbl) {
		const BNFieldList2 & r_fl = pTbl->GetFields();
		for(uint i = 0; !p_result && i < r_fl.getCount(); i++) {
			const BNField & r_fld = r_fl.at(i);
			if(GETSTYPE(r_fld.T) == S_AUTOINC) {
				p_result = &r_fld;
				fld_pos = i;
			}
		}
	}
	ASSIGN_PTR(pFldPos, fld_pos);
	return p_result;
}

/*virtual*/int SMySqlDbProvider::CreateDataFile(const DBTable * pTbl, const char * pFileName, int createMode, const char * pAltCode)
{
	//
	// В MySQL (MariaDB) нет понятия rowid. В связи с этим будем поступать следующим образом:
	//   -- если таблица имеет поле ID (autoincrement unique) и существует индекс по этому единственному полю (primary index),
	//     то значение этого поля трактуем как rowid
	//   -- в противном случае, добавляем искусственное поле (int64 MsqRowId) в начале таблицы и дополнительный (последний) индекс с именем
	//     idx[tbl_name]MsqRowId (P_SurrogateRowIdFieldName)
	//
	int    ok = 1;
	const int cm = RESET_CRM_TEMP(createMode);
	uint   ctf = Generator_SQL::ctfIndent;
	if(oneof2(cm, crmNoReplace, crmTTSNoReplace))
		ctf |= Generator_SQL::ctfIfNotExists;
	THROW(SqlGen.Z().CreateTable(*pTbl, 0, ctf, 0/*pCollationSymb*/));
	{
		SSqlStmt stmt(this, SqlGen);
		THROW(stmt.Exec(1, OCI_DEFAULT));
	}
	{
		const BNKeyList & r_indices = pTbl->Indices;
		const uint nk = r_indices.getNumKeys();
		for(uint j = 0; j < nk; j++) {
			bool   do_skip_index = false;
			// Если индекс построен по autoinc-полю, то явно не создаем его - MySQL сам это сделает.
			// При этом у нас потом возникнут проблемы со ссылками на такие индексы.
			{
				LongArray index_pos_list;
				for(uint fi = 0; !do_skip_index && fi < pTbl->GetFields().getCount(); fi++) {
					const BNField & r_fld = pTbl->GetFields()[fi];
					if(GETSTYPE(r_fld.T) == S_AUTOINC) {
						if(r_indices.GetKeyPosListByField(r_fld.Id, &index_pos_list) && index_pos_list.lsearch(j)) {
							do_skip_index = true;
						}
					}
				}
			}
			if(!do_skip_index) {
				THROW(SqlGen.Z().CreateIndex(*pTbl, pFileName, j, 0/*pCollationSymb*/));
				{
					SSqlStmt stmt(this, SqlGen);
					THROW(stmt.Exec(1, OCI_DEFAULT));
				}
			}
		}
	}
	/* (В mysql нет sequence) {
		for(uint j = 0; j < pTbl->fields.getCount(); j++) {
			TYPEID _t = pTbl->fields[j].T;
			if(GETSTYPE(_t) == S_AUTOINC) {
				THROW(SqlGen.Z().CreateSequenceOnField(*pTbl, pFileName, j, 0));
				{
					SSqlStmt stmt(this, SqlGen);
					THROW(stmt.Exec(1, OCI_DEFAULT));
				}
			}
		}
	}*/
	if(createMode < 0 && IS_CRM_TEMP(createMode)) {
		//
		// Регистрируем имя временного файла в драйвере БД для последующего удаления //
		//
		AddTempFileName(pFileName);
	}
	CATCHZOK
	return ok;
}

/*virtual*/SString & SMySqlDbProvider::MakeFileName_(const char * pTblName, SString & rBuf)
{
	return rBuf.SetIfEmpty(pTblName);
}

/*virtual*/int SMySqlDbProvider::IsFileExists_(const char * pFileName)
{
	return BIN(GetFileStat(pFileName, 0, 0) > 0);
}

/*virtual*/SString & SMySqlDbProvider::GetTemporaryFileName(SString & rFileNameBuf, long * pStart, bool forceInDataPath)
{
	return rFileNameBuf.Z();
}

/*virtual*/int SMySqlDbProvider::DropFile(const char * pFileName)
{
	return 0;
}

/*virtual*/int SMySqlDbProvider::PostProcessAfterUndump(DBTable * pTbl)
{
	return 0;
}

/*virtual*/int SMySqlDbProvider::StartTransaction()
{
	int    ok = 0;
	if(H) {
		if(!(State & stTransaction)) {
			SqlGen.Z().Tok(Generator_SQL::tokStart).Sp().Tok(Generator_SQL::tokTransaction);
			if(ProcessError(mysql_query(static_cast<MYSQL *>(H), SqlGen.GetTextC()))) {
				ok = 1;
				State |= stTransaction;
			}
		}
		else
			ok = -1;
	}
	return ok;
}

/*virtual*/int SMySqlDbProvider::CommitWork()
{
	int    ok = 0;
	int    rbr = 0; // rollback-result
	if(H) {
		if(State & stTransaction) {
			SqlGen.Z().Tok(Generator_SQL::tokCommit);
			if(ProcessError(mysql_query(static_cast<MYSQL *>(H), SqlGen.GetTextC()))) {
				ok = 1;
			}
			else {
				rbr = RollbackWork();
			}
			State &= ~stTransaction;
		}
		else
			ok = -1;
	}
	return ok;
}

/*virtual*/int SMySqlDbProvider::RollbackWork()
{
	int    ok = 0;
	if(H) {
		if(State & stTransaction) {
			SqlGen.Z().Tok(Generator_SQL::tokRollback);
			if(ProcessError(mysql_query(static_cast<MYSQL *>(H), SqlGen.GetTextC()))) {
				ok = 1;
			}
			State &= ~stTransaction;
		}
		else
			ok = -1;
	}
	return ok;
}

/*virtual*/int SMySqlDbProvider::GetDatabaseState(const char * pDbName, uint * pStateFlags) // @construction
{
	int    ok = -1;
	uint   state = dbstNormal;
	if(isempty(pDbName)) {
		state |= dbstNotExists;
	}
	else {
		char    database_name[128];
		database_name[0];
		uint   actual = 0;
		BNFieldList2 fld_list;
		fld_list.addField("SCHEMATA", MKSTYPE(S_ZSTRING, 128));
		// SHOW DATABASES LIKE '%s'
		SqlGen.Z().Tok(Generator_SQL::tokShow).Sp().Tok(Generator_SQL::tokDatabases).Sp().Tok(Generator_SQL::tokLike).Sp().QText(pDbName);
		SSqlStmt stmt(this, SqlGen);
		THROW(stmt.Exec(0, 0));
		THROW(stmt.BindData(+1, 1, fld_list, database_name, 0));
		if(Fetch(stmt, 1, &actual) && actual) {
			THROW(stmt.GetData(0));
			ok = 1;
		}
		else {
			state |= dbstNotExists;
		}
	}
	CATCHZOK
	ASSIGN_PTR(pStateFlags, state);
	return ok;
}

int SMySqlDbProvider::GetFileStat(const char * pFileName, long reqItems, DbTableStat * pStat)
{
	int    ok = 0;
	uint   actual = 0;
	SString temp_buf;
	BNFieldList2 fld_list;
	struct MySqlTblEntry {
		char   TableCatalog[32];
		char   TableSchema[32];
		char   TableName[32];
		char   TableType[32];
		char   Engine[32];
		int32  Version;
		int64  NumRows;
		LDATETIME CrTm;
		LDATETIME UpdTm;
		LDATETIME CheckTm;
		char   Collation[32];
		char   Temp[8];
		//uint8  Ens_Padding[512]; // @debug
	} rec_buf;
	MEMSZERO(rec_buf);
	char   name[64];
	STRNSCPY(name, pFileName);
	strupr(name);
	fld_list.addField("TABLE_CATALOG", MKSTYPE(S_ZSTRING, 32));
	fld_list.addField("TABLE_SCHEMA", MKSTYPE(S_ZSTRING, 32));
	fld_list.addField("TABLE_NAME", MKSTYPE(S_ZSTRING, 32));
	fld_list.addField("TABLE_TYPE", MKSTYPE(S_ZSTRING, 32));
	fld_list.addField("ENGINE", MKSTYPE(S_ZSTRING, 32));
	fld_list.addField("VERSION", MKSTYPE(S_INT, 4));
	fld_list.addField("TABLE_ROWS", MKSTYPE(S_INT, 8));
	fld_list.addField("CREATE_TIME", MKSTYPE(S_DATETIME, 8));
	fld_list.addField("UPDATE_TIME", MKSTYPE(S_DATETIME, 8));
	fld_list.addField("CHECK_TIME", MKSTYPE(S_DATETIME, 8));
	fld_list.addField("TABLE_COLLATION", MKSTYPE(S_ZSTRING, 32));
	fld_list.addField("TEMPORARY", MKSTYPE(S_ZSTRING, 8));
	SqlGen.Z().Select(&fld_list).From("information_schema.tables").Sp().Tok(Generator_SQL::tokWhere).Sp().Eq("TABLE_NAME", name);
	if(CurrentDatabase.NotEmpty()) {
		(temp_buf = CurrentDatabase).ToUpper(); // @debug
		SqlGen.Sp().Tok(Generator_SQL::tokAnd).Sp().Eq("TABLE_SCHEMA", temp_buf);
	}
	SSqlStmt stmt(this, SqlGen);
	THROW(stmt.Exec(0, 0));
	THROW(stmt.BindData(+1, 1, fld_list, &rec_buf, 0));
	if(Fetch(stmt, 1, &actual) && actual) {
		THROW(stmt.GetData(0));
		ok = 1;
		if(pStat) {
			pStat->NumRecs = rec_buf.NumRows;
			SETFLAG(pStat->Flags, XTF_TEMP, rec_buf.Temp[0] == 'Y');
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

/*virtual*/int SMySqlDbProvider::GetFileStat(DBTable * pTbl, long reqItems, DbTableStat * pStat)
{
	// information_schema
	int    ok = 0;
	return ok;
}

/*virtual*/int SMySqlDbProvider::Implement_Open(DBTable * pTbl, const char * pFileName, int openMode, char * pPassword)
{
	pTbl->FileName_ = NZOR(pFileName, pTbl->tableName);
	pTbl->OpenedFileName = pTbl->FileName_;
	pTbl->FixRecSize = pTbl->FldL.CalculateFixedRecSize(0/*BNFieldList2::crsfXXX*/);
	return 1;
}

/*virtual*/int SMySqlDbProvider::Implement_Close(DBTable * pTbl)
{
	return 1;
}

int SMySqlDbProvider::Helper_Fetch(DBTable * pTbl, DBTable::SelectStmt * pStmt, uint * pActual)
{
	int    ok = 1;
	uint   actual = 0;
	THROW(ok = Fetch(*pStmt, 1, &actual));
	if(ok > 0) {
		THROW(pStmt->GetData(0));
		/*if(pStmt->Sf & DBTable::sfForUpdate) {
			SString temp_buf;
			OD     rowid = OdAlloc(OCI_DTYPE_ROWID);
			// (ora) THROW(OhAttrGet(StmtHandle(*pStmt), OCI_ATTR_ROWID, (OCIRowid *)rowid, 0));
			// (ora) RowidToStr(rowid, temp_buf);
			pTbl->getCurRowIdPtr()->FromStr__(temp_buf);
			OdFree(rowid);
		}*/
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

SMySqlDbProvider::SearchQueryBlock::SearchQueryBlock() : Flags(0), SrchMode(0), AutoincFldIdx(_FFFF32), P_KeyData(0), SqlG(sqlstSQLite, 0) // повторяет то же из SSqliteDbProvider
{
	memzero(TempKey, sizeof(TempKey));
}

SMySqlDbProvider::SearchQueryBlock & SMySqlDbProvider::SearchQueryBlock::Z() // повторяет то же из SSqliteDbProvider
{
	Flags = 0;
	SrchMode = 0;
	AutoincFldIdx = _FFFF32;
	P_KeyData = 0;
	SqlG.Z();
	memzero(TempKey, sizeof(TempKey));
	return *this;
}
//
// Скорпировано из SSqliteDbProvider::Helper_MakeSearchQuery
//
int SMySqlDbProvider::Helper_MakeSearchQuery(DBTable * pTbl, int idx, void * pKey, int srchMode, long sf, SearchQueryBlock & rBlk) 
{
	rBlk.Z();
	int    ok = 1;
	const char * p_collation = 0;//P_CollationSymb;
	const bool   use_collation_term = isempty(p_collation) ? false : true;
	SString temp_buf;
	int    subst_no = 0;
	//uint   autoinc_fld_id = _FFFF32;
	const  uint fld_count = pTbl->FldL.getCount();
	const  BNKeyList & r_indices = pTbl->Indices;
	const  BNKey & r_key = r_indices[idx];
	const  int  ns = r_key.getNumSeg();
	const  char * p_alias = "t";
	uint   rowid_fld_pos = 0;
	const  BNField * p_rowid_fld = GetRowIdField(pTbl, &rowid_fld_pos);
	rBlk.SrchMode = (srchMode == spNext) ? spGt : ((srchMode == spPrev) ? spLt : srchMode);
	rBlk.P_KeyData = pKey;
	rBlk.SqlG.Z().Tok(Generator_SQL::tokSelect);
	rBlk.SqlG.Sp();
	{
		//rBlk.SqlG.Text(p_alias).Dot().Aster().Com().Text(p_alias).Dot().Tok(Generator_SQL::tokRowId);
		//rBlk.SqlG.Text(p_alias).Dot().Aster();
		//
		for(uint i = 0; i < fld_count; i++) {
			const BNField & r_fld = pTbl->FldL.GetFieldByPosition(i);
			if(i)
				rBlk.SqlG.Com();
			// @v12.4.10 SqlGen.Param(temp_buf.NumberToLat(subst_no));
			rBlk.SqlG.Text(p_alias).Dot().Text(r_fld.Name); // @v12.4.10 
			subst_no++;
			const bool is_surrogate_rowid_field = sstreqi_ascii(r_fld.Name, SlConst::P_SurrogateRowIdFieldName);
			if(GETSTYPE(r_fld.T) == S_AUTOINC) {
				rBlk.AutoincFldIdx = i;
				if(is_surrogate_rowid_field)
					rBlk.Flags |= SearchQueryBlock::fAutoincFldIsSurrogate;
			}
			//stmt.BindItem(subst_no, 1, r_fld.T, is_surrogate_rowid_field ? static_cast<void *>(&row_id_zero) : PTR8(pTbl->getDataBuf()) + r_fld.Offs);
		}
	}
	rBlk.SqlG.Sp().From(pTbl->FileName_, p_alias);
	if(sf & DBTable::sfDirect) {
		DBRowId * p_rowid = static_cast<DBRowId *>(pKey);
		THROW(p_rowid && (p_rowid->IsI32() || p_rowid->IsI64())); // @todo @err
		p_rowid->ToStr__(temp_buf);
		THROW(p_rowid_fld); // @todo @err
		rBlk.SqlG.Sp().Tok(Generator_SQL::tokWhere).Sp().Text(p_rowid_fld->Name)._Symb(_EQ_).QText(temp_buf);
	}
	else {
		if(rBlk.SrchMode != spEq) { // Если условие EQ, то нет смысла мучить сервер требования работы по индексу - результат инвариантен
			if(rBlk.SqlG.GetIndexName(*pTbl, idx, temp_buf)) {
				rBlk.SqlG.Sp().Tok(Generator_SQL::tokForceIndex).LPar().Text(temp_buf).RPar();
			}
		}
		if(oneof2(rBlk.SrchMode, spFirst, spLast)) {
			r_indices.setBound(idx, 0, BIN(rBlk.SrchMode == spLast), rBlk.TempKey);
			rBlk.P_KeyData = rBlk.TempKey;
		}
		rBlk.SqlG.Sp().Tok(Generator_SQL::tokWhere).Sp();
		{
			auto cat_field_term = [](Generator_SQL & rSg, const BNKey & rKey, int segN, const BNField & rFld)
			{
				if(rKey.getFlags(segN) & XIF_ACS) {
					//int   _func_tok = Generator_SQL::tokLower;
					//rSg.Func(_func_tok, rFld.Name);
					rSg.Text(rFld.Name);
				}
				else
					rSg.Text(rFld.Name);
			};
			if(rBlk.SrchMode == spEq) {
				for(int i = 0; i < ns; i++) {
					const BNField & r_fld = r_indices.field(idx, i);
					if(i > 0) { // НЕ первый сегмент
						rBlk.SqlG.Sp().Tok(Generator_SQL::tokAnd).Sp();
					}
					cat_field_term(rBlk.SqlG, r_key, i, r_fld);
					rBlk.SqlG._Symb(_EQ_);
					//rBlk.SqlG.Param(temp_buf.NumberToLat(i));
					rBlk.SqlG.Text("?");
					if(use_collation_term && r_key.getFlags(i) & XIF_ACS) {
						rBlk.SqlG.Sp().Tok(Generator_SQL::tokCollate).Sp().Text(p_collation);
					}
					rBlk.SegMap.add(i);
				}
			}
			else if(oneof6(rBlk.SrchMode, spLt, spLe, spLast, spGt, spGe, spFirst)) {
				// 
				// {s1, s2, s3} <  {V1, V2, V3} => (s1 <= V1) AND (s1 < V1 OR s2 <= V2) AND (s1 < V1 OR s2 < V2 OR s3 < V3)
				// {s1, s2, s3} <= {V1, V2, V3} => (s1 <= V1) AND (s1 < V1 OR s2 <= V2) AND (s1 < V1 OR s2 < V2 OR s3 <= V3)
				// {s1, s2, s3} >  {V1, V2, V3} => (s1 >= V1) AND (s1 > V1 OR s2 >= V2) AND (s1 > V1 OR s2 > V2 OR s3 > V3)
				// {s1, s2, s3} >= {V1, V2, V3} => (s1 >= V1) AND (s1 > V1 OR s2 >= V2) AND (s1 > V1 OR s2 > V2 OR s3 >= V3)
				// [ Для справки ниже приведена идентичная логика, но объедиенная оператором OR. Я стал использовать вышеприведенные
				//   варианты из-за того, что SQLite неадекватно реагирует на OR-связку отказывасью применять тот индекс, который я указал.
				//   {s1, s2, s3} <  {V1, V2, V3} => (s1 < V1) OR (s1 = V1 AND s2 < V2) OR (s1 = V1 AND s2 = V2 AND s3 < V3)
				//   {s1, s2, s3} <= {V1, V2, V3} => (s1 < V1) OR (s1 = V1 AND s2 < V2) OR (s1 = V1 AND s2 = V2 AND s3 <= V3)
				//   {s1, s2, s3} >  {V1, V2, V3} => (s1 > V1) OR (s1 = V1 AND s2 > V2) OR (s1 = V1 AND s2 = V2 AND s3 > V3)
				//   {s1, s2, s3} >= {V1, V2, V3} => (s1 > V1) OR (s1 = V1 AND s2 > V2) OR (s1 = V1 AND s2 = V2 AND s3 >= V3)
				// ]
				//cmps = (i == ns-1) ? _LT_ : _LE_;
				for(int i = 0; i < ns; i++) {
					const BNField & r_fld = r_indices.field(idx, i);
					if(i) {
						rBlk.SqlG.Sp().Tok(Generator_SQL::tokAnd).Sp();
					}
					{
						rBlk.SqlG.LPar();
						if(i) {
							for(int j = 0; j < i; j++) {
								const BNField & r_fld_j = r_indices.field(idx, j); // J!
								if(j) {
									rBlk.SqlG.Sp().Tok(Generator_SQL::tokOr).Sp();
								}
								cat_field_term(rBlk.SqlG, r_key, j, r_fld_j); // J!
								{
									int   cmps = 0;
									switch(rBlk.SrchMode) {
										case spLt: 
										case spLe:
										case spLast: cmps = _LT_; break;
										case spGt: 
										case spGe:
										case spFirst: cmps = _GT_; break;
									}
									assert(cmps != 0);
									rBlk.SqlG._Symb(cmps);
								}
								//rBlk.SqlG.Param(temp_buf.NumberToLat(j)); // J!
								rBlk.SqlG.Text("?"); // J!
								if(use_collation_term && r_key.getFlags(j) & XIF_ACS) { // J!
									rBlk.SqlG.Sp().Tok(Generator_SQL::tokCollate).Sp().Text(p_collation);
								}
								rBlk.SegMap.add(j); // J!
							}
							rBlk.SqlG.Sp().Tok(Generator_SQL::tokOr).Sp();
						}
						{
							cat_field_term(rBlk.SqlG, r_key, i, r_fld); // I!
							{
								int   cmps = 0;
								switch(rBlk.SrchMode) {
									case spLt: cmps = (i == (ns-1)) ? _LT_ : _LE_; break; // !
									case spLe: cmps = _LE_; break;
									case spLast: cmps = _LE_; break;
									case spGt: cmps = (i == (ns-1)) ? _GT_ : _GE_; break;
									case spGe: cmps = _GE_; break;
									case spFirst: cmps = _GE_; break;
								}
								assert(cmps != 0);
								rBlk.SqlG._Symb(cmps);
							}
							//rBlk.SqlG.Param(temp_buf.NumberToLat(i)); // I!
							rBlk.SqlG.Text("?"); // I!
							if(use_collation_term && r_key.getFlags(i) & XIF_ACS) { // I!
								rBlk.SqlG.Sp().Tok(Generator_SQL::tokCollate).Sp().Text(p_collation);
							}
							rBlk.SegMap.add(i); // I!
							rBlk.SqlG.RPar();
						}
						//
						//rBlk.SegMap.add(i);
					}
				}
			}
		}
		if(oneof3(rBlk.SrchMode, spLe, spLt, spLast)) { // Обратное направление поиска
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
//
// Эта функция строго заточена на поиск по конкретному индексу. Это значит, что в результирующем запросе не будет 
// никаких иных критериев, кроме сегментов индеска. Небольшое исключение - direct-search: в этом случае поиск осуществляется по 
// заданному rowid но, опять же, в случае MySQL это все равно будет индекс.
//
/*virtual*/int SMySqlDbProvider::Implement_Search(DBTable * pTbl, int idx, void * pKey, int srchMode, long sf)
{
	//
	// Изначально код функции скопирован из SSqliteDbProvider::Implement_Search() с целью быстрой имплементации.
	//
	// select /*+ index_asc(tbl_name index_name) */ * from
	//
	// В sqlite нет "for update" (вся база блокируется при изменении, ибо база не мультипользовательская)

	int    ok = 1;
	int    can_continue = false; // Если can_continue == true, то допускается последующий запрос spNext или spPrev. Соответственно, stmt сохраняется в pTbl.
	bool   new_stmt = false;
	uint   actual = 0;
	bool   surrogate_rowid_tag = false;
	uint64 surrogate_rowid_value_to_read = 0;
	DBTable::SelectStmt * p_stmt = 0;
	const BNKeyList & r_indices = pTbl->Indices;
	const char * p_alias = "t";
	SString temp_buf;
	DBRowId * p_cur_row_id = pTbl->getCurRowIdPtr();
	const  BNKey & r_key = r_indices[idx];
	const  bool is_key_uniq = !(r_key.getFlags(idx) & (XIF_DUP|XIF_REPDUP));
	//
	// Следующий флаг используется вот для чего: если поступил запрос на получение очередной записи, но pTbl->GetStmt() == 0,
	// то, скорее всего, это означает, что изменилось направление перебора записей. В этом случае мы строим запрос заново,
	// опираясь на последнее полученное значение ключа, по которому идет перебор. Здесь все бы хорошо, но есть один нюанс:
	// если ключ неуникальный, то мы точно не знаем от какой именно из записей-дубликатов начинать обратное движение.
	// Тем не менее у нас есть подсказка - rowid последней полученной записи (той же, которой соответствует последнее полученное значение ключа).
	// Таким образом, в этой ситуации мы действуем по следующему плану:
	//   запрос формируем так, будто очередная запись должна быть получена по условию РАВЕНСТВА (а не <= или >=) последнему значения ключа,
	//   далее, перебираем одну запись за другой пока ключ РАВЕН заданному и среди перебранных записей пытаемся найти ту, у которой
	//   rowid равен нашему текущему. Если мы нашли такую запись - прекрасно - дальше уже работаем как обычно.
	//   Плохо если такая запись отсутствует (нет, не фантастика) - тогда мы будем вынуждены "пожертвовать" непрочитанными записями
	//   с текущим значением ключа (если такие, конечно, были) и вести себя так, будто индекс уникальный - то есть, запрос, черт побери,
	//   опять придется перестроить.
	//
	bool   do_grop_next_rec = false; // @v12.4.6 
	//
	bool   do_make_query = true;
	//
	THROW(idx < (int)r_indices.getNumKeys());
	if(oneof2(srchMode, spNext, spPrev)) {
		assert(!(sf & DBTable::sfDirect)); // В режиме spNext/spPrev флаг sfDirect - бессмысленный
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
			// Так как у нас есть ключ pKey и нам не следует отчаиваться и возвращать ошибку - перестраиваем запрос и дуем в том направлении,
			// в котором нас просят. Единственный скользкий момент - если ключ не уникальный, то нет уверенности на какой именно записи
			// среди дубликатов мы находимся. Но эту проблему мы будем решать после того, как решим главную.
			do_make_query = true; // Да! Сработало! 
			// Проблему неуникальных ключей будем решать на следующей фунпацу.
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
			if(pTbl->P_Stmt) {
				ZDELETE(pTbl->P_Stmt);
			}
			THROW(p_stmt = new DBTable::SelectStmt(this, sqb.SqlG, idx, sqb.SrchMode, sf));
			new_stmt = true;
			THROW(p_stmt->IsValid());
			{
				int    subst_no = 0;
				uint8 * p_tbl_data_buf = PTR8(pTbl->getDataBuf());
				for(uint i = 0; i < pTbl->FldL.getCount(); i++) {
					const BNField & r_fld = pTbl->FldL.GetFieldByPosition(i);
					subst_no++;
					const bool is_surrogate_rowid_field = sstreqi_ascii(r_fld.Name, SlConst::P_SurrogateRowIdFieldName);
					if(is_surrogate_rowid_field) {
						//static_cast<void *>(p_cur_row_id)
						assert(!surrogate_rowid_tag);
						surrogate_rowid_tag = true;
						p_stmt->BindItem(subst_no, 1, r_fld.T, &surrogate_rowid_value_to_read);
					}
					else {
						p_stmt->BindItem(subst_no, 1, r_fld.T, p_tbl_data_buf + r_fld.Offs);
					}
				}
				THROW(Binding(*p_stmt, +1));
			}
			{
				size_t key_len = 0;
				p_stmt->BL.Dim = 1;
				for(uint i = 0; i < sqb.SegMap.getCount(); i++) {
					const int  seg = sqb.SegMap.get(i);
					const BNField & r_fld = r_indices.field(idx, seg);
					const size_t seg_offs = r_indices.getSegOffset(idx, seg);
					key_len += stsize(r_fld.T);
					/* (это, вроде, не надо - я установил collation-func) if(r_key.getFlags(seg) & XIF_ACS) {
						strlwr866((char *)(PTR8(sqb.P_KeyData) + seg_offs));
					}*/
					SSqlStmt::Bind b;
					b.Pos = -static_cast<int16>(i+1);
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
			/*if(!(sf & DBTable::sfForUpdate))*/ { // rowid безусловно запрашивается (see above)
				//const  int rowid_pos = pTbl->FldL.getCount()+1;
				//THROW(p_stmt->BindRowId(rowid_pos, 1, p_cur_row_id));
			}
			//THROW(p_stmt->BindData(+1, 1, pTbl->FldL, pTbl->getDataBufConst(), pTbl->getLobBlock()));
			p_cur_row_id->Z(); // @debug
			pTbl->clearDataBuf(); // @debug
			THROW(ok = Helper_Fetch(pTbl, p_stmt, &actual));
			if(ok > 0) {
				if(surrogate_rowid_tag) {
					p_cur_row_id->SetI64(surrogate_rowid_value_to_read);
				}
				else {
					uint8  rowid_data[64];
					size_t rowid_data_size = 0;
					uint   rowid_fld_pos = 0;
					const  BNField * p_rowid_fld = GetRowIdField(pTbl, &rowid_fld_pos);
					THROW(p_rowid_fld); // @todo @err
					THROW(pTbl->getFieldValue(rowid_fld_pos, rowid_data, &rowid_data_size)); // @todo @err
					if(rowid_data_size == 4) {
						p_cur_row_id->SetI32(reinterpret_cast<uint32>(rowid_data));
					}
					else if(rowid_data_size == 8) {
						p_cur_row_id->SetI64(reinterpret_cast<int64>(rowid_data));
					}
					else {
						constexpr bool InvalidRowIdDataSize = false;
						assert(InvalidRowIdDataSize);
					}
				}
				pTbl->copyBufToKey(idx, pKey);
			}
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

/*virtual*/int SMySqlDbProvider::Implement_InsertRec(DBTable * pTbl, int idx, void * pKeyBuf, const void * pData)
{
	int    ok = 1;
	int    subst_no = 0;
	uint   i;
	int    do_process_lob = 0;
	int    map_ret_key = 0;
	BNKey  key;
	uint   ns = 0;
	const  uint fld_count = pTbl->FldL.getCount();
	uint   autoinc_fld_id = _FFFF32;
	uint64 row_id_zero = 0ULL;
	SString temp_buf;
	SString let_buf;
	SBinaryChunk ret_buf;
	SSqlStmt  stmt(this);
	if(pData)
		pTbl->CopyBufFrom(pData);
	/* @20251204 if(pTbl->State & DBTable::sHasLob) {
		int    r = 0;
		THROW(r = pTbl->StoreAndTrimLob());
		if(r > 0)
			do_process_lob = 1;
	}*/
	SqlGen.Z().Tok(Generator_SQL::tokInsert).Sp().Tok(Generator_SQL::tokInto).Sp().Text(pTbl->FileName_).Sp();
	SqlGen.Tok(Generator_SQL::tokValues).Sp().LPar();
	stmt.BL.Dim = 1;
	stmt.BL.P_Lob = pTbl->getLobBlock();
	for(i = 0; i < fld_count; i++) {
		if(i)
			SqlGen.Com();
		// @v12.4.10 SqlGen.Param(temp_buf.NumberToLat(subst_no));
		SqlGen.Text("?"); // @v12.4.10 
		subst_no++;
		const BNField & r_fld = pTbl->FldL.GetFieldByPosition(i);
		const bool is_surrogate_rowid_field = sstreqi_ascii(r_fld.Name, SlConst::P_SurrogateRowIdFieldName);
		if(GETSTYPE(r_fld.T) == S_AUTOINC) {
			autoinc_fld_id = r_fld.Id;
			long   val = 0;
			size_t val_sz = 0;
			if(is_surrogate_rowid_field) {
				;
			}
			else {
				r_fld.getValue(pTbl->getDataBufConst(), &val, &val_sz);
				// (здесь это не так!) assert(val_sz == sizeof(val));
				if(val == 0) {
					// (ora) THROW(GetAutolongVal(*pTbl, i, &val));
					r_fld.setValue(pTbl->getDataBuf(), &val);
				}
			}
		}
		stmt.BindItem(-subst_no, 1, r_fld.T, is_surrogate_rowid_field ? static_cast<void *>(&row_id_zero) : PTR8(pTbl->getDataBuf()) + r_fld.Offs);
	}
	SqlGen.RPar();
	// В MySQL нет RETURNING
	{
		THROW(stmt.SetSqlText(SqlGen));
		THROW(Binding(stmt, -1));
		THROW(stmt.SetDataDML(0));
		THROW(stmt.Exec(1, OCI_DEFAULT));
		THROW(stmt.GetOutData(0));
		{
			DBRowId * p_row_id = pTbl->getCurRowIdPtr();
			const uint64 _last_inserted_id = mysql_stmt_insert_id(static_cast<MYSQL_STMT *>(stmt.H));
			if(p_row_id && _last_inserted_id) {
				p_row_id->SetI64(static_cast<int64>(_last_inserted_id));
			}
			if(pKeyBuf && idx >= 0 && idx < (int)pTbl->Indices.getNumKeys()) {
				map_ret_key = 1;
				key = pTbl->Indices[idx];
				ns = static_cast<uint>(key.getNumSeg());
				for(i = 0; i < ns; i++) {
					const BNField & r_fld = pTbl->Indices.field(idx, i);
					void * p_key_fld_data = PTR8(pKeyBuf)+pTbl->Indices.getSegOffset(idx, i);
					if(GETSTYPE(r_fld.T) == S_AUTOINC) {
						// В mysql нельзя в одну таблицу вставить больше одного autoinc-поля. По тому это - _last_inserted_id
						if(GETSSIZE(r_fld.T) == 4) {
							*static_cast<long *>(p_key_fld_data) = static_cast<long>(_last_inserted_id);
						}
						else if(GETSSIZE(r_fld.T) == 8) {
							*static_cast<int64 *>(p_key_fld_data) = static_cast<int64>(_last_inserted_id);
						}
					}
					else {
						
					}
					/*if(!fld_id_list.lsearch(r_fld.Id)) {
						//
						//SqlGen.Com().Text(r_fld.Name);
						//let_buf.NumberToLat(subst_no++);
						//temp_buf.CatDiv(',', 0).Colon().Cat(let_buf);
						//stmt.BindItem(-subst_no, 1, r_fld.T, PTR8(pKeyBuf)+pTbl->Indices.getSegOffset(idx, i));
						//
						subst_no++;
						fld_id_list.add(r_fld.Id);
						ret_fld_list.CatDivIfNotEmpty(',', 0).Cat(r_fld.Name);
						const size_t ret_buf_offs = ret_buf.Len();
						ret_buf.Cat(0, r_fld.size());
						stmt.BindItem(-subst_no, 1, r_fld.T, ret_buf.Ptr(ret_buf_offs));
					}*/
				}
			}
		}
		/*if(do_process_lob) {
			//
			// Если в записи были не пустые значения LOB-полей, то придется перечитать
			// вставленную запись и изменить значения LOB-полей.
			//
			// @todo Надо обновлять только LOB-поля, а не всю запись.
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

/*virtual*/int SMySqlDbProvider::Implement_UpdateRec(DBTable * pTbl, const void * pDataBuf, int ncc)
{
	int    ok = 1;
	int    subst_no = 0;
	SString temp_buf;
	SSqlStmt stmt(this);
	if(pDataBuf)
		pTbl->CopyBufFrom(pDataBuf);
	SqlGen.Z().Tok(Generator_SQL::tokUpdate).Sp().Text(pTbl->FileName_).Sp().Tok(Generator_SQL::tokSet).Sp();
	{
		const  uint fld_count = pTbl->FldL.getCount();
		for(uint i = 0; i < fld_count; i++) {
			const BNField & r_fld = pTbl->FldL.GetFieldByPosition(i);
			const bool is_surrogate_rowid_field = sstreqi_ascii(r_fld.Name, SlConst::P_SurrogateRowIdFieldName);
			if(GETSTYPE(r_fld.T) != S_AUTOINC) { // autoinc-поле менять не надо
				if(subst_no)
					SqlGen.Com();
				subst_no++;
				SqlGen.Text(pTbl->FldL[i].Name)._Symb(_EQ_).Text("?"); //.Param(temp_buf.NumberToLat(i));
				stmt.BindItem(-subst_no, 1, r_fld.T, PTR8(pTbl->getDataBuf()) + r_fld.Offs);
			}
		}
	}
	{
		/*
		//THROW(pTbl->getCurRowIdPtr()->IsI32());
		pTbl->getCurRowIdPtr()->ToStr__(temp_buf);
		SqlGen.Sp().Tok(Generator_SQL::tokWhere).Sp().Tok(Generator_SQL::tokRowId)._Symb(_EQ_).Text(temp_buf);
		*/
		uint   rowid_fld_pos = 0;
		const  BNField * p_rowid_fld = GetRowIdField(pTbl, &rowid_fld_pos);
		DBRowId * p_rowid = pTbl->getCurRowIdPtr();
		THROW(p_rowid && (p_rowid->IsI32() || p_rowid->IsI64())); // @todo @err
		p_rowid->ToStr__(temp_buf);
		THROW(p_rowid_fld); // @todo @err
		SqlGen.Sp().Tok(Generator_SQL::tokWhere).Sp().Text(p_rowid_fld->Name)._Symb(_EQ_).QText(temp_buf);
	}
	{
		THROW(stmt.SetSqlText(SqlGen));
		THROW(Binding(stmt, -1));
		THROW(stmt.SetDataDML(0));
		THROW(stmt.Exec(1, /*OCI_COMMIT_ON_SUCCESS*/OCI_DEFAULT)); // @debug(OCI_COMMIT_ON_SUCCESS)
	}
	CATCHZOK
	return ok;
}

/*virtual*/int SMySqlDbProvider::Implement_DeleteRec(DBTable * pTbl)
{
	return 0;
}

/*virtual*/int SMySqlDbProvider::Implement_BExtInsert(BExtInsert * pBei)
{
	return 0;
}

/*virtual*/int SMySqlDbProvider::Implement_GetPosition(DBTable * pTbl, DBRowId * pPos)
{
	return 0;
}

/*virtual*/int SMySqlDbProvider::Implement_DeleteFrom(DBTable * pTbl, int useTa, DBQ & rQ)
{
	return 0;
}

/*virtual*/int SMySqlDbProvider::ProtectTable(long dbTableID, char * pResetOwnrName, char * pSetOwnrName, int clearProtection)
{
	return 0;
}

/*virtual*/int SMySqlDbProvider::RecoverTable(BTBLID tblID, BRecoverParam * pParam)
{
	return 0;
}

/*virtual*/int SMySqlDbProvider::CreateStmt(SSqlStmt * pS, const char * pText, long flags)
{
	int    ok = 1;
	MYSQL_STMT * p_stmt = 0;
	THROW(p_stmt = mysql_stmt_init(static_cast<MYSQL *>(H)));
	pS->H = p_stmt;
	THROW(ProcessError(mysql_stmt_prepare(p_stmt, pText, sstrlen(pText))));
	CATCHZOK
	return ok;
}

/*virtual*/int SMySqlDbProvider::DestroyStmt(SSqlStmt * pS)
{
	if(pS) {
		if(pS->P_Result) {
			mysql_free_result(static_cast<MYSQL_RES *>(pS->P_Result));
			pS->P_Result = 0;
		}
		mysql_stmt_close(static_cast<MYSQL_STMT *>(pS->H));
	}
	return 1;
}

/*static*/MYSQL_STMT * FASTCALL SMySqlDbProvider::StmtHandle(const SSqlStmt & rS)
{
	return static_cast<MYSQL_STMT *>(rS.H);
}

/*virtual*/int SMySqlDbProvider::Binding(SSqlStmt & rS, int dir)
{
	int    ok = 1;
	bool   bind_result = true;
#if 1 // {
	const  uint row_count = rS.BL.Dim;
	const  uint col_count = rS.BL.getCount();
	MYSQL_STMT * h_stmt = StmtHandle(rS);
	TSVector <MYSQL_BIND> bind_list;
	assert(checkirange(row_count, 1U, 1024U));
	THROW(rS.SetupBindingSubstBuffer(dir, row_count));
	if(dir == -1) { // Входящие для SQL-запроса параметры (например для INSERT)
		for(uint i = 0; i < col_count; i++) {
			SSqlStmt::Bind & r_bind = rS.BL.at(i);
			if(r_bind.Pos < 0) {
				//OCIBind * p_bd = 0;
				THROW(ProcessBinding(0, row_count, &rS, &r_bind));
				{
					void * p_data = rS.GetBindOuterPtr(&r_bind, 0);
					MYSQL_BIND bind_item;
					MEMSZERO(bind_item);
					bind_item.buffer_type = static_cast<enum enum_field_types>(r_bind.NtvTyp);
					bind_item.buffer = p_data;
					bind_item.buffer_length = r_bind.NtvSize;
					// (все упало!) bind_item.length = &bind_item.buffer_length; // @v12.4.10 Важно! 
					bind_list.insert(&bind_item);
				}
			}
		}
		if(bind_list.getCount())
			bind_result = mysql_stmt_bind_param(static_cast<MYSQL_STMT *>(rS.H), static_cast<MYSQL_BIND *>(bind_list.dataPtr()));
	}
	else if(dir == +1) { // Исходящие из SQL-запроса значения (например для SELECT)
		for(uint i = 0; i < col_count; i++) {
			SSqlStmt::Bind & r_bind = rS.BL.at(i);
			if(r_bind.Pos > 0) {
				THROW(ProcessBinding(0, row_count, &rS, &r_bind));
				{
					void * p_data = rS.GetBindOuterPtr(&r_bind, 0);
					MYSQL_BIND bind_item;
					MEMSZERO(bind_item);
					bind_item.buffer_type = static_cast<enum_field_types>(r_bind.NtvTyp);
					if(r_bind.Flags & SSqlStmt::Bind::fUseRetActualSize) {
						bind_item.length = reinterpret_cast<ulong *>(&r_bind.RetActualSize);
					}
					else {
						bind_item.buffer_length = r_bind.NtvSize;
						bind_item.buffer = p_data;
					}
					//bind_item.is_null
					//bind_item.length
					//bind_item.error
					bind_list.insert(&bind_item);
				}
			}
		}
		if(bind_list.getCount())
			mysql_stmt_bind_result(static_cast<MYSQL_STMT *>(rS.H), &bind_list.at(0));
	}
	CATCH
		ok = 0;
		//DEBUG_LOG(LastErrMsg);
	ENDCATCH
#endif // } 0
	return ok;
}

int SMySqlDbProvider::ProcessBinding_SimpleType(int action, uint count, SSqlStmt * pStmt, SSqlStmt::Bind * pBind, uint ntvType)
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

SMySqlDbProvider::OD FASTCALL SMySqlDbProvider::OdAlloc(int descrType)
{
	void * p_h = 0;
	OD     o;
	if(descrType == descrMYSQLTIME) {
		p_h = new MYSQL_TIME;
		o.H = p_h;
		o.T = descrType;
	}
	return o;
}

void FASTCALL SMySqlDbProvider::OdFree(SMySqlDbProvider::OD & rO)
{
	if(rO.T == descrMYSQLTIME) {
		delete static_cast<MYSQL_TIME *>(rO.H);
		rO.H = 0;
		rO.T = 0;
	}
}

int SMySqlDbProvider::ProcessBinding_AllocDescr(uint count, SSqlStmt * pStmt, SSqlStmt::Bind * pBind, uint ntvType, int descrType)
{
	int    ok = 1;
	pBind->SetNtvTypeAndSize(ntvType, sizeof(void *));
	if(pStmt->AllocBindSubst(count, sizeof(OD), pBind) > 0) {
		OD d;
		for(uint i = 0; i < count; i++) {
			d = OdAlloc(descrType);
			memcpy(pStmt->GetBindOuterPtr(pBind, i), &d, sizeof(d));
		}
	}
	else
		ok = 0;
	return ok;
}

void SMySqlDbProvider::ProcessBinding_FreeDescr(uint count, SSqlStmt * pStmt, SSqlStmt::Bind * pBind)
{
	for(uint i = 0; i < count; i++)
		OdFree(*static_cast<OD *>(pStmt->GetBindOuterPtr(pBind, i)));
}
//
// ARG(action IN):
//   0 - инициализация структуры SSqlStmt::Bind
//   1 - извлечение данных из внешнего источника во внутренние буферы
//  -1 - перенос данных из внутренних буферов во внешний источник
//  1000 - разрушение специфичных для SQL-сервера элементов структуры SSqlStmt::Bind
//
/*virtual*/int SMySqlDbProvider::ProcessBinding(int action, uint count, SSqlStmt * pStmt, SSqlStmt::Bind * pBind)
{
	int    ok = 1;
	/*
enum enum_field_types { 
	MYSQL_TYPE_DECIMAL, 
	MYSQL_TYPE_TINY,
	MYSQL_TYPE_SHORT,  
	MYSQL_TYPE_LONG,
	MYSQL_TYPE_FLOAT,  
	MYSQL_TYPE_DOUBLE,
	MYSQL_TYPE_NULL,   
	MYSQL_TYPE_TIMESTAMP,
	MYSQL_TYPE_LONGLONG, 
	MYSQL_TYPE_INT24,
	MYSQL_TYPE_DATE,   
	MYSQL_TYPE_TIME,
	MYSQL_TYPE_DATETIME, 
	MYSQL_TYPE_YEAR,
	MYSQL_TYPE_NEWDATE, 
	MYSQL_TYPE_VARCHAR,
	MYSQL_TYPE_BIT,
	//
	// the following types are not used by client, only for mysqlbinlog!!
	//
	MYSQL_TYPE_TIMESTAMP2,
	MYSQL_TYPE_DATETIME2,
	MYSQL_TYPE_TIME2,
	// --------------------------------------------- 
	MYSQL_TYPE_JSON = 245,
	MYSQL_TYPE_NEWDECIMAL = 246,
	MYSQL_TYPE_ENUM = 247,
	MYSQL_TYPE_SET = 248,
	MYSQL_TYPE_TINY_BLOB = 249,
	MYSQL_TYPE_MEDIUM_BLOB = 250,
	MYSQL_TYPE_LONG_BLOB = 251,
	MYSQL_TYPE_BLOB = 252,
	MYSQL_TYPE_VAR_STRING = 253,
	MYSQL_TYPE_STRING = 254,
	MYSQL_TYPE_GEOMETRY = 255,
	MAX_NO_FIELD_TYPES 
};
	*/
	const  size_t sz = stsize(pBind->Typ);
	uint16 out_typ = 0;
	pBind->NtvSize = static_cast<uint16>(sz); // default value
	void * p_data = 0;
	// (Эти 2 строчки должны быть перед вызовом pStmt->GetBindOuterPtr поскольку он полагается на поле pBind->Dim) {
	if(action == 0) {
		pBind->Dim_ = count;
	}
	else if(oneof3(action, -2, -1, +1)) {
		p_data = pStmt->GetBindOuterPtr(pBind, count);
	}
	const int t = GETSTYPE(pBind->Typ);
	switch(t) {
		case S_CHAR: 
			ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_TINY); 
			break;
		case S_INT:
		case S_AUTOINC: 
			if(sz == 4)
				ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_LONG); 
			else if(sz == 8)
				ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_LONGLONG);
			else if(sz == 2)
				ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_SHORT); 
			else if(sz == 1)
				ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_TINY);
			break;
		case S_UINT: 
			if(sz == 4)
				ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_LONG/*signed*/);
			else if(sz == 8)
				ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_LONGLONG/*signed*/);
			else if(sz == 2)
				ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_SHORT/*signed*/);
			else if(sz == 1)
				ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_TINY/*signed*/);
			break;
		case S_INT64: 
			ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_LONGLONG); 
			break;
		case S_FLOAT: 
			if(sz == 8)
				ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_DOUBLE);
			else if(sz == 4)
				ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_FLOAT);
			else {
				constexpr int InvalidSizeOfFloatType = 0;
				assert(InvalidSizeOfFloatType);
			}
			break;
		case S_DATE:
			if(action == 0) {
				pBind->SetNtvTypeAndSize(MYSQL_TYPE_DATE, sizeof(MYSQL_TIME));
				pStmt->AllocBindSubst(count, pBind->NtvSize, pBind);
			}
			else if(action < 0) {
				MYSQL_TIME * p_ocidt = static_cast<MYSQL_TIME *>(p_data);
				LDATE * p_dt = static_cast<LDATE *>(pBind->P_Data);
				memzero(p_ocidt, sizeof(*p_ocidt));
				if(*p_dt == MAXDATE) {
					p_ocidt->year = 2200;
					p_ocidt->month = 12;
					p_ocidt->day = 31;
				}
				else if(*p_dt == ZERODATE) {
					p_ocidt->year = 1600;
					p_ocidt->month = 1;
					p_ocidt->day = 1;
				}
				else {
					p_ocidt->year = p_dt->year();
					p_ocidt->month = p_dt->month();
					p_ocidt->day = p_dt->day();
				}
				p_ocidt->time_type = MYSQL_TIMESTAMP_DATE;
			}
			else if(action == 1) {
				const MYSQL_TIME * p_ocidt = static_cast<const MYSQL_TIME *>(p_data);
				*static_cast<LDATE *>(pBind->P_Data) = encodedate(p_ocidt->day, p_ocidt->month, p_ocidt->year);
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
				MYSQL_TIME * p_ocidt = static_cast<MYSQL_TIME *>(p_data);
				LTIME * p_tm = static_cast<LTIME *>(pBind->P_Data);
				memzero(p_ocidt, sizeof(*p_ocidt));
				if(*p_tm == MAXTIME) {
					p_ocidt->hour = 23;
					p_ocidt->minute = 59;
					p_ocidt->second = 59;
					p_ocidt->second_part = 99 * 10000; // second_part - микросекунды
				}
				else if(*p_tm == ZEROTIME) {
					p_ocidt->hour = 0;
					p_ocidt->minute = 0;
					p_ocidt->second = 0;
					p_ocidt->second_part = 0; // second_part - микросекунды
				}
				else {
					p_ocidt->hour = p_tm->hour();
					p_ocidt->minute = p_tm->minut();
					p_ocidt->second = p_tm->sec();
					p_ocidt->second_part = p_tm->hs() * 10000; // second_part - микросекунды
				}
				p_ocidt->time_type = MYSQL_TIMESTAMP_TIME;
			}
			else if(action == 1) {
				const MYSQL_TIME * p_ocidt = static_cast<const MYSQL_TIME *>(p_data);
				*static_cast<LTIME *>(pBind->P_Data) = encodetime(p_ocidt->hour, p_ocidt->minute, p_ocidt->second, p_ocidt->second_part / 10000);
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
				MYSQL_TIME * p_ocidt = static_cast<MYSQL_TIME *>(p_data);
				const LDATETIME * p_dt = static_cast<const LDATETIME *>(pBind->P_Data);
				memzero(p_ocidt, sizeof(*p_ocidt));
				p_ocidt->year = p_dt->d.year();
				p_ocidt->month = p_dt->d.month();
				p_ocidt->day = p_dt->d.day();
				p_ocidt->hour = p_dt->t.hour();
				p_ocidt->minute = p_dt->t.minut();
				p_ocidt->second = p_dt->t.sec();
				p_ocidt->second_part = p_dt->t.hs() * 10000; // second_part - микросекунды
				p_ocidt->time_type = MYSQL_TIMESTAMP_DATETIME;
			}
			else if(action == 1) {
				const MYSQL_TIME * p_ocidt = static_cast<const MYSQL_TIME *>(p_data);
				static_cast<LDATETIME *>(pBind->P_Data)->d = encodedate(p_ocidt->day, p_ocidt->month, p_ocidt->year);
				static_cast<LDATETIME *>(pBind->P_Data)->t = encodetime(p_ocidt->hour, p_ocidt->minute, p_ocidt->second, p_ocidt->second_part / 10000);
			}
			else if(action == 1000)
				ProcessBinding_FreeDescr(count, pStmt, pBind);
			break;
		case S_NOTE:
			{
				const uint32 ntv_size = (sz * 2); // Удваиваем размер из-за того, что в сервере хранится utf8 а наруже мы (пока) используем OEM-encoding
				if(action == 0) {
					pBind->SetNtvTypeAndSize(MYSQL_TYPE_VAR_STRING, ntv_size);
					pStmt->AllocBindSubst(count, pBind->NtvSize, pBind);
				}
				else if(action < 0) {
					if(p_data) {
						SString & r_temp_buf = SLS.AcquireRvlStr();
						r_temp_buf.CatN(static_cast<const char *>(pBind->P_Data), sz).Transf(CTRANSF_INNER_TO_UTF8);
						strnzcpy(static_cast<char *>(p_data), r_temp_buf, ntv_size);
					}
				}
				else if(action == 1) {
					const int16 * p_ind = static_cast<const int16 *>(pStmt->GetIndPtr(pBind, count));
					if(p_ind && *p_ind == -1) {
						PTR8(pBind->P_Data)[0] = 0;
					}
					else {
						SString & r_temp_buf = SLS.AcquireRvlStr();
						r_temp_buf = static_cast<const char *>(p_data);
						r_temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						strnzcpy(static_cast<char *>(pBind->P_Data), r_temp_buf, sz);
					}
				}
			}
			break;
		case S_ZSTRING:
			{
				const uint32 ntv_size = (sz * 2); // Удваиваем размер из-за того, что в сервере хранится utf8 а наруже мы (пока) используем OEM-encoding
				if(action == 0) {
					pBind->SetNtvTypeAndSize(MYSQL_TYPE_VAR_STRING, ntv_size);
					pStmt->AllocBindSubst(count, pBind->NtvSize, pBind);
				}
				else if(action < 0) {
					char * p_outer = static_cast<char *>(p_data);
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
						// Для него перекодировка не применяется!
						//
						const bool is_max = ismemchr(pBind->P_Data, sz-1, 255U);
						{
							SString & r_temp_buf = SLS.AcquireRvlStr();
							r_temp_buf.CatN(static_cast<const char *>(pBind->P_Data), sz);
							if(!is_max) {
								r_temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
							}
							strnzcpy(p_outer, r_temp_buf, ntv_size);
							/*if(!is_max) {
								SOemToChar(p_outer);
							}*/
						}
					}
				}
				else if(action == 1) {
					const int16 * p_ind = static_cast<const int16 *>(pStmt->GetIndPtr(pBind, count));
					if(p_ind && *p_ind == -1) {
						PTR8(pBind->P_Data)[0] = 0;
					}
					else {
						SString & r_temp_buf = SLS.AcquireRvlStr();
						r_temp_buf = static_cast<const char *>(p_data);
						r_temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						strnzcpy(static_cast<char *>(pBind->P_Data), r_temp_buf, sz);
						//CharToOemA(static_cast<char *>(p_data), static_cast<char *>(pBind->P_Data));
						//trimright(static_cast<char *>(pBind->P_Data));
					}
				}
			}
			break;
		case S_UUID_:
			{
				const uint32 ntv_size = sizeof(S_GUID);
				if(action == 0) {
					pBind->SetNtvTypeAndSize(MYSQL_TYPE_BLOB, ntv_size);
					pStmt->AllocBindSubst(count, pBind->NtvSize, pBind);
				}
				else if(action < 0) {
					char * p_outer = static_cast<char *>(p_data);
					if(p_outer && pBind->P_Data)
						memcpy(p_outer, static_cast<const S_GUID *>(pBind->P_Data), ntv_size);
				}
				else if(action == 1) {
					/*
					const int csz = sqlite3_column_bytes(h_stmt, idx-1);
					const void * p_sqlt_data = sqlite3_column_blob(h_stmt, idx-1);
					if(p_sqlt_data && csz == sizeof(S_GUID))
						*static_cast<S_GUID *>(p_data) = *static_cast<const S_GUID *>(p_sqlt_data);
					else
						static_cast<S_GUID *>(p_data)->Z();
					*/
					//const MYSQL_TIME * p_ocidt = static_cast<const MYSQL_TIME *>(p_data);
					//static_cast<LDATETIME *>(pBind->P_Data)->d = encodedate(p_ocidt->day, p_ocidt->month, p_ocidt->year);
					//static_cast<LDATETIME *>(pBind->P_Data)->t = encodetime(p_ocidt->hour, p_ocidt->minute, p_ocidt->second, p_ocidt->second_part / 10000);
					const char * p_outer = static_cast<const char *>(p_data);
					if(pBind->P_Data && p_outer && pBind->NtvSize == ntv_size) {
						memcpy(pBind->P_Data, p_outer, ntv_size);
					}
				}
			}
			break;
		case S_CLOB:
			if(action == 0) {
				const uint32 ntv_size = 0; // @debug
				pBind->SetNtvTypeAndSize(/*MYSQL_TYPE_MEDIUM_BLOB*/MYSQL_TYPE_VAR_STRING, ntv_size);
				pBind->Flags |= SSqlStmt::Bind::fUseRetActualSize;
				//pStmt->AllocBindSubst(count, pBind->NtvSize, pBind);
			}
			else if(action < 0) {
				if(p_data) {
					SLob * p_lob = static_cast<SLob *>(p_data);
					const size_t lob_size = p_lob->GetPtrSize();
					const void * p_lob_data = p_lob->GetRawDataPtrC();
					if(p_lob_data && lob_size) {
						SString & r_temp_buf = SLS.AcquireRvlStr();
						r_temp_buf.CatN(static_cast<const char *>(p_lob_data), lob_size).Transf(CTRANSF_INNER_TO_UTF8);
						//sqlite3_bind_text(h_stmt, idx, r_temp_buf.cptr(), r_temp_buf.Len(), SQLITE_TRANSIENT);
						mysql_stmt_send_long_data(static_cast<MYSQL_STMT *>(pStmt->H), abs(pBind->Pos)-1, r_temp_buf.cptr(), r_temp_buf.Len());
					}
				}
			}
			else if(action == 1) {
				// @construction
				SLob * p_lob = static_cast<SLob *>(p_data);
				if(p_lob) {
					bool   _err = false;
					bool   _no_more_data = false;
					SString & r_temp_buf = SLS.AcquireRvlStr();
					const  ulong _total_len = pBind->RetActualSize;
					ulong  _offs = 0;
					char   _chunk_buffer[2048];
					while(!_err && !_no_more_data && _offs < _total_len) {
						ulong  _len = 0;
						MYSQL_BIND msq_bind;
						MEMSZERO(msq_bind);
						msq_bind.buffer_type = MYSQL_TYPE_VAR_STRING;
						msq_bind.buffer = _chunk_buffer;
						msq_bind.buffer_length = sizeof(_chunk_buffer);
						msq_bind.length = &_len;
						int fcr = mysql_stmt_fetch_column(static_cast<MYSQL_STMT *>(pStmt->H), &msq_bind, abs(pBind->Pos)-1, _offs);
						if(fcr == 0) {
							if(_len) {
								r_temp_buf.CatN(_chunk_buffer, _len);
								_offs += _len;
							}
							else
								_no_more_data = true;
						}
						else {
							_err = true;
						}
					} 
					{
						r_temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						p_lob->InitPtr(r_temp_buf.Len()+1);
						void * p_lob_ptr = p_lob->GetRawDataPtr();
						if(p_lob_ptr) {
							strcpy(static_cast<char *>(p_lob_ptr), r_temp_buf.cptr());
						}
					}
				}
			}
			break;
#if 0 // {
		case S_BLOB:
		case S_CLOB:
			if(action == 0)
				ProcessBinding_AllocDescr(count, pStmt, pBind, MYSQL_TYPE_BLOB, OCI_DTYPE_LOB);
			else if(action < 0) {
				OD ocilob = *static_cast<const OD *>(p_data);
				DBLobBlock * p_lob = pStmt->GetBindingLob();
				size_t lob_sz = 0;
				uint64 lob_loc = 0;
				if(p_lob) {
					p_lob->GetSize(labs(pBind->Pos)-1, &lob_sz);
					p_lob->GetLocator(labs(pBind->Pos)-1, &lob_loc);
					SETIFZ(lob_loc, reinterpret_cast<uint64>(OdAlloc(OCI_DTYPE_LOB).H));
					ProcessError(OCILobAssign(Env, Err, (const OCILobLocator *)(lob_loc), reinterpret_cast<OCILobLocator **>(&ocilob.H)));
				}
				LobWrite(ocilob, pBind->Typ, static_cast<SLob *>(pBind->P_Data), lob_sz);
			}
			else if(action == 1) {
				OD ocilob = *static_cast<const OD *>(p_data);
				DBLobBlock * p_lob = pStmt->GetBindingLob();
				size_t lob_sz = 0;
				uint64 lob_loc = 0;
				LobRead(ocilob, pBind->Typ, static_cast<SLob *>(pBind->P_Data), &lob_sz);
				if(p_lob) {
					SETIFZ(lob_loc, reinterpret_cast<uint64>(OdAlloc(OCI_DTYPE_LOB).H));
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
					*static_cast<double *>(p_data) = dectobin(static_cast<const char *>(pBind->P_Data), dec_len, dec_prc);
				else if(action == 1)
					dectodec(*static_cast<double *>(p_data), static_cast<char *>(pBind->P_Data), dec_len, dec_prc);
			}
			break;
		case S_RAW:
			if(action == 0) {
				pBind->SetNtvTypeAndSize(SQLT_BIN, static_cast<uint16>(sz));
				pStmt->AllocBindSubst(count, (sz * 2), pBind);
			}
			else if(action < 0) {
				uint16 * p_outer = static_cast<uint16 *>(p_data);
				memcpy(p_outer, pBind->P_Data, sz);
				/*
				for(uint i = 0; i < sz; i++)
					p_outer[i] = byte2hex(PTR8(pBind->P_Data)[i]);
				*/
			}
			else if(action == 1) {
				const uint16 * p_outer = static_cast<const uint16 *>(p_data);
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
				OD ocirid = *static_cast<const OD *>(p_data);
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
	return ok;
}

/*virtual*/int SMySqlDbProvider::Describe(SSqlStmt & rS, SdRecord &)
{
	return 0;
}

/*virtual*/int SMySqlDbProvider::Fetch(SSqlStmt & rS, uint count, uint * pActualCount)
{
	int    ok = 0;
	uint   actual = 0;
	if(rS.Flags & SSqlStmt::fNoMoreData)
		ok = -1;
	else {
		//int    err = 1; //OCIStmtFetch2(StmtHandle(rS), Err, count, OCI_FETCH_NEXT, 0, OCI_DEFAULT);
		int    _status = mysql_stmt_fetch(StmtHandle(rS));
		if(_status == 0) {
			actual = 1; // @debug
			ok = 1;
		}
		else if(_status == MYSQL_NO_DATA) {
			ok = -1;
		}
		else if(_status == MYSQL_DATA_TRUNCATED) { // @v12.4.12
			actual = 1; // @debug
			ok = 1; // ???
		}
		/*if(oneof2(err, OCI_SUCCESS, OCI_SUCCESS_WITH_INFO)) {
			actual = count;
			rS.Flags &= ~SSqlStmt::fNoMoreData;
			ok = 1;
		}
		else if(err == OCI_NO_DATA) {
			// (ora) OhAttrGet(StmtHandle(rS), OCI_ATTR_ROWS_FETCHED, &actual, 0);
			rS.Flags |= SSqlStmt::fNoMoreData;
			ok = actual ? 1 : -1;
		}
		else {
			ok = ProcessError(err);
		}*/
	}
	ASSIGN_PTR(pActualCount, actual);
	return ok;
}

#endif // } 0 @construction 
