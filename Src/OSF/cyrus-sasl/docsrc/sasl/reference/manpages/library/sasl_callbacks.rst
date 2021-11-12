.. saslman:: sasl_callbacks(3)

.. _sasl-reference-manpages-library-sasl_callbacks:

====================================================
**sasl_callbacks** - How to work with SASL callbacks
====================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

Description
===========

**sasl_callbacks**  are  used  when the application needs some
information from the application. Common reasons are  getting
for  getting  usernames and passwords. A client MUST
specify   what   callbacks    they    support    in    the
:saslman:`sasl_client_init(3)`/:saslman:`sasl_server_init(3)`
or   :saslman:`sasl_client_new(3)`/:saslman:`sasl_server_new(3)`
calls. If an authentication  mechanism  needs  a  callback
that  the application does not state it supports it cannot
be used.

If a callback has an id parameter that should  be  checked
to make sure you are giving the appropriate value.

If  an application is using the client side of the library
functions to  handle  the  callbacks  are  not  necessary.
Instead  the  application  may  deal  with  callbacks  via
SASL_INTERACT's.  See  :saslman:`sasl_client_start(3)`/:saslman:`sasl_client_step(3)`  for  more
information.

Common Callbacks
----------------

sasl_getopt_t
        Get an option value

sasl_log_t
        Log message handler

sasl_getpath_t
        Get  path  to search for plugins (e.g. SASL mechanisms)

sasl_verifyfile_t
        Verify files for use by SASL

:saslman:`sasl_canon_user_t(3)`
        Username canonicalization function

Client-only Callbacks
---------------------

sasl_getsimple_t
       Get user/language list

sasl_getsecret_t
       Get authentication secret

:saslman:`sasl_chalprompt_t(3)`
       Display challenge and prompt for response

sasl_getrealm_t
       Get the realm for authentication

Server-only Callbacks
---------------------

:saslman:`sasl_authorize_t(3)`
        Authorize policy callback

sasl_server_userdb_checkpass_t
        verify plaintext password

sasl_server_userdb_setpass_t
        set plaintext password

sasl_getconfpath_t
        Get path to search  for  SASL  configuration  file
        (server side only). New in SASL 2.1.22.

Return value
============

SASL  callback  functions should return SASL return codes.
See :saslman:`sasl_errors(3)` for a complete list.  :c:macro:`SASL_OK`  typically  indicates success.

Conforming to
=============

:rfc:`4422`

See Also
========

:saslman:`sasl(3)`, :saslman:`sasl_errors(3)`, :saslman:`sasl_authorize_t(3)`,
:saslman:`sasl_log_t(3)`, :saslman:`sasl_getpath_t(3)`,
:saslman:`sasl_getconfpath_t(3)`, :saslman:`sasl_verifyfile_t(3)`,
:saslman:`sasl_canon_user_t(3)`,  :saslman:`sasl_getsimple_t(3)`,
:saslman:`sasl_getsecret_t(3)`, :saslman:`sasl_chalprompt_t(3)`,
:saslman:`sasl_getrealm_t(3)`, :saslman:`sasl_server_userdb_checkpass_t(3)`,
:saslman:`sasl_server_userdb_setpass_t(3)`
