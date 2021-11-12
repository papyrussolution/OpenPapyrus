.. _upgrading-v1-v2:

=======================
Upgrading from v1 to v2
=======================

This document covers issues with upgrading from SASLv1 to SASLv2.
To upgrade:

*  Install Cyrus SASL v2 as normal according to the :ref:`installation guide <installation>`.
   This will overwrite
   some manpages, but will not affect your current applications.  Do NOT
   attempt to make it use the same directories, otherwise old Cyrus SASLv1
   applications will no longer function.

*  Install your new Cyrus SASL v2 applications. Applications that
   use Cyrus SASLv1 will *not* use the Cyrus SASL v2
   infrastructure (and vice-versa).

*  If you used ``/etc/sasldb`` for authentication, you'll need
   to take the following steps to convert to using ``/etc/sasldb2``
   with Cyrus SASL v2:

      1.  run ``utils/dbconverter-2`` after installation.
      2. change the ``pwcheck_method`` in any config files to
         ``auxprop``
      3. (optional) add ``auxprop_plugin`` to the config files,
         set to ``sasldb``

*  If you used ``passwd``, ``shadow``, ``pam``,
   ``kerberos_v4`` or ``sia`` as your ``pwcheck_method``
   in libsasl v1, you'll need to convert to using
   ``saslauthd``. Arrange to start ``saslauthd -a
   <method>`` on boot. Change ``pwcheck_method`` in any
   configuration files to ``saslauthd``.

*  If you used ``pwcheck`` with libsasl v1, you can either
   continue to use ``pwcheck`` with libsasl v1 or you can switch to
   ``saslauthd``, which offers more flexibility and a potentially
   much more efficient implementation.

*  If you are continuing to use some libsasl v1 applications, read
   onwards to understand the ramifications.

*  If you want to learn how to port applications from libsasl v1 to
   libsasl v2, you should read the :ref:`converting applications guide <appconvert>`.

Backwards Compatibility
=======================

Cyrus SASLv2 is incompatible with applications that use
Cyrus SASLv1.  This means that applications are unable to
simultaneously link both versions of the library, and developers are
encouraged to instead develop or upgrade their applications to link
against the new libsasl.

Likewise, the format for the sasldb database has been completely
revamped.  See :ref:`here <upgrading-db>` for a discussion of the relevant
upgrade issues related to sasldb.  All new passwords stored in the
sasldb database will be in plaintext, meaning that a compromised
sasldb will compromise all services with the same passwords.  (This
situation isn't significantly worse, cryptographically speaking, than
the old method and allows the database to be easy to transition to
another format, when the need arises.)  Mechanisms requiring a more
secure password database backend (e.g. SRP) should implement their own
or use alternate property names within sasldb.

.. _coexist:

Coexistence with SASLv1
=======================

The two library versions and the associated utilities should be able
to coexist on the same system.  The man pages will be unable to
coexist (but luckily the new manpages are much better!).  The libsasl
v2-specific utilities have had a "2" appended to their name for this
purpose (e.g. ``saslpasswd2``, ``sasldblistusers2``).  The
new-style sasldb now defaults to the name ``/etc/sasldb2``, but
this is configurable.

.. _upgrading-db:

Database Upgrades
=================

While there does not seem to be any conflict with the keys stored in
the database, it is not recommended for both versions of the library
to use the same database file.  Included in the utils directory is a
program called ``dbconverter-2`` which will allow you to convert
from the old-format database to the new format.  Note that if you continue to
run older applications that rely on Cyrus SASLv1, the databases for SASLv1
and SASLv2 will not automatically be kept in sync.


Errors on migration
===================

When migrating the ``/etc/sasldb`` database using the ``utils/dbconverter-2``
utility, you may encounter the error message "Error opening password
file". This is usually due to the fact your SASL V1 library was compiled
using a different version of Berkeley DB than the SASL V2 library.
You can work around this by using Berkeley DB's db_upgrade utility
(possibly chaining the DB3 and DB4 upgrade utilities) to upgrade a copy
of sasldb prior to conversion using dbconverter-2.

Here is the script we use at our installation, where SASL has to
coexist with SASL2::

    !/bin/sh
    cp /etc/sasldb /tmp/sasldb.$$
    /usr/local/BerkeleyDB.4/bin/db_upgrade /etc/sasldb
    echo ""|/usr/local/sasl/sbin/dbconverter-2
    cp /tmp/sasldb.$$ /etc/sasldb
