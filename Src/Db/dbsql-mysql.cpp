// DBSQL-MYSQL.CPP
// Copyright (c) A.Sobolev 2020
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <db.h>
#include <mysql/mysql.h>

#if 0 // @construction {

SMySqlDbProvider::SMySqlDbProvider() :
	DbProvider(DbDictionary::CreateInstance(0, 0), DbProvider::cSQL|DbProvider::cDbDependTa), H(0), SqlGen(sqlstMySQL, 0), Flags(0)
{
}

SMySqlDbProvider::~SMySqlDbProvider()
{
}

/*virtual*/int SMySqlDbProvider::Login(const DbLoginBlock * pBlk, long options)
{
	int    ok = 0;
	H = mysql_init(0);
	if(H) {
		MYSQL * p_h = static_cast<MYSQL *>(H);
		SString attr;
		pBlk->GetAttr(DbLoginBlock::attrDbName, attr);
		{
			int   port = 0;
			SString host, port_s, sid;
			SString user, pw;
			SString dbsymb;
			StringSet ss(':', attr);
			uint ssp = 0;
			if(ss.get(&ssp, host)) {
				if(ss.get(&ssp, port_s)) {
					if(ss.get(&ssp, sid)) {
						port = port_s.ToULong();
					}
					else {
						sid = port_s;
						port = 3306;
					}
				}
				else {
					sid = host;
					host = "localhost";
					port = 3306;
				}
			}
			pBlk->GetAttr(DbLoginBlock::attrDbSymb, dbsymb);
			pBlk->GetAttr(DbLoginBlock::attrUserName, user);
			pBlk->GetAttr(DbLoginBlock::attrPassword, pw);
			if(mysql_real_connect(p_h, host, user, pw, dbsymb, port, 0/*unix-socket*/, 0/*client_flags*/)) {
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

#endif // } 0 @construction 
