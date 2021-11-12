.. saslman:: sasl_set_alloc(3)

.. _sasl-reference-manpages-library-sasl_set_alloc:


=================================================================================
**sasl_set_alloc** - set the memory allocation functions used by the SASL library
=================================================================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    void sasl_set_alloc(sasl_malloc_t *m,
                        sasl_calloc_t *c,
                        sasl_realloc_t *r,
                        sasl_free_t *f);

Description
===========

.. c:function:: void sasl_set_alloc(sasl_malloc_t *m,
        sasl_calloc_t *c,
        sasl_realloc_t *r,
        sasl_free_t *f);

    Use the **sasl_set_alloc()** interface to set the memory allocation
    routines that the SASL library and plug‐ins will use.

    :param m: A pointer to a malloc() function.
    :param c: A pointer to a calloc() function.
    :param r: A pointer to a realloc() function.
    :param f: A pointer to a free() function.

Return Value
============

No return values.

See Also
========

:manpage:`malloc(3)`, :manpage:`calloc(3)`, :manpage:`realloc(3)`,
:manpage:`free(3)`.
