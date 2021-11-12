.. _installation:

============
Installation
============

Are you looking for the:

* :ref:`Quick install <installation_quick>` guide, or
* :ref:`Detailed installation instructions for compiling from source <installation_detailed>`

.. _installation_quick:

Quick install guide
===================
You can install Cyrus SASL via packages or via tarball.

Tarball installation
--------------------

Fetch the latest Cyrus SASL tarball from
https://github.com/cyrusimap/cyrus-sasl/releases

Untar it then::

    cd (directory it was untarred into)
    ./configure
    make
    make install
    ln -s /usr/local/lib/sasl2 /usr/lib/sasl2


Contributors will want to `compile from source`_.

.. _compile from source: developer/installation.html

Unix package Installation
-------------------------

Are you `upgrading from Cyrus SASLv1`_?

Please see the file install.php for instructions on how to install this package.

Note that the library can use the environment variable SASL_PATH to locate the directory where the mechanisms are; this should be a colon-separated list of directories containing plugins. Otherwise it will default to the value of `--with-plugindir` as supplied to `configure` (which itself defaults to `/usr/local/lib`).

Extra information for :ref:`Mac OSX installation <install-macos>`.

Extra information for :ref:`Windows installation <install-windows>`. This configuration has not been extensively tested.

Configuration
-------------

There are two main ways to configure the SASL library for a given application. The first (and typically easiest) is to make use of the application's configuration files. Provided the application supports it (via the `SASL_CB_GETOPT` callback), please refer to that documentation for how to supply SASL options.

Alternatively, Cyrus SASL looks for configuration files in `/usr/lib/sasl/Appname.conf` where Appname is settable by the application (for example, Sendmail 8.10 and later set this to "Sendmail").

Configuration using the application's configuration files (via the getopt callback) will override those supplied by the SASL configuration files.

For a detailed guide on configuring libsasl, please look at sysadmin.php and options.php

.. _upgrading from Cyrus SASLv1:


.. _installation_detailed:

Detailed installation guide
===========================

Before reading this section, please be sure you are comfortable with
the concepts presented in the :ref:`components <components>` guide
and in the :ref:`Quickstart <quickstart>` guide.

You will want to have answered the following questions about your intended
installation:


1.  What mechanisms do you want to support?  Are they plaintext (LOGIN, PLAIN),
shared secret (SCRAM, DIGEST-MD5, CRAM-MD5), or Kerberos (KERBEROS_V4, GSSAPI)?
Perhaps you will use some combination (generally plaintext with one of
the other two types).
2.  Given the answer to the previous question, how will the mechanisms
perform user verification?

    * Kerberos mechanisms just need your existing Kerberos infrastructure.
    * The shared secret mechanisms will need an auxprop plugin backend.
    * The plaintext mechanisms can make do with saslauthd, Courier authdaemond (not included), *or* by using an auxprop plugin backend.
    * To use Kerberos and Plaintext, you'll want to use saslauthd with a kerberos module for plaintext authentication.  To use Shared Secret and plaintext, you'll want to use the auxprop plugin for password verification.

3.  If you are using an auxprop plugin, will you be using SASLdb (and
if so, Berkeley DB [recommended], GDBM, or NDBM?), LDAP or an SQL backend
(Postgres? MySQL?).
4.  If you are using saslauthd, what module will you be using?  LDAP?
Kerberos?  PAM?
5.  Also if you are using saslauthd, what communication (IPC) method do
you want to use?  On most systems, the correct answer is the default
(unix sockets), but on Solaris you can use IPC doors, which have proven
to be more stable than equivalent Solaris systems using unix sockets.

Once you have answered these questions, properly configuring a working
configuration of Cyrus SASL becomes easier.

Requirements
------------

1. You'll need the source from https://github.com/cyrusimap/cyrus-sasl

2. You'll need `GNU make <ftp://ftp.gnu.org/pub/gnu/make/>`_.

3. If you are using SASLdb, you will need to pick your backend.
   libsasl2 can use `gdbm <ftp://ftp.gnu.org/pub/gnu/gdbm/>`_, `Berkeley db <http://www.sleepycat.com/>`_, or ndbm to implement its user/password lookup. Most systems come with ndbm.

4. If you are using SQL, you'll need to properly configure your server/tables,
   and build the necessary client libraries on the system where you will be
   building and using SASL.  Currently we support `PostgreSQL <http://postgresql.org>`_ v7.2 (or higher)
   and `MySQL <http://mysql.org>`_.

5. If you are using LDAPDB, you'll need SASL enabled `OpenLDAP <http://www.openldap.org>`_ libraries.
   v2.1.27 (or higher) or v2.2.6 (or higher) is supported.

6. For Kerberos support, you'll need the `kerberos <http://www.pdc.kth.se/kth-krb/>`_ libraries.

7. For GSSAPI support you will need either `MIT Kerberos 5 <http://web.mit.edu/kerberos/www/>`_,
   the `Heimdal <http://www.pdc.kth.se/heimdal>`_ or `CyberSafe <http://www.cybersafe.com/>`_.

Build Configuration
-------------------

Once you have answered all the necessary questions and installed
(and tested!) any required packages for your configuration, you are
ready to build SASL.  Building SASL is done with the aid of
an autoconf ``configure`` script, which has a *lot* of options.
Be sure to read the output of ``configure --help`` to be sure you
aren't missing any.  Note that an ``--enable-foo`` option has a counterpart ``--disable-foo``
to not enable that feature.

Some of the most important configuration options are those which allow
you to turn off the compilation of modules you do not need.  This is often
the easiest way to solve compilation problems with Cyrus SASL.
If you're not going to need a particular mechanism, don't build it!  Not
building them can also add performance improvements as it does take system
resources to load a given plugin, even if that plugin is otherwise unused
(even when it is disabled via the :option:`mech_list` option).

As of this writing, modules that are enabled by default but may not
be applicable to all systems include CRAM-MD5, DIGEST-MD5, SCRAM, OTP,
KERBEROS_V4, GSSAPI, PLAIN, and ANONYMOUS.  These can be disabled with::

    ``--disable-cram``, ``--disable-digest``,
    ``--disable-scram``, ``--disable-otp``,
    ``--disable-krb4``, ``--disable-gssapi``,
    ``--disable-plain``, and ``--disable-anon`` respecively.

If you are using an SQL auxprop plugin, you may want to specify one or more
of ``--enable-sql``, ``--with-mysql=PATH``, and
``--with-pgsql=PATH``, note that PATH in the later two should be replaced
with the path where you installed the necessary client libraries.

If you are using LDAPDB auxprop plugin, you will need to specify
``--enable-ldapdb`` and ``--with-ldap=PATH``.  <b>Warning:</b> LDAPDB
auxprop plugin (and LDAP enabled saslauthd) introduces a circular dependency
between OpenLDAP and SASL.  I.e., you must have OpenLDAP already built when
building LDAPDB in SASL.  In order for LDAPDB to work at runtime, you must have
OpenLDAP already built with SASL support. One way to solve this issue is to
build Cyrus SASL first without ldap support, then build OpenLDAP, and then come
back to SASL and build LDAPDB.

Given the myriad of ways that Berkeley DB can be installed on a system,
people using it may want to look at the ``--with-bdb-libdir`` and
``--with-bdb-incdir`` as alternatives to ``--with-dbbase`` for
specifying the paths to the Berkeley DB Library and Include directories.

In fact, if you're not planning on using SASLdb at all, it may be worth
your time to disable its use entirely with the ``--with-dblib=none``
option.

If you are planning on using LDAP with saslauthd, be sure to specify
the ``--with-ldap=PATH`` option to ``configure``.

Building and Installation
-------------------------

After configure runs, you should be able to build SASL just by
running ``make``.  If this runs into problems, be sure that you
have disabled everything that your system doesn't need, and that you have
correctly specified paths to any dependencies you may have.

To install the library, run ``make install`` as ``root`` followed by
``ln -s /usr/local/lib/sasl2 /usr/lib/sasl2`` (modified for your
installation path as appropriate).  Be sure to do this last step or
SASL will not be able to locate your plugins!

Compilation Hints
-----------------

You may need to play with your CPPFLAGS and LDFLAGS  if you're
using vendor compilers. We use ``gcc`` extensively, but you'll
probably have more luck if you use the same compiler for the library
as you do for your applications. You can see what compilers we use on
our platforms by looking at the "SMakefile".

Application Configuration
-------------------------

Please read about the :ref:`SASL Options <options>` to learn what
needs to be configured so that applications can successfully use the SASL
library.

You will want to ensure that the settings of ``pwcheck_method``
and ``auxprop_plugin`` match the decisions you made about your
authentication infrastructure.  (For example, if you are using
saslauthd as a password verifier, you'll want to be sure to set
``pwcheck_method: saslauthd``).

If you are using saslauthd, you will want to arrange for
``saslauthd -a pam`` (or ldap, or kerberos4, etc) to be run
at boot.  If you are not going to be using saslauthd, then this is
not necessary.

Many of these pieces are covered in more detail in the
:ref:`SASL System Administrator's Guide <sysadmin>`.

Supported platforms
===================

This has been tested under Linux 2.2, Linux 2.4, Solaris 2.7 and
Solaris 2.8.  It should work under any platform where dynamic objects
can be linked against other dynamic objects, and where the dynamic
library file extension is ".so", or where libtool creates the .la
files correctly.  There is also documentation for
:ref:`Win32 <install-windows>`, :ref:`MacOS X <install-macos>`, and
:ref:`OS/390 <install-os390>`.

.. toctree::
    :hidden:

    developer/installation
    macosx.rst
    windows.rst
    os390.rst
