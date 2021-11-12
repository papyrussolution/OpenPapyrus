.. saslman:: sasl_erasebuffer(3)

.. _sasl-reference-manpages-library-sasl_erasebuffer:

===================================
**sasl_erasebuffer** - erase buffer
===================================

Synopsis
========

.. code-block:: C

    #include <sasl/saslutil.h>

    void sasl_erasebuffer(char *pass, unsigned len);

Description
===========

.. c:function:: void sasl_erasebuffer(char *pass, unsigned len);

    **sasl_erasebuffer** erases a security sensitive buffer or
    password. The implementation may use recovery‐resistant
    erase logic.

    :param pass: a password

    :param len: length of the password

Return Value
============

The sasl_erasebuffer() interface returns no return values.
