.. saslman:: sasl_server_done(3)

.. _sasl-reference-manpages-library-sasl_server_done:


=======================================
**sasl_server_done** - Cleanup function
=======================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_server_done();

Description
===========

.. c:function::   int sasl_server_done();

    **sasl_server_done()** is a cleanup function, used to free all memory
    used by the library. Invoke when processing is complete.



Return Value
============

Returns :c:macro:`SASL_OK` if the whole cleanup is successful and
:c:macro:`SASL_CONTINUE` if this step is ok but at least one more step is needed.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`,
:saslman:`sasl_server_init(3)`, :saslman:`sasl_server_new(3)`,
:saslman:`sasl_server_start(3)`, :saslman:`sasl_errors(3)`,
:saslman:`sasl_done(3)`
