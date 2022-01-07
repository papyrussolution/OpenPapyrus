// DBSQL-SQLITE.CPP
// Copyright (c) A.Sobolev 2021
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
}

/*virtual*/int SSqliteDbProvider::Login(const DbLoginBlock * pBlk, long options)
{
	int    ok = 1;
	sqlite3 * h = 0;
	SString path;
	assert(pBlk);
	THROW(pBlk);
	pBlk->GetAttr(DbLoginBlock::attrDbPath, path);

	CATCHZOK
	return ok;
}

/*virtual*/int SSqliteDbProvider::Logout()
{
	int    ok = 1;
	return ok;
}

