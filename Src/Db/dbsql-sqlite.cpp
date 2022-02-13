// DBSQL-SQLITE.CPP
// Copyright (c) A.Sobolev 2021, 2022
// Модуль интерфейса с DBMS Sqlite3
// @construction
//
#include <slib-internal.h>
#pragma hdrstop
#include <snet.h>
#include <db.h>
#include <..\OSF\SQLite\sqlite3.h>

SSqliteDbProvider::SSqliteDbProvider() : DbProvider(DbDictionary::CreateInstance(0, 0), DbProvider::cSQL|DbProvider::cDbDependTa), SqlGen(sqlstMySQL, 0), H(0), Flags(0)
{
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

/*virtual*/int SSqliteDbProvider::Exec(SSqlStmt & rS, uint count, int mode)
{
	return 0;
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
