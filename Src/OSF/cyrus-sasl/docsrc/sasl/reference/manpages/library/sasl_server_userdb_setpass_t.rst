.. saslman:: sasl_server_userdb_setpass_t(3)

.. _sasl-reference-manpages-library-sasl_server_userdb_setpass_t:


=============================================================================
**sasl_server_userdb_setpass_t** - UserDB Plaintext Password Setting Callback
=============================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_server_userdb_setpass_t(sasl_conn_t *conn,
                                     void *context,
                                     const char *user,
                                     const char *pass,
                                     unsigned passlen,
                                     struct propctx *propctx,
                                     unsigned flags)

Description
===========

.. c:function:: int sasl_server_userdb_setpass_t(sasl_conn_t *conn,
        void *context,
        const char *user,
        const char *pass,
        unsigned passlen,
        struct propctx *propctx,
        unsigned flags)

    **sasl_server_userdb_setpass_t** is used to store or change  a plaintext
    password  in the callback‐supplier’s user database.

    :param conn: is the SASL connection

    :param context: context from the callback record

    :param user: NUL terminated user name with `user@realm` syntax

    :param pass: password to check (may not be NUL terminated)

    :param passlen: length of the password

    :param propctx: Auxilliary Properties (not stored)

    :param flags: These  are  the  same  flags  that  are  passed  to
        :saslman:`sasl_setpass(3)`, and are documented on that man page.

Return Value
============

SASL  callback  functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

Other return codes indicate errors and should be handled.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_errors(3)`
:saslman:`sasl_callbacks(3)`, :saslman:`sasl_server_userdb_checkpass_t(3)`,
:saslman:`sasl_setpass(3)`
