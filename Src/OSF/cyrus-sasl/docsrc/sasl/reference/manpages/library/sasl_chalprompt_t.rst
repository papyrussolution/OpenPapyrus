.. saslman:: sasl_chalprompt_t(3)

.. _sasl-reference-manpages-library-sasl_chalprompt_t:

==================================================
**sasl_chalprompt_t** - Realm acquisition callback
==================================================

Synopsis
========

.. code-block:: C

    #include <sasl/prop.h>

    int sasl_chalprompt_t(void *context, int id,
        const char *challenge,
        const char *prompt, const char *defresult,
        const char **result, unsigned *len)

Description
===========

.. c:function::     int sasl_chalprompt_t(void *context,
        int id,
        const char *challenge,
        const char *prompt,
        const char *defresult,
        const char **result,
        unsigned *len)

        **sasl_chalprompt_t**  is used to prompt for input in response to a server challenge.

        :param context: is the context from the callback record
        :param id: is the callback id (either SASL_CB_ECHOPROMPT or  SASL_CB_NOECHOPROMPT)
        :param challenge: the server's challenge
        :param prompt: A prompt for the user
        :param defresult: Default result (may be NULL)
        :result: The user's response (a NUL terminated string) or SASL error code.
        :param len: Length of the user's response.

Return Value
============

The user's response (NUL terminated), or a SASL error code. See :saslman:`sasl_errors(3)`.

See Also
========

:saslman:`sasl(3)`, :saslman:`sasl_errors(3)`, :saslman:`sasl_callbacks(3)`
