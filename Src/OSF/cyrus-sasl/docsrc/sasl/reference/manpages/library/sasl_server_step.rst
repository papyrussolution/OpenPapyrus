.. saslman:: sasl_server_step(3)

.. _sasl-reference-manpages-library-sasl_server_step:


=======================================================================
**sasl_server_step** - Perform a step in the authentication negotiation
=======================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_server_step(sasl_conn_t *conn,
        const char *clientin,
        unsigned clientinlen,
        const char ** serverout,
        unsigned * serveroutlen);

Description
===========

.. c:function::  int sasl_server_step(sasl_conn_t *conn,
        const char *clientin,
        unsigned clientinlen,
        const char ** serverout,
        unsigned * serveroutlen);

    **sasl_server_step()** performs a step in  the  authentication negotiation.  It
    returns :c:macro:`SASL_OK` if the whole negotiation is successful and
    :c:macro:`SASL_CONTINUE` if
    this step is ok but  at least  one more step is needed.

    :param conn: is the SASL connection context

    :param clientin: is the data given by the client (decoded  if  the
        protocol encodes requests sent over the wire)
    :param clientinlen: is the length of `clientin`

    :param serverout: set by the library and should be sent to the client.
    :param serveroutlen: length of `serverout`.


Return Value
============

SASL  callback  functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_CONTINUE` indicates success
and that there are more steps needed in the authentication. :c:macro:`SASL_OK`
indicates that the authentication is complete.

Other return codes indicate errors and should either be handled or the authentication
session should be quit.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`,
:saslman:`sasl_server_init(3)`, :saslman:`sasl_server_new(3)`,
:saslman:`sasl_server_start(3)`, :saslman:`sasl_errors(3)`
