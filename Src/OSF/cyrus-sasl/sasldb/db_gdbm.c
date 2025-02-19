/* db_gdbm.c--SASL gdbm interface
 * Rob Siemborski
 * Rob Earhart
 */
// Copyright (c) 1998-2016 Carnegie Mellon University.  All rights reserved.
//
#include <sasl-internal.h>
#pragma hdrstop
#include <gdbm.h>
#include <sys/stat.h>
#include "sasldb.h"

static int db_ok = 0;

int _sasldb_getdata(const sasl_utils_t * utils, sasl_conn_t * conn, const char * authid, const char * realm,
    const char * propName, char * out, const size_t max_out, size_t * out_len)
{
	int result = SASL_OK;
	char * key;
	size_t key_len;
	GDBM_FILE db;
	datum gkey, gvalue;
	void * cntxt;
	sasl_getopt_t * getopt;
	const char * path = SASL_DB_PATH;

	if(!utils) return SASL_BADPARAM;
	if(!authid || !propName || !realm || !out || !max_out) {
		utils->seterror(conn, 0,
		    "Bad parameter in db_gdbm.c: _sasldb_getdata");
		return SASL_BADPARAM;
	}

	if(!db_ok) {
		utils->seterror(conn, 0,
		    "Database not checked");
		return SASL_FAIL;
	}

	result = _sasldb_alloc_key(utils, authid, realm, propName,
		&key, &key_len);
	if(result != SASL_OK) {
		utils->seterror(conn, 0,
		    "Could not allocate key in _sasldb_getdata");
		return result;
	}
	if(utils->getcallback(conn, SASL_CB_GETOPT, (sasl_callback_ft*)&getopt, &cntxt) == SASL_OK) {
		const char * p;
		if(getopt(cntxt, 0, "sasldb_path", &p, 0) == SASL_OK && p && *p != 0) {
			path = p;
		}
	}
	db = gdbm_open((char *)path, 0, GDBM_READER, S_IRUSR | S_IWUSR, NULL);
	if(!db) {
		utils->seterror(cntxt, 0, "Could not open %s: gdbm_errno=%d", path, gdbm_errno);
		result = SASL_FAIL;
		goto cleanup;
	}
	gkey.dptr = key;
	gkey.dsize = key_len;
	gvalue = gdbm_fetch(db, gkey);
	int fetch_errno = gdbm_errno;

	gdbm_close(db);
	if(!gvalue.dptr) {
		if(fetch_errno == GDBM_ITEM_NOT_FOUND) {
			utils->seterror(conn, SASL_NOLOG,
			    "user: %s@%s property: %s not found in %s",
			    authid, realm, propName, path);
			result = SASL_NOUSER;
		}
		else {
			utils->seterror(conn, 0,
			    "Couldn't fetch entry from %s: gdbm_errno=%d",
			    path, gdbm_errno);
			result = SASL_FAIL;
		}
		goto cleanup;
	}

	if((size_t)gvalue.dsize > max_out + 1) {
		utils->seterror(cntxt, 0, "buffer overflow");
		return SASL_BUFOVER;
	}

	if(out_len) *out_len = gvalue.dsize;
	memcpy(out, gvalue.dptr, gvalue.dsize);
	out[gvalue.dsize] = '\0';

	/* Note: not sasl_FREE!  This is memory allocated by gdbm,
	 * which is using libc malloc/free. */
	free(gvalue.dptr);

cleanup:
	utils->FnFree(key);

	return result;
}

int _sasldb_putdata(const sasl_utils_t * utils,
    sasl_conn_t * conn,
    const char * authid,
    const char * realm,
    const char * propName,
    const char * data, size_t data_len)
{
	int result = SASL_OK;
	char * key;
	size_t key_len;
	GDBM_FILE db;
	datum gkey;
	void * cntxt;
	sasl_getopt_t * getopt;
	const char * path = SASL_DB_PATH;

	if(!utils) return SASL_BADPARAM;

	if(!authid || !realm || !propName) {
		utils->seterror(conn, 0,
		    "Bad parameter in db_gdbm.c: _sasldb_putdata");
		return SASL_BADPARAM;
	}

	result = _sasldb_alloc_key(utils, authid, realm, propName,
		&key, &key_len);
	if(result != SASL_OK) {
		utils->seterror(conn, 0,
		    "Could not allocate key in _sasldb_putdata");
		return result;
	}
	if(utils->getcallback(conn, SASL_CB_GETOPT, (sasl_callback_ft*)&getopt, &cntxt) == SASL_OK) {
		const char * p;
		if(getopt(cntxt, 0, "sasldb_path", &p, 0) == SASL_OK && p && *p != 0) {
			path = p;
		}
	}
	db = gdbm_open((char *)path, 0, GDBM_WRCREAT, S_IRUSR | S_IWUSR, NULL);
	if(!db) {
		utils->log(conn, SASL_LOG_ERR, "SASL error opening password file. Do you have write permissions?\n");
		utils->seterror(conn, 0, "Could not open %s for write: gdbm_errno=%d", path, gdbm_errno);
		result = SASL_FAIL;
		goto cleanup;
	}
	gkey.dptr = key;
	gkey.dsize = key_len;
	if(data) {
		datum gvalue;
		gvalue.dptr = (char *)data;
		if(!data_len) data_len = strlen(data);
		gvalue.dsize = data_len;
		if(gdbm_store(db, gkey, gvalue, GDBM_REPLACE)) {
			utils->seterror(conn, 0, "Couldn't replace entry in %s: gdbm_errno=%d", path, gdbm_errno);
			result = SASL_FAIL;
		}
	}
	else {
		if(gdbm_delete(db, gkey)) {
			utils->seterror(conn, 0, "Couldn't delete entry in %s: gdbm_errno=%d", path, gdbm_errno);
			result = SASL_NOUSER;
		}
	}
	gdbm_close(db);

cleanup:
	utils->FnFree(key);

	return result;
}

int _sasl_check_db(const sasl_utils_t * utils,
    sasl_conn_t * conn)
{
	const char * path = SASL_DB_PATH;
	int ret;
	void * cntxt;
	sasl_getopt_t * getopt;
	sasl_verifyfile_t * vf;

	if(!utils) return SASL_BADPARAM;

	if(utils->getcallback(conn, SASL_CB_GETOPT,
	    (sasl_callback_ft*)&getopt, &cntxt) == SASL_OK) {
		const char * p;
		if(getopt(cntxt, 0, "sasldb_path", &p, 0) == SASL_OK && p && *p != 0) {
			path = p;
		}
	}
	ret = utils->getcallback(NULL, SASL_CB_VERIFYFILE, (sasl_callback_ft*)&vf, &cntxt);
	if(ret != SASL_OK) {
		utils->seterror(conn, 0, "No verifyfile callback");
		return ret;
	}

	ret = vf(cntxt, path, SASL_VRFY_PASSWD);
	if(ret == SASL_OK) {
		db_ok = 1;
	}

	if(ret == SASL_OK || ret == SASL_CONTINUE) {
		return SASL_OK;
	}
	else {
		utils->seterror(conn, 0,
		    "Verifyfile failed");
		return ret;
	}
}

typedef struct gdbm_handle {
	GDBM_FILE db;
	datum dkey;
	int first;
} handle_t;

sasldb_handle _sasldb_getkeyhandle(const sasl_utils_t * utils,
    sasl_conn_t * conn)
{
	const char * path = SASL_DB_PATH;
	sasl_getopt_t * getopt;
	void * cntxt;
	GDBM_FILE db;
	handle_t * handle;

	if(!utils || !conn) return NULL;

	if(!db_ok) {
		utils->seterror(conn, 0, "Database not OK in _sasldb_getkeyhandle");
		return NULL;
	}
	if(utils->getcallback(conn, SASL_CB_GETOPT, (sasl_callback_ft*)&getopt, &cntxt) == SASL_OK) {
		const char * p;
		if(getopt(cntxt, 0, "sasldb_path", &p, 0) == SASL_OK && p && *p != 0) {
			path = p;
		}
	}
	db = gdbm_open((char *)path, 0, GDBM_READER, S_IRUSR | S_IWUSR, NULL);
	if(!db) {
		utils->seterror(conn, 0, "Could not open %s: gdbm_errno=%d", path, gdbm_errno);
		return NULL;
	}

	handle = utils->FnMalloc(sizeof(handle_t));
	if(!handle) {
		utils->seterror(conn, 0, "no memory in _sasldb_getkeyhandle");
		gdbm_close(db);
		return NULL;
	}

	handle->db = db;
	handle->first = 1;

	return (sasldb_handle)handle;
}

int _sasldb_getnextkey(const sasl_utils_t * utils __attribute__((unused)),
    sasldb_handle handle, char * out,
    const size_t max_out, size_t * out_len)
{
	handle_t * dbh = (handle_t*)handle;
	datum nextkey;

	if(!utils || !handle || !out || !max_out)
		return SASL_BADPARAM;

	if(dbh->first) {
		dbh->dkey = gdbm_firstkey(dbh->db);
		dbh->first = 0;
	}
	else {
		nextkey = gdbm_nextkey(dbh->db, dbh->dkey);
		dbh->dkey = nextkey;
	}

	if(dbh->dkey.dptr == NULL)
		return SASL_OK;

	if((uint)dbh->dkey.dsize > max_out)
		return SASL_BUFOVER;

	memcpy(out, dbh->dkey.dptr, dbh->dkey.dsize);
	if(out_len) *out_len = dbh->dkey.dsize;

	return SASL_CONTINUE;
}

int _sasldb_releasekeyhandle(const sasl_utils_t * utils,
    sasldb_handle handle)
{
	handle_t * dbh = (handle_t*)handle;

	if(!utils || !dbh) return SASL_BADPARAM;

	if(dbh->db) gdbm_close(dbh->db);

	utils->FnFree(dbh);

	return SASL_OK;
}
