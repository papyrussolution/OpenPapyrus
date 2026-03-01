// DBSQL-MYSQL.CPP
// Copyright (c) A.Sobolev 2020, 2021, 2022, 2023, 2025, 2026
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
#include <mariadb-20251208/mysql.h>
/*
	Включение режима логирования запросов:
	SET GLOBAL general_log = ON;
	SET GLOBAL log_output = 'TABLE';	

	Анализ лога запросов:
	SELECT * FROM mysql.general_log 
	SELECT * FROM mysql.general_log where argument LIKE '%Transfer%' ORDER BY event_time desc
*/
static void FASTCALL DEBUG_LOG(const char * pMsg)
{
	if(SlDebugMode::CT())
		SLS.LogMessage("dbmysql.log", pMsg, 0);
}

static MYSQL * FASTCALL GetHandle(DbProvider::Connection & rC) { return static_cast<MYSQL *>(rC.GetH()); }

SMySqlDbProvider::ConnectionEntry::ConnectionEntry() : State(0), P_LastSelectStmt(0)
{
}

SMySqlDbProvider::ConnectionEntry::~ConnectionEntry()
{
	State = 0;
	P_LastSelectStmt = 0;
}

SMySqlDbProvider::ConnectionEntry & SMySqlDbProvider::ConnectionEntry::Z()
{
	State = 0;
	P_LastSelectStmt = 0;
	Conn.Z();
	return *this;
}

int SMySqlDbProvider::ConnectionEntry::DestroyLastSelectStatement()
{
	int    ok = -1;
	if(SSqlStmt::IsConsistent(P_LastSelectStmt)) {
		if(DBTable::IsConsistent(P_LastSelectStmt->GetOwnerTblRef())) {
			P_LastSelectStmt->GetOwnerTblRef()->DestroySelectStmt();
			ok = 2;
		}
		else {
			delete P_LastSelectStmt;
			ok = 1;
		}
	}
	P_LastSelectStmt = 0;
	return ok;
}

SMySqlDbProvider::TransactionBlock::TransactionBlock(SMySqlDbProvider * pPrvdr) : P_Prvdr(pPrvdr)
{
	if(P_Prvdr) {
		ConnectionEntry * p_ce = P_Prvdr->GetConnection();
		if(p_ce)
			Conn = *p_ce;
	}
}
		
SMySqlDbProvider::TransactionBlock::~TransactionBlock()
{
	if(P_Prvdr) {
		P_Prvdr->ReleaseConnection(Conn);
	}
}

SMySqlDbProvider::SMySqlDbProvider(const char * pDataPath) :
	DbProvider(sqlstMySQL, cpUTF8, DbDictionary::CreateInstance(0, 0), DbProvider::cSQL|DbProvider::cDbDependTa), 
	SqlGen(sqlstMySQL, 0), Flags(0), P_TraBlk(0)
{
	DataPath = pDataPath;
	DBS.GetDbPathID(DataPath, &DbPathID);
}

SMySqlDbProvider::~SMySqlDbProvider()
{
	Logout();
}

int FASTCALL SMySqlDbProvider::ProcessError(Connection & rConn, int status)
{
	int    ok = 1;
	if(status) {
		LastErr.Code = status;
		LastErr.Descr = mysql_error(GetHandle(rConn));
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
		THROW(ProcessError(MainConn, mysql_select_db(GetHandle(MainConn), pDbName)));
		CurrentDatabase = pDbName;
		Lb.SetAttr(DbLoginBlock::attrDbName, pDbName);
		{
			for(uint i = 0; i < ConnPool.getCount(); i++) {
				Connection & r_conn = ConnPool.at(i).Conn;
				Helper_CloseConnection(r_conn);
			}
			ConnPool.clear();
		}
	}
	CATCHZOK
	return ok;
}

SMySqlDbProvider::ConnectionEntry * SMySqlDbProvider::GetConnection() // @v12.5.5
{
	ConnectionEntry * p_result = 0;
	for(uint i = 0; !p_result && i < ConnPool.getCount(); i++) {
		ConnectionEntry & r_item = ConnPool.at(i);
		if(!(r_item.State & ConnectionEntry::stBusy) && !!r_item.Conn) {
			p_result = &r_item;
			r_item.State |= ConnectionEntry::stBusy;
		}
	}
	if(!p_result) {
		ConnectionEntry new_item;
		if(Helper_Connect(&Lb, &CurrentDatabase, new_item, false/*dontUseDatabase*/)) {
			new_item.State |= ConnectionEntry::stBusy;
			ConnPool.insert(&new_item);
			p_result = &ConnPool.at(ConnPool.getCount()-1);
		}
	}
	return p_result;
}

SMySqlDbProvider::ConnectionEntry * SMySqlDbProvider::SearchConnectionByH(void * pH)
{
	SMySqlDbProvider::ConnectionEntry * p_result = 0;
	if(pH) {
		if(pH == MainConn.Conn.GetH())
			p_result = &MainConn;
		else {
			for(uint i = 0; !p_result && i < ConnPool.getCount(); i++) {
				ConnectionEntry & r_item = ConnPool.at(i);
				if(r_item.Conn.GetH() == pH)
					p_result = &r_item;
			}
		}
	}
	return p_result;
}

int SMySqlDbProvider::ReleaseConnection(ConnectionEntry & rConnEntry) // @v12.5.3
{
	int    ok = -1;
	if(!!rConnEntry.Conn) {
		if(rConnEntry.Conn.GetH() == MainConn.Conn.GetH()) {
			ok = 100; // Нельзя здесь закрывать основное соединение!  
		}
		else {
			for(uint i = 0; ok < 0 && i < ConnPool.getCount(); i++) {
				ConnectionEntry & r_item = ConnPool.at(i);
				if(r_item.Conn.GetH() == rConnEntry.Conn.GetH()) {
					r_item.State &= ~ConnectionEntry::stBusy;
					r_item.DestroyLastSelectStatement();
					r_item.P_LastSelectStmt = 0;
					//rConnEntry.Z();
					ok = 2;
				}
			}
		}
		if(ok < 0) {
			Helper_CloseConnection(rConnEntry);
			ok = 1;
		}
	}
	return ok;
}

int SMySqlDbProvider::Helper_CloseConnection(DbProvider::Connection & rConn) // @v12.5.3
{
	int    ok = -1;
	if(!!rConn) {
		if(P_TraBlk && P_TraBlk->Conn.Conn == rConn) {
			RollbackWork();
		}
		mysql_close(GetHandle(rConn));
		rConn.Z();
		ok = 1;
	}
	return ok;
}

bool SMySqlDbProvider::Helper_Connect(const DbLoginBlock * pBlk, SString * pDbName, ConnectionEntry & rConnEntry, bool dontUseDatabase) // @v12.5.3
{
	rConnEntry.Z();
	CALLPTRMEMB(pDbName, Z());
	//Connection _conn;
	bool   ok = false;
	if(pBlk) {
		rConnEntry.Conn.SetH(mysql_init(0));
		if(!!rConnEntry.Conn) {
			// SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED;

			MYSQL * p_h = GetHandle(rConnEntry.Conn);
			SString temp_buf;
			SString url_buf;
			SString db_name;
			{
				// Установка таймаутов (в секундах)
				uint connect_timeout = 10;    // Таймаут подключения
				uint read_timeout = 10;       // Таймаут чтения
				uint write_timeout = 30;      // Таймаут записи
    			mysql_options(p_h, MYSQL_OPT_CONNECT_TIMEOUT, &connect_timeout);
				mysql_options(p_h, MYSQL_OPT_READ_TIMEOUT, &read_timeout);
				mysql_options(p_h, MYSQL_OPT_WRITE_TIMEOUT, &write_timeout);				
			}
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
				//p_h->options.charset_name = newStr("utf8mb4"); // @v12.5.1
				const char * p_db_name = dontUseDatabase ? 0 : db_name.cptr();
				if(mysql_real_connect(p_h, host, user, pw, p_db_name, port, 0/*unix-socket*/, 0/*client_flags*/)) {
					//CurrentDatabase = db_name;
					mysql_query(p_h, "SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED"); // @v12.5.7
					/*
						Для отладочной проверки уровня изоляции перед исполнением select:
						// После SET SESSION ... но перед SELECT
						MYSQL_RES *res = mysql_query(p_h, "SELECT @@transaction_isolation");
						if(res) {
							MYSQL_ROW row = mysql_fetch_row(res);
							printf("Isolation level: %s\n", row[0]); // Должно быть "READ-COMMITTED"
							mysql_free_result(res);
						} 
						else {
							printf("Error: %s\n", mysql_error(p_h));
						}
					*/ 
					ASSIGN_PTR(pDbName, db_name);
					ok = true;
				}
				else {
					mysql_close(GetHandle(rConnEntry.Conn));
					rConnEntry.Z();
				}
			}
		}
	}
	return ok;
}

/*virtual*/int SMySqlDbProvider::DbLogin(const DbLoginBlock * pBlk, long options)
{
	int    ok = 1;
	//
	// Вполне может так случиться, что мы авторизуемся на dmbs-сервере но базы данных там еще нет.
	// Из-за такой возможности сначала логинимся на сервере без входа в базу данных Helper_Connect(..., true/*dontUseDatabase*/)
	// далее проверяем наличие базы данных. Если нет, то создаем и только после этого начинаем использовать базу данных (UseDatabase)
	//
	SString db_name;
	THROW(pBlk); // @todo @err
	pBlk->GetAttr(DbLoginBlock::attrDbName, db_name);
	if(db_name.IsEmpty()) {
		pBlk->GetAttr(DbLoginBlock::attrDbSymb, db_name);
	}
	THROW(db_name.NotEmptyS()); // @todo @err
	if(Helper_Connect(pBlk, &CurrentDatabase, MainConn, true/*dontUseDatabase*/)) {
		Common_Login(pBlk);
		{
			uint   database_state = 0;
			int    gdbsr = GetDatabaseState(db_name, &database_state);
			THROW(gdbsr);
			if(database_state & dbstNotExists) {
				THROW(CreateDatabase(db_name));
			}
			THROW(UseDatabase(db_name));
		}
	}
	else {
		assert(!MainConn.Conn);
		assert(CurrentDatabase.IsEmpty());
		CALLEXCEPT();
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

/*virtual*/int SMySqlDbProvider::Logout()
{
	int    ok = 0;
	//P_LastSelectStmt = 0; // @v12.5.2 Нет необходимости разрушать этот экземпляр: таблица-владелец сама это сделает.
	RollbackWork(); // @v12.5.4 заодно разрушит P_TraBlk
	ok = Helper_CloseConnection(MainConn);
	{
		for(uint i = 0; i < ConnPool.getCount(); i++) {
			Connection & r_conn = ConnPool.at(i).Conn;
			Helper_CloseConnection(r_conn);
		}
		ConnPool.clear();
	}
	CurrentDatabase.Z();
	Common_Logout(); // @v12.5.3
	return ok;
}

SMySqlDbProvider::ConnectionEntry & SMySqlDbProvider::GetWorkingConnection(SSqlStmt * pStmt, bool continuousSelection)
{
	SMySqlDbProvider::ConnectionEntry * p_result = 0;
	assert(!P_TraBlk || State & stTransaction);
	if(P_TraBlk && State & stTransaction) {
		p_result = &P_TraBlk->Conn;
	}
	else {
		void * p_sc = pStmt ? pStmt->GetExternalConnection() : 0;
		ConnectionEntry * p_ext_entry = SearchConnectionByH(p_sc);
		if(p_ext_entry)
			p_result = p_ext_entry;
		else {
			ConnectionEntry * p_ce = 0;
			if(continuousSelection && (p_ce = GetConnection()) != 0) {
				p_result = p_ce;
			}
			else
				p_result = &MainConn;
		}
	}
	assert(p_result != 0);
	return *p_result;
}

/*virtual*/int SMySqlDbProvider::ExecStmt(SSqlStmt & rS, uint count, int mode)
{
	ConnectionEntry & r_ce = GetWorkingConnection(&rS, false);
	int    ok = ProcessError(r_ce, mysql_stmt_execute(static_cast<MYSQL_STMT *>(rS.H)));
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
	const  bool is_temp_table = IS_CRM_TEMP(createMode);
	const  int cm = RESET_CRM_TEMP(createMode);
	uint   ctf = Generator_SQL::ctfIndent;
	if(oneof2(cm, crmNoReplace, crmTTSNoReplace))
		ctf |= Generator_SQL::ctfIfNotExists;
	if(is_temp_table)
		ctf |= Generator_SQL::ctfTemporary;
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
		AddTempFileName(pFileName); // Регистрируем имя временного файла в драйвере БД для последующего удаления //
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
	rFileNameBuf.Z();
	uint  _clk = clock();
	rFileNameBuf.Cat("_tt").Cat(SLS.GetSessUuid(), S_GUID::fmtPlain).CatChar('_').Cat(_clk);
	return rFileNameBuf;
}

/*virtual*/int SMySqlDbProvider::DropFile(const char * pFileName) // @v12.5.5
{
	int    ok = 1;
	if(IsFileExists_(pFileName) > 0) {
		SqlGen.Z().Tok(Generator_SQL::tokDrop).Sp().Tok(Generator_SQL::tokTable).Sp().Text(pFileName);
		SSqlStmt stmt(this, SqlGen);
		THROW(stmt.Exec(1, OCI_DEFAULT));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

/*virtual*/int SMySqlDbProvider::PostProcessAfterUndump(DBTable * pTbl)
{
	return 0;
}

/*virtual*/int SMySqlDbProvider::StartTransaction()
{
	int    ok = 0;
	if(!!MainConn.Conn) { // Транзакция будет работать на отдельном соединении (TransactionBlock::Conn), но корневое соединение mandatory!
		if(!(State & stTransaction)) {
			//DestroyLastSelectStatement();
			SqlGen.Z().Tok(Generator_SQL::tokStart).Sp().Tok(Generator_SQL::tokTransaction);
			{ // @v12.5.4 
				assert(P_TraBlk == 0);
				delete P_TraBlk;
				P_TraBlk = new TransactionBlock(this);
			}
			if(P_TraBlk) {
				P_TraBlk->Conn.DestroyLastSelectStatement();
				const  int tilr = mysql_query(GetHandle(P_TraBlk->Conn), "SET TRANSACTION ISOLATION LEVEL READ COMMITTED;"); // @v12.5.7
				assert(tilr == 0); // @debug
				if(ProcessError(P_TraBlk->Conn, mysql_query(GetHandle(P_TraBlk->Conn), SqlGen.GetTextC()))) {
					ok = 1;
					State |= stTransaction;
				}
				else { // @v12.5.4 
					ZDELETE(P_TraBlk);
				}
			}
		}
		else {
			ok = -1;
		}
	}
	assert((!ok || P_TraBlk) && (!P_TraBlk || ok)); // @v12.5.4
	return ok;
}

/*virtual*/int SMySqlDbProvider::CommitWork()
{
	int    ok = 0;
	int    rbr = 0; // rollback-result
	if(!!MainConn.Conn) { // Транзакция будет работать на отдельном соединении (TransactionBlock::Conn), но корневое соединение mandatory!
		if(State & stTransaction) {
			assert(P_TraBlk);
			if(P_TraBlk) {
				P_TraBlk->Conn.DestroyLastSelectStatement();
				SqlGen.Z().Tok(Generator_SQL::tokCommit);
				if(ProcessError(P_TraBlk->Conn, mysql_query(GetHandle(P_TraBlk->Conn), SqlGen.GetTextC()))) {
					ZDELETE(P_TraBlk); // @v12.5.4
					ok = 1;
				}
				else {
					rbr = RollbackWork();
				}
			}
			assert(P_TraBlk == 0); // @v12.5.4
			State &= ~stTransaction;
		}
		else
			ok = -1;
	}
	assert(!ok || !P_TraBlk); // @v12.5.4
	return ok;
}

/*virtual*/int SMySqlDbProvider::RollbackWork()
{
	int    ok = 0;
	if(!!MainConn.Conn) { // Транзакция будет работать на отдельном соединении (TransactionBlock::Conn), но корневое соединение mandatory!
		if(State & stTransaction) {
			assert(P_TraBlk);
			if(P_TraBlk) {
				P_TraBlk->Conn.DestroyLastSelectStatement();
				SqlGen.Z().Tok(Generator_SQL::tokRollback);
				if(ProcessError(P_TraBlk->Conn, mysql_query(GetHandle(P_TraBlk->Conn), SqlGen.GetTextC()))) {
					ok = 1;
				}
			}
			ZDELETE(P_TraBlk); // @v12.5.4
			State &= ~stTransaction;
		}
		else
			ok = -1;
	}
	assert(!ok || !P_TraBlk); // @v12.5.4
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
			pStat->CrDtm = rec_buf.CrTm;
			pStat->ModDtm = rec_buf.UpdTm;
			pStat->TblName = rec_buf.TableName;
			pStat->Collation = rec_buf.Collation;
			pStat->SpaceName = rec_buf.TableCatalog;
			pStat->DbEngine = rec_buf.Engine;
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
	if(pTbl) {
		ok = GetFileStat(pTbl->FileName_, reqItems, pStat);
	}
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
	return 1; // @nothingtodo
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

SMySqlDbProvider::SearchQueryBlock::SearchQueryBlock(SqlServerType sqlst) : 
	Flags(0), SrchMode(0), AutoincFldIdx(_FFFF32), P_KeyData(0), SqlG(sqlst, 0) // повторяет то же из SSqliteDbProvider
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
		for(uint i = 0; i < fld_count; i++) {
			const BNField & r_fld = pTbl->FldL.GetFieldByPosition(i);
			if(i)
				rBlk.SqlG.Com();
			rBlk.SqlG.Text(p_alias).Dot().Text(r_fld.Name);
			subst_no++;
			const bool is_surrogate_rowid_field = sstreqi_ascii(r_fld.Name, SlConst::P_SurrogateRowIdFieldName);
			if(GETSTYPE(r_fld.T) == S_AUTOINC) {
				rBlk.AutoincFldIdx = i;
				if(is_surrogate_rowid_field)
					rBlk.Flags |= SearchQueryBlock::fAutoincFldIsSurrogate;
			}
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
		{
			bool   do_force_index = true;
			if(rBlk.SrchMode == spEq) { // Если условие EQ, то нет смысла мучить сервер требования работы по индексу - результат инвариантен
				do_force_index = false;
			}
			/*else if(oneof3(rBlk.SrchMode, spLe, spLt, spLast)) { // При обратном направлении поиска force index мешает серверу.
				do_force_index = false;
			}*/
			if(do_force_index) {
				if(rBlk.SqlG.GetIndexName(*pTbl, idx, temp_buf)) {
					rBlk.SqlG.Sp().Tok(Generator_SQL::tokForceIndex).LPar().Text(temp_buf).RPar();
				}
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
		if(pTbl->IsQuerySingleTacted(idx, rBlk.SrchMode) < 0) // @v12.5.3
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
	//uint64 surrogate_rowid_value_to_read = 0;
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
			// Так как у нас есть ключ pKey и нам не следует отчаиваться и возвращать ошибку - перестраиваем запрос и дуем в том направлении,
			// в котором нас просят. Единственный скользкий момент - если ключ не уникальный, то нет уверенности на какой именно записи
			// среди дубликатов мы находимся. Но эту проблему мы будем решать после того, как решим главную.
			do_make_query = true; // Да! Сработало! 
		}
	}
	if(do_make_query) {
		SearchQueryBlock sqb(SqlSt);
		THROW(Helper_MakeSearchQuery(pTbl, idx, pKey, srchMode, sf, sqb));
		can_continue = LOGIC(sqb.Flags & SearchQueryBlock::fCanContinue);
		{
			//
			// Сейчас нам надо решить через какое соединение будет работать select-оператор.
			// Если мы - в транзакции, то однозначно через привязаное к транзакции.
			// Вне транзакции два варианта:
			//   - can_continue == false - работаем через основное соединение (MainConn)
			//   - can_continue == true - получаем свободное соединение из пула через GetConnection()
			//
			ConnectionEntry & r_ce = GetWorkingConnection(0, can_continue);
			assert(r_ce.IsConsistent());
			{
				pTbl->DestroySelectStmt();
				r_ce.DestroyLastSelectStatement();
			}
			// В качестве ExternalConnection в SelectStmt передается "чистый" хандлер MySQL-соединения. То есть, ConnectionEntry::Conn::GetH()
			THROW(p_stmt = new DBTable::SelectStmt(this, r_ce.Conn.GetH(), pTbl, sqb.SqlG, idx, sqb.SrchMode, sf));
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
						DBRowId _ri_;
						//static_cast<void *>(p_cur_row_id)
						assert(!surrogate_rowid_tag);
						surrogate_rowid_tag = true;
						int64 * p_tbl_rowid = pTbl->CurRowId.SetI64(0);
						p_stmt->BindItem(subst_no, 1, r_fld.T, p_tbl_rowid);
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
			p_cur_row_id->SetI64(0); // @debug
			pTbl->clearDataBuf(); // @debug
			THROW(ok = Helper_Fetch(pTbl, p_stmt, &actual));
			if(ok > 0) {
				if(surrogate_rowid_tag) {
					//p_cur_row_id->SetI64(surrogate_rowid_value_to_read);
				}
				else {
					uint8  rowid_data[64];
					size_t rowid_data_size = 0;
					uint   rowid_fld_pos = 0;
					const  BNField * p_rowid_fld = GetRowIdField(pTbl, &rowid_fld_pos);
					THROW(p_rowid_fld); // @todo @err
					THROW(pTbl->getFieldValue(rowid_fld_pos, rowid_data, &rowid_data_size)); // @todo @err
					if(rowid_data_size == 4) {
						p_cur_row_id->SetI32(*reinterpret_cast<uint32 *>(rowid_data));
					}
					else if(rowid_data_size == 8) {
						p_cur_row_id->SetI64(*reinterpret_cast<int64 *>(rowid_data));
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
		ConnectionEntry & r_ce = GetWorkingConnection(p_stmt, false/*can_continue = false иначе вероятно получение нового соединения, а это нам не нужно*/);
		r_ce.P_LastSelectStmt = p_stmt;
	}
	else {
		pTbl->SetStmt(0);
		ConnectionEntry & r_ce = GetWorkingConnection(p_stmt, false/*can_continue = false иначе вероятно получение нового соединения, а это нам не нужно*/);
		r_ce.P_LastSelectStmt = 0;
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
	{
		SqlGen.LPar();
		{
			bool is_first_fld_enum_item = true;
			for(i = 0; i < fld_count; i++) {
				const BNField & r_fld = pTbl->FldL.GetFieldByPosition(i);
				const bool is_surrogate_rowid_field = sstreqi_ascii(r_fld.Name, SlConst::P_SurrogateRowIdFieldName);
				bool  do_skip_this_fld = false;
				if(is_surrogate_rowid_field) {
					do_skip_this_fld = false; // @itsnotanerror! false
				}
				else if(GETSTYPE(r_fld.T) == S_AUTOINC) {
					int64  val = 0;
					size_t val_sz = 0;
					r_fld.getValue(pTbl->getDataBufConst(), &val, &val_sz);
					// @v12.5.6 assert(val_sz == sizeof(val));
					if(val == 0) {
						// (похоже, не надо так делать - надо привязывать null-значение явно) do_skip_this_fld = true;
					}
				}
				if(!do_skip_this_fld) {
					if(!is_first_fld_enum_item)
						SqlGen.Com();
					SqlGen.Text(r_fld.Name);
					is_first_fld_enum_item = false;
				}
			}
		}
		SqlGen.RPar();
	}
	SqlGen.Tok(Generator_SQL::tokValues).Sp().LPar();
	stmt.BL.Dim = 1;
	stmt.BL.P_Lob = pTbl->getLobBlock();
	for(i = 0; i < fld_count; i++) {
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
		if(subst_no)
			SqlGen.Com();
		SqlGen.Text("?");
		subst_no++;
		if(is_surrogate_rowid_field) {
			stmt.BindItem(-subst_no, 1, r_fld.T, &row_id_zero);
		}
		else {
			stmt.BindItem(-subst_no, 1, r_fld.T, PTR8(pTbl->getDataBuf()) + r_fld.Offs);
		}
	}
	SqlGen.RPar();
	// В MySQL нет RETURNING
	{
		ConnectionEntry & r_ce = GetWorkingConnection(0, false);
		r_ce.DestroyLastSelectStatement();
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
		uint   rowid_fld_pos = 0;
		const  BNField * p_rowid_fld = GetRowIdField(pTbl, &rowid_fld_pos);
		DBRowId * p_rowid = pTbl->getCurRowIdPtr();
		THROW(p_rowid && (p_rowid->IsI32() || p_rowid->IsI64())); // @todo @err
		p_rowid->ToStr__(temp_buf);
		THROW(p_rowid_fld); // @todo @err
		SqlGen.Sp().Tok(Generator_SQL::tokWhere).Sp().Text(p_rowid_fld->Name)._Symb(_EQ_).QText(temp_buf);
	}
	{
		ConnectionEntry & r_ce = GetWorkingConnection(0, false);
		r_ce.DestroyLastSelectStatement();
		THROW(stmt.SetSqlText(SqlGen));
		THROW(Binding(stmt, -1));
		THROW(stmt.SetDataDML(0));
		THROW(stmt.Exec(1, /*OCI_COMMIT_ON_SUCCESS*/OCI_DEFAULT)); // @debug(OCI_COMMIT_ON_SUCCESS)
	}
	CATCHZOK
	return ok;
}

/*virtual*/int SMySqlDbProvider::Implement_DeleteRec(DBTable * pTbl) // @v12.5.3
{
	int    ok = 1;
	const  uint fld_count = pTbl->FldL.getCount();
	SString temp_buf;
	SqlGen.Z().Tok(Generator_SQL::tokDelete).Sp().From(pTbl->FileName_, 0).Sp();
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
		ConnectionEntry & r_ce = GetWorkingConnection(0, false);
		r_ce.DestroyLastSelectStatement();
		SSqlStmt stmt(this, SqlGen);
		THROW(stmt.Exec(1, OCI_DEFAULT));
	}
	CATCHZOK
	return ok;
}

/*virtual*/int SMySqlDbProvider::Implement_BExtInsert(BExtInsert * pBei)
{
	int    ok = -1;
	//
	// Чтобы не затирать содержимое внутреннего буфера таблицы pBei->P_Tbl распределяем временный буфер rec_buf.
	//
	SBaseBuffer rec_buf;
	rec_buf.Init();
	const  uint num_recs = pBei->GetCount();
	if(num_recs) {
		uint   i;
		int    in_subst_no = 0;
		int    out_subst_no = 0;
		DBTable * p_tbl = pBei->getTable();
		const  uint fld_count = p_tbl->FldL.getCount();
		SString temp_buf;
		SSqlStmt stmt(this);
		/*
			INSERT INTO имя_таблицы (поле1, поле2, поле3, ...) VALUES (значение1, значение2, значение3, ...);
		*/ 
		SqlGen.Z().Tok(Generator_SQL::tokInsert).Sp().Tok(Generator_SQL::tokInto).Sp().Text(p_tbl->FileName_).Sp();
		{
			SqlGen.LPar();
			{
				bool is_first_fld_enum_item = true;
				for(i = 0; i < fld_count; i++) {
					const BNField & r_fld = p_tbl->FldL.GetFieldByPosition(i);
					bool  do_skip_this_fld = false;
					if(sstreqi_ascii(r_fld.Name, SlConst::P_SurrogateRowIdFieldName)) {
						do_skip_this_fld = true;
					}
					else if(GETSTYPE(r_fld.T) == S_AUTOINC) {
						int64  val = 0;
						size_t val_sz = 0;
						r_fld.getValue(p_tbl->getDataBufConst(), &val, &val_sz);
						// @v12.5.6 assert(val_sz == sizeof(val));
						if(val == 0) {
							// (похоже, не надо так делать - надо привязывать null-значение явно) do_skip_this_fld = true;
						}
					}
					if(!do_skip_this_fld) {
						if(!is_first_fld_enum_item)
							SqlGen.Com();
						SqlGen.Text(r_fld.Name);
						is_first_fld_enum_item = false;
					}
				}
			}
			SqlGen.RPar().Sp();
		}
		SqlGen.Tok(Generator_SQL::tokValues).Sp().LPar();
		//
			stmt.BL.Dim = 1;
			stmt.BL.P_Lob = p_tbl->getLobBlock();
			{
				bool is_first_fld_enum_item = true;
				for(i = 0; i < fld_count; i++) {
					const BNField & r_fld = p_tbl->FldL.GetFieldByPosition(i);
					bool  do_skip_this_fld = false;
					if(sstreqi_ascii(r_fld.Name, SlConst::P_SurrogateRowIdFieldName)) {
						do_skip_this_fld = true;
					}
					else if(GETSTYPE(r_fld.T) == S_AUTOINC) {
						int64  val = 0;
						size_t val_sz = 0;
						r_fld.getValue(p_tbl->getDataBufConst(), &val, &val_sz);
						// @v12.5.6 assert(val_sz == sizeof(val));
						if(val == 0) {
							// (похоже, не надо так делать - надо привязывать null-значение явно) do_skip_this_fld = true;
						}
					}
					if(!do_skip_this_fld) {
						if(!is_first_fld_enum_item)
							SqlGen.Com();
						SqlGen.Text("?");
						stmt.BindItem(-in_subst_no, 1, r_fld.T, PTR8(p_tbl->getDataBuf()) + r_fld.Offs);
						is_first_fld_enum_item = false;
					}
				}
			}
			SqlGen.RPar();
			// @v12.5.6 SqlGen.Sp().Tok(Generator_SQL::tokReturning).Sp().Tok(Generator_SQL::tokRowId);
		//
		{
			THROW(stmt.SetSqlText(SqlGen));
			THROW(rec_buf.Alloc(p_tbl->getBufLen()));
			for(i = 0; i < num_recs; i++) {
				SBaseBuffer b = pBei->Get(i);
				assert(b.Size <= rec_buf.Size);
				memcpy(rec_buf.P_Buf, b.P_Buf, b.Size);
				//
				THROW(stmt.BindData(-1, 1, p_tbl->FldL, rec_buf.P_Buf, p_tbl->getLobBlock()));
				/*
				if(p_tbl->State & DBTable::sHasAutoinc) {
					for(uint j = 0; j < fld_count; j++) {
						const BNField & r_fld = p_tbl->FldL[j];
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
				//THROW(ResetStatement(stmt));
			}
			ok = 1;
		}
	}
	CATCHZOK
	rec_buf.Destroy();
	return ok;
}

/*virtual*/int SMySqlDbProvider::Implement_GetPosition(DBTable * pTbl, DBRowId * pPos)
{
	ASSIGN_PTR(pPos, *pTbl->getCurRowIdPtr());
	return 1;
}

/*virtual*/int SMySqlDbProvider::Implement_DeleteFrom(DBTable * pTbl, int useTa, DBQ & rQ) // @todo @20260125 @test
{
	int    ok = 1;
	int    ta = 0;
	if(useTa) {
		THROW(StartTransaction());
		ta = 1;
	}
	SqlGen.Z().Tok(Generator_SQL::tokDelete).Sp().From(pTbl->FileName_, 0);
	if(&rQ && rQ.tree) {
		SqlGen.Sp().Tok(Generator_SQL::tokWhere).Sp();
		rQ.tree->CreateSqlExpr(SqlGen, -1);
	}
	{
		ConnectionEntry & r_ce = GetWorkingConnection(0, false);
		r_ce.DestroyLastSelectStatement();
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
	bool   debug_mark = false; // @debug
	if(pS) {
		MYSQL_STMT * p_stmt = 0;
		ConnectionEntry * p_ce_ = 0;
		if(pS->Typ == Generator_SQL::typSelect && pS->GetSignature() == SlConst::SSqlStmtSignature_Select && (pS->Sf & DBTable::sfBExtQuery)) {
			DBTable::SelectStmt * p_ss = static_cast<DBTable::SelectStmt *>(pS);
			debug_mark = true; // @debug
			//DBTable * p_tbl = p_ss->GetOwnerTblRef();
			if(/*p_tbl*/true) {
				//
				// Для BExtQuery безусловно получаем новое соединение не ассоциированное с транзакцией
				// 
				//ConnectionEntry & r_ce_local = GetWorkingConnection(0, true/*can_continue*/);
				ConnectionEntry * p_ce_local = GetConnection();
				THROW(p_ce_local);
				assert(p_ce_local->IsConsistent());
				{
					//p_tbl->DestroySelectStmt();
					p_ce_local->DestroyLastSelectStatement();
				}
				// В качестве ExternalConnection в SelectStmt передается "чистый" хандлер MySQL-соединения. То есть, ConnectionEntry::Conn::GetH()
				pS->SetExternalConnection(p_ce_local->Conn.GetH());
				p_ce_ = p_ce_local;
			}
		}
		if(!p_ce_) {
			ConnectionEntry & r_ce = GetWorkingConnection(pS, false);
			p_ce_ = &r_ce;
		}
		THROW(p_ce_); // @todo @err
		THROW(p_stmt = mysql_stmt_init(GetHandle(*p_ce_)));
		pS->H = p_stmt;
		THROW(ProcessError(*p_ce_, mysql_stmt_prepare(p_stmt, pText, static_cast<ulong>(sstrlen(pText)))));
	}
	else
		ok = 0;
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
		// @v12.5.5 {
		{
			void * p_ec = pS->GetExternalConnection();
			ConnectionEntry * p_ce = SearchConnectionByH(p_ec);
			if(p_ce) {
				if(p_ce->P_LastSelectStmt == pS) {
					p_ce->P_LastSelectStmt = 0; // Мы только что разрушили этот оператор (see above)
				}
				ReleaseConnection(*p_ce);
				p_ce = 0;
			}
		}
		// } @v12.5.5 
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
				if(!(r_bind.Flags & SSqlStmt::Bind::fSkip)) { // @v12.5.7 @condition
					void * p_data = rS.GetBindOuterPtr(&r_bind, 0);
					MYSQL_BIND bind_item;
					MEMSZERO(bind_item);
					bind_item.buffer_type = static_cast<enum enum_field_types>(r_bind.NtvTyp);
					bind_item.buffer = p_data;
					//bind_item.length = reinterpret_cast<ulong *>(&r_bind.RetActualSize); // @v12.5.1
					// (все упало!) bind_item.length = &bind_item.buffer_length; // @v12.4.10 Важно! 
					// @v12.5.1 {
					if(r_bind.Flags & SSqlStmt::Bind::fUseRetActualSize) {
						bind_item.buffer_length = r_bind.NtvSize;
						bind_item.length = reinterpret_cast<ulong *>(&r_bind.NtvSize);
					}
					else {
						bind_item.buffer_length = r_bind.NtvSize;
					}
					// } @v12.5.1 
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
				if(!(r_bind.Flags & SSqlStmt::Bind::fSkip)) { // @v12.5.7 @condition
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
	bool   debug_mark = false; // @debug
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
		case S_DEC:
		case S_MONEY:
			if(action == 0) {
				pBind->SetNtvTypeAndSize(MYSQL_TYPE_DOUBLE, sizeof(double));
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
		//
		// Замечание по типам S_DATE и S_TIME.
		// Во время отладки разных механизмов возникали подозрения на неадекватность представления даты и времени в родном MySQL-формате.
		// В дальнейшем они не подтвердились, но я решил пока оставить представление этих типов как int32.
		// 
		case S_DATE:
			if(SMySqlDbProvider::TypeDateAsInt32) {
				ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_LONG);
			}
			else {
				uint32 ntv_size = 0;
				if(action == 0) {
					if(pBind->Pos < 0) {
						ntv_size = sizeof(MYSQL_TIME);
						pBind->SetNtvTypeAndSize(MYSQL_TYPE_DATE, ntv_size);
						pStmt->AllocBindSubst(count, pBind->NtvSize, pBind);
					}
					else {
						ntv_size = sizeof(MYSQL_TIME);
						pBind->SetNtvTypeAndSize(MYSQL_TYPE_DATE, ntv_size);
						pStmt->AllocBindSubst(count, pBind->NtvSize, pBind);
					}
				}
				else if(action < 0) {
					const  LDATE * p_dt = static_cast<LDATE *>(pBind->P_Data);
					LDATE  temp_dt;
					if(*p_dt == MAXDATE) {
						temp_dt.encode(31, 12, 2200);
					}
					else if(*p_dt == ZERODATE) {
						temp_dt.encode(1, 1, 1600);
					}
					else 
						temp_dt = *p_dt;
					{
						MYSQL_TIME * p_ocidt = static_cast<MYSQL_TIME *>(p_data);
						memzero(p_ocidt, sizeof(*p_ocidt));
						p_ocidt->year = temp_dt.year();
						p_ocidt->month = temp_dt.month();
						p_ocidt->day = temp_dt.day();
						p_ocidt->time_type = MYSQL_TIMESTAMP_DATE;
					}
				}
				else if(action == 1) {
					const MYSQL_TIME * p_ocidt = static_cast<const MYSQL_TIME *>(p_data);
					*static_cast<LDATE *>(pBind->P_Data) = encodedate(p_ocidt->day, p_ocidt->month, p_ocidt->year);
				}
				else if(action == 1000)
					ProcessBinding_FreeDescr(count, pStmt, pBind);
			}
			break;
		case S_TIME:
			if(SMySqlDbProvider::TypeTimeAsInt32) {
				ProcessBinding_SimpleType(action, count, pStmt, pBind, MYSQL_TYPE_LONG);
			}
			else {
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
			}
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
		case S_ZSTRING:
			{
				//const uint32 ntv_size = sz; // Удваиваем размер из-за того, что в сервере хранится utf8 а наруже мы (пока) используем OEM-encoding
					// При удваивании mysql отказывается принимать данные ссылаясь на перехлест длины поля.
				if(action == 0) {
					if(pBind->Pos < 0) {
						pBind->Flags |= SSqlStmt::Bind::fUseRetActualSize;
						pBind->SetNtvTypeAndSize(MYSQL_TYPE_VAR_STRING, static_cast<uint32>(sz));
						pStmt->AllocBindSubst(count, pBind->NtvSize*2, pBind);
					}
					else {
						pBind->SetNtvTypeAndSize(MYSQL_TYPE_VAR_STRING, static_cast<uint32>(sz*2));
						pStmt->AllocBindSubst(count, static_cast<uint32>(sz*2), pBind);
					}
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
							r_temp_buf.Cat(static_cast<const char *>(pBind->P_Data));
							if(!is_max) {
								r_temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
							}
							r_temp_buf.CopyUtf8To(p_outer, sz*2);
							pBind->NtvSize = r_temp_buf.Len32(); // без терминального нуля!
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
					const char * p_outer = static_cast<const char *>(p_data);
					if(pBind->P_Data && p_outer && pBind->NtvSize == ntv_size) {
						memcpy(pBind->P_Data, p_outer, ntv_size);
					}
				}
			}
			break;
		case S_CLOB:
			if(action == 0) {
				const uint32 ntv_size = 0;
				pBind->SetNtvTypeAndSize(MYSQL_TYPE_LONG_BLOB, ntv_size);
			}
			else if(action < 0) {
				if(p_data) {
					DBLobBlock * p_lb = pStmt->GetBindingLob();
					size_t lob_sz = 0;
					if(p_lb) {
						p_lb->GetSize(labs(pBind->Pos)-1, &lob_sz);
						SLob * p_lob = static_cast<SLob *>(p_data);
						const void * p_lob_data = p_lob->GetRawDataPtrC();
						if(p_lob_data && lob_sz) {
							SString temp_buf;
							temp_buf.CatN(static_cast<const char *>(p_lob_data), lob_sz).Transf(CTRANSF_INNER_TO_UTF8);
							mysql_stmt_send_long_data(static_cast<MYSQL_STMT *>(pStmt->H), abs(pBind->Pos)-1, temp_buf.cptr(), temp_buf.Len32());
						}
					}
				}
			}
			else if(action == 1) {
				if(p_data) {
					SLob * p_lob = static_cast<SLob *>(p_data);
					bool   _err = false;
					bool   _no_more_data = false;
					//SBuffer _buffer;
					SString temp_buf;
					ulong  _offs = 0;
					char   _chunk_buffer[2048];
					ulong  _total_len = 0;
					bool   blob_is_null = 0;
					int    blob_error = 0;
					{
						// Сначала узнаем размер
						MYSQL_BIND blob_bind;
						MEMSZERO(blob_bind);
						blob_bind.buffer_type = MYSQL_TYPE_LONG_BLOB;
						blob_bind.buffer = NULL;
						blob_bind.buffer_length = 0;
						blob_bind.length = &_total_len;
						blob_bind.is_null = &blob_is_null;
						blob_bind.P_Error = &blob_error;
						int   fcr = mysql_stmt_fetch_column(static_cast<MYSQL_STMT *>(pStmt->H), &blob_bind, 6, 0);
					}
					while(!_err && !_no_more_data && _offs < _total_len) {
						ulong  _len = 0;
						MYSQL_BIND msq_bind;
						MEMSZERO(msq_bind);
						msq_bind.buffer_type = MYSQL_TYPE_LONG_BLOB; // @v12.5.7 MYSQL_TYPE_VAR_STRING-->MYSQL_TYPE_BLOB
						msq_bind.buffer = _chunk_buffer;
						msq_bind.buffer_length = sizeof(_chunk_buffer);
						msq_bind.length = &_len;
						int fcr = mysql_stmt_fetch_column(static_cast<MYSQL_STMT *>(pStmt->H), &msq_bind, abs(pBind->Pos)-1, _offs);
						if(fcr == 0) {
							if(_len) {
								temp_buf.CatN(_chunk_buffer, _len);
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
						temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						p_lob->InitPtr(temp_buf.Len32()+1);
						void * p_lob_ptr = p_lob->GetRawDataPtr();
						if(p_lob_ptr) {
							strcpy(static_cast<char *>(p_lob_ptr), temp_buf.cptr());
						}
					}
				}
			}
			break;
		case S_BLOB:
			if(action == 0) {
				const uint32 ntv_size = 0;
				pBind->SetNtvTypeAndSize(MYSQL_TYPE_LONG_BLOB, ntv_size);
			}
			else if(action < 0) {
				if(p_data) {
					DBLobBlock * p_lb = pStmt->GetBindingLob();
					size_t lob_sz = 0;
					if(p_lb) {
						p_lb->GetSize(labs(pBind->Pos)-1, &lob_sz);
						SLob * p_lob = static_cast<SLob *>(p_data);
						const void * p_lob_data = p_lob->GetRawDataPtrC();
						if(p_lob_data && lob_sz) {
							mysql_stmt_send_long_data(static_cast<MYSQL_STMT *>(pStmt->H), abs(pBind->Pos)-1, static_cast<const char *>(p_lob_data), lob_sz);
						}
					}
				}
			}
			else if(action == 1) {
				if(p_data) {
					SLob * p_lob = static_cast<SLob *>(p_data);
					bool   _err = false;
					bool   _no_more_data = false;
					SBuffer _buffer;
					ulong  _offs = 0;
					char   _chunk_buffer[2048];
					ulong  _total_len = 0;
					bool   blob_is_null = 0;
					int    blob_error = 0;
					{
						// Сначала узнаем размер
						MYSQL_BIND blob_bind;
						MEMSZERO(blob_bind);
						blob_bind.buffer_type = MYSQL_TYPE_LONG_BLOB;
						blob_bind.buffer = NULL;
						blob_bind.buffer_length = 0;
						blob_bind.length = &_total_len;
						blob_bind.is_null = &blob_is_null;
						blob_bind.P_Error = &blob_error;
						int   fcr = mysql_stmt_fetch_column(static_cast<MYSQL_STMT *>(pStmt->H), &blob_bind, 6, 0);
					}
					while(!_err && !_no_more_data && _offs < _total_len) {
						ulong  _len = 0;
						MYSQL_BIND msq_bind;
						MEMSZERO(msq_bind);
						msq_bind.buffer_type = MYSQL_TYPE_LONG_BLOB; // @v12.5.7 MYSQL_TYPE_VAR_STRING-->MYSQL_TYPE_BLOB
						msq_bind.buffer = _chunk_buffer;
						msq_bind.buffer_length = sizeof(_chunk_buffer);
						msq_bind.length = &_len;
						int fcr = mysql_stmt_fetch_column(static_cast<MYSQL_STMT *>(pStmt->H), &msq_bind, abs(pBind->Pos)-1, _offs);
						if(fcr == 0) {
							if(_len) {
								_buffer.Write(_chunk_buffer, _len);
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
						p_lob->InitPtr(_buffer.GetAvailableSize());
						void * p_lob_ptr = p_lob->GetRawDataPtr();
						if(p_lob_ptr) {
							memcpy(p_lob_ptr, _buffer.GetBufC(), _buffer.GetAvailableSize());
						}
					}
				}
			}
			break;
		case S_RAW:
			{
				if(action == 0) {
					pBind->SetNtvTypeAndSize(MYSQL_TYPE_BLOB, static_cast<uint16>(sz));
					pStmt->AllocBindSubst(count, sz, pBind);
				}
				else if(action < 0) {
					char * p_outer = static_cast<char *>(p_data);
					if(p_outer && pBind->P_Data)
						memcpy(p_outer, pBind->P_Data, sz);
				}
				else if(action == 1) {
					const char * p_outer = static_cast<const char *>(p_data);
					if(pBind->P_Data && p_outer && pBind->NtvSize == sz) {
						memcpy(pBind->P_Data, p_outer, sz);
					}
				}
			}
			break;
		// (В MySQL нет rowid) case S_ROWID: break;
#if 0 // {
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
			rS.Flags &= ~SSqlStmt::fNoMoreData;
			ok = 1;
		}
		else if(_status == MYSQL_NO_DATA) {
			rS.Flags |= SSqlStmt::fNoMoreData;
			ok = -1;
		}
		else if(_status == MYSQL_DATA_TRUNCATED) { // @v12.4.12
			actual = 1; // @debug
			rS.Flags &= ~SSqlStmt::fNoMoreData;
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
