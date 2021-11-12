.. saslman:: sasl_server_userdb_checkpass_t(3)

.. _sasl-reference-manpages-library-sasl_server_userdb_checkpass_t:


=============================================================================
**sasl_server_userdb_checkpass_t** - Plaintext Password Verification Callback
=============================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_server_userdb_checkpass_t(sasl_conn_t *conn,
            void *context,
            const char *user,
            const char *pass,
            unsigned passlen,
            struct propctx *propctx)


Description
===========

.. c:function::   int sasl_server_userdb_checkpass_t(sasl_conn_t *conn,
        void *context,
        const char *user,
        const char *pass,
        unsigned passlen,
        struct propctx *propctx)

    **sasl_server_userdb_checkpass_t()** is used to verify a plaintext
    password against the callback supplier’s user database. This is to
    allow additional ways to encode the userPassword property.

    :param conn: is the SASL connection context

    :param context: context from the callback record

    :param user: NUL terminated user name with `user@realm` syntax

    :param pass: password to check (may not be NUL terminated)
    :param passlen: length of the password

    :param propctx: property context to fill in with userPassword

Return Value
============

SASL  callback  functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

Other return codes indicate errors and should be handled.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_callbacks(3)`
:saslman:`sasl_errors(3)`, :saslman:`sasl_server_userdb_setpass_t(3)`
