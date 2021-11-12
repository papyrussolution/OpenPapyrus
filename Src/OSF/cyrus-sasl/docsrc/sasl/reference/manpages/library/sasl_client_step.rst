.. saslman:: sasl_client_step(3)

.. _sasl-reference-manpages-library-sasl_client_step:


=======================================================================
**sasl_client_step** - Perform a step in the authentication negotiation
=======================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_client_step(sasl_conn_t *conn,
                   const char *serverin,
                   unsigned serverinlen,
                   sasl_interact_t ** prompt_need,
                   const char ** clientout,
                   unsigned * clientoutlen);

Description
===========

.. c:function::   int sasl_client_step(sasl_conn_t *conn,
        const char *serverin,
        unsigned serverinlen,
        sasl_interact_t ** prompt_need,
        const char ** clientout,
        unsigned * clientoutlen);

    **sasl_client_step()** performs a step in  the  authentication negotiation.  It
    returns :c:macro:`SASL_OK` if the whole negotiation is successful and
    :c:macro:`SASL_CONTINUE` if
    this step is ok but  at least  one more step is needed. A client should not
    assume an authentication negotiation is successful  just  because the
    server  signaled  success  via  protocol (i.e. if the server  said  ".  OK
    Authentication  succeeded"  in  IMAP, sasl_client_step should still be called
    one more time with a `serverinlen` of zero.

    If :c:macro:`SASL_INTERACT` is returned the library needs some values
    to  be  filled  in  before it can proceed. `The prompt_need`
    structure will be filled in with requests. The application
    should  fulfill  these requests and call sasl_client_start
    again with identical parameters (the `prompt_need` parameter
    will  be  the  same pointer as before but filled in by the
    application).

    :param conn: is the SASL connection context

    :param serverin: is the data given by the server (decoded  if  the
        protocol encodes requests sent over the wire)

    :param serverinlen: is the length of `serverin`

    :param clientout: is created. It is  the  initial
        client  response  to  send to the server. It is the job of
        the client to send it over the network to the server.  Any
        protocol  specific encoding (such as base64 encoding) nec‐
        essary needs to be done by the client.

    :param clientoutlen: length of `clientout`.


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

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_callbacks(3)`,
:saslman:`sasl_client_init(3)`, :saslman:`sasl_client_new(3)`,
:saslman:`sasl_client_start(3)`, :saslman:`sasl_errors(3)`
