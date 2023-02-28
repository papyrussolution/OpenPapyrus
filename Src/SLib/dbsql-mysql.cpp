// DBSQL-MYSQL.CPP
// Copyright (c) A.Sobolev 2020, 2021, 2022, 2023
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
#include <snet.h>

#if 1 // @construction {

#include <mariadb/mysql.h>

#ifndef NDEBUG
	#define DEBUG_LOG(msg) SLS.LogMessage("dbmysql.log", msg, 0)
#else
	#define DEBUG_LOG(msg)
#endif

SMySqlDbProvider::SMySqlDbProvider() :
	DbProvider(DbDictionary::CreateInstance(0, 0), DbProvider::cSQL|DbProvider::cDbDependTa), H(0), SqlGen(sqlstMySQL, 0), Flags(0)
{
}

SMySqlDbProvider::~SMySqlDbProvider()
{
}

int FASTCALL SMySqlDbProvider::ProcessError(int status)
{
	if(status == 0)
		return 1;
	else {
		return 0;
	}
}

/*virtual*/int SMySqlDbProvider::Login(const DbLoginBlock * pBlk, long options)
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
			SString host, sid;
			SString user, pw;
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
				uint ssp = 0;
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

/*virtual*/int SMySqlDbProvider::CreateDataFile(const DBTable * pTbl, const char * pFileName, int createMode, const char * pAltCode)
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

/*virtual*/int SMySqlDbProvider::GetDatabaseState(uint * pStateFlags)
{
	ASSIGN_PTR(pStateFlags, dbstNormal);
	return 1;
}

/*virtual*/SString & SMySqlDbProvider::MakeFileName_(const char * pTblName, SString & rBuf)
{
	return rBuf.SetIfEmpty(pTblName);
}

/*virtual*/int SMySqlDbProvider::IsFileExists_(const char * pFileName)
{
	return BIN(GetFileStat(pFileName, 0, 0) > 0);
}

/*virtual*/SString & SMySqlDbProvider::GetTemporaryFileName(SString & rFileNameBuf, long * pStart, int forceInDataPath)
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
	return 0;
}

/*virtual*/int SMySqlDbProvider::CommitWork()
{
	return 0;
}

/*virtual*/int SMySqlDbProvider::RollbackWork()
{
	return 0;
}

int SMySqlDbProvider::GetFileStat(const char * pFileName, long reqItems, DbTableStat * pStat)
{
	int    ok = 0;
	uint   actual = 0;
	BNFieldList fld_list;
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
	SSqlStmt stmt(this, (const SString &)SqlGen);
	THROW(stmt.Exec(0, 0));
	THROW(stmt.BindData(+1, 1, fld_list, &rec_buf, 0));
	THROW(Binding(stmt, +1));
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
	pTbl->fileName = NZOR(pFileName, pTbl->tableName);
	pTbl->OpenedFileName = pTbl->fileName;
	pTbl->FixRecSize = pTbl->fields.getRecSize();
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
		if(pStmt->Sf & DBTable::sfForUpdate) {
			SString temp_buf;
			OD     rowid = OdAlloc(OCI_DTYPE_ROWID);
			// (ora) THROW(OhAttrGet(StmtHandle(*pStmt), OCI_ATTR_ROWID, (OCIRowid *)rowid, 0));
			// (ora) RowidToStr(rowid, temp_buf);
			pTbl->getCurRowIdPtr()->FromStr(temp_buf);
			OdFree(rowid);
		}
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

/*virtual*/int SMySqlDbProvider::Implement_Search(DBTable * pTbl, int idx, void * pKey, int srchMode, long sf)
{
	// BNKeyList BNFieldList BNKey Generator_SQL
	//
	// select /*+ index_asc(tbl_name index_name) */ * from
	//
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
	THROW(idx < (int)pTbl->indexes.getNumKeys());
	if(oneof2(srchMode, spNext, spPrev)) {
		p_stmt = pTbl->GetStmt();
		if(p_stmt) {
			THROW(ok = Helper_Fetch(pTbl, p_stmt, &actual));
			if(ok > 0) {
				int    r = 1;
				pTbl->copyBufToKey(idx, pKey);
				if(oneof5(p_stmt->Sp, spGt, spGe, spLt, spLe, spEq)) {
					int kc = pTbl->indexes.compareKey(p_stmt->Idx, pKey, p_stmt->Key);
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
	else {
		//
		// Для того, чтобы hint'ы работали, необходимо и в hint'е и в
		// префиксах списков полей указывать либо алиас, либо наименование таблицы,
		// но не смешивать.
		// Например конструкция //
		// SELECT/*+INDEX_DESC(Reference2 idxReference2Key0)*/ t.*, t.ROWID FROM Reference2 t WHERE ObjType<=6 AND (ObjID<0 OR (ObjType<>6 ))
		// работать будет не по hint'у поскольку в хинте указано наименование таблицы, а в списке
		// полей - алиас.
		//

		//
		// Алиас нужен в том случае, если кроме списка полей необходимо достать rowid
		// (for update и так возвращает rowid, то есть явно его указывать в этом случае не надо).
		//
		const char * p_alias = (sf & DBTable::sfForUpdate) ? 0 : "t";
		SString temp_buf;
		uint8  temp_key[1024];
		void * p_key_data = pKey;
		BNKey  key = pTbl->indexes[idx];
		const  int ns = key.getNumSeg();
		SqlGen.Z().Tok(Generator_SQL::tokSelect);
		if(!(sf & DBTable::sfDirect)) {
			SqlGen.HintBegin().
			HintIndex(*pTbl, p_alias, idx, BIN(oneof3(srchMode, spLt, spLe, spLast))).
			HintEnd();
		}
		SqlGen.Sp();
		if(!(sf & DBTable::sfForUpdate)) {
			// @v11.6.0 mysql не позволяет извлекать rowid как в oracle!
			SqlGen.Text(p_alias).Dot().Aster()/* @v11.6.0.Com().Text(p_alias).Dot().Tok(Generator_SQL::tokRowId)*/;
		}
		else
			SqlGen.Aster();
		SqlGen.Sp().From(pTbl->fileName, p_alias);
		if(sf & DBTable::sfDirect) {
			DBRowId * p_rowid = static_cast<DBRowId *>(pKey);
			THROW(p_rowid && p_rowid->IsLong());
			p_rowid->ToStr(temp_buf);
			SqlGen.Sp().Tok(Generator_SQL::tokWhere).Sp().Tok(Generator_SQL::tokRowId)._Symb(_EQ_).QText(temp_buf);
		}
		else {
			if(oneof2(srchMode, spFirst, spLast)) {
				memzero(temp_key, sizeof(temp_key));
				pTbl->indexes.setBound(idx, 0, BIN(srchMode == spLast), temp_key);
				p_key_data = temp_key;
			}
			SqlGen.Sp().Tok(Generator_SQL::tokWhere).Sp();
			for(int i = 0; i < ns; i++) {
				int fldid = key.getFieldID(i);
				const BNField fld = pTbl->indexes.field(idx, i);
				if(i > 0) { // НЕ первый сегмент
					SqlGen.Tok(Generator_SQL::tokAnd).Sp();
					if(srchMode != spEq)
						SqlGen.LPar();
				}
				if(key.getFlags(i) & XIF_ACS) {
					//
					// Для ORACLE нечувствительность к регистру символов
					// реализуется функциональным сегментом индекса nls_lower(fld).
					// Аналогичная конструкция применяется при генерации скрипта создания индекса
					// См. Generator_SQL::CreateIndex(const DBTable &, const char *, uint)
					//
					int   _func_tok = 0;
					if(SqlGen.GetServerType() == sqlstORA)
						_func_tok = Generator_SQL::tokNlsLower;
					else
						_func_tok = Generator_SQL::tokLower;
					SqlGen.Func(_func_tok, fld.Name);
				}
				else
					SqlGen.Text(fld.Name);
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
						const BNField fld2 = pTbl->indexes.field(idx, j);
						if(j > 0)
							SqlGen.Tok(Generator_SQL::tokAnd).Sp();
						if(key.getFlags(j) & XIF_ACS) {
							int   _func_tok = 0;
							if(SqlGen.GetServerType() == sqlstORA)
								_func_tok = Generator_SQL::tokNlsLower;
							else
								_func_tok = Generator_SQL::tokLower;
							SqlGen.Func(_func_tok, fld2.Name);
						}
						else
							SqlGen.Text(fld2.Name);
						SqlGen._Symb(_NE_);
						SqlGen.Param(temp_buf.NumberToLat(j));
						seg_map.add(j);
						SqlGen.Sp();
					}
					SqlGen.RPar().RPar().Sp();
				}
				SqlGen.Sp();
			}
			can_continue = 1;
		}
		if(sf & DBTable::sfForUpdate)
			SqlGen.Tok(Generator_SQL::tokFor).Sp().Tok(Generator_SQL::tokUpdate);
		{
			THROW(p_stmt = new DBTable::SelectStmt(this, (const SString &)SqlGen, idx, srchMode, sf));
			new_stmt = 1;
			THROW(p_stmt->IsValid());
			{
				size_t key_len = 0;
				p_stmt->BL.Dim = 1;
				for(uint i = 0; i < seg_map.getCount(); i++) {
					const int  seg = seg_map.get(i);
					const BNField & r_fld = pTbl->indexes.field(idx, seg);
					const size_t seg_offs = pTbl->indexes.getSegOffset(idx, seg);
					key_len += stsize(r_fld.T);
					if(key.getFlags(seg) & XIF_ACS) {
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
			// @v11.6.0 mysql не позволяет извлекать rowid как в oracle!
			/* @v11.6.0 if(!(sf & DBTable::sfForUpdate)) {
				int rowid_pos = pTbl->fields.getCount()+1;
				THROW(p_stmt->BindRowId(rowid_pos, 1, pTbl->getCurRowIdPtr()));
			}*/
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

/*virtual*/int SMySqlDbProvider::Implement_InsertRec(DBTable * pTbl, int idx, void * pKeyBuf, const void * pData)
{
	int    ok = 1;
	int    subst_no = 0;
	uint   i;
	int    do_process_lob = 0;
	int    map_ret_key = 0;
	BNKey  key;
	uint   ns = 0;
	const  uint fld_count = pTbl->fields.getCount();
	SString temp_buf, let_buf;
	SSqlStmt  stmt(this, 0);
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
			long val = 0;
			size_t val_sz = 0;
			r_fld.getValue(pTbl->getDataBufConst(), &val, &val_sz);
			assert(val_sz == sizeof(val));
			if(val == 0) {
				// (ora) THROW(GetAutolongVal(*pTbl, i, &val));
				r_fld.setValue(pTbl->getDataBuf(), &val);
			}
		}
		stmt.BindItem(-subst_no, 1, r_fld.T, PTR8(pTbl->getDataBuf()) + r_fld.Offs);
	}
	SqlGen.RPar();
	SqlGen.Sp().Tok(Generator_SQL::tokReturning).Sp().Tok(Generator_SQL::tokRowId);
	//
	// temp_buf будет содержать список переменных, в которые должны заносится возвращаемые значения //
	//
	let_buf.NumberToLat(subst_no++);
	temp_buf.Z().Colon().Cat(let_buf);
	stmt.BindRowId(-subst_no, 1, pTbl->getCurRowIdPtr());
	if(pKeyBuf && idx >= 0 && idx < (int)pTbl->indexes.getNumKeys()) {
		map_ret_key = 1;
		key = pTbl->indexes[idx];
		ns = (uint)key.getNumSeg();
		for(i = 0; i < ns; i++) {
			const BNField & r_fld = pTbl->indexes.field(idx, i);
			SqlGen.Com().Text(r_fld.Name);
			let_buf.NumberToLat(subst_no++);
			temp_buf.CatDiv(',', 0).Colon().Cat(let_buf);
			stmt.BindItem(-subst_no, 1, r_fld.T, PTR8(pKeyBuf)+pTbl->indexes.getSegOffset(idx, i));
		}
	}
	SqlGen.Sp().Tok(Generator_SQL::tokInto).Sp().Text(temp_buf);
	{
		THROW(stmt.SetText((SString &)SqlGen));
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

/*virtual*/int SMySqlDbProvider::Implement_UpdateRec(DBTable * pTbl, const void * pDataBuf, int ncc)
{
	return 0;
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
				OCIBind * p_bd = 0;
				THROW(ProcessBinding(0, row_count, &rS, &r_bind));
				{
					void * p_data = rS.GetBindOuterPtr(&r_bind, 0);
					MYSQL_BIND bind_item;
					MEMSZERO(bind_item);
					bind_item.buffer_type = static_cast<enum enum_field_types>(r_bind.NtvTyp);
					bind_item.buffer = p_data;
					bind_item.buffer_length = r_bind.NtvSize;
					bind_list.insert(&bind_item);
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
		if(bind_list.getCount())
			bind_result = mysql_stmt_bind_param(static_cast<MYSQL_STMT *>(rS.H), static_cast<MYSQL_BIND *>(bind_list.dataPtr()));
	}
	else if(dir == +1) { // Исходящие из SQL-запроса значения (например для SELECT)
		for(uint i = 0; i < col_count; i++) {
			SSqlStmt::Bind & r_bind = rS.BL.at(i);
			if(r_bind.Pos > 0) {
				//OCIDefine * p_bd = 0;
				THROW(ProcessBinding(0, row_count, &rS, &r_bind));
				{
					void * p_data = rS.GetBindOuterPtr(&r_bind, 0);
					MYSQL_BIND bind_item;
					MEMSZERO(bind_item);
					bind_item.buffer_type = static_cast<enum_field_types>(r_bind.NtvTyp);
					bind_item.buffer_length = r_bind.NtvSize;
					bind_item.buffer = p_data;
					//bind_item.is_null
					//bind_item.length
					//bind_item.error
					bind_list.insert(&bind_item);
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
	else if(pBind->Dim > 1) {
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
	if(action == 0)
		pBind->Dim = count;
	const int t = GETSTYPE(pBind->Typ);
	switch(t) {
		case S_CHAR: ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_TINY); break;
		case S_INT:
		case S_AUTOINC: ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_LONG); break;
		case S_UINT: ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_LONG /*signed*/); break;
		case S_INT64: ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_LONGLONG); break;
		case S_FLOAT: ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_DOUBLE); break;
		case S_DATE:
			if(action == 0) {
				pBind->SetNtvTypeAndSize(MYSQL_TYPE_DATE, sizeof(MYSQL_TIME));
				pStmt->AllocBindSubst(count, pBind->NtvSize, pBind);
			}
			else if(action < 0) {
				MYSQL_TIME * p_ocidt = static_cast<MYSQL_TIME *>(pStmt->GetBindOuterPtr(pBind, count));
				LDATE * p_dt = static_cast<LDATE *>(pBind->P_Data);
				memzero(p_ocidt, sizeof(*p_ocidt));
				p_ocidt->year = p_dt->year();
				p_ocidt->month = p_dt->month();
				p_ocidt->day = p_dt->day();
				p_ocidt->time_type = MYSQL_TIMESTAMP_DATE;
			}
			else if(action == 1) {
				const MYSQL_TIME * p_ocidt = static_cast<const MYSQL_TIME *>(pStmt->GetBindOuterPtr(pBind, count));
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
