.. saslman:: sasl_getrealm_t(3)

.. _sasl-reference-manpages-library-sasl_getrealm_t:


================================================
**sasl_getrealm_t** - Realm Acquisition Callback
================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    int sasl_getrealm_t(void *context,
        int id,
        const char **availrealms,
        const char **result)

Description
===========

.. c:function:: int sasl_getrealm_t(void *context,
        int id,
        const char **availrealms,
        const char **result)

    **sasl_getrealm_t()** is used when there is an interaction with
    SASL_CB_GETREALM as the type.

    If a mechanism would use this  callback,  but  it  is  not
    present,  then  the  first  realm  listed is automatically
    selected.  (Note that a  mechanism  may  still  force  the
    existence  of  a  getrealm callback by SASL_CB_GETREALM to
    its required_prompts list).

    :param context: context from the callback record

    :param id: callback ID (SASL_CB_GETREALM)

    :param availrealms: A string list of the available  realms.   NULL
        terminated, may be empty.

    :param result: The chosen realm. (a NUL terminated string)


Return Value
============

SASL  callback  functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

Other return codes indicate errors and should be handled.

See Also
========

:rfc:`4422`,:saslman:`sasl(3)`, :saslman:`sasl_callbacks(3)`
