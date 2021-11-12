.. saslman:: sasl_set_mutex(3)

.. _sasl-reference-manpages-library-sasl_set_mutex:


==========================================================================
**sasl_set_mutex** - set the mutex lock functions used by the SASL library
==========================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    void sasl_set_mutex(sasl_mutex_alloc_t *a,
                        sasl_mutex_lock_t *l,
                        sasl_mutex_unlock_t *u,
                        sasl_mutex_free_t *f);

Description
===========

.. c:function::  void sasl_set_mutex(sasl_mutex_alloc_t *a,
        sasl_mutex_lock_t *l,
        sasl_mutex_unlock_t *u,
        sasl_mutex_free_t *f);

    Use the **sasl_set_mutex()** interface to set the mutex lock
    routines that the SASL library and plug‐ins will use.

    :param a: A pointer to the mutex lock allocation function.
    :param l: A pointer to the mutex lock function.
    :param u: A pointer to the mutex unlock function.
    :param f: A pointer to the mutex free or destroy function.

Return Value
============

No return values.
