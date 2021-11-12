.. saslman:: sasl_server_init(3)

.. _sasl-reference-manpages-library-sasl_server_init:


================================================================
**sasl_server_init** - SASL server authentication initialization
================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_server_init(const sasl_callback_t *callbacks,
                         const char *appname);

Description
===========

.. c:function:: int sasl_server_init(const sasl_callback_t *callbacks,
                     const char *appname);

    **sasl_server_init()** initializes SASL.  It  must  be  called
    before  any  calls to sasl_server_start, and only once per
    process.  This call initializes all SASL mechanism drivers
    (e.g.  authentication mechanisms). These are usually found
    in the /usr/lib/sasl2 directory but the directory  may  be
    overridden  with the SASL_PATH environment variable (or at
    compile time).

    :param callbacks: specifies the base callbacks for all client connections.
        See the :saslman:`sasl_callbacks(3)` man page for more information.

    :param appname: is the name of the application.  It  is  used to find the
        default configuration file.

Return Value
============

SASL  callback  functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

Other return codes indicate errors and should either be handled or the authentication
session should be quit.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_callbacks(3)`,
:saslman:`sasl_server_new(3)`, :saslman:`sasl_server_start(3)`,
:saslman:`sasl_server_step(3)`, :saslman:`sasl_errors(3)`
