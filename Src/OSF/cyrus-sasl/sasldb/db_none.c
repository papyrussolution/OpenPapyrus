/* db_none.c--provides linkage for systems which lack a backend db lib
 * Rob Siemborski
 * Rob Earhart
 */
// Copyright (c) 1998-2016 Carnegie Mellon University.  All rights reserved.
//
#include <sasl-internal.h>
#pragma hdrstop
#include "sasldb.h"

/* This just exists to provide these symbols on systems where configure
 * couldn't find a database library (or the user says we do not want one). */
int _sasldb_getdata(const sasl_utils_t * utils,
    sasl_conn_t * conn,
    const char * authid __attribute__((unused)),
    const char * realm __attribute__((unused)),
    const char * propName __attribute__((unused)),
    char * out __attribute__((unused)),
    const size_t max_out __attribute__((unused)),
    size_t * out_len __attribute__((unused)))
{
	if(conn) utils->seterror(conn, 0, "No Database Driver");
	return SASL_FAIL;
}

int _sasldb_putdata(const sasl_utils_t * utils,
    sasl_conn_t * conn,
    const char * authid __attribute__((unused)),
    const char * realm __attribute__((unused)),
    const char * propName __attribute__((unused)),
    const char * data __attribute__((unused)),
    size_t data_len __attribute__((unused)))
{
	if(conn) utils->seterror(conn, 0, "No Database Driver");
	return SASL_FAIL;
}

int _sasl_check_db(const sasl_utils_t * utils, sasl_conn_t * conn)
{
	if(conn) utils->seterror(conn, 0, "No Database Driver");
	return SASL_FAIL;
}

sasldb_handle _sasldb_getkeyhandle(const sasl_utils_t * utils, sasl_conn_t * conn)
{
	if(conn) utils->seterror(conn, 0, "No Database Driver");
	return NULL;
}

int _sasldb_getnextkey(const sasl_utils_t * utils __attribute__((unused)),
    sasldb_handle handle __attribute__((unused)),
    char * out __attribute__((unused)),
    const size_t max_out __attribute__((unused)),
    size_t * out_len __attribute__((unused)))
{
	return SASL_FAIL;
}

int _sasldb_releasekeyhandle(const sasl_utils_t * utils __attribute__((unused)), sasldb_handle handle __attribute__((unused)))
{
	return SASL_FAIL;
}
