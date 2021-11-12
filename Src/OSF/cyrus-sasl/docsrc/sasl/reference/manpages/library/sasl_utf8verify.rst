.. saslman:: sasl_utf8verify(3)

.. _sasl-reference-manpages-library-sasl_utf8verify:


===================================================
**sasl_utf8verify** - Verify a string is valid utf8
===================================================

Synopsis
========

.. code-block:: C

    #include <sasl/saslutil.h>

    int sasl_utf8verify(const char *str,
                        unsigned len);

Description
===========

.. c:function:: int sasl_utf8verify(const char *str,
        unsigned len);

    Use the **sasl_utf8verify** interface to verify that a string is
    valid UTF‐8 and does not contain NULL, a carriage return, or a linefeed.
    If len == 0, strlen(str) will be used.

    :param str: A string.

    :param len: The length of the string. If len == 0, strlen(str) will be used.

Return Value
============

SASL functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

Other return codes indicate errors and should be handled.

* :c:macro:`SASL_BADPROT`: There was invalid UTF8, or an error was found.

See Also
========

:saslman:`sasl_errors(3)`
