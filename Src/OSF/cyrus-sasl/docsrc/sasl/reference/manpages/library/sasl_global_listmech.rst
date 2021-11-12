.. saslman:: sasl_global_listmech(3)

.. _sasl-reference-manpages-library-sasl_global_listmech:


===========================================================================
**sasl_global_listmech** - Retrieve a list of the supported SASL mechanisms
===========================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    const char ** sasl_global_listmech();

Description
===========

.. c:function::  const char ** sasl_global_listmech();


    **sasl_global_listmech** returns a null‐terminated array of
    strings that lists all mechanisms that are loaded by
    either the client or server side of the library.

Return Value
============

Returns a pointer to the array on success. NULL on failure
(sasl library uninitialized).

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_server_init(3)`,
:saslman:`sasl_listmech(3)`, :saslman:`sasl_client_init(3)`
