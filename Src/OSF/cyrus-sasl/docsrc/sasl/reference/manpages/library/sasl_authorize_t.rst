.. saslman:: sasl_authorize_t(3)

.. _sasl-reference-manpages-library-sasl_authorize_t:

======================================================
**sasl_authorize_t** - The SASL authorization callback
======================================================

Synopsis
========

.. parsed-literal::

    #include <sasl/sasl.h>


    int sasl_authorize_t(void \*context,
                        const char \*requested_user, unsigned alen,
                        const char \*auth_identity, unsigned alen,
                        const char \*def_realm, unsigned urlen,
                        struct propctx \*propctx)

Description
===========

**sasl_authorize_t**  is  used to check whether the authorized
user auth_identity may act  as  the  user  requested_user.
For  example  the  user root may wish to authenticate with
his credentials but act as the user mmercer (with  all  of
mmercer's  rights  not roots). A server application should
be very careful, and probably err on the side of  caution,
when determining which users may proxy as whom.


Return Value
============

SASL  callback  functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

See Also
========

:saslman:`sasl(3)`, :saslman:`sasl_callbacks(3)`
