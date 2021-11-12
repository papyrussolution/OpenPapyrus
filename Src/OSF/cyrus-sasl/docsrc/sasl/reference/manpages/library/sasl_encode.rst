.. saslman:: sasl_encode(3)

.. _sasl-reference-manpages-library-sasl_encode:


=================================================================
**sasl_encode** - Encode data for transport to authenticated host
=================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_encode(sasl_conn_t *conn,
                    const char * input,
                    unsigned inputlen,
                    const char ** output,
                    unsigned * outputlen);

    int sasl_encodev(sasl_conn_t *conn,
                    const struct iovec * invec,
                    unsigned numiov,
                    const char ** output,
                    unsigned * outputlen);


Description
===========

**sasl_encode** encodes data to be sent to be sent to a remote host  who  we’ve
had  a successful authentication session with. If there  is  a  negotiated
security  the  data  in signed/encrypted  and  the  output  should be sent
without modification to the remote host. If there is  no  security layer the
output is identical to the input.

**sasl_encodev** does the same, but for a `struct iovec` instead
of a character buffer.

.. c:function:: int sasl_encode(sasl_conn_t *conn,
            const char * input,
            unsigned inputlen,
            const char ** output,
            unsigned * outputlen);

    :param conn: is the SASL connection context

    :param output: contains the decoded data and is allocated/freed by
        the library.

    :param outputlen: length of `output`.

    .. c:function:: int sasl_encodev(sasl_conn_t *conn,
                const struct iovec * invec,
                unsigned numiov,
                const char ** output,
                unsigned * outputlen);

    :param conn: is the SASL connection context

    :param output: contains the decoded data and is allocated/freed by
        the library.

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
