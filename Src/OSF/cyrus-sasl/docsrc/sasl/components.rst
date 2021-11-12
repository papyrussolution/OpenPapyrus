.. _components:

==========
Components
==========

As the SASL library is a 'glue layer' between many different parts of the
authentication system, there are a lot of different components
that often cause confusion to users of the library who are trying to
configure it for use on their system.  This document will try to provide
some structure to all of these components, though you will also need
to read the :ref:`System Administration <sysadmin>` to have a full
understanding of how to install SASL on your system.

The first thing to realize is that there is a difference between SASL,
the protocol, and Cyrus SASL, the library.  The first is a specification
that describes how authentication mechanisms can be plugged into an application
protocol *on the wire*.  The later is an implementation that aims
to make this easier for application developers to integrate authentication
mechanisms into their application in a generic way.  It is quite possible
to have an application that uses SASL (the specification) without using
Cyrus SASL (the implementation).

The remainder of this document will refer to components of the Cyrus
SASL implementation, though some of these will necessarily have a broader
scope.

The Application
===============

The application is a client of the SASL library.  It can be a client or server
application (or both, in the case of a proxy).  It takes care of the
on-the-wire representation of the SASL negotiation, however it performs no
analysis of the exchange itself.  It relies on the judgment of the SASL
library whether authentication has occurred or not.  The application is also
responsible for determining if the authenticated user may authorize as another
user id (For more details on authentication and authorization identities
and their differences, see <a href=sysadmin.html>Cyrus SASL for System Administrators</a>)

Examples of applications are Cyrus IMAPd, OpenLDAP, Sendmail, Mutt,
sieveshell, cyradm, and many others.

The SASL Glue Layer
===================

The first component of the SASL library is affectionately called the
``glue`` layer.  It takes care of ensuring that the application and
the mechanisms can work together successfully.  To this end, it does a
variety of basic tasks:

* Loading of any plugins (more on these below)
* Ascertaining necessary security properties from the application to aid
  in the choice of mechanism (or to limit the available mechanisms)
* Listing of available plugins to the application (mostly used on the server
  side)
* Choosing the best mechanism from a list of available mechanisms
  for a particular authentication attempt (client-side)
* Routing the authentication (and in the case of a mechanism with a security
  layer, encrypted) data packets between the application and the
  chosen mechanism.
* Providing information about the SASL negotiation back to the application
  (authenticated user, requested authorization identity, security strength of
  any negotiated security layer, and so on).


The Cyrus SASL implementation also provides several other services to
both its plugins and applications.  Some of these are simply general utilities,
such as MIME Base-64 encoding and decoding, and random number generation.
Others are more specific to the task of authentication, such as providing
password verification services.  Such services are capable of taking
a username and a plaintext password and saying ``yes`` or
``no``.  Details of available password verification services are
discussed below.

Finally, the glue code allows the mechanisms and applications access to
two special types of plugins, Auxiliary Property or ``auxprop``
plugins, which provide a simple database interface and can return properties
about the user such as password, home directory, or mail
routing address, and Username Canonicalization, which might provide
site-specific ways to canonicalize a username or perform other tasks.

In the Cyrus SASL Implementation, the glue code is entirely contained
within ``libsasl2.so`` (or ``libsasl2.a``)

Plugins
=======

Plugins: General
----------------

The Cyrus SASL architechure is very modular, using loadable modules for
things such as the mechanism profiles and the database access done by the
auxillary property plugins.  This means that it is easy to limit what
parts are loaded by a given application, and that third parties can write
their own modules to provide services, just by adhering to the API description
in ``saslplug.h``.

Plugins: SASL Mechanisms
------------------------

The simplest types of plugins to understand are those which provide
SASL mechanisms, such as CRAM-MD5, DIGEST-MD5, GSSAPI, PLAIN, SCRAM, SRP, and so on.
These mechanisms take care of both server-side and client-side parts
of the SASL negotiation.  If the given mechanism supports a security layer
(that is, makes guarantees about privacy or integrity of data after the
negotiation is complete), the plugin provides that functionality as well.

SASL mechanisms are generally defined by the IETF standards process,
however, some mechanisms are not (For example, NTLM).  This is in contrast
to the other types of plugins, which provide database and username
canonicalization services to other plugins and thus aren't standardized in
their behavior (they are specific to our implementation).  Password verifiers
are also an implementation detail (though saslauthd makes use of
standards such as PAM and LDAP to perform that verification)

There are several types of mechanisms, in broad strokes we have:

Password Verification Mechanisms
    For example, PLAIN.
    These receive a raw password from the remote and then pass it into the glue code for
    verification by a password verifier.  These require the existence of an
    outside security layer to hide the otherwise plaintext password from people
    who might be snooping on the wire.  These mechanisms do not require that
    the server have access to a plaintext (or plaintext-equivalent) version
    of the password.
Shared Secret Mechanisms
    For these mechanisms,
    such as CRAM-MD5, DIGEST-MD5, OTP, SCRAM, and SRP,
    there is a shared secret between the server and client (e.g. a password).
    However, in this case the password itself does not travel on the wire.
    Instead, the client passes a server a token that proves that it knows
    the secret (without actually sending the secret across the wire).
    For these mechanisms, the server generally needs a plaintext equivalent of
    the secret to be in local storage (not true for SRP).
Kerberos Mechanisms
    Kerberos mechanisms use a trusted
    third party to authenticate the client.  These mechanisms don't require
    the server to share any secret information with the client, it is all performed
    through the Kerberos protocol.


Mechanism plugins are generally contained in a .so file that has a name
similar to the mechanism's name.  Though, in a static compilation they
can also be a part of ``libsasl2.a``

Plugins: Auxiliary Property
---------------------------

Auxiliary Property (or auxprop) plugins provide a database service for the
glue layer (and through it, to the mechanisms and application).  Cyrus SASL
ships with two auxprop plugins: SASLdb and SQL.  Though they can be use
in much more generic ways, auxprop plugins are mostly only used by
shared secret mechanisms (or by the auxprop password verify) to access the
``userPassword`` attribute.  This provides a plaintext copy of the
password that allows for authentication to take place.

Like the mechanism plugins, these are named similarly to the databases
that they implement an interface for.

Plugins: Username Canonicalization
----------------------------------

Username Canonicalization plugins are not widely used, however it may be
useful to use as a hook if your site has specific requirements for how userids
are presented to the applications.

Password Verification Services
==============================

As described above, the password verifiers take a username and plaintext
password, and say either ``yes`` or ``no``.  It is not possible
to use them to verify hashes that might be provided by the shared secret
mechanisms.

Password verifiers are selected using the ``pwcheck_method``
SASL option.  There are two main password verifiers provided with Cyrus SASL:

auxprop
    This uses an auxprop plugin to fetch the password and then
    compares it with the client-provided copy to make the determination.
saslauthd
    This calls out to the ``saslauthd`` daemon, which
    also ships with the distribution.  The ``saslauthd`` daemon has a number
    of modules of its own, which allow it to do verification of passwords in
    a variety of ways, including PAM, LDAP, against a Kerberos database, and so on.
    This is how you would want to, for example, use the data contained in
    ``/etc/shadow`` to authenticate users.
