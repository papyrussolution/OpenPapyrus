.. _quickstart:

Quickstart guide
================

This document offers a general overview of the Cyrus SASL library.
The Cyrus SASL Libray provides applications with an implementation
of the Simple Authentication and Security Layer (RFC2222), and
several authentication mechanisms.  Users interested in the "big picture"
of what is provided by the library should read about
:ref:`Cyrus SASL Components <components>`.

Features
--------

The following :ref:`authentication_mechanisms` are included in
this distribution:

*  ANONYMOUS
*  CRAM-MD5
*  DIGEST-MD5 (requires OpenSSL libcrypto)
*  EXTERNAL
*  GSSAPI (MIT Kerberos 5, Heimdal Kerberos 5 or CyberSafe)
*  KERBEROS_V4 (requires OpenSSL libcrypto)
*  LOGIN
*  NTLM (requires OpenSSL libcrypto)
*  OTP (requires OpenSSL libcrypto)
*  PASSDSS (requires OpenSSL libcrypto)
*  PLAIN
*  SCRAM (requires OpenSSL libcrypto)
*  SRP (requires OpenSSL libcrypto)


The library also supports storing user secrets in either a hash
database (e.g. Berkeley DB, gdbm, ndbm), LDAP, or in a SQL database
(MySQL, Postgres).


Additionally, mechanisms such as PLAIN and LOGIN
(where the plaintext password is directly supplied by the client)
can perform direct password verification via the saslauthd daemon.  This
allows the use of LDAP, PAM, and a variety of other password verification
routines.

The sample directory in the code contains two programs which provide a reference
for using the library, as well as making it easy to test a mechanism
on the command line.  See <a
href="programming.html">programming.html</a> for more information.

This library is believed to be thread safe **if**:

*  you supply mutex functions (see sasl_set_mutex())
*  you make no libsasl calls until sasl_client/server_init() completes
*  no libsasl calls are made after sasl_done() is begun
*  when using GSSAPI, you use a thread-safe GSS / Kerberos 5 library.


Typical Installation
--------------------

If you are upgrading from Cyrus SASLv1, please see :ref:`upgrade guide <upgrading-v1-v2>`.

Please see the :ref:`installation guide <installation>` for instructions
on how to install this package.

Note that the library can use the environment variable SASL_PATH to locate the
directory where the mechanisms are; this should be a colon-separated
list of directories containing plugins.  Otherwise it will default to the
value of ``--with-plugindir`` as supplied to ``configure`` (which
itself defaults to ``/usr/local/lib``).

Looking to :ref:`Install on Mac OSX? <install-macos>`

Looking to :ref:`Install on Windows? <install-windows>`

Configuration
-------------

There are two main ways to configure the SASL library for a given
application.  The first (and typically easiest) is to make use
of the application's configuration files.  Provided the application supports it
(via the ``SASL_CB_GETOPT`` callback), please refer to that documetation
for how to supply <a href=options.html>SASL options</a>.

Alternatively, Cyrus SASL looks for configuration files in
/usr/lib/sasl/Appname.conf where Appname is settable by the
application (for example, Sendmail 8.10 and later set this to
"Sendmail").

Configuration using the application's configuration files (via
the ``getopt`` callback) will override those supplied by
the SASL configuration files.

For a detailed guide on configuring libsasl, please look at the
:ref:`Sysadmin guide <sysadmin>` and the information on :ref:`options <options>`.
