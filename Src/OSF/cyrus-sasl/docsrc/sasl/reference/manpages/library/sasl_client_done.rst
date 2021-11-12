.. saslman:: sasl_client_done(3)

.. _sasl-reference-manpages-library-sasl_client_done:


=======================================
**sasl_client_done** - Cleanup function
=======================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_client_done();

Description
===========

.. c:function::   int sasl_client_done();

    **sasl_client_done()** is a cleanup function, used to free all memory
    used by the library. Invoke when processing is complete.



Return Value
============

Returns :c:macro:`SASL_OK` if the whole cleanup is successful and
:c:macro:`SASL_CONTINUE` if this step is ok but at least one more step is needed.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_done(3)`,
:saslman:`sasl_client_init(3)`, :saslman:`sasl_client_new(3)`,
:saslman:`sasl_client_start(3)`, :saslman:`sasl_errors(3)`
