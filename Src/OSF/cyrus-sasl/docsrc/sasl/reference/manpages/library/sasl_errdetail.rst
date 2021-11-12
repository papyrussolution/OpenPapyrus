.. saslman:: sasl_errdetail(3)

.. _sasl-reference-manpages-library-sasl_errdetail:


==================================================================
**sasl_errdetail** - Retrieve  detailed information about an error
==================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    const char *sasl_errdetail( sasl_conn_t *conn );

Description
===========

.. c:function::  const char *sasl_errdetail( sasl_conn_t *conn );

    **sasl_errdetail** provides more  detailed  information  about
    the  most  recent  error  to occur, beyond the information
    contained in the SASL result code.

    :param conn: the SASL connection context to inquire about.


Return Value
============

Returns the string describing the error that occurred,  or NULL  if  no  error
has  occurred,  or there was an error retrieving it.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`
