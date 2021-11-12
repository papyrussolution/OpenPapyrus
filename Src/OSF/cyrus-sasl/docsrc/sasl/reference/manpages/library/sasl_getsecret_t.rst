.. saslman:: sasl_getsecret_t(3)

.. _sasl-reference-manpages-library-sasl_getsecret_t:


================================================================
**sasl_getsecret_t** - The SASL callback for secrets (passwords)
================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_getsecret_t(sasl_conn_t *conn,
                        void *context,
                        int id,
                        sasl_secret_t ** psecret);


Description
===========

.. c:function:: int sasl_getsecret_t(sasl_conn_t *conn,
        void *context,
        int id,
        sasl_secret_t ** psecret);


    **sasl_getsecret_t()** is used to retrieve the secret  from  the
    application. A sasl_secret_t should be allocated to length
    `sizeof(sasl_secret_t) + <length of secret>`.  It  has two
    fields: `len` which is the length of the secret in bytes and
    `data` which contains the secret itself (does not need to be
    null terminated).

    :param conn: is the SASL connection context

Return Value
============

SASL  callback  functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

Other return codes indicate errors and should be handled.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_callbacks(3)`
