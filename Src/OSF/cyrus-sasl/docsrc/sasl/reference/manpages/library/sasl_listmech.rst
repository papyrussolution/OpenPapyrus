.. saslman:: sasl_listmech(3)

.. _sasl-reference-manpages-library-sasl_listmech:


====================================================================
**sasl_listmech** - Retrieve a list of the supported SASL mechanisms
====================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_listmech(sasl_conn_t *conn,
                     const char *user,
                     const char *prefix,
                     const char *sep,
                     const char *suffix,
                     const char **result,
                     unsigned *plen,
                     int *pcount);

Description
===========

.. c:function::  int sasl_listmech(sasl_conn_t *conn,
        const char *user,
        const char *prefix,
        const char *sep,
        const char *suffix,
        const char **result,
        unsigned *plen,
        int *pcount);

    **sasl_listmech** returns a string listing the SASL names of
    all the mechanisms available to the specified user. This
    is typically given to the client through a capability command
    or initial server response. Client applications need
    this list so that they know what mechanisms the server
    supports.

    :param conn: the SASL context for this connection
    :param user: (optional) restricts the mechanism list to only those
        available to the user.
    :param prefix: appended to beginning of result
    :param sep: appended between mechanisms
    :param suffix: appended to end of result
    :param result: NULL terminated result string (allocated/freed by
        library)
    :param plen: length of result filled in by library. May be NULL
    :param pcount: Number of mechanisms available (filled in by library).
        May be NULL

Example
=======

.. code-block:: C

    sasl_listmech(conn,NULL,"(",",",")",&mechlist,NULL,NULL);

may give the following string as a result:

    `(ANONYMOUS,KERBEROS_V4,DIGEST‐MD5)`

Return Value
============

SASL functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_server_new(3)`,
:saslman:`sasl_errors(3)`, :saslman:`sasl_client_new(3)`
