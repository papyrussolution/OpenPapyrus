/* mtest.c - memory-mapped database tester/toy */
/*
 * Copyright 2011-2021 Howard Chu, Symas Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
#include <slib-internal.h>
#pragma hdrstop
//#include <stdio.h>
//#include <stdlib.h>
//#include <time.h>
#include "lmdb.h"

char * mdb_dkey(MDB_val * key, char * buf); // @prototype (mdb.c)

static void Lmdb_CreateTestDirectory(SString & rPath)
{
	SLS.QueryPath("testroot", rPath);
	rPath.SetLastSlash().Cat("out").SetLastSlash().Cat("lmdb-test");
	::createDir(rPath);
}

static int Lmdb_Random(ulong _lim)
{
	return SLS.GetTLA().Rg.GetUniformInt(_lim);
}

static int * Lmdb_AllocateTestVector(int & rCount)
{
	rCount = SLS.GetTLA().Rg.GetUniformInt(384) + 64;
	int * p_values = (int *)SAlloc::M(rCount * sizeof(int));
	for(int i = 0; i < rCount; i++) {
		//values[i] = rand()%1024;
		p_values[i] = SLS.GetTLA().Rg.GetUniformInt(1024);
	}
	return p_values;
}

#if 0 // {

#define E(expr) CHECK((rc = (expr)) == MDB_SUCCESS, #expr)
#define RES(err, expr) ((rc = expr) == (err) || (CHECK(!rc, #expr), 0))
#define CHECK(test, msg) ((test) ? (void)0 : ((void)slfprintf_stderr("%s:%d: %s: %s\n", __FILE__, __LINE__, msg, mdb_strerror(rc)), abort()))

static int Lmdb_Test01()
{
	int i = 0, j = 0, rc;
	MDB_env * env;
	MDB_dbi dbi;
	MDB_val key, data;
	MDB_txn * txn;
	MDB_stat mst;
	MDB_cursor * cursor, * cur2;
	MDB_cursor_op op;
	char sval[32] = "";
	SString db_path;
	Lmdb_CreateTestDirectory(db_path);
	int count;
	int * values = Lmdb_AllocateTestVector(count);
	E(mdb_env_create(&env));
	E(mdb_env_set_maxreaders(env, 1));
	E(mdb_env_set_mapsize(env, 10485760));
	E(mdb_env_open(env, /*"./testdb"*/db_path, MDB_FIXEDMAP /*|MDB_NOSYNC*/, 0664));
	E(mdb_txn_begin(env, NULL, 0, &txn));
	E(mdb_dbi_open(txn, NULL, 0, &dbi));
	key.mv_size = sizeof(int);
	key.mv_data = sval;
	printf("Adding %d values\n", count);
	for(i = 0; i<count; i++) {
		sprintf(sval, "%03x %d foo bar", values[i], values[i]);
		/* Set <data> in each iteration, since MDB_NOOVERWRITE may modify it */
		data.mv_size = sizeof(sval);
		data.mv_data = sval;
		if(RES(MDB_KEYEXIST, mdb_put(txn, dbi, &key, &data, MDB_NOOVERWRITE))) {
			j++;
			data.mv_size = sizeof(sval);
			data.mv_data = sval;
		}
	}
	if(j) 
		printf("%d duplicates skipped\n", j);
	E(mdb_txn_commit(txn));
	E(mdb_env_stat(env, &mst));
	E(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn));
	E(mdb_cursor_open(txn, dbi, &cursor));
	while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
		printf("key: %p %.*s, data: %p %.*s\n", key.mv_data,  (int)key.mv_size,  (char *)key.mv_data, data.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	mdb_cursor_close(cursor);
	mdb_txn_abort(txn);
	j = 0;
	key.mv_data = sval;
	for(i = count - 1; i > -1; i -= Lmdb_Random(5)) {
		j++;
		txn = NULL;
		E(mdb_txn_begin(env, NULL, 0, &txn));
		sprintf(sval, "%03x ", values[i]);
		if(RES(MDB_NOTFOUND, mdb_del(txn, dbi, &key, NULL))) {
			j--;
			mdb_txn_abort(txn);
		}
		else {
			E(mdb_txn_commit(txn));
		}
	}
	SAlloc::F(values);
	printf("Deleted %d values\n", j);
	E(mdb_env_stat(env, &mst));
	E(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn));
	E(mdb_cursor_open(txn, dbi, &cursor));
	printf("Cursor next\n");
	while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
		printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	printf("Cursor last\n");
	E(mdb_cursor_get(cursor, &key, &data, MDB_LAST));
	printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
	printf("Cursor prev\n");
	while((rc = mdb_cursor_get(cursor, &key, &data, MDB_PREV)) == 0) {
		printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	printf("Cursor last/prev\n");
	E(mdb_cursor_get(cursor, &key, &data, MDB_LAST));
	printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
	E(mdb_cursor_get(cursor, &key, &data, MDB_PREV));
	printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
	mdb_cursor_close(cursor);
	mdb_txn_abort(txn);
	printf("Deleting with cursor\n");
	E(mdb_txn_begin(env, NULL, 0, &txn));
	E(mdb_cursor_open(txn, dbi, &cur2));
	for(i = 0; i<50; i++) {
		if(RES(MDB_NOTFOUND, mdb_cursor_get(cur2, &key, &data, MDB_NEXT)))
			break;
		printf("key: %p %.*s, data: %p %.*s\n", key.mv_data,  (int)key.mv_size,  (char *)key.mv_data, data.mv_data, (int)data.mv_size, (char *)data.mv_data);
		E(mdb_del(txn, dbi, &key, NULL));
	}
	printf("Restarting cursor in txn\n");
	for(op = MDB_FIRST, i = 0; i<=32; op = MDB_NEXT, i++) {
		if(RES(MDB_NOTFOUND, mdb_cursor_get(cur2, &key, &data, op)))
			break;
		printf("key: %p %.*s, data: %p %.*s\n", key.mv_data,  (int)key.mv_size,  (char *)key.mv_data, data.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	mdb_cursor_close(cur2);
	E(mdb_txn_commit(txn));
	printf("Restarting cursor outside txn\n");
	E(mdb_txn_begin(env, NULL, 0, &txn));
	E(mdb_cursor_open(txn, dbi, &cursor));
	for(op = MDB_FIRST, i = 0; i<=32; op = MDB_NEXT, i++) {
		if(RES(MDB_NOTFOUND, mdb_cursor_get(cursor, &key, &data, op)))
			break;
		printf("key: %p %.*s, data: %p %.*s\n", key.mv_data,  (int)key.mv_size,  (char *)key.mv_data, data.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	mdb_cursor_close(cursor);
	mdb_txn_abort(txn);
	mdb_dbi_close(env, dbi);
	mdb_env_close(env);
	return 0;
}
//
// Just like Lmdb_Test01, but using a subDB instead of the main DB 
//
static int Lmdb_Test02()
{
	int i = 0, j = 0, rc;
	MDB_env * env;
	MDB_dbi dbi;
	MDB_val key, data;
	MDB_txn * txn;
	MDB_stat mst;
	MDB_cursor * cursor;
	char sval[32] = "";
	SString db_path;
	Lmdb_CreateTestDirectory(db_path);
	int count;
	int * values = Lmdb_AllocateTestVector(count);
	E(mdb_env_create(&env));
	E(mdb_env_set_maxreaders(env, 1));
	E(mdb_env_set_mapsize(env, 10485760));
	E(mdb_env_set_maxdbs(env, 4));
	E(mdb_env_open(env, /*"./testdb"*/db_path, MDB_FIXEDMAP|MDB_NOSYNC, 0664));
	E(mdb_txn_begin(env, NULL, 0, &txn));
	E(mdb_dbi_open(txn, "id1", MDB_CREATE, &dbi));
	key.mv_size = sizeof(int);
	key.mv_data = sval;
	printf("Adding %d values\n", count);
	for(i = 0; i<count; i++) {
		sprintf(sval, "%03x %d foo bar", values[i], values[i]);
		data.mv_size = sizeof(sval);
		data.mv_data = sval;
		if(RES(MDB_KEYEXIST, mdb_put(txn, dbi, &key, &data, MDB_NOOVERWRITE)))
			j++;
	}
	if(j) 
		printf("%d duplicates skipped\n", j);
	E(mdb_txn_commit(txn));
	E(mdb_env_stat(env, &mst));
	E(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn));
	E(mdb_cursor_open(txn, dbi, &cursor));
	while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
		printf("key: %p %.*s, data: %p %.*s\n", key.mv_data,  (int)key.mv_size,  (char *)key.mv_data, data.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	mdb_cursor_close(cursor);
	mdb_txn_abort(txn);
	j = 0;
	key.mv_data = sval;
	for(i = count - 1; i > -1; i -= Lmdb_Random(5)) {
		j++;
		txn = NULL;
		E(mdb_txn_begin(env, NULL, 0, &txn));
		sprintf(sval, "%03x ", values[i]);
		if(RES(MDB_NOTFOUND, mdb_del(txn, dbi, &key, NULL))) {
			j--;
			mdb_txn_abort(txn);
		}
		else {
			E(mdb_txn_commit(txn));
		}
	}
	SAlloc::F(values);
	printf("Deleted %d values\n", j);
	E(mdb_env_stat(env, &mst));
	E(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn));
	E(mdb_cursor_open(txn, dbi, &cursor));
	printf("Cursor next\n");
	while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
		printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	printf("Cursor prev\n");
	while((rc = mdb_cursor_get(cursor, &key, &data, MDB_PREV)) == 0) {
		printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	mdb_cursor_close(cursor);
	mdb_txn_abort(txn);
	mdb_dbi_close(env, dbi);
	mdb_env_close(env);
	return 0;
}
//
// Tests for sorted duplicate DBs 
//
static int Lmdb_Test03()
{
	int i = 0, j = 0, rc;
	MDB_env * env;
	MDB_dbi dbi;
	MDB_val key, data;
	MDB_txn * txn;
	MDB_stat mst;
	MDB_cursor * cursor;
	char sval[32];
	char kval[sizeof(int)];
	SString db_path;
	Lmdb_CreateTestDirectory(db_path);
	//srand((uint)time(NULL));
	memzero(sval, sizeof(sval));
	int count;
	int * values = Lmdb_AllocateTestVector(count);
	E(mdb_env_create(&env));
	E(mdb_env_set_mapsize(env, 10485760));
	E(mdb_env_set_maxdbs(env, 4));
	E(mdb_env_open(env, /*"./testdb"*/db_path, MDB_FIXEDMAP|MDB_NOSYNC, 0664));
	E(mdb_txn_begin(env, NULL, 0, &txn));
	E(mdb_dbi_open(txn, "id2", MDB_CREATE|MDB_DUPSORT, &dbi));
	key.mv_size = sizeof(int);
	key.mv_data = kval;
	data.mv_size = sizeof(sval);
	data.mv_data = sval;
	printf("Adding %d values\n", count);
	for(i = 0; i<count; i++) {
		if(!(i & 0x0f))
			sprintf(kval, "%03x", values[i]);
		sprintf(sval, "%03x %d foo bar", values[i], values[i]);
		if(RES(MDB_KEYEXIST, mdb_put(txn, dbi, &key, &data, MDB_NODUPDATA)))
			j++;
	}
	if(j) 
		printf("%d duplicates skipped\n", j);
	E(mdb_txn_commit(txn));
	E(mdb_env_stat(env, &mst));
	E(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn));
	E(mdb_cursor_open(txn, dbi, &cursor));
	while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
		printf("key: %p %.*s, data: %p %.*s\n", key.mv_data,  (int)key.mv_size,  (char *)key.mv_data, data.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	mdb_cursor_close(cursor);
	mdb_txn_abort(txn);
	j = 0;
	for(i = count - 1; i > -1; i -= Lmdb_Random(5)) {
		j++;
		txn = NULL;
		E(mdb_txn_begin(env, NULL, 0, &txn));
		sprintf(kval, "%03x", values[i & ~0x0f]);
		sprintf(sval, "%03x %d foo bar", values[i], values[i]);
		key.mv_size = sizeof(int);
		key.mv_data = kval;
		data.mv_size = sizeof(sval);
		data.mv_data = sval;
		if(RES(MDB_NOTFOUND, mdb_del(txn, dbi, &key, &data))) {
			j--;
			mdb_txn_abort(txn);
		}
		else {
			E(mdb_txn_commit(txn));
		}
	}
	SAlloc::F(values);
	printf("Deleted %d values\n", j);
	E(mdb_env_stat(env, &mst));
	E(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn));
	E(mdb_cursor_open(txn, dbi, &cursor));
	printf("Cursor next\n");
	while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
		printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	printf("Cursor prev\n");
	while((rc = mdb_cursor_get(cursor, &key, &data, MDB_PREV)) == 0) {
		printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	mdb_cursor_close(cursor);
	mdb_txn_abort(txn);
	mdb_dbi_close(env, dbi);
	mdb_env_close(env);
	return 0;
}
//
// Tests for sorted duplicate DBs with fixed-size keys 
//
static int Lmdb_Test04()
{
	int i = 0, j = 0, rc;
	MDB_env * env;
	MDB_dbi dbi;
	MDB_val key, data;
	MDB_txn * txn;
	MDB_stat mst;
	MDB_cursor * cursor;
	char sval[8];
	char kval[sizeof(int)];
	SString db_path;
	Lmdb_CreateTestDirectory(db_path);
	memzero(sval, sizeof(sval));
	const int count = 510;
	int * values = (int *)SAlloc::M(count*sizeof(int));
	for(i = 0; i < count; i++) {
		values[i] = i*5;
	}
	E(mdb_env_create(&env));
	E(mdb_env_set_mapsize(env, 10485760));
	E(mdb_env_set_maxdbs(env, 4));
	E(mdb_env_open(env, /*"./testdb"*/db_path, MDB_FIXEDMAP|MDB_NOSYNC, 0664));
	E(mdb_txn_begin(env, NULL, 0, &txn));
	E(mdb_dbi_open(txn, "id4", MDB_CREATE|MDB_DUPSORT|MDB_DUPFIXED, &dbi));
	key.mv_size = sizeof(int);
	key.mv_data = kval;
	data.mv_size = sizeof(sval);
	data.mv_data = sval;
	printf("Adding %d values\n", count);
	strcpy(kval, "001");
	for(i = 0; i<count; i++) {
		sprintf(sval, "%07x", values[i]);
		if(RES(MDB_KEYEXIST, mdb_put(txn, dbi, &key, &data, MDB_NODUPDATA)))
			j++;
	}
	if(j) 
		printf("%d duplicates skipped\n", j);
	E(mdb_txn_commit(txn));
	E(mdb_env_stat(env, &mst));
	/* there should be one full page of dups now.
	 */
	E(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn));
	E(mdb_cursor_open(txn, dbi, &cursor));
	while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
		printf("key: %p %.*s, data: %p %.*s\n", key.mv_data,  (int)key.mv_size,  (char *)key.mv_data, data.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	mdb_cursor_close(cursor);
	mdb_txn_abort(txn);
	/* test all 3 branches of split code:
	 * 1: new key in lower half
	 * 2: new key at split point
	 * 3: new key in upper half
	 */
	key.mv_size = sizeof(int);
	key.mv_data = kval;
	data.mv_size = sizeof(sval);
	data.mv_data = sval;
	sprintf(sval, "%07x", values[3]+1);
	E(mdb_txn_begin(env, NULL, 0, &txn));
	RES(MDB_KEYEXIST, mdb_put(txn, dbi, &key, &data, MDB_NODUPDATA));
	mdb_txn_abort(txn);
	sprintf(sval, "%07x", values[255]+1);
	E(mdb_txn_begin(env, NULL, 0, &txn));
	RES(MDB_KEYEXIST, mdb_put(txn, dbi, &key, &data, MDB_NODUPDATA));
	mdb_txn_abort(txn);
	sprintf(sval, "%07x", values[500]+1);
	E(mdb_txn_begin(env, NULL, 0, &txn));
	RES(MDB_KEYEXIST, mdb_put(txn, dbi, &key, &data, MDB_NODUPDATA));
	E(mdb_txn_commit(txn));
	/* Try MDB_NEXT_MULTIPLE */
	E(mdb_txn_begin(env, NULL, 0, &txn));
	E(mdb_cursor_open(txn, dbi, &cursor));
	while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT_MULTIPLE)) == 0) {
		printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	mdb_cursor_close(cursor);
	mdb_txn_abort(txn);
	j = 0;
	for(i = count - 1; i > -1; i -= Lmdb_Random(3)) {
		j++;
		txn = NULL;
		E(mdb_txn_begin(env, NULL, 0, &txn));
		sprintf(sval, "%07x", values[i]);
		key.mv_size = sizeof(int);
		key.mv_data = kval;
		data.mv_size = sizeof(sval);
		data.mv_data = sval;
		if(RES(MDB_NOTFOUND, mdb_del(txn, dbi, &key, &data))) {
			j--;
			mdb_txn_abort(txn);
		}
		else {
			E(mdb_txn_commit(txn));
		}
	}
	SAlloc::F(values);
	printf("Deleted %d values\n", j);
	E(mdb_env_stat(env, &mst));
	E(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn));
	E(mdb_cursor_open(txn, dbi, &cursor));
	printf("Cursor next\n");
	while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
		printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	printf("Cursor prev\n");
	while((rc = mdb_cursor_get(cursor, &key, &data, MDB_PREV)) == 0) {
		printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	mdb_cursor_close(cursor);
	mdb_txn_abort(txn);

	mdb_dbi_close(env, dbi);
	mdb_env_close(env);
	return 0;
}
//
// Tests for sorted duplicate DBs using cursor_put 
//
static int Lmdb_Test05()
{
	int i = 0, j = 0, rc;
	MDB_env * env;
	MDB_dbi dbi;
	MDB_val key, data;
	MDB_txn * txn;
	MDB_stat mst;
	MDB_cursor * cursor;
	char sval[32];
	char kval[sizeof(int)];
	SString db_path;
	Lmdb_CreateTestDirectory(db_path);
	memzero(sval, sizeof(sval));
	int count;
	int * values = Lmdb_AllocateTestVector(count);
	E(mdb_env_create(&env));
	E(mdb_env_set_mapsize(env, 10485760));
	E(mdb_env_set_maxdbs(env, 4));
	E(mdb_env_open(env, /*"./testdb"*/db_path, MDB_FIXEDMAP|MDB_NOSYNC, 0664));

	E(mdb_txn_begin(env, NULL, 0, &txn));
	E(mdb_dbi_open(txn, "id2", MDB_CREATE|MDB_DUPSORT, &dbi));
	E(mdb_cursor_open(txn, dbi, &cursor));

	key.mv_size = sizeof(int);
	key.mv_data = kval;
	data.mv_size = sizeof(sval);
	data.mv_data = sval;

	printf("Adding %d values\n", count);
	for(i = 0; i<count; i++) {
		if(!(i & 0x0f))
			sprintf(kval, "%03x", values[i]);
		sprintf(sval, "%03x %d foo bar", values[i], values[i]);
		if(RES(MDB_KEYEXIST, mdb_cursor_put(cursor, &key, &data, MDB_NODUPDATA)))
			j++;
	}
	if(j) 
		printf("%d duplicates skipped\n", j);
	mdb_cursor_close(cursor);
	E(mdb_txn_commit(txn));
	E(mdb_env_stat(env, &mst));
	E(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn));
	E(mdb_cursor_open(txn, dbi, &cursor));
	while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
		printf("key: %p %.*s, data: %p %.*s\n", key.mv_data,  (int)key.mv_size,  (char *)key.mv_data, data.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	mdb_cursor_close(cursor);
	mdb_txn_abort(txn);
	j = 0;
	for(i = count - 1; i > -1; i -= Lmdb_Random(5)) {
		j++;
		txn = NULL;
		E(mdb_txn_begin(env, NULL, 0, &txn));
		sprintf(kval, "%03x", values[i & ~0x0f]);
		sprintf(sval, "%03x %d foo bar", values[i], values[i]);
		key.mv_size = sizeof(int);
		key.mv_data = kval;
		data.mv_size = sizeof(sval);
		data.mv_data = sval;
		if(RES(MDB_NOTFOUND, mdb_del(txn, dbi, &key, &data))) {
			j--;
			mdb_txn_abort(txn);
		}
		else {
			E(mdb_txn_commit(txn));
		}
	}
	SAlloc::F(values);
	printf("Deleted %d values\n", j);
	E(mdb_env_stat(env, &mst));
	E(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn));
	E(mdb_cursor_open(txn, dbi, &cursor));
	printf("Cursor next\n");
	while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
		printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	printf("Cursor prev\n");
	while((rc = mdb_cursor_get(cursor, &key, &data, MDB_PREV)) == 0) {
		printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	mdb_cursor_close(cursor);
	mdb_txn_abort(txn);
	mdb_dbi_close(env, dbi);
	mdb_env_close(env);
	return 0;
}
#if MDB_DEBUG // {
//
// Tests for DB splits and merges
//
static int Lmdb_Test06()
{
	static char dkbuf[1024];
	int i = 0, j = 0, rc;
	MDB_env * env;
	MDB_dbi dbi;
	MDB_val key, data, sdata;
	MDB_txn * txn;
	MDB_stat mst;
	MDB_cursor * cursor;
	int count;
	int * values;
	long kval;
	char * sval;
	SString db_path;
	Lmdb_CreateTestDirectory(db_path);
	srand((uint)time(NULL));
	E(mdb_env_create(&env));
	E(mdb_env_set_mapsize(env, 10485760));
	E(mdb_env_set_maxdbs(env, 4));
	E(mdb_env_open(env, /*"./testdb"*/db_path, MDB_FIXEDMAP|MDB_NOSYNC, 0664));
	E(mdb_txn_begin(env, NULL, 0, &txn));
	E(mdb_dbi_open(txn, "id6", MDB_CREATE|MDB_INTEGERKEY, &dbi));
	E(mdb_cursor_open(txn, dbi, &cursor));
	E(mdb_stat(txn, dbi, &mst));
	sval = (char *)SAlloc::C(1, mst.ms_psize / 4);
	key.mv_size = sizeof(long);
	key.mv_data = &kval;
	sdata.mv_size = mst.ms_psize / 4 - 30;
	sdata.mv_data = sval;
	printf("Adding 12 values, should yield 3 splits\n");
	for(i = 0; i<12; i++) {
		kval = i*5;
		sprintf(sval, "%08x", kval);
		data = sdata;
		RES(MDB_KEYEXIST, mdb_cursor_put(cursor, &key, &data, MDB_NOOVERWRITE));
	}
	printf("Adding 12 more values, should yield 3 splits\n");
	for(i = 0; i<12; i++) {
		kval = i*5+4;
		sprintf(sval, "%08x", kval);
		data = sdata;
		RES(MDB_KEYEXIST, mdb_cursor_put(cursor, &key, &data, MDB_NOOVERWRITE));
	}
	printf("Adding 12 more values, should yield 3 splits\n");
	for(i = 0; i<12; i++) {
		kval = i*5+1;
		sprintf(sval, "%08x", kval);
		data = sdata;
		RES(MDB_KEYEXIST, mdb_cursor_put(cursor, &key, &data, MDB_NOOVERWRITE));
	}
	E(mdb_cursor_get(cursor, &key, &data, MDB_FIRST));
	do {
		printf("key: %p %s, data: %p %.*s\n", key.mv_data,  mdb_dkey(&key, dkbuf), data.mv_data, (int)data.mv_size, (char *)data.mv_data);
	} while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0);
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	mdb_cursor_close(cursor);
	mdb_txn_commit(txn);
#if 0
	j = 0;
	for(i = count - 1; i > -1; i -= Lmdb_Random(5)) {
		j++;
		txn = NULL;
		E(mdb_txn_begin(env, NULL, 0, &txn));
		sprintf(kval, "%03x", values[i & ~0x0f]);
		sprintf(sval, "%03x %d foo bar", values[i], values[i]);
		key.mv_size = sizeof(int);
		key.mv_data = kval;
		data.mv_size = sizeof(sval);
		data.mv_data = sval;
		if(RES(MDB_NOTFOUND, mdb_del(txn, dbi, &key, &data))) {
			j--;
			mdb_txn_abort(txn);
		}
		else {
			E(mdb_txn_commit(txn));
		}
	}
	SAlloc::F(values);
	printf("Deleted %d values\n", j);
	E(mdb_env_stat(env, &mst));
	E(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn));
	E(mdb_cursor_open(txn, dbi, &cursor));
	printf("Cursor next\n");
	while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
		printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	printf("Cursor prev\n");
	while((rc = mdb_cursor_get(cursor, &key, &data, MDB_PREV)) == 0) {
		printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
	}
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	mdb_cursor_close(cursor);
	mdb_txn_abort(txn);
	mdb_dbi_close(env, dbi);
#endif
	mdb_env_close(env);
	return 0;
}
#endif // } MDB_DEBUG

int Lmdb_Test()
{
	int   ok = 1;
	Lmdb_Test01();
	Lmdb_Test02();
	Lmdb_Test03();
	Lmdb_Test04();
	Lmdb_Test05();
#if MDB_DEBUG
	Lmdb_Test06();
#endif
	return ok;
}
#endif // } 0

#if SLTEST_RUNNING

int DummyProc_LMDB() { return 1; } // @forcelink

SLTEST_R(LMDB)
{
	int    ok = 1;
	SString db_path;
	MDB_val key, data;
	MDB_cursor * cursor;
	//static int Lmdb_Test01()
	{
		int i = 0, j = 0, rc;
		MDB_env * env;
		MDB_dbi dbi;
		MDB_txn * txn;
		MDB_stat mst;
		MDB_cursor * cur2;
		MDB_cursor_op op;
		char sval[32] = "";
		Lmdb_CreateTestDirectory(db_path);
		int count;
		int * values = Lmdb_AllocateTestVector(count);
		THROW(SLTEST_CHECK_Z(mdb_env_create(&env)));
		THROW(SLTEST_CHECK_Z(mdb_env_set_maxreaders(env, 1)));
		THROW(SLTEST_CHECK_Z(mdb_env_set_mapsize(env, 10485760)));
		THROW(SLTEST_CHECK_Z(mdb_env_open(env, /*"./testdb"*/db_path, MDB_FIXEDMAP /*|MDB_NOSYNC*/, 0664)));
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, 0, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_dbi_open(txn, NULL, 0, &dbi)));
		key.mv_size = sizeof(int);
		key.mv_data = sval;
		printf("Adding %d values\n", count);
		for(i = 0; i<count; i++) {
			sprintf(sval, "%03x %d foo bar", values[i], values[i]);
			// Set <data> in each iteration, since MDB_NOOVERWRITE may modify it 
			data.mv_size = sizeof(sval);
			data.mv_data = sval;
			//if(RES(MDB_KEYEXIST, mdb_put(txn, dbi, &key, &data, MDB_NOOVERWRITE))) {
			rc = mdb_put(txn, dbi, &key, &data, MDB_NOOVERWRITE);
			THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_KEYEXIST, 0)));
			if(rc == MDB_KEYEXIST) {
				j++;
				data.mv_size = sizeof(sval);
				data.mv_data = sval;
			}
		}
		if(j) 
			printf("%d duplicates skipped\n", j);
		THROW(SLTEST_CHECK_Z(mdb_txn_commit(txn)));
		THROW(SLTEST_CHECK_Z(mdb_env_stat(env, &mst)));
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_cursor_open(txn, dbi, &cursor)));
		while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
			printf("key: %p %.*s, data: %p %.*s\n", key.mv_data,  (int)key.mv_size,  (char *)key.mv_data, data.mv_data, (int)data.mv_size, (char *)data.mv_data);
		}
		THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
		mdb_cursor_close(cursor);
		mdb_txn_abort(txn);
		j = 0;
		key.mv_data = sval;
		for(i = count - 1; i > -1; i -= Lmdb_Random(5)) {
			j++;
			txn = NULL;
			THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, 0, &txn)));
			sprintf(sval, "%03x ", values[i]);
			//if(RES(MDB_NOTFOUND, mdb_del(txn, dbi, &key, NULL))) {
			rc = mdb_del(txn, dbi, &key, NULL);
			THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_NOTFOUND, 0)));
			if(rc == MDB_NOTFOUND) {
				j--;
				mdb_txn_abort(txn);
			}
			else {
				THROW(SLTEST_CHECK_Z(mdb_txn_commit(txn)));
			}
		}
		SAlloc::F(values);
		printf("Deleted %d values\n", j);
		THROW(SLTEST_CHECK_Z(mdb_env_stat(env, &mst)));
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_cursor_open(txn, dbi, &cursor)));
		printf("Cursor next\n");
		while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
			printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
		}
		THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
		printf("Cursor last\n");
		THROW(SLTEST_CHECK_Z(mdb_cursor_get(cursor, &key, &data, MDB_LAST)));
		printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
		printf("Cursor prev\n");
		while((rc = mdb_cursor_get(cursor, &key, &data, MDB_PREV)) == 0) {
			printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
		}
		THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
		printf("Cursor last/prev\n");
		THROW(SLTEST_CHECK_Z(mdb_cursor_get(cursor, &key, &data, MDB_LAST)));
		printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
		THROW(SLTEST_CHECK_Z(mdb_cursor_get(cursor, &key, &data, MDB_PREV)));
		printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
		mdb_cursor_close(cursor);
		mdb_txn_abort(txn);
		printf("Deleting with cursor\n");
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, 0, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_cursor_open(txn, dbi, &cur2)));
		for(i = 0; i<50; i++) {
			//if(RES(MDB_NOTFOUND, mdb_cursor_get(cur2, &key, &data, MDB_NEXT))) {
			rc = mdb_cursor_get(cur2, &key, &data, MDB_NEXT);
			THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_NOTFOUND, 0)));
			if(rc == MDB_NOTFOUND) {
				break;
			}
			printf("key: %p %.*s, data: %p %.*s\n", key.mv_data,  (int)key.mv_size,  (char *)key.mv_data, data.mv_data, (int)data.mv_size, (char *)data.mv_data);
			THROW(SLTEST_CHECK_Z(mdb_del(txn, dbi, &key, NULL)));
		}
		printf("Restarting cursor in txn\n");
		for(op = MDB_FIRST, i = 0; i<=32; op = MDB_NEXT, i++) {
			//if(RES(MDB_NOTFOUND, mdb_cursor_get(cur2, &key, &data, op))) {
			rc = mdb_cursor_get(cur2, &key, &data, op);
			THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_NOTFOUND, 0)));
			if(rc == MDB_NOTFOUND) {
				break;
			}
			printf("key: %p %.*s, data: %p %.*s\n", key.mv_data,  (int)key.mv_size,  (char *)key.mv_data, data.mv_data, (int)data.mv_size, (char *)data.mv_data);
		}
		mdb_cursor_close(cur2);
		THROW(SLTEST_CHECK_Z(mdb_txn_commit(txn)));
		printf("Restarting cursor outside txn\n");
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, 0, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_cursor_open(txn, dbi, &cursor)));
		for(op = MDB_FIRST, i = 0; i<=32; op = MDB_NEXT, i++) {
			//if(RES(MDB_NOTFOUND, mdb_cursor_get(cursor, &key, &data, op))) {
			rc = mdb_cursor_get(cursor, &key, &data, op);
			THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_NOTFOUND, 0)));
			if(rc == MDB_NOTFOUND) {
				break;
			}
			printf("key: %p %.*s, data: %p %.*s\n", key.mv_data,  (int)key.mv_size,  (char *)key.mv_data, data.mv_data, (int)data.mv_size, (char *)data.mv_data);
		}
		mdb_cursor_close(cursor);
		mdb_txn_abort(txn);
		mdb_dbi_close(env, dbi);
		mdb_env_close(env);
	}
	{
		int i = 0, j = 0, rc;
		MDB_env * env;
		MDB_dbi dbi;
		MDB_txn * txn;
		MDB_stat mst;
		char sval[32] = "";
		Lmdb_CreateTestDirectory(db_path);
		int count;
		int * values = Lmdb_AllocateTestVector(count);
		THROW(SLTEST_CHECK_Z(mdb_env_create(&env)));
		THROW(SLTEST_CHECK_Z(mdb_env_set_maxreaders(env, 1)));
		THROW(SLTEST_CHECK_Z(mdb_env_set_mapsize(env, 10485760)));
		THROW(SLTEST_CHECK_Z(mdb_env_set_maxdbs(env, 4)));
		THROW(SLTEST_CHECK_Z(mdb_env_open(env, /*"./testdb"*/db_path, MDB_FIXEDMAP|MDB_NOSYNC, 0664)));
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, 0, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_dbi_open(txn, "id1", MDB_CREATE, &dbi)));
		key.mv_size = sizeof(int);
		key.mv_data = sval;
		printf("Adding %d values\n", count);
		for(i = 0; i < count; i++) {
			sprintf(sval, "%03x %d foo bar", values[i], values[i]);
			data.mv_size = sizeof(sval);
			data.mv_data = sval;
			//if(RES(MDB_KEYEXIST, mdb_put(txn, dbi, &key, &data, MDB_NOOVERWRITE))) {
			rc = mdb_put(txn, dbi, &key, &data, MDB_NOOVERWRITE);
			THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_KEYEXIST, 0)));
			if(rc == MDB_KEYEXIST) {
				j++;
			}
		}
		if(j) 
			printf("%d duplicates skipped\n", j);
		THROW(SLTEST_CHECK_Z(mdb_txn_commit(txn)));
		THROW(SLTEST_CHECK_Z(mdb_env_stat(env, &mst)));
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_cursor_open(txn, dbi, &cursor)));
		while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
			printf("key: %p %.*s, data: %p %.*s\n", key.mv_data,  (int)key.mv_size,  (char *)key.mv_data, data.mv_data, (int)data.mv_size, (char *)data.mv_data);
		}
		THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
		mdb_cursor_close(cursor);
		mdb_txn_abort(txn);
		j = 0;
		key.mv_data = sval;
		for(i = count - 1; i > -1; i -= Lmdb_Random(5)) {
			j++;
			txn = NULL;
			THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, 0, &txn)));
			sprintf(sval, "%03x ", values[i]);
			//if(RES(MDB_NOTFOUND, mdb_del(txn, dbi, &key, NULL))) {
			rc = mdb_del(txn, dbi, &key, NULL);
			THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_NOTFOUND, 0)));
			if(rc == MDB_NOTFOUND) {
				j--;
				mdb_txn_abort(txn);
			}
			else {
				THROW(SLTEST_CHECK_Z(mdb_txn_commit(txn)));
			}
		}
		SAlloc::F(values);
		printf("Deleted %d values\n", j);
		THROW(SLTEST_CHECK_Z(mdb_env_stat(env, &mst)));
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_cursor_open(txn, dbi, &cursor)));
		printf("Cursor next\n");
		while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
			printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
		}
		THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
		printf("Cursor prev\n");
		while((rc = mdb_cursor_get(cursor, &key, &data, MDB_PREV)) == 0) {
			printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
		}
		THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
		mdb_cursor_close(cursor);
		mdb_txn_abort(txn);
		mdb_dbi_close(env, dbi);
		mdb_env_close(env);
	}
	{
		int i = 0, j = 0, rc;
		MDB_env * env;
		MDB_dbi dbi;
		MDB_txn * txn;
		MDB_stat mst;
		char sval[32];
		char kval[sizeof(int)];
		Lmdb_CreateTestDirectory(db_path);
		//srand((uint)time(NULL));
		memzero(sval, sizeof(sval));
		int count;
		int * values = Lmdb_AllocateTestVector(count);
		THROW(SLTEST_CHECK_Z(mdb_env_create(&env)));
		THROW(SLTEST_CHECK_Z(mdb_env_set_mapsize(env, 10485760)));
		THROW(SLTEST_CHECK_Z(mdb_env_set_maxdbs(env, 4)))
		THROW(SLTEST_CHECK_Z(mdb_env_open(env, /*"./testdb"*/db_path, MDB_FIXEDMAP|MDB_NOSYNC, 0664)));
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, 0, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_dbi_open(txn, "id2", MDB_CREATE|MDB_DUPSORT, &dbi)));
		key.mv_size = sizeof(int);
		key.mv_data = kval;
		data.mv_size = sizeof(sval);
		data.mv_data = sval;
		printf("Adding %d values\n", count);
		for(i = 0; i<count; i++) {
			if(!(i & 0x0f))
				sprintf(kval, "%03x", values[i]);
			sprintf(sval, "%03x %d foo bar", values[i], values[i]);
			//if(RES(MDB_KEYEXIST, mdb_put(txn, dbi, &key, &data, MDB_NODUPDATA))) {
			rc = mdb_put(txn, dbi, &key, &data, MDB_NODUPDATA);
			THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_KEYEXIST, 0)));
			if(rc == MDB_KEYEXIST) {
				j++;
			}
		}
		if(j) 
			printf("%d duplicates skipped\n", j);
		THROW(SLTEST_CHECK_Z(mdb_txn_commit(txn)));
		THROW(SLTEST_CHECK_Z(mdb_env_stat(env, &mst)));
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_cursor_open(txn, dbi, &cursor)));
		while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
			printf("key: %p %.*s, data: %p %.*s\n", key.mv_data,  (int)key.mv_size,  (char *)key.mv_data, data.mv_data, (int)data.mv_size, (char *)data.mv_data);
		}
		THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
		mdb_cursor_close(cursor);
		mdb_txn_abort(txn);
		j = 0;
		for(i = count - 1; i > -1; i -= Lmdb_Random(5)) {
			j++;
			txn = NULL;
			THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, 0, &txn)));
			sprintf(kval, "%03x", values[i & ~0x0f]);
			sprintf(sval, "%03x %d foo bar", values[i], values[i]);
			key.mv_size = sizeof(int);
			key.mv_data = kval;
			data.mv_size = sizeof(sval);
			data.mv_data = sval;
			//if(RES(MDB_NOTFOUND, mdb_del(txn, dbi, &key, &data))) {
			rc = mdb_del(txn, dbi, &key, &data);
			THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_NOTFOUND, 0)));
			if(rc == MDB_NOTFOUND) {
				j--;
				mdb_txn_abort(txn);
			}
			else {
				THROW(SLTEST_CHECK_Z(mdb_txn_commit(txn)));
			}
		}
		SAlloc::F(values);
		printf("Deleted %d values\n", j);
		THROW(SLTEST_CHECK_Z(mdb_env_stat(env, &mst)));
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_cursor_open(txn, dbi, &cursor)));
		printf("Cursor next\n");
		while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
			printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
		}
		THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
		printf("Cursor prev\n");
		while((rc = mdb_cursor_get(cursor, &key, &data, MDB_PREV)) == 0) {
			printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
		}
		THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
		mdb_cursor_close(cursor);
		mdb_txn_abort(txn);
		mdb_dbi_close(env, dbi);
		mdb_env_close(env);
	}
	{
		//
		// Tests for sorted duplicate DBs with fixed-size keys 
		//
		int i = 0, j = 0, rc;
		MDB_env * env;
		MDB_dbi dbi;
		MDB_txn * txn;
		MDB_stat mst;
		char sval[8];
		char kval[sizeof(int)];
		Lmdb_CreateTestDirectory(db_path);
		memzero(sval, sizeof(sval));
		const int count = 510;
		int * values = (int *)SAlloc::M(count*sizeof(int));
		for(i = 0; i < count; i++) {
			values[i] = i*5;
		}
		THROW(SLTEST_CHECK_Z(mdb_env_create(&env)));
		THROW(SLTEST_CHECK_Z(mdb_env_set_mapsize(env, 10485760)));
		THROW(SLTEST_CHECK_Z(mdb_env_set_maxdbs(env, 4)));
		THROW(SLTEST_CHECK_Z(mdb_env_open(env, /*"./testdb"*/db_path, MDB_FIXEDMAP|MDB_NOSYNC, 0664)));
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, 0, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_dbi_open(txn, "id4", MDB_CREATE|MDB_DUPSORT|MDB_DUPFIXED, &dbi)));
		key.mv_size = sizeof(int);
		key.mv_data = kval;
		data.mv_size = sizeof(sval);
		data.mv_data = sval;
		printf("Adding %d values\n", count);
		strcpy(kval, "001");
		for(i = 0; i<count; i++) {
			sprintf(sval, "%07x", values[i]);
			//if(RES(MDB_KEYEXIST, mdb_put(txn, dbi, &key, &data, MDB_NODUPDATA))) {
			rc = mdb_put(txn, dbi, &key, &data, MDB_NODUPDATA);
			THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_KEYEXIST, 0)));
			if(rc == MDB_KEYEXIST) {
				j++;
			}
		}
		if(j) 
			printf("%d duplicates skipped\n", j);
		THROW(SLTEST_CHECK_Z(mdb_txn_commit(txn)));
		THROW(SLTEST_CHECK_Z(mdb_env_stat(env, &mst)));
		// there should be one full page of dups now.
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_cursor_open(txn, dbi, &cursor)));
		while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
			printf("key: %p %.*s, data: %p %.*s\n", key.mv_data,  (int)key.mv_size,  (char *)key.mv_data, data.mv_data, (int)data.mv_size, (char *)data.mv_data);
		}
		THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
		mdb_cursor_close(cursor);
		mdb_txn_abort(txn);
		/* test all 3 branches of split code:
		 * 1: new key in lower half
		 * 2: new key at split point
		 * 3: new key in upper half
		 */
		key.mv_size = sizeof(int);
		key.mv_data = kval;
		data.mv_size = sizeof(sval);
		data.mv_data = sval;
		sprintf(sval, "%07x", values[3]+1);
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, 0, &txn)));
		{
			//RES(MDB_KEYEXIST, mdb_put(txn, dbi, &key, &data, MDB_NODUPDATA));
			rc = mdb_put(txn, dbi, &key, &data, MDB_NODUPDATA);
			THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_KEYEXIST, 0)));
		}
		mdb_txn_abort(txn);
		sprintf(sval, "%07x", values[255]+1);
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, 0, &txn)));
		{
			//RES(MDB_KEYEXIST, mdb_put(txn, dbi, &key, &data, MDB_NODUPDATA));
			rc = mdb_put(txn, dbi, &key, &data, MDB_NODUPDATA);
			THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_KEYEXIST, 0)));
		}
		mdb_txn_abort(txn);
		sprintf(sval, "%07x", values[500]+1);
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, 0, &txn)));
		{
			//RES(MDB_KEYEXIST, mdb_put(txn, dbi, &key, &data, MDB_NODUPDATA));
			rc = mdb_put(txn, dbi, &key, &data, MDB_NODUPDATA);
			THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_KEYEXIST, 0)));
		}
		THROW(SLTEST_CHECK_Z(mdb_txn_commit(txn)));
		// Try MDB_NEXT_MULTIPLE 
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, 0, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_cursor_open(txn, dbi, &cursor)));
		while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT_MULTIPLE)) == 0) {
			printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
		}
		THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
		mdb_cursor_close(cursor);
		mdb_txn_abort(txn);
		j = 0;
		for(i = count - 1; i > -1; i -= Lmdb_Random(3)) {
			j++;
			txn = NULL;
			THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, 0, &txn)));
			sprintf(sval, "%07x", values[i]);
			key.mv_size = sizeof(int);
			key.mv_data = kval;
			data.mv_size = sizeof(sval);
			data.mv_data = sval;
			//if(RES(MDB_NOTFOUND, mdb_del(txn, dbi, &key, &data))) {
			rc = mdb_del(txn, dbi, &key, &data);
			THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_NOTFOUND, 0)));
			if(rc == MDB_NOTFOUND) {
				j--;
				mdb_txn_abort(txn);
			}
			else {
				THROW(SLTEST_CHECK_Z(mdb_txn_commit(txn)));
			}
		}
		SAlloc::F(values);
		printf("Deleted %d values\n", j);
		THROW(SLTEST_CHECK_Z(mdb_env_stat(env, &mst)));
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_cursor_open(txn, dbi, &cursor)));
		printf("Cursor next\n");
		while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
			printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
		}
		THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
		printf("Cursor prev\n");
		while((rc = mdb_cursor_get(cursor, &key, &data, MDB_PREV)) == 0) {
			printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
		}
		THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
		mdb_cursor_close(cursor);
		mdb_txn_abort(txn);
		mdb_dbi_close(env, dbi);
		mdb_env_close(env);
	}
	{
		//
		// Tests for sorted duplicate DBs using cursor_put 
		//
		int i = 0, j = 0, rc;
		MDB_env * env;
		MDB_dbi dbi;
		MDB_txn * txn;
		MDB_stat mst;
		char sval[32];
		char kval[sizeof(int)];
		Lmdb_CreateTestDirectory(db_path);
		memzero(sval, sizeof(sval));
		int   count;
		int * values = Lmdb_AllocateTestVector(count);
		THROW(SLTEST_CHECK_Z(mdb_env_create(&env)));
		THROW(SLTEST_CHECK_Z(mdb_env_set_mapsize(env, 10485760)));
		THROW(SLTEST_CHECK_Z(mdb_env_set_maxdbs(env, 4)));
		THROW(SLTEST_CHECK_Z(mdb_env_open(env, /*"./testdb"*/db_path, MDB_FIXEDMAP|MDB_NOSYNC, 0664)));
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, 0, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_dbi_open(txn, "id2", MDB_CREATE|MDB_DUPSORT, &dbi)));
		THROW(SLTEST_CHECK_Z(mdb_cursor_open(txn, dbi, &cursor)));
		key.mv_size = sizeof(int);
		key.mv_data = kval;
		data.mv_size = sizeof(sval);
		data.mv_data = sval;
		printf("Adding %d values\n", count);
		for(i = 0; i < count; i++) {
			if(!(i & 0x0f))
				sprintf(kval, "%03x", values[i]);
			sprintf(sval, "%03x %d foo bar", values[i], values[i]);
			//if(RES(MDB_KEYEXIST, mdb_cursor_put(cursor, &key, &data, MDB_NODUPDATA))) {
			rc = mdb_cursor_put(cursor, &key, &data, MDB_NODUPDATA);
			THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_KEYEXIST, 0)));
			if(rc == MDB_KEYEXIST) {
				j++;
			}
		}
		if(j) 
			printf("%d duplicates skipped\n", j);
		mdb_cursor_close(cursor);
		THROW(SLTEST_CHECK_Z(mdb_txn_commit(txn)));
		THROW(SLTEST_CHECK_Z(mdb_env_stat(env, &mst)));
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_cursor_open(txn, dbi, &cursor)));
		while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
			printf("key: %p %.*s, data: %p %.*s\n", key.mv_data,  (int)key.mv_size,  (char *)key.mv_data, data.mv_data, (int)data.mv_size, (char *)data.mv_data);
		}
		THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
		mdb_cursor_close(cursor);
		mdb_txn_abort(txn);
		j = 0;
		for(i = count - 1; i > -1; i -= Lmdb_Random(5)) {
			j++;
			txn = NULL;
			THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, 0, &txn)));
			sprintf(kval, "%03x", values[i & ~0x0f]);
			sprintf(sval, "%03x %d foo bar", values[i], values[i]);
			key.mv_size = sizeof(int);
			key.mv_data = kval;
			data.mv_size = sizeof(sval);
			data.mv_data = sval;
			//if(RES(MDB_NOTFOUND, mdb_del(txn, dbi, &key, &data))) {
			rc = mdb_del(txn, dbi, &key, &data);
			THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_NOTFOUND, 0)));
			if(rc == MDB_NOTFOUND) {
				j--;
				mdb_txn_abort(txn);
			}
			else {
				THROW(SLTEST_CHECK_Z(mdb_txn_commit(txn)));
			}
		}
		SAlloc::F(values);
		printf("Deleted %d values\n", j);
		THROW(SLTEST_CHECK_Z(mdb_env_stat(env, &mst)));
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_cursor_open(txn, dbi, &cursor)));
		printf("Cursor next\n");
		while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
			printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
		}
		THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
		printf("Cursor prev\n");
		while((rc = mdb_cursor_get(cursor, &key, &data, MDB_PREV)) == 0) {
			printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
		}
		THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
		mdb_cursor_close(cursor);
		mdb_txn_abort(txn);
		mdb_dbi_close(env, dbi);
		mdb_env_close(env);
	}
	//#if MDB_DEBUG // {
	{
		//
		// Tests for DB splits and merges
		//
		static char dkbuf[1024];
		int i = 0, j = 0, rc;
		MDB_env * env;
		MDB_dbi dbi;
		MDB_val sdata;
		MDB_txn * txn;
		MDB_stat mst;
		int   count = 0;
		int * values = 0;
		//long kval;
		char   kval[sizeof(int)];
		char * sval;
		Lmdb_CreateTestDirectory(db_path);
		srand((uint)time(NULL));
		THROW(SLTEST_CHECK_Z(mdb_env_create(&env)));
		THROW(SLTEST_CHECK_Z(mdb_env_set_mapsize(env, 10485760)));
		THROW(SLTEST_CHECK_Z(mdb_env_set_maxdbs(env, 4)));
		THROW(SLTEST_CHECK_Z(mdb_env_open(env, /*"./testdb"*/db_path, MDB_FIXEDMAP|MDB_NOSYNC, 0664)));
		THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, 0, &txn)));
		THROW(SLTEST_CHECK_Z(mdb_dbi_open(txn, "id6", MDB_CREATE|MDB_INTEGERKEY, &dbi)));
		THROW(SLTEST_CHECK_Z(mdb_cursor_open(txn, dbi, &cursor)));
		THROW(SLTEST_CHECK_Z(mdb_stat(txn, dbi, &mst)));
		sval = (char *)SAlloc::C(1, mst.ms_psize / 4);
		key.mv_size = sizeof(long);
		key.mv_data = &kval;
		sdata.mv_size = mst.ms_psize / 4 - 30;
		sdata.mv_data = sval;
		printf("Adding 12 values, should yield 3 splits\n");
		for(i = 0; i < 12; i++) {
			*reinterpret_cast<int *>(kval) = i*5;
			sprintf(sval, "%08x", *reinterpret_cast<int *>(kval));
			data = sdata;
			{
				//RES(MDB_KEYEXIST, mdb_cursor_put(cursor, &key, &data, MDB_NOOVERWRITE));
				rc = mdb_cursor_put(cursor, &key, &data, MDB_NOOVERWRITE);
				THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_KEYEXIST, 0)));
			}
		}
		printf("Adding 12 more values, should yield 3 splits\n");
		for(i = 0; i < 12; i++) {
			*reinterpret_cast<int *>(kval) = i*5+4;
			sprintf(sval, "%08x", *reinterpret_cast<int *>(kval));
			data = sdata;
			{
				//RES(MDB_KEYEXIST, mdb_cursor_put(cursor, &key, &data, MDB_NOOVERWRITE));
				rc = mdb_cursor_put(cursor, &key, &data, MDB_NOOVERWRITE);
				THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_KEYEXIST, 0)));
			}
		}
		printf("Adding 12 more values, should yield 3 splits\n");
		for(i = 0; i < 12; i++) {
			*reinterpret_cast<int *>(kval) = i*5+1;
			sprintf(sval, "%08x", *reinterpret_cast<int *>(kval));
			data = sdata;
			{
				//RES(MDB_KEYEXIST, mdb_cursor_put(cursor, &key, &data, MDB_NOOVERWRITE));
				rc = mdb_cursor_put(cursor, &key, &data, MDB_NOOVERWRITE);
				THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_KEYEXIST, 0)));
			}
		}
		THROW(SLTEST_CHECK_Z(mdb_cursor_get(cursor, &key, &data, MDB_FIRST)));
		do {
			printf("key: %p %s, data: %p %.*s\n", key.mv_data,  mdb_dkey(&key, dkbuf), data.mv_data, (int)data.mv_size, (char *)data.mv_data);
		} while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0);
		THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
		mdb_cursor_close(cursor);
		mdb_txn_commit(txn);
		#if 0
		{
			j = 0;
			for(i = count - 1; i > -1; i -= Lmdb_Random(5)) {
				j++;
				txn = NULL;
				THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, 0, &txn)));
				sprintf(kval, "%03x", values[i & ~0x0f]);
				sprintf(sval, "%03x %d foo bar", values[i], values[i]);
				key.mv_size = sizeof(int);
				key.mv_data = kval;
				data.mv_size = sizeof(sval);
				data.mv_data = sval;
				//if(RES(MDB_NOTFOUND, mdb_del(txn, dbi, &key, &data))) {
				rc = mdb_del(txn, dbi, &key, &data);
				THROW(SLTEST_CHECK_NZ(oneof2(rc, MDB_NOTFOUND, 0)));
				if(rc == MDB_NOTFOUND) {
					j--;
					mdb_txn_abort(txn);
				}
				else {
					THROW(SLTEST_CHECK_Z(mdb_txn_commit(txn)));
				}
			}
			SAlloc::F(values);
			printf("Deleted %d values\n", j);
			THROW(SLTEST_CHECK_Z(mdb_env_stat(env, &mst)));
			THROW(SLTEST_CHECK_Z(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn)));
			THROW(SLTEST_CHECK_Z(mdb_cursor_open(txn, dbi, &cursor)));
			printf("Cursor next\n");
			while((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
				printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
			}
			THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
			printf("Cursor prev\n");
			while((rc = mdb_cursor_get(cursor, &key, &data, MDB_PREV)) == 0) {
				printf("key: %.*s, data: %.*s\n", (int)key.mv_size,  (char *)key.mv_data, (int)data.mv_size, (char *)data.mv_data);
			}
			THROW(SLTEST_CHECK_EQ((long)rc, MDB_NOTFOUND));
			mdb_cursor_close(cursor);
			mdb_txn_abort(txn);
			mdb_dbi_close(env, dbi);
		}
		#endif
		mdb_env_close(env);
	}
	//#endif // } MDB_DEBUG
	CATCHZOK
	return ok;
}

#endif // } SLTEST_RUNNING 