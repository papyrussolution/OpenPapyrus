.. saslman:: sasl_auxprop_request(3)

.. _sasl-reference-manpages-library-sasl_auxprop_request:

=================================================================
**sasl_auxprop_request** - Request auxiliary properties from SASL
=================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_auxprop_request(sasl_conn_t *conn, const char ** propnames)

Description
===========

.. c:function:: int sasl_auxprop_request(sasl_conn_t *conn, const char ** propnames)

    **sasl_auxprop_request** will request that the SASL library
    obtain properties from any auxiliary property plugins that
    might be installed (such as the user's home directory from
    an LDAP server for example). Such lookup occurs just
    after username canonicalization is complete. Therefore,
    the request should be made before the call to
    :saslman:`sasl_server_start(3)`, but after the call to
    :saslman:`sasl_server_new(3)`.

    :param conn: the :c:type:`sasl_conn_t` for which the request is being made.

    :param propnames:  a NULL-terminated array of property names to
        request.  Note that this array must persist until a call to
        :saslman:`sasl_dispose(3)` on the :c:type:`sasl_conn_t`.

    :returns: Returns  :c:macro:`SASL_OK` on success. See :saslman:`sasl_errors(3)` for meanings of other return codes.

Conforming to
=============

:rfc:`4422`

See Also
========

:saslman:`sasl(3)`, :saslman:`sasl_errors(3)`, :saslman:`sasl_auxprop(3)`, :saslman:`sasl_auxprop_getctx(3)`,
:saslman:`sasl_server_new(3)`, :saslman:`sasl_server_start(3)`
