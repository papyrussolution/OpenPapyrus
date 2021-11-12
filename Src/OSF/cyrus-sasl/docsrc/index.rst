.. _sasl-index:

==========
Cyrus SASL
==========

Welcome to Cyrus SASL.

What is Cyrus SASL?
===================
Simple Authentication and Security Layer (SASL_) is a specification that describes how authentication mechanisms can be plugged into an application protocol on the wire. Cyrus SASL is an implementation of SASL that makes it easy for application developers to integrate authentication mechanisms into their application in a generic way.

The latest stable version of Cyrus SASL is |sasl_current_stable_version|.

:ref:`Cyrus IMAP <cyrusimap:imap-index>` uses Cyrus SASL to provide authentication support to the mail server, however it is just one project using Cyrus SASL.

Features
--------
Cyrus SASL provides a number of authentication plugins out of the box.

    Berkeley DB, GDBM, or NDBM (sasldb), PAM, MySQL, PostgreSQL, SQLite, LDAP, Active Directory (LDAP), DCE, Kerberos 4 and 5, proxied IMAP auth, getpwent, shadow, SIA, Courier Authdaemon, httpform, APOP and SASL mechanisms: ANONYMOUS, CRAM-MD5, DIGEST-MD5, EXTERNAL, GSSAPI, LOGIN, NTLM, OTP, PASSDSS, PLAIN, SCRAM, SRP

.. _SASL: https://en.wikipedia.org/wiki/Simple_Authentication_and_Security_Layer

This document is an introduction to **Cyrus SASL**. It is not intended to be an exhaustive reference for the SASL Application Programming Interface (API), which is detailed in the SASL manual pages, and the libsasl.h header file.


Known Bugs
----------

.. todo:
    Is this still true?

``libtool`` doesn't always link libraries together.  In our environment,
we only have static Krb5 libraries; the GSSAPI plugin should link
these libraries in on platforms that support it (Solaris and Linux
among them) but it does not.  It also doesn't always get the runpath
of libraries correct.

.. toctree::
    :maxdepth: 3
    :caption: Cyrus SASL

    download
    sasl/quickstart
    sasl/concepts
    setup
    operations
    developer
    support

.. OLD INDEX

    .. toctree::
        :maxdepth: 1
        :caption: Overview

        getsasl
        sasl/quickstart
        sasl/installation
        Contact Us <contribute>

    .. toctree::
        :maxdepth: 1
        :caption: Configuration

        sasl/concepts
        sasl/components
        sasl/options
        sasl/sysadmin
        sasl/advanced
        sasl/upgrading
        sasl/appconvert

    .. toctree::
        :maxdepth: 1
        :caption: Developers

        sasl/developer/programming
        sasl/developer/plugprog
        sasl/developer/testing

    .. toctree::
        :maxdepth: 1
        :caption: Reference

        sasl/auxiliary_properties
        sasl/authentication_mechanisms
        sasl/pwcheck
        sasl/faq
        sasl/resources
        sasl/manpages

.. toctree::
    :caption: IMAP

    Cyrus IMAP <http://www.cyrusimap.org>
