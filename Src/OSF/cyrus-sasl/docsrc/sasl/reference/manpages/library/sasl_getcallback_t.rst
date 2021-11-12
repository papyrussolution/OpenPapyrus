.. saslman:: sasl_getcallback_t(3)

.. _sasl-reference-manpages-library-sasl_getcallback_t:

=======================================================================================
**sasl_getcallback_t** - callback function to lookup a sasl_callback_t for a connection
=======================================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/saslplug.h>

    int sasl_getcallback_t(sasl_conn_t *conn,
                   unsigned long callbacknum,
                             int (**proc)( ),
                             void **pcontext);

Description
===========
.. c:function:: int sasl_getcallback_t(sasl_conn_t *conn,
        unsigned long callbacknum,
        int (**proc)( ),
        void **pcontext);

    The **sasl_getcallback_t()** function is a callback to lookup
    a sasl_callback_t for a connection.

    :param conn: The connection to lookup a callback for.
    :param callbacknum: The number of the callback.
    :param proc: Pointer to the callback function. The value of proc is
        set to NULL upon failure.
    :param pcontext: Pointer to the callback context. The value of pcontext
        is set to NULL upon failure.

Return value
============

SASL callback functions should return SASL return codes.
See :saslman:`sasl_errors(3)` for a complete list.  :c:macro:`SASL_OK`  typically indicates success.

* :c:macro:`SASL_FAIL`: Unable to find a callback of the requested type.
* :c:macro:`SASL_INTERACT`: The caller must use interaction to get data.

Conforming to
=============

:rfc:`4422`

See Also
========

:saslman:`sasl(3)`, :saslman:`sasl_errors(3)`, :saslman:`sasl_callbacks(3)`
