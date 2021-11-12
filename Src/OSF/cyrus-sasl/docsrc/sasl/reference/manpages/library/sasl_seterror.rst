.. saslman:: sasl_seterror(3)

.. _sasl-reference-manpages-library-sasl_seterror:


========================================
**sasl_seterror** - set the error string
========================================

Synopsis
========

.. code-block:: C

    #include <sasl/sasl.h>

    void sasl_seterror(sasl_conn_t *conn,
                       unsigned flags,
                       const char *fmt,
                       ...);

Description
===========

.. c:function::  void sasl_seterror(sasl_conn_t *conn,
        unsigned flags,
        const char *fmt,
        ...);

    The  **sasl_seterror()**  interface sets the error string that
    will be returned by :saslman:`sasl_errdetail(3)`. Use
    :manpage:`syslog(3)` style formatting; that is, use printf()—style with
    %m as the most recent errno error.

    The sasl_seterror() interface is primarily used by server
    callback functions and internal plugins, for example,
    with the :saslman:`sasl_authorize_t(3)`  callback.  The sasl_seterror()
    interface triggers a call to the SASL logging callback, if
    any, with a level of `SASL_LOG_FAIL`, unless the  `SASL_NOLOG`
    flag is set.

    Make the message string sensitive to the current language
    setting. If there is no SASL_CB_LANGUAGE callback, message
    strings must be i‐default. Otherwise, UTF‐8 is used. Use
    of :rfc:`2482` for mixed‐language text is encouraged.

    If the value of conn is NULL, the sasl_seterror() interface fails.

    :param conn: The sasl_conn_t for which the call to sasl_seterror() applies.
    :param flags: If set to SASL_NOLOG, the call to sasl_seterror() is not logged.
    :param fmt: A :manpage:`syslog(3)` style format string.

Return Value
============

No return values.

See also
========

:saslman:`sasl_errdetail(3)`, :manpage:`syslog(3)`, :rfc:`2482`
