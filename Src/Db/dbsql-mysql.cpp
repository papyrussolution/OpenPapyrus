// DBSQL-MYSQL.CPP
// Copyright (c) A.Sobolev 2020
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <db.h>

SMySqlDbProvider::SMySqlDbProvider() :
	DbProvider(DbDictionary::CreateInstance(0, 0), DbProvider::cSQL|DbProvider::cDbDependTa), SqlGen(sqlstMySQL, 0), Flags(0)
{
}

SMySqlDbProvider::~SMySqlDbProvider()
{
}