.. saslman:: sasl_setpass(3)

.. _sasl-reference-manpages-library-sasl_setpass:


=============================================
**sasl_setpass** - Check a plaintext password
=============================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_setpass(sasl_conn_t *conn,
                     const char *user,
                     const char *pass, unsigned passlen,
                      const char *oldpass, unsigned oldpasslen,
                      unsigned flags)

Description
===========

.. c:function::  int sasl_setpass(sasl_conn_t *conn,
    const char *user,
    const char *pass, unsigned passlen,
    const char *oldpass, unsigned oldpasslen,
    unsigned flags)

    **sasl_setpass** will set passwords in the sasldb, and trigger the setpass
    callbacks for all available mechanisms.

    :param conn: is the SASL connection context

    :param user: is the username to set the password for

    :param pass: the password to set
    :param passlen: length of the password to set (`pass`)

    :param oldpass: optional. The old password.
    :param oldpasslen: optional. The old password length.

    :param flags: are flags including `SASL_SET_CREATE` and
     `SASL_SET_DISABLE` (to cause the creating of nonexistent accounts and the
     disabling of an account, respectively)

     `oldpass` and `oldpasslen` are unused in the Cyrus SASL implementation, though
     are passed on to any mechanisms that may require them.


Return Value
============

SASL  callback  functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

Other return codes indicate errors and should be handled.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_errors(3)`,
:saslman:`sasl_checkpass(3)`
