.. saslman:: sasl_verifyfile_t(3)

.. _sasl-reference-manpages-library-sasl_verifyfile_t:


==================================================
**sasl_verifyfile_t** - The SASL file verification
==================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    typedef enum {
        SASL_VRFY_PLUGIN, /* a DLL/shared library plugin */
        SASL_VRFY_CONF,   /* a configuration file */
        SASL_VRFY_PASSWD, /* a password storage file */
        SASL_VRFY_OTHER   /* some other file type */
    } sasl_verify_type_t

    int sasl_verifyfile_t(void *context,
                    const char *file,
                    sasl_verify_type_t type)


Description
===========

.. c:function::    int sasl_verifyfile_t(void *context,
        const char *file,
        sasl_verify_type_t type)

    **sasl_verifyfile_t()** is used to check whether a given file is
    okay for use by the SASL library.   This  is  intended  to
    allow  applications  to  sanity  check  the environment. For example, to
    ensure that plugins or the config file cannot  be  written
    to.

    :param context: context from the callback record

    :param context: context from the callback record

    :param file: full path of the file to verify

    :param type: type of the file.


Return Value
============

SASL  callback  functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

Other return codes indicate errors and should be handled.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_callbacks(3)`
:saslman:`sasl_errors(3)`
