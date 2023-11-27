// DBSQL-SQLITE.CPP
// Copyright (c) A.Sobolev 2021, 2022, 2023
// Модуль интерфейса с DBMS Sqlite3
// @construction
//
#include <slib-internal.h>
#pragma hdrstop
//#include <snet.h>
#include <..\osf\SQLite\sqlite3.h>

#ifndef NDEBUG
	#define DEBUG_LOG(msg) SLS.LogMessage("dbsqlite.log", msg, 0)
#else
	#define DEBUG_LOG(msg)
#endif

SSqliteDbProvider::SSqliteDbProvider() : DbProvider(DbDictionary::CreateInstance(0, 0), DbProvider::cSQL|DbProvider::cDbDependTa), SqlGen(sqlstSQLite, 0), H(0), Flags(0)
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
