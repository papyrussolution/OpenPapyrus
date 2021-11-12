.. saslman:: sasl_checkapop(3)

.. _sasl-reference-manpages-library-sasl_checkapop:

=====================================================
**sasl_checkapop** - Check an APOP challenge/response
=====================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_checkapop(sasl_conn_t *conn,
                     const char *challenge,
                     unsigned challen,
                     const char *response,
                     unsigned resplen)

Description
===========

.. c:function::     int sasl_checkapop(sasl_conn_t *conn,
                     const char *challenge,
                     unsigned challen,
                     const char *response,
                     unsigned resplen)

    **sasl_checkapop**  will  check  an APOP challenge/response.
    APOP is an optional POP3 (:rfc:`1939`) authentication command
    which  uses  a  shared  secret (password). The password is
    stored in the SASL secrets database.  For  information  on
    the  SASL  shared secrets database see the :ref:`System Administrators Guide <sysadmin>`.

    If  called  with  a  NULL challenge, sasl_checkapop() will
    check to see if the APOP mechanism is enabled.

Return value
============

sasl_checkapop returns an integer which corresponds to one
of the following codes. :c:macro:`SASL_OK` indicates that the authentication is complete.  All  other  return  codes  indicate
errors  and should either be handled or the authentication
session should be quit.  See :saslman:`sasl_errors(3)`  for  meanings
of return codes.

Conforming to
=============

:rfc:`4422`, :rfc:`1939`

See Also
========

:saslman:`sasl(3)`, :saslman:`sasl_errors(3)`
