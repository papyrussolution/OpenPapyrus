.. saslman:: sasl_auxprop_getctx(3)

.. _sasl-reference-manpages-library-sasl_auxprop_getctx:

===============================================================
**sasl_auxprop_getctx** - Acquire an auxiliary property context
===============================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_auxprop_getctx(sasl_conn_t *conn)

Description
===========

.. c:function:: int sasl_auxprop_getctx(sasl_conn_t *conn)

    Fetches an auxiliary property context for the connection on which the functions
    described in :saslman:`sasl_auxprop(3)` can operate.

    :parameter conn: pointer to the :c:type:`sasl_conn_t` for which the request is being made.
    :return: A pointer to the context on success. Returns NULL on failure.

.. c:type:: sasl_conn_t

     Context for a SASL connection negotiation

Conforming to
=============

:rfc:`4422`

See Also
========

:saslman:`sasl(3)`, :saslman:`sasl_auxprop(3)`, :saslman:`sasl_auxprop_request(3)`
