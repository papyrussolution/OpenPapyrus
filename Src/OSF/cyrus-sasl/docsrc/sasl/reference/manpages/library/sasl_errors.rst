.. saslman:: sasl_errors(3)

.. _sasl-reference-manpages-library-sasl_errors:

==================================
**sasl_errors** - SASL error codes
==================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

Description
===========

The  following  are  the  general  error codes that may be
returned by calls into the SASL library, and  their  meanings (that may vary slightly based on context).

Common Result Codes
-------------------

.. c:macro:: SASL_OK

    Success

.. c:macro:: SASL_CONTINUE

       Another step is needed in authentication

.. c:macro:: SASL_FAIL

       Generic Failure

.. c:macro:: SASL_NOMEM

       Memory shortage failure

.. c:macro:: SASL_BUFOVER

       Overflowed buffer

.. c:macro:: SASL_NOMECH

       Mechanism  not  supported  / No mechanisms matched
       requirements

.. c:macro:: SASL_BADPROT

       Bad / Invalid Protocol or Protocol cancel

.. c:macro:: SASL_NOTDONE

       Cannot request information / Not  applicable  until
       later in exchange

.. c:macro:: SASL_BADPARAM

       Invalid Parameter Supplied

.. c:macro:: SASL_TRYAGAIN

       Transient Failure (e.g. weak key)

.. c:macro:: SASL_BADMAC

        Integrity Check Failed

.. c:macro:: SASL_NOTINIT

        SASL library not initialized


Client-only Result Codes
------------------------

.. c:macro:: SASL_INTERACT

       Needs user interaction

.. c:macro:: SASL_BADSERV

       Server failed mutual authentication step

.. c:macro:: SASL_WRONGMECH

       Mechanism does not support requested feature


Server-only Result Codes
------------------------

.. c:macro:: SASL_BADAUTH

       Authentication Failure

.. c:macro:: SASL_NOAUTHZ

       Authorization Failure

.. c:macro:: SASL_TOOWEAK

       Mechanism too weak for this user

.. c:macro:: SASL_ENCRYPT

       Encryption needed to use mechanism

.. c:macro:: SASL_TRANS

       One  time  use of a plaintext password will enable
       requested mechanism for user

.. c:macro:: SASL_EXPIRED

       Passphrase expired, must be reset

.. c:macro:: SASL_DISABLED

       Account Disabled

.. c:macro:: SASL_NOUSER

       User Not Found

.. c:macro:: SASL_BADVERS

       Version mismatch with plug-in

.. c:macro:: SASL_NOVERIFY

       User exists, but no verifier for user

Password Setting Result Codes
-----------------------------

.. c:macro:: SASL_PWLOCK

       Passphrase locked

.. c:macro:: SASL_NOCHANGE

       Requested change was not needed

.. c:macro:: SASL_WEAKPASS

       Passphrase is too week for security policy.

.. c:macro:: SASL_NOUSERPASS

       User supplied passwords are not permitted

Conforming to
=============

:rfc:`4422`

See Also
========

:saslman:`sasl(3)`
