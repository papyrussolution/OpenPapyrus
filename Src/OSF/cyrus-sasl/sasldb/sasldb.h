/* sasldb.h - SASLdb library header
 * Rob Siemborski
 * Tim Martin
 */
// Copyright (c) 1998-2016 Carnegie Mellon University.  All rights reserved.
//
#ifndef SASLDB_H
#define SASLDB_H

#include "sasl.h"
#include "saslplug.h"
/*
 * Note that some of these require a sasl_conn_t in order for
 * the getcallback stuff to work correctly.  This is great for
 * when they are called from a plugin or the library but makes
 * for much wierdness when an otherwise non-sasl application needs
 * to make use of this functionality.
 */
int _sasldb_getdata(const sasl_utils_t * utils, sasl_conn_t * conn, const char * authid, const char * realm,
    const char * propName, char * out, const size_t max_out, size_t * out_len);

/* pass NULL for data to delete it */
int _sasldb_putdata(const sasl_utils_t * utils, sasl_conn_t * conn, const char * authid, const char * realm,
    const char * propName, const char * data, size_t data_len);
/* Should be run before any db access is attempted */
LIBSASL_API int _sasl_check_db(const sasl_utils_t * utils, sasl_conn_t * conn);

/* These allow iterating through the keys of the database */
typedef void * sasldb_handle;

typedef int (* sasldb_list_callback_t) (const char * authid, const char * realm, const char * property, void * rock);

LIBSASL_API sasldb_handle _sasldb_getkeyhandle(const sasl_utils_t * utils, sasl_conn_t * conn);
LIBSASL_API int _sasldb_getnextkey(const sasl_utils_t * utils, sasldb_handle handle, char * out, const size_t max_out, size_t * out_len);
LIBSASL_API int _sasldb_releasekeyhandle(const sasl_utils_t * utils, sasldb_handle handle);
LIBSASL_API int _sasldb_listusers(const sasl_utils_t * utils, sasl_conn_t * context, sasldb_list_callback_t callback, void * callback_rock);

#if defined(KEEP_DB_OPEN)
void sasldb_auxprop_free(void * glob_context, const sasl_utils_t * utils);
#else
#define sasldb_auxprop_free     NULL
#endif

/* The rest are implemented in allockey.c and individual drivers need not
 * do so */
/* These two are aliases for getdata/putdata */
int _sasldb_getsecret(const sasl_utils_t * utils, sasl_conn_t * context, const char * auth_identity, const char * realm, sasl_secret_t ** secret);
int _sasldb_putsecret(const sasl_utils_t * utils, sasl_conn_t * context, const char * auth_identity, const char * realm, const sasl_secret_t * secret);

LIBSASL_API int _sasldb_parse_key(const char * key, const size_t key_len,
    char * authid, const size_t max_authid,
    char * realm, const size_t max_realm,
    char * propName, const size_t max_propname);
/* This function is internal, but might be useful to have around */
int _sasldb_alloc_key(const sasl_utils_t * utils,
    const char * auth_identity,
    const char * realm,
    const char * propName,
    char ** key,
    size_t * key_len);

#endif /* SASLDB_H */
