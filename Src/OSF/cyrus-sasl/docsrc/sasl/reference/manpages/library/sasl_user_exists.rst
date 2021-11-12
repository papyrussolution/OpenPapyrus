.. saslman:: sasl_user_exists(3)

.. _sasl-reference-manpages-library-sasl_user_exists:


=======================================================
**sasl_user_exists** - Check if a user exists on server
=======================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_user_exists( sasl_conn_t *conn,
                          const char *service,
                          const char *user_realm,
                          const char *user)


Description
===========

.. c:function:: int sasl_user_exists( sasl_conn_t *conn,
        const char *service,
        const char *user_realm,
        const char *user)


    **sasl_user_exists** will check if a user exists on the server.

    :param conn: the SASL context for this connection

    :param service: Service name or NULL (for service name of
        connection context)

    :param user_realm: Realm to check in or NULL (for default realm)

    :param user: User name to check for existence.


Return Value
============

SASL functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`,:saslman:`sasl_errors(3)`
