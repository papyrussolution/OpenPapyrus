.. saslman:: sasl_checkpass(3)

.. _sasl-reference-manpages-library-sasl_checkpass:

===============================================
**sasl_checkpass** - Check a plaintext password
===============================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_checkpass(sasl_conn_t *conn,
                     const char *user,
                     unsigned userlen,
                     const char *pass,
                     unsigned passlen);

Description
===========

.. c:function:: int sasl_checkpass(sasl_conn_t *conn,
        const char *user,
        unsigned userlen,
        const char *pass,
        unsigned passlen)

        **sasl_checkpass**  will check a plaintext password. This is
        needed for protocols that had a login method  before  SASL
        (for  example  the LOGIN command in IMAP). The password is
        checked with the pwcheck_method. See :saslman:`sasl_callbacks(3)`  for
        information on how this parameter is set.


Return value
============

sasl_checkpass returns an integer which corresponds to one
of the following codes. :c:macro:`SASL_OK` indicates that the authentication  is  complete.  All  other  return codes indicate
errors and should either be handled or the  authentication
session  should  be quit.  See :saslman:`sasl_errors(3)` for meanings
of return codes.

Conforming to
=============

:rfc:`4422`

See Also
========

:saslman:`sasl(3)`, :saslman:`sasl_errors(3)`, :saslman:`sasl_callbacks(3)`,
:saslman:`sasl_setpass(3)`
