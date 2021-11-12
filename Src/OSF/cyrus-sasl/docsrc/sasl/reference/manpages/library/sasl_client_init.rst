.. saslman:: sasl_client_init(3)

.. _sasl-reference-manpages-library-sasl_client_init:


================================================================
**sasl_client_init** - SASL client authentication initialization
================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_client_init(const  sasl_callback_t *callbacks )

Description
===========

**sasl_client_init** initializes SASL.

It  must  be  called  before  any  calls  to
:saslman:`sasl_client_start(3)`. This call initializes all SASL client  drivers
(e.g.  authentication mechanisms). These are usually found in the
`/usr/lib/sasl2` directory but the directory may be overridden with
the `SASL_PATH` environment variable.

Return Value
============

SASL  callback  functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

The following return codes indicate errors and should either be handled or the authentication
session should be quit:

* :c:macro:`SASL_BADVERS`: Mechanism version mismatch
* :c:macro:`SASL_BADPARAM`: Error in config file
* :c:macro:`SASL_NOMEM`: Not enough memory to complete operation

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_callbacks(3)`,
:saslman:`sasl_client_new(3)`, :saslman:`sasl_client_start(3)`,
:saslman:`sasl_client_step(3)`
