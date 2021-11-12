.. saslman:: sasl_decode64(3)

.. _sasl-reference-manpages-library-sasl_decode64:


========================================
**sasl_decode64** - Decode base64 string
========================================

Synopsis
========

.. code-block:: C

    #include <sasl/saslutil.h>

    int sasl_decode64(const char * input,
                    unsigned inputlen,
                   char * output,
                   unsigned outmax,
                   unsigned * outputlen);


Description
===========

.. c:function::   int sasl_decode64(const char * input,
        unsigned inputlen,
        const char ** output,
        unasigned outmax,
        unsigned * outputlen);


    **sasl_decode64** decodes a base64 encoded buffer.

    :param input: Input data.

    :param inputlen: The length of the input data.

    :param output: contains the decoded data. The value of output can be the
        same as in. However, there must be enough space.

    :param outmax: The maximum size of the output buffer.

    :param outputlen: length of `output`.

Return Value
============

SASL  callback  functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

Other return codes indicate errors and should be handled.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_decode(3)`,
:saslman:`sasl_errors(3)`
