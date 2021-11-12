.. saslman:: sasl_auxprop(3)

.. _sasl-reference-manpages-library-sasl_auxprop:

=============================================================
**sasl_auxprop** - How to work with SASL auxiliary properties
=============================================================

Synopsis
========

.. code-block:: C

    #include <sasl/prop.h>

    struct propctx *prop_new(unsigned estimate)

    int prop_dup(struct propctx *src_ctx,
                 struct propctx *dst_ctx)

    int prop_request(struct propctx *ctx,
                     const char **names)

    const struct propval *prop_get(struct propctx *ctx)

    int prop_getnames(struct propctx *ctx, const char **names,
                      struct propval *vals)

    void prop_clear(struct propctx *ctx, int requests)

    void prop_erase(struct propctx *ctx, const char *name)

    void prop_dispose(struct propctx **ctx)

    int prop_format(struct propctx *ctx, const char *sep, int seplen,
                    char *outbuf, unsigned outmax, unsigned *outlen)

    int prop_set(struct propctx *ctx, const char *name,
                 const char *value, int vallen)

    int prop_setvals(struct propctx *ctx, const char *name,
                     const char **values)

Description
===========

SASL auxiliary properties are used to obtain properties
from external sources during the authentication process.
For example,  a mechanism might need to query an LDAP
server to obtain the authentication secret.  The application probably needs other information from there as well,
such as home directory or UID.   The auxiliary property
interface allows the two to cooperate, and only results in
a single query against the LDAP server (or other property
sources).

Property lookups take place directly after user canonicalization occurs.  Therefore, all requests should be
registered with the context before that time.   Note that
requests can also be registered using the
:saslman:`sasl_auxprop_request(3)`  function.   Most of the functions listed
below, however, require a property context which can be
obtained by calling :saslman:`sasl_auxprop_getctx(3)`.

API description
===============

.. c:function:: struct propctx *prop_new(unsigned estimate)

    Create a new property context.  Probably unnecessary for application developers
    to call this at any point.

    :parameter estimate:  is the estimate of storage needed in total for requests & responses.  A value of 0 implies the library default.
    :type extimate: unsigned
    :return: a new property context: :c:type:`propctx`

.. c:function:: int prop_dup(struct propctx *src_ctx, struct propctx *dst_ctx)

    Duplicate a given property context.

    :parameter src_ctx: Property context to copy.
    :type src_ctx: :c:type:`propctx`
    :parameter dst_ctx: Destination context to copy into.
    :type dst_ctx: :c:type:`propctx`
    :return: SASL error code.

.. c:function:: int prop_request(struct propctx *ctx, const char **names)

    Add properties to the request list of a given context.

    :param ctx: The property context to add add the request list to.
    :type ctx: :c:type:`propctx`
    :param names: is the NULL-terminated array of property names,  and must persist until the requests are cleared or the context is disposed of with a call to :c:func:`prop_dispose`.
    :return: SASL error code

.. c:function:: const struct propval *prop_get(struct propctx *ctx)

    Fetch out the property values from a context.

    :param ctx: The property context to fetch from.
    :type ctx: :c:type:`propctx`
    :return: a NULL-terminated array of property values from the given context.

.. c:function:: int prop_getnames(struct propctx *ctx, const char **names, struct propval *vals)

    Fill in a (provided) array of property values based
    on a list of property names.  This implies that
    the ``vals`` array is at least as long as the  ``names``
    array.  The values that are filled in by this call
    persist   until   next   call   to   :c:func:`prop_request`,
    :c:func:`prop_clear`, or :c:func:`prop_dispose` on context.  If a name
    specified here was never requested, then its associated
    values entry will be set to NULL.

    :param ctx: The property context to fill in.
    :type ctx: :c:type:`propctx`
    :returns: number of matching properties that were found, or a SASL error code.

.. c:function:: void prop_clear(struct propctx *ctx, int requests)

    Clear values and (optionally) requests from a property context.

    :param ctx: The property context to clear.
    :type ctx: :c:type:`propctx`
    :param requests: set to 1 if the requests should be cleared, 0 otherwise.

.. c:function:: void prop_erase(struct propctx *ctx, const char *name)

    Securely erase the value of a property from a context.

    :param ctx: The property context to find the property in.
    :type ctx: :c:type:`propctx`
    :param name: is the name of the property to erase.

.. c:function:: void prop_dispose(struct propctx **ctx)

    Disposes of a property context and NULLifys the pointer.

    :param ctx: The property context to clear.
    :type ctx: :c:type:`propctx`

.. c:function:: int prop_format(struct propctx *ctx, const char *sep, int seplen, char *outbuf,  unsigned outmax, unsigned *outlen)

    Format the requested property names into a string.
    This not intended for use by the application (*only
    by auxprop plugins*).

    :param ctx: The property context to extract values from.
    :type ctx: :c:type:`propctx`
    :param sep: the separator to use for the string
    :param outbuf: destination string. Caller must allocate the buffer of length ``outmax`` (including NUL terminator).
    :param outlen: if non-NULL, will contain the length of the resulting string (excluding NUL terminator).
    :returns: SASL error code.

.. c:function:: int prop_set(struct propctx *ctx, const char *name, const char *value, int vallen)

    Adds a property value to the context.  *This is intended for use by auxprop plugins only.*

    :param ctx: The property context to add a value to.
    :type ctx: :c:type:`propctx`
    :param name: the name of the property to receive the new value,  or NULL, which implies that the value will be added to the same property as the last call to either :c:func:`prop_set` or :c:func:`prop_setvals`.
    :param value: the new value (of length `vallen`)
    :param vallen: the length of the string `value`.
    :returns: SASL error code


.. c:function:: int prop_setvals(struct propctx *ctx, const char *name, const char **values)

    Adds multiple values to a single property.  *This is intended for use by auxprop plugins only*.

    :param ctx: The property context to add values to.
    :type ctx: :c:type:`propctx`
    :param name: The name of the property to receive the new value, or NULL, which implies that the values will be added to the same property as the last call to either :c:func:`prop_set` or :c:func:`prop_setvals`.
    :param values: A NULL-terminated array of values to be added the property.
    :returns: SASL error code

Data structures
===============

.. c:type:: propval

    A struct holding a name and its property values. A name can have zero or more values.

    :param name: ``const char *``. Name of this propval. NULL means end of list.
    :param values: ``const char **``. List of string values. If property not found, values == NULL. If property found with no values, then \*values == NULL

.. c:type:: propctx

    A property context.

    :param values: List of property values in this context.
    :type values: :c:type:`propval` *

Return Value
============

The property functions that return an int return SASL error codes.   See  :saslman:`sasl_errors(3)`.   Those that return
pointers will return a valid pointer on success, or NULL on any error.

Conforming to
=============

:rfc:`4422`

See Also
========

:saslman:`sasl(3)`, :saslman:`sasl_errors(3)`,
:saslman:`sasl_auxprop_request(3)`, :saslman:`sasl_auxprop_getctx(3)`
