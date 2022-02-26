// DBKV-LMDB.CPP
// Copyright (c) A.Sobolev 2022
// @codepage UTF-8
// Интерфейс с key-value DBMS LMDB
//
#include <slib-internal.h>
#pragma hdrstop
#include <dbkv-lmdb.h>

/*static*/LmdbDatabase * LmdbDatabase::GetInstance(const char * pPath, uint flags, int mode)
{
	//static uint instance_idx = 0;
	LmdbDatabase * p_result = 0;
	if(!isempty(pPath)) {
		SString temp_buf;
		SString symb;
		SPathStruc::NormalizePath(pPath, SPathStruc::npfCompensateDotDot|SPathStruc::npfSlash, temp_buf);
		(symb = "lmdbenv").CatChar('-').Cat(temp_buf);
		long   symbol_id = SLS.GetGlobalSymbol(symb, -1, 0);
		if(symbol_id < 0) {
			TSClassWrapper <LmdbDatabase> cls;
			symbol_id = SLS.CreateGlobalObject(cls);
			p_result = static_cast<LmdbDatabase *>(SLS.GetGlobalObject(symbol_id));
			if(p_result) {
				p_result->SetOptions(SMEGABYTE(512), 16, 0);
				if(p_result->Open(pPath, flags, mode)) {
					{
						long s = SLS.GetGlobalSymbol(symb, symbol_id, 0);
						assert(symbol_id == s);
					}					
				}
				else {
					SLS.DestroyGlobalObject(symbol_id);
					p_result = 0;
				}
			}
		}
		else {
			p_result = static_cast<LmdbDatabase *>(SLS.GetGlobalObject(symbol_id));
		}
	}
	return p_result;
}

bool LmdbDatabase::ProcessError(int r)
{
	if(r == 0)
		return true;
	else
		return false;
}

LmdbDatabase::Transaction::Transaction(LmdbDatabase & rDb, bool readOnly) : State(stUndef), R_Db(rDb)
{
	MDB_txn * p_txn = 0;
	uint   txn_flags = readOnly ? MDB_RDONLY : 0;
	if(R_Db.ProcessError(mdb_txn_begin(R_Db.H, 0/*parent txn*/, txn_flags, &p_txn))) {
		H = p_txn;
		State = stCreated;
	}
}
		
LmdbDatabase::Transaction::~Transaction()
{
	if(H)
		Abort();
}

int LmdbDatabase::Transaction::Commit()
{
	int    ok = 1;
	if(State & stCreated) {
		THROW(H);
		THROW(R_Db.ProcessError(mdb_txn_commit(H)));
		State = stCommited;
		H = 0;
	}
	CATCHZOK
	return ok;
}

int LmdbDatabase::Transaction::Abort()
{
	int    ok = 1;
	if(State & stCreated) {
		THROW(H);
		mdb_txn_abort(H);
		State = stAborted;
		H = 0;
	}
	CATCHZOK
	return ok;
}

bool LmdbDatabase::Transaction::IsValid() const { return !!H; }

LmdbDatabase::Table::Table(Transaction & rTxn, const char * pName) : R_Txn(rTxn), H(0), Name(pName)
{
	//mdb_txn_begin()
	MDB_dbi dbi = 0;
	if(R_Txn.GetDb().ProcessError(mdb_dbi_open(R_Txn.GetHandle(), Name, MDB_CREATE/*flags*/, &dbi))) {
		H = dbi;
	}
}

LmdbDatabase::Table::~Table()
{
	//R_Txn.GetDb().GetHandle();
	//mdb_dbi_close()
}

int LmdbDatabase::Table::Get(const void * pKey, size_t keyLen, SBaseBuffer & rValBuf)
{
	int    ok = 1;
	rValBuf.Init();
	THROW(R_Txn.IsValid());
	{
		MDB_val key;
		MDB_val val;
		key.mv_data = const_cast<void *>(pKey);
		key.mv_size = keyLen;
		MEMSZERO(val);
		int r = mdb_get(R_Txn.GetHandle(), H, &key, &val);
		if(r == MDB_NOTFOUND) {
			ok = -1;
		}
		else {
			THROW(R_Txn.GetDb().ProcessError(r));
			rValBuf.Set(val.mv_data, val.mv_size);
		}
	}
	CATCHZOK
	return ok;
}

int LmdbDatabase::Table::Get(const void * pKey, size_t keyLen, SBinaryChunk & rValBuf)
{
	rValBuf.Z();
	SBaseBuffer b;
	int    ok = Get(pKey, keyLen, b);
	if(ok > 0) {
		assert(b.P_Buf);
		assert(b.Size > 0);
		ok = rValBuf.Put(b.P_Buf, b.Size);
	}
	return ok;
}

int LmdbDatabase::Table::GetStat(LmdbDatabase::Stat & rS)
{
	int    ok = 1;
	MDB_stat stat;
	THROW(R_Txn.IsValid());
	THROW(R_Txn.GetDb().ProcessError(mdb_stat(R_Txn.GetHandle(), H, &stat)));
	{
		rS.PageSize = stat.ms_psize;
		rS.BtreeDepth = stat.ms_depth;
		rS.BranchPageCount = stat.ms_branch_pages;
		rS.LeafPageCount = stat.ms_leaf_pages;
		rS.OverflowPageCount = stat.ms_overflow_pages;
		rS.EntryCount = stat.ms_entries;
	}
	CATCHZOK
	return ok;
}

int LmdbDatabase::Table::Put(const void * pKey, size_t keyLen, const void * pVal, size_t valLen, uint flags)
{
	// MDB_NOOVERWRITE|MDB_NODUPDATA|MDB_RESERVE|MDB_APPEND|MDB_APPENDDUP
	//int mdb_put(MDB_txn * txn, MDB_dbi dbi, MDB_val * key, MDB_val * data, uint flags)
	int    ok = 1;
	THROW(R_Txn.IsValid());
	{
		MDB_val key;
		MDB_val val;
		key.mv_data = const_cast<void *>(pKey);
		key.mv_size = keyLen;
		val.mv_data = const_cast<void *>(pVal);
		val.mv_size = valLen;
		THROW(R_Txn.GetDb().ProcessError(mdb_put(R_Txn.GetHandle(), H, &key, &val, flags)));
	}
	CATCHZOK
	return ok;
}

int LmdbDatabase::Table::Del(const void * pKey, size_t keyLen, const void * pVal, size_t valLen)
{
	int    ok = 1;
	THROW(R_Txn.IsValid());
	{
		MDB_val key;
		MDB_val val;
		MDB_val * p_val = 0;
		key.mv_data = const_cast<void *>(pKey);
		key.mv_size = keyLen;
		if(pVal && valLen) {
			val.mv_data = const_cast<void *>(pVal);
			val.mv_size = valLen;
			p_val = &val;
		}
		THROW(R_Txn.GetDb().ProcessError(mdb_del(R_Txn.GetHandle(), H, &key, p_val)));
	}
	CATCHZOK
	return ok;
}

LmdbDatabase::Cursor::Cursor(Table & rT) : R_T(rT)
{
	MDB_cursor * p_cur = 0;
	if(R_T.GetTxn().GetDb().ProcessError(mdb_cursor_open(R_T.GetTxn().GetHandle(), R_T.GetHandle(), &p_cur))) {
		H = p_cur;
	}
}

LmdbDatabase::Cursor::~Cursor()
{
	mdb_cursor_close(H);
}

LmdbDatabase::LmdbDatabase() : H(0), LastErr(0)
{
	MDB_env * p_env = 0;
	if(mdb_env_create(&p_env) == 0) {
		H = p_env;
	}
}
	
LmdbDatabase::~LmdbDatabase()
{
	if(H) {
		mdb_env_close(H);
		H = 0;
	}
}

bool LmdbDatabase::Open(const char * pPath, uint flags, int mode)
{
	bool   ok = true;
	THROW(H);
	if(mdb_env_open(H, pPath, flags, mode) == 0) {
		;
	}
	else {
		mdb_env_close(H);
		H = 0;
		ok = false;
	}
	CATCHZOK
	return ok;
}

bool LmdbDatabase::SetOptions(uint64 mapSize, uint maxDbEntities, uint maxReaders)
{
	bool   ok = true;
	THROW(H);
	if(mapSize) {
		THROW(ProcessError(mdb_env_set_mapsize(H, static_cast<size_t>(mapSize))));
	}
	if(maxDbEntities) {
		THROW(ProcessError(mdb_env_set_maxdbs(H, maxDbEntities)));
	}
	if(maxReaders) {
		THROW(ProcessError(mdb_env_set_maxreaders(H, maxReaders)));
	}
	CATCHZOK
	return ok;
}

bool LmdbDatabase::GetStat(Stat & rS)
{
	bool   ok = true;
	MDB_stat native_stat;
	THROW(H);
	THROW(ProcessError(mdb_env_stat(H, &native_stat)));
	rS.PageSize = native_stat.ms_psize;
	rS.BtreeDepth = native_stat.ms_depth;
	rS.BranchPageCount = native_stat.ms_branch_pages;
	rS.LeafPageCount = native_stat.ms_leaf_pages;
	rS.OverflowPageCount = native_stat.ms_overflow_pages;
	rS.EntryCount = native_stat.ms_entries;
	CATCHZOK
	return ok;
}
