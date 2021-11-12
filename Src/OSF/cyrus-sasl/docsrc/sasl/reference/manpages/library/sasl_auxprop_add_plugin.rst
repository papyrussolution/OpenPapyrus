.. saslman:: sasl_auxprop_add_plugin(3)

.. _sasl-reference-manpages-library-sasl_auxprop_add_plugin:

==================================================================
**sasl_auxprop_add_plugin** - add a SASL auxiliary property plugin
==================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/saslplug.h>

    int sasl_auxprop_add_plugin(const char *plugname,
                    sasl_auxprop_plug_init_t *cplugfunc);

Description
===========

.. c:function:: int sasl_auxprop_add_plugin(const char *plugname,
        sasl_auxprop_plug_init_t *cplugfunc);

    **sasl_auxprop_add_plugin** adds an auxiliary property plugin to the
    current list of auxiliary property plugins in the SASL library.

    :param plugname: is the name of the auxiliary property plugin.

    :param cplugfunc: is filled in by the sasl_auxprop_plug_init_t structure.

    :returns: Returns  :c:macro:`SASL_OK` on success. See :saslman:`sasl_errors(3)` for meanings of other return codes.

Return Value
============

SASL functions should return SASL return codes.
See sasl.h for a complete list. :c:macro:`SASL_OK` indicates success.

The following return codes indicate errors and should be handled:

* :c:macro:`SASL_BADVERS`: Version mismatch with plugin.
* :c:macro:`SASL_NOMEM`: Not enough memory to complete operation

Conforming to
=============

:rfc:`4422`

See Also
========

:saslman:`sasl(3)`, :saslman:`sasl_errors(3)`, :saslman:`sasl_auxprop(3)`,
