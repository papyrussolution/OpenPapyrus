.. saslman:: sasl_dispose(3)

.. _sasl-reference-manpages-library-sasl_dispose:

======================================================
**sasl_dispose** - Dispose of a SASL connection object
======================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    void sasl_dispose(sasl_conn_t **pconn );


Description
===========

.. c:function:: int sasl_encode(sasl_conn_t *conn,
            const char * input,
            unsigned inputlen,
            const char ** output,
            unsigned * outputlen);

    **sasl_dispose** is called when a SASL connection object is no longer needed.

    Note that this is usually when the protocol session is done NOT when the
    authentication is done since a security layer may have been negotiated.

    :param conn: is the SASL connection context


Return value
============

No return values

Conforming to
=============

:rfc:`4422`

See Also
========

:saslman:`sasl(3)`, :saslman:`sasl_server_new(3)`, :saslman:`sasl_client_new(3)`,
