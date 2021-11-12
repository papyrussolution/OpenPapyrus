.. saslman:: sasl_canon_user_t(3)

.. _sasl-reference-manpages-library-sasl_canon_user_t:

===========================================================================
**sasl_canon_user_t** - Application-supplied user canonicalization function
===========================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_canon_user_t(sasl_conn_t *conn, void *context, const char *user, unsigned ulen,
                          unsigned flags, const char *user_realm, char *out_user,
                          unsigned out_umax, unsigned *out_ulen)

Description
===========

.. c:function:: int sasl_canon_user_t(sasl_conn_t *conn,
        void *context,
        const char *user,
        unsigned ulen,
        unsigned flags,
        const char *user_realm,
        char *out_user,
        unsigned out_umax,
        unsigned *out_ulen)

    **sasl_canon_user_t** is the callback for an  application-supplied  user  canonicalization  function.  This function is
    subject to the requirements that all user canonicalization
    functions  are:  It  must  copy the result into the output
    buffers, but the output buffers and the input buffers  may
    be the same.

    :param context: context from the callback record
    :param user: un-canonicalized username
    :param ulen: length of user
    :param flags: Either SASL_CU_AUTHID (indicating the authentication ID is being canonicalized) or SASL_CU_AUTHZID  (indicating the  authorization ID is to be canonicalized) or a bitwise OR of the the two.
    :param user_realm: Realm of authentication.
    :param out_user: The output buffer for the canonicalized username
    :param out_umax: Maximum length for out_user
    :param out_ulen: Actual length of out_user
    :returns: :c:macro:`SASL_OK` indicates success. See :saslman:`sasl_errors(3)` for a full list of SASL error codes.

Conforming to
=============

:rfc:`4422`

See Also
========

:saslman:`sasl(3)`, :saslman:`sasl_errors(3)`, :saslman:`sasl_callbacks(3)`
