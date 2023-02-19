// dbkv-lmdb.cpp
// Copyright (c) A.Sobolev 2022
// Декларации модуля управления базой данных LMDB
// Attention! Don't include this file into slib.h or pp.h because of some symbols of LMDB duplicate other symbols in BerkeleyDB
//
#ifndef __DBKV_LMDB_H
#define __DBKV_LMDB_H

#include <..\slib\lmdb\lmdb.h>

class LmdbDatabase {
	friend TSClassWrapper <LmdbDatabase>;
public:
	static LmdbDatabase * GetInstance(const char * pPath, uint flags, int mode);

	class Table;
	struct Stat {
		uint   PageSize;          // Size of a database page. This is currently the same for all databases
		uint   BtreeDepth;        // Depth (height) of the B-tree 
		uint64 BranchPageCount;   // Number of internal (non-leaf) pages 
		uint64 LeafPageCount;     // Number of leaf pages 
		uint64 OverflowPageCount; // Number of overflow pages 
		uint64 EntryCount;        // Number of data items
	};
	class Transaction {
	public:
		friend class LmdbDatabase;
		enum {
			stUndef = 0,
			stCreated,
			stCommited,
			stAborted
		};
		Transaction(LmdbDatabase & rDb, bool readOnly);
		~Transaction();
		TSHandle <MDB_txn> GetHandle() const { return H; }
		LmdbDatabase & GetDb() const { return R_Db; }
		bool   IsValid() const;
		int    GetState() const { return State; }
		int    Commit();
		int    Abort();
	private:
		int   State;
		LmdbDatabase & R_Db;
		TSHandle <MDB_txn> H;
	};
	class Table {
	public:
		friend class Transaction;
		Table(Transaction & rTxn, const char * pName);
		~Table();
		bool   IsValid() const { return !!H; }
		uint   GetHandle() const { return H; }
		Transaction & GetTxn() { return R_Txn; }
		int    GetStat(LmdbDatabase::Stat & rS);
		int    Put(const void * pKey, size_t keyLen, const void * pVal, size_t valLen, uint flags);
		int    Del(const void * pKey, size_t keyLen, const void * pVal, size_t valLen);
		//
		// Returns:
		//   >0 - ключ pKey найден и данные, соответствующие ему, занесены в rValBuf
		//   <0 - ключ pKey не найден. Буфер rValBuf в результате пустой
		//    0 - error. Буфер rValBuf в результате пустой
		//
		int    Get(const void * pKey, size_t keyLen, SBaseBuffer & rValBuf);
		//
		// Descr: То же, что и Get(const void * pKey, size_t keyLen, SBaseBuffer & rValBuf), но
		//   результирующие данные заносятся в SBinaryChunk.
		// Returns:
		//   >0 - ключ pKey найден и данные, соответствующие ему, занесены в rValBuf
		//   <0 - ключ pKey не найден. Буфер rValBuf в результате пустой
		//    0 - error. Буфер rValBuf в результате пустой
		//
		int    Get(const void * pKey, size_t keyLen, SBinaryChunk & rValBuf);
		// MDB_NOOVERWRITE|MDB_NODUPDATA|MDB_RESERVE|MDB_APPEND|MDB_APPENDDUP
		//int mdb_put(MDB_txn * txn, MDB_dbi dbi, MDB_val * key, MDB_val * data, uint flags)
	private:
		Transaction & R_Txn;
		uint   H;
		SString Name;
	};
	class Cursor {
	public:
		Cursor(Table & rT);
		~Cursor();
	private:
		Table & R_T;
		TSHandle <MDB_cursor> H;
	};
	bool   SetOptions(uint64 mapSize, uint maxDbEntities, uint maxReaders);
	TSHandle <MDB_env> GetHandle() const { return H; }
	bool   GetStat(Stat & rS);
private:
	LmdbDatabase();
	~LmdbDatabase();
	bool   Open(const char * pPath, uint flags, int mode);
	bool   ProcessError(int r);
	int    LastErr;
	TSHandle <MDB_env> H;
};

#endif // __DBKV_LMDB_H