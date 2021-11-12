.. saslman:: sasl_idle(3)

.. _sasl-reference-manpages-library-sasl_idle:


=============================================================
**sasl_idle** - Perform precalculations during an idle period
=============================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_idle( sasl_conn_t *conn)

Description
===========

.. c:function:: int sasl_idle( sasl_conn_t *conn)

    **sasl_idle()** may be called during an idle period to allow the
    SASL library or any mechanisms to perform any necessary
    precalculation.

    :param conn: may be NULL to do precalculation prior to a
        connection taking place.

Return Value
============

Returns 1 if action was taken, 0 if no action was taken.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`
