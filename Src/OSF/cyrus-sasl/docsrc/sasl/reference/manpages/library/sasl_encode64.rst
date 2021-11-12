.. saslman:: sasl_encode64(3)

.. _sasl-reference-manpages-library-sasl_encode64:


========================================
**sasl_encode64** - Encode base64 string
========================================

Synopsis
========

.. code-block:: C

    #include <sasl/saslutil.h>

    int sasl_encode64(const char * input,
                    unsigned inputlen,
                    char * output,
                    unsigned outmax,
                    unsigned * outputlen);

Description
===========

Use the **sasl_encode64()** interface to convert an octet string into a base64
string. This routine is useful for SASL profiles that use base64, such as the
IMAP (IMAP4) and POP (POP_AUTH) profiles. The output is null‐terminated. If
outlen is non‐NULL, the length is placed in the outlen.

.. c:function:: int sasl_encode64(const char * input,
            unsigned inputlen,
            const char ** output,
            unsigned outmax,
            unsigned * outputlen);

    :param input: input data.

    :param inputlen: length of the input data.

    :param output: contains the decoded data. The value of out can be the
        same as in. However, there must be enough space.

    :param outputlen: length of `output`.

    :param outmax: The maximum size of the output buffer.

Return Value
============

SASL functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

Other return codes indicate errors and should be handled.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_decode64(3)`,
:saslman:`sasl_errors(3)`, :saslman:`sasl_encode(3)`
