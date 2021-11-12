.. saslman:: sasl_errstring(3)

.. _sasl-reference-manpages-library-sasl_errstring:


==========================================================================
**sasl_errstring** - Translate a SASL return code to a human-readable form
==========================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    const char * sasl_errstring(int saslerr,
        const char * langlist,
        const char ** outlang);

Description
===========

.. c:function::  const char * sasl_errstring(int saslerr,
        const char * langlist,
        const char ** outlang);

    **sasl_errstring** is called to convert a SASL return code (an
    integer) into a human readable string. At this time the
    only language available is American English. Note that if the string is
    going to be sent to the client, a server should
    call :saslman:`sasl_usererr(3)` on a return code first.

    :param saslerr: specifies the error number to convert.

    :param langlist: is currently unused; Use NULL.

    :param outlang:  specifies  the desired :rfc:`1766` language for
        output.  NULL defaults to "en‐us"; currently the only supported
        language.

    This function is not the recommended means of extracting error code
    information from SASL,  instead  application  should use
    :saslman:`sasl_errdetail(3)`, which contains this information (and more).

Return Value
============

Returns the string.  If  langlist  is  NULL,  US‐ASCII  is used.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_errdetail(3)`,
:saslman:`sasl_errors(3)`
