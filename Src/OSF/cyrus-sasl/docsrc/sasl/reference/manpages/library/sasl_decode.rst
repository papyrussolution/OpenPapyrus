.. saslman:: sasl_decode(3)

.. _sasl-reference-manpages-library-sasl_decode:


======================================
**sasl_decode** - Decode data received
======================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_decode(sasl_conn_t *conn,
                   const char * input,
                    unsigned inputlen,
                   const char ** output,
                   unsigned * outputlen);


Description
===========

.. c:function::   int sasl_decode(sasl_conn_t *conn,
        const char * input,
        unsigned inputlen,
        const char ** output,
        unsigned * outputlen);


    **sasl_decode** decodes   data  received.  After  successful authentication
    this function should be called on all  data received.  It  decodes  the
    data from encrypted or signed form to plain data. If there was no security
    layer negotiated the `output` is identical to the `input`.

    :param conn: is the SASL connection context

    :param output: contains the decoded data and is allocated/freed by
        the library.

    :param outputlen: length of `output`.

    One should not give  sasl_decode  more  data  than  the
    negotiated `maxbufsize` (see :saslman:`sasl_getprop(3)`).

    Note  that  sasl_decode  can  succeed and outputlen can be
    zero. If this is the case simply wait for  more  data  and
    call sasl_decode again.

Return Value
============

SASL  callback  functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

Other return codes indicate errors and should be handled.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_encode(3)`,
:saslman:`sasl_errors(3)`
