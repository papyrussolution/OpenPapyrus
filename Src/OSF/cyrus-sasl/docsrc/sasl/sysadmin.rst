.. _sysadmin:

=====================
System Administrators
=====================

This document covers configuring SASL for system administrators,
specifically those administrators who are installing a server that
uses the Cyrus SASL library.  You may want to read
:ref:`about components <components>` which presents an
overview of the Cyrus SASL distribution
and describes how the components interact, as well as the :ref:`installation guide <installation>`

.. _saslintro:

What SASL is
============

SASL, the Simple Authentication and Security Layer, is a generic
mechanism for protocols to accomplish authentication.  Since protocols
(such as SMTP or IMAP) use SASL, it is a natural place for code
sharing between applications.  Some notable applications that use the
Cyrus SASL library include `Sendmail <http://www.sendmail.org>`_,
`Cyrus imapd <http://www.cyrusimap.org>`_,
and `OpenLDAP <http://www.openldap.org>`_.

Applications use the SASL library to tell them how to accomplish
the SASL protocol exchange, and what the results were.

SASL is only a framework: specific SASL mechanisms govern the
exact protocol exchange.  If there are n protocols and m different
ways of authenticating, SASL attempts to make it so only n plus m
different specifications need be written instead of n times m
different specifications.  With the Cyrus SASL library, the mechanisms
need only be written once, and they'll work with all servers that use
it.

.. _authid:

Authentication and authorization identifiers
--------------------------------------------

An important concept to become familiar with is the difference between
an "authorization identifier" and an "authentication identifier".

userid (user id, authorization id)
    The userid is the
    identifier an application uses to check allowable options.  On my Unix
    system, the user ``bovik`` (the account of Harry Q. Bovik) is
    allowed to write to ``/home/bovik`` and its subdirectories but
    not to ``/etc``.
authid (authentication id)
    The authentication identifier is
    the identifier that is being checked.  "bovik"'s password might be
    "qqqq", and the system will authenticate anyone who knows "qqqq" as
    "bovik".

    However, it's possible to authenticate as one user but
    *act as* another user.  For instance, Harry might be away on
    vacation and assign one of his graduate students, Jane, to read his
    mail.  He might then allow Jane to act as him merely by supplying her
    password and her id as authentication but requesting authorization as
    "bovik". So Jane might log in with an authentication identifier of
    "jane" and an authorization id of "bovik" and her own (Jane's)
    password.


Applications can set their own proxy policies; by default, the SASL
library will only allow the same user to act for another (that is,
userid must equal authid).  See your application's documentation for
details about changing the default proxy/authorization policies.

.. _realms:

Realms
------

The Cyrus SASL library supports the concept of "realms".  A realm is
an abstract set of users and certain mechanisms authenticate users in
a certain realm.

In the simplest case, a single server on a single machine, the
realm might be the fully-qualified domain name of the server.  If the
applications don't specify a realm to SASL, most mechanisms will
default to this.

If a site wishes to share passwords between multiple machines, it
might choose it's authentication realm as a domain name, such as
"CMU.EDU".  On the other hand, in order to prevent the entire site's
security from being compromised when one machine is compromised, each
server could have it's own realm. Certain mechanisms force the user
(client side) to manually configure what realm they're in, making it
harder for users to authenticate.

A single site might support multiple different realms.  This can
confuse applications that weren't written in anticipation of this; make
sure your application can support it before adding users from different
realms into your databases.

To add users of different realms to sasldb, you can use the
``-u`` option to saslpasswd2.  The SQL plugin has a way of
integrating the realm name into the query string with the '%r' macro.

The Kerberos mechanisms treat the SASL realm as the Kerberos
realm.  Thus, the realm for Kerberos mechanisms defaults to the
default Kerberos realm on the server.  They may support cross-realm
authentication; check your application on how it deals with this.

Realms will be passed to saslauthd as part of the saslauthd protocol,
however the way each saslauthd module deals with the situation is
different (for example, the LDAP plugin allows you to use the realm
to query the server, while the rimap and PAM plugins ignore it entirely).

Realms are represented in a username string by any text followinhg
the '@' sign.  So, usernames like rjs3@ANDREW.CMU.EDU, is user 'rjs3'
in the realm 'ANDREW.CMU.EDU'.  If no realm is provided, the server's
FQDN is assumed (likewise when specifying a realm for saslpasswd2).

.. _saslhow:

How SASL works
==============

How SASL works is governed by what mechanism the client and server
choose to use and the exact implementation of that mechanism.  This
section describes the way these mechanisms act in the Cyrus SASL
implementation.

The PLAIN mechanism, ``sasl_checkpass()``, and plaintext passwords
------------------------------------------------------------------

The PLAIN mechanism is not a secure method of authentication by
itself.  It is intended for connections that are being encrypted by
another level.  (For example, the IMAP command "STARTTLS" creates an
encrypted connection over which PLAIN might be used.) The PLAIN
mechanism works by transmitting a userid, an authentication id, and a
password to the server, and the server then determines whether that is
an allowable triple.

The principal concern for system administrators is how the
authentication identifier and password are verified.  The Cyrus SASL
library is flexible in this regard:

auxprop
    checks passwords agains the ``userPassword`` attribute
    supplied by an auxiliary property plugin.  For example, SASL ships
    with a sasldb auxiliary property plugin, that can be used to
    authenticate against the passwords stored in ``/etc/sasldb2``.

    Since other mechanisms also use this database for passwords, using
    this method will allow SASL to provide a uniform password database to
    a large number of mechanisms.
saslauthd
    contacts the ``saslauthd`` daemon to to check passwords
    using a variety of mechanisms.  More information about the various invocations
    of saslauthd can be can be found in ``saslauthd(8)``.  Generally you
    want something like ``saslauthd -a pam``.  If plaintext authentications
    seem to be taking some time under load, increasing the value of the ``-n``
    parameter can help.

    Saslauthd keeps its named socket in "/var/state/saslauthd" by default.
    This can be overridden by specifying an alternate value to
    --with-saslauthd=/foo/bar at compile time, or by passing the -m
    parameter to saslauthd (along with setting the saslauthd_path SASL
    option).  Whatever directory this is, it must exist in order for
    saslauthd to function.

    Once you configure (and start) ``saslauthd``, there is a
    ``testsaslauthd`` program that can be built with ``make
    testsaslauthd`` in the ``saslauthd`` subdirectory of the
    source.  This can be used to check that that the ``saslauthd``
    daemon is installed and running properly.  An invocation like
    ``testsaslauthd -u rjs3 -p 1234`` with appropriate values for the
    username and password should do the trick.

    If you are using the PAM method to verify passwords with saslauthd, keep in
    mind that your PAM configuration will need to be configured for each service
    name that is using saslauthd for authentication. Common service names
    are ``imap``, ``sieve``, and ``smtp``.
Courier-IMAP authdaemond
    contacts Courier-IMAP's ``authdaemond`` daemon to check passwords.
    This daemon is simliar in functionality to ``saslauthd``, and is shipped
    separately with the `Courier <http://www.courier-mta.org>`_ mail server.

    Note: this feature is **not** compiled in the library by default, and is
    provided for sites with custom/special requirements only (because the
    internal authentication protocol its not documented anywhere so it could
    change at any time).  We have tested against the authdaemond included with
    Courier-IMAP 2.2.1.

    To enable ``authdaemond`` support, pass ``--with-authdaemon`` to the
    configuration script, set pwcheck_method to ``authdaemond'' and point
    authdaemond_path to ``authdaemond``'s unix socket. Optionally, you can
    specify --with-authdaemond=PATH to the configure script so that
    authdaemond_path points to a default, static, location.
pwcheck
    checks passwords with the use of a separate,
    helper daemon.  This feature is for backwards-compatibility
    only. New installations should use saslauthd.
write your own
    Last, but not least, the most flexible method of authentication
    for PLAIN is to write your own.  If you do so, any application that
    calls the ``sasl_checkpass()`` routine or uses PLAIN will
    invoke your code.  The easiest place to modify the plaintext
    authentication routines is to modify the routine
    ``_sasl_checkpass()`` in the file ``lib/server.c`` to
    support a new method, and to add that method to
    ``lib/checkpw.c``.  Be sure to add a prototype in
    ``lib/saslint.h``!

    However, the more flexible and preferred method of
    adding a routine is to create a new saslauthd mechanism.

The LOGIN mechanism (not to be confused with IMAP4's LOGIN command)
is an undocumented, unsupported mechanism.  It's included in the Cyrus
SASL distribution for the sake of SMTP servers that might want to
interoperate with old clients.  Do not enable this mechanism unless
you know you're going to need it.  When enabled, it verifies passwords
the same way the PLAIN mechanism does.

Shared secrets mechanisms
-------------------------

The Cyrus SASL library also supports some "shared secret"
authentication methods: CRAM-MD5, DIGEST-MD5 and its successor SCRAM.
These methods rely on the client and the server sharing a "secret",
usually a password.  The server generates a challenge and the client a
response proving that it knows the shared secret.  This is much more
secure than simply sending the secret over the wire proving that the
client knows it.

There's a downside: in order to verify such responses, the
server must keep passwords or password equivalents in a database;
if this database is compromised, it is the same as if all the
passwords for the realm are compromised.

Put another way, *you cannot use saslauthd with these methods*.
If you do not wish to advertise these methods for that reason (i.e. you
are only using saslauthd for password verification), then either remove
the non-plaintext plugins (those other than login and plain) from the
plugin directory, or use the :option:`mech_list` option to disable them.

For simplicity sake, the Cyrus SASL library stores plaintext
passwords only in the ``/etc/sasldb2`` database.  These passwords
are then shared among all mechanisms which choose to use it.
Depending on the exact database method
used (gdbm, ndbm, or db) the file may have different suffixes or may
even have two different files (``sasldb.dir`` and
``sasldb.pag``).  It is also possible for a server to define
it's own way of storing authentication secrets.  Currently, no
application is known to do this.

The principle problem for a system administrator is to make sure that
sasldb is properly protected. Only the servers that need to read it to
verify passwords should be able to.  If there are any normal shell
users on the system, they must not be able to read it.

This point is important, so we will repeat it: **sasldb stores the
plaintext versions of all of its passwords. If it is compromised so
are all of the passwords that it stores**.

Managing password changes is outside the scope of the library.
However, system administrators should probably make a way of letting
user's change their passwords available to users.  The
``saslpasswd2`` utility is provided to change the secrets in
sasldb.  It does not affect PAM, ``/etc/passwd``, or any other
standard system library; it only affects secrets stored in sasldb.

Finally, system administrators should think if they want to enable
"auto_transition".  If set, the library will automatically create
secrets in sasldb when a user uses PLAIN to successfully authenticate.
However, this means that the individual servers, such as imapd, need
read/write access to sasldb, not just read access.  By default,
"auto_transition" is set to false; set it to true to enable.  (There's
no point in enabling this option if "pwcheck_method" is "auxprop",
and the sasldb plugin is installed, since you'll be transitioning from
a plaintext store to a plaintext store)

Kerberos mechanisms
-------------------

The Cyrus SASL library also comes with two mechanisms that make use of
Kerberos: KERBEROS_V4, which should be able to use any Kerberos v4
implementation, and GSSAPI (tested against MIT Kerberos 5, Heimdal
Kerberos 5 and CyberSafe Kerberos 5).  These mechanisms make use of the kerberos infrastructure
and thus have no password database.

Applications that wish to use a kerberos mechanism will need access
to a service key, stored either in a :option:`srvtab` file (Kerberos 4) or a
:option:`keytab` file (Kerberos 5).

The Kerberos 4 :option:`srvtab` file location is configurable; by default it is
``/etc/srvtab``, but this is modifiable by the "srvtab" option.
Different SASL applications can use different srvtab files.

A SASL application must be able to read its srvtab or keytab file.

You may want to consult the <a href="gssapi.html">GSSAPI Tutorial</a>.</p>

The OTP mechanism
-----------------

The Cyrus SASL library also supports the One-Time-Password (OTP)
mechanism.  This mechanism is similar to CRAM-MD5, DIGEST-MD5, SCRAM
and SRP in that is uses a shared secret and a challenge/response exchange.
However, OTP is more secure than the other shared secret mechanisms in
that the secret is used to generate a sequence of one-time (single
use) passwords which prevents reply attacks, and that secret need
not be stored on the system.  These one-time passwords are stored in the
``/etc/sasldb2`` database.

OTP via OPIE
############

For sites with an existing OTP infrastructure using OPIE, Cyrus SASL
can be configured to use OPIE v2.4 instead of using its own database
and server-side routines.

OPIE should be configured with the ``--disable-user-locking``
option if the SASL server application will not be running as "root".

OPIE uses its own "opiekeys" database for storing the data necessary
for generating the server challenges.  The location of the :option:`opiekeys`
file is configurable in SASL; by default it is ``/etc/opiekeys``,
but this is modifiable by the :option:`opiekeys` option.

A SASL server application must be able to read and write the
opiekeys file.

Auxiliary Properties
====================

SASLv2 introduces the concept of Auxilliary Properties.  That is, the ability
for information related to authentication and authorization to all be looked
up at once from a directory during the authentication process.  SASL Plugins
internally take advantage of this to do password lookups in directories
such as the SASLdb, LDAP or a SQL database.  Applications can look up arbitrary properties through them.

Note that this means that if your password database is in a SASLdb, and
you wish to use it for plaintext password lookups through the sasldb, you
will need to set the sasl :option:`pwcheck_method` to be ``auxprop``.

How to set configuration options
================================

The Cyrus SASL library comes with a built-in configuration file
reader.  However, it is also possible for applications to redefine
where the library gets it's configuration options from.

.. _saslconf:

The default configuration file
------------------------------

By default, the Cyrus SASL library reads its options from
``/usr/lib/sasl2/App.conf`` (where "App" is the application
defined name of the application).  For instance, Sendmail reads its
configuration from ``/usr/lib/sasl2/Sendmail.conf`` and the
sample server application included with the library looks in
``/usr/lib/sasl2/sample.conf``.

A standard Cyrus SASL configuration file looks like::

    srvtab: /var/app/srvtab
    pwcheck_method: saslauthd

Application configuration
-------------------------

Applications can redefine how the SASL library looks for configuration
information.  Check your application's documentation for specifics.

For instance, Cyrus imapd reads its sasl options from its own
configuration file, ``/etc/imapd.conf``, by prepending all SASL
options with ``sasl_``: the SASL option "pwcheck_method" is set
by changing "sasl_pwcheck_method" in ``/etc/imapd.conf``.

Troubleshooting
===============

Why doesn't KERBEROS_V4 doesn't appear as an available mechanism?
    Check that the ``srvtab`` file is readable by the
    user running as the daemon.  For Cyrus imapd, it must be readable by
    the Cyrus user.  By default, the library looks for the srvtab in
    ``/etc/srvtab``, but it's configurable using the :option:`srvtab`
    option.
Why doesn't OTP doesn't appear as an available mechanism?
    If using OPIE, check that the ``opiekeys`` file is
    readable by the user running the daemon.  For Cyrus imapd, it must
    be readable by the Cyrus user.  By default, the library looks for the
    opiekeys in ``/etc/opiekeys``, but it's configurable using the
    :option:`opiekeys` option.
Why don't CRAM-MD5, DIGEST-MD5 and SCRAM work with my old sasldb?
    Because sasldb now stores plaintext passwords only, the old
    sasldb is incompatible.
I'm having performance problems on each authentication, there is a noticeable slowdown when sasl initializes, what can I do?
    libsasl reads from ``/dev/random`` as part of its
    initialization. ``/dev/random`` is a "secure" source of entropy,
    and will block your application until a sufficient amount of
    randomness has been collected to meet libsasl's needs.

    To improve performance, you can change DEV_RANDOM in
    ``config.h`` to be ``/dev/urandom`` and recompile
    libsasl. ``/dev/urandom`` offers less secure random numbers but
    should return immediately. The included mechanisms, besides OTP and
    SRP, use random numbers only to generate nonces, so using
    ``/dev/urandom`` is safe if you aren't using OTP or SRP.


I've converted the sasldb database to the new format. Why can't anybody authenticate?
    sasldb is now a plugin module for the auxprop method.
    Make sure you changed the /usr/lib/sasl2/\*.conf files to reflect
    ``pwcheck_method: auxprop``

    ...and if you're using cyrus-imapd, /etc/imapd.conf must reflect:
    ``sasl_pwcheck_method: auxprop``

Is LOGIN supported?
    The LOGIN mechanism is a non-standard, undocumented
    plaintext mechanism.  It's included in the SASL distribution purely
    for sites that need it to interoperate with old clients; we don't
    support it.  Don't enable it unless you know you need it.

Is NTLM supported?
    The NTLM mechanism is a non-standard, undocumented
    mechanism developed by Microsoft.  It's included in the SASL
    distribution purely for sites that need it to interoperate with
    Microsoft clients (ie, Outlook) and/or servers (ie, Exchange); we
    don't support it.  Don't enable it unless you know you need it.

How can I get a non-root application to check plaintext passwords?
    Use the "saslauthd" daemon and setting "pwcheck_method"
    to "saslauthd".

I want to use Berkeley DB, but it's installed in ``/usr/local/BerkeleyDB.3.1`` and ``configure`` can't find it.
    Try setting "CPPFLAGS" and "LDFLAGS" environment
    variables before running ``configure``, like so::

        env CPPFLAGS="-I/usr/local/BerkeleyDB.3.1/include" \
          LDFLAGS="-L/usr/local/BerkeleyDB.3.1/lib -R/usr/local/BerkeleyDB.3.1/lib" \
          ./configure --with-dblib=berkeley

It's not working and won't tell me why! Help!
    Check syslog output (usually stored in
    ``/var/log``) for more information. You might want to change your
    syslog configuration (usually ``/etc/syslogd.conf``) to log
    "\*.debug" to a file while debugging a problem.

    The developers make heavy use of ``strace`` or ``truss``
    when debugging a problem that isn't outputting any useful
    information.

Is there a mailing list to discuss the Cyrus SASL library?
    Check out our :ref:`contribution <contribute>` page for ways to get in touch
    with us, including mailing lists and IRC.
