.. saslman:: sasl_done(3)

.. _sasl-reference-manpages-library-sasl_done:


===================================================
**sasl_done** - Dispose of a SASL connection object
===================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    void sasl_done( void );

Description
===========

.. c:function::        void sasl_done( void );

    **sasl_done()** is  called  when  the application is completely
    done with the SASL library.

Return Value
============

No return values.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`,
:saslman:`sasl_server_init(3)`, :saslman:`sasl_client_init(3)`
