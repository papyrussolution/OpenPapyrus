.. saslman:: sasl_getsimple_t(3)

.. _sasl-reference-manpages-library-sasl_getsimple_t:


====================================================================
**sasl_getsimple_t** - The SASL callback for username/authname/realm
====================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_getsimple_t(void *context,
                        int id,
                        const char ** result,
                        unsigned * len);

Description
===========

.. c:function::   int sasl_getsimple_t(void *context,
        int id,
        const char ** result,
        unsigned * len);

    **sasl_getsimple_t** is used to retrieve simple things from
    the application. In practice this is authentication name,
    authorization name, and realm.

    :param context: SASL connection context
    :param id: indicates which value is being requested.  Possible values
        include:

        * SASL_CB_USER     ‐ Client user identity to login as
        * SASL_CB_AUTHNAME ‐ Client authentication name
        * SASL_CB_LANGUAGE ‐ Comma‐separated list of :rfc:`1766` languages
        * SASL_CB_CNONCE   ‐ Client‐nonce (for testing mostly)
    :param result: value of the item requested
    :param len: lenth of the result

Return Value
============

SASL  callback  functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_callbacks(3)`,
:saslman:`sasl_errors(3)`
