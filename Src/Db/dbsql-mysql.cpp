// DBSQL-MYSQL.CPP
// Copyright (c) A.Sobolev 2020, 2021
//
#include <slib-internal.h>
#pragma hdrstop
#include <snet.h>
#include <db.h>

#if 1 // @construction {

#include <mariadb/mysql.h>

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

/*virtual*/int SMySqlDbProvider::Exec(SSqlStmt & rS, uint count, int mode)
{
	int    ok = 0;
	//mysql_real_query(static_cast<MYSQL *>(H), query, strlen(query));
	return ok;
}

/*virtual*/int SMySqlDbProvider::CreateDataFile(const DBTable * pTbl, const char * pFileName, int createMode, const char * pAltCode)
{
	int    ok = 1;
	// @construction {
	THROW(SqlGen.Reset().CreateTable(*pTbl, 0));
	{
		SSqlStmt stmt(this, (const SString &)SqlGen);
		THROW(stmt.Exec(1, OCI_DEFAULT));
	}
	uint j;
	for(j = 0; j < pTbl->indexes.getNumKeys(); j++) {
		THROW(SqlGen.Reset().CreateIndex(*pTbl, pFileName, j));
		{
			SSqlStmt stmt(this, (const SString &)SqlGen);
			THROW(stmt.Exec(1, OCI_DEFAULT));
		}
	}
	for(j = 0; j < pTbl->fields.getCount(); j++) {
		TYPEID _t = pTbl->fields[j].T;
		if(GETSTYPE(_t) == S_AUTOINC) {
			THROW(SqlGen.Reset().CreateSequenceOnField(*pTbl, pFileName, j, 0));
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
	return 0;
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

/*virtual*/int SMySqlDbProvider::GetFileStat(DBTable * pTbl, long reqItems, DbTableStat * pStat)
{
	// information_schema
	return 0;
}

/*virtual*/int SMySqlDbProvider::Implement_Open(DBTable * pTbl, const char * pFileName, int openMode, char * pPassword)
{
	return 0;
}

/*virtual*/int SMySqlDbProvider::Implement_Close(DBTable * pTbl)
{
	return 0;
}

/*virtual*/int SMySqlDbProvider::Implement_Search(DBTable * pTbl, int idx, void * pKey, int srchMode, long sf)
{
	return 0;
}

/*virtual*/int SMySqlDbProvider::Implement_InsertRec(DBTable * pTbl, int idx, void * pKeyBuf, const void * pData)
{
	return 0;
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
	if(pS)
		mysql_stmt_close(static_cast<MYSQL_STMT *>(pS->H));
	return 1;
}

/*static*/MYSQL_STMT * FASTCALL SMySqlDbProvider::StmtHandle(const SSqlStmt & rS)
{
	return static_cast<MYSQL_STMT *>(rS.H);
}

/*virtual*/int SMySqlDbProvider::Binding(SSqlStmt & rS, int dir)
{
	int    ok = 0;
	bool bind_result = true;
#if 1 // {
	const  uint row_count = rS.BL.Dim;
	const  uint col_count = rS.BL.getCount();
	MYSQL_STMT * h_stmt = StmtHandle(rS);
	TSVector <MYSQL_BIND> bind_list;
	assert(checkirange(row_count, 1, 1024));
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
				OCIDefine * p_bd = 0;
				THROW(ProcessBinding(0, row_count, &rS, &r_bind));
				{
					void * p_data = rS.GetBindOuterPtr(&r_bind, 0);
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
	int    ok = 0;
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
				pBind->SetNtvTypeAndSize(MYSQL_TYPE_VARCHAR, static_cast<uint16>(sz));
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
	return 0;
}

#endif // } 0 @construction 
