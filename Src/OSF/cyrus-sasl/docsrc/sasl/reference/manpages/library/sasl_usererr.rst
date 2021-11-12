.. saslman:: sasl_usererr(3)

.. _sasl-reference-manpages-library-sasl_usererr:


===============================================================================
**sasl_usererr** - Remove information leak about accounts from sasl error codes
===============================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    static int sasl_usererr(int saslerr)

Description
===========

.. c:function::  int sasl_usererr(int saslerr)

    **sasl_usererr** is called to hide any potential data leaks to a client,
    by preventing a client from discovering if a username exists or if
    a user exists but the password is wrong.

    :param saslerr: specifies the error number to convert.

    This function should be called before calling :saslman:`sasl_errstring(3)`
    or :saslman:`sasl_errdetail(3)` if information is being passed to a client.

Return Value
============

Returns a client-safe error code.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_errdetail(3)`,
:saslman:`sasl_errors(3)`, :saslman:`sasl_errstring(3)`
