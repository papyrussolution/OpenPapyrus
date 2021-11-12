.. saslman:: sasl_log_t(3)

.. _sasl-reference-manpages-library-sasl_log_t:


==========================================
**sasl_log_t** - The SASL logging callback
==========================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_log_t(void *context,
                  int level,
                  const char * message);

Description
===========

.. c:function:: int sasl_log_t(void *context,
        int level,
        const char * message);

    **sasl_log_t** is used to log warning/error messages from the
       SASL library. If not specified :manpage:`syslog` will be used.


Return Value
============

SASL  callback  functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_callbacks(3)`,
:saslman:`sasl_errors(3)`
