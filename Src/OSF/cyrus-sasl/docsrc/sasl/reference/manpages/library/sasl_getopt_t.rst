.. saslman:: sasl_getopt_t(3)

.. _sasl-reference-manpages-library-sasl_getopt_t:

================================================
**sasl_getopt_t** - The SASL get option callback
================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_getopt_t(void *context,
                   const char *plugin_name,
                   const char *option,
                   const char ** result,
                   unsigned * len);

Description
===========

.. c:function:: int sasl_getopt_t(void *context,
        const char *plugin_name,
        const char *option,
        const char ** result,
        unsigned * len);

    **sasl_getopt_t** is used to retrieve an option, often mechanism specific,
    from the application. An example of this is
    requesting what KERBEROS_V4 srvtab file to use.

    :param context: is the SASL connection context
    :param plugin_name: is the plugin this value is for.
    :param option: is a string representing the option. A common option that all
        server applications should handle is the method for checking
        plaintext passwords.  See the `administrators
        guide <https://www.cyrusimap.org/sasl/sasl/sysadmin.html>`_ for a
        full description of this option.

    Memory management of options supplied by the getopt callback
    should be done by the application, however, any
    requested option must remain available until the callback
    is no longer valid.  That is, when :saslman:`sasl_dispose(3)` is called
    for a the connection it is associated with,  or  :saslman:`sasl_done(3)`
    is called for global callbacks.

Return Value
============

SASL callback functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

Other return codes indicate errors and should be handled.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_errors(3)`
:saslman:`sasl_callbacks(3)`
