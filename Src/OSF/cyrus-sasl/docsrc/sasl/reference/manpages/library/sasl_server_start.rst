.. saslman:: sasl_server_start(3)

.. _sasl-reference-manpages-library-sasl_server_start:


===========================================================
**sasl_server_start** - Begin an authentication negotiation
===========================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_server_start(sasl_conn_t * conn,
                 const char * mech,
                 const char * clientin,
                 unsigned clientinlen,
                 const char ** serverout,
                 unsigned * serveroutlen);

Description
===========

.. c:function::  int sasl_server_start(sasl_conn_t * conn,
           const char * mech,
           const char * clientin,
           unsigned * clientinlen,
           const char ** serverout,
           unsigned * serveroutlen);

    **sasl_server_start()** begins  the  authentication  with the
    mechanism specified with mech. This fails if the mechanism
    is  not  supported.

    :param conn: is the SASL connection context
    :param mech: is the mechanism name that the client requested
    :param clientin:  is the client initial response, NULL if the protocol
        lacks support for client‐send‐first or if the  other
        end  did  not  have an initial send.  Note that no initial
        client send is distinct from an initial  send  of  a  null
        string, and the protocol MUST account for this difference.
    :param clientinlen: is the length of initial response
    :param serverout: is created by the plugin library. It is the initial
        server response to send to the client. This is  allocated/freed by the
        library and it is the job of the client
        to send it over the network to the server.  Also  protocol
        specific  encoding (such as base64 encoding) must needs to
        be done by the server.
    :param serveroutlen: is set to the length of initial server challenge

Return Value
============

SASL  callback  functions should return SASL return codes. See sasl.h for a
complete list. :c:macro:`SASL_OK` is returned if the authentication is complete
and the user is authenticated.  :c:macro:`SASL_CONTINUE`  is returned if one or
more steps are still required in the authentication.

All other return values indicate errors and should be handled or the
authentication session should be quit.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`,
:saslman:`sasl_server_init(3)`, :saslman:`sasl_server_new(3)`,
:saslman:`sasl_server_step(3)`, :saslman:`sasl_errors(3)`
