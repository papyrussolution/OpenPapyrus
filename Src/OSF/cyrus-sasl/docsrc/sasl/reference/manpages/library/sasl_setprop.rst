.. saslman:: sasl_setprop(3)

.. _sasl-reference-manpages-library-sasl_setprop:


======================================
**sasl_setprop** - Set a SASL property
======================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_setprop(sasl_conn_t *conn,
                    int propnum,
                    const void * pvalue)

Description
===========

.. c:function::  int sasl_setprop(sasl_conn_t *conn,
    int propnum,
    const void * pvalue)

    **sasl_setprop**  sets the value of a SASL property. For example an
    application should tell the SASL library about  any external negotiated
    security layer (i.e. TLS).

    :param conn: is the SASL connection context

    :param propnum: is the identifier for the property requested

    :param pvalue: contains  a pointer  to  the  data. It is the applications
        job to make sure this type is correct. This is an easy way to crash  a
        program.

        * SASL_AUTH_EXTERNAL ‐ external authentication ID (const char \*)
        * SASL_SSF_EXTERNAL ‐  external SSF active ‐‐ (sasl_ssf_t)
        * SASL_DEFUSERREALM ‐ user realm (const char \*)
        * SASL_SEC_PROPS  ‐    `sasl_security_properties_t` (may be freed after call)
        * SASL_IPLOCALPORT ‐   string describing the local ip and port in the form
                             "a.b.c.d;p", or "e:f:g:h:i:j:k:l;port"
        * SASL_IPREMOTEPORT ‐  string describing the remote ip and port in the form
                             "a.b.c.d;p", or "e:f:g:h:i:j:k:l;port"


Return Value
============

SASL  callback  functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

Other return codes indicate errors and should be handled.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_errors(3)`
