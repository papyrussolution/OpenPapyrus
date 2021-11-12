.. saslman:: sasl_getprop(3)

.. _sasl-reference-manpages-library-sasl_getprop:


======================================
**sasl_getprop** - Get a SASL property
======================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_getprop(sasl_conn_t *conn,
                    int propnum,
                    const void ** pvalue);

Description
===========

.. c:function::  int sasl_getprop(sasl_conn_t *conn,
        int propnum,
        const void ** pvalue);

    **sasl_getprop**  gets the value of a SASL property. For example after
    successful authentication a server may  wish  to know  the  authorization
    name. Or a client application may wish to know  the  strength  of  the
    negotiated  security layer.

    :param conn: is the SASL connection context

    :param propnum: is the identifier for the property requested

    :param pvalue: is filled on success. List of properties:

        * SASL_USERNAME     ‐  pointer to NUL terminated user name
        * SASL_SSF          ‐  security layer security strength factor,
            if 0, call to :saslman:`sasl_encode(3)`, :saslman:`sasl_decode(3)`
            unnecessary
        * SASL_MAXOUTBUF    ‐  security layer max output buf unsigned
        * SASL_DEFUSERREALM ‐  server authentication realm used
        * SASL_GETOPTCTX    ‐  context for getopt callback
        * SASL_IPLOCALPORT  ‐  local address string
        * SASL_IPREMOTEPORT ‐  remote address string
        * SASL_SERVICE      ‐  service passed to `sasl_*_new`
        * SASL_SERVERFQDN   ‐  serverFQDN passed to `sasl_*_new`
        * SASL_AUTHSOURCE   ‐  name of auth source last used, useful for failed
            authentication tracking
        * SASL_MECHNAME     ‐  active mechanism name, if any
        * SASL_PLUGERR      ‐  similar to `sasl_errdetail`


Return Value
============

SASL  callback  functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

Other return codes indicate errors and should be handled.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_errors(3)`
:saslman:`sasl_server_new(3)`, :saslman:`sasl_client_new(3)`
