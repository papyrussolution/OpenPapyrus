.. _appconvert:

=====================================
Converting Applications from v1 to v2
=====================================

This documents our conversion experience with Cyrus IMAPd, an application
that uses almost every part of SASL, so it should give a good idea what caveats
need to be looked for when one is converting an application which uses SASLv1
to use SASLv2.

The major changes in the SASLv2 API have to do with memory management.
That is, the rule "If you allocate it, you free it" is now enforced.  That
means that if the application allocates something (for example, an interaction
or callback response), it must free it.  Likewise, the application does
NOT free anything handed to it by the SASL library, such as responses
given by sasl_client_step or sasl_decode.


Tips for both clients and servers
=================================

* Change configure scripts to search for libsasl2 and include files
  prefixed with sasl/ (sasl/sasl.h, sasl/saslutil.h, etc)
* ``sasl_decode64`` now takes an
  additional parameter that is the size of the buffer it is passed.
* External authentication properties are no longer handled by a
  ``sasl_external_properties_t``.  Instead you make 2 separate calls to
  ``sasl_setprop.``

  * One with SASL_SSF_EXTERNAL to tell the SASL library what SSF is being
    provided by the external layer.
  * The other sets SASL_AUTH_EXTERNAL to indicate
    the authentication name.

* ``sasl_getprop`` now returns its value in a ``const void \*\*``

* ``sasl_encode`` and ``sasl_decode`` now return a constant output buffer, which
  you do not need to free (it is only valid until the next call for this sasl\_
  conn_t, however)

* The SASL_IP_REMOTE and SASL_IP_LOCAL properties are now SASL_IPLOCALPORT
  and SASL_IPREMOTEPORT and take strings instead of sockaddrs. These strings
  may also be passed to the sasl_[client/server]_new functions.  They
  are in one of the following formats:

  * a.b.c.d;p (IPv4, with port)
  * e:f:g:h:i:j:k:l;p (IPv6, with port)
  * e:j:k:l;p (IPv6, abbreviated zero fields, with port)

* Error handling and reporting is different. All of the functions that used
  to return a "reply" string no longer do.  Now you should (always) check
  ``sasl_errdetail``.  Callbacks MUST likewise use ``sasl_seterror``
  instead of setting their (now nonexistent) reply parameter.

* Be very careful about your handling of maxoutbuf.  If you claim that
  you can only read 4096 bytes at a time, be sure to only pass at most
  that much at a time to the SASL library!


Tips for clients
================

* In ``sasl_client_new`` you can now pass ip address strings as
  parameters 3 and 4 instead of calling setprop later on sockaddr's.
  This is preferred but not required (not passing them by either method disables
  mechs which require IP address information).   You might find the iptostring()
  function in utils/smtptest.c to be useful for this.  If the protocol supports
  the server sending data on success you should pass SASL_SUCCESS_DATA as a
  flag.
* ``sasl_client_start`` loses the 3rd "secret" parameter.
  Also, NULL clientout and clientoutlen indicates that the protocol does not
  support client-send-first.  A NULL return value indicates that there is no
  first client send. (as opposed to an empty string, which indicates that
  the first client send is the empty string).

* Both ``sasl_client_start`` and ``sasl_client_step`` now take
  const clientout parameters that you are no longer responsible for freeing
  (it is only valid until the next call for this ``sasl_conn_t``, however)

* When interactions and callbacks happen you are responsible for freeing
  the results.

Tips for Servers
================

* SASL_SECURITY_LAYER flag no longer exists, whether or not to use a
  security layer is solely determined by the security properties information,
  namely, the ``maxbufsize`` member of the
  ``sasl_security_properties_t``
* ``sasl_server_new`` now can take ip address strings.
* ``sasl_checkpass`` no longer has a "reply" parameter.  There
  are also considerably fewer possible values for the pwcheck_method
  option (now only auxprop, saslauthd, authdaemond, and pwcheck).
* ``sasl_server_start`` / ``sasl_server_step`` have same
  output parameter deal as their equivalents on the client side
* ``sasl_listmech`` has a constant output parameter
* If you used to canonicalize the username in a SASL_CB_PROXY_POLICY
  callback you should now separate the functionality of authorization and
  canonicalization.  That is, only do authorization in SASL_CB_PROXY_POLICY,
  and do canonicalization in the SASL_CB_CANON_USER callback
